/*
 *****************************************************************************
 *
 * サンプルコミュニケーター  共通メインソース
 *
 *   Ver1・Ver2で共通となるコードを記述する。
 *   差分が発生する場合は、その部分を関数に切り出し、smpl_spec～()という名前で
 *   Ver1・Ver2の双方に記述する想定となる。
 *
 *   完全にどちらかにしかない場合は、遠慮せずに該当する方にだけコードを書けば
 *   良い。そういったものは、現状 Ver2のウィンドウ関連処理しか無い。
 *
 *****************************************************************************
 */

#include "smpl_if.h"
#include "smpl_com.h"
#include "smpl_spec.h"


///// データ定義 /////////////////////////////////////////////////////////////

// プロフィール構造体
typedef struct {
    char    handle[SMPL_HANDLE_MAX];         // ハンドルネーム
    char    addr[SMPL_ADDR_SIZE];            // 自listenソケットアドレス
    ushort  port;                            // 自listenソケットポート番号
    ushort  conCount;                        // コネクト回数
    int dummyFor64bit;
    com_selectId_t  sid;                     // 通信先ID
} smpl_profile_t;

static smpl_profile_t   gMyProf;
static smpl_profile_t*  gDstList = NULL;
static long  gDstCount = 0;
static long  gDstId = COM_NO_SOCK;
static struct addrinfo* gDstInf = NULL;
static BOOL gBroadcast = false;

static com_selectId_t  gListenId = COM_NO_SOCK;
static com_selectId_t  gStdinId  = COM_NO_SOCK;

static const char*  gMyListFile = "mylist";
static smpl_profile_t*  gMyList = NULL;
static long gMyListCount = 0;

static char gChatLog[SMPL_CHATLOG_SIZE] = "";
static FILE* gChatFp = NULL;

#define CHATLOG( ... )   fprintf( gChatFp, __VA_ARGS__ )
#define CHATFLUSH        fflush( gChatFp )


///// 共通リソース関連処理 ///////////////////////////////////////////////////

static void freeResource( void )
{
    // ソケット削除は comモジュールに任せる
    com_freeaddrinfo( &gDstInf );
    com_free( gDstList );
    com_free( gMyList );
}

static inline char *getCurTime( void )
{
    static char timeText[COM_TIME_DSIZE];
    com_getCurrentTime( COM_FORM_DETAIL, NULL, timeText, NULL );
    return timeText;
}

static void closeChatLog( void )
{
    if( !gChatFp ) { return; }
    CHATLOG( "<<<<< CLOSE(%s [%s]:%d) at %s >>>>>\n",
             gMyProf.handle, gMyProf.addr, gMyProf.port, getCurTime() );
    CHATFLUSH;
    (void)com_fclose( gChatFp );
}

static void exitPack( long iCode )
{
    freeResource();
    closeChatLog();
    com_exit( iCode );
}

#define ERRRET( ERRCODE, ... ) \
    com_error( (ERRCODE), __VA_ARGS__ ); \
    exitPack( ERRCODE ); \
    do{} while(0)

static void makeChatLogName( void )
{
    snprintf( gChatLog, sizeof(gChatLog), "chatlog.%s", gMyProf.handle );
}

static void openChatLog( void )
{
    makeChatLogName();
    gChatFp = com_fopen( gChatLog, "a" );
    if( !gChatFp ) { ERRRET( COM_ERR_LOGGINGNG, "fail to open chat log..." ); }
    CHATLOG( "\n<<<<< OPEN(%s [%s]:%d) at %s >>>>>\n",
             gMyProf.handle, gMyProf.addr, gMyProf.port, getCurTime() );
    CHATFLUSH;
}


///// ツール内 各イベント処理 ////////////////////////////////////////////////

static BOOL gPrompt = true;       // プロンプト表示有無

enum { SMPL_SYSMSG_SIZE = 32 };   // システムメッセージ最大長

static char gSysMsgBuff[SMPL_SYSMSG_SIZE];

static void procSysHELLO( smpl_profile_t *oProf, char *iDst, int iNum )
{
    char* curTime = getCurTime();
    com_strcpy( oProf->handle, iDst );
    oProf->port = (ushort)com_atoi( iDst + SMPL_HANDLE_MAX );
    SMPL_PRINT( "\n--- %d:%s [%s]:%d entered at %s ---\n",
                iNum, iDst, oProf->addr, oProf->port, curTime );
    CHATLOG( "%s <<%d:%s [%s]:%d entered>>\n",
             curTime, iNum, iDst, oProf->addr, oProf->port );
    CHATFLUSH;
    gPrompt = true;  // HELLOを受信したらプロンプト表示復帰
}

static void procSysRENAME( smpl_profile_t *oProf, char *iName, int iNum )
{
    char* curTime = getCurTime();
    SMPL_PRINT( "\n--- %d:%s renamed ", iNum, oProf->handle );
    CHATLOG( "%s <<%d:%s -> ", curTime, iNum, oProf->handle );
    com_strcpy( oProf->handle, iName );
    SMPL_PRINT( "%d:%s at %s ---\n", iNum, iName, curTime );
    CHATLOG( "%s renamed>>\n", iName );
    CHATFLUSH;
}

