/*
 * サンプルコミュニケータ サンプル2個別ソース
 *   TCPで複数の相手と文字のやり取りをするサンプルプログラム。
 *
 *   cursesを使ったウィンドウ制御により、比較的直感的なUIにした。
 *   このためウィンドウ関連処理を個別処理として大きく追加している。
 *
 *   その他の差分については smpl_com.h を参照。
 */

#include "smpl_if.h"
#include "smpl_com.h"


///// ウィンドウ関連処理 /////////////////////////////////////////////////////

void smpl_printLog( int iAttr, const char *iFormat, ... )
{
    // 今後追加予定
}


///// 共有個別関数 ///////////////////////////////////////////////////////////

void smpl_specStart( void ) {
    // 今後追加予定
}

void smpl_specEnd( void ) {
    // 今後追加予定
}

BOOL smpl_specStdin( com_getStdinCB_t iRecvFunc, com_selectId_t *oStdinId )
{
    // 今後追加予定
}

void smpl_specPrompt( const smpl_promptInf_t *iInf )
{
    // 今後追加予定
}

BOOL smpl_specWait( const smpl_promptInf_t *iInf )
{
    // 今後追加予定
}

void smpl_specDispMessage( const char *iMsg )
{
    // 今後追加予定
}

void smpl_specInvalidMessage( const char *iMsg )
{
    // 今後追加予定
}

void smpl_specRecvMessage(
        const char *iMsg, const char *iDst, ssize_t iSize, const char *iTime )
{
    // 今後追加予定
}

void smpl_specCommand( const char *iMsg )
{
    // 今後追加予定
}

BOOL smpl_specWideInput( smpl_wideInf_t *ioInf )
{
    // 今後追加予定
}

void smpl_specInitialize( void )
{
    // 今後追加予定
}

void smpl_specFinalize( void )
{
    // 今後追加予定
}

