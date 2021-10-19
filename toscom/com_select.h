/*
 *****************************************************************************
 *
 * 共通部セレクト機能    by TOS
 *
 *   セレクト機能が必要な場合のみ、このヘッダファイルをインクルードし、
 *   com_select.cをコンパイルするソースに含めること。
 *
 *   このファイルより先に com_if.h をインクルードすること。
 *
 *   現在のセレクト機能
 *   ・COMSELBASE:   セレクト機能基本I/F
 *   ・COMSELADDR:   アドレス関連I/F
 *   ・COMSELSOCK:   ソケット関連I/F
 *   ・COMSELTIMER:  タイマー関連I/F
 *   ・COMSELEVENT:  イベント関連I/F
 *   ・COMSELDBG:    セレクト機能デバッグ関連I/F
 *
 *   本ヘッダファイルのI/Fでは、struct addrinfoや getaddrinfo()を使用するが、
 *   これは std=c99 だけでは使えず、-D_POSIX_C_SOURCE の宣言を必要とする。
 *   同梱の makefileでは指定済みである。
 *
 *****************************************************************************
 */

#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <ifaddrs.h>
#ifdef LINUXOS   // Cygwinでは存在が確認できなかったヘッダファイル
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/netlink.h>
#include <linux/if.h>
#endif

/*
 *****************************************************************************
 * COMSELBASE:セレクト機能基本I/F
 *****************************************************************************
 */

// セレクト機能エラーコード定義  ＊連動テーブルは gErrorNameSelect[]
enum {
    COM_ERR_SOCKETNG   = 940,       // socket() NG
    COM_ERR_BINDNG     = 941,       // bind() NG
    COM_ERR_CONNECTNG  = 942,       // connect() NG
    COM_ERR_LISTENNG   = 943,       // listen() NG
    COM_ERR_ACCEPTNG   = 944,       // accept() NG
    COM_ERR_SENDNG     = 945,       // send() NG
    COM_ERR_RECVNG     = 946,       // recv() NG
    COM_ERR_SELECTNG   = 947,       // select() NG
    COM_ERR_CLOSENG    = 948,       // close() NG
    COM_ERR_GETIFINFO  = 949,       // I/F情報取得NG
};

// イベント管理ID型
typedef long  com_selectId_t;

// NG返却用定義
enum {
    COM_NO_SOCK = -1
};

// アドレス情報構造体
typedef struct {
    socklen_t      len;              // アドレス情報帳
    int  dummyFor64bit;
    struct sockaddr_storage  addr;   // アドレス情報
} com_sockaddr_t;

// ソケット通信タイプ
//   増やしたら com_select.c: logSocket() にある prot[]にラベル要追加。
//   rawソケットを使う時は root権限が必要。
typedef enum {
    // 最初にrawソケットを宣言
    COM_SOCK_RAWSND = 1,          // パケット送信用 rawソケット
    COM_SOCK_RAWRCV,              // パケット受信用 rawソケット
    COM_SOCK_NETLINK,             // netlinkソケット
    // COM_SOCK_UDP以上の場合は UDP/TCPと判定する処理あり
    COM_SOCK_UDP,                 // UDP通信
    // COM_SOCK_UDPより大きい場合は TCPと判定する処理あり
    COM_SOCK_TCP_CLIENT,          // TCP通信(クライアント側)
    COM_SOCK_TCP_SERVER,          // TCP通信(サーバー側)
} COM_SOCK_TYPE_t;

// IP rawソケット生成時の送信バッファサイズ
#define COM_IPRAW_SNDBUF_SIZE  65535
// もし com_spec.h などで COM_IPRAW_SNDBUF_SPEC が宣言されていたら、それを優先


void com_initializeSelect( void );


/*
 *****************************************************************************
 * COMSELADDR:アドレス関連I/F
 *****************************************************************************
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_getaddrinfo(
 *       struct addrinfo **oTarget, COM_SOCK_TYPE_t iType, int iFamily,
 *       const char *iAddress, ushort iPort );
 */
#define com_getaddrinfo( TARGET, TYPE, FAMILY, ADDR, PORT ) \
    com_getaddrinfoFunc( (TARGEY),(TYPE),(FAMILY),(ADDR),(PORT),COM_FILELOC )

