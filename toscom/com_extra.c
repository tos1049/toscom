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



// データ入力処理 ------------------------------------------------------------

static int dropOverBuffer( void )
{
    int count = 0;
    int over;
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
    if( !fgets( oData, (int)iSize, stdin ) ) { return false; }
    size_t len = strlen( oData );
    if( oData[len - 1] == '\n' ) {
        oData[len - 1] = '\0';
        if( 0 == strlen(oData) ) { *oOnlyEnter = true; }
    }
    else { (void)dropOverBuffer(); }  // バッファサイズを超えた入力の読み捨て
    com_printfLogOnly( "%s\n", oData );
    return true;
}

static char gPromptBuff[COM_TEXTBUF_SIZE];

static size_t inputProc(
        char *oData, size_t iSize, const com_valFunc_t *iVal,
        const com_actFlag_t *iFlag, const char *iFuncName,
        const char *iFormat, va_list iAp )
{
    if( !oData ) {COM_PRMNGF(iFuncName, 0);}
    if( !iFlag ) { iFlag = &(com_actFlag_t){ false, false }; }
    if( iFormat ) {vsnprintf( gPromptBuff, sizeof(gPromptBuff), iFormat, iAp );}
    char* prompt = iFormat ? gPromptBuff : NULL;
    BOOL onlyEnter = false;
    while(1) {
        if( iFlag->clear ) { com_clear(); }
        if( prompt ) { com_printf( "%s", prompt ); }
        if( !inputString( oData, iSize, &onlyEnter ) ) { continue; }
        if( onlyEnter ) { if( iFlag->enterSkip ) {break;} else {continue;} }
        // データ入力に問題がなければループを抜けて返す
        if( !iVal ) { break; }
        if( (iVal->func)( oData, iVal->cond ) ) { break; }
    }
    if( onlyEnter ) { return 0; }
    return strlen( oData );
}
        
// inputProc()を呼び、その結果を result に入れるマクロ
#define INPUT( DATA, SIZE, VAL, FLAG ) \
    size_t result = 0; \
    do { \
        va_list ap; \
        va_start( ap, iFormat ); \
        result = inputProc( DATA, SIZE, VAL, FLAG, __func__, iFormat, ap ); \
        va_end( ap ); \
    } while(0)

size_t com_inputVar(
        char *oData, size_t iSize, const com_valFunc_t *iVal,
        const com_actFlag_t *iFlag, const char *iFormat, ... )
{
    if( !oData ) {COM_PRMNG(0);}
    INPUT( oData, iSize, iVal, iFlag );
    return result;
}

// キー入力用共通バッファ
static char gKeyBuff[COM_TEXTBUF_SIZE];

size_t com_input(
        char **oData, const com_valFunc_t *iVal,
        const com_actFlag_t *iFlag, const char *iFormat, ... )
{
    if( !oData) {COM_PRMNG(0);}
    COM_CLEAR_BUF( gKeyBuff );
    *oData = gKeyBuff;
    INPUT( gKeyBuff, sizeof(gKeyBuff), iVal, iFlag );
    return result;
}

size_t com_inputMutliLine( char *oData, size_t iSize, const char *iFormat, ... )
{
    if( !oData ) {COM_PRMNG(0);}
    COM_SET_FORMAT( gPromptBuff );
    if( iFormat ) { com_printf( gPromptBuff ); }
    memset( oData, 0, iSize );
    clearerr( stdin );
    size_t result = fread( oData, 1, iSize, stdin );
    com_printfLogOnly( "%s", oData );
    if( ferror( stdin ) ) {
        com_error( COM_ERR_READTXTNG, "fail to read text" );
        return 0;
    }
    return result;
}

#define INPUTWRAP( DEFAULT, FUNC, FLAGS ) \
    char tmp[9] = {0}; \
    do { \
        const char defaultLabel[] = DEFAULT; \
        if( !iFormat ) { iFormat = defaultLabel; } \
        com_valFunc_t cond = { .func = (FUNC) }; \
        INPUT( tmp, sizeof(tmp), &cond, (FLAGS) ); \
        COM_UNUSED( result ); \
    } while(0)

BOOL com_askYesNo( const com_actFlag_t *iFlag, const char *iFormat, ... )
{
    INPUTWRAP( "(Yes/No)", com_valYesNo, iFlag );
    return com_isYes( tmp );
}




// データパッケージ関連 ------------------------------------------------------

#define ARCHLEN  COM_LINEBUF_SIZE
#define EXTZIP   ".zip"

static BOOL existPackFile( com_packInf_t *ioInf, char *oArcName )
{
    if( com_checkExistFile( ioInf->filename ) ) {return false;}
    if( ioInf->useZip ) {
        char* tmp = ioInf->archive ? ioInf->archive : ioInf->filename;
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
    if( !ioInf->useZip ) { return true; }
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
    char archive[ARCHLEN] = {0};
    BOOL exist = existPackFile( ioInf, archive );
    if( ioInf->writeFile ) {
        if( exist && iConfirm ) {
            if( !com_askYesNo( false, iConfirm ) ) { return false; }
            if( strlen(archive) ) { remove( archive ); }
        } // 確認しない(iConfirm==NULL)時は無条件で上書きする。
        ioInf->fp = com_fopen( ioInf->filename, "wb" );
    }
    else {
        if( !exist ) {
            com_error( COM_ERR_FILEDIRNG, "no data file" );
            return false;
        }
        if( !extractPack( ioInf, archive ) ) { return false; }
        ioInf->fp = com_fopen( ioInf->filename, "rb" );
    }
    return (0 != ioInf->fp);
}

BOOL com_finishPack( com_packInf_t *ioInf, BOOL iFinal )
{
    if( !ioInf ) {COM_PRMNG(false);}
    BOOL result = true;
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
        if( ioInf->writeFile || ioInf->useZip ) { remove( ioInf->filename ); }
    }
    return result;
}

