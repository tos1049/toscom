/*
 *****************************************************************************
 *
 * 共通部シグナル機能 (解析/デコード共通処理)    by TOS
 *
 *   外部公開I/Fの使用方法については com_signalCom.h を参照。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"
#include "com_signal.h"


// 信号解析起点 //////////////////////////////////////////////////////////////

BOOL com_analyzeSignal( COM_ANALYZER_PRM )
{
    if( COM_SGTYPE == COM_SIG_ALLZERO ) {return true;}
    if( COM_SGTYPE != COM_SIG_UNKNOWN ) {
        com_analyzeSig_t  func = com_searchSigAnalyzer( COM_SGTYPE );
        if( func ) {return func( COM_ANALYZER_VAR );}
        com_decodeData( ioHead, "NO ANALYZER " );
        return false;
    }
    DEBUGSIG( "#<<judge top header>\n" );
    // 不明の場合は、まずリンク層として解析試行
    if( com_analyzeAsLinkLayer( COM_ANALYZER_VAR ) ) {return true;}
    // リンク層として解析できなかったら、ネットワーク層として解析試行
    return com_analyzeAsNetworkLayer( COM_ANALYZER_VAR );
}

static BOOL needAnalyze( long iType )
{
    if( iType == COM_SIG_FRAGMENT ) {return false;}
    if( iType == COM_SIG_CONTINUE ) {return false;}
    if( iType == COM_SIG_END ) {return false;}
    return true;
}

static BOOL analyzeStacks( com_sigStk_t *iStack, BOOL iDecode )
{
    for( long cnt = 0;  cnt < iStack->cnt;  cnt++ ) {
        com_sigInf_t*  target = &(iStack->stack[cnt]);
        long  ptype = target->sig.ptype;
        if( !needAnalyze( ptype ) ) {continue;}
        if( !com_analyzeSignal( target, iDecode ) ) {return false;}
        if( target->next.cnt ) {
            if( !analyzeStacks( &target->next, iDecode ) ) {return false;}
        }
    }
    return true;
}

BOOL com_analyzeSignalToLast( COM_ANALYZER_PRM )
{
    return analyzeStacks( &(com_sigStk_t){ 1, ioHead }, iDecode );
}

static BOOL failRollback(
        com_sigInf_t *ioHead, com_sigInf_t *iOrg, const char *iNotCase )
{
    DEBUGSIG( "%s", iNotCase );
    com_rollbackSignal( ioHead, iOrg );
    return false;
}

#define ROLLBACK  return failRollback( ioHead, &org, iNotCase )

static BOOL analyzeTmpType(
        com_sigInf_t *ioHead, com_analyzeSig_t iAnalyzer,
        BOOL iDecode, com_decodeSig_t iDecoder, const char *iNotCase )
{
    com_sigInf_t  org = *ioHead;
    if( !iAnalyzer( ioHead, false ) ) {ROLLBACK;}
    if( COM_NEXTCNT == 0 ) {ROLLBACK;}
    if( COM_NEXTSTK->sig.ptype == COM_SIG_CONTINUE ) {ROLLBACK;}

    if( iDecode ) {iDecoder( ioHead );}
    return true;
}

BOOL com_analyzeAsLinkLayer( COM_ANALYZER_PRM )
{
    if( analyzeTmpType( ioHead, com_analyzeSll, iDecode, com_decodeSll,
                        "# ... not SLL, maybe Ether2\n" ) )
    {
        return true;
    }
    return analyzeTmpType( ioHead, com_analyzeEth2, iDecode, com_decodeEth2,
                           "# ... not Ether2, maybe Network Layer\n" );
}

static BOOL getIpVersion( com_sigInf_t *ioHead )
{
    if( !COM_SGTOP ) {return false;}
    com_sigBin_t*  sig = &ioHead->sig;
    long  version = *(sig->top) >> 8;
    if( version == COM_CAP_IPV4 ) {sig->ptype = COM_SIG_IPV4;  return true;}
    if( version == COM_CAP_IPV6 ) {sig->ptype = COM_SIG_IPV6;  return true;}
    return false;
}

BOOL com_analyzeAsNetworkLayer( COM_ANALYZER_PRM )
{
    long*  ptype = &COM_SGTYPE;
    if( *ptype != COM_SIG_IPV4 && *ptype != COM_SIG_IPV6 ) {
        if( !getIpVersion( ioHead ) ) {
           DEBUGSIG( "# ... not IP Header, tyy as ARP\n" );
          return com_analyzeArp( COM_ANALYZER_VAR );
        }
    }
    if( *ptype == COM_SIG_IPV4 ) {return com_analyzeIpv4( COM_ANALYZER_VAR );}
    if( *ptype == COM_SIG_IPV6 ) {return com_analyzeIpv6( COM_ANALYZER_VAR );}
    return false;
}

long com_getLayerType( long iType )
{
    if( iType > COM_SIG_TYPEMAX ) {return COM_SIG_UNKNOWN;}
    if( iType > COM_SIG_APP_LAYER ) {return COM_SIG_APP_LAYER;}
    if( iType > COM_SIG_NO7_LAYER ) {return COM_SIG_NO7_LAYER;}
    if( iType > COM_SIG_TRANSPORT_LAYER ) {return COM_SIG_TRANSPORT_LAYER;}
    if( iType > COM_SIG_NETWORK_LAYER ) {return COM_SIG_NETWORK_LAYER;}
    if( iType > COM_SIG_LINK_LAYER ) {return COM_SIG_LINK_LAYER;}
    // ファイルタイプの場合は不明で返す 
    return COM_SIG_UNKNOWN;
}

static inline BOOL isLastHead( com_sigInf_t *iHead )
{
    if( COM_INEXTCNT ) {return false;}
    return true;
}

static BOOL registHead(
        long *ioCnt, com_sigInf_t ***oResult, com_sigInf_t *iHead )
{
    long  cnt = (*ioCnt)++;
    if( oResult ) {
        com_sigInf_t**  inf =
            com_reallocAddr( oResult, sizeof(*oResult), COM_TABLEEND, &cnt, 1,
                             "com_detectProtocol data" );
        if( !inf ) {return false;}
        *inf = iHead;
    }
    return true;
}

static BOOL detectProtocol(
        long *ioCnt, com_sigInf_t ***oResult, com_sigInf_t *iHead,
        long iProtocol )
{
    if( iProtocol == COM_SIG_END ) {
        if( isLastHead( iHead ) ) {
            return registHead( ioCnt, oResult, iHead );
        }
    }
    else if( COM_ISGTYPE == iProtocol ) {
        return registHead( ioCnt, oResult, iHead );
    }
    if( isLastHead( iHead ) ) {return true;}
    for( long i = 0;  i < COM_INEXTCNT;  i++ ) {
        if( !detectProtocol( ioCnt,oResult,&COM_INEXTSTK[i],iProtocol ) ) {
            return false;
        }
    }
    return true;
}

static pthread_mutex_t  gMutexDetectProtocol = PTHREAD_MUTEX_INITIALIZER;

long com_detectProtocol(
        com_sigInf_t ***oResult, com_sigInf_t *iHead, long iProtocol )
{
    if( !iHead ) {COM_PRMNG(0);}
    COM_SET_IF_EXIST( oResult, NULL );
    long  cnt = 0;
    com_mutexLock( &gMutexDetectProtocol, __func__ );
    if( !detectProtocol( &cnt, oResult, iHead, iProtocol ) ) {
        cnt = 0;
        if( oResult ) {if( *oResult ) {com_free( *oResult );}}
    }
    com_mutexUnlock( &gMutexDetectProtocol, __func__ );
    return cnt;
}

com_sigInf_t *com_getProtocol( com_sigInf_t *iHead, long iProtocol )
{
    if( !iHead ) {COM_PRMNG(NULL);}
    com_skipMemInfo( true );
    com_sigInf_t*  result = NULL;
    long  cnt = 0;
    com_sigInf_t**  tmp = NULL;
    com_mutexLock( &gMutexDetectProtocol, __func__ );
    if( detectProtocol( &cnt, &tmp, iHead, iProtocol ) ) {
        if( cnt > 0 ) {result = tmp[0];}
    }
    com_mutexUnlock( &gMutexDetectProtocol, __func__ );
    // tmpが開放されても、tmp[0]の値は iHead内のデータアドレスなので健在
    com_free( tmp );
    com_skipMemInfo( false );
    return result;
}

void com_decodeSignal( COM_DECODER_PRM )
{
    if( !iHead ) {COM_PRMNG();}
    if( COM_ISGTYPE == COM_SIG_ALLZERO ) {
        com_decodeData( iHead, "DATA" );
        return;
    }
    if( COM_ISGTYPE == COM_SIG_CONTINUE ) {
        com_decodeData( iHead, "Unknown protocol" );
        return;
    }
    com_decodeSig_t  func = com_searchSigDecoder( COM_ISGTYPE );
    if( func ) {func( COM_DECODER_VAR );}
    else {com_printf( "# <unknown protocol>\n" );}
}



// 解析開始マクロ用処理 //////////////////////////////////////////////////////

#define PRMNG \
    do {com_prmNgFunc( COM_FILEVAR, NULL ); return false;} while(0)

BOOL com_checkAnalyzerPrm( COM_FILEPRM, com_sigInf_t *ioHead )
{
    if( !ioHead ) {PRMNG;}
    if( !COM_SGTOP ) {PRMNG;}
    if( COM_NEXTCNT == COM_SIG_STATIC && !COM_NEXTSTK ) {PRMNG;}
    return true;
}

BOOL com_isAllZeroBinary( COM_ANALYZER_PRM )
{
    for( com_off i = 0;  i < ioHead->sig.len;  i++ ) {
        if( ioHead->sig.top[i] ) {return false;}
    }
    ioHead->sig.ptype = COM_SIG_ALLZERO;
    if( iDecode ) {com_decodeSignal( ioHead );}
    return true;
}

void com_dummyAnlzPrm( COM_ANALYZER_PRM )
{
    // 引数使用を促すダミー関数
    COM_UNUSED( ioHead );
    COM_UNUSED( iDecode );
}

void com_inheritRasData( com_sigInf_t *ioHead )
{
    if( !ioHead->ras.top ) {return;}
    if( ioHead->ras.ptype == COM_SGTYPE ) {return;}
    for( long i = 0;  i < COM_NEXTCNT;  i++ ) {
        // ログから読んだ結合データは次スタックに継承
        if( ioHead->ras.ptype ) {
            com_sigInf_t*  nextStk = &(COM_NEXTSTK[i]);
            if( nextStk->ras.top ) {
                com_error( COM_ERR_ANALYZENG,
                           "already exist reassembled data(%ld:%ld)",
                           ioHead->ras.ptype, nextStk->ras.ptype );
                continue;
            }
            nextStk->ras = ioHead->ras;
        }
    }
}

void com_rollbackSignal( com_sigInf_t *oHead, com_sigInf_t *iOrgHead )
{
    com_freeSigInf( oHead, false );
    *oHead = *iOrgHead;
}



// プロトコル解析用共通処理 //////////////////////////////////////////////////

static com_sigInf_t *getStack( com_sigStk_t *oNext )
{
    long*  count = &(oNext->cnt);
    com_sigInf_t*  tmp = oNext->stack;
    size_t  sizeSig = sizeof(com_sigInf_t);
    if( tmp ) {
        tmp = com_reallocAddr( &oNext->stack, sizeSig, COM_TABLEEND,
                               count, 1, "add next[%ld]", *count );
        if( !tmp ) {return NULL;}
    }
    else {
        *count += 1;
        tmp = com_malloc( sizeSig * *count, "get next[%ld]", *count + 1 );
        if( !tmp ) {return NULL;}
        oNext->stack = tmp;
    }
    return tmp;
}

static void setSigInf(
        com_sigInf_t *oTarget, com_bin *iTop, com_off iLen,
        BOOL iOrder, long iType )
{
    oTarget->sig   = (com_sigBin_t){ iTop, iLen, iType };
    oTarget->order = iOrder;
}

BOOL com_setHeadInf(
        com_sigInf_t *ioHead, com_off iLen, long iType, long iNextType )
{
    if( !ioHead ) {COM_PRMNG(false);}
    com_off  orgLen = COM_SGLEN;
    if( orgLen < iLen ) {
        com_error( COM_ERR_ILLSIZE, "header length illegal" );
        return false;
    }
    setSigInf( ioHead, COM_SGTOP, iLen, COM_ORDER, iType );
    if( iNextType == COM_SIG_ONLYHEAD ) {return true;}
    if( orgLen == iLen ) {return true;}
    com_sigStk_t*  nextStack = &ioHead->next;
    com_sigInf_t*  nextSig = NULL;
    if( nextStack->cnt == COM_SIG_STATIC ) {nextSig = nextStack->stack;}
    else {if( !(nextSig = getStack( nextStack )) ) {return false;}}
    setSigInf( nextSig, COM_SGTOP + iLen, orgLen - iLen, COM_ORDER, iNextType );
    nextSig->prev = ioHead;
    return (iNextType != COM_SIG_UNKNOWN);
}

BOOL com_advancePtr( com_bin **oTarget, com_off *oLen, com_off iAdd )
{
    if( !oTarget ) {return false;}
    if( oLen ) {  // oLenが NULLのときは、単純に *oTargetを進めるのみ
        if( *oLen < iAdd ) {return false;}
        *oLen -= iAdd;
    }
    if( *oTarget ) {*oTarget += iAdd;}
    return true;
}

uint16_t com_getVal16( uint16_t iSource, BOOL iOrder )
{
    if( iOrder ) {return ntohs( iSource );}
    return iSource;
}

uint32_t com_getVal32( uint32_t iSource, BOOL iOrder )
{
    if( iOrder ) {return ntohl( iSource );}
    return iSource;
}

uint16_t com_setVal16( uint16_t iSource, BOOL iOrder )
{
    if( iOrder ) {return htons( iSource );}
    return iSource;
}

uint32_t com_setVal32( uint32_t iSource, BOOL iOrder )
{
    if( iOrder ) {return htonl( iSource );}
    return iSource;
}

uint32_t com_calcValue( const void *iTop, com_off iSize )
{
    if( !iTop ) {return 0;}
    if( iSize > COM_32BIT_SIZE ) {iSize = COM_32BIT_SIZE;}
    uint32_t  result = 0;
    for( com_off i = 0;  i < iSize;  i++ ) {
        result = (result << 8) + ((com_bin*)iTop)[i];
    }
    return result;
}

ulong com_getBitField( ulong iSource, ulong iMask, long iShift )
{
    ulong  value = (iSource & iMask);
    if( iShift > 0 ) {value = (value >>   iShift);}
    if( iShift < 0 ) {value = (value << (-iShift));}
    return value;
}

static BOOL checkNull( const void *ptr1, const void *ptr2, BOOL *oResult )
{
    *oResult = false;
    if( !ptr1 && !ptr2 ) {*oResult = true;   return true;}
    if( !ptr1 || !ptr2 ) {*oResult = false;  return true;}
    return false;
}

BOOL com_matchSigBin( const com_sigBin_t *iBin1, const com_sigBin_t *iBin2 )
{
    BOOL check = false;
    if( checkNull( iBin1, iBin2, &check ) ) {return check;}
    if( iBin1->len != iBin2->len ) {return false;}
    if( checkNull( iBin1->top, iBin2->top, &check ) ) {return check;}
    if( memcmp( iBin1->top, iBin2->top, iBin1->len ) ) {return false;}
    return true;
}

static com_sigInf_t *stackSigInf(
        com_sigStk_t *oTarget, com_sigInf_t *iHead, const char *iLabel,
        com_sigInf_t *iSource )
{
    com_sigInf_t*  newStack =
        com_reallocAddr( &(oTarget->stack), sizeof(*iSource), COM_TABLEEND,
                         &(oTarget->cnt), 1, "%s[%ld]", iLabel, oTarget->cnt );
    if( !newStack ) {return NULL;}
    *newStack = *iSource;
    newStack->prev = iHead;
    return newStack;
}

com_sigInf_t *com_stackSigNext( com_sigInf_t *ioHead, com_sigInf_t *iSource )
{
    if( !ioHead || !iSource ) {COM_PRMNG(NULL);}
    return stackSigInf( &(ioHead->next), ioHead, "add sig", iSource );
}

com_sigInf_t *com_getRecentStack( com_sigStk_t *iStk )
{
    if( !iStk ) {COM_PRMNG(NULL);}
    if( iStk->cnt == 0 ) {return NULL;}
    return &(iStk->stack[iStk->cnt - 1]);
}

BOOL com_stackSigMulti(
        com_sigInf_t *ioHead, const char *iLabel, com_sigBin_t *iData )
{
    if( !ioHead || !iLabel || !iData ) {COM_PRMNG(false);}
    com_sigInf_t  newSigStack;
    com_initSigInf( &newSigStack, ioHead );
    newSigStack.sig = *iData;
    if( !stackSigInf( &(ioHead->multi), ioHead, iLabel, &newSigStack ) ) {
        return false;
    }
    return true;
}

BOOL com_addPrmTlv( com_sigPrm_t *oTarget, com_sigTlv_t *oPrm )
{
    if( !oTarget || !oPrm ) {COM_PRMNG(false);}
    com_sigTlv_t*  newTlv =
        com_reallocAddr( &oTarget->list, sizeof(*oTarget->list), COM_TABLEEND,
                         &oTarget->cnt, 1, "add prm[%ld]", oTarget->cnt );
    if( !newTlv ) {return false;}
    *newTlv = *oPrm;
    return true;
}

BOOL com_addPrm(
        com_sigPrm_t *oTarget, void *iTag, com_off iTagSize,
        void *iLen, com_off iLenSize, void *iValue )
{
    if( !oTarget ) {COM_PRMNG(false);}
    com_sigTlv_t  newPrm = {
        .tagBin = (com_sigBin_t){ iTag, iTagSize, 0 },
        .tag = com_calcValue( iTag, iTagSize ),
        .lenBin = (com_sigBin_t){ iLen, iLenSize, 0 },
        .len = com_calcValue( iLen, iLenSize ),
        .value = iValue
    };
    return com_addPrmTlv( oTarget, &newPrm );
}

BOOL com_addPrmDirect(
        com_sigPrm_t *oTarget, com_off iTag, com_off iLen, void *iValue )
{
    if( !oTarget ) {COM_PRMNG(false);}
    com_sigTlv_t  newPrm = {
        .tagBin = (com_sigBin_t){ .top = NULL },
        .tag = iTag,
        .lenBin = (com_sigBin_t){ .top = NULL },
        .len = iLen,
        .value = iValue
    };
    return com_addPrmTlv( oTarget, &newPrm );
}

com_sigTlv_t *com_getRecentPrm( com_sigPrm_t *iPrm )
{
    if( !iPrm ) {COM_PRMNG(NULL);}
    if( iPrm->cnt == 0 ) {return NULL;}
    return &(iPrm->list[iPrm->cnt - 1]);
}

com_sigTlv_t *com_searchPrm( com_sigPrm_t *iTarget, com_off iTag )
{
    if( !iTarget ) {return NULL;}
    for( long i = 0;  i < iTarget->cnt;  i++ ) {
        com_sigTlv_t*  prm = &(iTarget->list[i]);
        if( prm->tag == iTag ) {return prm;}
    }
    return NULL;
}

BOOL com_matchPrm( const com_sigTlv_t *iTlv1, const com_sigTlv_t *iTlv2 )
{
    if( !iTlv1 || !iTlv2 ) {return false;}
    if ( !com_matchSigBin( &iTlv1->tagBin, &iTlv2->tagBin ) ) {return false;}
    if( iTlv1->tag != iTlv2->tag ) {return false;}
    if ( !com_matchSigBin( &iTlv1->lenBin, &iTlv2->lenBin ) ) {return false;}
    if( iTlv1->len != iTlv2->len ) {return false;}
    for( com_off i = 0;  i < iTlv1->len;  i++ ) {
        if( iTlv1->value[i] != iTlv2->value[i] ) {return false;}
    }
    return true;
}

static BOOL dupBinData( com_bin **oTarget, com_off iSize, const char *iLabel )
{
    if( !oTarget || !iSize ) {return true;}
    com_bin*  source = *oTarget;
    if( !(*oTarget = com_malloc( iSize, "dupBinData(%s)", iLabel )) ) {
        return false;
    }
    memcpy( *oTarget, source, iSize );
    return true;
}

BOOL com_dupPrm( com_sigTlv_t *oTarget, const com_sigTlv_t *iSource )
{
    if( !oTarget || !iSource ) {COM_PRMNG(false);}
    *oTarget = *iSource;
    if( dupBinData( &oTarget->tagBin.top, iSource->tagBin.len, "tagBin" ) &&
        dupBinData( &oTarget->lenBin.top, iSource->lenBin.len, "lenBin" ) &&
        dupBinData( &oTarget->value, oTarget->len, "value" ) )
    {
        return true;
    }
    com_free( oTarget->tagBin.top );
    com_free( oTarget->lenBin.top );
    com_free( oTarget->value );
    return false;
}

///// ノード情報管理 /////

static void setPort(
        com_nodeInf_t *oInf, com_bin *iSrcPort, com_off iSrcSize,
        com_bin *iDstPort, com_off iDstSize )
{
    memcpy( oInf->srcPort, iSrcPort, iSrcSize );
    memcpy( oInf->dstPort, iDstPort, iDstSize );
}

static void setPortInf( com_nodeInf_t *oInf, com_sigInf_t *iHead )
{
    switch( COM_ISGTYPE ) {
      case COM_SIG_TCP: {
        COM_CAST_HEAD( struct tcphdr, tcp, COM_ISGTOP );
        setPort( oInf, (com_bin*)&tcp->th_sport, sizeof(tcp->th_sport),
                 (com_bin*)&tcp->th_dport, sizeof(tcp->th_dport) );
        break;
      }
      case COM_SIG_UDP: {
        COM_CAST_HEAD( struct udphdr, udp, COM_ISGTOP );
        setPort( oInf, (com_bin*)&udp->uh_sport, sizeof(udp->uh_sport),
                 (com_bin*)&udp->uh_dport, sizeof(udp->uh_dport) );
        break;
      }
      case COM_SIG_SCTP: {
        COM_CAST_HEAD( com_sigSctpCommonHdr_t, sctp, COM_ISGTOP );
        setPort( oInf, (com_bin*)&sctp->srcPort, sizeof(sctp->srcPort),
                 (com_bin*)&sctp->dstPort, sizeof(sctp->dstPort) );
        break;
      }
      default: break;
    }
}

static void setAddr(
        com_nodeInf_t *oInf, com_sigInf_t *iHead,
        com_bin *iSrcAddr, com_off iSrcSize,
        com_bin *iDstAddr, com_off iDstSize )
{
    oInf->ptype = COM_ISGTYPE;
    memcpy( oInf->srcAddr, iSrcAddr, iSrcSize );
    memcpy( oInf->dstAddr, iDstAddr, iDstSize );
}

static void setAddrInf( com_nodeInf_t *oInf, com_sigInf_t *iHead )
{
    switch( COM_ISGTYPE ) {
      case COM_SIG_IPV4: {
        COM_CAST_HEAD( struct ip, ip, COM_ISGTOP );
        setAddr( oInf, iHead, (com_bin*)&ip->ip_src, sizeof(ip->ip_src),
                 (com_bin*)&ip->ip_dst, sizeof(ip->ip_dst) );
        break;
      }
      case COM_SIG_IPV6: {
        COM_CAST_HEAD( struct ip6_hdr, ip, COM_ISGTOP );
        setAddr( oInf, iHead, (com_bin*)&ip->ip6_src, sizeof(ip->ip6_src),
                 (com_bin*)&ip->ip6_dst, sizeof(ip->ip6_dst) );
        break;
      }
      default: break;
    }
}

BOOL com_getNodeInf( com_sigInf_t *iHead, com_nodeInf_t *oInf )
{
    if( !iHead || !oInf ) {COM_PRMNG(false);}
    memset( oInf, 0, sizeof(*oInf) );
    while( com_getLayerType( COM_ISGTYPE ) != COM_SIG_TRANSPORT_LAYER ) {
        iHead = iHead->prev;
        if( !iHead ) {return false;}
    }
    setPortInf( oInf, iHead );
    while( COM_ISGTYPE != COM_SIG_IPV4 && COM_ISGTYPE != COM_SIG_IPV6 ) {
        iHead = iHead->prev;
        if( !iHead ) {return false;}
    }
    setAddrInf( oInf, iHead );
    return true;
}

#define COMPARE_NODEINF( PRM, SIZE ) \
    if( memcmp( iInf1->PRM, iInf2->PRM, SIZE ) ) {return false;} \
    do{} while(0)

BOOL com_matchNodeInf( com_nodeInf_t *iInf1, com_nodeInf_t *iInf2 )
{
    if( !iInf1 || !iInf2 ) {COM_PRMNG(false);}
    COMPARE_NODEINF( srcAddr, COM_NODEADDR_SIZE );
    COMPARE_NODEINF( dstAddr, COM_NODEADDR_SIZE );
    COMPARE_NODEINF( srcPort, COM_NODEPORT_SIZE );
    COMPARE_NODEINF( dstPort, COM_NODEPORT_SIZE );
    return true;
}

static void reverseBin( com_bin *ioBin1, com_bin *ioBin2, size_t iSize )
{
    com_bin  tmp[COM_NODEADDR_SIZE] = {0};
    memcpy( tmp,    ioBin1, iSize );
    memcpy( ioBin1, ioBin2, iSize );
    memcpy( ioBin2, tmp,    iSize );
}

void com_reverseNodeInf( com_nodeInf_t *oInf )
{
    if( !oInf ) {COM_PRMNG();}
    reverseBin( oInf->srcAddr, oInf->dstAddr, COM_NODEADDR_SIZE );
    reverseBin( oInf->srcPort, oInf->dstPort, COM_NODEPORT_SIZE );
}


// テキストベースプロトコル解析用共通処理 ////////////////////////////////////

#define  CRLF  "\r\n"
#define  BOUNDARY   "boundary"
#define  BOUNDLINE  "--"
#define  STRNCMP( TARGET, SOURCE ) \
    strncmp( (TARGET), SOURCE, strlen(SOURCE) )

enum { STATUSCODE_DIGITS = 3 };

static BOOL getStatusCode( char *iSig, long *oType )
{
    char*  ptr = com_topString( iSig, false );
    char  code[STATUSCODE_DIGITS + 1] = {0};
    memcpy( code, ptr, STATUSCODE_DIGITS );
    if( !com_valDigit( code, NULL ) ) {return false;}
    ptr += STATUSCODE_DIGITS;
    if( *ptr != ' ' ) {return false;}
    COM_SET_IF_EXIST( oType, com_atol( code ) );
    return true;
}

BOOL com_getMethodName(
        com_sigMethod_t *iTable, void *iSig, const char **oLabel, long *oType )
{
    if( !iTable || !iSig ) {COM_PRMNG(false);}
    char*  ptr = iSig;
    for( long i = 0;  iTable[i].token;  i++ ) {
        const char*  token = iTable[i].token;
        if( strncmp( ptr, token, strlen( token ) ) ) {continue;}
        ptr += strlen( token );
        if( *ptr != ' ' ) {continue;}
        COM_SET_IF_EXIST( oLabel, token );
        COM_SET_IF_EXIST( oType, iTable[i].sigType );
        if( iTable[i].sigType == COM_SIG_METHOD_END ) {
            return getStatusCode( ptr, oType );
        }
        return true;
    }
    return false;
}

com_off com_getHeaderRest( char *iHdr, int iSep )
{
    if( !iHdr ) {COM_PRMNG(0);}
    com_off  result = 0;
    for( ; *iHdr && *iHdr != '\r' && *iHdr != '\n';  iHdr++ ) {
        if( iSep ) {if( *iHdr == iSep ) {break;}}
        result++;
    }
    return result;
}

static BOOL isWhiteSpace( char *iTarget )
{
    if( *iTarget == ' ' || *iTarget == '\t' ) {return true;}
    if( *iTarget == '\r' || *iTarget == '\n' ) {return true;}
    if( *iTarget == '\0' ) {return true;}
    return false;
}

static BOOL checkToken( char iChar, const int iSpec )
{
    if( isWhiteSpace( &iChar ) ) {return true;}
    if( iSpec && iChar == iSpec ) {return true;}
    return false;
}

enum { NO_MATCH = 0 };

com_off com_matchToken(
        char *iSource, const char *iTarget, BOOL iNoCase, const int iSpec )
{
    if( !com_compareString( iSource, iTarget, strlen(iTarget), iNoCase ) ) {
        return NO_MATCH;
    }
    com_off  pos = strlen( iTarget );
    if( !checkToken( iSource[pos], iSpec ) ) {return NO_MATCH;}
    while( checkToken( iSource[++pos], iSpec ) ) {}
    return pos;
}

void com_setTxtHeaderVal( com_sigTlv_t *oTlv, char *iHdr, int iSep )
{
    if( !oTlv || !iHdr ) {COM_PRMNG();}
    memset( oTlv, 0, sizeof(*oTlv) );
    oTlv->tagBin = (com_sigBin_t){
        (com_bin*)iHdr, com_getHeaderRest( iHdr, iSep ), 0
    };
    char*  val = com_topString( iHdr + oTlv->tagBin.len + 1, false );
    oTlv->len = com_getHeaderRest( val, 0 );
    oTlv->value = (com_bin*)val;
}

static void procMultiLine( char *ioVal )
{
    size_t  pos = 0;
    BOOL  isSkip = false;
    for( size_t i = 0;  i < strlen(ioVal);  i++ ) {
        if( !strncmp( &(ioVal[i]), CRLF, strlen(CRLF) ) ) {
            ioVal[pos++] = ' ';
            isSkip = true;
        }
        if( isSkip && isWhiteSpace( &(ioVal[i]) ) ) {continue;}
        ioVal[pos++] = ioVal[i];
    }
    ioVal[pos] = '\0';
}

char *com_getTxtHeaderVal( com_sigPrm_t *iPrm, const char *iHeader )
{
    if( !iPrm || !iHeader ) {COM_PRMNG(NULL);}
    static char  value[COM_TEXTBUF_SIZE];
    COM_CLEAR_BUF( value );
    for( long i = 0;  i < iPrm->cnt;  i++ ) {
        com_sigTlv_t*  prm = &(iPrm->list[i]);
        if( strlen( iHeader ) != prm->tagBin.len ) {continue;}
        if( com_compareString( (const char *)prm->tagBin.top,
                               iHeader, strlen(iHeader), true ) )
        {
            (void)com_strncpy( value, sizeof(value),
                               (char*)prm->value, prm->len );
            procMultiLine( value );
            return value;
        }
    }
    return NULL;
}

static BOOL addTxtHead( com_sigTextSeek_t *oInf, char *iLine )
{
    com_off*  hdrSize = oInf->size;
    char*  hdr = (char*)(oInf->head->sig.top) + *hdrSize;
    char*  top = com_topString( iLine, false );
    com_sigPrm_t*  prm = &oInf->head->prm;
    if( top == iLine ) { // 0カラムから始まる(新たなヘッダ行)
        com_sigTlv_t  newHdr;
        com_setTxtHeaderVal( &newHdr, hdr, ':' );
        if( !com_addPrmTlv( prm, &newHdr ) ) {return false;}
    }
    else {
        com_sigTlv_t*  lastHdr = com_getRecentPrm( prm );
        lastHdr->len += strlen( iLine );  // 直前ヘッダ値の続きとみなす
    }
    *hdrSize += strlen( iLine );
    return true;
}

// addTxtHead()の継続行の扱いについて
//   lastHdr->len += strlen( iLine ) とすることは iLine に含まれる \r\n も
//   入れることを意味するように見える。しかし、通常の .len は \r\n の前までの
//   レングス値となっており、この記述にすることで
//     前行の \r\n + iLine行末の \r\n直前
//   を示すレングス値になることを見越している。
//   com_getTxtHeaderVal() もそれを踏まえたヘッダ値を編集して返す。

static BOOL getTxtHeadVal( com_seekFileResult_t *iInf )
{
    com_sigTextSeek_t*  inf = iInf->userData;
    if( *inf->size == 0 ) {  // 先頭行はスキップ
        *inf->size = strlen( iInf->line );
        return true;
    }
    if( !strcmp( iInf->line, CRLF ) ) {
        *inf->size += strlen(CRLF);  // 空行分はヘッダサイズに加算
        return false;
    }
    if( !addTxtHead( inf, iInf->line ) ) {
        *inf->size = 0;  // 処理NGで中断
        return false;
    }
    return true;
}

static char  gTxtBuf[COM_TEXTBUF_SIZE];

char *com_getTxtHeader( com_sigInf_t *ioHead, com_off *oHdrSize )
{
    if( !ioHead || !oHdrSize ) {COM_PRMNG(NULL);}
    *oHdrSize = 0;
    com_sigTextSeek_t  inf = { ioHead, oHdrSize };
    if( !com_seekTextLine( COM_SGTOP, COM_SGLEN, true, getTxtHeadVal, &inf,
                           gTxtBuf, sizeof(gTxtBuf) ) )
    {
        if( *oHdrSize ) {return (char*)COM_SGTOP + *oHdrSize;}
    }
    return NULL;
}

BOOL com_seekTxtHeaderPrm( char *iHdr, char *iPrm, char **oTop, com_off *oLen )
{
    if( !iHdr || !iPrm || !oTop || !oLen ) {COM_PRMNG(false);}
    char*  bndr = com_searchString( iHdr, iPrm, NULL, 0, true );
    if( !bndr ) {return false;}
    bndr += strlen( iPrm );
    bndr += com_getHeaderRest( bndr, '=' ) + 1;
    if( *bndr == '\0' ) {return false;}
    *oTop = com_topString( bndr, false );
    *oLen = com_getHeaderRest( *oTop, ';' );
    return true;
}

static com_sigInf_t *addNewBody(
        com_sigInf_t *ioHead, char *iType, const char *iPtr, com_off iLen,
        COM_PRTCLTYPE_t iNext )
{
    com_sigInf_t  body;
    com_initSigInf( &body, ioHead );
    body.sig = (com_sigBin_t){
        (com_bin*)iPtr, iLen, com_getPrtclLabel( iNext, iType )
    };
    return com_stackSigNext( ioHead, &body );
}

static BOOL getBoundary( com_sigPrm_t *oHdr, char *iConType )
{
    char*  boundary = NULL;
    com_off  bSize  = 0;
    if( !com_seekTxtHeaderPrm( iConType, BOUNDARY, &boundary, &bSize ) ) {
        return false;
    }
    if( *boundary == '\"' || *boundary == '\'' ) {boundary++; bSize -= 2;}
    oHdr->spec = com_malloc( bSize + 1, "SIP multibody boundary" );
    if( !oHdr->spec ) {return false;}
    return com_strncpy( (char*)(oHdr->spec), bSize + 1, boundary, bSize );
}

static BOOL procBoundary(
        com_sigInf_t *ioHead, const char **ioPtr, com_off *oBodySize,
        COM_PRTCLTYPE_t iNext )
{
    com_sigInf_t*  recent = com_getRecentStack( &ioHead->next );
    if( recent && *oBodySize ) {recent->sig.len = *oBodySize;}
    *ioPtr += strlen(BOUNDLINE) + strlen((char*)(ioHead->prm.spec));
    if( !STRNCMP( *ioPtr, BOUNDLINE ) ) {*ioPtr += strlen(BOUNDLINE);}
    else {  // 新しいボディ部として枠だけ先に追加する
        if( !addNewBody( ioHead,NULL,NULL,0,iNext ) ) {return false;}
        *oBodySize = 0;
    }
    if( !STRNCMP( *ioPtr, CRLF ) ) {*ioPtr += strlen(CRLF);}
    return true;
}

static BOOL existBody( com_sigInf_t *iBody )
{
    if( !iBody ) {return false;}
    if( !iBody->sig.top ) {return false;}
    return true;
}

static BOOL checkBoundary(
        com_sigInf_t *ioHead, const char **ioPtr, com_off *oBodySize,
        BOOL *oContinue, COM_PRTCLTYPE_t iNext )
{
    *oContinue = false;
    if( STRNCMP( *ioPtr, BOUNDLINE ) ) {return false;}
    *oContinue = procBoundary( ioHead, ioPtr, oBodySize, iNext );
    return true;
}

static BOOL checkBlankLine( const char **ioPtr )
{
    if( STRNCMP( *ioPtr, CRLF ) ) {return false;}
    *ioPtr += strlen(CRLF);
    return true;
}

static BOOL judgeBody(
        com_sigInf_t *ioBody, com_off *oBodySize, const char *iPtr )
{
    if( !ioBody ) {return false;}  // 余分な空行のガード
    if( !existBody( ioBody ) ) {
        ioBody->sig.top = (com_bin*)iPtr;
        return true;
    }
    // 既にボディ内なら単純にボディサイズに計上
    *oBodySize += strlen( CRLF );
    return false;
}

static void getHeadInBody(
        com_sigInf_t *oBody, const char **ioPtr, COM_PRTCLTYPE_t iNext )
{
    com_off  lineSize = com_getHeaderRest( (char*)(*ioPtr), 0 );
    com_off  pos = com_matchToken(
                       (char*)*ioPtr, COM_CAP_SIPHDR_CTYPE, true, ':' );
    if( pos ) {
        oBody->sig.ptype = com_getPrtclLabel( iNext, (char*)(*ioPtr + pos) );
    }
    *ioPtr += lineSize;
    if( !STRNCMP( *ioPtr, CRLF ) ) {*ioPtr += strlen( CRLF );}
}

static BOOL getMultiBody(
        com_sigInf_t *ioHead, char *iConType, const char *iPtr, com_off iLen,
        COM_PRTCLTYPE_t iNext )
{
    if( !getBoundary( COM_APRM, iConType ) ) {return false;}
    const char*  ptr = iPtr;
    com_off  bodySize = 0;
    while( (com_off)(ptr - iPtr) < iLen ) {
        BOOL  needContinue = true;
        com_sigInf_t*  body = com_getRecentStack( &ioHead->next );
        // 一番最初のバウンダリー文字列行
        if( checkBoundary( ioHead,&ptr,&bodySize,&needContinue,iNext ) ) {
            if( needContinue ) {continue;} else {return false;}
        }
        else if( checkBlankLine( &ptr ) ) {
            // 2番目以降のバウンダリー文字列行 (\r\n + バウンダリー文字列)
            if( checkBoundary( ioHead,&ptr,&bodySize,&needContinue,iNext ) ) {
                if( needContinue ) {continue;} else {return false;}
            }
            if( judgeBody( body, &bodySize, ptr ) ) {continue;}
        }
        else if( !existBody( body ) ) {
            getHeadInBody( body, &ptr, iNext );
            continue;
        }
        bodySize++;
        ptr++;
    }
    return true;
}

static BOOL procTcpSeg( com_sigInf_t *ioHead, com_off iSigSize )
{
    if( !ioHead->prev ) {return false;}
    if( ioHead->prev->sig.ptype != COM_SIG_TCP ) {return false;}
    if( iSigSize <= COM_SGLEN ) {return false;}
    if( !com_stockTcpSeg( ioHead, ioHead->prev, iSigSize ) ) {return false;}
    return true;
}

BOOL com_getTxtBody(
        com_sigInf_t *ioHead, COM_PRTCLTYPE_t iNext, com_off iHdrSize,
        const char *iConLen, const char *iConType, const char *iMulti )
{
    if( !ioHead || !iConLen || !iConType || !iMulti ) {COM_PRMNG(false);}
    com_off  bodyLen = 0;
    com_sigPrm_t*  hdr = COM_APRM;
    char  *cLen = com_getTxtHeaderVal( hdr, iConLen );
    if( cLen ) {bodyLen = (com_off)com_atol( cLen );}
    if( !bodyLen ) {return true;}
    // ボディ長はあるが実データのボディが不完全 (セグメンテーション結合待ち)
    if( procTcpSeg( ioHead, iHdrSize + bodyLen ) ) {return true;}

    char  cType[COM_TERMBUF_SIZE] = {0};
    char*  getType = com_getTxtHeaderVal( hdr, iConType );
    (void)com_strcpy( cType, getType );
    char*  ptr = (char*)COM_SGTOP + iHdrSize;
    if( com_compareString( cType, iMulti, strlen(iMulti), true ) ) {
        return getMultiBody( ioHead, cType, ptr, bodyLen, iNext );
    }
    return (NULL != addNewBody( ioHead, cType, ptr, bodyLen, iNext ));
}

BOOL com_analyzeTxtBase(
        com_sigInf_t *ioHead, long iType, COM_PRTCLTYPE_t iNext,
        const char *iConLen, const char *iConType, const char *iMulti )
{
    if( !ioHead || !iConLen || !iConType || !iMulti ) {COM_PRMNG(false);}
    BOOL  result = false;
    com_off  hdrSize = 0;
    if( com_getTxtHeader( ioHead, &hdrSize ) ) {
        result = com_getTxtBody( ioHead, iNext, hdrSize,
                                 iConLen, iConType, iMulti );
    }
    if( result ) {
        if( !ioHead->isFragment ) {
            result = com_setHeadInf( ioHead,hdrSize,iType,COM_SIG_ONLYHEAD );
        }
        else {COM_SGTYPE = iType;}
    }
    return result;
}



// ASN.1解析用共通処理 ///////////////////////////////////////////////////////

enum { LONGFORM = 0x80 };

com_off com_getTagLength( com_bin *iSignal, com_off *oTag )
{
    COM_SET_IF_EXIST( oTag, 0 );
    // short form
    if( !COM_CHECKBIT( *iSignal, LONGFORM ) ) {
        COM_SET_IF_EXIST( oTag, *iSignal );
        return 1;
    }
    com_off  size = *iSignal - LONGFORM;
    // indefinite form (レングス値は 0 のまま返す)
    if( !size ) {return 1;}
    // long form
    iSignal++;
    for( com_off i = 0;  i < size;  i++, iSignal++ ) {
        COM_SET_IF_EXIST( oTag, ((*oTag) << 8) + *iSignal );
    }
    return (size + 1);
}

static BOOL addIndefinite( com_sigPrm_t *oPrm )
{
    if( oPrm->spec ) {  // 暫定処理(indefinite modeの入れ子は現状完璧ではない)
        long*  tmpIndef = oPrm->spec;
        (*tmpIndef)++;
        return true;
    }
    long*  newIndef = com_malloc( sizeof(long), "indefinite count" );
    if( !newIndef ) {return false;}
    *newIndef = 1;
    oPrm->spec = newIndef;
    return true;
}

static void delIndefinite( com_sigPrm_t *oPrm )
{
    if( !oPrm->spec ) {return;}
    long*  tmpIndef = oPrm->spec;
    (*tmpIndef)--;
    if( *tmpIndef ) {return;}
    com_free( oPrm->spec );
}

// indefinite modeの保持に、oPrm->specを使用していることに注意。
// TODO: 複数オクテットのタグが必要な場合は別途処理検討が必要。
BOOL com_getAsnTlv( com_bin **ioSignal, com_sigPrm_t *oPrm )
{
    com_sigTlv_t  tlv = { .tagBin = {*ioSignal, 1, 0}, .tag = **ioSignal };
    if( oPrm->spec && tlv.tag == 0x00 ) {
        delIndefinite( oPrm );
        (*ioSignal)++;
        return true;
    }
    com_off  lenSize = com_getTagLength( ++(*ioSignal), &tlv.len );
    if( lenSize == 1 && tlv.len == 0 ) {
        if( !addIndefinite( oPrm ) ) {return false;}
        if( !COM_ASN_ISCONST( tlv.tag ) ) {
            com_error( COM_ERR_INCORRECT,
                       "length is indefinite, but tag isn't constructor" );
        }
    }
    tlv.lenBin = (com_sigBin_t){ *ioSignal, lenSize, 0 };
    if( tlv.tag == COM_ASN1_EOC_TAG && tlv.len == COM_ASN1_EOC_LENGTH ) {
        *ioSignal += 2;
        return **ioSignal;
    }
    tlv.value = (*ioSignal += lenSize);
    if( !com_addPrmTlv( oPrm, &tlv ) ) {return false;}
    if( !COM_ASN_ISCONST( tlv.tag ) ) {*ioSignal += tlv.len;}
    return true;
}

#define TLV_LENGTH  (iTlv->tagBin.len + iTlv->lenBin.len + iTlv->len)

static void pushConLength(
        com_off *ioNest, com_off *ioRest, com_sigTlv_t *iTlv,
        com_strChain_t **ioRestStack )
{
    if( *ioNest ) {
        *ioRest -= TLV_LENGTH;
        if( !com_pushChainNum( ioRestStack, *ioRest ) ) {return;}
    }
    (*ioNest)++;
    *ioRest = iTlv->len;
}

static void popConLength(
        com_off *ioNest, com_off *ioRest, com_sigTlv_t *iTlv,
        com_strChain_t **ioRestStack )
{
    if( *ioRest == 0 ) {return;}
    *ioRest -= TLV_LENGTH;
    while( *ioRest == 0 ) {
        (*ioNest)--;
        if( *ioNest == 0 ) {break;}
        *ioRest = com_popChainNum( ioRestStack );
    }
}

static BOOL freeChainEnd( BOOL iResult, com_strChain_t **oStack )
{
    com_freeChainNum( oStack );
    return iResult;
}

BOOL com_searchAsnTlv(
        com_sigPrm_t *iPrm, uint32_t *iTags, size_t iTagSize,
        com_sigBin_t *oValue )
{
    if( !iPrm || !iTags || !oValue ) {COM_PRMNG(false);}
    com_off  nestCnt = 0;
    com_strChain_t*  restStack = NULL;
    com_off  rest = 0;
    com_off  tagsCnt = iTagSize / sizeof(uint32_t);
    com_off  hitCnt = 0;
    for( long i = 0;  i < iPrm->cnt;  i++ ) {
        com_sigTlv_t*  tlv = &(iPrm->list[i]);
        if( nestCnt < hitCnt ) {hitCnt = 0;}
        if( nestCnt == hitCnt && tlv->tag == iTags[hitCnt] ) {
            if( ++hitCnt == tagsCnt ) {
                *oValue = (com_sigBin_t){ tlv->value, tlv->len, 0 };
                return freeChainEnd( true, &restStack );
            }
        }
        if( !COM_ASN_ISCONST( tlv->tag ) ) {
            popConLength( &nestCnt, &rest, tlv, &restStack );
        }
        else {pushConLength( &nestCnt, &rest, tlv, &restStack );}
    }
    return freeChainEnd( false, &restStack );
}



// 解析結果出力共通処理 //////////////////////////////////////////////////////

void com_decodeData( COM_DECODER_PRM, const char *iLabel )
{
    com_dispDec( " %s  <length=%zu>", iLabel, COM_ISGLEN );
    if ( com_isAllZeroBinary( iHead, false ) ) {
        com_dispDec( "   << all 0x00 binaries >>" );
        return;
    }
    COM_ASCII_DUMP( &COM_ISG );
}

void com_onlyDispSig( COM_DECODER_PRM )
{
    char  label[COM_WORDBUF_SIZE] = {0};
    snprintf( label, sizeof(label), "%s ", com_searchSigProtocol(COM_ISGTYPE) );
    com_dispDec( " %s[%ld]  <length=%zu>", label, COM_ISGTYPE, COM_ISGLEN );
    COM_ASCII_DUMP( &COM_ISG );
    //com_dispSig( label, sig->ptype, sig );
    com_dispDec( "     *** %s not analyzed, display only ***", label );
}

void com_dispIfExist( com_sigPrm_t *iPrm, long iType, const char *iName )
{
    com_sigTlv_t*  prm = com_searchPrm( iPrm, iType );
    if( !prm ) {return;}
    com_dispBin( iName, prm->value, prm->len, "", true );
}

void com_dispDec( const char *iFormat, ... )
{
    char  buf[COM_TERMBUF_SIZE] = {0};
    COM_SET_FORMAT( buf );
    com_printf( "#%s\n", buf );
}

#define DUMP_COLS_MAX  32

void com_dispSig( const char *iName, long iCode, const com_sigBin_t *iSig )
{
    if( !iSig->top || !iSig->len ) {return;}
    if( iCode > COM_NO_PTYPE ) {
        com_dispDec( " %s[%ld]  <length=%zu>", iName, iCode, iSig->len );
    }
    else {com_dispDec( " %s  <length=%zu>", iName, iSig->len );}
    com_printBin_t  flags = {
        .prefix = "#   ",
        .cols = (iSig->len < DUMP_COLS_MAX) ? iSig->len : DUMP_COLS_MAX
    };
    com_printBinary( iSig->top, iSig->len, &flags );
}

static void dispName( const char *iName )
{
    com_printf( "#    %s = ", iName );
}

void com_dispVal( const char *iName, long iValue )
{
    dispName( iName );
    com_printf( "%ld\n", iValue );
}

static void dispBin(
        const com_bin *iTop, com_off iLen, const char *iSep, BOOL iHex )
{
    for( com_off i = 0;  i < iLen;  i++ ) {
        if( iHex ) {com_printf( "%02x", iTop[i] );}
        else {com_printf( "%d", iTop[i] );}
        if( i != iLen - 1 ) {com_printf( "%s", iSep );}
    }
    com_printLf();
}

static ulong dispValue( const void *iValue, com_off iSize )
{
    ulong  result = com_calcValue( iValue, iSize );
    com_printf( "0x%0*lx (%lu)\n", (int)iSize * 2, result, result );
    return result;
}

ulong com_dispPrm( const char *iName, const void *iValue, com_off iSize )
{
    dispName( iName );
    if( !iSize ) {com_printf( "%s\n", (char*)iValue );  return 0;}
    if( iSize <= COM_32BIT_SIZE ) {return dispValue( iValue, iSize );}
    dispBin( iValue, iSize, " ", true );
    return 0;
}

void com_dispBin(
        const char *iName, const void *iTop, com_off iLen,
        const char *iSep, BOOL iHex )
{
    dispName( iName );
    dispBin( iTop, iLen, iSep, iHex );
}

static long getDecSize( long iSize )
{
    if( iSize < 1 || iSize > 8) {return 0;}
    long  hexMax = 1 << (iSize * 4);
    long  digit = 1;
    while( hexMax > 9 ) {digit++;  hexMax /= 10;}
    return digit;
}

static void dispColSpace( long iHexSize )
{
    com_repeat( " ", iHexSize + getDecSize( iHexSize ), false );
}

static void dispColRuler( long iTagSize, long iLenSize )
{
    com_printf( "#    -tag-" );
    dispColSpace( iTagSize );
    com_printf( "-len-" );
    dispColSpace( iLenSize );
    com_printf( "-value-\n" );
}

static void dispPrmListVal( long iValue, long iHexSize )
{
    com_printf( "0x%0*lx/%-*ld  ", (int)iHexSize, iValue,
                                 (int)getDecSize( iHexSize ), iValue );
}

void com_dispPrmList(
        com_sigPrm_t *iPrm, long iTagSize, long iLenSize,
        com_judgeDispPrm_t iFunc )
{
    dispColRuler( iTagSize, iLenSize );
    for( long i = 0;  i < iPrm->cnt;  i++ ) {
        com_sigTlv_t*  tlv = &(iPrm->list[i]);
        com_printf( "#    " );
        dispPrmListVal( tlv->tag, iTagSize );
        dispPrmListVal( tlv->len, iLenSize );
        if( iFunc ) {
            if( !iFunc(tlv->tag) ) {
                com_printf("<payload data>\n");
                continue;
            }
        }
        com_printf( "0x" );
        dispBin( tlv->value, tlv->len, "", true );
    }
}

void com_dispNext( long iProtocol, size_t iSize, long iType )
{
    dispName( "next protocol" );
    com_printf( "0x%0*lx(%lu) -> ", (int)iSize * 2, iProtocol, iProtocol );
    if( iType > COM_SIG_UNKNOWN ) {
        com_printf( "%s [%ld]\n", com_searchSigProtocol( iType ), iType );
    }
    else {
        if( iType == COM_SIG_UNKNOWN ) {com_printf( "UNKNOWN\n" );}
        else if( iType == COM_SIG_FRAGMENT ) {com_printf( "FRAGMENTED\n" );}
        else if( iType == COM_SIG_END ) {com_printf( "ANALYZE END\n" );}
        else if( iType == COM_SIG_EXTENSION ) {com_printf( "EXTENSION\n" );}
        else {com_printf( "NOT SUPPORTED PROTOCOL\n" );}
    }
}

long com_getSigType( com_sigInf_t *iInf )
{
    if( !iInf ) {return 0;}
    long  ptype = iInf->sig.ptype;
    if( iInf->isFragment ) {ptype = COM_SIG_FRAGMENT;}
    return ptype;
}

enum { NAMEBUF_MAX = 5 };

char *com_searchDecodeName( com_decodeName_t *iList, long iCode, BOOL iAddNum )
{
    if( !iList ) {COM_PRMNG(NULL);}
    static char  nameBuf[NAMEBUF_MAX][COM_LINEBUF_SIZE];
    static long  idx = NAMEBUF_MAX - 1;  // 次行で最初は 0 になる仕掛け
    idx = (idx + 1 ) % NAMEBUF_MAX;
    memset( nameBuf[idx], 0, sizeof(*(nameBuf[idx])) );

    for( long i = 0;  iList[i].code >= 0;  i++ ) {
        if( iList[i].code == iCode ) {
            char*  hit = iList[i].name;
            if( !iAddNum ) {return hit;}
            snprintf( nameBuf[idx], sizeof(nameBuf[idx]),
                      "%s (0x%02x/%ld)", hit, (int)iCode, iCode );
            return nameBuf[idx];
        }
    }
    com_error( COM_ERR_NOTPROTO,
               "not found [type = %ld(0x%lx)]", iCode, iCode );
    return NULL;
}

static void dispHdr( com_sigPrm_t *iPrm, const char *iHdrName )
{
    char*  value = com_getTxtHeaderVal( iPrm, iHdrName );
    if( !value ) {return;}
    com_dispPrm( iHdrName, value, 0 );
}

static void dispHdrList( com_sigInf_t *iHead, const char **iHdrList )
{
    for( long i = 0;  iHdrList[i];  i++ ) {dispHdr( COM_IAPRM, iHdrList[i] );}
}

static void editHdrList( com_sigInf_t *iHead )
{
    com_buf_t  tmp;
    com_initBuffer( &tmp, 0, "editHdrList" );
    com_printf( "# --- header list -------\n" );
    for( long i = 0;  i < COM_IPRMCNT;  i++ ) {
        com_sigTlv_t*  tlv = &(COM_IPRMLST[i]);
        com_setBufferSize( &tmp, tlv->tagBin.len, (char*)tlv->tagBin.top );
        com_printf( "#    %s = ", tmp.data );
        com_setBufferSize( &tmp, tlv->len, (char*)tlv->value );
        com_printf( "%s\n", tmp.data );
    }
    com_resetBuffer( &tmp );
}

static void dispBodyAsText( com_sigBin_t *iText )
{
    for( com_off i = 0;  i < iText->len;  i++ ) {
        if( !i ) {com_printf( "#" );}
        else if( iText->top[i-1] == '\n' ) {com_printf( "#" );}
        com_printf( "%c", iText->top[i] );
    }
}

static void dispTxtBody( com_sigInf_t *iHead, const char *iConType )
{
    const char  BODYLABEL[] = "HTTP BODY";
    for( long i = 0;  i < COM_INEXTCNT;  i++ ) {
        com_sigInf_t*  tmp = &(COM_INEXTSTK[i]);
        if( tmp->sig.ptype != COM_SIG_END ) {continue;}
        char*  conType = com_getTxtHeaderVal( COM_IAPRM, iConType );
        if( com_compareString( conType, COM_CAP_CTYPE_TXTETC,
                               strlen(COM_CAP_CTYPE_TXTETC), true ) )
        {
            com_printf( "# %s[%ld]  <length=%zu>\n",
                        BODYLABEL, i, tmp->sig.len );
            dispBodyAsText( &(tmp->sig) );
            com_printf( "# --- %s END (text) ---\n", BODYLABEL );
        }
        else {
            com_dispSig( BODYLABEL, i, &(tmp->sig) );
            com_printf( "# --- %s END (binary) ---\n", BODYLABEL );
        }
    }
}

void com_decodeTxtBase(
        com_sigInf_t *iHead, const char **iHdrList,
        const char *iSigLabel, long iSigType, const char *iConType )
{
    if( !iHead || !iSigLabel || !iConType ) {COM_PRMNG();}
    com_printf( "# %s HEADER [%ld]  <length=%zu>\n",
                com_searchSigProtocol( COM_ISGTYPE ), COM_ISGTYPE, COM_ISGLEN );
    if( iSigType < COM_SIG_METHOD_END ) {com_dispPrm( "Method", iSigLabel, 0 );}
    else {com_dispVal( "Status Code", iSigType );}
    if( iHdrList ) {dispHdrList( iHead, iHdrList );}
    else {editHdrList( iHead );}
    dispTxtBody( iHead, iConType );
}

static void dispTagValue( const char *iLabel, com_bin *iTop, com_off iLen )
{
    if( !iLen ) {return;}
    com_printf( "%s=0x", iLabel );
    for( com_off i = 0;  i < iLen;  i++ ) {com_printf( "%02x", iTop[i] );}
}

static void dispTagLen(
        const com_sigTlv_t *iTlv, com_off iNest, const char *iLabel )
{
    com_printf( "%s", iLabel );
    com_repeat( " ", iNest, false );
    dispTagValue( "tag", iTlv->tagBin.top, iTlv->tagBin.len );
    dispTagValue( "len", iTlv->lenBin.top, iTlv->lenBin.len );
    com_printf( "(%0zu)", iTlv->len );
}

void com_dispAsnPrm( const com_sigPrm_t *iPrm, const char *iLabel, long iStart )
{
    if( !iPrm || !iLabel ) {COM_PRMNG();}
    com_off  nestCnt = 0;
    com_strChain_t*  restStack = NULL;
    com_off  rest = 0;
    for( long i = 0;  i < iPrm->cnt;  i++ ) {
        com_sigTlv_t*  tlv = &(iPrm->list[i]);
        if( i >= iStart ) {dispTagLen( tlv, nestCnt, iLabel );}
        if( !COM_ASN_ISCONST( tlv->tag ) ) {
            if( i >= iStart ) {dispTagValue( "value", tlv->value, tlv->len );}
            popConLength( &nestCnt, &rest, tlv, &restStack );
        }
        else {pushConLength( &nestCnt, &rest, tlv, &restStack );}
        if( i >= iStart ) {com_printLf();}
    }
    com_freeChainNum( &restStack );
}



// フラグメント処理 //////////////////////////////////////////////////////////

static long  gFrgInfCnt = 0;
static com_sigFrgManage_t*  gFrgInf = NULL;

void com_freeSigFrgCond( com_sigFrgCond_t *oTarget )
{
    if( !oTarget ) {COM_PRMNG();}
    com_free( oTarget->src.top );
    com_free( oTarget->dst.top );
}

static void freeFragMng( com_sigFrgManage_t *oTarget )
{
    if( !oTarget->isUse ) {return;}
    oTarget->isUse = false;
    com_freeSigFrgCond( &oTarget->cond );
    com_freeSigFrg( &oTarget->data );
} 

void com_freeFragments( const com_sigFrgCond_t *iCond )
{
    if( !iCond ) {COM_PRMNG();}
    com_sigFrgManage_t*  target = com_searchFragment( iCond );
    if( target ) {freeFragMng( target );}
}

static void freeAllFragInf( void )
{
    for( long i = 0;  i < gFrgInfCnt;  i++ ) {freeFragMng( &gFrgInf[i] );}
    com_free( gFrgInf );
}

static BOOL matchFrgCond(
        com_sigFrgCond_t *iCond1, const com_sigFrgCond_t *iCond2 )
{
    if( iCond1->type != iCond2->type || iCond1->id != iCond2->id ) {
        return false;
    }
    if( !com_matchSigBin( &iCond1->src, &iCond2->src ) ) {return false;}
    if( !com_matchSigBin( &iCond1->dst, &iCond2->dst ) ) {return false;}
    return true;
}

com_sigFrgManage_t *com_searchFragment( const com_sigFrgCond_t *iCond )
{
    if( !iCond ) {COM_PRMNG(NULL);}
    for( long i = 0;  i < gFrgInfCnt;  i++ ) {
        if( !gFrgInf[i].isUse ) {continue;}
        if( matchFrgCond( &gFrgInf[i].cond, iCond ) ) {return &(gFrgInf[i]);}
    }
    return NULL;
}

static com_sigFrgManage_t *getEmptyFrgInf( void )
{
    for( long i = 0;  i < gFrgInfCnt;  i++ ) {
        if( !gFrgInf[i].isUse ) {return &(gFrgInf[i]);}
    }
    return NULL;
}

static void initFrgInf(
        com_sigFrgManage_t *oInf, const com_sigFrgCond_t *iCond,
        com_sigBin_t *iSrc, com_sigBin_t *iDst )
{
    oInf->cond = (com_sigFrgCond_t){
        .type = iCond->type, .id = iCond->id, .src = *iSrc, .dst = *iDst
    };
    com_copySigBin( &oInf->cond.src, &iCond->src );
    com_copySigBin( &oInf->cond.dst, &iCond->dst );
    oInf->isUse = true;
}

static com_sigFrgManage_t *failGetFrg( com_sigBin_t *iSrc, com_sigBin_t *iDst )
{
    com_free( iSrc->top );
    com_free( iDst->top );
    return NULL;
}

static void createAddrInf(
        com_sigBin_t *oTarget, const com_sigBin_t *iSource, const char *iLabel )
{
    *oTarget = *iSource;
    oTarget->top = com_malloc( iSource->len, iLabel );
}

static com_sigFrgManage_t *getFrgInf( const com_sigFrgCond_t *iCond )
{
    com_sigFrgManage_t*  result = com_searchFragment( iCond );
    if( result ) {return result;}
    com_sigBin_t  src, dst;
    createAddrInf( &src, &iCond->src, "Fragment src address" );
    createAddrInf( &dst, &iCond->dst, "Fragment dst address" );
    if( !src.top || !dst.top ) {return failGetFrg( &src, &dst );}
    if( !(result = getEmptyFrgInf() ) ) {
        result = com_reallocAddr( &gFrgInf, sizeof(*gFrgInf),
                                  COM_TABLEEND, &gFrgInfCnt, 1,
                                  "Fragment Inf[%ld] type=%ld id=%ld",
                                  gFrgInfCnt, iCond->type, iCond->id );
        if( !result ) {return failGetFrg( &src, &dst );}
    }
    initFrgInf( result, iCond, &src, &dst );
    return result;
}

static com_sigFrg_t *stockNG( const com_sigFrgCond_t *iCond )
{
    com_freeFragments( iCond );
    return NULL;
}

com_sigFrg_t *com_stockFragments(
        const com_sigFrgCond_t *iCond, long iSeg, com_sigBin_t *iFrag )
{
    if( !iCond || !iFrag ) {COM_PRMNG(NULL);}
    com_sigFrgManage_t*  mng = getFrgInf( iCond );
    if( !mng ) {return NULL;}
    com_sigFrg_t*  frg = &(mng->data);
    com_sigSeg_t*  inf =
        com_reallocAddr( &(frg->inf), sizeof(*frg->inf),
                         COM_TABLEEND, &(frg->cnt), 1,
                         "Fragment segment[%ld] type=%ld id=%ld seg=%ld",
                         frg->cnt, iCond->type, iCond->id, iSeg );
    if( !inf ) {return stockNG( iCond );}
    if( !com_copySigBin( &inf->bin, iFrag ) ) {return stockNG( iCond );}
    inf->seg = iSeg;
    return frg;
}



// 初期化処理 ////////////////////////////////////////////////////////////////

static void finalizeAnalyzer( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    freeAllFragInf();
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}
//
void com_initializeAnalyzer( void )
{
    // com_initializeSignal()から呼ばれる限り、COM_DEBUG_AVOID_～は不要
    atexit( finalizeAnalyzer );
}

