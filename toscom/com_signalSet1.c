/*
 *****************************************************************************
 *
 * 共通部シグナル機能 (プロトコルセット1)    by TOS
 *
 *   外部公開I/Fの使用方法については com_signalSet1.h を参照。
 *
 *****************************************************************************
 */

#include <arpa/inet.h>     // inet_pton()のためにインクルード
#include "com_if.h"
#include "com_debug.h"
#include "com_signal.h"


// 内部共通処理 //////////////////////////////////////////////////////////////

static void checkUnknown(
        long *ioType, BOOL iDecode, const char *iLabel, long iValue )
{
    if( *ioType != COM_SIG_UNKNOWN ) {return;}
    if( iDecode ) {com_error( COM_ERR_NOSUPPORT, iLabel, iValue );}
    *ioType = COM_SIG_CONTINUE;
}

static BOOL analyzeSig(
        com_sigInf_t *ioHead, size_t iHeadSize, long iProto,
        com_sigInf_t *iOrgHead, const char *iLabel )
{
    if( !com_setHeadInf( ioHead, iHeadSize, iProto, COM_SIG_ONLYHEAD ) ) {
        return false;
    }
    return com_stackSigMulti( ioHead, iLabel,
                          &(com_sigBin_t){ iOrgHead->sig.top + iHeadSize,
                                           iOrgHead->sig.len - iHeadSize,
                                           iProto } );
}

static void decodeSig( com_sigInf_t *iHead, const char *iLabel, long iType )
{
    char  tmpLabel[32] = {0};
    snprintf( tmpLabel, sizeof(tmpLabel), "%s HEADER", iLabel );
    com_dispSig( tmpLabel, iType, &COM_ISG );
    if( !COM_IMLTICNT ) {return;}
    snprintf( tmpLabel, sizeof(tmpLabel), "%s BODY", iLabel );
    com_dispSig( tmpLabel, COM_NO_PTYPE, &COM_IMLTISTK[0].sig );
}



// リンク層ヘッダ ////////////////////////////////////////////////////////////

// 初期化用：次プロトコル値
static com_sigPrtclType_t  gLinkNext1[] = {
    { {ETHERTYPE_IP},     COM_SIG_IPV4 },
    { {ETHERTYPE_IPV6},   COM_SIG_IPV6 },
    { {ETHERTYPE_ARP},    COM_SIG_ARP },
    { {COM_PRTCLTYPE_END}, COM_SIG_UNKNOWN }
};

// 初期化用：VLANタグ値
static long  gVlanTag[] = {
    ETH_P_8021Q
};

enum { VLANTAG_SIZE = 4 };

BOOL com_analyzeEth2( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof(struct ether_header) );
    COM_CAST_HEAD( struct ether_header, eth2, COM_SGTOP );
    com_bin*  posType = (com_bin*)&(eth2->ether_type);
    com_off  vlan = 0;
    long  type = COM_SIG_UNKNOWN;
    long  ethType = com_getVal16( eth2->ether_type, COM_ORDER );
    while( !(type = com_getPrtclType( COM_LINKNEXT, ethType )) ) {
        if( !com_getPrtclType( COM_VLANTAG, ethType ) ) {break;}
        com_addPrmDirect( COM_APRM, ethType, VLANTAG_SIZE - COM_16BIT_SIZE,
                          posType + vlan + COM_16BIT_SIZE );
        vlan += VLANTAG_SIZE;
        ethType = com_getVal16( *((uint16_t*)(posType + vlan)), COM_ORDER );
    }
    checkUnknown( &type, iDecode,
                  "Ether2 not supported (ether type=%04x)", ethType );
    result = com_setHeadInf(ioHead, sizeof(*eth2) + vlan, COM_SIG_ETHER2, type);
    COM_ANALYZER_END;
}

void com_decodeEth2( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispSig( "Ether2 HEADER ", COM_SIG_ETHER2, &COM_ISG );
    com_off  vlan = 0;
    if( COM_IPRMCNT ) {
        for( long i = 0;  i < COM_IPRMCNT;  i++ ) {
            com_sigTlv_t*  tmp = &(COM_IPRMLST[i]);
            com_dispPrm( com_getVlanTagName( tmp->tag ), tmp->value, tmp->len );
            vlan += VLANTAG_SIZE;
        }
    }
    if( COM_INEXTCNT ) {
        COM_CAST_HEAD( struct ether_header, eth2, COM_ISGTOP );
        com_sigInf_t*  nextIp = COM_INEXTSTK;
        com_bin*  etype = (com_bin*)( &(eth2->ether_type) ) + vlan;
        com_dispNext( com_getVal16( *((uint16_t*)etype), COM_IORDER ),
                      sizeof( eth2->ether_type ), nextIp->sig.ptype );
    }
    COM_DECODER_END;
}

static com_decodeName_t  gVlanTagName[] = {
    { ETH_P_8021Q,         "802.1Q VLAN" },
    COM_DECODENAME_END  // 最後は必ずこれで
};

char *com_getVlanTagName( uint16_t iTag )
{
    char*  label = com_searchDecodeName( gVlanTagName, iTag, false );
    if( !label ) {label = "VLAN tag";}
    return label;
}


BOOL com_analyzeSll( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( com_sigSllHead_t ) );
    COM_CAST_HEAD( com_sigSllHead_t, sll, COM_SGTOP );
    long  sllPrt = com_getVal16( sll->protocol, COM_ORDER );
    long  type = com_getPrtclType( COM_LINKNEXT, sllPrt );
    checkUnknown( &type, iDecode,
                  "SLL not supported (protocol=%04x)", sllPrt );
    result = com_setHeadInf( ioHead, HDRSIZE, COM_SIG_SLL, type );
    COM_ANALYZER_END;
}

void com_decodeSll( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispSig( "SLL HEADER ", COM_SIG_SLL, &COM_ISG );
    if( COM_INEXTCNT ) {
        COM_CAST_HEAD( com_sigSllHead_t, sll, COM_ISGTOP );
        com_sigInf_t*  nextIp = COM_INEXTSTK;
        com_dispNext( com_getVal16( sll->protocol, COM_IORDER ),
                      sizeof( sll->protocol ), nextIp->sig.ptype );
    }
    COM_DECODER_END;
}



// IP ////////////////////////////////////////////////////////////////////////

// 初期化用：次プロトコル値
static com_sigPrtclType_t  gIpNext1[] = {
    { {IPPROTO_TCP},      COM_SIG_TCP },
    { {IPPROTO_UDP},      COM_SIG_UDP },
    { {IPPROTO_SCTP},     COM_SIG_SCTP },
    { {IPPROTO_ICMP},     COM_SIG_ICMP },
    { {IPPROTO_ICMPV6},   COM_SIG_ICMPV6 },
    { {IPPROTO_IP},       COM_SIG_IPV4 },
    { {IPPROTO_IPV6},     COM_SIG_IPV6 },
    { {COM_PRTCLTYPE_END}, COM_SIG_UNKNOWN }
};

enum { OPT_VARLEN = -1 };

//初期化用：IPv4オプション
static com_sigPrtclType_t  gIpOptLen1[] = {
    { {IPOPT_EOL},        0 },           // End of Option List
    { {IPOPT_NOP},        0 },           // No Operation
    { {IPOPT_SECURITY},  11 },           // Security
    { {IPOPT_LSRR},     OPT_VARLEN },    // Loose Source Routing
    { {IPOPT_SSRR},     OPT_VARLEN },    // Strict Source Routing
    { {IPOPT_RR},       OPT_VARLEN },    // Record Route
    { {IPOPT_SATID},      4 },           // Stream ID
    { {IPOPT_TS},       OPT_VARLEN },    // Internet Timestamp
    { {IPOPT_RA},         4 },           // IP Router Alert [RFC2113]
    { {COM_PRTCLTYPE_END}, COM_SIG_UNKNOWN }
};

// 初期化用：IPv6拡張ヘッダ
static long  gIpv6Ext[] = {
    IPPROTO_HOPOPTS, IPPROTO_ROUTING, IPPROTO_FRAGMENT, IPPROTO_NONE,
    IPPROTO_DSTOPTS
};


///// IPv4 (幾つかの関数は IPv6でも使用する) /////

enum { OCT_UNIT = 8 };

#define IPV4_HDRSIZE( IPHDR ) \
    (IPHDR)->ip_hl * COM_32BIT_SIZE

static BOOL getIpv4FragInf(
        com_sigInf_t *iHead, struct ip *iIpv4,
        BOOL *oDF, BOOL *oMF, BOOL *oOff )
{
    uint16_t  frags = com_getVal16( iIpv4->ip_off, COM_IORDER );
    COM_SET_IF_EXIST( oDF, COM_CHECKBIT( frags, IP_DF ) );
    COM_SET_IF_EXIST( oMF, COM_CHECKBIT( frags, IP_MF ) );
    COM_SET_IF_EXIST( oOff, (frags & IP_OFFMASK) * OCT_UNIT );
    return COM_CHECKBIT( frags, IP_DF );
    // oDFは NULLを許容するため、その内容を返すことはしない
}

static com_bin *combineSegments( com_sigFrg_t *iFrg )
{
    com_bin*  top = com_malloc( iFrg->segMax, "reassemble IP" );
    if( !top ) {return NULL;}
    for( long i = 0;  i < iFrg->cnt;  i++ ) {
        com_sigSeg_t*  frg = &(iFrg->inf[i]);
        memcpy( top + frg->seg, frg->bin.top, frg->bin.len );
    }
    return top;
}

static COM_SIG_FRG_t judgeTotalSize( com_sigFrg_t *iFrg )
{
    if( !iFrg->segMax ) {return COM_FRG_SEG;}  // 最初の断片
    long  totalSize = 0;
    for( long i = 0;  i < iFrg->cnt;  i++ ) {
        totalSize += iFrg->inf[i].bin.len;
    }
    if( totalSize < iFrg->segMax ) {return COM_FRG_SEG;}  // まだ断片揃わず
    if( totalSize > iFrg->segMax ) {
        com_error( COM_ERR_ILLSIZE, "IPv4 reassemble size is too big" );
        return COM_FRG_ERROR;
    }
    return COM_FRG_REASM;  // 全断片収集完了->結合
}

static COM_SIG_FRG_t checkReasmResult(
        COM_SIG_FRG_t iResult, com_sigFrgCond_t *iCond )
{
    if( iResult == COM_FRG_REASM || iResult == COM_FRG_ERROR ) {
        com_freeFragments( iCond );
    }
    return iResult;
}

static COM_SIG_FRG_t reassembleFragments(
        com_sigFrg_t *iFrg, com_sigBin_t *oRas, com_sigFrgCond_t *iCond )
{
    COM_SIG_FRG_t  result = judgeTotalSize( iFrg );
    if( result != COM_FRG_REASM ) {return checkReasmResult( result, iCond );}
    com_bin*  reasm = combineSegments( iFrg );
    if( !reasm ) {return checkReasmResult( COM_FRG_ERROR, iCond );}
    *oRas = (com_sigBin_t){ reasm, iFrg->segMax, 0 };
    return checkReasmResult( COM_FRG_REASM, iCond );
}

static void getIpv4Fragment(
        com_sigBin_t *oBody, com_sigInf_t *ioHead, struct ip *iIpv4 )
{
    size_t  hdrSize = IPV4_HDRSIZE( iIpv4 );
    oBody->top = COM_SGTOP + hdrSize;
    oBody->len = com_getVal16( iIpv4->ip_len, COM_ORDER ) - hdrSize;
}

static void setIpv4FragCond( com_sigFrgCond_t *oCond, struct ip *iIpv4 )
{
    *oCond = (com_sigFrgCond_t){
        COM_SIG_IPV4,
        iIpv4->ip_id,
        {(com_bin*)(&iIpv4->ip_src), sizeof(iIpv4->ip_src), 0},
        {(com_bin*)(&iIpv4->ip_dst), sizeof(iIpv4->ip_dst), 0},
        iIpv4->ip_p,
        0
    };
}

static BOOL existRas(
        com_sigInf_t *ioHead, com_sigBin_t *oRas, com_sigFrgCond_t *iCond,
        long iType )
{
    if( ioHead->ras.ptype != iType ) {return false;}
    *oRas = ioHead->ras;
    com_freeFragments( iCond );
    return true;
}

static COM_SIG_FRG_t procIpv4Fragment(
        com_sigInf_t *ioHead, struct ip *iIpv4, com_sigBin_t *oRas )
{
    BOOL  df, mf;
    long  fragOff;
    if( getIpv4FragInf( ioHead,iIpv4,&df,&mf,&fragOff ) ) {return COM_FRG_OK;}
    com_sigBin_t  ipBody;
    getIpv4Fragment( &ipBody, ioHead, iIpv4 );
    com_sigFrgCond_t  cond;
    setIpv4FragCond( &cond, iIpv4 );
    com_sigFrg_t*  frg = com_stockFragments( &cond, fragOff, &ipBody );
    if( !frg ) {return COM_FRG_ERROR;}
    if( !mf ) {
        if( !fragOff ) {com_freeFragments( &cond );  return COM_FRG_OK;}
        if( existRas( ioHead,oRas,&cond,COM_SIG_IPV4 ) ) {return COM_FRG_REASM;}
        frg->segMax = fragOff + com_getVal16( iIpv4->ip_len, COM_ORDER )
                      - IPV4_HDRSIZE( iIpv4 );
    }
    return reassembleFragments( frg, oRas, &cond );
}

static void getPrtOptions(
        COM_PRTCLTYPE_t iType, com_sigInf_t *ioHead, com_off iHdrLen,
        com_off iHdrBaseSize )
{
    com_bin*  ptr = COM_SGTOP;
    com_advancePtr( &ptr, &iHdrLen, iHdrBaseSize );
    while( *ptr && iHdrLen ) {  // 0x00(End of Option List)は一覧に含めない
        com_bin  opType = *ptr;
        com_bin*  opVal = NULL;
        long  opLen = com_getPrtclType( iType, opType );
        long  opValLen = opLen;  // 01(Nop) と レングス2で値無しを区別する為
        if( opLen == OPT_VARLEN ) {opLen = *(ptr + 1);}
        if( opLen ) {opValLen -= 2;  opVal = ptr + 2;}
        com_addPrmDirect( COM_APRM, opType, opValLen, opVal );
        if( !opLen ) {if( !com_advancePtr( &ptr,&iHdrLen,1 ) ) {return;}}
        else {if( !com_advancePtr( &ptr,&iHdrLen,opLen ) ) {return;}}
    }
}

static BOOL setRasToNext( com_sigInf_t *ioHead, com_sigBin_t *iRas, long iType )
{
    com_sigInf_t  body;
    com_initSigInf( &body, ioHead );
    body.ras = *iRas;
    body.ras.ptype *= -1;
    body.sig = body.ras;
    body.sig.ptype = iType;
    return (NULL != com_stackSigNext( ioHead, &body ));
}

static BOOL setIpv4Inf(
        COM_SIG_FRG_t iFrgRet, com_sigInf_t *ioHead, com_off iHdrLen,
        com_sigBin_t *iFrgRas, long iProto )
{
    if( iFrgRet == COM_FRG_REASM ) {
        (void)com_setHeadInf( ioHead, iHdrLen, COM_SIG_IPV4, COM_SIG_ONLYHEAD );
        return setRasToNext( ioHead, iFrgRas, iProto );
    }
    BOOL  result = com_setHeadInf( ioHead, iHdrLen, COM_SIG_IPV4, iProto );
    if( iFrgRet == COM_FRG_SEG && COM_NEXTCNT ) {
        COM_NEXTSTK[0].isFragment = true;
    }
    return result;
}

BOOL com_analyzeIpv4( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( struct ip ) );
    COM_CAST_HEAD( struct ip, ipv4, COM_SGTOP );
    com_off  hdrLen = IPV4_HDRSIZE( ipv4 );
    long  protocol = com_getPrtclType( COM_IPNEXT, ipv4->ip_p );
    com_sigBin_t  frgRas = { .top = NULL };
    COM_SIG_FRG_t  frgRet = procIpv4Fragment( ioHead, ipv4, &frgRas );
    if( frgRet != COM_FRG_ERROR ) {
        checkUnknown( &protocol, iDecode,
                      "IPv4 not suppoted (protocol=%02x)", ipv4->ip_p );
        getPrtOptions( COM_IPOPTLEN, ioHead, hdrLen, HDRSIZE );
        result = setIpv4Inf( frgRet, ioHead, hdrLen, &frgRas, protocol );
    }
    COM_ANALYZER_END;
}

