/*
 *****************************************************************************
 *
 * 共通部シグナル機能 (プロトコルセット3)    by TOS
 *
 *   シグナル機能の、プロトコル別I/Fのセット3(IP系プロトコル追加)となる。
 *   本ヘッダの前に com_if.h と com_signal.h のインクルードが必要になる。
 *   (つまりこれらのヘッダによって使用される全ファイルも必要となる)
 *
 *   対応しているプロトコルについては PROTOCOL_AVAILABLE を参照。
 *
 *****************************************************************************
 */

#pragma once

// PROTOCOL_AVAILABLE
enum {
    /*** ファイルタイプ (1-99) ***/
    /*** link層 (101-199) ***/
    COM_SIG_VXLAN    = 111,    // port 4789     RFC7348
    /*** Network層 (201-299) ***/
    COM_SIG_IGMP     = 211,    // IPPROTO_IGMP (2)  RFC1112/2237/3376
    /*** Transport層 (301-399) ***/
    /*** No.7系 (401-499) ***/
    /*** Application層 (501-599) ***/
    COM_SIG_HTTP     = 511,    // port 80,...   RFC2616
    COM_SIG_SSDP     = 512,    // port 1900     UPnP device architecture 2.0
    COM_SIG_SNMP     = 513,    // port 161      RFC1900-1908 -> 3411-3418
    COM_SIG_NTP      = 514,    // port 123      RFC5905
    /*** プロトコル認識のみ (601-699) ***/
    COM_SIG_ESP      = 611,    // IPPROTO_ESP  (50) IPsec(ESP)
    COM_SIG_AH       = 612,    // IPPROTO_AH   (51) IPsec(AH)
    COM_SIG_SSH      = 613,    // port 22       SSH
    COM_SIG_HSRP     = 614,    // port 1985     Cisco HSRP
    COM_SIG_NBNS     = 615,    // port 137      NetBIOS Name Service
};

// まだ処理は未実装のためダミー処理
void com_initializeSigSet3( void );

