/*
 *****************************************************************************
 *
 * 共通部シグナル機能 (プロトコルセット2)    by TOS
 *
 *   シグナル機能の、プロトコル別I/Fのセット2(No.7関連)となる。
 *   本ヘッダの前に com_if.h と com_signal.h のインクルードが必要になる。
 *   (つまりこれらのヘッダによって使用される全ファイルも必要となる)
 *
 *   対応しているプロトコルについては PROTOCOL_AVAILABLE を参照。
 *
 *****************************************************************************
 */

#pragma once

// PROTOCOL_AVAILABLE
enum {
    /*** ファイルタイプ (1-99) ***/
    /*** link層 (101-199) ***/
    /*** Network層 (201-299) ***/
    /*** Transport層 (301-399) ***/
    /*** No.7系 (401-499) ***/
    COM_SIG_M3UA     = 410,    // RFC3332 -> 4666
    COM_SIG_SCCP     = 411,    // Q713
    COM_SIG_ISUP     = 412,    // Q763
    COM_SIG_TCAP     = 413,    // Q773
    COM_SIG_INAP     = 414,    // Q1200-1699のどこか (未確認)
    COM_SIG_GSMMAP   = 414,
    /*** Application層 (501-599) ***/
    /*** プロトコル認識のみ (601-699) ***/
};

// まだ処理は未実装のためダミー処理
void com_initializeSigSet2( void );


// No.7信号パラメータ情報構造
typedef struct {
    long    tag;  // パラメータタグ
    long    len;  // パラメータ長(固定超必須時)/レングス長(可変長必須時)
} com_sigNo7Prm_t;

// No.7信号パラメータ終端
enum { COM_CAP_NO7_END = 0x00 };

// No.7メッセージ解析情報
typedef struct {
    long               msgType;     // メッセージ種別
    com_sigNo7Prm_t*   fix;         // 固定長必須パラメータ
    com_sigNo7Prm_t*   var;         // 可変長必須パラメータ
    BOOL               hasOpt;      // オプションパラメータ有無
} com_sigNo7Form_t;

BOOL com_getNo7Prm( com_sigInf_t *ioHead, com_sigNo7Form_t *iForm, long type );


/*
 *****************************************************************************
 * No.7系 (M3UA)
 *****************************************************************************
 */

// 次プロトコル定義 (com_setPrtclType(): COM_SCTPNEXT用)
enum {
    COM_CAP_SCTP_PROTO_M3UA = 0x03
};

// M3UA共通ヘッダ固定値
enum {
    COM_CAP_M3UA_VERSION  = 0x01,
    COM_CAP_M3UA_RESERVED = 0x00
};

// M3UA Message Class
typedef enum {
    COM_CAP_M3UA_CLASS_MGMT  = 0,
    COM_CAP_M3UA_CLASS_TRANS = 1,
    COM_CAP_M3UA_CLASS_SSNM  = 2,
    COM_CAP_M3UA_CLASS_ASPSM = 3,
    COM_CAP_M3UA_CLASS_ASPTM = 4,
    COM_CAP_M3UA_CLASS_RKM   = 9
} COM_CAP_M3UA_CLASS_t;

