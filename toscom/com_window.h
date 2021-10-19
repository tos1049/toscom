/*
 *****************************************************************************
 *
 * 共通部ウィンドウ機能    by TOS
 *
 *   ウィンドウ機能が必要な場合のみ、このヘッダファイルをインクルードし、
 *   com_window.hをコンパイルするソースに含めること。
 *
 *   このファイルより先に com_if.h をインクルードすること。
 *
 *   現在のウィンドウ機能
 *   ・COMWINBASE: ウィンドウ機能基本I/F
 *   ・COMWINCONT: ウィンドウ制御I/F
 *   ・COMWINDISP: ウィンドウ出力I/F
 *   ・COMWINKEY:  ウィンドウ入力I/F
 *   ・COMWINDBG:  ウィンドウ機能デバッグ関連I/F
 *
 *   cursesの昨日を全面的に利用するため、
 *     ncurses
 *     ncurses-devel
 *     ncurses-lib
 *   がインストールされている必要がある(そうしなければヘッダファイルも無い)。
 *   例えば「rpm -qa | grep ncurses」でインストール状況は確認出来るだろう。
 *
 *   本ヘッダは試験運用レベルのものとなる。まだ色々試行の余地があり、
 *   その結果分かったことは可能な限り本ヘッダファイルに記述した。
 *
 *****************************************************************************
 */

#pragma once

#ifndef USE_FUNCTRACE
#define _XOPEN_SOURCE_EXTENDED
#endif

#include <panel.h>
#include <ncurses.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>

/*
 *****************************************************************************
 * COMWINBASE:ウィンドウ機能基本I/F
 *****************************************************************************
 */

// ウィンドウ機能エラーコード宣言  ＊連動テーブルは gErrorNameWindow[]
enum {
    COM_ERR_WINDOWNG     = 950,    // ウィンドウ関連NG
};

void com_initializeWindow( void );




// ウィンドウ機能 導入フラグ
#define USING_COM_WINDOW
// ウィンドウ機能 初期化関数マクロ  ＊COM_INITIALIZE()で使用
#ifdef  COM_INIT_WINDOW
#undef  COM_INIT_WINDOW
#endif
#define COM_INIT_WINDOW  com_initializeWindow()

