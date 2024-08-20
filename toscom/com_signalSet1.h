/*
 *****************************************************************************
 *
 * 共通部シグナル機能 (プロトコルセット1)    by TOS
 *
 *   シグナル機能の、プロトコル別I/Fのセット1(基本セット)となる。
 *   本ヘッダは com_signal.h でインクルードされているため、
 *   個別にインクルードするのではなく com_signal.h のみインクルードして、
 *   開発を進めること。
 *
 *   対応しているプロトコルについては PROTOCOL_AVAILABLE を参照。
 *
 *   本ヘッダでは本来の命名ルールに従わず、com_ をつけない構造体宣言や、
 *   COM_ をつけないマクロ宣言が存在する。
 *   これは Linuxではヘッダに宣言されている構造体やマクロで、Cygwinには同名の
 *   ヘッダが存在せずインクルードが出来なかったため、toscomで使用する構造体や
 *   マクロを、独自で宣言する必要があった、という理由による。
 *
 *****************************************************************************
 */

#pragma once

#ifdef __linux__
#include <net/ethernet.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <net/if_arp.h>
#endif

#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include "com_signal.h"
#include "com_signalCom.h"



// PROTOCOL_AVAILABLE
//   セット1では IPに関連する基本的な以下のプロトコルをサポートする。
enum {
    /*** ファイルタイプ (1-99) ***/
    /*** link層 (101-199) ***/
    COM_SIG_SLL      = 101,    // DLT_EN10MB     (0x01)
    COM_SIG_ETHER2   = 102,    // DLT_LINUX_SLL  (0x71)
    /*** Network層 (201-299) ***/
    COM_SIG_IPV4     = 201,    // ETHERTYPE_IP   (0x0800) RFC791
    COM_SIG_IPV6     = 202,    // ETHERTYPE_IPV6 (0x86dd) RFC2460 -> 8200
    COM_SIG_ICMP     = 203,    // IPPROTO_ICMP   (0x01)   RFC792
    COM_SIG_ICMPV6   = 204,    // IPPROTO_ICMPV6 (0x3a)   RFC4443
    COM_SIG_ARP      = 205,    // ETHERTYPE_ARP  (0x0806) RFC826
    /*** Transport層 (301-399) ***/
    COM_SIG_TCP      = 301,    // IPPROTO_TCP    (0x06)   RFC793
    COM_SIG_UDP      = 302,    // IPPROTO_UDP    (0x11)   RFC768
    COM_SIG_SCTP     = 303,    // IPPROTO_SCTP   (0x84)   RFC2960 -> 4960
    /*** No.7系 (401-499) ***/
    /*** Application層 (501-599) ***/
    COM_SIG_SIP      = 501,    // port 5060     RFC3261
    COM_SIG_SDP      = 502,    //               RFC2327 -> 4566
    COM_SIG_RTP      = 503,    // port 5004     RFC3550
    COM_SIG_RTCP     = 504,    // port 5005     RFC3550
    COM_SIG_DIAMETER = 505,    // port 3868     RFC3588 3GPP/3GPP2も一部あり
    COM_SIG_DHCP     = 506,    // port 67/68    RFC2131
    COM_SIG_DHCPV6   = 507,    // port 546/547  RFC3315 -> 8415
    COM_SIG_DNS      = 508,    // port 53       RFC1036
    /*** プロトコル認識のみ (601-699) ***/
};

/*
 * シグナル機能セット1初期化  com_initializeSigSet1()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 * com_initializeSignal() から呼ばれるため、本I/Fを直接使用する必要はない。
 * 
 * 追加セットにも 同様の名前の com_initializeSigSet～() という初期化I/Fがある。
 * こちらは必要な場合は呼ぶ必要があるので注意すること。
 * (詳細は各セットのヘッダファイルを参照)
 */
void com_initializeSigSet1( void );



/*
 *****************************************************************************
 * リンク層 (SLL/Ether2)
 *****************************************************************************
 */

// 参考情報：リンク層ヘッダ
//
//   リンク層ヘッダは SLL(0x71) と Ether2(0x01) に対応する。
//   ファイルヘッダから読んでいけば、どちらなのか明確に特定可能。
//
// ＊SLLヘッダ構造  (Wiresharkでは「Linux cooked capture」と表示)
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
// |Packet |ARPHRD_|LL addr|     LL(Link-Layer) address    | proto |
// |  type |  type | length|                               |  col  |
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//   <pcap/sll.h>の struct sll_header で構造体宣言されているが、
//   libpcap未インストールの環境でも動作するように、
//   toscomでは独自の構造体 com_sigSllHead_t を用意する。
//
// ＊EtherIIヘッダ構造  (DIX = DEC,Intel,Xerox)
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+
// |    Destination        |     Source            | Type  |
// |       MAC address     |        MAC address    |       |
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//   <net/ethernet.h>の struct ether_header で構造体宣言されている。
//   Cygwinではこのヘッダが無いため、本ヘッダで敢えて定義する。

#ifdef __CYGWIN__  // Linuxでは net/ethernet.h があるので参考情報で

// 次プロトコル種別設定値 (com_setPrtclType()の:COM_LINKNEXT用)
#define  ETHERTYPE_IP    (0x0800)    // COM_SIG_IP4 になる
#define  ETHERTYPE_IPV6  (0x86dd)    // COM_SIG_IP6 になる
#define  ETHERTYPE_ARP   (0x0806)    // COM_SIG_ARP になる

#endif // __CYGWIN__


// <pcap/sll.h> の struct sll_header の代替構造体
typedef struct {
    uint16_t   pkttype;
    uint16_t   hattype;
    uint16_t   halen;
    uint8_t    addr[8];
    uint16_t   protocol;
} com_sigSllHead_t;

/*
 * プロトコル個別I/F (SLL)
 *   com_analyzeSll():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeSll():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeSll():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeSll()で ioHead->sigの信号データを SLLヘッダとして解析する。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       SLLヘッダサイズ (sizeof(com_sigSllHead_t))
 *     .ptype     COM_SIG_SLL
 *   .next
 *     .cnt       1固定
 *     .stack[0]
 *       .sig     SLLヘッダ以降のデータ
 *                .ptypeは SLLヘッダの .protocol から取得する
 *       .order   ioHead->orderの値
 *       .prev    ioHead自身
 * ioHead->sig.top を com_sigSllHead_t* でキャストしてヘッダ取得可能。
 *
 * 次プロトコル値(ioHead->next.stack[0].sig.ptype)は COM_LINKNEXT を指定した
 * com_getPrtclType()によって特定される。
 *
 *
 * com_decodeSll()で解析した iHeadの内容をデコード出力する。
 * ・SLLヘッダの内容ダンプ
 * ・次プロトコル
 */
BOOL com_analyzeSll( COM_ANALYZER_PRM );
void com_decodeSll( COM_DECODER_PRM );


// 参考情報：VLANタグについて
//
//   EtherIIヘッダには IEEE802.1Qで定められた VLANタグが入る可能性がある。
//   これは2つのMACアドレスの後に挿入される 4byteのデータで、
//   これが入ると次プロトコルを示す Type が ETH_P_8021Q(0x8100) になる。
//   つまり EtherIIヘッダのサイズが +4 される。これには対応済み。
//
//   しかし実際は複数のVLANタグが付く可能性もある。
//   また ETH_P_8021Q(0x8100) 以外の値がベンダー依存で入る可能性もある。
//   どんな値が付けられるかは予測不可能なため、データとして追加可能とする。
//
//   新たな値を登録したい場合、com_initializeSigSet1()で記述しているように
//   com_setBoolTable()を使い、COM_VLANTAG に対して必要な値を追加する。
//   そうすれば com_analyzeEth2()では その値を使った VLANタグに対応する。

#ifdef __CYGWIN__  // Linuxでは linux/if_ether.h があるので参考情報で

#define  ETH_P_8021Q   (0x8100)
#define  ETH_ALEN      (6)

#endif // __CYGWIN__

#ifdef __CYGWIN__  // Linuxでは net/ethernet.h があるので参考情報で

struct ether_header {
    u_int8_t   ether_dhost[ETH_ALEN];    /* destination eth addr */
    u_int8_t   ether_shost[ETH_ALEN];    /* source eth addr */
    u_int16_t  ether_type;               /* packet type ID field */
};

#endif // __CYGWIN__

/*
 * プロトコル個別I/F (Ether2)
 *   com_analyzeEth2():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeEth2():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeEth2():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeEth2()で ioHead->sigの信号データを Ether2ヘッダとして解析する。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       Ether2ヘッダサイズ (sizeof(struct ether_header))
 *     .ptype     COM_SIG_ETHER2
 *   .prm         VLANタグがあった場合、タグとその内容を格納
 *   .next
 *     .cnt       1固定
 *     .stack[0]
 *       .sig     Ether2ヘッダ以降のデータ
 *                .ptypeは Ether2ヘッダの ether_typeから取得する
 *       .order   ioHead->orderの値
 *       .prev    ioHead自身
 * ioHead->sig.top を struct ether_header* でキャストしてヘッダ取得可能。
 * VLANタグがあった場合 .prm にその内容を格納する。その場合 次プロトコルを
 * 示す値の位置が struct ether_headerからは取得できなくなるので注意すること。
 * (解析処理の中で正しく値は取得し .next.stack[0].sig.ptype に設定する)
 *
 * 前述した通り、VLANタグとしてサポートしているのは ETH_P_8021Qのみで、
 * それ以外に対応したい場合は、そのタグ値を登録する必要がある。
 *
 * 次プロトコル値(ioHead->next.stack[0].sig.ptype)は COM_LINKNEXT を指定した
 * com_getPrtclType()によって特定される。
 *
 *
 * com_decodeEth2()で解析した iHeadの内容をデコード出力する。
 * ・Ether2ヘッダの内容ダンプ
 * ・次プロトコル
 */
BOOL com_analyzeEth2( COM_ANALYZER_PRM );
void com_decodeEth2( COM_DECODER_PRM );

/*
 * VLANタグ名取得  com_getVlanTagName()
 *   タグ名文字列を返す。
 *   対応する文字列が見つからない場合は "VLAN tag"を返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iTagで指定したVLANタグ値(バイトオーダー変換済みを想定)に対応する
 * タグ名の文字列を返す。
 * gVlanTagName[]から文字列を検索する。
 *
 * データ登録されていないタグ値だった場合は "VLAN tag" を固定で返す。
 * 独自に VLANタグを追加していた場合は、これになる。
 */
char *com_getVlanTagName( uint16_t iTag );



/*
 *****************************************************************************
 * ネットワーク層 (IPv4/IPv6)
 *****************************************************************************
 */

// 参考情報：次プロトコル値設定  (com_setPrtclType(): COM_IPNEXT用)
//
//   IPPROTO_TCP     (0x06)   COM_SIG_TCP になる
//   IPPROTO_UDP     (0x11)   COM_SIG_UDP になる
//   IPPROTO_SCTP    (0x84)   COM_SIG_SCTP になる
//   IPPROTO_ICMP    (0x01)   COM_SIG_ICMP になる
//   IPPROTO_ICMPV6  (0x3a)   COM_SIG_ICMPV6 になる

#ifdef __CYGWIN__   // Cygwinで宣言していないものを独自宣言

enum {
    IPPROTO_SCTP = 0x84
};

#endif // __CYGWIN__

