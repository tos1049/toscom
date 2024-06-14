/*
 * サンプルコミュニケーター  個別ソース (Ver2)
 *   TCPで複数の相手と文字のやり取りをするサンプルプログラム。
 *
 *   Ver1とは内容が異なる。
 *   cursesを使ったウィンドウ制御により、比較的直感的な UIにした。
 *   このためウィンドウ関連処理を個別処理として大きく追加している。
 *
 *   その他の差分については smpl_com.h を参照。
 */

#include "smpl_if.h"
#include "smpl_com.h"


///// ウィンドウ関連処理 /////////////////////////////////////////////////////

static com_winId_t  gIdLog = COM_NO_WIN;    // ログ用ウィンドウ
static com_winId_t  gIdSys = COM_NO_WIN;    // システムメッセージ用ウィンドウ
static com_winId_t  gIdPrm = COM_NO_WIN;    // 入力用ウィンドウ

typedef enum {
    SMPL_WIN_NONE = 0,     // スクロールなし
    SMPL_WIN_IDL  = 1,     // ハードウェアスクロール指定
    SMPL_WIN_SCRL = 2      // ソフトウェアスクロール指定
} SMPL_WINTYPE_t;

typedef void(*smpl_setWinType_t)( const com_cwin_t *iInf );

// ハードウェアスクロールの設定
static void setWinIdl( const com_cwin_t *iInf )
{
    idlok( iInf->window, true );
    scrollok( iInf->window, true );
}

// ソフトウェアスクロールの設定
static void setWinScrl( const com_cwin_t *iInf )
{
    com_winpos_t  pos, max;
    com_getWindowPos( iInf->id, &pos );
    com_getWindowMax( iInf->id, &max );
    wsetscrreg( iInf->window, pos.y + (int)(iInf->border),
                              pos.y + max.y - (int)(iInf->border) );
    scrollok( iInf->window, true );
}

// ウィンドウごとに呼び出す関数を設定
static smpl_setWinType_t  gWinFunc[] = { NULL, setWinIdl, setWinScrl };

static com_winId_t setupWin(
        com_winpos_t *iPos, com_winpos_t *iSize, BOOL iBorder,
        SMPL_WINTYPE_t iWinType )
{
    com_winId_t  id = com_createWindow( iPos, iSize, iBorder );
    if( !id ) {return 0;}

    com_activateWindow( id );
    const com_cwin_t*  inf = NULL;
    (void)com_getWindowInf( id, &inf );
    if( gWinFunc[iWinType] ) {(gWinFunc[iWinType])( inf );}
    return id;
}

// TABを押すと、通常入力-ワイド入力をトグル切り替え
static BOOL switchWideMode( com_winId_t iId )
{
    com_cancelInputWindow( iId );
    smpl_switchWideMode();
    return true;
}

// 入力内容保存 (試験運用)
static char  gClipboard[COM_LINEBUF_SIZE];

static BOOL copyText( com_winId_t iId )
{
    char*  text = com_getInputWindow( iId );
    if( !text ) {return false;}
    (void)com_strncpy( gClipboard, sizeof(gClipboard), text, strlen(text) );
    return false;
}

static BOOL pasteText( com_winId_t iId )
{
    com_setInputWindow( iId, gClipboard );
    return false;
}

static com_intrKey_t  gPrmIntrKey[] = {
    {     '\t', switchWideMode },
    { KEY_F(5), copyText },
    { KEY_F(6), pasteText },
    {        0, NULL }
};

#define SETUPWIN( NEWSIZE, ID, BORDER, SCROLL, NAME ) \
    pos.y += size.y; \
    size.y = (NEWSIZE); \
    if( !(ID = setupWin( &pos, &size, (BORDER), (SCROLL) )) ) { \
        com_errorExit( COM_ERR_WINDOWNG, "fail to %s window..", NAME ); \
    }

static void startWindow( void )
{
    com_winopt_t  winOpt = { true, true, false, NULL };
    com_winpos_t  winSize;
    com_readyWindow( &winOpt, &winSize );

    com_winpos_t  pos  = { 0, 0 };
    com_winpos_t  size = { winSize.x, 0 };
    SETUPWIN( winSize.y * 2 / 3, gIdLog, false, SMPL_WIN_IDL,  "log" );
    SETUPWIN(                 3, gIdSys,  true, SMPL_WIN_NONE, "sysmsg" );
    SETUPWIN( winSize.y - pos.y, gIdPrm, false, SMPL_WIN_SCRL, "prompt" );
    com_setIntrInputWindow( gIdPrm, gPrmIntrKey );
}
    
