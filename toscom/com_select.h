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

/*
 *----------------------------------------------------------------------------
 * 標準関数の代替
 *   通信のために使用される可能性のある以下については、代替関数を用意している。
 *   いずれも元の関数より使い方を限定しているため、望むことが出来ない場合は
 *   元の関数を使っても問題ない。その時は一貫して標準関数を使うこと。
 *   つまり getaddrinfo()を使ったのなら、解放は freeaddrinfo()にすること。
 *
 *    --- アドレス関連I/F ---
 *     com_getaddrinfo()
 *                     getaddrinfo()を使い、メモリの捕捉を監視情報に加える。
 *                     com_freeaddrinfo()で解放しなかったら、プログラム終了時に
 *                     メモリ浮きとして指摘する。
 *                     ai_flags として AI_NUMERICSERV を固定設定するため、
 *                     それ以外のフラグ設定にしたい時は getaddrinfo()を使う。
 *     com_freeaddrinfo()
 *                     com_getaddrinfo()で捕捉したメモリを対象に解放する。
 *                     getaddrinfo()を使っていた場合は、freeaddrinfo()を使う。
 *                     (com_freeaddrinfo()でも一応メモリ解放は可能だが、
 *                      メモリ監視情報が無い旨のエラー出力は避けられない)
 *     com_getnameinfo()
 *                     flags を NI_NUMERICHOST | NI_NUMERICSERV で固定する。
 *                     それ以外のフラグにしたい時は getnameinfo()を使う。
 *
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

/*
 * セレクト機能初期化  com_initializeSelect()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドでの使用は想定していない。
 * ===========================================================================
 * com_initialize()の後に、これも実行すること。
 * COM_INITIALIZE()マクロを使用する場合、直接 本I/Fの記述は必要なく、マクロを
 * 記述しているソース冒頭で、本ヘッダファイルをインクルードするだけで良い。
 */
void com_initializeSelect( void );



/*
 *****************************************************************************
 * COMSELADDR:アドレス関連I/F
 *****************************************************************************
 */

/*
 * アドレス情報取得  com_getaddrinfo()
 *   情報生成の成否を true/false で返す。
 *   生成した情報 *oTarget は使用しなくなったら、com_freeaddrinfo()で解放する。
 *   この関数によるメモリ捕捉は、com_debug.c によるメモリ監視の対象となる。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG]
 *          !oTarget
 *          iType < COM_SOCK_RAWSND || iType > COM_SOCK_TCP_SERVER
 *          iFamily != AF_INET && iFamily != AF_INET6
 *          getaddrinfo() 処理NG
 *   監視情報追加のための com_addMemInfo()によるエラー
 * ===========================================================================
 *   マルチスレッドの影響は受けない。
 * ===========================================================================
 * struct addrinfo*型の変数を用意し、それに& を付けて oTargetに渡すと、
 * その他引数から、以後のソケット操作でも使用できるアドレス情報を生成する。
 * iTypeは実際に使うソケットの通信タイプを指定する。
 * iFamilyは AF_INET か AF_INET6。(IPv4なら前者、IPv6なら後者になる)
 * iAddressは自アドレスを表す文字列(ホスト名でも良い)。iPortは自ポート番号。
 *   例；UDP接続で 127.0.0.1:12345 のアドレス情報を生成したい場合
 *         struct addrinfo* inf;
 *         com_getaddrinfo( &inf, COM_SOCK_UDP, AF_INET, "127.0.0.1", 12345 );
 *       とすることで、アドレス情報を生成可能。
 * *oTargetに生成した情報のアドレスが入る。
 *
 * 実際の生成には getaddrinfo()を使用する。
 * hintsとして渡す ai_flags には AI_NUMERICSERV を指定しており変更不可。
 * それ以外のフラグ設定で生成したい時は getaddrinfo()を使うしか無い。
 *
 * 本I/F使用後のメモリ解放については、必ず com_freeaddrinfo()を使用すること。
 * ちなみに com_copyAddr()で struct addrinfo型のコピーが可能なので、
 * 必要に応じて使うとよいだろう。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_getaddrinfo(
 *       struct addrinfo **oTarget, COM_SOCK_TYPE_t iType, int iFamily,
 *       const char *iAddress, ushort iPort );
 */
#define com_getaddrinfo( TARGET, TYPE, FAMILY, ADDR, PORT ) \
    com_getaddrinfoFunc( (TARGET),(TYPE),(FAMILY),(ADDR),(PORT),COM_FILELOC )

BOOL com_getaddrinfoFunc(
        struct addrinfo **oTarget, COM_SOCK_TYPE_t iType, int iFamily,
        const char *iAddress, ushort iPort, COM_FILEPRM );


/*
 * アドレス情報解放  com_freeaddrinfo()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG]  !oTarget
 *   監視情報削除のための com_deleteMemInfo()によるエラー
 * ===========================================================================
 *   マルチスレッドの影響は受けない。
 * ===========================================================================
 * com_getaddrinfo()で捕捉した oTarget のメモリを解放し NULL を格納する。
 * 生成したアドレス情報を com_createSocket()や com_sendSocket()で使用後、
 * 再利用が不要なら、即座に本I/Fで解放して問題ない。
 *
 * 実際の解放には freeaddrinfo()を利用している。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_freeaddrinfo( struct addrinfo **oTarget );
 */
#define com_freeaddrinfo( TARGET ) \
    com_freeaddrinfoFunc( (TARGET), COM_FILELOC )

void com_freeaddrinfoFunc( struct addrinfo **oTarget, COM_FILEPRM );


/*
 * アドレス情報文字列取得  com_getnameinfo()
 *   getnameinfo()の成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG]  !oTarget
 *   監視情報削除のための com_deleteMemInfo()によるエラー
 * ===========================================================================
 *   マルチスレッドの影響は受けない。
 * ===========================================================================
 * iAddr は struct sockaddr_storage型と、それで表せる struct sockaddr_in型などの
 * アドレス情報のアドレスを想定する。様々な型を受け入れるため void*型にした。
 * getnameinfo()で求められている struct sockaddr *型ももちろん指定可能。
 *
 * iLenはそのアドレス情報の実サイズを指定する。
 * 0 にすると iAddrの sa_familyを参照し、それに対応する struct sockaddr_～型
 * のサイズを自動設定する。
 *
 * *oAddrと *oPortには、iAddr・iLenで与えられたアドレス情報から取得した
 * アドレスとポート番号を文字列として格納する。不要なら NULL指定する。
 * *oAddr・*oPortは解放不要だが、共通バッファであるため、他で更に使う予定なら
 * 返却後、速やかに別バッファにコピーすること。
 *
 * getnameinfo()を使う際、NI_NUMERICHOST | NI_NUMERICSERV を固定設定するので
 * アドレスもポート番号も単純な数値表現となる。
 * これ以外のフラグでの情報取得を望む場合は、getnameinfo()を使うこと。
 */
BOOL com_getnameinfo(
        char **oAddr, char **oPort, const void *iAddr, socklen_t iLen );


/*
 * 共通処理用アドレスコピー  com_copyAddr()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG]  !iSource || !oTarget
 * ===========================================================================
 *   マルチスレッドの影響は受けない。
 * ===========================================================================
 * iSourceで渡されるデータが struct addrinfo *型だった場合、
 * そのアドレス情報の内容を *oTargetにコピーする。
 * UDP接続で信号の送信先を生成する時に、本I/Fが役に立つはず。
 *
 * struct addrinfo_un *型(UNIXドメインソケット)が渡された場合は、
 * それに合わせた内容のコピーを実施して、*oTargetに設定する。
 */
void com_copyAddr( com_sockaddr_t *oTarget, const void *iSource );


/*
 * IPアドレス文字列判定  com_isIpAddr()
 *   文字列がIPv4なら COM_IPV4、IPv6なら COM_IPV6 を返す。
 *   これは そのまま AF_INET/AF_INET6 と同値になっている。
 *   文字列がIPアドレスとして認識できない時は COM_NOTIP を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドの影響は受けない。
 * ===========================================================================
 * iStringが IPv4/IPvtのアドレスを示す文字列であるかチェックする。
 * 判定には inet_pton()を使用している。このため IPv4アドレスの場合、
 * "1.1.1.1" というようにピリオド区切りの4つの10数値のパターンのみ受け付ける。
 * (基本的にこの形式ではない不完全な書き方はしないだろうという想定)
 */

// アドレスファミリー指定
typedef enum {
    COM_IPV4  = AF_INET,
    COM_IPV6  = AF_INET6,
    COM_NOTIP = -1,
} COM_AF_TYPE_t;

