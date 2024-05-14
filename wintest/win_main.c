/*
 * win_main.c   ウィンドウテスト用サンプルコード
 *   ウィンドウ機能を単純に使用する例としてのコードとなる。
 *   ルートウィンドウ上に５つのウィンドウを作成する。
 *   ・カーソルキーでカレントウィンドウを移動。
 *   ・hjklでカレントウィンドウ上のカーソルを移動。
 *   ・スペースで現在のカーソル位置に文字入力開始。
 *     Enterで文字入力完了、ESCで文字入力キャンセル。
 *     (com_inputWindow()で設定されているキー操作）
 *   ・TABでカレントウィンドウ切替。
 *   　SHIFT+TABで逆方向のカレントウィンドウ切替。
 *   ・CTRL+C でプログラム終了。
 *   ・左上に、その時押したキーのキーコードを表示する。
 */

#include "win_if.h"



/* ウィンドウ管理データ *****************************************************/

// 生成する子ウィンドウ数
enum { WIN_MAX = 5 };

// ルートウィンドウID (0固定)
#define ROOTWINID  0

// カレントウィンドウID
static long gCurWin = ROOTWINID;



/* キー操作とそれに対応する処理 *********************************************/

static void switchWindow( com_winId_t iId )
{
    gCurWin = (iId + 1) % (WIN_MAX + 1);
    // ウィンドウのアクティベートは loopMain()のループ内で実施
}

static void revertWindow( com_winId_t iId )
{
    gCurWin = iId - 1;
    if( gCurWin < 0 ) { gCurWin = WIN_MAX; }
    // ウィンドウのアクティベートは loopMain()のループ内で実施
}

static void moveCursor( com_winId_t iId, com_winpos_t *iMove )
{
    com_winpos_t  pos;
    com_getWindowCur( iId, &pos );

    pos.x = pos.x + iMove->x;
    if( pos.x < 0 ) { pos.x = 0; }
    
    pos.y = pos.y + iMove->y;
    if( pos.y < 0 ) { pos.y = 0; }
    
    com_moveCursor( iId, &pos );
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
    com_getWindowPos( iId, &pos );

    pos.x = pos.x + iMove->x;
    pos.y = pos.y + iMove->y;
    com_moveWindow( iId, &pos );
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

    com_getWindowCur( iId, &pos );
    while ( !com_inputWindow( iId, &string, &opt, &pos ) ) {}
}

// 全てのウィンドウ(ID=0も含む)で受け付けるキー操作
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

// 移動可能なウィンドウ(ID=0以外)でのみ受け付けるキー操作
static com_keymap_t  gWinKeymap[] = {
    { 0x41,         moveUpWindow },     // カーソル上  ウィンドウを上に移動
    { 0x42,         moveDownWindow },   // カーソル下  ウィンドウを下に移動
    { 0x43,         moveRightWindow },  // カーソル右  ウィンドウを右に移動
    { 0x44,         moveLeftWindow },   // カーソル左  ウィンドウを左に移動
    {   0, NULL }
};



/* ウィンドウ作成と終了処理 *************************************************/

static com_winId_t setupWin(
        com_winpos_t *iPos, com_winpos_t *iSize, BOOL iBorder )
{
    // 子ウィンドウのIDは 1から順にインクリメントという想定
    com_winId_t id = com_createWindow( iPos, iSize, iBorder );
    if( id == COM_NO_WIN ) { return 0; }

    com_mprintWindow( id, &(com_winpos_t){0,10}, A_REVERSE, true, "ID%ld", id );
    if( !com_setWindowKeymap( id, gWinKeymap ) ) { return 0; }
    if( !com_mixWindowKeymap( id, 0 ) ) { return 0; }
    if( !com_activateWindow( id ) ) { return 0; }
    gCurWin = id;
    return id;
}

// ループフラグ (trueの間はループ継続)
static BOOL gLoop = true;

#define ABORT  gLoop=false;return;

static void startWindow( void )
{
    // ルートウィンドウ作成
    com_winopt_t  winOpt = { true, true, false, NULL };
    com_winpos_t  winSize;
    if( !com_readyWindow( &winOpt, &winSize ) ) { ABORT; }
    if( !com_setWindowKeymap( ROOTWINID, gBaseKeymap ) ) { ABORT; }
    com_mprintWindow( ROOTWINID, &(com_winpos_t){0,winSize.y-1},
                      A_REVERSE, true, "BOTTOM" );
    com_mprintWindow( ROOTWINID, &(com_winpos_t){65,winSize.y-1},
                      A_UNDERLINE, true, "CTRL+C to quit" );

    // 子ウィンドウ作成
    com_winpos_t  winPos  = { 10,  0 };
    winSize = (com_winpos_t){ 40, 10 };
    for( int id = 0; id < WIN_MAX; id++ ) {
        if( !setupWin( &winPos, &winSize, true ) ) { ABORT; }
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

// シグナルハンドラ設定データ (エキストラ機能を使用)
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
        com_activateWindow( gCurWin );
        com_checkWindowKey( gCurWin, 50 );
        
        // 押したキーのキーコードをルートウィンドウ左上に表示
        int lastKey = com_getLastKey();
        if( lastKey > 0 ) {
            com_mprintWindow(
                    0, &(com_winpos_t){0,0}, A_BOLD, true, "0x%02x", lastKey );
        }
    }
}

static void finishWindow( void )
{
    com_finishWindow();
}



/* 外部公開関数 *************************************************************/

void win_main( void )
{
    startWindow();
    loopMain();
    finishWindow();
}