// M3UA Message Type
typedef enum {
    // MGMT (0)
    COM_CAP_M3UA_TYPE_ERR      = 0,
    COM_CAP_M3UA_TYPE_NTFY     = 1,
    // TRANS (1)
    COM_CAP_M3UA_TYPE_DATA     = 1,
    // SSNM (2)
    COM_CAP_M3UA_TYPE_DUNA     = 1,
    COM_CAP_M3UA_TYPE_DAVA     = 2,
    COM_CAP_M3UA_TYPE_DAUD     = 3,
    COM_CAP_M3UA_TYPE_SCON     = 4,
    COM_CAP_M3UA_TYPE_DUPU     = 5,
    COM_CAP_M3UA_TYPE_DRST     = 6,
    // ASPSN (3)
    COM_CAP_M3UA_TYPE_ASPUP    = 1,
    COM_CAP_M3UA_TYPE_ASPDN    = 2,
    COM_CAP_M3UA_TYPE_BEAT     = 3,
    COM_CAP_M3UA_TYPE_ASPUPACK = 4,
    COM_CAP_M3UA_TYPE_ASPDNACK = 5,
    COM_CAP_M3UA_TYPE_BEATACK  = 6,
    // ASPTM (4)
    COM_CAP_M3UA_TYPE_ASPAC    = 1,
    COM_CAP_M3UA_TYPE_ASPIA    = 2,
    COM_CAP_M3UA_TYPE_ASPACACK = 3,
    COM_CAP_M3UA_TYPE_ASPIAACK = 4,
    // RKM (9)
    COM_CAP_M3UA_TYPE_REGREQ   = 1,
    COM_CAP_M3UA_TYPE_REGRSP   = 2,
    COM_CAP_M3UA_TYPE_DEREGREQ = 3,
    COM_CAP_M3UA_TYPE_DEREGRSP = 4,
} COM_CAP_M3UA_TYPE_t;

// M3UAヘッダ構造
typedef struct {
    uint8_t    version;
    uint8_t    reserved;
    uint8_t    msgClass;
    uint8_t    msgType;
    uint32_t   msgLen;
} com_sigM3uaHdr_t;

// M3UAパラメータ
enum {
    COM_CAP_M3UA_PRM_ROUTECONT  = 0x0006,   // Routing Context
    COM_CAP_M3UA_PRM_CORRID     = 0x0013,   // Corralation ID
    COM_CAP_M3UA_PRM_NETAPP     = 0x0200,   // Network Appearance
    COM_CAP_M3UA_PRM_PROTDATA   = 0x0210    // Protocol Data
};

// M3UA パラメータ構造
typedef struct {
    uint16_t   tag;
    uint16_t   len;
    uint8_t    value[];
} com_sigM3uaPrm_t;

// M3UA Protocol Data構造
typedef struct {
    uint32_t   opc;
    uint32_t   dpc;
    uint8_t    si;
    uint8_t    ni;
    uint8_t    mp;
    uint8_t    sls;
    uint8_t    usrPrtData[];
} com_sigM3uaProtData_t;

BOOL com_analyzeM3ua( COM_ANALYZER_PRM );
void com_decodeM3ua( COM_DECODER_PRM );

char *com_getM3uaClassType( void *iSigTop );


/*
 *****************************************************************************
 * No.7系 (SCCP)
 *****************************************************************************
 */

// SCCPメッセージ種別
typedef enum {
    COM_CAP_SCCP_CR      = 0x01,
    COM_CAP_SCCP_CC      = 0x02,
    COM_CAP_SCCP_CREF    = 0x03,
    COM_CAP_SCCP_RLSD    = 0x04,
    COM_CAP_SCCP_RLC     = 0x05,
    COM_CAP_SCCP_DT1     = 0x06,     // 更にデータ解析する
    COM_CAP_SCCP_DT2     = 0x07,     // 更にデータ解析する
    COM_CAP_SCCP_AK      = 0x08,
    COM_CAP_SCCP_UDT     = 0x09,     // 更にデータ解析する
    COM_CAP_SCCP_UDTS    = 0x0a,
    COM_CAP_SCCP_ED      = 0x0b,
    COM_CAP_SCCP_EA      = 0x0c,
    COM_CAP_SCCP_RSR     = 0x0d,
    COM_CAP_SCCP_RSC     = 0x0e,
    COM_CAP_SCCP_ERR     = 0x0f,
    COM_CAP_SCCP_IT      = 0x10,
    COM_CAP_SCCP_XUDT    = 0x11,     // 更にデータ解析する
    COM_CAP_SCCP_XUDTS   = 0x12,
    COM_CAP_SCCP_LUDT    = 0x13,     // 更にデータ解析する
    COM_CAP_SCCP_LUDTS   = 0x14
} COM_CAP_SCCP_TYPE_t;

