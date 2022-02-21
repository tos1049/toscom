/*
 * main() ひな形
 *   toscomを使う際の最も基本的なひな形となるコード。
 *   初期化の処理は、基本的にこのパターンにしたほうが良い。
 *   実行しても何もせず終了となる。
 */

#include "com_if.h"                 // toscom基本機能
#include "com_extra.h"              // toscomエキストラ機能
#include "com_select.h"             // toscomセレクト機能
#include "com_window.h"             // toscomウィンドウ機能
#include "com_signal.h"             // toscomシグナル機能
#include "com_signalSet2.h"         // toscomシグナル機能セット2
#include "com_signalSet3.h"         // toscomシグナル機能セット3

/* 独自処理があれば記述 ******************************************************/


/* 初期化処理 ****************************************************************/

static void initialize( int iArgc, char **iArgv )
{
    COM_INITIALIZE( iArgc, iArgv );
    // これ以降に他モジュールの初期化処理が続く想定
}

/* 起動オプション判定 *******************************************************/
// 新しいオプションを追加したい時は、オプション処理関数を作成後、
// gOptions[]に定義を追加する。詳細は com_if.hの com_getOption()の説明を参照。

static BOOL viewVersion( com_getOptInf_t *iOptInf )
{
    COM_UNUSED( iOptInf );
    // 特に何もせずに処理終了させる
    com_exit( COM_NO_ERROR );
    return true;
}

static BOOL clearLogs( com_getOptInf_t *iOptInf )
{
    COM_UNUSED( iOptInf );
    com_system( "rm -fr .%s.log_*", com_getAplName() );
    com_printf( "<<< cleared all past logs >>>\n" );
    com_exit( COM_NO_ERROR );
    return true;
}

#ifdef    USE_TESTFUNC    // テスト関数使用のコンパイルオプション
static BOOL execTest( com_getOptInf_t *iOptInf )
{
    com_testCode( iOptInf->argc, iOptInf->argv );
    return true;
}
#endif // USE_TESTFUNC

static com_getOpt_t gOptions[] = {
    { 'v', "version",  0,           0, false, viewVersion },
    { 'c', "clearlog", 0,           0, false, clearLogs },
#ifdef    USE_TESTFUNC
    {   0, "debug",    COM_OPT_ALL, 0, false, execTest },
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