// SMPL_SYSMSG_t に追加したら、必ず gSysMsgList[]にも追加すること
typedef enum {
    SMPL_SYSMSG_NONE       = 0x0000,   // 非システムメッセージ
    SMPL_SYSMSG_HELLO      = 0x0001,   // 自分のハンドルとポート番号を通知
    SMPL_SYSMSG_RENAME     = 0x0002,   // ハンドル名変更を通知
} SMPL_SYSMSG_t;

// システムメッセージ受信処理関数プロトタイプ
typedef void(*smpl_sysMsg_t)( smpl_profile_t *oProf, char *iName, int iNum );

// システムメッセージ制御データ構造体
typedef struct {
    SMPL_SYSMSG_t  type;
    char name[20];
    smpl_sysMsg_t  func;
} smpl_sysMsg_func_t;

static smpl_sysMsg_func_t  gSysMsgList[] = {
    { SMPL_SYSMSG_HELLO,    "HELLO",    procSysHELLO },
    { SMPL_SYSMSG_RENAME,   "RENAME",   procSysRENAME },
    { SMPL_SYSMSG_NONE,     "",         NULL }  // 最後は必ずこれで
};

static void makeSysMsg( SMPL_SYSMSG_t iType, const char *iMsg, size_t iSize )
{
    COM_CLEAR_BUF( gSysMsgBuff );
    gSysMsgBuff[0] = (char)((iType & 0xff00) >> 8);
    gSysMsgBuff[1] = (char)(iType & 0x00ff);
    memcpy( gSysMsgBuff + 2, iMsg, iSize );
}

static smpl_sysMsg_func_t *getSysMsgCode( char *iMsg )
{
    uint type = (iMsg[0] << 8) + iMsg[1];
    for( smpl_sysMsg_func_t* tmp = gSysMsgList;  tmp->type;  tmp++ ) {
        if( tmp->type == type ) { return tmp; }
    }
    return NULL;
}

char *smpl_makeDstLabel( int iNum, const char *iHandle )
{
    static char label[SMPL_HANDLE_MAX + 6 + 5];
    if( iHandle ) { snprintf( label, sizeof(label), "%d:%s", iNum, iHandle ); }
    else { com_strcpy( label, "BROADCAST" ); }
    return label;
}

static void writeChatLog( BOOL isSend, char *iDst, char *curTime, char *iMsg )
{
    CHATLOG( "%s ", curTime );
    if( isSend ) { CHATLOG( "[%s -> %s]", gMyProf.handle, iDst ); }
    else         { CHATLOG( "[%s -> %s]", iDst, gMyProf.handle ); }
    CHATLOG( "\n%s\n", iMsg );
    CHATFLUSH;
}

static void writeSendLog(
        int iNum, char *iName, BOOL iResult, char *iMsg, size_t iMsgSize )
{
    smpl_specDispMessage( iMsg );
    char* dst = smpl_makeDstLabel( iNum, iName );
    SMPL_PRINT( " >> %s -> %s (%zu byte sent)", gMyProf.handle, dst, iMsgSize );
    if( iResult ) {
        char* curTime = getCurTime();
        SMPL_PRINT( " at %s\n", curTime );
        writeChatLog( true, dst, curTime, iMsg );
    }
    else { SMPL_PRINT( " failed...\n" ); }
}

static BOOL sendMessage( int iNum, char *iMsg, size_t iMsgSize, BOOL isSys )
{
    smpl_profile_t* dst = &(gDstList[iNum]);
    BOOL result = com_sendSocket( dst->sid, iMsg, iMsgSize, NULL );
    if( !isSys ) { writeSendLog( iNum, dst->handle, result, iMsg, iMsgSize ); }
    return result;
}

static void setDstList( smpl_profile_t *oTarget, com_selectId_t iId )
{
    com_sockaddr_t* dst = com_getDstInf( iId );
    char* addr;
    char* port;
    com_getnameinfo( &addr, &port, &dst->addr, dst->len );
    *oTarget = (smpl_profile_t){ .port = (ushort)com_atoi(port),  .sid = iId };
    com_strcpy( oTarget->addr, addr );
    gPrompt = false;   // HELLOが来るまでプロンプト非表示にする
}

static void sendHELLO( int iId, smpl_profile_t *iProf )
{
    char msg[24] = {0};
    com_strcpy( msg, iProf->handle );
    snprintf( msg + SMPL_HANDLE_MAX, sizeof(msg) - SMPL_HANDLE_MAX,
              "%d", iProf->port );
    makeSysMsg( SMPL_SYSMSG_HELLO, msg, sizeof(msg) );
    (void)sendMessage( iId, gSysMsgBuff, sizeof(gSysMsgBuff), true );
}

