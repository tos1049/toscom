/*
 *****************************************************************************
 *
 * 共通で色々使えそうな処理群  by TOS
 *   com_debug.c・com_procThread.c も併用しなければならない。
 *   com_if.h に書いた順番で関数が並んでいるわけではないことに注意すること。
 *
 *   ＊全てのソースで com_if.h をインクルードする想定。
 *   ＊外部高階関数の使い方については、com_if.h に記述しているので、必読。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"
#include <sys/sysmacros.h>


/* 基本情報保持 *************************************************************/

/* makefile のコンパイルオプション APLNAMEで指定された文字列 */
static char  gAPLNAME[] =
#ifdef APLNAME
    APLNAME;
#else
    "";    // 定義がないときは空文字とする
#endif

const char *com_getAplName( void )
{
    return gAPLNAME;
}

/* makefile のコンパイルオプション VERSIONで指定された文字列 */
static char  gVERSION[] =
#ifdef VERSION
    VERSION;
#else
    "0";   // 定義がないときは "0" とする
#endif

const char *com_getVersion( void )
{
    return gVERSION;
}

static int     gCmdArgc = 0;
static char**  gCmdArgv = NULL;

static void setCommandLine( int iArgc, char **iArgv )
{
    gCmdArgc = iArgc;
    gCmdArgv = iArgv;
}

void com_getCommandLine( int *oArgc, char ***oArgv )
{
    COM_SET_IF_EXIST( oArgc, gCmdArgc );
    COM_SET_IF_EXIST( oArgv, gCmdArgv );
}

static com_hashId_t  gOptHash = COM_HASHID_NOTREG;

static size_t getKeySize( void *iKey, BOOL iLong )
{
    if( iLong ) {return (strlen(iKey) + 1);}
    return sizeof(long);
}

static BOOL addOptHash( void *iKey, BOOL iLong, com_getOpt_t *iOpt )
{
    size_t  keySize = getKeySize( iKey, iLong );
    COM_HASH_t  result =
        com_addHash( gOptHash, false, iKey, keySize, iOpt, sizeof(*iOpt) );
    if( result != COM_HASH_OK ) {return false;}
    return true;
}

static com_getOpt_t *getOptHash( void *iKey, BOOL iLong )
{
    size_t  keySize = getKeySize( iKey, iLong );
    const void*  data = NULL;
    size_t  dataSize = 0;
    if( !com_searchHash( gOptHash, iKey, keySize, &data, &dataSize ) ) {
        return NULL;
    }
    return (com_getOpt_t*)data;
}

static char  gNoSameOpt[] = "cannot specify same options";

static BOOL  checkShortOpt( com_getOpt_t *iOpt )
{
    long  opt = iOpt->shortOpt;
    if( !opt ) {return true;}
    if( getOptHash( &opt, false ) ) {
        com_error(COM_ERR_DEBUGNG, "%s(%c)", gNoSameOpt, (int)opt );
        return false;
    }
    return addOptHash( &opt, false, iOpt );
}

static BOOL checkLongOpt( com_getOpt_t *iOpt )
{
    char*  opt = iOpt->longOpt;
    if( !opt ) {return true;}
    if( getOptHash( opt, true ) ) {
        com_error(COM_ERR_DEBUGNG, "%s(%s)", gNoSameOpt, opt );
        return false;
    }
    return addOptHash( opt, true, iOpt );
}

static long    gOptRestCnt = 0;
static char**  gOptRestList = NULL;

static com_strChain_t*  gOptMan = NULL;

enum {OPT_CHECK_ERROR = -1};

static void freeOptSetting( void )
{
    gOptRestCnt = 0;
    com_free( gOptRestList );
}

static void setOptionName( com_getOpt_t *iOpt, char *oBuf, size_t iBufSize )
{
    if( iOpt->shortOpt && iOpt->longOpt ) {
        snprintf( oBuf, iBufSize, "-%c/--%s",
                  (int)iOpt->shortOpt, iOpt->longOpt );
    }
    else if( iOpt->shortOpt ) {
        snprintf( oBuf, iBufSize, "-%c", (int)iOpt->shortOpt );
    }
    else {
        snprintf( oBuf, iBufSize, "--%s", iOpt->longOpt );
    }
}

static void addMandatoryOption( com_getOpt_t *iOpt )
{
    char  opt[COM_WORDBUF_SIZE] = {0};
    setOptionName( iOpt, opt, sizeof(opt) );
    if( !com_addChainData( &gOptMan, opt ) ) {com_exit( COM_ERR_NOMEMORY );}
}

static void delMandatoryOption( com_getOpt_t *iOpt )
{
    char  opt[COM_WORDBUF_SIZE] = {0};
    setOptionName( iOpt, opt, sizeof(opt) );
    (void)com_deleteChainData( &gOptMan, false, opt );
}

static long setOptSetting( com_getOpt_t *iOpts )
{
    long  result = 0;
    gOptHash = com_registerHash( 31, NULL );
    for( ; iOpts->func; iOpts++ ) {
        if( iOpts->mandatory ) { addMandatoryOption( iOpts ); result++; }
        if( !checkShortOpt( iOpts ) ) { return OPT_CHECK_ERROR; }
        if( !checkLongOpt( iOpts ) )  { return OPT_CHECK_ERROR; }
    }
    freeOptSetting();
    return result;
}

static char  gUnknownOpt[] = "unknown option";

static com_getOpt_t *searchOpt( char *iPrm, ulong iIdx, BOOL iLong )
{
    com_getOpt_t*  opt = NULL;
    if( iLong ) {
        if( !(opt = getOptHash( iPrm, true )) ) {
            com_error( COM_ERR_PARAMNG, "%s(%s)", gUnknownOpt, iPrm );
            return NULL;
        }
        return opt;
    }
    long shOpt = *(iPrm + iIdx);
    if( !(opt = getOptHash( &shOpt, false )) ) {
        com_error( COM_ERR_PARAMNG, "%s(%c)", gUnknownOpt, (int)shOpt );
        return NULL;
    }
    return opt;
}

static BOOL shortWithArg( com_getOpt_t *iOptInf, ulong iCnt )
{
    if( iOptInf->argvCnt > 0 && iCnt > 1 ) {
        com_error( COM_ERR_PARAMNG,
                   "'%c' can't be used in multiple options",
                   (int)iOptInf->shortOpt);
        return true;
    }
    return false;
}

static BOOL decideParams(
        com_getOptInf_t *oOptInf, com_getOpt_t *iOpt,
        int iArgc, char **iArgv, int iCnt )
{
    if( iOpt->argvCnt == COM_OPT_ALL ) {
        oOptInf->argc = iArgc;
        oOptInf->argv = iArgv;
    }
    if( iOpt->argvCnt <= 0 ) {return true;}  // COM_OPT_ALLもここも true返却
    if( iCnt + iOpt->argvCnt >= iArgc ) {
        com_error( COM_ERR_PARAMNG, "lack of option arguments" );
        return false;
    }
    oOptInf->argc = iOpt->argvCnt;
    oOptInf->argv = iArgv + iCnt + 1;
    return true;
}

static BOOL notifyOpt( com_getOpt_t *iOpt, com_getOptInf_t *iOptInf )
{
    COM_DEBUG_AVOID_END( COM_NO_FUNCNAME );   // ENDと STARTはこの順で正しい
    BOOL  result = true;
    if( iOpt->func ) {result = (iOpt->func)( iOptInf );}
    COM_DEBUG_AVOID_START( COM_NO_FUNCNAME | COM_NO_SKIPMEM );
    com_skipMemInfoFunc( __FILE__, __LINE__, "com_getOption", true );
    return result;
}

static BOOL checkOpt(
        int *ioCnt, int iArgc, char **iArgv,
        char **oPrm, long *oMonCnt, void *ioUserData ) 
{
    com_getOpt_t*  opt = NULL;
    *oPrm = NULL;
    char*  cur = iArgv[*ioCnt];
    BOOL  isLong = false;
    ulong  cnt = 1;
    if( com_compareString( cur, "--", 2, false ) ) {cur += 2; isLong = true;}
    else if( com_compareString( cur, "-", 1, false ) ) {
        cur += 1;  cnt = strlen(cur);
    }
    else {*oPrm = cur;  return true;}
    for( ulong idx = 0;  idx < cnt;  idx++ ) {
        if( !(opt = searchOpt( cur, idx, isLong )) ) {return false;}
        if( shortWithArg( opt, cnt ) ) {return false;}
        com_getOptInf_t inf = { opt->optValue, 0, NULL, ioUserData };
        if( !decideParams( &inf, opt, iArgc, iArgv, *ioCnt ) ) {return false;}
        if( !notifyOpt( opt, &inf ) ) {return false;}
        if( opt->mandatory ) { delMandatoryOption( opt ); (*oMonCnt)++; }
        if( opt->argvCnt > 0 ) {*ioCnt += opt->argvCnt;}
    }
    return true;
}

static BOOL addRest( char *iPrm, COM_FILEPRM )
{
    if( !iPrm ) {return true;}
    char**  addArgv = com_reallocAddrFunc( &gOptRestList, sizeof(char*),
                                           COM_TABLEEND, &gOptRestCnt, 1,
                                           COM_FILEVAR, "rest opt(%s)", iPrm );
    if( !addArgv ) {return false;}
    *addArgv = iPrm;
    return true;
}

static BOOL freeOptResource( BOOL iResult )
{
    com_cancelHash( gOptHash );
    com_freeChainData( &gOptMan );
    COM_DEBUG_AVOID_END( COM_NO_FUNCNAME );
    return iResult;
}

static BOOL getOptNG( long *oRestArgc, char ***oRestArgv )
{
    COM_SET_IF_EXIST( oRestArgc, 0 );
    COM_SET_IF_EXIST( oRestArgv, NULL );
    return freeOptResource( false );
}

#define GETOPTNG \
    return getOptNG( oRestArgc, oRestArgv )

static void dispLackOption( void )
{
    char  buf[COM_LINEBUF_SIZE] = {0};
    (void)com_spreadChainData( gOptMan, buf, sizeof(buf) );
    com_error( COM_ERR_PARAMNG, "lack of mandatory option: %s", buf );
}

BOOL com_getOption(
        int iArgc, char **iArgv, com_getOpt_t *iOpts,
        long *oRestArgc, char ***oRestArgv, void *ioUserData )
{
    if( !iOpts ) {COM_PRMNG(false);}
    COM_DEBUG_AVOID_START( COM_NO_FUNCNAME );
    long  manMax = setOptSetting( iOpts );
    if( manMax < 0 ) {GETOPTNG;}
    long  manCnt = 0;
    for( int i = 1;  i < iArgc;  i++ ) {  // 実行ファイル名はチェックしない
        char* prm = NULL;
        if( !checkOpt(&i, iArgc, iArgv, &prm, &manCnt, ioUserData) ) {GETOPTNG;}
        if( prm ) { if( !addRest( prm, COM_FILELOC ) ) {GETOPTNG;} }
    }
    COM_SET_IF_EXIST( oRestArgc, gOptRestCnt );
    COM_SET_IF_EXIST( oRestArgv, gOptRestList );
    if( manCnt != manMax ) {dispLackOption();  GETOPTNG;}
    return freeOptResource( true );
}

void com_exitFunc( long iType, COM_FILEPRM )
{
    if( iType != COM_NO_ERROR ) {
        // エラーコードが正常終了を示す場合、このメッセージは出力しない
        printf( "!!!!! \"%s\" forced to terminate\n", com_getAplName() );
        if( com_getDebugPrint() ) {
            printf( " in %s:line %ld %s()\n", COM_FILEVAR );
        }
    }
    exit( iType );
}

#ifndef CHECK_PRINT_FORMAT    // make checkfを打った時に指定されるマクロ
int com_system( const char *iFormat, ... )
{
    char  sysBuf[COM_LINEBUF_SIZE];
    if( !iFormat ) {COM_PRMNG(0);}
    COM_SET_FORMAT( sysBuf );
    if( strlen( sysBuf ) == sizeof(sysBuf) -1 ) {
        com_printf( "maybe... com_system() get too long command line"
                    "(%-10s...)\n", sysBuf );
    }
    return system( sysBuf );
}
#endif



/* メモリ処理関数 ***********************************************************/

static pthread_mutex_t  gMutexMem = PTHREAD_MUTEX_INITIALIZER;
static char  gMemLog[COM_LINEBUF_SIZE];

static void checkAllocation(
        void *iPtr, const char *iFormat, size_t iSize, COM_MEM_OPR_t iType,
        COM_FILEPRM )
{
    // メモリ捕捉できていたら、デバッグ情報を追加
    if( iPtr ) {
        char*  info = iFormat ? gMemLog : NULL;
        com_addMemInfo( COM_FILEVAR, iType, iPtr, iSize, info );
        return;
    }
    // メモリ捕捉できていなかったら、エラーメッセージ出力
    const char*  op = com_getMemOperator( iType );
    if( iFormat ) {
        com_error( COM_ERR_NOMEMORY, "com_%s NG (%s)", op, gMemLog );
    }
    else {
        com_error( COM_ERR_NOMEMORY, "com_%s NG (%zubyte)", op, iSize );
    }
}

void *com_mallocFunc( size_t iSize, COM_FILEPRM, const char *iFormat, ... )
{
    if( !iSize ) {COM_PRMNG(NULL);}
    com_mutexLock( &gMutexMem, "malloc(%zu)", iSize );
    void*  ptr = NULL;
    if( !com_debugMemoryError() ) {ptr = calloc( iSize, 1 );}
    COM_SET_FORMAT( gMemLog );
    checkAllocation( ptr, iFormat, iSize, COM_MALLOC, COM_FILEVAR );
    com_mutexUnlock( &gMutexMem, NULL );
    return ptr;
}

static void *procRealloc(
        void *iAddr, size_t iSize, const char *iFormat, COM_FILEPRM )
{
    void*  ptr = NULL;
    if( !com_debugMemoryError() ) {ptr = realloc( iAddr, iSize );}
    if( iAddr ) {
        // 元データありで、realloc()成功時は元データのメモリ監視情報を削除
        if( ptr ) {com_deleteMemInfo( COM_FILEVAR, COM_REALLOC, iAddr );}
        // realloc()失敗時は何もしない
    }
    // 新データのメモリ監視情報はこちらで追加される
    checkAllocation( ptr, iFormat, iSize, COM_REALLOC, COM_FILEVAR );
    return ptr;
}

void *com_reallocFunc(
        void *iAddr, size_t iSize, COM_FILEPRM, const char *iFormat, ... )
{
    if( !iSize ) {com_free( iAddr );  return NULL;}
    com_mutexLock( &gMutexMem, "realloc(%p)", iAddr );
    COM_SET_FORMAT( gMemLog );
    void*  result = procRealloc( iAddr, iSize, iFormat, COM_FILEVAR );
    com_mutexUnlock( &gMutexMem, NULL );
    return result;
}

void *com_reallocfFunc(
        void *iAddr, size_t iSize, COM_FILEPRM, const char *iFormat, ... )
{
    com_mutexLock( &gMutexMem, "reallocf(%p)", iAddr );
    COM_SET_FORMAT( gMemLog );
    void*  result = NULL;
    if( iSize ) {result = procRealloc( iAddr, iSize, iFormat, COM_FILEVAR );}
    if( !result ) {com_free( iAddr );}
    com_mutexUnlock( &gMutexMem, NULL );
    return result;
}

static BOOL returnRealloct( void *iAddr, BOOL iResult )
{
    COM_UNUSED(iAddr);
    com_mutexUnlock( &gMutexMem, NULL );
    return iResult;
}

static BOOL procRealloct(
        void *ioAddr, size_t iUnit, long *ioCount, long iResize,
        const char *iFormat, COM_FILEPRM, va_list iAp )
{
    if( !iResize ) {return true;}
    if( iResize < 0 && -iResize >= *ioCount ) {iResize = -(*ioCount);}
    com_mutexLock( &gMutexMem, "realloct(%p)", ioAddr );
    COM_CLEAR_BUF( gMemLog );
    vsnprintf( gMemLog, sizeof(gMemLog), iFormat, iAp );
    char**  dmy = ioAddr;   // ioAddrはダブルポインタが渡されていることを想定
    if( (*ioCount + iResize) == 0 ) {
        com_mutexUnlock( &gMutexMem, NULL );
        com_freeFunc( dmy, COM_FILEVAR );
        *ioCount = 0;
        return true;
    }
    char*  result = procRealloc( *dmy, iUnit * (size_t)(*ioCount + iResize),
                                 iFormat, COM_FILEVAR );
    if( !result ) {return returnRealloct( ioAddr, false );}
    if( iResize > 0 ) {
        memset( result + (long)iUnit * (*ioCount), 0, iUnit * (size_t)iResize );
    }
    *dmy = result;
    *ioCount += iResize;
    return returnRealloct( ioAddr, true );
}

#define PROC_REALLOCT \
    BOOL  result = false; \
    do { \
        va_list  ap; \
        va_start( ap, iFormat ); \
        result = procRealloct( ioAddr, iUnit, ioCount, iResize, \
                               iFormat, COM_FILEVAR, ap ); \
        va_end( ap ); \
    } while(0)

BOOL com_realloctFunc(
        void *ioAddr, size_t iUnit, long *ioCount, long iResize,
        COM_FILEPRM, const char *iFormat, ... )
{
    if( !iResize ) {return true;}
    PROC_REALLOCT;
    return result;
}

static void *returnTablePos(
        char **iAddr, size_t iUnit, long iPos, long iCount, long iResize )
{
    if( !iCount ) {return NULL;}
    iCount--;
    if( iPos >= 0 ) {
        if( iPos <= iCount ) {return (*iAddr + iPos * iUnit);}
    }
    else {
        if( iResize > 0 ) {return (*iAddr + (iCount - iResize + 1) * iUnit);}
    }
    return (*iAddr + iCount * iUnit );
}