// SCCPパラメータ
typedef enum {
    COM_CAP_SCCPPRM_END       = 0x00,  // end of optional parameters
    COM_CAP_SCCPPRM_DLR       = 0x01,  // destination local reference
    COM_CAP_SCCPPRM_SLR       = 0x02,  // source local reference
    COM_CAP_SCCPPRM_CLDPAD    = 0x03,  // called party address
    COM_CAP_SCCPPRM_CLGPAD    = 0x04,  // calling party address
    COM_CAP_SCCPPRM_PRTCLS    = 0x05,  // protocol class
    COM_CAP_SCCPPRM_SEGRAS    = 0x06,  // segmenting/reassembling
    COM_CAP_SCCPPRM_RCVSEQNO  = 0x07,  // receive sequence number
    COM_CAP_SCCPPRM_SEQSEG    = 0x08,  // sequencing/segmenting
    COM_CAP_SCCPPRM_CREDIT    = 0x09,  // credit
    COM_CAP_SCCPPRM_REFCAUSE  = 0x0a,  // refuse cause
    COM_CAP_SCCPPRM_RETCAUSE  = 0x0b,  // return cause
    COM_CAP_SCCPPRM_RSTCAUSE  = 0x0c,  // reset cause
    COM_CAP_SCCPPRM_ERRCAUSE  = 0x0d,  // error cause
    COM_CAP_SCCPPRM_DATA      = 0x0f,  // data
    COM_CAP_SCCPPRM_SEGMENT   = 0x10,  // segmentation
    COM_CAP_SCCPPRM_HOPCNT    = 0x11,  // hop counter
    COM_CAP_SCCPPRM_IMPORT    = 0x12,  // importance
    COM_CAP_SCCPPRM_LONGDATA  = 0x14   // long data
} COM_CAP_SCCP_PRM_t;

// SCCPセグメンテーション情報構造
typedef struct {
    uint8_t   segFlags;
    uint8_t   segLr[3];
} com_sigSccpSegment_t;

#define  COM_CAP_SCCP_1ST_SEG   0x80
#define  COM_CAP_SCCP_CLASS     0x40
#define  COM_CAP_SCCP_REM_SEG   0x0f


BOOL com_analyzeSccp( COM_ANALYZER_PRM );
void com_decodeSccp( COM_DECODER_PRM );

char *com_getSccpMsgName( void *iSigTop );



/*
 *****************************************************************************
 * No.7系 (ISUP)
 *****************************************************************************
 */

// 次プロトコル定義  (com_setPrtclType(): COM_SCCOSSN用)
enum {
    COM_CAP_SCCPSSN_ISUP = 0x03    // ISDN User Part
};

// 次プロトコル定義  (com_setPrtclType(): COM_SIPNEXT用)
#define  COM_CAP_CTYPE_ISUP  "application/ISUP"

// ISUPメッセージ種別
typedef enum {
    COM_CAP_ISUP_IAM = 0x01,    // Initial address
    COM_CAP_ISUP_ACM = 0x06,    // Address complete
    COM_CAP_ISUP_ANM = 0x09,    // Answer
    COM_CAP_ISUP_CPG = 0x2c,    // Call progress
    COM_CAP_ISUP_CON = 0x07,    // Connect
    COM_CAP_ISUP_REL = 0x0c,    // Release
    COM_CAP_ISUP_RLC = 0x10,    // Release complete
    COM_CAP_ISUP_RES = 0x0e,    // Resume
    COM_CAP_ISUP_SUS = 0x0d,    // Suspend
    COM_CAP_ISUP_CHG = 0xfe     // 課金 (TTC独自)
} COM_CAP_ISUP_MSG_t;