COM_AF_TYPE_t com_isIpAddr( const char *iString );


/*
 * IPアドレスチェック  com_valIpAddress()
 *   チェック結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドの影響は受けない。
 *   ただしファイル操作が別プロセスで行われることには対応できない。
 * ===========================================================================
 * com_addCfgValidator() や com_input() で使うチェック関数。
 * ioDataをIPアドレスとして扱い、アドレスを示す文字列なら true を返す。
 *
 * iCondで条件を設定する。NULL指定時は条件は全て true として動作する。
 * iCondは com_valCondIpAddress_t型のアドレスで、trueで設定したアドレスとして
 * 正しいかチェックする。両方 true なら IPv4・IPv6 どちらかなら良いことになる。
 * どちらも false だと 全て false を返すようになるので注意すること。
 *
 * com_input()では com_valIpAddress() しか使わない。
 * COM_VAL_IPADDRESS・com_valIpAddressCondCopy() は com_addCfgValidator()で
 * 使うものなので、詳細は そちらの説明も参照すること。
 */

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


/*
 * アドレス情報比較             com_compareAddr()
 * ソケットアドレス比較         com_compareSock()
 * IPアドレス・ポート番号比較   com_compareSockWithPort()
 *   ターゲットが比較対象と一致したら true、不一致なら false を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iTarget || iSockAddr (com_compareAddr()時)
 *                            !iTargetSock || iSockAddr (com_compareAddr()以外)
 * ===========================================================================
 *   マルチスレッドの影響は受けない。
 * ===========================================================================
 * ターゲットのアドレスを比較しその結果を返す。I/Fにより比較対象が異なる。
 *
 * com_compareAddr() の iTargetにはアドレス自体の先頭アドレスを指定する。
 * (例えば strcuct sockaddr_in型ではなく そのメンバーの .sin_addr.s_addr )
 *
 * iSockAddr には ソケットアドレスデータ(struct sockaddr_～型)のアドレスを指定
 * する。メンバー内の s～_family の値でどのソケットアドレスなのかを区別する為
 * そことアドレス自体が正しく設定されている必要がある。
 *   struct sockaddr_in型   (.sin_family = AF_INET)   IPv4アドレス
 *   struct sockaddr_in6型 (.sin6_family = AF_INET6)  IPv6アドレス
 *   struct sockaddr_ll型   (.sll_family = AF_PACKET) rawソケット [Linuxのみ]
 *   struct sockaddr_un型   (.sun_family = AF_UNIX)   UNIXドメインソケット
 * の4つを受け付ける。ただし struct sockaddr_ll型だけは Linuxのみとなる。
 *
 * com_compareAddr()は上記2つのアドレス内容のみ一致するか比較する。
 * 比較するのはアドレスだけで、ファミリーやポート番号はノーチェックとなる。
 *
 *
 * com_compareSock()と com_sompareSockWithPort() はどちらの引数も
 * 受け付けるのはソケットアドレス(struct sockaddr_～型)となる。
 *
 * その上で、com_compareSock()はアドレス内容のみ一致するか比較する。
 * 比較するのはアドレスだけで、ファミリーやポート番号はノーチェックとなる。
 *
 * com_compareSockWithPort()はアドレスとポート番号が一致するか比較する。
 * ただしファミリーが AF_INET または AF_INET6 ではない場合、その時点で falseを
 * 返す(他のファミリーではポート番号が特定できないため)
 */
BOOL com_compareAddr( void *iTarget, void *iSockAddr );
BOOL com_compareSock( void *iTargetSock, void *iSockAddr );
BOOL com_compareSockWithPort( void *iTargetSock, void *iSockAddr );



// IF個別情報
//   Netlinkは Linuxのみで使用可能。
typedef struct {
    struct sockaddr*  addr;       // アドレス情報 (.ifaddrs->ifa_addr)
    long              preflen;    // Netlink取得時のアドレスマスク長
    char*             label;      // Netlink取得時のラベル
} com_ifaddr_t;

// IF情報構造体
//   hwaddrには MACアドレスが入る想定。
//   ifaddrsの内容から cntAddrs と soAddrs が編集される。
//   ファミリーが AF_PACKET の場合、soAddrsには内容NULLの要素もあり得るので、
//   それを踏まえてデータを利用すること。(AF_PACKETは Linuxのみで使用可能)
typedef struct {
    char*             ifname;      // ioctlの情報取得に使用可能
    int               ifindex;     // ioctlの情報取得に使用可能
    uchar             hwaddr[12];  // パディング調整のため、サイズ12
    size_t            h_len;       // hwaddr長 (通常のMACアドレスなら 6)
    long              cntAddrs;    // soaddrs/ifaddrsの総数
    com_ifaddr_t*     soAddrs;     // アドレス情報
    struct ifaddrs*   ifaddrs;     // getifaddrs()で取得した生データ
} com_ifinfo_t;

// (参考)
// struct ifaddrs {
//     struct ifaddrs   *ifa_next;      // Next item in list
//     char             *ifa_name;      // Name of interface
//     unsigned int      ifa_flags;     // Flags from SIOCGIFFLAGS
//     struct sockaddr  *ifa_addr;      // Address of interface
//     struct sockaddr  *ifa_netmask;   // Netmask of interface
//     union {
//         struct sockaddr *ifu_broadaddr;
//                          // Broadcast address of intarface
//         struct sockaddr *ifu_dstaddr;
//                          // Point-to-point destination address
//     } ifa_ifu;
// #define               ifa_broadcast  ifa_ifu.ifu_broadcast
// #define               ifa_dstaddr    ifa_ifu.ifu_dstaddr
//     void             *ifa_data;      // Address-specific data
// };


/*
 * 通信インターフェース情報取得  com_collectIfInfo()・com_getIfInfo()
 *   取得できた情報数を返す。
 *   処理NGが発生した場合は COM_IFINFO_ERR を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oInf
 *   COM_ERR_DEBUGNG: [com_prmNG] iUseNetlink==true  [Linuxではない場合]
 *   COM_ERR_NO_MEMORY: 情報格納用のメモリ捕捉に失敗
 *   COM_ERR_GETINFO: 情報取得のための getifaddrs/socket/ioctl でNG
 * ===========================================================================
 *   マルチスレッドでの使用は想定していない。
 * ===========================================================================
 * 現在収集可能なネットワークインターフェースの情報を収集し、その個数を返す。
 * *oInfに収集した情報の先頭アドレスを格納する。
 * 複数I/Fがある場合は、線形にデータを並べたフィールドの先頭アドレスを返すので
 * []で添字を付けて一つずつアクセスできる(添字は返った個数 - 1まで)
 *
 * 幾つかメモリ捕捉も行っているが、その解放は全てI/F側で実施するので、
 * I/Fを使う側が解放を意識する必要はない。
 *
 * com_collectIfInfo()・com_getIfInfo()は返す結果は変わらない。その差分は
 * ・com_collectIfInfo()は必ず使用時点の情報を取得して返す。
 * ・com_getIfInfo()は既に取得済みの情報ある場合、それをそのまま返す。
 * というものになる。
 *
 * iUseNetlinkが falseの場合、getifaddrs()で情報を取得し、必要に応じて
 * 追加の情報を ioctl()で取得する。これらは ifconfigのデータ参照している。
 * Linuxでは伝統的に ifconfigでデータ管理が行われていたが、CentOS 7以降は
 * ipコマンドによる管理に移行しつつある。その結果、この伝統的な方法だと、
 * 情報を取りこぼす可能性が出てきている。
 * 気になるなら iUseNetlinkを trueにして情報収集を試みる。
 *
 * iUseNetlinkが trueの場合、情報取得に Netlinkソケットを使用する。
 * ただし Linuxのみで使えるもので、Linux以外で true指定時はエラーになる。
 * 取得結果の index値(.ifindex)は取得した順番で設定しているが、これは暫定措置。
 * 今後もっと明確な決め方が分かったら修正する。
 * .ifname .hwaddr[] .hwlen .cntAddrs .soaddrs は同じように設定する。
 * getifaddrs()による情報取得が無いため .ifaddrs は常にNULLになる。
 * iUseNetlinkが true時のみ .preflen .label を設定する。
 *
 * アドレスマスクはデータ形式に差分があるので注意。
 *   ・iUseNetlinkが falseの場合 .ifaddrs[].netmask が相当する。
 *     (アドレス形式で、マスク長(ビット数)ではない)
 *   ・iUseNetlinkが trueの場合  .preflen[] が相当する。
 *     (マスク長(ビット数)となる)
 *
 * I/F名(ラベル)も若干差分があるので注意。
 *   ・iUseNetlinkが falseの場合 .ifname に必ず文字列を設定する。
 *   ・iUseNetlinkが trueの場合  .label に文字列を設定するが、
 *     設定なし(NULL)になることもあるので、利用時は注意すること。
 */