static BOOL calcTmpSize(
        char **oTmp, size_t *oTmpSize, char **iAddr, size_t iUnit,
        long iPos, long iCount, long *ioResize )
{
    if( *ioResize > 0 ) { // 途中追加の場合は、拡張後の移動サイズを保持
        if( iPos > COM_TABLEEND ) {*oTmpSize = (iCount - iPos) * iUnit;}
    }
    else { 
        if( iPos - *ioResize >= iCount ) {*ioResize = iPos - iCount;}
        else { // 削除範囲より後にデータがある時は、内容を退避
            long  diff = iPos - *ioResize;
            *oTmpSize = (iCount - diff) * iUnit;
            *oTmp = malloc( *oTmpSize );
            if( !(*oTmp) ) {
                com_error( COM_ERR_NOMEMORY, "fail to get tmp area" );
                return false;
            }
            memcpy( *oTmp, *iAddr + diff * iUnit, *oTmpSize );
        }
    }
    return true;
}

static void moveTableData(
        char *iStack, size_t iMoveSize, char **iAddr, size_t iUnit,
        long iPos, long iResize )
{
    if( !iResize || !iMoveSize ) {return;}
    char*  target = *iAddr + iPos * iUnit;
    if( iResize > 0 ) {
        memmove( *iAddr + (iPos + iResize) * iUnit, target, iMoveSize );
        memset( target, 0, iUnit * iResize );
    }
    else {memcpy( target, iStack, iMoveSize );}
}

void *com_reallocAddrFunc(
        void *ioAddr, size_t iUnit, long iPos, long *ioCount, long iResize,
        COM_FILEPRM, const char *iFormat, ... )
{
    char**  dmy = ioAddr;
    if( !iPos && !(*ioCount) ) {iPos = COM_TABLEEND;}
    if( iPos < COM_TABLEEND || iPos >= *ioCount || !iResize ) {
        if( iPos < COM_TABLEEND || iPos >= *ioCount ) {com_prmNG(NULL);}
        return returnTablePos( dmy, iUnit, iPos, *ioCount, iResize );
    }
    char*  tmp = NULL;  // 捕捉解放に toscomを使用しない
    size_t  tmpSize = 0;
    if( !calcTmpSize( &tmp, &tmpSize, dmy, iUnit, iPos, *ioCount, &iResize ) ){
        return NULL;
    }
    PROC_REALLOCT;
    if( !result ) {free( tmp );  return NULL;}
    dmy = ioAddr; // アドレス変化の可能性があるので再取得
    moveTableData( tmp, tmpSize, dmy, iUnit, iPos, iResize );
    free( tmp );
    return returnTablePos( dmy, iUnit, iPos, *ioCount, iResize );
}

static char *procStrdup(
        const char *iString, size_t iSize, const char *iFormat,
        COM_MEM_OPR_t iType, COM_FILEPRM )
{
    char*  ptr = NULL;
    if( !com_debugMemoryError() ) {ptr = calloc( iSize, 1 );}
    if( ptr ) {(void)com_strncpy( ptr, iSize, iString, iSize - 1 );}
    if( !iFormat ) {
        (void)com_strncpy( gMemLog, sizeof(gMemLog), iString, iSize - 1 );
        iFormat = iString;  // checkAllocation()用に代入(非NULLであれば良い)
    }
    checkAllocation( ptr, iFormat, iSize, iType, COM_FILEVAR );
    return ptr;
}

char *com_strdupFunc(
        const char *iString, COM_FILEPRM,
        const char *iFormat, ... )
{
    if( !iString ) {COM_PRMNG(NULL);}
    com_mutexLock( &gMutexMem, "strdup(%s)", iString );
    COM_SET_FORMAT( gMemLog );
    if( !iFormat ) {(void)com_strcpy( gMemLog, iString );}
    char*  result = procStrdup( iString, strlen(iString)+1, iFormat,
                                COM_STRDUP, COM_FILEVAR );
    com_mutexUnlock( &gMutexMem, NULL );
    return result;
}

char *com_strndupFunc(
        const char *iString, size_t iSize, COM_FILEPRM,
        const char *iFormat, ... )
{
    if( !iString || !iSize ) {COM_PRMNG(NULL);}
    com_mutexLock( &gMutexMem, "strndup(%s)", iString );
    COM_SET_FORMAT( gMemLog );
    if( iSize > strlen(iString) ) {iSize = strlen(iString);}
    if( !iFormat ) {
        (void)com_strncpy( gMemLog, sizeof(gMemLog), iString, iSize );
    }
    iSize++;  // 終端文字分
    char*  result = procStrdup( iString, iSize, iFormat,
                                COM_STRNDUP, COM_FILEVAR );
    com_mutexUnlock( &gMutexMem, NULL );
    return result;
}

// 基本的にマクロ定義した com_free() を通して使うこと
void com_freeFunc( void *ioAddr, COM_FILEPRM )
{
    if( !ioAddr ) {COM_PRMNGF("com_free",);}
    char**  dmy = ioAddr;  // ioAddrはダブルポインタであることを想定
    if( *dmy == NULL ) {return;}
    com_mutexLock( &gMutexMem, "free(%p)", *dmy );
    com_deleteMemInfo( COM_FILEVAR, COM_FREE, *dmy );
    free(*dmy);
    *dmy = NULL;
    com_mutexUnlock( &gMutexMem, NULL );
}



/* 文字列バッファ処理関数 ***************************************************/

// バッファデータ設定関数名ラベル (COM_SETBUF_t に対応)
static char  *gSetFunc[] = {
    "com_setBuffer", "com_setBufferSize",
    "com_addBuffer", "com_addBufferSize", "com_copyBuffer",
    "com_createBuffer", "com_initBuffer"
};

static char *makeBufferLabel( COM_SETBUF_t iType, const char *iString )
{
    static char  buffLog[COM_WORDBUF_SIZE];
    char*  label = gSetFunc[iType];
    (void)com_strcpy( buffLog, label );
    if( iString ) {
        (void)com_connectString( buffLog, sizeof(buffLog), "(%s)", iString );
    }
    return buffLog;
}

// com_setBuffer～()と com_addBuffer～()の作業用バッファサイズ
#define COM_TMPBUFF_SIZE  COM_DATABUF_SIZE

static pthread_mutex_t  gMutexBuff = PTHREAD_MUTEX_INITIALIZER;
static char  gTmpBuff[COM_TMPBUFF_SIZE];

// oBufのサイズは必ず COM_TMPBUFF_SIZE であること
static size_t setBuffBySize( char *oBuf, size_t iSize )
{
    if( iSize == 0 ) {return strlen(oBuf); }
    oBuf[iSize] = '\0';
    return iSize;
}

static BOOL setBuffer(
        COM_FILEPRM, COM_SETBUF_t iType, com_buf_t *oBuf,
        char *iData, size_t iSize )
{
    char*  label = gSetFunc[iType];
    size_t  len = setBuffBySize( iData, iSize ) + 1;
    if( oBuf->size < len ) {
        oBuf->data = com_reallocfFunc( oBuf->data, len, COM_FILEVAR,
                                       "%s(%s)", label, iData );
        if( !(oBuf->data) ) {oBuf->size = 0;  return false;}
        oBuf->size = len;
    }
    memset( oBuf->data, 0, oBuf->size );
    strcpy( oBuf->data, iData );
    return true;
}

static BOOL initBuffer(
        COM_FILEPRM, COM_SETBUF_t iType, com_buf_t *oBuf,
        char *iData, size_t iSize )
{
    // oBufは 非NULLを期待
    *oBuf = (com_buf_t){ NULL, 0 };
    if( iSize > 0 ) {
        oBuf->data = com_mallocFunc( iSize, COM_FILEVAR,
                                     makeBufferLabel( iType, iData ) );
        if( !(oBuf->data) ) {return false;}
    }
    if( iData ) {
        return setBuffer( COM_FILEVAR, iType, oBuf, iData, strlen(iData) );
    }
    return true;
}

BOOL com_createBufferFunc(
        com_buf_t **oBuf, size_t iSize, COM_FILEPRM, const char *iFormat, ... )
{
    if( !oBuf ) {COM_PRMNG(false);}

    (void)com_mutexLockCom( &gMutexBuff, COM_FILELOC );
    COM_SET_FORMAT( gTmpBuff );
    char*  initString = iFormat ? gTmpBuff : NULL;
    BOOL  result = false;
    *oBuf = com_mallocFunc( sizeof(com_buf_t), COM_FILEVAR,
                            makeBufferLabel( COM_CREBUFF, initString ) );
    if( *oBuf ) {
        result = initBuffer( COM_FILEVAR, COM_CREBUFF,
                             *oBuf, initString, iSize );
        // 初期化が失敗した場合は、バッファ生成自体を中断する
        if( !result ) {com_free( *oBuf );}
    }
    return com_mutexUnlockCom( &gMutexBuff, COM_FILELOC, result );
}

BOOL com_initBufferFunc(
        com_buf_t *oBuf, size_t iSize, COM_FILEPRM, const char *iFormat, ... )
{
    if( !oBuf ) {COM_PRMNG(false);}

    com_mutexLockCom( &gMutexBuff, COM_FILELOC );
    COM_SET_FORMAT( gTmpBuff );
    char*  initString = iFormat ? gTmpBuff : NULL;
    BOOL  result = initBuffer( COM_FILEVAR, COM_INITBUFF,
                               oBuf, initString, iSize );
    // こちらは falseが返っても oBufを解放してはいけない
    return com_mutexUnlockCom( &gMutexBuff, COM_FILELOC, result );
}

BOOL com_setBufferFunc(
        com_buf_t *oBuf, size_t iSize, COM_SETBUF_t iType, COM_FILEPRM,
        const char *iFormat, ... )
{
    char*  label = gSetFunc[iType];
    if( !oBuf || !iFormat ) {COM_PRMNGF(label,false);}
    if( iSize > COM_TMPBUFF_SIZE - 1 ) {COM_PRMNGF(label,false);}

    com_mutexLockCom( &gMutexBuff, __FILE__, __LINE__, label );
    COM_SET_FORMAT( gTmpBuff );
    BOOL  result = setBuffer( COM_FILEVAR, iType, oBuf, gTmpBuff, iSize );
    return com_mutexUnlockCom( &gMutexBuff, __FILE__, __LINE__, label, result );
}

static char gTmpAddBuff[COM_TMPBUFF_SIZE];

BOOL com_addBufferFunc(
        com_buf_t *oBuf, size_t iSize, COM_SETBUF_t iType, COM_FILEPRM,
        const char *iFormat, ... )
{
    char*  label = gSetFunc[iType];
    if( !oBuf || !iFormat ) {COM_PRMNGF(label,false);}
    if( !(oBuf->data) ) {COM_PRMNGF(label,false);}
    if( iSize > COM_TMPBUFF_SIZE - 1 ) {COM_PRMNGF(label,false);}

    com_mutexLockCom( &gMutexBuff, __FILE__, __LINE__, label );
    COM_SET_FORMAT( gTmpAddBuff );
    (void)setBuffBySize( gTmpAddBuff, iSize );
    snprintf( gTmpBuff, sizeof(gTmpBuff), "%s%s", oBuf->data, gTmpAddBuff );
    BOOL  result = setBuffer( COM_FILEVAR, iType, oBuf, gTmpBuff, 0 );
    return com_mutexUnlockCom( &gMutexBuff, __FILE__, __LINE__, label, result );
}

BOOL com_copyBufferFunc(
        com_buf_t *oTarget, const com_buf_t *iSource, COM_FILEPRM )
{
    if( !oTarget || !iSource ) {COM_PRMNG(false);}

    com_skipMemInfo( true );
    BOOL  result =
        setBuffer( COM_FILEVAR, COM_COPYBUFF, oTarget, iSource->data, 0 );
    com_skipMemInfo( false );
    return result;
}

void com_clearBuffer( com_buf_t *oBuf )
{
    if( !oBuf ) {COM_PRMNG();}

    memset( oBuf->data, 0, oBuf->size );
    // oBuf->size は変動なし
}

static void resetBuffer( com_buf_t *oBuf, COM_FILEPRM )
{
    com_freeFunc( &(oBuf->data), COM_FILEVAR );
    oBuf->size = 0;
}

void com_resetBufferFunc( com_buf_t *oBuf, COM_FILEPRM )
{
    if( !oBuf ) {COM_PRMNG();}

    com_skipMemInfo( true );
    resetBuffer( oBuf, COM_FILEVAR );
    com_skipMemInfo( false );
}

void com_freeBufferFunc( com_buf_t **oBuf, COM_FILEPRM )
{
    if( !oBuf ) {COM_PRMNG();}

    if( !(*oBuf) ) {return;}
    com_skipMemInfo( true );
    resetBuffer( *oBuf, COM_FILEVAR );
    com_freeFunc( oBuf, COM_FILEVAR );
    com_skipMemInfo( false );
}



/* 各種変換関連関数 *********************************************************/

static void convertString( char *ioString, BOOL iIsUpper )
{
    for( size_t i = 0;  i < strlen(ioString);  i++ ) {
        if( iIsUpper ) {ioString[i] = (char)toupper( ioString[i] );}
        else           {ioString[i] = (char)tolower( ioString[i] );}
    }
}

void com_convertUpper( char *ioString )
{
    if( !ioString ) {COM_PRMNG();}
    convertString( ioString, true );
}

void com_convertLower( char *ioString )
{
    if( !ioString ) {COM_PRMNG();}
    convertString( ioString, false );
}

static void dispConvertNG(
        const char *iFunc, const char *iString, BOOL iError,
        char *iEndPtr, int iErrno )
{
    // 不正文字検出時
    if( iEndPtr ) {
        if( *iEndPtr != '\0' ) {
            if( iError ) {
                com_error( COM_ERR_CONVERTNG,
                           "%s illegal (%s)(%s)", iFunc, iEndPtr, iString );
            }
            errno = EINVAL;
            return;
        }
    }
    if( !iErrno ) {return;}
    // errno設定時
    if( iError ) {
        com_error(COM_ERR_CONVERTNG, "%s NG (%d)(%s)", iFunc, iErrno, iString);
    }
    errno = iErrno;
}

#define CONVERTVALUE( RESULT, CONVFUNC, STRING, BASE, DISPERROR ) \
    do { \
        if( !(STRING) ) {com_prmNgFunc( COM_FILELOC, NULL ); return 0; } \
        char*  endptr = NULL; \
        errno = 0; \
        (RESULT) = (CONVFUNC)( (STRING), &endptr, (BASE) ); \
        dispConvertNG( __func__, (STRING), (DISPERROR), endptr, errno ); \
    } while(0)

ulong com_strtoul( const char *iString, int iBase, BOOL iError )
{
    ulong  result = 0;
    CONVERTVALUE( result, strtoul, iString, iBase, iError );
    return result;
}

long com_strtol( const char *iString, int iBase, BOOL iError )
{
    long  result = 0;
    CONVERTVALUE( result, strtol, iString, iBase, iError );
    return result;
}

int com_atoi( const char *iString )
{
    long  result = com_strtol( iString, 10, false );
    if( result > INT_MAX ) {result = INT_MAX;}
    if( result < INT_MIN ) {result = INT_MIN;}
    return (int)result;
}

long com_atol( const char *iString )
{
    return com_strtol( iString, 10, false );
}

#define CONVERTFLOAT( RESULT, CONVFUNC, STRING, DISPERROR ) \
    do { \
        if( !(STRING) ) {com_prmNgFunc( COM_FILELOC, NULL ); return 0; } \
        char*  endptr = NULL; \
        errno = 0; \
        (RESULT) = (CONVFUNC)( (STRING), &endptr ); \
        dispConvertNG( __func__, (STRING), (DISPERROR), endptr, errno ); \
    } while(0)

float com_strtof( const char *iString, BOOL iError )
{
    float  result = 0.0;
    CONVERTFLOAT( result, strtof, iString, iError );
    return result;
}

double com_strtod( const char *iString, BOOL iError )
{
    double  result = 0.0;
    CONVERTFLOAT( result, strtod, iString, iError );
    return result;
}

float com_atof( const char *iString )
{
    return com_strtof( iString, false );
}

static size_t checkOctStrings( const char *iString )
{
    size_t  count = 0;
    for( size_t i = 0;  i < strlen(iString);  i++ ) {
        char  target = iString[i];
        if( isxdigit(target) ) {count++; continue;}
        // 空白と制御文字以外は駄目
        if( !isspace(target) && !iscntrl(target) ) {return 0;}
    }
    if( count % 2 ) {return 0;}  // 最終的な文字列長が奇数は駄目
    count /= 2;  // メモリ確保が必要なサイズを算出
    return count;
}

static void convertOct(
        const char *iString, size_t iLen, uchar *oOct, size_t *oLength,
        size_t iBufSize )
{
    char  oct[3] = {0};
    size_t  count = 0;
    for( size_t i = 0;  i < iLen;  i++ ) {
        char  target = iString[i];
        if( !isxdigit(target) ) {continue;}
        oct[count] = target;
        if( (count = (count == 0)) ) {continue;}
        ulong  tmp = com_strtoul( oct, 16, false );
        oOct[(*oLength)++] = (uchar)tmp;
        if( *oLength == iBufSize ) {return;}
    }
}

BOOL com_strtooctFunc(
        const char *iString, uchar **oOct, size_t *oLength, COM_FILEPRM )
{
    if( !iString || !oOct || !oLength ) {COM_PRMNG(false);}

    size_t  bufSize = (*oOct) ? *oLength : 0;
    *oLength = 0;
    size_t  count = checkOctStrings( iString );
    if( !count ) {return false;}
    if( !bufSize ) {
        bufSize = count;
        *oOct = com_mallocFunc( bufSize, COM_FILEVAR,
                                "com_strtooct(%zu)", bufSize );
    }
    if( !(*oOct) ) {return false;}
    convertOct( iString, strlen(iString), *oOct, oLength, bufSize );
    return true;
}

#define CONV_NUM_TO_STR( FORMAT ) \
    do { \
        memset( oBuf, 0, COM_LONG_SIZE ); \
        snprintf( oBuf, COM_LONG_SIZE, (FORMAT), iValue ); \
    } while(0)

void com_ltostr( char *oBuf, long iValue )
{
    CONV_NUM_TO_STR( "%ld" );
}

void com_ultostr( char *oBuf, ulong iValue )
{
    CONV_NUM_TO_STR( "%lu" );
}

void com_bintohex( char *oBuf, uchar iBin, BOOL iCase )
{
    char*  format = iCase ? "%02X" : "%02x";
    snprintf( oBuf, COM_BIN_SIZE, format, iBin );
}