static void dispIpv4FragInf( com_sigInf_t *iHead, struct ip *iIpv4 )
{
    BOOL  df, mf;
    long  fragOff;
    (void)getIpv4FragInf( iHead, iIpv4, &df, &mf, &fragOff );
    com_dispVal( "don't fragment", df );
    com_sigInf_t*  trans = COM_INEXTSTK;
    if( !df ) {
        com_dispVal( " more fragment", mf );
        com_dispVal( "fragment offset", fragOff );
        if( trans->ras.top ) {
            com_dispDec( "   <reassembled (size=%zu)>", trans->ras.len );
        }
    }
}

typedef char*(com_getOptNameFunc_t)( long iType );

static void dispPrtOptions( com_sigPrm_t *iPrm, com_getOptNameFunc_t func )
{
    for( long i = 0;  i < iPrm->cnt;  i++ ) {
        com_sigTlv_t*  tlv = &(iPrm->list[i]);
        com_printf( "#    (opt)%s", (func)( tlv->tag ) );
        for( com_off j = 0;  j < tlv->len;  j++ ) {
            if( !j ) {com_printf( " = 0x" );}
            com_printf( "%02x", tlv->value[j] );
        }
        com_printLf();
    }
}

static void dispIpNext(
        long iProto, size_t iSize, com_sigInf_t *iNext, BOOL isV6 )
{
    long  code = COM_SIG_UNKNOWN;
    if( isV6 && com_getPrtclType( COM_IP6XHDR, iProto ) ) {
        code = COM_SIG_EXTENSION;
    }
    else {
        if( iNext ) {code = com_getSigType( iNext );}
        else {code = com_getPrtclType( COM_IPNEXT, iProto );}
    }
    com_dispNext( iProto, iSize, code );
}

void com_decodeIpv4( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispSig( "IPv4 HEADER ", COM_SIG_IPV4, &COM_ISG );
    COM_CAST_HEAD( struct ip, ipv4, COM_ISGTOP );
    com_dispBin( "src IP", &ipv4->ip_src, COM_32BIT_SIZE, ".", false );
    com_dispBin( "dst IP", &ipv4->ip_dst, COM_32BIT_SIZE, ".", false );
    com_dispPrm( "identification", &ipv4->ip_id, COM_16BIT_SIZE );
    dispIpv4FragInf( iHead, ipv4 );
    if( COM_IPRMCNT ) {dispPrtOptions( COM_IAPRM, com_getIpv4OptName );}
    dispIpNext( ipv4->ip_p, sizeof(ipv4->ip_p), COM_INEXTSTK, false );
    COM_DECODER_END;
}

static com_decodeName_t  gIPv4OptName[] = {
    { IPOPT_EOL,         "End of Option List" },
    { IPOPT_NOP,         "No Operations" },
    { IPOPT_SECURITY,    "Security" },
    { IPOPT_LSRR,        "Loose Source Routing" },
    { IPOPT_SSRR,        "Strict Source Routing" },
    { IPOPT_RR,          "Record Route" },
    { IPOPT_SATID,       "Stream ID" },
    { IPOPT_TS,          "Internet Timestamp" },
    { IPOPT_RA,          "IP Router Alert" },
    COM_DECODENAME_END  // 最後は必ずこれで
};

char *com_getIpv4OptName( long iType )
{
    return com_searchDecodeName( gIPv4OptName, iType, true );
}

///// IPv6 /////

enum {
    IPV6EXTHDR_SIZE  = 8,    // IPv6拡張ヘッダサイズ
    IPV6EXT_UNITSIZE = 8     // IPv5拡張ヘッダレングス単位(8oct)
};

static com_sigTlv_t *getIpv6ExtHdr( com_sigInf_t *ioHead, long iHdrType )
{
    return com_searchPrm( COM_APRM, (com_off)iHdrType );
}

static void getIpv6FragInf(
        void *iFrg, BOOL iOrder, BOOL *oMf, long *oOff, long *oId )
{
    COM_CAST_HEAD( struct ip6_frag, frg, iFrg );
    uint16_t  offlg = com_getVal16( frg->ip6f_offlg, iOrder );
    COM_SET_IF_EXIST( oOff, ((offlg & IP6F_OFF_MASK) >> 3) * OCT_UNIT );
    COM_SET_IF_EXIST( oMf, COM_CHECKBIT( offlg, IP6F_MORE_FRAG ) );
    COM_SET_IF_EXIST( oId, com_getVal32( frg->ip6f_ident, iOrder ) );
}

static void getIpv6Fragment(
        com_sigBin_t *oBody, com_sigInf_t *iHead, struct ip6_hdr *iIpv6,
        com_off iHdrSize )
{
    *oBody = (com_sigBin_t){
        .top = COM_ISGTOP + iHdrSize,
        .len = com_getVal16( iIpv6->ip6_plen, COM_IORDER )
               - ( iHdrSize - sizeof(*iIpv6) )
    };
}

static void setIpv6FragCond(
        com_sigFrgCond_t *oCond, struct ip6_hdr *iIpv6, long iFragId )
{
    *oCond = (com_sigFrgCond_t){
        COM_SIG_IPV6,
        iFragId,
        {(com_bin*)&(iIpv6->ip6_src), sizeof(iIpv6->ip6_src), 0},
        {(com_bin*)&(iIpv6->ip6_dst), sizeof(iIpv6->ip6_dst), 0},
        iIpv6->ip6_nxt,
        0
    };
}

static COM_SIG_FRG_t procIpv6Fragment(
        com_sigInf_t *ioHead, com_off iHdrSize, com_sigBin_t *oRas )
{
    com_sigTlv_t*  frgHdr = getIpv6ExtHdr( ioHead, IPPROTO_FRAGMENT );
    if( !frgHdr ) {return COM_FRG_OK;}
    BOOL  mf = false;
    long  fragOff, fragId;
    getIpv6FragInf( frgHdr->value, ioHead->order, &mf, &fragOff, &fragId );
    COM_CAST_HEAD( struct ip6_hdr, ipv6, COM_SGTOP );
    com_sigBin_t  ipBody;
    getIpv6Fragment( &ipBody, ioHead, ipv6, iHdrSize );
    com_sigFrgCond_t  cond;
    setIpv6FragCond( &cond, ipv6, fragId );
    com_sigFrg_t*  frg = com_stockFragments( &cond, fragOff, &ipBody );
    if( !frg ) {return COM_FRG_ERROR;}
    if( !mf ) {
        if( existRas( ioHead,oRas,&cond,COM_SIG_IPV6 ) ) {return COM_FRG_REASM;}
        frg->segMax = fragOff + ipBody.len;
    }
    return reassembleFragments( frg, oRas, &cond );
}

static com_off getIpv6Head(
        com_sigInf_t *ioHead, long *oNextProto, COM_SIG_FRG_t *oFrgRet,
        com_sigBin_t *oRas )
{
    com_bin*  ptr = COM_SGTOP;
    COM_CAST_HEAD( struct ip6_hdr, ipv6, ptr );
    com_off  hdrSize = sizeof(*ipv6);
    ptr += hdrSize;
    *oNextProto = ipv6->ip6_nxt;
    while( com_getPrtclType( COM_IP6XHDR, *oNextProto ) ) {
        if( COM_SGLEN <= (com_off)(ptr - COM_SGTOP) ) {break;}
        if( *oNextProto == IPPROTO_NONE ) {break;}
        COM_CAST_HEAD( struct ip6_ext, extHdr, ptr );
        com_off  size = IPV6EXTHDR_SIZE + extHdr->ip6e_len * IPV6EXT_UNITSIZE;
        com_addPrmDirect( COM_APRM, *oNextProto, size, ptr );
        *oNextProto = extHdr->ip6e_nxt;
        hdrSize += size;
        ptr += size;
    }
    *oFrgRet = procIpv6Fragment( ioHead, hdrSize, oRas );
    return hdrSize;
}

static long setIpv6Next( com_sigInf_t *ioHead, long iNxt )
{
    if( com_getPrtclType( COM_IP6XHDR, iNxt ) ) {return COM_SIG_EXTENSION;}
    if( getIpv6ExtHdr( ioHead, IPPROTO_NONE ) ) {return COM_SIG_END;}
    return com_getPrtclType( COM_IPNEXT, iNxt );
}

static BOOL setIpv6Inf(
        COM_SIG_FRG_t iFrgRet, com_sigInf_t *ioHead, com_off iHdrLen,
        com_sigBin_t *iFrgRas, long iProto )
{
    if( iFrgRet == COM_FRG_REASM ) {
        (void)com_setHeadInf( ioHead, iHdrLen, COM_SIG_IPV6, COM_SIG_ONLYHEAD );
        return setRasToNext( ioHead, iFrgRas, iProto );
    }
    BOOL result = com_setHeadInf( ioHead, iHdrLen, COM_SIG_IPV6, iProto );
    if( iFrgRet == COM_FRG_SEG && COM_NEXTSTK ) {
        COM_NEXTSTK[0].isFragment = true;
    }
    return result;
}

BOOL com_analyzeIpv6( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( struct ip6_hdr ) );
    long  nxt = COM_SIG_UNKNOWN;
    COM_SIG_FRG_t  frgRet = COM_FRG_OK;
    com_sigBin_t   frgRas = { .top = NULL };
    com_off  hdrLen = getIpv6Head( ioHead, &nxt, &frgRet, &frgRas );
    if( hdrLen ) {
        long  proto = setIpv6Next( ioHead, nxt );
        checkUnknown( &proto, iDecode,
                      "IPv6 not supported (protocol=%02x)", nxt );
        result = setIpv6Inf( frgRet, ioHead, hdrLen, &frgRas, proto );
    }
    COM_ANALYZER_END;
}

enum { IPV6ADDR_16BIT_COUNT = 8 };

static void dispV6Addr( BOOL iSrc, struct ip6_hdr *iIpv6, BOOL iOrder )
{
    char*  label = iSrc ? "src" : "dst";
    struct in6_addr  addr = iSrc ? iIpv6->ip6_src : iIpv6->ip6_dst;
    com_printf( "#    %s IP = ", label );
    for( long i = 0;  i < IPV6ADDR_16BIT_COUNT;  i++ ) {
        com_printf( "%04x", com_getVal16( addr.s6_addr16[i], iOrder ) );
        if( i < (IPV6ADDR_16BIT_COUNT - 1) ) {com_printf( ":" );}
    }
    com_printLf();
}

#define SETPTRLEN( ADVANCE ) \
    com_bin*  ptr = iExt->value; \
    com_off   len = iExt->len; \
    if( ADVANCE ) {com_advancePtr( &ptr, &len, sizeof(struct ip6_ext) );} \
    do{} while(0)

static void dispIp6extOptions( com_sigTlv_t *iExt, com_sigInf_t *iHead )
{
    COM_UNUSED( iHead );
    SETPTRLEN( iExt );
    while( *ptr && len ) {
        com_printf( "#     type=%02x len=%02x ", ptr[0], ptr[1] );
        for( com_off i = 0;  i < ptr[1];  i++ ) {
            com_printf( "%02x", ptr[2 + i] );
        }
        com_printLf();
        com_advancePtr( &ptr, &len, ptr[1] + 2 );
    }
}

static void dispIp6extRouting( com_sigTlv_t *iExt, com_sigInf_t *iHead )
{
    COM_UNUSED( iHead );
    SETPTRLEN( iExt );
    com_dispDec( "      routing type = %02x", ptr[0] );
    com_dispDec( "     segments left = %02x", ptr[1] );
}

static void dispIp6extFragment( com_sigTlv_t *iExt, com_sigInf_t *iHead )
{
    BOOL  mf = false;
    long  fragOff, fragId;
    getIpv6FragInf( iExt->value, COM_IORDER, &mf, &fragOff, &fragId );
    com_dispDec( "     fragment offset = %ld", fragOff );
    com_dispDec( "      more fragment = %ld", mf );
    com_dispDec( "     identification = %08lx", fragId );
    if( !COM_INEXTCNT ) {return;}
    com_sigInf_t*  trans = COM_INEXTSTK;
    if( !mf && trans->ras.top ) {
        com_dispDec( "     <reassembled (size=%zu)>", trans->ras.len );
    }
}

static void dispIpv6ExtHdr( com_sigInf_t *iHead, com_sigTlv_t *iExt )
{
    if( iExt->tag == IPPROTO_HOPOPTS ) {dispIp6extOptions( iExt, iHead );}
    if( iExt->tag == IPPROTO_ROUTING ) {dispIp6extRouting( iExt, iHead );}
    if( iExt->tag == IPPROTO_FRAGMENT ) {dispIp6extFragment( iExt, iHead );}
    if( iExt->tag == IPPROTO_DSTOPTS ) {dispIp6extOptions( iExt, iHead );}
}

static uint8_t checkIpv6ExtHdr( com_sigInf_t *iHead, struct ip6_hdr *iIpv6 )
{
    uint8_t nxt = iIpv6->ip6_nxt;
    for( long i = 0;  i < COM_IPRMCNT;  i++ ) {
        com_sigTlv_t*  ext = &(COM_IPRMLST[i]);
        com_dispSig( "IPv6 EXT HEADER", COM_NO_PTYPE,
                     &(com_sigBin_t){ ext->value, ext->len, 0 } );
        com_dispPrm( "type", com_getIpv6ExtHdrName( ext->tag ), 0 );
        dispIpv6ExtHdr( iHead, ext );
        COM_CAST_HEAD( struct ip6_ext, extHdr, ext->value );
        nxt = extHdr->ip6e_nxt;
    }
    return nxt;
}

void com_decodeIpv6( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_sigBin_t  ipv6hdr = COM_ISG;
    ipv6hdr.len = sizeof(struct ip6_hdr);  // まずは基本ヘッダのみダンプ
    com_dispSig( "IPv6 HEADER ", COM_SIG_IPV6, &ipv6hdr );
    COM_CAST_HEAD( struct ip6_hdr, ipv6, COM_ISGTOP );
    com_dispPrm( "flow-ID", &ipv6->ip6_flow, COM_32BIT_SIZE );
    dispV6Addr( true,  ipv6, COM_IORDER );
    dispV6Addr( false, ipv6, COM_IORDER );
    uint8_t  nxt = checkIpv6ExtHdr( iHead, ipv6 );
    dispIpNext( nxt, sizeof(nxt), COM_INEXTSTK, true );
    COM_DECODER_END;
}

static com_decodeName_t  gIpv6ExtHdrName[] = {
    { IPPROTO_HOPOPTS,  "Hop-by-Hop Options" },
    { IPPROTO_ROUTING,  "Routing" },
    { IPPROTO_FRAGMENT, "Fragment" },
    { IPPROTO_DSTOPTS,  "Destination Options" },
    COM_DECODENAME_END  // 最後は必ずこれで
};

char *com_getIpv6ExtHdrName( long iCode )
{
    return com_searchDecodeName( gIpv6ExtHdrName, iCode, true );
}

// SCTP解析時に下位スタックのIPヘッダからコンテンツ長を取得
static com_off getIpConSize( com_sigInf_t *iHead )
{
    com_sigInf_t*  prev = iHead->prev;
    if( !prev ) {
        // sctpから解析開始時は sctpヘッダ以降全てを解析対象とする
        return (com_off)(COM_ISGTOP - sizeof(com_sigSctpCommonHdr_t));
    }
    if( prev->sig.ptype == COM_SIG_IPV4 ) {
        COM_CAST_HEAD( struct ip, ipv4, prev->sig.top );
        return (com_off)(com_getVal16( ipv4->ip_len,prev->order )
                         - IPV4_HDRSIZE(ipv4));
    }
    if( prev->sig.ptype == COM_SIG_IPV6 ) {
        COM_CAST_HEAD( struct ip6_hdr, ipv6, prev->sig.top );
        return (com_off)(com_getVal16( ipv6->ip6_plen, prev->order )
                         - prev->sig.len + sizeof(*ipv6));
    }
    com_error( COM_ERR_NOSUPPORT,
               "not IP packet(type=%ld)", prev->sig.ptype );
    return 0;
}



// ICMP //////////////////////////////////////////////////////////////////////

BOOL com_analyzeIcmp( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( struct icmphdr ) );
    COM_CAST_HEAD( struct icmphdr, icmp, COM_SGTOP );
    if( com_getIcmpTypeName( icmp ) ) {
        result = analyzeSig( ioHead, HDRSIZE, COM_SIG_ICMP, &orgHead,
                             "icmp body" );
    } 
    COM_ANALYZER_END;
}