enum { BINSIZE = 2 };

static BOOL writeDataText( com_packInf_t *ioInf, void *iAddr, size_t iSize )
{
    for( size_t i = 0;  i < iSize;  i++ ) {
        uchar* tmp = iAddr;
        if( BINSIZE != fprintf( ioInf->fp, "%02x", tmp[i] ) ) { return false; }
    }
    return true;
}

static BOOL writeDataBin( com_packInf_t *ioInf, void *iAddr, size_t iSize )
{
    size_t result = fwrite( iAddr, 1, iSize, ioInf->fp );
    if( result < iSize ) { return false; }
    return true;
}

static BOOL writeDataFile( com_packInf_t *ioInf, void *iAddr, size_t iSize )
{
    if( ioInf->byText ) { return writeDataText( ioInf, iAddr, iSize ); }
    return writeDataBin( ioInf, iAddr, iSize );
}

static BOOL writeDataSize( com_packInf_t *ioInf, com_packElm_t *iElm )
{
    if( iElm->size > 0xFFFF ) { return false; }
    ushort dataSize = iElm->size & 0xFFFF;
    return writeDataFile( ioInf, &dataSize, sizeof(dataSize) );
}

static BOOL writePackData( com_packInf_t *ioInf, com_packElm_t *iElm )
{
    if( iElm->varSize ) {if( !writeDataSize( ioInf, iElm ) ) { return false; }}
    return writeDataFile( ioInf, iElm->data, iElm->size );
}

BOOL com_writePack( com_packInf_t *ioInf, com_packElm_t *iElm, long iCount )
{
    if( !ioInf || !iElm || !iCount ) {COM_PRMNG(false);}
    if( !ioInf->writeFile ) {COM_PRMNG(false);}
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
    if( !ioInf || !iAddr ) {COM_PRMNG(false);}
    com_packElm_t tmp = { iAddr, iSize, iVarSize };
    return com_writePack( ioInf, &tmp, 1 );
}


static BOOL readPackText( com_packInf_t *ioInf, com_packElm_t *ioElm )
{
    uchar* target = ioElm->data;
    for( size_t i = 0;  i < ioElm->size;  i++ ) {
        char oct[BINSIZE + 1] = {0};
        if( !fgets( oct, sizeof(oct), ioInf->fp ) ) { return false; }
        target[i] = (uchar)com_strtoul( oct, 16, false );
    }
    return true;
}

static BOOL readPackBin( com_packInf_t *ioInf, com_packElm_t *ioElm )
{
    uchar* target = ioElm->data;
    if( ioElm->size != fread( target, 1, ioElm->size, ioInf->fp ) ) {
        return false;
    }
    return true;
}

static BOOL readDataSize( com_packInf_t *ioInf, com_packElm_t *ioElm )
{
    ushort len = 0;
    size_t size = sizeof(len);
    if( !com_readPackDirect( ioInf, &len, &size, false ) ) { return false; }
    ioElm->size = len;
    return true;
}

static BOOL allocMemory( com_packInf_t *ioInf, com_packElm_t *ioElm )
{
    if( ioElm->data ) { return true; }
    ioElm->data = com_malloc( ioElm->size, "allocate data from %s",
                              ioInf->filename );
    return (ioElm->data != NULL);
}

static BOOL readPackData( com_packInf_t *ioInf, com_packElm_t *ioElm )
{
    if( ioElm->varSize ) {if( !readDataSize( ioInf, ioElm ) ) { return false; }}
    if( !allocMemory( ioInf, ioElm ) ) { return false; }
    if( ioInf->byText ) { return readPackText( ioInf, ioElm ); }
    return readPackBin( ioInf, ioElm );

}

BOOL com_readPack( com_packInf_t *ioInf, com_packElm_t *iElm, long iCount )
{
    if( !ioInf || !iElm || !iCount ) {COM_PRMNG(false);}
    if( ioInf->writeFile ) {COM_PRMNG(false);}
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

void *com_readPackDirect(
        com_packInf_t *ioInf, void *iAddr, size_t *ioSize, BOOL iVarSize )
{
    if( !ioInf ) {COM_PRMNG(false);}
    com_packElm_t tmp = { iAddr, *ioSize, iVarSize };
    if( !com_readPack( ioInf, &tmp, 1 ) ) { return NULL; }
    *ioSize = tmp.size;
    return tmp.data;
}



// 初期化/終了処理 ///////////////////////////////////////////////////////////

static com_dbgErrName_t gErrorNameExtra[] = {
    { COM_ERR_READTXTNG,   "COM_ERR_READTXTNG" },
    { COM_ERR_SIGNALING,   "COM_ERR_SIGNALING" },
    { COM_ERR_END,         "" }  // 最後は必ずこれで
};

static void finalizeExtra( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

void com_initializeExtra( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    atexit( finalizeExtra );
    com_registerErrorCode( gErrorNameExtra );
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

