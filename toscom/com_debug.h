/*
 * comモジュール専用 デバッグ処理群  by TOS
 *   
 *  comモジュールのソースでは本ヘッダを com_if.h直後に必ずインクルードする。
 *  本ファイルは com_debug.c専用のヘッダファイルとなる。ただし、外部公開できる
 *  I/Fも含まれており、そうしたI/Fのプロトタイプ宣言などは com_if.hに存在する。
 *  (特に外部公開機能の代表格たる 画面出力＆ログ関連I/Fは、デバッグ機能とも
 *   深く関わる部分が多く、com_proc.c に別で書くのはあまり得策ではなかった。
 *   こうした理由で関数の実体定義を com_debug.c に記述している)
 *
 *  外部公開せず comモジュール内でのみ使うことを想定したI/Fも com_debug.cに
 *  記述されており、そうした関数のプロトタイプ宣言は本ファイルにまとめる。
 *  外部公開しないため、本ヘッダファイルのI/Fは aboutIf.txt には記述しない。
 *
 *  なお一部の関数定義は com_procThread.c にも記述されている。
 */

#pragma once

/*
 * ログバッファ格納処理マクロ  COM_SET_FORMAT()
 * ---------------------------------------------------------------------------
 * 過変数引数で指定されたパラメータを読み取り、TARGETで指定したバッファに
 * 文字列として編集するマクロ。TARGETには文字列バッファを指定すること。
 * sizeof( TARGET )が正しく計算出来る必要がある。
 * また書式文字列を iFormat という変数名で受け取ることを大前提とする。
 * iFormatが NULLの場合、TARGETへの文字列書き込みは行わない。
 */
#define COM_SET_FORMAT( TARGET ) \
    do {\
        COM_CLEAR_BUF( TARGET ); \
        if( iFormat ) { \
            va_list ap; \
            va_start( ap, iFormat ); \
            vsnprintf( (TARGET), sizeof(TARGET), iFormat, ap ); \
            va_end( ap ); \
        }\
    } while(0)

/*
 * メモリ操作対象処理文字列取得  com_getMemOperator()
 *   iTypeに対応する関数名の文字列を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 */

// メモリ操作関数指定
typedef enum {
    COM_FREE = 0,          // com_free()
    COM_MALLOC,            // com_malloc()
    COM_REALLOC,           // com_realloc()
    COM_STRDUP,            // com_strdup()
    COM_STRNDUP,           // com_strndup()
    COM_SCANDIR,           // com_scandir()
    COM_FREEADDRINFO,      // com_freeAddrInfo()  ＊com_select.h使用時のみ
    COM_GETADDRINFO        // com_getAddrInfo()   ＊com_select.h使用時のみ
} COM_MEM_OPR_t;

char *com_getMemOperator( COM_MEM_OPR_t iType );

/*
 * メモリ監視情報追加  com_addMemInfo()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: 情報保持のためのメモリ捕捉失敗
 *                    最大値を超える情報登録
 * ===========================================================================
 * メモリ捕捉時に呼んで、iPtrをキーにメモリ監視情報を保持する。
 * iPtr/iSize/iInfoは呼び元からそのまま受け取る。
 * 保持用のメモリ捕捉に失敗したら COM_ERR_DEBUGNG のエラーを出す。
 *
 * iFormat以降の内容は参考情報として保持するが、COM_DEBUGINFO_LABEL が、
 * 保持する最大サイズとなる。超える場合は切り捨てる。
 *
 * realloc()を使用する処理の場合、先に com_deleteMmeInfo()で元アドレスの
 * メモリ監視情報削除が必要。
 */
void com_addMemInfo(
        COM_FILEPRM, COM_MEM_OPR_t iType, const void *iPtr, size_t iSize,
        const char *iFormat, ... );

/*
 * メモリ監視情報確認  com_checkMemInfo()
 *   指定されたアドレスの監視情報有無を true/false で返す。
 *   メモリ監視モードが COM_DEBUG_OFFの場合、常に trueを返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * iPtrで指定したアドレスのメモリ監視情報があるかどうかをチェックし結果を返す。
 */
BOOL com_checkMemInfo( const void *iPtr );