static void endWindow( void )
{
    com_finishWindow();
}

// 画面出力
static char  gMsgBuff[COM_DATABUF_SIZE];

static void dispLine( int iAttr, const char *iText, char *iLf )
{
    const char*  form = (iLf) ? "%s\n" : "%s";
    com_printw( gIdLog, iAttr, form, iText );
    com_refreshWindow();
    com_printf( form, iText );
}

#define CREATE_MSG \
    do { \
        va_list  ap; \
        va_start( ap, iFormat ); \
        vsnprintf( gMsgBuff, sizeof(gMsgBuff), iFormat, ap ); \
        va_end( ap ); \
    } while(0)


void smpl_printLog( int iAttr, const char *iFormat, ... )
{
    CREATE_MSG;
    char*  tmp = gMsgBuff;
    char*  lf = NULL;
    while( *tmp ) {
        if( (lf = strchr( tmp, '\n' )) ) {*lf = '\0';}
        dispLine( iAttr, tmp, lf );
        tmp += strlen(tmp);
        if( lf ) {tmp++;}
    }
}

static void dispSysMsg( const char *iSub, const char *iFormat, ... )
{
    CREATE_MSG;
    com_fillWindow( gIdSys, ' ' );
    com_printw( gIdSys, 0, gMsgBuff );
    com_mprintWindow( gIdSys, &(com_winpos_t){-1, 0}, 0, false, iSub );
    com_activateWindow( gIdPrm );
}


///// 共有個別関数 ///////////////////////////////////////////////////////////

void smpl_specStart( void ) {
    startWindow();
}

void smpl_specEnd( void ) {
    endWindow();
}

BOOL smpl_specStdin( com_getStdinCB_t iRecvFunc, com_selectId_t *oStdinId )
{
    COM_UNUSED( iRecvFunc );
    COM_UNUSED( oStdinId );
    return true;    // Ver2では何も登録せず無条件true
}

void smpl_specPrompt( const smpl_promptInf_t *iInf )
{
    char*  help = iInf->wideMode ? "CTRL+D -> INPUT END / TAB -> NORMAL" :
                                   ":h -> HELP / TAB -> WIDE";
    char* isWide = iInf->wideMode ? "WIDE" : "";
    if( iInf->dstId == COM_NO_SOCK ) {
        dispSysMsg( help, "%s[NO DESTINATION] %s", isWide, iInf->myHandle );
    }
    else {
        char*  handle = (iInf->broadcast | iInf->wideBroadcast) ?
                        NULL : iInf->dstHandle;
        dispSysMsg( help, "%s[%s] %s", isWide,
                    smpl_makeDstLabel(iInf->dstId, handle), iInf->myHandle );
    }
}

BOOL smpl_specWait( const smpl_promptInf_t *iInf )
{
    (void)com_watchEvent();    // データ受信を先にチェック

    char*  text = NULL;
    com_wininopt_t  opt = {
        iInf->wideMode ? COM_IN_MULTILINE : COM_IN_1LINE, 50, 0, true, true 
    };
    BOOL  result = com_inputWindow( gIdPrm, &text, &opt, &(com_winpos_t){0,0} );
    if( !result ) {return true;}
    return smpl_procStdin( text, strlen(text) + 1 );
}

void smpl_specDispMessage( const char *iMsg )
{
    smpl_printLog( A_BOLD, "%s\n", iMsg );
}

void smpl_specInvalidMessage( const char *iMsg )
{
    smpl_printLog( 0, "%s\n", iMsg );
}

void smpl_specRecvMessage(
        const char *iMsg, const char *iDst, size_t iSize, const char *iTime )
{
    smpl_specDispMessage( iMsg );
    smpl_printLog( 0, "<< %s (%zu byte received) at %s\n", iDst, iSize, iTime );
}

void smpl_specCommand( const char *iData )
{
    smpl_printLog( A_REVERSE, "%s\n", iData );
}

BOOL smpl_specWideInput( smpl_wideInf_t *ioInf )
{
    *(ioInf->widemode) = (*(ioInf->widemode) == false);
    if( *(ioInf->widemode) ) {
        if( ioInf->withBcCmd ) {*(ioInf->wideBroadcast) = true;}
    }
    else {*(ioInf->wideBroadcast) = false;}
    return true;
}

void smpl_specInitialize( void )
{
    // 現状特に処理無し
}

void smpl_specFinalize( void )
{
    // 現状特に処理無し
}