// ISUPパラメータ (必須で使われることがあるもの)
typedef enum {
    COM_CAP_ISUPPRM_END       = 0x00,
    COM_CAP_ISUPPRM_TRMEDREQ  = 0x02,   // 3.54  通信路要求表示
    COM_CAP_ISUPPRM_CLDPN     = 0x04,   // 3.9   着番号
    COM_CAP_ISUPPRM_CONIND    = 0x06,   // 3.36  接続特性表示
    COM_CAP_ISUPPRM_FWDCIND   = 0x07,   // 3.23  順方向呼表示
    COM_CAP_ISUPPRM_CNGPCAT   = 0x09,   // 3.11  発ユーザ種別
    COM_CAP_ISUPPRM_BWDCIND   = 0x11,   // 3.5   逆方向呼表示
    COM_CAP_ISUPPRM_CAUSE     = 0x12,   // 3.12  理由表示
    COM_CAP_ISUPPRM_SUSRES    = 0x22,   // 3.52  中断/再開表示
    COM_CAP_ISUPPRM_EVINF     = 0x24,   // 3.21  イベント情報
    COM_CAP_ISUPPRM_CHGTYPE   = 0xfa,   // 3.105 課金情報種別
    COM_CAP_ISUPPRM_CHGINF    = 0xfb    // 3.104 課金情報
} COM_CAP_ISUP_PRM_t;

BOOL com_analyzeIsup( COM_ANALYZER_PRM );
void com_decodeIsup( COM_DECODER_PRM );

char *com_getIsupMsgName( void *iSigTop );



/*
 *****************************************************************************
 * No.7系 (TCAP)
 *****************************************************************************
 */

// ASN.1タグクラスのTCAPにおける扱いのメモ
//  COM_ASN1_UNIVERSAL (0x00)   X.208既定値
//  COM_ASN1_APPLI     (0x40)   No.7アプリ全体の既定値
//  COM_ASN1_CONTEXT   (0x80)   上位構造に乗った情報
//  COM_ASN1_PRIVATE   (0xc0)   国や網個別の情報

// 以後 タグ値は フォーム | クラス | タグコード という形で記述する。
// bit列的にはクラスの方が先だが、ITU-T等のタグ値定義の順序に沿ったものである。

// TCメッセージ種別
typedef enum {
    COM_CAP_TCAP_UNDIRECTIONAL =  // (0x61) Undirectional Tag
        COM_ASN1_CONFORM | COM_ASN1_APPLI     | 0x01,
    COM_CAP_TCAP_BEGIN =          // (0x62) Begin Tag
        COM_ASN1_CONFORM | COM_ASN1_APPLI     | 0x02,
    COM_CAP_TCAP_END =            // (0x64) End Tag
        COM_ASN1_CONFORM | COM_ASN1_APPLI     | 0x04,
    COM_CAP_TCAP_CONTINUE =       // (0x65) Continue Tag
        COM_ASN1_CONFORM | COM_ASN1_APPLI     | 0x05,
    COM_CAP_TCAP_ABORT =          // (0x67) Abort Tag
        COM_ASN1_CONFORM | COM_ASN1_APPLI     | 0x07
} COM_CAP_TCAP_MSG_t;

// トランザクション部の構成要素
typedef enum {
    COM_CAP_TCAP_OTID =           // (0x48) Origination TID Tag
        COM_ASN1_PRMFORM | COM_ASN1_APPLI     | 0x08,
    COM_CAP_TCAP_DTID =           // (0x49) Destination TID Tag
        COM_ASN1_PRMFORM | COM_ASN1_APPLI     | 0x09,
    COM_CAP_TCAP_PABORT =         // (0x4a) P-Abort Cause Tag
        COM_ASN1_PRMFORM | COM_ASN1_APPLI     | 0x0a
} COM_CAP_TCAP_TAG_t;

