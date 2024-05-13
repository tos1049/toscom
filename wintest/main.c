/*
 * main() ひな形
 *   toscomを使う際の最も基本的なひな形となるコード。
 *   初期化の処理は、基本的にこのパターンにしたほうが良い。
 *   実行しても何もせず終了となる。
 */

#include "com_if.h"                 // toscom基本機能
#include "com_extra.h"
#include "com_window.h"

/* 独自処理があれば記述 ******************************************************/

enum { WIN_MAX = 5 };
static com_winId_t gWinId[WIN_MAX];
static long gCurWin = 0;

static BOOL gLoop = true;

static void switchWindow( com_winId_t iId )
{
    gCurWin = (iId + 1) % (WIN_MAX + 1);
}

static void revertWindow( com_winId_t iId )
{
    gCurWin = iId - 1;
    if( gCurWin < 0 ) { gCurWin = WIN_MAX; }
}

static void moveCursor( com_winId_t iId, com_winpos_t *iMove )
{
    com_winpos_t  pos;
    (void)com_getWindowCur( iId, &pos );
    pos.x = pos.x + iMove->x;
    if( pos.x < 0 ) { pos.x = 0; }
    pos.y = pos.y + iMove->y;
    if( pos.y < 0 ) { pos.y = 0; }
    (void)com_moveCursor( iId, &pos );
}

static void cursorUp( com_winId_t iId )
{
    moveCursor( iId, &(com_winpos_t){ 0, -1 } );
}

static void cursorDown( com_winId_t iId )
{
    moveCursor( iId, &(com_winpos_t){ 0, 1 } );
}

static void cursorRight( com_winId_t iId )
{
    moveCursor( iId, &(com_winpos_t){ 1, 0 } );
}

static void cursorLeft( com_winId_t iId )
{
    moveCursor( iId, &(com_winpos_t){ -1, 0 } );
}

static void moveWindow( com_winId_t iId, com_winpos_t *iMove )
{
    com_winpos_t  pos;
    (void)com_getWindowPos( iId, &pos );
    pos.x = pos.x + iMove->x;
    pos.y = pos.y + iMove->y;
    (void)com_moveWindow( iId, &pos );
}

static void moveUpWindow( com_winId_t iId )
{
    moveWindow( iId, &(com_winpos_t){ 0, -1 } );
}

static void moveDownWindow( com_winId_t iId )
{
    moveWindow( iId, &(com_winpos_t){ 0, 1 } );
}

static void moveRightWindow( com_winId_t iId )
{
    moveWindow( iId, &(com_winpos_t){ 1, 0 } );
}

static void moveLeftWindow( com_winId_t iId )
{
    moveWindow( iId, &(com_winpos_t){ -1, 0 } );
}

static void inputString( com_winId_t iId )
{
    com_wininopt_t opt = { COM_IN_1LINE, -1, 0, true, false };
    com_winpos_t pos;
    char* string = NULL;

    (void)com_getWindowCur( iId, &pos );
    while ( !com_inputWindow( iId, &string, &opt, &pos ) ) {}
}

// 全てのウィンドウ(ID=0も含む)で受け付ける操作
static com_keymap_t  gBaseKeymap[] = {
    { '\t',        switchWindow },      // TAB         ウィンドウ切替(順方向)
    { 0x5a,        revertWindow },      // SHIFT+TAB   ウィンドウ切替(逆方向)
    { 'k',         cursorUp },          // カーソルを上に移動
    { 'j',         cursorDown },        // カーソルを下に移動
    { 'l',         cursorRight },       // カーソルを右に移動
    { 'h',         cursorLeft },        // カーソルを左に移動
    { ' ',         inputString },       // 文字列入力
    {   0, NULL }
};

// 移動可能なウィンドウ(ID=0以外)でのみ受け付ける動作
static com_keymap_t  gWinKeymap[] = {
    { 0x41,         moveUpWindow },     // カーソル上  ウィンドウを上に移動
    { 0x42,         moveDownWindow },   // カーソル下  ウィンドウを下に移動
    { 0x43,         moveRightWindow },  // カーソル右  ウィンドウを右に移動
    { 0x44,         moveLeftWindow },   // カーソル左  ウィンドウを左に移動
    {   0, NULL }
};