// IPヘッダにおけるバージョン値 (IPv4/IPv6で標準ヘッダは宣言がまちまち)
enum {
    COM_CAP_IPV4 = 4,  // netinet/ip.h の IPVERSION
    COM_CAP_IPV6 = 6   // nwrinwr/ip6.h の IPV6_VERSION は 0x60 になっている
};

// 参考情報：IPv4のヘッダ構造体 struct ip (netinet/ip.h)
#if 0  // IPv4Header (Linux限定の struct iphdr は使わないことにする)
struct ip {
#ifdef _IP_VHL
    u_int8_t  ip_vhl;       /* version << 4 | header length >> 2 */
#else
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ip_hl:4,       /* header length */
             ip_v:4;        /* version */
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned int ip_v:4,        /* version */
             ip_hl:4;       /* header length */
#endif
#endif /* not _IP_VHL */
    u_int8_t   ip_tos;      /* type of service */
    u_int16_t ip_len;       /* total length */
    u_int16_t ip_id;        /* identification */
    u_int16_t ip_off;       /* fragment offset field */
#define IP_RF 0x8000            /* reserved fragment flag */
#define IP_DF 0x4000            /* dont fragment flag */
#define IP_MF 0x2000            /* more fragments flag */
#define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
    u_int8_t  ip_ttl;       /* time to live */
    u_int8_t  ip_p;         /* protocol */
    u_int16_t ip_sum;       /* checksum */
    struct  in_addr ip_src,ip_dst;  /* source and dest address */
};
#endif // IPv4Header

// 参考情報：IPv4のフラグメント情報マスク定義 (netinet/ip.h)
//
//   フラグメント情報のビットマスクは
//   上記の IP_RF/IP_DF/IP_MF/IP_OFFMASK を使用する。

/*
 * プロトコル個別I/F (IPv4)
 *   com_analyzeIpv4():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeIpv4():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeIpv4():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeIpv4()で ioHead->sigの信号データを IPv4ヘッダとして解析する。
 * 参照勧告は RFC791(section 3.1)・RFC2113。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .top       IPフラグメント結合した場合、その結合結果に差し替わる
 *     .len       IPv4ヘッダサイズ (ipv4->ip_hl * 4)
 *     .ptype     COM_SIG_IPV4 (IPフラグメント断片だった場合 COM_SIG_FRAGMENT)
 *   .isFragment  IPフラグメント断片だった場合 true設定
 *   .ras         IPフラグメント結合したら、その結果を格納
 *   .prm         IPv4オプションがあった場合、その内容を格納
 *   .next
 *     .cnt       1固定
 *     .stack[0]
 *       .sig     IPv4ヘッダ以降のデータ
 *                .ptypeは IPv4ヘッダの ip_pから取得する。
 *       .order   ioHead->orderの値
 *       .prev    ioHead自身
 * ioHead->sig.top を struct ip* でキャストしてヘッダ取得可能。
 * IPv4オプションが含まれる場合 .prm に内容を格納する。
 *
 * 次プロトコル値(ioHead->next.stack[0].sig.ptype)は COM_IPNEXT を指定した
 * com_getPrtclType()によって特定される。
 *
 * IPフラグメントありの場合、ペイロードデータを一時蓄積し、ioHead->sig.ptypeを
 * COM_SIG_FRAGMENT にする。全フラグメントが揃ったら、結合して .ras に格納する。
 * テキストログ解析時は「Reassembled IPv4～」という文字列を検出した場合、
 * それ以降にフラグメント結合データがあるとみなし、その内容を .ras に格納し、
 * 各フラグメントの結合より、こちらを優先して使用する。
 *
 *
 * com_decodeIpv4()で解析した iHeadの内容をデコード出力する。
 * ・IPv4ヘッダの内容ダンプ
 * ・送信元IPアドレス
 * ・送信先IPアドレス
 * ・識別子
 * ・フラグメント情報
 * ・IPv4オプション一覧
 * ・次プロトコル
 */
BOOL com_analyzeIpv4( COM_ANALYZER_PRM );
void com_decodeIpv4( COM_DECODER_PRM );

/*
 * IPv4オプション名取得  com_getIpv4OptName()
 *   オプション名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iTypeで指定したオプション種別値に対応するオプション名文字列を返す。
 * gIPv4OptName[]から文字列を検索する。
 */
char *com_getIpv4OptName( ulong iType );


// 参考情報：IPv6のヘッダ構造体 struct ip6_hdr (netinet/ip6.h)
#if 0  // IPv6Header
struct ip6_hdr {
    union {
        struct ip6_hdrctl {
            u_int32_t ip6_un1_flow; /* 20 bits of flow-ID */
            u_int16_t ip6_un1_plen; /* payload length */
            u_int8_t  ip6_un1_nxt;  /* next header */
            u_int8_t  ip6_un1_hlim; /* hop limit */
        } ip6_un1;
        u_int8_t ip6_un2_vfc;   /* 4 bits version, top 4 bits class */
    } ip6_ctlun;
    struct in6_addr ip6_src;    /* source address */
    struct in6_addr ip6_dst;    /* destination address */
} __packed;

#define ip6_vfc     ip6_ctlun.ip6_un2_vfc
#define ip6_flow    ip6_ctlun.ip6_un1.ip6_un1_flow
#define ip6_plen    ip6_ctlun.ip6_un1.ip6_un1_plen
#define ip6_nxt     ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_hlim    ip6_ctlun.ip6_un1.ip6_un1_hlim
#define ip6_hops    ip6_ctlun.ip6_un1.ip6_un1_hlim
#endif // IPv6Header

// 参考情報：IPv6拡張ヘッダ構造体 struct ip6_ext/ip6_frag(netinet/ip6.h)
//   他の拡張ヘッダの構造体定義もあるが、使用するのは下記のみ
#if 0  // IPv6ExtensionHeader
struct  ip6_ext {
    u_int8_t ip6e_nxt;
    u_int8_t ip6e_len;
} __packed;

/* Fragment header */
struct ip6_frag {
    u_int8_t  ip6f_nxt;     /* next header */
    u_int8_t  ip6f_reserved;    /* reserved field */
    u_int16_t ip6f_offlg;       /* offset, reserved, and flag */
    u_int32_t ip6f_ident;       /* identification */
} __packed;
#endif // IPv6ExtensionHeader

// 参考情報：IPv6のフラグメント情報マスク定義 (netinet/ip6.h)
#if 0  // IPv6FragmentMask
#if BYTE_ORDER == BIG_ENDIAN
#define IP6F_OFF_MASK       0xfff8  /* mask out offset from _offlg */
#define IP6F_RESERVED_MASK  0x0006  /* reserved bits in ip6f_offlg */
#define IP6F_MORE_FRAG      0x0001  /* more-fragments flag */
#else /* BYTE_ORDER == LITTLE_ENDIAN */
#define IP6F_OFF_MASK       0xf8ff  /* mask out offset from _offlg */
#define IP6F_RESERVED_MASK  0x0600  /* reserved bits in ip6f_offlg */
#define IP6F_MORE_FRAG      0x0100  /* more-fragments flag */
#endif /* BYTE_ORDER == LITTLE_ENDIAN */
#endif // IPv6FragmentMask

/*
 * プロトコル個別I/F (IPv6)
 *   com_analyzeIpv6():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeIpv6():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeIpv6():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeIpv6()で ioHead->sigの信号データを IPv6ヘッダとして解析する。
 * 参照勧告は RFC8200(section 3)。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .top       IPフラグメント結合した場合、その結合結果に差し替わる
 *     .len       IPv6ヘッダサイズ (拡張ヘッダも含めたサイズ)
 *     .ptype     COM_SIG_IPV6 (IPフラグメント断片だった場合 COM_SIG_FRAGMENT)
 *   .isFragment  IPフラグメント断片だった場合 true設定
 *   .ras         IPフラグメント結合したら、その結果を格納
 *   .prm         IPv6拡張ヘッダがあった場合、その内容を格納
 *                (.tagにヘッダ種別、.lenにヘッダ長、.valueにデータ先頭)
 *   .next
 *     .cnt       1固定
 *     .stack[0]
 *       .sig     IPv6ヘッダ以降のデータ
 *                .ptypeは IPv6ヘッダの ip6_nxtから取得する。
 *                (拡張ヘッダがある場合は、その拡張ヘッダの ip6e_nxtから)
 *       .order   ioHead->orderの値
 *       .prev    ioHead自身
 * ioHead->sig.top を struct ip6_hdr* でキャストしてヘッダ取得可能。
 * IPv6拡張ヘッダがある場合 .prm にその内容を格納する。
 *
 * 次プロトコル値(ioHead->next.stack[0].sig.ptype)は COM_IPNEXT を指定した
 * com_getPrtclType()によって特定される。
 *
 * IPフラグメントありの場合、ペイロードデータを一時蓄積し、ioHead->sig.ptypeを
 * COM_SIG_FRAGMENT にする。全フラグメントが揃ったら、結合して .ras に格納する。
 * テキストログ解析時は「Reassembled IPv6～」という文字列を検出した場合、
 * それ以降にフラグメント結合データがあるとみなし、その内容を .ras に格納し、
 * 各フラグメントの結合より、こちらを優先して使用する。
 *
 *
 * com_decodeIpv6()で解析した iHeadの内容をデコード出力する。
 * ・IPv6ヘッダの内容ダンプ
 * ・flow-ID
 * ・送信元IPアドレス
 * ・送信先IPアドレス
 * ・IPv6拡張ヘッダ一覧
 * ・次プロトコル
 */
BOOL com_analyzeIpv6( COM_ANALYZER_PRM );
void com_decodeIpv6( COM_DECODER_PRM );

/*
 * IPv5拡張ヘッダ名取得  com_getIpv6ExtHdrName()
 *   拡張ヘッダ名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iCodeで指定したプロトコル識別値に対応する拡張ヘッダ名文字列を返す。
 * gIpv6ExtHdrName[]から文字列を検索する。
 */
char* com_getIpv6ExtHdrName( ulong iCode );


/*
 *****************************************************************************
 * ネットワーク層 (ICMP)
 *****************************************************************************
 */

#ifdef __CYGWIN__  // Linuxでは netnet/ip_icmp.h に記述があるので参考情報で

struct icmphdr {
    u_int8_t  type;           /* message type */
    u_int8_t  code;           /* type sub-code */
    u_int16_t  checksum;
    union {
        struct {
            u_int16_t  id;
            u_int16_t  sequence;
        } echo;               /* echo datagram */
        u_int32_t  gateway;   /* gateway address */
        struct {
            u_int16_t  unused;
            u_int16_t  mtu;
        } frag;               /* path mtu discovery */
    } un;
};

#define  ICMP_ECHOREPLY       0   /* Echo Reply */
#define  ICMP_DEST_UNREACH    3   /* Destination Unreachable */
#define  ICMP_SOURCE_QUENCH   4   /* Source Quench */
#define  ICMP_REDIRECT        5   /* Redirect (change route) */
#define  ICMP_ECHO            8   /* Echo Request */
#define  ICMP_TIME_EXCEEDED   11  /* Time Exceeded */
#define  ICMP_PARAMETERPROB   12  /* Parameter Problem */
#define  ICMP_TIMESTAMP       13  /* Timestamp Request */
#define  ICMP_TIMESTAMPREPLY  14  /* Timestamp Reply */
#define  ICMP_INFO_REQUEST    15  /* Information Request */
#define  ICMP_INFO_REPLY      16  /* Information Reply */
#define  ICMP_ADDRESS         17  /* Address Mask Request */
#define  ICMP_ADDRESSREPLY    18  /* Address Mask Reply */

