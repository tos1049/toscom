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

#define NCURSES_INTERNALS

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



/*
 *****************************************************************************
 * COMWINCONT:ウィンドウ制御I/F
 *****************************************************************************
 */

// ウィンドウID型
typedef long  com_winId_t;

// ウィンドウID用 定数
#define  COM_ACTWIN   LONG_MIN    // アクティブウィンドウ指定値
#define  COM_NO_WIN   -1          // ウィンドウID 処理NG

//テキストID型
typedef long  com_winTextId_t;

// ウィンドウ情報構造体
typedef struct {
    com_winId_t      id;           // ウィンドウID
    BOOL             border;       // ウィンドウ枠線有無
    BOOL             hide;         // 非表示設定
    WINDOW*          window;       // ウィンドウ情報
    PANEL*           panel;        // パネル情報
    com_hashId_t     keyHash;      // キーマップ用ハッシュテーブルID
    com_hashId_t     mixKeyHash;   // 他ウィンドウのキーマップを使う場合のID
    com_winTextId_t  textId;       // com_inputText()の入力データID
    com_hashId_t     intrHash;     // com_inputText()割込用ハッシュテーブルID
} com_cwin_t;

// カーソル位置構造体
typedef struct {
    int  x;
    int  y;
} com_winpos_t;



// 色指定データ型
typedef struct {
    short pair;   // COLOR_PAIR(n)の nで指定するペア番号
    short fore;   // 文字色
    short back;   // 背景色
} com_colorPair_t;

// 各種フラグ型
typedef struct {
    BOOL noEnter;   // 文字入力をエンター押下不要にするかどうか
    BOOL noEcho;    // 入力文字を画面出力するかどうか
    BOOL keypad;    // キーパッドを有効にするかどうか
    com_colorPair_t *colors;
} com_winopt_t;

BOOL com_readyWindow( com_winopt_t *iOpt, com_winpos_t *oSize );



com_winId_t com_createWindow(
        const com_winpos_t *iPos, const com_winpos_t *iSize, BOOL iBorder );



BOOL com_existWindow( com_winId_t iId );



BOOL com_setBackgroundWindow( com_winId_t iId, chtype iCh );



BOOL com_activateWindow( com_winId_t iId );



BOOL com_refreshWindow( void );



BOOL com_moveWindow( com_winId_t iId, const com_winpos_t *iPos );



BOOL com_floatWindow( com_winId_t iId );



BOOL com_sinkWindow( com_winId_t iId );



BOOL com_hideWindow( com_winId_t iId, BOOL iHide );



int com_getWindowInf( com_winId_t iId, const com_cwin_t **oInf );



BOOL com_getWindowPos( com_winId_t iId, com_winpos_t *oPos );
BOOL com_getWindowMax( com_winId_t iId, com_winpos_t *oMax );
BOOL com_getWindowCur( com_winId_t iId, com_winpos_t *oCur );



BOOL com_deleteWindow( com_winId_t iId );



BOOL com_finishWindow( void );



/*
 *****************************************************************************
 * COMWINDISP:ウィンドウ出力I/F
 *****************************************************************************
 */

BOOL com_fillWindow( com_winId_t iId, int iChr );



BOOL com_clearWindow( com_winId_t iId );



BOOL com_moveCursor( com_winId_t iId, const com_winpos_t *iPos );



BOOL com_shiftCursor(
        com_winId_t iId, const com_winpos_t *iShift, BOOL *oBlocked );

BOOL com_udlrCursor( com_winId_t iId, int iDir, int iDist, BOOL *oBlocked );

enum {
    COM_CUR_UP    = 1,    // 上に移動
    COM_CUR_DOWN  = 2,    // 下に移動
    COM_CUR_LEFT  = 4,    // 左に移動
    COM_CUR_RIGHT = 8     // 右に移動
};



/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_printWindow(
 *           com_winId_t iId, int iAttr, BOOL iWrap, const char *iFormat, ... );
 */
#define com_printWindow( ID, ATTR, WRAP, ... ) \
    com_mprintWindow( (ID), NULL, (ATTR), (WRAP), __VA_ARGS__ )

BOOL com_mprintWindow(
        com_winId_t iId, const com_winpos_t *iPos, int iAttr, BOOL iWrap,
        const char *iFormat, ... );

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_printw( com_winId_t iId, int iAttr, const char *iFormat, ... );
 */
#define com_printw( ID, ATTR, ... ) \
    com_mprintw( (ID), NULL, (ATTR), __VA_ARGS__ )

BOOL com_mprintw(
        com_winId_t iId, const com_winpos_t *iPos, int iAttr,
        const char *iFormat, ... );



/*
 *****************************************************************************
 * COMWINKEY: ウィンドウ入力I/F
 *****************************************************************************
 */

// キーを押した時のコールバック関数プロトタイプ宣言
typedef void(*com_keyCB_t)( com_winId_t iId );

// キーマップ登録用構造体
typedef struct {
    long  key;
    com_keyCB_t  func;
} com_keymap_t;

BOOL com_setWindowKeymap( com_winId_t iId, const com_keymap_t *iKeymap );



BOOL com_mixWindowKeymap( com_winId_t iId, com_winId_t iSourceId );

// 他ウィンドウのキーマップを使用しない指定(デフォルト)
#define COM_ONLY_OWN_KEYMAP  (LONG_MIN + 1)



BOOL com_checkWindowKey( com_winId_t iId, int iDelay );



int com_getLastKey( void );



// 入力タイプ
typedef enum {
    COM_IN_1KEY,       // キーを押した時点で、その文字を返す
    COM_IN_1LINE,      // エンターを押した時点で、その1行の文字列を返す
    COM_IN_MULTILINE   // Ctrl+Dを押した時点で、複数行の文字列を返す
} COM_IN_TYPE_t;

typedef struct {
    COM_IN_TYPE_t type;   // 入力タイプ
    int     delay;        // 受付待ち時間(負数でブロックモード)
    size_t  size;         // 入力最大サイズ(バッファサイズではない)
    BOOL    echo;         // 入力文字を画面出力するかどうか
    BOOL    clear;        // 入力完了時に画面出力クリアするかどうか
} com_wininopt_t;

BOOL com_inputWindow(
        com_winId_t iId, char **oInput, com_wininopt_t *iOpt,
        const com_winpos_t *iPos );

enum {
    COM_WINTEXT_BUF_SIZE = COM_DATABUF_SIZE,
    COM_WINTEXT_BUF_COUNT = 5
};



size_t com_getRestSize( com_winId_t iId );



// キー割込コールバック関数プロトタイプ
typedef BOOL(*com_intrKeyCB_t)( com_winId_t iId );

// キー割込登録用構造体
typedef struct {
    long  key;
    com_intrKeyCB_t func;
} com_intrKey_t;

BOOL com_setIntrInputWindow( com_winId_t iId, const com_intrKey_t *iIntrKey );



char *com_getInputWindow( com_winId_t iId );



void com_setInputWindow( com_winId_t iId, const char *iText );



void com_cancelInputWindow( com_winId_t iId );



/*
 *****************************************************************************
 * COMWINDBG: ウィンドウ機能デバッグ関連I/F
 *****************************************************************************
 */

void com_setDebugWindow( BOOL iMode );


// ウィンドウ機能 導入フラグ
#define USING_COM_WINDOW
// ウィンドウ機能 初期化関数マクロ  ＊COM_INITIALIZE()で使用
#ifdef  COM_INIT_WINDOW
#undef  COM_INIT_WINDOW
#endif
#define COM_INIT_WINDOW  com_initializeWindow()