enum { COM_IFINFO_ERR = -1 };

long com_collectIfInfo( com_ifinfo_t **oInf, BOOL iUseNetlink );
long com_getIfInfo( com_ifinfo_t **oInf, BOOL iUseNetlink );


/*
 * 通信インターフェース情報検索  com_seekIfInfo()
 *   該当するインターフェース情報のアドレスを返す。
 *   該当するものがない場合は NULLを返す。
 *   (インターフェース有無の判定だけなら NULLかどうかのチェックだけで済む)
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG]
 *         iFlag <= 0 || iFlag > COM_IF_CONDMAX || !iCond
 *         COM_IF_NAME指定ありで !iCond->ifname
 *         COM_IF_INDEX指定ありで iCond->ifindex < 0
 *         COM_IF_IPTXT・COM_IF_IPSA 同時指定
 *         COM_IF_IPTXT指定ありで !iCond->ipaddr || com_getaddrinfo() NG
 *         COM_IF_HWTXT・COM_IF_HWBIN 同時指定
 *         COM_IF_HWTXT指定ありで !iCond->hwaddr || MACアドレス文字列不正
 *   COM_ERR_NOMEMORY: 条件設定の領域確保失敗
 * ===========================================================================
 *   マルチスレッドでの使用は想定していない。
 * ===========================================================================
 * 特定の条件に合致する通信インターフェース情報を返す。
 * 先に com_collectIfInfo()か com_getIfInfo()を使って情報取得済みの場合、
 * その情報を再利用するため、常に最新の情報で検索したい場合は、事前で明示的に
 * com_collectIfInfo()を使用しておくこと。
 *
 * iFlagは実際に使用する検索条件で | で複数を並列指定可能。
 * その条件に従って内容を設定した構造体のアドレスを iCondに指定する。
 *
 * COM_IF_IPTXT・COM_IF_IPSAはどちらか一方だけが指定可能で、
 * それに沿った型のデータのアドレスを iCond->ipaddrに設定すること。
 *
 * COM_IF_HWTXT・COM_IF_HWBINはどちらか一方だけが指定可能で、
 * それに沿った型のデータのアドレスを iCond->hwaddrに設定すること。
 *
 * 取得できるインターフェース情報は一意に決まることを想定するが、
 * IPアドレスに関しては、同じものが複数指定される可能性は低いがゼロではない。
 * その場合は一番最初に見つかった情報を返すのみとなるため、完璧な検索を
 * 望む場合は com_collectIfInfo()・com_getIfInfo()で全ての情報を取得し、
 * 自分でアドレス内容をチェックするほうが良いだろう。
 *
 * また取得する情報は各インターフェースに登録されたIPアドレスがメインであり
 * ifconfig や ip addr で表示される全ての情報取得を目的にはしていない。
 * そうした情報が必要な場合は、本I/Fで取得したデータ(.ifnameや .ifindex)を使い
 * 自力で ioctl()や AF_NETLINKソケットを使って情報を引き出す必要がある。
 */

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

com_ifinfo_t *com_seekIfInfo(
        int iFlags, com_seekIf_t *iCond, BOOL iUseNetlink );



/*
 *****************************************************************************
 * COMSELSOCK;ソケット関連I/F
 *****************************************************************************
 */

/*
 * ソケット通信のプログラムを記述する際のガイドライン
 * ---------------------------------------------------------------------------
 * iSrcInf(自側アドレス)・iDstInf(対向アドレス)は com_getaddrinfo()で生成でき
 * ソケット関連I/Fで使用後は、再利用する予定が無いなら com_freeaddrinfoFunc()
 * でいつでも解放できる。メモリ監視の対象なので、意識して解放すること。
 *
 * ソケットで通信するための処理記述の流れと、それに対応するI/Fは以下：
 *
 * UDP通信 (発着どちらも同じ手順になる)
 *  (1)com_createSocket()でソケット生成。iSrcInfが必要になる。
 *     konoI/Fにより、socket()でソケット生成し、iSrcInfの情報を bind()する。
 *     全て成功したら管理IDを返す。以後、この管理IDを使って通信する。
 *  (2)com_sendSocket()でデータ送信。管理IDと iDstInfが必要になる。
 *     iDstInfを送信先として sendto()を実施する。
 *  (3)データ受信の方法は3通りある。いずれも recvfrom()で受信する。
 *     ・com_waitEvent()で複数ソケットを同期(block)受信する。
 *         生成した全ソケット監視し、受信があったら select()で処理する。
 *         監視しているどれかのソケットで受信するまで処理が止まる。
 *         受信があると、イベント関数を COM_EVENT_RECEIVE指定で呼ぶので、
 *         そこで受信処理を記述する。
 *     ・com_watchEvent()で複数ソケットを非同期(non block)受信する。
 *         データ受信なしでもひとまず終了して返る。
 *         データ受信時は com_waitEvent()と同じくイベント関数を使う。
 *     ・com_receiveSocket()使用。管理IDで指定したソケットのみ受信する。
 *         同期/非同期は選択可能。
 *         COM_RECV_OKが返ってきたらデータ受信できているので、
 *         そのデータ内容の処理を続ける。イベント関数は使用しない。
 *  (4)com_deleteSocket()で、管理IDで指定したソケットを削除する。
 *     close()でソケットを閉じ、明示的に通信終了する。
 *     以後は同じ管理IDで通信しないように処理を記述すること。
 *
 * TCP通信 (クライアント側)
 *  (1)com_createSocket()でソケット生成。iSrcInfと iDstInfが必要。
 *     socket()・bind()が成功したら、そのソケットで iDstInfに対して connect()
 *     を実施する。サーバーから応答が来て TCP接続が確立できたら、管理IDを返す。
 *     以後、この管理IDを使って通信する。
 *     サーバーから応答が来ない場合、一度ソケットを削除して COM_SOCK_NGを返す。
 *     その後 再接続したい時は com_createSocket() をやり直す。
 *  (2)com_sendSocket()でデータ送信。(UDPと同じ手順だが iDstInfは不要)
 *  (3)データ受信方法も UDPと同じ3通りから選ぶ。
 *     相手から切断された場合、サイズ0のデータを受信する。その時の処理は
 *     ・com_waitEvent()/com_watchEvent()使用時
 *         ソケット削除後、イベント関数を COM_EVENT_CLOSE指定で呼ぶので、
 *         そこで通信終了時の処理を書く。
 *     ・com_receiveSocket()使用時
 *         COM_RECV_CLOSEが返るので、自分で com_deleteSocket()を使用して
 *         ソケット削除する必要がある。
 *  (4)com_deleteSocket()で、管理IDで指定したソケットを削除する。
 *     close()でソケットを閉じ、明示的に通信終了する。
 *     自分から切断する時は、この方法でソケット削除して対応する。
 *     以後は同じ管理IDで通信しないように処理を記述すること。
 *
 * TCP通信 (サーバー時)
 *  (1)com_createSocket()でソケット生成。iSrcInfが必要。
 *     socket()・bind()が成功したら listen()でクライアント待ちになる。
 *     この時 返される管理IDは あくまで listen()用のソケットのもの。
 *  (2)クライアントからの接続を受け付ける方法も3通り。
 *     ・com_waitEvent()/com_watchEvent()を使う。(同期か非同期かの違い)
 *         自動で com_acceptSocket()を実行し、通信用ソケットを確立して、
 *         イベント関数で COM_RECV_ACCEPT指定で 通信用管理IDでも通知する。
 *         その中で通知された管理IDを保持すること。
 *     ・com_acceptSocket()を使う。(1)で受けた管理IDが必要。
 *         通信確率出来たら、通信用管理IDを返すので、それを保持する。
 *  (3)データ送信の方法は他と同様。
 *     使用する管理IDが (1)ではなく (2)で得たものにする必要がある。
 *  (4)データ受信の方法は TCPクライアント時と同様。
 *     使用する管理IDが (1)ではなく (2)で得たものにする必要がある。
 *  (5)com_deleteSocket()で、管理IDを指定してソケット削除。
 *     (1)(2)双方で取得した管理ID指定して両方とも削除する必要がある。
 * 
 * 同期通信(＝受信するまで止まって待つ)で問題がないのであれば、
 * ソケット生成後 受信などの処理は com_waitEvnet()に任せるのが一番楽。
 * TCP時の accept()処理も、対向側からの切断も自動で処理するし、
 * 複数ソケットをまとめて受信もできるし、タイマーや標準入力も対応可能。
 *
 * ループ処理の中で、他の処理を継続しながら受信を待つ状況で使用するのが
 * 非同期通信(＝受信がない場合は処理終了)の一番の目的となる。
 * com_watchEvent()は ソケットとタイマーを全て監視できるが、標準入力は
 * 受け付けない。残念ながら非同期通信と標準入力は並行が難しい。
 * 非同期通信をしながら文字入力をしたい場合、ウィンドウ機能も併用して
 * com_inputWindow()を使う方法は考えられる。
 *
 * com_waitEvent()や com_watchEvent()を使うのが処理的には一番楽だが、
 * それで処理しきれないことがある場合、com_acceptSocket()や
 * com_receiveSocket()による個別処理も出来るようにはしている。
 * UDP接続で相手が一つだけの場合も com_receiveSocket()を使うほうが、
 * データ受信がしやすいとも言える。
 *
 * ----- UNIXドメインソケット -----
 *
 *  UDP/TCP接続指定時、iSrcInf・iDstInf に struct sockaddr_un*型のデータを
 *  渡すことにより、UNIXドメインソケットの生成が可能。
 *  発着アドレスの型が変わるだけで、通信手順は何も変わらない。
 *
 * ----- rawソケット [Linuxのみ] -----
 *
 *  Linuxであれば、主にIP通信用に rawソケットが使用できる。
 *  rawソケットは生成時に 送信用か受信用かを選ぶ必要がある。
 *  (送受信どちらもしたい場合、2つソケットを作る必要があるということ)
 *  
 *  信号送受信の手順は UDP と同様になる。
 *
 *  ソケット生成時のプロトコル指定が IP (ETH_P_IP/ETH_P_IPV6) の場合、
 *  受信データはIPヘッダからとなり、それ以外なら Etherヘッダからになる。
 *
 *  送信時のデータも同じヘッダから始める必要がある。
 *  チェックサムについてはソケットで自動計算する設定にしているので、
 *  自力計算がどうしても必要な場合、com_createSocket()で指定するオプション(iOpt)
 *  で、受信バッファサイズ(SO_SNDBUF)のみ設定するデータを作成して指定すること。
 *  (デフォルトはこのオプション + チェックサム自動計算 の設定になっている)
 *
 * ----- Netlinkソケット [Linuxのみ] -----
 *
 *  Linuxであれば、Netlinkソケットにも対応する。
 *  他と違い、発着のアドレス情報は存在しないため、データ送受信の方法は
 *  TCP に近くなる。
 *
 *  com_collectIfInfo() は Netlinkを使った情報取得をサポートしており、
 *  Netlinkソケットを使ったデータ通信のサンプルソースの側面も持つ。
 *  getIfInfoByNetlink()が処理の起点になっていて、com_createSocket()・
 *  com_sendSocket()・com_receiveSocket()・com_deleteSocket() を流れに沿って
 *  使用している。
 *
 *
 * ＜マルチスレッドでの動作＞
 *
 *  セレクト機能はマルチスレッドで動作することはあまり想定していない。
 *  特に異なるスレッドでソケット生成した場合、生成ソケットを全監視する
 *  com_waitEvent()や com_watchEvent()でイベント発生時、適切なスレッドで
 *  イベント関数が呼ばれる保証は全くできない。
 *
 *  しかし com_receiveSocket()や com_acceptSocket()はソケット管理IDを指定して
 *  個別にデータ受信チェックを可能であるため、それぞれのスレッドで自身が持つ
 *  ソケットの受信状況を確認することは可能となる。
 *  データ送信については、もともとソケット管理IDを指定する com_sendSocket()を
 *  使うので、これは適宜 いずれのスレッドでも使用可能。
 *
 *  イベント登録(ソケットやタイマー)と削除については、排他制御を行うが、
 *  データ送受信については必要な場合、呼び元で排他処理を確認すること。
 * ---------------------------------------------------------------------------
 */