BOOL com_octtostrFunc(
        char **oBuf, size_t iBufSize, void *iOct, size_t iLength, BOOL iCase,
        COM_FILEPRM )
{
    if( !oBuf || !iOct ) {COM_PRMNG(false);}

    if( !iLength ) {return true;}
    if( !(*oBuf) ) {
        iBufSize = iLength * 2 + 1;
        *oBuf = com_mallocFunc( iBufSize, COM_FILEVAR,
                                "com_octtostr(%zu)", iBufSize );
    }
    else {memset( *oBuf, 0, iBufSize );}
    if( !iBufSize || !(*oBuf) ) {return false;}
    uchar*  oct = iOct;
    for( size_t i = 0;  i < iLength;  i++ ) {
        COM_BINTOHEX( bin, oct[i], iCase );
        if( !com_strncat( *oBuf, iBufSize, bin, strlen(bin) ) ) {
            return false;
        }
    }
    return true;
}



/* 文字列操作関連 ***********************************************************/

BOOL com_strncpy(
        char *oTarget, size_t iTargetSize,
        const char *iSource, size_t iCopyLen )
{
    if( !oTarget || !iSource || iTargetSize <= 1 ) {COM_PRMNG(false);}

    BOOL  result = true;
    memset( oTarget, 0, iTargetSize );
    if( iCopyLen > strlen(iSource) ) {iCopyLen = strlen(iSource);}
    iTargetSize--;  // 最後の文字列終端のスペースを確保
    if( iCopyLen > iTargetSize ) {
        iCopyLen = iTargetSize;
        result = false;
    }
    strncpy( oTarget, iSource, iCopyLen );
    // 確実に最後を終端文字にしておく
    oTarget[iCopyLen] = '\0';
    return result;
}

BOOL com_strncat(
        char *oTarget, size_t iTargetSize,
        const char *iSource, size_t iCatLen )
{
    if(!oTarget || !iSource || iTargetSize <= 1 ) {COM_PRMNG(false);}
    size_t  targetLen = strlen(oTarget);
    if( targetLen > iTargetSize - 1 ) {COM_PRMNG(false);}

    if( iCatLen > strlen(iSource) ) {iCatLen = strlen(iSource);}
    iTargetSize--;  // 最後の文字列終端のスペースを確保
    if( targetLen == iTargetSize ) {return false;}  // これ以上連結不可
    BOOL  result = true;
    if( targetLen + iCatLen > iTargetSize ) {
        iCatLen = iTargetSize - targetLen;
        result = false;
    }
    strncat( oTarget, iSource, iCatLen );
    // 確実に最後を終端文字にしておく
    oTarget[targetLen + iCatLen] = '\0';
    return result;
}

static pthread_mutex_t  gMutexConn = PTHREAD_MUTEX_INITIALIZER;
static char  gConnBuff[COM_TEXTBUF_SIZE];

BOOL com_connectString( char *oBuf, size_t iBufSize, const char *iFormat, ... )
{
    if( !oBuf || !iBufSize || !iFormat ) {COM_PRMNG(false);}

    com_mutexLock( &gMutexConn, __func__ );
    COM_SET_FORMAT( gConnBuff );
    BOOL  result = com_strncat( oBuf, iBufSize, gConnBuff, strlen(gConnBuff) );
    com_mutexUnlock( &gMutexConn, __func__ );
    return result;
}

static BOOL checkSearchEnd(
        const char *iPtr, const char *iTop, size_t iEnd, const char *iLabel )
{
    size_t  dist = (size_t)(iPtr - iTop);
    if( iEnd ) {if( dist >= iEnd ) {return true;}}
    else {
        if( dist >= COM_SEARCH_LIMIT ) {
            com_error( COM_ERR_DEBUGNG, "%s() search limit reached(%d)",
                       iLabel, COM_SEARCH_LIMIT );
            return true;
        }
    }
    return false;
}

char *com_searchString(
        const char *iSource, const char *iTarget, long *ioIndex,
        size_t iSearchEnd, BOOL iNoCase )
{
    if( !iSource || !iTarget ) {COM_PRMNG(NULL);}
    if( iSearchEnd && iSearchEnd < strlen(iTarget) ) {
        COM_SET_IF_EXIST( ioIndex, 0 );  return NULL;
    }
    long  idx = 1;
    if( ioIndex ) {if( !(idx = *ioIndex) ) {idx = 1;}}
    const char*  srch = iSource;
    char*  result = NULL;
    long  cnt = 0;
    while( *srch ) {
        if( com_compareString( srch, iTarget, strlen(iTarget), iNoCase ) ) {
            if(++cnt == idx || idx == COM_SEARCH_LAST) {result = (char*)srch;}
            srch += strlen(iTarget);
        }
        else {srch++;}
        if( checkSearchEnd( srch, iSource, iSearchEnd, "com_searchString" ) ) {
            break;
        }
    }
    COM_SET_IF_EXIST( ioIndex, cnt );
    return result;
}

static BOOL checkReplaceArg(
        const char *iFunc, char **oTarget, size_t *oLength,
        const char *iSource, com_replaceCond_t *iCond )
{
    if( !oTarget || !oLength || !iSource || !iCond ) {COM_PRMNGF(iFunc,false);}
    if( !(iCond->replacing) || !(iCond->replaced) )  {COM_PRMNGF(iFunc,false);}
    if( !strcmp(iCond->replacing, iCond->replaced) ) {COM_PRMNGF(iFunc,false);}
    return true;
}

static pthread_mutex_t  gMutexRep = PTHREAD_MUTEX_INITIALIZER;
static char  gRepBuf[COM_TEXTBUF_SIZE];

static void resetReplaceOutput( char **oTarget, size_t *oLength )
{
    COM_CLEAR_BUF( gRepBuf );
    *oTarget = gRepBuf;
    *oLength = 0;
}

static BOOL copyRepBuf( const char *iSource, size_t iSize, size_t *ioSize )
{
    if( !iSize ) {return true;}
    if( !com_strncat( gRepBuf, sizeof(gRepBuf), iSource, iSize ) ) {
        *ioSize = sizeof( gRepBuf ) - 1;
        return false;
    }
    *ioSize += iSize;
    return true;
}

// 文字列置換処理用の引き継ぎデータ
typedef struct {
    com_replaceCond_t*  cond;    // 入力された置換条件
    long  hitCount;              // 置換対象の検索ヒット数
    long  repCount;              // 実際に置換した数
} com_repInf_t;

static const char *replaceString(
        const char *iPos, const char *iSrc,
        size_t *oLength, com_repInf_t *ioInf )
{
    // 見つかったより前の文字列をコピー
    size_t  fwdSize = (size_t)iSrc - (size_t)iPos;
    if( !copyRepBuf( iPos, fwdSize, oLength ) ) {return false;}
    iPos += fwdSize;
    // 置換対象であれば、置換後文字列をセット
    const char*  obj = ioInf->cond->replacing;   // 置換前文字列セット
    long  idx = ioInf->cond->index;
    if( idx == COM_REPLACE_ALL || idx == ++(ioInf->hitCount) ) {
        obj = ioInf->cond->replaced;            // 置換後文字列に変更
        (ioInf->repCount)++;
    }
    if( !copyRepBuf( obj, strlen(obj), oLength ) ) {return false;}
    iPos += strlen( ioInf->cond->replacing );   // 置換前文字列分 移動
    return iPos;
}

long com_replaceString(
        char **oTarget, size_t *oLength, const char *iSource,
        com_replaceCond_t *iCond )
{
    if( !checkReplaceArg( __func__, oTarget, oLength, iSource, iCond ) ) {
        return COM_REPLACE_NG;
    }
    com_mutexLockCom( &gMutexRep, COM_FILELOC );
    const char*  pos = iSource;  // 処理中文字列位置
    const char*  src = NULL;     // 置換対象文字列位置
    resetReplaceOutput( oTarget, oLength );
    com_repInf_t  inf = { iCond, 0, 0 };
    while( (src = strstr( pos, iCond->replacing )) ) {
        if( !(pos = replaceString( pos, src, oLength, &inf )) ) {
            (void)com_mutexUnlockCom( &gMutexRep, COM_FILELOC, true );
            return COM_REPLACE_NG;
        }
    }
    //残りを全てコピー
    BOOL  result = copyRepBuf( pos, strlen(pos), oLength );
    (void)com_mutexUnlockCom( &gMutexRep, COM_FILELOC, true );
    if( !result ) {return COM_REPLACE_NG;}
    return inf.repCount;
}

static BOOL hasNull( const char *iString, size_t iLen )
{
    // iStringが \0 で終端しない可能性を考えて strlen()は使わない
    for( size_t i = 0;  i < iLen;  i++ ) {if( !iString[i] ) {return true;}}
    return false;  // iLen == 0 なら無条件で falseを返すことになる
}

BOOL com_compareString(
        const char *iString1, const char *iString2, size_t iCmpLen,
        BOOL iNoCase )
{
    // NULLでも com_prmNG()によるパラメータNGにはしない
    if( !iString1 || !iString2 ) {return false;}
    // iString1・iString2が iCmpLenより短い場合はアンマッチで抜ける
    if( hasNull(iString1,iCmpLen) || hasNull(iString2, iCmpLen) ){return false;}

    for( size_t i = 0;  ;  i++ ) {
        if( iCmpLen && i == iCmpLen ) {break;}
        int  chr1 = iString1[i];
        int  chr2 = iString2[i];
        if( chr1 == '\0' && chr2 == '\0' ) {break;}
        if( chr1 == '\0' || chr2 == '\0' ) {return false;}
        if( iNoCase ) {
            chr1 = toupper(chr1);
            chr2 = toupper(chr2);
        }
        if( chr1 != chr2 ) {return false;}
    }
    return true;
}

char *com_topString( char *iString, BOOL iCrLf )
{
    if( !iString ) {COM_PRMNG(false);}

    for( ;  *iString;  iString++ ) {
        if(*iString == ' ' || *iString == '\t' || *iString == '\v') {continue;}
        if( iCrLf && ( *iString == '\r' || *iString == '\n' ) ) {continue;}
        return iString;
    }
    return NULL;
}

BOOL com_checkString(
        const char *iString, com_isFunc_t *iCheckFuncs, COM_CHECKOP_t iOp )
{
    if( !iString || !iCheckFuncs ) {COM_PRMNG(false);}

    int  funcCnt = 0;
    for( com_isFunc_t* tmp = iCheckFuncs;  *tmp;  tmp++ ) {funcCnt++;}
    for( ;  *iString;  iString++ ) {
        int  hitCnt = 0;
        for( com_isFunc_t* tmp = iCheckFuncs;  *tmp;  tmp++ ) {
            if( (*tmp)( (int)(*iString) ) ) {hitCnt++;}
        }
        if( iOp == COM_CHECKOP_AND && hitCnt != funcCnt ) {return false;}
        if( iOp == COM_CHECKOP_OR  && !hitCnt ) {return false;}
        if( iOp == COM_CHECKOP_NOT &&  hitCnt ) {return false;}
    }
    return true;
}

static pthread_mutex_t  gMutexForm = PTHREAD_MUTEX_INITIALIZER;
static com_buf_t  gFormString[COM_FORMSTRING_MAX];
static char  gFormBuf[COM_TEXTBUF_SIZE];
static char  gEmpty[] = "";

static void freeFormString( void )
{
    for( int i = 0;  i < COM_FORMSTRING_MAX;  i++ ) {
        com_resetBuffer( &(gFormString[i]) );
    }
}

char *com_getString( const char *iFormat, ... )
{
    static int  bufCount = 0;
    com_mutexLockCom( &gMutexForm, COM_FILELOC );
    COM_SET_FORMAT( gFormBuf );
    BOOL  result = com_setBuffer( &gFormString[bufCount], gFormBuf );
    if( !result ) {
        (void)com_mutexUnlockCom( &gMutexForm, COM_FILELOC, false );
        return gEmpty;
    }
    char*  bufData = gFormString[bufCount].data;
    bufCount = (bufCount + 1) % COM_FORMSTRING_MAX;
    (void)com_mutexUnlockCom( &gMutexForm, COM_FILELOC, true );
    return bufData;
}

long com_searchStrList( const char **iList, const char *iTarget, BOOL iNoCase )
{
    if( !iList || !iTarget ) {COM_PRMNG( COM_NO_DATA ); }
    if( !(*iList) ) {COM_PRMNG( COM_NO_DATA ); }

    long  idx = 0;
    for( ;  iList[idx];  idx++ ) {
        if( com_compareString( iList[idx], iTarget, 0, iNoCase ) ) {
            return idx;
        }
    }
    return COM_NO_DATA;
}



/* 日時取得関連 *************************************************************/

static void getDateTime(
        const struct tm *iTm, const struct timeval *iTv,
        COM_TIMEFORM_TYPE_t iType, com_time_t *oTm )
{
    *oTm = (com_time_t){
        iTm->tm_year + 1900, iTm->tm_mon + 1, iTm->tm_mday, iTm->tm_wday,
        iTm->tm_hour, iTm->tm_min, iTm->tm_sec, 0
    };
    if( iType == COM_FORM_SIMPLE ) {oTm->year = iTm->tm_year - 100;}
    if( iTv ) {oTm->usec = iTv->tv_usec;}
}

static void makeTmData(
        const struct tm *iTm, const struct timeval *iTv,
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal,
        const char *iFunc )
{
    if( iType != COM_FORM_DETAIL && iType != COM_FORM_SIMPLE ) {
        COM_PRMNGF(iFunc,);
    }
    const char*  DATE_FMT[] = { "%04ld/%02ld/%02ld", "%02ld%02ld%02ld" };
    const char*  TIME_FMT[] = { "%02ld:%02ld:%02ld.%06ld", "%02ld%02ld%02ld" };
    com_time_t  ctm;
    getDateTime( iTm, iTv, iType, &ctm );
    size_t  size = 0;
    if( oDate ) {
        size = (iType == COM_FORM_SIMPLE ) ? COM_DATE_SSIZE : COM_DATE_DSIZE;
        snprintf( oDate, size,
                  DATE_FMT[iType], ctm.year, ctm.mon, ctm.day );
    }
    if( oTime ) {
        size = (iType == COM_FORM_SIMPLE) ? COM_TIME_SSIZE : COM_TIME_DSIZE;
        snprintf( oTime, size,
                  TIME_FMT[iType], ctm.hour, ctm.min, ctm.sec, ctm.usec );
    }
    COM_SET_IF_EXIST( oVal, ctm );
}

typedef struct tm*(*com_timeFunc)(const time_t *timep, struct tm *result );

static void setTime(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal,
        const time_t *iTime, BOOL iLocal )
{
    struct tm  result;
    com_timeFunc  func = iLocal ? localtime_r : gmtime_r;
    if( !func( iTime, &result ) ) {
        com_error( COM_ERR_TIMENG, "convert NG (%ld)", *iTime );
        return;
    }
    makeTmData( &result, NULL, iType, oDate, oTime, oVal, __func__ );
}

void com_setTime(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal,
        const time_t *iTime )
{
    if( !iTime ) {COM_PRMNG();}
    setTime( iType, oDate, oTime, oVal, iTime, true );
}

void com_setTimeGmt(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal,
        const time_t *iTime )
{
    if( !iTime ) {COM_PRMNG();}
    setTime( iType, oDate, oTime, oVal, iTime, false );
}

static void setTimeVal(
        const struct timeval *iTv, COM_TIMEFORM_TYPE_t iType,
        char *oDate, char *oTime, com_time_t *oVal, const char *iFunc,
        BOOL iLocal )
{
    struct tm  result;
    com_timeFunc  func = iLocal ? localtime_r : gmtime_r;
    if( !func( &(iTv->tv_sec), &result ) ) {
        com_error( COM_ERR_TIMENG, "%s convert NG (%ld)", iFunc, iTv->tv_sec );
        return;
    }
    makeTmData( &result, iTv, iType, oDate, oTime, oVal, iFunc );
}

void com_setTimeval(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal,
        const struct timeval *iTv )
{
    if( !iTv ) {COM_PRMNG();}
    setTimeVal( iTv, iType, oDate, oTime, oVal, __func__, true );
}

void com_setTimevalGmt(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal,
        const struct timeval *iTv )
{
    if( !iTv ) {COM_PRMNG();}
    setTimeVal( iTv, iType, oDate, oTime, oVal, __func__, false );
}

BOOL com_getCurrentTime(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal )
{
    struct timeval  tv;
    if( !com_gettimeofday( &tv, NULL ) ) {return false;}
    setTimeVal( &tv, iType, oDate, oTime, oVal, __func__, true );
    return true;
}

BOOL com_getCurrentTimeGmt(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal )
{
    struct timeval  tv;
    if( !com_gettimeofday( &tv, NULL ) ) {return false;}
    setTimeVal( &tv, iType, oDate, oTime, oVal, __func__, false );
    return true;
}

BOOL com_gettimeofday( struct timeval *oTime, const char *iLabel )
{
    if( !oTime ) {COM_PRMNG(false);}

    int  result = gettimeofday( oTime, NULL );
    if( 0 > result ) {
        const char* label = !iLabel ? "current time" : iLabel;
        com_error( COM_ERR_TIMENG, "fail to get %s[%d]", label, errno );
        return false;
    }
    return true;
}

BOOL com_startStopwatch( com_stopwatch_t *oWatch )
{
    if( !oWatch ) {COM_PRMNG(false);}

    if( !com_gettimeofday( &(oWatch->start), "start time" ) ) {return false;}
    return true;
}

BOOL com_checkStopwatch( com_stopwatch_t *ioWatch )
{
    if( !ioWatch ) {COM_PRMNG(false);}

    struct timeval  now;
    if( !com_gettimeofday( &now, "check time" ) ) {return false;}
    ioWatch->passed = (struct timeval){
                          now.tv_sec  - ioWatch->start.tv_sec,
                          now.tv_usec - ioWatch->start.tv_usec
    };
    if( ioWatch->passed.tv_usec < 0 ) {
        (ioWatch->passed.tv_sec)--;
        ioWatch->passed.tv_usec += 1000000;
    }
    return true;
}

static time_t calcTimeVal( time_t *oValue, time_t iSec, int iDivine )
{
    *oValue = iSec / iDivine;
    return (iSec % iDivine);
}

enum {
    SEC_MIN  = 60,                // 1分の秒数
    SEC_HOUR = SEC_MIN * 60,      // 1時間の秒数
    SEC_DAY  = SEC_HOUR * 24      // 1日の秒数
};