#endif // __CYGWIN__

/*
 * プロトコル個別I/F (ICMP)
 *   com_analyzeIcmp():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeIcmp():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeIcmp():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeIcmp()で ioHead->sigの信号データを ICMP信号として解析する。
 * 参照勧告は RFC792。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       ICMPヘッダサイズ (sizeof(struct icmphdr)
 *     .ptype     COM_SIG_ICMP
 *   .prm         ICMPオプションがあった場合、その内容を格納
 *   .multi
 *     .cnt       1固定
 *     .stack[0]
 *       .sig     ICMP基本ヘッダ以降のデータ (.ptypeは COM_SIG_ICMP)
 *       .order   ioHead->orderの値
 *       .prev    ioHead自身
 * ioHead->sig.top を struct icmphdr* でキャストしてヘッダ取得可能。
 * ICMPオプションがある場合 .prm にその内容を格納する。
 * ヘッダ以降の内容は .multi に格納する。
 *
 *
 * com_decodeIcmp()で解析した iHeadの内容をデコード出力する。
 * ・ICMPヘッダとボディのダンプ
 * ・ICMPメッセージ種別
 * ・Destination Unreachableの場合、送信できなかった信号内容もデコード
 */
BOOL com_analyzeIcmp( COM_ANALYZER_PRM );
void com_decodeIcmp( COM_DECODER_PRM );

/*
 * ICMPメッセージ種別名取得  com_getIcmpTypeName()
 *   信号種別名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSigTopでICMP信号データの先頭を指定することで、メッセージ種別の文字列を返す。
 * gIcmpTypeName[]から文字列を検索する。
 */
char *com_getIcmpTypeName( void *iSigTop );



/*
 *****************************************************************************
 * ネットワーク層 (ICMPv6)
 *****************************************************************************
 */

#ifdef __CYGWIN__  // Linuxでは netnet/icmp6.h があるので参考情報で

struct icmp6_hdr {
    uint8_t    icmp6_type;     /* type field */
    uint8_t    icmp6_code;     /* code field */
    uint16_t   icmp6_cksum;    /* checksum field */
    union {
        uint32_t  icmp6_un_data32[1];    /* type-specific field */
        uint16_t  icmp6_un_data16[2];    /* type-specific field */
        uint8_t   icmp6_un_data8[4];     /* type-specific field */
    } icmp6_dataun;
};

#define  ICMP6_DST_UNREACH            1
#define  ICMP6_PACKET_TOO_BIG         2
#define  ICMP6_TIME_EXCEEDED          3
#define  ICMP6_PARAM_PROB             4

#define  ICMP6_ECHO_REQUEST           128
#define  ICMP6_ECHO_REPLY             129
#define  MLD_LISTENER_QUERY           130    // 参考情報
#define  MLD_LISTENER_REPORT          131    // 参考情報
#define  MLD_LISTENER_REDUCTION       132    // 参考情報

#define  ND_ROUTER_SOLICIT            133
#define  ND_ROUTER_ADVERT             134
#define  ND_NEIGHBOR_SOLICIT          135
#define  ND_NEIGHBOR_ADVERT           136
#define  ND_REDIRECT                  137    // 参考情報

#endif // __CYGWIN__

/*
 * プロトコル個別I/F (ICMPv6)
 *   com_analyzeIcmpv6():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeIcmpv6():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeIcmpv6():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeIcmpv6()で ioHead->sigの信号データを ICMPv6信号として解析する。
 * 参照勧告は RFC4443/2461。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       ICMPv6ヘッダサイズ (sizeof(struct icmp6_hdr)
 *     .ptype     COM_SIG_ICMPV6
 *   .multi       ICMPv6基本ヘッダ(struct icmp6_hdr)以降のデータ
 *     .cnt       1固定
 *     .stack[0]
 *       .sig     ICMPv6基本ヘッダ以降のデータ (.ptypeは COM_SIG_ICMPV6)
 *       .order   ioHead->orderの値
 * ioHead->sig.top を struct icmp6_hdr* でキャストしてヘッダ取得可能。
 * ヘッダ以降の内容は .multi に格納する。
 *
 *
 * com_decodeIcmpv6()で解析した iHeadの内容をデコード出力する。
 * ・ICMPv6ヘッダとボディのダンプ
 * ・ICMPv6メッセージ種別
 */
BOOL com_analyzeIcmpv6( COM_ANALYZER_PRM );
void com_decodeIcmpv6( COM_DECODER_PRM );

/*
 * ICMPv6メッセージ種別名取得  com_getIcmpv6TypeName()
 *   信号種別名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSigTopでICMPv6信号データの先頭を指定することで、メッセージ種別の文字列を
 * 返す。
 * gIcmpv6TypeName[]から文字列を検索する。
 */
char *com_getIcmpv6TypeName( void *iSigTop );



/*
 *****************************************************************************
 * ネットワーク層 (ARP)
 *****************************************************************************
 */

#ifdef __CYGWIN__  // Linuxでは net/if_arp.h があるので参考情報で

struct arphdr {
    ushort  ar_hrd;     /* Format of hardware address */
    ushort  ar_pro;     /* Format of protocol address */
    uchar   ar_hln;     /* Length of hardware address */
    uchar   ar_pln;     /* Length of protocol address */
    ushort  ar_op;      /* ARP opcode (command) */
#if 0 // この後に以下のようなイメージでデータが続くだろうという例
    /* Ehternet looks like this : This bit is variable sized however... */
    uchar  __ar_sha[ETH_ALEN];       /* Sender hardware address */
    uchar  __ar_sip[4];              /* Sender IP address */
    uchar  __ar_tha[ETH_ALEN];       /* Target hardware address */
    uchar  __ar_tip[4];              /* Target IP address */
#endif
};

#define ARPOP_REQUEST      1        /* ARP request. */
#define ARPOP_REPLY        2        /* ARP reply. */
#define ARPOP_RREQUEST     3        /* RARP request/ */
#define ARPOP_RREPLY       4        /* RARP reply. */
#define ARPOP_InREQUEST    8        /* InARP request. */
#define ARPOP_InREPLY      9        /* InARP reply. */
#define ARPOP_NAK         10        /* (ATM)ARP NAK. */

#endif // __CYGWIN__

/*
 * プロトコル個別I/F (ARP)
 *   com_analyzeArp():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeArp():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeArp():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeArp()で ioHead->sigの信号データを ARP信号として解析する。
 * 参照勧告は RFC826。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       ARPヘッダサイズ (sizeof(struct arphdr)
 *     .ptype     COM_SIG_ARP
 *   .multi       ARPヘッダ(struct arphdr)以降のデータ
 *     .cnt       1固定
 *     .stack[0]
 *       .sig     ARPヘッダ以降のデータ (.ptypeは COM_SIG_ARP)
 *       .order   ioHead->orderの値
 * ioHead->sig.top を struct arphdr* でキャストしてヘッダ取得可能。
 * ヘッダ以降の内容は .multi に格納する。
 *
 *
 * com_decodeArp()で解析した iHeadの内容をデコード出力する。
 * ・ARPヘッダとボディのダンプ
 * ・オペレーション種別
 */
BOOL com_analyzeArp( COM_ANALYZER_PRM );
void com_decodeArp( COM_DECODER_PRM );

/*
 * ARPオペレーション名取得  com_getArpOpName()
 *   オペレーション名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSigTopでARP信号データの先頭を指定することで、信号種別の文字列を返す。
 * iOrderにはバイトオーダー(例えば com_sigInf_t型の .order)を設定する。
 * gArpOpName[]から文字列を検索する。
 */
char *com_getArpOpName( void *iSigTop, long iOrder );



/*
 *****************************************************************************
 * トランスポート層 (TCP)
 *****************************************************************************
 */

// TCPノード情報データ構造
typedef struct {
    uchar  addr[16];    // IPアドレス
    ulong  port;        // TCPポート番号
} com_sigIpNode_t;

typedef struct {
    com_sigIpNode_t  src;    // source情報
    com_sigIpNode_t  dst;    // destination情報
    BOOL  isReverse;
} com_sigTcpNode_t;

typedef enum {
    COM_SIG_TS_NONE = 0,     // 何も処理されていない
    COM_SIG_TS_OFFER,        // 通知実施
    COM_SIG_TS_ACK           // 通知に対する応答実施
} COM_SIG_TCPSTAT_t;

// TCP情報データ構造
typedef struct {
    BOOL    isReverse;
    ulong   seqSyn;          // TCPシーケンス番号 (同期初期値）
    ulong   ackSyn;          // TCP ACK番号 (同期初期値)
    ulong   srcRecv;
    ulong   dstRecv;
    long    statSyn;         // SYN進行度  COM_SIG_TCPSTAT_t型の値を格納
    long    statFin;         // FIN進行度  COM_SIG_TCPSTAT_t型の値を格納
} com_sigTcpInf_t;

// 参考情報：TCPヘッダ構造体 struct tcphdr (netinet/tcp.h)
//   古い構造体は th_ がメンバー名に付かないなど、作りに違いがある。
//   Linuxでは2つの形式のどちらかを選ぶか、unionでどちらでもよくするか
//   gccの版数によって扱いがまちまちになっている。
#if 0  // TcpHeader
struct tcphdr {
    u_int16_t th_sport;             /* source port */
    u_int16_t th_dport;             /* destination port */
    tcp_seq   th_seq;               /* sequence number */
    tcp_seq   th_ack;               /* acknowledgement number */
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int  th_x2:4,          /* (unused) */
              th_off:4;         /* data offset */
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned int  th_off:4,         /* data offset */
              th_x2:4;          /* (unused) */
#endif
    u_int8_t  th_flags;
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG)

    u_int16_t th_win;                 /* window */
    u_int16_t th_sum;                 /* checksum */
    u_int16_t th_urp;                 /* urgent pointer */
};

#endif // TcpHeader

// struct tcphdr*  tcpでヘッダ情報を取り込んでいるとき限定のフラグチェック
#define  COM_IS_TCP_FIN    COM_CHECKBIT( tcp->th_flags, TH_FIN )
#define  COM_IS_TCP_SYN    COM_CHECKBIT( tcp->th_flags, TH_SYN )
#define  COM_IS_TCP_RST    COM_CHECKBIT( tcp->th_flags, TH_RST )
#define  COM_IS_TCP_PSH    COM_CHECKBIT( tcp->th_flags, TH_PUSH )
#define  COM_IS_TCP_ACK    COM_CHECKBIT( tcp->th_flags, TH_ACK )
#define  COM_IS_TCP_URG    COM_CHECKBIT( tcp->th_flags, TH_URG )

