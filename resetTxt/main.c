/*
 * main() ひな形
 *   toscomを使う際の最も基本的なひな形となるコード。
 *   初期化の処理は、基本的にこのパターンにしたほうが良い。
 *   実行しても何もせず終了となる。
 */

#include "com_if.h"                 // toscom基本機能
#include "com_extra.h"
#include <stdint.h>

/* 独自処理があれば記述 ******************************************************/
static com_buf_t gPath;
static com_buf_t gOut;

static void freeData( void )
{
    com_resetBuffer( &gPath );
    com_resetBuffer( &gOut );
}

// 文字として "*" は使わないようにすること(aboutIf.txt内で行頭として使うため)
static char* gMark[] = { "+", "!", "@", "-" };

typedef enum {
    MARK_NEW = 0,    // 新規I/Fマーク (+)
    MARK_MOD,        // 変更I/Fマーク (!)
    MARK_CHG,        // 処理修正I/Fマーク (@)
    MARK_DEL,        // 削除I/Fマーク (-)
    MARK_MAX         // マーク最大数
} tmpMark_t;

typedef struct {
    FILE*  ofp;                   // 結果出力先ファイルポインタ
    long   lineCount;             // 対象の総ライン数
    long   rstCount[MARK_MAX];    // 各マークごとの数
    BOOL   startList;             // リスト開始フラグ
    intptr_t   seekPos;           // マークのあったインデント位置
} tmpData_t;

static void outResult( FILE *iFp, const char *iLine, const char *iPrefix )
{
    if( iFp == stdout ) {
        if( iLine ) { fprintf( iFp, "%s%s", iPrefix, iLine ); }
        else { fprintf( iFp, "%s\n", iPrefix ); }
    }
    else {
        // iPrefix は 1文字以上の文字列であることを想定する
        if( iLine ) { fprintf( iFp, "%s%s", iPrefix + 1, iLine ); }
    }
}

static void deleteResult( tmpData_t *ioUd, const char *iMark )
{
    outResult( ioUd->ofp, NULL, iMark );
}

static void noChangeResult( tmpData_t *ioUd, const char *iLine )
{
    outResult( ioUd->ofp, iLine, " " );
    ioUd->seekPos = 0;
}

static void seekTop( char *iLine, char **oTop, intptr_t *oPos )
{
    *oPos = -1;
    *oTop = com_topString( iLine, true );
    if( *oTop ) {
        intptr_t posLine = (intptr_t)iLine;
        intptr_t posTop  = (intptr_t)(*oTop);
        *oPos = posTop - posLine;
    }
}

static tmpMark_t checkTop( char iTop )
{
    for( long idx = MARK_NEW; idx < MARK_MAX; idx++ ) {
        if( iTop == *gMark[idx] ) { return idx; }
    }
    return MARK_MAX;
}

static void countItem(
        tmpData_t *ioUd, tmpMark_t iChecked, char *iTop, intptr_t iPos )
{
    (ioUd->rstCount[iChecked])++;
    ioUd->seekPos = iPos;
    if( iChecked != MARK_DEL ) {
        com_buf_t tmpHead;
        com_initBuffer( &tmpHead, 0, gMark[iChecked] );
        for( intptr_t tmp = 0; tmp <= iPos; tmp++ ) {
            com_addBuffer( &tmpHead, " " );
        }
        outResult( ioUd->ofp, iTop + 1, tmpHead.data );
        com_resetBuffer( &tmpHead );
    }
    else { deleteResult( ioUd, gMark[iChecked] ); }
}

static BOOL checkList( com_seekFileResult_t *iInf )
{
    tmpData_t* ud = iInf->userData;
    char* top = NULL;
    intptr_t topPos = 0;

    seekTop( iInf->line, &top, &topPos );
    if( top ) {
        tmpMark_t checked = checkTop( *top );
        if( checked < MARK_MAX ) { countItem( ud, checked, top, topPos ); }
        else {
            if( ud->seekPos && (topPos > (ud->seekPos + 1)) ) {
                deleteResult( ud, "*" );
            }
            else { noChangeResult( ud, iInf->line ); }
        }
    }
    else { noChangeResult( ud, iInf->line ); }

    return true;
}

static const char* gStartLine="===== 基本機能 (com_if.h) ====================================================\n";

static BOOL resetTextProc( com_seekFileResult_t *iInf )
{
    tmpData_t* ud = iInf->userData;
    (ud->lineCount)++;

    // リスト開始前だったら、マークのチェックはしない
    if( ud->startList ) { return checkList( iInf ); }
    // 開始行の内容は今後変わるかもしれない
    if( com_compareString(iInf->line, gStartLine, strlen(gStartLine), false) ) {
        ud->startList = true;
    }
    outResult( ud->ofp, iInf->line, " " );
    return true;
}

static void makeOutFile( void )
{
    tmpData_t userData = {0};

    if( !(userData.ofp = com_fopen( gOut.data, "w" )) ) {
        com_errorExit( COM_ERR_FILEDIRNG, "fail to open output file" );
    }

    BOOL result = com_seekFile( gPath.data, resetTextProc, &userData, NULL, 0 );
    com_fclose( userData.ofp );
    if( !result ) {
        com_errorExit( COM_ERR_FILEDIRNG, "file access failure" );
    }

    com_repeat( "-", 75, true );
    for( tmpMark_t idx = MARK_NEW; idx < MARK_MAX; idx++ ) {
        com_printf( " [%s] count: %ld\n", gMark[idx], userData.rstCount[idx] );
    }
}

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