void com_convertSecond( com_time_t *oTime, const struct timeval *iTimeVal )
{
    memset( oTime, 0, sizeof(*oTime) );
    time_t  tmpSec = iTimeVal->tv_sec;
    tmpSec     = calcTimeVal( &(oTime->day),   tmpSec, SEC_DAY );
    tmpSec     = calcTimeVal( &(oTime->hour),  tmpSec, SEC_HOUR );
    oTime->sec = calcTimeVal( &(oTime->min),   tmpSec, SEC_MIN );
    oTime->usec = iTimeVal->tv_usec;
}

COM_WD_TYPE_t com_getDotw( time_t iYear, time_t iMonth, time_t iDay )
{
    if( iYear <= 1752 || iMonth < 1 || iMonth > 12 || iDay < 1 || iDay > 31 ) {
        COM_PRMNG(COM_WD_NG);
    }

    // C言語FAQ 20.31の「坂本智彦がポストした気の利いたコード」を流用
    static time_t  t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    iYear -= iMonth < 3;
    return ( iYear + iYear / 4 - iYear / 100 + iYear / 400
             + t[iMonth - 1] + iDay ) % 7;
}

// 曜日文字列リスト
static const char*  gWoDJp1[] = {
    "日", "月", "火", "水", "木", "金", "土"
};
static const char*  gWoDJp2[] = {
    "日曜日", "月曜日", "火曜日", "水曜日", "木曜日", "金曜日", "土曜日"
};
static const char*  gWoDEn1[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char*  gWoDEn2[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thusday", "Friday", "Saturday"
};
static const char**  gWoDList[] = { gWoDJp1, gWoDJp2, gWoDEn1, gWoDEn2 };

const char *com_strDotw( COM_WD_TYPE_t iDotw, COM_WDSTR_TYPE_t iType )
{
    if( iDotw < COM_WD_SUNDAY || iDotw > COM_WD_SATURDAY ) {COM_PRMNG(NULL);}
    if( iType < COM_WDSTR_JP1 || iType > COM_WDSTR_EN2 ) {COM_PRMNG(NULL);}
    return gWoDList[iType - 1][iDotw];
}
// コード上 上記の COM_WDSTR_JP1 を 0 にすると警告が出る。
// 「列挙体定義上 正数しか無いので 0未満にはならない」という指摘なのだが、
// C言語は列挙体定義しても、それ以外の値を普通に入れることが可能なため、
// 数値チェックが無理なく出来るように COM_WDSTR_JP1 を 1 とした。
// そのため最後に文字列を返すときの配列変数指定で iType - 1 となっている。



/* 文字列チェーンデータ処理 *************************************************/

static com_strChain_t *createNewChainData( COM_FILEPRM, const char *iKey )
{
    com_strChain_t*  new = com_mallocFunc( sizeof(com_strChain_t), COM_FILEVAR,
                                           "newChainData(%s)", iKey );
    if( !new ) {return NULL;}
    new->data =
        com_strdupFunc( iKey, COM_FILEVAR, "newChainData->data(%s)", iKey );
    if( !(new->data) ) {
        com_free( new );
        return NULL;
    }
    new->next = NULL;
    return new;
}

static pthread_mutex_t  gMutexKey = PTHREAD_MUTEX_INITIALIZER;
static char  gKeyBuff[COM_TEXTBUF_SIZE];

BOOL com_addChainDataFunc(
        com_strChain_t **ioChain, COM_FILEPRM, const char *iFormat, ... )
{
    if( !ioChain || !iFormat ) {COM_PRMNG(false);}

    com_mutexLockCom( &gMutexKey, COM_FILELOC );
    COM_SET_FORMAT( gKeyBuff );
    com_strChain_t*  new = createNewChainData( COM_FILEVAR, gKeyBuff );
    (void)com_mutexUnlockCom( &gMutexKey, COM_FILELOC, true );
    if( !new ) {return false;}
    // 何もないときはそれが先頭データ
    if( !(*ioChain) ) {
        *ioChain = new;
        return true;
    }
    // 既に何かあったら末尾に追加
    com_strChain_t*  tmp = *ioChain;
    while( tmp->next ) {tmp = tmp->next;}
    tmp->next = new;
    return true;
}

// 数値ソートが必要なときに trueにする。
static __thread BOOL gNeedNumSort = false;

static BOOL compareResult( char *iNew, char *iExist, COM_ADDCHAIN_t iSort )
{
    if( gNeedNumSort ) {
        long  newVal = com_atol( iNew );
        long  extVal = com_atol( iExist );
        if( newVal == extVal ) {return true;}
        if( iSort == COM_SORT_ASCEND  && (newVal <= extVal) ) {return true;}
        if( iSort == COM_SORT_DESCEND && (newVal >= extVal) ) {return true;}
    }
    else {
        int  result = strcmp( iNew, iExist );
        if( !result ) {return true;}
        if( iSort == COM_SORT_ASCEND  && (result <= 0) ) {return true;}
        if( iSort == COM_SORT_DESCEND && (result >= 0) ) {return true;}
    }
    return false;
}
    
BOOL com_sortAddChainDataFunc(
        com_strChain_t **ioChain, COM_ADDCHAIN_t iSort, COM_FILEPRM,
        const char *iFormat, ... )
{
    if( !ioChain || !iFormat ||
        (iSort != COM_SORT_ASCEND && iSort != COM_SORT_DESCEND ) )
    {
        COM_PRMNG(false);
    }

    com_mutexLockCom( &gMutexKey, COM_FILELOC );
    COM_SET_FORMAT( gKeyBuff );
    com_strChain_t*  new = createNewChainData( COM_FILEVAR, gKeyBuff );
    (void)com_mutexUnlockCom( &gMutexKey, COM_FILELOC, true );
    if( !new ) {return false;}

    com_strChain_t*  last = NULL;
    for( com_strChain_t* tmp = *ioChain;  tmp;  tmp = tmp->next ) {
        if( compareResult( new->data, tmp->data, iSort ) ) {break;}
        last = tmp;
    }
    if( !last ) { COM_RELAY( new->next, *ioChain, new ); }  // 先頭に追加
    else { COM_RELAY( new->next, last->next, new ); }       // 以降に追加
    gNeedNumSort = false;  // デフォルトは文字列ソート
    return true;
}

com_strChain_t *com_searchChainData(
        com_strChain_t *iChain, const char *iFormat, ... )
{
    if( !iFormat ) {COM_PRMNG(NULL);}

    com_mutexLock( &gMutexKey, __func__ );
    COM_SET_FORMAT( gKeyBuff );
    com_strChain_t*  result = NULL;
    for( com_strChain_t* tmp = iChain;  tmp;  tmp = tmp->next ) {
        if( !strcmp( gKeyBuff, tmp->data ) ) {result = tmp;  break;}
    }
    com_mutexUnlock( &gMutexKey, __func__ );
    return result;
}

static com_strChain_t *cutChainData(
        COM_FILEPRM, com_strChain_t **oChain, com_strChain_t *oForward,
        com_strChain_t **oTarget )
{
    com_strChain_t*  next = (*oTarget)->next;
    com_strChain_t*  target = *oTarget;
    if( oForward ) {oForward->next = next;} else {*oChain = next;}
    com_skipMemInfo( true );
    com_freeFunc( &(target->data), COM_FILEVAR );
    com_freeFunc( &target, COM_FILEVAR );
    com_skipMemInfo( false );
    return next;
}

long com_deleteChainDataFunc(
        com_strChain_t **ioChain, BOOL iContinue, COM_FILEPRM,
        const char *iFormat, ... )
{
    if( !ioChain || !iFormat ) {COM_PRMNG(0);}

    com_mutexLockCom( &gMutexKey, COM_FILELOC );
    COM_SET_FORMAT( gKeyBuff );
    long  count = 0;
    com_strChain_t*  fwd = NULL;
    com_strChain_t*  tmp = *ioChain;
    while( tmp ) {
        if( !strcmp( gKeyBuff, tmp->data ) ) {
            count++;
            tmp = cutChainData( COM_FILEVAR, ioChain, fwd, &tmp );
            if( !iContinue ) {break;}
        }
        else { COM_RELAY( fwd, tmp, tmp->next ); }
    }
    (void)com_mutexUnlockCom( &gMutexKey, COM_FILELOC, true );
    return count;
}

BOOL com_pushChainDataFunc(
        com_strChain_t **ioChain, COM_FILEPRM, const char *iFormat, ... )
{
    if( !ioChain || !iFormat ) {COM_PRMNG(false);}

    com_mutexLockCom( &gMutexKey, COM_FILELOC );
    COM_SET_FORMAT( gKeyBuff );
    com_strChain_t* new = createNewChainData( COM_FILEVAR, gKeyBuff );
    (void)com_mutexUnlockCom( &gMutexKey, COM_FILELOC, true );
    if( !new ) {return false;}
    // 無条件で先頭に追加
    COM_RELAY( new->next, *ioChain, new );
    return true;
}

// pop用バッファ (com_popChainData()で捕捉し、finalizeCom()で解放する)
static com_buf_t  gPopBuff;

char *com_popChainDataFunc( com_strChain_t **ioChain, COM_FILEPRM )
{
    if( !ioChain ) {COM_PRMNG(NULL);}

    // 先頭データ内容を渡して削除
    if( !(*ioChain) ) {return NULL;}
    com_mutexLockCom( &gMutexKey, COM_FILELOC );
    char*  result = NULL;
    if( com_setBuffer( &gPopBuff, (*ioChain)->data ) ) {
        (void)cutChainData( COM_FILEVAR, ioChain, NULL, ioChain );
        result = gPopBuff.data;
    }
    (void)com_mutexUnlockCom( &gMutexKey, COM_FILELOC, true );
    return result;
}

#define CONNECT_CHAIN( STR, LEN ) \
    if( !com_strncat( oResult, iSize, (STR), (LEN) ) ) { \
        resultSize = COM_SIZE_OVER; \
        break; \
    } \
    do{} while(0)

static size_t connectChainData(
        const com_strChain_t *iChain, char *oResult, size_t iSize )
{
    size_t  resultSize = 0;
    for( const com_strChain_t* tmp = iChain;  tmp;  tmp = tmp->next ) {
        CONNECT_CHAIN( tmp->data, strlen(tmp->data) );
        CONNECT_CHAIN( " ", 1 );
        resultSize += strlen( oResult );
    }
    return resultSize;
}

size_t com_spreadChainData(
        const com_strChain_t *iChain, char *oResult, size_t iSize )
{
    if( !oResult || iSize < 2 || iSize >= COM_SIZE_OVER ) {COM_PRMNG(0);}

    memset( oResult, 0, iSize );
    return connectChainData( iChain, oResult, iSize );
}

void com_freeChainDataFunc( com_strChain_t **ioChain, COM_FILEPRM )
{
    if( !ioChain ) {COM_PRMNG();}

    com_skipMemInfo( true );
    com_strChain_t*  next = NULL;
    com_strChain_t*  tmp = *ioChain;
    while( tmp ) {
        next = tmp->next;
        com_freeFunc( &(tmp->data), COM_FILEVAR );
        com_freeFunc( &tmp, COM_FILEVAR );
        tmp = next;
    }
    *ioChain = NULL;
    com_skipMemInfo( false );
}

static pthread_mutex_t  gMutexNum = PTHREAD_MUTEX_INITIALIZER;

static char *getNumStr( long iNum )
{
    static char  numstr[COM_LONG_SIZE];
    com_ltostr( numstr, iNum );
    return numstr;
}

BOOL com_addChainNumFunc( com_strChain_t **ioChain, long iKey, COM_FILEPRM )
{
    if( !ioChain ) {COM_PRMNG(false);}

    com_mutexLockCom( &gMutexNum, COM_FILELOC );
    BOOL  result =
        com_addChainDataFunc( ioChain, COM_FILEVAR, getNumStr(iKey) );
    (void)com_mutexUnlockCom( &gMutexNum, COM_FILELOC, true );
    return result;
}

BOOL com_sortAddChainNumFunc(
        com_strChain_t **ioChain, COM_ADDCHAIN_t iSort, long iKey, COM_FILEPRM )
{
    if( !ioChain || (iSort != COM_SORT_ASCEND && iSort != COM_SORT_DESCEND) ) {
        COM_PRMNG(false);
    }

    com_mutexLockCom( &gMutexNum, COM_FILELOC );
    gNeedNumSort = true;
    BOOL  result = com_sortAddChainDataFunc( ioChain, iSort, COM_FILEVAR,
                                             getNumStr(iKey) );
    (void)com_mutexUnlockCom( &gMutexNum, COM_FILELOC, true );
    return result;
}

com_strChain_t *com_searchChainNum( com_strChain_t *iChain, long iKey )
{
    com_mutexLockCom( &gMutexNum, COM_FILELOC );
    com_strChain_t*  result = com_searchChainData( iChain, getNumStr(iKey) );
    (void)com_mutexUnlockCom( &gMutexNum, COM_FILELOC, true );
    return result;
}

long com_deleteChainNumFunc(
        com_strChain_t **ioChain, BOOL iContinue, long iKey, COM_FILEPRM )
{
    if( !ioChain ) {COM_PRMNG(0);}

    com_mutexLockCom( &gMutexNum, COM_FILELOC );
    long  result = com_deleteChainDataFunc( ioChain, iContinue, COM_FILEVAR,
                                            getNumStr(iKey) );
    (void)com_mutexUnlockCom( &gMutexNum, COM_FILELOC, true );
    return result;
}

BOOL com_pushChainNumFunc( com_strChain_t **ioChain, long iKey, COM_FILEPRM )
{
    if( !ioChain ) {COM_PRMNG(false);}

    com_mutexLockCom( &gMutexNum, COM_FILELOC );
    BOOL  result = com_pushChainDataFunc(ioChain, COM_FILEVAR, getNumStr(iKey));
    (void)com_mutexUnlockCom( &gMutexNum, COM_FILELOC, true );
    return result;
}

long com_popChainNumFunc( com_strChain_t **ioChain, COM_FILEPRM )
{
    if( !ioChain ) {COM_PRMNG(0);}

    com_mutexLockCom( &gMutexNum, COM_FILELOC );
    char*  tmp = com_popChainDataFunc( ioChain, COM_FILEVAR );
    (void)com_mutexUnlockCom( &gMutexNum, COM_FILELOC, true );
    if( !tmp ) {return 0;}
    return com_atol(tmp);
}

size_t com_spreadChainNum(
        const com_strChain_t *iChain, char *oResult, size_t iSize )
{
    if( !oResult || iSize < 2 || iSize >= COM_SIZE_OVER ) {COM_PRMNG(0);}

    return com_spreadChainData( iChain, oResult, iSize );
}

void com_freeChainNumFunc( com_strChain_t **ioChain, COM_FILEPRM )
{
    com_mutexLockCom( &gMutexNum, COM_FILELOC );
    com_freeChainDataFunc( ioChain, COM_FILEVAR );
    (void)com_mutexUnlockCom( &gMutexNum, COM_FILELOC, true );
}



/* ハッシュテーブル処理 *****************************************************/

static pthread_mutex_t  gMutexHash = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    const void*  data;
    size_t       size;
} com_hashData_t;

typedef struct com_hashUnit {
    com_hashData_t        key;
    com_hashData_t        data;
    struct com_hashUnit*  next;
} com_hashUnit_t;

typedef struct com_hashMng {
    size_t            tableSize;
    com_calcHashKey   func;
    com_hashUnit_t**  table;
} com_hashMng_t;

static com_hashMng_t*  gHash = NULL;
static long  gHashCount = 0;

static BOOL initHash(
        com_hashData_t *oTarget, const void *iAddr, size_t iSize )
{
    if( !iAddr ) {return false;}
    *oTarget = (com_hashData_t){ iAddr, iSize };
    return true;
}

static void freeUnit( COM_FILEPRM, com_hashUnit_t **oTarget )
{
    if( !(*oTarget) ) {return;}
    com_freeFunc( &((*oTarget)->key.data), COM_FILEVAR );
    com_freeFunc( &((*oTarget)->data.data), COM_FILEVAR );
    com_freeFunc( oTarget, COM_FILEVAR );
}

static void freeTable( COM_FILEPRM, com_hashUnit_t *oTable )
{
    com_hashUnit_t*  next = NULL;
    while( oTable ) {
        next = oTable->next;
        freeUnit( COM_FILEVAR, &oTable );
        oTable = next;
    }
}

static void freeAllHash( COM_FILEPRM )
{
    if( !gHash || !gHashCount ) {return;}
    for( int i = 0;  i < gHashCount;  i++ ) {
        if( gHash[i].tableSize > 0 ) {com_cancelHashFunc( i, COM_FILEVAR );}
    }
    com_freeFunc( &gHash, COM_FILEVAR );
}

com_hashId_t com_registerHashFunc(
        size_t iTableSize, com_calcHashKey iFunc, COM_FILEPRM )
{
    if( gHashCount == COM_HASHID_MAX ) {
        com_errorExit( COM_ERR_HASHNG, "cannot create more hash table" );
    }
    com_mutexLockCom( &gMutexHash, COM_FILELOC );
    com_hashUnit_t**  newTable =
        com_mallocFunc( sizeof(com_hashUnit_t*) * iTableSize, COM_FILEVAR,
                        "newHashTable(%ld)", gHashCount );
    if( !newTable ) {
        (void)com_mutexUnlockCom( &gMutexHash, COM_FILELOC, true );
        com_errorExit( COM_ERR_HASHNG, "fail to create hash table" );
    }
    for( size_t i = 0;  i < iTableSize;  i++ ) {newTable[i] = NULL;}
    gHash = com_reallocfFunc( gHash,
                              sizeof(com_hashMng_t) * ((size_t)gHashCount + 1),
                              COM_FILEVAR, "newHashMng(%ld)", gHashCount );
    if( !gHash ) {
        (void)com_mutexUnlockCom( &gMutexHash, COM_FILELOC, true );
        com_errorExit( COM_ERR_HASHNG, "fail to create hash manage data" );
    }
    gHash[gHashCount++] = (com_hashMng_t){ iTableSize, iFunc, newTable };
    (void)com_mutexUnlockCom( &gMutexHash, COM_FILELOC, true );
    return (gHashCount - 1);
}

