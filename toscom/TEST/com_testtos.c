/*
 *****************************************************************************
 *
 * toscomテストコード  by TOS
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

#include <unistd.h>
#include "com_if.h"
#include "com_debug.h"     // デバッグ機能確認のためインクルード (通常は不要)
#include "com_extra.h"
#include "com_select.h"
#include "com_window.h"

#ifdef    USE_TESTFUNC

static void startFunc( const char *iFunc )
{
    com_printTag( "*", 79, COM_PTAG_CENTER, "%s", iFunc );
}
 


///// test_realloct() ////////////////////////////////////////////////////////

static void viewTable( int *iTable, long iCount )
{
    com_printf( "\ntable count = %ld\n", iCount );
    for( long i = 0;  i < iCount;  i++ ) {
        com_printf( "table[%ld] = %d\n", i, iTable[i] );
    }
}

void test_realloct( void )
{
    startFunc( __func__ );
    int* table = NULL;
    long count = 0;

    com_assertTrue( "com_realloc() <TEST+3>",
            com_realloct( &table, sizeof(int), &count, 3, "TEST+3" ) );
    viewTable( table, count );
    table[0] = 3;  table[1] = 2;  table[2] = 1;
    viewTable( table, count );

    com_assertTrue( "com_realloc() <TEST-2>",
            com_realloct( &table, sizeof(int), &count, -2, "TEST-2" ) );
    viewTable( table, count );

    com_assertTrue( "com_realloc() <TEST+5>",
            com_realloct( &table, sizeof(int), &count, 5, "TEST+5" ) );
    viewTable( table, count );

    int* newTable = com_reallocAddr( &table, sizeof(int), COM_TABLEEND,
                                     &count, 3, "TEST ADD" );
    com_assertNotNull( "com_reallocAddr() <TEST ADD>", newTable );
    newTable[1] = 100;
    viewTable( table, count );
    table[3] = 3;  table[4] = 4;

    newTable = com_reallocAddr(&table, sizeof(int), 4, &count, 4, "TEST ADD2" );
    com_assertNotNull( "com_reallocAddr() <TEST ADD2>", newTable );
    newTable[2] = 999;
    viewTable( table, count );

    newTable = com_reallocAddr(&table, sizeof(int), 4, &count, -4, "TEST DEL" );
    com_assertNotNull( "com_reallocAddr() <TEST DEL>", newTable );
    newTable[2] = 999;
    viewTable( table, count );

    newTable = com_reallocAddr(&table, sizeof(int), 7, &count, -4, "TEST DEL2");
    com_assertNotNull( "com_reallocAddr() <TEST DEL2>", newTable );
    newTable[2] = 999;
    viewTable( table, count );
    com_free( table );
    com_assertNull( "table", table );
}

///// test_getOpt() //////////////////////////////////////////////////////////

static BOOL optFuncI( com_getOptInf_t *iOptInf )
{
    com_printf( "init = ON (%ld/%s)\n", iOptInf->argc, iOptInf->argv[0] );
    return true;
}

static BOOL optFuncX( com_getOptInf_t *iOptInf )
{
    COM_UNUSED( iOptInf );
    static BOOL debug = false;
    debug = (debug == false);
    com_printf( "debug = %ld\n", debug );
    return true;
}

static BOOL optFuncZ( com_getOptInf_t *iOptInf )
{
    COM_UNUSED( iOptInf );
    com_printf( "z option accepted\n" );
    return true;
}

// i が２つあるためエラーになる
static com_getOpt_t gOptList1[] = {
    { 'i', "init",   0, 0, false, optFuncI },
    { 'i', "ignite", 0, 0, false, optFuncI },
    COM_OPTLIST_END
};

// init が２つあるためエラーになる
static com_getOpt_t gOptList2[] = {
    { 'i', "init", 0, 0, false, optFuncI },
    { 'g', "init", 0, 0, false, optFuncI },
    COM_OPTLIST_END
};

// エラーにならず logfile0 logfile1 logfile2 が未処理オプションとして残る
static com_getOpt_t gOptList3[] = {
    { 'i', NULL,    1, 0, true,  optFuncI },
    { 'x', "debug", 0, 0, false, optFuncX },
    { 'z', NULL,    0, 0, false, optFuncZ },
    COM_OPTLIST_END
};

// 必須オプション -M がないというエラーになる
static com_getOpt_t gOptList4[] = {
    { 'i', NULL,    1, 0, false, optFuncI },
    { 'x', "debug", 0, 0, false, optFuncX },
    { 'z', NULL,    0, 0, false, optFuncZ },
    { 'M', NULL,    0, 0, true,  optFuncX },
    COM_OPTLIST_END
};

// -z はマルチオプションで使えないというエラーになる
static com_getOpt_t gOptList5[] = {
    { 'i', NULL,    1, 0, true,  optFuncI },
    { 'x', "debug", 0, 0, false, optFuncX },
    { 'z', NULL,    1, 0, false, optFuncZ },
    COM_OPTLIST_END
};

static long   gTestArgvCnt = 8;
static char*  gTestArgv[] = {
    "test", "logfile0", "-i", "test1", "-xz", "--debug", "logfile1", "logfile2"
};

static void viewRestList( long *ioCnt, char ***ioList )
{
    for( long i = 0;  i < *ioCnt;  i++ ) {
        com_printf( "(%ld) %s\n", i, (*ioList)[i] );
    }
    com_printf( "---get option OK\n" );
}

#define GETOPTION( OPTLIST ) \
    com_getOption( (int)gTestArgvCnt, gTestArgv, OPTLIST, &cnt, &list, NULL )

void test_getOpt( void )
{
    startFunc( __func__ );
    long cnt = 0;
    char** list = NULL;
    for( long i = 0;  i < gTestArgvCnt;  i++ ) {
        com_printf( "%s ", gTestArgv[i] );
    }
    com_printLf();
    com_printLf();

    char errMsg[] = "---error is correct result\n";
    com_assertFalse( "gOptList1", GETOPTION( gOptList1 ) );
    com_printf( "%s", errMsg );
    com_assertFalse( "gOptList2", GETOPTION( gOptList2 ) );
    com_printf( "%s", errMsg );
    com_assertTrue(  "gOptList3", GETOPTION( gOptList3 ) );
    com_printf("** no error **\n");
    viewRestList( &cnt, &list );  // これだけはエラーにならず残り引数取得可能
    com_assertFalse( "gOptList4", GETOPTION( gOptList4 ) );
    com_printf( "%s", errMsg );
    com_assertFalse( "gOptList5", GETOPTION( gOptList5 ) );
    com_printf( "%s", errMsg );
}

///// test_strdup() //////////////////////////////////////////////////////////

void test_strdup( void )
{
    startFunc( __func__ );
    char* str1 = com_strdup(  "1234567", NULL );
    char* str2 = com_strndup( "1234567", 3, NULL );
    char* str3 = com_strndup( "1234567", 10, NULL );

    com_assertString( "str1", "1234567", str1 );
    com_assertString( "str2", "123", str2 );
    com_assertString( "str3", "1234567", str3 );

    com_free( str1 );
    com_free( str2 );
    com_free( str3 );
}

///// test_convertString() ///////////////////////////////////////////////////

void test_convertString( void )
{
    startFunc( __func__ );
    char text[] = "aBcDeFgHiJ";

    com_convertUpper( text );
    com_assertString( "com_convertUpper()", "ABCDEFGHIJ", text );
    com_convertLower( text );
    com_assertString( "com_convertLower()", "abcdefghij", text );

    ulong test1 = 11223445566;
    COM_ULTOSTR( str1, test1 );
    com_assertString( "COM_ULTOSTR()", "11223445566", str1 );

    uchar oct[] = { 0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x70 };
    char* result = NULL;
    (void)com_octtostr( &result, 0, oct, COM_ELMCNT(oct), false );
    com_assertString( "com_octtostr()", "1a2b3c4d5e6f70", result );
    com_free( result );

    char  result2[15];
    char* tmp = result2;
    (void)com_octtostr( &tmp, sizeof(result2), oct, COM_ELMCNT(oct), true );
    com_assertString( "com_octtostr()", "1A2B3C4D5E6F70", result2 );
}

///// test_searchString() ////////////////////////////////////////////////////

static void searchString( const char *iTarget, long *ioIndex, char **oResult )
{
    static char text[] = "ABCD BCDE CDEF DEFG EFGH";
    com_printf( "%s\n", text );
    // 検索対象は最初の15文字
    *oResult = com_searchString(text, iTarget, ioIndex, 15, false);
    com_printf( " %s\n", *oResult );
    if( ioIndex ) { com_printf( "  count = %ld\n", *ioIndex ); }
}

void test_searchString( void )
{
    startFunc( __func__ );
    char* result = NULL;
    searchString( "BC", NULL, &result );
    com_assertString( "BC,NULL", "BCD BCDE CDEF DEFG EFGH", result );
    long idx = 0;
    searchString( "BC", &idx, &result );
    com_assertString( "BC,0", "BCD BCDE CDEF DEFG EFGH", result );
    com_assertEquals( "BC,0", 2, idx );
    idx = 2;
    searchString( "CD", &idx, &result );
    com_assertString( "CD,2", "CDE CDEF DEFG EFGH", result );
    com_assertEquals( "CD,2", 3, idx );
    idx = 5;
    searchString( "EF", &idx, &result );
    com_assertNull( "EF,5", result );
    com_assertEquals( "EF,5", 1, idx );
}

///// test_replaceString() ///////////////////////////////////////////////////

static void replaceString( const char *iReplacing, const char *iReplaced,
                           long iIndex, char **oResult, size_t *oResultSize )
{
    char source[] = "ABD TESTTEST TEST ABC";
    long count = 0;

    com_replaceCond_t cond = { iReplacing, iReplaced, iIndex };
    count = com_replaceString( oResult, oResultSize, source, &cond );
    if( count == COM_REPLACE_NG ) {
        com_printf( "replace NG...\n" );
        return;
    }
    com_printf( " replace : %ld\n", count );
    com_printf( "   (%zu) %s\n-> (%zu) %s\n",
                strlen(source), source, *oResultSize, *oResult );
}

void test_replaceString( void )
{
    startFunc( __func__ );
    char* result = NULL;
    size_t resultSize = 0;

    replaceString( "TEST", "TOS", COM_REPLACE_ALL, &result, &resultSize );
    com_assertString(  "TEST -> TOS, ALL", "ABD TOSTOS TOS ABC", result );
    com_assertEqualsU("TEST -> TOS, ALL", 18, resultSize );

    replaceString( "TEST", "CHECKOK!", 1, &result, &resultSize );
    com_assertString(  "TEST -> CHECKOK!, 1", "ABD CHECKOK!TEST TEST ABC", result );
    com_assertEqualsU("TEST -> CHECKOK!, 1", 25, resultSize );

    replaceString( "ABD",  "ABC", COM_REPLACE_ALL, &result, &resultSize );
    com_assertString(  "ABD -> ABC, ALL", "ABC TESTTEST TEST ABC", result );
    com_assertEqualsU("ABD -> ABC, ALL", 21, resultSize );
}

///// test_seekText() ////////////////////////////////////////////////////////

static char* gAssertText1[] = {
    "testLine"
};

static char* gAssertText2[] = {
    "test1",
    "",
    "test2"
};

static char* gAssertText3[] = {
    "",
    "<<TEST>>",
    "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111122222222222222222222",
    "222222222222222222222222222222222222222222222222222222222222222222222222223333333333333333333333333333"
};

typedef struct {
    char** assertLine;
    long   linecnt;
} assertData_t;

static BOOL dispSeekTextLine( com_seekFileResult_t *iInf )
{
    assertData_t* data = iInf->userData;
    com_printf( ">>%s\n", iInf->line );
    com_assertString( "iInf->line", data->assertLine[data->linecnt], iInf->line );
    (data->linecnt)++;
    return true;
}

static void testTextLine( const char *iText, char **iAssert )
{
    com_repeat( "-", 40, true );
    char buf[COM_LINEBUF_SIZE];
    assertData_t  data = { iAssert, 0 };
    com_assertTrue( "com_seekTextLine()",
            com_seekTextLine( iText, strlen(iText), false, dispSeekTextLine,
                              &data, buf, sizeof(buf) ) );
}

void test_seekText( void )
{
    startFunc( __func__ );
    testTextLine( "testLine", gAssertText1 );
    testTextLine( "test1\n\ntest2\n", gAssertText2 );
    // 長い行はテキスト走査関数のバッファサイズで区切られる
    testTextLine( "\n<<TEST>>\n0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111122222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222223333333333333333333333333333", gAssertText3 );
}

///// test_seekFunc() ////////////////////////////////////////////////////////

static char* gAssertMakefile[] = {
    "\n",
    "COMMAND := test\n",
    "VERSION := 0.0\n",
    "\n"
};

static void dispSeekLine( int *ioTest, char *iLine )
{
    com_printf( "%03d> %s", *ioTest, iLine );
    if( *ioTest < 104 ) {
        // 内容の変動があり得るので、最初の4行のみチェックする
        com_assertString( "iLine", gAssertMakefile[*ioTest - 100], iLine );
    }
    if( *ioTest == 500 ) {
        // 内容は変動があり得るので、最初の com.o のみチェックする
        char assertcom[128] = {0};
        int result = sscanf( iLine, "%*s %*s %*s %*s %*s %*s %*s %*s %s",
                             assertcom );
        com_assertEquals( "result", 1, result );
        com_assertString( "file",   "com.o", assertcom );
    }
    (*ioTest)++;
}

static BOOL seekMakefile( com_seekFileResult_t *iInf )
{
    int* test = iInf->userData;
    // コメント行は出力しない
    if( iInf->line[0] != '#' ) { dispSeekLine( test, iInf->line ); }
    return true;
}

static BOOL seekObjFile( com_seekFileResult_t *iInf )
{
    int* test = iInf->userData;
    if( strstr( iInf->line, ".o" ) ) { dispSeekLine( test, iInf->line ); }
    return true;
}

void test_seekFile( void )
{
    startFunc( __func__ );
    int test;

    test = 100;
    if( COM_UNLIKELY(
        !com_seekFile("./makefile", seekMakefile, &test, NULL, 0)) )
    {
        com_printf( "something wrong at com_seekFile...\n" );
    }
    test = 500;
    if( COM_UNLIKELY(
        !com_pipeCommand("ls -l", seekObjFile, &test, NULL, 0)) )
    {
        com_printf( "something wrong at com_seekFile...\n" );
    }
}

///// test_seekBin() /////////////////////////////////////////////////////////

static BOOL gCheckedTopLine = false;
static uchar  gAssertBin[] = {
    0x23, 0x0a, 0x23, 0x20, 0x6d, 0x61, 0x6b, 0x65,
    0x66, 0x69, 0x6c, 0x65, 0x20, 0xe3, 0x82, 0xb5
};

static size_t readBinary( com_seekBinResult_t *iInf )
{
    com_printBin_t flags = { .prefix = "> ", .seq = "", .seqAscii = " : " };
    com_printBinary( iInf->data, iInf->length, &flags );
    if( !gCheckedTopLine ) {
        // 内容の変動があり得るので、最初の 16octのみチェックするt
        gCheckedTopLine = true;
        com_assertEqualsU( "iInf->length", 16, iInf->length );
        for( size_t i = 0;  i < iInf->length;  i++ ) {
            com_assertEqualsU( "iInf->data[]", gAssertBin[i], iInf->data[i] );
        }
    }
    return 16;
}

void test_seekBin( void )
{
    startFunc( __func__ );
    uchar buf[16];
    com_seekBinary( "./makefile", 16, readBinary, NULL, buf, sizeof(buf) );
}

///// test_removeDir() ///////////////////////////////////////////////////////

void test_removeDir( void )
{
    startFunc( __func__ );
    // 本当にファイルを削除するので注意すること
    char target[] = "./removeTest2341";
    long fileCount, dirCount;
    long count = 0;
    
    com_assertTrue( "com_makeDir()",
            com_makeDir( com_getString("%s/tmp", target) ) );

    com_system( "cp ../*.c %s", target );
    com_system( "ls -a %s", target );

    com_assertTrue( "com_countFiles()",
            com_countFiles( &fileCount, &dirCount, NULL, target, true) );

    com_printf( "remove %s\n", target );
    com_printf( " count result : %ld + %ld = %ld\n",
                fileCount, dirCount, fileCount + dirCount );
    count = com_removeDir( target );
    com_printf( " delete result : %ld\n", count );
    com_assertEquals( "count", fileCount + dirCount + 1, count );
}

///// test_seekDir() /////////////////////////////////////////////////////////

// 実在するディレクトリを指定すること
static char gSeekPath[COM_TEXTBUF_SIZE] = "..";

static BOOL setPath( int iArgc, char **iArgv )
{
    if( iArgc > 2 ) {
        if( COM_UNLIKELY( !com_strcpy( gSeekPath, iArgv[2] ) ) ) {
            com_printf( "too long path(%zu)\n", strlen(iArgv[2]) );
            return false;
        }
    }
    com_printf( "(inspected: %s)\n", gSeekPath );
    return true;
}

void test_seekDir( int iArgc, char **iArgv )
{
    startFunc( __func__ );
    long fileCount = 0;
    long dirCount = 0;
    off_t totalSize = 0;
    FILE* fp = com_fopen( "./makefile", "r" );  // ファイルオープン動作確認

    if( COM_UNLIKELY( !setPath( iArgc, iArgv )) ) {
        com_fclose( fp );
        return;
    }
    com_assertTrue( "com_countFiles()",
            com_countFiles(&fileCount, &dirCount, &totalSize, gSeekPath, true) );
    com_printf( "file count = %ld\n", fileCount );
    // 変動の可能性があるので 0ではないことのみチェック
    com_assertNotEquals( "fileCount", 0, fileCount );
    com_printf( " dir count = %ld\n", dirCount );
    // 変動の可能性があるので 0ではないことのみチェック
    com_assertNotEquals( "dirCount", 0, dirCount );
    com_printf( "total size = %lu\n", totalSize );
    // 変動の可能性があるので 0ではないことのみチェック
    com_assertNotEqualsU( "totalSize", 0, (ulong)totalSize );

    int closeErr = com_fclose( fp );  // ファイルクローズ動作確認
    com_printf( "fclose errno = %d\n", closeErr );
    com_assertEquals( "closeErr", 0, closeErr );
}

///// test_seekDir2() ////////////////////////////////////////////////////////

static int checkCSource( const struct dirent *iEntry )
{
    const char* name = iEntry->d_name;
    char ext[16];
    if( !com_getFileExt( ext, sizeof(ext), name ) ) {return 0;}
    if( com_compareString( ext, "c", 0, false ) ) {return 1;}
    return 0;
}

static BOOL dispCName( const com_seekDirResult_t *iInf )
{
    BOOL*  found = iInf->userData;
    *found = true;
    com_printf( "found C source [%s]\n", iInf->entry->d_name );
    return true;
}

void test_seekDir2( void )
{
    startFunc( __func__ );
    BOOL  foundCsource = false;
    com_assertTrue( "com_seekDir2()",
            com_seekDir2( "..", checkCSource, COM_SEEK_FILE, dispCName,
                          &foundCsource) );
    com_assertTrue( "foundCsource", foundCsource );
}

///// test_scanDir() /////////////////////////////////////////////////////////

void test_scanDir( int iArgc, char **iArgv )
{
    startFunc( __func__ );
    int count = 0;
    struct dirent** list = NULL;
    com_buf_t path;

    if( COM_UNLIKELY( !setPath( iArgc, iArgv )) ) { return; }
    com_assertTrue( "com_initBuffer()",
            com_initBuffer( &path, strlen(gSeekPath)+16, gSeekPath ) );
    count = com_scanDir( gSeekPath, checkCSource, &list );
    com_printf( "count = %d\n", count );
    com_assertNotEquals( "count", 0, count );

    com_setWatchMemInfo( COM_DEBUG_SILENT );
    for( int i = 0;  i < count;  i++ ) {
        char* name = list[i]->d_name;
        com_assertTrue( "com_setBuffer()",
                com_setBuffer( &path, "%s/%s", gSeekPath, name ) );
        if( com_checkIsDir( path.data ) ) { com_printf( "*" ); }
        else { com_printf( " " ); }
        com_printf( "%-19s\n", name );
    }
    com_printLf();
    com_setWatchMemInfo( COM_DEBUG_ON );
    com_resetBuffer( &path );
    com_freeScanDir( count, &list );
}

///// test_checkFiles() //////////////////////////////////////////////////////

static void checkExistFiles( const char *iFiles, BOOL iAssert )
{
    com_printf( "%s\n", iFiles );
    if( com_checkExistFiles( iFiles, " " ) ) {
        com_assertTrue( "all exist", iAssert );
    }
    else {
        com_assertFalse( "not all exist...", iAssert );
    }
}

void test_checkFiles( void )
{
    startFunc( __func__ );
    checkExistFiles( "../com_if.h", true );
    checkExistFiles( "../com_if.h  ../com_proc.c", true );
    checkExistFiles( "../com_if.h ../com_proc.c ../com_dummy.c  ", false );
}

///// test_printTag() ////////////////////////////////////////////////////////

static void testPrintTag( char *iTag, size_t iWidth, const char *iFormat, ... )
{
    char label[COM_LINEBUF_SIZE];
    COM_SET_FORMAT( label );    // [DBG] com_debug.h が必要
    for( COM_PTAG_POS_t pos = COM_PTAG_LEFT; pos <= COM_PTAG_RIGHT; pos++ ) {
        com_printTag( iTag, iWidth, pos, "%s", label );
        // 目視で 左詰め・センター・右詰め になっているか確認
    }
}

void test_printTag( void )
{
    startFunc( __func__ );
    testPrintTag( "*",   79, "com_printTag() TEST" );
    testPrintTag( "=",   20, "1234567890123456" );
    testPrintTag( "=",   20, "12345678901234567" );
    testPrintTag( "=*=", 79, "multi tag line" );
}

///// test_debugLog() ////////////////////////////////////////////////////////

void test_debugLog( void ) 
{
    startFunc( __func__ );
    com_debug( "test\ndebug\nmode\n" );
    com_dbgCom( "\ntest\ndebug\nmode" );       // [DBG] com_debug.h が必要
    com_debug( "test\ndebug\nmode" );
    com_noComDebugLog( true );
    com_printf( " (disable COM log)\n" );
    // 以後 次文も含めて [COM] が付くログは出力されない
    com_dbgCom( "THIS LOG IS NOT WRITTEN" );   // [DBG] com_debug.h が必要
    // 画面出力と .test.log を目視確認
}

/////test_chainData() ////////////////////////////////////////////////////////

static char *spreadChain( com_strChain_t *iChain )
{
    static char spread[32];
    if(com_spreadChainData(iChain, spread, sizeof(spread)) == COM_SIZE_OVER){
        com_printf( "!" );
    }
    com_printf( "<%s>\n", spread );
    return spread;
}

void test_chainData( void )
{
    startFunc( __func__ );
    char* pop = NULL;
    com_strChain_t* chain = NULL;
    COM_ADDCHAIN_t sort = COM_SORT_ASCEND;  // ここを変えてソート順を指定

    // わざと末尾にスペースを含む文字列を oushする
    com_pushChainData( &chain, "%-5s", "ABC" );
    com_pushChainData( &chain, "%-5s", "DEFG" );
    com_pushChainData( &chain, "%-5s", "HI" );
    com_assertString( "chain", "HI    DEFG  ABC   ", spreadChain( chain ) );
    for( int i = 0;  i < 4;  i++ ) {
        char* assertPop[] = { "HI   ", "DEFG ", "ABC  " };
        pop = com_popChainData( &chain );
        com_printf( " \"%s\" ", pop );
        if( i < 3 ) { com_assertString( "pop", assertPop[i], pop ); }
        else { com_assertNull( "pop", pop ); }
        spreadChain( chain );
    }
    com_sortAddChainData( &chain, sort, "abc" );
    com_sortAddChainData( &chain, sort, "def" );
    com_sortAddChainData( &chain, sort, "ghi" );
    com_sortAddChainData( &chain, sort, "abc" );
    com_sortAddChainData( &chain, sort, "def" );
    com_assertString( "chain", "abc abc def def ghi ", spreadChain( chain ) );
    com_deleteChainData( &chain, false, "def" );
    com_assertString( "del \"def\" no repeat", "abc abc def ghi ", spreadChain( chain ) );
    com_deleteChainData( &chain, true,  "abc" );
    com_assertString( "del \"abc\" repeat", "def ghi ", spreadChain( chain ) );

    com_freeChainData( &chain );
}

///// test_chainNum() ////////////////////////////////////////////////////////

void test_chainNum( void )
{
    startFunc( __func__ );
    long pop = 0;
    com_strChain_t* chain = NULL;
    char spread[COM_LINEBUF_SIZE];

    com_pushChainNum( &chain, 123456 );
    com_pushChainNum( &chain, 23455 );
    com_pushChainNum( &chain, 999999999 );
    com_spreadChainNum( chain, spread, sizeof(spread) );
    com_assertString( "spread", "999999999 23455 123456 ", spread );
    long idx = 0;
    while( chain ) {
        long  assertPop[] = { 999999999, 23455, 123456 };
        pop = com_popChainNum( &chain );
        com_assertEquals( "pop", assertPop[idx++], pop );
    }

    COM_ADDCHAIN_t sort = COM_SORT_ASCEND;  // ここを変えてソート順を指定
    com_sortAddChainNum( &chain, sort,  11223344 );
    com_sortAddChainNum( &chain, sort,  22334455 );
    com_sortAddChainNum( &chain, sort, -22334455 );
    com_sortAddChainNum( &chain, sort, -44556677 );
    com_spreadChainNum( chain, spread, sizeof(spread) );
    if( sort == COM_SORT_ASCEND ) {
        com_assertString( "spread(COM_SORT_ASCEND)",
                      "-44556677 -22334455 11223344 22334455 ", spread );
    }
    if( sort == COM_SORT_DESCEND ) {
        com_assertString( "spread(COM_SORT_DESCEND)",
                      "22334455 11223344 -22334455 -44556677 ", spread );
    }

    com_freeChainNum( &chain );
}

///// test_bufferData() //////////////////////////////////////////////////////

static void printBuffer(
        const char *iLabel, com_buf_t *iBuf, char *iAssert, size_t iExpected )
{
    com_printf( "%s:\n", iLabel );
    if( !iBuf ) { return; }
    if( iBuf->data ) {
        com_assertString( "iBuf->data", iAssert, iBuf->data );
    }
    else {
        com_assertNull( "iAssert", iAssert );
    }
    com_assertEqualsU( "iBuf->size", iExpected, iBuf->size );
    com_printLf();
}

void test_bufferData( void )
{
    startFunc( __func__ );
    com_buf_t* buf1 = NULL;
    com_buf_t  buf2, buf3;
    com_assertTrue( "com_createBuffer() <TEST BUFFER>",
            com_createBuffer( &buf1, 0, "TEST BUFFER" ) );
    printBuffer( "buf1", buf1, "TEST BUFFER", 12 );
    com_initBuffer( &buf2, 0, NULL );
    com_initBuffer( &buf3, 0, "DUMMY" );
    com_resetBuffer( &buf2 );
    com_assertTrue( "com_setBuffer() <TESTING NOW123>",
            com_setBuffer( &buf2, "%s%d", "TESTING NOW", 123 ) );
    printBuffer( "buf2", &buf2, "TESTING NOW123", 15 );
    com_clearBuffer( buf1 );
    com_printf( "clear buf1\n" );
    printBuffer( "buf1", buf1, "", 12 );
    com_assertTrue( "com_addBuffer() ABCDE", com_addBuffer( buf1, "ABCDE" ) );
    com_assertTrue( "com_addBuffer() fghij", com_addBuffer( buf1, "fghij" ) );
    com_assertTrue( "com_addBuffer() 00023", com_addBuffer( buf1, "%05d", 23 ) );
    com_assertTrue( "com_addBufferSize() pqr", com_addBufferSize( buf1, 3, "pqrst" ) );
    printBuffer( "buf1", buf1, "ABCDEfghij00023pqr", 19 );
    com_freeBuffer( &buf1 );
    com_assertTrue( "com_copyBuffer() buf2->buf3", com_copyBuffer( &buf3, &buf2 ) );
    com_resetBuffer( &buf2 ); 
    com_printf( "reset buf2\n" );
    printBuffer( "buf2", &buf2, NULL, 0 );
    printBuffer( "buf3", &buf3, "TESTING NOW123", 15 );
    com_resetBuffer( &buf3 );
}

///// test_strop() ///////////////////////////////////////////////////////////

void test_strop( void )
{
    startFunc( __func__ );
    char testBuf[10] = {0};
    char source[] = "12345678901234567890";
    char text1[] = "aBcDeFgHiJ";
    char text2[] = "ABCDEFGHIJ";

    com_assertFalse( "com_strncpy()",
            com_strncpy( testBuf, sizeof(testBuf), source, strlen(source) ) );
    com_printf( "[1]buffer size over(%s)\n", testBuf );
    COM_CLEAR_BUF( testBuf );
    com_assertTrue( "com_strncpy)()",
            com_strncpy( testBuf, sizeof(testBuf), source, 6 ) );
    com_printf( "[2]buffer size OK! (%s)\n", testBuf );
    com_assertFalse( "com_strncat()",
            com_strncat( testBuf, sizeof(testBuf), source, 6 ) );
    com_printf( "[3]buffer size over(%s)\n", testBuf );
    com_assertFalse( "com_strncat()",
            com_strncat( testBuf, sizeof(testBuf), source, 6 ) );
    com_printf( "[4]buffer size over(%s)\n", testBuf );

    BOOL result = com_compareString( text1, text2, 0, false );
    com_printf( "compare (%s : %s)\n", text1, text2 );
    com_assertFalse( "result", result );
    result = com_compareString( text1, text2, 0, true  );
    com_printf( "no case compare (%s : %s)\n", text1, text2 );
    com_assertTrue( "result", result );

    char topStr[] = "   \t   \nABC";
    char* top = com_topString( topStr, true );
    com_assertString( "top", "ABC", top );
    top = com_topString( topStr, false );
    com_assertString( "top", "\nABC", top );
}

///// test_checkString() /////////////////////////////////////////////////////

static void checkStringProc(
        char *iText, com_isFunc_t *iFuncs, char *iLabel, COM_CHECKOP_t iAssert )
{
    com_printf( "%s<%s>\n", iText, iLabel );
    if( com_checkString( iText, iFuncs, COM_CHECKOP_AND ) ) {
        com_assertEquals( "AND", COM_CHECKOP_AND, iAssert );
    }
    if( com_checkString( iText, iFuncs, COM_CHECKOP_OR ) ) {
        com_assertEquals( "OR", COM_CHECKOP_OR, iAssert );
    }
    if( com_checkString( iText, iFuncs, COM_CHECKOP_NOT ) ) {
        com_assertEquals( "NOT", COM_CHECKOP_NOT, iAssert );
    }
    com_printf( "--------------------\n" );
}

void test_checkString( void )
{
    startFunc( __func__ );
    char test[] = "abcABC123\n";
    {
        com_isFunc_t funcs[] = { isalnum, iscntrl, NULL };
        checkStringProc( test, funcs, "isalnum, iscntrl", COM_CHECKOP_OR );
        // OR のみ条件が成立
    }
    {
        com_isFunc_t funcs[] = { isblank, ispunct, NULL };
        checkStringProc( test, funcs, "isblank, ispunct", COM_CHECKOP_NOT );
        // NOT のみ条件が成立
    }
}

///// test_checkDigit() //////////////////////////////////////////////////////

static void useDigitFunc(
        const char *iLabel, char* iTarget, com_validator_t iFunc, BOOL iResult )
{
    BOOL result = (iFunc)( iTarget, NULL );
    com_printf( " %s? %s\n", iLabel, iTarget );
    com_assertEquals( "result", iResult, result );
}

void test_checkDigit( void )
{
    startFunc( __func__ );
    useDigitFunc( "DEC", "+123456", com_valDigit, true );
    useDigitFunc( "DEC", "-123456", com_valDigit, true );
    useDigitFunc( "DEC", "1234ABC", com_valDigit, false );
    useDigitFunc( "HEX", "1234ABC", com_valHex,   true );
    useDigitFunc( "DEC", "0x12345", com_valDigit, false );
    useDigitFunc( "HEX", "0x12345", com_valHex,   true );
    useDigitFunc( "HEX", "0xABCDE", com_valHex,   true );
    useDigitFunc( "HEX", "0xVWXYZ", com_valHex,   false );
}

///// test_getString() ///////////////////////////////////////////////////////

void test_getString( void )
{
    startFunc( __func__ );
    char tmpbuf[COM_LINEBUF_SIZE] = {0};
    snprintf( tmpbuf, sizeof(tmpbuf), "0:%s 1:%s 2:%s 3:%s 4:%s 5:%s",
                com_getString( "%d", 123 ),
                com_getString( "%c", 65 ),
                com_getString( "%s.test", "TEST" ),
                com_getString( "%03d", 234 ),
                com_getString( "%04o", 12 ),
                com_getString( "%p", com_printf ) );
    com_printf( "%s\n", tmpbuf );
    char tmp1[] = "0:123 1:A 2:TEST.test 3:234 4:0014 5:";
    com_assertStringLen( "tmpbuf", tmp1, tmpbuf, strlen(tmp1) );

    snprintf( tmpbuf, sizeof(tmpbuf), "0:%s 1:%s 2:%s 3:%s 4:%s 5:%s",
                com_getString( "%s.test", "TEST" ),
                com_getString( "%d", 123 ),
                com_getString( "%03d", 234 ),
                com_getString( "%c", 65 ),
                com_getString( "%p", com_printf ),
                com_getString( "%04o", 12 ) );
    com_printf( "%s\n", tmpbuf );
    char tmp2[] = "0:TEST.test 1:123 2:234 3:A 4:0x100401a74 5:";
    com_assertStringLen( "tmpbuf", tmp2, tmpbuf, strlen(tmp2) );
}
// COM_FORMSTRING_MAX の数まではバッファが保持できるので、
// 上記はどちらも 0～4は正しく出力されるが、5の内容は不定(0の内容？)。
// これは一文での処理なので、二度目の 0 の評価は最初にしているためと思われる。
// そのため2つ目のsnprintf文では 最初に 0 に正しく内容が格納されている。
//
// ただ、二度目の 0の内容は不定と考えられるため、assert()は非対象とした。

///// test_getTime() /////////////////////////////////////////////////////////

void test_getTime( void )
{
    startFunc( __func__ );
    char dateText[COM_DATE_DSIZE], timeText[COM_TIME_DSIZE];

    com_getCurrentTime( COM_FORM_DETAIL, dateText, timeText, NULL );
    com_printf( "%s %s\n", dateText, timeText );
    com_getCurrentTime( COM_FORM_SIMPLE, dateText, timeText, NULL );
    com_printf( "%s %s\n", dateText, timeText );
    // 目視で現在日時が出力されることを確認。
}

///// test_convertTime() /////////////////////////////////////////////////////

static void convertTime( struct timeval *iTv, char *iAssert)
{
    com_time_t inf;
    com_convertSecond( &inf, iTv );
    char result[COM_TEXTBUF_SIZE] = {0};
    snprintf( result, sizeof(result),
              "%5ld days %02ld:%02ld:%02ld.%06ld",
              inf.day, inf.hour, inf.min, inf.sec, inf.usec );
    com_printf( "%ld.%06ld sec\n", iTv->tv_sec, iTv->tv_usec );
    com_assertString( "result", iAssert, result );
}

void test_convertTime( void )
{
    startFunc( __func__ );
    convertTime( &(struct timeval){    3601,   0 },
                 "    0 days 01:00:01.000000" );
    convertTime( &(struct timeval){ 1000000, 100 },
                 "   11 days 13:46:40.000100" );
}

///// test_archive() /////////////////////////////////////////////////////////

typedef struct {
    long    linecnt;
    long    assertCnt;
    char*   assertLine;
} assertArchive_t;

static BOOL checkLsResult( com_seekFileResult_t *iInf )
{
    assertArchive_t* assertData = iInf->userData;

    com_printf( "%s", iInf->line );
    if( assertData->linecnt == assertData->assertCnt ) {
        com_assertString( "iInf->line", assertData->assertLine, iInf->line );
    }
    (assertData->linecnt)++;
    return true;  // 最初の行だけを見たいので falseを返すので問題なし
}

static void dispLs( const char *iLsPath, assertArchive_t *iAssert )
{
    com_printf( "--- %s   assertion:%s", iLsPath, iAssert->assertLine );
    com_buf_t  lscmd;
    com_assertTrue( "com_initBuffer()",
            com_initBuffer( &lscmd, 0, "ls -aF %s", iLsPath ) );
    com_assertTrue( "com_pipeCommand()",
            com_pipeCommand( lscmd.data, checkLsResult, iAssert, NULL, 0 ) );
    com_printf( "-----\n" );
    com_resetBuffer( &lscmd );
    return;
}

void test_archive( void ) 
{
    startFunc( __func__ );
    char testdir[] = "testDir1";

    com_assertEquals( "rm -fr",
                  0, com_system( "rm -fr %s makefile,zip", testdir ) );

    com_printf( "create makefile.zip\n" );
    com_assertTrue( "com_zipFile()",
                com_zipFile( NULL, "makefile", NULL, false, true ) );
    com_assertTrue( "com_unzipFile()",
                com_unzipFile( "makefile.zip", testdir, NULL, true ) );
    com_assertEquals( "ls", 0, com_system( "ls" ) );
    assertArchive_t  test1 = { 0, 2, "makefile\n" };
    dispLs( testdir, &test1 );

    com_printf( "\ncreate .testzip\n" );
    char testzip[] = ".testzip";
    com_assertTrue( "com_zipFile()",
                com_zipFile( testzip, "makefile", "PASS", true, true ));
    com_assertEquals( "ls", 0, com_system( "ls %s", testzip ) );
    com_assertTrue( "com_unzipFile()",
                com_unzipFile( testzip,
                    com_getString("%s/test2",testdir), "PASS", false ));
    com_assertEquals( "ls", 0, com_system( "ls" ) );
    assertArchive_t  test2 = { 0, 3, "test2/\n" };
    dispLs( testdir, &test2 );

    com_printf( "\nremove %s\n", testdir );
    remove( testzip );
    com_removeDir( testdir );
    com_assertNotEquals( "ls", 0, com_system( "ls %s", testdir ) );
}

///// test_archive2() ////////////////////////////////////////////////////////

static BOOL checkArchive2(
        const char *iArch, const char *iSource, const char *iKey, BOOL iNoZip )
{
    if( com_zipFile( iArch, iSource, iKey, iNoZip, true ) ) {
        com_debug( "archive suceed" );
        com_assertEquals( "ls", 0, com_system( "ls %s.zip", iArch ) );
        com_assertEquals( "unzip", 0, com_system( "unzip -l %s.zip", iArch ) );
        // pack.zip の中身が com_* であることは目視で確認
        remove( "pack.zip" );
    }
    else {
        com_debug( "archive failed...");
        return false;
    }
    return true;
}

void test_archive2( void )
{
    startFunc( __func__ );
    com_assertFalse( "NULL",
            checkArchive2( NULL,   "../com*", NULL, false ) );   // 失敗する
    com_assertTrue(  "pack",
            checkArchive2( "pack", "../com*", NULL, false ) );   // 成功する
}

///// test_strtoul() /////////////////////////////////////////////////////////

static void checkConvertErrno( int iErrno, int iAssert )
{
    if( iErrno ) { com_printf( " (%s)\n", com_strerror( iErrno ) ); }
    else { com_printLf(); }
    com_assertEquals( "errno", iAssert, iErrno );
}

#define DISPCONVERTS( TYPE, FUNC, FORMAT, ASSERT, LABEL ) \
    TYPE value = FUNC( iSource, iBase, iCheck ); \
    int err = errno; \
    com_printf( "%s -> " FORMAT "  errno = %d", iSource, value, err ); \
    checkConvertErrno( err, iAssert ); \
    ASSERT( LABEL, iConvert, value );

static void dispConvertResult(
        char *iSource, int iBase, BOOL iCheck, ulong iConvert, int iAssert )
{
    DISPCONVERTS( ulong, com_strtoul, "%-8lx",
                 com_assertEqualsU, "com_strtoul()" );
}

static void dispConvertResult2(
        char *iSource, int iBase, BOOL iCheck, long iConvert, int iAssert )
{
    DISPCONVERTS( long, com_strtol, "%-8ld",
                 com_assertEquals, "com_strtol()" );
}

#define DISPCONVERTA( TYPE, FUNC, FORMAT, ASSERT, LABEL ) \
    TYPE value = FUNC( iSource ); \
    int err = errno; \
    com_printf( "%s -> " FORMAT "  errno = %d", iSource, value, err ); \
    checkConvertErrno( err, iAssert ); \
    ASSERT( LABEL, iConvert, value );

static void dispConvertResult3( char *iSource, int iConvert, int iAssert )
{
    DISPCONVERTA( int, com_atoi, "%-8d",
                 com_assertEquals, "com_atoi()" );
}

static void dispConvertResult4( char *iSource, float iConvert, int iAssert )
{
    DISPCONVERTA( float, com_atof, "%f",
                  com_assertEqualsF, "com_atof()" );
}

static void dispConvertResult5( char *iSource, uint iConvert, int iAssert )
{
    DISPCONVERTA( uint, com_atou, "%-8u",
                  com_assertEqualsU, "com_atou()" );
}

static void dispConvertResult6( char *iSource, ulong iConvert, int iAssert )
{
    DISPCONVERTA( ulong, com_atoul, "%lu",
                  com_assertEqualsU, "com_atoul()" );
}

void test_strtoul( void )
{
    startFunc( __func__ );
    char text1[] = "abcdefghijklmn";
    char text2[] = "1a2b3c";

    com_printf( "--- com_strtoul() ---\n" );
    dispConvertResult( text1, 16, true,      0xabcdef,   EINVAL );
    dispConvertResult( text1, 16, false,     0xabcdef,   EINVAL );
    dispConvertResult( text2, 16, true,      0x1a2b3c,   0 );
    dispConvertResult( "FFFFFFFF", 16, true, 0xffffffff, 0 );

    com_printf( "--- com_strtol() ---\n" );
    dispConvertResult2( text1, 16, true,      0xabcdef,   EINVAL );
    dispConvertResult2( text1, 16, false,     0xabcdef,   EINVAL );
    dispConvertResult2( text2, 16, true,      0x1a2b3c,   0 );
    dispConvertResult2( "FFFFffffFFFFffff", 16, true, LONG_MAX, ERANGE );

    com_printf( "--- com_atoi() ---\n" );
    dispConvertResult3( "123456", 123456, 0 );
    dispConvertResult3( "090807", 90807,  0 );
    dispConvertResult3( "-2147483648", -2147483648,  0 );
    dispConvertResult3( "0x1234", 0,      EINVAL );
    dispConvertResult3( "12abcd", 12,     EINVAL );
    dispConvertResult3( "9999999999", 2147483647, ERANGE );
    dispConvertResult3( "-9999999999", -2147483648, ERANGE );

    com_printf( "--- com_atof() ---\n" );
    dispConvertResult4( "123.45678", 123.45678F, 0 );
    dispConvertResult4( "123.DUMMY", 123.0F,     EINVAL );
    dispConvertResult4( "0x234.abc", 564.67090F,  0 );

    com_printf( "--- com_atou() ---\n" );
    dispConvertResult5( "4294967295", 4294967295, 0 );
    dispConvertResult5( "9999999999", 4294967295, ERANGE );

    com_printf( "--- com_atoul() ---\n" );
    dispConvertResult6( "9999999999999", 9999999999999, 0 );
    dispConvertResult6( "0xabcdefabcdef", 0, EINVAL );

}

///// test_hash() ////////////////////////////////////////////////////////////

static com_hash_t calcHashTest(
        const void *iKey, size_t iKeySize, size_t iTableSize )
{
    com_hash_t  result = iTableSize;
    uchar* tmp = (uchar*)iKey;
    com_dump( iKey, iKeySize, "key data" );
    com_printf( "table size = %zu\n", iTableSize );
    for( size_t i = 0;  i < iKeySize;  i++ ) {result += (com_hash_t)(tmp[i]);}
    result %= iTableSize;
    com_printf( "calcHashTest = %lu\n", result );
    return result;
}

void test_hash( void )
{
    startFunc( __func__ );
    com_hashId_t  id1, id2;
    const void* val = NULL;
    size_t size = 0;
    int dummy = 1234567;

    com_assertTrue( "com_checkHash(0)", com_checkHash( 0 ) );
    id1 = com_registerHash( 101, calcHashTest );
    com_assertNotEquals( "hash id1", 0, id1 );
    id2 = com_registerHash(  17, NULL );
    com_assertNotEquals( "hash id2", 0, id2 );

    COM_HASH_t result = com_addHash( id1, false, "AAA", 4, "BCD", 4 );
    com_printf( "add(1) AAA->BCD = %d\n", result );
    com_assertEquals( "add result", COM_HASH_OK, result );

    result = com_addHash( id1, false, "AAA", 4, "CDE", 4 );
    com_printf( "add(1) AAA->CDE = %d\n", result );
    com_assertEquals( "add result", COM_HASH_EXIST, result );

    result = com_addHash( id1, false, "CCC", 4, "DEF", 4 );
    com_printf( "add(1) CCC->DEF = %d\n", result );
    com_assertEquals( "add result", COM_HASH_OK, result );

    result = com_addHash( id1, false, "CCC", 4, "123456", 4 );
    com_printf( "add(1) CCC->123456 = %d\n", result );
    com_assertEquals( "add result", COM_HASH_EXIST, result );

    com_assertTrue( "com_searchHash() \"AAA\"",
            com_searchHash( id1, "AAA", 4, &val, &size ) );
    com_printf( "  (1)AAA = %s\n", (char*)val );
    com_assertString( "AAA", "BCD", (char*)val );

    com_assertFalse( "com_searchHash() \"BBB\"",
            com_searchHash( id1, "BBB", 4, &val, &size ) );
    com_printf( "  (1)BBB = ---\n" );

    com_assertTrue( "com_searchHash() \"CCC\"",
            com_searchHash( id1, "CCC", 4, &val, &size ) );
    com_printf( "  (1)CCC = %s\n", (char*)val );
    com_assertString( "CCC", "DEF", (char*)val );

    com_dump( val, size, "" );

    result = com_addHash(id2, false, "12345667", 9, &dummy, sizeof(dummy));
    com_printf( "add(2) = %d\n", result );
    com_assertEquals( "add result", COM_HASH_OK, result );

    com_cancelHash( id2 );

    result = com_addHash( id1, true, "AAA", 4, "CCCCCC", 7 );
    com_printf( "add(1) = %d\n", result );
    com_assertEquals( "add result", COM_HASH_OVERWRITE, result );

    com_assertTrue( "com_searchHash() \"AAA\"",
            com_searchHash( id1, "AAA", 4, &val, &size ) );
    com_printf( "   (1)AAA = %s\n", (char*)val );

    BOOL delResult = com_deleteHash( id1, "BBB", 4 );
    com_printf( "del(1) BBB = %ld\n", delResult );
    com_assertTrue( "com_deleteHash() \"BBB\"", delResult );

    delResult = com_deleteHash( id1, "CCC", 4 );
    com_printf( "del(1) CCC = %ld\n", delResult );
    com_assertTrue( "com_deleteHash() \"CCC\"", delResult );
}
    
///// test_sortTable() ///////////////////////////////////////////////////////

typedef struct {
    long   key;
    char*  str;
} com_testSort_t;

enum {
    SORTTEST_MAX = 44,
    SORTDEBUG = 1
};

static com_testSort_t gSortTestData[] = {
    { 10, "beta" },
    { 11, "gamma" },
    {  1, "alpha" },
    { SORTTEST_MAX, "epsilon" },
    { 11, "gamma2" },
    { 25, "delta" },
    {  1, "alpha2" },
    { 11, "gamma3" },
    {  0, "TOP" }
};

static long modRange( long iRange )
{
    if( iRange > 1 ) {return (iRange - 1);}
    return 0;
}

static void setAssert( long *ioTable, com_sort_t *iSort, size_t iIdx )
{
    if( SORTDEBUG ) {com_printf( " *%ld*", iIdx );}
    for( long pos = 0; pos <= modRange( iSort->range ); pos++ ) {
        if( SORTDEBUG ) {com_printf( " %ld", iSort->key + pos );}
        ioTable[iSort->key + pos] = (long)iIdx;
    }
    if( SORTDEBUG ) {com_printLf();}
}

static BOOL checkRange( com_sort_t *iData, long *iTable )
{
    for( long i = 0;  i <= modRange( iData->range );  i++ ) {
        long idx = iData->key + i;
        if( SORTDEBUG ) {com_printf( " %ld", idx );}
        if ( iTable[idx] ) {
            if( SORTDEBUG ) {com_printf( " [%ld]\n", iTable[idx] );}
            return false;
        }
    }
    if( SORTDEBUG ) {com_printLf();}
    return true;
}

static BOOL expectedResult(
        size_t iIdx, com_sort_t *iData, COM_SORT_MATCH_t iMode, long *iTable )
{
    if( !iIdx ) {return true;}
    if( checkRange( iData, iTable ) ) {return true;}
    if( SORTDEBUG ) {com_printf( " --- %zu ---\n", iData->range );}
    if( iData->range > 1 ) {return false;}
    if( SORTDEBUG ) {com_printf( " --- %u ---\n", iMode );}
    if( iMode == COM_SORT_SKIP ) {return false;}
    return true;
}

static void showKeyData( com_sort_t *iData )
{
    com_printf( "  %3ld", iData->key );
    if( modRange( iData->range ) > 0 )  {
        com_printf( "-%3ld", iData->key + modRange( iData->range ) );
    }
    com_printf( " %s\n", (char*)iData->data );
}

static void showTable( com_sortTable_t *iTable )
{
    com_printf( "--- count = %ld ---\n", iTable->count );
    for( int j = 0;  j < iTable->count;  j++ ) {
        showKeyData( &(iTable->table[j]) );
    }
}

static com_sort_t gTarget[] = {
    { 11, 35, NULL, 0 },
    { 11,  1, "gamma2", 7 },
    {  5,  1, NULL, 0 },
    { 50,  1, NULL, 0 }
};

static long expectedHit( com_sortTable_t *iTable, com_sort_t *iTarget )
{
    long  result = 0;
    for( long i = 0;  i < iTable->count;  i++ ) {
        com_sort_t* data = &(iTable->table[i]);
        if( iTarget->data ) {
            if( !com_compareString( data->data, iTarget->data, 0, false ) ) {
                continue;
            }
        }
        if( (data->key <= (iTarget->key + modRange(iTarget->range))) &&
            (iTarget->key <= (data->key + modRange(data->range))) )
        {
            result++;
        }
    }
    return result;
}

static void searchTable( com_sortTable_t *iTable, com_sort_t *iTarget )
{
    long searchCount = 0;
    long expected = expectedHit( iTable, iTarget );
    com_sort_t** searchResult = NULL;

    searchCount = com_searchSortTable( iTable, iTarget, &searchResult );
    com_printf( "search " );
    showKeyData( iTarget );
    for( long i = 0;  i < searchCount;  i++ ) {
        showKeyData( searchResult[i] );
    }
    if( expected >= 0 ) {com_assertEquals( "hit count", expected, searchCount );}
}

#define  TEST_MODE   COM_SORT_OVERWRITE
#define  TEST_RANGE  0

void test_sortTable( void )
{
    startFunc( __func__ );
    com_sortTable_t test;
    long testassert[SORTTEST_MAX + TEST_RANGE + 1] = {0};

    // TEST_MODE や TEST_RANGE を変えることで挙動が変わることを確認
    com_printf( "TEST_MODE:%d, TEST_RANGE:%d\n", TEST_MODE, TEST_RANGE );

    com_initializeSortTable( &test, TEST_MODE );
    for( size_t i = 0;  i < COM_ELMCNT( gSortTestData );  i++ ) {
        com_sort_t data = {
            gSortTestData[i].key, TEST_RANGE,
            gSortTestData[i].str, strlen(gSortTestData[i].str) + 1
        };  // TEST_RANGEの値を変えることで範囲付きキーの動作確認可能
        BOOL expected = expectedResult( i, &data, TEST_MODE, testassert );
        BOOL result = com_addSortTable( &test, &data, NULL );
        if( !result ) {
            com_printf( ">> canceled :" );
            showKeyData( &data );
            com_assertEquals( "com_addSortTable() canceled", expected, result );
            continue;
        }
        com_printf( " >> input :" );
        showKeyData( &data );
        setAssert( testassert, &data, i + 1 );
        com_assertEquals( "com_addSortTable() input", expected, result );

        showTable( &test );
    }
    for( long i = 0;  i < 4;  i++ ) {
        searchTable( &test, &(gTarget[i]) );
    }

    long delCount = com_deleteSortTable( &test, &gTarget[0], true );
    showTable( &test );
    com_printf( " deleted : %ld\n", delCount );

    com_freeSortTable( &test );
}

///// test_stopwatch /////////////////////////////////////////////////////////

void test_stopwatch( void )
{
    startFunc( __func__ );
    com_stopwatch_t sw;
    struct timeval tv;

    do { com_startStopwatch( &sw ); } while( sw.start.tv_usec < 950000 );
    do { com_gettimeofday( &tv, "stop time" ); } while( tv.tv_usec >= 950000 );
    com_checkStopwatch( &sw );
    com_printf( " passed: %ld.%06ldsec\n", sw.passed.tv_sec, sw.passed.tv_usec);
    com_assertEquals( " sec", 0,     sw.passed.tv_sec );
    com_assertEquals( "usec", 50000, sw.passed.tv_usec );
}

///// test_dayOfWeek() ///////////////////////////////////////////////////////

static void checkDayOfWeek(
        time_t iYear, time_t iMon, time_t iDay, long iAssert )
{
    COM_WD_TYPE_t dotw = com_getDotw( iYear, iMon, iDay );
    com_printf( "%4ld/%02ld/%02ld: ", iYear, iMon, iDay );
    com_printf( "%s (%d)\n", com_strDotw( dotw, COM_WDSTR_EN2 ), dotw );
    if ( iAssert != COM_WD_NG ) {com_assertEquals( "dotw", iAssert, dotw );}
}

void test_dayOfWeek( void )
{
    startFunc( __func__ );
    com_time_t  tm;

    com_getCurrentTime( COM_FORM_DETAIL, NULL, NULL, &tm );
    checkDayOfWeek( tm.year, tm.mon, tm.day, COM_WD_NG );
    checkDayOfWeek( 1973, 2, 28, COM_WD_WEDNESDAY );
    checkDayOfWeek( 1978, 9, 30, COM_WD_SATURDAY );
}

///// test_printEsc() ////////////////////////////////////////////////////////

void test_printEsc( void )
{
    startFunc( __func__ );
    com_printf( "count test: " );
    for( int i = 9;  i < 12;  i++ ) {
        com_printBack( "%4d\nTEST", i );
        sleep( 1 );
    }
    // "count test:   11" と標準出力されることを目視
    com_printLf();
    com_repeat( "#", 70, true );
    com_repeat( "#", 70, false );
    com_printCr( "### caridge return test \n" );
    com_repeat( "#", 70, true );
    // # で囲まれ "caridge return test" という文字が標準出力されることを目視
}

///// test_fileInfo() ////////////////////////////////////////////////////////

static void getFileInfo( const char *iPath, BOOL iAssert )
{
    com_fileInfo_t  info;
    char dateText[COM_DATE_SSIZE];
    char timeText[COM_DATE_SSIZE];

    com_printf( "--- %s ---\n", iPath );
    BOOL result = com_getFileInfo( &info, iPath, false );
    com_assertEquals( "com_getFileInfo()", iAssert, result );
    if( !result ) {
        com_printf( "not exist...\n" );
        return;
    }
    com_printf( " device:%lu (%u:%u)\n",
                (ulong)info.device, info.dev_major, info.dev_minor );
    com_printf( " inode :%lu\n", info.inode );
    com_printf( " mode  :%06o\n", info.mode );
    com_printf( "  isReg:%ld isDir:%ld isLink:%ld\n",
                info.isReg, info.isDir, info.isLink );
    com_printf( "  permission:%03o\n",
                info.mode & (S_IRWXU | S_IRWXG | S_IRWXO ) );
    com_printf( " uid:%u  gid:%u\n", info.uid, info.gid );
    com_printf( " size:%ld\n", info.size );

    com_setTime( COM_FORM_SIMPLE, dateText, timeText, NULL, &(info.atime) );
    com_printf( " last access: %s %s\n", dateText, timeText );
    com_setTime( COM_FORM_SIMPLE, dateText, timeText, NULL, &(info.mtime) );
    com_printf( " last modify: %s %s\n", dateText, timeText );
    com_setTime( COM_FORM_SIMPLE, dateText, timeText, NULL, &(info.ctime) );
    com_printf( " last change: %s %s\n", dateText, timeText );
}

void test_fileInfo( void )
{
    startFunc( __func__ );
    getFileInfo( "fields", false );
    com_printLf();
    getFileInfo( ".", true );
}

///// test_strtooct() ////////////////////////////////////////////////////////

static void convertBinary(
        uchar *oOct, size_t iLen, const char *iSource, uchar *iAssert )
{
    com_printf( "[%s]\n", iSource );
    
    BOOL noNeedFree = (BOOL)iLen;
    if( !com_strtooct( iSource, &oOct, &iLen ) ) {
        com_assertNull( "assertion", iAssert );
        return;
    }
    com_assertNotNull( "assertion", iAssert );
    for( size_t i = 0;  i < iLen;  i++ ) {
        com_assertEqualsU( "binary", iAssert[i], oOct[i] );
    }
    //com_printBin_t flags = { .prefix = " ", .colSeq = 1 };
    //com_printBinary( oOct, iLen, &flags );
    if( !noNeedFree ) { com_free( oOct ); }
}

void test_strtooct( void )
{
    startFunc( __func__ );
    char test[] = { '3', '1', '\r', '\n', '3', '2', '\0' };
    uchar buf[4];
    // 出力先サイズが 4 なので 4オクテットのみの取得
    convertBinary( buf, sizeof(buf), "010203040506",
               (uchar[]){ 0x01, 0x02, 0x03, 0x04 } );
    convertBinary( NULL, 0, "01 02 03 04 05 06 07 08",
               (uchar[]){ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 } );
    // 16進数以外の文字が入っているのでNG
    convertBinary( NULL, 0, "aa bbc cdd eef ffg", NULL );
    // 数字が奇数個なのでNG
    convertBinary( NULL, 0, "aa bb cc dd ee ff 0", NULL );
    convertBinary( NULL, 0, "a1b     2c3      d4e  5f6",
               (uchar[]){ 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6 } );
    convertBinary( NULL, 0, test, (uchar[]){ 0x31, 0x32 } );
}

///// test_doublefree() //////////////////////////////////////////////////////

void test_doublefree( void )
{
    startFunc( __func__ );
    int* test = com_malloc( sizeof(int), __func__ );
    com_assertNotNull( "test", test );
    int* dummy = test;
    //com_printf( "test = %p, dummy = %p\n", (void*)test, (void*)dummy );
    com_free( test );
    com_assertNull( "test", test );
    //com_printf( "test = %p, dummy = %p\n", (void*)test, (void*)dummy );
    com_free( dummy );  // COM_DOUBLEFREE でエラーが出る。
    com_assertEquals( "last error", COM_ERR_DOUBLEFREE, com_getLastError() );
    com_assertNull( "dummy", dummy );
}

///// test_prmNG() ///////////////////////////////////////////////////////////

void test_prmNG( void )
{
    startFunc( __func__ );
    int num = 2;
    char text[] = "text";

    // [DBG] com_prmNG() は com_debug.h が必要
    com_prmNG( "ABC" );
    com_prmNG( "%s -ABC%d", text, num );
    com_prmNG( NULL );
    // 出力は目視確認
}

///// test_getFileFunc() /////////////////////////////////////////////////////

static void testGetFile(
        char *oBuf, size_t iBufSize, long iType, char *iTarget,
        BOOL iAssertResult, char *iAssertText )
{
    com_printf( "type %ld:%s\n", iType, iTarget );
    BOOL result = false;
    if( iType == 1 ) {result = com_getFileName(oBuf,iBufSize,iTarget);}
    if( iType == 2 ) {result = com_getFilePath(oBuf,iBufSize,iTarget);}
    if( iType == 3 ) {result = com_getFileExt( oBuf,iBufSize,iTarget);}
    com_assertEquals( "result", iAssertResult, result );
    com_assertString( "buffer", iAssertText, oBuf );
}

#define GETFILE( TYPE, PATH, RESULT, TEXT )  \
    testGetFile( buf, sizeof(buf), (TYPE), PATH, RESULT, TEXT )

void test_getFileFunc( void )
{
    startFunc( __func__ );
    char buf[10] = {0};
    GETFILE( 1, "testfile.txt",   false, "testfile." );  // バッファ不足
    GETFILE( 1, "aa/bb/cc.ex",    true,  "cc.ex" );
    GETFILE( 2, "testfile.txt",   false, "" );
    GETFILE( 2, "aa/bb/cc.ex",    true,  "aa/bb/" );
    GETFILE( 2, "aa/bb/cc/",      true,  "aa/bb/cc/" );
    GETFILE( 3, "testfile.txt",   true,  "txt" );
    GETFILE( 3, "aa/bb/cc.ex",    true,  "ex" );
    GETFILE( 3, "aa/bb/cc.ex.",   false, "" );
    GETFILE( 3, "aa/bb/.ccextra", true,  "ccextra" );
    GETFILE( 3, "aa/bb/ccextra",  false, "" );
}

///// test_ringBuffer() //////////////////////////////////////////////////////

static void freeRingUnit( void *oData )
{
    char** tmp = oData;
    if( !(*tmp) ) {return;}
    com_printf( "*** delete %s ***\n", *tmp );
    com_free( *tmp );
}

void test_ringBuffer( void )
{
    startFunc( __func__ );
    char* tmp = NULL;
    com_ringBuf_t* ring =
        com_createRingBuf( sizeof(char*), 5, false, true, freeRingUnit );
    com_assertNotNull( "create", ring );
    if( (tmp = com_strdup( "ABCDE", "test1(ABCDE)" )) ) {
        com_assertTrue( "push1", com_pushRingBuf( ring, &tmp, sizeof(tmp) ) );
    }
    if( (tmp = com_strdup( "abcde", "test2(abcde)" )) ) {
        com_assertTrue( "push2", com_pushRingBuf( ring, &tmp, sizeof(tmp) ) );
    }
    com_printf( "current ring rest = %zu\n", com_getRestRingBuf( ring ) );

    char** data = com_pullRingBuf( ring );
    com_assertString( "pull", "ABCDE", *data );

    if( (tmp = com_strdup( "33333", "test3(33333)" )) ) {
        com_assertTrue( "push3", com_pushRingBuf( ring, &tmp, sizeof(tmp) ) );
    }
    if( (tmp = com_strdup( "44444", "test4(44444)" )) ) {
        com_assertTrue( "push4", com_pushRingBuf( ring, &tmp, sizeof(tmp) ) );
    }
    if( (tmp = com_strdup( "55555", "test5(55555)" )) ) {
        com_assertTrue( "push5", com_pushRingBuf( ring, &tmp, sizeof(tmp) ) );
    }
    if( (tmp = com_strdup( "66666", "test6(66666)" )) ) {
        com_assertTrue( "push6", com_pushRingBuf( ring, &tmp, sizeof(tmp) ) );
    }

    com_printf( "--- push ring error test ---\n" );
    
    com_resetLastError();
    if( (tmp = com_strdup( "77777", "test7(77777)" )) ) {
        // エラーになる
        com_assertFalse( "push7", com_pushRingBuf( ring, &tmp, sizeof(tmp) ) );
        com_assertEquals( "errno", COM_ERR_RING, com_getLastError() );
        com_free(tmp);
    }

    data = com_pullRingBuf( ring );
    com_assertString( "pull", "abcde", *data );

    if( (tmp = com_strdup( "88888", "test8(88888)" )) ) {
        com_assertTrue( "push8", com_pushRingBuf( ring, &tmp, sizeof(tmp) ) );
    }

    com_printf( "--- push ring error test2 ---\n" );
    com_resetLastError();
    if( (tmp = com_strdup( "99999", "test9(99999)" )) ) {
        // エラーになる
        com_assertFalse( "push9", com_pushRingBuf( ring, &tmp, sizeof(tmp) ) );
        com_assertEquals( "errno", COM_ERR_RING, com_getLastError() );
        com_free(tmp);
    }

    com_printf( "--- ring test end ---\n" );
    com_assertEqualsU( "overwrite count", 0, ring->owCount );
    com_freeRingBuf( &ring );
}

///// test_config() //////////////////////////////////////////////////////////

#define KEY_TEST1  "TEST1"
#define KEY_TEST2  "TEST2"
#define KEY_TEST3  "TEST3"
#define KEY_TEST4  "TEST4"
#define KEY_TEST5  "TEST5"

static void setCfgTest( char *iKey, char *iData, long iAssert )
{
    com_printf( "%s -> %s\n", iKey, iData );
    com_notifyError( false, true );  // エラー出力抑制(ログには残す)
    long result = com_setCfg( iKey, iData );
    com_notifyError( true, true );   // エラー出力抑制を解除
    com_assertEquals( "result", iAssert, result );
}

void test_config( void )
{
    startFunc( __func__ );

    com_assertTrue( "register TEST1", com_registerCfg( KEY_TEST1, NULL ) );
    com_assertTrue( "empty TEST1", com_isEmptyCfg( KEY_TEST1 ) );
    com_valCondDigit_t cond = { 10, 40 };
    com_addCfgValidator( KEY_TEST1, &cond, COM_VAL_DIGIT );
    setCfgTest( KEY_TEST1, "0",  1 );
    setCfgTest( KEY_TEST1, "10", 0 );
    setCfgTest( KEY_TEST1, "40", 0 );
    setCfgTest( KEY_TEST1, "41", 1 );
    setCfgTest( KEY_TEST1, "dummy", 1 );
    com_assertEquals( "TEST1", 40, com_getCfgDigit( KEY_TEST1 ) );

    com_resetLastError();
    com_assertFalse( "register TEST1", com_registerCfg( KEY_TEST1, "ALPHA" ) );
    com_assertEquals( "last error", COM_ERR_CONFIG, com_getLastError() );

    com_assertTrue(  "register TEST2", com_registerCfg( KEY_TEST2, "ALPHA" ) );
    char* list2[] = { "ALPHA", "BETA", "GAMMA", NULL };
    com_valCondStrList_t cond2 = { false, list2 };
    com_addCfgValidator( KEY_TEST2, &cond2, COM_VAL_STRLIST );
    setCfgTest( KEY_TEST2, "BETA",  0 );
    setCfgTest( KEY_TEST2, "DELTA", 1 );
    setCfgTest( KEY_TEST2, "alpha", 1 );
    com_assertString( "TEST2", "BETA", (char*)com_getCfg( KEY_TEST2 ) );

    com_assertTrue( "register TEST3", com_registerCfg( KEY_TEST3, NULL ) );
    ulong list3[] = { 0xAAA, 0xBBB, 0xCCC };
    com_valCondUDgtList_t cond3 = { COM_ELMCNT(list3), list3 };
    com_addCfgValidator( KEY_TEST3, &cond3, COM_VAL_HEXLIST );
    setCfgTest( KEY_TEST3, "aAa", 0 );
    com_assertEqualsU( "TEST3", 0xaaa, com_getCfgHex( KEY_TEST3 ) );
    setCfgTest( KEY_TEST3, "0xbbb", 0 );
    setCfgTest( KEY_TEST3, "Oxccc", 1 );
    com_assertEqualsU( "TEST3", 0xbbb, com_getCfgHex( KEY_TEST3 ) );

    com_assertTrue( "register TEST4", com_registerCfg( KEY_TEST4, NULL ) );
    com_addCfgValidator( KEY_TEST4, NULL, COM_VAL_DIGIT );
    com_valCondString_t cond4 = { 1, 10 };
    com_addCfgValidator( KEY_TEST4, &cond4, COM_VAL_STRING );
    setCfgTest( KEY_TEST4, "-1000", 0 );
    setCfgTest( KEY_TEST4, "1234567890", 0 );
    setCfgTest( KEY_TEST4, "12345678901", 2 );
    setCfgTest( KEY_TEST4, "10*10", 1 );
    setCfgTest( KEY_TEST4, "0xFFFFFFFF", 1 );
    com_assertEqualsU( "TEST4", 0x1234567890, com_getCfgHex( KEY_TEST4 ) );

    com_assertTrue( "register TEST5", com_registerCfg( KEY_TEST5, NULL ) );
    com_addCfgValidator( KEY_TEST5, NULL, COM_VAL_YESNO );
    setCfgTest( KEY_TEST5, "On", 1 );
    setCfgTest( KEY_TEST5, "YeS", 0 );
    com_assertTrue( "TEST5", com_getCfgBool( KEY_TEST5 ) );

    com_resetLastError();
    com_assertString( "DUMMY", "", (char*)com_getCfg( "DUMMY" ) );
    com_assertEquals( "last error", COM_ERR_CONFIG, com_getLastError() );

#if 0
    com_printLf();
    long count = 0;
    const char *key, *data;
    while( com_getCfgAll( &count, &key, &data ) ) {
        com_printf( "%s = %s\n", key, data );
    }
#endif
}

#define ASSERTEQ   com_assertEquals
#define ASSERTNE   com_assertNotEquals
#define ASSERTEQU  com_assertEqualsU
#define ASSERTNEU  com_assertNotEqualsU
#define ASSERTEQF  com_assertEqualsF
#define ASSERTNEF  com_assertNotEqualsF
#define ASSERTEQD  com_assertEqualsD
#define ASSERTNED  com_assertNotEqualsD

#define ASSERT( LABEL, EXPECTED, RESULT, SUFFIX )  \
    ASSERTEQ##SUFFIX( LABEL, (EXPECTED), (EXPECTED) ); \
    ASSERTNE##SUFFIX( LABEL, (EXPECTED), (RESULT) ); \

void test_assertion( void )
{
    startFunc( __func__ );
    ASSERT( "long", 100L, 101,  );
    ASSERT( "ulong", 100UL, 101, U );
    ASSERT( "float", 0.0001F, 0.0002F, F );
    com_printf( "FLT_EPSILON=%.16f\n", FLT_EPSILON );
    ASSERT( "double", 0.0000000001, 0.0000000002, D );
    com_printf( "DBL_EPSILON=%.16f\n", DBL_EPSILON );
    
    com_assertString( "string", "ABCDE", "ABCDE" );
    com_assertStringLen( "string(length=5)", "ABCDEFGHIJ", "ABCDEVWXY", 5 );

    com_assertTrue( "bool", TRUE );
    com_assertFalse( "bool", FALSE );

    com_assertNull( "pointer", NULL );
    com_assertNotNull( "pointer", (void*)0xabcde );
}


#ifdef USING_COM_EXTRA    // エキストラ機能を使うテストコード

// test_multiThread() ////////////////////////////////////////////////////////
// スレッドは基本機能だが、乱数を使いたいため、エキストラ機能での実行とする

static pthread_mutex_t gMutexTest = PTHREAD_MUTEX_INITIALIZER;

static void *procThread( void *ioInf )
{
    com_threadInf_t* inf = ioInf;
    int* userData = inf->data;
    long waitSec = com_rand(9);
    pid_t pid = getpid();

    com_readyThread( ioInf );
    for( long s = 0;  s < waitSec;  s++ ) {
        sleep( 1 );
        com_mutexLock( &gMutexTest, "procThread" );
        com_printf( "[%d]thread%ld ", pid, inf->thId );
        if( com_checkChance( 5 ) ) {
            com_printf( "aborted\n" );
            com_mutexUnlock( &gMutexTest, "procThread" );
            return NULL;   // 敢えて com_finishThread() を使わない
        }
        (*userData)++;
        com_printf( "%d/%ld %ld%ld\n", pid, inf->thId, s, waitSec );
        com_mutexUnlock( &gMutexTest, "procThread" );
    }
    return com_finishThread( ioInf );
}

static void notifyThread( com_threadInf_t *iInf )
{
    int* userData = iInf->data;
    com_printf( "*** thread%ld finished... result = %02d\n",
                iInf->thId, *userData );
}

#define THREAD_TEST  10

void test_multiThread( void ) {
    startFunc( __func__ );
    int userData = 0;
    pthread_t ptid[THREAD_TEST];
    BOOL ret = false;

    com_printf( "--multi thread test--\n" );
    for( int i = 0;  i < THREAD_TEST;  i++ ) {
        userData = i * 10;
        ret = com_createThread( &ptid[i], procThread,
                                &userData, sizeof(userData),
                                notifyThread, "thread%d", i );
        if( !ret ) { return; }
    }
    while( com_watchThread( true ) ) { usleep( 500 ); }
    for( int i = 0;  i < THREAD_TEST;  i++ ) {
        int* data = com_getThreadUserData( ptid[i] );
        if( data ) { com_printf( "%02d", *data ); }
        else { com_printf( " --" ); }
        if( !com_freeThread( ptid[i] ) ) { com_printf( "***" ); }
    }
    com_printLf();
}

// test_comInputVar() ////////////////////////////////////////////////////////

// 動作については目視確認
void test_comInputVar( void )
{
    startFunc( __func__ );
#define TESTSIZE 5
    char* val = NULL;
    char* text = com_malloc( TESTSIZE, "test text" );
    com_actFlag_t flags = { true, false };
    size_t len = com_inputVar( text, TESTSIZE, NULL, &flags, "text > " );
    com_printf( "%-5s (length=%zu)\n", text, len );
    com_free( text );

    while(1) {
        long randMax = com_rand( 100 );
        flags = (com_actFlag_t){ false, false };
        com_valFunc_t valRand = {
            com_valDigit,
            &(com_valCondDigit_t){ .min = 1, .max = randMax }
        };
        com_input( &val, &valRand, &flags, "value(1-%d): ", randMax );
        if( com_askYesNo( false, "answer = %ld, OK? ", com_atol(val) ) ) {
            break;
        }
    }
}

// test_random() /////////////////////////////////////////////////////////////

// 動作については目視確認
void test_random( void )
{
    startFunc( __func__ );
    int succeed = 0;

    for( long i = 0;  i < 10;  i++ ) { com_printf( "%3ld ", com_rand( 20 ) ); }
    for( long i = 0;  i < 100; i++ ) {if( com_checkChance(50) ) { succeed++; }}
    com_printf( "\n succeed = %d\n", succeed );
}

// test_stat() ///////////////////////////////////////////////////////////////

static BOOL inputValue( com_calcStat_t *oStat, long iData )
{
    com_printf( " input -> %ld\n", iData );
    if( !com_inputStat( oStat, iData ) ) {
        com_printf( "   *** FAILED ***\n" );
        return false;
    }
    com_printf( "   total: %5.2f   sqtotal = %10.2f\n",
                oStat->total, oStat->sqtotal );
    return true;
}

#define EXPMEMBER( MEMBER ) \
    (iExpected ? iExpected->MEMBER : 0)

#define ASSERTFUNC( LABEL, EXPECTED, RESULT, FUNC, FORMAT ) \
    if( iExpected ) { FUNC( LABEL, (EXPECTED), (RESULT) ); } \
    else { com_printf( "%s: " FORMAT "\n", LABEL, (RESULT) ); } 

static void calcStat(
        long *iList, int iCount, com_calcStat_t *iExpected, BOOL iNeed )
{
    com_calcStat_t statInf;
    com_readyStat( &statInf, iNeed );
    for( int i = 0;  i < iCount;  i++ ) {
        if( !inputValue( &statInf, iList[i] ) ) { return; }
    }
    com_repeat( "-", 40, true );
    com_printf( " count = %3ld\n", statInf.count );
    if( statInf.needList ) {
        for( int i = 0;  i < statInf.count;  i++ ) {
            com_printf( "%ld ", statInf.list[i] );
        }
        com_printLf();
    }
    ASSERTFUNC( "    最大値",
      EXPMEMBER(max),      statInf.max, com_assertEquals, "%ld" );
    ASSERTFUNC( "    最小値",
      EXPMEMBER(min),      statInf.min, com_assertEquals, "%ld" );
    ASSERTFUNC( "算術平均値",
      EXPMEMBER(average),  statInf.average, com_assertEqualsD, "%f" );
    ASSERTFUNC( "幾何平均値",
      EXPMEMBER(geoavr),   statInf.geoavr, com_assertEqualsD, "%f" );
    ASSERTFUNC( "調和平均値",
      EXPMEMBER(hrmavr),   statInf.hrmavr, com_assertEqualsD, "%f" );
    ASSERTFUNC( "      分散",
      EXPMEMBER(variance), statInf.variance, com_assertEqualsD, "%f" );
    ASSERTFUNC( "  標準偏差",
      EXPMEMBER(stddev),   statInf.stddev, com_assertEqualsD, "%f" );
    ASSERTFUNC( "  変動係数",
      EXPMEMBER(coevar),   statInf.coevar, com_assertEqualsD, "%f" );
    ASSERTFUNC( "      歪度",
      EXPMEMBER(skewness), statInf.skewness, com_assertEqualsD, "%f" );
    ASSERTFUNC( "      尖度",
      EXPMEMBER(kurtosis), statInf.kurtosis, com_assertEqualsD, "%f" );
    com_repeat( "-", 40, true );
    com_finishStat( &statInf );
}

void test_stat( void )
{
    startFunc( __func__ );
    long data1[] = { 21, 16,  9, 43, 24, 30, 17, 12, 25, 28 };
    com_calcStat_t  stat1 = {
        .max = 43,  .min = 9,
        .average  = 22.5000000000000000,
        .geoavr   = 20.5405039527952340,
        .hrmavr   = 18.6095979871424362,
        .variance = 88.2500000000000000,
        .stddev   =  9.3941471140279678,
        .coevar   =  0.4175176495123541,
        .skewness =  0.6224115318589741,
        .kurtosis =  2.9379402771870473
    };
    long data2[] = { 30, 34,  9, 43, 35, 30, 11, 12,  8, 10 };
    com_calcStat_t  stat2 = {
        .max = 43,  .min = 8,
        .average  = 22.2000000000000000,
        .geoavr   = 18.3703078974651532,
        .hrmavr   = 15.1915838919724013,
        .variance = 161.1600000000000250,
        .stddev   = 12.6948808580466803,
        .coevar   =  0.5718414800921928,
        .skewness =  0.2073877259280199,
        .kurtosis =  1.3881690210604427
    };
    long data3[] = { 21, 26, 29, 23, 24, 20, 27, 22, 25, 28 };
    com_calcStat_t  stat3 = {
        .max = 29,  .min = 20,
        .average  = 24.5000000000000000,
        .geoavr   = 24.3301294651750055,
        .hrmavr   = 24.1595998370295284,
        .variance =  8.2500000000000000,
        .stddev   =  2.8722813232690143,
        .coevar   =  0.1172359723783271,
        .skewness =  0.0000000000000000,
        .kurtosis =  1.7757575757582600
    };
    long data4[] = {  9,  5,  2,  9,  5,  3,  7,  5 };
    com_calcStat_t  stat4 = {
        .max = 9,   .min = 2,
        .average  =  5.6250000000000000,
        .geoavr   =  5.0533635131234522,
        .hrmavr   =  4.4483671668137692,
        .variance =  5.7343750000000000,
        .stddev   =  2.3946555075835021,
        .coevar   =  0.4257165346815115,
        .skewness =  0.1186224676595500,
        .kurtosis =  1.8386430963181848
    };

    calcStat( data1, COM_ELMCNT( data1 ), &stat1, true );
    calcStat( data2, COM_ELMCNT( data2 ), &stat2, false );
    calcStat( data3, COM_ELMCNT( data3 ), &stat3, false );
    calcStat( data4, COM_ELMCNT( data4 ), &stat4, false );
    com_printf( " *DBL_MAX = %f\n", DBL_MAX );
}

// test_randStat() ///////////////////////////////////////////////////////////

// 動作については目視確認
void test_randStat( void )
{
    startFunc( __func__ );
    long randList[100];

    for( size_t i = 0;  i < COM_ELMCNT( randList );  i++ ) {
        randList[i] = com_rand(10000);
    }
    calcStat( randList, COM_ELMCNT( randList ), NULL, false );
}

// test_menu() ///////////////////////////////////////////////////////////////

static BOOL menuItem( long iCode )
{
    com_printf( "item%ld!\n", iCode );
    com_assertTrue( "code=1 or 3", ((iCode == 1) || (iCode == 3)) );
    return true;
}

static BOOL menuCancel( long iCode )
{
    com_assertEquals( "cancel code", 2, iCode );
    return false;   // 無条件で非表示にする
}

static com_selector_t gTestMenu[] = {
    { 1, "item1", NULL,       menuItem },
    { 2, "item2", menuCancel, menuItem },
    { 3, "item3", NULL,       menuItem },
    { 0, NULL, NULL, NULL }
};

void test_menu( void )
{
    startFunc( __func__ );
    com_selPrompt_t prompt = { NULL, "\n> ", 5, "##" };
    com_actFlag_t flags = { false, false };

    com_assertTrue( "com_execSelector()",
                    com_execSelector( gTestMenu, &prompt, &flags ) );
}

// test_searchPrime() ////////////////////////////////////////////////////////

static void searchPrime( uint iStart, uint iEnd )
{
    com_stopwatch_t swinf;
    struct timeval* tmp = &(swinf.passed);

    com_printf( "prime number (%u-%u)\n", iStart, iEnd );
    com_startStopwatch( &swinf );
    for( uint i = iStart;  i <= iEnd;  i++ ) {
        if( com_isPrime( i ) ) { com_printf( "%u ", i ); }
    }
    com_printLf();
    com_checkStopwatch( &swinf );
    com_printf( " passed: %ld.%06ldsec\n", tmp->tv_sec, tmp->tv_usec );
}

// 動作については目視確認
void test_searchPrime( void )
{
    startFunc( __func__ );
    searchPrime( 0, 5000 );
    searchPrime( 1000, 6000 );
    searchPrime( 1000000, 1005000 );
    searchPrime( 100000000, 100005000 );
}

// test_dice() ///////////////////////////////////////////////////////////////

static void dispDiceAvr( char *iLabel, double iExpected, double iAvr )
{
    com_printf( "%s = %f (expected:%f)\n", iLabel, iAvr, iExpected );
    com_assertTrue( "avarage", (fabs(iAvr - iExpected) < 0.1) );
}

void test_dice( void )
{
    startFunc( __func__ );
    const int trial = 100000;
    long sum1 = 0;
    long sum2 = 0;
    long sum3 = 0;
    BOOL disp = false;
    for( int i = 0;  i < trial;  i++ ) {
        disp = false;
        if( i % (trial / 10) == 0 ) {
            disp = true;
            com_printf( "--Trial %d--\n", i );
        }
        if( disp ) { com_printf( "*1 " ); }
        sum1 += com_rollDice( 2, 6, 2, disp );
        if( disp ) { com_printf( "*2 " ); }
        sum2 += com_rollDice( 2, 10, 0, disp );
        if( disp ) { com_printf( "*3 " ); }
        sum3 += com_rollDice( 1, 20, 0, disp );
    }
    com_printf( "---- average ----\n" );
    dispDiceAvr( "2d6+2", 9.0, (double)sum1 / trial );
    dispDiceAvr( "2d10", 11.0, (double)sum2 / trial );
    dispDiceAvr( "1d20", 10.5, (double)sum3 / trial );
}

// test_procPack() ///////////////////////////////////////////////////////////

// 読み書きどちらでもベースとなるデータパッケージ設定
static com_packInf_t gPackInf = {
    .filename = "./packTest4523",   .byText = true,
    .useZip = true,
    .archive = "packtest",  .key = "test",  .noZip = true,  .delZip = false
};

// 実際のデータ型
typedef struct {
    char*   name;
    long    age;
    long    score;
} testPack_t;

enum { ELMCNT = 3 };

static com_packElm_t *setElmList( testPack_t *iTarget )
{
    static com_packElm_t  elm[ELMCNT];
    if( iTarget->name ) {
        elm[0] = (com_packElm_t){
            iTarget->name, strlen(iTarget->name) + 1, true
        };
    }
    else {
        elm[0] = (com_packElm_t){ iTarget->name, 0, true };
    }
    elm[1] = (com_packElm_t){ &iTarget->age,   sizeof(iTarget->age),   false };
    elm[2] = (com_packElm_t){ &iTarget->score, sizeof(iTarget->score), false };
    return elm;
}

enum { FIX_TEXTLEN = 16 };

// ファイル保存時はデータ数を最初に書き込み、
// その後にデータを順番に書き出す。


// false で試験をしたい時はこれをコメントアウト
#define SET_VARSIZE_TRUE

static void writePack( com_packInf_t *ioInf, testPack_t *iList )
{
    for( size_t i = 0;  i < 3;  i++ ) {
        com_packElm_t* elmList = setElmList( &(iList[i]) );
        if( !com_writePack( ioInf, elmList, ELMCNT ) ) {return;}
    }

    char test[FIX_TEXTLEN] = "TEXT END";
#ifdef SET_VARSIZE_TRUE   // .varSizeを true設定する場合
    (void)com_writePackDirect( ioInf, test, strlen(test)+1, true );
#else                     // .varSizeを false設定する場合
    (void)com_writePackDirect( ioInf, test, sizeof(test), false );
#endif
}

static testPack_t gPackDataList[] = {
    {"ALPHA", 26, 70}, {"BETA", 32, 84}, {"GAMMA", 14, 40}
};

static void writePackFile( void )
{
    com_packInf_t  wInf = gPackInf;
    wInf.writeFile = true;
    if( !com_readyPack( &wInf, "same file exist, overwrite OK? " ) ) {return;}
    size_t num = COM_ELMCNT( gPackDataList );
    if( com_writePackDirect( &wInf, &num, sizeof(num), false ) ) {
        writePack( &wInf, gPackDataList );
    }
    com_finishPack( &wInf, true );
    com_assertEquals( "file created", 0,
                      com_system( "ls %s", gPackInf.archive ) );
}
// 作ったデータファイルは そのまま readPackFile()で読み出し確認する。

static void dispReadData( testPack_t *iData, size_t iNum ) 
{
    com_assertString( "  name", gPackDataList[iNum].name,  iData->name );
    com_assertEquals( "   age", gPackDataList[iNum].age,   iData->age );
    com_assertEquals( " score", gPackDataList[iNum].score, iData->score );
    com_free( iData->name );
    com_repeat( "-", 40, true );
}

static void readPack( com_packInf_t *ioInf ) {
    size_t num = 0;
    if( com_readPackFix( ioInf, &num, sizeof(num) ) ) {
        com_printf( "found %zd items\n", num );
        for( size_t i = 0;  i < num;  i++ ) {
            testPack_t testElm = { NULL, 0, 0 };
            com_packElm_t* elmList = setElmList( &testElm );
            size_t dmy;
            if( !com_readPackVar( ioInf, &(testElm.name), &dmy ) ) { return; }
            if( !com_readPack( ioInf, elmList + 1, 2 ) ) { return; }
            dispReadData( &testElm, i );
        }
    }
    char test[FIX_TEXTLEN] = {0};
    size_t testSize = sizeof(test);
#ifdef SET_VARSIZE_TRUE   // .varSizeを true設定する場合
    char* dmyAddr = test;
    (void)com_readPackVar( ioInf, &dmyAddr, &testSize );
#else                     // .varSizeを false設定する場合
    (void)com_readPackFix( ioInf, test, testSize );
#endif
    com_printf( "********* %s (%zu) **********\n", test, testSize );
}

// writePackFile() で作ったデータファイルを読み込む
// その後、作成したデータファイルは削除する
static void readPackFile( void )
{
    com_packInf_t rInf = gPackInf;
    if( !com_readyPack( &rInf, NULL ) ) { return; }
    readPack( &rInf );
    remove( rInf.archive );    // データファイルの削除
    com_assertNotEquals( "file removed", 0,
                         com_system( "ls %s", gPackInf.archive ) );
    com_finishPack( &rInf, true );
}

void test_procPack( void )
{
    startFunc( __func__ );
    writePackFile();
    readPackFile();
}

#define NMATCH_MAX  4

static void checkRegexec(
        char *iLabel, com_regex_id_t iId, com_regexec_t *ioExec,
        BOOL iExpected, char **iList )
{
    com_assertEquals( iLabel, iExpected, com_regexec( iId, ioExec ) );
    for( size_t i = 0;  i < NMATCH_MAX;  i++ ) {
        regmatch_t* tmp = &(ioExec->pmatch[i]);
        size_t len;
        com_printf( " %zu: (%ld:%ld)\n", i, tmp->rm_so, tmp->rm_eo );
        com_assertString( "  ", iList[i],
                          com_analyzeRegmatch( ioExec, i, &len) );
        com_assertEqualsU( "len", strlen(iList[i]), len );
    }
}

void test_regex( void )
{
    startFunc( __func__ );
    com_regcomp_t rcomp = { "((abc)+)([DE])", REG_EXTENDED | REG_ICASE };
    com_regexec_t rexec1;
    com_regexec_t rexec2;
    com_regexec_t rexec3;
    regmatch_t  tmppmatch[NMATCH_MAX];
    char *expected1[] = { "ABCabcD", "ABCabc", "abc", "D" };
    char *expected2[] = { "", "", "", "" };
    char *expected3[] = { "ABCE", "ABC", "ABC", "E" };

    com_regex_id_t  id = com_regcomp( &rcomp );
    com_assertEquals( "com_regcomp()", 0, id );

    com_assertTrue( "com_makeRegexec() 1", com_makeRegexec( &rexec1,
                            "ABCabcDbcABCDEFABZ", 0, NMATCH_MAX, NULL ) );
    com_assertTrue( "com_makeRegexec() 2", com_makeRegexec( &rexec2,
                            "ABZDEFABXABYDEFABZ", 0, NMATCH_MAX, NULL ) );
    com_assertTrue( "com_makeRegexec() 3", com_makeRegexec( &rexec3,
                            "ABZDEFABCEBYDEFABZ", 0, NMATCH_MAX, tmppmatch ) );

    checkRegexec( "com_regexec() 1", id, &rexec1, true,  expected1 );
    checkRegexec( "com_regexec() 2", id, &rexec2, false, expected2 );
    checkRegexec( "com_regexec() 3", id, &rexec3, true,  expected3 );

    com_freeRegexec( &rexec1 );
    com_freeRegexec( &rexec2 );
    // rexec3 は解放不要なので、com_freeRegexec()は使わない
}

#endif // USING_COM_EXTRA


#ifdef USING_COM_SELECT    // セレクト機能を使うテストコード

// test_convertAddr() ////////////////////////////////////////////////////////

static void procConvert(
        COM_SOCK_TYPE_t iType, int iFamily, const char *iAddr, ushort iPort,
        BOOL iExpected )
{
    static int count = 0;
    struct addrinfo* tmp = NULL;
    com_sockaddr_t test;
    count++;
    com_resetLastError();
    BOOL result = com_getaddrinfo( &tmp, iType, iFamily, iAddr, iPort );
    com_assertEquals( "result", iExpected, result );
    if( result ) {
        char *addr, *port;
        com_copyAddr( &test, tmp );
        com_getnameinfo( &addr, &port, &test.addr, 0 );
        com_printf( "[%s]:%s\n", addr, port );
        com_freeaddrinfo( &tmp );
    }
    else { com_assertEquals( "errno", COM_ERR_DEBUGNG, com_getLastError() ); }
}

void test_convertAddr( void )
{
    startFunc( __func__ );
    procConvert( COM_SOCK_UDP, AF_INET, "192.168.1.1", 12345, true );
    procConvert( COM_SOCK_TCP_CLIENT, AF_INET, "10.1.2.3", 34, true );
    procConvert( COM_SOCK_UDP, AF_INET, "192.3.56.78.123.3", 222, false );
    procConvert( COM_SOCK_TCP_SERVER, AF_INET6, "2001:aaaa:bbbb::1:2", 456, true );
    procConvert( COM_SOCK_UDP, AF_INET6, "2001::1::23:x", 222, false );
}

// test_ipAddr() /////////////////////////////////////////////////////////////

static void checkAddr( const char *iString, COM_AF_TYPE_t iExpected )
{
    COM_AF_TYPE_t type = com_isIpAddr( iString );

    com_printf( "%s", iString );
    if( type == AF_INET )   { com_printf( " ...IPv4 address\n" ); }
    if( type == COM_IPV6 )  { com_printf( " ...IPv6 address\n" ); }
    if( type == COM_NOTIP ) { com_printf( " ...not IP address\n" ); }
    com_assertEquals( "type", iExpected, type );
}

void test_ipAddr( void )
{
    startFunc( __func__ );
    checkAddr( "127.0.0.1", COM_IPV4 );
    checkAddr( "123.4", COM_NOTIP );
    checkAddr( "123.A", COM_NOTIP );
    checkAddr( "2001::1", COM_IPV6 );
    checkAddr( "1111:2222:3333:4444:5555:6666:7777:8888", COM_IPV6 );
    checkAddr( "111::222::333", COM_NOTIP );
    checkAddr( "ABC:DEF::GHI", COM_NOTIP );
}

// test_network() ////////////////////////////////////////////////////////////

static BOOL gIsServer = false;
static BOOL gIsTcp    = false;
static BOOL gIsV6     = false;
static BOOL gIsUNIX   = false;
static com_selectId_t gConnId = COM_NO_SOCK;
static com_selectId_t gAcptId = COM_NO_SOCK;

static struct addrinfo* gAddr1 = NULL;
static struct addrinfo* gAddr2 = NULL;

static char gV4Addr[] = "127.0.0.1";
static ushort gV4Port1 = 23456;
static ushort gV4Port2 = 23457;
static char gV6Addr[] = "::1";
static ushort gV6Port1 = 34567;
static ushort gV6Port2 = 34567;

static char* gUnixDomain1 = "./tmp_toscom_testnet1";
static char* gUnixDomain2 = "./tmp_toscom_testnet2";
static struct sockaddr_un gUDom1, gUDom2;

static void setTestAddr( void )
{
    if( gIsUNIX ) {
        gUDom1.sun_family = AF_UNIX;
        strcpy( gUDom1.sun_path, gUnixDomain1 );
        gUDom2.sun_family = AF_UNIX;
        strcpy( gUDom2.sun_path, gUnixDomain2 );
        return;
    }
    COM_SOCK_TYPE_t type = gIsTcp ? COM_SOCK_TCP_SERVER : COM_SOCK_UDP;
    if( gIsV6 ) {
        com_getaddrinfo( &gAddr1, type, AF_INET6, gV6Addr, gV6Port1 );
        com_getaddrinfo( &gAddr2, type, AF_INET6, gV6Addr, gV6Port2 );
    }
    else {
        com_getaddrinfo( &gAddr1, type, AF_INET,  gV4Addr, gV4Port1 );
        com_getaddrinfo( &gAddr2, type, AF_INET,  gV4Addr, gV4Port2 );
    }
}

static void freeAddr( void )
{
    com_freeaddrinfo( &gAddr1 );
    com_freeaddrinfo( &gAddr2 );
}

static void sendPacket( com_selectId_t iId, char *iData, size_t iDataSize,
                        com_sockaddr_t *iDst )
{
    if( !com_sendSocket( iId, iData, iDataSize, iDst ) ) { return; }
    com_printf( "<-- SEND[%ld]: %s (%zu)\n", iId, iData, iDataSize );
}

static void makeUnixDst( com_sockaddr_t *oDst, struct sockaddr_un *iDst )
{
    memset( oDst, 0, sizeof(*oDst) );
    oDst->len = (socklen_t)(SUN_LEN( iDst ));
    memcpy( &oDst->addr, iDst, (size_t)(oDst->len) );
}

static BOOL recvTest(
        com_selectId_t iId, COM_SOCK_EVENT_t iEvent,
        void *iData, size_t iDataSize )
{
    if( iEvent == COM_EVENT_ACCEPT ) {
        char HELLO[] = "HELLO";
        gAcptId = iId;
        sendPacket( gAcptId, HELLO, sizeof(HELLO), NULL );
        return true;
    }
    if( iEvent == COM_EVENT_CLOSE ) {
        if( gIsServer ) { gAcptId = COM_NO_SOCK; }
        else { gConnId = COM_NO_SOCK; }
        return true;
    }
    com_printf( "--> RECEIVE[%ld]: %s (%zu)\n", iId, (char*)iData, iDataSize );
    if( !gIsTcp && gIsServer ) {
        char text[] = "OK!";
        com_sockaddr_t* dst = NULL;
        com_sockaddr_t  tmp;
        if( !gIsUNIX ) { dst = com_getDstInf( gConnId ); }
        else { makeUnixDst( &tmp, &gUDom2 ); dst = &tmp; }
        sendPacket( gConnId, text, sizeof(text), dst );
    }
    return !gIsServer;
}

static BOOL sendTest( com_selectId_t iId )
{
    com_sockaddr_t dst;
    char sendData[] = "DATA SEND TEST!";
    com_printf( "*** SEND TIMER EXPIRED[%ld] ***\n", iId );
    if( !gIsUNIX ) { com_copyAddr( &dst, gAddr1 ); }
    else { makeUnixDst( &dst, &gUDom1 ); }
    sendPacket( gConnId, sendData, sizeof(sendData), &dst );
    return true;
}

static BOOL recvKey( char *iData, size_t iDataSize )
{
    com_printf( "### KEY IN ###\n" );
    com_printf( " %s (%zu)\n", iData, iDataSize );
    if( iData[0] == 'q' ) { return false; }
    return true;
}

static com_selectId_t gSendTimer = COM_NO_SOCK;  // 再接続時の再設定用
static com_selectId_t gQuitTimer = COM_NO_SOCK;  // 終了待ちの再設定用

// clientConnect()で reconnect()を指定する必要があるので、プロトタイプ宣言
static BOOL reconnect( com_selectId_t iId );

static void clientConnect( COM_SOCK_TYPE_t iType )
{
    if( !gIsUNIX ) {
        gConnId = com_createSocket( iType, gAddr2, NULL, recvTest, gAddr1 );
    }
    else {
        if( com_checkExistFile( gUnixDomain2 ) ) { remove( gUnixDomain2 ); }
        gConnId = com_createSocket( iType, &gUDom2, NULL, recvTest, &gUDom1 );
    }
    if( iType == COM_SOCK_UDP ) { return; }
    if( 0 > gConnId ) { (void)com_registerTimer( 2000, reconnect ); }
    else { gSendTimer = com_registerTimer( 3000, sendTest ); }
}

static BOOL reconnect( com_selectId_t iId )
{
    com_printf( "*** RECONNECT TIMER EXPIRED[%ld] ***", iId );
    clientConnect( COM_SOCK_TCP_CLIENT );
    com_resetTimer( gQuitTimer, 0 );
    return true;
}

static BOOL quitNow( com_selectId_t iId )
{
    com_printf( "*** QUIT TIMER EXPIRED[%ld] ***", iId );
    return false;
}

// データ通信とタイマー動作の確認
//   サーバーとクライアントで2つ起動する必要がある。
//   (同じディレクトリ上で動かすとログがおかしくなるので注意)
//
//  UDP接続時：
//   サーバー側を先に起動。
//   クライアント起動3秒後に "DATA SEND TEST!" 送信。
//   サーバーはそれを受信後即座に "OK!" を送信して処理終了。
//   クライアントは更に2秒後に処理終了。
//   どちらも q で始まる文字を入力したら即時終了。
//
//  TCP接続時
//   サーバー側を先に起動。
//   クライアント起動でTCP接続確立、サーバーから "HELLO" 送信
//   クライアント起動3秒後に "DATA SEND TEST!" 送信
//   サーバーはそれを受信後即座にTCP接続を切り、処理を終了。
//   クライアントは更に2秒後に処理終了。
//   どちらも q で始まる文字を入力したら即時終了。
//
//   もしサーバー起動前にクライアントを起動したら、TCP接続失敗となり、
//   2秒ごとに再接続を試み続ける。サーバーを起動すれば接続して動作する。
//
// 4/6 の代わりに UNIX とオプション指定すると、UNIXドメインソケットで接続する。
// (この場合も UDP/TCP は選択可能である)

void test_network( int iArgc, char **iArgv )
{
    startFunc( __func__ );
    COM_SOCK_TYPE_t type = COM_SOCK_UDP;

    // com_select.h 動作チェック用オプション
    if( iArgc < 5 ) { com_printf( "need s/c t/u 4/6/U\n" ); return; }
    gIsServer = com_compareString( iArgv[2], "s", 0, false );
    gIsTcp    = com_compareString( iArgv[3], "t", 0, false );
    gIsV6     = com_compareString( iArgv[4], "6", 0, false );
    gIsUNIX   = com_compareString( iArgv[4], "U", 0, false );
    setTestAddr();

    if( gIsServer ) {
        if( gIsTcp ) { type = COM_SOCK_TCP_SERVER; }
        (void)com_registerTimer( 100000, quitNow );
        if( !gIsUNIX ) {
            gConnId = com_createSocket( type, gAddr1, NULL, recvTest, gAddr2 );
        }
        else {
            if( com_checkExistFile( gUnixDomain1 ) ) { remove( gUnixDomain1 ); }
            gConnId = com_createSocket(type, &gUDom1, NULL, recvTest, NULL );
        }
        (void)com_registerStdin( recvKey );
        com_printf( "*** WAIT EVENTS (SERVER) ***\n" );
        while( com_waitEvent() ) { /* 待機中はやることなし */ }
        // TCP接続の場合、サーバー側からコネクションを切る動作とする
        if( gAcptId != COM_NO_SOCK ) { com_deleteSocket( gAcptId ); }
    }
    else {
        if( gIsTcp ) { type = COM_SOCK_TCP_CLIENT; }
        else { gSendTimer = com_registerTimer( 3000, sendTest ); }
        clientConnect( type );
        gQuitTimer = com_registerTimer( 5000, quitNow );
        (void)com_registerStdin( recvKey );
        com_printf( "*** WAIT EVENTS (CLIENT) ***\n" );
        while( com_waitEvent() ) { /* 待機中はやることなし */ }
    }
    com_dispDebugSelect();
    freeAddr();
}