static int addDestination( com_selectId_t iId )
{
    long newId = gDstCount;
    if( !com_realloct( &gDstList, sizeof(*gDstList), &gDstCount, 1,
                       "add dest list[%d]", gDstCount ) )
    {
        char msg[] = "<<SYSTEM>> sorry, my memory is lack ...";
        (void)sendMessage( iId, msg, sizeof(msg), false );
        com_deleteSocket( iId );
        return COM_NO_SOCK;
    }
    if( gDstId == COM_NO_SOCK ) { gDstId = newId; }
    setDstList( &(gDstList[newId]), iId );
    sendHELLO( newId, &gMyProf );
    return newId;
}

static smpl_profile_t *searchDst( com_selectId_t iId, int *oNum )
{
    *oNum = COM_NO_SOCK;
    for( int i = 0;  i < gDstCount;  i++ ) {
        if( gDstList[i].sid == iId ) {
            *oNum = i;
            return &(gDstList[i]);
        }
    }
    return NULL;
}

// 切断しても、gDstList[]からデータは削除しない。
// .sidを COM_NO_SOCKにすることで、以後「切断済み」のデータとして残す。
// その後、同じ相手と再接続した場合、gDstList[]に新たに追加し、
// 相手先index値も別のものになる。
// 今の所、メモリが続く限り追加していくが、上限を設ける措置は必要かもしれない。

static void closeDstList( int iDstId )
{
    smpl_profile_t* tmp = &(gDstList[iDstId]);
    char* curTime = getCurTime();
    SMPL_PRINT( "\n--- %d:%s [%s]:%d exited at %s ---\n",
                iDstId, tmp->handle, tmp->addr, tmp->port, curTime );
    CHATLOG( "%s <<%d:%s [%s]:%d exited>>\n",
             curTime, iDstId, tmp->handle, tmp->addr, tmp->port );
    CHATFLUSH;
    tmp->sid = COM_NO_SOCK;
    if( gDstList[gDstId].sid == COM_NO_SOCK ) { gDstId = COM_NO_SOCK; }
}

static BOOL closeDst( com_selectId_t iId )
{
    int num;
    smpl_profile_t* tmp = searchDst( iId, &num );
    if( !tmp ) { return true; }   // 無いなら何もしない(falseにはしない)
    closeDstList( num );
    return true;
}

static BOOL recvSysMsg(
        com_selectId_t iId, smpl_sysMsg_func_t *iSys, char *iSysMsg )
{
    int num;
    smpl_profile_t* tmp = searchDst( iId, &num );
    if( !tmp ) { return true; }   // 無いなら何もしない(falseにはしない)
    com_dump( iSysMsg, SMPL_SYSMSG_SIZE - 2,
              "recv SysMsg (%s(%d):%04x)", iSys->name, iId, iSys->type );
    (iSys->func)( tmp, iSysMsg, num );
    return true;
}

static char *getHandle( com_selectId_t iId, int *oNum )
{
    smpl_profile_t* tmp = searchDst( iId, oNum );
    if( !tmp ) { return "UNKNOWN"; }
    return tmp->handle;
}

static int gLastRecv = COM_NO_SOCK;    // 最後に受信した相手

static BOOL recvMessage( com_selectId_t iId, char *iMsg, ssize_t iSize )
{
    int num;
    char* handle = getHandle( iId, &num );
    char* dst = smpl_makeDstLabel( num, handle );
    char* curTime = getCurTime();
    gLastRecv = num;
    smpl_specRecvMessage( iMsg, dst, iSize, curTime );
    writeChatLog( false, dst, curTime, iMsg );
    return true;
}


///// ツール内イベント起点関数 ///////////////////////////////////////////////

static BOOL procEvent(
        com_selectId_t iId, COM_SOCK_EVENT_t iEvent,
        void *iData, size_t iSize )
{
    // 他者からの接続要求
    if( iEvent == COM_EVENT_ACCEPT ) { (void)addDestination(iId); return true; }
    // 他者の切断検出
    if( iEvent == COM_EVENT_CLOSE ) { return closeDst( iId ); }
    // システムメッセージ受信
    smpl_sysMsg_func_t* sys = getSysMsgCode( iData );
    if( sys ) { return recvSysMsg( iId, sys, (char*)iData + 2 ); }
    // 通常メッセージ受信
    return recvMessage( iId, iData, iSize );
}


///// ツール内各コマンド処理 /////////////////////////////////////////////////

static char gSmplBuff[COM_LINEBUF_SIZE];

static char *nextPrm( char *iData )
{
    char* result = strchr( iData, ' ' );
    if( !result ) { return NULL; }
    while( *result == ' ' ) { result++; }    // 非空白まで読み進める
    if( *result == '\0' ) { return NULL; }   // 末尾が空白のみなら次パラ無し
    return result;
}

static BOOL isAvailDst( int iDst )
{
    if( iDst < 0 || iDst >= gDstCount ) {
        SMPL_PRINT( "*** illegal destination ID\n" );
        return false;
    }
    if( gDstList[iDst].sid == COM_NO_SOCK ) {
        SMPL_PRINT( "*** already disconnected\n" );
        return false;
    }
    return true;
}