/*
 * プロトコル個別I/F (TCP)
 *   com_analyzeTcp():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeTcp():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeTcp():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeTcp()で ioHead->sigの信号データを TCP信号として解析する。
 * 参照勧告は RFC793(section 3.1)。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       TCPヘッダサイズ (sizeof(struct tcphdr)
 *     .ptype     COM_SIG_TCP
 *   .prm         TCPオプションがあれば格納
 *   .ext         次プロトコルを判定するために使用したポート番号(long*型)
 *   .next        ペイロードデータ
 *     .cnt       1固定
 *     .stack[0]
 *       .sig     TCPヘッダ以降のデータ
 *                .ptypeは TCPヘッダのポート番号から取得する。
 *       .order   ioHead->orderの値
 *       .prev    ioHead自身
 * ioHead->sig.top を struct tcphdr* でキャストしてヘッダ取得可能。
 *
 * 次プロトコルを特定するためのポート番号は 送信元->送信先 の順で調べる。
 * 最終的に使用したポート番号を .ext に long*型で格納する。
 * 次プロトコル値(ioHead->next.stack[0].sig.ptype)は COM_IPPORT を指定した
 * com_getPrtclType()によって特定される。
 * プロトコルが特定できなかったら 次プロトコル種別は COM_SIG_CONTINUE になる。
 *
 * TCPヘッダのシーケンス番号と ACK番号については .prm.spec に保持している。
 * そのデータ型は com_sigTcpInf_t*型となっている。
 * SYNで情報を生成し、FINでメモリ解放する。
 * しかし常に SYNがあるとは限らない為無い場合は ACKを元に相対シーケンス番号を
 * 出すような作りとしている。(これは Wireshark と同様の手法となる)
 *
 * TCPセグメンテーションの認識と結合にも対応しているが、これについては
 * com_stockTcpSeg()・com_continueTcpSeg()・com_reassembleTcpSeg()の説明を参照。
 * TCPセグメンテーションがあるかどうかは、さらにその上位プロトコルスタックで
 * データサイズの総量を認識する必要があるため、TCPだけでは判定できない。
 * そのため上記の TCPセグメンテーションの処理I/Fは、上位プロトコルの解析時に、
 * 必要に応じて呼ぶ形となっている。
 *
 *
 * com_decodeTcp()で解析した iHeadの内容をデコード出力する。
 * ・TCPヘッダ内容のダンプ
 * ・送信元ポート番号
 * ・送信先ポート番号
 * ・シーケンス番号
 * ・制御フラグ
 * ・TCPオプション一覧
 * ・次プロトコル
 */
BOOL com_analyzeTcp( COM_ANALYZER_PRM );
void com_decodeTcp( COM_DECODER_PRM );

/*
 * TCPオプション名取得  com_getTcpOptName()
 *   オプション名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iTypeで指定したオプション値に対応するオプション名文字列を返す。
 * gTcpOptName[]から文字列を検索する。
 */
char *com_getTcpOptName( ulong iType );

/*
 * TCPセグメンテーション データ保持  com_stockTcpSeg()
 *   実際にデータを保持したフラグメント情報のアドレスを返す。
 *   処理NG時は NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNH] !ioTarget || !iTcp
 *   com_stockFragments()によるエラー
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * ioTargetに実際のセグメントデータを有するプロトコルデータ
 * (TCPの直上スタックのプロトコルデータになると想定している)
 * iTcpにそのデータと同じパケット内でスタックされている TCP部のデータ、
 * iTotalSizeにセグメンテーションされたデータの全長を指定することで、
 * セグメントデータを toscomのフラグメント情報(com_sigFrg_t型)に格納し、
 * そのデータのアドレスを返す。
 *
 * 基本的には一番最初に iTotalSizeを指定して本I/Fを使用し、
 * (TCPより上位のプロトコルで算出する手立てがあるはずと期待する)
 * その後のセグメンテーションデータ保持は com_continueTcpSeg()の使用し、
 * 受信したデータ総量が iTotalSizeに達したらデータを結合する流れを想定している。
 *
 * iTotalSizeは不明の場合 0 でも構わない(データ保持を優先)。
 * しかし上位プロトコルでそのサイズが分かったら、その値を指定して本I/Fを
 * 使って通知すること。
 * そうしなければ com_continueTcpSeg()でセグメンテーション終了の判定ができない。
 *
 * 本I/FによるTCPセグメンテーション処理は暫定対処の扱い。
 * ただ上位プロトコルでなければセグメンテーションデータの全長が分からない
 * という現状では、これ以外に方法はないと考えられる。
 * com_analyzeTctBase() で本I/Fを使う処理が記述されている。
 */
com_sigFrg_t *com_stockTcpSeg(
        com_sigInf_t *ioTarget, com_sigInf_t *iTcp, com_off iTotalSize );

/*
 * TCPセグメンテーション データ継続  com_continueTcpSeg()
 *   セグメントデータを全て取得したら trueを返す。
 *   まだ全て取得できていないときや、処理NG時は falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNH] !ioTarget || !iTcp
 *   com_stockFragments()によるエラー
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * ioTargetと iTcpの指定方法は com_stockTcpSeg()と同様となる。
 * 最初に com_stockTcpSeg()を実行後、以後は本I/Fを使用することを想定する。
 * 本I/Fはセグメントデータを収集し、全てのデータが揃ったかどうか、内部で
 * com_reassembleTcpSeg()を使用してチェックし、揃っていたらデータを結合する。
 * 結合した結果は ioTarget->ras に設定し、ioTarget->sigの内容を差し替えて、
 * trueを返す。その内容をプロトコル解析I/Fにかければ良いことになる。
 *
 * 処理NGが発生した場合、フラグメント情報はメモリ解放して falseを返すが、
 * 呼び元で 処理NGなのか セグメントデータ不足なのかは特定できない。
 * もし特定したい場合は本I/Fではなく com_stockTcpSeg()でデータを保持する。
 * この際 iTotalSizeには 0 を指定する。そして、この方法で保持をした後、
 * 返されたデータアドレスを com_reassembleTcpSeg()に掛け、全データが揃ったか
 * どうかのチェックと、揃ったときの結合を実施する(チェックや結合・データ格納は
 * 全て com_reassembleTcpSeg()で実施可能)
 *
 * データ保持でNGが発生時は com_stockTcpSeg()は NULLを返し、
 * データ結合でNGが発生時は com_reassembleTcpSeg()は COM_FRG_ERRORを返すので、
 * どこで処理NGが起きたかをある程度特定可能。
 */
BOOL com_continueTcpSeg( com_sigInf_t *ioTarget, com_sigInf_t *iTcp );

/*
 * TCPセグメンテーション データ結合  com_reassembleTcpSeg()
 *   処理結果を COM_SIG_FRG_t型の値で返す。COM_FRG_OK が帰ることはない。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNH] !iFrg || !ioTarget || !iTcp
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * iFrgはセグメンテーションデータを保持した情報のアドレス、
 * (com_stockTcpSeg()で返されたアドレスを想定)
 * ioTarget・iTcpは com_stockTcpSeg()と同様の内容を指定することで、
 * セグメンテーションデータが全て揃っているかどうかをチェックし、
 * 揃っていたらデータを結合して ioTarget->ras に格納し COM_FRG_REASMを返す。
 * (併せて ioTarget->sig のポイント先を ioTarget->ras に切り替える)
 *
 * 揃っていない場合は 何もせずに COM_SIG_FRGを返す。(後続データ待ち)
 *
 * セグメンテーションデータが揃っているかチェックするためには、
 * com_stockTcpSeg()で iTotalSizeを 1以上指定して予め呼んでいる必要がある。
 * この指定がない場合 セグメンテーションデータが揃っているかのチェックは
 * 行わず、無条件で COM_SIG_FRG を返す。
 *
 * なお既に iTotalSize指定済みの場合、com_continueTcpSeg()を使って
 * セグメンテーションデータを保持すると、併せて内部で本I/Fを呼んで、
 * セグメンテーションデータの終了チェックや結合も行うため、
 * 本I/Fを改めて呼ぶ必要はない。
 */
COM_SIG_FRG_t com_reassembleTcpSeg(
        com_sigFrg_t *iFrg, com_sigInf_t *ioTarget, com_sigInf_t *iTcp );



/*
 *****************************************************************************
 * トランスポート層 (UDP)
 *****************************************************************************
 */

// 参考情報：UDPヘッダ構造体 struct udphdr (netinet/udp.h)
//   古い構造体は uh_ がメンバー名に付かないなど、作りに違いがある。
//   Linuxでは2つの形式のどちらかを選ぶか、unionでどちらでもよくするか
//   gccの版数によって扱いがまちまちになっている。
#if 0  // UdpHeader
struct udphdr {
    u_int16_t uh_sport;     /* source port */
    u_int16_t uh_dport;     /* destination port */
    u_int16_t uh_ulen;      /* udp length */
    u_int16_t uh_sum;       /* udp checksum */
};
#endif // UdpHeader

/*
 * プロトコル個別I/F (UDP)
 *   com_analyzeUdp():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeUdp():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeUdp():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeUdp()で ioHead->sigの信号データを UDP信号として解析する。
 * 参照勧告は RFC768。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       UDPヘッダサイズ (sizeof(struct udphdr)
 *     .ptype     COM_SIG_UDP
 *   .ext         次プロトコルを判定するために使用したポート番号(long*型)
 *   .next        ペイロードデータ
 *     .cnt       1固定
 *     .stack[0]
 *       .sig     UDPヘッダ以降のデータ
 *                .ptypeは UDPヘッダのポート番号から取得する。
 *       .order   ioHead->orderの値
 *       .prev    ioHead自身
 * ioHead->sig.top を struct udphdr* でキャストしてヘッダ取得可能。
 *
 * 次プロトコルを特定するためのポート番号は 送信元->送信先 の順で調べる。
 * 最終的に使用したポート番号を .ext に long*型で格納する。
 * 次プロトコル値(ioHead->next.stack[0].sig.ptype)は COM_IPPORT を指定した
 * com_getPrtclType()によって特定される。
 * プロトコルが特定できなかったら 次プロトコル種別は COM_SIG_CONTINUE になる。
 *
 *
 * com_decodeUdp()で解析した iHeadの内容をデコード出力する。
 * ・UDPヘッダ内容のダンプ
 * ・送信元ポート番号
 * ・送信先ポート番号
 * ・次プロトコル
 */
BOOL com_analyzeUdp( COM_ANALYZER_PRM );
void com_decodeUdp( COM_DECODER_PRM );



/*
 *****************************************************************************
 * トランスポート層 (SCTP)
 *****************************************************************************
 */

// SCTP共通ヘッダ構造
typedef struct {
    uint16_t  srcPort;
    uint16_t  dstPort;
    uint32_t  verifTag;
    uint32_t  checksum;
} com_sigSctpCommonHdr_t;

// SCTP chunkヘッダ構造
typedef struct {
    uint8_t   type;      // COM_CAP_SCTP_TYPE_t型の値を期待
    uint8_t   flags;
    uint16_t  length;
} com_sigSctpChunkHdr_t;

// SCTP chunk種別
typedef enum {
    COM_CAP_SCTP_DATA     = 0x00,
    COM_CAP_SCTP_INIT     = 0x01,
    COM_CAP_SCTP_INITACK  = 0x02,
    COM_CAP_SCTP_SACK     = 0x03,
    COM_CAP_SCTP_HB       = 0x04,
    COM_CAP_SCTP_HBACK    = 0x05,
    COM_CAP_SCTP_ABORT    = 0x06,
    COM_CAP_SCTP_SD       = 0x07,    // SHUTDOWN
    COM_CAP_SCTP_SDACK    = 0x08,    // SHUTDOWN ACK
    COM_CAP_SCTP_ERROR    = 0x09,
    COM_CAP_SCTP_CKECHO   = 0x0a,    // COOKIE ECHO
    COM_CAP_SCTP_CKACK    = 0x0b,    // COOKIE ACK
    COM_CAP_SCTP_ECNE     = 0x0c,
    COM_CAP_SCTP_CWR      = 0x0d
} COM_CAP_SCTP_TYPE_t;

