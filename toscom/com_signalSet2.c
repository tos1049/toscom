/*
 *****************************************************************************
 *
 * 共通部シグナル機能 (プロトコルセット2)    by TOS
 *
 *   外部公開I/Fの使用方法については com_signalSet2.h を参照。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"
#include "com_signal.h"
#include "com_signalSet2.h"


// M3UA //////////////////////////////////////////////////////////////////////

// SCTPの次プロトコルとして M3UAを登録
static com_sigPrtclType_t  gSctpNext2[] = {
    { {COM_CAP_SCTP_PROTO_M3UA},  COM_SIG_M3UA },
    { {COM_PRTCLTYPE_END}, COM_SIG_CONTINUE }
};

static BOOL isM3uaMessage( com_sigM3uaHdr_t *iM3ua )
{
    return( iM3ua->version  == COM_CAP_M3UA_VERSION &&
            iM3ua->reserved == COM_CAP_M3UA_RESERVED );
}

static BOOL hasM3uaData( com_sigM3uaHdr_t *iM3ua )
{
    return( iM3ua->msgClass == COM_CAP_M3UA_CLASS_TRANS && 
            iM3ua->msgType  == COM_CAP_M3UA_TYPE_DATA );
}

static BOOL getM3uaPrm( com_sigInf_t *iM3ua, com_sigInf_t *oHead )
{
    com_bin*  ptr = iM3ua->sig.top;
    COM_CAST_HEAD( com_sigM3uaHdr_t, m3ua, ptr );
    com_off  msgLen = com_getVal32( m3ua->msgLen, iM3ua->order );
    if( !com_advancePtr( &ptr, &msgLen, sizeof(*m3ua) ) ) {return false;}
    while( msgLen ) {
        COM_CAST_HEAD( com_sigM3uaPrm_t, prm, ptr );
        if( !com_addPrm( &oHead->prm, &prm->tag, sizeof(prm->tag),
                         &prm->len, sizeof(prm->len), prm->value ) )
        {
            return false;
        }
        // 信号上のパラメータ長からパラメータヘッダ長を引いておく
        (com_getRecentPrm( &oHead->prm ))->len -= sizeof(com_sigM3uaPrm_t);
        uint16_t  prmLen = com_getVal16( prm->len, iM3ua->order );
        prmLen =
            (uint16_t)(((prmLen - 1) / COM_32BIT_SIZE + 1) * COM_32BIT_SIZE);
        if( !com_advancePtr( &ptr, &msgLen, prmLen ) ) {return false;}
    }
    return true;
}

static BOOL setNextInf(
        com_sigInf_t *oNext, com_sigInf_t *ioHead, long iTargetType,
        BOOL iHasData )
{
    com_initSigInf( oNext, ioHead );
    oNext->sig.ptype = COM_SIG_END;
    if( !iHasData ) {return false;}
    com_sigTlv_t*  prm = com_searchPrm( COM_APRM, (com_off)iTargetType );
    if( !prm ) {return false;}
    oNext->sig = (com_sigBin_t){ prm->value, prm->len, COM_SIG_UNKNOWN };
    return true;
}

static BOOL getM3uaProtocolData( com_sigInf_t *ioHead )
{
    COM_CAST_HEAD( com_sigM3uaHdr_t, m3ua, COM_SGTOP );
    com_sigInf_t  body;
    if( setNextInf(&body,ioHead,COM_CAP_M3UA_PRM_PROTDATA,hasM3uaData(m3ua)) ) {
        body.sig.top  += sizeof(com_sigM3uaProtData_t);
        body.sig.len  -= sizeof(com_sigM3uaProtData_t);
        body.sig.ptype = COM_SIG_SCCP;
    }
    return (NULL != com_stackSigNext( ioHead, &body ));
}

BOOL com_analyzeM3ua( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( sizeof( com_sigM3uaHdr_t ) );
    COM_CAST_HEAD( com_sigM3uaHdr_t, m3ua, COM_SGTOP );
    if( isM3uaMessage( m3ua ) && com_getM3uaClassType( m3ua ) ) {
        (void)com_setHeadInf( ioHead, HDRSIZE, COM_SIG_M3UA, COM_SIG_ONLYHEAD );
        if( (result = getM3uaPrm( &orgHead, ioHead )) ) {
            result = getM3uaProtocolData( ioHead );
        }
    }
    COM_ANALYZER_END;
}

static com_sigM3uaProtData_t  gM3uaDummy;

static com_sigM3uaProtData_t *getM3uaDataPrm( com_sigInf_t *iHead )
{
    if( !iHead ) {return &gM3uaDummy;}
    com_sigTlv_t*  prm = com_searchPrm( COM_IAPRM, COM_CAP_M3UA_PRM_PROTDATA );
    if( !prm ) {return NULL;}
    COM_CAST_HEAD( com_sigM3uaProtData_t, pdata, prm->value );
    return pdata;
}

void com_decodeM3ua( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispSig( "M3UA HEADER ", COM_SIG_M3UA, &COM_ISG );
    COM_CAST_HEAD( com_sigM3uaHdr_t, m3ua, COM_ISGTOP );
    com_dispPrm( "class:type", com_getM3uaClassType( m3ua ), 0 );
    com_sigM3uaProtData_t*  pdata = getM3uaDataPrm( iHead );
    if( pdata ) {
        com_dispDec( "   -protocol data-" );
        com_dispPrm( "opc", &pdata->opc, COM_32BIT_SIZE );
        com_dispPrm( "dpc", &pdata->dpc, COM_32BIT_SIZE );
        com_dispPrm( "si ", &pdata->si,  COM_8BIT_SIZE );
        com_dispPrm( "ni ", &pdata->ni,  COM_8BIT_SIZE );
        com_dispPrm( "mp ", &pdata->ni,  COM_8BIT_SIZE );
        com_dispPrm( "sls", &pdata->sls, COM_8BIT_SIZE );
    }
    COM_DECODER_END;
}

static com_decodeName_t  gM3uaMgmtType[] = {
    { COM_CAP_M3UA_TYPE_ERR,        "MGMT(0):ERR(0)" },
    { COM_CAP_M3UA_TYPE_NTFY,       "MGMT(0):NTFY(1)" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

static com_decodeName_t  gM3uaTransType[] = {
    { COM_CAP_M3UA_TYPE_DATA,       "TRANS(1):DATA(1)" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

static com_decodeName_t  gM3uaSsnmType[] = {
    { COM_CAP_M3UA_TYPE_DUNA,       "SSNM(2):DUNA(1)" },
    { COM_CAP_M3UA_TYPE_DAVA,       "SSNM(2):DAVA(2)" },
    { COM_CAP_M3UA_TYPE_DAUD,       "SSNM(2):DUAD(3)" },
    { COM_CAP_M3UA_TYPE_SCON,       "SSNM(2):SCON(4)" },
    { COM_CAP_M3UA_TYPE_DUPU,       "SSNM(2):DUPU(5)" },
    { COM_CAP_M3UA_TYPE_DRST,       "SSNM(2):DRST(6)" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

static com_decodeName_t  gM3uaAspsmType[] = {
    { COM_CAP_M3UA_TYPE_ASPUP,      "ASPSM(3):ASPUP(1)" },
    { COM_CAP_M3UA_TYPE_ASPDN,      "ASPSM(3):ASPDN(2)" },
    { COM_CAP_M3UA_TYPE_BEAT,       "ASPSM(3):BEAT(3)" },
    { COM_CAP_M3UA_TYPE_ASPUPACK,   "ASPSM(3):ASPUP ACK(4)" },
    { COM_CAP_M3UA_TYPE_ASPDNACK,   "ASPSM(3):ASPDN ACK(5)" },
    { COM_CAP_M3UA_TYPE_BEATACK,    "ASPSM(3):BEAT ACK(6)" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

static com_decodeName_t  gM3uaAsptmType[] = {
    { COM_CAP_M3UA_TYPE_ASPAC,      "ASPTM(4):ASPAC(1)" },
    { COM_CAP_M3UA_TYPE_ASPIA,      "ASPTM(4):ASPIA(2)" },
    { COM_CAP_M3UA_TYPE_ASPACACK,   "ASPTM(4):ASPAC ACK(3)" },
    { COM_CAP_M3UA_TYPE_ASPIAACK,   "ASPTM(4):ASPIA ACK(4)" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

static com_decodeName_t  gM3uaRkmType[] = {
    { COM_CAP_M3UA_TYPE_REGREQ,     "RKM(9):REG REQ(1)" },
    { COM_CAP_M3UA_TYPE_REGRSP,     "RKM(9):REG RSP(2)" },
    { COM_CAP_M3UA_TYPE_DEREGREQ,   "RKM(9):DEREG REQ(3)" },
    { COM_CAP_M3UA_TYPE_DEREGRSP,   "RKM(9):DEREG RSP(4)" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

char *com_getM3uaClassType( void *iSigType )
{
    com_sigM3uaHdr_t*  m3ua = iSigType;
    uint8_t  mClass = m3ua->msgClass;
    uint8_t  mType  = m3ua->msgType;

    if( mClass == COM_CAP_M3UA_CLASS_MGMT ) {
        return com_searchDecodeName( gM3uaMgmtType, mType, false );
    }
    if( mClass == COM_CAP_M3UA_CLASS_TRANS ) {
        return com_searchDecodeName( gM3uaTransType, mType, false );
    }
    if( mClass == COM_CAP_M3UA_CLASS_SSNM ) {
        return com_searchDecodeName( gM3uaSsnmType, mType, false );
    }
    if( mClass == COM_CAP_M3UA_CLASS_ASPSM ) {
        return com_searchDecodeName( gM3uaAspsmType, mType, false );
    }
    if( mClass == COM_CAP_M3UA_CLASS_ASPTM ) {
        return com_searchDecodeName( gM3uaAsptmType, mType, false );
    }
    if( mClass == COM_CAP_M3UA_CLASS_RKM ) {
        return com_searchDecodeName( gM3uaRkmType, mType, false );
    }
    com_prmNG( "com_getM3uaClassType() %d:%d", mClass, mType );
    return false;
}



// SCCP //////////////////////////////////////////////////////////////////////

// SCCP.SSNと次プロトコルとの対応
static com_sigPrtclType_t  gSccpSsn2[] = {
    { {COM_CAP_SCCPSSN_ISUP}, COM_SIG_ISUP },
    { {COM_CAP_SCCPSSN_INAP}, COM_SIG_TCAP },
    { {COM_CAP_SCCPSSN_HLR},  COM_SIG_TCAP },
    { {COM_CAP_SCCPSSN_MSC},  COM_SIG_TCAP },
    { {COM_PRTCLTYPE_END}, COM_SIG_CONTINUE }
};

static com_sigNo7Form_t  gSccpForm[] = {
    { COM_CAP_SCCP_CR,
      (com_sigNo7Prm_t[]) {  // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_SCCPPRM_SLR,        3},
          {COM_CAP_SCCPPRM_PRTCLS,     1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]) {  // 可変長必須 (.lenはレングス長 ＊SCCPは基本1)
          {COM_CAP_SCCPPRM_CLDPAD,     1},
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_SCCP_CC,
      (com_sigNo7Prm_t[]) {  // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_SCCPPRM_DLR,        3},
          {COM_CAP_SCCPPRM_SLR,        3},
          {COM_CAP_SCCPPRM_PRTCLS,     1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]) {  // 可変長必須 (.lenはレングス長 ＊SCCPは基本1)
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_SCCP_CREF,
      (com_sigNo7Prm_t[]) {  // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_SCCPPRM_DLR,        3},
          {COM_CAP_SCCPPRM_REFCAUSE,   1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]) {  // 可変長必須 (.lenはレングス長 ＊SCCPは基本1)
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_SCCP_RLSD,
      (com_sigNo7Prm_t[]) {  // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_SCCPPRM_DLR,        3},
          {COM_CAP_SCCPPRM_SLR,        3},
          {COM_CAP_SCCPPRM_REFCAUSE,   1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]) {  // 可変長必須 (.lenはレングス長 ＊SCCPは基本1)
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_SCCP_RLC,
      (com_sigNo7Prm_t[]) {  // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_SCCPPRM_DLR,        3},
          {COM_CAP_SCCPPRM_SLR,        3},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]) {  // 可変長必須 (.lenはレングス長 ＊SCCPは基本1)
          {COM_CAP_NO7_END, 0}
      },
      false
    },
    { COM_CAP_SCCP_DT1,
      (com_sigNo7Prm_t[]) {  // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_SCCPPRM_DLR,        3},
          {COM_CAP_SCCPPRM_SEGRAS,     1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]) {  // 可変長必須 (.lenはレングス長 ＊SCCPは基本1)
          {COM_CAP_SCCPPRM_DATA,       1},
          {COM_CAP_NO7_END, 0}
      },
      false
    },
    { COM_CAP_SCCP_DT2,
      (com_sigNo7Prm_t[]) {  // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_SCCPPRM_DLR,        3},
          {COM_CAP_SCCPPRM_SEGRAS,     2},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]) {  // 可変長必須 (.lenはレングス長 ＊SCCPは基本1)
          {COM_CAP_SCCPPRM_DATA,       1},
          {COM_CAP_NO7_END, 0}
      },
      false
    },
    { COM_CAP_SCCP_UDT,
      (com_sigNo7Prm_t[]) {  // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_SCCPPRM_PRTCLS,     1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]) {  // 可変長必須 (.lenはレングス長 ＊SCCPは基本1)
          {COM_CAP_SCCPPRM_CLDPAD,     1},
          {COM_CAP_SCCPPRM_CLGPAD,     1},
          {COM_CAP_SCCPPRM_DATA,       1},
          {COM_CAP_NO7_END, 0}
      },
      false
    },
    { COM_CAP_SCCP_XUDT,
      (com_sigNo7Prm_t[]) {  // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_SCCPPRM_PRTCLS,     1},
          {COM_CAP_SCCPPRM_HOPCNT,     1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]) {  // 可変長必須 (.lenはレングス長 ＊SCCPは基本1)
          {COM_CAP_SCCPPRM_CLDPAD,     1},
          {COM_CAP_SCCPPRM_CLGPAD,     1},
          {COM_CAP_SCCPPRM_DATA,       1},
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_SCCP_LUDT,
      (com_sigNo7Prm_t[]) {  // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_SCCPPRM_PRTCLS,     1},
          {COM_CAP_SCCPPRM_HOPCNT,     1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]) {  // 可変長必須 (.lenはレングス長 ＊SCCPは基本1)
          {COM_CAP_SCCPPRM_CLDPAD,     1},
          {COM_CAP_SCCPPRM_CLGPAD,     1},
          {COM_CAP_SCCPPRM_LONGDATA,   2},  // できるはずだが未検証
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { 0, NULL, NULL, false }  // 最後は必ずこれで
};

static BOOL getFixPrm(
        com_sigPrm_t *oPrm, com_bin **ioPtr, com_off *ioLen,
        com_sigNo7Prm_t *iFix )
{
    for( long i = 0;  iFix[i].tag != COM_CAP_NO7_END;  i++ ) {
        if( !com_addPrmDirect( oPrm, iFix[i].tag, iFix[i].len, *ioPtr ) ) {
            return false;
        }
        if( !com_advancePtr( ioPtr, ioLen, iFix[i].len ) ) {return false;}
    }
    return true;
}

static BOOL getVarPrm(
        com_sigPrm_t *oPrm, com_bin **ioPtr, com_off *ioLen,
        com_sigNo7Prm_t *iVar )
{
    for( long i = 0;  iVar[i].tag != COM_CAP_NO7_END;  i++ ) {
        com_bin*  vPos = (*ioPtr) + (**ioPtr);
        com_off   vLen = com_calcValue( vPos, iVar[i].len );
        if( !com_addPrmDirect( oPrm, iVar[i].tag, vLen, vPos + iVar[i].len )) {
            return false;
        }
        if( !com_advancePtr( ioPtr, ioLen, 1 ) ) {return false;}
    }
    return true;
}

static BOOL getOptPrm( com_sigPrm_t *oPrm, com_bin **ioPtr, com_off *ioLen )
{
    if( **ioPtr == COM_CAP_SCCPPRM_END ) {return true;}
    if( !com_advancePtr( ioPtr, ioLen, **ioPtr ) ) {return false;}
    while( **ioPtr != COM_CAP_SCCPPRM_END ) {
        com_off  prmLen = *(*ioPtr + 1);
        if( !com_addPrmDirect( oPrm, **ioPtr, prmLen, *ioPtr + 2 ) ) {
            return false;
        }
        if( !com_advancePtr( ioPtr, ioLen, prmLen + 2 ) ) {return false;}
    }
    return true;
}

static BOOL seekNo7Msg( com_sigInf_t *ioHead, com_sigNo7Form_t *iForm )
{
    com_bin*  ptr = COM_SGTOP;
    com_off   len = COM_SGLEN;
    if( !com_advancePtr( &ptr, &len, 1 ) ) {return false;}  // 信号種別
    if( !getFixPrm( COM_APRM, &ptr, &len, iForm->fix ) ) {return false;}
    if( !getVarPrm( COM_APRM, &ptr, &len, iForm->var ) ) {return false;}
    if( len && iForm->hasOpt ) {
        if( !getOptPrm( COM_APRM, &ptr, &len ) ) {return false;}
    }
    return true;
}

BOOL com_getNo7Prm( com_sigInf_t *ioHead, com_sigNo7Form_t *iForm, ulong iType )
{
    if( !ioHead || !iForm ) {COM_PRMNG(false);}
    for( long i = 0;  iForm[i].msgType;  i++ ) {
        if( iForm[i].msgType == iType ) {
            return seekNo7Msg( ioHead, &iForm[i] );
        }
    }
    return false;
}


typedef struct {
    BOOL          isUse;
    uint32_t      opc;
    uint32_t      dpc;
    com_sigTlv_t  slr;
    com_sigTlv_t  dlr;
    com_sigTlv_t  cldpad;
} sccpConnInf_t;

static long gSccpConnCnt = 0;
static sccpConnInf_t*  gSccpConnInf = NULL;

static void clearSccpConnInf( sccpConnInf_t *oInf )
{
    if( oInf->isUse ) {
        oInf->isUse = false;
        com_free( oInf->slr.value );
        com_free( oInf->dlr.value );
        com_free( oInf->cldpad.value );
    }
}

static void freeSccpConnInf( void )
{
    for( long i = 0;  i < gSccpConnCnt;  i++ ) {
        clearSccpConnInf( &(gSccpConnInf[i]) );
    }
    gSccpConnCnt = 0;
    com_free( gSccpConnInf );
}

static void collectSccpConnData(
        com_sigInf_t *iHead, com_sigM3uaProtData_t **oM3ua,
        com_sigTlv_t **oSlr, com_sigTlv_t **oDlr, com_sigTlv_t **oCldpad )
{
    COM_SET_IF_EXIST( oM3ua, getM3uaDataPrm( iHead->prev ) );
    COM_SET_IF_EXIST( oSlr, com_searchPrm( COM_IAPRM, COM_CAP_SCCPPRM_SLR ) );
    COM_SET_IF_EXIST( oDlr, com_searchPrm( COM_IAPRM, COM_CAP_SCCPPRM_DLR ) );
    COM_SET_IF_EXIST( oCldpad,
                      com_searchPrm( COM_IAPRM, COM_CAP_SCCPPRM_CLDPAD ) );
}

static BOOL matchAnyPc( uint32_t iPc, sccpConnInf_t *iInf )
{
    if( iPc == iInf->opc || iPc == iInf->dpc ) {return true;}
    return false;
}

static BOOL matchLr( com_sigTlv_t *iLr1, com_sigTlv_t *iLr2 )
{
    if( iLr1->len != iLr2->len ) {return false;}
    for( com_off i = 0;  i < iLr1->len;  i++ ) {
        if( iLr1->value[i] != iLr2->value[i] ) {return false;}
    }
    return true;
}

static BOOL matchAnyLr( com_sigTlv_t *iLr, sccpConnInf_t *iInf )
{
    if( !iLr ) {return false;}
    if( matchLr( iLr, &iInf->slr ) ) {return true;}
    if( matchLr( iLr, &iInf->dlr ) ) {return true;}
    return false;
}

static sccpConnInf_t *getUsableInf( long *oId )
{
    for( *oId = 0;  *oId < gSccpConnCnt;  (*oId)++ ) {
        sccpConnInf_t*  inf = &(gSccpConnInf[*oId]);
        if( !inf->isUse ) {
            memset( inf, 0, sizeof(*inf) );
            return inf;
        }
    }
    sccpConnInf_t*  inf =
        com_reallocAddr( &gSccpConnInf, sizeof(*gSccpConnInf), COM_TABLEEND,
                         &gSccpConnCnt, 1, "sccp connection inf" );
    *oId = gSccpConnCnt - 1;
    return inf;
}

static sccpConnInf_t *getSccpConnInf(
        uint32_t iOpc, uint32_t iDpc, com_sigTlv_t *iSlr, com_sigTlv_t *iDlr,
        long *oId, BOOL iAdd )
{
    if( !iSlr ) {com_error( COM_ERR_NODATA, "no slr data" );  return NULL;}
    for( *oId = 0;  *oId < gSccpConnCnt;  (*oId)++ ) {
        sccpConnInf_t*  inf = &(gSccpConnInf[*oId]);
        if( !inf->isUse ) {continue;}
        if( !matchAnyPc( iOpc,inf ) || !matchAnyPc( iDpc,inf ) ) {continue;}
        if( !matchAnyLr( iSlr,inf ) ) {continue;}
        if( iDlr ) {if( !matchAnyLr( iDlr,inf ) ) {continue;}}
        return inf;
    }
    if( !iAdd ) {return NULL;}
    sccpConnInf_t*  inf = getUsableInf( oId );
    if( inf ) {
        *inf = (sccpConnInf_t){ .isUse = true, .opc = iOpc, .dpc = iDpc };
    }
    return inf;
}

#define  SCCPCONINF  "sccp connection information"

#define  COLLECT_SCCP_CONN_DATA( HEAD )  \
    com_sigM3uaProtData_t*  m3uaPrm; \
    com_sigTlv_t  *slr, *dlr, *cldpad; \
    collectSccpConnData( (HEAD), &m3uaPrm, &slr, &dlr, &cldpad );

static void regSccpConn( com_sigInf_t *iHead, ulong iType )
{
    if( !iHead->prev ) {return;}  // 下位プロトコル情報がない時は何もしない
    COLLECT_SCCP_CONN_DATA( iHead );
    long  id = -1;
    if( iType == COM_CAP_SCCP_CR ) {
        sccpConnInf_t*  inf =
            getSccpConnInf( m3uaPrm->opc, m3uaPrm->dpc, slr, NULL, &id, true );
        if( !inf ) {return;}
        (void)com_dupPrm( &inf->slr, slr );
        (void)com_dupPrm( &inf->cldpad, cldpad );
        com_dispDec( "    <<set %s#%ld>>", SCCPCONINF, id );
    }
    else if( iType == COM_CAP_SCCP_CC ) {
        sccpConnInf_t*  inf =
            getSccpConnInf( m3uaPrm->dpc, m3uaPrm->opc, dlr, NULL, &id, false );
        if( !inf ) {return;}
        (void)com_dupPrm( &inf->dlr, slr );  // コピー先は slr で正しい
        com_dispDec( "    <<complete %s#%ld>>", SCCPCONINF, id );
    }
}

static void judgeFreeConnInf( void )
{
    for( long i = 0;  i < gSccpConnCnt;  i++ ) {
        if( gSccpConnInf[i].isUse ) {return;}
    }
    // 全データが未使用の場合、gSccpConnInfをメモリ解放する
    gSccpConnCnt = 0;
    com_free( gSccpConnInf );
}

#define  LRVALUE( LR ) \
    (LR)->value[0], (LR)->value[1], (LR)->value[2]

static void canSccpConn( com_sigInf_t *iHead )
{
    COLLECT_SCCP_CONN_DATA( iHead );
    long  id = -1;
    sccpConnInf_t*  inf =
        getSccpConnInf( m3uaPrm->dpc, m3uaPrm->opc, dlr, slr, &id, false );
    if( !inf ) {
        com_error( COM_ERR_ANALYZENG,
                   "no sccp conn inf(%02x%02x%02x:%02x%02x%02x)",
                   LRVALUE( slr ), LRVALUE( dlr ) );
        return;
    }
    com_dispDec( "    <<discard %s#%ld", SCCPCONINF, id );
    clearSccpConnInf( inf );  // 削除ではなく、空きとして再利用可能とする
    judgeFreeConnInf();
}

static void getSccpConn( com_sigInf_t *ioHead, ulong iType )
{
    if( iType == COM_CAP_SCCP_CR || iType == COM_CAP_SCCP_CC ) {
        regSccpConn( ioHead, iType );
    }
    if( iType == COM_CAP_SCCP_CREF || iType == COM_CAP_SCCP_RLSD ) {
        canSccpConn( ioHead );
    }
}

static long getSccpDataTag( ulong iType )
{
    if( iType == COM_CAP_SCCP_LUDT ) {return COM_CAP_SCCPPRM_LONGDATA;}
    return COM_CAP_SCCPPRM_DATA;
}

enum {
    NO_SSN = 0,             // SSN無し
    ADDRID_SIZE = 1,        // アドレス識別子のサイズ (1固定)
    POINTCODE_SIZE = 2,     // PC(信号局コード)のサイズ (2固定)
    SSN_SIZE = 1            // SSN(サブシステム番号)のサイズ (1固定)
};

// SCCPアドレスビット列構造 (リトルエンディアンのみ考慮)
typedef struct {
    uint  pcId:1;        // 信号局コード(PC)識別子
    uint  ssnId:1;       // サブシステム番号(SSN)識別子
    uint  gtId:4;        // グローバルタイトル(GT)識別子
    uint  routingId:1;   // ルーチン具識別子
    uint  reserved:1;    // 国内使用のため留保
    uint8_t  dmy[7];
} sigSccpAddrId_t;

static long getSsn( com_sigTlv_t *iAdrInf )
{
    if( !iAdrInf ) {return NO_SSN;}
    com_bin*  val = iAdrInf->value;
    COM_CAST_HEAD( sigSccpAddrId_t, addrId, val );
    if( !addrId->ssnId ) {return NO_SSN;}
    val += ADDRID_SIZE;
    if( addrId->pcId ) {val += POINTCODE_SIZE;}
    return *val;
}

static void getSccpSsn( com_sigInf_t *ioHead )
{
    long  ssn = getSsn( com_searchPrm( COM_APRM, COM_CAP_SCCPPRM_CLDPAD ) );
    if( !ssn ) {
        ssn = getSsn( com_searchPrm( COM_APRM, COM_CAP_SCCPPRM_CLGPAD ) );
    }
    if( !ssn ) {  //DT1/DT2の場合、LRから cldpadを引き出す
        COLLECT_SCCP_CONN_DATA( ioHead );
        if( !slr && !dlr ) {return;}
        if( !slr ) {slr = dlr;  dlr = NULL;}
        long  id = -1;
        sccpConnInf_t*  inf = 
            getSccpConnInf( m3uaPrm->dpc, m3uaPrm->opc, slr, dlr, &id, false );
        if( inf ) {
            com_dispDec( "    <<match %s#%ld>>", SCCPCONINF, id );
            ssn = getSsn( &inf->cldpad );
        }
    }
    if( !ssn ) {return;}
    com_free( ioHead->ext );
    if( (ioHead->ext = com_malloc( sizeof(ssn), "hit ssn" )) ) {
        long*  tmp = ioHead->ext;
        *tmp = ssn;
    }
}

static BOOL sccpSeg( com_sigInf_t *ioBody )
{
    ioBody->isFragment = true;
    return true;
}

static BOOL reassembleSccp( com_sigInf_t *ioBody, com_sigFrg_t *iFrg )
{
    com_off  totalLen = 0;
    for( long i = 0;  i < iFrg->cnt;  i++ ) {totalLen += iFrg->inf[i].bin.len;}
    com_bin*  top = com_malloc( totalLen, "sccp reassemble" );
    if( !top ) {return false;}
    com_bin*  ptr = top;
    for( long i = (long)(iFrg->segMax - 1);  i >= 0;  i-- ) {
        // 負数でループ抜けるようにしたいので、i は敢えて long型で定義
        BOOL  found = false;
        for( long j = 0;  j < iFrg->cnt;  j++ ) {
            com_sigSeg_t*  segInf = &(iFrg->inf[j]);
            if( (ulong)i == segInf->seg ) {
                found = true;
                memcpy( ptr, segInf->bin.top, segInf->bin.len );
                ptr += segInf->bin.len;
            }
        }
        if( !found ) {com_free(top);  return false;}
    }
    ioBody->ras = (com_sigBin_t){ top, totalLen, COM_SIG_UNKNOWN };
    return true;
}

static void setSccpRas( com_sigInf_t *ioBody )
{
    // ioBody->sig.ptypeを上書き変更しないように留意
    ioBody->sig.top = ioBody->ras.top;
    ioBody->sig.len = ioBody->ras.len;
    ioBody->ras.ptype *= -1;  // 使用済みであることを明示するため負数に
}

static BOOL procSccpFragment(
        com_sigInf_t *ioHead, com_sigTlv_t *iSeg, com_sigInf_t *ioBody )
{
    COM_CAST_HEAD( com_sigSccpSegment_t, seg, iSeg->value );
    com_sigM3uaProtData_t*  m3uaPrm = getM3uaDataPrm( ioHead->prev );
    com_sigFrgCond_t  cond = {
        COM_SIG_SCCP, com_calcValue( seg->segLr, sizeof(seg->segLr) ),
        {(com_bin*)&m3uaPrm->opc, sizeof(m3uaPrm->opc), 0},
        {(com_bin*)&m3uaPrm->dpc, sizeof(m3uaPrm->dpc), 0}, 0, 0
    };
    ulong  remSeg = (seg->segFlags & COM_CAP_SCCP_REM_SEG);
    com_sigFrg_t*  frg = com_stockFragments( &cond, remSeg, &ioBody->sig );
    if( COM_CHECKBIT( seg->segFlags, COM_CAP_SCCP_1ST_SEG ) ) {
        frg->segMax = remSeg + 1;
    }
    if( !frg->segMax ) {return sccpSeg( ioBody );}
    BOOL  result = true;
    // 結合データ取得済みなら、それを優先 (テキストログ読み込み時のみ)
    if( ioHead->ras.ptype == COM_SIG_SCCP ) {ioBody->ras = ioHead->ras;}
    else {
        if( frg->segMax > (ulong)(frg->cnt) ) {return sccpSeg( ioBody );}
        result = reassembleSccp( ioBody, frg );
    }
    com_freeFragments( &cond );
    if( result ) {setSccpRas( ioBody );}
    return result;
}

static BOOL getSccpPayload( com_sigInf_t *ioHead, com_sigInf_t *oBody )
{
    if( !ioHead->ext ) {getSccpSsn( ioHead );}
    if( ioHead->ext ) {
        ulong*  ssn = ioHead->ext;
        oBody->sig.ptype = com_getPrtclType( COM_SCCPSSN, *ssn );
    }
    else {return false;}
    // TODO:DT1/DT2のフラグメントには未対応
    com_sigTlv_t*  seg = com_searchPrm( COM_APRM, COM_CAP_SCCPPRM_SEGMENT );
    BOOL  frgRet = true;
    if( seg ) {frgRet = procSccpFragment( ioHead, seg, oBody );}
    if( frgRet ) {return (NULL != com_stackSigNext( ioHead, oBody ));}
    return true;
}

BOOL com_analyzeSccp( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( COM_NO_MIN_LEN );
    if( com_getSccpMsgName( COM_SGTOP ) ) {
        ulong  type = *COM_SGTOP;
        result = com_getNo7Prm( ioHead, gSccpForm, type );
        COM_SGTYPE = COM_SIG_SCCP;
        if( result ) {getSccpConn( ioHead, type );}
        com_sigInf_t  body;
        if( result && setNextInf( &body,ioHead,getSccpDataTag(type),true ) ) {
            result = getSccpPayload( ioHead, &body );
        }
    }
    COM_ANALYZER_END;
}

static BOOL judgeData( com_bin iTag )
{
    if( iTag == COM_CAP_SCCPPRM_DATA || iTag == COM_CAP_SCCPPRM_LONGDATA ) {
        return false;
    }
    return true;
}

static void dispNextFromSsn( ulong *iSsn, long iNextType )
{
    if( !iSsn ) {return;}
    com_dispNext( *iSsn, 1, iNextType );
}

void com_decodeSccp( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispDec( " SCCP MESSAGE [%d]", COM_SIG_SCCP );
    com_dispPrm( "message type", com_getSccpMsgName( COM_ISGTOP ), 0 );
    com_dispPrmList( COM_IAPRM, 2, 2, judgeData );
    if( COM_INEXTCNT ) {
        dispNextFromSsn( iHead->ext, com_getSigType( COM_INEXTSTK ) );
    }
    if( com_searchPrm( COM_IAPRM, COM_CAP_SCCPPRM_SEGMENT ) ) {
        if( COM_INEXTSTK->ras.top ) {
            com_dispDec( "    <reassembled (size=%zu)", COM_INEXTSTK->ras.len );
        }
    }
    COM_DECODER_END;
}

static com_decodeName_t  gSccpMsg[] = {
    { COM_CAP_SCCP_CR,       "CR" },
    { COM_CAP_SCCP_CC,       "CC" },
    { COM_CAP_SCCP_CREF,     "CREF" },
    { COM_CAP_SCCP_RLSD,     "RLSD" },
    { COM_CAP_SCCP_RLC,      "RLC" },
    { COM_CAP_SCCP_DT1,      "DT1" },
    { COM_CAP_SCCP_DT2,      "DT2" },
    { COM_CAP_SCCP_AK,       "AK" },
    { COM_CAP_SCCP_UDT,      "UDT" },
    { COM_CAP_SCCP_UDTS,     "UDTS" },
    { COM_CAP_SCCP_ED,       "ED" },
    { COM_CAP_SCCP_EA,       "EA" },
    { COM_CAP_SCCP_RSR,      "RSR" },
    { COM_CAP_SCCP_RSC,      "RSC" },
    { COM_CAP_SCCP_ERR,      "ERR" },
    { COM_CAP_SCCP_IT,       "IT" },
    { COM_CAP_SCCP_XUDT,     "XUDT" },
    { COM_CAP_SCCP_XUDTS,    "XUDTS" },
    { COM_CAP_SCCP_LUDT,     "LUDT" },
    { COM_CAP_SCCP_LUDTS,    "LUDTS" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

char *com_getSccpMsgName( void *iSigTop )
{
    com_bin*  sccp = iSigTop;
    return com_searchDecodeName( gSccpMsg, *sccp, true );
}



// ISUP //////////////////////////////////////////////////////////////////////

// SIPの次プロトコルとして ISUPを登録 (SCCP側の登録は gSccpSsn2[]にて)
static com_sigPrtclType_t  gSipNext2[] = {
    { {.label=COM_CAP_CTYPE_ISUP}, COM_SIG_ISUP },
    { {COM_PRTCLTYPE_END}, COM_SIG_UNKNOWN }
};

static com_sigNo7Form_t  gIsupForm[] = {
    { COM_CAP_ISUP_IAM,
      (com_sigNo7Prm_t[]){ // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_ISUPPRM_CONIND,    1},
          {COM_CAP_ISUPPRM_FWDCIND,   2},
          {COM_CAP_ISUPPRM_CNGPCAT,   1},
          {COM_CAP_ISUPPRM_TRMEDREQ,  1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]){ // 可変長必須 (.lenはレングス長)
          {COM_CAP_ISUPPRM_CLDPN,     1},
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_ISUP_ACM,
      (com_sigNo7Prm_t[]){ // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_ISUPPRM_BWDCIND,   2},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]){ // 可変長必須 (.lenはレングス長)
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_ISUP_ANM,
      (com_sigNo7Prm_t[]){ // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]){ // 可変長必須 (.lenはレングス長)
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_ISUP_CPG,
      (com_sigNo7Prm_t[]){ // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_ISUPPRM_EVINF,     1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]){ // 可変長必須 (.lenはレングス長)
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_ISUP_CON,
      (com_sigNo7Prm_t[]){ // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_ISUPPRM_BWDCIND,   2},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]){ // 可変長必須 (.lenはレングス長)
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_ISUP_REL,
      (com_sigNo7Prm_t[]){ // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]){ // 可変長必須 (.lenはレングス長)
          {COM_CAP_ISUPPRM_CAUSE,     1},
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_ISUP_RLC,
      (com_sigNo7Prm_t[]){ // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]){ // 可変長必須 (.lenはレングス長)
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_ISUP_RES,
      (com_sigNo7Prm_t[]){ // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_ISUPPRM_SUSRES,    1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]){ // 可変長必須 (.lenはレングス長)
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_ISUP_SUS,
      (com_sigNo7Prm_t[]){ // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_ISUPPRM_SUSRES,    1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]){ // 可変長必須 (.lenはレングス長)
          {COM_CAP_NO7_END, 0}
      },
      true
    },
    { COM_CAP_ISUP_CHG,
      (com_sigNo7Prm_t[]){ // 固定長必須 (.lenはパラメータ長)
          {COM_CAP_ISUPPRM_CHGTYPE,   1},
          {COM_CAP_NO7_END, 0}
      },
      (com_sigNo7Prm_t[]){ // 可変長必須 (.lenはレングス長)
          {COM_CAP_ISUPPRM_CHGINF,    1},
          {COM_CAP_NO7_END, 0}
      },
      true
    }
};

BOOL com_analyzeIsup( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( COM_NO_MIN_LEN );
    if( com_getIsupMsgName( COM_SGTOP ) ) {
        result = com_getNo7Prm( ioHead, gIsupForm, *COM_SGTOP );
        COM_SGTYPE = COM_SIG_ISUP;
    }
    COM_ANALYZER_END;
}

void com_decodeIsup( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispDec( " ISUP MESSAGE[%d]", COM_SIG_ISUP );
    com_dispPrm( "type", com_getIsupMsgName( COM_ISGTOP ), 0 );
    com_dispPrmList( COM_IAPRM, 2, 2, NULL );
    COM_DECODER_END;
}

static com_decodeName_t  gIsupMsg[] = {
    { COM_CAP_ISUP_IAM,  "IAM" },
    { COM_CAP_ISUP_ACM,  "ACM" },
    { COM_CAP_ISUP_ANM,  "ANM" },
    { COM_CAP_ISUP_CPG,  "CPG" },
    { COM_CAP_ISUP_CON,  "CON" },
    { COM_CAP_ISUP_REL,  "REL" },
    { COM_CAP_ISUP_RLC,  "RLC" },
    { COM_CAP_ISUP_RES,  "RES" },
    { COM_CAP_ISUP_SUS,  "SUS" },
    { COM_CAP_ISUP_CHG,  "CHG" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

char *com_getIsupMsgName( void *iSigTop )
{
    com_bin*  isup = iSigTop;
    return com_searchDecodeName( gIsupMsg, *isup, true );
}



// TCAP //////////////////////////////////////////////////////////////////////

// TCAP.SSNと次プロトコルとの対応
static com_sigPrtclType_t  gTcapSsn2[] = {
    { {COM_CAP_SCCPSSN_INAP}, COM_SIG_INAP },
    { {COM_CAP_SCCPSSN_HLR},  COM_SIG_GSMMAP },
    { {COM_CAP_SCCPSSN_MSC},  COM_SIG_GSMMAP },
    { {COM_PRTCLTYPE_END}, COM_SIG_UNKNOWN }
};

static void getComponentTypeFromSccp( com_sigInf_t *iSccp, com_sigInf_t *oComp )
{
    if( !iSccp ) {return;}
    ulong*  ssn = iSccp->ext;
    if( ssn ) {
        long  prtcl = com_getPrtclType( COM_TCAPSSN, *ssn );
        if( prtcl == COM_SIG_UNKNOWN ) {return;}
        oComp->sig.ptype = prtcl;
    }
}

static BOOL getComponents( com_bin *iSignal, com_sigInf_t *ioHead )
{
    com_off  compTotalLen = 0;
    iSignal++;
    iSignal += com_getTagLength( iSignal, &compTotalLen );
    com_off  compSize = 0;
    while( compSize < compTotalLen ) {
        if( !com_getTcapCompName( *iSignal ) ) {return false;}
        com_off  compLen = 0;
        com_off  compLenSize = com_getTagLength( iSignal + 1, &compLen );
        com_off  tmpSize = compLen + compLenSize + 1;
        com_sigInf_t comp = {.sig = {iSignal, tmpSize, COM_SIG_CONTINUE}};
        getComponentTypeFromSccp( ioHead->prev, &comp );
        if( !com_stackSigNext( ioHead, &comp ) ) {return false;}
        compSize += tmpSize;
        iSignal += tmpSize;
    }
    return true;
}

BOOL com_analyzeTcap( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( COM_NO_MIN_LEN );
    com_bin*  signal = COM_SGTOP;
    if( com_getTcapTranName( *signal ) ) {
        while( *signal != COM_CAP_TCAP_COMPONENT &&
                signal < (COM_SGTOP + COM_SGLEN) )
        { // トランザクション部とダイアログ部を TLV分解して取得
            if( !(result = com_getAsnTlv( &signal, COM_APRM )) ) {break;}
        }
        if( result && *signal == COM_CAP_TCAP_COMPONENT ) {
            result = getComponents( signal, ioHead );
        }
    }
    COM_ANALYZER_END;
}

// Dialogue-As-Idのタグパターン(先頭要素は処理でTCメッセージ種別タグを格納)
static com_off  gTagsDlgAsId[] = {
    0x00, COM_CAP_TCAP_DIALOGUE, COM_CAP_TCAP_DLG_EXT, COM_CAP_TCAP_DLG_AS_ID
};

// Single ASN Typeのタグパターン(先頭要素は処理でTCメッセージ種別タグを格納)
static com_off  gTagsSAsn[] = {
    0x00, COM_CAP_TCAP_DIALOGUE, COM_CAP_TCAP_DLG_EXT, COM_CAP_TCAP_DLG_ASN1
};

static void decodeTcapDlg( com_sigPrm_t *iPrm, long iId )
{
    com_dispDec( "  -Dialogue Portion-" );
    com_dispAsnPrm( iPrm, "#  ", iId );
    com_sigBin_t  tmpVal = {.top = NULL};
    com_bin  pduType = 0;
    com_bin  pduTag = 0;
    gTagsDlgAsId[0] = iPrm->list[0].tag;
    if( com_searchAsnTlv( iPrm,gTagsDlgAsId,sizeof(gTagsDlgAsId),&tmpVal ) ) {
        pduTag = *tmpVal.top;
        gTagsSAsn[0] = iPrm->list[0].tag;
        if( com_searchAsnTlv( iPrm,gTagsSAsn,sizeof(gTagsSAsn),&tmpVal ) ) {
            pduTag = *tmpVal.top;
            com_dispPrm( "pdu tag",com_getTcapDlgPduName( pduTag,pduType ),0 );
        }
    }
}

void com_decodeTcap( COM_DECODER_PRM )
{
    COM_DECODER_START;
    com_dispDec( " TCAP MESSAGE [%d]", COM_SIG_TCAP );
    com_dispDec( "  -Transaction Portion-" );
    com_dispPrm( "message type", com_getTcapTranName( COM_IPRMLST[0].tag ), 9 );
    for( long prmId = 1;  prmId < COM_IPRMCNT;  prmId++ ) {
        com_sigTlv_t*  tlv = &(COM_IPRMLST[prmId]);
        if( tlv->tag == COM_CAP_TCAP_DIALOGUE ) {
            decodeTcapDlg( COM_IAPRM, prmId );
            break;
        }
    }
    COM_DECODER_END;
}

static com_decodeName_t  gTcapTranName[] = {
    { COM_CAP_TCAP_UNDIRECTIONAL,   "Undirectional" },
    { COM_CAP_TCAP_BEGIN,           "Begin" },
    { COM_CAP_TCAP_END,             "End" },
    { COM_CAP_TCAP_CONTINUE,        "Continue" },
    { COM_CAP_TCAP_ABORT,           "Abort" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

char *com_getTcapTranName( ulong iTag )
{
    return com_searchDecodeName( gTcapTranName, iTag, true );
}

static com_decodeName_t  gTcapTranPrm[] = {
    { COM_CAP_TCAP_OTID,            "Org Transaction ID" },
    { COM_CAP_TCAP_DTID,            "Dst Transaction ID" },
    { COM_CAP_TCAP_PABORT,          "P-Abort Cause" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

char *com_getTcapTranPrm( ulong iTag )
{
    return com_searchDecodeName( gTcapTranPrm, iTag, true );
}

static com_decodeName_t  gTcapDlgPduName[] = {
    { COM_CAP_TCAP_DLG_REQUEST,     "Dialogue Request" },
    { COM_CAP_TCAP_DLG_RESPONSE,    "Dialogue Response" },
    { COM_CAP_TCAP_DLG_ABORT,       "Dialogue Abort" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

static com_decodeName_t  gTcapUndlgPduName[] = {
    { COM_CAP_TCAP_DLG_UNDIRECT,    "Undirectional Dialogue" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

char *com_getTcapDlgPduName( ulong iTag, long iType )
{
    if( iType == COM_CAP_TCAP_DLGPDU ) {
        return com_searchDecodeName( gTcapDlgPduName, iTag, true );
    }
    if( iType == COM_CAP_TCAP_UNDLGPDU ) {
        return com_searchDecodeName( gTcapUndlgPduName, iTag, true );
    }
    return NULL;
}

static com_decodeName_t  gTcapCompName[] = {
    { COM_CAP_TCAP_INVOKE,          "Invoke" },
    { COM_CAP_TCAP_RETURNRESULT,    "Return Result(last)" },
    { COM_CAP_TCAP_RETURNERROR,     "Return Error" },
    { COM_CAP_TCAP_REJECT,          "Reject" },
    { COM_CAP_TCAP_RETURNRESULT2,   "Return Result(not last)" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

char *com_getTcapCompName( ulong iTag )
{
    return com_searchDecodeName( gTcapCompName, iTag, true );
}

// TCAPコンポーネント部データ構造
typedef struct {
    long  type;
    long  isMondatry;
    long  cnt;
    long  tag[5];
} sigTcapTags_t;

typedef struct {
    long  type;
    long  cnt;
    sigTcapTags_t  tags[5];
} sigTcapComp_t;

static sigTcapComp_t  gTcapComp[] = {
    { COM_CAP_TCAP_INVOKE, 4, {
        { COM_SIG_TCAP_INVOKEID, true,
          2, {COM_CAP_TCAP_INVOKEID, COM_CAP_TCAP_NULL} },
        { COM_SIG_TCAP_LINKEDID, false,
          1, {COM_CAP_TCAP_LINKEDID} },
        { COM_SIG_TCAP_OPCODE, true,
          2, {COM_CAP_TCAP_OPCODE_LOCAL, COM_CAP_TCAP_OPCODE_GLOBAL} },
        { COM_SIG_TCAP_PARAM, false,
          2, {COM_CAP_TCAP_PARAM_SEQ, COM_CAP_TCAP_PARAM_SET} },
      }
    },
    { COM_CAP_TCAP_RETURNRESULT, 4, {
        { COM_SIG_TCAP_INVOKEID, true,
          2, {COM_CAP_TCAP_INVOKEID, COM_CAP_TCAP_NULL} },
        { COM_SIG_TCAP_TMP, false,
          1, {COM_CAP_TCAP_PARAM_SEQ} },
        { COM_SIG_TCAP_OPCODE, false,
          2, {COM_CAP_TCAP_OPCODE_LOCAL, COM_CAP_TCAP_OPCODE_GLOBAL} },
        { COM_SIG_TCAP_PARAM, false,
          2, {COM_CAP_TCAP_PARAM_SEQ, COM_CAP_TCAP_PARAM_SET} },
      }
    },
    { COM_CAP_TCAP_RETURNERROR, 3, {
        { COM_SIG_TCAP_INVOKEID, true,
          2, {COM_CAP_TCAP_INVOKEID, COM_CAP_TCAP_NULL} },
        { COM_SIG_TCAP_ERROR, true,
          2, {COM_CAP_TCAP_ERRCODE_LOCAL, COM_CAP_TCAP_ERRCODE_GLOBAL} },
        { COM_SIG_TCAP_PARAM, false,
          2, {COM_CAP_TCAP_PARAM_SEQ, COM_CAP_TCAP_PARAM_SET} },
      }
    },
    { COM_CAP_TCAP_REJECT, 2, {
        { COM_SIG_TCAP_INVOKEID, true,
          2, {COM_CAP_TCAP_INVOKEID, COM_CAP_TCAP_NULL} },
        { COM_SIG_TCAP_PROBLEM, true,
          4, {COM_CAP_TCAP_PROBLEM_GEN, COM_CAP_TCAP_PROBLEM_INV,
              COM_CAP_TCAP_PROBLEM_RR,  COM_CAP_TCAP_PROBLEM_RE} },
      }
    },
    { COM_CAP_TCAP_RETURNRESULT2, 4, {
        { COM_SIG_TCAP_INVOKEID, true,
          2, {COM_CAP_TCAP_INVOKEID, COM_CAP_TCAP_NULL} },
        { COM_SIG_TCAP_TMP, false,
          1, {COM_CAP_TCAP_PARAM_SEQ} },
        { COM_SIG_TCAP_OPCODE, false,
          2, {COM_CAP_TCAP_OPCODE_LOCAL, COM_CAP_TCAP_OPCODE_GLOBAL} },
        { COM_SIG_TCAP_PARAM, false,
          2, {COM_CAP_TCAP_PARAM_SEQ, COM_CAP_TCAP_PARAM_SET} },
      }
    }
};

static sigTcapComp_t *getCompConf( long iType )
{
    for( ulong i = 0;  i < COM_ELMCNT(gTcapComp);  i++ ) {
        if( gTcapComp[i].type == iType ) {return &(gTcapComp[i]);}
    }
    return NULL;
}

static BOOL matchTags( com_bin *iSignal, sigTcapComp_t *iConf, long *iCnt )
{
    if( *iCnt >= iConf->cnt ) {return false;}
    sigTcapTags_t*  tags = &(iConf->tags[*iCnt]);
    for( long i = 0;  i < tags->cnt;  i++ ) {
        if( tags->tag[i] == *iSignal ) {return true;}
    }
    if( tags->isMondatry ) {return false;}
    // 必須でないなら次の情報と比較する
    (*iCnt)++;
    return matchTags( iSignal, iConf, iCnt );
}

static com_bin *checkSignal( com_sigInf_t *ioHead, sigTcapComp_t **oConf )
{
    com_bin*  signal = COM_SGTOP;
    if( !com_getTcapCompName( *signal ) ) {return NULL;}
    if( !(*oConf = getCompConf( *signal )) ) {return NULL;}
    signal += com_getTagLength( signal + 1, NULL );
    return signal;
}

static BOOL checkHeadPrm(
        long *ioCnt, com_sigPrm_t *iTarget, com_sigPrm_t *iHead,
        com_bin *iSignal, sigTcapComp_t *iConf )
{
    if( iTarget != iHead ) {return true;}
    if( *ioCnt > iConf->cnt || !matchTags( iSignal, iConf, ioCnt ) ) {
        return false;
    }
    return true;
}

static void setCompHead(
        long *ioCnt, com_sigPrm_t **ioTarget, com_sigPrm_t *iHead,
        com_sigPrm_t *iPrm, sigTcapComp_t *iConf,
        com_sigTcapCompHead_t *oCompHead )
{
    if( *ioTarget != iHead ) {return;}
    long  prmType = iConf->tags[*ioCnt].type;
    if( prmType != COM_SIG_TCAP_TMP ) {
        oCompHead->prm[prmType] = *com_getRecentPrm( iHead );
    }
    // パラメータが始まるなら、データ保持先をパラメータに付け替え
    if( prmType == COM_SIG_TCAP_PARAM ) {*ioTarget = iPrm;}
    else {(*ioCnt)++;}
}

static BOOL freeCompInf(
        BOOL iResult, com_sigPrm_t *oHead, com_sigTcapCompHead_t *oCompHead )
{
    if( oHead ) {com_freeSigPrm( oHead );}
    if( oCompHead ) {com_free( oCompHead );}
    return iResult;
}

BOOL com_getTcapCompInf( com_sigInf_t *ioHead )
{
    sigTcapComp_t*  headConf = NULL;
    com_bin*  signal = checkSignal( ioHead, &headConf );
    if( !signal ) {return false;}
    com_sigTcapCompHead_t*  compHead =
        com_malloc( sizeof(com_sigTcapCompHead_t), "component head inf" );
    if( !compHead ) {return false;}
    com_sigPrm_t  head;
    com_initSigPrm( &head );
    com_sigPrm_t*  target = &head;
    long  cnt = 0;
    while( signal < (COM_SGTOP + COM_SGLEN) ) {
        if( !checkHeadPrm( &cnt, target, &head, signal, headConf ) ) {
            return freeCompInf( false, &head, compHead );
        }
        if( !com_getAsnTlv( &signal, target ) ) {
            return freeCompInf( false, &head, compHead );
        }
        setCompHead( &cnt, &target, &head, COM_APRM, headConf, compHead );
    }
    com_free( ioHead->ext );  // 念の為、解放
    ioHead->ext = compHead;
    return freeCompInf( true, &head, NULL );
}

// コンポーネント部オペレーションコード名取得関数プロトタイプ
typedef char *(getOpName_t)( ulong iOpcode );

// コンポーネント部出力 内部共通処理
static void decodeTcapComponents( com_sigInf_t *iHead, getOpName_t iFunc )
{
    COM_DECODER_START;
    com_dispDec( " %s MESSAGE [%ld]  <length=%zu>",
             com_searchSigProtocol( COM_ISGTYPE ), COM_ISGTYPE, COM_ISGLEN );
    com_dispPrm( "tcap component type", com_getTcapCompName( *COM_ISGTOP ), 0 );
    COM_CAST_HEAD( com_sigTcapCompHead_t, compPrm, iHead->ext );
    for( ulong i = 0;  i < COM_SIG_TCAP_MAX;  i++ ) {
        com_sigTlv_t*  tlv = &(compPrm->prm[i]);
        if( i == COM_SIG_TCAP_OPCODE ) {
            com_dispPrm( com_getTcapCompPrm(i), iFunc( *tlv->value ), 0 );
        }
        else if( i != COM_SIG_TCAP_PARAM && tlv->tag ) {
            com_dispBin( com_getTcapCompPrm(i),tlv->value,tlv->len,"",true );
        }
    }
    com_dispDec( "  -parameters-" );
    com_dispAsnPrm( COM_IAPRM, "#   ", 0 );
    COM_DECODER_END;
}

static com_decodeName_t  gTcapCompPrm[] = {
    { COM_SIG_TCAP_INVOKEID,        "Invoke ID" },
    { COM_SIG_TCAP_LINKEDID,        "Linked ID" },
    { COM_SIG_TCAP_OPCODE,          "Operation Code" },
    { COM_SIG_TCAP_PARAM,           "Parameter" },
    { COM_SIG_TCAP_ERROR,           "Error Code" },
    { COM_SIG_TCAP_PROBLEM,         "Problem Code" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

char *com_getTcapCompPrm( ulong iTag )
{
    return com_searchDecodeName( gTcapCompPrm, iTag, false );
}

enum { NO_TCAPCOMP_PRM = ULONG_MAX };

// コンポーネント部オペレーションコード取得 内部共通処理
static ulong getTcapCompOpcode( com_sigInf_t *iHead )
{
    COM_CAST_HEAD( com_sigTcapCompHead_t, compPrm, iHead->ext );
    if( !compPrm ) {return NO_TCAPCOMP_PRM;}
    com_sigTlv_t*  tlv = &(compPrm->prm[COM_SIG_TCAP_OPCODE]);
    return *tlv->value;
}



// INAP //////////////////////////////////////////////////////////////////////

BOOL com_analyzeInap( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( COM_NO_MIN_LEN );
    if( (result = com_getTcapCompInf( ioHead )) ) {
        COM_SGTYPE = COM_SIG_INAP;
        result = (com_getInapOpName( (ulong)getTcapCompOpcode( ioHead ) )
                  != NULL);
    }
    COM_ANALYZER_END;
}

void com_decodeInap( COM_DECODER_PRM )
{
    COM_DECODER_START;
    decodeTcapComponents( iHead, com_getInapOpName );
    COM_DECODER_END;
}

static com_decodeName_t  gInapOpName[] = {
    { COM_CAP_INAP_OP_IDP,     "IDP" },
    { COM_CAP_INAP_OP_CON,     "CON" },
    { COM_CAP_INAP_OP_ENC,     "ENC" },
    { COM_CAP_INAP_OP_ERBCSM,  "ERBCSM" },
    { COM_CAP_INAP_OP_RNCE,    "RNCE" },
    { COM_CAP_INAP_OP_RRBE,    "RRBE" },
    { COM_CAP_INAP_OP_SCI,     "SCI" },
    { COM_CAP_INAP_OP_RLC,     "RLC" },
    { COM_CAP_INAP_OP_CUE,     "CUE" },
    { COM_CAP_INAP_OP_DFC,     "DFC" },
    { COM_CAP_INAP_OP_DFCWA,   "DFCWA" },
    { COM_CAP_INAP_OP_ETC,     "ETC" },
    { COM_CAP_INAP_OP_ITC,     "ITC" },
    { COM_CAP_INAP_OP_MCS,     "MCS" },
    { COM_CAP_INAP_OP_ML,      "ML" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

char *com_getInapOpName( ulong iOpcode )
{
    return com_searchDecodeName( gInapOpName, iOpcode, true );
}



// GSMMAP ////////////////////////////////////////////////////////////////////

BOOL com_analyzeGsmmap( COM_ANALYZER_PRM )
{
    COM_ANALYZER_START( COM_NO_MIN_LEN );
    if( (result = com_getTcapCompInf( ioHead )) ) {
        COM_SGTYPE = COM_SIG_GSMMAP;
        result = (com_getGsmmapOpName( getTcapCompOpcode( ioHead ) ) != NULL);
    }
    COM_ANALYZER_END;
}

void com_decodeGsmmap( COM_DECODER_PRM )
{
    COM_DECODER_START;
    decodeTcapComponents( iHead, com_getGsmmapOpName );
    COM_DECODER_END;
}

static com_decodeName_t  gGsmmapOpName[] = {
    { COM_CAP_GSMMAP_OP_UPDLOC,       "updateLocation" },
    { COM_CAP_GSMMAP_OP_CANLOC,       "cancelLocation" },
    { COM_CAP_GSMMAP_OP_PRGMS,        "purgeMS" },
    { COM_CAP_GSMMAP_OP_SNDID,        "sendIdentification" },
    { COM_CAP_GSMMAP_OP_UPDGLOC,      "updateGprsLocation" },
    { COM_CAP_GSMMAP_OP_PRVSUBINF,    "provideSubscriberInfo" },
    { COM_CAP_GSMMAP_OP_ANTITR,       "anyTimeInterrogation" },
    { COM_CAP_GSMMAP_OP_ANTSUBITR,    "anyTimeSubscriptionInterrogation" },
    { COM_CAP_GSMMAP_OP_ANTMOD,       "anyTimeModification" },
    { COM_CAP_GSMMAP_OP_NOTSUBMOD,    "noteSubscriberModified" },
    { COM_CAP_GSMMAP_OP_PREHO,        "prepareHandover" },
    { COM_CAP_GSMMAP_OP_SNDENDSIG,    "sendEndSignal" },
    { COM_CAP_GSMMAP_OP_PRCACCSIG,    "processAccessSignaling" },
    { COM_CAP_GSMMAP_OP_FWDACCSIG,    "forwardAccessSignaling" },
    { COM_CAP_GSMMAP_OP_PRESUBHO,     "prepareSubsequentHandover" },
    { COM_CAP_GSMMAP_OP_SNDATHINF,    "sendAuthenticationInfo" },
    { COM_CAP_GSMMAP_OP_ATHFALREP,    "authenticationFailureReport" },
    { COM_CAP_GSMMAP_OP_CHKIMEI,      "checkIMEI" },
    { COM_CAP_GSMMAP_OP_INSSUB,       "insertSubscriberData" },
    { COM_CAP_GSMMAP_OP_DELSUB,       "deleteSubscriberData" },
    { COM_CAP_GSMMAP_OP_RESET,        "reset" },
    { COM_CAP_GSMMAP_OP_FWDCHKSSI,    "forwardCheckSS-Indication" },
    { COM_CAP_GSMMAP_OP_RESDAT,       "restoreData" },
    { COM_CAP_GSMMAP_OP_SNDRINFGP,    "sendRoutingInfoForGprs" },
    { COM_CAP_GSMMAP_OP_FALREP,       "failureReport" },
    { COM_CAP_GSMMAP_OP_NOTMSPRGP,    "noteMsPresentForGprs" },
    { COM_CAP_GSMMAP_OP_NOTMMEV,      "noteMM-Event" },
    { COM_CAP_GSMMAP_OP_ACTTRC,       "activateTraceMode" },
    { COM_CAP_GSMMAP_OP_DEATRC,       "deactivateTraceMode" },
    { COM_CAP_GSMMAP_OP_SNDIMSI,      "sendIMSI" },
    { COM_CAP_GSMMAP_OP_SNDRINF,      "sendRoutingInfo" },
    { COM_CAP_GSMMAP_OP_PRVROAMNO,    "provideRoamingNumber" },
    { COM_CAP_GSMMAP_OP_RESCALHND,    "resumeCallHandling" },
    { COM_CAP_GSMMAP_OP_SETREPST,     "setReportingState" },
    { COM_CAP_GSMMAP_OP_STREP,        "StatusReport" },
    { COM_CAP_GSMMAP_OP_REMUSRFRE,    "remoteUserFree" },
    { COM_CAP_GSMMAP_OP_ISTALT,       "ist-Alert" },
    { COM_CAP_GSMMAP_OP_ISTCMD,       "ist-Command" },
    { COM_CAP_GSMMAP_OP_RELRES,       "releaseResources" },
    { COM_CAP_GSMMAP_OP_REGSS,        "registerSS" },
    { COM_CAP_GSMMAP_OP_ERASS,        "eraseSS" },
    { COM_CAP_GSMMAP_OP_ACTSS,        "activateSS" },
    { COM_CAP_GSMMAP_OP_DEASS,        "deactivateSS" },
    { COM_CAP_GSMMAP_OP_ITRSS,        "interrogateSS" },
    { COM_CAP_GSMMAP_OP_PRCUSSREQ,    "processUnstructuredSS-Request" },
    { COM_CAP_GSMMAP_OP_USSREQ,       "unstructuredSS-Request" },
    { COM_CAP_GSMMAP_OP_USSNOT,       "unstructuredSS-Notify" },
    { COM_CAP_GSMMAP_OP_REGPW,        "registerPassword" },
    { COM_CAP_GSMMAP_OP_GETPW,        "getPassword" },
    { COM_CAP_GSMMAP_OP_SSINVNOT,     "ss-InvocationNotification" },
    { COM_CAP_GSMMAP_OP_REGCCENT,     "registerCC-Entry" },
    { COM_CAP_GSMMAP_OP_ERACCENT,     "eraseCC-Entry" },
    { COM_CAP_GSMMAP_OP_SNDRINFSM,    "sendRoutingInfoForSM" },
    { COM_CAP_GSMMAP_OP_MOFWDSM,      "mo-ForwardSM" },
    { COM_CAP_GSMMAP_OP_MTFWDSM,      "mt-ForwardSM" },
    { COM_CAP_GSMMAP_OP_REPSMDELS,    "reportSM-DeliveryStatus" },
    { COM_CAP_GSMMAP_OP_ALTSRVCNT,    "alertServiceCentre" },
    { COM_CAP_GSMMAP_OP_INFSRCCNT,    "informServiceCentre" },
    { COM_CAP_GSMMAP_OP_RDYSM,        "readyForSM" },
    { COM_CAP_GSMMAP_OP_MTFWDSMVG,    "mt-ForwardSM-VGCS" },
    COM_DECODENAME_END    // 最後は必ずこれで
};

char *com_getGsmmapOpName( ulong iOpcode )
{
    return com_searchDecodeName( gGsmmapOpName, iOpcode, true );
}



// 初期化処理 ////////////////////////////////////////////////////////////////

static void finalizeSet2( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    freeSccpConnInf();
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

static com_analyzeFuncList_t  gFuncSignal2[] = {
    { COM_SIG_M3UA,        0,          "M3UA",
      com_analyzeM3ua,   com_decodeM3ua,   NULL },
    { COM_SIG_SCCP,        0,          "SCCP",
      com_analyzeSccp,   com_decodeSccp,   NULL },
    { COM_SIG_TCAP,        0,          "TCAP",
      com_analyzeTcap,   com_decodeTcap,   NULL },
    { COM_SIG_INAP,        0,          "INAP",
      com_analyzeInap,   com_decodeInap,   NULL },
    { COM_SIG_GSMMAP,      0,          "GSMMAP",
      com_analyzeGsmmap, com_decodeGsmmap, NULL },
//    { COM_SIG_ISUP,        0,          "ISUP",
//      com_analyzeIsup,   com_decodeIsup,   NULL },
    { COM_SIG_END, 0, NULL, NULL, NULL, NULL }  // 最後は必ずこれで
};

void com_initializeSigSet2( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    com_setInitStage( COM_INIT_STAGE_PROCCESSING, false );
    atexit( finalizeSet2 );
    (void)com_registerAnalyzer( gFuncSignal2, COM_SCCPSSN );
    com_setPrtclType( COM_SCTPNEXT, gSctpNext2 );
    com_setPrtclType( COM_SIPNEXT, gSipNext2 );
    com_setPrtclType( COM_SCCPSSN, gSccpSsn2 );
    com_setPrtclType( COM_TCAPSSN, gTcapSsn2 );
    com_setInitStage( COM_INIT_STAGE_FINISHED, false );
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