static BOOL cmdDest( char *iData )
{
    int dst = com_atoi( iData + 1 );
    char* msg = nextPrm( iData );
    if( !isAvailDst( dst ) ) { return true; }
    if( !msg ) { gDstId = dst; return true; }
    (void)sendMessage( dst, msg, strlen(msg)+1, false );
    return true;
}

static BOOL broadcast( char *iBcMsg, size_t iBcMsgSize, BOOL isSys )
{
    if( !gDstCount ) {
        if( !isSys ) { SMPL_PRINT( "%s\n*** no destinations\n", iBcMsg ); }
        return true;
    }
    BOOL result = true;
    for( int i = 0;  i < gDstCount;  i++ ) {
        if( gDstList[i].sid != COM_NO_SOCK ) {
            if( !sendMessage( i, iBcMsg, iBcMsgSize, true ) ) {
                result = false;
            }
        }
    }
    if( !isSys ) { writeSendLog( 0, NULL, result, iBcMsg, iBcMsgSize ); }
    return true;
}

BOOL smpl_sendStdin( char *iData, size_t iDataSize )
{
    if( gDstId == COM_NO_SOCK ) {
        smpl_specInvalidMessage( iData );
        SMPL_PRINT( "*** no destinations\n" );
        return true;
    }
    if( gBroadcast ) { return broadcast( iData, iDataSize, false ); }
    if( !isAvailDst( gDstId ) ) { return true; }
    (void)sendMessage( gDstId, iData, iDataSize, false );
    return true;
}

static BOOL cmdQuit( char *iData )
{
    char* msg = nextPrm( iData );
    if( msg ) { broadcast( msg, strlen(msg)+1, false ); }
    return false;
}

static BOOL getDstInfo( COM_AF_TYPE_t *oAf, char *oAddr, ushort *oPort )
{
    char* saveptr;
    char* tmp = strtok_r( gSmplBuff, " ", &saveptr );
    if( !tmp ) { return false; }
    strcpy( oAddr, tmp );
    if( COM_NOTIP == (*oAf = com_isIpAddr( oAddr )) ) { return false; }
    if( !(tmp = strtok_r( NULL, " ", &saveptr )) ) { return false; }
    int tmpPort = com_atoi( tmp );
    if( tmpPort < SMPL_PORT_MIN || tmpPort > SMPL_PORT_MAX ) { return false; }
    *oPort = (ushort)tmpPort;
    return true;
}

static BOOL getDstAddrInfo( struct addrinfo **oTarget )
{
    COM_AF_TYPE_t af = COM_NOTIP;
    char addr[SMPL_ADDR_SIZE] = {0};
    ushort port = 0;
    if( getDstInfo( &af, addr, &port ) ) {
        if( !strcmp( addr, gMyProf.addr ) && (port == gMyProf.port) ) {
            SMPL_PRINT( "*** cannnot connect to self\n" );
            return false;
        }
        com_freeaddrinfo( oTarget );
        if( !com_getaddrinfo( oTarget, COM_SOCK_TCP_CLIENT, af, addr, port ) ) {
            SMPL_PRINT( "*** fail to create destination information\n" );
            return false;
        }
    }
    else {  // 無指定時は登録済みデータを使用。それも無い時はNG
        if( !(*oTarget) ) {
            SMPL_PRINT( "*** illegal destination\n" );
            return false;
        }
    }
    return true;
}

static void dispDstInfo( struct addrinfo *iDst )
{
    char* addr = NULL;
    char* port = NULL;
    (void)com_getnameinfo( &addr, &port, iDst->ai_addr, iDst->ai_addrlen );
    SMPL_PRINT( "\n--- try to connect [%s]:%s ---\n", addr, port );
}

static com_selectId_t tryConnect(
        ushort iPort, int *oSockErr, struct addrinfo *iDst )
{
    SMPL_PRINT( "--- use [%s]:%d ---\n", gMyProf.addr, iPort );
    struct addrinfo* src = NULL;
    COM_AF_TYPE_t af = com_isIpAddr( gMyProf.addr );
    if( !com_getaddrinfo( &src,COM_SOCK_TCP_CLIENT,af,gMyProf.addr,iPort ) ) {
        return COM_NO_SOCK;
    }
    com_resetLastError();
    com_selectId_t result = 
        com_createSocket( COM_SOCK_TCP_CLIENT, src, NULL, procEvent, iDst );
    *oSockErr = com_getLastError();
    com_freeaddrinfo( &src );
    return result;
}

static com_selectId_t connectWithRetry( struct addrinfo *iDst )
{
    dispDstInfo( iDst );
    ushort port = gMyProf.port;
    // ポート番号を+1しながら試行する
    for( int cnt = 0;  cnt < SMPL_RETRY_MAX;  cnt++ ) {
        if( port == 65535 ) { gMyProf.conCount = 0; }
        else { (gMyProf.conCount)++; }
        port = (ushort)(gMyProf.port + gMyProf.conCount);

        int sockErr = COM_NO_ERROR;
        com_selectId_t result = tryConnect( port, &sockErr, iDst );
        if( result != COM_NO_SOCK ) { return result; }  // 接続成功
        if( sockErr == COM_ERR_CONNECTNG ) {
            SMPL_PRINT( "*** fail to connect...\n" );
            return COM_NO_SOCK;   // 恐らく相手側要因で接続失敗
        }
        // COM_ERR_SOCKETNG や COM_ERR_BINDNG の場合、ポート番号を変えて試行
    }
    SMPL_PRINT( "*** port retry out...\n" );
    return COM_NO_SOCK;
}