// test_timer() //////////////////////////////////////////////////////////////

static BOOL testTimerFunc( com_selectId_t iId )
{
    static int count = 0;
    com_printf( "<<%ld timer expired [%d]>>\n", iId, count );
    count++;
    if( count < 5 ) {
        com_resetTimer( iId, 0 );
        return true;
    }
    count = 0;
    return false;
}

void test_timer( void )
{
    startFunc( __func__ );
    com_registerTimer( 1000, testTimerFunc );
    while( com_watchEvent() ) {}
    com_printf( "--- switch timer ---" );
    com_registerTimer(  500, testTimerFunc );
    while( com_checkTimer( COM_ALL_TIMER ) ) {}
}

// test_checkSize() //////////////////////////////////////////////////////////

// ネット用の構造体のサイズを目視確認
void test_checkSize( void )
{
    startFunc( __func__ );
    struct sockaddr_storage  ss;
    struct addrinfo  ai;
    struct sockaddr  sa;

    memset( &ss, 0, sizeof(ss) );
    memset( &ai, 0, sizeof(ai) );
    memset( &sa, 0, sizeof(sa) );

    com_printf( "%5zu : struct sockaddr_storage\n", sizeof(ss) );
    com_printf( "%5zu : struct addrinfo\n", sizeof(ai) );
    com_printf( "%5zu : struct sockaddr\n", sizeof(sa) );
}

