/*
 *****************************************************************************
 *
 * 簡易信号アナライザー Analyzer 個別モジュールソース
 *
 *****************************************************************************
 */

#include <assert.h>
#include "anlz_if.h"

#ifdef ANLZ_DECODE
static BOOL  gDecode = true;
#else
static BOOL  gDecode = false;
#endif

// 起動オプションのプロトコル指定
static long  gSetProto = COM_SIG_UNKNOWN;

// 起動オプションのフィルタリング指定
static long  gPickProto = COM_SIG_UNKNOWN;

// フィルタリング対象チェック
static BOOL isPickupStack( com_sigInf_t *iSignal )
{
    if( gPickProto == COM_SIG_UNKNOWN ) {return true;}
    return ( 0 < com_detectProtocol( NULL, iSignal, gPickProto ) );
}

// フレーム番号 (解析したパケットの通し番号)
static long  gFrameNo = 0;    // 現在のフレーム番号
static long  gJumpNo = 0;     // ジャンプ先フレーム番号

// 信号データ読込後に呼ばれる共通の解析起点関数
static BOOL execAnalyze( com_sigInf_t *ioSignal )
{
    com_printf( "Frame:%5ld\n", ++gFrameNo );
    if( gSetProto ) {ioSignal->sig.ptype = gSetProto;}
    if( !com_analyzeSignalToLast( ioSignal, gDecode ) ) {
        com_printf( "<< fail to analyze >>\n" );
    }
    if( (gJumpNo > gFrameNo) || (!isPickupStack( ioSignal )) ) {return true;}
    char*  frame = NULL;
    if( com_input( &frame, NULL, &(com_actFlag_t){false, true},
                   " -- only ENTER to next packet(%ld) --\n", gFrameNo+1 ) )
    {
        long  tmpNo = com_atol( frame );
        if( errno ) {
            if( errno != ERANGE ) {return false;}
            tmpNo = 0;
        }
        if( tmpNo > gFrameNo ) {gJumpNo = tmpNo;}
    }
    return true;
}

// キャプチャファイルからの信号読込
static void readCapFile( char *iFile )
{
    com_capInf_t  inf;
    while( com_readCapFile( iFile, &inf ) ) {
        if( inf.cause == COM_CAPERR_NOERROR ) {
            if( !execAnalyze( &inf.signal ) ) {break;}
        }
        iFile = NULL;  // 継続読込のためにファイル名はNULL設定
    }
    com_debug( " cause = %ld", inf.cause );
    com_freeCapInf( &inf );
}

// ログファイルからの信号読込 (処理は2パターンある)
static void readCapLog( char *iFile )
{
#ifdef ANLZ_USE_SEEKFILE   // com_seekFile()を使って読み込むサンプル
    com_seekCapInf_t  userData = {.notify = execAnalyze, .order = true};
    (void)com_seekFile( iFile, com_seekCapLog, &userData, NULL, 0 );
    com_freeCapInf( &userData.capInf );
#else  // com_readCapLog()を使って読み込むサンプル
    com_capInf_t  inf;
    while( com_readCapLog( iFile, &inf, true ) ) {
        // com_seekFile()使用時と execAnalyze()返り値判定が逆転するので注意
        if( inf.cause == COM_CAPERR_NOERROR ) {
            if( !execAnalyze( &inf.signal ) ) {break;}
        }
        iFile = NULL;  // 継続読込のためにファイル名はNULL設定
    }
    com_freeCapInf( &inf );
#endif
}

static long  gFileCnt = 0;
static char**  gFileList = NULL;

static BOOL viewVersion( com_getOptInf_t *iOptInf )
{
    COM_UNUSED( iOptInf );
    // 特に何もせずに処理終了させる
    com_exit( COM_NO_ERROR );
    return true;
}

static BOOL clearLogs( com_getOptInf_t *iOptInf )
{
    COM_UNUSED( iOptInf );
    com_system( "rm -fr .%s.log_*", com_getAplName() );
    com_printf( "<<< cleared all past logs >>>\n" );
    com_exit( COM_NO_ERROR );
    return true;
}

static long getProtoCode( const char *iLabel )
{
    long  type = com_searchPrtclByLabel( iLabel );
    if( type == COM_SIG_UNKNOWN ) {
        com_error( COM_ERR_PARAMNG, "not support %s", iLabel );
    }
    return type;
}

