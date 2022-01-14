/*
 * main() ひな形
 *   toscomを使う際の最も基本的なひな形となるコード。
 *   初期化の処理は、基本的にこのパターンにしたほうが良い。
 *   実行しても何もせず終了となる。
 */

#include "com_if.h"                 // toscom基本機能
#include "com_extra.h"

/* 独自処理があれば記述 ******************************************************/
static com_buf_t gPath;
static com_buf_t gOut;

static void freeData( void )
{
    com_resetBuffer( &gPath );
    com_resetBuffer( &gOut );
}

typedef struct {
    FILE*  ofp;
    long   lineCount;
    long   newCount;
    long   modCount;
    long   chgCount;
    long   delCount;
    BOOL   startList;
    BOOL   foundMark;
    long   seekPos;
} tmpData_t;

static char *gMark[] = { "+", "!", "@", "-" };

typedef enum {
    MARK_NEW = 0,    // 新規I/Fマーク (+)
    MARK_MOD,        // 変更I/Fマーク (!)
    MARK_CHG,        // 処理修正I/Fマーク (@)
    MARK_DEL,        // 削除I/Fマーク (-)
    MARK_MAX         // マーク最大数
} tmpMark_t;


static void resetText( int iArgc, char **iArgv )
{
    if( !com_initBuffer( &gPath, 0, "./aboutIf.txt" ) ) {
        com_exit( COM_ERR_NOMEMORY );
    }
    if( iArgc > 1 ) {
        if( !com_setBuffer( &gPath, iArgv[1] ) ) {
            com_exit(COM_ERR_NOMEMORY);
        }
    }
    if( !com_checkExistFile( gPath.data ) ) {
        com_errorExit( COM_ERR_FILEDIRNG, "%s not found", gPath.data );
    }
    if( !com_initBuffer( &gOut, 0, "%s.out", gPath.data ) ) {
        com_exit( COM_ERR_NOMEMORY );
    }
    makeOutFile();
}

static void finalize( void )
{
    freeData();
}

/* 初期化処理 ****************************************************************/

static void initialize( int iArgc, char **iArgv )
{
    COM_INITIALIZE( iArgc, iArgv );
    // これ以降に他モジュールの初期化処理が続く想定
    atexit( finalize );
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

#ifdef    USE_TESTFUNC    // テスト関数使用のコンパイルオプション
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
    resetText( iArgc, iArgv );

    return EXIT_SUCCESS;
}