// DATAヘッダ構造
typedef struct {
    com_sigSctpChunkHdr_t  chunkHdr;
    uint32_t  tsn;
    uint16_t  streamId;
    uint16_t  streamSeqno;
    uint32_t  protocol;
} com_sigSctpDataHdr_t;

// INIT/INIT_ACKヘッダ構造
typedef struct {
    com_sigSctpChunkHdr_t  chunkHdr;
    uint32_t  initTag;
    uint32_t  a_rwnd;
    uint16_t  noOutboundStrm;
    uint16_t  noInboundStrm;
    uint32_t  initTsn;
} com_sigSctpInitHdr_t;

// SACKヘッダ構造
typedef struct {
    com_sigSctpChunkHdr_t  chunkHdr;
    uint32_t  cmlTsnAck;
    uint32_t  a_rwnd;
    uint16_t  noGapAckBlock;
    uint16_t  noDupTsns;
    // 以後に GapAckBlock と DupTsns が続く
} com_sigSctpSackHdr_t;

/*
 * プロトコル個別I/F (SCTP)
 *   com_analyzeSctp():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeSctp():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeSctp():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeSctp()で ioHead->sigの信号データを SCTP信号として解析する。
 * 参照勧告は RFC4960(section 3)。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       SCTP共通ヘッダサイズ (sizeof(com_sigSctpCommonHdr_t)
 *     .ptype     COM_SIG_SCTP
 *   .multi
 *     .cnt       メッセージ内の chunk数
 *     .statck[]
 *       .sig     各chunkの内容 (.ptypeは COM_SIG_SCTP)
 *       .order   ioHead->orderの値
 *   .ext         次プロトコルを判定するために使用したポート番号(long*型)
 *   .next        ペイロードデータ
 *     .cnt       実際のデータ数(DATA chunkの数)
 *     .stack[]
 *       .sig     DATA chunkの場合、ペイロードデータ
 *                .ptypeの取得方法は後述。
 *                DATA以外の chunkの場合 .topは NULL、.lenは 0が設定され、
 *                .ptypeは COM_SIG_ENDになる。
 *       .order   ioHead->orderの値
 *       .prev    ioHead自身
 * ioHead->sig.top を com_sigSctpCommonHdr_t* でキャストしてヘッダ取得可能。
 * ioHead->stack[].sig.topは com_sigSctpChunkHdr_t*でキャストして chunkヘッダ
 * 取得可能。DATA/INIT/SACK については個別にヘッダ内容の構造体あり。
 *
 * DATA chunkのヘッダ(com_sigSctpDataHdr_t)には .protocol があり、
 * これが次プロトコルを示していて COM_SCTPNEXT を指定した com_getPrtclType()に
 * よって次プロトコル値を取得できるが、これは 0 になっている可能性がある。
 * その場合はポート番号を 送信元->送信先の順で調べる。
 * 最終的に使用したポート番号を .ext に long*型で格納する。
 * この場合 次プロトコル値は COM_SCTPPORT を指定した com_getPrtclType()により
 * 特定される。
 * プロトコルが特定できなかったら 次プロトコル種別は COM_SIG_CONTINUE になる。
 * いずれにせよ ioHead->next.stack[].sig.ptype に設定される。
 *
 *
 * com_decodeSctp()で解析した iHeadの内容をデコード出力する。
 * ・SCTP共通ヘッダ内容をダンプ
 * ・送信元ポート番号
 * ・送信先ポート番号
 * ・verification Tag
 * ・chunk内容をダンプ(複数あれば chunk内容も複数になる)
 * ・次プロトコル
 */
BOOL com_analyzeSctp( COM_ANALYZER_PRM );
void com_decodeSctp( COM_DECODER_PRM );

/*
 * SCTPチャンク名取得  com_getSctpChunkName()
 *   チャンク名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSigTopでSCTP信号の chunkデータ先頭を指定することで、chunk名文字列を返す。
 * gSctpChunkName[]から文字列を検索する。
 */
char *com_getSctpChunkName( void *iSigTop );



/*
 *****************************************************************************
 * アプリケーション層 (SIP)
 *****************************************************************************
 */

// 次プロトコル定義 (com_setPrtclType(): COM_IPPORT用)
enum {
    COM_CAP_WPORT_SIP = 5060
};

// SIPリクエストメソッド
typedef enum {
    COM_SIG_SIP_UNKNOWN = 0,
    COM_SIG_SIP_REGISTER,
    COM_SIG_SIP_INVITE,
    COM_SIG_SIP_ACK,
    COM_SIG_SIP_CANCEL,
    COM_SIG_SIP_BYE,
    COM_SIG_SIP_OPTIONS,
    COM_SIG_SIP_INFO,         // RFC2976
    COM_SIG_SIP_PRACK,        // RFC3262
    COM_SIG_SIP_SUBSCRIBE,    // RFC3265
    COM_SIG_SIP_NOTIFY,       // RFC3265
    COM_SIG_SIP_UPDATE,       // RFC3311
    COM_SIG_SIP_MESSAGE,      // RFC3428
    COM_SIG_SIP_REFFER,       // RFC3515
    COM_SIG_SIP_PUBLISH,      // RFC3903
    COM_SIG_SIP_RESPONSE = COM_SIG_METHOD_END    // SIP/2.0
    // 100以降はそのままレスポンスのステータスコードとして使用
} COM_SIG_SIP_NAME_t;

// 必須ヘッダ名
#define  COM_CAP_SIPHDR_FROM          "From"
#define  COM_CAP_SIPHDR_TO            "To"
#define  COM_CAP_SIPHDR_CALLID        "Call-ID"
#define  COM_CAP_SIPHDR_CSEQ          "CSeq"
#define  COM_CAP_SIPHDR_MAXFWD        "Max-Forward"
#define  COM_CAP_SIPHDR_VIA           "Via"
// 主要ヘッダ名 (アルファベット順)
#define  COM_CAP_SIPHDR_CONTACT       "Contact"
#define  COM_CAP_SIPHDR_CLENGTH       "Content-Length"
#define  COM_CAP_SIPHDR_CTYPE         "Content-Type"
#define  COM_CAP_SIPHDR_DATE          "Date"
#define  COM_CAP_SIPHDR_EVENT         "Event"
#define  COM_CAP_SIPHDR_EXPIRE        "Expire"
#define  COM_CAP_SIPHDR_RACK          "RAck"
#define  COM_CAP_SIPHDR_RECROUTE      "Record-Route"
#define  COM_CAP_SIPHDR_ROUTE         "Route"
#define  COM_CAP_SIPHDR_RSEQ          "RSeq"

/*
 * プロトコル個別I/F (SIP)
 *   com_analyzeSip():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeSip():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeSip():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeSip()で ioHead->sigの信号データを SIP信号として解析する。
 * 参照勧告は RFC3261。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       先頭から、SIPヘッダとボディの間の改行までのサイズ
 *     .ptype     COM_SIG_SIP
 *   .prm         SIPヘッダ一覧
 *                マルチボディの場合 .prm.spec に boundary文字列(char*型)
 *                .list[]の個々の内容は以下のようになる：
 *                   .tagBin: ヘッダ名(行頭から : までの文字列)
 *                   .tag:    0固定
 *                   .lenBin: 0固定
 *                   .len:    ヘッダ名と : 後の最初の非空白文字～改行の長さ
 *                   .value:  ヘッダ名と : 後の最初の非空白文字の位置
 *   .next        ボディ部の内容
 *     .cnt       ボディ部の数(Content-Typeが multipart/mixedだと複数ある)
 *     .stack[]
 *       .sig     ボディ部の内容
 *                .ptypeは Content-Typeの内容から取得する。
 *       .order   ioHead->orderの値
 *       .prev    ioHead自身
 * SIPはテキストベースのプロトコルのため、ここまでに登場したプロトコルとは、
 * 処理内容が大きく異なる。
 *
 * ヘッダ一覧は .prm、ボディ内容は .nextに格納する。
 * ヘッダ名を見たい時は .list[].tagBin を見る必要があり、
 * ヘッダ値を見たい時は .valueを先頭に ,lenのサイズwお取得する。
 * com_getTxtHeaderVal()を使えば特定ヘッダの値を簡単に取得できる。
 *
 * ボディ部はテキスト/バイナリどちらの可能性もあるが、.next.stack[].sigの
 * 保持の仕方はどちらも変わらず、呼び元で扱い方を判断する必要がある。
 * 次プロトコル(ボディ部のプロトコル)は Content-Type から取得する。
 * COM_SIPNEXT を指定した com_getPrtclType() によって取得可能。
 *
 *
 * com_decodeSip()で解析した iHeadの内容をデコード出力する。
 * ・ヘッダ内容一覧
 * ・次プロトコル
 */
BOOL com_analyzeSip( COM_ANALYZER_PRM );
void com_decodeSip( COM_DECODER_PRM );

/*
 * SIP信号名取得  com_getSipName()
 *   取得結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   com_getMethodName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * 信号の先頭を iSigTopで指定することにより、
 * *oLabelにはメソッド名文字列(リクエストのときのみ)、
 * *oTypeみにはステータスコードの数値(レスポンスのときのみ)を格納する。
 * 格納が不要な場合 oLabelと oTypeは NULLを設定する。
 */
BOOL com_getSipName( void *iSigTop, const char **oLabel, long *oType );



/*
 *****************************************************************************
 * アプリケーション層 (SDP)
 *****************************************************************************
 */

// 次プロトコル値定義  (com_setPrtclType(): COM_SIPNEXT用)
#define  COM_SIG_CTYPE_SDP   "application/sdp"

/*
 * プロトコル個別I/F (SDP)
 *   com_analyzeSdp():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeSdp():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeSdp():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeSdp()で ioHead->sigの信号データを SDP記述として解析する。
 * 参照勧告は RFC4566。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .ptype     COM_SIG_SDP
 *   .prm         SDPディレクティブ一覧
 *                   .tagBin: ディレクティブ名(行頭から = までの文字列)
 *                   .tag:    0固定
 *                   .lenBin: 0固定
 *                   .len:    ディレクティブ名と = 後の最初の非空白文字～
 *                            改行の長さ
 *                   .value:  ディレクティブ名と = 後の最初の非空白文字の位置
 * SDP記述は1行ずつ解析して、ディレクティブとして取得し .prmに一覧を格納する。
 * データの格納方法は SIPヘッダと同様で、com_getTxtHeaderVal()で特定の
 * ディレクティブを指定して値を取得することも可能。
 * 
 * SDPは SIPのボディになることを想定している。
 * C-Planeと U-PlaneのIPアドレスとポート番号を、セッション情報として保持する。
 * (C-Planeは 下位のIPヘッダからアドレス、TCP/UDPヘッダからポート番号を取得、
 *  U-Planeは SDP記述の c に乗ったアドレスと、m に乗ったポート番号を取得)
 * SIPリクエストのボディだったら発側、レスポンスのボディだったら着側とみなす。
 * 複数回、SDP記述があった場合、その内容を上書きしていく。
 *
 * こうして生成したセッション情報は、その後解析するパケットが RTP/RTCP信号か
 * どうか判定するために使用する。
 *
 *
 * com_decodeSdp()で解析した iHeadの内容をデコード出力する。
 * ・SDPディレクティブ一覧
 */