static void createConnect( struct addrinfo *iDst )
{
    com_selectId_t dstId = connectWithRetry( iDst );
    if( dstId == COM_NO_SOCK ) { return; }
    int tmp = addDestination( dstId );
    if( tmp == COM_NO_SOCK ) {
        com_deleteSocket( dstId );
        SMPL_PRINT( "*** fail to add destination list...\n" );
    }
    gDstId = tmp;
}

static BOOL cmdConnect( char *iData )
{
    com_strcpy( gSmplBuff, iData + 2 );
    if( !getDstAddrInfo( &gDstInf ) ) { return true; }
    createConnect( gDstInf );
    return true;
}

static BOOL cmdList( char *iData )
{
    int srchId = COM_NO_SOCK;
    char* id = nextPrm( iData );
    if( com_valDigit( id, NULL ) ) { srchId = com_atoi( id ); }
    SMPL_PRINT( "\n--- connection list start ---\n" );
    for( int i = 0;  i < gDstCount;  i++ ) {
        smpl_profile_t* tmp = &(gDstList[i]);
        if( srchId != COM_NO_SOCK && srchId != i ) { continue; }
        if( tmp->sid != COM_NO_SOCK ) {
            SMPL_PRINT( "%3d:%-*s from [%s]:%d\n", i, SMPL_HANDLE_MAX,
                        tmp->handle, tmp->addr, tmp->port );
        }
        if( srchId != COM_NO_SOCK ) { break; }
    }
    SMPL_PRINT( "--- connection list end ---\n" );
    return true;
}

static BOOL cmdDisconnect( char *iData )
{
    int dst;
    char* tkn = nextPrm( iData );
    if( !tkn ) { dst = gDstId; }
    else {
        if( !com_valDigit( tkn, NULL ) ) {
            SMPL_PRINT( "*** illegal destination ID\n" );
            return true;
        }
        dst = com_atoi( tkn );
    }
    if( !isAvailDst( dst ) ) { return true; }
    (void)com_deleteSocket( gDstList[dst].sid );
    closeDstList( dst );
    return true;
}

static BOOL cmdBroadcast( char *iData )
{
    char* msg = nextPrm( iData );
    if( msg ) { return broadcast( msg, strlen(msg)+1, false ); }
    gBroadcast = (gBroadcast == false);
    return true;
}

static BOOL cmdResponse( char *iData )
{
    char* msg = nextPrm( iData );
    if( gLastRecv == COM_NO_SOCK ) {
        SMPL_PRINT( "*** not receive any message\n" );
        return true;
    }
    if( !msg ) { gDstId = gLastRecv;  return true; }
    (void)sendMessage( gLastRecv, msg, strlen(msg)+1, false );
    return true;
}

static BOOL dispMyList( void )
{
    SMPL_PRINT( "\n--- my list start ---\n" );
    for( long i = 0;  i < gMyListCount;  i++ ) {
        smpl_profile_t* tmp = &(gMyList[i]);
        SMPL_PRINT( " %3ld:%-*s [%s]:%d\n", i+1, SMPL_HANDLE_MAX,
                    tmp->handle, tmp->addr, tmp->port );
    }
    SMPL_PRINT( "--- my list end ---\n" );
    return true;
}

static void addMyList( smpl_profile_t *iProf )
{
    smpl_profile_t* newProf =
        com_reallocAddr( &gMyList, sizeof(*gMyList), COM_TABLEEND,
                         &gMyListCount, 1, "add my list[%ld]", gMyListCount );
    if( newProf ) { *newProf = *iProf; }
}

static void tryAddMyList( smpl_profile_t *iProf )
{
    for( long id = 0;  id < gMyListCount;  id++ ) {
        smpl_profile_t* tmp = &(gMyList[id]);
        if( !strcmp(tmp->addr, iProf->addr) && tmp->port == iProf->port ) {
            SMPL_PRINT( " overwrite (%ld:%s -> %s)\n",
                        id + 1, tmp->handle, iProf->handle );
            com_strcpy( tmp->handle, iProf->handle );
            return;
        }
    }
    // アドレスとポートが重複するものが無い場合は新規登録
    SMPL_PRINT( " add (%ld:%s)\n", gMyListCount + 1, iProf->handle );
    addMyList( iProf );
}

static void writeMyList( void )
{
    FILE* fp = com_fopen( gMyListFile, "w" );
    if( !fp ) { return; }
    for( long id = 0;  id < gMyListCount;  id++ ) {
        smpl_profile_t* tmp = &(gMyList[id]);
        fprintf( fp, "%s,%s,%d\n", tmp->handle, tmp->addr, tmp->port );
    }
    fflush( fp );
    (void)com_fclose( fp );
}