void com_decodeIcmp( COM_DECODER_PRM )
{
    COM_DECODER_START;
    COM_CAST_HEAD( struct icmphdr, icmp, COM_ISGTOP );
    decodeSig( iHead, "ICMP", COM_SIG_ICMP );
    if( icmp->type == ICMP_DEST_UNREACH ) {
        com_dispDec( " <<< ICMP BODY Decode Result Start >>>" );
        com_sigInf_t*  body = COM_IMLTISTK;
        body->sig.ptype = COM_SIG_IPV4;
        com_analyzeSignalToLast( body, true );
        com_dispDec( " <<< ICMP BODY Decode Result End >>>" );
    }
    com_dispPrm( "type", com_getIcmpTypeName( icmp ), 0 );
    COM_DECODER_END;
}

static com_decodeName_t  gIcmpTypeName[] = {
    { ICMP_ECHOREPLY,        "Echo Reply" },
    { ICMP_DEST_UNREACH,     "Destination Unreachable" },
    { ICMP_SOURCE_QUENCH,    "Source Quench" },
    { ICMP_REDIRECT,         "Redirect (change route)" },
    { ICMP_ECHO,             "Echo Request" },
    { ICMP_TIME_EXCEEDED,    "Time Exceeded" },
    { ICMP_PARAMETERPROB,    "Parameter Problem" },
    { ICMP_TIMESTAMP,        "Timestamp Request" },
    { ICMP_TIMESTAMPREPLY,   "Timestamp Reply" },
    { ICMP_INFO_REQUEST,     "Information Request" },
    { ICMP_INFO_REPLY,       "Information Reply" },
    { ICMP_ADDRESS,          "Address Mask Request" },
    { ICMP_ADDRESSREPLY,     "Address Mask Reply" },
    COM_DECODENAME_END  // 最後は必ずこれで
};

char *com_getIcmpTypeName( void *iSigTop )
{
    struct icmphdr*  icmp = iSigTop;
    return com_searchDecodeName( gIcmpTypeName, icmp->type, true );
}



// ICMPv6 ////////////////////////////////////////////////////////////////////

BOOL com_analyzeIcmpv6( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( struct icmp6_hdr ) );
    COM_CAST_HEAD( struct icmp6_hdr, icmp6, COM_SGTOP );
    if( com_getIcmpv6TypeName( icmp6 ) ) {
        result = analyzeSig( ioHead, HDRSIZE, COM_SIG_ICMPV6, &orgHead,
                             "icmpV6 body" );
    } 
    COM_ANALYZER_END;
}

void com_decodeIcmpv6( COM_DECODER_PRM )
{
    COM_DECODER_START;
    COM_CAST_HEAD( struct icmp6_hdr, icmp6, COM_ISGTOP );
    decodeSig( iHead, "ICMPv6", COM_SIG_ICMPV6 );
    com_dispPrm( "type", com_getIcmpv6TypeName( icmp6 ), 0 );
    COM_DECODER_END;
}

static com_decodeName_t  gIcmpv6TypeName[] = {
    { ICMP6_DST_UNREACH,       "Destination Unreachable" },
    { ICMP6_PACKET_TOO_BIG,    "Packet Too Big" },
    { ICMP6_TIME_EXCEEDED,     "Time Exceeded" },
    { ICMP6_PARAM_PROB,        "Parameter Problem" },
    { ICMP6_ECHO_REQUEST,      "Echo Request" },
    { ICMP6_ECHO_REPLY,        "Echo Reply" },
    // 以下 RFC2461 にて規定
    { ND_ROUTER_SOLICIT,       "Router Solicitation" },
    { ND_ROUTER_ADVERT,        "Router Advertisement" },
    { ND_NEIGHBOR_SOLICIT,     "Neighbor Solicitation" },
    { ND_NEIGHBOR_ADVERT,      "Neighbor Advertisement" },
    COM_DECODENAME_END  // 最後は必ずこれで
};

char *com_getIcmpv6TypeName( void *iSigTop )
{
    struct icmp6_hdr*  icmp6 = iSigTop;
    return com_searchDecodeName( gIcmpv6TypeName, icmp6->icmp6_type, true );
}


// ARP ///////////////////////////////////////////////////////////////////////

BOOL com_analyzeArp( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( struct arphdr ) );
    COM_CAST_HEAD( struct arphdr, arp, COM_SGTOP );
    if( com_getArpOpName( arp, COM_ORDER ) ) {
        result = analyzeSig( ioHead, HDRSIZE, COM_SIG_ARP, &orgHead,
                             "arp body" );
    } 
    COM_ANALYZER_END;
}

void com_decodeArp( COM_DECODER_PRM )
{
    COM_DECODER_START;
    COM_CAST_HEAD( struct arphdr, arp, COM_ISGTOP );
    decodeSig( iHead, "ARP", COM_SIG_ARP );
    com_dispPrm( "opcode", com_getArpOpName( arp, COM_IORDER ), 0 );
    COM_DECODER_END;
}

static com_decodeName_t  gArpOpName[] = {
    { ARPOP_REQUEST,       "ARP request" },
    { ARPOP_REPLY,         "ARP_reply" },
    { ARPOP_RREQUEST,      "RARP request" },
    { ARPOP_RREPLY,        "RARP_reply" },
    { ARPOP_InREQUEST,     "InARP request" },
    { ARPOP_InREPLY,       "InARP_reply" },
    COM_DECODENAME_END  // 最後は必ずこれで
};

char *com_getArpOpName( void *iSigTop, BOOL iOrder )
{
    struct arphdr*  arp = iSigTop;
    return com_searchDecodeName( gArpOpName,
                                 com_getVal16( arp->ar_op, iOrder ), true );
}



// TCP ///////////////////////////////////////////////////////////////////////

// 初期化用：TCPオプション長
static com_sigPrtclType_t  gTcpOptLen1[] = {
    { {TCPOPT_EOL},        0 },                // End of Options List
    { {TCPOPT_NOP},        0 },                // No Operation
    { {TCPOPT_MAXSEG},  TCPOLEN_MAXSEG },      // Maximum Segment Size (4)
    { {TCPOPT_WINDOW},  TCPOLEN_WINDOW },      // Window Scale (3)
    { {TCPOPT_SACK_PERMITTED}, TCPOLEN_SACK_PERMITTED }, // SACK Permitted (2)
    { {TCPOPT_SACK},    OPT_VARLEN },          // SACK
    { {TCPOPT_TIMESTAMP}, TCPOLEN_TIMESTAMP }, // Time Stamp (10)
    { {COM_PRTCLTYPE_END}, COM_SIG_UNKNOWN }
};

typedef void(*com_setAddrFunc_t)(
        com_sigTcpNode_t *oNode, com_sigInf_t *ioHead );

static void setV4Addr( com_sigTcpNode_t *oNode, com_sigInf_t *ioHead )
{
    COM_CAST_HEAD( struct ip, ipv4, ioHead->prev->sig.top );
    memcpy( oNode->src.addr, &ipv4->ip_src, sizeof(ipv4->ip_src) );
    memcpy( oNode->dst.addr, &ipv4->ip_dst, sizeof(ipv4->ip_dst) );
}

static void setV6Addr( com_sigTcpNode_t *oNode, com_sigInf_t *ioHead )
{
    COM_CAST_HEAD( struct ip6_hdr, ipv6, ioHead->prev->sig.top );
    memcpy( oNode->src.addr, &ipv6->ip6_src, sizeof(ipv6->ip6_src) );
    memcpy( oNode->dst.addr, &ipv6->ip6_dst, sizeof(ipv6->ip6_dst) );
}

static void setTcpPort( com_sigTcpNode_t *oNode, com_sigInf_t *ioHead )
{
    COM_CAST_HEAD( struct tcphdr, tcp, COM_SGTOP );
    oNode->src.port = com_getVal16( tcp->th_sport, COM_ORDER );
    oNode->dst.port = com_getVal16( tcp->th_dport, COM_ORDER );
}

static BOOL makeNodeInf( com_sigTcpNode_t *oNode, com_sigInf_t *ioHead )
{
    memset( oNode, 0, sizeof(*oNode) );
    if( !ioHead->prev ) {return false;}
    com_setAddrFunc_t  func = NULL;
    long  ipType = ioHead->prev->sig.ptype;
    if( ipType == COM_SIG_IPV4 ) {func = setV4Addr;}
    else if( ipType == COM_SIG_IPV6 ) {func = setV6Addr;}
    else {return false;}
    (func)( oNode, ioHead );
    setTcpPort( oNode, ioHead );
    return true;
}

static com_hashId_t  gHashTcp = COM_HASHID_NOTREG;

static void freeTcpNodeInf( void )
{
    if( gHashTcp == COM_HASHID_NOTREG ) {return;}
    com_cancelHash( gHashTcp );
}

static const com_sigTcpInf_t *searchTcpNode( com_sigTcpNode_t *ioNode )
{
    if( gHashTcp == COM_HASHID_NOTREG ) {
        gHashTcp = com_registerHash( 11, NULL );
        return NULL;
    }
    const void*  data = NULL;
    if( !com_searchHash( gHashTcp, ioNode, sizeof(*ioNode), &data, NULL ) ) {
        com_sigTcpNode_t  tmp = { ioNode->dst, ioNode->src, false };
        if( !com_searchHash( gHashTcp, &tmp, sizeof(tmp), &data, NULL ) ) {
            return NULL;
        }
        ioNode->isReverse = true;
    }
    return data;
}

static BOOL addTcpHash(
        BOOL iOverwrite, com_sigTcpNode_t *iNode, com_sigTcpInf_t *iInf )
{
    COM_HASH_t  result = COM_HASH_NG;
    com_sigTcpNode_t  tmp = *iNode;
    if( iNode->isReverse ) {
        tmp = (com_sigTcpNode_t){ iNode->dst, iNode->src, false };
    }
    result = com_addHash( gHashTcp, iOverwrite,
                          &tmp, sizeof(tmp), iInf, sizeof(*iInf) );
    if( result == COM_HASH_NG || result == COM_HASH_EXIST ) {return false;}
    return true;
} 

static void getTcpSeq( ulong *oSeq, ulong *oAck, com_sigInf_t *ioHead )
{
    COM_CAST_HEAD( struct tcphdr, tcp, COM_SGTOP );
    *oSeq = com_getVal32( tcp->th_seq, COM_ORDER );
    *oAck = com_getVal32( tcp->th_ack, COM_ORDER );
}

static void addRecvSize(
        com_sigTcpNode_t *iNode, com_sigTcpInf_t *oInf, com_off iDataSize )
{
    if( iNode->isReverse ) {oInf->srcRecv += iDataSize;}
    else {oInf->dstRecv += iDataSize;}
}

typedef BOOL(*com_procTcpFunc_t)(
        com_sigTcpInf_t *oInf, com_off iDataSize, com_sigTcpNode_t *iNode,
        com_sigInf_t *ioHead );

static BOOL procSyn(
        com_sigTcpInf_t *oInf, com_off iDataSize, com_sigTcpNode_t *iNode,
        com_sigInf_t *ioHead )
{
    COM_UNUSED( iDataSize );
    (void)searchTcpNode( iNode );
    ulong  seq, ack;
    getTcpSeq( &seq, &ack, ioHead );
    COM_CAST_HEAD( struct tcphdr, tcp, COM_SGTOP );
    if( !COM_IS_TCP_ACK ) {
        *oInf = (com_sigTcpInf_t){
            false, seq, ack, 0, 0, COM_SIG_TS_OFFER, COM_SIG_TS_NONE
        };
    }
    else {
        *oInf = (com_sigTcpInf_t){
            false, ack - 1, seq, 0, 1, COM_SIG_TS_ACK, COM_SIG_TS_NONE
        };
    }
    return addTcpHash( true, iNode, oInf );
}

static BOOL procFin(
        com_sigTcpInf_t *oInf, com_off iDataSize, com_sigTcpNode_t *iNode,
        com_sigInf_t *ioHead )
{
    ulong  seq, ack;
    getTcpSeq( &seq, &ack, ioHead );
    COM_CAST_HEAD( struct tcphdr, tcp, COM_SGTOP );  // ACKビット確認で使用
    *oInf = (com_sigTcpInf_t){
        false, seq - 2, ack - 1 - COM_IS_TCP_ACK, 2, 1 + COM_IS_TCP_ACK,
        COM_SIG_TS_ACK, COM_SIG_TS_OFFER
    };
    const com_sigTcpInf_t*  tmp = searchTcpNode( iNode );
    if( tmp ) {
        *oInf = *tmp;
        if( oInf->statFin == COM_SIG_TS_NONE ) {
            oInf->statFin = COM_SIG_TS_OFFER;
        }
        else if( oInf->statFin == COM_SIG_TS_OFFER && COM_IS_TCP_ACK ) {
            oInf->statFin = COM_SIG_TS_ACK;
        }
    } // !tmpは NG ではなく「SYNなしの途中キャプチャ」と認識する。
    addRecvSize( iNode, oInf, iDataSize );
    return addTcpHash( true, iNode, oInf );
}

static BOOL procAck(
        com_sigTcpInf_t *oInf, com_off iDataSize, com_sigTcpNode_t *iNode,
        com_sigInf_t *ioHead )
{
    ulong  seq, ack;
    getTcpSeq( &seq, &ack, ioHead );
    *oInf = (com_sigTcpInf_t){
        false, seq - 1, ack - 1, 1, 1, COM_SIG_TS_ACK, COM_SIG_TS_NONE
    };
    const com_sigTcpInf_t*  tmp = searchTcpNode( iNode );
    if( tmp ) {
        *oInf = *tmp;
        if( tmp->statFin == COM_SIG_TS_ACK ) {
            return com_deleteHash( gHashTcp, iNode, sizeof(*iNode) );
        }
    } // !tmpは NG ではなく「SYNなしの途中キャプチャ」と認識する。
    else {
        if( iNode->isReverse ) {oInf->ackSyn -= iDataSize;}
    }
    addRecvSize( iNode, oInf, iDataSize );
    return addTcpHash( true, iNode, oInf );
}

static BOOL getSeq(
        com_sigTcpInf_t *oInf, com_off iDataSize, com_sigTcpNode_t *iNode,
        com_sigInf_t *ioHead )
{
    memset( oInf, 0, sizeof(*oInf) );
    COM_CAST_HEAD( struct tcphdr, tcp, COM_SGTOP );
    com_procTcpFunc_t  func = procAck;
    if( COM_IS_TCP_SYN ) {func = procSyn;}
    if( COM_IS_TCP_FIN ) {func = procFin;}
    return (func)( oInf, iDataSize, iNode, ioHead );
}

static BOOL makeTcpInf( com_sigInf_t *ioHead, com_off iDataSize )
{
    com_sigTcpInf_t  inf;
    com_sigTcpNode_t  node;
    if( !makeNodeInf( &node, ioHead ) ) {return false;}
    if( !getSeq( &inf, iDataSize, &node, ioHead ) ) {return false;}
    com_sigTcpInf_t*  prmSpec = ioHead->prm.spec;
    if( !prmSpec ) {
        prmSpec = com_malloc( sizeof(com_sigTcpInf_t), "TCP seq/ack inf" );
        if( !prmSpec ) {return false;}
        ioHead->prm.spec = prmSpec;
    }
    inf.isReverse = node.isReverse;
    *prmSpec = inf;
    return true;
}

static long getPortFromSdpInf( com_nodeInf_t *iInf );

static void searchPort( com_sigInf_t *oTarget, long iBase, long *oHitPort )
{
    com_nodeInf_t  node;
    if( oTarget ) {com_getNodeInf( oTarget, &node );}
    long  srcPort = com_calcValue( node.srcPort, COM_16BIT_SIZE );
    long  dstPort = com_calcValue( node.dstPort, COM_16BIT_SIZE );
    COM_SET_IF_EXIST( oHitPort, srcPort );
    long  type = com_getPrtclType( iBase, srcPort );
    if( type == COM_SIG_CONTINUE ) {  // 発ポートでダメなら着ポート
        COM_SET_IF_EXIST( oHitPort, dstPort );
        type = com_getPrtclType( iBase, dstPort );
    }
    if( type == COM_SIG_CONTINUE ) {  // 着ポートでもダメならSDP情報
        COM_SET_IF_EXIST( oHitPort, 0 );
        type = getPortFromSdpInf( &node );
    }
    if( type != COM_SIG_CONTINUE ) {
        if( oTarget ) {oTarget->sig.ptype = type;}
    }
}

static void judgePayload( com_sigInf_t *ioHead )
{
    com_sigTcpInf_t*  inf = ioHead->prm.spec;
    if( inf->statSyn == COM_SIG_TS_ACK && inf->statFin != COM_SIG_TS_ACK ) {
        com_free( ioHead->ext );  // 念のため解放
        ioHead->ext = com_malloc( sizeof(long), "tcp port" );
        searchPort( COM_NEXTSTK, COM_IPPORT, ioHead->ext );
    }
    else {
        if( COM_NEXTSTK == 0 ) {return;}
        com_sigInf_t*  tmp = COM_NEXTSTK;
        tmp->sig = (com_sigBin_t){ NULL, 0, COM_SIG_END };
    }
}

