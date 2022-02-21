/*
 *****************************************************************************
 *
 * 共通部シグナル機能 (プロトコルセット2)    by TOS
 *
 *   シグナル機能の、プロトコル別I/Fのセット2(No.7関連)となる。
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
    /*** Network層 (201-299) ***/
    /*** Transport層 (301-399) ***/
    /*** No.7系 (401-499) ***/
    COM_SIG_M3UA     = 410,    // RFC3332 -> 4666
    COM_SIG_SCCP     = 411,    // Q713
    COM_SIG_IUSP     = 412,    // Q763
    COM_SIG_TCAP     = 413,    // Q773
    COM_SIG_INAP     = 414,    // Q1200-1699のどこか (未確認)
    COM_SIG_GSMMAP   = 414,
    /*** Application層 (501-599) ***/
    /*** プロトコル認識のみ (601-699) ***/
};

// まだ処理は未実装のためダミー処理
void com_initializeSigSet2( void );