static BOOL setProtoConfig( char *iToken, long *oTarget, const char *iLabel )
{
    long  type = getProtoCode( iToken );
    if( type == COM_SIG_UNKNOWN ) {return false;}
    com_printf( "--- %s %s ---\n", iLabel, iToken );
    *oTarget = type;
    return true;
}

static BOOL setProto( com_getOptInf_t *iOptInf )
{
    return setProtoConfig( iOptInf->argv[0], &gSetProto, "analzye as" );
}

static BOOL setFilter( com_getOptInf_t *iOptInf )
{
    return setProtoConfig( iOptInf->argv[0], &gPickProto, "pickup" );
}

static BOOL addPrtclPort( long iPort, char *iPrtcl, long iType )
{
    long  nextType = getProtoCode( iPrtcl );
    if( nextType == COM_SIG_UNKNOWN ) {return false;}
    com_sigPrtclType_t  tmp[] = {
        {{iPort},nextType},
        {{COM_PRTCLTYPE_END},0}
    };
    com_setPrtclType( iType, tmp );
    return true;
}

static BOOL addPort( com_getOptInf_t *iOptInf )
{
    long  port = com_atol( iOptInf->argv[0] );
    return addPrtclPort( port, iOptInf->argv[1], iOptInf->optValue );
}

static char  gHelpBasic[] =
    "コマンドライン:  Analyzer [options] [files]\n"
    "\n"
    "  現在、解析可能なプロトコルは以下。\n"
    "  (プロトコル名指定が必要なオプションで指定可能な文字列ともなる。\n"
    "   この際 英文字の大文字小文字は問わない)\n"
    "\n";

static char  gHelpOption[] =
    "  [options]\n"
    "    --help\n"
    "    -h\n"
    "      このヘルプを表示する。\n"
    "\n"
    "    --proto (プロトコル名)\n"
    "    -p (プロトコル名)\n"
    "      指定プロトコルでの解析を実施する。\n"
    "      プロトコル名に指定する文字列は前述した。\n"
    "\n"
    "    --filter (プロトコル名)\n"
    "    -f (プロトコル名)\n"
    "      指定プロトコルのスタックがあるときのみ停止する。\n"
    "      プロトコル名に指定する文字列は前述した。\n"
    "\n"
    "    -ipport (ポート番号) (プロトコル名)\n"
    "      UDP/TCP/SCTPのポート番号とプロトコルの対応を指定する。\n"
    "      SCTPのポート番号は、SCTPの次プロトコル値が 0 のときのみ見る。\n"
    "      プロトコル名に指定する文字列は前述した。\n"
    "\n"
    "    --sctpnext (プロトコル値) (プロトコル名)\n"
    "      SCTPの次プロトコル値とプロトコルの対応を指定する。\n"
    "      プロトコル名に指定する文字列は前述した。\n"
    "\n"
    "    --sccpssn (SSN値) (プロトコル名)\n"
    "      SSNと SCCPの次プロトコル対応を指定する。\n"
    "      プロトコル名に指定する文字列は前述した。\n"
    "\n"
    "    --tcapssn (SSN値) (プロトコル名)\n"
    "      SSNと TCAPの次プロトコル対応を指定する。\n"
    "      プロトコル名に指定する文字列は前述した。\n";

static char  gHelpFiles[] =
    "  [files]\n"
    "    オプション以外の文字列はファイル名と認識。\n"
    "    複数あれば、順番に開いて解析を試みる。\n"
    "\n"
    "    ファイル名指定がない場合はダイレクト入力モードとなる。\n"
    "    バイナリデータのテキストを貼り付け、CTRL+D で解析開始する。\n"
    "    (データ末尾に改行がない場合は CTRL+D を2回入力すること)\n"
    "    何もデータを入れずに CTRL+D のみでツールを終了する。\n";