BOOL com_analyzeTcp( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( struct tcphdr ) );
    COM_CAST_HEAD( struct tcphdr, tcp, COM_SGTOP );
    size_t  pktSize = COM_SGLEN;
    size_t  hdrSize = tcp->th_off * COM_32BIT_SIZE;
    if( com_setHeadInf( ioHead, hdrSize, COM_SIG_TCP, COM_SIG_CONTINUE ) ) {
        getPrtOptions( COM_TCPOPTLEN, ioHead, hdrSize, HDRSIZE );
        result = makeTcpInf( ioHead, pktSize - hdrSize );
        if( result ) {judgePayload( ioHead );}
    }
    COM_ANALYZER_END;
}

static void dispNextIfExist( com_sigInf_t *iHead )
{
    long*  port = iHead->ext;
    if( !COM_INEXTCNT || !port ) {return;}
    if( COM_INEXTSTK[0].sig.ptype != COM_SIG_CONTINUE ) {
        com_dispNext( *port, sizeof(short), COM_INEXTSTK[0].sig.ptype );
    }
}

static void dispSeq( const char *iLabel, ulong iSeq, ulong iBase )
{
    com_dispDec( "    %s = %lu (0x%lx)", iLabel, iSeq - iBase, iSeq );
}

static void dispSeqAck( com_sigInf_t *iHead )
{
    COM_CAST_HEAD( struct tcphdr, tcp, COM_ISGTOP );
    if( !iHead->prm.spec ) {
        com_dispPrm( "seqno", &tcp->th_seq, COM_32BIT_SIZE );
        com_dispPrm( "ackno", &tcp->th_ack, COM_32BIT_SIZE );
        return;
    }
    com_sigTcpInf_t*  inf = iHead->prm.spec;
    ulong  seq = inf->seqSyn;
    ulong  ack = inf->ackSyn;
    if( inf->isReverse ) {seq = inf->ackSyn;  ack = inf->seqSyn;}
    dispSeq( "seqno", com_getVal32( tcp->th_seq, COM_IORDER ), seq );
    dispSeq( "ackno", com_getVal32( tcp->th_ack, COM_IORDER ), ack );
    com_off  size = 0;
    if( COM_INEXTCNT ) {size = COM_INEXTSTK->sig.len;}
    com_dispVal( "payload size", size );
}

static void dispTcpFlg( const char *iLabel, long *ioCount )
{
    if( *ioCount ) {com_printf( "," );}
    com_printf( "%s", iLabel );
    (*ioCount)++;
}

static void dispTcpFlgSet( struct tcphdr *iTcp )
{
    struct tcphdr* tcp = iTcp;
    long flgCnt = 0;
    if( COM_IS_TCP_URG ) {dispTcpFlg( "URG", &flgCnt );}
    if( COM_IS_TCP_SYN ) {dispTcpFlg( "SYN", &flgCnt );}
    if( COM_IS_TCP_FIN ) {dispTcpFlg( "FIN", &flgCnt );}
    if( COM_IS_TCP_RST ) {dispTcpFlg( "RST", &flgCnt );}
    if( COM_IS_TCP_PSH ) {dispTcpFlg( "PSH", &flgCnt );}
    if( COM_IS_TCP_ACK ) {dispTcpFlg( "ACK", &flgCnt );}
    com_printLf();
}

void com_decodeTcp( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispSig( "TCP HEADER ", COM_SIG_TCP, &COM_ISG );
    COM_CAST_HEAD( struct tcphdr, tcp, COM_ISGTOP );
    com_dispPrm( "src port", &tcp->th_sport, COM_16BIT_SIZE );
    com_dispPrm( "dst port", &tcp->th_dport, COM_16BIT_SIZE );
    dispSeqAck( iHead );
    com_printf( "#    control = " );
    dispTcpFlgSet( tcp );
    if( COM_IPRMCNT ) {dispPrtOptions( COM_IAPRM, com_getTcpOptName );}
    dispNextIfExist( iHead );
    COM_DECODER_END;
}

static com_decodeName_t  gTcpOptName[] = {
    { TCPOPT_EOL,              "End of Option List" },
    { TCPOPT_NOP,              "No Operation" },
    { TCPOPT_MAXSEG,           "Maximum Segment Size" },
    { TCPOPT_WINDOW,           "Window Scale" },
    { TCPOPT_SACK_PERMITTED,   "SACK permitted" },
    { TCPOPT_SACK,             "SACK" },
    { TCPOPT_TIMESTAMP,        "Time Stamp" },
    COM_DECODENAME_END  // 最後は必ずこれで
};

char *com_getTcpOptName( long iType )
{
    return com_searchDecodeName( gTcpOptName, iType, true );
}


static BOOL makeSegCond(
        com_sigTcpNode_t *oNode, com_sigFrgCond_t *oCond,
        com_sigInf_t *iTarget, com_sigInf_t *iTcp )
{
    if( !makeNodeInf( oNode, iTcp ) ) {return false;}
    if( !searchTcpNode( oNode ) ) {return false;}
    memset( oCond, 0, sizeof(*oCond) );
    *oCond = (com_sigFrgCond_t){
        COM_SIG_TCP, (oNode->src.port << 16) + oNode->dst.port,
        {oNode->src.addr, sizeof(oNode->src.addr), 0},
        {oNode->dst.addr, sizeof(oNode->dst.addr), 0},
        iTarget->sig.ptype, 0
    };
    return true;
}

com_sigFrg_t *com_stockTcpSeg(
        com_sigInf_t *ioTarget, com_sigInf_t *iTcp, com_off iTotalSize )
{
    if( !ioTarget || !iTcp ) {COM_PRMNG(NULL);}
    com_sigTcpNode_t  node;
    com_sigFrgCond_t  cond;
    if( !makeSegCond( &node, &cond, ioTarget, iTcp ) ) {return NULL;}
    COM_CAST_HEAD( struct tcphdr, tcp, iTcp->sig.top );
    ulong  seq = com_getVal32( tcp->th_seq, iTcp->order );
    com_sigFrg_t*  frg = com_stockFragments( &cond, seq, &ioTarget->sig );
    if( !frg ) {return NULL;}
    if( iTotalSize ) {
        frg->segMax = iTotalSize;
        ulong*  seq1st = com_malloc( sizeof(ulong), "TCP segment 1st seqno" );
        if( !seq1st ) {return NULL;}
        *seq1st = seq;
        frg->ext = seq1st;
    }
    ioTarget->isFragment = true;
    com_dispDec( "   TCP segmentation proccessed[%lu]", frg->cnt );
    return frg;
}

static long calcTotalFrg( com_sigFrg_t *iFrg )
{
    long  total = 0;
    for( long i = 0;  i < iFrg->cnt;  i++ ) {total += iFrg->inf[i].bin.len;}
    return total;
}

static COM_SIG_FRG_t freeTcpSeg(
        com_sigFrgCond_t *iCond, COM_SIG_FRG_t iResult )
{
    com_freeFragments( iCond );
    return iResult;
}

static long checkLastTcpSeg( com_sigFrg_t *iFrg )
{
    if( !iFrg->ext ) {return -1;}
    long  lastCnt = -1;
    long  maxSeg = 0;
    for( long i = 0;  i < iFrg->cnt;  i++ ) {
        if( iFrg->inf[i].seg > maxSeg ) {
            maxSeg = iFrg->inf[i].seg;
            lastCnt = i;
        }
    }
    return lastCnt;
}

// TODO: iCutについては、まだ要検討事項あり
// もし iCut が最後のセグメントデータサイズより大きいときに問題があるため。

static void combineTcpSeg( com_bin *oRas, com_sigFrg_t *iFrg, com_off iCut )
{
    com_off  offset = 0;
    long  lastCnt = checkLastTcpSeg( iFrg );
    for( long i = 0;  i < iFrg->cnt;  i++ ) {
        com_sigSeg_t*  inf = &(iFrg->inf[i]);
        com_off  size = inf->bin.len;
        if( i == lastCnt ) {size -= iCut;}
        if( iFrg->ext ) {
            ulong*  top = iFrg->ext;
            offset = (ulong)(inf->seg) - *top;
            memcpy( oRas + offset, inf->bin.top, size );
            offset += inf->bin.len;
        }
    }
}

COM_SIG_FRG_t com_reassembleTcpSeg(
        com_sigFrg_t *iFrg, com_sigInf_t *ioTarget, com_sigInf_t *iTcp )
{
    if( !iFrg || !ioTarget || !iTcp ) {COM_PRMNG(COM_FRG_ERROR);}
    if( !iFrg->segMax ) {return COM_FRG_SEG;}
    com_sigTcpNode_t  node;
    com_sigFrgCond_t  cond;
    if( !makeSegCond( &node, &cond, ioTarget, iTcp ) ) {return COM_FRG_ERROR;}
    long  total = calcTotalFrg( iFrg );
    com_off  cutSize = 0;
    if( total < iFrg->segMax ) {return COM_FRG_SEG;}
    if( total > iFrg->segMax ) {cutSize = total - iFrg->segMax;}
    total = iFrg->segMax;
    com_bin*  ras = com_malloc( total, "TCP reassemble" );
    if( !ras ) {return freeTcpSeg( &cond, COM_FRG_ERROR );}
    combineTcpSeg( ras, iFrg, cutSize );
    ioTarget->ras = (com_sigBin_t){ ras, total, 0 };
    ioTarget->sig = ioTarget->ras;
    ioTarget->isFragment = false;
    return freeTcpSeg( &cond, COM_FRG_REASM );
}

BOOL com_continueTcpSeg( com_sigInf_t *ioTarget, com_sigInf_t *iTcp )
{
    if( !ioTarget ) {COM_PRMNG(false);}
    if( !iTcp ) {return false;}
    com_sigTcpNode_t  node;
    com_sigFrgCond_t  cond;
    if( !makeSegCond( &node, &cond, ioTarget, iTcp ) ) {return false;}
    com_sigFrg_t*  frg;
    if( com_searchFragment( &cond ) ) {
        if( (frg = com_stockTcpSeg( ioTarget, iTcp, 0 )) ) {
            COM_SIG_FRG_t  result = com_reassembleTcpSeg( frg, ioTarget, iTcp );
            if( result == COM_FRG_REASM ) {return true;}
        }
    }
    return false;
}



// UDP ///////////////////////////////////////////////////////////////////////

BOOL com_analyzeUdp( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( struct udphdr ) );
    result =
        com_setHeadInf( ioHead, HDRSIZE, COM_SIG_UDP, COM_SIG_CONTINUE );
    if( result ) {
        com_free( ioHead->ext );  // 年のため解放
        ioHead->ext = com_malloc( sizeof(long), "udp port" );
        searchPort( COM_NEXTSTK, COM_IPPORT, ioHead->ext );
    }
    COM_ANALYZER_END;
}

void com_decodeUdp( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispSig( "UDP HEADER ", COM_SIG_UDP, &COM_ISG );
    COM_CAST_HEAD( struct udphdr, udp, COM_ISGTOP );
    com_dispPrm( "src port", &udp->uh_sport, COM_16BIT_SIZE );
    com_dispPrm( "dst port", &udp->uh_dport, COM_16BIT_SIZE );
    dispNextIfExist( iHead );
    COM_DECODER_END;
}



// SCTP //////////////////////////////////////////////////////////////////////

// 初期化用：次プロトコル値
static com_sigPrtclType_t  gSctpNext1[] = {
    { {COM_CAP_SCTP_PROTO_DIAMETER},  COM_SIG_DIAMETER },
    { {COM_PRTCLTYPE_END}, COM_SIG_CONTINUE }
};

static BOOL getChunkHead(
        com_bin *iTop, com_sigInf_t *oHead, com_off *oChunkLen )
{
    COM_CAST_HEAD( com_sigSctpChunkHdr_t, chunk, iTop );
    *oChunkLen = com_getVal16( chunk->length, oHead->order );
    return com_stackSigMulti( oHead, "sctp chunk",
                           &(com_sigBin_t){ iTop, *oChunkLen, COM_SIG_SCTP } );
}

static BOOL getDataPayload( com_bin *iTop, com_sigInf_t *oHead )
{
    COM_CAST_HEAD( com_sigSctpDataHdr_t, data, iTop );
    BOOL  order = oHead->order;
    com_sigInf_t  body;
    com_initSigInf( &body, oHead );
    if( data->chunkHdr.type == COM_CAP_SCTP_DATA ) {
        body.sig = (com_sigBin_t){
            iTop + sizeof(*data),
            com_getVal16( data->chunkHdr.length, order ) - sizeof(*data),
            com_getPrtclType( COM_SCTPNEXT,com_getVal32(data->protocol, order) )
        };
    }
    else {body.sig.ptype = COM_SIG_END;}
    com_sigInf_t*  newStack = com_stackSigNext( oHead, &body );
    if( newStack && body.sig.ptype == COM_SIG_CONTINUE ) {
        newStack->ext = com_malloc( sizeof(long), "sctp port" );
        searchPort( newStack, COM_IPPORT, newStack->ext );
    }
    return (newStack != NULL);
}

static BOOL getChunks( com_sigInf_t *iSctp, com_sigInf_t *oHead )
{
    com_bin*  tmpPtr = iSctp->sig.top;
    com_off   tmpLen = getIpConSize( oHead );  // IPから実サイズ取得
    if( !com_advancePtr( &tmpPtr, &tmpLen, sizeof(com_sigSctpCommonHdr_t) ) ) {
        return false;
    }
    while( tmpLen > 0 ) {
        com_off  chunkLen = 0;
        if( !getChunkHead( tmpPtr, oHead, &chunkLen ) ) {return false;}
        if( !getDataPayload( tmpPtr, oHead ) ) {return false;}
        chunkLen += chunkLen % 4;
        if( !com_advancePtr( &tmpPtr, &tmpLen, chunkLen ) ) {return false;}
    }
    return true;
}

BOOL com_analyzeSctp( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( com_sigSctpCommonHdr_t ) );
    (void)com_setHeadInf( ioHead, HDRSIZE, COM_SIG_SCTP, COM_SIG_ONLYHEAD );
    result = getChunks( &orgHead, ioHead );
    COM_ANALYZER_END;
}

#define DISPBIN( LABEL, PRM, SIZE ) \
    com_dispBin( LABEL, &(PRM), SIZE, "", true )

static void dispChunkData( void *iTop )
{
    COM_CAST_HEAD( com_sigSctpDataHdr_t, dataHdr, iTop );
    DISPBIN( "tsn",          dataHdr->tsn,         COM_32BIT_SIZE );
    DISPBIN( "stream ID",    dataHdr->streamId,    COM_16BIT_SIZE );
    DISPBIN( "stream seqno", dataHdr->streamSeqno, COM_16BIT_SIZE );
}

static void dispChunkInit( void *iTop )
{
    COM_CAST_HEAD( com_sigSctpInitHdr_t, initHdr, iTop );
    DISPBIN( "initTag",          initHdr->initTag,        COM_32BIT_SIZE );
    DISPBIN( "a_rwnd",           initHdr->a_rwnd,         COM_32BIT_SIZE );
    DISPBIN( "outbound streams", initHdr->noOutboundStrm, COM_16BIT_SIZE );
    DISPBIN( "inboud streams",   initHdr->noInboundStrm,  COM_16BIT_SIZE );
    DISPBIN( "initTsn",          initHdr->initTsn,        COM_32BIT_SIZE );
}

static void dispChunkSack( void *iTop )
{
    COM_CAST_HEAD( com_sigSctpSackHdr_t, sackHdr, iTop );
    DISPBIN( "cumulative TSN Ack", sackHdr->cmlTsnAck,     COM_32BIT_SIZE );
    DISPBIN( "a_rwnd",             sackHdr->a_rwnd,        COM_32BIT_SIZE );
    DISPBIN( "#Gap Ack Block",     sackHdr->noGapAckBlock, COM_16BIT_SIZE );
    DISPBIN( "#Duplicate TSNs",    sackHdr->noDupTsns,     COM_16BIT_SIZE );
}

static BOOL dispChunkInf( long iChunkType, com_sigBin_t *iSig )
{
    if( iChunkType == COM_CAP_SCTP_DATA ) {
        dispChunkData( iSig->top );
        return true;
    }
    if( iChunkType == COM_CAP_SCTP_INIT || iChunkType == COM_CAP_SCTP_INITACK ){
        dispChunkInit( iSig->top );
    }
    if( iChunkType == COM_CAP_SCTP_SACK ) {dispChunkSack( iSig->top );}
    return false;
}