BOOL com_getaddrinfoFunc(
        struct addrinfo **oTarget, COM_SOCK_TYPE_t iType, int iFamily,
        const char *iAddress, ushort iPort, COM_FILEPRM );



/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_freeaddrinfo( struct addrinfo **oTarget );
 */
#define com_freeaddrinfo( TARGET ) \
    com_freeaddrinfoFunc( (TARGET), COM_FILELOC )

void com_freeaddrinfoFunc( struct addrinfo **oTarget, COM_FILEPRM );



BOOL com_getnameinfo(
        char **oAddr, char **oPort, const void *iAddr, socklen_t iLen );


void com_copyAddr( com_sockaddr_t *oTarget, const void *iSource );


// アドレスファミリー指定
typedef enum {
    COM_IPV4  = AF_INET,
    COM_IPV6  = AF_INET6,
    COM_NOTIP = -1,
} COM_AF_TYPE_t;

COM_AF_TYPE_t com_isIpAddr( const char *iString );


////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_IPADDRESS \
    com_valIpAddress, com_valIpAddressCondCopy, NULL

// com_valIpAddress()用 条件指定構造体
typedef struct {
    BOOL   ipv4;   // IPv4アドレスかどうかをチェック
    BOOL   ipv6;   // IPv6アドレスかどうかをチェック
} com_valCondIpAddress_t;

BOOL com_valIpAddress( char *ioData, void *iCond );
BOOL com_valIpAddressCondCopy( void **oCond, void *iCond );
// 解放は単純な com_free()で問題ない



BOOL com_compareAddr( void *iTarget, void *iSockAddr );
BOOL com_compareSock( void *iTargetSock, void *iSockAddr );
BOOL com_compareSockWithPort( void *iTargetSock, void *iSockAddr );



typedef struct {
    struct sockaddr*  addr;       // アドレス情報 (.ifaddrs->ifa_addr)
    long              preflen;    // Netllink取得時のアドレスマスク長
    char*             label;      // Netlink取得時のラベル
} com_ifaddr_t;

typedef struct {
    char*             ifname;      // ioctlの情報取得に使用可能
    int               ifindex;     // ioctlの情報取得に使用可能
    uchar             hwaddr[12];  // パディング調整のため、サイズ12
    size_t            h_len;       // hwaddr長 (通常のMACアドレスなら 6)
    long              cntAddrs;    // soaddrs/ifaddrsの総数
    com_ifaddr_t*     soAddrs;     // アドレス情報
    struct ifaddrs*   ifaddrs;     // getifaddrs()で取得した生データ
} com_ifinfo_t;

enum { COM_IFINFO_ERR = -1 };

long com_collectIfInfo( com_ifinfo_t **oInf, BOOL iUseNetlink );
long com_getIfInfo( com_ifinfo_t **oInf, BOOL iUseNetlink );



// 検索条件設定値
enum {
    COM_IF_NAME  = 1,       // ifname(char*)指定
    COM_IF_INDEX = 2,       // ifindex指定
    COM_IF_IPTXT = 4,       // IPアドレスのテキスト(char*)指定
    COM_IF_IPSA  = 8,       // IPアドレスの struct sockaddr*型指定
    COM_IF_HWTXT = 16,      // MACアドレスのテキスト(char*)指定
    COM_IF_HWBIN = 32,      // MACアドレスのバイナリ(uchar*)指定
    COM_IF_CONDMAX = 63
};

// 検索条件データ構造体
typedef struct {
    char*   ifname;     // COM_IF_NAME指定時に名前の文字列を設定
    long    ifindex;    // COM_IF_INDEX指定時にインデックス値を指定(0～)
    void*   ipaddr;     // COM_IF_IPTXT/COM_IF_IPSA指定時に対応データを設定
    void*   hwaddr;     // COM_IF_HWTXT/COM_IF_HWBIN指定時に対応データを設定
} com_seekIf_t;

com_ifinfo_t com_seekIfInfo(
        int iFlags, com_seekIf_t *iCond, BOOL iUseNetlink );



/*
 *****************************************************************************
 * COMSELSOCK;ソケット関連I/F
 *****************************************************************************
 */

