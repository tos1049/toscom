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
 *   加えてコンパイル時に「-lpanel -lncursesw」のライブラリ指定も必要になる。
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

/*
 * ウィンドウ機能初期化  com_initializeWindow()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで使用することは想定していない。
 * ===========================================================================
 * com_initialize()の後に、これも実行すること。
 * COM_INITIALIZE()マクロを使用する場合、直接 本I/Fの記述は必要なく、マクロを
 * 記述しているソース冒頭で、本ヘッダファイルをインクルードするだけで良い。
 */
void com_initializeWindow( void );



// ---------------------------------------------------------------------------
// ウィンドウ機能概要
// ---------------------------------------------------------------------------
// cursesを利用して、表示位置を指定した文字出力や、
// エンターキーを必要としない文字入力を可能とする特別なモードを実現する。
// toscomではこのモードを「ウィンドウモード」と称する。
// 現状は「ウィンドウ」だけに対応しており「パッド」には対応していない。
//
// com_readyWindow() を一番最初に必ず呼ぶ。
// これが init_scr()などウィンドウ機能に必要な設定を行うものとなっている。
//
// これにより ID=0の基本となるウィンドウが生成される。
// cursesライブラリでは「stdscr」とか「スクリーン」と呼ばれるもので、
// サイズは呼んだ時点の端末ウィンドウ全体を使ったものとなっている。
// このウィンドウは階層的に一番下に固定され、階層を動かすことは出来ないし、
// 削除もできない。ウィンドウモードを終了することで、このウィンドウは消える。
//
// 現時点でこの最初のウィンドウのサイズを変更することには対応していない。
//
// 以後、com_createWindow()で この上にウィンドウを追加できる。
// これは「cursesの stdscr上に生成するウィンドウ」ということになる。
// ウィンドウの重なりや移動の表示に Panelライブラリの機能を使用している。
// ウィンドウは IDで管理され、追加時に通知される。以後の操作に必ず必要となる。
//
// 追加したウィンドウを画面に出すには com_refreshWindow() を使用する。
// この再描画I/Fはウィンドウになにか変化があるたびに使用する。
//
// ウィンドウのうち、どれかひとつが「アクティブウィンドウ」になる。
// 文字入力や操作対象を示すカーソルはアクティブウィンドウにのみ現れる。
//
// 最初は ID=0のウィンドウ(=stdscr)がアクティブになる。
// com_activateWindow()で任意のウィンドウをアクティブに出来る。
// ウィンドウIDが必要なI/Fにおいて、ID値の代わりに COM_ACTWIN を指定すると
// その時のアクティブウィンドウに対する操作ができるようになっている。
//
// com_moveWindow()・com_floatWindow()・com_sinkWindow()で移動ができる。
// com_hideWindow()で表示/非表示を切り替えられる。
//
// com_printWindow() か com_printw()でウィンドウに対して文字出力する。
// この２つの処理差分は以下になる。
// ・com_printWindow()
//    カーソル位置に文字列を出力後、カーソル位置は移動しない。
//    文字出力後 自動で com_refreshWindow()を呼ぶ。
//    画面右端を超えた時の処理が独自のものになる。
// ・com_printw()
//    カーソル位置に文字列を出力後、カーソル位置を出力した文字の直後に移動。
//    com_refreshWindow()は実施しないので、呼び元で別途必要。
//    画面右端を超えた時は、無常家で折り返し出力となる。
// 
// com_mprintWindow() と com_mprintw() は表示位置も指定できる。
// 文字出力後のカーソル位置は、com_mprintWindow()は変動せず、com_mprintw()は
// 出力した文字の直後に移動する。カーソル位置と表示位置の違いに注意すること。
//
// com_setWindowKeymap()で、ウィンドウごとに「キーマップ」を指定可能。
// これは特定のキーを押した時に関数コールバックを実施する仕掛け。
// この設定後 com_checkKeymap()でキー入力を待つことが出来る。
//
// com_inputWindow()で、ウィンドウに対して文字列入力も可能。
// これは Enterを押して文字入力完了となるタイプのもの。
//
// com_deleteWindow()で使い終わったウィンドウを削除し、リソースを解放する。
//
// com_finishWindow()でウィンドウモード自体を終了する。
// この時残っていたウィンドウは全て自動で com_deleteWindow()で削除する。
// プログラム終了時もこのI/Fは呼ばれる。



// ---------------------------------------------------------------------------
// WINDOW*型について
// ---------------------------------------------------------------------------
// cursesがウィンドウを管理するための型が WINDOW型となっている。
// toscomでウィンドウ操作をする場合、この型は隠蔽されていて、ウィンドウIDで
// 簡単に処理できるように見せている。が機能的に限定があるのは間違いない。
//
// toscomのI/Fを使わずに、cursesのライブラリを直接使用する余地も残すため、
// com_getWindowInf()でウィンドウIDに対応する WINDOW*型ポインタも取得可能。
//
// WINDOW*型を使ってライブラリを使用する例として「スクロール」を挙げる。
// 何も指定しない場合(＝toscomでそのままウィンドウを使用する場合)、
// ウィンドウ内の文字列はスクロールしない。
//
// これに関しては以下のライブラリ関数で設定が出来る。
//     scrollok()    ウィンドウのスクロール可否を設定
//     idlok()       ウィンドウのハードウェアスクロール使用を設定
//     wsetscrreg()  ウィンドウのスクロール範囲を設定
// こうした関数を使用したい場合、WINDOW*型が必要になる。toscomでこれらを
// 設定するI/Fは敢えて用意していないので、必要があれば使用を検討する。
//
// 単純に scrollok()で trueに指定すると cursesのソフトウェアスクロールになる。
// 例えば teratermを使っていて、スクロールバーで戻した時にバックログが欲しい
// と思ったら、これでは駄目でハードウェアスクロールが必要になる。その時は
// idlok()で trueを指定する、という寸法になる。
//
// man curs_outopts と打つと、関連するライブラリ関数の情報を確認可能。
// サンプルの「コミュニケーター」はスクロール設定もしているので参考に。