// ダイアログ部の情報要素
typedef enum {
    COM_CAP_TCAP_DIALOGUE =       // (0x6b) Dialogue portion Tag
        COM_ASN1_CONFORM | COM_ASN1_APPLI     | 0x0b,

    COM_CAP_TCAP_DLG_EXT =        // (0x28) External Tag
        COM_ASN1_CONFORM | COM_ASN1_UNIVERSAL | COM_ASN1_EXT,
    COM_CAP_TCAP_DLG_AS_ID =      // (0x06) dialogue-As-ID Tag
        COM_ASN1_PRMFORM | COM_ASN1_UNIVERSAL | COM_ASN1_OBJID,

    COM_CAP_TCAP_DLGPDU   = 0x01,  // dialoguePDU Value
    COM_CAP_TCAP_UNDLGPDU = 0x02,  // undialoguePDU Value

    COM_CAP_TCAP_DLG_ASN1 =       // (0xa0) Single ASN.1-Type Tag
        COM_ASN1_CONFORM | COM_ASN1_CONTEXT   | 0x00,
    COM_CAP_TCAP_DLG_REQUEST =    // (0x60) Dialogue Request Tag
        COM_ASN1_CONFORM | COM_ASN1_APPLI     | 0x00,
    COM_CAP_TCAP_DLG_RESPONSE =   // (0x61) Dialogue Response Tag
        COM_ASN1_CONFORM | COM_ASN1_APPLI     | 0x01,
    COM_CAP_TCAP_DLG_ABORT =      // (0x64) Dialogue Abort Tag
        COM_ASN1_CONFORM | COM_ASN1_APPLI     | 0x04,
    COM_CAP_TCAP_DLG_UNDIRECT =   // (0x60) Undirectional Dialogue Tag
        COM_ASN1_CONFORM | COM_ASN1_APPLI     | 0x00,

    COM_CAP_TCAP_PROTOVERSION =   // (0x80) Protocol Version Tag
        COM_ASN1_PRMFORM | COM_ASN1_CONTEXT   | 0x00,
    COM_CAP_TCAP_ACNAME =         // (0xa1) Application Context Name Tag
        COM_ASN1_CONFORM | COM_ASN1_CONTEXT   | 0x01,
    COM_CAP_TCAP_ACNAMEVAL =      // (0x06) Application Context Name Value Tag
        COM_ASN1_PRMFORM | COM_ASN1_UNIVERSAL | COM_ASN1_OBJID,
    COM_CAP_TCAP_USERINF =        // (0xb7) User Information Tga
        COM_ASN1_CONFORM | COM_ASN1_CONTEXT   | 0x1e
} COM_CAP_TCAP_DLG_t;

// コンポーネント部種別
typedef enum {
    COM_CAP_TCAP_COMPONENT =      // (0x6c) Compornent portion Tag
        COM_ASN1_CONFORM | COM_ASN1_APPLI     | 0x0c,

    COM_CAP_TCAP_INVOKE =         // (0xa1) Invoke Tag
        COM_ASN1_CONFORM | COM_ASN1_CONTEXT   | 0x01,
    COM_CAP_TCAP_RETURNRESULT =   // (0xa2) Return Result(last) Tag
        COM_ASN1_CONFORM | COM_ASN1_CONTEXT   | 0x02,
    COM_CAP_TCAP_RETURNERROR =    // (0xa3) Return Error Tag
        COM_ASN1_CONFORM | COM_ASN1_CONTEXT   | 0x03,
    COM_CAP_TCAP_REJECT =         // (0xa4) Reject Tag
        COM_ASN1_CONFORM | COM_ASN1_CONTEXT   | 0x04,
    COM_CAP_TCAP_RETURNRESULT2 =  // (0xa7) Return Result(not last) Tag
        COM_ASN1_CONFORM | COM_ASN1_CONTEXT   | 0x07
} COM_CAP_TCAP_COMP_t;


BOOL com_analyzeTcap( COM_ANALYZER_PRM );
void com_decodeTcap( COM_DECODER_PRM );

char *com_getTcapTranName( long iTag );
char *com_getTcapTranPrm( long iTag );
char *com_getTcapDlgPduName( long iTag, long iType );
char *com_getTcapCompName( long iTag );

// TCAPコンポーネント部パラメータ (全てを定義しているわけではない)
enum {
    COM_CAP_TCAP_INVOKEID =       // (0x02) Invoke ID Tag
        COM_ASN1_PRMFORM | COM_ASN1_UNIVERSAL | COM_ASN1_INT,
    COM_CAP_TCAP_LINKEDID =       // (0x80) Linked ID Tag
        COM_ASN1_PRMFORM | COM_ASN1_CONTEXT   | 0x00,
    COM_CAP_TCAP_NULL =           // (0x05) NULL Tag
        COM_ASN1_PRMFORM | COM_ASN1_UNIVERSAL | COM_ASN1_NULL,