// イベント種別
typedef enum {
    COM_EVENT_RECEIVE,   // データ受信
    COM_EVENT_ACCEPT,    // TCP接続受付  (TCPサーバのみ)
    COM_EVENT_CLOSE,     // TCP接続切断  (TCPサーバ/クライアント)
} COM_SOCK_EVENT_t;

// ソケットオプション データ構造体
typedef struct {
    int          level;      // オプション層
    int          optname;    // オプション名
    const void*  optval;     // オプション値
    ulong        optlen;     // オプション値サイズ
} com_sockopt_t;

// ソケットイベント発生時の通知関数プロトタイプ宣言
typedef BOOL(*com_sockEventCB_t)(
    com_selectId_t iId, COM_SOCK_EVENT_t iEvent, void *iData, size_t iDataSize);

com_selectId_t com_createSocket(
        COM_SOCK_TYPE_t iType, const void *iSrcInf,
        const com_sockopt_t *iOpt, com_sockEventCB_t iEventFunc,
        const void *iDstInf );



int com_getSockId( com_selectId_t iId );



com_sockaddr_t *com_getSrcInf( com_selectId_t iId );
com_sockaddr_t *com_getDstInf( com_selectId_t iId );



BOOL com_deleteSocket( com_selectId_t iId );



BOOL com_sendSocket(
        com_selectId_t iId, const void *iData, size_t iDataSize,
        const com_sockaddr_t *iDst );



ushort com_cksumRfc( void *iBin, size_t iBinSize );



typedef enum {
    COM_RECV_NG = 0,       // 関数引数内容不正、recvfrom()結果NG
    COM_RECV_OK = 1,       // recvfrom()結果OK
    COM_RECV_ACCEPT = 2,   // TCPサーバー時、accept()による接続を要求
    COM_RECV_CLOSE = 3,    // TCP接続時、相手の切断検出
    COM_RECV_NODATA = 4,   // 非同期指定地、データ受信なし
    COM_RECV_DROP = 5,     // フィルタリング関数によりドロップ
} COM_RECV_RESULT_t;

COM_RECV_RESULT_t com_receiveSocket(
        com_selectId_t iId, void *oData, size_t iDataSize,
        BOOL iNonBlock, ssize_t *oLen, com_sockaddr_t *oFrom );



com_selectId_t com_acceptSocket(
        com_selectId_t iListen, BOOL iNonBlock, com_sockEventCB_t iEventFunc );



typedef BOOL(*com_sockFilterCB_t)(
        com_selectId_t iId, void *iData, ssize_t iDataSize );

BOOL com_filterSocket( com_selectId_t iId, com_sockFilterCB_t iFilterFunc );



/*
 *****************************************************************************
 * COMSELTIMER:タイマー関連I/F
 *****************************************************************************
 */

typedef BOOL(*com_expireTimerCB_t)( com_selectId_t iId );

com_selectId_t com_registerTimer( long iTimer, com_expireTimerCB_t iExpireFunc);



enum {
    COM_ALL_TIMER = -10    // 全タイマーの満了をチェックする時に指定
};

BOOL com_checkTimer( com_selectId_t iId );



BOOL com_resetTimer( com_selectId_t iId, long iTimer );



BOOL com_cancelTimer( com_selectId_t iId );



/*
 *****************************************************************************
 * COMSELEVEBT:イベント関連I/F
 *****************************************************************************
 */

BOOL com_waitEvent( void );



BOOL com_watchEvent( void );



typedef BOOL(*com_getStdinCB_t)( char *iData, size_t iDataSize );

com_selectId_t com_registerStdin( com_getStdinCB_t iRecvFunc );



void com_cancelStdin( com_selectId_t iId );



/*
 *****************************************************************************
 * COMSELDBG:セレクト機能デバッグ関連I/F
 *****************************************************************************
 */

void com_setDebugSelect( BOOL iMode );



void com_dispDebugSelect( void );



// セレクト機能 導入フラグ
#define USING_COM_SELECT
// セレクト機能 初期化関数マクロ  ＊COM_INITIALIZE()で使用
#ifdef  COM_INIT_SELECT
#undef  COM_INIT_SELECT
#endif
#define COM_INIT_SELECT  com_initializeSelect()

