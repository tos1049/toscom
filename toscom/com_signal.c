/*
 *****************************************************************************
 *
 * 共通部シグナル機能     by TOS
 *
 *   外部公開I/Fの使用方法については com_signal.h を参照。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"
#include "com_signal.h"


// 信号データ共通処理 ////////////////////////////////////////////////////////

void com_initSigBin( com_sigBin_t *oTarget )
{
    if( !oTarget ) {COM_PRMNG();}
    *oTarget = (com_sigBin_t){ .top = NULL };
}

void com_freeSigBin( com_sigBin_t *oTarget )
{
    if( !oTarget ) {return;}    // エラーにはしない
    com_skipMemInfo( true );
    com_free( oTarget->top );
    com_initSigBin( oTarget );  // データのクリア
    com_skipMemInfo( false );
}

BOOL com_copySigBin( com_sigBin_t *oTarget, const com_sigBin_t *iSource )
{
    if( !oTarget || !iSource ) {COM_PRMNG(false);}
    com_off  copySize = iSource->len;
    if( !oTarget->top ) {
        oTarget->top =
            com_malloc( copySize, "copy sigBin(%02x...)", *iSource->top );
        if( !oTarget->top ) {return false;}
    }
    else {if( oTarget->len < copySize ) {copySize = oTarget->len;}}
    memcpy( oTarget->top, iSource->top, copySize );
    oTarget->len = copySize;
    oTarget->ptype = iSource->ptype;
    return true;
}

void com_initSigFrg( com_sigFrg_t *oTarget )
{
    if( !oTarget ) {COM_PRMNG();}
    *oTarget = (com_sigFrg_t){ .ext = NULL, .inf = NULL };
}

void com_freeSigFrg( com_sigFrg_t *oTarget )
{
    if( !oTarget ) {return;}    // エラーにはしない
    com_skipMemInfo( true );
    com_free( oTarget->ext );
    for( long i = 0;  i < oTarget->cnt;  i++ ) {
        com_freeSigBin( &(oTarget->inf[i].bin) );
    }
    com_free( oTarget->inf );
    com_initSigFrg( oTarget );  // データをクリア
    com_skipMemInfo( false );
}

void com_initSigPrm( com_sigPrm_t *oTarget )
{
    if( !oTarget ) {COM_PRMNG();}
    *oTarget = (com_sigPrm_t){ .list = NULL, .spec = NULL };
}

void com_freeSigPrm( com_sigPrm_t *oTarget )
{
    if( !oTarget ) {return;}    // エラーにはしない
    com_skipMemInfo( true );
    com_free( oTarget->list );
    com_free( oTarget->spec );
    com_initSigPrm( oTarget );  // データをクリア
    com_skipMemInfo( false );
}

void com_initSigStk( com_sigStk_t *oTarget )
{
    if( !oTarget ) {COM_PRMNG();}
    *oTarget = (com_sigStk_t){ .stack = NULL };
}

void com_freeSigStk( com_sigStk_t *oTarget, BOOL iBin )
{
    if( !oTarget ) {return;}    // エラーにはしない
    com_skipMemInfo( true );
    if( oTarget->cnt != COM_SIG_STATIC ) {
        for( long i = 0;  i < oTarget->cnt;  i++ ) {
            com_freeSigInf( &(oTarget->stack[i]), iBin );
        }
        com_free( oTarget->stack );
    }
    com_initSigStk( oTarget );  // データをクリア
    com_skipMemInfo( false );
}

void com_setStaticStk( com_sigStk_t *oTarget, com_sigInf_t *iSource )
{
    if( !oTarget || !iSource ) {COM_PRMNG();}
    *oTarget = (com_sigStk_t){ .cnt = COM_SIG_STATIC, .stack = iSource };
}

void com_initSigInf( com_sigInf_t *oTarget, com_sigInf_t *iOrg )
{
    if( !oTarget ) {COM_PRMNG();}
    com_initSigBin( &oTarget->sig );
    oTarget->isFragment = false;
    com_initSigBin( &oTarget->ras );
    oTarget->order = iOrg ? iOrg->order : true;
    com_initSigStk( &oTarget->multi );
    com_initSigPrm( &oTarget->prm );
    oTarget->ext = NULL;
    com_initSigStk( &oTarget->next );
    oTarget->prev = iOrg;
}

void com_makeSigInf( com_sigInf_t *oTarget, void *iData, com_off iSize )
{
    if( !oTarget ) {COM_PRMNG();}
    com_initSigInf( oTarget, NULL );
    oTarget->sig = (com_sigBin_t){ iData, iSize, COM_SIG_UNKNOWN };
}

void com_freeSigInf( com_sigInf_t *oTarget, BOOL iBin )
{
    if( !oTarget ) {return;}    // エラーにはしない
    com_skipMemInfo( true );
    if( iBin ) {
        com_freeSigBin( &oTarget->sig );
        com_freeSigBin( &oTarget->ras );
        oTarget->isFragment = false;
    }
    else {
        if( oTarget->ras.top && !oTarget->ras.ptype ) {
            com_freeSigBin( &oTarget->ras );
        }
        // .ptype == 0 なら、その場でセグメント結合したデータ
        // .ptype != 0 なら、ログの Reassembledから予め読んだ結合データ
        // 後者は oCapInf開放時(iBin==true)にメモリ解放するので、それまで放置
    }
    com_freeSigStk( &oTarget->multi, false );
    com_freeSigPrm( &oTarget->prm );
    com_freeSigInfExt( oTarget );
    oTarget->order = true;
    com_freeSigStk( &oTarget->next, false );
    com_skipMemInfo( false );
}

void com_freeSigInfExt( com_sigInf_t *oTarget )
{
    if( !oTarget ) {return;}     // エラーにはしない
    if( oTarget->sig.ptype > COM_SIG_LINK_LAYER ) {
        com_freeSig_t  func = com_searchSigFreer( oTarget->sig.ptype );
        if( func ) {func( oTarget ); return; }
    }
    com_free( oTarget->ext );
}

static BOOL setSigInf(
        com_sigInf_t *oTarget, com_sigInf_t *oStaticNext,
        const uchar *iSignal, size_t iSize, BOOL iOrder )
{
    com_sigBin_t  tmp = { (com_bin*)iSignal, iSize, 0 };
    if( !com_copySigBin( &oTarget->sig, &tmp ) ) {return false;}
    oTarget->order = iOrder;
    if( oStaticNext ) {com_setStaticStk( &oTarget->next, oStaticNext );}
    return true;
}

BOOL com_setSigInf(
        com_sigInf_t *oTarget, com_sigInf_t *oStaticNext,
        const uchar *iSignal, size_t iSize, BOOL iOrder )
{
    if( !oTarget || !iSignal ) {COM_PRMNG(false);}
    com_initSigInf( oTarget, NULL );
    com_skipMemInfo( true );
    BOOL  result = setSigInf( oTarget, oStaticNext, iSignal, iSize, iOrder );
    com_skipMemInfo( false );
    if( !result ) {com_freeSigInf( oTarget, true );}
    return result;
}

BOOL com_addSigInf( com_sigBin_t *oTarget, const uchar *iSignal, size_t iSize )
{
    if( !oTarget || iSignal ) {COM_PRMNG(false);}
    com_skipMemInfo( true );
    com_bin*  tmp = com_realloc( oTarget->top, oTarget->len + iSize, __func__ );
    com_skipMemInfo( false );
    if( !tmp ) {return false;}
    oTarget->top = tmp;
    memcpy( oTarget->top + oTarget->len, iSignal, iSize );
    oTarget->len += iSize;
    return true;
}



// プロトコル情報登録 ////////////////////////////////////////////////////////

static pthread_mutex_t  gMutexAnalyzer = PTHREAD_MUTEX_INITIALIZER;
static com_sortTable_t  gAnalyzeList;

static void registerPort(
        com_analyzeFuncList_t *iList, COM_PRTCLTYPE_t iNumSys )
{
    com_sigPrtclType_t  tmp[] = {
        { {iList->port}, iList->type },
        { {COM_PRTCLTYPE_END}, COM_SIG_CONTINUE }
    };
    com_setPrtclType( iNumSys, tmp );
}

BOOL com_registerAnalyzer(
        com_analyzeFuncList_t *iList, COM_PRTCLTYPE_t iNumSys )
{
    if( !iList ) {COM_PRMNG(false);}
    com_mutexLock( &gMutexAnalyzer, __func__ );
    for( ; iList->type != COM_SIG_END;  iList++ ) {
        BOOL  collision = false;
        BOOL  result = com_addSortTableByKey( &gAnalyzeList, iList->type,
                                        iList, sizeof(*iList), &collision );
        if( !result ) {com_exit( COM_ERR_NOMEMORY );}
        if( collision ) {
            com_printf( "protocol type(%ld) already exist\n", iList->type );
        }
        if( iList->port ) {registerPort( iList, iNumSys );}
    }
    com_mutexUnlock( &gMutexAnalyzer, __func__ );
    return true;
}

static com_analyzeFuncList_t *searchAnalyzeList( long iKey )
{
    com_sort_t**  result = NULL;
    if( !com_searchSortTableByKey( &gAnalyzeList, iKey, &result ) ) {
        //com_error( COM_ERR_NOSUPPORT,
        //           "not supported protocol(%ld)", iKey );
        return NULL;
    }
    return (com_analyzeFuncList_t*)(result[0]->data);
}

char *com_searchSigProtocol( long iType )
{
    com_analyzeFuncList_t*  anlz = searchAnalyzeList( iType );
    if( !anlz ) {return NULL;}
    return anlz->label;
}

com_analyzeSig_t com_searchSigAnalyzer( long iType )
{
    com_analyzeFuncList_t*  anlz = searchAnalyzeList( iType );
    if( !anlz ) {return NULL;}
    return anlz->func;
}

com_decodeSig_t com_searchSigDecoder( long iType )
{
    com_analyzeFuncList_t*  anlz = searchAnalyzeList( iType );
    if( !anlz ) {return NULL;}
    return anlz->decoFunc;
}

com_freeSig_t com_searchSigFreer( long iType )
{
    com_analyzeFuncList_t*  anlz = searchAnalyzeList( iType );
    if( !anlz ) {return NULL;}
    return anlz->freeFunc;
}

long com_searchPrtclByLabel( const char *iLabel )
{
    if( !iLabel ) {COM_PRMNG(COM_SIG_UNKNOWN);}
    long  result = COM_SIG_UNKNOWN;
    com_mutexLock( &gMutexAnalyzer, __func__ );
    for( long i = 0;  i < gAnalyzeList.count;  i++ ) {
        com_analyzeFuncList_t*  tmp = gAnalyzeList.table[i].data;
        if( com_compareString( iLabel, tmp->label, 0, true ) ) {
            result = tmp->type;
            break;
        }
    }
    com_mutexUnlock( &gMutexAnalyzer, __func__ );
    return result;
}

long com_showAvailProtocols( long **oList )
{
    long  cnt = gAnalyzeList.count;
    COM_SET_IF_EXIST(oList,com_malloc(sizeof(long)*cnt,"show avail protocols"));
    for( long i = 0;  i < cnt;  i++ ) {
        com_sort_t*  tmp = &(gAnalyzeList.table[i]);
        if( !oList ) {com_printf( "%s ", com_searchSigProtocol( tmp->key ) );}
        else {if( *oList ) {(*oList)[i] = tmp->key;}}
    }
    if( !oList ) {com_printLf();}
    return cnt;
}



// プロトコル判定処理 ////////////////////////////////////////////////////////

typedef struct {
    long  base;
    long  def;
    long  initEnd;
    long  cnt;
    com_sigPrtclType_t*  list;
} com_mngPrtclType_t;

static long  gPrtclTypeCnt = 0;
static com_mngPrtclType_t*  gPrtclTypeInf = NULL;

static com_mngPrtclType_t *getPrtclTypeInf( long iBase, BOOL iMakeNew )
{
    for( long i = 0;  i < gPrtclTypeCnt;  i++ ) {
        if( gPrtclTypeInf[i].base == iBase ) {return &(gPrtclTypeInf[i]);}
    }
    if( !iMakeNew ) {return NULL;}
    com_mngPrtclType_t*  tmp =
        com_reallocAddr( &gPrtclTypeInf, sizeof(*gPrtclTypeInf),
                         COM_TABLEEND, &gPrtclTypeCnt, 1,
                         "add protocol type inf" );
    if( !tmp ) {com_exit( COM_ERR_NOMEMORY );}
    tmp->base = iBase;
    return tmp;
}

void com_setPrtclType( COM_PRTCLTYPE_t iBase, com_sigPrtclType_t *iList )
{
    if( iBase == COM_NOT_USE ) {return;}
    if( !iList ) {COM_PRMNG();}
    com_skipMemInfo( true );
    com_mngPrtclType_t*  mngInf = getPrtclTypeInf( iBase, true );
    for( ;  iList->target.type != COM_PRTCLTYPE_END;  iList++ ) {
        com_sigPrtclType_t*  newList =
            com_reallocAddr( &(mngInf->list), sizeof(*(mngInf->list)),
                    COM_TABLEEND, &(mngInf->cnt), 1, "add protocol type" );
        if( !newList ) {com_exit( COM_ERR_NOMEMORY );}
        *newList = *iList;
    }
    // 終端データの codeを、その種別のデフォルト値として設定する
    if( !mngInf->initEnd ) {
        mngInf->def = iList->code;
        mngInf->initEnd = true;
    }
    com_skipMemInfo( false );
}

long com_getPrtclType( COM_PRTCLTYPE_t iBase, long iType )
{
    if( iBase == COM_NOT_USE ) {return 0;}
    com_mngPrtclType_t*  mngInf = getPrtclTypeInf( iBase, false );
    if( !mngInf ) {COM_PRMNG(COM_SIG_UNKNOWN);}
    for( long i = 0;  i < mngInf->cnt;  i++ ) {
        com_sigPrtclType_t*  tmp = &(mngInf->list[i]);
        if( tmp->target.type == iType ) {return tmp->code;}
    }
    return mngInf->def;
}

long com_getPrtclLabel( COM_PRTCLTYPE_t iBase, char *iLabel )
{
    if( iBase == COM_NOT_USE ) {return 0;}
    com_mngPrtclType_t*  mngInf = getPrtclTypeInf( iBase, false );
    if( !mngInf ) {COM_PRMNG(COM_SIG_UNKNOWN);}
    for( long i = 0;  i < mngInf->cnt;  i++ ) {
        char*  target = mngInf->list[i].target.label;
        if( com_compareString( iLabel, target, strlen(target), true ) ) {
            return mngInf->list[i].code;
        }
    }
    return mngInf->def;
}

void com_setBoolTable( COM_PRTCLTYPE_t iBase, long *iTypes, long iTypeCnt )
{
    if( !iTypes ) {COM_PRMNG();}
    com_skipMemInfo( true );
    com_sigPrtclType_t*  list =
        com_malloc( sizeof(com_sigPrtclType_t) * (iTypeCnt + 1),
                    "create list data (cnt=%ld)", iTypeCnt );
    if( !list ) {com_exit( COM_ERR_NOMEMORY );}
    long  i = 0;
    for( ;  i < iTypeCnt;  i++ ) {
        list[i] = (com_sigPrtclType_t){ {iTypes[i]}, true };
    }
    list[i] = (com_sigPrtclType_t){ {COM_PRTCLTYPE_END}, false };
    com_setPrtclType( iBase, list );
    com_free( list );
    com_skipMemInfo( false );
}

void com_freePrtclType( void )
{
    for( long i = 0;  i < gPrtclTypeCnt;  i++ ) {
        com_free( gPrtclTypeInf[i].list );
    }
    gPrtclTypeCnt = 0;
    com_free( gPrtclTypeInf );
}



// 信号データ取得・生成I/F ///////////////////////////////////////////////////

void com_initCapInf( com_capInf_t *oTarget )
{
    if( !oTarget ) {COM_PRMNG();}
    *oTarget = (com_capInf_t){ .fileName = NULL, .fp = NULL };
    com_initSigInf( &oTarget->head, NULL );
    com_initSigStk( &oTarget->ifs );
    com_initSigInf( &oTarget->signal, NULL );
}

void com_freeCapInf( com_capInf_t *oTarget )
{
    if( !oTarget ) {return;}    // エラーにはしない
    com_skipMemInfo( true );
    com_free( oTarget->fileName );
    com_fclose( oTarget->fp );
    com_freeSigInf( &oTarget->head, true );
    com_freeSigStk( &oTarget->ifs, true );
    com_freeSigInf( &oTarget->signal, true );
    com_skipMemInfo( false );
}

static BOOL returnFalse(
        com_capInf_t *oCapInf, COM_CAPERR_t iCause,
        long iErrCode, const char *iErrMsg, COM_FILEPRM )
{
    if( iErrMsg ) {com_errorFunc( iErrCode, true, COM_FILEVAR, iErrMsg );}
    oCapInf->cause = iCause;
    return false;
}

#define CAUSEIS( CAUSE, ERRCODE, ERRMSG ) \
    return returnFalse( oCapInf, (CAUSE), (ERRCODE), ERRMSG, COM_FILELOC )

static BOOL openCapture( const char *iPath, com_capInf_t *oCapInf )
{
    if( !(oCapInf->fileName = com_strdup( iPath, NULL )) ) {
        CAUSEIS( COM_CAPERR_COPYNAME,
                 COM_ERR_ANALYZENG, "fail to store capture file name" );
    }
    if( !(oCapInf->fp = com_fopen( iPath, "r" )) ) {
        CAUSEIS( COM_CAPERR_OPENFILE,
                 COM_ERR_ANALYZENG, "fail to open capture file" );
    }
    return true;
}

static com_bin  gCapBuf[COM_DATABUF_SIZE];

// oBufが NULLの場合は バッファリングせず読み飛ばす処理になる
static BOOL readCapture( com_capInf_t *oCapInf, com_bin *oBuf, com_off iSize )
{
    com_off  count = 0;
    int  oct;
    if( oBuf ) {memset( oBuf, 0, iSize );}
    errno = 0;
    while( EOF != (oct = fgetc( oCapInf->fp )) ) {
        if( oBuf ) {oBuf[count++] = (com_bin)oct;}
        if( count == iSize ) {return true;}
    }
    // ファイルの末尾まで読んだ場合はエラー出力はしない
    if( !errno ) {CAUSEIS( COM_CAPERR_NOMOREDATA, COM_NO_ERROR, NULL );}
    CAUSEIS( COM_CAPERR_NOMOREDATA, COM_ERR_ILLSIZE, "fail to read file" );
}

static ulong readValue( com_capInf_t *oCapInf, com_bin **oBuf, com_off iSize )
{
    static com_bin  buf[8];
    memset( buf, 0, sizeof(buf) );
    *oBuf = NULL;
    if( iSize > 8 ) {COM_PRMNG(0);}
    if( !readCapture( oCapInf, buf, iSize ) ) {return 0;}
    ulong*  result = (ulong*)buf;
    *oBuf = buf;
    return *result;
}

#define SETSIGNAL( SIG, BUF, SIZE ) \
    setSignalData( __func__, (SIG), (BUF), (SIZE) )

static BOOL setSignalData(
        const char *iFunc, com_sigInf_t *oSig, com_bin *iBuf, com_off iSize )
{
    oSig->sig.top = 
        com_reallocf( oSig->sig.top, oSig->sig.len + iSize, iFunc );
    if( !(oSig->sig.top) ) {return false;}
    memcpy( oSig->sig.top + oSig->sig.len, iBuf, iSize );
    oSig->sig.len += iSize;
    return true;
}

static BOOL addSignalData( com_capInf_t *oCapInf, com_off iSize )
{
    com_sigBin_t*  sig = &oCapInf->signal.sig;
    sig->top = com_reallocf( sig->top, sig->len + iSize, __func__ );
    if( !(sig->top) ) {return false;}
    com_off  cnt = 0;
    for( ;  cnt < iSize;  cnt++ ) {
        com_bin  oct;
        if( !readCapture( oCapInf, &oct, 1 ) ) {break;}
        sig->top[sig->len + cnt] = oct;
    }
    sig->len += cnt;
    return true;
}

static long getHeadType( ulong iHeadValue )
{
    if( iHeadValue == COM_CAP_FILE_SHB  ) {return COM_SIG_PCAPNG;}
    if( iHeadValue == COM_CAP_FILE_PCAP ) {return COM_SIG_LIBPCAP;}
    return COM_SIG_UNKNOWN;
}

#define FILE_HEAD_SIZE  COM_32BIT_SIZE

static BOOL getFileHead( com_capInf_t *oCapInf )
{
    com_sigInf_t*  tmp = &(oCapInf->head);
    com_bin*  buf;
    ulong  headValue = readValue( oCapInf, &buf, FILE_HEAD_SIZE );
    if( !buf ) {return false;}
    SETSIGNAL( tmp, buf, FILE_HEAD_SIZE );
    tmp->sig.ptype = getHeadType( headValue );
    return ( tmp->sig.ptype != COM_SIG_UNKNOWN );
}

#define BLOCK_LENGTH_SIZE   COM_32BIT_SIZE
#define SIZE_BEFORE_BLOCK  (COM_32BIT_SIZE * 2)

// pcapng形式のブロックタイプ(32bit)まで読んだ状態にしておくと
// その後のブロック内容を読み込んで取得する
static BOOL getBlock( com_capInf_t *oCapInf, com_sigInf_t *oTarget )
{
    com_bin*  buf;
    ulong  blockLen = readValue( oCapInf, &buf, BLOCK_LENGTH_SIZE );
    if( !buf ) {return false;}
    if( !SETSIGNAL( oTarget, buf, BLOCK_LENGTH_SIZE ) ) {return false;}
    blockLen -= SIZE_BEFORE_BLOCK;
    if( !readCapture( oCapInf, gCapBuf, blockLen ) ) {return false;}
    if( !SETSIGNAL( oTarget, gCapBuf, blockLen ) ) {return false;}
    return true;
}

static com_sigInf_t *addSigInf( com_sigStk_t *ioTarget, char *iLabel )
{
    return com_reallocAddr( &(ioTarget->stack), sizeof(com_sigInf_t),
                            COM_TABLEEND, &(ioTarget->cnt), 1,
                            "%s[%ld]", iLabel, ioTarget->cnt );
}

static com_sigPrtclType_t  gFileNext[] = {
    { {COM_CAP_ETHER},     COM_SIG_ETHER2 },
    { {COM_CAP_SLL},       COM_SIG_SLL },
    { {COM_PRTCLTYPE_END}, COM_SIG_UNKNOWN }
};

static long getIdbLinkType( com_bin *iIdb )
{
    COM_CAST_HEAD( com_pcapngIdb_t, idb, iIdb );
    return com_getPrtclType( COM_FILENEXT, idb->linkType );
}

#define BLOCK_TYPE_SIZE  COM_32BIT_SIZE

static uint checkBlockHead(
        com_capInf_t *oCapInf, com_bin **oBuf, uint *iExpect, BOOL *oReturn )
{
    fpos_t  back;
    fgetpos( oCapInf->fp, &back );
    uint  blockType = readValue( oCapInf, oBuf, BLOCK_TYPE_SIZE );
    if( !(*oBuf) ) {*oReturn = false;  return 0;}
    *oReturn = true;
    for( long i = 0;  iExpect[i];  i++ ) {
        if( blockType == iExpect[i] ) {return (i + 1);}
    }
    fsetpos( oCapInf->fp, &back );
    return 0;
}

static BOOL getBlockData(
        com_capInf_t *oCapInf, com_sigInf_t *oTarget, com_bin *iType )
{
    if( !SETSIGNAL( oTarget, iType, BLOCK_TYPE_SIZE ) ) {return false;}
    if( !getBlock( oCapInf, oTarget ) ) {return false;}
    return true;
}

static BOOL checkMagicNumber( com_bin *iMagNum, com_bin *iSig )
{
    uint*  uintVal = (uint*)iMagNum;
    uint*  orgVal = (uint*)iSig;
    DEBUGSIG( "#magic number = %08x\n", *uintVal );
    DEBUGSIG( "#header magic = %08x\n", *orgVal );
    return (*uintVal != *orgVal);
}

static void getPcapngOrder( com_capInf_t *oCapInf )
{
    com_bin  msgNum[] = { 0x1a, 0x2b, 0x3c, 0x4d };
    COM_CAST_HEAD( com_pcapngShb_t, shb, oCapInf->head.sig.top );
    BOOL  nwOrder = checkMagicNumber( msgNum, (com_bin*)(&shb->boMagic) );
    oCapInf->head.order = nwOrder;
}

static BOOL getIDB( com_capInf_t *oCapInf )
{
    com_bin*  typeBuf;
    uint  IDB[] = { COM_CAP_FILE_IDB, 0 };
    BOOL  result;
    while(1) {
        if( !checkBlockHead( oCapInf,&typeBuf,IDB,&result ) ) {return result;}
        com_sigInf_t*  tmpIdb = addSigInf( &oCapInf->ifs, "ifs" );
        if( !tmpIdb ) {
            CAUSEIS( COM_CAPERR_GETLINK, COM_ERR_INCORRECT, "IDB not found" );
        }
        if( !getBlockData( oCapInf, tmpIdb, typeBuf ) ) {
            CAUSEIS( COM_CAPERR_GETLINK, COM_ERR_INCORRECT, "IDB data NG" );
        }
        tmpIdb->sig.ptype = getIdbLinkType( tmpIdb->sig.top );
        if( !tmpIdb->sig.ptype ) {
            CAUSEIS(COM_CAPERR_GETLINK, COM_ERR_INCORRECT, "IDB link type NG");
        }
    }
    return true;
}

#define MAGIC_NUMBER_SIZE  COM_32BIT_SIZE

static BOOL getLibpcapHead( com_capInf_t *oCapInf )
{
    com_off  restSize = sizeof(com_pcapHead_t) - MAGIC_NUMBER_SIZE;
    if( !readCapture( oCapInf, gCapBuf, restSize ) ) {return false;}
    if( !SETSIGNAL( &oCapInf->head, gCapBuf, restSize ) ) {return false;}
    return true;
}

static void getLibpcapOrder( com_capInf_t *oCapInf )
{
    com_bin  magNum[] = { 0xa1, 0xb2, 0xc3, 0xd4 };
    BOOL  nwOrder = checkMagicNumber( magNum, oCapInf->head.sig.top );
    oCapInf->head.order = nwOrder;
}

static BOOL getLinkType( com_capInf_t *oCapInf )
{
    if( !addSigInf( &oCapInf->ifs, "ifs" ) ) {return false;}
    COM_CAST_HEAD( com_pcapHead_t, libpcap, oCapInf->head.sig.top );
    oCapInf->ifs.stack[0].sig.ptype =
        com_getPrtclType( COM_FILENEXT, libpcap->linktype );
    return true;
}

static void debugHead( com_capInf_t *oCapInf )
{
    if( !com_getDebugSignal() ) {return;}
    com_printf( "#HEAD\n" );
    com_sigInf_t*  tmp = &(oCapInf->head);
    BINDUMP( &tmp->sig );
    com_printf( "# network byte order = %ld\n", tmp->order );
    for( long i = 0;  i < oCapInf->ifs.cnt;  i++ ) {
        com_printf( "#ifs[%ld]\n", i );
        com_sigInf_t*  tmpIdb = oCapInf->ifs.stack;
        if( tmpIdb[i].sig.top ) {BINDUMP( &tmpIdb[i].sig );}
        com_printf( "# network = %ld\n", tmpIdb[i].sig.ptype );
    }
}

#define  GETHEADNG( LABEL ) \
    CAUSEIS( COM_CAPERR_GETHEAD, COM_ERR_INCORRECT, LABEL )

static long getHeadInf( com_capInf_t *oCapInf )
{
    com_sigInf_t*  head = &(oCapInf->head);
    if( !getFileHead( oCapInf ) ) {GETHEADNG( "no file head" );}
    if( head->sig.ptype == COM_SIG_PCAPNG ) {
        if( !getBlock( oCapInf, head ) ) {GETHEADNG( "fail to get block" );}
        if( !getIDB( oCapInf ) ) {return false;}
        getPcapngOrder( oCapInf );
    }
    else {
        if( !getLibpcapHead( oCapInf ) ) {GETHEADNG( "no libpcap head" );}
        if( !getLinkType( oCapInf ) ) {GETHEADNG( "link type NG" );}
        getLibpcapOrder( oCapInf );
    }
    debugHead( oCapInf );
    return true;
}

static BOOL skipBlockData( com_capInf_t *oCapInf )
{
    com_bin*  buf;
    ulong  blockLen = readValue( oCapInf, &buf, BLOCK_LENGTH_SIZE );
    if( !buf ) {return false;}
    blockLen -= SIZE_BEFORE_BLOCK;
    return readCapture( oCapInf, NULL, blockLen );
}

static uint searchPacketBlock( com_capInf_t *oCapInf, com_sigInf_t *oXPB )
{
    uint  XPB[] = { COM_CAP_FILE_EPB, COM_CAP_FILE_SPB,  // データ有のブロック
                    COM_CAP_FILE_NRB, COM_CAP_FILE_ISB,
                    COM_CAP_FILE_CB1, COM_CAP_FILE_CB1 };
    uint  type = 0;
    while(1) {
        com_bin*  typeBuf;
        BOOL  result;
        type = checkBlockHead( oCapInf, &typeBuf, XPB, &result );
        if( !type ) {return 0;}
        type = XPB[type - 1];
        if( type == COM_CAP_FILE_EPB || type == COM_CAP_FILE_SPB ) {
            memset( oXPB, 0, sizeof(*oXPB) );
            if( !getBlockData( oCapInf, oXPB, typeBuf ) ) {return 0;}
            return type;
        }
        // データブロックではない場合、スキップする
        if( !skipBlockData( oCapInf ) ) {return 0;}
    }
    return 0;  // ここには来ないが関数構造的に必要
}

static BOOL getEpbData( com_capInf_t *oCapInf, com_sigInf_t *iEpb )
{
    COM_CAST_HEAD( com_pcapngEpb_t, epb, iEpb->sig.top );
    if( epb->ifID >= (ulong)oCapInf->ifs.cnt ) {return false;}
    if( !SETSIGNAL( &(oCapInf->signal), epb->pktData, epb->orgPktLen ) ) {
        return false;
    }
    oCapInf->signal.sig.ptype = oCapInf->ifs.stack[epb->ifID].sig.ptype;
    return true;
}

static BOOL getSpbData( com_capInf_t *oCapInf, com_sigInf_t *iSpb )
{
    COM_CAST_HEAD( com_pcapngSpb_t, spb, iSpb->sig.top );
    if( !SETSIGNAL( &(oCapInf->signal), spb->pktData, spb->orgPktLen ) ) {
        return false;
    }
    oCapInf->signal.sig.ptype = COM_SIG_UNKNOWN;
    return true;
}

static BOOL getPcapng( com_capInf_t *oCapInf )
{
    com_sigInf_t  xpb;
    uint  type = searchPacketBlock( oCapInf, &xpb );
    if( !type ) {
        if( oCapInf->cause ) {return false;}  // 設定済みなら何もしない
        CAUSEIS( COM_CAPERR_GETSIGNAL, COM_ERR_ANALYZENG, "no data remain" );
    }
    DEBUGSIG( "\n#Block(%u)\n", type );
    BOOL  result = false;
    switch( type ) {
        case COM_CAP_FILE_EPB: result = getEpbData( oCapInf, &xpb );  break;
        case COM_CAP_FILE_SPB: result = getSpbData( oCapInf, &xpb );  break;
        default: return false;
    }
    com_freeSigInf( &xpb, true );
    if( !result ) {
        CAUSEIS( COM_CAPERR_GETSIGNAL, COM_ERR_INCORRECT, "packet block NG" );
    }
    return true;
}

static BOOL getLibpcap( com_capInf_t *oCapInf )
{
    static com_bin  buf[sizeof(com_pcapPkthdr_t)];
    if( !readCapture( oCapInf, buf, sizeof(buf) ) ) {return false;}
    COM_CAST_HEAD( com_pcapPkthdr_t, pkthdr, buf );
    if( !addSignalData( oCapInf, pkthdr->capLen ) ) {
        CAUSEIS( COM_CAPERR_GETSIGNAL, COM_ERR_INCORRECT, "signal data NG" );
    }
    com_sigInf_t*  sig = &oCapInf->signal;
    if( oCapInf->ifs.cnt ) {sig->sig.ptype = oCapInf->ifs.stack[0].sig.ptype;}
    else {sig->sig.ptype = COM_SIG_UNKNOWN;}
    return true;
}

static BOOL getSignal( com_capInf_t *oCapInf )
{
    if( oCapInf->head.sig.ptype == COM_SIG_PCAPNG ) {
        if( !getPcapng( oCapInf ) ) {return false;}
    }
    else {
        if( !getLibpcap( oCapInf ) ) {return false;}
    }
    // バイトオーダーのフラグをヘッダ情報からコピー
    oCapInf->signal.order = oCapInf->head.order;
    return true;
}

static void debugSignal( com_capInf_t *oCapInf )
{
    if( !com_getDebugSignal() ) {return;}
    com_sigInf_t*  sig = &oCapInf->signal;
    if( !(sig->sig.top) ) {return;}
    com_printf( "#signal\n" );
    BINDUMP( &sig->sig );
    com_printf( "# type : %ld\n", sig->sig.ptype );
    com_printf( "# network byte order = %ld\n", sig->order );
    com_printLf();
}

static BOOL readEnd( com_capInf_t *oCapInf, BOOL iResult )
{
    if( !iResult ) {com_freeCapInf( oCapInf );}
    com_skipMemInfo( false );
    return iResult;
}

#define READEND( RESULT ) \
    return readEnd( oCapInf, (RESULT) )

BOOL com_readCapFile( const char *iPath, com_capInf_t *oCapInf )
{
    if( !oCapInf ) {COM_PRMNG(false);}
    com_skipMemInfo( true );
    if( iPath ) {
        com_initCapInf( oCapInf );
        if( !openCapture( iPath, oCapInf ) ) {READEND(false);}
        if( !getHeadInf( oCapInf ) ) {READEND(false);}
    }
    if( !oCapInf->fp ) {
        CAUSEIS( COM_CAPERR_NOMOREDATA, COM_ERR_NOMOREDATA, NULL );
    }
    com_freeSigInf( &(oCapInf->signal), true );
    if( !getSignal( oCapInf ) ) {READEND(false);}
    debugSignal( oCapInf );
    READEND(true);
}


enum {
    MAXHEXCOUNT = 16 * 2,    // 信号データ最大文字数
    LIMITHEXCOUNT = 160,     // 最大走査数
    NOTSEEMAX = INT_MAX      // 下層終端の初期値
};

static long  gVirtualMax = NOTSEEMAX;    // あり得ない値で初期化

static BOOL checkHexCode( char *iText )
{
    long  hexCount = 0;
    for( long i = 0;  i < LIMITHEXCOUNT;  i++ ) {
        char  c = iText[i];
        if( c == ' ' || c == '\t' ) {continue;}
        if( c == '\r' || c == '\n' ) {break;}
        if( i > gVirtualMax ) {break;}  // 最大文字数以降は無視して終了
        if( !isxdigit( (uchar)c ) ) {return false;}
        // 必要な16進数文字を認識したら、その位置を保持
        hexCount++;
        if( hexCount == MAXHEXCOUNT ) {gVirtualMax = i;  break;}
    }
    if( !hexCount || hexCount % 2 ) {return false;}
    return true;
}

#define ADDRLEN  4    // 当分 tcpdumpも tsharkも 4桁(16bit)と思われる

static BOOL isSignalBinary( char **iBuf )
{
    // 先頭トークンが 4桁の 16進数値かチェック (0xは付いていても良い)
    char  addr[8] = {0};
    size_t  addrLen = ADDRLEN + (strncmp( *iBuf, "0x", 2 )) ? 0 : 2;
    (void)com_strncpy( addr, sizeof(addr), *iBuf, addrLen );
    if( !com_valHex( addr, NULL ) ) {return false;}
    *iBuf += addrLen;
    // 数値の後が 空白か : でなければアウト (tcpdumpはアドレスの後が : )
    if( **iBuf != ' ' && **iBuf != ':' ) {return false;}
    *iBuf += 1;
    return checkHexCode( *iBuf );
}

#define CAPLOG_OCT_COUNT  16

static BOOL getBinaryTxt( char *oTarget, char *iBuf )
{
    BOOL    detect   = false;  // 数字群検出中(最初の16進数値から空白まで)
    BOOL    notDigit = false;  // 前の文字が16進数文字列かどうか
    size_t  binTop   = 0;      // 数字群の開始位置(必ず1カラム目とは限らない)
    size_t  unitLen  = 9;      // 数字群の文字数
    size_t  unitOct  = 0;      // 1つの数字群に入っていたオクテット数
    size_t  cnt      = 0;      // 16進数値文字数
    for( size_t i = 0;  iBuf[i];  i++ ) {
        if( isxdigit( (uchar)iBuf[i] ) ) {
            if( detect && notDigit ) {detect = false;  unitOct = cnt / 2;}
            if( !cnt ) {detect = true;  binTop = i;}
            oTarget[cnt++] = iBuf[i];
            if( cnt == CAPLOG_OCT_COUNT * 2 ) {return true;}
        }
        else if( iBuf[i] !=' ' && iBuf[i] != '\t' ) {return false;}
        notDigit = !isxdigit( (uchar)iBuf[i] );
        if( detect ) {unitLen++;}
        if( unitOct ) {  // 16oct分の文字列を操作したら途中でも処理終了
            if( i - binTop == unitLen * (CAPLOG_OCT_COUNT / unitOct) ) {break;}
        }
    }
    return true;
}

static BOOL getBinary( com_capInf_t *oCapInf, char *iBuf )
{
    char  txt[CAPLOG_OCT_COUNT * 2 + 1] = {0};
    if( !getBinaryTxt( txt, iBuf ) ) {return false;}
    uchar  oct[CAPLOG_OCT_COUNT];
    uchar*  buf = oct;  // com_strtooct()にダブルポインタを渡すために用意
    size_t  len = sizeof(oct);
    if( !com_strtooct( txt, &buf, &len ) ) {return false;}
    com_sigBin_t*  target = &(oCapInf->signal.sig);
    if( oCapInf->hasRas ) {target = &(oCapInf->signal.ras);}
    if( !com_addSigInf( target, oct, len ) ) {return false;}
    return true;
}

static BOOL seekCapture( char *iBuf, com_capInf_t *oCapInf, BOOL *ioReading )
{
    char*  top = com_topString( iBuf, false );
    if( !top ) {return true;}  // テキストではないものを読むとあり得る
    if( !isSignalBinary( &top ) ) {return !(*ioReading);}
    if( !(*ioReading) ) {com_freeSigInf( &(oCapInf->signal), true );}
    *ioReading = true;
    if( !getBinary( oCapInf, top ) ) {
        // エラー出力はしない (異常データとは限らないため)
        oCapInf->cause = COM_CAPERR_GETSIGNAL;
        return false;
    }
    return true;
}

#define GETCAPBUF \
    fgets( buf, sizeof(buf), oCapInf->fp )

static BOOL getSignalText( com_capInf_t *oCapInf )
{
    char  buf[COM_LINEBUF_SIZE];
    BOOL  isReading = oCapInf->hasRas;
    com_skipMemInfo( true );
    while( GETCAPBUF ) {if( !seekCapture( buf,oCapInf,&isReading ) ) {break;}}
    com_skipMemInfo( false );
    return isReading;
}

static BOOL getReasmType( char *ioBuf )
{
    char*  ptr = ioBuf;
    for( ; *ptr; ptr++ ) {if( *ptr == ' ' ) {*ptr = '\0';  break;}}
    long  ptype = com_searchPrtclByLabel( ioBuf );
    // 0 (COM_SIG_UNKNOWN) は返さないように配慮
    // ただここで COM_SIG_UNKNOWN にならないように処理を検討すべき
    if( ptype == COM_SIG_UNKNOWN ) {ptype = COM_SIG_END;}
    return ptype;
}

#define REASM  "Reassembled"

static BOOL findReassembled( com_capInf_t *oCapInf )
{
    if( oCapInf->hasRas ) {return false;}
    char  buf[COM_LINEBUF_SIZE] = {0};
    fpos_t  tmpPos;
    if( fgetpos( oCapInf->fp, &tmpPos ) ) {return false;}
    if( !GETCAPBUF ) {return false;}
    if( !com_compareString( buf, REASM, strlen(REASM), false ) ) {
        (void)fsetpos( oCapInf->fp, &tmpPos );
        return false;
    }   // Reassembledの文字列が無い場合は、ファイル読み込み位置を戻しておく
    oCapInf->signal.ras.ptype = getReasmType( buf + strlen(REASM) );
    oCapInf->hasRas = true;
    if( !GETCAPBUF ) {return false;}
    return true;
}

BOOL com_readCapLog( const char *iPath, com_capInf_t *oCapInf, BOOL iOrder )
{
    if( !oCapInf) {COM_PRMNG(false);}
    oCapInf->cause = COM_CAPERR_NOERROR;
    com_skipMemInfo( true );
    if( iPath ) {
        com_initCapInf( oCapInf );
        if( !openCapture( iPath, oCapInf ) ) {READEND(false);}
    }
    if( !getSignalText( oCapInf ) ) {READEND(false);}
    if( findReassembled( oCapInf ) ) {
        if( !getSignalText( oCapInf ) ) {READEND(false);}
    }
    oCapInf->signal.order = iOrder;
    debugSignal( oCapInf );
    oCapInf->hasRas = false;  // 次回検索に備えて、ここでリセット
    READEND( true );
}

static BOOL judgeSeek( BOOL iResult, com_seekCapInf_t *oInf )
{
    if( !iResult ) {
        oInf->isReading = false;
        if( oInf->capInf.cause == COM_CAPERR_NOERROR ) {
            if( findReassembled( &oInf->capInf ) ) {
                oInf->isReading = true;  // もう少し読込を継続
                iResult = true;
            }
            else {
                com_sigInf_t*  sig = &(oInf->capInf.signal);
                sig->order = oInf->order;
                debugSignal( &oInf->capInf );
                if( oInf->notify ) {iResult = (oInf->notify)( sig );}
            }
        }
    }
    return iResult;
}

BOOL com_seekCapLog( com_seekFileResult_t *iInf )
{
    com_seekCapInf_t*  tmp = iInf->userData;
    tmp->capInf.fp = iInf->fp;    // ファイルポインタを臨時で設定
    com_skipMemInfo( true );
    BOOL  result = seekCapture( iInf->line, &tmp->capInf, &tmp->isReading );
    result = judgeSeek( result, tmp );
    tmp->capInf.fp = NULL;    // ファイルポインタクリア(二度クローズ防止)
    if( !result ) {com_freeCapInf( &tmp->capInf );}
    com_skipMemInfo( false );
    return result;
}



// デバッグ関連 //////////////////////////////////////////////////////////////

static BOOL  gDebugSignal = false;

void com_setDebugSignal( BOOL iMode )
{
    gDebugSignal = iMode;
}

BOOL com_getDebugSignal( void )
{
    return gDebugSignal;
}



// 初期化処理 ////////////////////////////////////////////////////////////////

static com_dbgErrName_t  gErrorNameSignal[] = {
    { COM_ERR_NOSUPPORT,    "COM_ERR_NOSUPPORT" },
    { COM_ERR_NOMOREDATA,   "COM_ERR_NOMOREDATA" },
    { COM_ERR_NOTPROTO,     "COM_ERR_NOTPROTO" },
    { COM_ERR_INCORRECT,    "COM_ERR_INCORRECT" },
    { COM_ERR_ILLSIZE,      "COM_ERR_ILLSIZE" },
    { COM_ERR_NODATA,       "COM_ERR_NODATA" },
    { COM_ERR_ANALYZENG,    "COM_ERR_ANALYZENG" },
    { COM_ERR_END,          "" }  // 最後は必ずこれで
};

static void finalizeSignal( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    com_freeSortTable( &gAnalyzeList );
    com_freePrtclType();
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

void com_initializeSignal( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    atexit( finalizeSignal );
    com_registerErrorCode( gErrorNameSignal );
    com_initializeSortTable( &gAnalyzeList, COM_SORT_OVERWRITE );
    com_setPrtclType( COM_FILENEXT, gFileNext );
    com_initializeAnalyzer();
    com_initializeSigPrt1();
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