    COM_CAP_TCAP_OPCODE_LOCAL =   // (0x02) Local Operation Code Tag (Q.773)
        COM_ASN1_PRMFORM | COM_ASN1_UNIVERSAL | COM_ASN1_INT,
    COM_CAP_TCAP_OPCODE_GLOBAL =  // (0x06) Global Operation Code Tag (X.208)
        COM_ASN1_PRMFORM | COM_ASN1_UNIVERSAL | COM_ASN1_OBJID,

    COM_CAP_TCAP_PARAM_SEQ =      // (0x39) (Parameter) Sequence Tag
        COM_ASN1_CONFORM | COM_ASN1_UNIVERSAL | COM_ASN1_SEQ,
    COM_CAP_TCAP_PARAM_SET =      // (0x31) (Parameter) Set Tag
        COM_ASN1_CONFORM | COM_ASN1_UNIVERSAL | COM_ASN1_SET,

    COM_CAP_TCAP_ERRCODE_LOCAL =  // (0x02) Local Error Code Tag (Q.773)
        COM_ASN1_PRMFORM | COM_ASN1_UNIVERSAL | COM_ASN1_INT,
    COM_CAP_TCAP_ERRCODE_GLOBAL = // (0x06) Global Error Code Tag (X.208)
        COM_ASN1_PRMFORM | COM_ASN1_UNIVERSAL | COM_ASN1_OBJID,

    COM_CAP_TCAP_PROBLEM_GEN =    // (0x80) (Problem) General Problem Tag
        COM_ASN1_PRMFORM | COM_ASN1_CONTEXT   | 0x00,
    COM_CAP_TCAP_PROBLEM_INV =    // (0x81) (Problem) Invoke Tag
        COM_ASN1_PRMFORM | COM_ASN1_CONTEXT   | 0x01,
    COM_CAP_TCAP_PROBLEM_RR  =    // (0x82) (Problem) Return Result Tag
        COM_ASN1_PRMFORM | COM_ASN1_CONTEXT   | 0x02,
    COM_CAP_TCAP_PROBLEM_RE  =    // (0x83) (Problem) Return Error Tag
        COM_ASN1_PRMFORM | COM_ASN1_CONTEXT   | 0x03
};

// TCAPコンポーネントパラメータ
typedef enum {
    COM_SIG_TCAP_INVOKEID,    // Invoke ID
    COM_SIG_TCAP_LINKEDID,    // Linked ID
    COM_SIG_TCAP_OPCODE,      // Operation Code
    COM_SIG_TCAP_PARAM,       // Pramater
    COM_SIG_TCAP_ERROR,       // Error Code
    COM_SIG_TCAP_PROBLEM,     // Problem Code
    // 以降 追加/変更しないこと
    COM_SIG_TCAP_MAX,
    COM_SIG_TCAP_TMP
} COM_SIG_TCAP_PRM_t;

// TCAPコンポーネントパラメータ保持
typedef struct {
    com_sigTlv_t  prm[COM_SIG_TCAP_MAX];
} com_sigTcapCompHead_t;

char *com_getTcapCompPrm( long iTag );
BOOL com_getTcapCompInf( com_sigInf_t *ioHead );


/*
 *****************************************************************************
 * No.7系 (INAP)
 *****************************************************************************
 */

// 次プロトコル値  (com_setPrtclType(): COM_SCCPSSN/COM_TCAPSSN用)
enum {
    COM_CAP_SCCPSSN_INAP = 0xbf   // 191 (国内用INAP)
};