static void dispChunk(
        com_sigSctpDataHdr_t *iSctp, com_sigInf_t *iHead,
        com_sigInf_t *iPayload, com_sigInf_t *iChunk )
{
    com_dispPrm( "type", com_getSctpChunkName( iSctp ), 0 );
    if( dispChunkInf( iSctp->chunkHdr.type, &iChunk->sig ) ) {
        if( iPayload->ext ) {
            long*  port = iPayload->ext;
            com_dispNext( *port, sizeof(short), iPayload->sig.ptype );
            com_free( iPayload->ext );
        }
        else {
            com_dispNext( com_getVal32( iSctp->protocol, COM_IORDER ),
                          sizeof( iSctp->protocol ), iPayload->sig.ptype );
        }
    }
}

void com_decodeSctp( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispSig( "SCTP HEADER ", COM_SIG_SCTP, &COM_ISG );
    COM_CAST_HEAD( com_sigSctpCommonHdr_t, cmnHdr, COM_ISGTOP );
    com_dispPrm( "src port", &cmnHdr->srcPort, COM_16BIT_SIZE );
    com_dispPrm( "dst port", &cmnHdr->dstPort, COM_16BIT_SIZE );
    DISPBIN( "verification Tag", cmnHdr->verifTag, COM_32BIT_SIZE );
    com_sigInf_t*  payload = COM_INEXTSTK;
    for( long i = 0;  i < COM_INEXTCNT;  i++ ) {
        com_sigInf_t*  chunk = COM_IMLTISTK;
        com_dispSig( " chunk", i, &chunk[i].sig );
        COM_CAST_HEAD( com_sigSctpDataHdr_t, sctp, chunk[i].sig.top );
        dispChunk( sctp, iHead, &payload[i], &chunk[i] );
    } 
    COM_DECODER_END;
}

static com_decodeName_t  gSctpChunkName[] = {
    { COM_CAP_SCTP_DATA,      "DATA" },
    { COM_CAP_SCTP_INIT,      "INIT" },
    { COM_CAP_SCTP_INITACK,   "INIT_ACK" },
    { COM_CAP_SCTP_SACK,      "SACK" },
    { COM_CAP_SCTP_HB,        "HEARTBEAT" },
    { COM_CAP_SCTP_HBACK,     "HEATBEAT_ACK" },
    { COM_CAP_SCTP_ABORT,     "ABORT" },
    { COM_CAP_SCTP_SD,        "SHUTDOWN" },
    { COM_CAP_SCTP_SDACK,     "SHUTDOWN_ACK" },
    { COM_CAP_SCTP_ERROR,     "ERROR" },
    { COM_CAP_SCTP_CKECHO,    "COOKIE ECHO" },
    { COM_CAP_SCTP_CKACK,     "COOKIE ACK" },
    { COM_CAP_SCTP_ECNE,      "ECNE" },
    { COM_CAP_SCTP_CWR,       "CWR" },
    COM_DECODENAME_END  // 最後は必ずこれで
};

char* com_getSctpChunkName( void *iSigTop )
{
    com_sigSctpChunkHdr_t*  sctp = iSigTop;
    return com_searchDecodeName( gSctpChunkName, sctp->type, true );
}



// SIP ///////////////////////////////////////////////////////////////////////

// 初期化用：次プロトコル値
static com_sigPrtclType_t  gSipNext1[] = {
    { {.label = COM_SIG_CTYPE_SDP},  COM_SIG_SDP },
    { {COM_PRTCLTYPE_END}, COM_SIG_CONTINUE }
};

BOOL com_analyzeSip( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( COM_NO_MIN_LEN );
    if( com_getSipName( COM_SGTOP, NULL, NULL ) ) {
        result = com_analyzeTxtBase( ioHead, COM_SIG_SIP, COM_SIPNEXT,
                                     COM_CAP_SIPHDR_CLENGTH,
                                     COM_CAP_SIPHDR_CTYPE,
                                     COM_CAP_CTYPE_MULTI );
    }
    COM_ANALYZER_END;
}

void com_decodeSip( COM_DECODER_PRM )
{
    COM_DECODER_START;
    const char*  label = NULL;
    long  type = COM_SIG_SIP_UNKNOWN;
    if( !com_getSipName( COM_ISGTOP, &label, &type ) ) {return;}
    com_decodeTxtBase( iHead, NULL, label, type, COM_CAP_SIPHDR_CTYPE );
    COM_DECODER_END;
}

static com_sigMethod_t  gSipName[] = {
    { "REGISTER",   COM_SIG_SIP_REGISTER },
    { "INVITE",     COM_SIG_SIP_INVITE },
    { "ACK",        COM_SIG_SIP_ACK },
    { "CANCEL",     COM_SIG_SIP_CANCEL },
    { "BYE",        COM_SIG_SIP_BYE },
    { "OPTIONS",    COM_SIG_SIP_OPTIONS },
    { "INFO",       COM_SIG_SIP_INFO },
    { "PRACK",      COM_SIG_SIP_PRACK },
    { "SUBSCRIBE",  COM_SIG_SIP_SUBSCRIBE },
    { "NOTIFY",     COM_SIG_SIP_NOTIFY },
    { "UPDATE",     COM_SIG_SIP_UPDATE },
    { "MESSAGE",    COM_SIG_SIP_MESSAGE },
    { "REFFER",     COM_SIG_SIP_REFFER },
    { "PUBLISH",    COM_SIG_SIP_PUBLISH },
    { "SIP/2.0",    COM_SIG_SIP_RESPONSE },
    { NULL, COM_SIG_SIP_UNKNOWN }  // 最後は必ずこれで
};

BOOL com_getSipName( void *iSigTop, const char **oLabel, long *oType )
{
    COM_SET_IF_EXIST( oLabel, "" );
    COM_SET_IF_EXIST( oType, COM_SIG_SIP_UNKNOWN );
    return com_getMethodName( gSipName, iSigTop, oLabel, oType );
}



// SDP ///////////////////////////////////////////////////////////////////////

// セッション情報データ構造
typedef struct {
    BOOL           isUse;
    com_nodeInf_t  cpInf;
    char*          callId;
    com_nodeInf_t  upInf;
    long           payloadType;
} com_sdpSessionInf_t;

static long  gSdpSesCnt = 0;
static com_sdpSessionInf_t*  gSdpSesInf = NULL;

static void freeSdpSesInf( void )
{
    for( long i = 0;  i < gSdpSesCnt;  i++ ) {
        com_free( gSdpSesInf[i].callId );
    }
    com_free( gSdpSesInf );
}

static com_sdpSessionInf_t *checkSdpSesInf( long iCnt, char *iCallId )
{
    com_sdpSessionInf_t*  tmp = &(gSdpSesInf[iCnt]);
    if( !tmp->isUse ) {return NULL;}
    if( strcmp( iCallId, tmp->callId ) ) {return NULL;}
    return tmp;
}

static com_sdpSessionInf_t *clearSdpSesInf( com_sdpSessionInf_t *oInf )
{
    com_free( oInf->callId );
    memset( oInf, 0, sizeof(*oInf) );
    return oInf;
}

static BOOL setSdpSesInf(
        com_sdpSessionInf_t *oInf, com_nodeInf_t *iInf, char *iCallId )
{
    if( !(oInf->callId = com_strdup( iCallId, "new sdp session callId" )) ) {
        return false;
    }
    oInf->isUse = true;
    oInf->cpInf = *iInf;
    return true;
}

static com_sdpSessionInf_t *getSdpSesInf(
        com_nodeInf_t *iInf, char *iCallId, long *oId, BOOL iAdd )
{
    com_sdpSessionInf_t*  tmp = NULL;
    for( long i = 0;  i < gSdpSesCnt;  i++ ) {
        if( (tmp = checkSdpSesInf( i, iCallId )) ) {*oId = i;  return tmp;}
    }
    if( !iAdd ) {return NULL;}
    tmp = NULL;
    for( long i = 0;  i < gSdpSesCnt;  i++ ) {
        com_sdpSessionInf_t*  inf = &(gSdpSesInf[i]);
        if( !inf->isUse ) {tmp = clearSdpSesInf( inf );  *oId = i;  break;}
    }
    if( !tmp ) {  // 再利用できるものがないときは新規追加
        tmp = com_reallocAddr( &gSdpSesInf, sizeof(*gSdpSesInf), COM_TABLEEND,
                               &gSdpSesCnt, 1, "new sdp session inf" );
    }
    if( tmp ) {
        if( !setSdpSesInf( tmp, iInf, iCallId ) ) {return NULL;}
        *oId = gSdpSesCnt - 1;
    }
    return tmp;
}

static void setSdpAddrPort(
        char *iCval, char *iMval, long *oPtype, com_bin *oAddr, com_bin *oPort )
{
    char*  saveptr;
    (void)strtok_r( iCval, " ", &saveptr );
    char*  tok = strtok_r( NULL, " ", &saveptr );
    int  af;
    if( !strcmp( tok, "IP4" ) ) {*oPtype = COM_SIG_IPV4;  af = AF_INET;}
    else if( !strcmp( tok, "IP6" ) ) {*oPtype = COM_SIG_IPV6;  af = AF_INET6;}
    else {return;}
    tok = strtok_r( NULL, " ", &saveptr );
    inet_pton( af, tok, oAddr );
    (void)strtok_r( iMval, " ", &saveptr );
    tok = strtok_r( NULL, " ", &saveptr );
    uint16_t  port = com_setVal16( com_atol( tok ), true );
    memmove( oPort, &port, sizeof(port) );
}

static void setSdpPrmInf(
        com_sdpSessionInf_t *oSdp, com_sigInf_t *iHead, BOOL iOrg )
{
    char*  cVal = com_strdup(com_getTxtHeaderVal( COM_IAPRM, "c" ), "sdp c");
    char*  mVal = com_strdup(com_getTxtHeaderVal( COM_IAPRM, "m" ), "sdp m");
    if( cVal && mVal ) {
        com_nodeInf_t*  up = &oSdp->upInf;
        com_bin*  addr = iOrg ? up->srcAddr : up->dstAddr;
        com_bin*  port = iOrg ? up->srcPort : up->dstPort;
        setSdpAddrPort( cVal, mVal, &up->ptype, addr, port );
    }
    com_free( cVal );
    com_free( mVal );
}

#define SDPINF  "sdp information"

static void createSdpSession( com_sigInf_t *iHead )
{
    long  sipType;
    com_sigInf_t*  sip = iHead->prev;
    if( !sip ) {return;}  // 下位プロトコル情報(SIP)が無い場合、何もできない
    (void)com_getSipName( sip->sig.top, NULL, &sipType );
    com_nodeInf_t  cpInf;
    com_getNodeInf( iHead, &cpInf );
    char*  callId = com_getTxtHeaderVal( &sip->prm, COM_CAP_SIPHDR_CALLID );
    long  id = 0;
    com_sdpSessionInf_t*  sdpInf = NULL;
    if( sipType < COM_SIG_SIP_RESPONSE ) {
        if( !(sdpInf = getSdpSesInf( &cpInf,callId,&id,true )) ) {return;}
        setSdpPrmInf( sdpInf, iHead, true );
        DEBUGSIG( "#    << set src %s#%ld >>\n", SDPINF, id );
    }
    else {
        com_reverseNodeInf( &cpInf );
        if( !(sdpInf = getSdpSesInf( &cpInf,callId,&id,true )) ) {return;}
        setSdpPrmInf( sdpInf, iHead, false );
        DEBUGSIG( "#    << set dst %s#%ld >>\n", SDPINF, id );
    }
}

static char  gSdpBuf[COM_LINEBUF_SIZE];

static BOOL getSdpData( com_seekFileResult_t *iInf )
{
    com_sigTextSeek_t*  inf = iInf->userData;
    com_off*  discLen = inf->size;
    char*  hdr = (char*)(inf->head->sig.top) + *discLen;
    com_sigTlv_t  newDisc;
    com_setTxtHeaderVal( &newDisc, hdr, '=' );
    if( !com_addPrmTlv( &inf->head->prm, &newDisc ) ) {return false;}
    *discLen += strlen( iInf->line );
    return true;
}

BOOL com_analyzeSdp( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( COM_NO_MIN_LEN );
    com_off  discLen = 0;
    com_sigTextSeek_t  inf = { ioHead, &discLen };
    result = com_seekTextLine( COM_SGTOP, COM_SGLEN, true, getSdpData, &inf,
                               gSdpBuf, sizeof(gSdpBuf) );
    if( result ) {createSdpSession( ioHead );}
    COM_ANALYZER_END;
}

static void dispString( void *iTop, com_off iLen )
{
    char*  top = iTop;
    for( com_off i = 0;  i < iLen;  i++ ) {com_printf( "%c", top[i] );}
}

static void dispTxtPrm( com_sigTlv_t *iTlv )
{
    com_printf( "#    " );
    dispString( iTlv->tagBin.top, iTlv->tagBin.len );
    com_printf( " = " );
    dispString( iTlv->value, iTlv->len );
    com_printLf();
}

void com_decodeSdp( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispDec( " SDP DESCRIPTIONS [%d]", COM_SIG_SDP );
    com_sigPrm_t*  disc = COM_IAPRM;
    for( long i = 0;  i < disc->cnt;  i++ ) {dispTxtPrm( &(disc->list[i]) );}
    COM_DECODER_END;
}


static void addPortNum( com_bin *ioPort, long iPortAdd )
{
    uint16_t  port = (ioPort[0] << 8) + ioPort[1];
    port += iPortAdd;
    ioPort[0] = port >> 8;
    ioPort[1] = port & 0xff;
}

static void modifyNodeInf( com_nodeInf_t *oTarget, long iPortAdd )
{
    addPortNum( oTarget->srcPort, iPortAdd );
    addPortNum( oTarget->dstPort, iPortAdd );
}

static BOOL checkSdpNodeInf(
        com_nodeInf_t *iTarget, com_nodeInf_t *iInf, long iPortAdd )
{
    com_nodeInf_t  target = *iTarget;
    if( iPortAdd ) {modifyNodeInf( &target, iPortAdd );}
    if( com_matchNodeInf( &target, iInf ) ) {return true;}
    com_reverseNodeInf( &target );
    return com_matchNodeInf( &target, iInf );
}

static long getPortFromSdpInf( com_nodeInf_t *iInf )
{
    for( long i = 0;  i < gSdpSesCnt;  i++ ) {
        com_sdpSessionInf_t*  tmp = &(gSdpSesInf[i]);
        if( !tmp->isUse ) {continue;}
        if( checkSdpNodeInf( iInf, &tmp->upInf, 0 ) ) {
            DEBUGSIG( "#   <<match %s#%ld as RTP>>\n", SDPINF, i );
            return COM_SIG_RTP;
        }
        if( checkSdpNodeInf( iInf, &tmp->upInf, -1 ) ) {
            DEBUGSIG( "#   <<match %s#%ld as RTCP>>\n", SDPINF, i );
            return COM_SIG_RTCP;
        }
    }
    return COM_SIG_CONTINUE;
}



// RTP ///////////////////////////////////////////////////////////////////////

static com_off calcRtpHdrSize( com_sigRtpHdr_t *iRtp )
{
    com_off  size = sizeof( com_sigRtpHdr_t );
    size += (iRtp->rtpBits & COM_CAP_RTP_CCBIT) * COM_32BIT_SIZE;
    if( COM_CHECKBIT( iRtp->rtpBits, COM_CAP_RTP_XBIT ) ) {
        COM_CAST_HEAD( com_sigRtpExtHdr_t, rtpExt, iRtp + size );
        size += rtpExt->length * COM_32BIT_SIZE;
    }
    return size;
}

BOOL com_analyzeRtp( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( com_sigRtpHdr_t ) );
    COM_CAST_HEAD( com_sigRtpHdr_t, rtp, COM_SGTOP );
    if( (rtp->rtpBits & COM_CAP_RTP_VBIT) == COM_CAP_RTP_VERSION ) {
        com_off  hdrSize = calcRtpHdrSize( rtp );
        result = analyzeSig( ioHead, hdrSize, COM_SIG_RTP, &orgHead, "RTP" );
    }
    COM_ANALYZER_END;
}

