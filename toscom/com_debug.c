/*
 *****************************************************************************
 *
 * デバッグ用共通処理 by TOS
 *   本ソースは com_proc.c とともに使用する必要がある。
 *
 *   本ソース内でのメモリ確保は com_malloc()を使わないケースがある。
 *   そのためそうしたメモリは確実に free()で解放すること。
 *
 *   ＊本ソース内で外部モジュールに公開する関数は com_if.h
 *     comモジュール内のみ公開する関数は com_debug.h にて
 *     プロトタイプ宣言と説明を記載しているので、使い方については
 *     このどちらかのヘッダファイルを必ず参照すること。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"

#ifdef USE_FUNCTRACE
#include <dlfcn.h>
#endif


// 初期化ステージ管理 --------------------------------------------------------

// 現在の初期化処理ステージ
static COM_INIT_STAGE_t  gInitStage = COM_INIT_STAGE_NOTEXEC;

// 基本機能の初期化完了フラグ
static BOOL gBaseInitFinished = false;

COM_INIT_STAGE_t com_getInitStage( void )
{
    return gInitStage;
}

void com_setInitStage( COM_INIT_STAGE_t iStage, BOOL iIsBase )
{
    if( iIsBase ) {
        if( iStage == COM_INIT_STAGE_FINISHED ) { gBaseInitFinished = true; }
    }
    else {
        if( !gBaseInitFinished && iStage == COM_INIT_STAGE_FINISHED ) {
            printf( "##### You need initialize base function\n" );
            return;
        }
    }
    gInitStage = iStage;
}

// デバッグログファイル関連 --------------------------------------------------

static FILE*  gDebugLog = NULL;        // デバッグログファイルポインタ

static char*  gLogFile = NULL;         // デバッグログファイル名
static char*  gDefaultLogFile = "." MAKEFILE_COMMAND ".log";
                                       // NULLに設定したら作成抑制の意味となる

void com_setLogFile( const char *iFileName )
{
    // デバッグログが既にオープンされているときは何もしない
    if( gDebugLog ) {return;}
    // デバッグログ抑制指定
    if( !iFileName ) {gDefaultLogFile = NULL;  return;}
    // デバッグ機能動作前に動くこともあり得るので、com_malloc()は使用しない
    if( !(gLogFile = malloc( strlen(iFileName) + 1 )) ) {
        com_errorExit( COM_ERR_NOMEMORY, "fail to store log file name" );
    }
    (void)com_strcpy( gLogFile, iFileName );
}

// タイトルライン書式
static char  gLogTitle[COM_LINEBUF_SIZE] =
    "\n##########   %s ver%s %-8s ##########\n";

void com_setTitleForm( const char *iTitle )
{
    if( !iTitle ) {return;}
    if( strlen( iTitle ) > (sizeof( gLogTitle ) - 1) ) {
        // ロギング機能動作前に呼ばれることを考慮し、標準関数使用
        printf( "com_setTitleForm() NG\n" );
        return;
    }
    (void)com_strcpy( gLogTitle, iTitle );
}

// エラー番号書式
static char  gLogErrorCode[COM_WORDBUF_SIZE] = "!%03ld! ";

void com_setErrorCodeForm( const char *iError )
{
    if( !iError ) {return;}
    if( strlen( iError ) > (sizeof( gLogErrorCode ) - 1) ) {
        // ロギング機能動作前に呼ばれることを考慮し、標準関数使用
        printf( "com_getErrorCodeForm() NG\n" );
        return;
    }
    (void)com_strcpy( gLogErrorCode, iError );
}

static BOOL  gNoComDebugLog = false;

void com_noComDebugLog( BOOL iMode )
{
    gNoComDebugLog = iMode;
}

// 既に同盟ログファイルが存在する場合は、その時点の日時を付けてリネーム
static void checkLogFile( void )
{
    if( !com_checkExistFile( gLogFile ) ) {return;}
    com_time_t data;
    com_getCurrentTime( COM_FORM_SIMPLE, NULL, NULL, &data );

    size_t  nameSize = strlen( gLogFile ) + 12;  // _ + 日付4桁 + 時刻6桁 + \0
    char*  tmp = malloc( nameSize );
    if( !tmp ) {
        com_errorExit( COM_ERR_NOMEMORY, "fail to modify log file name" );
    }
    snprintf( tmp, nameSize, "%s_%02lu%02lu%02lu%02lu%02lu", gLogFile,
              data.mon, data.day, data.hour, data.min, data.sec );
    if( 0 > rename( gLogFile, tmp ) ) {
        com_errorExit( COM_ERR_FILEDIRNG, "fail to rename log file" );
    }
    free( tmp );
}

static void openDebugLog( void )
{
    // ログファイルを作らない設定の場合は、何もせずに返る
    if( !gDefaultLogFile ) {return;}

    // ファイル名未指定時はデフォルト名をつける
    if( !gLogFile ) {
        // デバッグ機能動作前に動くこともあり得るので、com_malloc()は使用しない
        if( !(gLogFile = malloc( strlen(gDefaultLogFile) + 1 )) ) {
            // ファイル名のメモリすら確保できない場合は落とす
            com_errorExit( COM_ERR_NOMEMORY, "fail to make log file name" );
        }
        strcpy( gLogFile, gDefaultLogFile );
    }
    checkLogFile();
    // デバッグ機能動作前なので、com_fopen()は使用しない
    if( !(gDebugLog = fopen( gLogFile, "w" )) ) {
        // ファイルを開くことすら出来ない場合は落とす
        com_errorExit(COM_ERR_FILEDIRNG, "fail to open log file(%s)", gLogFile);
    }
}

void com_closeDebugLog( void )
{
    if( !gDebugLog ) {return;}
    // fopen()で開いているので、com_fclose()は使用しない
    fclose( gDebugLog );
    gDebugLog = NULL;
    // malloc()で確保しているので、com_free()は使用しない
    free( gLogFile );
    gLogFile = NULL;
}

void com_dispTitle( const char *iAdd )
{
    if( strlen( gLogTitle ) ) {
        com_printf( gLogTitle, com_getAplName(), com_getVersion(), iAdd );
        com_printLf();
    }
}


// 共通ログ出力処理 ----------------------------------------------------------

static pthread_mutex_t  gMutexOut = PTHREAD_MUTEX_INITIALIZER;
static char  gLogBuff[COM_DATABUF_SIZE];    // ログ出力用共通バッファ

#define MAKELOG( ... ) \
    snprintf( gLogBuff, sizeof(gLogBuff), __VA_ARGS__ )

static char  gWriteBuf[COM_DATABUF_SIZE];  // 書き込み用一時バッファ

// 画面クリア
static char  gClearLine[] = "***************************************"
                            "***************************************\n";

void com_clear( void )
{
    if( 0 > system( "clear" ) ) {return;}
    for( int i = 0; i < 3; i++ ) {com_printfLogOnly( "%s", gClearLine );}
}

static __thread BOOL  gSuspendStdout = false;

void com_suspendStdout( BOOL iMode )
{
    gSuspendStdout = iMode;
}

// デバッグログ用タイムスタンプ
typedef struct {
    char date[16];  // パディングの都合で COM_DATA_SSIZE の代わりに 16
    char time[16];  // パディングの都合で COM_DATA_SSIZE の代わりに 16
} stamp_t;

static void dispTimeStamp( FILE *ioFp, stamp_t *iStamp )
{
    if( iStamp ) {fprintf( ioFp, "[%s %s]", iStamp->date, iStamp->time );}
}

static void writeFile(
        FILE *ioFp, BOOL iLineTop, stamp_t *iStamp, char *iPrefixLabel )
{
    if( COM_UNLIKELY(!ioFp) ) {return;}    // 念の為 NULLチェックを入れておく
    if( iLineTop ) {
        dispTimeStamp( ioFp, iStamp );
        if( iPrefixLabel ) {fprintf( ioFp, "[%s] ", iPrefixLabel );}
        else if( iStamp ) {fprintf( ioFp, " " );}
    }
    // 改行なしの途中でもデバッグ出力はまず改行して出力する
    else if( iPrefixLabel ) {
        fprintf( ioFp, "\n" );
        dispTimeStamp( ioFp, iStamp );
        fprintf( ioFp, "[%s] ", iPrefixLabel );
    }
    fprintf( ioFp, "%s", gWriteBuf );
    fflush( ioFp );
}

// 画面出力先 (stdoutは定数ではないので、静的変数の初期化には使えない)
static FILE*  gOutput = NULL;

// 画面出力用諸元データ
typedef struct {
    long         mode;       // デバッグモード (COM_DEBUG_MODE_t)
    FILE*        fp;         // ログ出力先ファイルポインタ
    BOOL*        lineTop;    // 先頭行フラグ
    stamp_t      stamp;      // タイムスタンプ
    char*        prefix;     // プレフィックス
} outputInf_t;

static BOOL outputLine( com_seekFileResult_t *iInf )
{
    if( !gOutput ) {gOutput = stdout;}
    outputInf_t*  inf = iInf->userData;
    // 画面出力(デバッグ表示ONの場合のみ
    if( inf->mode == COM_DEBUG_ON  && !gSuspendStdout ) {
        writeFile( gOutput, *(inf->lineTop), NULL,          inf->prefix );
    }
    // ファイル出力(ファイルが開かれている時のみ)
    if( inf->fp ) {
        writeFile( inf->fp, *(inf->lineTop), &(inf->stamp), inf->prefix );
    }
    // 行頭フラグの更新
    *(inf->lineTop) = (strchr( gWriteBuf, '\n' ) != 0);
    return true;
}

// 付与プレフィックス
typedef enum {
    NO_PREFIX = 0,
    PREFIX_DBG,
    PREFIX_COM
} PREFIX_t;

static BOOL getDebugPrefix( PREFIX_t iPrefix, char **oPrefixLabel )
{
    if( iPrefix == PREFIX_DBG ) {*oPrefixLabel = "DBG";}
    if( iPrefix == PREFIX_COM ) {
        if( gNoComDebugLog ) {return false;}
        *oPrefixLabel = "COM";
    }
    return true;
}

static void printProc(
        COM_DEBUG_MODE_t iMode, PREFIX_t iPrefix, FILE *ioFp,
        const char *iText )
{
    static BOOL  lineTop = true;
    if( iMode == COM_DEBUG_OFF ) {return;}

    outputInf_t  outInf = { iMode, ioFp, &lineTop, {{0},{0}}, NULL };
    stamp_t*  stamp = &(outInf.stamp);
    com_getCurrentTime( COM_FORM_SIMPLE, stamp->date, stamp->time, NULL );
    if( !getDebugPrefix( iPrefix, &(outInf.prefix) ) ) {return;}
    // 改行コードごとに区切って1行ずつ出力
    (void)com_seekTextLine( iText, 0, true, outputLine, &outInf,
                            gWriteBuf, sizeof(gWriteBuf) );
}

// 通常の画面出力＆ログ出力
#define PRINT( TEXT ) \
    printProc( COM_DEBUG_ON, NO_PREFIX, gDebugLog, (TEXT) )

// 通常の画面出力のみ
#define PRINTOUT( TEXT ) \
    printProc( COM_DEBUG_ON, NO_PREFIX, NULL, (TEXT) )

// 通常のログ出力のみ
#define PRINTLOG( TEXT ) \
    printProc( COM_DEBUG_SILENT, NO_PREFIX, gDebugLog, (TEXT) )

#define DBGOUT_HEAD  COM_DEBUG_LOCKOFF( &gMutexOut, __func__ )
#define DBGOUT_TAIL  COM_DEBUG_UNLOCKON( &gMutexOut, __func__ )

#ifndef CHECK_PRINT_FORMAT    // make checkfを打った時に指定されるマクロ
void com_printf( const char *iFormat, ... )
{
    DBGOUT_HEAD;
    COM_SET_FORMAT( gLogBuff );
    // 無条件で画面出力＆ログ出力する
    PRINT( gLogBuff );
    DBGOUT_TAIL;
}
#endif

void com_repeat( const char *iSource, long iCount, BOOL iUseLf )
{
    if( COM_UNLIKELY(!iSource) ) {COM_PRMNG();}
    DBGOUT_HEAD;
    for( long i = 0;  i < iCount;  i++ ) {PRINT( iSource );}
    if( iUseLf ) {PRINT( "\n" );}
    DBGOUT_TAIL;
}

void com_printLf( void )
{
    com_setFuncTrace( false );
    com_printf( "\n" );
    com_setFuncTrace( true );
}

#ifndef CHECK_PRINT_FORMAT    // make checkfを打った時に指定されるマクロ
void com_printCr( const char *iFormat, ... )
{
    DBGOUT_HEAD;
    PRINTOUT( "\r" );         // 行頭復帰する
    PRINTLOG( "\n<CR>" );     // ログ出力の方のみ改行して CRタグを入れる
    // その後通常の出力
    COM_SET_FORMAT( gLogBuff );
    PRINT( gLogBuff );
    DBGOUT_TAIL;
}

void com_printBack( const char *iFormat, ... )
{
    DBGOUT_HEAD;
    COM_SET_FORMAT( gLogBuff );
    // 改行コードがあったら、そこで文字列は切断
    char*  tmp = strstr( gLogBuff, "\n" );
    if( tmp ) {*tmp = '\0';}
    PRINT( gLogBuff );

    // 出力文字数分カーソルを戻す(エスケープ文字があると厳しい)
    // この出力は画面のみで、ログ出力しないようにする
    size_t  back = strlen( gLogBuff );
    for( size_t i = 0;  i < back;  i++ ) {PRINTOUT( "\b" );}
    // ログ出力の方にのみ改行を入れる
    MAKELOG( "\n<b%zu>", back );
    PRINTLOG( gLogBuff );
    DBGOUT_TAIL;
}

static size_t calcTagLen(
        const char *iTag, size_t iWidth, COM_PTAG_POS_t iPos,
        const char *iLabel )
{
    size_t  labelLen = strlen( iLabel );
    uint isCenter = (uint)(iPos == COM_PTAG_CENTER);
    if( iWidth >= (isCenter + 1) ) { iWidth -= isCenter + 1; }
    if( iWidth <= labelLen ) {return 0;}
    size_t  tagWidth = iWidth - labelLen;
    size_t  result = tagWidth / strlen( iTag );
    if( isCenter && result > 1 ) {result /= 2;}
    return result;
}

static size_t printTag(
        const char *iTag, size_t iWidth, COM_PTAG_POS_t iPos,
        const char *iLabel, BOOL iIsLeft )
{
    size_t  tagLen = calcTagLen( iTag, iWidth, iPos, iLabel );
    if( !tagLen ) {return 0;}
    if( iIsLeft && iPos == COM_PTAG_LEFT ) {return 0;}
    if( !iIsLeft && iPos == COM_PTAG_RIGHT ) {return 0;}
    if( !iIsLeft ) {com_printf( " " );}
    com_repeat( iTag, (long)tagLen, false );
    if( iIsLeft ) {com_printf( " " );}
    return (tagLen * strlen(iTag) + 1);
}

static void printRestTag( const char *iTag, size_t iRest, COM_PTAG_POS_t iPos )
{
    if( iPos == COM_PTAG_RIGHT ) {
        for( size_t i = 0; i < iRest;  i++ ) { com_printf( " " ); }
        return;
    }
    size_t  tagLen = strlen( iTag );
    while( iRest > tagLen ) {com_printf( "%s", iTag );  iRest -= tagLen;}
    for( size_t i = 0;  i < iRest;  i++ ) {
        com_printf( "%c", iTag[i % tagLen] );
    }
}

void com_printTag(
        const char *iTag, size_t iWidth, COM_PTAG_POS_t iPos,
        const char *iFormat, ... )
{
    char  label[COM_LINEBUF_SIZE] = {0};
    if( COM_UNLIKELY(!iTag || !iFormat) ) {COM_PRMNG();}
    if( iPos < 0 || iPos > COM_PTAG_RIGHT ) {COM_PRMNG();}
    com_setFuncTrace( false );
    COM_SET_FORMAT( label );
    size_t  dispLen = printTag( iTag, iWidth, iPos, label, true );
    com_printf( "%s", label );
    dispLen += strlen( label );
    if( dispLen + strlen(iTag) + 1 < iWidth ) {
        dispLen += printTag( iTag, iWidth, iPos, label, false );
    }
    if( dispLen < iWidth ) {printRestTag( iTag, iWidth - dispLen, iPos );}
    com_printLf();
    com_setFuncTrace( true );
}
#endif   // CHECK_PRINT_FORMAT

static void setFlags( com_printBin_t *oFlags, com_printBin_t *iFlags )
{
    if( !iFlags ) {
        *oFlags = (com_printBin_t){
            .topSep = "-",  .prefix = NULL,  .suffix = NULL,
            .cols = 16,  .colSeq = 4,  .seq = " ",  .seqAscii = ": ",
            .onlyText = false
        };
        return;
    }
    *oFlags = *iFlags;
    if( !oFlags->cols )   {oFlags->cols = 16;}
    if( !oFlags->colSeq ) {oFlags->colSeq = 4;}
    if( !oFlags->seq )    {oFlags->seq = " ";}
}

static void dispTopSeparator( com_printBin_t *iFlags )
{
    size_t  count = iFlags->cols * 2 + iFlags->cols / iFlags->colSeq + 1;
    if( iFlags->prefix ) {count += strlen( iFlags->prefix );}
    if( iFlags->suffix ) {count += strlen( iFlags->suffix );}
    if( iFlags->seqAscii ) {count += strlen( iFlags->seqAscii ) + iFlags->cols;}
    for( size_t i = 0;  i < count;  i++ ) {PRINT( iFlags->topSep );}
    PRINT( "\n" );
}

static void dispBinary(
        size_t iCnt, const uchar *iBinary, size_t iLength,
        com_printBin_t *iFlags )
{
    for( size_t i = iCnt;  i < iCnt + iFlags->cols;  i++ ) {
        if( i < iLength ) {
            COM_BINTOHEX( tmpBin, iBinary[i], false );
            PRINT( tmpBin );
        }
        else {PRINT( "  " );}
        if( iFlags->colSeq == 1 ) {PRINT( iFlags->seq );}
        else if( i && !((i + 1) % iFlags->colSeq) ) {PRINT( iFlags->seq );}
    }
}

static void dispAscChr( int iCode )
{
    if( !isprint( (uchar)iCode ) ) {PRINT( "." );  return;}
    char  tmpChr[2] = {0};
    snprintf( tmpChr, sizeof(tmpChr), "%c", iCode );
    PRINT( tmpChr );
}

static void dispAscii(
        size_t iCnt, const uchar *iBinary, size_t iLength,
        com_printBin_t *iFlags )
{
    for( size_t i = iCnt;  i < iCnt + iFlags->cols;  i++ ) {
        if( i < iLength ) {dispAscChr( iBinary[i] );}
        else if( !iFlags->onlyText ) {PRINT( " " );}
    }
}

void com_printBinary(
        const void *iBinary, size_t iLength, com_printBin_t *iFlags )
{
    if( COM_UNLIKELY(!iBinary || !iLength) ) {COM_PRMNG();}
    com_mutexLock( &gMutexOut, __func__ );
    const uchar*  top = iBinary;
    com_printBin_t  flags;
    setFlags( &flags, iFlags );
    if( flags.topSep ) {dispTopSeparator( &flags );}
    for( size_t i = 0;  i < iLength;  i += flags.cols ) {
        if( flags.prefix ) {PRINT( flags.prefix );}
        if( !flags.onlyText ) {dispBinary( i, top, iLength, &flags );}
        if( flags.seqAscii ) {PRINT( flags.seqAscii );}
        if( flags.seqAscii || flags.onlyText ) {
            dispAscii( i, iBinary, iLength, &flags );
        }
        if( flags.suffix ) {PRINT( flags.suffix );}
        if( !flags.onlyText ) {PRINT( "\n" );}
    }
    if( flags.topSep ) {dispTopSeparator( &flags );}
    com_mutexUnlock( &gMutexOut, __func__ );
}

static void getLockAndForm(
        const char *iFunc, char *oBuf, size_t iBufSize,
        const char *iFormat, va_list iAp )
{
    com_mutexLock( &gMutexOut, iFunc );
    memset( oBuf, 0, iBufSize );
    if( iFormat ) {vsnprintf( oBuf, iBufSize, iFormat, iAp );}
}

#ifndef CHECK_PRINT_FORMAT    // make checkfを打った時に指定されるマクロ
static void dispLog( BOOL iCom, const char *iFormat, va_list iAp )
{
    getLockAndForm( __func__, gLogBuff, sizeof(gLogBuff), iFormat, iAp );
    PREFIX_t  prefix = iCom ? PREFIX_COM : NO_PREFIX;
    printProc( COM_DEBUG_SILENT, prefix, gDebugLog, gLogBuff );
    com_mutexUnlock( &gMutexOut, __func__ );
}

#define VADISP( FUNC, USECOM ) \
    do {\
        va_list  ap; \
        va_start( ap, iFormat ); \
        (FUNC)( (USECOM), iFormat, ap ); \
        va_end( ap ); \
    } while(0)

void com_printfLogOnly( const char *iFormat, ... )
{
    com_setFuncTrace( false );
    VADISP( dispLog, false );
    com_setFuncTrace( true );
}

void com_logCom( const char *iFormat, ... )
{
    com_setFuncTrace( false );
    VADISP( dispLog, true );
    com_setFuncTrace( true );
}

void com_printfDispOnly( const char *iFormat, ... )
{
    DBGOUT_HEAD;
    COM_SET_FORMAT( gLogBuff );
    // 画面出力のみする
    PRINTOUT( gLogBuff );
    DBGOUT_TAIL;
}
#endif   // CHECK_PRINT_FORMAT

// デバッグ表示設定 ----------------------------------------------------------

static void setDebugMode( COM_DEBUG_MODE_t *oMode, COM_DEBUG_MODE_t iValue )
{
    if( iValue > COM_DEBUG_SILENT ) {iValue = COM_DEBUG_SILENT;}
    *oMode = iValue;
}

static COM_DEBUG_MODE_t gPrintDebugMode = COM_DEBUG_OFF;

void com_setDebugPrint( COM_DEBUG_MODE_t iMode )
{
    setDebugMode( &gPrintDebugMode, iMode );
}

COM_DEBUG_MODE_t com_getDebugPrint( void )
{
    return gPrintDebugMode;
}

static void debugCom( BOOL iCom )
{
    (void)com_strcat( gLogBuff, "\n" );
    PREFIX_t  prefix = iCom ? PREFIX_COM : PREFIX_DBG;
    printProc( gPrintDebugMode, prefix, gDebugLog, gLogBuff );
    gLogBuff[0] = '\0';
}

#ifndef CHECK_PRINT_FORMAT    // make checkfを打った時に指定されるマクロ
static void dispDebug( BOOL iCom, const char *iFormat, va_list iAp )
{
    if( gPrintDebugMode == COM_DEBUG_OFF ) {return;}
    getLockAndForm( __func__, gLogBuff, sizeof(gLogBuff), iFormat, iAp );
    debugCom( iCom );
    com_mutexUnlock( &gMutexOut, __func__ );
}

void com_debug( const char *iFormat, ... )
{
    com_setFuncTrace( false );
    VADISP( dispDebug, false );
    com_setFuncTrace( true );
}

void com_dbgCom( const char *iFormat, ... )
{
    com_setFuncTrace( false );
    VADISP( dispDebug, true );
    com_setFuncTrace( true );
}
#endif   // CHECK_PRINT_FORMAT

static char gFuncBuff[COM_LINEBUF_SIZE];

static void dispFunc( BOOL iCom, COM_FILEPRM, const char *iFormat, va_list iAp )
{
    if( gPrintDebugMode == COM_DEBUG_OFF ) {return;}
    getLockAndForm( __func__, gFuncBuff, sizeof(gFuncBuff), iFormat, iAp );
    MAKELOG( "%s() %s   <%s:line %ld>", iFUNC, gFuncBuff, iFILE, iLINE );
    debugCom( iCom );
    com_mutexUnlock( &gMutexOut, __func__ );
}

#define VADISPFUNC( USECOM ) \
    do {\
        va_list  ap; \
        va_start( ap, iFormat ); \
        dispFunc( (USECOM), COM_FILEVAR, iFormat, ap ); \
        va_end( ap ); \
    } while(0)

void com_debugFuncWrap( COM_FILEPRM, const char *iFormat, ... )
{
    com_setFuncTrace( false );
    VADISPFUNC( false );
    com_setFuncTrace( true );
}

void com_dbgFuncComWrap( COM_FILEPRM, const char *iFormat, ... )
{
    com_setFuncTrace( false );
    VADISPFUNC( true );
    com_setFuncTrace( true );
}

static void dispDump(
        BOOL iCom, const void *iAddr, size_t iSize,
        const char *iFormat, va_list iAp )
{
    if( gPrintDebugMode == COM_DEBUG_OFF ) {return;}
    getLockAndForm( __func__, gLogBuff, sizeof(gLogBuff), iFormat, iAp );
    if( !iFormat ) {MAKELOG( "dump %8p (size=%zu)", iAddr, iSize );}
    debugCom( iCom );
    if( iSize > 0 ) {
        size_t  cnt;  // for文外でも使うので、for文の外で定義
        for( cnt = 0;  cnt < iSize;  cnt++ ) {
            com_connectString( gLogBuff, sizeof(gLogBuff),
                               " %02x", ((uchar*)iAddr)[cnt] );
            if( (cnt + 1) % 16 == 0 ) {debugCom( iCom );}
        }
        if( cnt % 16 ) {debugCom( iCom );}
    }
    com_mutexUnlock( &gMutexOut, __func__ );
}

#define VADISPDUMP( USECOM ) \
    do {\
        va_list  ap; \
        va_start( ap, iFormat ); \
        dispDump( (USECOM), iAddr, iSize, iFormat, ap ); \
        va_end( ap ); \
    } while(0)

void com_dump( const void *iAddr, size_t iSize, const char *iFormat, ... )
{
    com_setFuncTrace( false );
    VADISPDUMP( false );
    com_setFuncTrace( true );
}

void com_dumpCom( const void *iAddr, size_t iSize, const char *iFormat, ... )
{
    com_setFuncTrace( false );
    VADISPDUMP( true );
    com_setFuncTrace( true );
}


// エラーメッセージ処理 ------------------------------------------------------

static pthread_mutex_t  gMutexError = PTHREAD_MUTEX_INITIALIZER;
static BOOL  gNotProcError = false;
static BOOL  gCountError = false;

typedef struct {
    long      code;    // エラーコード
    ulong     count;   // 発生カウント
    char*     name;    // 出力用ラベル
} dbgErrCount_t;

static com_sortTable_t  gErrorTable = {0, COM_SORT_SKIP, 0, NULL, NULL};
static BOOL  gOccuredError = false;   // エラー発生有無フラグ

void com_registerErrorCode( const com_dbgErrName_t *iList )
{
    if( COM_UNLIKELY(!iList) ) {return;}
    com_mutexLockCom( &gMutexError, COM_FILELOC );
    BOOL  collision;
    for( ;  iList->code != COM_ERR_END;  iList++ ) {
        dbgErrCount_t errData = { iList->code, 0, iList->name };
        if( !com_addSortTableByKey( &gErrorTable, iList->code,
                                    &errData, sizeof(errData), &collision ) )
        {
            com_exit( COM_ERR_NOMEMORY );
        }
        if( collision ) {
            com_printf( "error code(%ld) already exist", iList->code );
        }
    }
    // 一つでも登録できたら以後エラーカウント処理実施
    gCountError = true;
    (void)com_mutexUnlockCom( &gMutexError, COM_FILELOC, true );
}

ulong com_getErrorCount( long iCode )
{
    COM_DEBUG_LOCKOFF( &gMutexError, __func__ );
    ulong  errCount = 0;
    com_sort_t**  srchData;
    if( com_searchSortTableByKey( &gErrorTable, iCode, &srchData ) ) {
        dbgErrCount_t* err = srchData[0]->data;
        errCount = err->count;
    }
    COM_DEBUG_UNLOCKON( &gMutexError, __func__ );
    return errCount;
}

void com_outputErrorCount( void )
{
    COM_DEBUG_LOCKOFF( &gMutexError, __func__ );
    PRINT( "\n### Error Count ###\n" );
    for( long i = 0;  i < gErrorTable.count;  i++ ) {
        dbgErrCount_t* tmp = gErrorTable.table[i].data;
        if( !tmp->count ) {continue;}
        COM_CLEAR_BUF( gLogBuff );
        MAKELOG( " %10ld [%03ld:%s]\n", tmp->count, tmp->code, tmp->name );
        PRINT( gLogBuff );
    }
    COM_DEBUG_UNLOCKON( &gMutexError, __func__ );
}

void com_adjustError( void )
{
    if( gPrintDebugMode != COM_DEBUG_OFF ) {
        // エラー発生時のみエラー情報を出力する
        if( gOccuredError ) {com_outputErrorCount();}
    }
    com_setFuncTrace( false );
    gNotProcError = true;   // 以後エラーが出てもカウントしない
    com_skipMemInfo( true );
    com_freeSortTable( &gErrorTable );
    com_skipMemInfo( false );
    com_setFuncTrace( true );
}

// 最新エラーコード
static __thread long  gLastErrorCode = COM_NO_ERROR;

long com_getLastError( void )
{
    return gLastErrorCode;
}

void com_resetLastError( void )
{
    gLastErrorCode = COM_NO_ERROR;
}

static __thread BOOL  gErrDisp = true;
static __thread BOOL  gErrLog  = true;

void com_notifyError( BOOL iDisp, BOOL iLog )
{
    gErrDisp = iDisp;
    gErrLog  = iLog;
}

static __thread BOOL  gUseStderr = false;

void com_useStderr( BOOL iUse )
{
    gUseStderr = iUse;
}

static com_hookErrCB_t  gHookErrorCB = NULL;

void com_hookError( com_hookErrCB_t iFunc )
{
    gHookErrorCB = iFunc;
}

static COM_HOOKERR_ACTION_t hookExec( long iCode, char *iBuf, COM_FILEPRM )
{
    if( !gHookErrorCB ) {return COM_HOOKERR_EXEC;}
    com_hookErrInf_t  inf = { iCode, iBuf, COM_FILEVAR };
    return gHookErrorCB( &inf );
}

static void countError( long iCode )
{
    if( !gCountError ) {return;}
    com_sort_t** srchData;
    if( com_searchSortTableByKey( &gErrorTable, iCode, &srchData ) ) {
        dbgErrCount_t* err = srchData[0]->data;
        (err->count)++;
    }
    else {com_debug( "!! fail to count error code(%ld)", iCode );}
    gOccuredError = true;
}

// 出力書式文字列のエラーログ編集処理マクロ
#define SET_ERRORLOG \
    do {\
        va_list  ap; \
        COM_CLEAR_BUF( gErrorLogBuf ); \
        snprintf( gErrorLogBuf, sizeof(gErrorLogBuf), gLogErrorCode, iCode );\
        va_start( ap, iFormat ); \
        vsnprintf( gErrorLogBuf + strlen(gErrorLogBuf), \
            sizeof(gErrorLogBuf) - strlen(gErrorLogBuf) - 1, iFormat, ap ); \
        va_end( ap ); \
        (void)com_strcat( gErrorLogBuf, "\n" ); \
    } while(0)

static char  gErrorLogBuf[COM_LINEBUF_SIZE];

static void branchErrorOutput( char *iBuf, COM_DEBUG_MODE_t iDispMode )
{
    FILE* logTarget = gErrLog ? gDebugLog : NULL;
    // 画面出力なし＆ログ出力なし…なら、無駄なので何もせずにリターン
    if( iDispMode != COM_DEBUG_ON && !logTarget ) {return;}
    printProc( iDispMode, NO_PREFIX, logTarget, iBuf );
}

static void outputErrorMessage( COM_FILEPRM )
{
    // まず生成したエラーメッセージの出力
    COM_DEBUG_MODE_t dispMode = gErrDisp ? COM_DEBUG_ON : COM_DEBUG_SILENT;
    // エラー出力は gPrintDebugModeの内容に関わらず出力していたが
    // gErrDispを falseにした場合は、それすらも抑制することとする。
    branchErrorOutput( gErrorLogBuf, dispMode );
    // 続いてエラー発生箇所の出力(元々 gPrintDebugMode で出力可否が決まる）
    dispMode = gPrintDebugMode;
    if( dispMode == COM_DEBUG_OFF ) {return;}
    COM_CLEAR_BUF( gLogBuff );
    MAKELOG( " in %s:line %ld %s()\n", COM_FILEVAR );
    if( !gErrDisp ) {dispMode = COM_DEBUG_SILENT;}
    // gPrintDebugModeが COM_DEBUG_SILENT なら gErrDispが trueでも そのまま
    branchErrorOutput( gLogBuff, dispMode );
}

static void setErrorInf( long iCode )
{
    gLastErrorCode = iCode;
    countError( iCode );
    gOutput = stdout;
}

static void onlyError( long iCode, COM_FILEPRM )
{
    printf( "##### ERROR OCCURED (code=%ld)\n", iCode );
    printf( "#####   in %s:line %ld %s()\n", iFILE, iLINE, iFUNC );
    printf( "#####   (warning: initialization " );
    if( com_getInitStage() == COM_INIT_STAGE_PROCCESSING ) {
        printf( "is proccessing)\n" );
    }
    if( com_getInitStage() == COM_INIT_STAGE_NOTEXEC ) {
        printf( "is not executed)\n" );
    }
}

void com_errorFunc(
        long iCode, BOOL iReturn, COM_FILEPRM, const char *iFormat, ... )
{
    if( gNotProcError ) {return;}
    if( gInitStage != COM_INIT_STAGE_FINISHED ) {
        onlyError( iCode, COM_FILEVAR );
        return;
    }
    COM_DEBUG_LOCKOFF( &gMutexError, __func__ );
    SET_ERRORLOG;
    COM_HOOKERR_ACTION_t hookAct = hookExec( iCode, gErrorLogBuf, COM_FILEVAR );
    if( hookAct == COM_HOOKERR_SKIP ) {
        setErrorInf( iCode );
        COM_DEBUG_UNLOCKON( &gMutexError, __func__ );
        return;
    }
    if( gUseStderr ) {gOutput = stderr;}
    if( hookAct != COM_HOOKERR_SILENT ) { outputErrorMessage( COM_FILEVAR ); }
    setErrorInf( iCode );
    COM_DEBUG_UNLOCKON( &gMutexError, __func__ );
    if( !iReturn ) { com_exitFunc( iCode, COM_FILEVAR ); }
}

static __thread char  gStrerrBuf[COM_LINEBUF_SIZE];

char *com_strerror( int iErrno )
{
    COM_CLEAR_BUF( gStrerrBuf );
#ifndef  _GNU_SOURCE   // POSIX仕様と GNU仕様で 返り値が変わる
    int  result = strerror_r( iErrno, gStrerrBuf, sizeof(gStrerrBuf) );
    if( !result ) {return gStrerrBuf; }
#else
    char*  result = strerror_r( iErrno, gStrerrBuf, sizeof(gStrerrBuf) );
    if( result ) {return result;}
#endif
    return "<strerror_r NG>";
}



// デバッグ監視機能共通処理 --------------------------------------------------

// デバッグ監視共通データ
typedef struct watchInfo {
    long          type;
    long          seqno;
    const void*   ptr;
    size_t        size;
    char          label[COM_DEBUGINFO_LABEL]; // マクロ宣言は com_custom.h
    struct watchInfo*  prev;
    struct watchInfo*  next;
    char          file[COM_DEBUGINFO_LABEL];  // 呼び元ファイル名
    long          line;                       // 呼び元ライン数
    char          func[COM_DEBUGINFO_LABEL];  // 呼び元関数名
} watchInfo_t;

// デバッグ監視情報グループ
typedef struct {
    watchInfo_t*   top;       // 先頭データ
    watchInfo_t*   last;      // 最終データ
    watchInfo_t**  ptrList;   // ptrリスト(ptrでソートしたデータ)
    long           seqno;     // シーケンス番号
    size_t         count;     // 現存データ数
    size_t         using;     // 使用量
    size_t         usingMax;  // 最大使用量
    long           err;       // エラー
} watchGroup_t;

// 監視情報のためのメモリ捕捉が失敗した場合、このフラグを trueにし、
// その旨のエラー表示はするが、監視機能自体はその後も動作させる。
// ただ情報の欠落は発生するため、その信頼度は落ちる。
static BOOL  gMemoryFailure = false;

static inline void setLabel( const char *iLabel, char *oTarget, size_t iSize )
{
    if( !iLabel ) {return;}
    (void)com_strncpy( oTarget, iSize, iLabel, strlen(iLabel) );
    // 改行コードは空白に変換して保持する
    char*  tmp = NULL;
    while( (tmp = strstr( oTarget, "\n" )) ) {*tmp = ' ';}
}

#define SET_LABEL( LABEL, TARGET ) \
    setLabel( (LABEL), (TARGET), sizeof(TARGET) )

static inline void setWatchInfo(
        watchInfo_t *oTarget, const char *iLabel, COM_FILEPRM )
{
    SET_LABEL( iLabel, oTarget->label );
    SET_LABEL( iFILE,  oTarget->file );
    oTarget->line = iLINE;
    SET_LABEL( iFUNC,  oTarget->func );
}

static inline long increaseSeqno( watchGroup_t *ioGrp )
{
    ioGrp->seqno = (ioGrp->seqno % COM_DEBUG_SEQNO_MAX) + 1;
    return ioGrp->seqno;
}

// 監視情報を watchGroup_t型に集約。
// .top から始まる線形リストは追加時は末尾追加のみの双方向リストに変更。
// 更に .ptrList に 各要素の .ptr でソートした要素リストを作成するように修正。
//
// 以後 .ptrの検索をしたい時は .ptrList の方で検索して、要素のアドレスを受け
// そのアドレスで処理を進めることとした。二分探索で検索できるので、
// 単純な線形リストよりは、検索が速くなるはず。

#define IDXVAL( INDEX )   (const void*)ioGrp->ptrList[*oIdx + INDEX]->ptr

static void adjustIndex(
        watchGroup_t *ioGrp, const void *iPtr, size_t *oIdx )
{
    while( (*oIdx > 0) && (IDXVAL(0) > iPtr) ) {(*oIdx)--;}
    while( (*oIdx < ioGrp->count - 1) && IDXVAL(0) < iPtr ) {(*oIdx)++;}
}

// ioGrp->count は既に変更後の値であり、
// ioGrp->ptrListの要素数とは一致しない。そのため、iSizeで要素数を受け取る
//
static BOOL binSearch(
        watchGroup_t *ioGrp, const void *iPtr, size_t *oIdx, size_t iSize )
{
    if( !(ioGrp->ptrList) ) {*oIdx = 0; return false;}
    size_t binMin = 0;
    size_t binMax = iSize - 1;
    *oIdx = iSize / 2;
    while(1) {
        if( iPtr > IDXVAL(0) ) {
            if( *oIdx == iSize - 1 ) {break;}
            binMin = *oIdx + 1;
        }
        else if( iPtr < IDXVAL(0) ) {
            if( *oIdx == 0 ) {break;}
            binMax = *oIdx - 1;
        }
        else {return true;}
        if( binMin > binMax ) {break;}
        *oIdx = binMin + (binMax - binMin) / 2;
    }
    adjustIndex( ioGrp, iPtr, oIdx );
    return false;
}

static BOOL reallocInner( watchGroup_t *ioGrp )
{
    if( !(ioGrp->count) ) {
        free( ioGrp->ptrList );
        ioGrp->ptrList = NULL;
        return true;
    }
    void* tmp = realloc( ioGrp->ptrList, sizeof(void*) * ioGrp->count );
    if( tmp ) {ioGrp->ptrList = tmp;  return true;}
    com_error( COM_ERR_DEBUGNG, "##### watch list realloc failure #####" );
    return false;
}

static void addWatchTable( watchGroup_t *ioGrp, watchInfo_t *iNew )
{
    size_t  index = 0;
    if( binSearch( ioGrp, iNew->ptr, &index, ioGrp->count - 1 ) ) {
        com_error( COM_ERR_DEBUGNG,
              "##### same address(%p) already registered #####", iNew->ptr );
        return;
    }
    if( !reallocInner( ioGrp ) ) {return;}
    if( index < ioGrp->count - 1 ) {
        watchInfo_t**  base = ioGrp->ptrList + index;
        memmove( base + 1, base,
                 sizeof(watchInfo_t*) * (ioGrp->count - index - 1) );
    }
    ioGrp->ptrList[index] = iNew;
}

static void addWatchList( watchGroup_t *ioGrp, watchInfo_t *ioNew )
{
    if( ioGrp->top ) {
        ioNew->prev = ioGrp->last;
        (ioGrp->last)->next = ioNew;
    }
    else { ioGrp->top = ioNew; }
    ioGrp->last = ioNew;
    (ioGrp->count)++;
    addWatchTable( ioGrp, ioNew );
}

static BOOL addWatchInfo(
        watchGroup_t *ioGrp, watchInfo_t **oNew, long iType,
        const void *iPtr, size_t iSize, const char *iLabel, COM_FILEPRM )
{
    watchInfo_t*  newInfo = malloc( sizeof(watchInfo_t) );
    if( !newInfo || ioGrp->count == COM_DEBUG_SEQNO_MAX ) {
        com_error( COM_ERR_DEBUGNG,
                   "##### fail to create new watchInfo(%s) #####", iLabel );
        gMemoryFailure = true;
        if( newInfo ) {free( newInfo );}
        return false;
    }
    *newInfo = (watchInfo_t){ iType, increaseSeqno(ioGrp),
                              iPtr, iSize, "", NULL, NULL, "", 0, "" };
    setWatchInfo( newInfo, iLabel, COM_FILEVAR );
    addWatchList( ioGrp, newInfo );
    *oNew = newInfo;
    return true;
}

static void updateUsing( watchGroup_t *ioGrp, size_t iAdd )
{
    ioGrp->using += iAdd;
    if( ioGrp->using > ioGrp->usingMax ) { ioGrp->usingMax = ioGrp->using; }
}

enum { NO_DEBUG_INFO = -1 };

static BOOL searchWatchList(
        watchGroup_t *iGrp, const void *iPtr, watchInfo_t **oCur )
{
    size_t  index = 0;
    if( binSearch( iGrp, iPtr, &index, iGrp->count ) ) {
        *oCur = iGrp->ptrList[index];
        return true;
    }
    *oCur = NULL;
    return false;
}

static long checkWatchInfo( watchGroup_t *iGrp, const void *iPtr )
{
    watchInfo_t* tmp;
    if( searchWatchList( iGrp, iPtr, &tmp ) ) {return tmp->seqno;}
    return NO_DEBUG_INFO;
}

static long getSeqno( watchGroup_t *iGrp, const void *iPtr )
{
    long  result = checkWatchInfo( iGrp, iPtr );
    if( result == NO_DEBUG_INFO ) {return 0;}
    return result;
}

static void deleteWatchTable( watchGroup_t *ioGrp, watchInfo_t *iCur )
{
    size_t  index = 0;
    if( !binSearch( ioGrp, iCur->ptr, &index, ioGrp->count + 1 ) ) {
        com_error( COM_ERR_DEBUGNG,
              "##### address(%p) not registered #####", iCur->ptr );
        return;
    }
    if( index < ioGrp->count ) {
        watchInfo_t**  base = ioGrp->ptrList + index;
        memmove( base, base + 1,
                 sizeof(watchInfo_t*) * (ioGrp->count - index) );
    }
    ioGrp->ptrList[ioGrp->count] = NULL;
    (void)reallocInner( ioGrp );
}

static void deleteWatchList( watchGroup_t *ioGrp, watchInfo_t *ioCur )
{
    if( !(ioCur->prev) ) {
        ioGrp->top = ioCur->next;
        if( ioCur->next ) { ioCur->next->prev = NULL; }
    }
    else {
        ioCur->prev->next = ioCur->next;
    }
    if ( ioGrp->last == ioCur ) {
        if( !(ioCur->prev) ) { ioGrp->last = NULL; }
        else { ioGrp->last = ioCur->prev; }
    }
    else { ioCur->next->prev = ioCur->prev; }
    (ioGrp->count)--;
    deleteWatchTable( ioGrp, ioCur );
}

static BOOL deleteWatchInfo(
        watchGroup_t *ioGrp, const void *iPtr, watchInfo_t *oTarget )
{
    watchInfo_t*  tmp = NULL; 
    if( searchWatchList( ioGrp, iPtr, &tmp ) ) {
        // 削除対象のデータをコピーして通知
        COM_SET_IF_EXIST( oTarget, *tmp );
        deleteWatchList( ioGrp, tmp );
        free( tmp );
        return true;
    }
    return false;
}

static void setDebugErrorOn( BOOL *oMode, long *oSeqno, long iSeqno )
{
    *oMode = true;
    if( iSeqno > 0 ) {iSeqno--;}
    *oSeqno = iSeqno;
}

static void setDebugErrorOff( BOOL *oMode, long *oSeqno )
{
    *oMode = false;
    *oSeqno = 0;
}

static BOOL judgeDebugError( BOOL iMode, long iDebug, long iInfo )
{
    if( !iMode ) {return false;}
    if( iInfo < iDebug ) {return false;}
    return true;
}

#define PRINTCOM( TEXT ) \
    printProc( iMode, PREFIX_COM, gDebugLog, (TEXT) )

static void dispWatchInfo(
        COM_DEBUG_MODE_t iMode, const char *iPre, const char *iLabel,
        COM_FILEPRM )
{
    PRINTCOM( gLogBuff );
    if( iLabel ) {
        MAKELOG( "%.2s          %s\n", iPre, iLabel );
        PRINTCOM( gLogBuff );
        MAKELOG( "%.2s   %s() in %s line %ld\n", iPre,iFUNC,iFILE,iLINE );
        PRINTCOM( gLogBuff );
    }
}



// メモリ監視処理 ------------------------------------------------------------

static COM_DEBUG_MODE_t  gWatchMemInfoMode = COM_DEBUG_OFF;

void com_setWatchMemInfo( COM_DEBUG_MODE_t iMode )
{
    setDebugMode( &gWatchMemInfoMode, iMode );
}

COM_DEBUG_MODE_t com_getWatchMemInfo( void )
{
    return gWatchMemInfoMode;
}

// メモリ監視情報グループ
static watchGroup_t  gMemGrp;

static char*  gMemOperator[] = {
    "free", "malloc", "realloc", "strdup", "strndup", "scanDir",
    "freeaddrinfo", "getaddrinfo"      // セレクト機能用
};

char *com_getMemOperator( COM_MEM_OPR_t iType )
{
    return gMemOperator[iType];
}

static void dispMemInfo(
        const char *iPre, COM_MEM_OPR_t iType, const watchInfo_t *oInfo,
        COM_DEBUG_MODE_t iMode, COM_FILEPRM )
{
    COM_CLEAR_BUF( gLogBuff );
    // iTypeだけは iIndo->typeと異なる可能性があるため、別入力
    MAKELOG( "%s%08ld com_%-12s %12p %10zu (%10zu)\n",
                 iPre, oInfo->seqno, com_getMemOperator( iType ),
                 oInfo->ptr, oInfo->size, gMemGrp.using );
    dispWatchInfo( iMode, iPre, oInfo->label, COM_FILEVAR );
}

static __thread BOOL  gSkipMemInfo = false;
static __thread long  gSkipMemInfoCount = 0;
static __thread BOOL  gForceLog = false;

void com_skipMemInfoFunc( COM_FILEPRM, BOOL iMode )
{
#ifdef NOTSKIP_MEMWATCH     // makefileでコンパイルオプションとして指定可能
    // スキップなしの場合、この関数は何もせずに終了
    COM_UNUSED( iFILE );
    COM_UNUSED( iLINE );
    COM_UNUSED( iFUNC );
    COM_UNUSED( iMode );
#else
    static __thread long  stackCount = 0;
    static __thread char  triggerFunc[COM_WORDBUF_SIZE];

    if( iMode ) {
        if( !stackCount ) {(void)com_strcpy( triggerFunc, iFUNC );}
        stackCount++;
    }
    else { stackCount--; }
    if( !iMode && stackCount ) {return;}
    if( gSkipMemInfo == iMode ) {return;}
    gSkipMemInfo = iMode;
    if( iMode ) {gSkipMemInfoCount = 0;}
    else if ( gSkipMemInfoCount ) {
        com_logCom( "%ld memInfo output skipped by %s() <%s:line %ld>\n",
                    gSkipMemInfoCount, triggerFunc, iFILE, iLINE );
    }
#endif  // NOTSKIP_MEMWATCH
}

// com_skipMemInfoFunc()の stackCountについて
//   com_skipMemInfo()が色々なところで指定された場合を想定した。
//   既に trueが指定されている状況で、さらに trueが設定されても問題は無いが、
//   その後 false設定されても、先に trueにしている処理があるなら、trueのまま
//   出力抑制を続行し、一番最初の trueが falseに直された時に初めて抑制解除、
//   となるようにした。
//
//   そして falseになった時にデバッグログにスキップ数を出力するときは
//   最初に true指定した関数名が出るようにした。しかし、ファイル名と行数は
//   false指定した位置を指すため、true設定したのと別関数の場合、関数名と
//   ファイル位置が不整合になる。しかし、それも事実なので、そのままとする。

void com_forceLog( BOOL iMode )
{
    gForceLog = iMode;
}

void com_addMemInfo(
        COM_FILEPRM, COM_MEM_OPR_t iType, const void *iPtr, size_t iSize,
        const char *iFormat, ... )
{
    if( !gWatchMemInfoMode ) {return;}

    com_setFuncTrace( false );
    COM_SET_FORMAT( gLogBuff );
    watchInfo_t*  new = NULL;
    if( addWatchInfo( &gMemGrp,&new,iType,iPtr,iSize,gLogBuff,COM_FILEVAR ) ) {
        updateUsing( &gMemGrp, iSize );
        if( gSkipMemInfo && !gForceLog ) {gSkipMemInfoCount++;}
        else {dispMemInfo( "++M", iType, new, gWatchMemInfoMode, COM_FILEVAR );}
    }
    com_setFuncTrace( true );
}

long com_checkMemInfo( const void *iPtr )
{
    if( !gWatchMemInfoMode ) {return 0;}

    com_setFuncTrace( false );
    long  seqno = getSeqno( &gMemGrp, iPtr );
    com_setFuncTrace( true );
    return seqno;
}

void com_deleteMemInfo( COM_FILEPRM, COM_MEM_OPR_t iType, const void *iPtr )
{
    if( !gWatchMemInfoMode ) {return;}

    com_setFuncTrace( false );
    watchInfo_t  tmp;
    memset( &tmp, 0, sizeof(tmp) );
    if( !deleteWatchInfo( &gMemGrp, iPtr, &tmp ) ) {
        // 該当する情報がない＝二重解放の疑いが濃厚
        com_error( COM_ERR_DOUBLEFREE, "no meminfo to free(%p)", iPtr );
        com_setFuncTrace( true );
        return;
    }
    updateUsing( &gMemGrp, -(tmp.size) );
    if( gSkipMemInfo && !gForceLog ) {gSkipMemInfoCount++;}
    else {dispMemInfo( "--M", iType, &tmp, gWatchMemInfoMode, COM_FILEVAR );}
    com_setFuncTrace( true );
}

// メモリ・ファイル どちらでも使えるようにマクロ化
#define DISP_LIST_INFO( USING, LIST, DISPFUNC, GROUP ) \
    com_dbgCom( "\n### %s = %zu ###\n", USING, GROUP.usingMax ); \
    if( !GROUP.count ) {return;} \
    com_printf( "\n### %s (%zu) ###\n", LIST, GROUP.count ); \
    if( gMemoryFailure ) {com_printf( "### but not enough memory ###\n" );} \
    if( iByAddr ) { \
        for( size_t idx = 0;  idx < GROUP.count;  idx++ ) { \
            DISPFUNC( GROUP.ptrList[idx] ); \
        } \
    } \
    else { \
        for( watchInfo_t* tmp = GROUP.top;  tmp;  tmp = tmp->next ) { \
            DISPFUNC( tmp ); \
        } \
    }

static void dispNotFreed( watchInfo_t *iInfo )
{
    // 強制画面出力する
    gNoComDebugLog = false;
    dispMemInfo( "? M", iInfo->type, iInfo, COM_DEBUG_ON,
                        iInfo->file, iInfo->line, iInfo->func );
}

static void listMemInfo( BOOL iByAddr )
{
    DISP_LIST_INFO( "Using Memory MAX", "not freed memory list",
                    dispNotFreed, gMemGrp );
}

#define LIST_MEM_INFO( BYADDR ) \
    if( !gWatchMemInfoMode ) {return;} \
    com_setFuncTrace( false ); \
    listMemInfo( BYADDR ); \
    com_setFuncTrace( true );

void com_listMemInfo( void )
{
    LIST_MEM_INFO( false )
}

void com_listMemInfoByAddr( void )
{
    LIST_MEM_INFO( true )
}

static BOOL  gDebugMemMode = false;
static long  gDebugMemSeqno = 0;

void com_debugMemoryErrorOn( long iSeqno )
{
    setDebugErrorOn( &gDebugMemMode, &gDebugMemSeqno, iSeqno );
}

void com_debugMemoryErrorOff( void )
{
    setDebugErrorOff( &gDebugMemMode, &gDebugMemSeqno );
}

BOOL com_debugMemoryError( void )
{
    return judgeDebugError( gDebugMemMode, gDebugMemSeqno, gMemGrp.seqno );
}



// ファイル監視処理 ----------------------------------------------------------

static COM_DEBUG_MODE_t gWatchFileInfoMode = COM_DEBUG_OFF;

void com_setWatchFileInfo( COM_DEBUG_MODE_t iMode )
{
    setDebugMode( &gWatchFileInfoMode, iMode );
}

COM_DEBUG_MODE_t com_getWatchFileInfo( void )
{
    return gWatchFileInfoMode;
}

static __thread BOOL gSkipFileInfo = false;

void com_skipFileInfo( BOOL iMode )
{
#ifdef NOTSKIP_FILEWATCH     // makefileでコンパイルオプションとして指定可能
    // スキップなしの場合、この関数は何もせずに終了
    COM_UNUSED( iMode );
#else
    gSkipFileInfo = iMode;
#endif
}

// ファイル監視情報グループ
static watchGroup_t  gFileGrp;

static void dispFileInfo(
        const char *iPre, COM_FILE_OPR_t iType, const watchInfo_t *oInfo,
        COM_DEBUG_MODE_t iMode, COM_FILEPRM )
{
    char*  type[] = { "open", "close" };

    COM_CLEAR_BUF( gLogBuff );
    // iTypeだけは iIndo->typeと異なる可能性があるため、別入力
    MAKELOG( "%s%08ld com_f%-7s %12p (%5zu)\n",
                 iPre, oInfo->seqno, type[iType], oInfo->ptr, gFileGrp.count );
    dispWatchInfo( iMode, iPre, oInfo->label, COM_FILEVAR );
}

void com_addFileInfo(
        COM_FILEPRM, COM_FILE_OPR_t iType, const FILE *iFp, const char *iPath )
{
    if( !gWatchFileInfoMode ) {return;}

    com_setFuncTrace( false );
    watchInfo_t*  new = NULL;
    if( addWatchInfo( &gFileGrp, &new, iType, iFp, 0, iPath, COM_FILEVAR ) ) {
        if( iType == COM_FOPEN ) {updateUsing( &gFileGrp, 1 );}
        if( !gSkipFileInfo ) {
            dispFileInfo( "++F", iType, new, gWatchFileInfoMode, COM_FILEVAR );
        }
    }
    com_setFuncTrace( true );
}

long com_checkFileInfo( const FILE *iFp )
{
    if( !gWatchFileInfoMode ) {return 0;}

    com_setFuncTrace( false );
    long  seqno = getSeqno( &gFileGrp, iFp );
    com_setFuncTrace( true );
    return seqno;
}

void com_deleteFileInfo( COM_FILEPRM, COM_FILE_OPR_t iType, const FILE *iFp )
{
    if( !gWatchFileInfoMode ) {return;}

    com_setFuncTrace( false );
    watchInfo_t  tmp;
    memset( &tmp, 0, sizeof(tmp) );
    if( !deleteWatchInfo( &gFileGrp, iFp, &tmp ) ) {
        // 該当する情報がない＝二重解放の疑いが濃厚
        com_error( COM_ERR_DOUBLEFREE, "no fileinfo to free(%p)", (void*)iFp );
        com_setFuncTrace( true );
        return;
    }
    if( !gSkipFileInfo ) {
        dispFileInfo( "--F", iType, &tmp, gWatchFileInfoMode, COM_FILEVAR );
    }
    com_setFuncTrace( true );
}

static void dispNotClosed( watchInfo_t *iInfo )
{
    // 強制画面出力する
    gNoComDebugLog = false;
    dispFileInfo( "? F", iInfo->type, iInfo, COM_DEBUG_ON,
                         iInfo->file, iInfo->line, iInfo->func );
}

static void listFileInfo( BOOL iByAddr )
{
    DISP_LIST_INFO( "total opened file count", "not closed file list",
                    dispNotClosed, gFileGrp );
}

#define LIST_FILE_INFO( BYADDR ) \
    if( !gWatchFileInfoMode ) {return;} \
    com_setFuncTrace( false ); \
    listFileInfo( BYADDR ); \
    com_setFuncTrace( true );

void com_listFileInfo( void )
{
    LIST_FILE_INFO( false )
}

void com_listFileInfoByAddr( void )
{
    LIST_FILE_INFO( true )
}


static BOOL  gDebugFopenMode  = false;
static long  gDebugFopenSeqno = 0;

void com_debugFopenErrorOn( long iSeqno )
{
    setDebugErrorOn( &gDebugFopenMode, &gDebugFopenSeqno, iSeqno );
}

void com_debugFopenErrorOff( void )
{
    setDebugErrorOff( &gDebugFopenMode, &gDebugFopenSeqno );
}

BOOL com_debugFopenError( void )
{
    return judgeDebugError( gDebugFopenMode, gDebugFopenSeqno, gFileGrp.seqno );
}

static BOOL  gDebugFcloseMode  = false;
static long  gDebugFcloseSeqno = 0;

void com_debugFcloseErrorOn( long iSeqno )
{
    // 扱いが少し違うため、setDebugErrorOn()は使わない
    gDebugFcloseMode = true;
    gDebugFcloseSeqno = iSeqno;
}

void com_debugFcloseErrorOff( void )
{
    setDebugErrorOff( &gDebugFcloseMode, &gDebugFcloseSeqno );
}

BOOL com_debugFcloseError( const FILE *iFp )
{
    // 扱いが少し違うため、judgeDebugError()は使わない
    if( !gDebugFcloseMode ) {return false;}
    if( !gDebugFcloseSeqno ) {return true;}
    // カウント決め打ちでNGにする
    long  infoSeqno = checkWatchInfo( &gFileGrp, iFp );
    if( infoSeqno == NO_DEBUG_INFO ) {return false;}
    if( infoSeqno != gDebugFcloseSeqno ) {return false;}
    return true;
}



// パラメータチェックNG表示 --------------------------------------------------

static char  gParamNG[COM_LINEBUF_SIZE];

void com_prmNgFunc( COM_FILEPRM, const char *iFormat, ... )
{
    COM_SET_FORMAT( gParamNG );
    if( !iFormat ) {(void)com_strcpy( gParamNG, iFUNC );}
    (void)com_strcat( gParamNG, "() parameter NG" );
    // 敢えて実巻数を呼ぶ(com_error()だと呼出位置がここになるため
    com_errorFunc( COM_ERR_DEBUGNG, true, COM_FILEVAR, gParamNG );
}



// comモジュール内専用 排他＆メモリ監視スキップ ------------------------------

void com_mutexLockCom( pthread_mutex_t *ioMutex, COM_FILEPRM )
{
    com_mutexLock( ioMutex, iFUNC );
    com_skipMemInfoFunc( COM_FILEVAR, true );
}

BOOL com_mutexUnlockCom( pthread_mutex_t *ioMutex, COM_FILEPRM, BOOL iResult )
{
    com_skipMemInfoFunc( COM_FILEVAR, false );
    com_mutexUnlock( ioMutex, iFUNC );
    return iResult;
}



// 関数呼出トレース機能関連 --------------------------------------------------

#ifdef USE_FUNCTRACE
static __thread  char gFileNameBuf[COM_LINEBUF_SIZE];
static __thread  char gFilePathBuf[COM_LINEBUF_SIZE];

typedef struct {
    void*   addr;
    char*   name;
} funcName_t;

enum { NEED_DATA_COUNT = 2 };

__attribute__((no_instrument_function))
static BOOL seekFuncName( com_seekFileResult_t *iInf )
{
    funcName_t*  data = iInf->userData;
    ulong  addr = com_strtoul( iInf->line, 16, false );
    if( addr != (ulong)(data->addr) ) {return true;}
    int cnt = sscanf( iInf->line, "%*s %*s %s %s", gFileNameBuf, gFilePathBuf );
    if( cnt != NEED_DATA_COUNT ) {return true;}
    if( !strcmp( gFileNameBuf, ".text" ) ) {return true;}
    char  fileName[COM_WORDBUF_SIZE] = {0};
    com_getFileName( fileName, sizeof(fileName), gFilePathBuf );
    com_connectString( gFileNameBuf, sizeof(gFileNameBuf), " (%s)", fileName );
    data->name = gFileNameBuf;
    return false;
}

__attribute__((no_instrument_function))
static const char *seekNameList( void *iFunc )
{
    if( !com_checkExistFile( MAKEFILE_NAMELIST ) ) {return NULL;}
    funcName_t  data = { iFunc, NULL };
    char  nmBuf[COM_LINEBUF_SIZE];
    (void)com_seekFile( MAKEFILE_NAMELIST, seekFuncName, &data,
                        nmBuf, sizeof(nmBuf) );
    return data.name;
}

const char *com_seekNameList( void *iAddr )
{
    if( !iAddr ) {COM_PRMNG(NULL);}
    com_setFuncTrace( false );
    const char*  result = seekNameList( iAddr );
    com_setFuncTrace( true );
    return result;
}

static __thread BOOL  gFuncTraceMode = true;

// デバッガでの確認用に呼び元情報を受け取る
#define FILEPRM_FOR_DEBUG \
    do {\
        COM_UNUSED( iFILE ); \
        COM_UNUSED( iLINE ); \
        COM_UNUSED( iFUNC ); \
    } while(0)

__attribute__((no_instrument_function))
void com_setFuncTraceReal( BOOL iMode, COM_FILEPRM )
{
    static __thread long  muteCount = 0;
    FILEPRM_FOR_DEBUG;
    if( !iMode ) {muteCount++;} else {muteCount--;}
    if( muteCount ) {gFuncTraceMode = false;}
    else {gFuncTraceMode = true;}
}

typedef struct {
    void*  func;
    long   level;
} funcTrace_t;

static __thread funcTrace_t  gFuncTraceList[COM_FUNCTRACE_MAX];

// まだデバッグ機能が動作していない状態から動作することを想定し、
// com_createRingBuf()によるバッファ領域の静的確保を行わず、手動設定する。
// .bufについては別途関数内で gFuncTraceList を設定する。

static __thread com_ringBuf_t  gFuncTracer = {
    NULL, sizeof(funcTrace_t), COM_FUNCTRACE_MAX, 0, 0, 0,
    true, false, 0, NULL
};
static __thread long  gFuncNestCount = 0;

__attribute__((no_instrument_function))
void __cyg_profile_func_enter( void *iFunc, void *iCaller )
{
    COM_UNUSED( iCaller );
    if( !gFuncTraceMode ) {return;}
    COM_TRACER_SET( false );
    gFuncTracer.buf = gFuncTraceList;  // 初期化ではエラーになるのでここで設定
    com_pushRingBuf( &gFuncTracer,
                     &(funcTrace_t){ iFunc, gFuncNestCount++ },
                     sizeof( funcTrace_t ) );
    COM_TRACER_RESUME;
}

__attribute__((no_instrument_function))
void __cyg_profile_func_exit( void *iFunc, void *iCaller )
{
    COM_UNUSED( iFunc );
    COM_UNUSED( iCaller );
    if( !gFuncTraceMode ) {return;}
    gFuncNestCount--;
}

__attribute__((no_instrument_function))
static const char *getFuncName( void *iFunc )
{
    const char*  result = seekNameList( iFunc );
    if( result ) {return result;}

    Dl_info dli;
    if( dladdr( iFunc, &dli ) ) {
        if( dli.dli_sname ) {return dli.dli_sname;}
        return "<<internal function>>";
    }
    return "<<unknown>>";
}

static pthread_mutex_t gMutexFuncTrace = PTHREAD_MUTEX_INITIALIZER;

__attribute__((no_instrument_function))
void com_dispFuncTrace( void )
{
    com_mutexLock( &gMutexFuncTrace, __func__ );
    COM_TRACER_SET( false );
    com_printf( "\n##### function trace list #####\n" );
    funcTrace_t*  tracer = NULL;
    for( size_t i = 0;  (tracer = com_pullRingBuf( &gFuncTracer ));  i++ ) {
        com_printf( "[%06zd] %p ", i, tracer->func );
        com_repeat( " ", tracer->level, false );
        com_printf( "%s\n", getFuncName( tracer->func ) );
    }
    COM_TRACER_RESUME;
    com_mutexUnlock( &gMutexFuncTrace, __func__ );
}
#endif // USE_FUNCTRACE


// アサート機能関連 ----------------------------------------------------------

static void dispAssertLine( char *iLabel )
{
    com_printf( "* assertion( " );
    if( iLabel ) { com_printf( "%s: ", iLabel ); }
}

static void dispAssertLocation( COM_FILEPRM )
{
    com_printf( "   in %s:line %ld  %s()\n", iFILE, iLINE, iFUNC );
}

// 最終的に NDEBUGが宣言されている場合、com_assert*()はマクロ宣言により
// コードから消える。そのため、ここで記載した NDEBUG宣言時のルートは、
// 現状は通らず、必ず trueを返す。(処理自体の削除はしないこととした)
//
static BOOL checkNDEBUG( void )
{
    BOOL  result = true;
#ifdef NDEBUG
    result = false;
    com_printf( "* assertion not executed (NDEBUG is defined)" );
#endif
    return result;
}

#define ASSERTSTART \
    do { \
        if( com_getDebugPrint() == COM_DEBUG_OFF ) {return;} \
        dispAssertLine( iLabel ); \
    } while(0)

void com_assertEqualsFunc(
        char *iLabel, long iExpected, long iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%ld == %ld )\n", iExpected, iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( iExpected == iResult );}
}

void com_assertNotEqualsFunc(
        char *iLabel, long iExpected, long iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%ld != %ld )\n", iExpected, iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( iExpected != iResult );}
}

void com_assertEqualsUFunc(
        char *iLabel, ulong iExpected, ulong iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%lu(0x%lx) == %lu(0x%lx) )\n",
                iExpected, iExpected, iResult, iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( iExpected == iResult );}
}

void com_assertNotEqualsUFunc(
        char *iLabel, ulong iExpected, ulong iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%lu(0x%lx) != %lu(0x%lx) )\n",
                iExpected, iExpected, iResult, iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( iExpected != iResult );}
}

// 判定に使用する FLT_EPSILON は float.h で宣言されている
void com_assertEqualsFFunc(
        char *iLabel, float iExpected, float iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%f == %f )\n", iExpected, iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( fabsf( iExpected - iResult ) < FLT_EPSILON );}
}

// 判定に使用する FLT_EPSILON は float.h で宣言されている
void com_assertNotEqualsFFunc(
        char *iLabel, float iExpected, float iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%f != %f )\n", iExpected, iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( fabsf( iExpected - iResult ) >= FLT_EPSILON );}
}

// 判定に使用する DBL_EPSILON は float.h で宣言されている
void com_assertEqualsDFunc(
        char *iLabel, double iExpected, double iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%.16f == %.16f )\n", iExpected, iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( fabs( iExpected - iResult ) < DBL_EPSILON );}
}

// 判定に使用する DBL_EPSILON は float.h で宣言されている
void com_assertNotEqualsDFunc(
        char *iLabel, double iExpected, double iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%.16f != %.16f )\n", iExpected, iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( fabs( iExpected - iResult ) >= DBL_EPSILON );}
}

void com_assertStringFunc(
        char *iLabel, char *iExpected, char *iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "\"%s\" == \"%s\" )\n", iExpected, iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( !strcmp( iExpected, iResult ) );}
}

static void dispGuideLine( size_t iLength )
{
    com_printf( "   " );
    com_repeat( "~", (long)iLength, false );
    if( checkNDEBUG() ) {com_printf( " (check length = %zu)\n", iLength );}
}

void com_assertStringLenFunc(
        char *iLabel, char *iExpected, char *iResult, size_t iLength,
        COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "\n  \"%s\"\n", iExpected );
    dispGuideLine( iLength );
    com_printf( "  \"%s\"\n", iResult );
    dispGuideLine( iLength );
    com_printf( "  )\n" );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( !strncmp(iExpected, iResult, iLength) );}
}

void com_assertTrueFunc( char *iLabel, BOOL iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%ld is true )\n", iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( iResult );}
}

void com_assertFalseFunc( char *iLabel, BOOL iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%ld is false )\n", iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( !iResult );}
}

void com_assertNullFunc( char *iLabel, void *iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%p is NULL )\n", iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( !iResult );}
}

void com_assertNotNullFunc( char *iLabel, void *iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%p is not NULL )\n", iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( iResult );}
}

void com_assertEqualsPFunc(
        char *iLabel, void* iExpected, void* iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%p == %p )\n", iExpected, iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( iExpected == iResult );}
}

void com_assertNotEqualsPFunc(
        char *iLabel, void* iExpected, void* iResult, COM_FILEPRM )
{
    ASSERTSTART;
    com_printf( "%p != %p )\n", iExpected, iResult );
    dispAssertLocation( COM_FILEVAR );
    if( checkNDEBUG() ) {assert( iExpected != iResult );}
}


// デバッグ機能 初期化処理 ---------------------------------------------------

// makefile(コンパイルオプション)に定義がある場合は、それを反映する
static void setDefaultDebugMode( void )
{
#ifndef USEDEBUGLOG   // デバッグログ出力抑制
    gDefaultLogFile = NULL;
#endif
#ifdef DEBUGPRINT     // デバッグ表示設定
    com_setDebugPrint( DEBUGPRINT );
#else
    com_setDebugPrint( COM_DEBUG_OFF );
#endif
#ifdef WATCHMEM       // メモリ監視モード
    com_setWatchMemInfo( WATCHMEM );
#else
    com_setWatchMemInfo( COM_DEBUG_OFF );
#endif
#ifdef WATCHFILE      // ファイル監視モード
    com_setWatchFileInfo( WATCHFILE );
#else
    com_setWatchFileInfo( COM_DEBUG_OFF );
#endif
}

void com_initializeDebugMode( void )
{
    // com_initialize()から呼ばれる限り、COM_DEBUG_AVOID_～は不要
    setDefaultDebugMode();
    // 個別部初期化(ここで呼ぶことにより、デバッグモード等も設定変更可能)
    com_initializeSpec();
    // エラー情報初期化
    gOccuredError = false;
    // デバッグログファイルオープン
    openDebugLog();
    com_dispTitle( "start" );
}



// デバッグ機能 終了処理 -----------------------------------------------------

void com_finalizeDebugMode( void )
{
    // finalizeCom()から呼ばれる限り、COM_DEBUG_AVOID～は不要
    com_listMemInfo();
    com_listFileInfo();
    com_dispTitle( "end" );
    com_closeDebugLog();
}