BOOL com_analyzeSdp( COM_ANALYZER_PRM );
void com_decodeSdp( COM_DECODER_PRM );



/*
 *****************************************************************************
 * アプリケーション層 (RTP)
 *****************************************************************************
 */

// 次プロトコル定義 (com_setPrtclType(): COM_IPPORT用)
enum {
    COM_CAP_WPORT_RTP = 5004
};

// RTPヘッダ構造
typedef struct {
    uint8_t    rtpBits;    // V(2bit) P(1bit) X(1bit) CC(4bit)
    uint8_t    pt;         // M(1bit) PT(7bit)
    uint16_t   seq;
    uint32_t   ts;
    uint32_t   ssrc;
    uint32_t   csrc[];
} com_sigRtpHdr_t;

#define COM_CAP_RTP_VBIT   0xc0    // version (V) 2bit
#define COM_CAP_RTP_PBIT   0x20    // padding (P) 1bit
#define COM_CAP_RTP_XBIT   0x10    // extension (X) 1bit
#define COM_CAP_RTP_CCBIT  0x0f    // CSRC count (CC) 4bit

#define COM_CAP_RTP_MBIT   0x80    // market (M) 1bit
#define COM_CAP_RTP_PTBIT  0x7f    // payload type (PT) 7bit

enum {
    COM_CAP_RTP_VERSION = 0x80     // V (上位2ビットなので実際の値は 2)
};

// RTP拡張ヘッダ構造
typedef struct {
    uint16_t  extHdr;
    uint16_t  length;
} com_sigRtpExtHdr_t;

/*
 * プロトコル個別I/F (RTP)
 *   com_analyzeRtp():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeRtp():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeRtp():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeRtp()で ioHead->sigの信号データを RTP信号として解析する。
 * 参照勧告は RFC3550。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     ,len       RTPヘッダサイズ
 *                (sizeof(com_sigRtpHdr_t) + CCRCサイズ + 拡張ヘッダサイズ)
 *     .ptype     COM_SIG_RTP
 *   .multi
 *     .cnt       1固定
 *     .stack[0]  RTPペイロードデータ (.ptypeは COM_SIG_RTP)
 *     .order     ioHead->orderの値
 * ioHead->sig.top を com_sigRtpHdr_t* でキャストしてヘッダ取得可能。
 * ヘッダはサイズ算出のためだけに参照して詳細解析はしないので、
 * 必要なら呼び元で処理を検討すること。
 *
 * トランスポート層(主にUDP)のポート番号から RTPと判定することに対応し、
 * 5004 をそのポート番号として COM_IPPORT で com_getPrtclType()で取得可能。
 * またそれより前のパケット解析で、SIP/SDP の解析をしていた場合、
 * C-Plane/U-Planeのノード情報をまとめているので、U-Plane側の発信元/発信先
 * 情報と一致する場合は RTP信号と認識する。
 *
 *
 * com_decodeRtp()で解析した iHeadの内容をデコード出力する。
 * ・Payload Type名
 * ・シーケンス番号
 * ・RTPビット
 */
BOOL com_analyzeRtp( COM_ANALYZER_PRM );
void com_decodeRtp( COM_DECODER_PRM );



/*
 *****************************************************************************
 * アプリケーション層 (RTCP)
 *****************************************************************************
 */

// 次プロトコル定義 (com_setPrtclType(): COM_IPPORT用)
enum {
    COM_CAP_WPORT_RTCP = 5005
};

// RTCP共通ヘッダ構造
typedef struct {
    uint8_t    count;        // V(2bit) P(1bit) *C(5bit)
    uint8_t    packetType;
    uint16_t   length;
} com_sigRtcpHdrCommon_t;

#define COM_CAP_RTCP_VBIT   0xc0    // version (V) 2bit
#define COM_CAP_RTCP_PBIT   0x20    // padding (P) 1bit
#define COM_CAP_RTCP_CBIT   0x1f    // *** count (*C) 5bit

// RTCP packet type
typedef enum {
    COM_CAP_RTCP_PT_SR   = 200,    // Sender Report
    COM_CAP_RTCP_PT_RR   = 201,    // Reciever Report
    COM_CAP_RTCP_PT_SDES = 202,    // Source Description
    COM_CAP_RTCP_PT_BYE  = 203,    // Goodbye
    COM_CAP_RTCP_PT_APP  = 204,    // Application-Defined
} COM_CAP_RTCP_PT_TYPE_t;

// SRヘッダ構造
typedef struct {
    uint32_t  ssrc;       // SSRC of sender
    uint32_t  ntp_sec;    // NTP timestamp, most significant word
    uint32_t  ntp_frac;   // NTP timestamp, least significant word
    uint32_t  rtp_ts;     // RTP timestamp
    uint32_t  psent;      // sender's packet count
    uint32_t  osent;      // sender's octet count
} com_sigRtcpSr_t;

// RRヘッダ構造
typedef struct {
    uint32_t  ssrc;       // SSRC of packet sender
} com_sigRtcpRr_t;

// report block (SR/RR共通)
typedef struct {
    uint32_t  ssrc;       // SSRC
    uint8_t   fraction;   // fraction lost
    uint8_t   lost[3];    // cumulative number of packet lost
    uint32_t  last_seq;   // extended highest sequence number recieved
    uint32_t  jitter;     // interarrival jitter
    uint32_t  lsr;        // last SR (LSR)
    uint32_t  dlsr;       // delay since last SR (DLSR)
} com_sigRtcpRep_t;

// SDESヘッダ構造
typedef struct {
    uint32_t  src;
} com_sigRtcpSdes_t;

// SDESアイテム構造
typedef struct {
    uint8_t   type;
    uint8_t   length;
    uint8_t   data[];
} com_sigRtcpItem_t;

// SDES Item type
typedef enum {
    COM_CAP_RTCP_SDES_END   = 0,
    COM_CAP_RTCP_SDES_CNAME = 1,    // Canonical End-Point Identifier
    COM_CAP_RTCP_SDES_NAME  = 2,    // User Name
    COM_CAP_RTCP_SDES_EMAIL = 3,    // Electrival Mail Address
    COM_CAP_RTCP_SDES_PHONE = 4,    // Phone Number
    COM_CAP_RTCP_SDES_LOC   = 5,    // Geographic User Location
    COM_CAP_RTCP_SDES_TOOL  = 6,    // Application or Tool Name
    COM_CAP_RTCP_SDES_NOTE  = 7,    // Notice/Status
    COM_CAP_RTCP_SDES_PRIV  = 8,    // Private Extensions
} COM_CAP_RTCP_SDES_ITEM_t;

// BYEヘッダ構造  (共通ヘッダのみのため個別定義なし)

// APPヘッダ構造
typedef struct {
    com_sigRtcpHdrCommon_t  cmn;
    uint32_t  src;
    uint8_t   name[4];
} com_sigRtcpApp_t;

/*
 * プロトコル個別I/F (RTCP)
 *   com_analyzeRtcp():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeRtcp():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeRtcp():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeRtcp()で ioHead->sigの信号データを RTCP信号として解析する。
 * 参照勧告は RFC3550。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .ptype     COM_SIG_RTCP
 *   .multi
 *     .cnt       RTCPパケット個数
 *     .stack[]   各RTCPパケット内容
 *     .order     ioHead->orderの値
 * ioHead->sig.top を com_sigRtcpHdr_t* でキャストしてヘッダ取得可能。
 * ヘッダはサイズ算出のためだけに参照して詳細解析はしないので、
 * 必要なら呼び元で処理を検討すること。
 * 
 * 同様に、個々のRTCPパケットについても、データとして分割はするが、
 * それ以上の解析は実施しない。
 *
 * またそれより前のパケット解析で、SIP/SDP の解析をしていた場合、
 * C-Plane/U-Planeのノード情報をまとめているので、U-Plane側の発信元/発信先
 * 情報と一致する場合は RTCP信号と認識する。
 * この判定時は ポート番号は保持しているノード情報の番号+1 で判定する。
 *
 *
 * com_decodeRtcp()で解析した iHeadの内容をデコード出力する。
 * ・RTCPパケット一覧
 * ・パケット種別名
 */
BOOL com_analyzeRtcp( COM_ANALYZER_PRM );
void com_decodeRtcp( COM_DECODER_PRM );

/*
 * RTCPパケット種別名取得  com_getRtcpPtName()
 *   ペイロード種別名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSigTopでRTCPパケットの先頭を指定することで、パケット種別名文字列を返す。
 * gRtcpPtName[]から文字列を検索する。
 */
char *com_getRtcpPtName( void *iSigTop );



/*
 *****************************************************************************
 * アプリケーション層 (Diameter)
 *****************************************************************************
 */

// 次プロトコル値定義  (com_setPrtclType(): COM_SCTPNEXT用)
enum {
    COM_CAP_SCTP_PROTO_DIAMETER = 0x2e
};

// 次プロトコル値定義  (com_setPrtclType(): COM_IPPORT用)
enum {
    COM_CAP_WPORT_DIAMETER = 3868
};

// Diameterコマンドコード
//   Base: RFC3588
//   Cx/Dx: TS29.229   Sh/Dh: TS29.329
//   Tx: X.S0013-013   Ty: X.S0013-014
typedef enum {
    COM_CAP_DIAM_CE = 257,    // (Base)  Capabilities-Exchange
    COM_CAP_DIAM_DW = 280,    // (Base)  Device-Watchdog
    COM_CAP_DIAM_DP = 282,    // (Base)  Disconnect-Peer
    COM_CAP_DIAM_UA = 300,    // (Cx/Dx) User-Authorization
    COM_CAP_DIAM_SA = 301,    // (Cx/Dx) Server-Assignment
    COM_CAP_DIAM_LI = 302,    // (Cx/Dx) Location-Info
    COM_CAP_DIAM_MA = 303,    // (Cx/Dx) Multimedia-Auth
    COM_CAP_DIAM_RT = 304,    // (Cx/Dx) Registraion-Termination
    COM_CAP_DIAM_PP = 305,    // (Cx/Dx) Push-Profile
    COM_CAP_DIAM_UD = 306,    // (Sh/Dh) User-Data
    COM_CAP_DIAM_PU = 307,    // (Sh/Dh) Profile-Update
    COM_CAP_DIAM_SN = 308,    // (Sh/Dh) Subscribe-Notification
    COM_CAP_DIAM_PN = 309,    // (Sh/Dh) Push-Notification
    COM_CAP_DIAM_RA = 258,    // (Tx/Ty) Re-Auth
    COM_CAP_DIAM_AA = 265,    // (Tx)    AA
    COM_CAP_DIAM_AS = 274,    // (Tx)    Abort-Session
    COM_CAP_DIAM_ST = 275,    // (Tx)    Session-Termination
    COM_CAP_DIAM_CC = 272,    // (Ty)    Credit-Control
} COM_CAP_DIAM_COMMAND_t;

// Diameterヘッダ構造
typedef struct {
    uint8_t    version;
    uint8_t    msgLen[3];
    uint8_t    cmdFlags;
    uint8_t    cmdCode[3];
    uint32_t   AppliID;
    uint32_t   HopByHopID;
    uint32_t   EndToEndID;
} com_sigDiamHdr_t;

#define  COM_CAP_DIAMHDR_VER    0x01

#define  COM_CAP_DIAMHDR_RBIT   0x80   // Request
#define  COM_CAP_DIAMHDR_PBIT   0x40   // Proxiable
#define  COM_CAP_DIAMHDR_EBIT   0x20   // Error
#define  COM_CAP_DIAMHDR_TBIT   0x10   // Potentially re-transmitted message