/*
 * ソケット生成  com_createSocket()
 *   ソケットが生成できたら、comモジュール内の管理ID(0以上)を返す。
 *   この管理IDは以後 捜査対象のソケット指定に使用するので保持が必須となる。
 *   socket()によって受け取るソケットIDとは別の値なので注意すること。
 *   (ソケットID自体も保持しており、必要なら com_getSocketId()で取得可能)
 *
 *   TCPサーバーのソケット生成時に返る管理IDは、listenソケットのものなので、
 *   実際のデータ送受信には使えず、accept時に生成する管理IDを別途保持して、
 *   そちらを使う必要がある。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iSrcInf  (COM_SOCK_NETLINK以外の場合)
 *                                !iDstInf  (COM_SOCK_TCP_CLIRNT時のみ)
 *   管理情報追加時の com_realloct()によるエラー
 *   COM_ERR_SOCKETNG:  socket()/setsockopt()処理NG
 *   COM_ERR_BINDNG:    bind()処理NG
 *   COM_ERR_CONNECTNG: connect()処理NG
 *   COM_ERR_LISTENNG:  listen()処理NG
 * ===========================================================================
 *   排他制御を実施するため、スレッドセーフとなる。
 * ===========================================================================
 * 入力に従ってソケットを生成し、その結果を返す。
 * 本I/Fでソケット生成に必要な情報はだいたい設定するため、以後の操作は
 * どのソケット種別でもほぼ同じように記述できるようになる。
 *
 * iTypeは生成するソケットの種別で、以下が指定可能。
 *   COM_SOCK_UDP:        UDPソケット/UNIXドメインソケット
 *   COM_SOCK_TCP_CLIRNT: TCPソケット(クライアント側)/UNIXドメインソケット
 *   COM_SOCK_TCP_SERVER: TCPソケット(サーバー側)/UNIXドメインソケット
 *   COM_SOCK_RAWSND;     [Linuxのみ] rawソケット(送信用) ＊要root権限
 *   COM_SOCK_RAWRCV:     [Linuxのみ] rawソケット(受信用) ＊要root権限
 *   COM_SOCK_NETLINK:    [Linuxのみ] Netlinkソケット(試験実装段階)
 * 各ソケットがどのような性質のものかは、ネット等でいくらでも調べられるので
 * ここで長々と説明記述を行うことはしない。
 *
 * iSrcInfは自側のアドレス情報を設定する。主に nind()で使用する。iTypeに何を
 * 設定したかで、求められるデータ型が変わるため、仮引数は void*型とした。
 *   COM_SOCK_UDP/COM_SOCK_TCP_CLIRNT/COM_SOCK_TCP_SERVER::
 *     基本的に struct addrinfo*型を設定すると、IP通信ソケット生成になる。
 *     com_getaddrinfo()で生成した情報をそのまま使用する事が可能。
 *     (その際の iTypeは本I/Fの iTypeと合わせるほうが無難)。
 *
 *     strcut sockaddr_UN*型を設定すると、UNIXドメインソケットの生成になる。
 *        .sun_family = AF_UNIX
 *        .sun_path = 通信に使うファイルパス文字列
 *     という設定をして渡すこと。
 *   COM_SOCK_RAWSND:
 *     struct addrinfo*型を設定する。ただ必要な設定は .ai_family だけで、
 *     それ以外(アドレスも含む)は内容を見ない。
 *     .ai_family が AF_INET/AF_INET6 の場合、IP rawソケットを生成する。
 *       ソケットのプロトコル設定が IPPROTO_RAW になる。
 *     .ai_family が AF_INET/AF_INET6以外の場合、Ether2 rawソケットを生成する。
 *       ソケットのファミリー設定は AF_PACKET になり、プロトコル設定が
 *       本I/Fの iSrcInfに設定した .ai_familyを htons()変換した値になる。
 *       MACアドレスが必要な場合、com_collectIfInfo()/com_getIfInfo()で調べるか
 *       何らかの方法で入力させるかを検討する必要があるだろう。
 *   COM_SOCK_RAWRCV:
 *     strict addrinfo_ll*型を設定する。以下の設定を期待する。
 *     .sll_family:   内容不問(AF_PACKET で自動設定する)
 *     .sll_protocol: htons(ETH_P_ALL) のようにバイトオーダー変換した値。
 *       ETH_P_IP/ETH_P_IPV6指定時は SOCK_DGRAMを設定し IPヘッダから受信、
 *       それ以外を指定時は SOCK_RAWを設定し Ether2ヘッダから受信、となる)
 *     .sll_ifindex:  通信に使用したI/Fのインデックス値。
 *       com_collectIfInfo()/com_getIfInfo()/com_seekIfInfo()で調べるか、
 *       何らかの方法で入力させるかを検討する必要があるだろう。
 *   COM_SOCK_NETLINK:
 *     プロトコル値を格納した int型変数のアドレスを設定すると、ソケット生成時に
 *     用いる。NULL設定でもよく、その場合のプロトコル値は NETLINK_ROUTE になる。
 *
 * iOptは設定したいソケットオプションになる。.optvalは変数アドレスが必要なので
 * 別途定義して、そのアドレスを指定する、といった工夫が必要になるだろう。
 * 配列で複数指定を可能としており、その場合は .levelが負数のデータを最後に必ず
 * 定義すること(それをでデータ終端を判定するため)。
 * NULL指定することも可能で、その場合は iTypeによって 以下を自動設定する。
 *   COM_SOCK_UDP/COM_SOCK_TCP_CLIENT/COM_SOCK_TCP_SERVER
 *     ・無条件で SO_REUSEADDR を設定する
 *     ・iSrcInfに IPv6アドレス指定時は IPV6_V6ONLY を設定する。
 *     ただし UNIXドメインソケット時は何も設定しない。
 *   COM_SOCK_RAWSND
 *     ・iSrcInfに IPv4アドレス指定時は IP_HDRINCL を設定する。
 *     ・無条件で SO_SNFBUF を 65535 で設定する。
 *       com_spec.h 等で COM_IPRAW_SNDBUF_SPEC を設定時はその内容値で設定する。
 *   COM_SOCK_RAWRCV/COM_SOCK_NETLINK
 *     何も設定しない。
 * 上記の自動設定を使いたくない場合は、例えば .levelが負数のデータを1つだけ
 * iOptで指定すれば、一切何もオプション設定しない状態のソケットに出来る。
 *
 * iEventFuncは「イベント関数」と呼ばれ、イベント監視I/F(com_waitEvent() または
 * com_watchEvent() のこと)使用時、そのソケットで何らかのデータ受信があった時、
 * コールバックされ その受信処理を行う関数となる。関数記述のガイドラインは、
 * この後の 関数プロトタイプ宣言にコメントされているので、そちらを参照。
 * イベント監視I/Fでを使わず、com_acceptSocket()/com_receiveSocket() を使って
 * 自力で受信処理を記述するつもりの場合、NULL指定しても問題ない。
 *
 * iDstInf は iTypeを COM_SOCK_TCP_CLIENT (TCPクライアントソケット)を指定時に
 * 接続する相手TCPサーバーのアドレス情報を設定する。生成方法は iSrcInfと同様。
 * IPソケットの場合は struct sockaddr*型で com_getaddrinfo()の生成結果が使える。
 * UNIXドメインソケットの場合は struct sockaddr_un*型になる。
 * ソケット生成時、ここで指定した相手へのTCP接続を試みる。もし失敗したら、
 * こちらもソケット削除して COM_NO_SOCK を返す。
 * iTypeが COM_SOCK_TCP_CLIENT 以外の場合、参照しないので NULLで問題ない。
 *
 *
 * 本I/Fで生成したソケットは、その後 com_deleteSocket()で能動的に削除可能。
 * TCP接続時は com_waitEvent()/com_watchEvent()使用時に 相手からの切断検出したら
 * そのTCPコネクションのソケットは自動で削除する。
 * また何もしなくてもプログラム終了処理(finalizeSelect())で、最後にソケットは
 * 全て自動でエラー/警告なしに削除するので、無理に削除を意識しなくても良い。
 *
 * デバッグ出力が ONの場合、ソケット生成のログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 */