BOOL com_checkHash( com_hashId_t iID )
{
    com_mutexLock( &gMutexHash, __func__ );
    BOOL  result = true;
    if( iID < 0 || iID >= gHashCount ) {result = false;}
    else if( !(gHash[iID].tableSize) ) {result = false;}
    com_mutexUnlock( &gMutexHash, __func__ );
    return result;
}

// テーブルデータは削除するが、gHashCountや gHash自体のサイズは変えない。
// tableSizeを 0にすることで、以後このIDのテーブルは無効という判定になる。
// 削除されたIDの再利用はしない。

void com_cancelHashFunc( com_hashId_t iID, COM_FILEPRM )
{
    if( !com_checkHash( iID ) ) {COM_PRMNG();}

    com_skipMemInfo( true );
    for( size_t i = 0;  i < gHash[iID].tableSize;  i++ ) {
        freeTable( COM_FILEVAR, gHash[iID].table[i] );
    }
    com_freeFunc( &(gHash[iID].table), COM_FILEVAR );
    gHash[iID].tableSize = 0;
    com_skipMemInfo( false );
}

// ハッシュキーの計算に他のロジックを使いたい場合は
// com_registerHash()で計算用関数のアドレスを渡す。

static com_hash_t calcHashKey(
        const void *iKey, size_t iKeySize, size_t iTableSize )
{
    com_hash_t  sum = 0;
    const uchar*  tmp = iKey;
    for( size_t i = 0;  i < iKeySize;  i++ ) {
        sum = (sum << 3) + (com_hash_t)(tmp[i]);
    }
    return (sum & iTableSize);
}

static com_hashUnit_t **getTop( com_hashId_t iID, com_hashData_t *iKey )
{
    com_calcHashKey  func = gHash[iID].func;
    if( !func ) {func = calcHashKey;}
    com_hash_t  key = func( iKey->data, iKey->size, gHash[iID].tableSize );
    // 与えられたキーが存在するテーブルの先頭アドレスを返す
    return (gHash[iID].table + key);
}

static BOOL compareHashData( com_hashData_t *iData1, com_hashData_t *iData2 )
{
    if( iData1->size != iData2->size ) {return false;}
    return !(memcmp( iData1->data, iData2->data, iData2->size ));
}

static com_hashUnit_t *searchHashData(
        com_hashUnit_t *iUnit, com_hashData_t *iKey )
{
    // iUnitは getTop()で与えられたアドレスのポイント先が来ることを想定
    for( ;  iUnit;  iUnit = iUnit->next ) {
        // iKey指定ありの場合、キーが一致するデータを検索
        if( iKey ) {if(compareHashData(&(iUnit->key), iKey)) {return iUnit;}}
        // iKey指定なし(NULL)の場合、最終データを検索
        else {if( !(iUnit->next) ) {return iUnit;}}
    }
    // iKeyありなら検索未ヒット、iKeyなしならデータ無しでここに来る
    return NULL;
}

static com_hashUnit_t *getNewUnit( COM_FILEPRM )
{
    com_hashUnit_t*  new = com_mallocFunc( sizeof(com_hashUnit_t), COM_FILEVAR,
                                           "getNewUnit" );
    if( !new ) {return NULL;}
    *new = (com_hashUnit_t){ {NULL, 0}, {NULL, 0}, NULL };
    return new;
}

static BOOL initHashData(
        COM_FILEPRM, const com_hashData_t *iData, com_hashData_t *oData,
        const char *iLabel )
{
    void*  tmp = com_reallocFunc( (void*)oData->data, iData->size, COM_FILEVAR,
                                  iLabel );
    if( !tmp ) {return false;}
    memcpy( tmp, iData->data, iData->size );
    *oData = (com_hashData_t){ tmp, iData->size };
    return true;
}

static COM_HASH_t addNG( com_hashUnit_t **iNew )
{
    freeUnit( COM_FILELOC, iNew );
    return COM_HASH_NG;
}

static COM_HASH_t addHashData(
        COM_FILEPRM, com_hashUnit_t **iTop, const com_hashData_t *iKey,
        const com_hashData_t *iData )
{
    com_hashUnit_t*  new = getNewUnit( COM_FILEVAR );
    if( !new ) {return COM_HASH_NG;}
    if( !initHashData( COM_FILEVAR, iKey, &(new->key), "addKey" ) ) {
        return addNG( &new );
    }
    if( !initHashData( COM_FILEVAR, iData, &(new->data), "addData" ) ) {
        return addNG( &new );
    }
    new->next = NULL;

    com_hashUnit_t*  last = searchHashData( *iTop, NULL );
    if( !last ) {*iTop = new;} else {last->next = new;}

    return COM_HASH_OK;
}

static COM_HASH_t overwriteHashData(
        COM_FILEPRM, com_hashUnit_t *iPos, com_hashData_t *iData )
{
    if( !initHashData( COM_FILEVAR, iData, &iPos->data, "overwriteData" ) ) {
        return COM_HASH_NG;
    }
    return COM_HASH_OVERWRITE;
}

COM_HASH_t com_addHashFunc(
        com_hashId_t iID, BOOL iOverwrite, const void *iKey, size_t iKeySize,
        const void *iData, size_t iDataSize, COM_FILEPRM )
{
    com_hashData_t  key, data;
    if( !com_checkHash( iID ) ) {COM_PRMNG(COM_HASH_NG);}
    if( !initHash( &key, iKey, iKeySize ) ) {COM_PRMNG(COM_HASH_NG);}
    if( !initHash( &data, iData, iDataSize ) ) {COM_PRMNG(COM_HASH_NG);}

    com_skipMemInfo( true );
    com_hashUnit_t**  top = getTop( iID, &key );
    com_hashUnit_t*  cur = searchHashData( *top, &key );
    COM_HASH_t  result = COM_HASH_NG;
    if( !cur ) {result = addHashData( COM_FILEVAR, top, &key, &data );}
    else {
        if(iOverwrite) {result = overwriteHashData( COM_FILEVAR, cur, &data );}
        else {result = COM_HASH_EXIST;}
    }
    com_skipMemInfo( false );
    return result;
}

BOOL com_searchHash(
        com_hashId_t iID, const void *iKey, size_t iKeySize,
        const void **oData, size_t *oDataSize )
{
    if( !com_checkHash( iID ) ) {COM_PRMNG(false);}
    com_hashData_t  key;
    if( !initHash( &key, iKey, iKeySize ) ) {COM_PRMNG(false);}

    com_hashUnit_t*  tmp = searchHashData( *(getTop( iID, &key )), &key );
    if( !tmp ) {return false;}
    COM_SET_IF_EXIST( oData, tmp->data.data );
    COM_SET_IF_EXIST( oDataSize, tmp->data.size );
    return true;
}

BOOL com_deleteHashFunc(
        com_hashId_t iID, const void *iKey, size_t iKeySize, COM_FILEPRM )
{
    if( !com_checkHash( iID ) ) {COM_PRMNG(false);}
    com_hashData_t  key;
    if( !initHash( &key, iKey, iKeySize ) ) {COM_PRMNG(false);}

    com_hashUnit_t**  top = getTop( iID, &key );
    // 該当キーなしの場合は、そのままOKで返すことにする
    com_hashUnit_t*  tmp = searchHashData( *top, &key );
    if( !tmp ) {return true;}

    com_hashUnit_t*  prev = NULL;
    if( *top == tmp ) {*top = tmp->next;}
    else {
        prev = *top;
        while( prev->next != tmp ) {prev = prev->next;}
        prev->next = tmp->next;
    }
    com_skipMemInfo( true );
    freeUnit( COM_FILEVAR, &tmp );
    com_skipMemInfo( false );
    return true;
}



/* ソートテーブル処理 *******************************************************/

void com_initializeSortTable( com_sortTable_t *oTable, COM_SORT_MATCH_t iAct )
{
    if( !oTable ) {COM_PRMNG();}

    *oTable = (com_sortTable_t){ 0, iAct, 0, NULL, NULL };
}

static BOOL setSortData( COM_FILEPRM, com_sort_t *oTarget, com_sort_t *iData )
{
    void*  tmp = com_mallocFunc( iData->size, COM_FILEVAR,
                                 "new sortData(%ld)", iData->key );
    if( !tmp ) {return false;}
    *oTarget = *iData;
    oTarget->data = tmp;
    memcpy( oTarget->data, iData->data, iData->size );
    return true;
}

static void freeSortData( COM_FILEPRM, com_sort_t *oData )
{
    if( !oData ) {return;}
    if( oData->size ) {
        com_freeFunc( &(oData->data), COM_FILEVAR );
        oData->size = 0;
    }
}

BOOL com_setSortDataFunc( com_sort_t *oTarget, com_sort_t *iData, COM_FILEPRM )
{
    if( !oTarget || !iData ) {COM_PRMNG(false);}

    com_skipMemInfo( true );
    if( oTarget->data ) {
        iData->key = oTarget->key;   // 既存データなので keyは変更させない
        iData->range = oTarget->range;
        freeSortData( COM_FILELOC, oTarget );
    }
    BOOL  result = setSortData( COM_FILEVAR, oTarget, iData );
    com_skipMemInfo( false );
    return result;
}

static void resizeSortSearch( COM_FILEPRM, com_sortTable_t *oTable )
{
    long  cnt = oTable->count;
    // 検索結果格納用領域の確保
    oTable->search = com_reallocfFunc( oTable->search,
                                       sizeof(com_sort_t*) * cnt, COM_FILEVAR,
                                       "resize sort search(%ld)", cnt );
    // 確保できない場合は最初に見つかったもののみを検索するようになる
}

static BOOL resizeSortTable( COM_FILEPRM, com_sortTable_t *oTable, long iMod )
{
    return com_realloctFunc( &(oTable->table), sizeof(com_sort_t),
                             &(oTable->count), iMod, COM_FILEVAR,
                             "resize sort table(%ld)", oTable->count + iMod );
    // 関数処理を com_realloct()で共通化した
}

// 二分検索(見つからない場合、追加されるべき位置を特定)
static BOOL binarySearchPos( com_sortTable_t *iTable, long iKey, long *oPos )
{
    com_sort_t*  tbl = iTable->table;
    long  posMax = iTable->count - 1;
    long  posMin = 0;
    if( tbl[posMin].key > iKey ) {*oPos = posMin;  return false;}
    if( tbl[posMax].key < iKey ) {*oPos = posMax + 1;  return false;}

    *oPos = posMax / 2;
    BOOL  hit = false;
    while(1) {
        if( tbl[*oPos].key > iKey ) {posMax = *oPos - 1;}
        else if( tbl[*oPos].key < iKey ) {posMin = *oPos + 1;}
        else {hit = true;  break;}

        if( posMin > posMax ) {break;}
        *oPos = posMin + (posMax - posMin) / 2;
    }
    if( !hit ) {if( tbl[*oPos].key < iKey ) {(*oPos)++;}}
    // 同一キーが複数ある場合に備え、その先頭データを探す
    for( ;  *oPos > 0;  (*oPos)-- ) {if( tbl[*oPos-1].key != iKey ) {break;}}
    return hit;
}

// データ重複があった場合の処理(返り値は true＝要処理継続)
static BOOL solveCollision(
        com_sortTable_t *oTable, long *ioPos, com_sort_t *iData, BOOL *oResult )
{
    if( oTable->matchAction == COM_SORT_SKIP ) {
        *oResult = false;
        return false;
    }
    com_sort_t*  tmp = oTable->table;
    if( oTable->matchAction == COM_SORT_OVERWRITE ) {
        *oResult = com_setSortData( &tmp[*ioPos], iData );
        return false;
    }
    // oTable->matchAction == COM_SORT_MULTI の場合、同一キーの最後に追加
    do {
        (*ioPos)++;
        if( tmp[*ioPos].key != iData->key ) {break;}
    } while( *ioPos < oTable->count );
    return true;
}

static int getRangeMin( com_sort_t *iData )
{
    return iData->key;
}

static int getRangeMax( com_sort_t *iData )
{
    return (iData->key + iData->range - 1);
}

// 範囲がかぶっているかチェックする
static BOOL occurCollision( com_sort_t *iData1, com_sort_t *iData2 )
{
    if( getRangeMax(iData1) < getRangeMin(iData2) ) {return false;}
    if( getRangeMax(iData2) < getRangeMin(iData1) ) {return false;}
    return true;
}

// 前後を含めてキーとその範囲がかぶってないかチェックする
static BOOL noRangeHit( com_sortTable_t *oTable, long iPos, com_sort_t *iData )
{
    if( oTable->count == 0 ) {return true;}
    com_sort_t*  tmp = oTable->table;
    if( iPos == oTable->count ) {
        if( occurCollision( &tmp[iPos-1], iData ) ) {return false;}
        return true;
    }
    if( occurCollision( &tmp[iPos], iData ) ) {return false;}
    if( iPos > 0 ) {
        if( occurCollision( &tmp[iPos-1], iData ) ) {return false;}
    }
    if( iPos + 1 < oTable->count ) {
        if( occurCollision( &tmp[iPos+1], iData ) ) {return false;}
    }
    return true;
}

static BOOL addSortTable(
        COM_FILEPRM, com_sortTable_t *oTable, com_sort_t *iData, long iPos )
{
    if( !resizeSortTable( COM_FILEVAR, oTable, 1 ) ) {return false;}
    resizeSortSearch( COM_FILEVAR, oTable );
    com_sort_t*  tmp = oTable->table;
    for( long i = oTable->count - 1;  i > iPos;  i-- ) {tmp[i] = tmp[i - 1];}
    return setSortData( COM_FILEVAR, &tmp[iPos], iData );
}

BOOL com_addSortTableFunc(
        com_sortTable_t *oTable, com_sort_t *iData, BOOL *oCollision,
        COM_FILEPRM )
{
    if( !oTable || !iData ) {COM_PRMNG(false);}

    com_sort_t  data = *iData;
    if( data.range < 1 ) {data.range = 1;}
    if( data.range > 1 ) {oTable->matchAction = COM_SORT_SKIP;}
    COM_SET_IF_EXIST( oCollision, false );

    BOOL  result = true;
    long  pos = 0;
    if( oTable->count ) {(void)binarySearchPos( oTable, data.key, &pos );}
    if( !noRangeHit( oTable, pos, &data ) ) {
        COM_SET_IF_EXIST( oCollision, true );
        if( !solveCollision( oTable, &pos, &data, &result ) ) {return result;}
    }
    com_skipMemInfo( true );
    result = addSortTable( COM_FILEVAR, oTable, &data, pos );
    com_skipMemInfo( false );
    return result;
}

BOOL com_addSortTableByKeyFunc(
        com_sortTable_t *oTable, long iKey, void *iData, size_t iSize,
        BOOL *oCollision, COM_FILEPRM )
{
    if( !oTable || !iData ) {COM_PRMNG(false);}

    com_skipMemInfo( true );
    com_sort_t  tmp = { iKey, 1, iData, iSize };
    BOOL  result =
        com_addSortTableFunc( oTable, &tmp, oCollision, COM_FILEVAR );
    com_skipMemInfo( false );
    return result;
}

static BOOL compareData( com_sort_t *iTarget, com_sort_t *iSource )
{
    if( !(iSource->data) ) {return false;}
    if( iTarget->size != iSource->size ) {return false;}
    if( memcmp(iTarget->data, iSource->data, iSource->size) ) {return false;}
    return true;
}

static long editSearch(
        com_sortTable_t *iTable, com_sort_t *iTarget, long iPos,
        com_sort_t ***oResult )
{
    static com_sort_t*  only1 = NULL;
    // 検索結果領域が確保できていないときは最初の1つだけを返す
    if( !(*oResult = iTable->search) ) {*oResult = &only1;}
    else {memset( *oResult, 0, sizeof(*(iTable->search)) * iTable->count );}
    long  result = 0;
    for( long i = iPos;  i < iTable->count;  i++ ) {
        com_sort_t*  tbl = &(iTable->table[i]);
        if( getRangeMax(iTarget) < getRangeMin(tbl) ) {break;}
        if( !occurCollision(tbl, iTarget) ) {continue;}
        if( iTarget->data ) {if( !compareData(tbl, iTarget) ) {continue;}}
        (*oResult)[result] = tbl;
        result++;
        if( only1 ) {break;}
    }
    return result;
}

long com_searchSortTable(
        com_sortTable_t *iTable, com_sort_t *iTarget, com_sort_t ***oResult )
{
    *oResult = NULL;
    if( !iTable || !iTarget || !oResult ) {COM_PRMNG(0);}
    if( iTarget->data && !(iTarget->size) ) {COM_PRMNG(0);}

    if( iTable->count == 0 ) {return 0;}
    com_sort_t  target = *iTarget;
    if( target.range < 1 ) {target.range = 1;}
    long  pos = 0;
    (void)binarySearchPos( iTable, target.key, &pos );
    if( noRangeHit( iTable, pos, &target ) ) {return 0;}
    if( pos > 0 ) {pos--;}  // 範囲検索のため1つ前からチェックする
    return editSearch( iTable, &target, pos, oResult );
}

long com_searchSortTableByKey(
        com_sortTable_t *iTable, long iKey, com_sort_t ***oResult )
{
    if( !iTable || !oResult ) {COM_PRMNG(0);}

    com_sort_t  tmp = { iKey, 1, NULL, 0 };
    return com_searchSortTable( iTable, &tmp, oResult );
}

static void deleteSortData(
        COM_FILEPRM, com_sort_t *ioTbl, long iPos, long *ioCount,
        long *oResult )
{
    freeSortData( COM_FILEVAR, &(ioTbl[iPos]) );
    for( long i = iPos;  i < *ioCount - 1;  i++ ) {ioTbl[i] = ioTbl[i+1];}
    (*ioCount)--;
    (*oResult)++;
}

static BOOL inRangeKey( com_sort_t *iTarget, com_sort_t *iRange )
{
    if( getRangeMin(iTarget) < getRangeMin(iRange) ) {return false;}
    if( getRangeMax(iTarget) > getRangeMax(iRange) ) {return false;}
    return true;
}

