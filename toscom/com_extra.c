/*
 *****************************************************************************
 *
 * 共通処理エキストラ機能    by TOS
 *
 *   外部公開I/Fの使用方法については com_extra.h を必読。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"
#include "com_extra.h"


// com_input()用 チェック関数群 ----------------------------------------------

BOOL com_isYes( const char *iData )
{
    if( com_compareString( "yes", iData, 0, true ) ||
        com_compareString( "y", iData, 0, true ) )
    {
        return true;
    }
    return false;
}

BOOL com_valOnlyEnter( char *ioData, void *iCond )
{
    COM_UNUSED( iCond );
    if( strlen( ioData ) ) {return false;}
    return true;
}

BOOL com_valSaveFile( char *ioData, void *iCond )
{
    com_valCondSave_t*  cond = iCond;
    if( !com_checkExistFile( ioData ) ) {return true;}
    if( cond ) {
        if( cond->forceRetry ) {return false;}
        if( cond->forceOverwrite ) {return true;}
    }
    com_actFlag_t  flags = { .clear = false,  .enterSkip = false };
    char*  form = "%s already exist. are you sure to overwrite?\n";
    if( cond ) {if(cond->fileExist) {form = cond->fileExist;}}
    return com_askYesNo( &flags, form, ioData );
}
BOOL com_valSaveFileCondCopy( void **oCond, void *iCond )
{
    com_valCondSave_t*  source = iCond;
    com_valCondSave_t*  tmp = com_malloc( sizeof( com_valCondSave_t ),
                                          "copy save file condition" );
    if( COM_UNLIKELY(!tmp) ) {return false;}
    *tmp = *source;
    if( !(tmp->fileExist = com_strdup( source->fileExist, NULL )) ) {
        com_free( tmp );
        return false;
    }
    *oCond = tmp;
    return true;
}
void com_valSaveFileCondFree( void **oCond )
{
    com_valCondSave_t*  target = *oCond;
    if( target ) {com_free( target->fileExist );}
}

BOOL com_valLoadFile( char *ioData, void *iCond )
{
    com_valCondLoad_t*  cond = iCond;
    char*  cancel = "-";
    if( cond ) {if( cond->cancel ) {cancel = cond->cancel;}}
    if( !strcmp( ioData, cancel ) ) {return true;}
    if( !com_checkExistFile( ioData ) ) {
        char*  form = "%s not exist...\n";
        if( cond ) {
            if( cond->noDisp ) {return false;}
            if( cond->noFile ) {form = cond->noFile;}
        }
        com_printf( form, ioData );
        return false;
    }
    return true;
}
BOOL com_valLoadFileCondCopy( void **oCond, void* iCond )
{
    com_valCondLoad_t*  source = iCond;
    com_valCondLoad_t*  tmp = com_malloc( sizeof( com_valCondLoad_t),
                                          "copy load file condition" );
    if( COM_UNLIKELY(!tmp) ) {return false;}
    *tmp = *source;
    if( !(tmp->cancel = com_strdup( source->cancel, NULL )) ) {
        com_free( tmp );
        return false;
    }
    if( !(tmp->noFile = com_strdup( source->noFile, NULL )) ) {
        com_free( tmp->cancel );
        com_free( tmp );
        return false;
    }
    *oCond = tmp;
    return true;
}
void com_valLoadFileCondFree( void **oCond )
{
    com_valCondLoad_t*  target = *oCond;
    if( target ) {
        com_free( target->cancel );
        com_free( target->noFile );
    }
}



// データ入力処理 ------------------------------------------------------------

static int dropOverBuffer( void )
{
    int  count = 0;
    int  over;
    // バッファサイズを超えてから入力された内容を読み捨てる
    do {
        over = getchar();
        count++;
    } while( over != '\n' && over != EOF );
    return count;  // 読み捨てた数を一応返す
}

static BOOL inputString( char *oData, size_t iSize, BOOL *oOnlyEnter )
{
    *oOnlyEnter = false;
    if( !fgets( oData, (int)iSize, stdin ) ) {return false;}
    size_t len = strlen( oData );
    if( oData[len - 1] == '\n' ) {
        oData[len - 1] = '\0';
        if( 0 == strlen(oData) ) {*oOnlyEnter = true;}
    }
    else {(void)dropOverBuffer();}  // バッファサイズを超えた入力の読み捨て
    com_printfLogOnly( "%s\n", oData );
    return true;
}

static char  gPromptBuff[COM_TEXTBUF_SIZE];

static size_t inputProc(
        char *oData, size_t iSize, const com_valFunc_t *iVal,
        const com_actFlag_t *iFlag, const char *iFuncName,
        const char *iFormat, va_list iAp )
{
    if( COM_UNLIKELY(!oData) ) {COM_PRMNGF(iFuncName, 0);}
    if( !iFlag ) {iFlag = &(com_actFlag_t){ false, false };}
    if( iFormat ) {vsnprintf( gPromptBuff, sizeof(gPromptBuff), iFormat, iAp );}
    char*  prompt = iFormat ? gPromptBuff : NULL;
    BOOL  onlyEnter = false;
    while(1) {
        if( iFlag->clear ) {com_clear();}
        if( prompt ) {com_printf( "%s", prompt );}
        if( !inputString( oData, iSize, &onlyEnter ) ) {continue;}
        if( onlyEnter ) {if( iFlag->enterSkip ) {break;} else {continue;}}
        // データ入力に問題がなければループを抜けて返す
        if( !iVal ) {break;}
        if( (iVal->func)( oData, iVal->cond ) ) {break;}
    }
    if( onlyEnter ) {return 0;}
    return strlen( oData );
}
        
// inputProc()を呼び、その結果を result に入れるマクロ
#define INPUT( DATA, SIZE, VAL, FLAG ) \
    size_t  result = 0; \
    memset( DATA, 0, SIZE ); \
    do { \
        va_list  ap; \
        va_start( ap, iFormat ); \
        result = inputProc( DATA, SIZE, VAL, FLAG, __func__, iFormat, ap ); \
        va_end( ap ); \
    } while(0)

size_t com_inputVar(
        char *oData, size_t iSize, const com_valFunc_t *iVal,
        const com_actFlag_t *iFlag, const char *iFormat, ... )
{
    if( COM_UNLIKELY(!oData) ) {COM_PRMNG(0);}
    INPUT( oData, iSize, iVal, iFlag );
    return result;
}

// キー入力用共通バッファ
static char  gKeyBuff[COM_TEXTBUF_SIZE];

size_t com_input(
        char **oData, const com_valFunc_t *iVal,
        const com_actFlag_t *iFlag, const char *iFormat, ... )
{
    if( COM_UNLIKELY(!oData) ) {COM_PRMNG(0);}
    *oData = gKeyBuff;
    INPUT( gKeyBuff, sizeof(gKeyBuff), iVal, iFlag );
    return result;
}

size_t com_inputMultiLine( char *oData, size_t iSize, const char *iFormat, ... )
{
    if( COM_UNLIKELY(!oData) ) {COM_PRMNG(0);}
    COM_SET_FORMAT( gPromptBuff );
    if( iFormat ) {com_printf( "%s", gPromptBuff );}
    memset( oData, 0, iSize );
    clearerr( stdin );
    size_t  result = fread( oData, 1, iSize, stdin );
    com_printfLogOnly( "%s", oData );
    if( ferror( stdin ) ) {
        com_error( COM_ERR_READTXTNG, "fail to read text" );
        return 0;
    }
    return result;
}

#define INPUTWRAP( DEFAULT, FUNC, FLAGS ) \
    char  tmp[8] = {0}; \
    do { \
        const char  defaultLabel[] = DEFAULT; \
        if( !iFormat ) {iFormat = defaultLabel;} \
        com_valFunc_t  cond = { .func = (FUNC) }; \
        INPUT( tmp, sizeof(tmp), &cond, (FLAGS) ); \
        COM_UNUSED( result ); \
    } while(0)

BOOL com_askYesNo( const com_actFlag_t *iFlag, const char *iFormat, ... )
{
    INPUTWRAP( "(Yes/No) ", com_valYesNo, iFlag );
    return com_isYes( tmp );
}

void com_waitEnter( const char *iFormat, ... )
{
    com_actFlag_t  flags = { .clear = false,  .enterSkip = true };
    INPUTWRAP( "  --- HIT ENTER KEY ---\n", com_valOnlyEnter, &flags );
}

FILE *com_askFile( const com_actFlag_t *iFlags, const com_askFile_t *iAskCond,
                   const char *iFormat, ... )
{
    char filename[COM_TEXTBUF_SIZE] = {0};
    COM_ASK_TYPE_t askType = iAskCond->type;
    com_valFunc_t val = {
        (askType == COM_ASK_SAVEFILE) ?  com_valSaveFile : com_valLoadFile,
        (void*)&(iAskCond->valCond)
    };
    INPUT( filename, sizeof(filename), &val, iFlags );
    if( !result ) {return NULL;}
    char* mode = (askType == COM_ASK_SAVEFILE) ? "w+" : "r+";
    return com_fopen( filename, mode );
}



// 入力メニュー生成処理 ------------------------------------------------------

// 入力メニュー共通ルーチン処理
static char  gMenuBuff[COM_DATABUF_SIZE];

static size_t  gMenuCount = 0;     // メニューリストバッファのカウント
static long*   gMenuList = NULL;   // メニューリストバッファ(終了処理で解放)

static void setListBuffer( const com_selector_t *iSelector )
{
    size_t  count = 0;
    for( ;  iSelector->code > 0;  iSelector++ ) {count++;}
    // 確保済みサイズに足りるなら何もしない
    if( count <= gMenuCount ) {return;}
    gMenuList = com_reallocf( gMenuList, sizeof(long) * count, "gMenuList" );
    // メモリ不足の場合、処理続行不可能
    if( !gMenuList ) {com_exit( COM_ERR_NOMEMORY );}
    gMenuCount = count;
    return;
}

static void checkList( long iCount, long iCode )
{
    for( long i = 0;  i < iCount;  i++ ) {
        // メニュー番号に被りがあったら、その場で終了(バグなので要修正)
        if( gMenuList[i] == iCode ) {
            com_errorExit( COM_ERR_DEBUGNG,
                           "menu code conflict (%ld)\n", iCode );
        }
    }
}

#define ADD_MENU_BUFF( ... ) \
    (void)com_connectString( gMenuBuff, sizeof(gMenuBuff), __VA_ARGS__ )

static void addWithPrompt( const char *iText, const char *iPrompt )
{
    ADD_MENU_BUFF( iText );
    if( iPrompt ) {ADD_MENU_BUFF( iPrompt );}
}

static long gPrevCode = 0;

static BOOL judgeLf( long iCode, long iBorderLf )
{
    if( (iCode - 1) / iBorderLf != (gPrevCode - 1) / iBorderLf ) {return true;}
    return false;
}

static void addMenu(
        const com_selector_t *iMenu, long *ioCount, long iBorderLf,
        const char *iPrompt )
{
    long  code = iMenu->code;
    // 作業データ初期化
    if( gPrevCode < 0 ) {
        addWithPrompt( "", iPrompt );
        gPrevCode = code;
    }
    // 改行タイミングか判定
    if( judgeLf( code, iBorderLf ) ) {addWithPrompt( "\n", iPrompt );}
    // メニューを実際に出すかチェック
    BOOL dispFlag = true;
    if( iMenu->check ) { dispFlag = (iMenu->check)( code ); }
    if( dispFlag ) {
        ADD_MENU_BUFF( "  %2ld:%s", code, iMenu->label );
        checkList( *ioCount, code );
        gMenuList[(*ioCount)++] = code;
    }
    else {ADD_MENU_BUFF( "     %*s", (int)strlen( iMenu->label ), " " );}
    gPrevCode = code;
}

static void makeMenu(
        const com_selector_t *iSelector, const com_selPrompt_t *iPrompt,
        long *oCount )
{
    COM_CLEAR_BUF( gMenuBuff );
    gPrevCode = -1;    // 負数を入れて addMenu()に初期化を促す

    if( iPrompt->head ) {(void)com_strcat( gMenuBuff, iPrompt->head );}
    setListBuffer( iSelector );
    for( const com_selector_t* tmp = iSelector;  tmp->code > 0;  tmp++ ) {
        addMenu( tmp, oCount, iPrompt->borderLf, iPrompt->prompt );
    }
    if( iPrompt->foot ) {(void)com_strcat( gMenuBuff, iPrompt->foot );}
}

static BOOL callMenuFunc( long iCode, const com_selector_t *iSelector )
{
    while( iSelector->code != iCode ) {iSelector++;}
    BOOL  result = true;
    if( iSelector->func ) {result = (iSelector->func)( iCode );}
    return result;
}

BOOL com_execSelector(
        const com_selector_t *iSelector, const com_selPrompt_t *iPrompt,
        const com_actFlag_t *iFlag )
{
    if( COM_UNLIKELY(!iSelector) ) {COM_PRMNG(false);}
    long  count = 0;
    makeMenu( iSelector, iPrompt, &count );
    com_valCondDgtList_t  cond = { count, gMenuList };
    com_valFunc_t  val = { .func = com_valDgtList,  .cond = &cond };
    char  key[10] = {0};
    com_inputVar( key, sizeof(key), &val, iFlag, gMenuBuff );
    return callMenuFunc( com_atol(key), iSelector );
}



// 統計情報計算処理 ----------------------------------------------------------

void com_readyStat( com_calcStat_t *oStat, BOOL iNeedList )
{
    if( COM_UNLIKELY(!oStat) ) {COM_PRMNG();}
    *oStat = (com_calcStat_t){
        .needList = iNeedList,  .existNegative = false,  .list = NULL,
        .max = COM_VAL_NO_MIN,  .min = COM_VAL_NO_MAX
    };
}

static BOOL addInputData( com_calcStat_t *oStat, long iData )
{
    if( !(oStat->needList) ) {return true;}
    oStat->list = com_reallocf( oStat->list, sizeof(long) * (uint)oStat->count,
                                "stat list (%ld)", oStat->count );
    if( !(oStat->list) ) {return false;}
    oStat->list[oStat->count - 1] = iData;
    return true;
}

static BOOL checkOverflow( double *oTarget, double iInput )
{
    if( DBL_MAX - *oTarget < iInput ) {return false;}
    *oTarget += iInput;
    return true;
}

static double calcSkewness(
        double iCbT, double iSqT, double iAvr, double iDev, double iCnt )
{
    return ( iCbT / iCnt - 3 * iAvr * iSqT / iCnt + 2 * pow( iAvr, 3 ) )
           / pow( iDev, 3 );
}

static double calcKurtosis(
        double iFoT, double iCbT, double iSqT, double iAvr, double iDev,
        double iCnt )
{
    return ( iFoT / iCnt - 4 * iAvr * iCbT / iCnt
             + 6 * iAvr * iAvr * iSqT / iCnt - 3 * pow( iAvr, 4 ) )
           / pow( iDev, 4 );
}

static void calcStat( com_calcStat_t *oStat )
{
    double  cnt = (double)(oStat->count);
    oStat->average = oStat->total / cnt;
    oStat->geoavr = oStat->existNegative ? 0 : exp( oStat->lgtotal / cnt );
    oStat->hrmavr = oStat->existNegative ? 0 : 1 / ( oStat->rvtotal / cnt );

    oStat->variance = oStat->sqtotal / cnt - pow( oStat->average, 2 );
    oStat->stddev = sqrt( oStat->variance );
    oStat->coevar = oStat->stddev / oStat->average;

    oStat->skewness = calcSkewness( oStat->cbtotal, oStat->sqtotal,
                                    oStat->average, oStat->stddev, cnt );
    oStat->kurtosis = calcKurtosis( oStat->fototal, oStat->cbtotal,
                                    oStat->sqtotal, oStat->average,
                                    oStat->stddev, cnt );
}

BOOL com_inputStat( com_calcStat_t *oStat, long iData )
{
    if( COM_UNLIKELY(!oStat) ) {COM_PRMNG(false);}
    double  tmp = (double)iData;
    (oStat->count)++;
    if( iData < 0 ) {oStat->existNegative = true;}
    if( !addInputData( oStat, iData ) ) {return false;}
    if( iData > oStat->max ) {oStat->max = iData;}
    if( iData < oStat->min ) {oStat->min = iData;}
    if( !checkOverflow( &(oStat->total), tmp ) ) {return false;}
    if( !checkOverflow( &(oStat->sqtotal), tmp * tmp ) ) {return false;}
    if( !checkOverflow( &(oStat->cbtotal), pow(tmp,3) ) ) {return false;}
    if( !checkOverflow( &(oStat->fototal), pow(tmp,4) ) ) {return false;}
    if( !checkOverflow( &(oStat->lgtotal), log(tmp) ) ) {return false;}
    if( !checkOverflow( &(oStat->rvtotal), 1 / tmp ) ) {return false;}
    calcStat( oStat );
    return true;
}

void com_finishStat( com_calcStat_t *oStat )
{
    if( COM_UNLIKELY(!oStat) ) {COM_PRMNG();}
    if( oStat->needList ) {com_free( oStat->list );}
}



// 数学計算 ------------------------------------------------------------------

BOOL com_isPrime( ulong iNumber )
{
    if( iNumber < 2 ) {return false;}
    ulong  sqrtNum = (ulong)(lrint( sqrt( (double)iNumber ) ));
    if( sqrtNum < 2 ) {return true;}
    for( ulong i = 2;  i <= sqrtNum;  i++ ) {
        if( !(iNumber % i) ) {return false;}
    }
    return true;
}



// 乱数関連 ------------------------------------------------------------------

// マルチスレッドに耐えられるよう srandom_r() と random_r() を使う
static __thread BOOL  gSetSeed = false;

#ifdef __linux__  // Linux用はリエントラントな関数群を使用を試みる
static __thread struct random_data  gRandomBuf;
enum { STATE_SIZE = 64 };
static __thread char  gRandomState[STATE_SIZE];

static BOOL setSeed( void )
{
    static uint  adjust = 0; // 同じ時間帯に複数呼ばれても seedが変わるように
    uint  seed = (uint)time(0) + adjust;
    adjust++;
    // srandom_r()を呼ぶ前に gRandomBuf の内容を初期化
    if( initstate_r( seed, gRandomState, sizeof(gRandomState), &gRandomBuf ) ) {
        com_error( COM_ERR_RANDOMIZE, "initstate_r() NG" );
        return false;
    }
    if( srandom_r( seed, &gRandomBuf ) ) {
        com_error( COM_ERR_RANDOMIZE, "srandom_r() NG" );
        return false;
    }
    return true;
}

long com_rand( long iMax )
{
    if( iMax == 0 ) { return 0; }
    if( iMax > INT_MAX ) { iMax = INT_MAX; }
    if( iMax < INT_MIN ) { iMax = INT_MIN; }
    BOOL  negative = false;
    if( iMax < 0 ) {iMax = labs( iMax );  negative = true;}
    if( !gSetSeed ) {if( !(gSetSeed = setSeed()) ) {return 0;}}
    int  result = 0;
    if( !random_r( &gRandomBuf, &result ) ) {
        result = result % (int)iMax + 1;
        if( negative ) {result *= -1;}
    }
    else {com_error( COM_ERR_RANDOMIZE, "random_r() NG" );}
    return (long)result;
}
#else   // __linux__  (Windows版は こちらの一般的な標準関数で)
static BOOL setSeed( void )
{
    static uint  adjust = 0; // 同じ時間帯に複数呼ばれても seedが変わるように
    uint  seed = (uint)time(0) + adjust;
    adjust++;
    srandom( seed );
    return true;
}

long com_rand( long iMax )
{
    if( iMax == 0 ) {return 0;}
    BOOL  negative = false;
    if( iMax < 0 ) {iMax = labs( iMax );  negative = true;}
    if( !gSetSeed ) {if( !(gSetSeed = setSeed()) ) {return 0;}}
    long  randomValue = random();
    long  result = randomValue % iMax + 1;
    if( negative ) {result *= -1;}
    return result;
}
#endif  // __linux__

BOOL com_checkChance( long iChance )
{
    long  dice = com_rand( 100 );
    if( dice > iChance ) {return false;}
    return true;
}

static void dispAdjust( long iAdjust )
{
    if( iAdjust ) {
        if( iAdjust > 0 ) {com_printf( " +%ld", iAdjust );}
        else {com_printf( " %ld", iAdjust );}
    }
}

long com_rollDice( long iDice, long iSide, long iAdjust, BOOL iDisp )
{
    if( iDisp ) {
        com_printf( "%ldd%ld", iDice, iSide );
        dispAdjust( iAdjust );
        com_printf( ": " );
    }
    long  result = 0;
    for( long i = 0;  i < iDice;  i++ ) {
        long  dice = com_rand( iSide );
        if( iDisp ) {
            if( i > 0 ) {com_printf( "+" );}
            com_printf( "%ld", dice );
        }
        result += dice;
    }
    if( iDisp && iDice > 1 ) {com_printf( " = %ld", result );}
    result += iAdjust;
    if( iDisp ) {
        dispAdjust( iAdjust );
        if( iAdjust ) {com_printf( " = %ld", result );}
        com_printLf();
    }
    return result;
}

// シグナルハンドラー関連 ----------------------------------------------------

static __thread com_sortTable_t  gSigOldAct;
static __thread BOOL  gInitSigOldAct = false;

static void initializeSigOldAct( void )
{
    gInitSigOldAct = true;
    com_initializeSortTable( &gSigOldAct, COM_SORT_SKIP );
}

static long countSignals( const com_sigact_t *iSigList )
{
    if( !iSigList ) {return 0;}
    long  count = 0;
    for( ; iSigList->signal != COM_SIGACT_END;  iSigList++ ) {count++;}
    return count;
}

static BOOL procSigaction( long iCount, const com_sigact_t *iSigList )
{
    for( long i = 0;  i < iCount;  i++ ) {
        const com_sigact_t*  tmp = &(iSigList[i]);
        if( tmp->signal > INT_MAX ) {
            com_error( COM_ERR_DEBUGNG,
                       "signal code (%ld) is too large", tmp->signal );
            return false;
        }
        struct sigaction  oldact;
        if( 0 > sigaction( (int)tmp->signal, &(tmp->action), &oldact ) ) {
            com_error( COM_ERR_SIGNALNG, "fail to register sigaction(%ld/%d)",
                       tmp->signal, errno );
            return false;
        }
        BOOL  isCol = false;
        BOOL  result = com_addSortTableByKey( &gSigOldAct, tmp->signal,
                                              &oldact, sizeof(oldact), &isCol );
        if( !result ) {return false;}
        if( isCol ) {  // 上書き発生時エラーは出すが、処理は続行する
            com_error( COM_ERR_DEBUGNG,
                       "same signal already registered(%ld)", tmp->signal );
            isCol = false;
        }
    }
    return true;
}

BOOL com_setSignalAction( const com_sigact_t *iSigList )
{
    long  count = countSignals( iSigList );
    if( !count ) {return true;}
    if( !gInitSigOldAct ) {initializeSigOldAct();}
    com_skipMemInfo( true );
    BOOL  result = procSigaction( count, iSigList );
    if( !result ) {com_resumeSignalAction( COM_SIGACT_ALL );}
    com_skipMemInfo( false );
    return result;
}

void com_resumeSignalAction( long iSignum )
{
    if( COM_UNLIKELY(iSignum > INT_MAX) ) {COM_PRMNG();}
    BOOL  resumed = false;
    for( long i = 0;  i < gSigOldAct.count;  i++ ) {
        com_sigact_t*  tmp = gSigOldAct.table[i].data;
        if( iSignum == COM_SIGACT_ALL || iSignum == tmp->signal ) {
            (void)sigaction( (int)tmp->signal, &(tmp->action), NULL );
            resumed = true;
        }
    }
    if( !resumed && iSignum != COM_SIGACT_ALL ) {
        com_error( COM_ERR_DEBUGNG, "no such signal registered(%ld)",iSignum );
        return;
    }
    com_skipMemInfo( true );
    if( iSignum == COM_SIGACT_ALL ) {com_freeSortTable( &gSigOldAct );}
    else {com_deleteSortTableByKey( &gSigOldAct, iSignum );}
    com_skipMemInfo( false );
}



// データパッケージ関連 ------------------------------------------------------

#define ARCHLEN  COM_LINEBUF_SIZE
#define EXTZIP   ".zip"

static BOOL existPackFile( com_packInf_t *ioInf, char *oArcName )
{
    if( com_checkExistFile( ioInf->filename ) ) {return false;}
    if( ioInf->useZip ) {
        char*  tmp = ioInf->archive ? ioInf->archive : ioInf->filename;
        (void)com_strncpy( oArcName, ARCHLEN, tmp, strlen(tmp) );
        if( !ioInf->noZip ) {
            (void)com_strncat( oArcName, ARCHLEN, EXTZIP, strlen(EXTZIP) );
        }
        return com_checkExistFile( oArcName );
    }
    return false;
}

static BOOL extractPack( com_packInf_t *ioInf, char *iArchive )
{
    if( !ioInf->useZip ) {return true;}
    if( !com_unzipFile( iArchive, NULL, ioInf->key, ioInf->delZip ) ) {
        return false;
    }
    if( !com_checkExistFile( ioInf->filename ) ) {
        com_error( COM_ERR_FILEDIRNG, "extracted file name unmatch" );
        return false;
    }
    return true;
}

BOOL com_readyPack( com_packInf_t *ioInf, const char *iConfirm )
{
    if( !ioInf ) {COM_PRMNG(false);}
    if( !ioInf->filename ) {COM_PRMNG(false);}
    char  archive[ARCHLEN] = {0};
    BOOL  exist = existPackFile( ioInf, archive );
    if( ioInf->writeFile ) {
        if( exist && iConfirm ) {
            if( !com_askYesNo( false, iConfirm ) ) {return false;}
            if( strlen(archive) ) {remove( archive );}
        } // 確認しない(iConfirm==NULL)時は無条件で上書きする。
        ioInf->fp = com_fopen( ioInf->filename, "wb" );
    }
    else {
        if( !exist ) {
            com_error( COM_ERR_FILEDIRNG, "no data file" );
            return false;
        }
        if( !extractPack( ioInf, archive ) ) {return false;}
        ioInf->fp = com_fopen( ioInf->filename, "rb" );
    }
    return (0 != ioInf->fp);
}

BOOL com_finishPack( com_packInf_t *ioInf, BOOL iFinal )
{
    if( COM_UNLIKELY(!ioInf) ) {COM_PRMNG(false);}
    com_skipMemInfo( false );
    BOOL  result = true;
    com_fclose( ioInf->fp );
    if( iFinal ) {
        if( ioInf->useZip ) {
            if( ioInf->writeFile ) {
                result = com_zipFile( ioInf->archive, ioInf->filename,
                                      ioInf->key, ioInf->noZip, true );
            }
            remove( ioInf->filename );  // 読み書きいずれも元ファイルは削除
        }
    }
    else {  // 未完了の場合はファイル削除して終了
        if( ioInf->writeFile || ioInf->useZip ) {remove( ioInf->filename );}
    }
    com_skipMemInfo( true );
    return result;
}

enum { BINSIZE = 2 };

static BOOL writeDataText( com_packInf_t *ioInf, void *iAddr, size_t iSize )
{
    for( size_t i = 0;  i < iSize;  i++ ) {
        uchar*  tmp = iAddr;
        if( BINSIZE != fprintf( ioInf->fp, "%02x", tmp[i] ) ) {return false;}
    }
    return true;
}

static BOOL writeDataBin( com_packInf_t *ioInf, void *iAddr, size_t iSize )
{
    size_t  result = fwrite( iAddr, 1, iSize, ioInf->fp );
    if( result < iSize ) {return false;}
    return true;
}

static BOOL writeDataFile( com_packInf_t *ioInf, void *iAddr, size_t iSize )
{
    if( ioInf->byText ) {return writeDataText( ioInf, iAddr, iSize );}
    return writeDataBin( ioInf, iAddr, iSize );
}

static BOOL writeDataSize( com_packInf_t *ioInf, com_packElm_t *iElm )
{
    if( iElm->size > 0xFFFF ) {return false;}
    ushort dataSize = iElm->size & 0xFFFF;
    return writeDataFile( ioInf, &dataSize, sizeof(dataSize) );
}

static BOOL writePackData( com_packInf_t *ioInf, com_packElm_t *iElm )
{
    if( iElm->varSize ) {if( !writeDataSize( ioInf, iElm ) ) {return false;}}
    return writeDataFile( ioInf, iElm->data, iElm->size );
}

BOOL com_writePack( com_packInf_t *ioInf, com_packElm_t *iElm, long iCount )
{
    if( COM_UNLIKELY(!ioInf || !iElm || !iCount) ) {COM_PRMNG(false);}
    if( COM_UNLIKELY(!ioInf->writeFile) ) {COM_PRMNG(false);}
    for( long i = 0;  i < iCount;  i++ ) {
        if( !writePackData( ioInf, iElm + i ) ) {
            com_error( COM_ERR_FILEDIRNG, "fail to write %s(%ld:%p)",
                       ioInf->filename, i, iElm[i].data );
            return false;
        }
    }
    return true;
}

BOOL com_writePackDirect(
        com_packInf_t *ioInf, void *iAddr, size_t iSize, BOOL iVarSize )
{
    if( COM_UNLIKELY(!ioInf || !iAddr) ) {COM_PRMNG(false);}
    com_packElm_t  tmp = { iAddr, iSize, iVarSize };
    return com_writePack( ioInf, &tmp, 1 );
}


static BOOL readPackText( com_packInf_t *ioInf, com_packElm_t *ioElm )
{
    uchar*  target = ioElm->data;
    for( size_t i = 0;  i < ioElm->size;  i++ ) {
        char  oct[BINSIZE + 1] = {0};
        if( !fgets( oct, sizeof(oct), ioInf->fp ) ) {return false;}
        target[i] = (uchar)com_strtoul( oct, 16, false );
    }
    return true;
}

static BOOL readPackBin( com_packInf_t *ioInf, com_packElm_t *ioElm )
{
    uchar*  target = ioElm->data;
    if( ioElm->size != fread( target, 1, ioElm->size, ioInf->fp ) ) {
        return false;
    }
    return true;
}

static BOOL readDataSize( com_packInf_t *ioInf, com_packElm_t *ioElm )
{
    ushort  len = 0;
    if( !com_readPackFix( ioInf, &len, sizeof(len) ) ) {return false;}
    ioElm->size = len;
    return true;
}

static BOOL allocPackMemory( com_packInf_t *ioInf, com_packElm_t *ioElm )
{
    if( ioElm->data ) {return true;}
    ioElm->data = com_malloc( ioElm->size, "allocate data pack from %s",
                              ioInf->filename );
    return (ioElm->data != NULL);
}

static BOOL readPackData( com_packInf_t *ioInf, com_packElm_t *ioElm )
{
    if( ioElm->varSize ) {if( !readDataSize( ioInf, ioElm ) ) {return false;}}
    if( !allocPackMemory( ioInf, ioElm ) ) {return false;}
    if( ioInf->byText ) {return readPackText( ioInf, ioElm );}
    return readPackBin( ioInf, ioElm );

}

BOOL com_readPack( com_packInf_t *ioInf, com_packElm_t *iElm, long iCount )
{
    if( COM_UNLIKELY(!ioInf || !iElm || !iCount) ) {COM_PRMNG(false);}
    if( COM_UNLIKELY(ioInf->writeFile) ) {COM_PRMNG(false);}
    for( long i = 0;  i < iCount;  i++ ) {
        if( !readPackData( ioInf, iElm + i ) ) {
            com_error( COM_ERR_FILEDIRNG, "fail to read %s (%ld:%p)",
                       ioInf->filename, i, iElm[i].data );
            for( long j = 0;  j <= i;  j++ ) {
                if( iElm[i].data && iElm[i].varSize ) {com_free(iElm[i].data);}
            }
            return false;
        }
    }
    return true;
}

BOOL com_readPackFix( com_packInf_t *ioInf, void *iAddr, size_t iSize )
{
    if( COM_UNLIKELY(!ioInf || !iAddr) ) {COM_PRMNG(false);}
    com_packElm_t  tmp = { iAddr, iSize, false };
    if( !com_readPack( ioInf, &tmp, 1 ) ) {return false;}
    return true;
}

BOOL com_readPackVar( com_packInf_t *ioInf, void *ioAddr, size_t *ioSize )
{
    if( COM_UNLIKELY(!ioInf || !ioAddr || !ioSize) ) {COM_PRMNG(false);}
    char**  adr = ioAddr;
    com_packElm_t  tmp = { *adr, *ioSize, true };
    if( !com_readPack( ioInf, &tmp, 1 ) ) {return false;}
    *adr    = tmp.data;
    *ioSize = tmp.size;
    return true;
}



// 正規表現関連 --------------------------------------------------------------

static pthread_mutex_t  gMutexRegxp = PTHREAD_MUTEX_INITIALIZER;

static regex_t  *gRegexList = NULL;
static com_regex_id_t  gRegexId = 0;

com_regex_id_t com_regcomp( com_regcomp_t *iRegex )
{
    if( COM_UNLIKELY(!iRegex) ) {COM_PRMNG(COM_REGXP_NG);}
    if( COM_UNLIKELY(!(iRegex->regex)) ) {COM_PRMNG(COM_REGXP_NG);}

    regex_t tmpreg;
    int result = regcomp( &tmpreg, iRegex->regex, (int)iRegex->cflags );
    if( result ) {
        com_error( COM_ERR_REGXP, "regcomp() failed(%d)", result );
        return COM_REGXP_NG;
    }
    long  newId = gRegexId;
    com_skipMemInfo( true );
    com_mutexLock( &gMutexRegxp, __func__ );
    long addresult = com_realloct(
                       &gRegexList, sizeof(gRegexList[0]), &gRegexId, 1,
                       "regex comp list(%ld)", gRegexId );
    com_mutexUnlock( &gMutexRegxp, __func__ );
    com_skipMemInfo( false );
    if( !addresult ) {return COM_REGXP_NG;}
    gRegexList[newId] = tmpreg;
    return newId;
}

BOOL com_regexec( com_regex_id_t iId, com_regexec_t *ioRegexec )
{
    if( iId < 0 || iId >= gRegexId ) {COM_PRMNG(false);}
    if( COM_UNLIKELY(!ioRegexec) ) {COM_PRMNG(false); }
    if( COM_UNLIKELY(!(ioRegexec->target)) ) {COM_PRMNG(false);}

    int result = regexec( &gRegexList[iId], ioRegexec->target,
                          ioRegexec->nmatch, ioRegexec->pmatch,
                          (int)ioRegexec->eflags );
    return !result;
}

BOOL com_makeRegexec(
        com_regexec_t *oRegexec, const char *iTarget, long iEflags,
        size_t iNmatch, regmatch_t *iPmatch )
{
    if( COM_UNLIKELY(!oRegexec || !iTarget) ) {COM_PRMNG(false);}
    if( iNmatch && !iPmatch ) {
        iPmatch = com_malloc( sizeof(regmatch_t) * iNmatch,
                              "com_makeRegexec pmatch[%zu]", iNmatch );
        if( !iPmatch ) {return false;}
    }
    *oRegexec = (com_regexec_t){ iTarget, iEflags, iNmatch, iPmatch };
    return true;
}

void com_freeRegexec( com_regexec_t *ioRegexec )
{
    if( COM_UNLIKELY(!ioRegexec) ) {return;}
    if( ioRegexec->nmatch && ioRegexec->pmatch ) {
        com_free( ioRegexec->pmatch );
    }
}

static __thread char gRegexAnlyzeBuf[COM_TEXTBUF_SIZE];

char *com_analyzeRegmatch(
        com_regexec_t *iRegexec, size_t iIndex, size_t *oSize )
{
    COM_SET_IF_EXIST( oSize, 0 );
    if( COM_UNLIKELY(!iRegexec) ) {COM_PRMNG(NULL);}
    if( iIndex >= iRegexec->nmatch ) {
        com_error( COM_ERR_REGXP, 
                   "index(%ld) over (max=%ld)", iIndex, iRegexec->nmatch );
        return NULL;
    }
    gRegexAnlyzeBuf[0] = '\0';
    regmatch_t  *target = &(iRegexec->pmatch[iIndex]);
    if( target->rm_so >= 0 && target->rm_eo >= 0 ) {
        regoff_t  reglen = target->rm_eo - target->rm_so;
        (void)com_strncpy( gRegexAnlyzeBuf, sizeof(gRegexAnlyzeBuf),
                           &(iRegexec->target[target->rm_so]), (size_t)reglen );
        COM_SET_IF_EXIST( oSize, (size_t)reglen );
    }
    return gRegexAnlyzeBuf;
}

regex_t *com_getRegex( com_regex_id_t iId )
{
    if( iId < 0 || iId >= gRegexId ) {COM_PRMNG(NULL);}
    return &(gRegexList[iId]);
}

static void freeRegex( void )
{
    for( com_regex_id_t i = 0;  i < gRegexId;  i++ ) {
        regfree( &(gRegexList[i]) );
    }
    com_free( gRegexList );
}



// 初期化/終了処理 ///////////////////////////////////////////////////////////

static com_dbgErrName_t  gErrorNameExtra[] = {
    { COM_ERR_READTXTNG,   "COM_ERR_READTXTNG" },
    { COM_ERR_SIGNALNG,    "COM_ERR_SIGNALNG" },
    { COM_ERR_REGXP,       "COM_ERR_REGXP" },
    { COM_ERR_END,         "" }  // 最後は必ずこれで
};

static void finalizeExtra( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    com_free( gMenuList );
    if( gSigOldAct.count ) {com_freeSortTable( &gSigOldAct );}
    freeRegex();
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

void com_initializeExtra( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    com_setInitStage( COM_INIT_STAGE_PROCCESSING, false );
    atexit( finalizeExtra );
    com_registerErrorCode( gErrorNameExtra );
    com_setInitStage( COM_INIT_STAGE_FINISHED, false );
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