// ソケットオプション データ構造体
typedef struct {
    int          level;      // オプション層
    int          optname;    // オプション名
    const void*  optval;     // オプション値
    ulong        optlen;     // オプション値サイズ
} com_sockopt_t;
// optlenの本来の型は socklen_t型だが、 サイズが32bitの定義だった時に、
// 64bit環境でパディングが発生し、コンパイル時に警告が出てしまうため、
// 敢えて ulong型で定義している。
// toscom内部で実際に使う必要がある時は、socklen_t型でキャストしている。

// イベント種別(イベント関数や受信I/Fで使用)
typedef enum {
    COM_EVENT_RECEIVE,   // データ受信
    COM_EVENT_ACCEPT,    // TCP接続受付  (TCPサーバのみ)
    COM_EVENT_CLOSE,     // TCP接続切断  (TCPサーバ/クライアント)
} COM_SOCK_EVENT_t;

// イベント関数 プロトタイプ宣言
//   イベント監視によってデータ受信があった時にコールバックされる関数。
//   iIdは受信があったソケットの管理ID、iEventは受信イベント種別。
//   COM_EVENT_RECEIVE以外のイベントは TCP接続でなければ通知されることはない。
//   iDataと iDataSizeは実際に受信したデータの内容となる。
//   イベント種別が COM_EVENT_RECEIVE以外だと iData=NULL・iDataSize=0で固定。
//
//   イベント関数では、通知される可能性のあるイベント発生時の処理を記述する。
//
//   iEvent == COM_EVENT_RECEIVE
//     データ受信時の処理を記述する。実際にデータを扱う別関数を呼ぶ等して
//     イベント関数があまり肥大化しないように務めるのが望ましいだろう。
//
//   iEvent == COM_EVENT_ACCEPT   (TCPサーバー時のみ)
//     iIdで accept()による確立された通信用ソケットの管理IDが通知されるので、
//     その内容を保持する。以後の通信はその管理IDで行う。
//
//   iEvent == COM_EVENT_CLOSED   (TCPサーバー/クライアント)
//     通知された iIdでの通信はもう出来ないので、以後 使わないようにする。
//     その通信のために持っていたデータも不要なら破棄/解放する。
//
//   イベント関数の返り値は、com_waitEvent()/com_watchEvent()にもそのまま使用
//   される。
//
typedef BOOL(*com_sockEventCB_t)(
    com_selectId_t iId, COM_SOCK_EVENT_t iEvent, void *iData, size_t iDataSize);

com_selectId_t com_createSocket(
        COM_SOCK_TYPE_t iType, const void *iSrcInf,
        const com_sockopt_t *iOpt, com_sockEventCB_t iEventFunc,
        const void *iDstInf );


/*
 * ソケットID取得  com_getSockId()
 *   指定した管理IDで保持しているソケットIDを返す。
 *   その管理IDが標準入力用の場合は 0 を返す。
 *   その管理IDが未使用時やソケット情報無しの場合は COM_NO_SOCK を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iId不正
 * ===========================================================================
 *   排他制御を実施するため、スレッドセーフとなる。
 * ===========================================================================
 * iIdで指定した管理IDで使用しているソケットのソケットIDを返す。
 * toscom使用時は、管理IDが分かっていれば、そのソケットIDは意識不要。
 * それでもソケットIDが必要な時に、本I/Fを使用する。
 * (どうしても標準関数でソケットを操作する必要がある時等か)
 */
int com_getSockId( com_selectId_t iId );


/*
 * アドレス情報取得  com_getSrcInf()・com_getDstInf()
 *   指定した管理IDがで保持しているアドレス情報を返す。
 *   その管理IDが未使用時やソケット情報無しの場合は NULL を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iId不正
 * ===========================================================================
 *   排他制御を実施するため、スレッドセーフとなる。
 * ===========================================================================
 * iIdで指定した管理IDごとに保持しているアドレス情報を返す。
 * toscom使用時は、管理IDが分かっていれば、保持するアドレス情報は意識不要。
 * それでもアドレス情報が必要な時に、本I/Fを使用する。
 * com_getSrcInf()は自側、com_getDstInf()は対向の情報を取得する。
 * 自側アドレス情報は com_createSocket()で受け取ったものになる。
 */
com_sockaddr_t *com_getSrcInf( com_selectId_t iId );
com_sockaddr_t *com_getDstInf( com_selectId_t iId );


/*
 * ソケット削除  com_deleteSocket()
 *   処理の成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iId不正
 *   COM_ERR_CLOSENG: close()失敗
 * ===========================================================================
 *   排他制御を実施するため、スレッドセーフとなる。
 * ===========================================================================
 * iIdで指定された管理IDのソケットを削除し、通信を終了する。
 * TCP接続の場合、このI/Fによる切断で、切断通知のコールバックは実施しない。
 * 本I/Fにより削除されたソケットの管理IDは、以後 再利用される可能性がある。
 *
 * com_waitEvent()・com_watchEvent()を使っている場合、TCP接続のソケットは
 * 対向から切断されると、自動で本I/Fによるソケット削除を実施する。
 * 自分から能動的に切断したい時に、本I/Fを使用する。
 *
 * デバッグ出力が ONの場合、ソケット削除のログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 */