// Diameteer AVPヘッダ構造
typedef struct {
    uint32_t  avpCode;
    uint8_t   flags;
    uint8_t   avpLen[3];
} com_sigDiamAvp_t;

#define  COM_CAP_DIAMAVP_VBIT   0x80   // Vendor-Specific
#define  COM_CAP_DIAMAVP_MBIT   0x40   // Mandatory
#define  COM_CAP_DIAMAVP_PBIT   0x20   // need for encryption

/*
 * プロトコル個別I/F (Diameter)
 *   com_analyzeDiameter():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeDiameter():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeDiameter():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeDiameter()で ioHead->sigの信号データをDiameter信号として解析する。
 * 参照勧告は RFC3588(Base)・TS29.229(Cx/Dx)・TS29.329(Sh/DH)
 * ・X.S0013-003(Tx)・X.S0013-014(Ty)。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       Diameter共通ヘッダサイズ (sizeof(com_sigDiamHdr_t)) 
 *     .ptype     COM_SIG_DIAMETER
 *   .prm         AVP一覧
 *     .cnt       AVP個数
 *     .stack[]   各AVP内容
 *                 .tagBin: 信号バイナリのAVP先頭位置
 *                 .tag:    AVPコード
 *                 .lenBin: 信号バイナリのレングス位置
 *                 .len:    AVP値のサイズ (値無しのAVPなら 0)
 *                 .value:  AVP値の先頭位置 (値なしの場合は NULL)
 * ioHead->sig.top を com_sigDiamHdr_t* でキャストしてヘッダ取得可能。
 *
 * AVP一覧では .stack[].value は NULLになり得ることに注意すること。
 * 特に Gropued AVP(＝従属するAVPが存在する)の時は、NULLになり、
 * Diameterの場合、これによってそれが Gropued AVPか判定可能。
 *
 * なお、Grouped AVP となる AVPコードは予め登録が必要になる。
 * COM_DIAMAVPG を指定した com_getPrtclType() で trueであれば、
 * その AVPコードが Grouped AVPである。と判定できる。
 * toscomで解析可能なインターフェースについては登録済みとなっている。
 *
 * AVPヘッダの内容を .prmで解析保持はしないが .stack[].tagBin で各AVPの先頭は
 * 分かるので、そこから AVPヘッダを別途解析することは可能。
 * 
 * 同様に、個々のRTCPパケットについても、データとして分割はするが、
 * それ以上の解析は実施しない。
 *
 * またそれより前のパケット解析で、SIP/SDP の解析をしていた場合、
 * C-Plane/U-Planeのノード情報をまとめているので、U-Plane側の発信元/発信先
 * 情報と一致する場合は RTCP信号と認識する。
 * この判定時は ポート番号は保持しているノード情報の番号+1 で判定する。
 *
 *
 * com_decodeDiameter()で解析した iHeadの内容をデコード出力する。
 * ・Diameter共通ヘッダ内容のダンプ
 * ・コマンド名
 * ・コマンドフラグ
 * ・Application ID
 * ・Hop-by-Hop ID
 * ・End-to-End ID
 * ・AVP一覧
 */
BOOL com_analyzeDiameter( COM_ANALYZER_PRM );
void com_decodeDiameter( COM_DECODER_PRM );

/*
 * Diameterコマンド名取得  com_getDiameterCmdName()
 *   コマンド名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSigTopで Diameter信号の先頭を指定することで、コマンド名文字列を返す。
 * gDiameterCmdName[]から文字列を検索する。
 *
 * コマンドフラグを参照して3文字目は変動する。
 *   Requestビットあり: R
 *   Errorビットあり：  E
 *   上記どちらも無し； A
 */
char *com_getDiameterCmdName( void *iSigTop );



/*
 *****************************************************************************
 * アプリケーション層 (DHCP)
 *****************************************************************************
 */

// 次プロトコル定義 (com_setPrtclType(): COM_IPPORT用)
enum {
    COM_CAP_WPORT_DHCPCL = 67,
    COM_CAP_WPORT_DHCPSV = 68
};

// DHCPヘッダ構造
typedef struct {
    uint8_t    op;             // Message op code / meesage type
    uint8_t    htype;          // Hardware address type
    uint8_t    hlen;           // Hardware address length
    uint8_t    hops;
    uint32_t   xid;            // Transaction ID
    uint16_t   secs;           // Seconds elapsed
    uint16_t   flags;          // Flags
    uint32_t   ciaddr;         // Client IP address
    uint32_t   yiaddr;         // Your IP address
    uint32_t   siaddr;         // Next server IP address
    uint32_t   giaddr;         // Relay agent IP address
    uint8_t    chaddr[16];     // Client hardware address
    uint8_t    sname[64];      // Server host name
    uint8_t    file[128];      // Boot file name
    uint32_t   magicCookie;    // DHCP magic cookie (0x63825363)
} com_sigDhcpHdr_t;

// DHCP op code
typedef enum {
    COM_CAP_DHCP_OP_BREQ = 1,    // BOOTREQUEST
    COM_CAP_DHCP_OP_BREP = 2     // BOORREPLY
} COM_CAP_DHCP_OP_CODE_t;

#define COM_CAP_DHCP_BROADCAST      0x1000      // flags
#define COM_CAP_DHCP_MAGIC_COOKIE   0x63825363  // optinosフィールドの先頭

// DHCP Message Type (option 53)
typedef enum {
    COM_CAP_DHCP_MSG_DISCOVER = 1,
    COM_CAP_DHCP_MSG_OFFER    = 2,
    COM_CAP_DHCP_MSG_REQUEST  = 3,
    COM_CAP_DHCP_MSG_DECLINE  = 4,
    COM_CAP_DHCP_MSG_ACK      = 5,
    COM_CAP_DHCP_MSG_NAK      = 6,
    COM_CAP_DHCP_MSG_RELEASE  = 7,
    COM_CAP_DHCP_MSG_INFORM   = 8
} COM_CAP_DHCP_MSG_TYPE_t;

// option code (RFC2132)
typedef enum {
    COM_CAP_DHCP_OPT_PAD     = 0,     // Pad Option
    COM_CAP_DHCP_OPT_MSGTYPE = 53,    // DHCP Message Type
    COM_CAP_DHCP_OPT_END     = 255    // End Option
} COM_CAP_DHCP_OPT_t;

/*
 * プロトコル個別I/F (DHCP)
 *   com_analyzeDhcp():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeDhcp():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeDhcp():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeDhcp()で ioHead->sigの信号データを DHCP信号として解析する。
 * 参照勧告は RFC2131。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       DHCPヘッダサイズ (sizeof(com_sigDhcpHdr_t))
 *     .ptype     COM_SIG_DHCP
 *   .prm         オプション一覧
 * ioHead->sig.top を com_sigDhcpHdr_t* でキャストしてヘッダ取得可能。
 * ヘッダはサイズ算出のためだけに参照して詳細解析はしないので、
 * 必要なら呼び元で処理を検討すること。
 *
 *
 * com_decodeDhcp()で解析した iHeadの内容をデコード出力する。
 * ・DHCPヘッダ内容のダンプ
 * ・DHCP信号名
 * ・DHCPオプション一覧
 */
BOOL com_analyzeDhcp( COM_ANALYZER_PRM );
void com_decodeDhcp( COM_DECODER_PRM );

/*
 * DHCP信号名取得  com_getDhcpSigName()
 *   信号名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iDhcpで DHCPで始まる信号情報を指定することで、その信号名文字列を返す。
 * gDhcpOpName[]から文字列を検索する。
 *
 * 基本的に DHCPヘッダに乗ったオペレータ(.op)を使用するが、
 * .prm(DHCPオプション)の中に DHCP Message Typeがあった場合は、それを優先する。
 * そのためには iDhcpに com_analyzeDhcp()で解析した結果を渡す必要がある。
 */
char *com_getDhcpSigName( com_sigInf_t *iDhcp );



/*
 *****************************************************************************
 * アプリケーション層 (DHCPv6)
 *****************************************************************************
 */

// 次プロトコル定義 (com_setPrtclType(): COM_IPPORT用)
enum {
    COM_CAP_WPORT_DHCP6CL = 546,
    COM_CAP_WPORT_DHCP6SV = 547
};

// DHCPv6ヘッダ構造
typedef struct {
    uint8_t   msgType;
    uint8_t   tranId[3];
} com_sigDhcpv6Hdr_t;

// DHCPv6ヘッダ構造(Relay用)
typedef struct {
    uint8_t   msgType;
    uint8_t   hopCount;
    uint8_t   linkAddr[12];
    uint8_t   peerAddr[12];
} com_sigDhcpv6Relay_t;

typedef enum {
    COM_CAP_DHCP6_MSG_SOLICIT     = 1,
    COM_CAP_DHCP6_MSG_ADVERTISE   = 2,
    COM_CAP_DHCP6_MSG_REQUEST     = 3,
    COM_CAP_DHCP6_MSG_CONFIRM     = 4,
    COM_CAP_DHCP6_MSG_RENEW       = 5,
    COM_CAP_DHCP6_MSG_REBIND      = 6,
    COM_CAP_DHCP6_MSG_REPLY       = 7,
    COM_CAP_DHCP6_MSG_RELEASE     = 8,
    COM_CAP_DHCP6_MSG_DECLINE     = 9,
    COM_CAP_DHCP6_MSG_RECONFIGURE = 10,
    COM_CAP_DHCP6_MSG_INFOREQUEST = 11,    // INFORMATION-REQUEST
    COM_CAP_DHCP6_MSG_RELAYFORM   = 12,    // RELAY-FORM
    COM_CAP_DHCP6_MSG_RELAYREPL   = 13     // RELAY-REPL
} COM_CAP_DHCP6_MSG_TYPE_t;

/*
 * プロトコル個別I/F (DHCPv6)
 *   com_analyzeDhcpv6():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeDhcpv6():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeDhcpv6():
 *     エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeDhcpv6()で ioHead->sigの信号データを DHCPv6信号として解析する。
 * 参照勧告は RFC8415。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       DHCPv6ヘッダサイズ (sizeof(com_sigDhcpv6Hdr_t))
 *     .ptype     COM_SIG_DHCPV6
 *   .prm         オプション一覧
 * ioHead->sig.top を com_sigDhcpv6Hdr_t* でキャストしてヘッダ取得可能。
 * ヘッダはサイズ算出のためだけに参照して詳細解析はしないので、
 * 必要なら呼び元で処理を検討すること。
 *
 *
 * com_decodeDhcpv6()で解析した iHeadの内容をデコード出力する。
 * ・DHCPv6ヘッダ内容のダンプ
 * ・DHCPv6信号名
 * ・DHCPv6オプション一覧
 */
BOOL com_analyzeDhcpv6( COM_ANALYZER_PRM );
void com_decodeDhcpv6( COM_DECODER_PRM );

/*
 * DHCPv6メッセージ種別名取得  com_getDhcpv6MsgType()
 *   メッセージ種別名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSigTopで DHCPv6で始まる信号情報を指定することで、その信号名文字列を返す。
 * gDhcpv6MsgName[]から文字列を検索する。
 */
char *com_getDhcpv6MsgType( void *iSigTop );



/*
 *****************************************************************************
 * アプリケーション層 (DNS)
 *****************************************************************************
 */