/*
 * メモリ監視情報削除  com_deleteMemInfo()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DOUBLEFREE: 指定されたアドレスのメモリ監視情報なし
 * ===========================================================================
 * メモリ解放時に呼んで、iPtrをキーにしたメモリ監視情報を削除する。
 * 情報がなかった場合、COM_ERR_DOUBLEFREE のエラーを出す。
 * 
 * 正しく削除できた時は true、上記のエラー時は false を返すことにより、
 * 二重解放で落ちることを避けるのも可能ではある。
 * しかし本I/Fはデバッグ用であり、リリース版でこれが動かない可能性が高いこと
 * を考えると、「デバッグ版は落ちなかったが、リリース版では落ちる」という
 * 事態は避けた方が良いと判断している。
 */
void com_deleteMemInfo( COM_FILEPRM, COM_MEM_OPR_t iType, const void *iPtr );

/*
 * メモリ監視情報出力  com_listMemInfo()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * 現在保持しているメモリ監視情報を全て出力する。
 */
void com_listMemInfo( void );

/*
 * メモリ捕捉デバッグNG判定  com_debugMemoryError()
 *   NG発生有無を true/false で返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * デバッグモードでメモリ捕捉NGを発生させる条件を満たしているか判定する。
 * trueを返した場合、メモリ捕捉NGを返すタイミングであることを示す。
 * com_debugMemoryErrorOn()・com_debugMemoryErrorOff() で制御する。
 */
BOOL com_debugMemoryError( void );

/*
 * メモリ監視情報出力スキップ  com_skipMemInfo()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * comモジュール内専用。MODE(iMode)に trueを指定すると、メモリ監視情報出力を
 * 抑制する。false指定でもう一度 本I/Fを使用するまでこの状態は続く。
 *
 * false指定でこの抑制が解除されたら、抑制した出力数をデバッグログに出力する。
 * デバッグ表示設定が COM_DEBUG_OFF以外なら、true指定した関数名と、false指定
 * したファイル位置も出力する。true/falseの指定が別の関数で行われた場合、
 * 関数名とファイル位置は整合が取れなくなるが、意味は通じるはずである。
 *
 * 一度 false指定を挟まずに true指定を複数回実施した場合、false指定も同じ回数
 * 実施しなければ、最初の状態(出力スキップしない状態)には戻らない。
 * 最初の状態に戻った時に、抑制した出力数をまとめて出す動作となる。
 *
 * メモリ捕捉のために comモジュールでも com_malloc()等を利用しており、
 * その分のメモリ監視情報も追加される。それは出力する場合 [COM] が付与されるが
 * その数はかなり多い。そこで動作が保証され、ログを残す必要性が大きくない場合
 * メモリ捕捉の前に本I/Fを使用し、toscom使用者のデバッグ情報を見やすくする。
 * ログが出なくなるだけで、メモリ監視情報は保持している。そのため浮きがあれば
 * プログラム終了時にデバッグ出力する。
 *
 * 浮き発生時、デバッグでは必要なことも依然としてあるため、
 * マクロ NOTSKIP_MEMWATCH コンパイルオプションで宣言してビルドした場合、
 * 本I/Fの設定に関わらず、メモリ監視情報出力をスキップしなくなる。
 */
#define com_skipMemInfo( MODE ) \
    com_skipMemInfoFunc( COM_FILELOC, (MODE) )

void com_skipMemInfoFunc( COM_FILEPRM, BOOL iMode );

/*
 * メモリ監視情報 強制出力
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * iModeを trueに設定すると、comモジュールのログ出力抑制対象となっていても、
 * メモリ監視情報の出力を行う。false を設定すればこの状態は解除される。
 *
 * メモリ捕捉は出力抑制するのに、メモリ解放は出力抑制しない、という処理があると
 * ログ的に不整合が生じるように見える。メモリ解放も抑制すると都合が悪い場合に
 * メモリ捕捉時に本I/Fでメモリ監視情報を強制出力させれば、捕捉・解放のどちらも
 * ログで確認できるようになる。
 */
void com_forceLog( BOOL iMode );


// ファイル監視関数指定
typedef enum {
    COM_FOPEN = 0,      // com_fopen()
    COM_FCLOSE          // com_fclose()
} COM_FILE_OPR_t;

/*
 * ファイル監視情報追加  com_addFileInfo()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: 情報保持のためのメモリ捕捉失敗
 *                    最大値を超える情報登録
 * ===========================================================================
 * ファイルオープン時に呼んで、fpをキーにファイル監視情報を保持する。
 * iFp/iPath は呼び元からそのまま受け取る。
 * 保持用のメモリ捕捉に失敗したら COM_ERR_DEBUGNG のエラーを出す。
 *
 * iPathの内容は参考情報として保持するが、COM_DEBUGINFO_LABEL が最大サイズで
 * それを超える場合は切り捨てられる。
 */