// INAPオペレーションコード
typedef enum {
    COM_CAP_INAP_OP_IDP    = 0x00,
    COM_CAP_INAP_OP_CON    = 0x14,
    COM_CAP_INAP_OP_ENC    = 0x1a,
    COM_CAP_INAP_OP_ERBCSM = 0x18,
    COM_CAP_INAP_OP_RNCE   = 0x19,
    COM_CAP_INAP_OP_RRBE   = 0x17,
    COM_CAP_INAP_OP_SCI    = 0x2e,
    COM_CAP_INAP_OP_RLC    = 0x16,
    COM_CAP_INAP_OP_CUE    = 0x1f,
    COM_CAP_INAP_OP_DFC    = 0x12,
    COM_CAP_INAP_OP_DFCWA  = 0x56,
    COM_CAP_INAP_OP_ETC    = 0x11,
    COM_CAP_INAP_OP_ITC    = 0x80,
    COM_CAP_INAP_OP_MCS    = 0x5b,
    COM_CAP_INAP_OP_ML     = 0x5d
} COM_CAP_INAP_OP_t;

BOOL com_analyzeInap( COM_ANALYZER_PRM );
void com_decodeInap( COM_DECODER_PRM );

char *com_getInapOpName( long iOpcode );


/*
 *****************************************************************************
 * No.7系 (GSMMAP)
 *****************************************************************************
 */

// 次プロトコル値  (com_setPrtclType(): COM_SCCPSSN/COM_TCAPSSN用)
enum {
    COM_CAP_SCCPSSN_HLR = 0x06,   // HLR向け GSM MAP
    COM_CAP_SCCPSSN_MSC = 0x08    // MSC向け GSM MAP
};