// //test_ifinfo() ///////////////////////////////////////////////////////////

static void dispIfInf( long iCnt, com_ifinfo_t *iInf )
{
    for( long i = 0;  i < iCnt;  i++ ) {
        uchar* mac = iInf[i].hwaddr;
        com_printf( "%-16s  index=%d [%02x:%02x:%02x:%02x:%02x:%02x]\n",
                    iInf[i].ifname, iInf[i].ifindex,
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
        for( long j = 0;  j < iInf[i].cntAddrs;  j++ ) {
            com_ifaddr_t* soAddr = &(iInf[i].soAddrs[j]);
            struct sockaddr* sa = soAddr->addr;
            if( !sa ) {continue;}
            char* addr = NULL;
            if( com_getnameinfo( &addr, NULL, sa, 0 ) ) {
                com_printf( "   %s", addr );
                if( iInf[i].ifaddrs ) {
                    struct ifaddrs* ia = iInf[i].ifaddrs[j];
                    if( com_getnameinfo( &addr, NULL, ia->ifa_netmask, 0 ) ) {
                        com_printf( " (mask %s)", addr );
                    }
                }
                else if( soAddr->preflen ) {
                    com_printf( "/%ld", soAddr->preflen );
                }
                if( soAddr->label ) {
                    com_printf( "  label=%s", soAddr->label );
                }
                com_printLf();
            }
            else { com_printf( "  address unavailable\n" ); }
        }
    }
}

static com_ifinfo_t *getSomeIf( long iCnt, com_ifinfo_t *iInf )
{
    for( long i = 0;  i < iCnt;  i++ ) {
        if( !com_compareString( iInf[i].ifname, "lo", 0, false ) ) {
            return &(iInf[i]);
        }
    }
    return NULL;
}

// IF情報取得に Netlinkを使用する場合は true定義(Linuxのみ)
static BOOL gUseNetlink = false;

static void examSeekIf(
        char *iLabel, com_ifinfo_t *iTarget, int iFlags,
        com_seekIf_t *iCond, BOOL iExpected )
{
    com_ifinfo_t* result = com_seekIfInfo( iFlags, iCond, gUseNetlink );
    if( iExpected ) { com_assertNotNull( "result", result ); }
    else { com_assertNull( "result", result ); }
    if( result ) {
        com_printf( "--found ifindex=%d(%s)\n",
                    result->ifindex, result->ifname );
        if( result->ifindex == iTarget->ifindex ) {
            com_printf( "seek OK! (%s)\n", iLabel );
        }
        else { com_printf( "seek NG..(%s)\n", iLabel ); }
    }
    else { com_printf( "--cannot seek (%s)\n", iLabel ); }
}

void test_ifinfo( void )
{
    startFunc( __func__ );
    com_ifinfo_t* inf = NULL;
    long cnt = com_getIfInfo( &inf, gUseNetlink );  // gUseNetlinkの定義は上の方
    com_printf( "--- found interfaces (%ld) ---\n", cnt );
    if( cnt <= 0 ) { return; }
    dispIfInf( cnt, inf );
    com_ifinfo_t* target = getSomeIf( cnt, inf );
    if( !target ) { return; }
    examSeekIf( "ifname", target, COM_IF_NAME,
                &(com_seekIf_t){ .ifname = target->ifname }, true );
    examSeekIf( "ifindex", target, COM_IF_INDEX,
                &(com_seekIf_t){ .ifindex = target->ifindex }, true );
    examSeekIf( "ifname & ifindex", target, COM_IF_NAME | COM_IF_INDEX,
                &(com_seekIf_t){ .ifname = target->ifname,
                                 .ifindex = target->ifindex }, true );
    examSeekIf( "ipaddr(sa)", target, COM_IF_IPSA,
                &(com_seekIf_t){ .ipaddr = target->soAddrs[0].addr }, true );
    examSeekIf( "ipaddr(text) -> NG", target, COM_IF_IPTXT,
                &(com_seekIf_t){ .ipaddr = "255.255.255.2" }, false );
    examSeekIf( "hwaddr(bin)", target, COM_IF_HWBIN,
                &(com_seekIf_t){ .hwaddr = target->hwaddr }, true );
    examSeekIf( "hwaddr(text) -> NG", target, COM_IF_HWTXT,
                &(com_seekIf_t){ .hwaddr = "FF:FF:FF:F1:F2:F3" }, false );
}

void test_cksumRfc( void )
{
    startFunc( __func__ );
    uchar bindata[] = { 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
                        0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0 };
    ushort sum = com_cksumRfc( bindata, 16 );
    com_assertEqualsU( "sum", 0xfc7b, sum );
}

#ifdef USING_COM_SIGNAL1   // セレクト機能＋シグナル機能1を使うテストコード

#endif // USING_COM_SIGNAL1
#endif // USING_COM_SELECT

#ifdef USING_COM_WINDOW  // ウィンドウ機能テスト
#ifdef USING_COM_EXTRA   // シグナルハンドラーを使うためエキストラ機能も使用

// test_window() /////////////////////////////////////////////////////////////

static BOOL gWindowLoop = true;

#define WINPOS    &(com_winpos_t)

static void printSignum( int iSignum )
{
    com_mprintWindow( 0, WINPOS{0, -1}, A_REVERSE | A_UNDERLINE, false,
                      " signal: %-3d", iSignum );
}

static com_sigact_t gSigList[] = {
    { SIGINT,         { .sa_handler = printSignum } },  // Ctrl + c
    { SIGTSTP,        { .sa_handler = printSignum } },  // Ctrl + z
    { SIGUSR1,        { .sa_handler = printSignum } },
    { SIGUSR2,        { .sa_handler = printSignum } },
    { COM_SIGACT_END, { .sa_handler = NULL } }
};

static void wkey_up( com_winId_t iId )
{
    com_udlrCursor( iId, COM_CUR_UP, 1, NULL );
}

static void wkey_down( com_winId_t iId )
{
    com_udlrCursor( iId, COM_CUR_DOWN, 1, NULL );
}

static void wkey_left( com_winId_t iId )
{
    com_udlrCursor( iId, COM_CUR_LEFT, 1, NULL );
}

static void wkey_right( com_winId_t iId )
{
    com_udlrCursor( iId, COM_CUR_RIGHT, 1, NULL );
}

static void wkey_moveWindow( com_winId_t iId )
{
    int inkey = com_getLastKey();
    com_winpos_t pos;

    if( !com_getWindowPos( iId, &pos ) ) { return; }
    if( inkey == 'H' ) { (pos.x)--; }
    if( inkey == 'J' ) { (pos.y)++; }
    if( inkey == 'K' ) { (pos.y)--; }
    if( inkey == 'L' ) { (pos.x)++; }
    com_moveWindow( iId, &pos );
    //com_clearWindow( 0 );
}

static void wkey_now( com_winId_t iId )
{
    com_winpos_t cur;
    int color = 0;

    (void)com_getWindowCur( iId, &cur );
    if( iId >= 1 && iId <= 3 ) { color = COLOR_PAIR(iId); }
    com_printWindow( iId, color | (int)A_BOLD, false,
                     "(%d, %d)", cur.x, cur.y );
}

static com_winId_t gWinIdMax = 0;

static void wkey_switch( com_winId_t iId )
{
    BOOL reverse = ( com_getLastKey() == 'S' );
    while(1) {
        if( !reverse ) { iId++; if( iId > gWinIdMax ) { iId = 0; } }
        else { iId--; if( iId < 0 ) { iId = gWinIdMax; } }
        if( !com_existWindow( iId ) ) { continue; }
        if( com_activateWindow( iId ) ) { break; }
    }
}

static void wkey_delete( com_winId_t iId )
{
    if( iId ) { com_deleteWindow( iId ); }
}

static void wkey_quit( com_winId_t iId )
{
    COM_UNUSED( iId );
    gWindowLoop = false;
}

static void wkey_float( com_winId_t iId )
{
    com_floatWindow( iId );
}

static void wkey_sink( com_winId_t iId )
{
    com_sinkWindow( iId );
}

static void wkey_hide( com_winId_t iId )
{
    com_hideWindow( iId, true );
}

static void wkey_showAll( com_winId_t iId )
{
    COM_UNUSED( iId );
    for( int id = 0;  id <= gWinIdMax;  id++ ) {
        if( com_existWindow( id ) ) { com_hideWindow( id, false ); }
    }
    com_refreshWindow();
}

static void wkey_refresh( com_winId_t iId )
{
    com_activateWindow( iId );
}

static void wkey_clear( com_winId_t iId )
{
    com_clearWindow( iId );
}

static void wkey_fill( com_winId_t iId )
{
    com_fillWindow( iId, '*' );
}

static void wkey_escape( com_winId_t iId )
{
    com_mprintWindow( iId, WINPOS{-1, 0}, A_UNDERLINE, false,
                      "Window ID = %d", iId );
}

static void wkey_text( com_winId_t iId )
{
    char* text = NULL;
    com_winpos_t cur;
    com_wininopt_t opt = { .type = COM_IN_1LINE, .delay = 50,
                           .echo = true,  .clear = false };
    const com_cwin_t* win;
    
    com_getWindowCur( iId, &cur );
    opt.size = com_getRestSize( iId );
    com_getWindowInf( iId, &win );
    while( !com_inputWindow( iId, &text, &opt, &cur ) ) {}
}

static com_keymap_t gKeymapWin0[] = {
    { 'h', wkey_left },           // カーソルを左に移動
    { 'j', wkey_down },           // カーソルを下に移動
    { 'k', wkey_up },             // カーソルを上に移動
    { 'l', wkey_right },          // カーソルを右に移動
    { ' ', wkey_now },            // 現在のカーソル座標を表示
    { 's', wkey_switch },         // ウィンドウ切替
    { 'S', wkey_switch },         // ウィンドウ切替(逆方向)
    { 'r', wkey_refresh },        // ウィンドウ再描画
    { '/', wkey_showAll },        // ウィンドウ全表示
    { 'c', wkey_clear },          // ウィンドウ内クリア
    { 'q', wkey_quit },           // 終了
    { 0x1b, wkey_escape },        // ウィンドウID表示
    { KEY_F(12), wkey_text },     // テキスト入力
    { 0, NULL }
};

static com_keymap_t gKeymapWin1[] = {
    { KEY_LEFT, wkey_left },      // カーソルを左に移動
    { KEY_DOWN, wkey_down },      // カーソルを下に移動
    { KEY_UP, wkey_up },          // カーソルを上に移動
    { KEY_RIGHT, wkey_right },    // カーソルを右に移動
    { 'H', wkey_moveWindow },     // ウィンドウを左に移動
    { 'J', wkey_moveWindow },     // ウィンドウを下に移動
    { 'K', wkey_moveWindow },     // ウィンドウを上に移動
    { 'L', wkey_moveWindow },     // ウィンドウを右に移動
    { ' ', wkey_now },            // カーソルの現在座標を表示
    { 'u', wkey_float },          // ウィンドウを最上層に
    { 'U', wkey_float },          // ウィンドウを最上層に
    { 'n', wkey_sink },           // ウィンドウを最下層に
    { 'N', wkey_sink },           // ウィンドウを最下層に
    { '*', wkey_hide },           // ウィンドウを隠蔽
    { 'D', wkey_delete },         // ウィンドウを削除
    { 'F', wkey_fill },           // ウィンドウ内を * でフィル
    { 'q', NULL },                // q は何もしない
    { 'z', wkey_quit },           // 終了 (gKeymapWin0[]と違うキーになる)
    { 0, NULL }
};

typedef struct {
    com_winpos_t   start;
    com_winpos_t   size;
    BOOL           border;
} winData_t;

static winData_t gWinList[] = {
    { { 40,  2}, { 20, 10 }, true },
    { {  5,  8}, { 65,  8 }, true },
    { { 24,  0}, { 10, 20 }, false },
    { { -1, -1}, { -1, -1 }, true }   // 最後は必ずこれで
};

static com_colorPair_t  gColorList[] = {
    { 1, COLOR_RED,     COLOR_BLACK },
    { 2, COLOR_BLUE,    COLOR_WHITE },
    { 3, COLOR_YELLOW,  COLOR_BLACK },
    { 0,0,0 }   // 最後はこれで
};

static void createWindows( void )
{
    com_winopt_t opt = { .noEnter = true, .noEcho = true, .keypad = true,
                         .colors = gColorList };
    com_readyWindow( &opt, NULL );
    com_setBackgroundWindow( 0, ',' );
    com_setWindowKeymap( 0, gKeymapWin0 );

    for( int i = 0;  gWinList[i].start.x != -1;  i++ ) {
        winData_t* tmp = &(gWinList[i]);
        gWinIdMax = com_createWindow( &tmp->start, &tmp->size, tmp->border );
        if( gWinIdMax < 0 ) { com_printf( "FAILURE[%d]", i ); return; }
        com_setWindowKeymap( gWinIdMax, gKeymapWin1 );
        com_mixWindowKeymap( gWinIdMax, 0 );
    }
    com_setBackgroundWindow( 1, COLOR_PAIR(2) );
    // com_refreshWindow() は別のところで実施
}

static void testPrintWindow( void )
{
    char testText[] = "12345678901234567890123456789012345678901234567890"
                      "12345678901234567890123456789012345678901234567890";

    com_moveCursor( 0, WINPOS{72,22} );
    com_printWindow( 1, 0, false, testText );
    com_printWindow( 1, 0, true,  testText );

    com_mprintWindow( 0, WINPOS{0,0}, A_UNDERLINE, false,
                      "日本語出力テスト" );
    // stdscr(id = 0)のカーソル位置は (72,22) から変わらないはず

    com_refreshWindow();
    com_activateWindow( 1 );
}

void test_window( void )
{
    startFunc( __func__ );
    com_setDebugWindow( true );
    com_setSignalAction( gSigList );

    createWindows();
    testPrintWindow();
    while( gWindowLoop ) {
        if( !com_checkWindowKey( COM_ACTWIN, -1 ) ) {
            int inkey = com_getLastKey();
            if( inkey == ERR ) { continue; }
            com_mprintWindow( COM_ACTWIN, WINPOS{1,-1}, A_REVERSE, false,
                              "[%3d %c]", inkey, inkey );
        }
    }
    com_finishWindow();
    com_resumeSignalAction( COM_SIGACT_ALL );
}

#endif // USING_COM_EXTRA
#endif // USING_COM_WINDOW

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

static void exam_baseFunctions( int iArgc, char **iArgv )
{
    COM_UNUSED( iArgc );  COM_UNUSED( iArgv );   // 警告対策
    // テストしたい関数のコメントアウトを外して再ビルドする

    //test_realloct();                // テーブル捕捉解放
    //test_getOpt();                  // オプションチェック
    //test_strdup();                  // 文字列複製
    //test_convertString();           // 文字列変換
    //test_searchString();            // 文字列検索
    //test_replaceString();           // 文字列置換
    //test_seekText();                // 文字列行単位走査
    //test_seekFile();                // ファイル走査・コマンド実行結果走査
    //test_seekBin();                 // ファイルのバイナリ読み込み
    //test_removeDir();               // ディレクトリ追加と削除
    //test_seekDir( iArgc, iArgv );   // ディレクトリ使用量計算
    //test_seekDir2();                // ディレクトリ内容走査(軽量版)
    //test_scanDir( iArgc, iArgv );   // ディレクトリ走査
    //test_checkFiles();              // 複数ファイル有無チェック
    //test_printTag();                // タグ出力チェック
    //test_debugLog();                // デバッグログ出力
    //test_chainData();               // チェーン構造データ
    //test_chainNum();                // チェーン構造データ (数値版)
    //test_bufferData();              // 文字列バッファデータ
    //test_strop();                   // 文字列操作
    //test_checkString();             // 文字列チェック
    //test_checkDigit();              // 数値文字列チェック
    //test_getString();               // 文字列取得
    //test_getTime();                 // 日時取得
    //test_convertTime();             // 日時データ変換
    //test_archive();                 // アーカイブ操作
    //test_archive2();                // アーカイブ操作(複数ファイル)
    //test_strtoul();                 // 文字列数値変換
    //test_hash();                    // ハッシュテーブル
    //test_sortTable();               // ソートテーブル
    //test_stopwatch();               // 時間計測
    //test_dayOfWeek();               // 曜日計算
    //test_printEsc();                // エスケープコードの出力
    //test_fileInfo();                // ファイル情報取得
    //test_strtooct();                // バイナリテキストのバイナリ化
    //test_doublefree();              // 二重解放
    //test_prmNG();                   // パラメータNG処理
    //test_getFileFunc();             // ファイル名取得
    //test_ringBuffer();              // リングバッファ
    //test_config();                  // コンフィグ機能
    //test_assertion();               // アサート機能
}

static void exam_extraFunctions( void )
{
#ifdef USING_COM_EXTRA
    // テストしたい関数のコメントアウトを外して再ビルドする
    //
    //test_multiThread();             // マルチスレッド
        // マルチスレッドは基本機能だが、テストコードにエキストラ機能が必要
    //test_comInputVar();             // データ入力
    //test_random();                  // 乱数生成
    //test_stat();                    // 統計計算
    //test_randStat();                // 乱数統計計算
    //test_menu();                    // メニュー生成
    //test_searchPrime();             // 素数判定
    //test_dice();                    // ダイス判定
    //test_procPack();                // データ保存・読込
    //test_regex();                   // 正規表現
#endif // USING_COM_EXTRA
}

static void exam_selectFunctions( int iArgc, char **iArgv )
{
    COM_UNUSED( iArgc );  COM_UNUSED( iArgv );   // 警告対策
#ifdef USING_COM_SELECT
    // テストしたい関数のコメントアウトを外して再ビルドする

    //test_convertAddr();             // アドレスデータ変換
    //test_ipAddr();                  // IPアドレス文字列チェック
    //test_network( iArgc, iArgv );   // 通信とタイマーのテスト
    //test_timer();                   // 非同期タイマーのテスト
    //test_checkSize();               // 構造体サイズチェック
    //test_ifinfo();                  // IF情報取得
    //test_cksumRfc();                // チェックサム値計算
#ifdef USING_COM_SIGNAL1  // シグナル機能1も使う
    //test_rawSocket();               // パケット rawソケット動作
#endif // USING_COM_SIGNAL1
#endif // USING_COM_SELECT
}

static void exam_windowFunctions( void )
{
#ifdef USING_COM_WINDOW
#ifdef USING_COM_EXTRA   // エキストラ機能も使う(シグナルハンドラー操作のため)
    // テストしたい関数のコメントアウトを外して再ビルドする

    //test_window();                  // ウィンドウ機能テスト
#endif // USING_COM_EXTRA
#endif // USING_COM_WINDOW
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void com_testCode( int iArgc, char **iArgv )
{
    for( int i = 0;  i < iArgc;  i++ ) { com_printf( "%s\n", iArgv[i] ); }

    com_setDebugPrint( COM_DEBUG_ON );
    com_setWatchMemInfo( COM_DEBUG_ON );
    com_setWatchFileInfo( COM_DEBUG_ON );

    //com_noComDebugLog( true );
    //com_debugFileErrorOn( 0 );
    //com_debugMemoeyErrorOn( 0 );
    
    com_printTag( "=", 79, COM_PTAG_CENTER, "TEST START" );

    // テスト関数を追加するときは、最初の行に
    //     startFunc( __func__ );
    // と入れること。

    // 各機能のテスト関数実行
    exam_baseFunctions( iArgc, iArgv );
    exam_extraFunctions();
    exam_selectFunctions( iArgc, iArgv );
    exam_windowFunctions();

    com_printTag( "=", 79, COM_PTAG_CENTER, "TEST END" );

    exit( EXIT_SUCCESS );
}
#endif // USE_TESTFUNC