void com_decodeRtp( COM_DECODER_PRM )
{
    COM_DECODER_START;
    COM_CAST_HEAD( com_sigRtpHdr_t, rtp, COM_ISGTOP );
    decodeSig( iHead, "RTP", COM_SIG_RTP );
    long  pt = rtp->pt & COM_CAP_RTP_PTBIT;
    com_dispVal( "PT(payload type)", pt );
    com_dispVal( "sequence number", com_getVal16( rtp->seq, COM_IORDER ) );
    uint8_t bits = rtp->rtpBits;
    if( bits & COM_CAP_RTP_PBIT ) {com_dispPrm( "P(padding)", "on", 0 );}
    if( bits & COM_CAP_RTP_XBIT ) {com_dispPrm( "X(extension)", "on", 0 );}
    if( rtp->pt & COM_CAP_RTP_MBIT ) {com_dispPrm( "M(marker)", "on", 9 );}
    COM_DECODER_END;
}



// RTCP //////////////////////////////////////////////////////////////////////

BOOL com_analyzeRtcp( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( com_sigRtcpHdrCommon_t ) );
    com_off  packetLen = 0;
    for( com_off offset = 0;  offset < COM_SGLEN;  offset += packetLen ) {
        com_bin*  sigTop = COM_SGTOP + offset;
        COM_CAST_HEAD( com_sigRtcpHdrCommon_t, rtcp, sigTop );
        packetLen = com_getVal16( rtcp->length, COM_ORDER ) + 1;
        packetLen *= COM_32BIT_SIZE;
        result = com_stackSigMulti( ioHead, "rtcp packet",
                           &(com_sigBin_t){sigTop, packetLen, COM_SIG_RTCP} );
        if( !result ) {break;}
        if( !com_getRtcpPtName( sigTop ) ) {result = false;  break;}
        COM_SGTYPE = COM_SIG_RTCP;
    }
    COM_ANALYZER_END;
}

void com_decodeRtcp( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispDec( " RTCP [%d]", COM_SIG_RTCP );
    for( long i = 0;  i < COM_IMLTICNT;  i++ ) {
        com_sigInf_t*  rtcp = &(COM_IMLTISTK[i]);
        com_dispSig( " packet", i, &rtcp->sig );
        com_dispPrm( "packet type", com_getRtcpPtName( rtcp->sig.top ), 0 );
    }
    COM_DECODER_END;
}

static com_decodeName_t  gRtcpPtName[] = {
    { COM_CAP_RTCP_PT_SR,    "SR" },     // Sender Report
    { COM_CAP_RTCP_PT_RR,    "RR" },     // Receiver Report
    { COM_CAP_RTCP_PT_SDES,  "SDES" },   // Source Description
    { COM_CAP_RTCP_PT_BYE,   "BYE" },    // Goodbye
    { COM_CAP_RTCP_PT_APP,   "APP" },    // Application-Defined
    COM_DECODENAME_END  // 最後は必ずこれで
};

char *com_getRtcpPtName( void *iSigTop )
{
    com_sigRtcpHdrCommon_t*  rtcp = iSigTop;
    return com_searchDecodeName( gRtcpPtName, rtcp->packetType, true );
}



// Diameter //////////////////////////////////////////////////////////////////

// 初期化用：Grouped AVP
static long  gDiamAvpg1[] = {
    /*** Base   (RFC6733) ***/
    260,    // Vendor-Specific-Application-Id
    279,    // Failed-AVP
    284,    // Proxy-Info
    297,    // Experimental-Result
    /*** Cx/Dx  (TS29.229 f10) ***/
    603,    // Server-Capabilities
    612,    // SIP-Auth-Data-Item
    615,    // Deregistration-Reason
    618,    // Charging-Information
    628,    // Supported-Features
    631,    // Supported-Applications
    632,    // Associated-Identity
    635,    // SIP-Digest-Authenticate
    639,    // SCSCF-Restoration-Info
    642,    // Subscription-Info
    647,    // Associated-Registered-Identities
    649,    // Restoration-Info
    651,    // Identity-With-Emergency-Registration
    621,    // CC-Supported-Features
    623,    // CC-OLR
    656,    // Allowed-WAF-WWSF-Identities
    /*** Sh/Dh  (TS29.329 f10) ***/
    700,    // User-Identity
    715,    // Repository-Data-ID
    720,    // Call-Reference-Info
    /*** Tx     (X.S0013-013) ***/
    502,    // Access-Network-Charging-Identifier
    510,    // Flows
    519,    // Media-Sub-Component
    900,    // Access-Network-Physical-Access-ID
    /*** Ty     (X.S0013-014) ***/
    1002,   // Charging-Rule-Remove
    1022,   // Access-Network-Charging-Identifier-Ty
    801,    // Charging-Rule-Install
    802,    // Charging-Rule-Defintion
    804,    // QoS-Information
    805,    // Charging-Rule-Report
    809,    // Flow-Info
    811,    // Granted-QoS
    812,    // Requested-QoS
    813     // Flow-Description-Info
};

static com_off addAvpList( com_sigPrm_t *oPrm, com_bin *iPtr );

static BOOL getAvps( com_sigPrm_t *oPrm, com_bin *iPtr, com_off iSize )
{
    while( iSize > 0 ) {
        com_off  avpLen = addAvpList( oPrm, iPtr );
        if( !avpLen ) {return false;}
        com_advancePtr( &iPtr, &iSize, avpLen );
    }
    return true;
}

static com_off addPadding( com_off iSize )
{
    if( iSize % COM_32BIT_SIZE ) {iSize += 4 - iSize % COM_32BIT_SIZE;}
    return iSize;
}

static com_off calcAvpHdrSize( com_sigDiamAvp_t *iAvpHdr )
{
    com_off  size = sizeof(*iAvpHdr);
    if( COM_CHECKBIT( iAvpHdr->flags, COM_CAP_DIAMAVP_VBIT ) ) {
        size += COM_32BIT_SIZE;
    }
    return size;
}

static com_off addAvpList( com_sigPrm_t *oPrm, com_bin *iPtr )
{
    COM_CAST_HEAD( com_sigDiamAvp_t, avpHdr, iPtr );
    com_off  avpHdrSize = calcAvpHdrSize( avpHdr );
    BOOL  result = 
        com_addPrm( oPrm,
                    (com_bin*)(&avpHdr->avpCode), sizeof(avpHdr->avpCode),
                    (com_bin*)(&avpHdr->avpLen),  sizeof(avpHdr->avpLen),
                    iPtr + avpHdrSize );
    if( !result ) {return 0;}
    com_sigTlv_t*  newAvp = com_getRecentPrm( oPrm );
    com_off  avpSize = addPadding( newAvp->len );
    newAvp->len -= avpHdrSize;
    if( com_getPrtclType( COM_DIAMAVPG, newAvp->tag ) ) {
        com_bin*  tmp = newAvp->value;
        newAvp->value = NULL;
        getAvps( oPrm, tmp, newAvp->len );
    }
    return avpSize;
}

static BOOL getDiamAvp( com_sigInf_t *ioHead )
{
    COM_CAST_HEAD( com_sigDiamHdr_t, diam, COM_SGTOP );
    com_bin*  ptr = COM_SGTOP + COM_SGLEN;
    com_off   len = com_calcValue( diam->msgLen, sizeof(diam->msgLen) )
                    - COM_SGLEN;  // == sizeof(com_sigDiamHdr_t)
    while( len > 9 ) {
        com_off  avpLen = addAvpList( COM_APRM, ptr );
        if( !avpLen ) {return false;}
        com_advancePtr( &ptr, &len, avpLen );
    }
    return true;
}

BOOL com_analyzeDiameter( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( com_sigDiamHdr_t ) );
    COM_CAST_HEAD( com_sigDiamHdr_t, diam, COM_SGTOP );
    if( diam->version == COM_CAP_DIAMHDR_VER && com_getDiameterCmdName(diam) ) {
        (void)com_setHeadInf( ioHead, HDRSIZE, COM_SIG_DIAMETER,
                              COM_SIG_ONLYHEAD );
        result = getDiamAvp( ioHead );
    }
    COM_ANALYZER_END;
}

static BOOL checkAsString( com_bin *iTop, com_off iSize )
{
    for( com_off i = 0;  i < iSize;  i++ ) {
        if( !isprint( iTop[i] ) ) {return false;}
    }
    return true;
}

static void dumpMulti( void *iTop, com_off iSize, BOOL isValue )
{
    com_bin*  top = iTop;
    BOOL  isString = checkAsString( top, iSize );
    if( !isValue ) {isString = false;}
    if( isString ) {com_printf( "\"" );}
    for( com_off i = 0;  i < iSize;  i++ ) {
        if( !isString ) {com_printf( "%02x", top[i] );}
        else {com_printf( "%c", top[i] );}
    }
    if( isString ) {com_printf( "\"" );}
    else {
        if( isValue && iSize == COM_32BIT_SIZE ) {
            com_printf( " (%u)", com_calcValue( iTop, iSize ) );
        }
    }
}

static void dispAvpHdrRest( com_bin *iTop )
{
    COM_CAST_HEAD( com_sigDiamAvp_t, avpHdr, iTop );
    com_off  size = calcAvpHdrSize( avpHdr );
    com_advancePtr( &iTop, &size, COM_32BIT_SIZE );
    dumpMulti( iTop, size, false );
}

// Grouped AVPかどうかを true/false で返す
static BOOL dispAvpValue( com_sigTlv_t *iAvp, long iLevel )
{
    BOOL  isGrouped = true;
    com_printf( "#     " );
    for( long i = 0;  i < iLevel;  i++ ) {com_printf( " " );}
    com_printf( "%08zx(%4zu) ", iAvp->tag, iAvp->tag );
    dispAvpHdrRest( iAvp->tagBin.top );
    com_printf( " <%3zu> ", iAvp->len );
    if( iAvp->value ) {
        dumpMulti( iAvp->value, iAvp->len, true );
        isGrouped = false;
    }
    // iAvp->valueが NULLの場合は Grouped AVP
    else {com_printf( "----- Grouped -----" );}
    com_printLf();
    return isGrouped;
}

static void dispAvpCascade( com_sigPrm_t *iPrm, long *iPos, long iLevel )
{
    com_off  groupLen = 0;
    if( iLevel ) {
        groupLen = iPrm->list[*iPos].len;
        (*iPos)++;
    }
    for( ;  *iPos < iPrm->cnt;  (*iPos)++ ) {
        com_sigTlv_t*  avp = &(iPrm->list[*iPos]);
        if( dispAvpValue( avp, iLevel ) ) {
            dispAvpCascade( iPrm, iPos, iLevel + 1 );
        }
        if( groupLen ) {
            groupLen -= addPadding(
                         com_calcValue( avp->lenBin.top, avp->lenBin.len ) );
            if( !groupLen ) {return;}
        }
    }
}

static void dispAvpList( com_sigPrm_t *iPrm )
{
    com_dispDec( " DIAMETER AVPs" );
    long  pos = 0;
    dispAvpCascade( iPrm, &pos, 0 );
}

void com_decodeDiameter( COM_DECODER_PRM )
{
    COM_DECODER_START;
    COM_CAST_HEAD( com_sigDiamHdr_t, diam, COM_ISGTOP );
    com_dispSig( "DIAMETER COMMON HEAD ", COM_SIG_DIAMETER, &COM_ISG );
    com_dispPrm( "Command", com_getDiameterCmdName( diam ), 0 );
    if( COM_CHECKBIT( diam->cmdFlags, COM_CAP_DIAMHDR_PBIT ) ) {
        com_dispPrm( "proxiable bit", "on", 0 );
    }
    if( COM_CHECKBIT( diam->cmdFlags, COM_CAP_DIAMHDR_EBIT ) ) {
        com_dispPrm( "    error bit", "on", 0 );
    }
    if( COM_CHECKBIT( diam->cmdFlags, COM_CAP_DIAMHDR_TBIT ) ) {
        com_dispPrm( " re-trans bit", "on", 0 );
    }
    com_dispPrm( "Application ID", &diam->AppliID, COM_32BIT_SIZE );
    com_dispPrm( " Hop-by-Hop ID", &diam->HopByHopID, COM_32BIT_SIZE );
    com_dispPrm( " End-to-End ID", &diam->EndToEndID, COM_32BIT_SIZE );
    dispAvpList( COM_IAPRM );
    COM_DECODER_END;
}

// 3文字目は com_getDiameterCmdName()の中で決定する
static com_decodeName_t  gDiameterCmdName[] = {
    { COM_CAP_DIAM_CE,  "CEx" },
    { COM_CAP_DIAM_DW,  "DWx" },
    { COM_CAP_DIAM_DP,  "DPx" },
    { COM_CAP_DIAM_UA,  "UAx" },
    { COM_CAP_DIAM_SA,  "SAx" },
    { COM_CAP_DIAM_LI,  "LIx" },
    { COM_CAP_DIAM_MA,  "MAx" },
    { COM_CAP_DIAM_RT,  "RTx" },
    { COM_CAP_DIAM_PP,  "PPx" },
    { COM_CAP_DIAM_UD,  "UDx" },
    { COM_CAP_DIAM_PU,  "PUx" },
    { COM_CAP_DIAM_SN,  "SNx" },
    { COM_CAP_DIAM_PN,  "PNx" },
    { COM_CAP_DIAM_RA,  "RAx" },
    { COM_CAP_DIAM_AA,  "AAx" },
    { COM_CAP_DIAM_AS,  "ASx" },
    { COM_CAP_DIAM_ST,  "STx" },
    { COM_CAP_DIAM_CC,  "CCx" },
    COM_DECODENAME_END
};

char *com_getDiameterCmdName( void *iSigTop )
{
    com_sigDiamHdr_t*  diam = iSigTop;
    long  cmdCode = com_calcValue( diam->cmdCode, sizeof(diam->cmdCode) );
    char*  name = com_searchDecodeName( gDiameterCmdName, cmdCode, true );
    if( !name ) {return NULL;}
    char*  op = name + 2;  // 3文字目を以下で決める
    uint8_t  flags = diam->cmdFlags;
    if( COM_CHECKBIT( flags, COM_CAP_DIAMHDR_RBIT ) ) {*op = 'R';}
    else if( COM_CHECKBIT( flags, COM_CAP_DIAMHDR_EBIT ) ) {*op = 'E';}
    else {*op = 'A';}
    return name;
}



// DHCP //////////////////////////////////////////////////////////////////////

// 初期化用：DHCPサーバーポート番号 (DHCPv6の分もまとめて登録)
static com_sigPrtclType_t  gDhcpSvPort[] = {
    { {COM_CAP_WPORT_DHCPSV},   COM_SIG_DHCP },
    { {COM_CAP_WPORT_DHCP6SV},  COM_SIG_DHCPV6 },
    { {COM_PRTCLTYPE_END}, COM_SIG_CONTINUE }
};

static BOOL getDhcpOption( com_sigInf_t *ioHead )
{
    if( COM_SGLEN <= sizeof( com_sigDhcpHdr_t ) ) {return true;}
    com_bin*  opt = COM_SGTOP + sizeof( com_sigDhcpHdr_t );
    while( *opt != COM_CAP_DHCP_OPT_END ) {
        if( *opt == COM_CAP_DHCP_OPT_PAD ) {return true;}
        com_off  optLen = *(opt + 1);
        com_addPrmDirect( COM_APRM, *opt, optLen, opt + 2 );
        opt += optLen + 2;
    }
    // optionの中に 53 (DHCP Message Type)があれば、有効値か判定
    return (com_getDhcpSigName( ioHead ) != NULL );
}

BOOL com_analyzeDhcp( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( com_sigDhcpHdr_t ) );
    if( com_getDhcpSigName( ioHead ) ) {
        COM_CAST_HEAD( com_sigDhcpHdr_t, dhcp, COM_SGTOP );
        BOOL isDhcp = ( com_getVal32( dhcp->magicCookie, COM_ORDER )
                        == COM_CAP_DHCP_MAGIC_COOKIE );
        if( isDhcp ) {
            result = getDhcpOption( ioHead );
            com_setHeadInf( ioHead, HDRSIZE, COM_SIG_DHCP, COM_SIG_ONLYHEAD );
        }
    }
    COM_ANALYZER_END;
}

void com_decodeDhcp( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispSig( "DHCP HEADER ", COM_SIG_DHCP, &COM_ISG );
    com_dispPrm( "type", com_getDhcpSigName( iHead ), 0 );
    if( COM_IPRMCNT ) {
        com_dispDec( " DHCP options" );
        com_dispPrmList( COM_IAPRM, 2, 2, NULL );
    }
    COM_DECODER_END;
}

static com_decodeName_t  gDhcpOpName[] = {
    { COM_CAP_DHCP_OP_BREQ,  "BOOT REQUEST" },
    { COM_CAP_DHCP_OP_BREP,  "BOOT REPLY" },
    COM_DECODENAME_END
};