static com_winId_t setupWin(
        com_winpos_t *iPos, com_winpos_t *iSize, BOOL iBorder )
{
    com_winId_t id = com_createWindow( iPos, iSize, iBorder );
    if( id == COM_NO_WIN ) { return 0; }

    (void)com_mprintWindow( id, &(com_winpos_t){0,10},
                            A_REVERSE, true, "ID%ld", id );
    if( !com_setWindowKeymap( id, gWinKeymap ) ) { return 0; }
    if( !com_mixWindowKeymap( id, 0 ) ) { return 0; }
    if( !com_activateWindow( id ) ) { return 0; }
    gCurWin = id;
    return id;
}

#define ABORT  gLoop=false;return;

static void startWindow( void )
{
    com_winopt_t  winOpt = { true, true, false, NULL };
    com_winpos_t  winSize;
    if( !com_readyWindow( &winOpt, &winSize ) ) { ABORT; }
    if( !com_setWindowKeymap( 0, gBaseKeymap ) ) { ABORT; }
    (void)com_mprintWindow( 0, &(com_winpos_t){0,winSize.y-1},
                            A_REVERSE, true, "BOTTOM" );
    (void)com_mprintWindow( 0, &(com_winpos_t){65,winSize.y-1},
                            A_UNDERLINE, true, "CTRL+C to quit" );

    com_winpos_t  winPos  = { 10,  0 };
    winSize = (com_winpos_t){ 40, 10 };
    for( int id = 0; id < WIN_MAX; id++ ) {
        if( !(gWinId[id] = setupWin( &winPos, &winSize, true )) ) { ABORT; }
        winPos.x += 5;
        winPos.y += 3;
    }
}

static void quitLoop( int iSignal )
{
    // CTRL+C のみ終了の処理とする
    if( iSignal == SIGINT ) { gLoop = false; }
    // CTRL+Z は現状無視。今後 何か機能を追加出来るように本関数を呼ぶ作りとする
}

static com_sigact_t gSigHandler[] = {
    { SIGINT,         { .sa_handler = quitLoop } },    // CTRL+C
    { SIGTSTP,        { .sa_handler = quitLoop } },    // CTRL+Z
    { COM_SIGACT_END, { .sa_handler = NULL } }
};
      
static void loopMain( void ) 
{
    if( !com_setSignalAction( gSigHandler ) ) {return;}
    gCurWin = 1;
    while( gLoop ) {
        (void)com_activateWindow( gCurWin );
        (void)com_checkWindowKey( gCurWin, 50 );
        int lastKey = com_getLastKey();
        if( lastKey > 0 ) {
            (void)com_mprintWindow(
                    0, &(com_winpos_t){0,0}, A_BOLD, true, "%02x", lastKey );
        }
    }
}

static void finishWindow( void )
{
    (void)com_finishWindow();
}

/* 初期化処理 ****************************************************************/

static void initialize( int iArgc, char **iArgv )
{
    COM_INITIALIZE( iArgc, iArgv );
    // これ以降に他モジュールの初期化処理が続く想定
}

/* 起動オプション判定 *******************************************************/
// 新しいオプションを追加したい時は、オプション処理関数を作成後、
// gOptions[]に定義を追加する。詳細は com_if.hの com_getOption()の説明を参照。

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
    com_system( "rm -fr .%s.log_*", com_getAplName() );
    com_printf( "<<< cleared all past logs >>>\n" );
    com_exit( COM_NO_ERROR );
    return true;
}

#ifdef    USE_TESTFUNC    // テスト関数使用のコンパイルオプション
static BOOL execTest( com_getOptInf_t *iOptInf )
{
    com_testCode( iOptInf->argc, iOptInf->argv );
    return true;
}
#endif // USE_TESTFUNC

static com_getOpt_t gOptions[] = {
    { 'v', "version",  0,           0, false, viewVersion },
    { 'c', "clearlog", 0,           0, false, clearLogs },
#ifdef    USE_TESTFUNC
    {   0, "debug",    COM_OPT_ALL, 0, false, execTest },
#endif // USE_TESTFUNC
    COM_OPTLIST_END
};

/* メイン関数 ***************************************************************/

int main( int iArgc, char **iArgv )
{
    initialize( iArgc, iArgv );
    long   restArgc = 0;    // オプションチェック後、まだ残ったオプション数
    char** restArgv = NULL; // オプションチェック後の内容リスト
    com_getOption( iArgc, iArgv, gOptions, &restArgc, &restArgv, NULL );

    /* 以後、必要な処理を記述 */
    startWindow();
    loopMain();
    finishWindow();

    return EXIT_SUCCESS;
}