BOOL com_deleteSocket( com_selectId_t iId );


/*
 * データ送信  com_sendSocket()
 *   処理の成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iId不正 || !iData
 *   COM_ERR_SENDNG: send()/sendto()失敗
 * ===========================================================================
 *   マルチスレッドの影響は考慮されていない。
 * ===========================================================================
 * iIdで指定された管理IDのソケットを使用し、データを送信する。
 * iData と iDataSize が実際に送信するデータの情報となる。
 * iDstは信号送信先で、ソケットが COM_SOCK_UDP のときのみ有効なデータとなる。
 * それ以外の種別のソケットでは参照しないので、NULLで問題ない。
 *
 * iDstの型は com_createSocket()のときとは異なり、com_sockaddr_t*型である。
 * com_getaddrinfo()で生成した後、com_copyAddr()で com_sockaddr_t型にその
 * アドレス情報をコピーすることで、iDstに設定できる情報を作れる。
 *
 * UDPのUNIXドメインソケットの場合は 通信先のファイルパスを指定した
 * struct sockaddr_un型のデータを作成後、com_copyAddr()でコピーすると良い。
 *
 * COM_SOCK_UDPの場合、iDstの内容は信号送信後、管理情報で保持する。
 * 必要があれば com_getDstInf()でいつでも取得できる。
 * com_getDstInf()で取得した情報は、そのまま iDstに使用できる。
 *
 * デバッグ出力が ONの場合、データ送信のログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 */
BOOL com_sendSocket(
        com_selectId_t iId, const void *iData, size_t iDataSize,
        const com_sockaddr_t *iDst );


/*
 * チェックサム計算  com_cksumRfc()
 *   計算結果を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドの影響を受ける処理はない。
 * ===========================================================================
 * iBinと iBinSizeで渡されたバイナリデータのチェックサム値を計算して返す。
 * 計算結果が 0xFFFF になる場合、RFCによる議論の推移から 0 を返す。
 * (RFC1624「3.Disccusion」を参照)
 *
 * このチェックサム値は IPv4/ICMP/UDPヘッダ等で使用されるもので、
 * 「16bit単位で 1の補数和を取った結果」となる。
 * rawソケットを使ったデータ通信で、必要になることもあるだろう。
 *
 * なお AF_INETを指定した rawソケット(COM_SOCK_RAWSND)を普通に生成した場合、
 * IPヘッダのチェックサム計算はOSがするように設定しているため、本I/Fを使って
 * 計算値を設定する処理は不要。もし設定しても OSの計算値に上書きされる。
 */
ushort com_cksumRfc( void *iBin, size_t iBinSize );


/*
 * データ受信  com_receiveSocket()
 *   受信結果を返す。返す内容は COM_RECV_RESULT_t の定数宣言を参照。
 *   受信には recvfrom()を使い、その返り値(受信サイズ)は *oLenに格納する。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iId不正 || !oData || !oLen
 *   COM_ERR_SENDNG: send()/sendto()失敗
 * ===========================================================================
 *   マルチスレッドの影響は考慮されていない。
 * ===========================================================================
 * iIdで指定された管理IDのソケットを使用し、データを受信する。
 * oDataと iDataSizeが が受信データを格納するバッファとそのサイズになる。
 *
 * iNonBlock を trueにすると、非同期指定となり recvfrom()実行時、引数flagsに
 * MSG_DONTWAIT を設定する。これによりデータ受信が無くても返るようになる。
 * recvfrom()の返り値が 0未満で errnoが EAGAIN または EWOULDBLOCK の場合は
 * COM_RECV_NODATA を返すので、受信処理をリトライする方向で考えること。
 * 
 * iNonBlock を falseにしたときは 同期指定となり flagsは 0で受信する。
 * この場合は、データ受信があるまで、処理が止まる。
 *
 * oLen は recvfrom()の返り値(正常なら受信サイズ、負数ならNG)を格納する。
 * 受信サイズは必須情報なので、これのNULL指定は許容されていない。
 *
 * oFrom は recvfrom()が送信元を取得時の格納先となる。取得不要なら NULLにする。
 *
 * COM_RECV_OKを返したら、正しくデータ受信できているので、oData と *oLen で
 * 受信データを参照できる。
 * COM_RECV_NGが返った時、どうするかはそのプログラムによるだろう。
 * COM_RECV_DROPは com_filterSocket()で登録したフィルター関数に「受信不要」と
 * 判定されてデータが破棄されたことを示す。次の受信に移るといいだろう。
 *
 * TCP接続をしている場合、さらに以下が返ってくる可能性がある。
 *
 * COM_RECV_ACCEPT は指定したソケットが TCPサーバーで最初に作った Listenソケット
 * で recvfrom()が負数を返したことを示す。これを受けた時は com_acceptSocket()を
 * 直ちに使って、TCP接続を確立すると良い。以後、com_acceptSocket()で返ってきた
 * 管理IDを使って信号送受信が出来るようになる。
 *
 * COM_RECV_CLOSE は TCP接続のソケットで recvfrom()が 0を返したことを示す。
 * これは相手からの接続が切断されたことを示すものなので、com_deleteSocket()で
 * こちらもTCP接続を終了させると良いだろう。
 *
 * ちなみに com_waitEvent()や com_watchEvent()は COM_RECV_ACCEPT/COM_RECV_CLOSE
 * になったときの処理を自動で行って通知してくれるので、特に事情がない限りは、
 * そちらを使ったデータ受信とすることを推奨する。
 *
 * デバッグ出力が ONの場合、データ受信のログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 */

// com_receiveSocket()の返り値
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


/*
 * TCP接続受付  com_acceptSocket()
 *   対向からの接続要求を受け入れ、生成された通信用ソケットの管理IDを返す。
 *   accept()で処理NG時は COM_NO_SOCK を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iListen不正 || listenソケットでない管理ID
 *   COM_ERR_ACCEPTNG: accept()失敗
 *   イベント情報追加のための com_reallocによるエラー
 * ===========================================================================
 *   マルチスレッドの影響は考慮されていない。
 * ===========================================================================
 * iListenで指定した管理IDの listenソケットに接続要求が来た時、それを受け入れ
 * TCP接続を確立する。確立して通信ソケットが生成できたら、その管理IDを返す。
 * 以後、その管理IDを使ってデータ送受信を行える。
 *
 * iListenに指定できるのは、COM_SOCK_TCP_SERVERを指定して com_createSocket()で
 * 生成したソケットの管理IDのみ。そうではない管理IDを指定するとエラーになる。
 *
 * iNonBlockの値により、listenソケットに対し fcntl()で以下の設定をする。
 *  ・true:   F_SETFL, O_NONBLOCK  (非停止モード＝非同期)
 *  ・false:  F_SETFL, O_SYNC      (同期モード)
 * accept()が負数を返すとどちらの設定でも COM_NO_SOCK を返すが、
 * iNonBlockが true時は、errnoが EAGAIN または EWOULDBLOCK の場合、
 * COM_ERR_ACCEPTNG のエラーは出さず、リトライを期待するので、errnoのチェックを
 * 必ず実施すること。(com_receiveSocket()と同じような扱いとなる)
 *
 * iEventFuncは 新たに生成される通信用ソケットのイベント関数指定になる。
 * NULL指定でもよく、その場合は listenソケットのイベント関数を流用する。
 * それとは別の関数にしたい場合に備えて、この引数は用意されている。
 *
 * com_waitEvent()や com_watchEvent()を使う場合、iEventFunc=NULLで動作となる。
 * ただイベント関数内で 受信/アクセプト/切断の何が起きたのか区別出来るので、
 * listenソケットと同じイベント関数でも困ることはない。
 *
 * デバッグ出力が ONの場合、accept()の結果をログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 */
com_selectId_t com_acceptSocket(
        com_selectId_t iListen, BOOL iNonBlock, com_sockEventCB_t iEventFunc );


/*
 * ソケットフィルター設定  com_filterSocket()
 *   処理結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iId不正 || !oData || !oLen
 *   COM_ERR_SENDNG: send()/sendto()失敗
 * ===========================================================================
 *   マルチスレッドの影響は考慮されていない。
 * ===========================================================================
 * iIdで指定した管理IDのソケットのデータ受信に対するフィルター関数を登録する。
 *
 * iFilterFuncが実際に登録するフィルター関数。登録できる関数はひとつのみで
 * 複数回同じ管理IDに登録した時は 最後に登録した関数のみ使われる。
 * NULL指定も可能で、その時は先に登録されていたフィルター関数の解除になる。
 *
 * このフィルタリング関数はデータ受信に自動コールバックされる。
 * 受信したソケットの管理ID(iId)・受信データ(iData)・データサイズ(iDataSize)
 * を渡すので、その関数の中で受信データの内容をチェックし、破棄するなら true、
 * 破棄せず受信するなら falseを返すように記述する。
 *
 * com_receiveSocket()では本関数によりデータ破棄が行われた場合、
 * COM_RECV_DROPを返して、それを通知する。
 *
 * com_waitEvent()ではデータ受信があっても、その全てが破棄された場合、
 * 呼び元には何も返さず、受信待ちを継続する。
 *
 * com_watchEvent()では、データ受信が全て破棄されても、受信待ちにはせず
 * 必ず呼び元に返る。
 */

