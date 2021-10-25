/*
 *****************************************************************************
 *
 * 共通部ウィンドウ機能    by TOS
 *
 *   外部公開I/Fの使用方法については com_window.h を必読。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"
#include "com_window.h"


// ウィンドウ関連処理 --------------------------------------------------------

typedef struct {
    com_winId_t  cnt;      // 生成ウィンドウ数
    com_cwin_t*  inf;      // ウィンドウ情報リスト
    com_winId_t  act;      // アクティブウィンドウID
    BOOL         keypad;   // キーパッド有無
} com_winInf_t;

static com_winInf_t  gWin;  // ウィンドウモードの情報を一元管理

#define ACTWIN  (&(gWin.inf[gWin.act]))

static BOOL  gDbgWin = false;

void com_setDebugWindow( BOOL iMode )
{
    gDbgWin = iMode;
}

static char gDbgWinBuff[COM_LINEBUF_SIZE];

static void dbgWin( const char *iFormat, ... )
{
    if( !gDbgWin ) { return; }
    COM_SET_FORMAT( gDbgWinBuff );
    com_dbgCom( gDbgWinBuff );
}

static com_winpos_t *getCurPos( com_cwin_t *iWin )
{
    static com_winpos_t cur;
    if( !iWin ) { return NULL; }
    getyx( iWin->window, cur.y, cur.x );
    return &cur;
}

static com_winpos_t *getMaxPos( com_cwin_t *iWin )
{
    static com_winpos_t max;
    if( !iWin ) { return NULL; }
    getmaxyx( iWin->window, max.y, max.x );
    return &max;
}

static void getWindowPos(
        com_cwin_t *iWin, com_winpos_t *oMax, com_winpos_t *oMin )
{
    if( !iWin ) { return; }
    if( oMax ) {
        com_winpos_t max = *(getMaxPos( iWin ));
        oMax->x = max.x - iWin->border;
        oMax->y = max.y - iWin->border;
    }
    if( oMin ) {
        oMin->x = iWin->border;
        oMin->y = iWin->border;
    }
}

static void setInitPos( com_cwin_t *ioWin )
{
    //ioWin->window->_curx = ioWin->border;
    //ioWin->window->_cury = ioWin->border;
    wmove( ioWin->window, ioWin->border, ioWin->border );
}

static com_winId_t createWindow( WINDOW *iWin, BOOL iBorder )
{
    com_winId_t newId = gWin.cnt;
    com_cwin_t* tmp =
        com_reallocAddr( &(gWin.inf), sizeof(*gWin.inf), COM_TABLEEND,
                         &(gWin.cnt), 1, "new window (%ld)", gWin.cnt );
    if( !tmp ) { return COM_NO_WIN; }
    *tmp = (com_cwin_t){
        newId, iBorder, false, iWin, NULL,
        com_registerHash( 127, NULL ), COM_ONLY_OWN_KEYMAP,
        COM_NO_WIN, com_registerHash( 11, NULL )
    };
    setInitPos( tmp );
    return newId;
}

static com_cwin_t *checkWinId( com_winId_t *ioId )
{
    if( *ioId >= gWin.cnt ) { return NULL; }
    if( *ioId == COM_ACTWIN ) { *ioId = gWin.act; }
    if( *ioId < 0 ) { return NULL; }
    com_cwin_t* inf = &(gWin.inf[*ioId]);
    if( !(inf->window) ) { return NULL; }
    return inf;
}

static void forceExit( const char *iError )
{
    com_finishWindow();
    com_errorExit( COM_ERR_WINDOWNG, iError );
}

static void moveToActCursor( com_cwin_t *ioAct )
{
    if( !ioAct ) { ioAct = ACTWIN; }
    com_winpos_t cur = *(getCurPos( ioAct ));
    wmove( ioAct->window, cur.y, cur.x );
}

static void showBorder( com_cwin_t *iWin )
{
    if( iWin->border ) { wborder( iWin->window, 0,0,0,0,0,0,0,0 ); }
}

static void showWindow( com_cwin_t *iWin )
{
    showBorder( iWin );
    if( iWin->panel ) { show_panel( iWin->panel ); }
    moveToActCursor( iWin );
}

static void showPanels( void )
{
    update_panels();
    doupdate();
    moveToActCursor( NULL );
}

com_winId_t com_createWindow(
        const com_winpos_t *iPos, const com_winpos_t *iSize, BOOL iBorder )
{
    if( !gWin.cnt ) { return 0; }
    WINDOW* win = newwin( iSize->y, iSize->x, iPos->y, iPos->x );
    if( !win ) { forceExit( "fail to create new window..." ); }
    keypad( win, gWin.keypad );
    com_skipMemInfo( true );
    int newId = createWindow( win, iBorder );
    com_skipMemInfo( false );
    if( newId == COM_NO_WIN ) { forceExit( "no memory..." ); }
    PANEL* pan = new_panel( win );
    if( !pan ) { forceExit( "fail to set panel..." ); }
    com_cwin_t* inf = &(gWin.inf[newId]);
    inf->panel = pan;
    showBorder( inf );
    int maxx, maxy;
    getmaxyx( win, maxy, maxx );
    dbgWin( ">> window[%ld] size %dx%d", newId, maxx+1, maxy+1 );
    return newId;
}

BOOL com_existWindow( com_winId_t iId )
{
    if( !checkWinId( &iId ) ) { return false; }
    return true;
}

BOOL com_setBackgroundWindow( com_winId_t iId, chtype iCh )
{
    com_cwin_t* win = checkWinId( &iId );
    if( !win ) {COM_PRMNG(false);}

    if( OK == wbkgd( win->window, iCh ) ) {
        dbgWin( ">> set background[%ld] %d", iId, iCh );
    }
    return true;
}

static void cancelHash( com_cwin_t *oWin )
{
    com_cancelHash( oWin->keyHash );
    com_cancelHash( oWin->intrHash );
}

static BOOL deleteWindow( com_cwin_t *ioWin )
{
    del_panel( ioWin->panel );
    showPanels();
    delwin( ioWin->window );
    ioWin->window = NULL;
    cancelHash( ioWin );
    return true;
}

static void slideActWindow( com_winId_t iId )
{
    if( gWin.act != iId ) { return; }
    while( ++(gWin.act) < gWin.cnt ) {
        if( ACTWIN->window && !(ACTWIN->hide) ) { return; }
    }
    gWin.act = 0;
}

BOOL com_deleteWindow( com_winId_t iId )
{
    com_cwin_t* win = checkWinId( &iId );
    if( !win || iId == 0 ) {COM_PRMNG(false);}
    com_skipMemInfo( true );
    slideActWindow( iId );
    deleteWindow( win );
    com_skipMemInfo( false );
    dbgWin( ">> window[%ld] deleted", iId );
    return true;
}




typedef struct {
    com_cwin_t*    win;        // ウィンドウ情報
    com_winId_t    id;         // ウィンドウID      (com_inputWindow()の引数)
    size_t         size;       // 文字列サイズ指定  (com_inputWindow()の引数)
    char**         input;      // 入力内容 出力先   (com_inputWindow()の引数)
    const com_winpos_t*  pos;  // 入力内容 表示位置 (com_inputWindow()の引数)
    BOOL           echo;       // 入力内容 表示要否 (com_inputWindow()の引数)
    BOOL           clear;      // 入力内容 消去要否 (com_inputWindow()の引数)
} com_workInput_t;

static com_workInput_t gWorkUser[COM_WINTEXT_BUF_COUNT];

static void initializeInput( void )
{
    for( com_winTextId_t id = 0;  id < COM_WINTEXT_BUF_COUNT;  id++ ) {
        gWorkUser[id].id = COM_NO_WIN;
    }
}






static void configColors( const com_colorPair_t *iColors )
{
    if( !has_colors() ) {
        com_error( COM_ERR_WINDOWNG, "can not use color setting" );
        return;
    }
    start_color();
    for( ;  iColors->pair;  iColors++ ) {
        if( OK == init_pair( iColors->pair, iColors->fore, iColors->back ) ) {
            dbgWin( ">> set COLOR_PAIR(%d) = (%d,%d)",
                    iColors->pair, iColors->fore, iColors->back );
        }
    }
}

BOOL com_readyWindow( com_winopt_t *iOpt, com_winpos_t *oSize )
{
    if( gWin.cnt ) { return false; }   // 既に起動中なら NGを返す
    com_setFuncTrace( false );
    com_dbgCom( ">> window mode start <<" );
    setlocale( LC_ALL, "" );  // マルチバイト文字使用時のおまじない
    WINDOW* win = initscr();
    if( !win ) { com_errorExit( COM_ERR_WINDOWNG, "fail to start stdscr" ); }
    com_skipMemInfo( true );
    int result = createWindow( win, false );
    com_skipMemInfo( false );
    if( result == COM_NO_WIN ) { forceExit( "no memory..." ); }
    com_suspendStdout( true );
    if( iOpt->noEnter ) { cbreak(); }    // 原則 true
    if( iOpt->noEcho) { noecho(); }      // 原則 true
    keypad( win, iOpt->keypad );         // 原則 true
    gWin.keypad = iOpt->keypad;
    if( iOpt->colors ) { configColors( iOpt->colors ); }
    initializeInput();
    if( oSize ) { com_getWindowMax( 0, oSize ); }
    com_setFuncTrace( true );
    return true;
}

BOOL com_finishWindow( void )
{
    if( !gWin.act ) { return false; }
    COM_DEBUG_AVOID_START( COM_NO_FUNCTRACE | COM_NO_SKIPMEM );
    for( com_winId_t id = 1;  id < gWin.cnt;  id++ ) {
        if( gWin.inf[id].window ) { com_deleteWindow( id ); }
    }
    endwin();
    cancelHash( &(gWin.inf[0]) );
    com_free( gWin.inf );
    gWin.cnt = 0;
    com_skipMemInfo( false );
    com_suspendStdout( false );
    com_dbgCom( ">> window mode finish <<" );
    com_setFuncTrace( true );
    return true;
}

// 初期化処理 ----------------------------------------------------------------

static com_dbgErrName_t  gErrorNameWindow[] = {
    { COM_ERR_WINDOWNG,     "COM_ERR_WINDOWNG" },
    { COM_ERR_END,          "" }  // 最後は必ずこれで
};

static void finalizeWindow( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    com_finishWindow();
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

void com_initializeWindow( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    atexit( finalizeWindow );
    com_registerErrorCode( gErrorNameWindow );
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

