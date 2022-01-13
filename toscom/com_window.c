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

static char  gDbgWinBuff[COM_LINEBUF_SIZE];

static void dbgWin( const char *iFormat, ... )
{
    if( !gDbgWin ) {return;}
    COM_SET_FORMAT( gDbgWinBuff );
    com_dbgCom( gDbgWinBuff );
}

static com_winpos_t *getCurPos( com_cwin_t *iWin )
{
    static com_winpos_t  cur;
    if( !iWin ) {return NULL;}
    getyx( iWin->window, cur.y, cur.x );
    return &cur;
}

static com_winpos_t *getMaxPos( com_cwin_t *iWin )
{
    static com_winpos_t  max;
    if( !iWin ) {return NULL;}
    getmaxyx( iWin->window, max.y, max.x );
    return &max;
}

static void getWindowPos(
        com_cwin_t *iWin, com_winpos_t *oMax, com_winpos_t *oMin )
{
    if( !iWin ) {return;}
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

#define GETWINPOS( WINDOW ) \
    com_winpos_t  wmax = {0,0}, wmin = {0,0}; \
    getWindowPos( (WINDOW), &wmax, &wmin ); \
    do{} while(0)

static void setInitPos( com_cwin_t *ioWin )
{
    //ioWin->window->_curx = ioWin->border;
    //ioWin->window->_cury = ioWin->border;
    wmove( ioWin->window, ioWin->border, ioWin->border );
}

static com_winId_t createWindow( WINDOW *iWin, BOOL iBorder )
{
    com_winId_t  newId = gWin.cnt;
    com_cwin_t*  tmp =
        com_reallocAddr( &(gWin.inf), sizeof(*gWin.inf), COM_TABLEEND,
                         &(gWin.cnt), 1, "new window (%ld)", gWin.cnt );
    if( !tmp ) {return COM_NO_WIN;}
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
    if( *ioId >= gWin.cnt ) {return NULL;}
    if( *ioId == COM_ACTWIN ) {*ioId = gWin.act;}
    if( *ioId < 0 ) {return NULL;}
    com_cwin_t*  inf = &(gWin.inf[*ioId]);
    if( !(inf->window) ) {return NULL;}
    return inf;
}

static void forceExit( const char *iError )
{
    com_finishWindow();
    com_errorExit( COM_ERR_WINDOWNG, iError );
}

static void moveToActCursor( com_cwin_t *ioAct )
{
    if( !ioAct ) {ioAct = ACTWIN;}
    com_winpos_t  cur = *(getCurPos( ioAct ));
    wmove( ioAct->window, cur.y, cur.x );
}

static void showBorder( com_cwin_t *iWin )
{
    if( iWin->border ) {wborder( iWin->window, 0,0,0,0,0,0,0,0 );}
}

static void showWindow( com_cwin_t *iWin )
{
    showBorder( iWin );
    if( iWin->panel ) {show_panel( iWin->panel );}
    moveToActCursor( iWin );
}

static void showPanels( void )
{
    update_panels();
    doupdate();
    moveToActCursor( NULL );
}

#define NOWINDOW( RETURN ) \
    if( !gWin.cnt ) {return (RETURN);} \
    do{} while(0)

com_winId_t com_createWindow(
        const com_winpos_t *iPos, const com_winpos_t *iSize, BOOL iBorder )
{
    NOWINDOW( COM_NO_WIN );
    WINDOW*  win = newwin( iSize->y, iSize->x, iPos->y, iPos->x );
    if( !win ) {forceExit( "fail to create new window..." );}
    keypad( win, gWin.keypad );
    com_skipMemInfo( true );
    int  newId = createWindow( win, iBorder );
    com_skipMemInfo( false );
    if( newId == COM_NO_WIN ) {forceExit( "no memory..." );}
    PANEL*  pan = new_panel( win );
    if( !pan ) {forceExit( "fail to set panel..." );}
    com_cwin_t*  inf = &(gWin.inf[newId]);
    inf->panel = pan;
    showBorder( inf );
    int  maxx, maxy;
    getmaxyx( win, maxy, maxx );
    dbgWin( ">> window[%ld] size %dx%d", newId, maxx+1, maxy+1 );
    return newId;
}

BOOL com_existWindow( com_winId_t iId )
{
    if( !checkWinId( &iId ) ) {return false;}
    return true;
}

#define GET_WIN( RET ) \
    com_cwin_t*  win = checkWinId( &iId ); \
    if( !win ) {COM_PRMNG(RET);} \
    do {} while(0)

#define NO_ROOTWIN( RET ) \
    if( iId == 9 ) {COM_PRMNG(RET);} \
    do {} while(0)

BOOL com_setBackgroundWindow( com_winId_t iId, chtype iCh )
{
    GET_WIN( false );
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
    if( gWin.act != iId ) {return;}
    while( ++(gWin.act) < gWin.cnt ) {
        if( ACTWIN->window && !(ACTWIN->hide) ) {return;}
    }
    gWin.act = 0;
}

BOOL com_deleteWindow( com_winId_t iId )
{
    NO_ROOTWIN( false );
    GET_WIN( false );
    com_skipMemInfo( true );
    slideActWindow( iId );
    deleteWindow( win );
    com_skipMemInfo( false );
    dbgWin( ">> window[%ld] deleted", iId );
    return true;
}

int com_getWindowInf( com_winId_t iId, const com_cwin_t **oInf )
{
    COM_SET_IF_EXIST( oInf, NULL );
    if( !oInf ) {COM_PRMNG(COM_NO_WIN);}
    const GET_WIN( COM_NO_WIN );
    *oInf = win;
    return iId;
}

BOOL com_getWindowPos( com_winId_t iId, com_winpos_t *oPos )
{
    if( !oPos ) {COM_PRMNG(false);}
    GET_WIN( false );
    getbegyx( win->window, oPos->y, oPos->x );
    dbgWin( ">> window[%ld] pos(%d,%d)", iId, oPos->x, oPos->y );
    return true;
}

BOOL com_getWindowMax( com_winId_t iId, com_winpos_t *oSize )
{
    if( !oSize ) {COM_PRMNG(false);}
    GET_WIN( false );
    *oSize = *(getMaxPos( win ));
    dbgWin( ">> window[%ld] size %dx%d", iId, oSize->x, oSize->y );
    return true;
}

BOOL com_getWindowCur( com_winId_t iId, com_winpos_t *oCur )
{
    if( !oCur ) {COM_PRMNG(false);}
    GET_WIN( false );
    *oCur = *(getCurPos( win ));
    dbgWin( ">> window[%ld] cur(%d,%d)", iId, oCur->x, oCur->y );
    return true;
}

BOOL com_activateWindow( com_winId_t iId )
{
    GET_WIN( false );
    if( win->hide ) {return false;}
    gWin.act = iId;
    showWindow( win );
    top_panel( win->panel );
    showPanels();
    return true;
}

BOOL com_refreshWindow( void )
{
    NOWINDOW( false );
    showPanels();
    dbgWin( ">> refresh" );
    return true;
}

BOOL com_moveWindow( com_winId_t iId, const com_winpos_t *iPos )
{
    if( !iPos ) {COM_PRMNG(false);}
    NO_ROOTWIN( false );
    GET_WIN( false );
    if( ERR == move_panel( win->panel, iPos->y, iPos->x ) ) {return false;}
    showPanels();
    dbgWin( ">> place[%ld](%d,%d)", iId, iPos->x, iPos->y );
    return true;
}

BOOL com_floatWindow( com_winId_t iId )
{
    NO_ROOTWIN( false );
    GET_WIN( false );
    top_panel( win->panel );
    showPanels();
    return true;
}

BOOL com_sinkWindow( com_winId_t iId )
{
    NO_ROOTWIN( false );
    GET_WIN( false );
    bottom_panel( win->panel );
    showPanels();
    return true;
}

BOOL com_hideWindow( com_winId_t iId, BOOL iHide )
{
    NO_ROOTWIN( false );
    GET_WIN( false );
    if( win->hide != iHide ) {
        if( iHide ) {
            hide_panel( win->panel );
            slideActWindow( iId );
        }
        else {show_panel( win->panel );}
    }
    win->hide = iHide;
    com_activateWindow( COM_ACTWIN );
    dbgWin( ">> hide[%ld] = %ld", iId, iHide );
    return true;
}

static void addWinPos( com_winpos_t *oTarget, const com_winpos_t *iAdd )
{
    oTarget->x += iAdd->x;
    oTarget->y += iAdd->y;
}

BOOL com_fillWindow( com_winId_t iId, int iChr )
{
    GET_WIN( false );
    GETWINPOS( win );
    for( int row = wmin.y;  row < wmax.y;  row++ ) {
        for( int col = wmin.x;  col < wmax.x;  col++ ) {
            mvwprintw( win->window, row, col, "%c", iChr );
        }
    }
    setInitPos( win );
    dbgWin( ">> fill[%ld] '%c'", iId, iChr );
    return true;
}

BOOL com_clearWindow( com_winId_t iId )
{
    GET_WIN( false );
    wclear( win->window );
    setInitPos( win );
    showBorder( win );
    dbgWin( ">> clear[%ld]", iId );
    return true;
}

static void checkWinPos( com_cwin_t *iWin, com_winpos_t *iPos )
{
    GETWINPOS( iWin );
    if( iPos->x < 0 ) {iPos->x += wmax.x;}
    if( iPos->y < 0 ) {iPos->y += wmax.y;}

    if( iPos->x <  wmin.x ) {iPos->x = wmin.x;}
    if( iPos->x >= wmax.x ) {iPos->x = wmax.x - 1;}
    if( iPos->y <  wmin.y ) {iPos->y = wmin.y;}
    if( iPos->y >= wmax.y ) {iPos->y = wmax.y - 1;}
}

BOOL com_moveCursor( com_winId_t iId, const com_winpos_t *iPos )
{
    if( !iPos ) {COM_PRMNG(false);}
    GET_WIN( false );
    com_winpos_t  pos = *iPos;
    checkWinPos( win, &pos );
    wmove( win->window, pos.y, pos.x );
    dbgWin( ">> move[%ld](%d,%d)", iId, pos.x, pos.y );
    return true;
}

BOOL com_shiftCursor(
        com_winId_t iId, const com_winpos_t *iShift, BOOL *oBlocked )
{
    if( !iShift ) {COM_PRMNG(false);}
    GET_WIN( false );
    GETWINPOS( win );
    com_winpos_t  pos = *(getCurPos( win ));
    addWinPos( &pos, iShift );

    BOOL blocked = false;
    if( pos.x <  wmin.x ) {pos.x = wmin.x;      blocked = true;}
    if( pos.x >= wmax.x ) {pos.x = wmax.x - 1;  blocked = true;}
    if( pos.y <  wmin.y ) {pos.y = wmin.y;      blocked = true;}
    if( pos.y >= wmax.y ) {pos.y = wmax.y - 1;  blocked = true;}

    COM_SET_IF_EXIST( oBlocked, blocked );
    wmove( win->window, pos.y, pos.x );
    dbgWin( ">> shift[%ld](%d,%d) blocked=%ld", iId, pos.x, pos.y, blocked );
    return true;
}

BOOL com_udlrCursor( com_winId_t iId, int iDir, int iDist, BOOL *oBlocked )
{
    if( COM_CHECKBIT( iDir, COM_CUR_UP | COM_CUR_DOWN ) ) {COM_PRMNG(false);}
    if( COM_CHECKBIT( iDir, COM_CUR_RIGHT | COM_CUR_LEFT ) ) {COM_PRMNG(false);}
    if( !iDist ) {return true;}
    com_winpos_t  shift = { 0, 0 };
    if( iDir & COM_CUR_UP )    {shift.y = iDist * -1;}
    if( iDir & COM_CUR_DOWN )  {shift.y = iDist;}
    if( iDir & COM_CUR_LEFT )  {shift.x = iDist * -1;}
    if( iDir & COM_CUR_RIGHT ) {shift.x = iDist;}
    return com_shiftCursor( iId, &shift, oBlocked );
}



// ウィンドウ出力関連 --------------------------------------------------------

static void writeWindow(
        com_cwin_t *iWin, com_winpos_t *iPos, int iAttr, const char *iText )
{
    WINDOW*  window = iWin->window;
    if( iAttr ) {wattron( window, iAttr );}
    if( iPos ) {wmove( window, iPos->y, iPos->x );}
    wprintw( window, iText );
    if( iAttr ) {wattroff( window, iAttr );}

    if( iPos ) {
        dbgWin( ">> print[%ld](%d,%d) %s", iWin->id, iPos->x, iPos->y, iText );
    }
    else {
        dbgWin( ">> print[%ld](cur) %s", iWin->id, iText );
    }
}

static char  gFrmBuff[COM_DATABUF_SIZE];
static char  gFrmEditBuff[COM_TEXTBUF_SIZE];

static void wrapWindow( com_cwin_t *iWin, int iAttr )
{
    com_winpos_t  pos = *(getCurPos( iWin ));
    char*  text = gFrmBuff;
    GETWINPOS( iWin );
    for( ;  pos.y < wmax.y;  (pos.y)++ ) {
        size_t  textWidth = (size_t)(wmax.x - pos.x);
        (void)com_strncpy( gFrmEditBuff,sizeof(gFrmEditBuff),text,textWidth );
        writeWindow( iWin, &pos, iAttr, gFrmEditBuff );
        if( strlen(text) <= textWidth ) {break;}
        text += textWidth;
        pos.x = wmin.x;
    }
}

static void adjustWindow( com_cwin_t *iWin, int iAttr )
{
    com_winpos_t  pos = *(getCurPos( iWin ));
    com_winpos_t  wmax;
    getWindowPos( iWin, &wmax, NULL );
    size_t  frmWidth = (size_t)( wmax.x - iWin->border );
    (void)com_strncpy( gFrmEditBuff, sizeof(gFrmEditBuff), gFrmBuff, frmWidth );
    int  editLen = (int)strlen( gFrmEditBuff );
    if( pos.x + editLen >= wmax.x ) {
        pos.x = wmax.x - editLen;
        if( pos.x == 0 && iWin->border ) {
            (pos.x)++;
            gFrmEditBuff[editLen - 1] = '\0';
        }
    }
    writeWindow( iWin, &pos, iAttr, gFrmEditBuff );
}

BOOL com_mprintw(
        com_winId_t iId, const com_winpos_t *iPos, int iAttr,
        const char *iFormat, ... )
{
    GET_WIN( false );
    COM_SET_FORMAT( gFrmBuff );
    com_winpos_t*  usePos = NULL;
    com_winpos_t  pos = {0,0};
    if( iPos ) {
        pos = *iPos;
        checkWinPos( win, &pos );
        usePos = &pos;
    }
    writeWindow( win, usePos, iAttr, gFrmBuff );
    return true;
}

BOOL com_mprintWindow(
        com_winId_t iId, const com_winpos_t *iPos, int iAttr, BOOL iWrap,
        const char *iFormat, ... )
{
    GET_WIN( false );
    com_winpos_t  cur = *(getCurPos( win ));   // 対象カーソル位置保存
    if( iPos ) {com_moveCursor( iId, iPos );}
    COM_SET_FORMAT( gFrmBuff );
    if( iWrap ) {wrapWindow( win, iAttr );}
    else {adjustWindow( win, iAttr );}
    com_moveCursor( iId, &cur );              // 対象カーソル位置復元
    if( gWin.act != iId ) {moveToActCursor( NULL );}
    showPanels();
    return true;
}



// ウィンドウ入力関連 --------------------------------------------------------

typedef struct {
    com_cwin_t*    win;        // ウィンドウ情報
    com_winId_t    id;         // ウィンドウID      (com_inputWindow()の引数)
    size_t         size;       // 文字列サイズ指定  (com_inputWindow()の引数)
    char**         input;      // 入力内容 出力先   (com_inputWindow()の引数)
    const com_winpos_t*  pos;  // 入力内容 表示位置 (com_inputWindow()の引数)
    BOOL           echo;       // 入力内容 表示要否 (com_inputWindow()の引数)
    BOOL           clear;      // 入力内容 消去要否 (com_inputWindow()の引数)
} com_workInput_t;

static com_workInput_t  gWorkUser[COM_WINTEXT_BUF_COUNT];

static void initializeInput( void )
{
    for( com_winTextId_t id = 0;  id < COM_WINTEXT_BUF_COUNT;  id++ ) {
        gWorkUser[id].id = COM_NO_WIN;
    }
}

// かなり大きなサイズになるので適正サイズを再考する余地あり
static wchar_t*  gWorkWstr;
static wchar_t   gInputWstr[COM_WINTEXT_BUF_COUNT][COM_WINTEXT_BUF_SIZE];
static size_t*   gWorkSize;
static size_t    gInputSize[COM_WINTEXT_BUF_COUNT];
// MB_CUR_MAX に相当する 6 を入れた。
// このマクロは定数マクロではないようで、静的変数定義にはつかえなかった。
static char      gInputText[COM_WINTEXT_BUF_SIZE * 6];

enum {
    KEYLF  = 0x0a,   // Enter
    KEYEOT = 0x04,   // Ctrl + D
    KEYBS  = 0x08,   // Backspace
    KEYESC = 0x1B    // ESC
};

static com_workInput_t *setWork( com_winTextId_t iId, com_workInput_t *iWork )
{
    if( iId == COM_NO_WIN ) {return NULL;}
    if( iWork ) {gWorkUser[iId] = *iWork;}
    gWorkWstr = gInputWstr[iId];
    gWorkSize = &(gInputSize[iId]);
    return &(gWorkUser[iId]);
}

static BOOL getWorkBuff( com_workInput_t *iWork )
{
    com_winTextId_t*  tid = &(iWork->win->textId);
    if( *tid == COM_NO_WIN ) {
        for( com_winTextId_t i = 0;  i < COM_WINTEXT_BUF_COUNT;  i++ ) {
            if( gWorkUser[i].id == COM_NO_WIN ) {
                *tid = i;
                dbgWin( ">> use wtext buffer[%ld]", i );
                break;
            }
        }
        if( *tid == COM_NO_WIN ) {
            com_error( COM_ERR_WINDOWNG, "fail to get input text buffer" );
            return false;
        }
    }
    (void)setWork( *tid, iWork );
    return true;
}

static void deleteInput( com_workInput_t *iWork, wchar_t iTarget )
{
    // 多バイト文字を頑張って消す作業
    char  tmp[8] = {0};
    if( wctomb( tmp, iTarget ) ) {
        for( int i = 0;  tmp[i];  i++ ) {
            wprintw( iWork->win->window, " " );
            (*gWorkSize)--;
        }
    }
}

static void clearText( com_workInput_t *iWork, BOOL iBackSpace )
{
    if( iWork->pos ) {com_moveCursor( iWork->id, iWork->pos );}
    if( wcslen( gWorkWstr ) == 0 ) {return;}
    WINDOW*  win = iWork->win->window;
    size_t  i = 0;   // for文外でも使うため、ここで定義
    for( ;  i < wcslen( gWorkWstr ) - 1;  i++ ) {
        if( gWorkWstr[i] == KEYLF ) {wprintw( win, "\n" );  continue;}
        if( iBackSpace ) {wprintw( win, "%lc", gWorkWstr[i] );}
        else {deleteInput( iWork, gWorkWstr[i] );}
    }
    deleteInput( iWork, gWorkWstr[i] );
    // com_refreshWindow()は、この後の処理で実施する。
}

static BOOL isEnter( COM_IN_TYPE_t iType, wint_t iWch )
{
    if( iType == COM_IN_1LINE && iWch == KEYLF ) {return true;}
    if( iType == COM_IN_MULTILINE && iWch == KEYEOT ) {return true;}
    return false;
}

static void resetWstrBuff( com_workInput_t *iWork )
{
    memset( gWorkWstr, 0, COM_WINTEXT_BUF_SIZE );
    *gWorkSize = 0;
    com_winTextId_t*  tid = &(iWork->win->textId);
    gWorkUser[*tid].id = COM_NO_WIN;
    *tid = COM_NO_WIN;
}

static void resetWorkBuffer( com_workInput_t *iWork )
{
    if( iWork->echo && iWork->clear ) {
        clearText( iWork, false );
        com_moveCursor( iWork->id, iWork->pos );
        com_refreshWindow();
    }
    resetWstrBuff( iWork );
}

static void convertText( void )
{
    COM_CLEAR_BUF( gInputText );
    wcstombs( gInputText, gWorkWstr, sizeof(gInputText) );
}

static BOOL confirmText( com_workInput_t *iWork )
{
    convertText();
    *(iWork->input) = gInputText;
    resetWorkBuffer( iWork );
    dbgWin( ">> text[%ld] %.20s", iWork->id, *(iWork->input) );
    return true;
}

static BOOL backText( com_workInput_t *iWork )
{
    if( *gWorkWstr == L'\0' ) {return false;}
    if( iWork->echo ) {clearText( iWork, true );}
    gWorkWstr[wcslen(gWorkWstr) - 1] = L'\0';
    if( iWork->echo ) {
        com_mprintw( iWork->id, iWork->pos, 0, "%ls", gWorkWstr );
        com_refreshWindow();
    }
    return false;
}

static BOOL cancelText( com_workInput_t *iWork )
{
    if( iWork->echo ) {iWork->clear = true;}
    resetWorkBuffer( iWork );
    COM_CLEAR_BUF( gInputText );
    *(iWork->input) = gInputText;
    return true;
}

static int getInputSize( wint_t iVal )
{
    char  tmp[MB_CUR_MAX];
    return (size_t)wctomb( tmp, iVal );
}

static int isOverSize( com_workInput_t *iWork, wint_t iInput )
{
    int  inSize = getInputSize( iInput );
    if( iWork->win->border ) {
        com_winpos_t  cur, size;
        com_getWindowCur( iWork->id, &cur );
        com_getWindowMax( iWork->id, &size );
        if( cur.x + inSize >= size.x ) {return 0;}
    }
    if( iWork->size ) {
        if( *gWorkSize + inSize >= iWork->size ) {return 0;}
    }
    if( wcslen(gWorkWstr) + 1 >= COM_WINTEXT_BUF_SIZE ) {return 0;}
    return inSize;
}

static BOOL addText( com_workInput_t *iWork, wint_t *iInput )
{
    if( *iInput != KEYLF && !iswprint( *iInput ) ) {return false;}
    int  size = isOverSize( iWork, *iInput );
    if( !size ) {return false;}
    wcscat( gWorkWstr, (wchar_t*)iInput );
    *gWorkSize += size;
    return true;
}

// 関数ポインタは void*型と直接交換できないため、構造体を噛ませる
typedef struct {
    com_intrKeyCB_t func;
} tmpIntrKeyCBFunc_t;

static BOOL setInterruptKey(
        com_winId_t iId, const com_intrKey_t *iIntrKey, com_hashId_t iHash )
{
    long  key;
    for( ;  (key = iIntrKey->key);  iIntrKey++ ) {
        tmpIntrKeyCBFunc_t  tmp = { iIntrKey->func };
        if( !com_addHash( iHash,true,&key,sizeof(key),&tmp,sizeof(tmp) ) ) {
            return false;
        }
        dbgWin( ">> interrupt[%ld] added (%s)", iId, key );
    }
    return true;
}

BOOL com_setIntrInputWindow( com_winId_t iId, const com_intrKey_t *iIntrKey )
{
    if( !iIntrKey ) {COM_PRMNG(false);}
    GET_WIN( false );
    com_skipMemInfo( true );
    BOOL  result = setInterruptKey( iId, iIntrKey, win->intrHash );
    com_skipMemInfo( false );
    return result;
}

static BOOL searchKeyHash( com_hashId_t iHash, long iKey, const void **oData )
{
    if( !com_searchHash( iHash, &iKey, sizeof(iKey), oData, NULL ) ) {
        return false;
    }
    return true;
}

static BOOL detectInterrupt(
        com_workInput_t *iWork, wint_t iWch, BOOL *oResult )
{
    const void* data;
    if( !searchKeyHash( iWork->win->intrHash, (long)iWch, &data ) ) {
        return false;
    }
    com_intrKeyCB_t  func;
    memcpy( &func, data, sizeof(func) );
    *oResult = (func)( iWork->id );
    return true;
}

static void echoKey( com_workInput_t *iWork )
{
    if( !iWork->echo ) {return;}
    com_mprintw( iWork->id, iWork->pos, 0, "%ls", gWorkWstr );
    com_refreshWindow();
}

static BOOL pushOneKey( wint_t *iWch, com_workInput_t *iWork )
{
    *gWorkWstr = (wchar_t)(*iWch);
    echoKey( iWork );
    return confirmText( iWork );
}

static BOOL inputStringLine( 
        com_workInput_t *iWork, wint_t *iWch, com_wininopt_t *iOpt )
{
    if( isEnter( iOpt->type, *iWch ) ) {return confirmText( iWork );}
    else if( *iWch == KEYBS ) {return backText( iWork );}
    else if( *iWch == KEYESC ) {return cancelText( iWork );}
    else {
        BOOL  result = false;
        if( detectInterrupt( iWork, *iWch, &result ) ) {return result;}
        if( !addText( iWork, iWch ) ) {return false;}
    }
    echoKey( iWork );
    return false;
}

BOOL com_inputWindow(
        com_winId_t iId, char **oInput, com_wininopt_t *iOpt,
        const com_winpos_t *iPos )
{
    if( !iOpt ) {COM_PRMNG(false);}
    com_workInput_t  work = {
        checkWinId( &iId ), iId, iOpt->size, oInput, iPos,
        iOpt->echo, iOpt->clear
    };
    if( !work.win || !oInput || !iPos ) {COM_PRMNG(false);}
    if( !getWorkBuff( &work ) ) {return true;}

    timeout( iOpt->delay );
    wint_t  wch[2] = {0};
    if( ERR == get_wch( wch ) ) {return false;}
    if( iOpt->type == COM_IN_1KEY ) {return pushOneKey( wch, &work );}
    // 以後 文字入力処理 (iType = COM_IN_LINE/COM_IN_MULTILINE)
    return inputStringLine( &work, wch, iOpt );
}

size_t com_getRestSize( com_winId_t iId )
{
    GET_WIN(0);
    com_winpos_t  cur = *(getCurPos( win ));
    com_winpos_t  max = *(getMaxPos( win ));
    if( win->border ) {return (max.x - cur.x);}
    return (size_t)(max.x * max.y - ((max.x * cur.y) + cur.x) + 1);
}

#define GET_WINWORK( NGRET ) \
    GET_WIN( NGRET ); \
    com_workInput_t*  work = NULL; \
    if( !(work = setWork( win->textId, NULL )) ) {COM_PRMNG( NGRET );} \
    do{} while(0)

char *com_getInputWindow( com_winId_t iId )
{
    GET_WINWORK( NULL );
    convertText();
    return gInputText;
}

void com_setInputWindow( com_winId_t iId, const char *iText )
{
    GET_WINWORK();
    if( work->echo ) {(void)clearText( work, false );}
    mbstowcs( gWorkWstr, iText, COM_WINTEXT_BUF_SIZE - 1 );
    if( work->echo ) {
        com_mprintw( work->id, work->pos, 0, "%ls", gWorkWstr );
        com_refreshWindow();
    }
}

void com_cancelInputWindow( com_winId_t iId )
{
    GET_WINWORK();
    (void)cancelText( work );
}

// 関数ポインタは void*型と直接交換できないため、構造体を噛ませる
typedef struct {
    com_keyCB_t func;
} tmpKeyCBfunc_t;

static BOOL setWindowKeymap(
        com_winId_t iId, const com_keymap_t *iKeymap, com_hashId_t iHash )
{
    long  key;
    for( ;  (key = iKeymap->key);  iKeymap++ ) {
        tmpKeyCBfunc_t  tmp = { iKeymap->func };
        if( !com_addHash( iHash,true,&key,sizeof(key),&tmp,sizeof(tmp) ) ) {
            return false;
        }
        dbgWin( ">> keymap[%ld] added (%c)", iId, key );
    }
    return true;
}

BOOL com_setWindowKeymap( com_winId_t iId, const com_keymap_t *iKeymap )
{
    if( !iKeymap ) {COM_PRMNG(false);}
    GET_WIN( false );
    com_skipMemInfo( true );
    com_hash_t  result = setWindowKeymap( iId, iKeymap, win->keyHash );
    com_skipMemInfo( false );
    return result;
}

BOOL com_mixWindowKeymap( com_winId_t iId, com_winId_t iSourceId )
{
    if( iId == iSourceId ) {COM_PRMNG(false);}
    GET_WIN( false );
    com_cwin_t*  src = checkWinId( &iSourceId );
    if( iSourceId == COM_ONLY_OWN_KEYMAP ) {  // srcは NULLになっているはず
        win->mixKeyHash = iSourceId;
        dbgWin( ">> set[%ld] keymap no mix", iId );
        return true;
    }
    if( COM_UNLIKELY( !src )) {COM_PRMNG(false);}
    win->mixKeyHash = src->keyHash;
    dbgWin( ">> set[%ld] mix keymap[%ld]", iId, iSourceId );
    return true;
}

static int  gLastKey = 0;

int com_getLastKey( void )
{
    return gLastKey;
}

static BOOL searchWindowKeymap( com_keyCB_t *iFunc, com_hashId_t iHash )
{
    const void*  data;
    if( !searchKeyHash( iHash, (long)gLastKey, &data ) ) {return false;}
    tmpKeyCBfunc_t tmp;
    memcpy( &tmp, data, sizeof(*iFunc) );
    *iFunc = tmp.func;
    return true;
}

static BOOL checkKeymap( com_keyCB_t *iFunc, com_cwin_t *iWin, BOOL *oMixed )
{
    *oMixed = false;
    if( searchWindowKeymap( iFunc, iWin->keyHash ) ) {return true;}
    if( iWin->mixKeyHash == COM_ONLY_OWN_KEYMAP ) {return false;}
    *oMixed = true;
    return searchWindowKeymap( iFunc, iWin->mixKeyHash );
}

BOOL com_checkWindowKey( com_winId_t iId, int iDelay )
{
    GET_WIN( false );
    wtimeout( win->window, iDelay );
    if( ERR == (gLastKey = wgetch( win->window )) ) {return false;}
    com_keyCB_t  func = NULL;
    BOOL  mixed = false;
    if( checkKeymap( &func, win, &mixed ) ) {
        if( func ) {
            dbgWin( ">> keymap[%ld] exist %d '%c' (use mixed=%ld)",
                    iId, gLastKey, gLastKey, mixed );
            func( iId );
            return true;
        }
    }
    dbgWin( ">> no keymap[%ld] %d '%c'", iId, gLastKey, gLastKey );
    return false;
}



// ウィンドウ初期化/終了処理 -------------------------------------------------

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
    WINDOW*  win = initscr();
    if( !win ) {com_errorExit( COM_ERR_WINDOWNG, "fail to start stdscr" );}
    com_skipMemInfo( true );
    int  result = createWindow( win, false );
    com_skipMemInfo( false );
    if( result == COM_NO_WIN ) {forceExit( "no memory..." );}
    com_suspendStdout( true );
    if( iOpt->noEnter ) {cbreak();}    // 原則 true
    if( iOpt->noEcho) {noecho();}      // 原則 true
    keypad( win, iOpt->keypad );         // 原則 true
    gWin.keypad = iOpt->keypad;
    if( iOpt->colors ) {configColors( iOpt->colors );}
    initializeInput();
    if( oSize ) {com_getWindowMax( 0, oSize );}
    com_setFuncTrace( true );
    return true;
}

BOOL com_finishWindow( void )
{
    NOWINDOW( false );
    COM_DEBUG_AVOID_START( COM_NO_FUNCTRACE | COM_NO_SKIPMEM );
    for( com_winId_t id = 1;  id < gWin.cnt;  id++ ) {
        if( gWin.inf[id].window ) {com_deleteWindow( id );}
    }
    endwin();
    cancelHash( &(gWin.inf[0]) );
    com_skipMemInfo( true );
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