long com_deleteSortTableFunc(
        com_sortTable_t *oTable, com_sort_t *iTarget, BOOL iDeleteAll,
        COM_FILEPRM )
{
    if( !oTable || !iTarget ) {COM_PRMNG(0);}

    long  count = oTable->count;   // oTable->countは後の処理で再計算する
    if( !count ) {return 0;}
    long  pos = 0;
    com_skipMemInfo( true );
    long  result = 0;
    com_sort_t*  tbl = oTable->table;
    while( pos < count ) {
        if( !inRangeKey( &tbl[pos], iTarget ) ) {pos++;  continue;}
        if( iTarget->data ) {
            if( !compareData( &(tbl[pos]), iTarget ) ) {pos++;  continue;}
        }
        deleteSortData( COM_FILEVAR, tbl, pos, &count, &result );
        if( !iDeleteAll ) {break;}
    }
    if( result ) {
        if( resizeSortTable( COM_FILEVAR, oTable, -result ) ) {
            resizeSortSearch( COM_FILEVAR, oTable );
        }
    }
    com_skipMemInfo( false );
    return result;
}

long com_deleteSortTableByKeyFunc(
        com_sortTable_t *oTable, long iKey, COM_FILEPRM )
{
    if( !oTable ) {COM_PRMNG(0);}

    com_skipMemInfo( true );
    com_sort_t  tmp = { iKey, 1, NULL, 0 };
    long  result = com_deleteSortTableFunc( oTable, &tmp, true, COM_FILEVAR );
    com_skipMemInfo( false );
    return result;
}

static void freeSortList( COM_FILEPRM, long *ioCount, com_sort_t **oList )
{
    if( !ioCount || !oList ) {return;}
    for( int i = 0;  i < *ioCount;  i++ ) {
        freeSortData( COM_FILEVAR, &((*oList)[i]) );
    }
    com_freeFunc( oList, COM_FILEVAR );
    *ioCount = 0;
}

void com_freeSortTableFunc( com_sortTable_t *oTable, COM_FILEPRM )
{
    if( !oTable ) {COM_PRMNG();}

    com_skipMemInfo( true );
    freeSortList( COM_FILEVAR, &(oTable->count), &(oTable->table) );
    com_freeFunc( &(oTable->table), COM_FILEVAR );
    com_freeFunc( &(oTable->search), COM_FILEVAR );
    com_skipMemInfo( false );
}



/* リングバッファ関連 *******************************************************/

com_ringBuf_t *com_createRingBufFunc(
        size_t iUnit, size_t iSize, BOOL iOverwrite, BOOL iNotifyOverwrite,
        com_freeRingBuf_t iFunc, COM_FILEPRM )
{
    com_ringBuf_t*  ring =
        com_mallocFunc( sizeof(com_ringBuf_t), COM_FILEVAR,
                        "new ring buffer(%zu * %zu)", iUnit, iSize );
    if( !ring ) {return NULL;}
    void*  buf = com_mallocFunc( iUnit * iSize, COM_FILEVAR,
                        "new ring buffer area(%zu + %zu)", iUnit, iSize );
    if( !buf ) {
        com_freeFunc( ring, COM_FILEVAR );
        return NULL;
    }
    *ring = (com_ringBuf_t){
        buf, iUnit, iSize, 0, 0, 0, iOverwrite, iNotifyOverwrite, 0, iFunc
    };
    return ring;
}

#define RINGNEXT( RING, MEMBER ) \
    (RING)->MEMBER = (((RING)->MEMBER + 1) % (RING)->size )

#define RINGBUF( RING, POS ) \
    ((void*)((char*)((RING)->buf) + (POS) * (RING)->unit))

static void notifyOverwrite( com_ringBuf_t *ioRing )
{
    (ioRing->owCount)++;
    if( !ioRing->owNotify ) {return;}
    com_error( COM_ERR_RING, "ring buffer(%p) is overwritten", (void*)ioRing );
}

BOOL com_pushRingBuf( com_ringBuf_t *oRing, void *iData, size_t iDataSize )
{
    if( !oRing || !iData ) {COM_PRMNG(false);}
    if( iDataSize > oRing->unit ) {
        com_error( COM_ERR_RING, "ring buffer unit size over(%zu)", iDataSize );
        return false;
    }
    BOOL  occurOverwrite = false;
    if( oRing->used == oRing->size ) {
        if( !oRing->overwrite ) {
            com_error( COM_ERR_RING, "ring buffer (%p) full", (void*)oRing );
            return false;
        }
        occurOverwrite = true;
        notifyOverwrite( oRing );
    }
    void*  tmp = RINGBUF( oRing, oRing->tail );
    if( oRing->freeFunc ) {(oRing->freeFunc)( tmp );}
    memset( tmp, 0, oRing->unit );
    memcpy( tmp, iData, iDataSize );
    RINGNEXT( oRing, tail );
    if( !occurOverwrite ) {(oRing->used)++;}
    else {RINGNEXT( oRing, head );}
    return true;
}

void *com_pullRingBuf( com_ringBuf_t *oRing )
{
    if( !oRing ) {COM_PRMNG(NULL);}

    if( oRing->used == 0 ) {return NULL;}
    void*  tmp = RINGBUF( oRing, oRing->head );
    RINGNEXT( oRing, head );
    (oRing->used)--;
    return tmp;
}

size_t com_getRestRingBuf( com_ringBuf_t *iRing )
{
    if( !iRing ) {COM_PRMNG(0);}
    return iRing->used;
}

void com_freeRingBufFunc( com_ringBuf_t **oRing, COM_FILEPRM )
{
    if( !oRing ) {COM_PRMNG();}
    if( !(*oRing) ) {COM_PRMNG();}

    if( (*oRing)->freeFunc ) {
        for( size_t i = 0;  i < (*oRing)->size;  i++ ) {
            ((*oRing)->freeFunc)( RINGBUF( *oRing, i ) );
        }
    }
    com_freeFunc( &((*oRing)->buf), COM_FILEVAR );
    com_freeFunc( oRing, COM_FILEVAR );
}           



/* コンフィグ関連 ***********************************************************/

typedef struct {
    com_validator_t    func;           // チェック関数
    void*              cond;           // チェック条件
    com_valCondFree_t  freeFunc;       // チェック条件解放関数
} com_cfgVald_t;

typedef struct {
    char*              key;
    long               valdCnt;
    com_cfgVald_t*     vald;
    char*              data;
} com_cfgData_t;

static com_cfgData_t* gCfgData = NULL;
static long  gCfgDataCnt = 0;

static void freeCfgVald( com_cfgVald_t *ioVald )
{
    if( !(ioVald->cond) ) {return;}
    if( ioVald->freeFunc ) {(ioVald->freeFunc)( &(ioVald->cond) );}
    com_free( ioVald->cond );
}

static void freeCfgData( com_cfgData_t *oCfg )
{
    com_free( oCfg->key );
    for( long i = 0 ;  i< oCfg->valdCnt;  i++ ) {
        freeCfgVald( &(oCfg->vald[i]) );
    }
    com_free( oCfg->vald );
    oCfg->valdCnt = 0;
    com_free( oCfg->data );
}

static void freeAllCfgData( void )
{
    for( long i = 0;  i < gCfgDataCnt;  i++ ) {freeCfgData( &gCfgData[i] );}
    com_free( gCfgData );
    gCfgDataCnt = 0;
}

static com_cfgData_t *searchCfgData( char *iKey )
{
    for( long i = 0; i < gCfgDataCnt;  i++ ) {
        if( !strcmp( iKey, gCfgData[i].key ) ) {return &gCfgData[i];}
    }
    return NULL;
}

static com_cfgData_t *addNewCfgData( char *iKey, char *iData )
{
    com_cfgData_t*  tmp =
        com_reallocAddr( &gCfgData, sizeof(*gCfgData), COM_TABLEEND,
                         &gCfgDataCnt, 1, "addNewCfgData" );
    if( !tmp ) {return NULL;}
    if( (tmp->key = com_strdup( iKey, NULL ) ) ) {
        if( !iData ) {return tmp;}
        if( (tmp->data = com_strdup( iData, NULL )) ) {return tmp;}
    }
    freeCfgData( tmp );
    (void)com_realloct( &gCfgData, sizeof(*gCfgData), &gCfgDataCnt, -1,
                        "addNewCfgData NG" );
    return NULL;
}

BOOL com_registerCfg( char *iKey, char *iData )
{
    if( !iKey ) {COM_PRMNG(false);}
    if( searchCfgData( iKey ) ) {
        com_error( COM_ERR_CONFIG, "config already exist (%s)", iKey );
        return false;
    }
    COM_DEBUG_AVOID_START( COM_NO_FUNCNAME );
    void*  tmp = addNewCfgData( iKey, iData );
    COM_DEBUG_AVOID_END( COM_NO_FUNCNAME );
    if( !tmp ) {
        com_error( COM_ERR_CONFIG, "fail to add config (%s)", iKey );
        return false;
    }
    return true;
}

#define WRAP_CFG_VAL( FUNC, KEY, FORM, VAL ) \
    char  tmp[32] = {0}; \
    snprintf( tmp, sizeof(tmp), FORM, (VAL) ); \
    return (FUNC)( (KEY), tmp );

BOOL com_registerCfgDigit( char *iKey, long iData )
{
    WRAP_CFG_VAL( com_registerCfg, iKey, "%ld", iData );
}

BOOL com_registerCfgUDigit( char *iKey, ulong iData )
{
    WRAP_CFG_VAL( com_registerCfg, iKey, "%lu", iData );
}

#define GET_CONFIG_DATA( CFG, NGCAUSE ) \
    if( !iKey ) {COM_PRMNG(NGCAUSE);} \
    com_cfgData_t*  CFG = searchCfgData( iKey ); \
    if( !(CFG) ) { \
        com_error( COM_ERR_CONFIG, "config not exist (%s)", iKey ); \
        return (NGCAUSE); \
    }

static BOOL copyConditions( com_valCondCopy_t iCopy, void **oCond, void *iCond )
{
    BOOL  result = true;
    if( iCopy ) {result = (iCopy)( oCond, iCond );}
    if( result ) {COM_DEBUG_AVOID_END( COM_NO_FUNCNAME );}
    return result;
}

BOOL com_addCfgValidator(
        char *iKey, void *iCond, com_validator_t iFunc,
        com_valCondCopy_t iCopy, com_valCondFree_t iFree )
{
    GET_CONFIG_DATA( cfg, false );
    COM_DEBUG_AVOID_START( COM_NO_FUNCNAME );
    com_cfgVald_t*  vald =
        com_reallocAddr( &(cfg->vald), sizeof(*(cfg->vald)), COM_TABLEEND,
                         &(cfg->valdCnt), 1, "addCfgValidator" );
    if( !vald ) {
        COM_DEBUG_AVOID_END( COM_NO_FUNCNAME );
        return false;
    }
    vald->func = iFunc;
    vald->freeFunc = iFree;
    if( copyConditions( iCopy, &(vald->cond), iCond ) ) {return true;}
    com_error( COM_ERR_CONFIG, "fail to add condition (%s)", iKey );
    freeCfgVald( vald );
    (void)com_realloct( &(cfg->vald), sizeof(*(cfg->vald)),
                        &(cfg->valdCnt), -1, "addCfgValidator NG" );
    COM_DEBUG_AVOID_END( COM_NO_FUNCNAME );
    return false;
}

#define VAL_NUM( DATA, COND, TYPE, FUNC, BASE, CONDTYPE ) \
    if( !(DATA) ) {COM_PRMNG(false);} \
    errno = 0; \
    TYPE  value = (FUNC)( (DATA), (BASE), false ); \
    if( errno ) {return false;} \
    CONDTYPE*  cond = (COND); \
    if( cond ) {if(value < cond->min || value > cond->max) {return false;}} \
    return true;

#define COPY_COND( TYPE ) \
    if( !iCond ) {*oCond = NULL;  return true;} \
    TYPE*  source = iCond; \
    TYPE*  target = com_malloc( sizeof(TYPE), "copy simple condition" ); \
    if( !target ) {return false;} \
    *target = *source; \
    *oCond = target;

BOOL com_valDigit( char *ioData, void *iCond )
{
    VAL_NUM( ioData, iCond, long, com_strtol, 10, com_valCondDigit_t );
}
BOOL com_valDigitCondCopy( void **oCond, void *iCond )
{
    COPY_COND( com_valCondDigit_t );
    return true;
}

BOOL com_valUDigit( char *ioData, void *iCond )
{
    VAL_NUM( ioData, iCond, ulong, com_strtoul, 10, com_valCondUDigit_t );
}
BOOL com_valUDigitCondCopy( void **oCond, void *iCond )
{
    COPY_COND( com_valCondUDigit_t );
    return true;
}

BOOL com_valHex( char *ioData, void *iCond )
{
    VAL_NUM( ioData, iCond, ulong, com_strtoul, 16, com_valCondUDigit_t );
}

#define VAL_NUM_LIST( DATA, COND, TYPE, FUNC, BASE, CONDTYPE ) \
    if( !(DATA) ) {COM_PRMNG(false);} \
    errno = 0; \
    TYPE  value = (FUNC)( (DATA), (BASE), false ); \
    if( errno ) {return false;} \
    if( !(COND) ) {return true;} \
    CONDTYPE* cond = (COND); \
    for( long i = 0;  i < cond->count;  i++ ) { \
        if( value == cond->list[i] ) {return true;} \
    } \
    return false;

#define COPY_VAL_LIST( TYPE ) \
    if( !target->count ) { \
        com_error( COM_ERR_CONFIG, "no condition num list" ); \
        return false; \
    } \
    target->list = com_malloc( sizeof(TYPE) * target->count, \
                               "copy condition num list" ); \
    if( !target->list ) {return false;} \
    for( long i = 0;  i < target->count;  i++ ) { \
        target->list[i] = source->list[i]; \
    }

#define FREE_VAL_LIST( TYPE, FREE_ELM ) \
    TYPE*  target = *oCond; \
    if( target ) { \
        if( FREE_ELM ) { \
            for( long i = 0;  target->list[i];  i++ ) { \
                com_free( target->list[i] ); \
            } \
        } \
        com_free( target->list ); \
    }

BOOL com_valDgtList( char *ioData, void *iCond )
{
    VAL_NUM_LIST( ioData, iCond, long, com_strtol, 10, com_valCondDgtList_t );
}
BOOL com_valDgtListCondCopy( void **oCond, void *iCond )
{
    COPY_COND( com_valCondDgtList_t );
    COPY_VAL_LIST( long );
    return true;
}
void com_valDgtListCondFree( void **oCond )
{
    FREE_VAL_LIST( com_valCondDgtList_t, false );
}

BOOL com_valUDgtList( char *ioData, void *iCond )
{
    VAL_NUM_LIST( ioData, iCond, ulong, com_strtoul,10, com_valCondUDgtList_t );
}
BOOL com_valUDgtListCondCopy( void **oCond, void *iCond )
{
    COPY_COND( com_valCondUDgtList_t );
    COPY_VAL_LIST( ulong );
    return true;
}
void com_valUDgtListCondFree( void **oCond )
{
    FREE_VAL_LIST( com_valCondUDgtList_t, false );
}

BOOL com_valHexList( char *ioData, void *iCond )
{
    VAL_NUM_LIST( ioData, iCond, ulong, com_strtoul,16, com_valCondUDgtList_t );
}

BOOL com_valString( char *ioData, void *iCond )
{
    if( !ioData ) {COM_PRMNG(false);}
    if( !iCond ) {return true;}
    com_valCondString_t* cond = iCond;
    if( cond->minLen) {if( strlen(ioData) < cond->minLen ) {return false;}}
    if( cond->maxLen) {if( strlen(ioData) > cond->maxLen ) {return false;}}
    return true;
}
BOOL com_valStringCondCopy( void **oCond, void *iCond )
{
    COPY_COND( com_valCondString_t );
    return true;
}

static BOOL checkList( const char **iList, char *ioData, BOOL iNoCase )
{
    if( COM_NO_DATA == com_searchStrList( iList, ioData, iNoCase ) ) {
        return false;
    }
    return true;
}

static long getListCount( char **iList )
{
    long cnt = 0;
    if( iList ) {for( ; iList[cnt]; cnt++ );  cnt++;} // 文字列終端分 +1
    return cnt;
}

BOOL com_valStrList( char *ioData, void *iCond )
{
    if( !ioData ) {COM_PRMNG(false);}
    if( !iCond ) {return true;}
    com_valCondStrList_t*  cond = iCond;
    return checkList( (const char **)cond->list, ioData, cond->noCase );
}
BOOL com_valStrListCondCopy( void **oCond, void *iCond )
{
    COPY_COND( com_valCondStrList_t );
    long  cnt = getListCount( source->list );
    if( !cnt ) {
        com_error( COM_ERR_CONFIG, "no condition string list" );
        return false;
    }
    target->list = com_malloc( sizeof(char*) * cnt, "copy string list" );
    if( !target->list ) {return false;}
    for( long i = 0;  source->list[i];  i++ ) {
        if( !(target->list[i] = com_strdup( source->list[i], NULL )) ) {
            return false;
        }
    }
    return true;
}
void com_valStrListCondFree( void **oCond )
{
    FREE_VAL_LIST( com_valCondStrList_t, true );
}

#define VAL_STRLIST \
    COM_UNUSED( iCond ); \
    if( !ioData ) {COM_PRMNG(false);} \
    return checkList( list, ioData, true );

BOOL com_valBool( char *ioData, void *iCond )
{
    const char*  list[] = { "true", "false", NULL };
    VAL_STRLIST;
}

BOOL com_valYesNo( char *ioData, void *iCond )
{
    const char*  list[] = { "yes", "y", "no", "n", NULL };
    VAL_STRLIST;
}

BOOL com_valOnOff( char *ioData, void *iCond )
{
    const char*  list[] = { "on", "off", NULL };
    VAL_STRLIST;
}

static BOOL checkCfgData( com_cfgData_t *iCfg, char *iData )
{
    for( long i = 0;  i < iCfg->valdCnt;  i++ ) {
        com_cfgVald_t* vald = &(iCfg->vald[i]);
        if( vald ) {
            if( !(vald->func)( iData, vald->cond ) ) {
                com_error( COM_ERR_CONFIG, "data is invalid (%s)", iData );
                return (i + 1);
            }
        }
    }
    return 0;
}