static com_decodeName_t  gDhcpMsgName[] = {
    { COM_CAP_DHCP_MSG_DISCOVER,   "DISCOVER" },
    { COM_CAP_DHCP_MSG_OFFER,      "OFFER" },
    { COM_CAP_DHCP_MSG_REQUEST,    "REQUEST" },
    { COM_CAP_DHCP_MSG_DECLINE,    "DECLINE" },
    { COM_CAP_DHCP_MSG_ACK,        "ACK" },
    { COM_CAP_DHCP_MSG_NAK,        "NAK" },
    { COM_CAP_DHCP_MSG_RELEASE,    "RELEASE" },
    { COM_CAP_DHCP_MSG_INFORM,     "INFORM" },
    COM_DECODENAME_END
};

char *com_getDhcpSigName( com_sigInf_t *iDhcp )
{
    uint8_t*  op = iDhcp->sig.top;
    char*  sigName = com_searchDecodeName( gDhcpOpName, *op, true );
    if( !sigName ) {return NULL;}
    if( iDhcp->prm.cnt ) {
        com_sigTlv_t*  msgType = 
            com_searchPrm( &(iDhcp->prm), COM_CAP_DHCP_OPT_MSGTYPE );
        if( msgType ) {
            char*  msgName =
                com_searchDecodeName( gDhcpMsgName, *(msgType->value), true );
            if( msgName ) {sigName = msgName;}
            else {return NULL;}
        }
    }
    return sigName;
}



// DHCPv6 ////////////////////////////////////////////////////////////////////

static uint16_t getAs16bit( void *iTarget, BOOL iOrder )
{
    return com_getVal16( *((uint16_t*)iTarget), iOrder );
}

static BOOL getDhcpv6Option( com_sigInf_t *ioHead, com_off iHdrSize )
{
    com_bin*  opt = COM_SGTOP + iHdrSize;
    com_off  optLen = COM_SGLEN - iHdrSize;
    while( optLen > 4 ) {
        uint16_t  tag = getAs16bit( opt, COM_ORDER );
        uint16_t  len = getAs16bit( opt + 2, COM_ORDER );
        if( !com_addPrmDirect( COM_APRM, tag, len, opt + 4 ) ) {return false;}
        com_advancePtr( &opt, &optLen, len + 4 );
    }
    return true;
}

BOOL com_analyzeDhcpv6( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( com_sigDhcpv6Hdr_t ) );
    if( com_getDhcpv6MsgType( COM_SGTOP ) ) {
        com_bin  type = *COM_SGTOP;
        if( type == COM_CAP_DHCP6_MSG_RELAYFORM ||
            type == COM_CAP_DHCP6_MSG_RELAYREPL )
        {
            HDRSIZE = sizeof( com_sigDhcpv6Relay_t );
        }
        if( (result = getDhcpv6Option( ioHead, HDRSIZE )) ) {
            result = com_setHeadInf( ioHead, HDRSIZE, COM_SIG_DHCPV6,
                                     COM_SIG_ONLYHEAD );
        }
    }
    COM_ANALYZER_END;
}

void com_decodeDhcpv6( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispSig( "DHCPv6 HEADER", COM_SIG_DHCPV6, &COM_ISG );
    com_dispPrm( "type", com_getDhcpv6MsgType( COM_ISGTOP ), 0 );
    if( COM_IPRMCNT ) {
        com_dispDec( " DHCPv6 options" );
        com_dispPrmList( COM_IAPRM, 4, 4, NULL );
    }
    COM_DECODER_END;
}

static com_decodeName_t  gDhcpv6MsgName[] = {
    { COM_CAP_DHCP6_MSG_SOLICIT,       "SOLICIT" },
    { COM_CAP_DHCP6_MSG_ADVERTISE,     "ADVERTISE" },
    { COM_CAP_DHCP6_MSG_REQUEST,       "REQUEST" },
    { COM_CAP_DHCP6_MSG_CONFIRM,       "CONFIRM" },
    { COM_CAP_DHCP6_MSG_RENEW,         "RENEW" },
    { COM_CAP_DHCP6_MSG_REBIND,        "REBIND" },
    { COM_CAP_DHCP6_MSG_REPLY,         "REPLY" },
    { COM_CAP_DHCP6_MSG_RELEASE,       "RELEASE" },
    { COM_CAP_DHCP6_MSG_DECLINE,       "DECLINE" },
    { COM_CAP_DHCP6_MSG_RECONFIGURE,   "RECONFIGURE" },
    { COM_CAP_DHCP6_MSG_INFOREQUEST,   "INFORMATION-REQUEST" },
    { COM_CAP_DHCP6_MSG_RELAYFORM,     "RELAY-FORM" },
    { COM_CAP_DHCP6_MSG_RELAYREPL,     "RELAY-REPL" },
    COM_DECODENAME_END
};

char *com_getDhcpv6MsgType( void *iSigTop )
{
    uint8_t*  type = iSigTop;
    return com_searchDecodeName( gDhcpv6MsgName, *type, true );
}



// DNS ///////////////////////////////////////////////////////////////////////

// DNSヘッダのビットフィールド取得用データ構造
typedef struct {
    ulong  bitmask;
    long   shift;
} com_sigDnsBitMap_t;

static com_sigDnsBitMap_t  gDnsBitMap[] = {
    { 0x8000, 15 },  // QR      (1bit)
    { 0x7800, 11 },  // OPCODE  (4bit)
    { 0x0400, 10 },  // AA      (1bit)
    { 0x0200,  9 },  // TC      (1bit)
    { 0x0100,  8 },  // RD      (1bit)
    { 0x0080,  7 },  // RA      (1bit)
    { 0x0070,  4 },  // Z       (3bit)
    { 0x000f,  0 }   // RCODE   (4bit)
};

ulong com_getDnsFlagsField( com_sigInf_t *iHead, COM_CAP_DNSBIT_t iId )
{
    COM_CAST_HEAD( com_sigDnsHdr_t, dns, COM_ISGTOP );
    uint16_t  flags = com_getVal16( dns->flags, COM_IORDER );
    com_sigDnsBitMap_t*  tmp = &(gDnsBitMap[iId]);
    return com_getBitField( flags, tmp->bitmask, tmp->shift );
}

static BOOL getDnsMessage( com_sigInf_t *ioHead )
{
    com_freeSigInfExt( ioHead );  // 念のため一旦解放
    ioHead->ext = com_malloc( sizeof( com_sigDnsData_t ), "DNS data" );
    if( !(ioHead->ext) ) {return false;}
    com_sigDnsData_t*  tmpDns = ioHead->ext;
    *tmpDns = (com_sigDnsData_t){
        .qrbit  = com_getDnsFlagsField( ioHead, COM_CAP_DNSBIT_QR ),
        .opcode = com_getDnsFlagsField( ioHead, COM_CAP_DNSBIT_OPCODE ),
        .rcode  = com_getDnsFlagsField( ioHead, COM_CAP_DNSBIT_RCODE )
    };
    return true;
}

static com_off getDomain( com_sigBin_t *oDomain, com_bin *iSig, com_off iRest )
{
    oDomain->top = iSig;
    com_off  qnameSize = 0;
    for( ;  *iSig;  /* ループ増分はなし */ ) {
        if( COM_CHECKBIT( *iSig, COM_CAP_DNS_COMPRESS_BIT ) ) {
            qnameSize++;
            break;
        }
        qnameSize += *iSig + 1;
        if( !com_advancePtr( &iSig, &iRest, *iSig + 1 ) ) {return 0;}
    }
    qnameSize++;
    oDomain->len = qnameSize;
    return qnameSize;
}

static com_sigDnsRecord_t *getNewRd( com_sigInf_t *ioHead )
{
    com_sigDnsData_t*  tmpDns = ioHead->ext;
    com_sigDnsRecord_t*  newRec =
        com_reallocAddr( &(tmpDns->rd), sizeof(*(tmpDns->rd)),
                         COM_TABLEEND, &(tmpDns->rcnt), 1,
                         "DNS RD%ld", tmpDns->rcnt );
    return newRec;
}

static BOOL setQd(
        com_sigInf_t *ioHead, com_sigBin_t *iDomain, com_sigDnsQd_t *iQd )
{
    com_sigDnsRecord_t*  newRec = getNewRd( ioHead );
    if( !newRec ) {return false;}
    *newRec = (com_sigDnsRecord_t){
        .recSec = COM_CAP_DNS_QD,  .rname = *iDomain,
        .rtype =  (ulong)com_getVal16( iQd->qtype,  COM_ORDER ),
        .rclass = (ulong)com_getVal16( iQd->qclass, COM_ORDER )
        // question sectionのレコードは TTL や RDATA は存在しない
    };
    return true;
}

static com_off getQuestionSection(
        com_sigInf_t *ioHead, com_bin **ioPtr, com_off *ioRest )
{
    com_off  qdSize = 0;
    COM_CAST_HEAD( com_sigDnsHdr_t, dns, COM_SGTOP );
    ulong  qdcnt = (ulong)com_getVal16( dns->qdcount, COM_ORDER );
    for( ulong i = 0;  i < qdcnt;  i++ ) {
        com_sigBin_t  domain = { NULL, 0, 0 };
        com_off  qnameSize = getDomain( &domain, *ioPtr, *ioRest );
        if( !qnameSize ) {return 0;}
        if( !com_advancePtr( ioPtr, ioRest, qnameSize ) ) {return 0;}
        COM_CAST_HEAD( com_sigDnsQd_t, qd, *ioPtr );
        if( !setQd( ioHead, &domain, qd ) ) {return 0;}
        if( !com_advancePtr( ioPtr, ioRest, sizeof(*qd) ) ) {return 0;}
        qdSize += qnameSize + sizeof(*qd);
    }
    return qdSize;
}

static com_off getRdata( com_sigBin_t *oRdata, com_bin *iSig, com_off iRest )
{
    com_off  rdLen = com_calcValue( iSig, COM_16BIT_SIZE );
    if( !com_advancePtr( &iSig, &iRest, COM_16BIT_SIZE ) ) {return 0;}
    *oRdata = (com_sigBin_t){ iSig, rdLen, 0 };
    return rdLen;
}

static BOOL setRr(
        com_sigInf_t *ioHead, com_sigBin_t *iDomain, com_sigDnsRr_t *iRr,
        com_sigBin_t *iRdata, COM_CAP_DNS_RECSEC_t iRecSec )
{
    com_sigDnsRecord_t*  newRec = getNewRd( ioHead );
    if( !newRec ) {return false;}
    *newRec = (com_sigDnsRecord_t){
        .recSec = iRecSec,  .rname = *iDomain,
        .rtype  = (ulong)com_getVal16( iRr->rtype, COM_ORDER ),
        .rclass = (ulong)com_getVal16( iRr->rclass, COM_ORDER ),
        .rttl   = (ulong)com_getVal16( iRr->rttl, COM_ORDER ),
        .rdata  = *iRdata
    };
    return true;
}

static BOOL getRecords(
        com_sigInf_t *ioHead, com_bin **ioPtr, com_off *ioRest, ulong iCount,
        COM_CAP_DNS_RECSEC_t iRecSec )
{
    for( ulong i = 0;  i < iCount;  i++ ) {
        if( !(*ioRest) ) {return false;}
        com_sigBin_t  domain = { NULL, 0, 0 };
        com_off  nameSize = getDomain( &domain, *ioPtr, *ioRest );
        if( !nameSize ) {return false;}
        if( !com_advancePtr( ioPtr, ioRest, nameSize ) ) {return false;}
        COM_CAST_HEAD( com_sigDnsRr_t, rr, *ioPtr );
        if( !com_advancePtr( ioPtr, ioRest, sizeof(*rr) ) ) {return false;}
        com_sigBin_t  rdata = { NULL, 0, 0 };
        com_off  rdLen = getRdata( &rdata, *ioPtr, *ioRest );
        if( !rdLen ) {return false;}
        if( !setRr( ioHead, &domain, rr, &rdata, iRecSec ) ) {return false;}
        if( !com_advancePtr( ioPtr, ioRest, COM_16BIT_SIZE + rdLen ) ) {
            return false;
        }
    }
    return true;
}

static BOOL getResourceRecords(
        com_sigInf_t *ioHead, com_bin **ioPtr, com_off *ioRest )
{
    COM_CAST_HEAD( com_sigDnsHdr_t, dns, COM_SGTOP );
    ulong  ancnt = (ulong)com_getVal16( dns->ancount, COM_ORDER );
    if( !getRecords( ioHead, ioPtr, ioRest, ancnt, COM_CAP_DNS_AN ) ) {
        return false;
    }
    ulong  nscnt = (ulong)com_getVal16( dns->nscount, COM_ORDER );
    if( !getRecords( ioHead, ioPtr, ioRest, nscnt, COM_CAP_DNS_NS ) ) {
        return false;
    }
    ulong  arcnt = (ulong)com_getVal16( dns->arcount, COM_ORDER );
    if( !getRecords( ioHead, ioPtr, ioRest, arcnt, COM_CAP_DNS_AR ) ) {
        return false;
    }
    return true;
}

BOOL com_analyzeDns( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( com_sigDnsHdr_t ) );
    if( getDnsMessage( ioHead ) ) {
        com_bin*  ptr = COM_SGTOP;
        com_off  rest = COM_SGLEN;
        if( com_advancePtr( &ptr, &rest, HDRSIZE ) ) {
            com_off  qdSize = getQuestionSection( ioHead, &ptr, &rest );
            if( (result = (qdSize > 0)) ) {
                result = getResourceRecords( ioHead, &ptr, &rest );
            }
        }
        result =
            com_setHeadInf( ioHead, HDRSIZE, COM_SIG_DNS, COM_SIG_ONLYHEAD );
    }
    COM_ANALYZER_END;
}


static char*  gDnsQrBit[] = { "QUERY(0)", "RESPONSE(1)" };
static char*  gDnsRecType[] = {
    "question", "answer", "name server resource records", "additional records"
};

typedef struct {
    long    type;
    char*   label;
} com_sigDnsTypeUnit_t;

enum { RR_DATACNT_MAX = 8 };

typedef struct {
    long    type;
    char*   label;
    com_sigDnsTypeUnit_t  dataType[RR_DATACNT_MAX];
} com_sigDnsType_t;

#define  COM_ANYLABEL_ANY   "anything"
#define  COM_ANYLABEL_TXT   "TXT-DATA"

static com_sigDnsType_t  gDnsTypeInf[] = {
    { COM_CAP_DNS_RTYPE_A,      "A (1) host address",
      { {COM_CAP_DNS_RR_V4ADDR,    "ADDRESS"} } },
    { COM_CAP_DNS_RTYPE_NS,     "NS (2) authoritative name server",
      { {COM_CAP_DNS_RR_DNAME,     "NSDNAME"} } },
    { COM_CAP_DNS_RTYPE_MD,     "MD (3) mail destination",
      { {COM_CAP_DNS_RR_DNAME,     "MADNAME"} } },
    { COM_CAP_DNS_RTYPE_MF,     "MF (4) mail forwarder",
      { {COM_CAP_DNS_RR_DNAME,     "MADNAME"} } },
    { COM_CAP_DNS_RTYPE_CNAME,  "CNAME (5) canonical name for on alias",
      { {COM_CAP_DNS_RR_DNAME,     "CNAME"} } },
    { COM_CAP_DNS_RTYPE_SOA,    "SOA (6) start of authority",
      { {COM_CAP_DNS_RR_DNAME,     "MNAME"},
        {COM_CAP_DNS_RR_DNAME,     "RNAME"},
        {COM_CAP_DNS_RR_32BIT,     "SERIAL"},
        {COM_CAP_DNS_RR_32BIT,     "REFRESH"},
        {COM_CAP_DNS_RR_32BIT,     "RETRY"},
        {COM_CAP_DNS_RR_32BIT,     "EXPIRE"},
        {COM_CAP_DNS_RR_32BIT,     "MINIMUM"} } },
    { COM_CAP_DNS_RTYPE_MB,     "MB (7) mailbox domain name",
      { {COM_CAP_DNS_RR_DNAME,     "MADNAME"} } },
    { COM_CAP_DNS_RTYPE_MG,     "MG (8) mailbox group member",
      { {COM_CAP_DNS_RR_DNAME,     "MGMNAME"} } },
    { COM_CAP_DNS_RTYPE_MR,     "MG (9) mail rename domain name",
      { {COM_CAP_DNS_RR_DNAME,     "NEWNAME"} } },
    { COM_CAP_DNS_RTYPE_NULL,   "NULL (10)",
      { {COM_CAP_DNS_RR_ANY,       COM_ANYLABEL_ANY} } },  // 形式不問
    { COM_CAP_DNS_RTYPE_WKS,    "WKS (11) well known service description",
      { {COM_CAP_DNS_RR_V4ADDR,    "ADDRESS"},
        {COM_CAP_DNS_RR_8BIT,      "PROTOCOL"},
        {COM_CAP_DNS_RR_BITMAP,    "BIT MAP"} } },
    { COM_CAP_DNS_RTYPE_PTR,    "PTR (12) domain name pointer",
      { {COM_CAP_DNS_RR_DNAME,     "PTRDNAME"} } },
    { COM_CAP_DNS_RTYPE_HINFO,  "HINFO (13) host information",
      { {COM_CAP_DNS_RR_CHSTR,     "CPU"},
        {COM_CAP_DNS_RR_CHSTR,     "OS"} } },
    { COM_CAP_DNS_RTYPE_MINFO,  "MINFO (14) mailbox or mail list information",
      { {COM_CAP_DNS_RR_DNAME,     "RMAILBX"},
        {COM_CAP_DNS_RR_DNAME,     "EMAILBX"} } },
    { COM_CAP_DNS_RTYPE_MX,     "MX (15) mail exchange",
      { {COM_CAP_DNS_RR_16BIT,     "REFERENCE"},
        {COM_CAP_DNS_RR_DNAME,     "EXCHANGE"} } },
    { COM_CAP_DNS_RTYPE_TXT,    "TXT (16) text strings",
      { {COM_CAP_DNS_RR_TXT,       "TXT-DATA"} } },
    { COM_CAP_DNS_RTYPE_SRV,    "SRV (33) location of service",
      { {COM_CAP_DNS_RR_16BIT,     "Priority"},
        {COM_CAP_DNS_RR_16BIT,     "Weight"},
        {COM_CAP_DNS_RR_16BIT,     "Port"},
        {COM_CAP_DNS_RR_DNAME,     "Target"} } },
    { 0, NULL, {{0, NULL}} }  // 最後は必ずこれで
};