static BOOL pickupDst( void )
{
    SMPL_PRINT( "\n--- my list update start ---\n" );
    long count = 0;
    for( long id = 0;  id < gDstCount;  id++ ) {
        smpl_profile_t* tmp = &(gDstList[id]);
        if( tmp->sid != COM_NO_SOCK ) {
            tryAddMyList( tmp );
            count++;
        }
    }
    if( count ) { writeMyList(); }
    SMPL_PRINT( "--- my list update end ---\n" );
    return true;
}

static BOOL connecMyList( long iIdx )
{
    if( iIdx == 0 ) { return pickupDst(); }
    if( iIdx < 0 || iIdx > gMyListCount ) {
        SMPL_PRINT( "*** illegal index of my list\n" );
        return true;
    }
    smpl_profile_t* tmp = &(gMyList[--iIdx]);
    // マイリストの内容は信用する
    com_freeaddrinfo( &gDstInf );
    (void)com_getaddrinfo( &gDstInf, COM_SOCK_TCP_CLIENT,
                           com_isIpAddr(tmp->addr), tmp->addr, tmp->port );
    createConnect( gDstInf );
    return true;
}

static BOOL cmdMyList( char *iData )
{
    char* prm = nextPrm( iData );
    if( !prm ) { return dispMyList(); }
    if( com_valDigit( prm, NULL ) ) { return connecMyList( com_atoi(prm) ); }
    return true;
}

static BOOL writeNewLog( FILE *iOld, FILE *iNew )
{
    if( !iOld || !iNew ) {
        com_error( COM_ERR_LOGGINGNG, "fail to rename chat log" );
        return false;
    }
    while( fgets( gSmplBuff, sizeof(gSmplBuff), iOld ) ) {
        fprintf( iNew, gSmplBuff );
    }
    fflush( iNew );
    return true;
}

static void addOldToNew( char *iOldLog )
{
    // 新ファイル既存の場合、その後ろに今のログを書き足し、以後それを使用
    fflush( gChatFp );
    (void)com_fclose( gChatFp );
    FILE* oldFp = com_fopen( iOldLog, "r" );   // 結果判定は writeNewLog() で
    FILE* newFp = com_fopen( gChatLog, "w" );  // 編集判定は writeNewLog() で
    BOOL result = writeNewLog( oldFp, newFp );
    (void)com_fclose( oldFp );
    // 現状、失敗すると以後チャットログ無し
    if( !result ) { com_fclose( newFp );  return; }
    gChatFp = newFp;
    (void)remove( iOldLog );
}

static void renameChatLog( void )
{
    char oldLog[SMPL_CHATLOG_SIZE] = {0};
    com_strcpy( oldLog, gChatLog );
    makeChatLogName();
    if( com_checkExistFile( gChatLog ) ) { addOldToNew( oldLog ); }
    else { (void)rename( oldLog, gChatLog ); }
}

static BOOL cmdRename( char *iData )
{
    char* name = nextPrm( iData );
    if( !name ) { SMPL_PRINT( "*** no new name\n" );  return true; }
    CHATLOG( "%s <<%s ->", getCurTime(), gMyProf.handle );
    com_strncpy( gMyProf.handle, sizeof(gMyProf.handle), name, strlen(name) );
    CHATLOG( "%s renamed>>\n", gMyProf.handle );
    CHATFLUSH;
    renameChatLog();
    makeSysMsg( SMPL_SYSMSG_RENAME, gMyProf.handle, sizeof(gMyProf.handle) );
    broadcast( gSysMsgBuff, sizeof(gSysMsgBuff), true );
    return true;
}

static BOOL gWideMode = false;
static BOOL gWideBroadcast = false;

// サンプル1では使われない。
// サンプル2でのみ、この関数は smpl_spec.c: switchWideMode()から呼ばれる。

void smpl_switchWideMode( void )
{
    gWideMode = (gWideMode == false);
}

static BOOL cmdWideInput( char *iData )
{
    char* prm = nextPrm( iData );
    smpl_wideInf_t inf = { &gWideMode, &gBroadcast, &gWideBroadcast, false };
    if( prm ) { if( !strcmp(prm, ":b") ) { inf.withBcCmd = true; } }
    return smpl_specWideInput( &inf );
}

// コマンド処理関数プロトタイプ
typedef BOOL(*smpl_cmd_t)( char *iData );

typedef struct {
    long  cmd;
    smpl_cmd_t  func;
} smpl_cmd_func_t;

static smpl_cmd_func_t  gCmdList[] = {
    { 'q', cmdQuit },          // プログラム終了
    { 'c', cmdConnect },       // コネクト
    { 'l', cmdList },          // コネクト一覧
    { 'd', cmdDisconnect },    // 切断
    { 'b', cmdBroadcast },     // ブロードキャスト(トグル)
    { 'r', cmdResponse },      // 直前に受信した相手に送信先変更
    { 'm', cmdMyList },        // マイリスト管理
    { 'n', cmdRename },        // ハンドル変更
    { 'w', cmdWideInput },     // 複数行送信
    { 0, NULL }
};

