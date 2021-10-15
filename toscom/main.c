/*
 * main() ひな形
 *   toscomを使う際の最も基本的なひな形となるコード。
 *   初期化の処理は、基本的にこのパターンにしたほうが良い。
 *   実行しても何もせず終了となる。
 */

#include "com_if.h"
#include "com_debug.h"

/* 独自処理があれば記述 ******************************************************/


/* 初期化処理 ****************************************************************/

static void initialize( int iArgc, char **iArgv )
{
    COM_INITIALIZE( iArgc, iArgv );
    // これ以降に他モジュールの初期化処理が続く想定
}

/* 起動オプション判定 *******************************************************/

static BOOL viewVersion( com_getOptInf_t *iOptInf )
{
    COM_UNUSED( iOptInf );
    // 特に何もせずに処理終了させる
    com_exit( COM_NO_ERROR );
    return true;
}

#ifdef    USE_TESTFUNC
static BOOL execTest( com_getOptInf_t *iOptInf )
{
    com_testCode( iOptInf->argc, iOptInf->argv );
    return true;
}
#endif // USE_TESTFUNC

static com_getOpt_t gOptions[] = {
    { 'v', "version", 0,           0, false, viewVersion },
#ifdef    USE_TESTFUNC
    {   0, "debug",   COM_OPT_ALL, 0, false, execTest },
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

    return EXIT_SUCCESS;
}