static void dispProtocolList( long iIndent, long iWidth )
{
    long*  list = NULL;
    long   cnt = com_showAvailProtocols( &list );
    long  dispCnt = 0;
    for( long i = 0;  i < cnt;  i++ ) {
        if( !dispCnt ) {com_repeat( " ", iIndent, false );}
        char*  label = com_searchSigProtocol( list[i] );
        com_printf( "%s ", label );
        dispCnt += strlen( label ) + 1;
        if( dispCnt > iWidth - iIndent ) {
            com_printLf();
            dispCnt = 0;
        }
    }
    if( dispCnt ) {com_printLf();}
    com_free( list );
}

enum {
    PROTOLIST_LEFT  = 4,
    PROTOLIST_RIGHT = 70
};

static BOOL showHelp( com_getOptInf_t *iOptInf )
{
    COM_UNUSED( iOptInf );
    com_printf( "%s", gHelpBasic );
    dispProtocolList( PROTOLIST_LEFT, PROTOLIST_RIGHT );
    com_printLf();
    com_printf( "%s", gHelpOption );
    com_printLf();
    com_printf( "%s", gHelpFiles );
    com_printLf();
    com_exit( COM_NO_ERROR );
    return true;  // ここは通らない
}

static com_getOpt_t  gAnlzOpts[] = {
    { 'v', "version",  0, 0,            false, viewVersion },
    { 'c', "clearlog", 0, 0,            false, clearLogs },
    { 'p', "proto",    1, 0,            false, setProto },
    { 'f', "filter",   1, 0,            false, setFilter },
    { 'h', "help",     0, 0,            false, showHelp },
    {   0, "ipport",   2, COM_IPPORT,   false, addPort },
    {   0, "sctpnext", 2, COM_SCTPNEXT, false, addPort },
    {   0, "sccpssn",  2, COM_SCCPSSN,  false, addPort },
    {   0, "tcapssn",  2, COM_TCAPSSN,  false, addPort },
    COM_OPTLIST_END
};

// 起動パラメータ解析
static void checkParameters( int iArgc, char **iArgv )
{
    if( !com_getOption( iArgc,iArgv,gAnlzOpts,&gFileCnt,&gFileList,NULL ) ) {
        com_exit( COM_ERR_PARAMNG );
    }
}

static char  gPasteBuff[COM_DATABUF_SIZE];

static void directMode( void )
{
    gJumpNo = LONG_MAX;
    while(1) {
        size_t  inRet = com_inputMultiLine( gPasteBuff, sizeof(gPasteBuff),
                                       "<<paste signal data directly>>\n" );
        if( inRet ) {break;}
        uchar*  result = NULL;
        size_t  length = 0;
        if( !com_strtooct( gPasteBuff, &result, &length ) ) {
            com_printf( "input data illegal\n" );
            continue;
        }
        com_sigInf_t  data;
        com_makeSigInf( &data, result, length );
        execAnalyze( &data );
        com_freeSigInf( &data, true );
        com_printLf();
    }
}

// 解析起点関数
void anlz_start( int iArgc, char **iArgv )
{
    checkParameters( iArgc, iArgv );
    if( !gFileCnt ) {directMode();}
    for( long i = 0;  i < gFileCnt;  i++ ) {
        com_printLf();
        com_printTag( "-", 79, COM_PTAG_LEFT, "%s", gFileList[i] );
        com_printLf();
        if( !strstr( gFileList[i], ".log" ) ) {readCapFile( gFileList[i] );}
        else {readCapLog( gFileList[i] );}
    }
}

#ifndef ANLZ_DEBUG
// エラー発生時にフックして、解析エラーはスキップを指定する処理
static COM_HOOKERR_ACTION_t skipAnalyzeError( com_hookErrInf_t *iInf )
{
    if( iInf->code < COM_ERR_NOSUPPORT || iInf->code > COM_ERR_ANALYZENG ) {
        return COM_HOOKERR_EXEC;
    }
    // 解析エラーについては出さないようにする。(呼び元でだすことを期待)
    return COM_HOOKERR_SKIP;
}
#endif

// anlzモジュール初期化 (シグナル機能の初期化も実施)
void anlz_initialize( void )
{
#ifdef ANLZ_DEBUG
    com_setDebugSignal( true );  // toscomのシグナル機能でバッグ出力を全面使用
#else
    com_setDebugSignal( false );
    com_hookError( skipAnalyzeError );
#endif
    (void)addPrtclPort( 5070, "SIP", COM_IPPORT );
}