static BOOL viewCmdHelp( void )
{
    SMPL_PRINT(
"--- command help start ---\n"
" :q [msg]       終了 [msgをブロードキャスト送信後、終了]\n"
" :c [addr port] 再接続 [addr:port の相手に接続]\n"
" :l [index]     接続中リスト表示 [index指定で表示]\n"
" :index [msg]   送信先を index値に変更 [msg を indexに一時送信]\n"
" :d [index]     現在の送信先との接続を切断 [index指定で切断]\n"
" :r [msg]       送信先を最新受信元に [msg を最新受信元に一時送信]\n"
" :b [msg]       ブロードキャストモード切替 [msg を一時ブロードキャスト送信]\n"
" :m [myIndex]   マイリスト表示 [myIndex に接続]\n"
" :m 0           マイリスト更新\n"
" :n newName     ハンドルを newNameに変更\n"
" :w [:b]        ワイドモード切替 [加えてブロードキャストモードに]\n"
" :h             このヘルプを表示\n"
"--- command help end ---\n"
    );
    return true;
}

static BOOL procCommand( char *iData )
{
    long cmd = iData[1];
    char check[2] = { iData[1], 0 };
    smpl_specCommand( iData );
    if( *check == 'h' ) { return viewCmdHelp(); }
    if( com_valDigit( check, NULL ) ) { return cmdDest( iData ); }
    for( long id = 0;  gCmdList[id].cmd;  id++ ) {
        if( gCmdList[id].cmd == cmd ) { return (gCmdList[id].func)( iData ); }
    }
    SMPL_PRINT( "*** not available command (%c)\n", (int)cmd );
    return viewCmdHelp();
}


///// ツール内コマンド起点関数 ///////////////////////////////////////////////

BOOL smpl_procStdin( char *iData, size_t iDataSize )
{
    if( *iData == '\0' ) { return true; }  // リターン空打ちは無視
    if( *iData == ':' ) { return procCommand( iData ); }
    return smpl_sendStdin( iData, iDataSize );
}


///// メインループ ///////////////////////////////////////////////////////////

static void loopMain( void )
{
    smpl_promptInf_t  promptInf;
    do {
        promptInf = (smpl_promptInf_t){
            gMyProf.handle, gDstList[gDstId].handle, gDstId,
            gPrompt, gWideMode, gBroadcast, gWideBroadcast
        };
        smpl_specPrompt( &promptInf );
    } while( smpl_specWait( &promptInf ) );
}


///// 以下、起動/終了処理 ////////////////////////////////////////////////////

static BOOL getMyList( char *ioBuff, smpl_profile_t *oProf )
{
    memset( oProf, 0, sizeof(*oProf) );
    if( ioBuff[strlen(ioBuff)-1] == '\n' ) { ioBuff[strlen(ioBuff)-1] = '\0'; }
    char* saveptr = NULL;
    char* tkn = strtok_r( ioBuff, ",", &saveptr );
    if( !tkn ) { return false; }
    if( strlen(tkn) >= SMPL_HANDLE_MAX ) { return false; }
    com_strcpy( oProf->handle, tkn );
    if( !(tkn = strtok_r( NULL, ",", &saveptr )) ) { return false; }
    if( COM_NOTIP == com_isIpAddr( tkn ) ) { return false; }
    com_strcpy( oProf->addr, tkn );
    if( !(tkn = strtok_r( NULL, ",", &saveptr )) ) { return false; }
    int tmpPort = com_atoi( tkn );
    if( errno || tmpPort < SMPL_PORT_MIN || tmpPort > SMPL_PORT_MAX ) {
        return false;
    }
    oProf->port = (ushort)tmpPort;
    return true;
}

static BOOL readMyList( com_seekFileResult_t *iInf )
{
    smpl_profile_t tmp;
    if( getMyList( iInf->line, &tmp ) ) { addMyList( &tmp ); }
    return true;
}


static void setMyProf(
        struct addrinfo **oInf, const char *iHandle,
        const char *iAddr, const char *iPort )
{
    if( strlen(iHandle) > SMPL_HANDLE_MAX - 1 ) {
        ERRRET( COM_ERR_PARAMNG, "handle too long(%s)", iHandle );
    }
    COM_AF_TYPE_t af = com_isIpAddr( iAddr );
    if( af == COM_NOTIP ) {
        ERRRET( COM_ERR_PARAMNG, " illegal address(%s)", iAddr );
    }
    int port = com_atoi( iPort );
    if( port < SMPL_PORT_MIN || port > SMPL_PORT_MAX ) {
        ERRRET( COM_ERR_PARAMNG, "illegal port(%s)", iPort );
    }
    if( !com_getaddrinfo( oInf,COM_SOCK_TCP_SERVER,af,iAddr,(ushort)port ) ) {
        ERRRET( COM_ERR_PARAMNG,
                "fail to make self socket data [%s]:%s", iAddr, iPort );
    }
    com_strcpy( gMyProf.handle, iHandle );
    com_strcpy( gMyProf.addr, iAddr );
    gMyProf.port = (ushort)port;
}

