/*
 *****************************************************************************
 *
 * テストコード  by TOS
 *
 *   各員でテストコードを書くときは、本ファイルを直接編集はせず、
 *   必ず別ファイルにリネームコピーし、そちらの com_testCode()を修正すること。
 *   その後、本ファイル自体を削除して、ビルドすること。
 *   あるいは本ファイル内の com_testCode()のコメントアウトでも問題ない。
 *
 *   実行用ファイルの最初に特定のオプションを指定したら、
 *   com_testCode()を呼ぶようにすれば、デバッグ確認がしやすくなる(はず)。
 *
 *****************************************************************************
 */

#include "com_if.h"

#ifdef    USE_TESTFUNC
void com_testCode( int iArgc, char **iArgv )
{
    // この関数はあくまでサンプルコードなので踏襲しなくて良い。

    for( int i = 0;  i < iArgc;  i++ ) { com_printf( "%s\n", iArgv[i] ); }

    // 例えば強制的にデバッグモードをONにしてから試験開始
    // ただしここに来るまではデフォルト設定のままなので注意

    com_setDebugPrint( COM_DEBUG_ON );
    com_setWatchMemInfo( COM_DEBUG_ON );
    com_setWatchFileInfo( COM_DEBUG_ON );

    // 以降にテストコードを記述

    exit( EXIT_SUCCESS );
}
#endif // USE_TESTFUNC