long com_setCfg( char *iKey, char *iData )
{
    if( !iData ) {COM_PRMNG(-1);}  // iKeyは GET_CONFIG_DATAでチェック
    GET_CONFIG_DATA( cfg, -1 );
    long  checkResult = checkCfgData( cfg, iData );
    if( checkResult ) {return checkResult;}
    COM_DEBUG_AVOID_START( COM_NO_FUNCNAME );
    char*  tmp = com_strdup( iData, "setCdf (%s)", iData );
    if( tmp ) {
        com_free( cfg->data );
        cfg->data = tmp;
    }
    else {checkResult = -1;}
    COM_DEBUG_AVOID_END( COM_NO_FUNCNAME );
    return checkResult;
}

long com_setCfgDigit( char *iKey, long iData )
{
    WRAP_CFG_VAL( com_setCfg, iKey, "%ld", iData );
}

long com_setCfgUDigit( char *iKey, ulong iData )
{
    WRAP_CFG_VAL( com_setCfg, iKey, "%lu", iData );
}

BOOL com_isEmptyCfg( char *iKey )
{
    GET_CONFIG_DATA( cfg, true );
    if( cfg->data ) {return false;}
    return true;
}

const char *com_getCfg( char *iKey )
{
    GET_CONFIG_DATA( cfg, "" );
    if( !cfg->data ) {return "";}
    return cfg->data;
}

#define GET_NUM( TYPE, FUNC, BASE ) \
    GET_CONFIG_DATA( cfg, 0 ); \
    if( !cfg->data ) {return 0;} \
    TYPE  value = (FUNC)( cfg->data, (BASE), false ); \
    if( errno ) {return 0;} \
    return value;

long com_getCfgDigit( char *iKey )
{
    GET_NUM( long, com_strtol, 10 );
}

ulong com_getCfgUDigit( char *iKey )
{
    GET_NUM( ulong, com_strtoul, 10 );
}

ulong com_getCfgHex( char *iKey )
{
    GET_NUM( ulong, com_strtoul, 16 );
}

#define CFGIS( STR )  com_compareString( STR, cfg->data, 0, true )

BOOL com_getCfgBool( char *iKey )
{
    GET_CONFIG_DATA( cfg, false );
    if( !cfg->data ) {return false;}
    if( CFGIS("TRUE") || CFGIS("YES") || CFGIS("Y") || CFGIS("ON") ) {
        return true;
    }
    return false;
}

BOOL com_getCfgAll( long *ioCount, const char **oKey, const char **oData )
{
    if( !ioCount || !oKey || !oData ) {COM_PRMNG(false);}
    if( *ioCount < 0 || *ioCount >= gCfgDataCnt ) {return false;}
    com_cfgData_t*  tmp = &(gCfgData[*ioCount]);
    *oKey  = tmp->key;
    *oData = tmp->data;
    (*ioCount)++;
    return true;
}



/* ファイル操作関連 *********************************************************/

static pthread_mutex_t gMutexFile = PTHREAD_MUTEX_INITIALIZER;

FILE *com_fopenFunc( const char *iPath, const char *iMode, COM_FILEPRM )
{
    if( !iPath || !iMode ) {COM_PRMNGF("com_fopen",NULL);}

    com_mutexLock( &gMutexFile, __func__ );
    FILE*  fp = NULL;
    if( !com_debugFopenError() ) {fp = fopen( iPath, iMode );}
    if( fp ) {com_addFileInfo( COM_FILEVAR, COM_FOPEN, fp, iPath );}
    else {com_error( COM_ERR_FILEDIRNG, "com_fopen NG (%s)", iPath );}
    com_mutexUnlock( &gMutexFile, __func__ );
    return fp;
}

int com_fcloseFunc( FILE **ioFp, COM_FILEPRM )
{
    if( !ioFp ) {COM_PRMNGF("com_fclose", -1);}

    if( !(*ioFp) ) {return 0;}
    com_mutexLock( &gMutexFile, __func__ );
    int  fcloseErrno = 0;
    if( com_debugFcloseError( *ioFp ) ) {fcloseErrno = EINVAL;}
    com_deleteFileInfo( COM_FILEVAR, COM_FCLOSE, *ioFp );
    if( fclose( *ioFp ) ) {fcloseErrno = errno;}
    if( fcloseErrno ) {
        com_error( COM_ERR_FILEDIRNG, "com_fclose NG (%p:%s)",
                   (void*)ioFp, com_strerror( fcloseErrno ) );
    }
    *ioFp = NULL;
    com_mutexUnlock( &gMutexFile, __func__ );
    return fcloseErrno;
}

BOOL com_checkExistFile( const char *iPath )
{
    if( !iPath ) {COM_PRMNG(false);}

    struct  stat status;
    int  result = stat( iPath, &status );
    if( result ) {return false;}
    return true;
}

BOOL com_checkExistFiles( const char *iSource, const char *iDelim )
{
    if( !iSource || !iDelim ) {COM_PRMNG(false);}

    com_skipMemInfo( true );
    char*  src = com_strdup( iSource, "checkExist(%s)", iSource );
    if( !src ) {com_skipMemInfo( false );  return false;}

    BOOL  result = true;
    char*  saveptr;
    for( char* tmp = strtok_r( src,  iDelim, &saveptr );
         tmp;  tmp = strtok_r( NULL, iDelim, &saveptr ) )
    {
        if( !com_checkExistFile( tmp ) ) {result = false;  break;}
    }
    com_free( src );
    com_skipMemInfo( false );
    return result;
}

enum { DMY = 0 };

BOOL com_getFileInfo( com_fileInfo_t *oInfo, const char *iPath, BOOL iLink )
{
    if( !iPath ) {COM_PRMNG(false);}

    struct stat  st;
    memset( &st, 0, sizeof(st) );
    int  result = 0;
    if( iLink ) {result = lstat( iPath, &st );}
    else {result = stat( iPath, &st );}
    if( result ) {return false;}
    if( oInfo ) {
        *oInfo = (com_fileInfo_t){
            st.st_dev, major(st.st_dev), minor(st.st_dev),
#ifndef LINUXOS
            DMY,
#endif
            st.st_ino, st.st_mode, DMY,
            S_ISREG(st.st_mode), S_ISDIR(st.st_mode), S_ISLNK(st.st_mode),
            st.st_uid, st.st_gid, st.st_size,
            st.st_atime, st.st_mtime, st.st_ctime
        };
    }
    return true;
}

static char *getLastMark( const char *iPath, const char *iMark )
{
    long  srchIdx = COM_SEARCH_LAST;
    return com_searchString( iPath, iMark, &srchIdx, 0, true );
}

BOOL com_getFileName( char *oBuf, size_t iBufSize, const char *iPath )
{
    if( !oBuf || !iBufSize || !iPath ) {COM_PRMNG(false);}

    memset( oBuf, 0, iBufSize );
    char*  lastSlash = getLastMark( iPath, "/" );
    if( lastSlash ) {iPath = lastSlash + 1;}
    return com_strncpy( oBuf, iBufSize, iPath, strlen(iPath) );
}

BOOL com_getFilePath( char *oBuf, size_t iBufSize, const char *iPath )
{
    if( !oBuf || !iBufSize || !iPath ) {COM_PRMNG(false);}

    memset( oBuf, 0, iBufSize );
    char*  lastSlash = getLastMark( iPath, "/" );
    if( !lastSlash ) {return false;}
    return com_strncpy(oBuf, iBufSize, iPath, (size_t)(lastSlash - iPath + 1));
}

BOOL com_getFileExt( char *oBuf, size_t iBufSize, const char *iPath )
{
    if( !oBuf || !iBufSize || !iPath ) {COM_PRMNG(false);}

    memset( oBuf, 0, iBufSize );
    char*  lastSlash = getLastMark( iPath, "/" );
    const char*  filename = lastSlash ? lastSlash + 1 : iPath;
    char*  lastPeriod = getLastMark( filename, "." );
    if( lastPeriod ) {
        lastPeriod++;
        if( *lastPeriod ) {
            return com_strncpy( oBuf,iBufSize,lastPeriod,strlen(lastPeriod) );
        }
    }
    return false;
}

static BOOL notifyTextLine(
        char *oBuf, size_t iBufSize, FILE *ioFp, com_seekFileCB_t iFunc,
        void *ioUserData )
{
    while( fgets( oBuf, iBufSize, ioFp ) ) {
        com_seekFileResult_t  inf = { oBuf, ioFp, ioUserData };
        if( !iFunc( &inf ) ) {return false;}
    }
    return true;
}

static char gSeekFileBuff[COM_LINEBUF_SIZE];

BOOL com_seekFile(
        const char *iPath, com_seekFileCB_t iFunc, void *ioUserData,
        char *oBuf, size_t iBufSize )
{
    if( !iPath || !iFunc ) {COM_PRMNG(false);}
    if( !oBuf ) {oBuf = gSeekFileBuff;  iBufSize = sizeof(gSeekFileBuff);}
    if( !com_checkExistFile( iPath ) ) {
        com_error( COM_ERR_FILEDIRNG, "%s not found", iPath );
        return false;
    }
    COM_FOPEN_MUTE( fp, iPath, "r" );
    if( !fp ) {return false;}
    BOOL  result = notifyTextLine( oBuf, iBufSize, fp, iFunc, ioUserData );
    COM_FCLOSE_MUTE( fp );
    return result;
}

static size_t notifyBin(
        com_seekBinCB_t iFunc, uchar *iBuf, size_t iBufSize, size_t *ioOffset,
        FILE *iFp, void *ioUserData )
{
    com_seekBinResult_t  inf = { iBuf, *ioOffset, iFp, ioUserData };
    *ioOffset = 0;
    size_t  nextSize = iFunc( &inf );
    if( nextSize > iBufSize ) {nextSize = iBufSize;}
    return nextSize;
}

static BOOL seekBin(
        uchar *oBuf, size_t iBufSize, FILE *ioFp, size_t iNextSize,
        com_seekBinCB_t iFunc, void *ioUserData )
{
    size_t  nextSize = iNextSize;
    size_t  offset = 0;
    int  oct;
    while( EOF != (oct = fgetc( ioFp )) ) {
        oBuf[offset++] = (uchar)oct;
        if( offset < nextSize ) {continue;}
        nextSize = notifyBin(iFunc, oBuf, iBufSize, &offset, ioFp, ioUserData);
        if( !nextSize ) {return false;}
    }
    // バッファに残っているものがあったら、それを最後に通知
    if( offset ) {
        if( !notifyBin(iFunc, oBuf, iBufSize, &offset, ioFp, ioUserData) ) {
            return false;
        }
    }
    return true;
}

static uchar  gSeekBinBuf[16];

BOOL com_seekBinary(
        const char *iPath, size_t iNextSize, com_seekBinCB_t iFunc,
        void *ioUserData, uchar *oBuf, size_t iBufSize )
{
    if( !iPath || !iNextSize || !iFunc ) {COM_PRMNG(false);}
    if( !oBuf ) {oBuf = gSeekBinBuf;  iBufSize = sizeof(gSeekBinBuf);}
    if( iNextSize > iBufSize ) {
        com_error( COM_ERR_FILEDIRNG, "next size is larger than buffer size" );
        return false;
    }
    if( !com_checkExistFile( iPath ) ) {
        com_error( COM_ERR_FILEDIRNG, "%s not found", iPath );
        return false;
    }
    COM_FOPEN_MUTE( fp, iPath, "rb" );
    if( !fp ) {return false;}
    BOOL  result = seekBin( oBuf, iBufSize, fp, iNextSize, iFunc, ioUserData );
    COM_FCLOSE_MUTE( fp );
    return result;
}

static char gPipeCmdBuff[COM_LINEBUF_SIZE];

BOOL com_pipeCommand(
        const char *iCommand, com_seekFileCB_t iFunc, void *ioUserData,
        char *oBuf, size_t iBufSize )
{
    if( !iCommand || !iFunc ) {COM_PRMNG(false);}
    if( !oBuf ) {oBuf = gPipeCmdBuff;  iBufSize = sizeof(gPipeCmdBuff);}
    FILE*  fp = popen( iCommand, "r" );
    if( !fp ) {
        com_error( COM_ERR_FILEDIRNG, "fail to execute [%s]", iCommand );
        return false;
    }
    BOOL  result = notifyTextLine( oBuf, iBufSize, fp, iFunc, ioUserData );
    pclose( fp );
    return result;
}

static BOOL notifyLine(
        com_seekFileCB_t iFunc, void *ioUserData, size_t *oPtr,
        char *oBuf, size_t iBufSize, BOOL iNeedLf )
{
    if( oBuf[*oPtr - 1] == '\n' && !iNeedLf ) {oBuf[*oPtr - 1] = '\0';}
    else {oBuf[*oPtr] = '\0';}
    com_seekFileResult_t  inf = { oBuf, NULL, ioUserData };
    BOOL  result = iFunc( &inf );
    memset( oBuf, 0, iBufSize );
    *oPtr = 0;
    return result;
}

static BOOL seekTextLine(
        const char *iText, size_t iTextSize, char *oBuf, size_t iBufSize,
        BOOL iNeedLf, com_seekFileCB_t iFunc, void *ioUserData )
{
    memset( oBuf, 0, iBufSize );
    size_t  bufPtr = 0;
    for( const char* ptr = iText;  (size_t)(ptr - iText) < iTextSize;  ptr++ ) {
        oBuf[bufPtr++] = *ptr;
        if( *ptr == '\n' || bufPtr == iBufSize ) {
            if( !notifyLine(iFunc,ioUserData,&bufPtr,oBuf,iBufSize,iNeedLf) ) {
                return false;
            }
        }
    }
    // 最後にバッファに残ったものがあれば通知する
    if( bufPtr ) {
        return notifyLine(iFunc,ioUserData,&bufPtr,oBuf,iBufSize,iNeedLf);
    }
    return true;
}

static char gLineCmdBuff[COM_LINEBUF_SIZE];

BOOL com_seekTextLine(
        const void *iText, size_t iTextSize, BOOL iNeedLf,
        com_seekFileCB_t iFunc, void *ioUserData, char *oBuf, size_t iBufSize )
{
    if( !iText || !iFunc ) {COM_PRMNG(false);}
    if( !oBuf ) {oBuf = gLineCmdBuff;  iBufSize = sizeof(gLineCmdBuff);}
    const char*  ptr = iText;
    if( !iTextSize ) {iTextSize = strlen( ptr );}
    return seekTextLine( ptr, iTextSize, oBuf, iBufSize, iNeedLf,
                         iFunc, ioUserData );
}



/* ディレクトリ操作関連 *****************************************************/

static pthread_mutex_t  gMutexDir = PTHREAD_MUTEX_INITIALIZER;
static char  gPathBuf1[COM_TEXTBUF_SIZE];
static char  gPathBuf2[COM_TEXTBUF_SIZE];

BOOL com_checkIsDir( const char *iPath )
{
    if( !iPath ) {COM_PRMNG(false);}
    DIR*  check_dir = opendir( iPath );
    if( !check_dir ) {return false;}
    closedir( check_dir );
    return true;
}

static BOOL needMakeDir( const char *iPath )
{
    if( !com_checkExistFile( iPath ) ) {return true;} // 同名なし
    if( !com_checkIsDir( iPath ) ) {
        // 同名ファイルがある場合はエラー出力することとする
        com_error( COM_ERR_FILEDIRNG,
                   "same name file already exists (mkdir %s)", iPath );
    }
    // 同名ディレクトリがある場合、false(新規生成不要)は返すがエラーではない
    return false;
}

static BOOL unlockDir( BOOL iResult )
{
    com_mutexUnlock( &gMutexDir, __func__ );
    return iResult;
}

BOOL com_makeDir( const char *iPath )
{
    if( !iPath ) {COM_PRMNG(false);}

    // 既に指定ディレクトリがある場合は何もしない
    if( com_checkIsDir( iPath ) ) {return true;}
    com_mutexLock( &gMutexDir, __func__ );
    (void)com_strcpy( gPathBuf1, iPath );
    COM_CLEAR_BUF( gPathBuf2 );
    if( gPathBuf1[0] == '/' ) {(void)com_strcpy( gPathBuf2, "/" );}
    char* saveptr;
    // ディレクトリ階層ごとに有無をチェックして無ければ作成する
    for( char* tmp = strtok_r( gPathBuf1, "/", &saveptr );
         tmp;  tmp = strtok_r( NULL,      "/", &saveptr ) )
    {
        (void)com_connectString( gPathBuf2, sizeof(gPathBuf2), "%s/", tmp );
        if( needMakeDir( gPathBuf2 ) ) {
            if( mkdir( gPathBuf2, 0777 ) ) {
                com_error( COM_ERR_FILEDIRNG, "mkdir NG (%s)", gPathBuf2 );
                return unlockDir( false );
            }
        }
        if( !com_checkIsDir( gPathBuf2 ) ) {return unlockDir( false );}
    }
    return unlockDir( true );
}

static BOOL deleteFiles( const com_seekDirResult_t *iInf )
{
    if( 0 > remove( iInf->path ) ) {
        com_error( COM_ERR_FILEDIRNG, "remove NG (%s)", iInf->path );
        return false;
    }
    long*  deleteCount = iInf->userData;
    (*deleteCount)++;
    return true;
}

static BOOL deleteSubDirs( const com_seekDirResult_t *iInf )
{
    char*  path = com_strdup( iInf->path, "remove sub(%s)", iInf->path );
    if( !path ) {return false;}
    int  result = com_removeDir( path );
    com_free( path );
    if( result < 0 ) {return false;}

    long*  deleteCount = iInf->userData;
    *deleteCount += result;
    return true;
}

long com_removeDir( const char *iPath )
{
    if( !iPath ) {COM_PRMNG(COM_RMDIR_PRMNG);}

    if( !com_checkExistFile( iPath ) ) {return 0;}
    BOOL  result = false;
    long  deleteCount = 0;
    com_skipMemInfo( true );
    result = com_seekDir( iPath, NULL, COM_SEEK_FILE, false,
                          deleteFiles, &deleteCount );
    if( !result ) {com_skipMemInfo( false );  return COM_RMDIR_SEEKFILE_NG;}
    result = com_seekDir( iPath, NULL, COM_SEEK_DIR, false,
                          deleteSubDirs, &deleteCount );
    if( !result ) {com_skipMemInfo( false );  return COM_RMDIR_RMDIR_NG;}
    if ( 0 > remove( iPath ) ) {
        com_error( COM_ERR_FILEDIRNG, "rmdir NG (%s)", iPath );
        return COM_RMDIR_RMDIR_NG;
    }
    deleteCount++;
    return deleteCount;
}