void com_addFileInfo(
        COM_FILEPRM, COM_FILE_OPR_t iType, const FILE *iFp, const char *iPath );

/*
 * ファイル監視情報確認  com_checkFileInfo()
 *   指定されたアドレスの監視情報有無を true/false で返す。
 *   ファイル監視モードが COM_DEBUG_OFFの場合、常に trueを返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 */
BOOL com_checkFileInfo( const FILE *iFp );

/*
 * ファイル監視情報削除  com_deleteFileInfo()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DOUBLEFREE: 指定されたアドレスのメモリ監視情報なし
 * ===========================================================================
 * メモリ解放時に呼んで、iFpをキーにしたファイル監視情報を削除する。
 * 情報がなかった場合、COM_ERR_DOUBLEFREE のエラーを出す。
 */
void com_deleteFileInfo( COM_FILEPRM, COM_FILE_OPR_t iType, const FILE *iFp );

/*
 * ファイル監視情報出力  com_listFileInfo()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * 現在保持しているファイル監視情報を全て出力する。
 */
void com_listFileInfo( void );

/*
 * ファイルオープンデバッグNG判定  com_debugFopenError()
 *   NG発生有無を true/false で返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * デバッグモードでオープンNGを発生させる条件を満たしているか判定する。
 * trueを返した場合、NGを返すタイミングであることを示す。
 * com_debugFopenErrorOn()・com_debugFopenErrorOff() で制御する。
 */
BOOL com_debugFopenError( void );

/*
 * ファイルクローズデバッグNG判定  com_debugFcloseError()
 *   NG発生有無を true/false で返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * iFpでクローズしようとしているファイルポインタを指定する。
 * デバッグモードでクローズNGを発生させる条件を満たしているか判定する。
 * trueを返した場合、NGを返すタイミングであることを示す。
 * com_debugFcloseErrorOn()・com_debugFcloseErrorOff() で制御する。
 */
BOOL com_debugFcloseError( const FILE *iFp );

/*
 * ファイル監視情報出力スキップ  com_skipFileInfo()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * iModeに trueを指定するとファイル監視情報出力を抑制し、falseで抑制解除する。
 * ファイル開閉が必要な comモジュールのI/Fは com_fopen/com_fcloseを使用する。
 * その場合、本I/Fでファイル監視情報出力は抑制している。
 */
void com_skipFileInfo( BOOL iMode );

// com_skipFileInfo()を使い、ファイル監視情報の出力を抑制しながら、
// ファイルの開閉を行う関数マクロ

#define COM_FOPEN_MUTE( FP, PATH, MODE ) \
    com_skipFileInfo( true ); \
    FILE *FP = com_fopen( (PATH), (MODE) ); \
    com_skipFileInfo( false ); \
    do{} while(0)

#define COM_FCLOSE_MUTE( FP ) \
    com_skipFileInfo( true ); \
    com_fclose( FP ); \
    com_skipFileInfo( false ); \
    do{} while(0)

/*
 * 関数パラメータNG表示  com_prmNG()・COM_PRMNG()・COM_PRMNGF()
 * ---------------------------------------------------------------------------
 *   本I/Fは COM_ERR_DEBUGNG を発生させる共通処理として、comモジュールで使う。
 *   基本的に、本I/Fによるエラーが発生しないようにしなければならない。
 * ===========================================================================
 * 本I/Fは comモジュール内で関数引数が不正な場合のエラー出力をする。
 *
 * com_prmNG()の引数で関数名の文字列が渡されることを想定している。
 * NULLを指定した場合、マクロ展開されて渡された関数名がそのまま使われる。
 * エラーメッセージに出すべき関数名とエラー発生関数名が同じなら NULLで良い。
 * 関数名を別で指定したい時に、文字列を引数で設定する。
 *
 * toscomの外部公開関数使用時に引数不正あり＝使い方を誤ったバグである可能性が
 * 高いため、敢えて COM_ERR_DEBUGNG のエラーを出して警告するのが目的で、
 * このエラーが出ないようにして toscomは使わなければならない。
 *
 * また toscomでも全てのI/Fが本I/Fを使っているわけではない。
 * 引数内容がある程度保証されている以下は、そもそも引数チェックを殆どしない。
 * ・static宣言された内部関数
 * ・com_val～ のが付された文字列条件判定関数
 *
 * 引数で返り値を指定できるマクロ形式関数 COM_PRMNG()を用意する。
 * 返り値がない場合は COM_PRMNG(); という記述で問題ない。
 * 引数で関数名と返り値を指定するマクロ形式関数 COM_PRMNGF()も用意する。
 * これも返り値がない場合は COM_PRMNGF( "function", ); という記述が可能。
 */