static void inputMyProf( struct addrinfo **oInf )
{
    char handle[SMPL_HANDLE_MAX] = {0};
    com_valCondString_t condName = { 1, SMPL_HANDLE_MAX - 1 };
    com_valFunc_t val = { com_valString, &condName };
    com_actFlag_t flag = { false, false };
    (void)com_inputVar( handle, sizeof(handle), &val, &flag,
                        "your handle name:\n> " );

    char addr[SMPL_ADDR_SIZE] = {0};
    val = (com_valFunc_t){ com_valIpAddress, NULL };
    flag.enterSkip = true;
    BOOL ret = com_inputVar( addr, sizeof(addr), &val, &flag,
                             "address (default:127.0.0.1)\n> " );
    if( !ret ) { com_strcpy( addr, "127.0.0.1" ); }

    char port[8] = {0};
    com_valCondDigit_t condPort = { SMPL_PORT_MIN, SMPL_PORT_MAX };
    val = (com_valFunc_t){ com_valDigit, &condPort };
    ret = com_inputVar( port, sizeof(port), &val, &flag,
                        "port (default:40000)\n> " );
    if( !ret ) { com_strcpy( port, "40000" ); }

    setMyProf( oInf, handle, addr, port );
}

static void makeMyProf( long iCnt, char **iList, struct addrinfo **oInf )
{
    if( iCnt == 3 ) { setMyProf( oInf, iList[0], iList[1], iList[2] ); }
    else if( iCnt == 0 ) { inputMyProf( oInf ); }
    else { ERRRET( COM_ERR_PARAMNG, "illegal parameter" ); }
}

static void makeServerStatus( long iCnt, char **iList )
{
    struct addrinfo* svInf;
    makeMyProf( iCnt, iList, &svInf );
    gListenId = com_createSocket( COM_SOCK_TCP_SERVER, svInf, NULL,
                                  procEvent, NULL );
    com_freeaddrinfo( &svInf );
    if( gListenId == COM_NO_SOCK ||
        !smpl_specStdin( smpl_procStdin, &gStdinId ) )
    {
        ERRRET( COM_ERR_COMMNG,
                "fail to create socket (%ld/%ld)", gListenId, gStdinId );
    }
}


///// 起動オプションチェック /////////////////////////////////////////////////

static BOOL viewVersion( com_getOptInf_t *iOptInf )
{
    COM_UNUSED( iOptInf );
    // 特に何もせずに処理終了させる
    com_exit( COM_NO_ERROR );
    return true;
}

static BOOL clearLogs( com_getOptInf_t *iOptInf )
{
    COM_UNUSED( iOptInf );
    com_system( "rm -fr .%s.log_* chatlog.*", com_getAplName() );
    com_printf( "<<< cleared all past logs >>>\n" );
    com_exit( COM_NO_ERROR );
    return true;
}

#ifdef USE_TESTFUNC
static BOOL optTest( com_getOptInf_t *iOptInf )
{
    com_testCode( iOptInf->argc, iOptInf->argv );
    return true;
}
#endif // USE_TESTFUNC

static com_getOpt_t  gOptions[] = {
    { 'v', "version",  0,           0, false, viewVersion },
    { 'c', "clearlog", 0,           0, false, clearLogs },
#ifdef USE_TESTFUNC
    {   0, "debug",    COM_OPT_ALL, 0, false, optTest },
#endif // USE_TESTFUNC
    COM_OPTLIST_END
};


///// smpl初期化関数 /////////////////////////////////////////////////////////

void smpl_initialize( void )
{
    atexit( smpl_specFinalize );
    smpl_specInitialize();
}


///// サンプルコミュニケーター起点関数 ///////////////////////////////////////

void smpl_communicator( int iArgc, char **iArgv )
{
    long   prmCnt  = 0;       // オプション外パラメータ数
    char** prmList = NULL;    // オプション外パラメータリスト
    if( !com_getOption( iArgc, iArgv, gOptions, &prmCnt, &prmList, NULL ) ) {
        com_exit( COM_ERR_PARAMNG );
    }

    com_seekFile( gMyListFile, readMyList, NULL, NULL, 0 );
    makeServerStatus( prmCnt, prmList );
    openChatLog();
    com_printf( "\n--- comunication start ---\n" );
    smpl_specStart();
    SMPL_PRINT( "--- my profile (%s) ---\n [%s]:%d\n",
                gMyProf.handle, gMyProf.addr, gMyProf.port );
    loopMain();
    smpl_specEnd();
    com_printf( "\n--- communication end ---\n" );
    freeResource();
    closeChatLog();
}

//
// 本来 freeResource()と closeChatLog()は終了処理で呼ぶようにするべき。
// そうすれば CTRL+C等 シグナルによるプログラム終了時もリソース解放出来る。
//
// しかし敢えて終了処理を分けていない。
//
// これは そうすることで CTRL+C によって強制終了させた時等は、
// smplのリソース解放が行われず、メモリやファイルの浮き発生が確認できる、
// そのサンプルとできるためである。修正案を是非考えてみて欲しい。
//