// ディレクトリ走査情報
typedef struct {
    char*             path;       // 走査対象パス
    com_seekFilter_t  filter;     // 走査フィルター関数
    long              type;       // 走査種別(COM_SEEK_TYPE_t)
    com_seekDirCB_t   func;       // コールバック関数
    void*             userData;   // コールバック関数に渡すユーザーデータ
    struct dirent**   nameList;   // scandir()結果
    long              nameCount;  // scandir()結果
    com_strChain_t*   child;      // サブディレクトリ名リスト
} com_dirInf_t;

static int isSkipEntry( const struct dirent *iEntry )
{
    const char*  name = iEntry->d_name;
    if( !strcmp( name, "." ) || !strcmp( name, ".." ) ) {return 0;}
    return 1;
}

static int scanDir(
        const char *iPath, struct dirent ***oList, com_seekFilter_t iFilter )
{
    if( !iFilter ) {iFilter = isSkipEntry;}
    int  count = scandir( iPath, oList, iFilter, alphasort );
    if( 0 > count ) {
        com_error( COM_ERR_FILEDIRNG, "scandir() NG [%s]", iPath);
    }
    return count;
}

static BOOL readyDir(
        com_dirInf_t *oInf, const char *iPath, com_seekFilter_t iFilter,
        COM_SEEK_TYPE_t iType, com_seekDirCB_t iFunc, void *ioUserData )
{
    *oInf = (com_dirInf_t){
        NULL, iFilter, iType, iFunc, ioUserData, NULL, 0, NULL
    };
    if( !(oInf->path = com_strdup( iPath, "readyDir(%s)", iPath )) ) {
        return false;
    }
    for( char* pathTail = oInf->path + strlen(oInf->path) - 1;
         pathTail > oInf->path && *pathTail == '/';  pathTail-- )
    {
        *pathTail = '\0';  // 末尾の / を削除する
    }
    oInf->nameCount = (long)scanDir( iPath, &(oInf->nameList), oInf->filter );
    if( oInf->nameCount < 0 ) {return false;}
    return true;
}

static void finishDir( com_dirInf_t *oInf )
{
    com_free( oInf->path );
    for( long n = 0; n < oInf->nameCount; n++ ) {free( oInf->nameList[n] );}
    free( oInf->nameList );
    com_freeChainData( &(oInf->child) );
}

static pthread_mutex_t  gMutexPath = PTHREAD_MUTEX_INITIALIZER;
static char  gPathTmp[COM_TEXTBUF_SIZE];

static void makePathTmp( const char *iPath, const char *iName )
{
    snprintf( gPathTmp, sizeof(gPathTmp), "%s/%s", iPath, iName );
}

static BOOL matchEntryType( BOOL iIsDir, COM_SEEK_TYPE_t iType )
{
    if( iIsDir ) {if( !(iType & COM_SEEK_DIR) ) {return false;}}
    else {if( !(iType & COM_SEEK_FILE) ) {return false;}}
    return true;
}

static BOOL procEntry(
        com_seekDirResult_t *oResult, com_dirInf_t *iInf,
        struct dirent *iEntry )
{
    makePathTmp( iInf->path, iEntry->d_name );
    oResult->isDir = com_checkIsDir( gPathTmp );
    if( !matchEntryType( oResult->isDir, iInf->type ) ) {return true;}
    *oResult = (com_seekDirResult_t){
        gPathTmp, strlen(iInf->path), iEntry, oResult->isDir, iInf->userData
    };
    return (iInf->func)( oResult );
}

static BOOL checkEntryList( com_dirInf_t *ioInf )
{
    for( long i = 0;  i < ioInf->nameCount;  i++ ) {
        char*  dname = ioInf->nameList[i]->d_name;
        com_seekDirResult_t result;
        BOOL  isContinue = procEntry( &result, ioInf, ioInf->nameList[i] );
        if( result.isDir ) {
            // サブディレクトリはその名前をリスト保持し、後で使用する
            if( !com_addChainData( &(ioInf->child), dname ) ) {return false;}
        }
        if( !isContinue ) {return false;}
    }
    return true;
}

static BOOL checkChildDir( com_dirInf_t *ioInf )
{
    for( com_strChain_t* child = ioInf->child;  child;  child = child->next ) {
        makePathTmp( ioInf->path, child->data );
        BOOL  result = com_seekDir( gPathTmp, ioInf->filter, ioInf->type, true,
                                    ioInf->func, ioInf->userData );
        if( !result ) {return false;}
    }
    return true;
}

static BOOL seekDir(
        com_dirInf_t *oInf, const char *iPath, com_seekFilter_t iFilter,
        COM_SEEK_TYPE_t iType, BOOL iCheckChild, com_seekDirCB_t iFunc,
        void *ioUserData )
{
    if( !readyDir( oInf,iPath,iFilter,iType,iFunc,ioUserData ) ) {return false;}
    if( !checkEntryList( oInf ) ) {return false;}
    if( iCheckChild ) {if( !checkChildDir( oInf ) ) {return false;}}
    return true;
}

BOOL com_seekDir(
        const char *iPath, com_seekFilter_t iFilter, COM_SEEK_TYPE_t iType,
        BOOL iCheckChild, com_seekDirCB_t iFunc, void *ioUserData )
{
    static __thread long  retryCount = 0;
    if( !iPath || !(iType & COM_SEEK_BOTH) || !iFunc ) {COM_PRMNG(false);}

    if( !retryCount++ ) {com_mutexLockCom( &gMutexPath, COM_FILELOC );}
    com_dirInf_t  inf;
    BOOL  result = seekDir( &inf, iPath, iFilter, iType, iCheckChild, iFunc,
                            ioUserData );
    finishDir( &inf );
    if( --retryCount ) {return result;}
    return com_mutexUnlockCom( &gMutexPath, COM_FILELOC, result );
}

int com_scanDirFunc(
        const char *iPath, com_seekFilter_t iFilter, struct dirent ***oList,
        COM_FILEPRM )
{
    if( !iPath || !oList ) {COM_PRMNG(-1);}

    com_mutexLockCom( &gMutexMem, COM_FILELOC );
    int  result = scanDir( iPath, oList, iFilter );
    if( 0 < result ) {
        // d_name等の使用量は含まれないので、メモリサイズは参考情報程度で
        for( int n = 0;  n < result;  n++ ) {
            com_addMemInfo( COM_FILEVAR, COM_SCANDIR, (*oList)[n],
                            sizeof(*((*oList)[n])),
                            "scandir(dirent %d:%s)", n, (*oList)[n]->d_name );
        }
        com_addMemInfo( COM_FILEVAR, COM_SCANDIR, *oList,
                        sizeof(struct dirent*) * (size_t)result,
                        "scandir(%s)", iPath );

    }
    return com_mutexUnlockCom( &gMutexMem, COM_FILELOC, result );
}
    
void com_freeScanDirFunc( int iCount, struct dirent ***oList, COM_FILEPRM )
{
    if( !oList ) {COM_PRMNG();}
    if( !(*oList) ) {return;}
    com_skipMemInfo( true );
    for( int n = 0;  n < iCount;  n++ ) {
        com_freeFunc( &((*oList)[n]), COM_FILEVAR );
    }
    com_freeFunc( oList, COM_FILEVAR );
    com_skipMemInfo( false );
}

// com_countFiles()の集計データ用
typedef struct {
    long*   fileCount;
    long*   dirCount;
    off_t*  totalSize;
} com_countFile_t;

static void initCountInf( com_countFile_t *oData )
{
    COM_SET_IF_EXIST( oData->fileCount, 0 );
    COM_SET_IF_EXIST( oData->dirCount,  0 );
    COM_SET_IF_EXIST( oData->totalSize, 0 );
}

static void incrementData( long *oTarget )
{
    COM_SET_IF_EXIST( oTarget, *oTarget + 1 );
}

static BOOL addSize( const char *iPath, off_t *oSize )
{
    if( !oSize ) {return true;}
    com_fileInfo_t  info;
    if( !com_getFileInfo( &info, iPath, true ) ) {
        com_error( COM_ERR_FILEDIRNG, "stat NG (%s)", iPath );
        return false;
    }
    *oSize += info.size;
    return true;
}

static BOOL countFiles( const com_seekDirResult_t *iInf )
{
    com_countFile_t*  data = iInf->userData;
    if( !(iInf->isDir) ) {incrementData( data->fileCount );}
    else {incrementData( data->dirCount );}

    (void)addSize( iInf->path, data->totalSize );  // NGになっても処理続行
    return true;
}

BOOL com_countFiles(
        long *oFileCount, long *oDirCount, off_t *oTotalSize,
        const char *iPath, BOOL iCheckChild )
{
    if( !iPath ) {COM_PRMNG(false);}

    com_countFile_t  data = { oFileCount, oDirCount, oTotalSize };
    initCountInf( &data );
    if( !addSize( iPath, oTotalSize ) ) {return false;}  // 自身のサイズ加算
    com_skipMemInfo( true );
    BOOL  result = com_seekDir( iPath, NULL, COM_SEEK_BOTH, iCheckChild,
                                countFiles, &data );
    com_skipMemInfo( false );
    return result;
}

static BOOL checkFilter( const struct dirent *iEntry, com_seekFilter_t iFilter )
{
    if( iFilter ) {return (iFilter)( iEntry );}
    return isSkipEntry( iEntry );
}

static int seekDir2( DIR *ioDir, com_dirInf_t *iInf, com_seekFilter_t iFilter )
{
    int  readResult = 0;
    struct dirent*  dEntry;
    while( (dEntry = readdir( ioDir )) ) {
        if( !checkFilter( dEntry, iFilter ) ) {continue;}
        com_seekDirResult_t  result;
        BOOL  isContinue = procEntry( &result, iInf, dEntry );
        if( !isContinue ) {readResult = -1;  break;}
    }
    return readResult;
}

BOOL com_seekDir2(
        const char *iPath, com_seekFilter_t iFilter, COM_SEEK_TYPE_t iType,
        com_seekDirCB_t iFunc, void *ioUserData )
{
    if( !iPath || !(iType & COM_SEEK_BOTH) || !iFunc ) {COM_PRMNG(false);}

    DIR*  dir = opendir( iPath );
    if( !dir ) {
        com_error( COM_ERR_FILEDIRNG, "fail to opendir(%s)", iPath );
        return false;
    }
    // iPathの const外しになるが内容変更はしない
    com_dirInf_t  inf = {
        (char*)iPath, NULL, iType, iFunc, ioUserData, NULL, 0, NULL
    };
    int  readResult = seekDir2( dir, &inf, iFilter );
    closedir( dir );
    if( readResult > 0 ) {
        com_error( COM_ERR_FILEDIRNG, "fail to readdir(%s)", iPath );
    }
    return !readResult;
}



/* アーカイブ関連 ***********************************************************/

static BOOL checkFileName(
        const char *iSource, const char *iArchive, BOOL iNoZip,
        BOOL iOverwrite )
{
    if( !iSource ) {COM_PRMNG(false);}
    if( !iArchive ) {
        if( iNoZip ) {COM_PRMNG(false);}
        if( strchr( iSource, '*' ) || strchr( iSource, '?' ) ||
            strchr( iSource, ' ' ) )
        {
            COM_PRMNG(false);
        }
    }
    else {
        if( com_checkExistFile( iArchive ) && !iOverwrite ) {
            com_error( COM_ERR_FILEDIRNG, "file already exist (%s)", iArchive );
            return false;
        }
    }
    return true;
}

static pthread_mutex_t  gMutexPack = PTHREAD_MUTEX_INITIALIZER;
static char  gPackBuff[COM_TEXTBUF_SIZE];

#define catCmd( ... ) \
    com_connectString( gPackBuff, sizeof(gPackBuff), __VA_ARGS__ )

static BOOL setZipCommand(
        const char *iSource, const char *iArchive, const char *iKey )
{
    COM_CLEAR_BUF( gPackBuff );
    (void)com_strcpy( gPackBuff, "zip " );
    if( iKey ) {if( !catCmd( "-P %s ", iKey ) ) {return false;}}
    if( iArchive ) {if( !catCmd( "%s ", iArchive ) ) {return false;}}
    else {if( !catCmd( "%s.zip ", iSource ) ) {return false;}}
    return catCmd( "%s >& /dev/null", iSource );
}

static BOOL trimZip( const char *iArchive )
{
    if( !iArchive ) {return true;}
    char*  posSlash  = strrchr( iArchive, '/' );
    char*  posPeriod = strrchr( iArchive, '.' );
    if( posPeriod ) {
        if( !posSlash ) {return true;}
        if( posSlash < posPeriod ) {return true;}
    }
    snprintf( gPackBuff, sizeof(gPackBuff), "%s.zip", iArchive );
    if( 0 > rename( gPackBuff, iArchive ) ) {
        com_error( COM_ERR_ARCHIVENG, "fail to rename file(%s)", iArchive );
        return false;
    }
    return true;
}

BOOL com_zipFile(
        const char *iArchive, const char *iSource, const char *iKey,
        BOOL iNoZip, BOOL iOverwrite )
{
    if( !checkFileName(iSource, iArchive, iNoZip, iOverwrite ) ) {return false;}
    com_mutexLockCom( &gMutexPack, COM_FILELOC );
    if( !setZipCommand( iSource, iArchive, iKey ) ) {
        com_error( COM_ERR_ARCHIVENG, "fail to create archive command" );
        return com_mutexUnlockCom( &gMutexPack, COM_FILELOC, false );
    }
    if( system( gPackBuff ) ) {
        com_error( COM_ERR_ARCHIVENG, "fail to archive file (%s)", iSource );
        return com_mutexUnlockCom( &gMutexPack, COM_FILELOC, false );
    }
    BOOL  result = true;
    if( iNoZip ) {if( !trimZip( iArchive ) ) {result = false;}}
    return com_mutexUnlockCom( &gMutexPack, COM_FILELOC, result );
}

static BOOL setUnzipCommand(
        const char *iArchive, const char *iPath, const char *iKey )
{
    COM_CLEAR_BUF( gPackBuff );
    (void)com_strcpy( gPackBuff, "unzip " );
    if( iKey ) {if( !catCmd( "-P %s ", iKey ) ) {return false;}}
    if( !catCmd( "%s ", iArchive ) ) {return false;}
    if( iPath ) {if( !catCmd( "-d %s ", iPath ) ) {return false;}}
    return catCmd( ">& /dev/null" );
}

BOOL com_unzipFile(
        const char *iArchive, const char *iTargetPath, const char *iKey,
        BOOL iDelete )
{
    if( !iArchive ) {COM_PRMNG(false);}
    else {
        if( !com_checkExistFile( iArchive ) ) {
            com_error( COM_ERR_FILEDIRNG, "file not exist (%s)", iArchive );
            return false;
        }
    }
    com_mutexLockCom( &gMutexPack, COM_FILELOC );
    if( !setUnzipCommand( iArchive, iTargetPath, iKey ) ) {
        com_error( COM_ERR_ARCHIVENG, "fail to create unarchive command" );
        return com_mutexUnlockCom( &gMutexPack, COM_FILELOC, false );
    }
    if( system( gPackBuff ) ) {
        com_error( COM_ERR_ARCHIVENG, "fail to unarchive file (%s)", iArchive );
        return com_mutexUnlockCom( &gMutexPack, COM_FILELOC, false );
    }
    if( iDelete ) {remove( iArchive );}
    return com_mutexUnlockCom( &gMutexPack, COM_FILELOC, true );
}



/* 共通モジュール初期化 *****************************************************/

static com_dbgErrName_t  gErrorNameCom[] = {
    /* COM_NO_ERROR の定義は不要 */
    /**** システムエラー ****/
    { COM_ERR_PARAMNG,       "COM_ERR_PARAMNG" },
    { COM_ERR_NOMEMORY,      "COM_ERR_NOMEMORY" },
    { COM_ERR_DOUBLEFREE,    "COM_ERR_DOUBLEFREE" },
    { COM_ERR_CONVERTNG,     "COM_ERR_CONVERTNG" },
    { COM_ERR_FILEDIRNG,     "COM_ERR_FILEDIRNG" },
    { COM_ERR_HASHNG,        "COM_ERR_HASHNG" },
    { COM_ERR_TIMENG,        "COM_ERR_TIMENG" },
    { COM_ERR_ARCHIVENG,     "COM_ERR_ARCHIVENG" },
    { COM_ERR_THREADNG,      "COM_ERR_THREADNG" },
    { COM_ERR_MLOCKNG,       "COM_ERR_MLOCKNG" },
    { COM_ERR_MUNLOCKNG,     "COM_ERR_MUNLOCKNG" },
    { COM_ERR_RING,          "COM_ERR_RING" },
    { COM_ERR_CONFIG,        "COM_ERR_CONFIG" },
    /**** システムエラー(デバッグ) ****/
    { COM_ERR_DEBUGNG,       "COM_ERR_DEBUGNG" },
    { COM_ERR_END,           "" } // 最後は必ずこれで
};

static void finalizeCom( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    com_finalizeSpec();
    freeOptSetting();
    freeAllCfgData();
    com_resetBuffer( &gPopBuff );  // これ以降 com_popChainData()使用不可
    freeFormString();              // これ以降 com_getString()使用不可
    freeAllHash( COM_FILELOC );    // これ以降 ハッシュテーブル使用不可
    com_finalizeThread();
    com_dispFuncTrace();           // 関数呼び出しトレース未使用時は空白化
    com_adjustError();             // これ以降 エラー出力不可
    com_skipMemInfo( false );
    com_finalizeDebugMode();
    COM_DEBUG_AVOID_END( COM_NO_SKIPMEM );
}

void com_initialize( int iArgc, char **iArgv )
{
    COM_DEBUG_AVOID_START( COM_NO_FUNCNAME | COM_NO_SKIPMEM );
    atexit( finalizeCom );
    setCommandLine( iArgc, iArgv );
    com_initializeDebugMode();     // com_initializeSpec() はこの中で呼ぶ
    com_dbgFuncCom( NULL );
    com_skipMemInfo( true );
    com_registerErrorCode( gErrorNameCom );
    com_registerErrorCodeSpec();
    com_initializeThread();
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