// GSM MAPオペレーションコード
typedef enum {
    // Mobile Service Operations
    COM_CAP_GSMMAP_OP_UPDLOC    = 2,   // updateLocation
    COM_CAP_GSMMAP_OP_CANLOC    = 3,   // cancelLocation
    COM_CAP_GSMMAP_OP_PRGMS     = 67,  // purgeMS
    COM_CAP_GSMMAP_OP_SNDID     = 55,  // sendIdentification
    COM_CAP_GSMMAP_OP_UPDGLOC   = 23,  // updateGprsLocation
    COM_CAP_GSMMAP_OP_PRVSUBINF = 70,  // provideSubscriberInfo
    COM_CAP_GSMMAP_OP_ANTITR    = 71,  // anyTimeInterrogation
    COM_CAP_GSMMAP_OP_ANTSUBITR = 62,  // anyTimeSubscriptionInterrogation
    COM_CAP_GSMMAP_OP_ANTMOD    = 65,  // anyTimeModification
    COM_CAP_GSMMAP_OP_NOTSUBMOD = 5,   // noteSubscriberDataModified
    COM_CAP_GSMMAP_OP_PREHO     = 68,  // prepareHandover
    COM_CAP_GSMMAP_OP_SNDENDSIG = 29,  // sednEndSignal
    COM_CAP_GSMMAP_OP_PRCACCSIG = 33,  // processAccessSignalling
    COM_CAP_GSMMAP_OP_FWDACCSIG = 34,  // forwardAccessSignalling
    COM_CAP_GSMMAP_OP_PRESUBHO  = 69,  // prepareSubsequentHandover
    COM_CAP_GSMMAP_OP_SNDATHINF = 56,  // sendAuthenticationInfo
    COM_CAP_GSMMAP_OP_ATHFALREP = 15,  // authenticationFailureReport
    COM_CAP_GSMMAP_OP_CHKIMEI   = 43,  // checkIMEI
    COM_CAP_GSMMAP_OP_INSSUB    = 7,   // insertSubscriberData
    COM_CAP_GSMMAP_OP_DELSUB    = 8,   // deleteSubscriberData
    COM_CAP_GSMMAP_OP_RESET     = 37,  // reset
    COM_CAP_GSMMAP_OP_FWDCHKSSI = 38,  // forwardCheckSS-Indication
    COM_CAP_GSMMAP_OP_RESDAT    = 57,  // restoreData
    COM_CAP_GSMMAP_OP_SNDRINFGP = 24,  // sendRoutingInfoForGprs
    COM_CAP_GSMMAP_OP_FALREP    = 25,  // failureReport
    COM_CAP_GSMMAP_OP_NOTMSPRGP = 26,  // noteMsPresentForGprs
    COM_CAP_GSMMAP_OP_NOTMMEV   = 89,  // noteMM-Event
    // Operation and Maintenance Operations
    COM_CAP_GSMMAP_OP_ACTTRC    = 50,  // activateTraceMode
    COM_CAP_GSMMAP_OP_DEATRC    = 51,  // deactivateTraceMode
    COM_CAP_GSMMAP_OP_SNDIMSI   = 58,  // sendIMSI
    // Call Handling Operations
    COM_CAP_GSMMAP_OP_SNDRINF   = 22,  // sendRoutingInfo
    COM_CAP_GSMMAP_OP_PRVROAMNO = 4,   // provideRoamingNumber
    COM_CAP_GSMMAP_OP_RESCALHND = 6,   // resumeCallHandling
    COM_CAP_GSMMAP_OP_SETREPST  = 73,  // setReportingState
    COM_CAP_GSMMAP_OP_STREP     = 74,  // StatusReport
    COM_CAP_GSMMAP_OP_REMUSRFRE = 75,  // remoteUserFree
    COM_CAP_GSMMAP_OP_ISTALT    = 87,  // ist-Alert
    COM_CAP_GSMMAP_OP_ISTCMD    = 88,  // ist-Command
    COM_CAP_GSMMAP_OP_RELRES    = 20,  // releaseResources
    // Supplementary service operations
    COM_CAP_GSMMAP_OP_REGSS     = 10,  // registerSS
    COM_CAP_GSMMAP_OP_ERASS     = 11,  // eraseSS
    COM_CAP_GSMMAP_OP_ACTSS     = 12,  // activateSS
    COM_CAP_GSMMAP_OP_DEASS     = 13,  // deactivateSS
    COM_CAP_GSMMAP_OP_ITRSS     = 14,  // interrogateSS
    COM_CAP_GSMMAP_OP_PRCUSSREQ = 59,  // processUnstructuredSS-Request
    COM_CAP_GSMMAP_OP_USSREQ    = 60,  // unstructuredSS-Request
    COM_CAP_GSMMAP_OP_USSNOT    = 61,  // unstructuredSS-Notify
    COM_CAP_GSMMAP_OP_REGPW     = 17,  // registerPassword
    COM_CAP_GSMMAP_OP_GETPW     = 18,  // getPassword
    COM_CAP_GSMMAP_OP_SSINVNOT  = 72,  // ss-InvocationNotification
    COM_CAP_GSMMAP_OP_REGCCENT  = 76,  // registerCC-Entry
    COM_CAP_GSMMAP_OP_ERACCENT  = 77,  // eraseCC-Entry
    // Short message service operations
    COM_CAP_GSMMAP_OP_SNDRINFSM = 45,  // sendRoutingInfoForSM
    COM_CAP_GSMMAP_OP_MOFWDSM   = 46,  // mo-ForwardSM
    COM_CAP_GSMMAP_OP_MTFWDSM   = 44,  // mt-ForwardSM
    COM_CAP_GSMMAP_OP_REPSMDELS = 47,  // reportSM-DeliveryStatus
    COM_CAP_GSMMAP_OP_ALTSRVCNT = 64,  // alertServiceCentre
    COM_CAP_GSMMAP_OP_INFSRCCNT = 63,  // informServiceCentre
    COM_CAP_GSMMAP_OP_RDYSM     = 66,  // readyForSM
    COM_CAP_GSMMAP_OP_MTFWDSMVG = 21   // mt-ForwardSM-VGCS
} COM_CAP_GSMMAP_OP_t;


BOOL com_analyzeGsmmap( COM_ANALYZER_PRM );
void com_decodeGsmmap( COM_DECODER_PRM );

char *com_getGsmmapOpName( long iOpcode );



// シグナル機能セット2 導入フラグ ////////////////////////////////////////////
#define USING_COM_SIGNAL2
// シグナル機能セット2 初期化関数マクロ (COM_INITIALIZE()で使用 //////////////
#ifdef COM_INIT_SIGNAL2
#undef COM_INIT_SIGNAL2
#endif
#define COM_INIT_SIGNAL2  com_initializeSigSet2()

