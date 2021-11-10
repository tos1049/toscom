/*
 *****************************************************************************
 *
 * 共通処理セレクト機能    by TOS
 *
 *   外部公開I/Fの使用方法については com_select.h を必読。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"
#include "com_select.h"

#ifdef LINUXOS
#include <linux/rtnetlink.h>
#include <net/if_arp.h>
#endif

// イベント情報生成取得処理 --------------------------------------------------

// 内部イベント情報構造体
typedef struct {
    BOOL isUse;                       // 使用フラグ
    BOOL isListen;                    // listenソケットフラグ
    long timer;                       // タイマー値
    com_expireTimerCB_t expireFunc;   // タイマー満了処理関数
    struct timeval startTime;         // タイマー開始時刻
    BOOL isStopped;                   // タイマー停止中か
    int sockId;                       // ソケットID
    COM_SOCK_TYPE_t type;             // 通信種別
    BOOL isPacket;                    // raw時パケット用か(false=IP用)
    BOOL isUnix;                      // UNIXドメインソケットか
    com_sockEventCB_t eventFunc;      // イベント処理関数
    com_getStdinCB_t stdinFunc;       // 標準入力受付関数
    com_sockFilterCB_t filterFunc;    // 受信フィルタリング関数
    com_sockaddr_t srcInf;            // 自側アドレス情報
    com_sockaddr_t dstInf;            // 対抗アドレス情報
} com_eventInf_t;

static pthread_mutex_t  gMutexEvent = PTHREAD_MUTEX_INITIALIZER;
static com_selectId_t   gEventId = 0;       // 現在のイベント登録数
static com_eventInf_t*  gEventInf = NULL;   // イベント情報実体

enum {
    NO_EVENTS = -1,
    FD_ERROR = -1,
    ID_STDIN = 0
};

// 排他アンロックしてから復帰するマクロ
#define UNLOCKRETURN( RETURN ) \
    do { \
        com_mutexUnlock( &gMutexEvent, __func__ ); \
        return (RETURN); \
    } while(0)

static BOOL checkAvailInf( com_eventInf_t *iInf, BOOL iIsTimer )
{
    if( iInf->isUse ) {return false;}
    // タイマーイベント用なら、前回タイマーイベントの空き情報は使わない
    if( iIsTimer ) {if( iInf->expireFunc ) {return false;}}
    // ソケットイベント用なら、前回ソケットイベントの空き情報は使わない
    else {if( !iInf->expireFunc ) {return false;}}
    return true;
}

static com_selectId_t selectEventId( BOOL iIsTimer )
{
    // 未使用領域がある時は、それを利用する
    com_mutexLock( &gMutexEvent, __func__ );
    com_selectId_t  id = 0;  // for文外でも使うため、for文外で定義
    for( ;  id < gEventId;  id++ ) {
        if( checkAvailInf( &(gEventInf[id]), iIsTimer ) ) {break;}
    }
    return id;
}

static com_selectId_t getEventId( BOOL iIsTimer )
{
    // 未使用領域がない時は、末尾に情報を追加
    com_selectId_t  id = selectEventId( iIsTimer );
    if( id == gEventId ) {
        BOOL  result = com_realloct( &gEventInf, sizeof(*gEventInf), &gEventId,
                                     1, "new event inf(%ld)", gEventId );
        if( !result ) {UNLOCKRETURN( COM_NO_SOCK );}
    }
    // 情報初期化
    gEventInf[id] = (com_eventInf_t){
        .isUse = false,  .timer = COM_NO_SOCK,  .expireFunc = NULL,
        .sockId = COM_NO_SOCK,  .eventFunc = NULL,  .stdinFunc = NULL,
        .filterFunc = NULL
    };
    UNLOCKRETURN( id );
}

// デバッグ出力用イベント一覧
typedef enum {
    COM_MOD_SOCKET,             // socket()実施
    COM_MOD_BIND,               // bind()実施
    COM_MOD_LISTEN,             // listen()実施
    COM_MOD_CONNECT,            // connect()実施
    COM_MOD_ACCEPT,             // accept()実施
    COM_MOD_SEND,               // データ送信
    COM_MOD_RECEIVE,            // データ受信
    COM_MOD_DROP,               // データ受信がフィルタリングにより破棄
    COM_MOD_CLOSE,              // 自分で close()実施
    COM_MOD_CLOSED,             // 対向からのTCPクローズ
    COM_MOD_STDINON,            // 標準入力受付登録
    COM_MOD_STDIN,              // 標準入力受付
    COM_MOD_STDINOFF,           // 標準入力受付解除
    COM_MOD_TIMERON,            // タイマー起動
    COM_MOD_EXPIRED,            // タイマー満了
    COM_MOD_TIMERMOD,           // タイマー修正
    COM_MOD_TIMERSTOP,          // タイマー停止
    COM_MOD_TIMEROFF,           // タイマー解除
    COM_MOD_NOEVENT     // 最後は必ずこれで
} COM_MOD_EVENT_t;

static char* gModEvent[] = {
    "socket", "bind", "listen", "connect", "accept",
    "send", "receive", "drop", "close_self", "closed_by_dst",
    "stdin_on", "stdin", "stdin_off",
    "timer_on", "timer_expired", "timer_reset", "timer_stop", "timer_off"
};

static BOOL  gDebugEvent = true;

void com_setDebugSelect( BOOL iMode )
{
    gDebugEvent = iMode;
}

static void printAddr( com_sockaddr_t *iInf, const char *iLabel )
{
    char*  addr;
    char*  port;
    if( com_getnameinfo( &addr, &port, &iInf->addr, iInf->len ) ) {
        com_dbgCom( "      %s = [%s]:%s", iLabel, addr, port );
    }
}

static void printSignal(
        com_sockaddr_t *iFrom, com_sockaddr_t *iTo,
        const uchar *iData, size_t iSize )
{
    printAddr( iFrom, "from" );
    printAddr( iTo,   "  to" );
    com_dumpCom( iData, iSize, "      size = %zu", iSize );
}

static void printStartTime( struct timeval *iTime )
{
    char  startDate[COM_DATE_DSIZE];
    char  startTime[COM_TIME_DSIZE];
    com_setTimeval( COM_FORM_DETAIL, startDate, startTime, NULL, iTime );
    com_dbgCom( "      start = %s %s", startDate, startTime );
}

static void logStdin( const void *iData, size_t iSize )
{
    com_dbgCom( "      length = %zu", iSize );
    com_dbgCom( "      %s", (const char*)iData );
}

static char *getAddLabel( COM_MOD_EVENT_t iModEvent, com_eventInf_t *iInf )
{
    COM_UNUSED( iModEvent );  // 今後使うかもしれないので仮引数はキープ
    if( iInf->type > COM_SOCK_RAWRCV ) {
        return ( iInf->isUnix ? "UNIX" : NULL );
    }
    return ( iInf->isPacket ? "Packet" : "IP" );  // rawソケットのみ返す
}

static void logSocket(
        COM_MOD_EVENT_t iModEvent, com_eventInf_t *iInf,
        const uchar *iData, size_t iSize )
{
    char*  prot[] = {
        "", "rawSnd", "rawRcv", "Netlink", "UDP", "TCP_CLIENT", "TCP_SERVER"
    };
    com_dbgCom( "      sockId = %d", iInf->sockId );
    char*  addLabel = getAddLabel( iModEvent, iInf );
    if( !addLabel ) {com_dbgCom( "        type = %s", prot[iInf->type] );}
    else {com_dbgCom( "        type = %s (%s)", prot[iInf->type], addLabel );}
    if( iModEvent == COM_MOD_SEND ) {
        printSignal( &iInf->srcInf, &iInf->dstInf, iData, iSize );
    }
    else if( iModEvent == COM_MOD_RECEIVE || iModEvent == COM_MOD_DROP ) {
        printSignal( &iInf->dstInf, &iInf->srcInf, iData, iSize );
    }
    else {
        printAddr( &iInf->srcInf, "src" );
        if( iInf->type > COM_SOCK_UDP ) {printAddr( &iInf->dstInf, "dst" );}
    }
}

static void logTimer( com_eventInf_t *iInf )
{
    com_dbgCom( "      timer = %ld msec", iInf->timer );
    printStartTime( &iInf->startTime );
}

static void debugEventLog( 
        COM_MOD_EVENT_t iModEvent, com_selectId_t iId, com_eventInf_t *iInf,
        const uchar *iData, size_t iSize )
{
    if( !com_getDebugPrint() || !gDebugEvent ) {return;}
    com_dbgCom( "==SELECT %s (ID=%ld)", gModEvent[iModEvent], iId );
    if( iModEvent == COM_MOD_STDIN ) {logStdin( iData, iSize );}
    else if( iModEvent <= COM_MOD_CLOSED ) {
        logSocket( iModEvent, iInf, iData, iSize );
    }
    else if( iModEvent >= COM_MOD_TIMERON ) {logTimer( iInf );}
}



// ソケット関連処理 ----------------------------------------------------------

static void setSockCondNormal(
        const struct addrinfo *iSrc,
        int *oFamily, int *oType, int *oProtocol )
{
    *oFamily = iSrc->ai_family;
    *oType = iSrc->ai_socktype;
    *oProtocol = iSrc->ai_protocol;
}

#ifdef LINUXOS
static BOOL setSockCondRawSnd(
        const struct addrinfo *iSrc,
        int *oFamily, int *oType, int *oProtocol )
{
    *oType = SOCK_RAW;
    if( iSrc->ai_family == AF_INET ) {
        *oFamily = iSrc->ai_family;
        *oProtocol = IPPROTO_RAW;
        return false;
    }
    if( iSrc->ai_family == AF_INET6 ) {
        *oFamily = AF_PACKET;
        *oProtocol = IPPROTO_RAW;
        return true;
    }
    *oFamily = AF_PACKET;
    *oProtocol = htons( iSrc->ai_family );
    return true;
}

static BOOL setSockCondRawRcv(
        const struct sockaddr_ll *iSrc,
        int *oFamily, int *oType, int *oProtocol )
{
    *oFamily = AF_PACKET;
    *oProtocol = iSrc->sll_protocol;
    if( *oProtocol == htons(ETH_P_IP) || *oProtocol == htons(ETH_P_IPV6) ) {
        *oType = SOCK_DGRAM;
        return false;
    }
    *oType = SOCK_RAW;
    return true;
}

static void setSockCondNetlink(
        int *iSrc, int *oFamily, int *oType, int *oProtocol )
{
    *oFamily = AF_NETLINK;
    *oType = SOCK_RAW;
    if( !iSrc ) {*oProtocol = NETLINK_ROUTE;}
    else {*oProtocol = *iSrc;}
}
#endif // LINUXOS

static void setSockCondUnix(
        COM_SOCK_TYPE_t iType, const struct sockaddr_un *iSun,
        int *oFamily, int *oType, int *oProtocol )
{
    *oFamily = AF_UNIX;
    *oType = (iType == COM_SOCK_UDP) ? SOCK_DGRAM : SOCK_STREAM;
    *oProtocol = 0;
    if( com_checkExistFile( iSun->sun_path ) ) {remove( iSun->sun_path );}
}

static BOOL isUnixSocket( const void *iSrc )
{
    if( !iSrc ) {return false;}
    const struct sockaddr_un*  sun = iSrc;
    return (sun->sun_family == AF_UNIX);
}

static void setSocketCondition(
        COM_SOCK_TYPE_t iType, const struct addrinfo *iSrc,
        int *oFamily, int *oType, int *oProtocol, BOOL *oIsPkt, BOOL *oIsUnix )
{
#ifdef LINUXOS
    if( iType == COM_SOCK_RAWSND ) {
        *oIsPkt = setSockCondRawSnd( iSrc, oFamily, oType, oProtocol );
        return;
    }
    else if( iType == COM_SOCK_RAWRCV ) {
        *oIsPkt = setSockCondRawRcv( (void*)iSrc, oFamily, oType, oProtocol );
        return;
    }
    else if( iType == COM_SOCK_NETLINK ) {
        setSockCondNetlink( (void*)iSrc, oFamily, oType, oProtocol );
        return;
    }
#else  // LINUXOS
    COM_UNUSED( oIsPkt );
#endif // LINUXOS
    if( !isUnixSocket( iSrc ) ) {  // 通常のUDP/TCPソケット生成
        setSockCondNormal( iSrc, oFamily, oType, oProtocol );
        return;
    }
    else { // UNIXドメインソケット生成
        setSockCondUnix( iType, (void*)iSrc, oFamily, oType, oProtocol );
        *oIsUnix = true;
    }
}

static BOOL setUserOpt( int iSockId, const com_sockopt_t *iOpt )
{
    BOOL  result = true;
    for( ;  iOpt->level >= 0;  iOpt++ ) {
        if( 0 > setsockopt( iSockId, iOpt->level, iOpt->optname,
                            iOpt->optval, (socklen_t)(iOpt->optlen) ) )
        {
            com_error( COM_ERR_SOCKETNG,
                       "fail to set option(%d:%d) [%s]",
                       iOpt->level, iOpt->optname, com_strerror(errno) );
            result = false;
        }
    }
    return result;
}

static int getSndBufSize( void )
{
#ifndef COM_IPRAW_SNDBUF_SPEC
    return COM_IPRAW_SNDBUF_SIZE;
#else
    return COM_IPRAW_SNDBUF_SPEC;
#endif
}

static BOOL sockoptNG( const char *iOptName )
{
    com_error( COM_ERR_SOCKETNG, "fail to set %s", iOptName );
    return false;
}

static const int  YES = 1;

#define SETSOOPT( OPT ) \
    setsockopt( iSockId, proto, (OPT), &YES, sizeof(YES) )

static BOOL setRawsndOpt( int iSockId, const struct addrinfo *iSrc )
{
    if( iSrc->ai_family == AF_INET ) {
        int  proto = IPPROTO_IP;
        if( 0 > SETSOOPT( IP_HDRINCL ) ) {return sockoptNG( "IP_HDRINCL" );}
    }
    const int  buf = getSndBufSize();
    if( 0 > setsockopt( iSockId, SOL_SOCKET, SO_SNDBUF, &buf, sizeof(buf) ) ) {
        return sockoptNG( "SO_SNDBUF" );
    }
    return true;
}

static BOOL setSocketOptions(
        COM_SOCK_TYPE_t iType, int iSockId, const struct addrinfo *iSrc,
        const com_sockopt_t *iOpt )
{
    if( iOpt ) {return setUserOpt( iSockId, iOpt );}
    if( iType == COM_SOCK_RAWRCV ) {return true;}  // オプション設定なし
    if( iType == COM_SOCK_RAWSND ) {return setRawsndOpt( iSockId, iSrc );}
    if( iType == COM_SOCK_NETLINK || isUnixSocket( iSrc ) ) {return true;}
    int  proto = (iSrc->ai_family == AF_INET) ? IPPROTO_IP : IPPROTO_IPV6;
    // UDP/TCPのIP通信ソケットの設定開始
    if( proto == IPPROTO_IPV6 ) {
        if( 0 > SETSOOPT( IPV6_V6ONLY ) ) {return sockoptNG( "IPV6_V6ONLY" );}
    }
    proto = SOL_SOCKET;
    if( 0 > SETSOOPT( SO_REUSEADDR ) ) {return sockoptNG( "SO_REUSEADDR" );}
    return true;
}

static BOOL createSocket(
        COM_SOCK_TYPE_t iType, com_selectId_t iId, com_eventInf_t *oInf,
        const void *iSrc, const com_sockopt_t *iOpt )
{
    int  family, type, protocol;
    setSocketCondition( iType, iSrc, &family, &type, &protocol,
                        &oInf->isPacket, &oInf->isUnix );
    oInf->sockId = socket( family, type, protocol );
    if( oInf->sockId < 0 ) {
        com_error( COM_ERR_SOCKETNG,
                   "fail to create socket[%s]", com_strerror( errno ) );
        return false;
    }
    if( !setSocketOptions( iType, oInf->sockId, iSrc, iOpt ) ) {
        // エラーメッセージはオプション指定NG時に出力済み
        close( oInf->sockId );
        return false;
    }
    debugEventLog( COM_MOD_SOCKET, iId, oInf, NULL, 0 );
    return true;
}

static BOOL execBind(
        com_selectId_t iId, com_eventInf_t *oInf,
        const void *iAddr, socklen_t iLen )
{
    int  ret = bind( oInf->sockId, iAddr, iLen );
    if( ret < 0 ) {
        com_error( COM_ERR_BINDNG,
                   "fail to bind socket[%s]", com_strerror( errno ) );
        return false;
    }
    debugEventLog( COM_MOD_BIND, iId, oInf, NULL, 0 );
    return true;
}

#ifdef LINUXOS
static BOOL bindPacketRaw(
        com_selectId_t iId, com_eventInf_t *oInf,
        const struct sockaddr_ll *iSrc )
{
    if( iSrc->sll_ifindex < 0 ) {return true;}
    struct sockaddr_ll  sll;
    memcpy( &sll, iSrc, sizeof(sll) );
    sll.sll_family = AF_PACKET;
    return execBind( iId, oInf, &sll, sizeof(sll) );
}
#endif // LINUXOS

static BOOL bindUnixDomain(
        com_selectId_t iId, com_eventInf_t *oInf,
        const struct sockaddr_un *iSrc )
{
    com_copyAddr( &(oInf->srcInf), (const void*)iSrc );
    return execBind( iId, oInf, iSrc, SUN_LEN( iSrc ) );
}

static BOOL makeBind(
        COM_SOCK_TYPE_t iType, com_selectId_t iId, com_eventInf_t *oInf,
        const void *iSrc )
{
#ifdef LINUXOS
    if( iType == COM_SOCK_RAWSND || iType == COM_SOCK_NETLINK ) {return true;}
    if( iType == COM_SOCK_RAWRCV ) {return bindPacketRaw( iId, oInf, iSrc );}
#else
    COM_UNUSED( iType );
#endif
    if( oInf->isUnix ) {return bindUnixDomain( iId, oInf, iSrc );}
    // UDP/TCPソケットへのアドレスバインド
    const struct addrinfo*  src = iSrc;
    com_copyAddr( &(oInf->srcInf), src );
    return execBind( iId, oInf, src->ai_addr, src->ai_addrlen );
}

static BOOL startTcpClient(
        com_eventInf_t *oInf, const struct addrinfo *iDst )
{
    const void*  addr = NULL;
    socklen_t  len = 0;
    if( !oInf->isUnix ) {addr = iDst->ai_addr;  len = iDst->ai_addrlen;}
    else {addr = iDst;  len = SUN_LEN((const struct sockaddr_un*)iDst);}
    if( 0 > connect( oInf->sockId, addr, len ) ) {
        com_error( COM_ERR_CONNECTNG,
                   "fail to connect tcp client[%s]", com_strerror(errno) );
        return false;
    }
    if( !oInf->isUnix ) {com_copyAddr( &(oInf->dstInf), iDst );}
    return true;
}

static BOOL startTcpServer( com_eventInf_t *oInf )
{
    int  ret = listen( oInf->sockId, 128 );
    if( ret < 0 ) {
        com_error( COM_ERR_LISTENNG,
                   "fail to listen as tcp server[%s]", com_strerror(errno) );
        return false;
    }
    oInf->isListen = true;
    return true;
}

static int closeSocket( com_eventInf_t *oInf )
{
    oInf->isUse = false;
    if( oInf->isUnix ) {
        struct sockaddr_un*  sun = (void*)(&oInf->srcInf.addr);
        remove( sun->sun_path );
    }
    return close( oInf->sockId );
}

static com_selectId_t closeSocketEnd( com_eventInf_t *oInf )
{
    (void)closeSocket( oInf );  // close()に失敗しても気にしない
    UNLOCKRETURN( COM_NO_SOCK );
}

static com_selectId_t readyTransport(
        COM_SOCK_TYPE_t iType, com_selectId_t iId, com_eventInf_t *oInf,
        const void *iDstInf )
{
    BOOL  result = true;
    COM_MOD_EVENT_t  event = COM_MOD_NOEVENT;
    switch( iType ) {
        case COM_SOCK_TCP_CLIENT:   // サーバーにつなぎに行く
            result = startTcpClient( oInf, iDstInf );
            event = COM_MOD_CONNECT;
            break;
        case COM_SOCK_TCP_SERVER:   // クライアントからの接続を待つ
            result = startTcpServer( oInf );
            event = COM_MOD_LISTEN;
            break;
        default:  break;            // TCP以外は処理なし
    }
    if( !result ) {return COM_NO_SOCK;}
    // TCP接続のイベントログは接続成功時しか出さないようにする
    if( iType > COM_SOCK_UDP ) {debugEventLog( event, iId, oInf, NULL, 0 );}
    return iId;
}

// イベント関数NULL指定時のダミー関数
static BOOL dummyEventFunc(
        com_selectId_t iId, COM_SOCK_EVENT_t iEvent,
        void *iData, size_t iDataSize )
{
    if( iEvent == COM_EVENT_ACCEPT ) {com_dbgFuncCom( "ACCEPT(%ld)", iId );}
    if( iEvent == COM_EVENT_CLOSE )  {com_dbgFuncCom( "CLOSE(%ld)",  iId );}
    if( iEvent == COM_EVENT_RECEIVE ) {
        com_dumpCom( iData, (size_t)iDataSize, "RECEIVE(%ld)", iId );
    }
    return true;
}

static BOOL checkCreatePrm(
        COM_SOCK_TYPE_t iType, const void *iSrcInf, const void *iDstInf )
{
#ifndef LINUXOS
    if( iType <= COM_SOCK_NETLINK ) {return false;}
#endif
    if( !iSrcInf && iType != COM_SOCK_NETLINK ) {return false;}
    if( !iDstInf && iType == COM_SOCK_TCP_CLIENT ) {return false;}
    return true;
}

com_selectId_t com_createSocket(
        COM_SOCK_TYPE_t iType, const void *iSrcInf,
        const com_sockopt_t *iOpt, com_sockEventCB_t iEventFunc,
        const void *iDstInf )
{
    if( !checkCreatePrm( iType, iSrcInf, iDstInf ) ) {COM_PRMNG(COM_NO_SOCK);}
    com_selectId_t  id = selectEventId( false );  // 仮ID取得
    if( !iEventFunc ) {iEventFunc = dummyEventFunc;}
    com_eventInf_t  inf = { .isUse = true,  .timer = COM_NO_SOCK,
                            .eventFunc = iEventFunc, .type = iType };
    com_mutexUnlock( &gMutexEvent, __func__ );
    if( !createSocket( iType, id, &inf, iSrcInf, iOpt ) ) {return COM_NO_SOCK;}
    if( !makeBind( iType, id, &inf, iSrcInf ) ) {return closeSocketEnd( &inf );}
    if( COM_NO_SOCK == readyTransport( iType, id, &inf, iDstInf ) ) {
        return closeSocketEnd( &inf );
    }
    com_skipMemInfo( true );
    id = getEventId( false );
    com_skipMemInfo( false );
    if( id == COM_NO_SOCK ) {return closeSocketEnd( &inf );}
    gEventInf[id] = inf;
    UNLOCKRETURN( id );
}

static com_eventInf_t *checkSocketInf( com_selectId_t iId, BOOL iLock )
{
    // returnSocketInf()で排他アンロックする想定
    if( iLock ) {com_mutexLock( &gMutexEvent, __func__ );}
    if( iId < 0 || iId > gEventId ) {return NULL;}
    com_eventInf_t*  tmp = &(gEventInf[iId]);
    if( !tmp->isUse || tmp->sockId == COM_NO_SOCK ) {return NULL;}
    return tmp;
}

BOOL com_deleteSocket( com_selectId_t iId )
{
    com_eventInf_t*  tmp = checkSocketInf( iId, true );
    if( !tmp ) {com_prmNG(NULL);  UNLOCKRETURN( false );}
    if( tmp->sockId == ID_STDIN ) {UNLOCKRETURN( false );}
    if( 0 > closeSocket( tmp ) ) {
        com_error( COM_ERR_CLOSENG,
                   "fail to close socket[%s]", com_strerror(errno) );
        UNLOCKRETURN( false );
    }
    // ユーザーからの切断要求なので、TCPでも切断通知は出さない
    debugEventLog( COM_MOD_CLOSE, iId, tmp, NULL, 0 );
    UNLOCKRETURN( true );
}

BOOL com_sendSocket(
        com_selectId_t iId, const void *iData, size_t iDataSize,
        const com_sockaddr_t *iDst )
{
    int  ret = 0;
    com_eventInf_t* tmp = checkSocketInf( iId, false );
    if( !tmp || !iData ) {COM_PRMNG(false);}
    if( tmp->type == COM_SOCK_RAWRCV ) {COM_PRMNG(false);}
    if( tmp->type == COM_SOCK_UDP ||
        (tmp->type == COM_SOCK_RAWSND && !tmp->isPacket) )
    {
        ret = sendto( tmp->sockId, iData, iDataSize, 0,
                      (const struct sockaddr*)(&iDst->addr), iDst->len );
        tmp->dstInf = *iDst;
    }
    else {ret = send( tmp->sockId, iData, iDataSize, 0 );}
    if( ret < 0 ) {
        com_error( COM_ERR_SENDNG,
                   "fail to send packet by %ld [%s]", iId, com_strerror(errno));
        return false;
    }
    debugEventLog( COM_MOD_SEND, iId, tmp, iData, iDataSize );
    return true;
}

// 計算ロジック自体は伝統的なものをそのまま使用
ushort com_cksumRfc( void *iBin, size_t iBinSize )
{
    size_t   nleft = iBinSize;
    int      sum = 0;
    ushort*  w = iBin;
    ushort   answer = 0;

    while( nleft > 1 ) {
        sum += *w++;
        nleft -= 2;
    }
    if( nleft == 1 ) {
        *(uchar *)(&answer) = *(uchar*)w;
        sum += answer;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    if( 0xffff == (answer = ~sum) ) {return 0;}
    return answer;
}

static BOOL checkNoRecv( BOOL iNonBlock, int iErrno )
{
    if( !iNonBlock ) {return false;}
    if( iErrno == EAGAIN ) {return true;}
    // EAGAIN と EWOULDBLOCK が同値宣言されている場合を考慮
    if( EAGAIN != EWOULDBLOCK ) {if(iErrno == EWOULDBLOCK) {return true;}}
    return false;
}

static COM_RECV_RESULT_t checkRecvResult(
        com_selectId_t iId, ssize_t iLen, int iErrno, com_eventInf_t *iInf,
        BOOL iNonBlock, void *iData )
{
    if( iLen ==  0  && iInf->type > COM_SOCK_UDP ) {
        debugEventLog( COM_MOD_CLOSED, iId, iInf, NULL, 0 );
        return COM_RECV_CLOSE;
    }
    if( iLen < 0 ) {
        if( iInf->isListen ) {return COM_RECV_ACCEPT;}
        if( checkNoRecv( iNonBlock, iErrno ) ) {return COM_RECV_NODATA;}
        com_error( COM_ERR_RECVNG, "fail to receive packet [%s]",
                   com_strerror( iErrno ) );
        return COM_RECV_NG;
    }
    if( iInf->filterFunc ) {
        if( (iInf->filterFunc)( iId, iData, iLen ) ) {
            debugEventLog( COM_MOD_DROP, iId, iInf, iData, (size_t)iLen );
            return COM_RECV_DROP;
        }
    }
    debugEventLog( COM_MOD_RECEIVE, iId, iInf, iData, (size_t)iLen );
    return COM_RECV_OK;
}

COM_RECV_RESULT_t com_receiveSocket(
        com_selectId_t iId, void *oData, size_t iDataSize,
        BOOL iNonBlock, ssize_t *oLen, com_sockaddr_t *oFrom )
{
    COM_SET_IF_EXIST( oLen, 0 );
    com_eventInf_t*  tmp = checkSocketInf( iId, false );
    if( !tmp || !oData || !oLen ) {COM_PRMNG(COM_RECV_NG);}
    struct sockaddr*  srcAddr = NULL;
    socklen_t*  addrLen = NULL;
    if( oFrom ) {
        memset( oFrom, 0, sizeof(*oFrom) );
        srcAddr = (struct sockaddr*)(&oFrom->addr);
        oFrom->len = sizeof(oFrom->addr);
        addrLen = &(oFrom->len);
    }
    errno = 0;
    if( tmp->isUnix ) {*oLen = read( tmp->sockId, oData, iDataSize );}
    else {
        int  flags = iNonBlock ? MSG_DONTWAIT : 0;
        *oLen = recvfrom( tmp->sockId, oData, iDataSize, flags,
                          srcAddr, addrLen );
    }
    return checkRecvResult( iId, *oLen, errno, tmp, iNonBlock, oData );
}

BOOL com_filterSocket( com_selectId_t iId, com_sockFilterCB_t iFilterFunc )
{
    com_eventInf_t*  tmp = checkSocketInf( iId, false );
    if( !tmp ) {COM_PRMNG(false);}
    tmp->filterFunc = iFilterFunc;
    return true;
}

int com_getSockId( com_selectId_t iId )
{
    com_eventInf_t*  tmp = checkSocketInf( iId, true );
    if( !tmp ) {com_prmNG(NULL);  UNLOCKRETURN( false );}
    // タイマーならNG返却
    if( tmp->expireFunc ) {UNLOCKRETURN( COM_NO_SOCK );}
    // 標準入力なら 0返却
    if( tmp->stdinFunc ) {UNLOCKRETURN( 0 );}
    // ソケットなので ID返却
    UNLOCKRETURN( tmp->sockId );
}

static com_sockaddr_t *getAddrInf( com_selectId_t iId, BOOL iIsSrc )
{
    com_eventInf_t*  tmp = checkSocketInf( iId, true );
    if( !tmp ) {com_prmNG(NULL);  UNLOCKRETURN( NULL );}
    if( iIsSrc ) {UNLOCKRETURN( &tmp->srcInf );}
    UNLOCKRETURN( &tmp->dstInf );
}

com_sockaddr_t *com_getSrcInf( com_selectId_t iId )
{
    return getAddrInf( iId, true );
}

com_sockaddr_t *com_getDstInf( com_selectId_t iId )
{
    return getAddrInf( iId, false );
}



// タイマー関連処理 ----------------------------------------------------------

com_selectId_t com_registerTimer(
        long iTimer, com_expireTimerCB_t iExpireFunc )
{
    if( iTimer < 0  || !iExpireFunc ) {COM_PRMNG(COM_NO_SOCK);}
    com_skipMemInfo( true );
    com_selectId_t  id = getEventId( true );
    com_skipMemInfo( false );
    if( id == COM_NO_SOCK ) {UNLOCKRETURN( COM_NO_SOCK );}
    com_eventInf_t*  inf = &(gEventInf[id]);
    *inf = (com_eventInf_t){
        .isUse = true,  .timer = iTimer,  .expireFunc = iExpireFunc,
        .isStopped = false,  .sockId = COM_NO_SOCK
    };
    if( !com_gettimeofday( &(inf->startTime), "start timer" ) ) {
        inf->isUse = false;
        UNLOCKRETURN( COM_NO_SOCK );
    }
    debugEventLog( COM_MOD_TIMERON, id, inf, NULL, 0 );
    UNLOCKRETURN( id );
}

static com_eventInf_t *checkTimerInf( com_selectId_t iId, BOOL iLock )
{
    if( iId < 0 || iId > gEventId ) { return NULL; }
    if( iLock ) { com_mutexLock( &gMutexEvent, __func__ ); }
    com_eventInf_t*  tmp = &(gEventInf[iId]);
    if( tmp->timer == COM_NO_SOCK ) {return NULL;}
    if( !tmp->isUse ) {return NULL;}
    return tmp;
}

BOOL com_stopTimer( com_selectId_t iId )
{
    com_eventInf_t*  tmp = checkTimerInf( iId, true );
    if( !tmp ) {com_prmNG(NULL); UNLOCKRETURN(false);}
    tmp->isStopped = true;
    debugEventLog( COM_MOD_TIMERSTOP, iId, tmp, NULL, 0 );
    UNLOCKRETURN( true );
}

BOOL com_cancelTimer( com_selectId_t iId )
{
    com_eventInf_t*  tmp = checkTimerInf( iId, true );
    if( !tmp ) {com_prmNG(NULL); UNLOCKRETURN(false);}
    tmp->isUse = false;
    debugEventLog( COM_MOD_TIMEROFF, iId, tmp, NULL, 0 );
    UNLOCKRETURN( true );
}

static BOOL expireTimer( com_selectId_t iId )
{
    com_eventInf_t*  inf = &(gEventInf[iId]);
    debugEventLog( COM_MOD_EXPIRED, iId, inf, NULL, 0 );
    inf->isStopped = true;
    return (inf->expireFunc)( iId );
}

#define TIMEUNIT (1000000L)

static long calcMsec( struct timeval *iTime )
{
    if( !iTime ) {return 0;}
    return (TIMEUNIT * iTime->tv_sec + iTime->tv_usec);
}

static long calculateRestTime( com_selectId_t iId, struct timeval *iNow )
{
    com_eventInf_t*  tmp = &(gEventInf[iId]);
    long  pastTime = calcMsec( iNow ) - calcMsec( &(tmp->startTime) );
    return (1000L * tmp->timer - pastTime);
}

static BOOL checkTimer( com_selectId_t iId, struct timeval *iNow )
{
    com_eventInf_t*  tmp = checkTimerInf( iId, false );
    if( !tmp ) {return true;}
    if( tmp->isStopped ) {return true;}
    if( 0 < calculateRestTime( iId, iNow ) ) {return true;}
    return expireTimer( iId );
}

static struct timeval *getCurrentTime( struct timeval *oNow )
{
    if( !com_gettimeofday( oNow, "time for events" ) ) {return NULL;}
    return oNow;
}

#define GET_CURRENT \
    struct timeval   tmpTime; \
    struct timeval*  current = getCurrentTime( &tmpTime ); \
    do{} while(0)

BOOL com_checkTimer( com_selectId_t iId )
{
    BOOL  result = true;
    GET_CURRENT;
    if( !current ) {return false;}
    for( com_selectId_t id = 0;  id < gEventId;  id++ ) {
        if( iId == COM_ALL_TIMER || iId == id ) {
            if( !checkTimer( id, current ) ) {result = false;}
        }
    }
    return result;
}

BOOL com_resetTimer( com_selectId_t iId, long iTimer )
{
    com_eventInf_t*  tmp = checkTimerInf( iId, true );
    if( !tmp || iTimer < 0 ) {com_prmNG(NULL);  UNLOCKRETURN(false); }
    if( iTimer > 0 ) {tmp->timer = iTimer;}
    tmp->isStopped = false;
    if( !com_gettimeofday( &(tmp->startTime), "reset timer time" ) ) {
        tmp->isStopped = true;
    }
    if( !tmp->isStopped ) {debugEventLog( COM_MOD_TIMERMOD,iId,tmp,NULL,0 );}
    UNLOCKRETURN( !tmp->isStopped );
}



// イベント待受処理 ----------------------------------------------------------

static void addFdList(
        int iFd, fd_set *oFds, int *oMax, int *oFdList, int *ioCount )
{
    FD_SET( iFd, oFds );
    if( *oMax < iFd ) {*oMax = iFd;}
    oFdList[*ioCount] = iFd;
    (*ioCount)++;
}

static int createFdList( fd_set *oFds, int *oMax, int *oFdList )
{
    for( com_selectId_t id = 0;  id < gEventId;  id++ ) {
        oFdList[id] = NO_EVENTS;
    }
    FD_ZERO( oFds );
    int  count = 0;
    for( com_selectId_t id = 0;  id < gEventId;  id++ ) {
        com_eventInf_t* tmp = &(gEventInf[id]);
        if( !(tmp->isUse) ) {continue;}
        if( tmp->sockId == COM_NO_SOCK ) {continue;}
        addFdList( tmp->sockId, oFds, oMax, oFdList, &count );
    }
    return count;
}

static com_selectId_t  gWaitingId = COM_NO_SOCK;

static void getTimeValue(
        long *oTimer, struct timeval *iNow, com_selectId_t iId )
{
    long  restTime = calculateRestTime( iId, iNow );
    if( restTime <= 0 ) {restTime = 0;}
    if( gWaitingId == COM_NO_SOCK || restTime < *oTimer ) {
        gWaitingId = iId;
        *oTimer = restTime;
    }
}

static struct timeval *checkNextTimer( struct timeval *iTm )
{
    gWaitingId = COM_NO_SOCK;
    struct timeval  now;
    if( !com_gettimeofday( &now, "time for next timer" ) ) {return NULL;}

    long  count = 0;
    long  timer = 0;
    for( com_selectId_t id = 0;  id < gEventId;  id++ ) {
        com_eventInf_t*  tmp = &(gEventInf[id]);
        if( !(tmp->isUse) ) {continue;}
        if( tmp->timer == COM_NO_SOCK || tmp->isStopped ) {continue;}
        getTimeValue( &timer, &now, id );
        count++;
    }
    if( !count ) {return NULL;}
    *iTm = (struct timeval){ timer / TIMEUNIT, timer & TIMEUNIT };
    return iTm;
}

static com_selectId_t searchId( int iFd )
{
    for( com_selectId_t id = 0;  id < gEventId;  id++ ) {
        com_eventInf_t*  tmp = &(gEventInf[id]);
        if( tmp->isUse && tmp->sockId == iFd ) {return id;}
    }
    return FD_ERROR;
}

static int getRecvId(
        fd_set *iFds, int iCount, int *iWait, com_selectId_t *oRecv )
{
    int  count = 0;
    for( int i = 0;  i < iCount;  i++ ) {
        if( FD_ISSET( iWait[i], iFds ) ) {
            oRecv[count++] = searchId( iWait[i] );
        }
    }
    return count;
}

static BOOL acceptSocket(
        int *oAccFd, int iSockId, com_sockaddr_t *ioAddr, BOOL iNonBlock )
{
    memset( ioAddr, 0, sizeof(*ioAddr) );
    ioAddr->len = sizeof(ioAddr->addr);

    if( iNonBlock ) {fcntl( iSockId, F_SETFL, O_NONBLOCK );}
    else {fcntl( iSockId, F_SETFL, O_SYNC );}

    *oAccFd = accept( iSockId, (struct sockaddr*)(&ioAddr->addr),
                      &ioAddr->len );
    if( *oAccFd < 0 ) {
        if( checkNoRecv( iNonBlock, errno ) ) {return false;}
        com_error( COM_ERR_ACCEPTNG,
                   "fail to accept tcp connection [%s]", com_strerror(errno) );
        return false;
    }
    return true;
}

static void setAcceptInf(
        com_selectId_t iAcc, com_selectId_t iListen,
        com_sockEventCB_t iEventFunc, int iAccSd, com_sockaddr_t *iFrom )
{
    com_eventInf_t*  tmp = &(gEventInf[iAcc]);
    *tmp = gEventInf[iListen];   // まず listenソケットの設定をコピー
    // 構造体リテラルを使うと指定しないものが 0になるため、個別に設定する
    tmp->isListen = false;
    if( iEventFunc ) {tmp->eventFunc = iEventFunc;}
    tmp->sockId = iAccSd;
    tmp->dstInf = *iFrom;
}

com_selectId_t com_acceptSocket(
        com_selectId_t iListen, BOOL iNonBlock, com_sockEventCB_t iEventFunc )
{
    com_eventInf_t*  tmp = checkSocketInf( iListen, false );
    if( !tmp ) {COM_PRMNG(COM_NO_SOCK);}
    if( !tmp->isListen ) {COM_PRMNG(COM_NO_SOCK);}

    int  accSd = COM_NO_SOCK;
    com_sockaddr_t  fromAddr;
    if( !acceptSocket( &accSd, tmp->sockId, &fromAddr, iNonBlock ) ) {
        return COM_NO_SOCK;
    }
    // 確立したTCP接続用の管理IDを取得し、情報設定
    com_skipMemInfo( true );
    com_selectId_t  accId = getEventId( false );
    com_skipMemInfo( false );
    if( accId == COM_NO_SOCK ) {close( accSd );  return COM_NO_SOCK;}
    setAcceptInf( accId, iListen, iEventFunc, accSd, &fromAddr );
    debugEventLog( COM_MOD_ACCEPT, accId, tmp, NULL, 0 );
    return accId;
}

static BOOL callEventFunc(
        const com_eventInf_t *iInf, com_selectId_t iId,
        int iEvent, void *iData, size_t iDataSize, int iError )
{
    if( !iInf->eventFunc ) {
        com_error( iError, "event function not registered" );
        return false;
    }
    return (iInf->eventFunc)( iId, iEvent, iData, iDataSize );
}

static BOOL acceptTcpConnection( com_selectId_t iId, BOOL iNonBlock )
{
    // accept()に失敗しても、それが理由でループ終了とはしない
    com_selectId_t  accId = com_acceptSocket( iId, iNonBlock, NULL );
    if( accId == COM_NO_SOCK ) {return true;}

    return callEventFunc( &(gEventInf[iId]), accId,
                          COM_EVENT_ACCEPT, NULL, 0, COM_ERR_ACCEPTNG );
}

static BOOL closeTcpConnection( com_selectId_t iId )
{
    com_eventInf_t*  tmp = &(gEventInf[iId]);
    // 非TCP接続時は何もせずに返す(念の為の処理)
    if( COM_UNLIKELY( tmp->type <= COM_SOCK_UDP )) {return true;}

    (void)closeSocket( tmp );  // close()に失敗しても気にしない(何も出来ない)
    return callEventFunc( tmp, iId, COM_EVENT_CLOSE, NULL, 0, COM_ERR_CLOSENG );
}

static uchar gDefaultRecvBuf[COM_DATABUF_SIZE];
// 実際に使用するバッファ (デフォルト設定は com_initializeSelect()で実施
static uchar*  gRecvBuf = NULL;
static size_t  gRecvBufSize = 0;

void com_switchEventBuffer( void *iBuf, size_t iBufSize )
{
    gRecvBuf = iBuf;
    gRecvBufSize = iBufSize;
}


static BOOL readStdin( com_selectId_t iId )
{
    ssize_t  bytes = read( 0, gRecvBuf, gRecvBufSize );
    com_printfLogOnly( (char*)gRecvBuf );
    gRecvBuf[bytes - 1] = '\0';
    com_eventInf_t*  inf = &(gEventInf[iId]);
    debugEventLog( COM_MOD_STDIN, iId, inf, gRecvBuf, (size_t)bytes );
    return (inf->stdinFunc)( (char*)gRecvBuf, (size_t)bytes );
}

static BOOL recvPacket( com_selectId_t iId, BOOL iNonBlock, long *oDrop )
{
    memset( gRecvBuf, 0, gRecvBufSize );
    com_eventInf_t*  inf = &(gEventInf[iId]);
    if( inf->sockId == ID_STDIN ) {return readStdin( iId );}
    if( inf->isListen ) {return acceptTcpConnection(iId,iNonBlock);}

    com_sockaddr_t  fromAddr;
    ssize_t  recvSize;
    COM_RECV_RESULT_t  ret = com_receiveSocket(
            iId, gRecvBuf, gRecvBufSize, iNonBlock, &recvSize, &fromAddr );
    if( !ret ) { return false; } // エラーは com_receiveSocket()で出力済み
    if( ret == COM_RECV_NODATA ) {return true;}
    if( ret == COM_RECV_DROP ) {(*oDrop)++;  return true;}
    // TCP接続固有処理 (acceptは本関数冒頭で普通は済んでいるはずだが念の為)
    if( ret == COM_RECV_ACCEPT ) {return acceptTcpConnection(iId,iNonBlock);}
    if( ret == COM_RECV_CLOSE )  {return closeTcpConnection( iId );}
    // 非TCP接続時は送信元を対向として保持
    if( inf->type <= COM_SOCK_UDP ) {inf->dstInf = fromAddr;}
    return callEventFunc( inf, iId, COM_EVENT_RECEIVE, gRecvBuf, recvSize,
                          COM_ERR_RECVNG );
}

static BOOL recvBySelect( fd_set *iFds, int iCount, int *iWait, BOOL *oRetry )
{
    com_selectId_t  recvId[gEventId];  // さりげなく動的確保
    int  count = getRecvId( iFds, iCount, iWait, recvId );
    BOOL  result = false;
    BOOL  lastResult = true;
    long  drop = 0;
    for( int i = 0;  i < count;  i++ ) {
        if( recvId[i] == FD_ERROR ) {result = false;}
        else {result = recvPacket( recvId[i], false, &drop );}
        // ひとつでも結果が falseなら最終結果は falseになる
        if( !result ) {lastResult = false;}
    }
    *oRetry = (drop == count);
    return lastResult;
}

BOOL com_waitEvent( void )
{
    fd_set  fds;
    int  maxFd = 0;
    int  waitFd[gEventId];
    while(1) {
        int  count = createFdList( &fds, &maxFd, waitFd );
        struct timeval  tm;
        struct timeval*  selTimer = checkNextTimer( &tm );
        if( !selTimer && !count ) {return false;}  // 待機するもの無し

        int  result = 0;
        if( 0 > (result = select( maxFd+1, &fds, NULL, NULL, selTimer )) ) {
            com_error( COM_ERR_SELECTNG,
                       "fail to select[%s]", com_strerror(errno) );
            return false;
        }
        if( !result ) {return expireTimer( gWaitingId );}
        BOOL  retry = false;
        if( recvBySelect(&fds, count, waitFd, &retry) ) {if(!retry){break;}}
        else {return false;}
    }
    return true;
}

BOOL com_watchEvent( void ) {
    BOOL  result = true;
    GET_CURRENT;
    long  drop = 0;
    for( com_selectId_t id = 0;  id < gEventId;  id++ ) {
        com_eventInf_t* inf = &(gEventInf[id]);
        if( !inf->isUse ) {continue;}
        if( inf->eventFunc ) {
            if( !recvPacket( id, true, &drop ) ) {result = false;}
        }
        else if(current) {if( !checkTimer(id,current) ) {result = false;}}
    }
    // こちらは dropがあっても何もしない
    return result;
}

static com_selectId_t  gStdinId = COM_NO_SOCK;

com_selectId_t com_registerStdin( com_getStdinCB_t iRecvFunc )
{
    if( gStdinId != COM_NO_SOCK ) {return gStdinId;}  // 複数登録は不可
    if( !iRecvFunc ) {COM_PRMNG(COM_NO_SOCK);}
    com_skipMemInfo( true );
    com_selectId_t  id = getEventId( false );
    com_skipMemInfo( false );
    if( id == COM_NO_SOCK ) {return COM_NO_SOCK;}
    com_eventInf_t*  tmp = &(gEventInf[id]);
    *tmp = (com_eventInf_t){
        .isUse = true,  // .allowMultiLine = iMulti, // 複数行は中断中
        .timer = COM_NO_SOCK,  .sockId = ID_STDIN,  .stdinFunc = iRecvFunc
    };
    gStdinId = id;
    debugEventLog( COM_MOD_STDINON, id, tmp, NULL, 0 );
    return gStdinId;
}

void com_cancelStdin( com_selectId_t iId )
{
    com_eventInf_t*  tmp = checkSocketInf( iId, false );
    if( !tmp ) {COM_PRMNG();}
    if( tmp->sockId != ID_STDIN ) {COM_PRMNG();}

    tmp->isUse = false;
    gStdinId = COM_NO_SOCK;
    debugEventLog( COM_MOD_STDINOFF, iId, tmp, NULL, 0 );
    return;
}



// アドレス情報取得/解放 -----------------------------------------------------

static size_t calcSize( struct addrinfo *oInfo )
{
    size_t  size = sizeof( *oInfo );
    if( (size_t)oInfo->ai_addrlen > sizeof( *oInfo ) ) {
        size += oInfo->ai_addrlen;
    }
    else {size += sizeof( struct sockaddr );}
    if( oInfo->ai_canonname ) {size += strlen( oInfo->ai_canonname ) + 1;}
    return size;
}

static BOOL checkPrmGetaddrinfo(
        struct addrinfo **oTarget, COM_SOCK_TYPE_t iType, int iFamily )
{
    if( !oTarget ) {return false;}
    if( iType < COM_SOCK_RAWSND || iType > COM_SOCK_TCP_SERVER ) {return false;}
    if( iFamily != AF_INET && iFamily != AF_INET6 ) {return false;}
    return true;
}

BOOL com_getaddrinfoFunc(
        struct addrinfo **oTarget, COM_SOCK_TYPE_t iType, int iFamily,
        const char *iAddress, ushort iPort, COM_FILEPRM )
{
    if( !checkPrmGetaddrinfo( oTarget, iType, iFamily ) ) {COM_PRMNG(false);}
    char  portText[6] = {0};  // ポート番号は最大5桁 + 文字列終端
    snprintf( portText, sizeof(portText), "%d", iPort );
    struct addrinfo  hints = {
        .ai_family = iFamily,
        .ai_socktype = (iType <= COM_SOCK_UDP) ? SOCK_DGRAM : SOCK_STREAM,
        .ai_flags = AI_NUMERICSERV
    };
    *oTarget = NULL;
    int  result = getaddrinfo( iAddress, portText, &hints, oTarget );
    if( result ) {
        com_error( COM_ERR_DEBUGNG,
                   "getaddrinfo() NG [%s]", com_strerror(result) );
        return false;
    }
    com_addMemInfo( COM_FILEVAR, COM_GETADDRINFO, *oTarget, calcSize(*oTarget),
                    "addrinfo [%s]:%d", iAddress, iPort );
    // 解放は com_freeaddrinfo() を使う
    return true;
}

void com_freeaddrinfoFunc( struct addrinfo **oTarget, COM_FILEPRM )
{
    if( !oTarget ) {COM_PRMNG();}
    if( !(*oTarget) ) {return;}
    freeaddrinfo( *oTarget );
    com_deleteMemInfo( COM_FILEVAR, COM_FREEADDRINFO, *oTarget );
    *oTarget = NULL;
}

void com_copyAddr( com_sockaddr_t *oTarget, const void *iSource )
{
    if( !iSource || !oTarget ) {COM_PRMNG();}
    memset( oTarget, 0, sizeof(*oTarget) );
    if( !isUnixSocket( iSource ) ) {
        const struct addrinfo* ai = iSource;
        oTarget->len = ai->ai_addrlen;
        memcpy( &oTarget->addr, ai->ai_addr, oTarget->len );
    }
    else {
        const struct sockaddr_un* sun = iSource;
        oTarget->len = SUN_LEN( sun );
        memcpy( &oTarget->addr, sun, oTarget->len );
    }
}

COM_AF_TYPE_t com_isIpAddr( const char *iString )
{
    struct sockaddr_storage  dmy;
    if( 1 == inet_pton( AF_INET,  iString, &dmy ) ) {return COM_IPV4;}
    if( 1 == inet_pton( AF_INET6, iString, &dmy ) ) {return COM_IPV6;}
    return COM_NOTIP;
}

BOOL com_valIpAddress( char *ioData, void *iCond )
{
    com_valCondIpAddress_t*  cond = iCond;
    if( !cond ) {cond = &(com_valCondIpAddress_t){ true, true };}
    COM_AF_TYPE_t af = com_isIpAddr( ioData );
    if( cond->ipv4 && af == COM_IPV4 ) {return true;}
    if( cond->ipv6 && af == COM_IPV6 ) {return true;}
    return false;
}

BOOL com_valIpAddressCondCopy( void **oCond, void *iCond )
{
    com_valCondIpAddress_t*  source = iCond;
    com_valCondIpAddress_t*  tmp =
        com_malloc(sizeof(com_valCondIpAddress_t), "copy ip address condition");
    if( !tmp ) {return false;}
    *tmp = *source;
    *oCond = tmp;
    return true;
}

static socklen_t getAddrSin( void *iSockAddr, void **oAddr )
{
    struct sockaddr_in*  sin = iSockAddr;
    *oAddr = &(sin->sin_addr.s_addr);
    return sizeof(sin->sin_addr.s_addr);
}

static socklen_t getAddrSin6( void *iSockAddr, void **oAddr )
{
    struct sockaddr_in6*  sin6 = iSockAddr;
    *oAddr = &(sin6->sin6_addr.s6_addr);
    return sizeof(sin6->sin6_addr.s6_addr);
}

#ifdef LINUXOS
static socklen_t getAddrLl( void *iSockAddr, void **oAddr )
{
    struct sockaddr_ll*  sll = iSockAddr;
    *oAddr = &(sll->sll_addr);
    return sll->sll_halen;
}
#endif

static socklen_t getAddrUn( void *iSockAddr, void **oAddr )
{
    struct sockaddr_un*  sun = iSockAddr;
    *oAddr = &(sun->sun_path);
    return strlen(sun->sun_path);
}

static socklen_t getAddrLen( void *iSockAddr, void **oAddr )
{
    struct sockaddr*  sa = iSockAddr;
    int family = sa->sa_family;
    if( family == AF_INET )   {return getAddrSin(  iSockAddr, oAddr ); }
    if( family == AF_INET6 )  {return getAddrSin6( iSockAddr, oAddr ); }
#ifdef LINUXOS
    if( family == AF_PACKET ) {return getAddrLl(   iSockAddr, oAddr ); }
#endif
    if( family == AF_UNIX )   {return getAddrUn(   iSockAddr, oAddr ); }
    *oAddr = sa->sa_data;
    return 0;
}

BOOL com_compareAddr( void *iTarget, void *iSockAddr )
{
    if( !iTarget || !iSockAddr ) {COM_PRMNG(false);}
    void*  addr = NULL;
    socklen_t len = getAddrLen( iSockAddr, &addr );
    if( !len ) {return false;}
    return !memcmp( iTarget, addr, len );
}

static BOOL compareSockFamily(
        struct sockaddr *iTarget, struct sockaddr *iSource )
{
    return (iTarget->sa_family == iSource->sa_family);
}

static BOOL compareSockAddr( void *iTarget, void *iSource )
{
    void*  addrTarget = NULL;
    socklen_t lenTarget = getAddrLen( iTarget, &addrTarget );
    if( !lenTarget ) {return false;}
    void*  addrSource = NULL;
    socklen_t lenSource = getAddrLen( iSource, &addrSource );
    if( !lenSource ) {return false;}
    if( lenTarget != lenSource ) {return false;}
    return !memcmp( addrTarget, addrSource, lenSource );
}

BOOL com_compareSock( void *iTargetSock, void *iSockAddr )
{
    if( !iTargetSock || !iSockAddr ) {COM_PRMNG(false);}
    if( !compareSockFamily( iTargetSock, iSockAddr ) ) {return false;}
    return compareSockAddr( iTargetSock, iSockAddr );
}

static BOOL isIp( struct sockaddr *iSock )
{
    if( iSock->sa_family == AF_INET )  {return true;}
    if( iSock->sa_family == AF_INET6 ) {return true;}
    return false;
}

static ushort getSockPort( void *iSockAddr )
{
    struct sockaddr*  sa = iSockAddr;
    if( sa->sa_family == AF_INET ) {
        struct sockaddr_in*  sin = iSockAddr;
        return sin->sin_port;
    }
    if( sa->sa_family == AF_INET6 ) {
        struct sockaddr_in6*  sin6 = iSockAddr;
        return sin6->sin6_port;
    }
    return 0;
}

static BOOL compareSockPort( void *iTarget, void *iSource )
{
    ushort  portTarget = getSockPort( iTarget );
    ushort  portSource = getSockPort( iSource );
    return (portTarget == portSource);
}

BOOL com_compareSockWithPort( void *iTargetSock, void *iSockAddr )
{
    if( !iTargetSock || !iSockAddr ) {COM_PRMNG(false);}
    if( !isIp(iTargetSock) || !isIp(iSockAddr) ) {return false;}
    if( !compareSockFamily( iTargetSock, iSockAddr ) ) {return false;}
    if( !compareSockPort( iTargetSock, iSockAddr ) ) {return false;}
    return compareSockAddr( iTargetSock, iSockAddr );
}



// IF情報データ定義 ----------------------------------------------------------

static struct ifaddrs*  gIfAddr = NULL;
static com_ifinfo_t*  gIfInfo = NULL;
static long  gIfInfoCnt = 0;
static int  gIfInfoSock = COM_NO_SOCK;  // 情報取得用に生成するソケット

#ifndef LINUXOS
#ifndef ETH_ALEN
#define   ETH_ALEN  6
#endif // ETH_ALEN
#endif // LINUXOS

// 旧来のIF情報取得関連 ------------------------------------------------------

/////////////// getifaddrs()や ioctl()を使ったI/F情報取得処理
/////////////// 処理起点は collectIfInfo()

static BOOL openSocketForInfo( void )
{
    if( gIfInfoSock < 0 ) {
        // PF_PACKETを使う例が多いが、要root権限なので避ける
        if( 0 > (gIfInfoSock = socket( PF_INET, SOCK_DGRAM, 0 )) ) {
            com_error( COM_ERR_GETIFINFO, "fail to create socket for ifinfo" );
            return false;
        }
    }
    // 一度生成したソケットをそのまま使い続ける
    // close()は freeIfInfo()で実施 (finalizeSelect()でも呼ばれる)
    return true;
}

static void setHwAddr( com_ifinfo_t *oInf, void *iMacAddr )
{
    memcpy( oInf->hwaddr, iMacAddr, ETH_ALEN );
    oInf->h_len = ETH_ALEN;
}

static void setIfInfo( com_ifinfo_t *oInf, char *iIfname )
{
    oInf->ifname = iIfname;
    if( !openSocketForInfo() ) {return;}
    struct ifreq  ifr;
    strcpy( ifr.ifr_name, iIfname );
    if( 0 > ioctl( gIfInfoSock, (int)SIOCGIFINDEX, &ifr ) ) {
        com_error( COM_ERR_GETIFINFO, "fail to get index for %s", iIfname );
    }
    else {oInf->ifindex = ifr.ifr_ifindex;}
    if( 0 > ioctl( gIfInfoSock, (int)SIOCGIFHWADDR, &ifr ) ) {
        com_error( COM_ERR_GETIFINFO, "fail to get hwaddr for %s", iIfname );
    }
    else {setHwAddr( oInf, ifr.ifr_hwaddr.sa_data );}
}

static com_ifinfo_t *getIfInfo( char *iIfname )
{
    for( long i = 0;  i < gIfInfoCnt;  i++ ) {
        if( !strcmp( gIfInfo[i].ifname, iIfname ) ) {return &(gIfInfo[i]);}
    }
    com_ifinfo_t*  newInfo =
        com_reallocAddr( &gIfInfo, sizeof(*gIfInfo), COM_TABLEEND,
                         &gIfInfoCnt, 1, "ifinfo list" );
    if( newInfo ) {setIfInfo( newInfo, iIfname );}
    return newInfo;
}

static void freeIfInfo( void )
{
    if( gIfAddr ) {freeifaddrs( gIfAddr );  gIfAddr = NULL;}
    if( gIfInfoSock >= 0 ) {close( gIfInfoSock );  gIfInfoSock = COM_NO_SOCK;}
    if( !gIfInfoCnt ) {return;}
    for( long i = 0;  i < gIfInfoCnt;  i++ ) {
        com_ifinfo_t* tmp = &(gIfInfo[i]);
        if( !tmp->ifaddrs ) {  // Netlink取得の場合
            com_free( tmp->ifname );
            for( long j = 0;  j < tmp->cntAddrs;  j++ ) {
                com_free( tmp->soAddrs[j].addr );
                com_free( tmp->soAddrs[j].label );
            }
        }
        com_free( tmp->soAddrs );
        com_free( tmp->ifaddrs );
    }
    com_free( gIfInfo );
    gIfInfoCnt = 0;
}

static struct ifaddrs *getifaddrsFunc( void )
{
    struct ifaddrs*  ifaddr;
    if( 0 > getifaddrs( &ifaddr ) ) {
        com_error( COM_ERR_GETIFINFO, "fail to getifaddrs()" );
        return NULL;
    }
    return ifaddr;
}

enum { ADDRCNT_ADD = 1 };

static void *reallocSync(
        void *oTarget, size_t iSize, long iDmyCnt, const char *iLabel )
{
    iDmyCnt -= ADDRCNT_ADD;
    return com_reallocAddr( oTarget, iSize, 0,
                            &iDmyCnt, 1, "ifinfo (%s)", iLabel );
}

static BOOL addIfAddr( struct ifaddrs *iIfa )
{
    com_ifinfo_t*  inf = getIfInfo( iIfa->ifa_name );
    if( !inf ) {return false;}
    struct ifaddrs**  ifAddr =
        com_reallocAddr( &inf->ifaddrs, sizeof(struct ifaddrs*), 0,
                         &inf->cntAddrs, ADDRCNT_ADD, "addAddrInf (ifaddr)" );
    if( !ifAddr ) {return false;}
    *ifAddr = iIfa;
    com_ifaddr_t*  soAddrs = reallocSync( &inf->soAddrs, sizeof(*soAddrs),
                                          inf->cntAddrs, "sockaddr" );
    if( !soAddrs ) {return false;}
    struct sockaddr*  sa = iIfa->ifa_addr;
    soAddrs->addr = (sa->sa_family == AF_INET || sa->sa_family == AF_INET6)
                    ? sa : NULL;
    return true;
}

static long collectIfInfo( void )
{
    if( !(gIfAddr = getifaddrsFunc()) ) {return COM_IFINFO_ERR;}
    for( struct ifaddrs* ifa = gIfAddr;  ifa;  ifa = ifa->ifa_next ) {
        if( !ifa->ifa_addr ) {continue;}
        if( !addIfAddr( ifa ) ) {freeIfInfo();  return COM_IFINFO_ERR;}
    }
    return gIfInfoCnt;
}

#ifdef LINUXOS
/////////////// Netlinkソケットを使ったI/F情報取得処理
/////////////// 処理起点は getIfInfoByNetlink()

typedef struct {
    struct nlmsghdr     hdr;
    struct ifinfomsg    body;
} com_getLinkReq_t;

typedef struct {
    struct nlmsghdr     hdr;
    struct ifaddrmsg    body;
} com_getAddrReq_t;

static void setNlmsgHdr( struct nlmsghdr *oHdr, size_t iSize, ushort iType )
{
    *oHdr = (struct nlmsghdr){
        .nlmsg_len   = NLMSG_LENGTH( iSize ),
        .nlmsg_type  = iType,
        .nlmsg_flags = (NLM_F_REQUEST | NLM_F_DUMP),
        .nlmsg_pid   = getpid()
    };
}

#define SETMSGHDR( MSGTYPE, MSGOP ) \
    MSGTYPE  msg; \
    do { \
        memset( &msg, 0, sizeof(msg) ); \
        setNlmsgHdr( &msg.hdr, sizeof(msg), (MSGOP) ); \
    } while(0)

static char  gAddrBuff[COM_DATABUF_SIZE];
static char  gRouteBuff[COM_DATABUF_SIZE];

typedef BOOL(*com_nlmsgFunc_t)(
    struct nlmsghdr *iMsg, size_t iMsgLen, void *iUserData );

#define BREAK( RESULT ) \
    result = (RESULT); \
    break; \
    do {} while(0)

static BOOL recvNlmsg(
        com_selectId_t iId, char *oBuf, size_t iBufSize,
        com_nlmsgFunc_t iFunc, void *iUserData )
{
    BOOL  result = false;
    while(1) {
        memset( oBuf, 0, iBufSize );
        ssize_t  msgLen = 0;
        if( !com_receiveSocket( iId, oBuf, iBufSize, false, &msgLen, NULL ) ) {
            BREAK( false );
        }
        if( (size_t)msgLen < sizeof(struct nlmsghdr) ) {
            com_error( COM_ERR_RECVNG, "received illegal packet (netlink)" );
            BREAK( false );
        }
        struct nlmsghdr* msg  = (struct nlmsghdr*)oBuf;
        if( msg->nlmsg_type == NLMSG_DONE ) {BREAK( true );} // 処理終了
        if( !iFunc( msg, msgLen, iUserData ) ) {BREAK( false );}
    }
    com_deleteSocket( iId );
    return result;
}

static BOOL setIfName( com_ifinfo_t *oInf, void *iIfname )
{
    if( !(oInf->ifname = com_strdup( (char*)iIfname, NULL )) ) {
        return false;
    }
    return true;
}

static com_ifinfo_t *getLinkDataFromAttr(
        struct ifinfomsg *iIfMsg, int iMsgLen )
{
    com_ifinfo_t  inf;
    memset( &inf, 0, sizeof(inf) );
    for( struct rtattr* attr = (struct rtattr*)IFLA_RTA( iIfMsg );
         RTA_OK( attr, iMsgLen );  attr = RTA_NEXT( attr, iMsgLen ) )
    {
        if( attr->rta_type == IFLA_IFNAME ) {
            if( !setIfName( &inf, RTA_DATA(attr) ) ) {return NULL;}
        }
        if( attr->rta_type == IFLA_ADDRESS ) {
            setHwAddr( &inf, RTA_DATA(attr) );
        }
    }
    if( !inf.ifname ) {return NULL;}
    com_ifinfo_t*  tmp = getIfInfo( inf.ifname );
    if( !tmp ) {return NULL;}
    *tmp = inf;
    tmp->ifindex = iIfMsg->ifi_index;
    return tmp;
}

static void *addSaForNetlink( com_ifinfo_t *oInf, int iFamily )
{
    struct sockaddr*  tmp = NULL;
    if( iFamily == AF_INET ) {
        tmp = com_malloc( sizeof(struct sockaddr_in),  "IPv4 by Netlink" );
    }
    else {
        tmp = com_malloc( sizeof(struct sockaddr_in6), "IPv6 by Netlink" );
    }
    if( !tmp ) {return NULL;}
    tmp->sa_family = iFamily;

    com_ifaddr_t*  soAddr =
        com_reallocAddr( &oInf->soAddrs, sizeof(*soAddr), 0,
                         &oInf->cntAddrs, 1, "setIpAddr (sockaddr)" );
    if( !soAddr ) {com_free( tmp );  return NULL;}
    soAddr->addr = tmp;
    return tmp;
}

static BOOL setIpAddr( com_ifinfo_t *oInf, struct ifaddrmsg *iMsg, void *iAddr )
{
    int family  = iMsg->ifa_family;
    if( family == AF_INET ) {
        struct sockaddr_in*  sa = addSaForNetlink( oInf, family );
        if( !sa ) {return false;}
        memcpy( &sa->sin_addr.s_addr, iAddr, sizeof(struct in_addr) );
        
    }
    else if( family == AF_INET6 ) {
        struct sockaddr_in6*  sa6 = addSaForNetlink( oInf, family );
        if( !sa6 ) {return false;}
        memcpy( &sa6->sin6_addr.s6_addr, iAddr, sizeof(struct in6_addr) );
    }
    oInf->soAddrs[0].preflen = iMsg->ifa_prefixlen;
    return true;
}

static BOOL setIfLabel( com_ifinfo_t *oInf, void *iLabel )
{
    char*  tmp = com_strdup( (char*)iLabel, NULL );
    if( !tmp ) {return false;}
    oInf->soAddrs[0].label = tmp;
    return true;
}

static BOOL getAddrDataFromAttr(
        struct ifaddrmsg *iAddrMsg, int iMsgLen, com_ifinfo_t *oInf )
{
    for( struct rtattr* attr = (struct rtattr*)IFA_RTA( iAddrMsg );
         RTA_OK( attr, iMsgLen );  attr = RTA_NEXT( attr, iMsgLen ) )
    {
        if( attr->rta_type == IFA_ADDRESS ) {
            if( !setIpAddr( oInf, iAddrMsg, RTA_DATA( attr ) ) ) {
                return false;
            }
        }
        if( attr->rta_type == IFA_LABEL ) {
            if( !setIfLabel( oInf, RTA_DATA( attr ) ) ) {return false;}
        }
    }
    return true;
}

struct linkinfo {
    ulong index;
    com_ifinfo_t* ifInfo;
};

// com_nlmsgFunc_t型関数
static BOOL getAddr( struct nlmsghdr *iMsg, size_t iMsgLen, void *iUserData )
{
    struct linkinfo*  inf = iUserData;

    for( ;  NLMSG_OK(iMsg, iMsgLen);  iMsg = NLMSG_NEXT(iMsg, iMsgLen) ) {
        struct ifaddrmsg*  msg = (void*)NLMSG_DATA( iMsg );
        int  msgLen = IFA_PAYLOAD( iMsg );
        if( msg->ifa_index == inf->index ) {
            getAddrDataFromAttr( msg, msgLen, inf->ifInfo );
        }
    }
    return true;
}

static com_selectId_t createNetlinkSocket( void )
{
    return com_createSocket( COM_SOCK_NETLINK, NULL, NULL, NULL, NULL );
}

static BOOL getAddrList( struct linkinfo *ioInf )
{
    com_selectId_t  id = createNetlinkSocket();
    if( id == COM_NO_SOCK ) {return false;}
    SETMSGHDR( com_getAddrReq_t, RTM_GETADDR );
    msg.body.ifa_family = 0;  // AF_INET
    if( !com_sendSocket( id, &msg, msg.hdr.nlmsg_len, NULL ) ) {return false;}
    return recvNlmsg( id, gAddrBuff, sizeof(gAddrBuff), getAddr, ioInf );
}

// com_nlmsgFunc_t型関数
static BOOL getLink( struct nlmsghdr *iMsg, size_t iMsgLen, void *iUserData )
{
    COM_UNUSED( iUserData );
    for( ;  NLMSG_OK( iMsg, iMsgLen );  iMsg = NLMSG_NEXT( iMsg, iMsgLen ) ) {
        struct ifinfomsg*  msg = (void*)NLMSG_DATA( iMsg );
        int  msgLen = IFLA_PAYLOAD( iMsg );
        com_ifinfo_t*  inf = getLinkDataFromAttr( msg, msgLen );
        struct linkinfo  linkInf = { (ulong)(msg->ifi_index), inf };
        if( !getAddrList( &linkInf ) ) {return false;}
    }
    return true;
}

static long getIfInfoByNetlink( void )
{
    com_selectId_t  id = createNetlinkSocket();
    if( id == COM_NO_SOCK ) {return COM_IFINFO_ERR;}
    SETMSGHDR( com_getLinkReq_t, RTM_GETLINK );
    msg.body.ifi_family = ARPHRD_ETHER;
    if( !com_sendSocket( id, &msg, msg.hdr.nlmsg_len, NULL ) ) {
        return COM_IFINFO_ERR;
    }
    (void)recvNlmsg( id, gRouteBuff, sizeof(gRouteBuff), getLink, NULL );
    return gIfInfoCnt;
}
#endif // LINUXOS

long com_collectIfInfo( com_ifinfo_t **oInf, BOOL iUseNetlink )
{
#ifndef LINUXOS
    if( iUseNetlink ) {COM_PRMNG(COM_IFINFO_ERR);}
#endif
    if( !oInf ) {COM_PRMNG(COM_IFINFO_ERR);}
    com_skipMemInfo( true );
    freeIfInfo();
    long  cnt = 0;
    *oInf = NULL;
    long(*func)(void) = collectIfInfo;
#ifdef LINUXOS
    if( iUseNetlink ) {func = getIfInfoByNetlink;}
#endif
    if( 0 <= (cnt = func()) ) {*oInf = gIfInfo;}
    com_skipMemInfo( false );
    return cnt;
}

long com_getIfInfo( com_ifinfo_t **oInf, BOOL iUseNetlink )
{
#ifndef LINUXOS
    if( iUseNetlink ) {COM_PRMNG(COM_IFINFO_ERR);}
#endif
    if( !oInf ) {COM_PRMNG(COM_IFINFO_ERR);}
    long  cnt = gIfInfoCnt;
    if( !(*oInf = gIfInfo) ) {cnt = com_collectIfInfo( oInf, iUseNetlink );}
    return cnt;
}

static BOOL setIfAddr( void **oAddr, struct addrinfo **oTmp, void *iAddr )
{
    if( !iAddr ) {return false;}
    BOOL  result = com_getaddrinfo(
            oTmp, COM_SOCK_UDP, (strstr(iAddr, ".")) ? AF_INET : AF_INET6,
            iAddr, 0 );
    if( !result ) {return false;}
    *oAddr = (*oTmp)->ai_addr;
    return true;
}

static BOOL setIfMac( uchar *oMac, char *iMac )
{
    uint  mac[ETH_ALEN];
    int  cnt = sscanf( iMac, "%02x:%02x:%02x:%02x:%02x:%02x",
                      &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] );
    if( cnt != ETH_ALEN ) {return false;}
    for( int i = 0;  i < ETH_ALEN;  i++ ) {oMac[i] = mac[i];}
    return true;
}

#define BOTHEXIST( COND1, COND2 ) \
    if( COM_CHECKBIT( iFlags, (COND1) ) && COM_CHECKBIT( iFlags, (COND2) ) ) { \
        COM_PRMNG( false ); \
    } \
    do{} while(0)

#define MAKEADDRS( SA, VAL ) \
    if( COM_CHECKBIT( iFlags, (SA) ) ) { \
        if( !iCond->VAL ) {return false;} \
        oCond->VAL = iCond->VAL; \
    } \
    do{} while(0)

static BOOL makeSearchIfCond(
        int iFlags, com_seekIf_t *oCond, com_seekIf_t *iCond,
        struct addrinfo **oTmp, uchar *oTmpMac )
{
    if( COM_CHECKBIT( iFlags, COM_IF_NAME ) ) {
        if( !iCond->ifname ) {COM_PRMNG(false);}
        oCond->ifname = iCond->ifname;
    }
    if( COM_CHECKBIT( iFlags, COM_IF_INDEX ) ) {
        if( iCond->ifindex < 0 ) {COM_PRMNG(false);}
        oCond->ifindex = iCond->ifindex;
    }
    BOTHEXIST( COM_IF_IPTXT, COM_IF_IPSA );
    MAKEADDRS( COM_IF_IPSA, ipaddr );
    if( COM_CHECKBIT( iFlags, COM_IF_IPTXT ) ) {
        if(!setIfAddr(&oCond->ipaddr,oTmp, iCond->ipaddr)) {COM_PRMNG(false);}
    }
    BOTHEXIST( COM_IF_HWTXT, COM_IF_HWBIN );
    MAKEADDRS( COM_IF_HWBIN, hwaddr );
    if( COM_CHECKBIT( iFlags, COM_IF_HWTXT ) ) {
        if( !iCond->hwaddr ) {COM_PRMNG(false);}
        oCond->hwaddr = oTmpMac;
        if( !setIfMac( oCond->hwaddr, iCond->hwaddr ) ) {COM_PRMNG(false);}
    }
    return true;
}

static void *getAddrPoint( struct sockaddr *iSa )
{
    if( iSa->sa_family == AF_INET6 ) {
        return ((struct sockaddr_in6*)iSa)->sin6_addr.s6_addr;
    }
    return &(((struct sockaddr_in*)iSa)->sin_addr.s_addr);
}

static BOOL seekIfLabel( com_ifinfo_t *iInf, char *iName )
{
    if( !iInf->soAddrs ) {return false;}
    for( long i = 0;  i < iInf->cntAddrs;  i++ ) {
        char*  label = iInf->soAddrs[i].label;
        if( !label ) {continue;}
        if( !strcmp( label, iName ) ) {return true;}
    }
    return false;
}

static BOOL compareAddr(
        long iCnt, com_ifaddr_t *iAddrs, struct sockaddr *iCond )
{
    if( !iAddrs ) {return false;}
    for( long i = 0;  i < iCnt;  i++ ) {
        if( !(iAddrs[i].addr) ) {continue;}  // AF_PACKETではあり得る
        if( iAddrs[i].addr->sa_family != iCond->sa_family ) {continue;}
        socklen_t  len = (iCond->sa_family == AF_INET ) ? 4 : 16;
        void*  addrIf = getAddrPoint( iAddrs[i].addr );
        void*  addrCo = getAddrPoint( iCond );
        if( !memcmp( addrIf, addrCo, len ) ) {return true;}
    }
    return false;
}

static BOOL matchIfCond( int iFlags, com_ifinfo_t *iInf, com_seekIf_t *iCond )
{
    if( COM_CHECKBIT( iFlags, COM_IF_NAME ) ) {
        if( strcmp( iInf->ifname, iCond->ifname ) ) {
            return seekIfLabel( iInf, iCond->ifname );
        }
    }
    if( COM_CHECKBIT( iFlags, COM_IF_INDEX ) ) {
        if( iInf->ifindex != iCond->ifindex ) {return false;}
    }
    if( COM_CONTAINBIT( iFlags, (COM_IF_IPTXT | COM_IF_IPSA) ) ) {
        if( !compareAddr( iInf->cntAddrs, iInf->soAddrs, iCond->ipaddr ) ) {
            return false;
        }
    }
    if( COM_CONTAINBIT( iFlags, (COM_IF_HWTXT | COM_IF_HWBIN) ) ) {
        uchar*  condMac = iCond->hwaddr;
        for( int i = 0;  i < ETH_ALEN;  i++ ) {
            if( iInf->hwaddr[i] != condMac[i] ) {return false;}
        }
    }
    return true;
}

#define SEEKIFRETURN( RET )    return seekIfResult( &tmp, RET )

static com_ifinfo_t *seekIfResult( struct addrinfo **oTmp, com_ifinfo_t *iRet )
{
    if( oTmp ) {com_freeaddrinfo( oTmp );}
    com_skipMemInfo( false );
    return iRet;
}

com_ifinfo_t *com_seekIfInfo(
        int iFlags, com_seekIf_t *iCond, BOOL iUseNetlink )
{
#ifndef LINUXOS
    if( iUseNetlink ) {COM_PRMNG(NULL);}
#endif
    if( iFlags <= 0 || iFlags > COM_IF_CONDMAX || !iCond ) {COM_PRMNG(NULL);}
    com_seekIf_t  cond = {NULL, -1, NULL, NULL};
    struct addrinfo*  tmp = NULL;
    uchar  tmpMac[ETH_ALEN];
    com_skipMemInfo( true );
    if( !makeSearchIfCond( iFlags, &cond, iCond, &tmp, tmpMac ) ) {
        SEEKIFRETURN( NULL );
    }
    com_ifinfo_t* inf = NULL;
    long  infCnt = com_getIfInfo( &inf, iUseNetlink );
    if( infCnt <= 0 ) {SEEKIFRETURN( NULL );}
    for( long i = 0;  i < infCnt;  i++ ) {
        com_ifinfo_t* ifinf = &(gIfInfo[i]);
        if( matchIfCond( iFlags, ifinf, &cond ) ) {SEEKIFRETURN( ifinf );}
    }
    SEEKIFRETURN( NULL );
}



// デバッグ用イベント情報出力 ------------------------------------------------

static socklen_t getSockLen( const void *iAddr )
{
    const struct sockaddr*  addr = iAddr;
    size_t  len = 0;
    if( addr->sa_family == AF_INET )  {len = sizeof(struct sockaddr_in);}
    if( addr->sa_family == AF_INET6 ) {len = sizeof(struct sockaddr_in6);}
    if( addr->sa_family == AF_UNIX ) {
        len = SUN_LEN( (struct sockaddr_un*)iAddr );
    }
    return (socklen_t)len;
}

BOOL com_getnameinfo(
        char **oAddr, char **oPort, const void *iAddr, socklen_t iLen )
{
    if( !iAddr ) {COM_PRMNG(false);}
    if( !iLen ) {iLen = getSockLen( iAddr );}
    if( !iLen ) {return false;}

    static char  hbuf[COM_LINEBUF_SIZE];
    static char  sbuf[COM_WORDBUF_SIZE];
    COM_CLEAR_BUF( hbuf );
    COM_CLEAR_BUF( sbuf );
    if( getnameinfo( (struct sockaddr*)iAddr, iLen,
                     hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                     NI_NUMERICHOST | NI_NUMERICSERV ) )
    {
        COM_SET_IF_EXIST( oAddr, NULL );
        COM_SET_IF_EXIST( oPort, NULL );
        return false;
    }
    COM_SET_IF_EXIST( oAddr, hbuf );
    COM_SET_IF_EXIST( oPort, sbuf );
    return true;
}

static void dumpSockAddr( const char *iLabel, com_sockaddr_t *iInf )
{
    if( iInf->len > 0 ) {
        char*  addr = NULL;
        char*  port = NULL;
        if( com_getnameinfo( &addr, &port, &iInf->addr, iInf->len ) ) {
            com_dbgCom( "     %s=[%s]:%s", iLabel, addr, port );
        }
        else {com_dbgCom( "     %s=(illegal data?)", iLabel );}
    }
    else {    com_dbgCom( "     %s=(none)", iLabel );}
    com_dumpCom( &(iInf->addr), iInf->len, "     (len=%zu)", iInf->len );
}

static void dispConvertTime( struct timeval *iTime )
{
    char  startDate[COM_DATE_DSIZE];
    char  startTime[COM_TIME_DSIZE];
    com_setTimeval( COM_FORM_DETAIL, startDate, startTime, NULL, iTime );
    com_dbgCom( "               (%s %s)", startDate, startTime );
}

void com_dispDebugSelect( void )
{
    if( !com_getDebugPrint() ) {return;}
    com_dbgCom( "=== Current Event Info List Start ===" );
    for( com_selectId_t id = 0;  id < gEventId;  id++ ) {
        com_eventInf_t* tmp = &(gEventInf[id]);
        com_dbgCom( "#%-3ld  isUse=%ld  isListen=%ld",
                      id, tmp->isUse, tmp->isListen );
        com_dbgCom( "    timer=%ld", tmp->timer );
        com_dbgCom( "     isStopped=%ld", tmp->isStopped );
        com_dbgCom( "     expireFunc=%zx", (intptr_t)tmp->expireFunc );
        com_dbgCom( "     startTime=%ld.%06ld",
                      tmp->startTime.tv_sec, tmp->startTime.tv_usec );
        if( tmp->timer != COM_NO_SOCK ) {dispConvertTime( &tmp->startTime );}
        com_dbgCom( "    sockId=%d", tmp->sockId );
        com_dbgCom( "     eventFunc=%zx", (intptr_t)tmp->eventFunc );
        com_dbgCom( "     stdinFunc=%zx", (intptr_t)tmp->stdinFunc );
        com_dbgCom( "     type=%d", tmp->type );
        dumpSockAddr( "srcInf", &tmp->srcInf );
        dumpSockAddr( "dstInf", &tmp->dstInf );
        if( id < (gEventId - 1) ) {com_dbgCom( " ");}
    }
    com_dbgCom( "=== Current Event Info List End ===" );
}


// 初期化処理 ----------------------------------------------------------------

static com_dbgErrName_t  gErrorNameSelect[] = {
    { COM_ERR_SOCKETNG,     "COM_ERR_SOCKETNG" },
    { COM_ERR_BINDNG,       "COM_ERR_BINDNG" },
    { COM_ERR_CONNECTNG,    "COM_ERR_CONNECTNG" },
    { COM_ERR_LISTENNG,     "COM_ERR_LISTENNG" },
    { COM_ERR_ACCEPTNG,     "COM_ERR_ACCEPTNG" },
    { COM_ERR_SENDNG,       "COM_ERR_SENDNG" },
    { COM_ERR_RECVNG,       "COM_ERR_RECVNG" },
    { COM_ERR_SELECTNG,     "COM_ERR_SELECTNG" },
    { COM_ERR_CLOSENG,      "COM_ERR_CLOSENG" },
    { COM_ERR_GETIFINFO,    "COM_ERR_GETIFINFO" },
    { COM_ERR_END,          "" }  // 最後は必ずこれで
};

static void finalizeSelect( void )
{
    COM_DEBUG_AVOID_START( COM_NO_SKIPMEM );
    for( com_selectId_t id = 0;  id < gEventId;  id++ ) {
        com_eventInf_t*  tmp = &(gEventInf[id]);
        if( !tmp->isUse ) {continue;}
        if( tmp->timer != COM_NO_SOCK ) {com_cancelTimer( id );}
        if( tmp->sockId != COM_NO_SOCK ) {
            if( tmp->sockId != ID_STDIN ) {com_deleteSocket( id );}
            else {com_cancelStdin( id );}
        }
    }
    com_skipMemInfo( true );
    com_free( gEventInf );
    freeIfInfo();
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

void com_initializeSelect( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    atexit( finalizeSelect );
    com_registerErrorCode( gErrorNameSelect );
    gRecvBuf = gDefaultRecvBuf;
    gRecvBufSize = sizeof(gDefaultRecvBuf);
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