#define com_prmNG( ... ) \
    com_prmNgFunc( COM_FILELOC, __VA_ARGS__ )

void com_prmNgFunc( COM_FILEPRM, const char *iFormat, ... );

#define COM_PRMNG( RESULT ) \
    do{ com_prmNG(NULL); return RESULT; }while(0)

#define COM_PRMNGF( FUNCNAME, RESULT ) \
    do{ com_prmNG(FUNCNAME); return RESULT; }while(0)


/*
 * スレッド処理の初期化/終了処理  com_initializeThread()・com_finalizeThread()
 * ---------------------------------------------------------------------------
 * ===========================================================================
 * com_initialize()/finalizeCom()から呼ばれる専用となる。
 */
void com_initializeThread( void );
void com_finalizeThread( void );

/*
 * 排他とメモリ監視表示スキップの連動  com_mutexLockCom()・com_mutexUnlockCom()
 * ---------------------------------------------------------------------------
 * ===========================================================================
 * com_mutexLockCom()は com_mutexLock()と com_skipMemInfo(true)を実施する。
 *
 * com_mutexUnlockCom()は com_mutexUnlock()と com_skipMemInfo(false)を実施し
 * さらに iResultで指定した返り値をそのまま返す。これは return時の返却値として
 * 直接記述する利用方法を想定している。(returnとの併用は BOOL型のときのみ)
 */
void com_mutexLockCom( pthread_mutex_t *ioMutex, COM_FILEPRM );
BOOL com_mutexUnlockCom( pthread_mutex_t *ioMutex, COM_FILEPRM, BOOL iResult );

/*
 * comモジュール専用デバッグ表示I/F
 *   com_dbgCom()・com_dbgFuncCom()・com_dumpCom()・com_logCom()
 * ---------------------------------------------------------------------------
 * ===========================================================================
 * デバッグ出力時に [DBG] ではなく [COM] を付加する。
 * comモジュール以外での使用は禁止。
 * 逆に comモジュール内のデバッグ表示は本I/Fを使うことが必須。
 *
 * CHECK_PRINT_FORMAT については、com_if.hの com_printf()の説明記述参照。
 */

// com_debug() COM版
#ifndef CHECK_PRINT_FORMAT
void com_dbgCom( const char *iFormat, ... );
#else 
#define  com_dbgCom  printf
#endif

//com_debugFunc() COM版
#define com_dbgFuncCom( ... ) \
    com_dbgFuncComWrap( COM_FILELOC, __VA_ARGS__ )

void com_dbgFuncComWrap( COM_FILEPRM, const char *iFormat, ... );

// com_dump() COM版
void com_dumpCom( const void *iAddr, size_t iSize, const char *iFormat, ... );

// com_printfLogOnly() COM版
#ifndef CHECK_PRINT_FORMAT
void com_logCom( const char *iFormat, ... );
#else
#define  com_logCom  printf
#endif

/*
 * 標準出力抑制  com_suspendStdout()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * iModeを trueに設定すると、com_printf()・com_debug()とこれを利用するI/Fに
 * よる標準出力への表示を抑制する。デバッグログへの書き込みは実施する。
 * falseにすれば解除される。
 *
 * 主にエキストラ機能のウィンドウモードでの使用を想定したもの。
 */
void com_suspendStdout( BOOL iMode );

/*
 * タイトル表示  com_dispTitle()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * タイトルを表示する。そのフォーマットは com_setTitleForm()で変更可能。
 * iAddにはそのフォーマット内で最後に出力する文字列を指定する。
 */
void com_dispTitle( const char *iAdd );

/*
 * デバッグ機能初期化  com_initializeDebugMode()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * com_initialize()で呼ばれることを想定する(それ以外で呼ぶ必要なし)。
 */
void com_initializeDebugMode( void );

/*
 * デバッグ機能終了  com_finalizeDebugMode()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * 最後に com_listMemInfo()・com_listFileInfo()・com_closeDebugLog()を呼ぶ。
 * その他デバッグ機能関連のリソース解放も実施する。
 * finalizeCom()で呼ばれることを想定する(それ以外で呼ぶ必要なし)。
 */