// 次プロトコル定義 (com_setPrtclType(): COM_IPPORT用)
enum {
    COM_CAP_WPORT_DNS = 53
};

// DNS header section データ構造
typedef struct {
    uint16_t   id;
    uint16_t   flags;
    uint16_t   qdcount;
    uint16_t   ancount;
    uint16_t   nscount;
    uint16_t   arcount;
} com_sigDnsHdr_t;

// question section データ構造
typedef struct {
    // ここに QNAME がまず入る
    uint16_t   qtype;
    uint16_t   qclass;
} com_sigDnsQd_t;

// resource record データ構造
typedef struct {
    // ここに NAME がまず入る
    uint16_t   rtype;
    uint16_t   rclass;
    uint32_t   rttl;
    // この後に RLENGTH・RDATA が続く
} com_sigDnsRr_t;

typedef enum {
    COM_CAP_DNS_QD,    // question section
    COM_CAP_DNS_AN,    // answer section
    COM_CAP_DNS_NS,    // name server resource records section
    COM_CAP_DNS_AR     // additional records section
} COM_CAP_DNS_RECSEC_t;

// DNSレコード情報データ構造
typedef struct {
    ulong         recSec;    // セクション種別 (COM_CAP_DNS_RECSEC_t型の値)
    com_sigBin_t  rname;     // QNAME or NAME
    ulong         rtype;     // QTYPE or TYPE
    ulong         rclass;    // QCLASS or CLASS
    ulong         rttl;      // ttl   (Resource Record用)
    com_sigBin_t  rdata;     // RDATA (Resource Record用)
} com_sigDnsRecord_t;

// DNS信号情報データ構造
typedef struct {
    ulong                qrbit;    // DNSヘッダの QRビット値
    ulong                opcode;   // DNSヘッダの opcode値
    ulong                rcode;    // DNSヘッダの rcpde値
    com_sigDnsRecord_t*  rd;       // resource record
    long                 rcnt;     // resource record数
} com_sigDnsData_t;

// DNS header section: flags要素
typedef enum {
    COM_CAP_DNSBIT_QR = 0,
    COM_CAP_DNSBIT_OPCODE,
    COM_CAP_DNSBIT_AA,
    COM_CAP_DNSBIT_TC,
    COM_CAP_DNSBIT_RD,
    COM_CAP_DNSBIT_RA,
    COM_CAP_DNSBIT_Z,
    COM_CAP_DNSBIT_RCODE
} COM_CAP_DNSBIT_t;

// flags値
enum {
    COM_CAP_DNS_QR_QUERY    = 0,
    COM_CAP_DNS_QR_RESPONSE = 1,

    COM_CAP_DNS_OP_QUERY    = 0,    // standard query
    COM_CAP_DNS_OP_IQUERY   = 1,    // inverse query
    COM_CAP_DNS_OP_STATUS   = 2,    // server status request

    COM_CAP_DNS_RCODE_NOERR   = 0,  // no error condition
    COM_CAP_DNS_RCODE_FORMAT  = 1,  // format error
    COM_CAP_DNS_RCODE_SRVFAIL = 2,  // server failure
    COM_CAP_DNS_RCODE_NAME    = 3,  // name error
    COM_CAP_DNS_RCODE_NOTIMPL = 4,  // not implemented
    COM_CAP_DNS_RCODE_REFUSED = 5,  // refused
};

// ドメイン名の圧縮ビット表示
enum {
    COM_CAP_DNS_COMPRESS_BIT = 0xc0
};

// QTYPE値
typedef enum {
    COM_CAP_DNS_QTYPE_AXFR  = 252,  // request for a transfer of an antire zone
    COM_CAP_DNS_QTYPE_MAILB = 253,  // request for mailbox-related records
    COM_CAP_DNS_QTYPE_MAILA = 254,  // request for mail agent RRs
    COM_CAP_DNS_QTYPE_ALL   = 255   // request for all records
} COM_CAP_DNS_QTYPE_t;

// CLASS/QCLASS値
typedef enum {
    COM_CAP_DNS_CLASS_IN  = 1,    // the Internet
    COM_CAP_DNS_CLASS_CS  = 2,    // the CSNET class
    COM_CAP_DNS_CLASS_CH  = 3,    // the CHAOS class
    COM_CAP_DNS_CLASS_HS  = 4,    // Hesiod
    COM_CAP_DNS_CLASS_ANY = 255   // any class
} COM_CAP_DNS_CLASS_t;

// RR TYPE値
typedef enum {
    COM_CAP_DNS_RTYPE_A     = 1,   // host address
    COM_CAP_DNS_RTYPE_NS    = 2,   // authoritative name server
    COM_CAP_DNS_RTYPE_MD    = 3,   // mail destination
    COM_CAP_DNS_RTYPE_MF    = 4,   // mail forwarder
    COM_CAP_DNS_RTYPE_CNAME = 5,   // canonical name for an alias
    COM_CAP_DNS_RTYPE_SOA   = 6,   // marks the start of a zone of authority
    COM_CAP_DNS_RTYPE_MB    = 7,   // mailbox domain name
    COM_CAP_DNS_RTYPE_MG    = 8,   // mail group member
    COM_CAP_DNS_RTYPE_MR    = 9,   // mail rename domain name
    COM_CAP_DNS_RTYPE_NULL  = 10,  // null RR
    COM_CAP_DNS_RTYPE_WKS   = 11,  // well known service description
    COM_CAP_DNS_RTYPE_PTR   = 12,  // domain name pointer
    COM_CAP_DNS_RTYPE_HINFO = 13,  // host information
    COM_CAP_DNS_RTYPE_MINFO = 14,  // mailbox or mail list information
    COM_CAP_DNS_RTYPE_MX    = 15,  // mail exchange
    COM_CAP_DNS_RTYPE_TXT   = 16,  // text strings
    COM_CAP_DNS_RTYPE_SRV   = 33   // location of service
} COM_CAP_DNS_RTYPE_t;

// RRデータ形式
typedef enum {
    COM_CAP_DNS_RR_END = 0,
    COM_CAP_DNS_RR_DNAME,        // <domain-name>
    COM_CAP_DNS_RR_CHSTR,        // <character-string>
    COM_CAP_DNS_RR_8BIT,         // 8 bit value
    COM_CAP_DNS_RR_16BIT,        // 16 bit value
    COM_CAP_DNS_RR_32BIT,        // 32 bit value
    COM_CAP_DNS_RR_V4ADDR,       // 32 bit address
    COM_CAP_DNS_RR_BITMAP,       // bit map
    COM_CAP_DNS_RR_ANY,          // any data
    COM_CAP_DNS_RR_TXT           // text data
} COM_CAP_DNS_RR_t;

/*
 * プロトコル個別I/F (DNS)
 *   com_analyzeDns():
 *     処理成否を true/false で返す
 * ---------------------------------------------------------------------------
 *   com_analyzeDns():
 *     COM_ANALYZER_STARTで発生するエラー
 *   com_decodeDns():
 *     エラーは発生しない。
 *   com_freeDns():
 *     COM_ERR_DEBUGNG: [com_prmNG] !oHead
 *     com_free()によるエラー。
 * ===========================================================================
 *   マルチスレッドで動くことは想定していない。
 * ===========================================================================
 * com_analyzeDns()で ioHead->sigの信号データを DNS信号として解析する。
 * 参照勧告は RFC1036。
 * 解析の結果、ioHeadで変動するのは以下となる：
 *   .sig
 *     .len       DNSヘッダサイズ (sizeof(com_sigDnsHdr_t))
 *     .ptype     COM_SIG_DNS
 *   .ext         リソースレコード一覧 (com_sigDnsData_t*型)
 *                (独自のデータ構造が必要なため .prm は使用しない)
 * ioHead->sig.top を com_sigDnsHdr_t* でキャストしてヘッダ取得可能。
 * ヘッダはサイズ算出のためだけに参照して詳細解析はしないので、
 * 必要なら呼び元で処理を検討すること。
 *
 * 信号内のリソースレコードは全て .ext に格納する。
 * このデータのメモリ解放には、専用の解放I/F(後述)を必要とする。
 *
 * .ext に格納したデータでは，ドメイン名に圧縮が使われている場合、
 * 圧縮オフセット位置だけの格納となる。そのため実際のドメイン名を取得するには
 * com_getDomain() を使う必要がある。このI/Fは無圧縮でも問題なく機能するので、
 * ドメイン名取得時には全て使う方針で問題ない。
 *
 *
 * com_decodeDhcpv6()で解析した iHeadの内容をデコード出力する。
 * ・DNSヘッダ内容のダンプ
 * ・オペレーション名
 * ・RCODE
 * ・各セクションのリソースレコード一覧
 *
 *
 * com_freeDns()は .ext に格納したリソースレコード一覧をメモリ解放する。
 * 単純な com_free()では不十分なため、このI/Fを用意している。
 */
BOOL com_analyzeDns( COM_ANALYZER_PRM );
void com_decodeDns( COM_DECODER_PRM );
void com_freeDns( com_sigInf_t *oHead );

/*
 * DNSフラグフィールド取得  com_getDnsFlagsField()
 c   指定されたフラグの値を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iHeadは DNS信号データを含む信号情報、iIdで取得したいデータを指定すると、
 * DNS信号ヘッダ内に含まれる QR・OPCODE・AA・TC・RD・RA・Z・RCODE といった
 * ビットフィールド上で定義されているパラメータの値を取得する。
 */
ulong com_getDnsFlagsField( com_sigInf_t *iHead, COM_CAP_DNSBIT_t iId );

/*
 * DNSドメイン名取得  com_getDomain()
 c   取得したドメイン名のサイズを返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iHeadは DNS信号データを含む信号情報、iTopはドメインを示すデータの先頭位置、
 * oBufと iBufSizeは取得したドメイン名を格納するバッファを指定する。
 * これにより、iTopで始まるデータからドメイン名を取得し、*oBufに格納する。
 * またそのドメイン名のサイズを返す。
 *
 * ドメイン名の最大長は 255 と決まっているため、
 * 用意すべきバッファのサイズとしては COM_LINEBUF_SIZE が適正となる。
 *
 * ドメイン名については 圧縮 の概念があり、それを解決してドメイン名を得るには
 * 信号データ全体が必要となる。そのため iHead->sig を用いる。
 */
com_off com_getDomain(
        com_sigInf_t *iHead, void *iTop, char *oBuf, size_t iBufSize );

/*
 * DNSオペレーション名取得  com_getDnsOpName()
 *   オペレーション名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iOpcodeで DNSオペレーションコードを指定すると、それに対応するオペレーション
 * 名文字列を返す。
 * gDnsOpName[]から文字列を検索する。
 */
char *com_getDnsOpName( ulong iOpcode );

/*
 * DNS RCODE説明取得  com_getDnsRcodeName()
 *   Rcode名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iRcodeで DNSの RCODEを指定すると、それに対応する RCODE説明文字列を返す。
 * gDnsRcode[]から文字列を検索する。
 */
char *com_getDnsRcodeName( ulong iRcode );

/*
 * DNS クラス名取得  com_getDnsClassName()
 *   クラス名文字列を返す。
 *   対応する文字列が見つからない場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   com_searchDecodeName()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iClassで DNSの CLASSを指定すると、それに対応する CLASS名文字列を返す。
 * gDnsClass[]から文字列を検索する。
 */
char *com_getDnsClassName( ulong iClass );

