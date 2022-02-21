/*
 * main() ひな形
 *   toscomを使う際の最も基本的なひな形となるコード。
 *   初期化の処理は、基本的にこのパターンにしたほうが良い。
 *   実行しても何もせず終了となる。
 */

#include "anlz_if.h"

/* 独自処理があれば記述 ******************************************************/


/* 初期化処理 ****************************************************************/

static void initialize( int iArgc, char **iArgv )
{
    COM_INITIALIZE( iArgc, iArgv );
    // これ以降に他モジュールの初期化処理が続く想定
    anlz_initialize();
}

/* メイン関数 ***************************************************************/

int main( int iArgc, char **iArgv )
{
    initialize( iArgc, iArgv );

    /* 以後、必要な処理を記述 */
    anlz_start( iArgc, iArgv );

    return EXIT_SUCCESS;
}