void com_finalizeDebugMode( void );

/*
 * エラー統計情報の精算  com_adjustError()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * プログラム終了時に呼ばれて、エラーカウント一覧を出力する。出力先については
 * デバッグ表示モードの設定に従う。
 * この関数でエラー統計情報のメモリ解放も実施する。
 * finalizeCom()で呼ばれることを想定する(それ以外で呼ぶ必要なし)。
 */
void com_adjustError( void );

/*
 * デバッグログファイルクローズ  com_closeDebugLog()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 * finalizeCom()で呼ばれることを想定する(それ以外で呼ぶ必要なし)。
 */
void com_closeDebugLog( void );

#ifdef USE_FUNCTRACE
/*
 * ファイルの呼出/復帰時にコールされるシステム関数
 *   ＊関数呼出トレース機能用。この機能自体の説明は com_if.hの
 *     com_dispFuncTrace() の記述を参照。
 * ---------------------------------------------------------------------------
 *   幾つかエラーが出る可能性があるが、発生時は作者に連絡すること。
 * ===========================================================================
 * 関数呼出トレース機能を使用するために、GNU拡張で定義が定められているI/Fの
 * プロトタイプ宣言となる。本関数を使用するためには「関数呼出トレース機能」が
 * 使えるようにコンパイルオプションを設定する必要がある。その設定方法については
 * com_dispFuncTrace()の説明記述を参照。
 *
 * GNU拡張で名称が固定されているため、関数名やプロトタイプの変更は不可能。
 *  __cyg_profile_func_enter()は関数呼出時にコールされる。
 *  __cyg_profile_func_exit()は関数復帰時にコールされる。
 * 使い方の詳細については、これらの関数の使い方を示した情報を確認すること。
 */
void __cyg_profile_func_enter( void *iFunc, void *iCaller );
void __cyg_profile_func_exit( void *iFunc, void *iCaller );
#endif // USE_FUNCTRACE

// 記述簡略化のために、mutex関連と関数トレースの設定をまとめたマクロ
// (USE_FUNCTRACEの ifdef内に記述しないのが正解)

#define COM_DEBUG_LOCKOFF( MUTEX, FUNC ) \
    com_setFuncTrace( false ); \
    com_mutexLock( (MUTEX), (FUNC) ); \
    do{} while(0)

#define COM_DEBUG_UNLOCKON( MUTEX, FUNC ) \
    com_mutexUnlock( (MUTEX), (FUNC) ); \
    com_setFuncTrace( true ); \
    do{} while(0)

// さらに記述簡略化のために、デバッグ出力を回避する設定をまとめたマクロ。
// 一応どの処理を行うのか選択可能。全てやるので良いなら COM_PROC_ALL を指定。
// 処理をやりたくないものは | で結合してフラグ指定する。

#define COM_PROC_ALL         0
#define COM_NO_FUNCTRACE     1
#define COM_NO_FUNCNAME      2
#define COM_NO_SKIPMEM       4

#define COM_DEBUG_AVOID_START( FLAG ) \
    if(!COM_CHECKBIT( FLAG, COM_NO_FUNCTRACE )) {com_setFuncTrace( false );} \
    if(!COM_CHECKBIT( FLAG, COM_NO_FUNCNAME )) {com_dbgFuncCom( NULL );} \
    if(!COM_CHECKBIT( FLAG, COM_NO_SKIPMEM )) {com_skipMemInfo( true );} \
    do{} while(0)

#define COM_DEBUG_AVOID_END( FLAG ) \
    if(!COM_CHECKBIT( FLAG, COM_NO_SKIPMEM )) {com_skipMemInfo( false );} \
    if(!COM_CHECKBIT( FLAG, COM_NO_FUNCTRACE )) {com_setFuncTrace( true );} \
    do{} while(0)

// トレーサモードの強制設定を関数呼出せずに実行するマクロ。
// COM_TRACER_SETと COM_TRACER_RESUMEは必ずセットで使用すること。
// (BKU_MODE_～は他と被らないことを目指した適当な変数名)

#define COM_TRACER_SET( MODE ) \
    BOOL BKU_MODE_10394857 = gFuncTraceMode; \
    gFuncTraceMode = (MODE); \
    do{} while(0)

#define COM_TRACER_RESUME \
    gFuncTraceMode = BKU_MODE_10394857; \
    do{} while(0)