com_off com_getDomain(
        com_sigInf_t *iHead, void *iTop, char *oBuf, size_t iBufSize )
{
    com_off  size = 0;
    BOOL  calcSize = false;
    memset( oBuf, 0, iBufSize );
    for( com_bin* ptr = iTop;  *ptr;  /* 増分処理なし */ ) {
        if( COM_CHECKBIT( *ptr, COM_CAP_DNS_COMPRESS_BIT ) ) {
            uint16_t  offset = (com_calcValue( ptr, COM_16BIT_SIZE ) & 0x3f);
            ptr = COM_ISGTOP + offset;
            size += COM_16BIT_SIZE;
            calcSize = false;
            continue;
        }
        (void)com_strncat( oBuf, iBufSize, (char*)(ptr + 1), *ptr );
        if( calcSize ) {size += *ptr + 1;}
        ptr += *ptr + 1;
        if( *ptr && strlen(oBuf) < (iBufSize - 1) ) {strcat( oBuf, "." );}
    }
    return size;
}

static com_sigDnsType_t *getDnsTypeInf( long iType )
{
    if( iType < 0 ) {return NULL;}
    for( com_sigDnsType_t * inf = gDnsTypeInf;  inf->type;  inf++ ) {
        if( inf->type == iType ) {return inf;}
    }
    return NULL;
}

#define COM_DISP_RDATA_PRM \
    com_bin **ioPtr, com_off *ioRest, \
    com_sigDnsTypeUnit_t *iUnit, com_sigInf_t *iHead

typedef BOOL(*com_dispDnsRdata_t)( COM_DISP_RDATA_PRM );

static BOOL dispRd_DNAME( COM_DISP_RDATA_PRM )
{
    char  tmpName[COM_LINEBUF_SIZE + 1];
    com_off  size = com_getDomain( iHead, *ioPtr, tmpName, sizeof(tmpName) );
    if( !size ) {return false;}
    if( !com_advancePtr( ioPtr, ioRest, size ) ) {return false;}
    com_dispDec( "       (%s = %s)", iUnit->label, tmpName );
    return true;
}

static com_off getChrStr( char *oBuf, com_bin **ioPtr, com_off *ioRest )
{
    com_off  size = **ioPtr;
    memset( oBuf, 0, COM_LINEBUF_SIZE );  // サイズは固定
    memcpy( oBuf, *ioPtr, size );
    size++;  // 先頭オクテットの文字列長分、最終サイズは +1
    if( !com_advancePtr( ioPtr, ioRest, size ) ) {return false;}
    return true;
}

static BOOL dispRd_CHSTR( COM_DISP_RDATA_PRM )
{
    COM_UNUSED( iHead );
    char  tmpTxt[COM_LINEBUF_SIZE] = {0};
    if( !getChrStr( tmpTxt, ioPtr, ioRest ) ) {return false;}
    com_dispDec( "       (%s = %s)", iUnit->label, tmpTxt );
    return true;
}

static BOOL dispRdValue( COM_DISP_RDATA_PRM, long iSize )
{
    COM_UNUSED( iHead );
    ulong  value = (ulong)com_calcValue( *ioPtr, iSize );
    if( !com_advancePtr( ioPtr, ioRest, iSize ) ) {return false;}
    com_dispDec( "       (%s = %lu/0x%0*lx)",
                 iUnit->label, value, (int)iSize * 2, value );
    return true;
}

static BOOL dispRd_8BIT( COM_DISP_RDATA_PRM )
{
    return dispRdValue( ioPtr, ioRest, iUnit, iHead, COM_8BIT_SIZE );
}

static BOOL dispRd_16BIT( COM_DISP_RDATA_PRM )
{
    return dispRdValue( ioPtr, ioRest, iUnit, iHead, COM_16BIT_SIZE );
}

static BOOL dispRd_32BIT( COM_DISP_RDATA_PRM )
{
    return dispRdValue( ioPtr, ioRest, iUnit, iHead, COM_32BIT_SIZE );
}

static BOOL dispRd_V4ADDR( COM_DISP_RDATA_PRM )
{
    COM_UNUSED( iHead );
    char  tmpAddr[16] = {0};
    snprintf( tmpAddr, sizeof(tmpAddr), "%d.%d.%d.%d",
              *ioPtr[0], *ioPtr[1], *ioPtr[2], *ioPtr[3] );
    if( !com_advancePtr( ioPtr, ioRest, COM_32BIT_SIZE ) ) {return false;}
    com_dispDec( "       (%s = %s)", iUnit->label, tmpAddr );
    return true;
}

static BOOL dispRd_BITMAP( COM_DISP_RDATA_PRM )
{
    COM_UNUSED( iHead );
    com_dispDec( "       <%s>", iUnit->label );
    com_printBinary( *ioPtr, *ioRest,
                     &(com_printBin_t){ .prefix = "# ",  .colSeq = 1} );
    (void)com_advancePtr( ioPtr, ioRest, *ioRest );
    return true;
}

static BOOL dispRd_TXT( COM_DISP_RDATA_PRM )
{
    COM_UNUSED( iHead );
    char  tmpTxt[COM_LINEBUF_SIZE] = {0};
    com_dispDec( "       <%s>", iUnit->label );
    while( *ioRest ) {
        if( !getChrStr( tmpTxt, ioPtr, ioRest ) ) {return false;}
        com_dispDec( "        %s", tmpTxt );
    }
    return true;
}

static BOOL dispRd_ANY( COM_DISP_RDATA_PRM )
{
    if( !strcmp( iUnit->label, COM_ANYLABEL_ANY ) ) {
        return dispRd_BITMAP( ioPtr, ioRest, iUnit, iHead );
    }
    return false;
}

static com_dispDnsRdata_t  gDispRdata[] = {
    NULL,           dispRd_DNAME,    dispRd_CHSTR,    dispRd_8BIT,
    dispRd_16BIT,   dispRd_32BIT,    dispRd_V4ADDR,   dispRd_BITMAP,
    dispRd_ANY,     dispRd_TXT
};

static void dispRdata(
        com_sigInf_t *iHead, com_sigDnsRecord_t *iRd, com_sigDnsType_t *iInf )
{
    com_dispDec( "    TTL = %ld", iRd->rttl );
    if( !iInf ) {return;}
    com_sigDnsTypeUnit_t*  list = iInf->dataType;
    com_bin*  ptr = iRd->rdata.top;
    com_off  rest = iRd->rdata.len;
    for( long i = 0;  list[i].type && (i < RR_DATACNT_MAX);  i++ ) {
        com_dispDnsRdata_t  func = gDispRdata[list[i].type];
        if( func ) {
            com_dispDec( "    RDLEN = %zu", iRd->rdata.len );
            com_dispBin( "RDATA", iRd->rdata.top, iRd->rdata.len, "", true );
            if( !(func)( &ptr, &rest, &(list[i]), iHead ) ) {return;}
        }
    }
}

static void dispRrs( com_sigInf_t *iHead, com_sigDnsRecord_t *iRd )
{
    com_dispDec( "   %s section", gDnsRecType[iRd->recSec] );
    char*  qd = (iRd->recSec == COM_CAP_DNS_QD) ? "Q" : "";
    char  tmpName[COM_LINEBUF_SIZE + 1];
    (void)com_getDomain( iHead, iRd->rname.top, tmpName, sizeof(tmpName) );
    com_dispDec( "    %sNAME = %s", qd, tmpName );
    com_printf( "#    %sTYPE = ", qd );
    com_sigDnsType_t*  inf = getDnsTypeInf( iRd->rtype );
    if( inf ) {com_printf( "%s\n", inf->label );}
    else {com_printf( "%lu (unknwon type)\n", iRd->rtype );}
    com_dispDec( "    %sCLASS = %s", qd, com_getDnsOpName( iRd->rclass ) );
    if( iRd->recSec != COM_CAP_DNS_QD ) {dispRdata( iHead, iRd, inf );}
}

void com_decodeDns( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispSig( "DNS HEADER", COM_SIG_DNS, &COM_ISG );
    com_sigDnsData_t*  inf = iHead->ext;
    if( !inf ) {return;}
    char  opName[32] = {0};
    snprintf( opName, sizeof(opName), "%s/%s",
              com_getDnsOpName( inf->opcode ), gDnsQrBit[inf->qrbit] );
    com_dispPrm( "operation", opName, 0 );
    com_dispPrm( "RCODE", com_getDnsRcodeName( inf->rcode ), 0 );
    for( long i = 0;  i < inf->rcnt;  i++ ) {dispRrs( iHead, &(inf->rd[i]) );}
    COM_DECODER_END;
}

static com_decodeName_t  gDnsOpName[] = {
    { COM_CAP_DNS_OP_QUERY,    "QUERY" },
    { COM_CAP_DNS_OP_IQUERY,   "IQUERY" },
    { COM_CAP_DNS_OP_STATUS,   "STATUS" },
    COM_DECODENAME_END  // 最後は必ずこれで
};

char *com_getDnsOpName( ulong iOpcode )
{
    return com_searchDecodeName( gDnsOpName, iOpcode, true );
}

static com_decodeName_t  gDnsRcode[] = {
    { COM_CAP_DNS_RCODE_NOERR,     "no error condition" },
    { COM_CAP_DNS_RCODE_FORMAT,    "format error" },
    { COM_CAP_DNS_RCODE_SRVFAIL,   "server failure" },
    { COM_CAP_DNS_RCODE_NAME,      "name error" },
    { COM_CAP_DNS_RCODE_NOTIMPL,   "not implemented" },
    { COM_CAP_DNS_RCODE_REFUSED,   "refused" },
    COM_DECODENAME_END  // 最後は必ずこれで
};

char *com_getDnsRcodeName( ulong iRcode )
{
    return com_searchDecodeName( gDnsRcode, iRcode, true );
}

static com_decodeName_t  gDnsClass[] = {
    { COM_CAP_DNS_CLASS_IN,   "IN" },
    { COM_CAP_DNS_CLASS_CS,   "CS" },
    { COM_CAP_DNS_CLASS_CH,   "CH" },
    { COM_CAP_DNS_CLASS_HS,   "HS" },
    { COM_CAP_DNS_CLASS_ANY,  "ANY" },
    COM_DECODENAME_END  // 最後は必ずこれで
};

char *com_getDnsClassName( ulong iClass )
{
    return com_searchDecodeName( gDnsClass, iClass, true );
}

void com_freeDns( com_sigInf_t *oHead )
{
    if( !oHead ) {COM_PRMNG();}
    com_sigDnsData_t*  tmpDns = oHead->ext;
    if( !tmpDns ) {return;}
    com_free( tmpDns->rd );
    com_free( oHead->ext );
}



// 初期化処理 ////////////////////////////////////////////////////////////////

static void finalizeSigPrt1( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    freeTcpNodeInf();
    freeSdpSesInf();
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

static com_analyzeFuncList_t  gFuncSignal1[] = {
    { COM_SIG_SLL,         0,                   "SLL",
      com_analyzeSll,    com_decodeSll,    NULL },
    { COM_SIG_ETHER2,      0,                   "Ether2",
      com_analyzeEth2,   com_decodeEth2,   NULL },
    { COM_SIG_IPV4,        0,                   "IPv4",
      com_analyzeIpv4,   com_decodeIpv4,   NULL },
    { COM_SIG_IPV6,        0,                   "IPv6",
      com_analyzeIpv6,   com_decodeIpv6,   NULL },
    { COM_SIG_ICMP,        0,                   "ICMP",
      com_analyzeIcmp,   com_decodeIcmp,   NULL },
    { COM_SIG_ICMPV6,      0,                   "ICMPv6",
      com_analyzeIcmpv6, com_decodeIcmpv6, NULL },
    { COM_SIG_ARP,         0,                   "ARP",
      com_analyzeArp,    com_decodeArp,    NULL },
    { COM_SIG_TCP,         0,                   "TCP",
      com_analyzeTcp,    com_decodeTcp,    NULL },
    { COM_SIG_UDP,         0,                   "UDP",
      com_analyzeUdp,    com_decodeUdp,    NULL },
    { COM_SIG_SCTP,        0,                   "SCTP",
      com_analyzeSctp,   com_decodeSctp,   NULL },
    { COM_SIG_SIP,      COM_CAP_WPORT_SIP,      "SIP",
      com_analyzeSip,    com_decodeSip,    NULL },
    { COM_SIG_SDP,         0,                   "SDP",
      com_analyzeSdp,    com_decodeSdp,    NULL },
    { COM_SIG_RTP,      COM_CAP_WPORT_RTP,      "RTP",
      com_analyzeRtp,    com_decodeRtp,    NULL },
    { COM_SIG_RTCP,     COM_CAP_WPORT_RTCP,     "RTCP",
      com_analyzeRtcp,   com_decodeRtcp,   NULL },
    { COM_SIG_DIAMETER, COM_CAP_WPORT_DIAMETER, "DIAMETER",
      com_analyzeDiameter, com_decodeDiameter, NULL },
    { COM_SIG_DHCP,     COM_CAP_WPORT_DHCPCL,   "DHCP",
      com_analyzeDhcp,   com_decodeDhcp,   NULL },
    { COM_SIG_DHCPV6,   COM_CAP_WPORT_DHCP6CL,  "DHCPv6",
      com_analyzeDhcpv6, com_decodeDhcpv6, NULL },
    { COM_SIG_DNS,      COM_CAP_WPORT_DNS,      "DNS",
      com_analyzeDns,    com_decodeDns,    com_freeDns },
    { COM_SIG_END, 0, NULL, NULL, NULL, NULL }  // 最後は必ずこれで
};

void com_initializeSigSet1( void )
{
    // com_initializeSignal()から呼ばれる限り、COM_DEBUG_AVOID_～は不要
    atexit( finalizeSigPrt1 );
    (void)com_registerAnalyzer( gFuncSignal1, COM_IPPORT );
    com_setPrtclType( COM_LINKNEXT, gLinkNext1 );
    com_setBoolTable( COM_VLANTAG, gVlanTag, COM_ELMCNT(gVlanTag) );
    com_setPrtclType( COM_IPNEXT, gIpNext1 );
    com_setPrtclType( COM_IPOPTLEN, gIpOptLen1 );
    com_setBoolTable( COM_IP6XHDR, gIpv6Ext, COM_ELMCNT(gIpv6Ext) );
    com_setPrtclType( COM_TCPOPTLEN, gTcpOptLen1 );
    com_setPrtclType( COM_SCTPNEXT, gSctpNext1 );
    com_setPrtclType( COM_SIPNEXT, gSipNext1 );
    com_setBoolTable( COM_DIAMAVPG, gDiamAvpg1, COM_ELMCNT(gDiamAvpg1) );
    com_setPrtclType( COM_IPPORT, gDhcpSvPort );
}