// パケットフィルター関数プロトタイプ宣言
typedef BOOL(*com_sockFilterCB_t)(
        com_selectId_t iId, void *iData, ssize_t iDataSize );

BOOL com_filterSocket( com_selectId_t iId, com_sockFilterCB_t iFilterFunc );



/*
 *****************************************************************************
 * COMSELTIMER:タイマー関連I/F
 *****************************************************************************
 */

/*
 * タイマー登録  com_registerTimer() 
 *   正常に登録できたら、その管理ID(0以上)を返す。
 *   この管理IDはタイマー解除等に使うので保持すること。
 *   登録できなかった場合は COM_NO_SOCK を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iTimer < 0 || !iExpireFunc
 *   COM_ERR_TIMENG: 現在日時取得失敗
 *   イベント情報追加のための com_realloc()によるエラー
 * ===========================================================================
 *   排他制御を実施するため、スレッドセーフとなる。
 * ===========================================================================
 * iTimerにはタイマー値(ミリ秒単位)を指定する。
 * iExpireFuncは そのタイマーが満了した時にコールバックする関数。
 *
 * タイマーの満了＝タイマー登録から iTimerの時間が経過 を意味する。
 * 本I/Fによる登録＝タイマー開始であることに注意すること。
 * 満了により、iExpireFuncがコールバックされると タイマーは停止する。
 * 解除されるわけではないので、com_resetTimer()で再設定出来る。
 *
 * com_stopTimer()でタイマーを停止することも可能。
 * これも解除はされていないので、com_resetTimer()で再設定できる。
 *
 * タイマー登録後、その満了チェックをするには com_checkTimer() または
 * com_waitEvent()・com_watchEvent()を使ってタイマーを監視する必要がある。
 * これらのI/Fをを使うと、その時点でタイマーが満了していたら、コールバックを
 * 起こすことが出来る。
 *
 * com_checkTimer()は特定のタイマーまたは登録されている全タイマーを対象に
 * 満了しているものがあったらコールバックする。
 * com_waitEvent()/com_watchEvent()は通信ソケットとともにタイマーも監視していて
 * 一番最初に満了したタイマーが検出されるとコールバックする。
 *
 * com_waitEvent()は同期型なので、ソケットによるデータ受信または タイマー満了が
 * 検出されるまでは、そこで処理を停止する。
 * com_checkTimer()と com_watchEvent()は非同期型なので、該当がなければ
 * 何も待たずに結果を返してくるので、ループ処理内で定期的に使用する想定。
 * 常に待たない分だけ、特にタイマー精度は落ちる可能性はある。
 *
 * どのI/Fでタイマーを監視するかは、使う側に委ねられている。
 * 一番簡単なのは com_waitEvent()ではあるが、他の処理を並行したいなら、
 * 他のI/Fが候補に上がるだろう。
 *
 * タイマーを登録したままプログラム終了した時は、全タイマー解除処理が動く。
 * このため、無理にタイマー解除する必要はない。
 *
 * デバッグ出力が ONの場合、タイマー登録のログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 */

// タイマー満了時のコールバック関数プロトタイプ宣言
//   iIdは満了したタイマーの管理IDが入る。
//   この関数の返り値は com_checkTimer()/com_waitEvent()/com_watchEvent()の
//   返り値にもなる。
typedef BOOL(*com_expireTimerCB_t)( com_selectId_t iId );

com_selectId_t com_registerTimer( long iTimer, com_expireTimerCB_t iExpireFunc);


/*
 * タイマー停止  com_stopTimer()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] 管理ID不正
 * ===========================================================================
 *   排他制御を実施するため、スレッドセーフとなる。
 * ===========================================================================
 * iIdで指定した管理IDのタイマー動作を停止する。
 * あくまで停止するだけで解除(イベントとしての削除)はしない。
 *
 * タイマー満了時は本I/Fを使わずともタイマー停止する。
 *
 * com_resetTimer()により停止したタイマーの再使用が可能。
 * 
 * デバッグ出力が ONの場合、タイマー終了のログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 */
BOOL com_stopTimer( com_selectId_t iId );


/*
 * タイマー満了チェック  com_checkTimer()
 *   基本的には trueを返す。
 *   タイマー満了のコールバック関数がひとつでも falseを返したら、falseを返す。
 *   現在時刻が取得できなかったときも falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_TIMENG: 現在日時取得失敗
 * ===========================================================================
 *   マルチスレッドの影響は考慮されていない。
 * ===========================================================================
 * iIdで指定した管理IDのタイマーをチェックし、満了していたら 満了処理関数を
 * コールバックし、その返り値を返す。(返り値の使い方は呼び元に委ねられる)
 *
 * iIdに COM_ALL_TIMER を指定した場合、登録されている全タイマーを順にチェックし
 * 満了していたら、それぞれの満了処理関数をコールバックする。この場合は、
 * ひとつでも false が返ってきたら、falseを返す処理となる。
 *
 * iIdが有効なタイマーの管理IDでなかった場合は何もしない。
 *
 * 満了したタイマーは停止するので、com_resetTimer()で再設定可能。
 *
 * 本I/Fよりは com_watchEvent()のほうがまとめて色々チェックできるが、
 * 特定のタイマーだけチェックしたいなどの理由があれば、本I/Fを使うと良い。
 * 
 * デバッグ出力が ONの場合、タイマー満了があれば そのログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 */
enum {
    COM_ALL_TIMER = -10    // 全タイマーの満了をチェックする時に指定
};

BOOL com_checkTimer( com_selectId_t iId );


/*
 * タイマー再設定  com_resetTimer()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] 管理ID不正
 * ===========================================================================
 *   排他制御を実施するため、スレッドセーフとなる。
 * ===========================================================================
 * iIdで指定した管理IDのタイマーを iTimer(ミリ秒単位)で再設定する。
 * iTimerに 0を指定した場合、先に登録済みのタイマー値で再設定する。
 *
 * 指定されていたタイマーが満了していたり、解除されていても復活させる。
 * ただしソケットやタイマーを新たに登録するときの空きエリアと認識され、
 * 別情報で上書きされている可能性もあるので、この方法を使うのは、当該タイマー
 * の満了処理関数内にとどめておくことを強く推奨する。
 *
 * ソケットやタイマーの登録が行われていない状態なら、特に問題はないので、
 * 本I/Fを使用する可能性のあるタイマーを使う時は、
 * 処理を始める前に他のタイマーも全て登録しておくことを推奨する。
 * 
 * デバッグ出力が ONの場合、タイマー再設定のログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 */
BOOL com_resetTimer( com_selectId_t iId, long iTimer );


/*
 * タイマー解除  com_cancelTimer()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] 管理ID不正
 * ===========================================================================
 *   排他制御を実施するため、スレッドセーフとなる。
 * ===========================================================================
 * iIdで指定した管理IDのタイマーを登録解除する。
 * com_resetTimer()による再使用も当然出来ないので、タイマーを止めたいだけなら
 * com_stopTimer()を使用すること。
 * 
 * デバッグ出力が ONの場合、タイマー解除のログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 *
 * プログラム終了時に起動中のタイマーは全て自動で登録解除するので、
 * 個別に本I/Fを使う必要はあまりない。
 */
BOOL com_cancelTimer( com_selectId_t iId );



/*
 *****************************************************************************
 * COMSELEVENT:イベント関連I/F
 *****************************************************************************
 */

/*
 * イベント用バッファ切替  com_switchEventBuffer()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない
 * ===========================================================================
 *   マルチスレッドの影響を受ける処理はない
 * ===========================================================================
 * com_waitEvent()・com_watchEvent()でデータ受信するときは、
 * 内部で持つ サイズ 4096 のバッファをデフォルトでは使用する。
 *
 * 事前に本I/Fを使うことで、使用するバッファを任意のものに切り替える。
 * (もっと大きな/小さなバッファで・・という目的が多いと想定)
 */
void com_switchEventBuffer( void *iBuf, size_t iBufSize );