// ---------------------------------------------------------------------------
// curses使用時に参考にできる情報ソース (toscomでも参考にした)
// ---------------------------------------------------------------------------
//   http://www.on-sky.net/~hs/misc/?NCURSES+Programming+HOWTO#l0
//   http://www.fireproject.jp/feature/c-language/curses/basic.html
//   http://www.kis-lab.com/serikashiki/man/ncurses.html
//   http://www.kushiro-ct.ac.jp/yanagawa/ex-2017/2-game/01.html
//
//   *** 上記は 2021年10月時点の存在を確認済み ***



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
// cursesのライブラリ関数は y,x の順で引数とするのが基本となっているが
// 直感的に分かりづらかったため、toscomでは x,y の順で宣言した。


/*
 * ウィンドウモード開始  com_readyWindow()
 *   処理成否を true/false で返す。
 *   既に com_readyWindow()実施済みの場合、何もせずに falseを返す。
 *   ウィンドウモードの終了には必ず com_finishWindow()を使用して、
 *   ウィンドウ用のリソース解放を実施すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_WINDOWNG: initscr()失敗。色指定不可能な端末色指定。
 *   ウィンドウ追加のための com_realloc()や com_registerHash()によるエラー。
 *   なお com_registerHash()はエラー発生時プログラムを強制終了する仕様。
 * ===========================================================================
 *   マルチスレッドで使用することは想定していない。
 * ===========================================================================
 * ウィンドウモードを開始する。
 *
 * iOptにはオプション設定を格納した構造体のアドレスを指定する。
 *  .noEnterを trueにすると、文字入力に Enter押下不要となる。
 *  .noEchoを trueにすると、文字入力後にその文字を画面に出力しない。
 *  .keypadを trueにすると、キーパッドが有効になる。
 *    cursesを使用する場合、上記3つは全て trueにするのが基本とされている。
 *    ただ .keypadを trueにすると teratermの画面上でマウススクロールが効かない
 *    現象が発生した。画面のバックスクロールも使いたいと思う場合 falseが良い。
 *    ただその場合キー入力で KEY_ で始まるマクロは使用できない。
 *  .colorsは色設定。色設定が不要なら NULLを指定する。
 *    最終データの .pairを 0にした線形(配列)データを指定する。
 *    ここで色設定しておくと、以後 ライブラリのマクロ COLOR_PAIR(n) で
 *    色指定が可能となるように init_pair()によるデータ登録が行われる。
 *    com_readyWindow()実行後、自力で init_pair()による登録をしても問題ない。
 *    COLOR_PAIR(n)による色指定が可能なのは、com_setBackgroundWindow()の iCh、
 *    com_mprintw()や com_mprintWindow()の iAttrが挙げられる。
 *
 * oSizeを指定すると、strscrのサイズ(その時点の端末ウィンドウサイズ)を格納する。
 * 格納が不要なら NULL指定する。
 *
 * 本I/Fにより strscrがウィンドウID=0で開始する。これは固定ウィンドウとなる。
 * 以後 com_createWindow()により、stdscr上にウィンドウが生成され、1以上の
 * ウィンドウIDが通知される。ウィンドウIDはウィンドウ操作に必要なものだが、
 * I/Fによっては 0を指定するとエラーになるものも存在する。
 *
 * ロケールも設定するため、ワイド文字が使用可能になる。
 * com_inputWindow()ではワイド文字も使って、日本語文字入力が矛盾なく出来る
 * ことを目指している。
 *
 * 本I/Fを使用すると、一時的に com_printf()や com_debug()などの標準出力は
 * 完全に抑制され、ウィンドウ機能で提供する以下のI/F以外で文字出力出来ない。
 *   com_printw()/com_mprintw()/com_printWindow()/com_mprintWindow()
 * その場合でもデバッグログへの出力はされているので、ログ出力目的で、
 * com_printf()等を使用することは出来る。
 * この画面抑制はデバッグログ上で「>> window mode start <<」と出力されてから
 * com_finishWindow()実施で「>> window mode end <<」と出るまで継続される。
 */

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

// com_colorPair_tの .fore や .back に指定できる色のマクロとして、
// cursesでは以下が用意されている(その右に実際の数値も示す)。
//     COLOR_BLACK      0
//     COLOR_RED        1
//     COLOR_GREEN      2
//     COLOR_YELLOW     3
//     COLOR_BLUE       4
//     COLOR_MAGENTA    5
//     COLOR_CYAN       6
//     COLOR_WHITE      7
// cursesライブラリ関数 init_color()で独自色を作成することも可能。
// RGBを0-1000の範囲で指定するもので .fore や .back に直接指定で良いだろう。
// この関数の第1引数 color は 8以上を指定して、上記の設定と被るのを避け、
// 以後 COLOR_PAIR(n)の nとして使うと良いだろう。


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

