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
//   <net/ethernet.h>の struct ether_header で構造体せんげされている。
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

BOOL com_analyzeEth2( COM_ANALYZER_PRM );
void com_decodeEth2( COM_DECODER_PRM );
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

BOOL com_analyzeIpv4( COM_ANALYZER_PRM );
void com_decodeIpv4( COM_DECODER_PRM );
char *com_getIpv4OptName( long iType );

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

BOOL com_analyzeIpv6( COM_ANALYZER_PRM );
void com_decodeIpv6( COM_DECODER_PRM );
char* com_getIpv6ExtHdrName( long iCode );

BOOL com_checkIpAddrPort(
        void *iData, ssize_t iDataSize,
        struct sockaddr_storage *iList, long iListCnt );



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

BOOL com_analyzeIcmp( COM_ANALYZER_PRM );
void com_decodeIcmp( COM_DECODER_PRM );
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

BOOL com_analyzeIcmpv6( COM_ANALYZER_PRM );
void com_decodeIcmpv6( COM_DECODER_PRM );
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

BOOL com_analyzeArp( COM_ANALYZER_PRM );
void com_decodeArp( COM_DECODER_PRM );
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

BOOL com_analyzeTcp( COM_ANALYZER_PRM );
void com_decodeTcp( COM_DECODER_PRM );
char *com_getTcpOptName( long iType );

/*
 *
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 *
 */
com_sigFrg_t *com_stockTcpSeg(
        com_sigInf_t *ioTarget, com_sigInf_t *iTcp, com_off iTotalSize );

/*
 *
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 *
 */
BOOL com_continueTcpSeg( com_sigInf_t *ioTarget, com_sigInf_t *iTcp );

/*
 *
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 *
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

BOOL com_analyzeSctp( COM_ANALYZER_PRM );
void com_decodeSctp( COM_DECODER_PRM );
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

BOOL com_analyzeSip( COM_ANALYZER_PRM );
void com_decodeSip( COM_DECODER_PRM );
BOOL com_getSipName( void *iSigTop, const char **oLabel, long *oType );



/*
 *****************************************************************************
 * アプリケーション層 (SDP)
 *****************************************************************************
 */

// 次プロトコル値定義  (com_setPrtclType(): COM_SIPNEXT用)
#define  COM_SIG_CTYPE_SDP   "application/sdp"

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

BOOL com_analyzeRtcp( COM_ANALYZER_PRM );
void com_decodeRtcp( COM_DECODER_PRM );
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
//   Tx: X.S0013-013   Ty: X.S1003-014
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

BOOL com_analyzeDiameter( COM_ANALYZER_PRM );
void com_decodeDiameter( COM_DECODER_PRM );
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

BOOL com_analyzeDhcp( COM_ANALYZER_PRM );
void com_decodeDhcp( COM_DECODER_PRM );
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

BOOL com_analyzeDhcpv6( COM_ANALYZER_PRM );
void com_decodeDhcpv6( COM_DECODER_PRM );
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

BOOL com_analyzeDns( COM_ANALYZER_PRM );
void com_decodeDns( COM_DECODER_PRM );
ulong com_getDnsFlagsField( com_sigInf_t *iHead, COM_CAP_DNSBIT_t iId );
com_off com_getDomain(
        com_sigInf_t *iHead, void *iTop, char *oBuf, size_t iBufSize );
char *com_getDnsOpName( ulong iOpcode );
char *com_getDnsRcodeName( ulong iRcode );
char *com_getDnsClassName( ulong iClass );
void com_freeDns( com_sigInf_t *oHead );

