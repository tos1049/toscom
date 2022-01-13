/*
 * main() ひな形
 *   toscomを使う際の最も基本的なひな形となるコード。
 *   初期化の処理は、基本的にこのパターンにしたほうが良い。
 *   実行しても何もせず終了となる。
 */

#include "smpl_if.h"      // toscomヘッダも、このファイルでインクルード済み


/* 独自処理があれば記述 ******************************************************/


/* 初期化処理 ****************************************************************/

static void initialize( int iArgc, char **iArgv )
{
    COM_INITIALIZE( iArgc, iArgv );
    // これ以降に他モジュールの初期化処理が続く想定
    smpl_initialize();
}


/* メイン関数 ***************************************************************/

int main( int iArgc, char **iArgv )
{
    initialize( iArgc, iArgv );
    smpl_communicator( iArgc, iArgv );  // 起動オプションチェックもこちらで
    return EXIT_SUCCESS;
}

