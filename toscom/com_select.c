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

static pthread_mutex_t gMutexEvent = PTHREAD_MUTEX_INITIALIZER;
static com_selectId_t  gEventId = 0;       // 現在のイベント登録数
static com_eventInf_t* gEventInf = NULL;   // イベント情報実体

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
    if( iInf->isUse ) { return false; }
    // タイマーイベント用なら、前回タイマーイベントの空き情報は使わない
    if( iIsTimer ) {if( iInf->expireFunc ) { return false; }}
    // ソケットイベント用なら、前回ソケットイベントの空き情報は使わない
    else {if( !iInf->expireFunc ) { return false; }}
    return true;
}

static com_selectId_t selectEventId( BOOL iIsTimer )
{
    // 未使用領域がある時は、それを利用する
    com_mutexLock( &gMutexEvent, __func__ );
    com_selectId_t id = 0;  // for文外でも使うため、for文外で定義
    for( ;  id < gEventId;  id++ ) {
        if( checkAvailInf( &(gEventInf[id]), iIsTimer ) ) { break; }
    }
    return id;
}

static com_selectId_t getEventId( BOOL iIsTimer )
{
    // 未使用領域がない時は、末尾に情報を追加
    com_selectId_t id = selectEventId( iIsTimer );
    if( id == gEventId ) {
        BOOL result = com_realloct( &gEventInf, sizeof(*gEventInf), &gEventId,
                                    1, "new event inf(%ld)", gEventId );
        if( !result ) { UNLOCKRETURN( COM_NO_SOCK ); }
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
    COM_MOD_STDIDON,            // 標準入力受付登録
    COM_MOD_STDIN,              // 標準入力受付
    COM_MOD_STDINOFF,           // 標準入力受付解除
    COM_MOD_TIMERON,            // タイマー起動
    COM_MOD_EXPIRED,            // タイマー満了
    COM_MOD_TIMERMOD,           // タイマー修正
    COM_MOD_TIMEROFF,           // タイマー解除
    COM_MOD_NOEVENT     // 最後は必ずこれで
} COM_MOD_EVENT_t;

static char* gModEvent[] = {
    "socket", "bind", "listen", "connect", "accept",
    "send", "receive", "drop", "close_self", "closed_by_dst",
    "stdin_on", "stdin", "stdin_off",
    "timer_on", "timer_expired", "timer_reset", "timer_off"
};

static BOOL gDebugEvent = true;

void com_setDebugSelect( BOOL iMode )
{
    gDebugEvent = iMode;
}

static void printAddr( com_sockaddr_t *iInf, const char *iLabel )
{
    char* addr;
    char* port;
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
    char startDate[COM_DATE_DSIZE];
    char startTime[COM_TIME_DSIZE];
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
    char* prot[] = {
        "", "rawSnd", "rawRcv", "Netlink", "UDP", "TCP_CLIENT", "TCP_SERVER"
    };
    com_dbgCom( "      sockId = %d", iInf->sockId );
    char* addLabel = getAddLabel( iModEvent, iInf );
    if( !addLabel ) { com_dbgCom( "        type = %s", prot[iInf->type] ); }
    else { com_dbgCom( "        type = %s (%s)", prot[iInf->type], addLabel ); }
    if( iModEvent == COM_MOD_SEND ) {
        printSignal( &iInf->srcInf, &iInf->dstInf, iData, iSize );
    }
    else if( iModEvent == COM_MOD_RECEIVE || iModEvent == COM_MOD_DROP ) {
        printSignal( &iInf->dstInf, &iInf->srcInf, iData, iSize );
    }
    else {
        printAddr( &iInf->srcInf, "src" );
        if( iInf->type > COM_SOCK_UDP ) { printAddr( &iInf->dstInf, "dst" ); }
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
    if( !com_getDebugPrint() || !gDebugEvent ) { return; }
    com_dbgCom( "==SELECT %s (ID=%ld)", gModEvent[iModEvent], iId );
    if( iModEvent == COM_MOD_STDIN ) { logStdin( iData, iSize ); }
    else if( iModEvent <= COM_MOD_CLOSED ) {
        logSocket( iModEvent, iInf, iData, iSize );
    }
    else if( iModEvent >= COM_MOD_TIMERON ) { logTimer( iInf ); }
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
        const struct addrinfo_ll *iSrc,
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
    if( !iSrc ) { *oProtocol = NETLINK_ROUTE; }
    else { *oProtocol = *iSrc; }
}
#endif // LINUXOS

static void setSockCondUnix(
        COM_SOCK_TYPE_t iType, const struct sockaddr_un *iSun,
        int *oFamily, int *oType, int *oProtocol )
{
    *oFamily = AF_UNIX;
    *oType = (iType == COM_SOCK_UDP) ? SOCK_DGRAM : SOCK_STREAM;
    *oProtocol = 0;
    if( com_checkExistFile( iSun->sun_path ) ) { remove( iSun->sun_path ); }
}

static BOOL isUnixSocket( const void *iSrc )
{
    if( !iSrc ) { return false; }
    const struct sockaddr_un* sun = iSrc;
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
    BOOL result = true;
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

static const int YES = 1;

#define SETSOOPT( OPT ) \
    setsockopt( iSockId, proto, (OPT), &YES, sizeof(YES) )

static BOOL setRawsndOpt( int iSockId, const struct addrinfo *iSrc )
{
    if( iSrc->ai_family == AF_INET ) {
        int proto = IPPROTO_IP;
        if( 0 > SETSOOPT( IP_HDRINCL ) ) {  return sockoptNG( "IP_HDRINCL" ); }
    }
    const int buf = getSndBufSize();
    if( 0 > setsockopt( iSockId, SOL_SOCKET, SO_SNDBUF, &buf, sizeof(buf) ) ) {
        return sockoptNG( "SO_SNDBUF" );
    }
    return true;
}

static BOOL setSocketOptions(
        COM_SOCK_TYPE_t iType, int iSockId, const struct addrinfo *iSrc,
        const com_sockopt_t *iOpt )
{
    if( iOpt ) { return setUserOpt( iSockId, iOpt ); }
    if( iType == COM_SOCK_RAWRCV ) { return true; }  // オプション設定なし
    if( iType == COM_SOCK_RAWSND ) { return setRawsndOpt( iSockId, iSrc ); }
    if( iType == COM_SOCK_NETLINK || isUnixSocket( iSrc ) ) { return true; }
    int proto = (iSrc->ai_family == AF_INET) ? IPPROTO_IP : IPPROTO_IPV6;
    // UDP/TCPのIP通信ソケットの設定開始
    if( proto == IPPROTO_IPV6 ) {
        if( 0 > SETSOOPT( IPV6_V6ONLY ) ) { return sockoptNG( "IPV6_V6ONLY" ); }
    }
    proto = SOL_SOCKET;
    if( 0 > SETSOOPT( SO_REUSEADDR ) ) { return sockoptNG( "SO_REUSEADDR" ); }
    return true;
}

static BOOL createSocket(
        COM_SOCK_TYPE_t iType, com_selectId_t iId, com_eventInf_t *oInf,
        const void *iSrc, const com_sockopt_t *iOpt )
{
    int family, type, protocol;
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
    int ret = bind( oInf->sockId, iAddr, iLen );
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
    if( iSrc->sll_ifindex < 0 ) { return true; }
    struct sockaddr_ll sll;
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
    const struct addrinfo* src = iSrc;
    com_copyAddr( &(oInf->srcInf), src );
    return execBind( iId, oInf, src->ai_addr, src->ai_addrlen );
}

static BOOL startTcpClient(
        com_eventInf_t *oInf, const struct addrinfo *iDst )
{
    const void* addr = NULL;
    socklen_t len = 0;
    if( !oInf->isUnix ) { addr = iDst->ai_addr;  len = iDst->ai_addrlen; }
    else { addr = iDst;  len = SUN_LEN((const struct sockaddr_un*)iDst); }
    if( 0 > connect( oInf->sockId, addr, len ) ) {
        com_error( COM_ERR_CONNECTNG,
                   "fail to connect tcp client[%s]", com_strerror(errno) );
        return false;
    }
    if( !oInf->isUnix ) { com_copyAddr( &(oInf->dstInf), iDst ); }
    return true;
}

static BOOL startTcpServer( com_eventInf_t *oInf )
{
    int ret = listen( oInf->sockId, 128 );
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
        struct sockaddr_un* sun = (void*)(&oInf->srcInf.addr);
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
    BOOL result = true;
    COM_MOD_EVENT_t event = COM_MOD_NOEVENT;
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
    if( !result ) { return COM_NO_SOCK; }
    // TCP接続のイベントログは接続成功時しか出さないようにする
    if( iType > COM_SOCK_UDP ) { debugEventLog( event, iId, oInf, NULL, 0 ); }
    return iId;
}

// イベント関数NULL指定時のダミー関数
static BOOL dummyEventFunc(
        com_selectId_t iId, COM_SOCK_EVENT_t iEvent,
        void *iData, size_t iDataSize )
{
    if( iEvent == COM_EVENT_ACCEPT ) { com_dbgFuncCom( "ACCEPT(%ld)", iId ); }
    if( iEvent == COM_EVENT_CLOSE )  { com_dbgFuncCom( "CLOSE(%ld)",  iId ); }
    if( iEvent == COM_EVENT_RECEIVE ) {
        com_dumpCom( iData, (size_t)iDataSize, "RECEIVE(%ld)", iId );
    }
    return true;
}

static BOOL checkCreatePrm(
        COM_SOCK_TYPE_t iType, const void *iSrcInf, const void *iDstInf )
{
#ifndef LINUXOS
    if( iType <= COM_SOCK_NETLINK ) { return false; }
#endif
    if( !iSrcInf && iType != COM_SOCK_NETLINK ) { return false; }
    if( !iDstInf && iType == COM_SOCK_TCP_CLIENT ) { return false; }
    return true;
}

com_selectId_t com_createSocket(
        COM_SOCK_TYPE_t iType, const void *iSrcInf,
        const com_sockopt_t *iOpt, com_sockEventCB_t iEventFunc,
        const void *iDstInf )
{
    if( !checkCreatePrm( iType, iSrcInf, iDstInf ) ) {COM_PRMNG(COM_NO_SOCK);}
    com_selectId_t id = selectEventId( false );  // 仮ID取得
    if( !iEventFunc ) { iEventFunc = dummyEventFunc; }
    com_eventInf_t inf = { .isUse = true,  .timer = COM_NO_SOCK,
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
    if( id == COM_NO_SOCK ) { return closeSocketEnd( &inf ); }
    gEventInf[id] = inf;
    UNLOCKRETURN( id );
}

static com_eventInf_t *checkSocketInf( com_selectId_t iId, BOOL iLock )
{
    // returnSocketInf()で排他アンロックする想定
    if( iLock ) { com_mutexLock( &gMutexEvent, __func__ ); }
    if( iId < 0 || iId > gEventId ) { return NULL; }
    com_eventInf_t* tmp = &(gEventInf[iId]);
    if( !tmp->isUse || tmp->sockId == COM_NO_SOCK ) { return NULL; }
    return tmp;
}

BOOL com_deleteSocket( com_selectId_t iId )
{
    com_eventInf_t* tmp = checkSocketInf( iId, true );
    if( !tmp ) { com_prmNG(NULL); UNLOCKRETURN( false ); }
    if( tmp->sockId == ID_STDIN ) { UNLOCKRETURN( false ); }
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
    int ret = 0;
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
    else { ret = send( tmp->sockId, iData, iDataSize, 0 ); }
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
    if( 0xffff == (answer = ~sum) ) { return 0; }
    return answer;
}

static BOOL checkNoRecv( BOOL iNonBlock, int iErrno )
{
    if( !iNonBlock ) { return false; }
    if( iErrno == EAGAIN ) { return false; }
    // EAGAIN と EWOULDBLOCK が同値宣言されている場合を考慮
    if( EAGAIN != EWOULDBLOCK ) {if(iErrno == EWOULDBLOCK) {return false;}}
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
        if( iInf->isListen ) { return COM_RECV_ACCEPT; }
        if( checkNoRecv( iNonBlock, iErrno ) ) { return COM_RECV_NODATA; }
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
    com_eventInf_t* tmp = checkSocketInf( iId, false );
    if( !tmp || !oData || !oLen ) {COM_PRMNG(COM_RECV_NG);}
    struct sockaddr* srcAddr = NULL;
    socklen_t* addrLen = NULL;
    if( oFrom ) {
        memset( oFrom, 0, sizeof(*oFrom) );
        srcAddr = (struct sockaddr*)(&oFrom->addr);
        oFrom->len = sizeof(oFrom->addr);
        addrLen = &(oFrom->len);
    }
    errno = 0;
    if( tmp->isUnix ) { *oLen = read( tmp->sockId, oData, iDataSize ); }
    else {
        int flags = iNonBlock ? MSG_DONTWAIT : 0;
        *oLen = recvfrom( tmp->sockId, oData, iDataSize, flags,
                          srcAddr, addrLen );
    }
    return checkRecvResult( iId, *oLen, errno, tmp, iNonBlock, oData );
}

BOOL com_filterSocket( com_selectId_t iId, com_sockFilterCB_t iFilterFunc )
{
    com_eventInf_t* tmp = checkSocketInf( iId, false );
    if( !tmp ) {COM_PRMNG(false);}
    tmp->filterFunc = iFilterFunc;
    return true;
}

int com_getSockId( com_selectId_t iId )
{
    com_eventInf_t* tmp = checkSocketInf( iId, true );
    if( !tmp ) { com_prmNG(NULL); UNLOCKRETURN( false ); }
    // タイマーならNG返却
    if( tmp->expireFunc ) { UNLOCKRETURN( COM_NO_SOCK ); }
    // 標準入力なら 0返却
    if( tmp->stdinFunc ) { UNLOCKRETURN( 0 ); }
    // ソケットなので ID返却
    UNLOCKRETURN( tmp->sockId );
}

static com_sockaddr_t *getAddrInf( com_selectId_t iId, BOOL iIsSrc )
{
    com_eventInf_t* tmp = checkSocketInf( iId, true );
    if( !tmp ) { com_prmNG(NULL); UNLOCKRETURN( NULL ); }
    if( iIsSrc ) { UNLOCKRETURN( &tmp->srcInf ); }
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
    com_selectId_t id = getEventId( true );
    com_skipMemInfo( false );
    if( id == COM_NO_SOCK ) { UNLOCKRETURN( COM_NO_SOCK ); }
    com_eventInf_t* inf = &(gEventInf[id]);
    *inf = (com_eventInf_t){
        .isUse = true,  .timer = iTimer,  .expireFunc = iExpireFunc,
        .sockId = COM_NO_SOCK
    };
    if( !com_gettimeofday( &(inf->startTime), "start timer" ) ) {
        inf->isUse = false;
        UNLOCKRETURN( COM_NO_SOCK );
    }
    debugEventLog( COM_MOD_TIMERON, id, inf, NULL, 0 );
    UNLOCKRETURN( id );
}

static com_eventInf_t *checkTimerInf(
        com_selectId_t iId, BOOL iUse, BOOL iLock )
{
    if( iId < 0 || iId > gEventId ) { return NULL; }
    if( iLock ) { com_mutexLock( &gMutexEvent, __func__ ); }
    com_eventInf_t* tmp = &(gEventInf[iId]);
    if( tmp->timer == COM_NO_SOCK ) { return NULL; }
    if( iUse && !tmp->isUse ) { return NULL; }
    return tmp;
}

static void cancelTimer( com_eventInf_t *oInf )
{
    // 今の所 isUse のフラグを落とすだけ
    oInf->isUse = false;
}

BOOL com_cancelTimer( com_selectId_t iId )
{
    com_eventInf_t* tmp = checkTimerInf( iId, true, true );
    if( !tmp ) {com_prmNG(NULL); UNLOCKRETURN(false); }
    cancelTimer( tmp );
    debugEventLog( COM_MOD_TIMEROFF, iId, tmp, NULL, 0 );
    UNLOCKRETURN( true );
}

static BOOL expireTimer( com_selectId_t iId )
{
    com_eventInf_t* inf = &(gEventInf[iId]);
    debugEventLog( COM_MOD_EXPIRED, iId, inf, NULL, 0 );
    cancelTimer( inf );
    return (inf->expireFunc)( iId );
}

#define TIMEUNIT (1000000L)

static long calcMsec( struct timeval *iTime )
{
    if( !iTime ) { return 0; }
    return (TIMEUNIT * iTime->tv_sec + iTime->tv_usec);
}

static long calculateRestTime( com_selectId_t iId, struct timeval *iNow )
{
    com_eventInf_t* tmp = &(gEventInf[iId]);
    long pastTime = calcMsec( iNow ) - calcMsec( &(tmp->startTime) );
    return (1000L * tmp->timer - pastTime);
}

static BOOL checkTimer( com_selectId_t iId, struct timeval *iNow )
{
    if( !checkTimerInf( iId, true, false ) ) { return true; }
    if( 0 < calculateRestTime( iId, iNow ) ) { return true; }
    return expireTimer( iId );
}

static struct timeval *getCurrentTime( struct timeval *oNow )
{
    if( !com_gettimeofday( oNow, "time for events" ) ) { return NULL; }
    return oNow;
}

#define GET_CURRENT \
    struct timeval  tmpTime; \
    struct timeval* current = getCurrentTime( &tmpTime ); \
    do{} while(0)

BOOL com_checkTimer( com_selectId_t iId )
{
    BOOL result = true;
    GET_CURRENT;
    if( !current ) { return false; }
    for( com_selectId_t id = 0;  id < gEventId;  id++ ) {
        if( iId == COM_ALL_TIMER || iId == id ) {
            if( !checkTimer( id, current ) ) { result = false; }
        }
    }
    return result;
}

BOOL com_resetTimer( com_selectId_t iId, long iTimer )
{
    com_eventInf_t* tmp = checkTimerInf( iId, false, true );
    if( !tmp || iTimer < 1 ) {com_prmNG(NULL); UNLOCKRETURN(false); }
    if( iTimer > 0 ) { tmp->timer = iTimer; }
    tmp->isUse = true;
    if( !com_gettimeofday( &(tmp->startTime), "reset timer time" ) ) {
        tmp->isUse = false;
    }
    if( tmp->isUse ) { debugEventLog( COM_MOD_TIMERMOD, iId, tmp, NULL, 0 ); }
    UNLOCKRETURN( tmp->isUse );
}



// イベント待受処理 ----------------------------------------------------------





// アドレス情報取得/解放 -----------------------------------------------------

static size_t calcSize( struct addrinfo *oInfo )
{
    size_t size = sizeof( *oInfo );
    if( (size_t)oInfo->ai_addrlen > sizeof( *oInfo ) ) {
        size += oInfo->ai_addrlen;
    }
    else { size += sizeof( struct sockaddr ); }
    if( oInfo->ai_canonname ) { size += strlen( oInfo->ai_canonname ) + 1; }
    return size;
}

static BOOL checkPrmGetaddrinfo(
        struct addrinfo **oTarget, COM_SOCK_TYPE_t iType, int iFamily )
{
    if( !oTarget ) { return false; }
    if( iType < COM_SOCK_RAWSND || iType > COM_SOCK_TCP_SERVER ) {return false;}
    if( iFamily != AF_INET && iFamily != AF_INET6 ) { return false; }
    return true;
}

BOOL com_getaddrinfoFunc(
        struct addrinfo **oTarget, COM_SOCK_TYPE_t iType, int iFamily,
        const char *iAddress, ushort iPort, COM_FILEPRM )
{
    if( !checkPrmGetaddrinfo( oTarget, iType, iFamily ) ) {COM_PRMNG(false);}
    char portText[6] = {0};  // ポート番号は最大5桁 + 文字列終端
    snprintf( portText, sizeof(portText), "%d", iPort );
    struct addrinfo hints = {
        .ai_family = iFamily,
        .ai_socktype = (iType <= COM_SOCK_UDP) ? SOCK_DGRAM : SOCK_STREAM,
        .ai_flags = AI_NUMERICSERV
    };
    *oTarget = NULL;
    int result = getaddrinfo( iAddress, portText, &hints, oTarget );
    if( result ) {
        com_error(COM_ERR_DEBUGNG,"getaddrinfo() NG [%s]", com_strerror(errno));
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
    if( !(*oTarget) ) { return; }
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
    struct sockaddr_storage dmy;
    if( 1 == inet_pton( AF_INET,  iString, &dmy ) ) { return COM_IPV4; }
    if( 1 == inet_pton( AF_INET6, iString, &dmy ) ) { return COM_IPV6; }
    return COM_NOTIP;
}

BOOL com_valIpAddress( char *ioData, void *iCond )
{
    com_valCondIpAddress_t* cond = iCond;
    if( !cond ) { cond = &(com_valCondIpAddress_t){ true, true }; }
    COM_AF_TYPE_t af = com_isIpAddr( ioData );
    if( cond->ipv4 && af == COM_IPV4 ) { return true; }
    if( cond->ipv6 && af == COM_IPV6 ) { return true; }
    return false;
}

BOOL com_valIpAddressCondCopy( void **oCond, void *iCond )
{
    com_valCondIpAddress_t* source = iCond;
    com_valCondIpAddress_t* tmp =
        com_malloc(sizeof(com_valCondIpAddress_t), "copy ip address condition");
    if( !tmp ) { return false; }
    *tmp = *source;
    *oCond = tmp;
    return true;
}

static socklen_t getAddrSin( void *iSockAddr, void **oAddr )
{
    struct sockaddr_in* sin = iSockAddr;
    *oAddr = &(sin->sin_addr.s_addr);
    return sizeof(sin->sin_addr.s_addr);
}

static socklen_t getAddrSin6( void *iSockAddr, void **oAddr )
{
    struct sockaddr_in6* sin6 = iSockAddr;
    *oAddr = &(sin6->sin6_addr.s6_addr);
    return sizeof(sin6->sin6_addr.s6_addr);
}

#ifdef LINUXOS
static socklen_t getAddrLl( void *iSockAddr, void **oAddr )
{
    struct sockaddr_ll* sll = iSockAddr;
    *oAddr = &(sll->sll_addr);
    return sizeof(sll->sll_addr);
}
#endif

static socklen_t getAddrUn( void *iSockAddr, void **oAddr )
{
    struct sockaddr_un* sun = iSockAddr;
    *oAddr = &(sun->sun_path);
    return strlen(sun->sun_path);
}

static socklen_t getAddrLen( void *iSockAddr, void **oAddr )
{
    struct sockaddr* sa = iSockAddr;
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
    void* addr = NULL;
    socklen_t len = getAddrLen( iSockAddr, &addr );
    if( !len ) { return false; }
    return !memcmp( iTarget, addr, len );
}

static BOOL compareSockFamily(
        struct sockaddr *iTarget, struct sockaddr *iSource )
{
    return (iTarget->sa_family == iSource->sa_family);
}

static BOOL compareSockAddr( void *iTarget, void *iSource )
{
    void* addrTarget = NULL;
    socklen_t lenTarget = getAddrLen( iTarget, &addrTarget );
    if( !lenTarget ) { return false; }
    void* addrSource = NULL;
    socklen_t lenSource = getAddrLen( iSource, &addrSource );
    if( !lenSource ) { return false; }
    if( lenTarget != lenSource ) { return false; }
    return !memcmp( addrTarget, addrSource, lenSource );
}

BOOL com_compareSock( void *iTargetSock, void *iSockAddr )
{
    if( !iTargetSock || !iSockAddr ) {COM_PRMNG(false);}
    if( !compareSockFamily( iTargetSock, iSockAddr ) ) { return false; }
    return compareSockAddr( iTargetSock, iSockAddr );
}

static BOOL isIp( struct sockaddr *iSock )
{
    if( iSock->sa_family == AF_INET )  { return true; }
    if( iSock->sa_family == AF_INET6 ) { return true; }
    return false;
}

static ushort getSockPort( void *iSockAddr )
{
    struct sockaddr* sa = iSockAddr;
    if( sa->sa_family == AF_INET ) {
        struct sockaddr_in* sin = iSockAddr;
        return sin->sin_port;
    }
    if( sa->sa_family == AF_INET6 ) {
        struct sockaddr_in6* sin6 = iSockAddr;
        return sin6->sin6_port;
    }
    return 0;
}

static BOOL compareSockPort( void *iTarget, void *iSource )
{
    ushort portTarget = getSockPort( iTarget );
    ushort portSource = getSockPort( iSource );
    return (portTarget == portSource);
}

BOOL com_compareSockWithPort( void *iTargetSock, void *iSockAddr )
{
    if( !iTargetSock || !iSockAddr ) {COM_PRMNG(false);}
    if( !isIp(iTargetSock) || isIp(iSockAddr) ) { return false; }
    if( !compareSockFamily( iTargetSock, iSockAddr ) ) { return false; }
    if( !compareSockPort( iTargetSock, iSockAddr ) ) { return false; }
    return compareSockAddr( iTargetSock, iSockAddr );
}



// IF情報データ定義 ----------------------------------------------------------

static struct ifaddrs* gIfAddr = NULL;
static com_ifinfo_t* gIfInfo = NULL;
static long gIfInfoCnt = 0;
static int gIfInfoSock = COM_NO_SOCK;  // 情報取得用に生成するソケット

#ifndef LINUXOS
#define  ETH_ALEN  6
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

static void setIfInfo( com_ifinfo_t *oInf, char *iIfname )
{
    oInf->ifname = iIfname;
    if( !openSocketForInfo() ) { return; }
    struct ifreq ifr;
    strcpy( ifr.ifr_name, iIfname );
    if( 0 > ioctl( gIfInfoSock, (int)SIOCGIFINDEX, &ifr ) ) {
        com_error( COM_ERR_GETIFINFO, "fail to get index for %s", iIfname );
    }
    else { oInf->ifindex = ifr.ifr_ifindex; }
    if( 0 > ioctl( gIfInfoSock, (int)SIOCGIFHWADDR, &ifr ) ) {
        com_error( COM_ERR_GETIFINFO, "fail to get hwaddr for %s", iIfname );
    }
    else {
        memcpy( oInf->hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN );
        oInf->h_len = ETH_ALEN;
    }
}

static com_ifinfo_t *getIfInfo( char *iIfname )
{
    for( long i = 0;  i < gIfInfoCnt;  i++ ) {
        if( !strcmp( gIfInfo[i].ifname, iIfname ) ) { return &(gIfInfo[i]); }
    }
    com_ifinfo_t* newInfo =
        com_reallocAddr( &gIfInfo, sizeof(*gIfInfo), COM_TABLEEND,
                         &gIfInfoCnt, 1, "ifinfo list" );
    if( newInfo ) { setIfInfo( newInfo, iIfname ); }
    return newInfo;
}

static void freeIfInfo( void )
{
    if( gIfAddr ) { freeifaddrs( gIfAddr );  gIfAddr = NULL; }
    if( gIfInfoSock >= 0 ) { close( gIfInfoSock );  gIfInfoSock = COM_NO_SOCK; }
    if( !gIfInfoCnt ) { return; }
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
    struct ifaddrs* ifaddr;
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
    com_ifinfo_t* inf = getIfInfo( iIfa->ifa_name );
    if( !inf ) { return false; }
    struct ifaddrs** ifAddr =
        com_reallocAddr( &inf->ifaddrs, sizeof(struct ifaddrs), 0,
                         &inf->cntAddrs, ADDRCNT_ADD, "addAddrInf (ifaddr)" );
    if( !ifAddr ) { return false; }
    *ifAddr = iIfa;
    com_ifaddr_t* soAddrs = reallocSync( &inf->soAddrs, sizeof(*soAddrs),
                                         inf->cntAddrs, "sockaddr" );
    if( !soAddrs ) { return false; }
    struct sockaddr* sa = iIfa->ifa_addr;
    soAddrs->addr = (sa->sa_family == AF_INET || sa->sa_family == AF_INET6)
                    ? sa : NULL;
    return true;
}

static long collectIfInfo( void )
{
    if( !(gIfAddr = getifaddrsFunc()) ) { return COM_IFINFO_ERR; }
    for( struct ifaddrs* ifa = gIfAddr;  ifa;  ifa = ifa->ifa_next ) {
        if( !ifa->ifa_addr ) { continue; }
        if( !addIfAddr( ifa ) ) { freeIfInfo();  return COM_IFINFO_ERR; }
    }
    return gIfInfoCnt;
}
        



long com_collectIfInfo( com_ifinfo_t **oInf, BOOL iUseNetlink )
{
#ifndef LINUXOS
    if( iUseNetlink ) {COM_PRMNG(COM_IFINFO_ERR); }
#endif
    if( !oInf ) {COM_PRMNG(COM_IFINFO_ERR);}
    com_skipMemInfo( true );
    freeIfInfo();
    long cnt = 0;
    *oInf = NULL;
    long(*func)(void) = collectIfInfo;
#ifdef LINUXOS
    if( iUseNetlink ) { func = getIfInfoByNetlink; }
#endif
    if( 0 <= (cnt = func()) ) { *oInf = gIfInfo; }
    com_skipMemInfo( false );
    return cnt;
}

long com_getIfInfo( com_ifinfo_t **oInf, BOOL iUseNetlink )
{
#ifndef LINUXOS
    if( iUseNetlink ) {COM_PRMNG(COM_IFINFO_ERR);}
#endif
    if( !oInf ) {COM_PRMNG(COM_IFINFO_ERR);}
    long cnt = gIfInfoCnt;
    if( !(*oInf = gIfInfo) ) { cnt = com_collectIfInfo( oInf, iUseNetlink ); }
    return cnt;
}





// デバッグ用イベント情報出力 ------------------------------------------------

BOOL com_getnameinfo(
        char **oAddr, char **oPort, const void *iAddr, socklen_t iLen )
{
    if( !iAddr ) {COM_PRMNG(false);}

    static char hbuf[COM_LINEBUF_SIZE];
    static char sbuf[COM_WORDBUF_SIZE];
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




// 初期化処理 ----------------------------------------------------------------

static com_dbgErrName_t gErrorNameSelect[] = {
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
#ifdef TO_BE_DEVELOPPED
    for( com_selectId_t id = 0;  id < gEventId;  id++ ) {
        com_eventInf_t* tmp = &(gEventInf[id]);
        if( !tmp->isUse ) { continue; }
        if( tmp->timer != COM_NO_SOCK ) { com_cancelTimer( id ); }
        if( tmp->sockId != COM_NO_SOCK ) {
            if( tmp->sockId != ID_STDIN ) { com_deleteSocket( id ); }
            else { com_cancelStdin( id ); }
        }
    }
#endif
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
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