/*
 * イベント同期待機  com_waitEvent()
 *   コールバックした関数の返り値をそのまま返す。
 *   基本的にはイベント待機を継続するかどうかの目的で使用する返り値と想定する
 *   が、その使い方は呼び元に委ねられる。
 * ---------------------------------------------------------------------------
 *   COM_ERR_SELECTNG: 待受FDリスト作成失敗、select()処理NG
 *   COM_ERR_RECVNG: recvfrom()処理NG
 *   COM_ERR_ACCEPTNG: accept()処理NG
 *   COM_ERR_SENDNG: システムメッセージ送信NG
 *   COM_ERR_TIMENG: 現在日時取得失敗
 * ===========================================================================
 *   マルチスレッドの影響は考慮されていない。
 * ===========================================================================
 * select()を利用して、現在登録されているソケットデータ受信・タイマー満了・
 * 標準入力を監視し、イベントが発生したら、それぞれに対応するコールバック関数を
 * 呼んで、その結果を返す。
 *
 * 同期型であるため、特定のイベントが発生しなければ、そのまま処理停止となる。
 *
 * 監視対象となるのは以下の該当する全て。
 *  ・com_createSocket()で生成されたソケット
 *  ・com_acceptSocket()/com_waitEvent/com_watchEvent()で生成されたソケット
 *  ・com_registerTimer()で登録された停止していないタイマー
 *  ・com_registerStdin()で登録された標準入力(キーボードからの入力)
 * 個別に監視したい時は、それぞれ別のI/Fがあるので、それを使用すること。
 * (ただし標準入力は個別監視はできない)
 *
 * 概ね下記のようなイメージでの使用を想定している。
 *     while( com_waitEvent() ) { }
 * コールバック関数はイベント待機を続けるなら true を返すようにする。
 * この方法が比較的楽だが、イベント待機で止めたくない時は、com_watchEvent()か
 * 個別監視のI/Fを使っていくことを検討するしか無い。
 *
 * デバッグ出力が ONの場合、発生したイベントのログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 */
BOOL com_waitEvent( void );


/*
 * イベント非同期監視  com_watchEvent()
 *   基本的に trueを返す。
 *   コールバックした関数の返り値に一つでも falseがあったら falseを返す。
 *   基本的にはイベント待機を継続するかどうかの目的で使用する返り値と想定する
 *   が、その使い方は呼び元に委ねられる。
 * ---------------------------------------------------------------------------
 *   COM_ERR_RECVNG: recvfrom()処理NG
 *   COM_ERR_ACCEPTNG: accept()処理NG
 *   COM_ERR_SENDNG: システムメッセージ送信NG
 *   COM_ERR_TIMENG: 現在日時取得失敗
 * ===========================================================================
 *   マルチスレッドの影響は考慮されていない。
 * ===========================================================================
 * 現在登録されているソケットデータ受信・タイマー満了を監視し、
 * イベントが発生したら、それぞれに対応するコールバック関数を呼んで、
 * その結果を返す。
 *
 * 非同期型であるため、特定のイベントが発生しなければ、何もせずに trueを返す。
 * com_registerStdin()で標準入力を登録していても、本I/Fでは読み取らない。
 * これも com_waitEvent()との大きな差分である。
 *
 * タイマー満了は、本I/Fが呼ばれた時間とタイマー開始時刻との比較で判定する。
 * 指定していた時間を経過していたら「タイマー満了」とする判定であるため、
 * 本I/Fを呼ぶタイミング次第では、正確な時間での満了にはならない可能性がある。
 *
 * デバッグ出力が ONの場合、発生したイベントのログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 */
BOOL com_watchEvent( void );


/*
 * 標準入力受付登録  com_registerStdin()
 *   登録できたら、管理IDを返す。
 *   登録できなかったら COM_NO_SOCK を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iRecvFunc
 * ===========================================================================
 *   マルチスレッドの影響は考慮されていない。
 * ===========================================================================
 * com_waitEvent()で標準入力を受け付けるように登録する。
 * 文字入力後エンターを押すことで、入力となる。
 * iRecvFuncは入力された文字列を通知し、処理するためのコールバック関数。
 *
 * com_waitEvent()を使わないのなら、意味のない登録なので注意すること。
 *
 * 現状返された管理IDは com_cancelStdin()で使う以外に用途はない。
 * プログラム終了時に自動的に解除するので、無理に使う必要もない。
 *
 * 既に標準入力を登録済みの場合、2回目以降の登録は無視され、
 * 最初に返した管理IDを返すのみとなる。
 *
 * デバッグ出力が ONの場合、標準入力登録のログ出力する。
 * その ON/OFF は com_setDebugSelect() で可能。
 */

// 標準入力コールバック関数
//   入力された文字列とそのサイズを通知する。
//   エンターキー押下による最後の '\n' は削除して通知する。
//   この関数の返り値は com_waitEvent()の返り値として使用される。
typedef BOOL(*com_getStdinCB_t)( char *iData, size_t iDataSize );

com_selectId_t com_registerStdin( com_getStdinCB_t iRecvFunc );


/*
 * 標準入力受付解除  com_cancelStdin()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !管理IDの不正
 * ===========================================================================
 *   マルチスレッドの影響は考慮されていない。
 * ===========================================================================
 * 管理IDで指定されたのが標準入力受付であれば、それを解除する。
 *
 * プログラム終了時に自動で解除するので、無理に書く必要はないが、
 * 明示的に標準入力の受付を止めたい時に使える。
 */
void com_cancelStdin( com_selectId_t iId );



/*
 *****************************************************************************
 * COMSELDBG:セレクト機能デバッグ関連I/F
 *****************************************************************************
 */

/*
 * 発生イベントデバッグ出力  com_setDebugSelect()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドの影響を受ける処理はない
 * ===========================================================================
 * iModeで出力ON/OFFを指定する。
 * デフォルトは true(出力ON)なので、止めたければ falseを設定すること。
 *
 * 出力ONにしていると、以下のイベントについてデバッグ出力する。
 *      socket            socket()成功
 *      bind              bind()成功
 *      listen            listen()成功   (TCPサーバー)
 *      connect           connect()成功  (TCPクライアント)
 *      accept            accept()成功   (TCPサーバー)
 *      receive           データ受信
 *      send              データ送信
 *      close_self        自分でclose()  (TCPサーバー/クライアント)
 *      closed_by_dst     対向からの切断 (TCPサーバー/クライアント)
 *      stdin_on          標準入力受付登録
 *      stdin             標準入力受付
 *      stdni_off         標準入力受付解除
 *      timer_on          タイマー登録
 *      timer_expired     タイマー満了
 *      timer_reset       タイマー再設定
 *      timer_stop        タイマー停止
 *      timer_off         タイマー解除
 *
 * デバッグ出力は「SELECT」で始まり、続いてイベント名と管理IDを出力する。次いで
 * ・ソケットであれば、ソケットIDやアドレス情報を出力。
 *   データ送受信の場合、そのデータ内容をバイナリダンプ。
 *   ソケット生成時に限り、rawソケットは IP/Packetどちらなのか出力。
 * ・タイマーであれば、タイマー値やタイマー設定時刻を出力。
 * ・標準入力受付であれば、入力された内容をダンプ。
 *
 * 出力先(画面/ログ)は com_setDebugPrint()の設定に従う。
 * つまり、これが COM_DEBUG_OFF の場合、本I/Fによるデバッグ出力もしない。
 * com_noComDebugLog()を true にしたときもデバッグ出力はしなくなる。
 */
void com_setDebugSelect( BOOL iMode );


/*
 * イベント情報デバッグ出力  com_dispDebugSelect()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドの影響を受ける処理はない
 * ===========================================================================
 * 現在生成されているソケットやタイマーの一覧を出力する。
 * isUse=0 となっているデータは解除されて無効であることに注意すること。
 * それでもデータとして残しているのは主にデバッグ用途のためだが、
 * 新たなソケットやタイマー生成時にいつ上書きされてもおかしくない。
 *
 * 出力先(画面/ログ)は com_setDebugPrint()の設定に従う。
 * つまり、これが COM_DEBUG_OFF の場合、本I/Fによるデバッグ出力もしない。
 * com_noComDebugLog()を true にしたときもデバッグ出力はしなくなる。
 */
void com_dispDebugSelect( void );



// セレクト機能 導入フラグ
#define USING_COM_SELECT
// セレクト機能 初期化関数マクロ  ＊COM_INITIALIZE()で使用
#ifdef  COM_INIT_SELECT
#undef  COM_INIT_SELECT
#endif
#define COM_INIT_SELECT  com_initializeSelect()

