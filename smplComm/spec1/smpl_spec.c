/*
 *****************************************************************************
 *
 * サンプルコミュニケーター  個別ソース (Ver1)
 *   TCPで複数の相手と文字のやり取りをするサンプルプログラム。
 *
 *   Ver2とは内容が異なる。
 *   com_waitEvent()による受信待受をするパターンとなる。
 *   UIは Ver2の方が比較的直感的だが、コードはこちらの方が単純。
 *   標準の画面出力を使っているため、画面構成もシンプルになる。
 *
 *   その他の差分については smpl_com.h を参照。

 *****************************************************************************
 */

#include "smpl_if.h"
#include "smpl_com.h"


///// 共有個別関数 ///////////////////////////////////////////////////////////

void smpl_specStart( void )
{
    // 現状処理なし
}

void smpl_specEnd( void )
{
    // 現状処理なし
}

BOOL smpl_specStdin( com_getStdinCB_t iRecvFunc, com_selectId_t *oStdinId )
{
    *oStdinId = com_registerStdin( iRecvFunc );
    if( *oStdinId == COM_NO_SOCK ) { return false; }
    return true;
}

void smpl_specPrompt( const smpl_promptInf_t *iInf )
{
    if( !iInf->usePrompt ) { return; }
    com_printLf();
    if( iInf->dstId == COM_NO_SOCK ) {
        com_printf( "[NO DESTINATION] %s\n", iInf->myHandle );
    }
    else {
        char* handle = iInf->broadcast ? NULL : iInf->dstHandle;
        com_printf( "[%s] %s\n", smpl_makeDstLabel( iInf->dstId, handle ),
                                 iInf->myHandle );
    }
}

BOOL smpl_specWait( const smpl_promptInf_t *iInf )
{
    COM_UNUSED( iInf );
    return com_waitEvent();  // toscomの受信処理に全て委ねる
}

void smpl_specDispMessage( const char *iMsg )
{
    COM_UNUSED( iMsg );
    // ログ画面と入力画面の区別はないので、改めて表示は不要
}

void smpl_specInvalidMessage( const char *iMsg )
{
    COM_UNUSED( iMsg );
    // ログ画面と入力画面の区別はないので、改めて表示は不要
}

void smpl_specRecvMessage(
        const char *iMsg, const char *iDst, ssize_t iSize, const char *iTime )
{
    com_printf( "\n%s\n << %s (%zd byte received at %s\n",
                iMsg, iDst, iSize, iTime );
}

void smpl_specCommand( const char *iMsg )
{
    COM_UNUSED( iMsg );
    // ログ画面と入力画面の区別はないので、改めて表示は不要
}

static char gWideBuff[COM_DATABUF_SIZE];

BOOL smpl_specWideInput( smpl_wideInf_t *ioInf )
{
    COM_CLEAR_BUF( gWideBuff );
    size_t result = com_inputMutliLine( gWideBuff, sizeof(gWideBuff),
                                        " >> input text (finish: CTRL+D)\n" );
    com_printLf();
    if( result ) {
        BOOL isBroadcast = false;
        BOOL curBroadcast = *(ioInf->broadcast);
        if( ioInf->withBcCmd ) { isBroadcast = true; }
        if( isBroadcast ) { *(ioInf->broadcast) = true; }
        smpl_sendStdin( gWideBuff, strlen(gWideBuff) + 1 );
        if( isBroadcast ) { *(ioInf->broadcast) = curBroadcast; }
    }
    return true;
}

void smpl_specInitialize( void )
{
    // 現状処理なし
}

void smpl_specFinalize( void )
{
    // 現状処理なし
}

