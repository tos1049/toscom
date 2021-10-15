/*
 *****************************************************************************
 *
 * 共通で色々使えそうな処理群 by TOS
 *
 *  ＊全てのソースでこのヘッダをインクルードする想定。
 *    具体的な使用方法は同梱の DOCUMENTS/ を参照。
 *
 *  ＊本ファイルで標準ヘッダのインクルードも基本的に済ませる。
 *    独自にインクルード追加したい場合は、com_if.hではなく
 *      com_spec.h
 *    の方に追加するようにすることで、com_if.hへの独自変更を抑える。
 *    com_spec.hは独自の関数プロトタイプや構造体等の宣言に使用しても良い。
 *    そして独自関数の定義については
 *      com_sepc.c
 *    に追加する。
 *
 *  現在の処理機能(vimなら COM～の上にカーソルを移動させて * や # で移動可能)
 *  ・COMDEF   ：共通データ宣言
 *  ・COMBASE  ：モジュール基本I/F
 *  ・COMMEM   ：メモリ関連I/F
 *  ・COMBUF   ：文字列バッファ関連I/F
 *  ・COMCHAIN ：文字列チェーンバッファ関連I/F ＋ それを利用した数値チェーンも
 *  ・COMHASH  ：ハッシュテーブル関連I/F (ハッシュ検索を伴うデータ形式)
 *  ・COMSORT  ：ソートテーブル関連I/F (二分探索法を使うデータ形式)
 *  ・COMRING  ：リングバッファ関連I/F
 *  ・COMCFG   ：コンフィグ関連I/F
 *  ・COMCONV  ：データ変換関連I/F
 *  ・COMUSTR  ：文字列ユーティリティ関連I/F
 *  ・COMUTIME ：時間ユーティリティ関連I/F
 *  ・COMFILE  ：ファイル/ディレクトリ関連I/F
 *  ・COMTHRD  ：スレッド関連I/F
 *  ・COMPRINT ：画面出力/ロギング関連I/F
 *  ・COMDEBUG ：デバッグ関連I/F
 *  ・COMTEST  ：テスト関連I/F
 *
 *****************************************************************************
 */

/*
 *----------------------------------------------------------------------------
 * 標準関数の代替
 *   com_(標準関数名) としたI/Fが幾つか存在する。
 *   標準関数ではなく、これらの代替I/Fを使用することを強く推奨する。
 *   現状、該当するのは以下。標準関数との差分を概説しておく。
 *   詳細は各ファイルの各I/Fに付された説明を確認すること。
 *
 *    --- モジュール基本I/F ---
 *      com_getOption()
 *                     getopt()/getopt_long() の統合機能拡張版と言えるもの。
 *                     引数の構成は全く違うが、代替I/Fとして使用可能。
 *      com_exit()     強制終了のメッセージを出力する。
 *      com_system()   書式文字列を使ったコマンドライン指定が可能・
 *
 *    --- メモリ関連I/F ---
 *      com_malloc()   メモリ確保後 0クリアする。(実は calloc()を使用)
 *                     確保失敗時にエラー出力。故意に失敗するデバッグ機能あり。
 *      com_realloc()  再確保に差分はないが、派生で com_reallocf()が存在する。
 *                     確保失敗時にエラー出力。故意に失敗するデバッグ機能あり。
 *      com_strdup()   malloc()と strcpy()を使用して処理をする。
 *                     確保失敗時にエラー出力。故意に失敗するデバッグ機能あり。
 *      com_free()     メモリ解放後、NULLを代入する。
 *
 *    --- データ変換関連I/F ---
 *      com_strtoul()  変換のNG判定を実施しエラー出力し、errnoを設定する。
 *                     エラー出力は抑制可能。
 *      com_strtol()   com_strtoul()と同様のNG判定とエラー出力判定を行う。
 *      com_strtof()   com_strtoul()と同様のNG判定とエラー出力判定を行う。
 *      com_strtod()   com_strtoul()と同様のNG判定とエラー出力判定を行う。
 *      com_atoi()     エラー出力はしないが、errnoにより変換NGを検出可能。
 *      com_atol()     エラー出力はしないが、errnoにより変換NGを検出可能。
 *      com_atof()     エラー出力はしないが、errnoにより変換NGを検出可能。
 *
 *    --- 時間ユーティリティ関連I/F ---
 *      com_gettimeofday()
 *                     gettimeofday()に失敗した時にエラー出力する。
 *  
 *    --- 文字列ユーティリティ関連I/F ---
 *      com_strncpy()  コピー元文字列が指定長より短くても \0で埋めない。
 *                     コピーした文字列の最後に必ず '\0'を付与する。
 *                     バッファサイズの引数が増えており、バッファサイズを
 *                     超すようなコピーの場合は、バッファサイズに収まる範囲で
 *                     コピーする長さを調節し、返り値でその発生を通知する。
 *      com_strcpy()   書式は strcpy()と同じだが、com_strncpy()を使用する為、
 *                     コピー先の変数は sizeof()で正しくサイズ計算できる必要が
 *                     ある。つまり全ての strcpy()を代用できるわけではない。
 *                     またコピー先バッファのサイズが十分保証できるならば、
 *                     無理にこれを使う必要は無い。
 *      com_strncat()  返り値が処理成否の true/falseになる。
 *                     バッファサイズの引数が増えており、バッファサイズを
 *                     超すような連結の場合は、バッファサイズに収まる範囲で
 *                     連結する長さを調節し、返り値でその発生を通知する。
 *      com_strcat()   書式は strcat()と同じだが、com_strncat()を使用する為、
 *                     コピー先の変数は sizeof()で正しくサイズ計算できる必要が
 *                     ある。つまり全ての strcat()を代用できるわけではない。
 *                     また連結先バッファのサイズが十分保証できるならば、
 *                     無理にこれを使う必要は無い。
 *
 *    --- ファイル/ディレクトリ関連I/F ---
 *      com_fopen()    ファイルオープン失敗時にエラー出力。
 *                     故意に失敗するデバッグ機能あり。
 *      com_fclose()   ファイルクローズ後、NULLを代入する。
 *                     fclose()の返り値をチェックしクローズ失敗時にエラー出力。
 *                     com_fclose()の返り値はエラー時の errno(正常時は 0)。
 *                     故意に失敗するデバッグ機能あり。
 *
 *    --- 画面出力/ロギング関連I/F ---
 *      com_printf()   返り値を持たない。デバッグログにも同じ内容を出力する。
 *                     COM_TEXTBUF_SIZEを超える出力は切り捨てる。
 *      com_clear()    デバッグログに ******～ を3行書き込む。
 *
 *    --- デバッグ関連I/F ---
 *      com_strerror() strerr_r()を使ってスレッドセーフな処理を実施し、
 *                     自分が指定したバッファをそのまま返す。
 *                     これがNGを返したときは "<strerror_r NG>"を返す。
 *
 *   処理差分を利用したくない場合、標準関数を使用する方法を検討する。
 *   しかしメモリ捕捉/解放とファイルオープン/クローズは例外なく上記を使うこと。
 *   そうしなければデバッグ機能によるリソース監視が正しく出来なくなる。
 *   (何らかの理由でリソース監視から外したい場合は、標準関数を使用する)
 *----------------------------------------------------------------------------
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdbool.h>
#include <pthread.h>

/* toscomの挙動でカスタマイズできるものについては、以下に宣言する */
#include "com_custom.h"

/* com_spec.h は本ヘッダの末尾にインクルードしている */


/*
 *****************************************************************************
 * COMDEF:共通データ宣言
 *****************************************************************************
 */

/* 符号なし型の短縮形宣言 */
typedef  unsigned int     uint;
typedef  unsigned long    ulong;
typedef  unsigned short   ushort;
typedef  unsigned char    uchar;

/*
 * com_custom.h では toscomでも多用する BOOL型 を宣言している。
 * これの元は long型だが、値としては true/false を期待している。
 * なぜ com_custom.hで宣言しているかについては、com_custom.h の方に記述した。
 * 詳細については、そちらを参照。
 */

/* 共通定数宣言 */
enum {
    COM_WORDBUF_SIZE = 64,       // ワードバッファサイズ
    COM_TERMBUF_SIZE = 80,       // ターミナルバッファサイズ
    COM_LINEBUF_SIZE = 256,      // ラインバッファサイズ
    /*
     * 基本的にこれ以降のサイズの自動変数の定義は非推奨。静的定義の検討を。
     * (これ以前の比較的小さなサイズのバッファも、多用されることが分かって
     *  いる場合は、関数内であっても staticを付けて静的定義としたほうが良い)
     * やむを得ず、自動変数にする場合、その理由をコメントに書くこと。
     * 例えば再帰呼び出しありの関数内では、静的変数に出来ないことはあり得る。
     */
    COM_TEXTBUF_SIZE = 1024,     // テキストバッファサイズ
    COM_DATABUF_SIZE = 4096      // データバッファサイズ
};

/* COMモジュール共通部 エラー宣言 ＊連動テーブルは gErrorNameCom[] */
enum {
    COM_NO_ERROR = 0,            // エラー無し
    /**** システムエラー ****/
    COM_ERR_PARAMNG    = 900,    // パラメータNG (起動オプションNGで使用)
    COM_ERR_NOMEMORY   = 901,    // メモリ不足
    COM_ERR_DOUBLEFREE = 902,    // 二重解放の疑い (メモリ/ファイル監視時のみ)
    COM_ERR_CONVERTNG  = 903,    // データ変換NG
    COM_ERR_FILEDIRNG  = 904,    // ファイル/ディレクトリ操作NG
    COM_ERR_HASHNG     = 905,    // ハッシュ検索処理NG
    COM_ERR_TIMENG     = 906,    // 時間取得NG
    COM_ERR_ARCHIVENG  = 907,    // アーカイブ操作NG
    COM_ERR_RING       = 908,    // リングバッファ処理NG
    COM_ERR_CONFIG     = 909,    // コンフィグ処理NG
    COM_ERR_THREADNG   = 915,    // スレッド関連NG
    COM_ERR_MLOCKNG    = 916,    // mutexロックNG
    COM_ERR_MUNLOCKNG  = 917,    // mutexアンロックNG
    /**** システムエラー(デバッグ) ****/
    COM_ERR_DEBUGNG    = 999,    // デバッグ処理
    /**** 最終データ表示用 ****/
    COM_ERR_END        = -1
};

/*
 * 関数の未使用引数の明示マクロ
 *   未使用引数があると警告が出る。-Wno-unused-parameter を付けてのコンパイルは
 *   バグ防止の観点から言って、あまり得策ではない。そこでこのマクロを使って
 *       COM_UNUSED( iId );
 *   というように未使用引数を記述すると、渓谷が出なくなるし、未使用引数である
 *   ことを明示も出来る。これを関数冒頭に記述すると良いだろう。
 *
 *   「それぐらいなら関数引数自体を削除すれば良い」という意見もあるだろうし、
 *   それが可能ならもちろんそれがベストな解決方法。しかし「関数プロトタイプが
 *   指定されていて、必ず引数を入れなければならないけれど、それは使わない…」
 *   という状況は確かに起こり得る(I/Fで登録した関数のコールバック時等)。
 *   そうしたときにこのマクロは使えると思われる。
 */
#define COM_UNUSED( PRM )    (void)(PRM)

/*
 * バッファクリアマクロ
 *   BUFFERに対象バッファの自体を指定することで、その全体をゼロクリアする。
 *   指定できるのは sizeof()で直接サイズを取得できる実態変数のみ。
 *   くれぐれも BUFFERにポインタ変数を使用しないこと。
 */
#define COM_CLEAR_BUF( BUFFER ) \
    do { memset( (BUFFER), 0, sizeof( BUFFER ) ); } while(0)

/*
 * ポインタ設定ありの場合に値設定するマクロ
 *   PTRが NULLでなければ、そのポイント先に VALを設定する。
 */
#define COM_SET_IF_EXIST( PTR, VAL ) \
    do { if( PTR ) { *(PTR) = (VAL); } } while(0)

/*
 * 配列の要素数計算マクロ
 *   ARRAYに指定できるのは実体定義した配列変数のみ。
 *   くれぐれも ARRAYにポインタ変数を指定しないこと。
 */
#define COM_ELMCNT( ARRAY ) \
    (sizeof(ARRAY) / sizeof(*(ARRAY)) )

/*
 * ビットパターン判定マクロ
 *   TARGETに対する BITPATTERNのマスクを実施し、
 *   BITPATTERN値と等しくなるかどうか判定する。
 */
#define COM_CHECKBIT( TARGET, BITPATTERN ) \
    ( ((TARGET) & (BITPATTERN)) == (BITPATTERN) )

/*
 * 3つのデータの受け渡しマクロ
 *   DATA1 <- DATA2 <- DATA3 と順に値を受け渡す。
 */
#define COM_RELAY( DATA1, DATA2, DATA3 ) \
    do { (DATA1) = (DATA2); (DATA2) = (DATA3); } while(0)

/*
 * デバッグ情報用のファイル位置マクロ
 *   COM_FILEPRM は関数定義/プロトタイプ宣言用の仮引数定義マクロ。
 *   COM_FILEVAR は引数定義マクロで受け取った内容を渡すための実引数マクロ。
 *    (重複を避けるため、上記2つについては変数名に大文字を使用している)
 *   COM_FILELOC は現在位置を渡すための実引数マクロ。
 *
 *   なお COM_FILEPRM を引数に持つI/Fの多くはその指定の手間を省くため、
 *   I/F名の定義が関数形式マクロとなっている。これにより見た目の引数は
 *   減らせるが、I/F名と実体関数名が変わるということにもなっている。
 *   関数引数のチェックは実体関数で当然行われるため、関数形式マクロとは
 *   引数の数が違うために混亂する可能性は否定できない(申し訳ない)。
 *   エラー発生時はどの引数と対応するのか、マクロ定義をよく見ておくこと。
 */
#define COM_FILEPRM  const char *iFILE, long iLINE, const char *iFUNC

#define COM_FILEVAR  iFILE, iLINE, iFUNC

#define COM_FILELOC  __FILE__, __LINE__, __func__

/*
 * 条件式の実現性について、コンパイラに示唆するマクロ
 *   if文の条件式 CONDITION がよく真になり得るなら LIKELY(CONDITION) を記述し、
 *   まず起こり得ないなら UNLIKELY(CONDITION) と記述することで、
 *   コンパイル時の処理が最適化される。
 */
#define LIKELY(CONDITION)    __builtin_expect(!!(CONDITION), 1)
#define UNLIKELY(CONDITION)  __builtin_expect(!!(CONDITION), 0)

/*
 * 以後、I/Fのプロトタイプ宣言のコメント記述は
 *
 * I/F名  実際の関数/マクロ名
 *   返り値がある場合は、そのパターン
 * ---------------------------------------------------------------------------
 *   発生し得るエラー
 * ===========================================================================
 *   スレッドセーフかどうかの考察
 * ===========================================================================
 * そのI/Fの詳細記述
 *
 * という形を取る。基本的にI/Fはエラー発生時のエラーコードは返さないので、
 * 処理NGの詳細を確認したい時は、com_getLastError()を使って、一番最後に
 * 発生したエラーコードを取得することが、手がかりとなるだろう。
 *
 * 注意したいのは、返り値が正常な場合でも、エラーは発生し得ること。
 * それも踏まえて検出したい場合は、I/F使用前に com_resetLastError()で、
 * 保持しているエラーコードをリセットし、I/Fから返ってきたら、
 * com_getLastError()の返り値が COM_NO_ERROR以外かどうかを見ることで実現する。
 *
 * エラー一覧のうち "COM_ERR_DEBUGNG: [com_prmNG]" となっているものは、
 * 主にI/F引数をチェックして異常だった場合のエラーで、基本的にバグとみなす。
 * このエラーが出ないように開発を進めなければならない。
 */



/*
 *****************************************************************************
 * COMBASE:モジュール基本I/F (com_proc.c)
 *****************************************************************************
 */

/*
 * 共通処理モジュール初期化  com_initialize()
 * ---------------------------------------------------------------------------
 *   エラー発生時はプログラム強制終了する。
 * ===========================================================================
 *   マルチスレッドで起動することは想定していない。
 * ===========================================================================
 * プログラム起動時に必ず呼ぶこと。
 * iArgc と iArgv は、main()で受け取るオプション情報をそのまま入れる想定。
 * その情報は、以後 com_getCommandLine() でどこからでも取得可能になる。
 * プログラムを作る側がきちんと整理できているのであれば、同じ形式の別データを
 * 渡すようにしても問題ない。
 *
 * 処理の中で 終了処理の atexit()による登録も実施するため、本I/Fを呼べば
 * プログラム終了時に終了処理が動作するようになった。
 *
 * なお特に何か制限がない限り、COM_INITIALIZE() のマクロを使うことを推奨する。
 */
void com_initialize( int iArgc, char **iArgv );

// 初期化関数ダミーマクロ(各ヘッダで実体定義)
// (各機能のヘッダファイルをインクルードすると、その初期化関数で再定義される)
#define COM_INIT_EXTRA            // com_extra.h
#define COM_INIT_SELECT           // com_select.h
#define COM_INIT_WINDOW           // com_window.h
#define COM_INIT_SIGNAL1          // com_signalPrt1.h
#define COM_INIT_SIGNAL2          // com_signalPrt2.h
#define COM_INIT_SIGNAL3          // com_signalPrt3.h

/*
 * toscom初期化マクロ  COM_INITIALIZE()
 *   toscomの初期化を実施する関数形式マクロ。
 *   ARGC と ARGV については com_initialize()と同じものを指定する。
 *   基本的には main()内で使用することを想定する。
 *
 *   あとはマクロを使用しているソースで、オプション機能のヘッダファイルを
 *   インクルードしていれば、自動でその機能の初期化処理を実行する。
 */
#define COM_INITIALIZE( ARGC, ARGV ) \
    do { \
        com_initialize( ARGC, ARGV ); \
        COM_INIT_EXTRA; \
        COM_INIT_SELECT; \
        COM_INIT_WINDOW; \
        COM_INIT_SIGNAL1; \
        COM_INIT_SIGNAL2; \
        COM_INIT_SIGNAL3; \
    } while(0)

/*
 * アプリ名取得  com_getAplName()
 *   makefileで、コンパイル時に APLNAMEでマクロ指定した文字列を返す。
 *   (実行用ファイル名になる想定)
 *   未定義時は空文字""を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで起動することは想定していない。
 * ===========================================================================
 * makefileでコンパイルオプション指定した内容をそのまま返す。
 * オプションの指定がなかったときは、空文字 "" を返す。
 */
const char *com_getAplName( void );

/*
 * バージョン取得  com_getVersion()
 *   makefileで、コンパイル時に VERSIONでマクロ指定した文字列を返す。
 *   未定義時は "0" を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで起動することは想定していない。
 * ===========================================================================
 * makefileでコンパイルオプション指定した内容をそのまま返す。
 * オプションの指定がなかったときは、"0" を返す。
 */
const char *com_getVersion( void );

/*
 * 起動コマンドライン取得  com_getCommandLine()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで起動することは想定していない。
 * ===========================================================================
 * oArgcには int型変数のアドレス、oArgvには char**型変数のアドレスを指定すると
 * com_initialzie()で渡された iArgcと iArgvの内容をそのまま格納して返す。
 *
 * 主に main()で受け取る引数個数(argcと書かれることが多い)と、実際の引数内容
 * (argvと書かれることが多い)を共有する目的で用意したI/Fとなるが、
 * com_initialzie()でも書いた通り、プログラムの書き手の合意があれば、別の
 * データを渡すものとして扱っても構わない。
 *
 * このデータを内部保持するのは、com_initialzie()であるため、これが呼ばれて
 * いない状態で本I/Fを使用した場合、*oArgcは 0、*oArgvは NULL を格納する。
 */
void com_getCommandLine( int *oArgc, char ***oArgv );

/*
 * 起動オプション取得  com_getOption()
 *   解析結果を true/false
 *   何らかの解析NGがあった場合は falseを返す。
 *   !! oResArgc・oRestArgvを指定した場合、そのデータを使用後に *oRestArgvを
 *      com_free()で解放すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iOpts
 *   ・iOptsでオプション名が重複したデータがあった場合も、COM_ERR_DEBUGNG
 *   COM_ERR_PARAMNG: 以下のようなときにエラーを出して、falseを返す。
 *   ・"-"で始まる2文字以上のオプション指定で、引数が必要なオプションがあった
 *   ・"-" "--"で始まる中に、iOptsで未定義のオプションがあった
 *   ・さらに引数を必要とするのにパラメータ数が不足していた
 *   ・必須オプションが含まれていなかった
 * ===========================================================================
 *   マルチスレッドで起動することは想定していない。
 * ===========================================================================
 * "-" または "--" で始まる起動オプションを検索し、処理を切り分ける。
 * 一般的なスタイルとして "-" で始まる場合は1文字のショートオプション、
 * "--" で始まる場合は2文字以上のロングオプションとして解釈している。
 * それ以外の指定については「その他」として除外し、呼び元に委ねる。
 *
 * iArgc・iArgvは main関数で受け取る起動オプション数とそのリストで解析対象。
 *
 * iOptsは指定したいオプション設定の線形データ。最後を COM_OPTLIST_END にする。
 * (より正確には .funcが NULLになったデータが最終要素になっていれば良い)
 * 以下、iOptsのメンバーについての補足説明を続ける。
 *
 *    .shortOpt で指定した場合 "-"+1文字 のオプションであることを期待する。
 *    (データ的には 's' のように '’で囲んだ文字を指定するイメージとなる)
 *    なお1文字オプションは一度に複数指定可能。例えば -ivh と指定した場合は、
 *    -i -v -h と指定したのと同じ動作になる。ただしその中に更に引数を要求する
 *    オプション(.argCntが1以上)が含まれていた場合はエラーとなる。
 *    複数オプションの設定を含む構造体データの場合に .shortOptの内容が同じ
 *    複数のデータが設定されていた場合もエラーとなる。
 *
 *    .longOpt で指定した場合 "--"+2文字以上 のオプションであることを期待する。
 *    (データ的には "statics" のように ""で囲んだ文字列を指定するイメージとなる)
 *    .shortOptと合わせて設定することで、短縮形/省略形とその正式名 というような
 *    想定の設定が可能となる。
 *    複数オプションの設定を含む構造体データの場合に .longOptの内容が同じ
 *    複数のデータが設定されていた場合もエラーとなる。
 *
 *    .argvCnt は、そのオプションのあとに続く引数の個数を示す。
 *    オプションのみで良いなら 0 を指定すること。
 *    COM_OPT_ALL を指定した場合、その時入力された全てのオプションを無条件で
 *    通知するようになる。
 *    この設定が1以上のショートオプションはマルチ指定できなくなる。
 *
 *    .optValue は、オプションが見つかった時に一緒に通知される任意の値である。
 *    どのように用いるのかは使う側に委ねられており、不要なら Don't Careで良い。
 *
 *    .mandatory を true に設定すると必須オプションの指定となる。
 *    そのオプションが指定されていない場合はエラーになる。
 *
 *    .func は そのオプション指定があった時にその処理の為に呼ばれる関数となる。
 *    関数の挙動については、com_getOptFunc_t型の説明を参照。
 *
 * "-" や "--" が付かず、オプション引数でも無いものは、全て「残ったオプション」
 * となり、toscom内部で保持し、*oRestArgcと *oRestArgvにその内容を返送する。
 * この保持は1回分しか行わない。本I/Fを複数回呼んだ場合、前回の保持内容は
 * 解放して上書きされる。
 * どちらも NULLが指定が可能で、その場合 内容返送を行わない。
 *
 * oRestArgc・oRestArgvをどのように使うかは呼び元の判断に委ねられる。
 * 例えば、特定のファイル名と共に起動オプションを適宜指定可能なケースで、
 * 起動オプションは iOptsに従った分析を行うことを想定する。その結果、
 * 起動オプションとは判断されずに残ったもの(oRestArgc・oRestArgvに入るもの)は
 * ファイル名であると認識して処理を開始できる。
 *
 * ioUserDataは該当オプションの処理関数(com_getOptFunc_t型)に、そのまま渡す。
 * 共有したいデータがあれば設定する。無ければNULLで良い。
 * 本I/Fで ioUserDataのポイント先に対して何かすることは一切ない。
 */

/*
 * パラメータ処理関数プロトタイプ宣言
 *   com_getOption()で解析され、該当パラメータであった場合に呼ばれる関数。
 *   情報は com_getOptInf_t型で渡される。
 *
 *   .optValueは該当パラメータの com_getOpt_t型データの .optValueがそのまま
 *   設定される。どのように使うかは呼び元で自由に決めて良い。
 *
 *   また com_getOpt_t型データの .argvCntが 1以上だった場合は、そこから続く
 *   その数の引数を .argc と .argv で参照できるようになる。指定した数以上の
 *   引数参照/変更による動作は保証できなくなるので注意。
 *
 *   com_getOpt_t型データの .argvCnt に COM_OPT_ALL を指定していた場合、
 *   自身も含めて main()で受け取れる argc・argv がそのまま渡される。
 *   ただし、argvは int型ではなく long型で渡されるので注意すること。
 *
 *   .userDataはユーザーが自由に受け渡ししたいデータのアドレスを設定できる。
 *   特に使う必要がない場合は NULLを指定すると良いだろう。
 */

/* 処理関数により渡される情報 */
typedef struct {
    long   optValue;     // 該当するオプションで定義された optValueの内容
    long   argc;         // そのオプションのための引数の数(オプションのみなら 0)
    char** argv;         // そのオプションのための引数の内容
    void*  userData;     // ユーザーデータ
} com_getOptInf_t;

/* 処理関数プロトタイプ */
typedef BOOL(*com_getOptFunc_t)( com_getOptInf_t *iOptInf );

/* パラメータ情報宣言 */
typedef struct {
    long               shortOpt;    // 1文字のオプション名 ( - で指定)
    char*              longOpt;     // 2文字以上のロングオプション名( -- で指定)
    long               argvCnt;     // 続く引数の個数
    long               optValue;    // 処理関数に渡すオプション値
    BOOL               mandatory;   // 必須オプションかどうか
    com_getOptFunc_t   func;        // 処理関数
} com_getOpt_t;

/* com_getOpt_t[]型の最後は必ず以下を指定する */
#define COM_OPTLIST_END   { 0, NULL, 0, 0, false, NULL }

// .argvCntで 全パラ通知を指定したい時に使用
#define  COM_OPT_ALL  INT_MIN

BOOL com_getOption(
        int iArgc, char **iArgv, com_getOpt_t *iOpts,
        long *oRestArgc, char ***oRestArgv, void *ioUserData );

/*
 * プログラムの強制終了  com_exit()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで起動することは想定していない。
 * ===========================================================================
 * iTypeを返り値として、exit()を実行して強制終了する。
 * 基本的にマクロの com_exit()を使用すること。
 *
 * エラーなしの意味で終了させたい場合は、COM_NO_ERRORを仕様することが基本で
 * これが iTypeに指定された場合は強制終了を意味する画面出力は何もせずに、
 * 静かに修了する。
 *
 * iTypeは単純な数値なので、COM_NO_ERROR と同値の EXIT_SUCCESS を使っても
 * 特に問題はない。もしくは本I/Fを使わず exit(EXIT_SUCCESS); を使うことで、
 * 一切何の表示もなしにプログラムを強制終了させることを選択しても良い。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_exit( long iType );
 */
#define com_exit( TYPE ) \
    com_exitFunc( (TYPE), COM_FILELOC )

/* 処理実体関数 */
void com_exitFunc( long iType, COM_FILEPRM );

/*
 * システムコマンド実行  com_system()
 *   system()の返り値をそのまま返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iFormat
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iFormat以降で示した文字列を 標準関数 system()でコマンド実行し、
 * その返り値をそのまま返す。
 *
 * make checkf と打つことで、iFormatとそれ以降の記述の正当性をチェック可能。
 * この詳細は com_printf()の説明記述を参照。
 */
#ifndef CHECK_PRINT_FORMAT
int com_system( const char *iFormat, ... );
#else  // make checkfを打った時に設定されるマクロ
#define com_system  printf
#endif


/*
 *****************************************************************************
 * COMMEM:メモリ関連I/F (com_proc.c)
 *****************************************************************************
 */

/*
 * そういえばメモリ解放のこと
 *   「メモリを捕捉したら必ず解放せよ」というのは実は必ずしも義務ではない。
 *   なぜならプロセス終了時にメモリは自動的に解放されるからである。
 *   しかしながら、ループ内でのメモリ捕捉を開放せずに放置すれば、
 *   そのループ内で際限なくメモリを確保し続けることにもなり、非常に危険。
 *
 *   そして、大小の差はあるにしても、ループを使用する処理を作る機会は多く、
 *   正しい動作を保証するために、メモリ解放を義務付けざるを得なくなる場合が
 *   殆どとなる。このため toscomでも、メモリ捕捉後に解放することとしているし
 *   メモリ監視のデバッグ機能は解放漏れを容赦なく指摘する。
 */

/*
 * メモリ捕捉  com_malloc()
 *   malloc()と同じように捕捉したアドレスを返す。捕捉NGのときは NULLを返す。
 *   捕捉したアドレスは、必ず com_free()で解放すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG:  [com_prmNG] !iSize
 *   COM_ERR_NOMEMORY: メモリ捕捉NG
 *   監視情報登録時の com_addMemInfo()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iSizeでサイズ指定。0を指定した場合はエラーを出して NULLを返す。
 * 呼び元で 0を指定することが無いように配慮すること。サイズが 0の場合の動作は
 * 処理系依存となっており、移植性を考えた時に malloc()任せは危険。
 *
 * iFormat以降はメモリ捕捉する変数名等をデバッグ表示するために使用する。
 * printf()と同じ書式が使える。何も入れない場合は NULLを指定する。
 * メモリ捕捉に失敗したときにエラーメッセージとともに出力する他、
 * デバッグ機能でメモリ捕捉監視している時も、情報の一つとして保持する。
 *
 * 実際のメモリ捕捉には calloc()を使用しており、内容の0クリアを実施する。
 * 大抵問題ないが、捕捉した領域の中でポインタ変数を保持する予定があるなら
 * そのポインタ変数は NULLで改めて初期化することを推奨する。
 *
 * ＊com_debugMemoryErrorOn()を使うことで、わざと結果をNGにすることが可能。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void *com_malloc( size_t iSize, const char *iFormat, ... );
 */
#define com_malloc( SIZE, ... ) \
    com_mallocFunc( (SIZE), COM_FILELOC, __VA_ARGS__ )

/* 処理実体関数 */
void *com_mallocFunc( size_t iSize, COM_FILEPRM, const char *iFormat, ... );

/*
 * メモリ再捕捉  com_realloc()・com_reallocf()
 *   realloc()と同じように捕捉したアドレスを返す。捕捉NGのときは NULLを返す。
 *   再捕捉したアドレスは、必ず com_free()で解放すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_NOMEMORY: メモリ捕捉NG
 *   監視情報登録時の com_addMemInfo()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iAddrはサイズを変更するメモリポインタ、iSizeで変更後のサイズ。
 * iSizeに 0を指定した場合、iAddrのメモリ解放(com_free()を呼ぶ)を実施し、
 * NULLをを返すが、この動作は以下に示す幾つかの問題があるため、最初から iSize=0
 * となるような呼び出しはしないように配慮する方が望ましい。下記を意識できるなら
 * そのまま com_realloc()の処理に任せても問題ない。
 * ・com_free()はマクロ定義により指定したポインタ変数を NULLにできるが
 *   com_realloc()で解放したときは、元アドレス自体を NULLにはできない。
 * ・NULLが返ったら処理NGとすることが出来なくなる。iSize > 0 だった場合は
 *   NULLが返ったら NGだが、iSize = 0 なら NULLが返るのが OKとなるので、
 *   返り値の判定は配慮しなければならない。
 * realloc()でサイズ0が指定された場合の規定が「NULLを返す or free()出来る
 * アドレスを返す」という比較的正反対の結果のどちらかで処理系依存であるため、
 * サイズ0の動作を realloc()に任せるのは移植性の問題で危険、というのが堅牢な
 * コードを書く上での注意事項となっている。
 *
 * iFormat以降はメモリ捕捉する変数名等をデバッグ表示するために使用する。
 * printf()と同じ書式が使える。何も入れない場合は NULLを指定する。
 * メモリ捕捉に失敗したときにエラーメッセージとともに出力する他、
 * デバッグ機能でメモリ捕捉監視している時も、情報の一つとして保持する。
 *
 * メモリ再捕捉後の内容の 0クリアはもちろんしない。
 *
 * もし realloc()に失敗した場合、iAddrで与えられたメモリの解放は行われず
 * そのまま残る点に注意すること。
 * 下記が無難な使用例 (realloc()に失敗しても、addrの内容は変わらない)。
 *
 *     tmp = com_realloc( addr, newSize, NULL );
 *     if( !tmp ) { return false; }
 *     addr = tmp;
 *
 * com_reallocf()は realloc()に失敗したときに、iAddrで与えられたメモリの解放も
 * 併せて実施する。例えば、
 *
 *     addr = com_reallocf( addr, newSize, NULL );
 *
 * というように、第一引数のポインタ変数に結果を再代入することを可能とする。
 * realloc()に失敗して NULLを返す時に addrに残ったメモリを解放するので、
 * 返り値(NULL)が addrに代入されても矛盾が発生しない。
 *
 * しかし com_reallocf()使用時は、そのまま解放するのが問題ないか吟味すること。
 * ・addrが構造体のポインタで、その構造体が com_malloc()を使って
 *   メモリ捕捉したアドレスをメンバとして持つ場合、その解放が出来なくなるため、
 *   addrをいきなり解放すると、メモリ浮きを確実に発生させる。
 * ・addrがチェーン構造のデータだった場合、その解放をするには前後のデータを
 *   つなぎかえる必要があるはずで、単純な解放だけだとデータ構造を破壊する。
 * など、安易に使えない状況は決して珍しくはない。
 *
 * ＊com_debugMemoryErrorOn()を使うことで、わざと結果をNGにすることが可能。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void *com_realloc( void *iAddr, size_t iSize, const char *iFormat, ... );
 *   void *com_reallofc( void *iAddr, size_t iSize, const char *iFormat, ... );
 */
#define com_realloc( ADDR, SIZE, ... ) \
    com_reallocFunc( (ADDR), (SIZE), COM_FILELOC, __VA_ARGS__ )

#define com_reallocf( ADDR, SIZE, ... ) \
    com_reallocfFunc( (ADDR), (SIZE), COM_FILELOC, __VA_ARGS__ )

/* 処理実体関数 */
void *com_reallocFunc(
        void *iAddr, size_t iSize, COM_FILEPRM, const char *iFormat, ... );

void *com_reallocfFunc(
        void *iAddr, size_t iSize, COM_FILEPRM, const char *iFormat, ... );

/*
 * メモリ再捕捉(テーブル拡縮)  com_realloct()
 *   処理結果を true/false で返す。
 *   テーブル拡縮に成功したら true を返し、**ioAddr を再捕捉したアドレスにする。
 *   再捕捉したアドレスは、必ず com_free()で解放すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_NOMEMORY: メモリ捕捉NG
 *   監視情報登録時の com_addMemInfo()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 一定サイズの要素からなる線形データのサイズを変更するようなメモリの再捕捉を
 * 実施する。
 *
 * ioAddr には対象となるテーブルの先頭アドレスに & を付けて指定する。
 * つまり最低でもダブルポインタ型を要求するが、void *型はどんなものでも
 * 受け付けるため、ioAddrは void *型になっている。
 * これは文法ミスではないし、void *型の入力を求めているわけでもない。
 *
 * iUnit はそのテーブルの1つ1つの要素サイズを指定する。
 *
 * ioCount はそのテーブルの要素数の格納先アドレスを指定する。
 * テーブル拡縮が正しく行われれば、拡縮後の新たな値に更新する。
 *
 * iResize は要素数変分を指定する。負数を指定して縮小することも可能。
 * テーブルサイズを拡大する(iResize > 0)場合、その拡張に成功して新たに
 * 追加されたエリアの内容を 0クリアする。
 * iResizeが 0 の場合、何もせずに trueを返す。
 *
 * 最終的な個数が 0 になる場合( *ioCount + iResize が 0 になる場合)、
 * *ioAddr に対する com_free()を実施する。従って *ioAddrは NULLになって返る。
 *
 * *iFormat以降はメモリ捕捉する変数名などをデバッグ表示するために使用する。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_realloct( void *ioAddr, size_t iUnit, long *iCount,
 *                      long iResize, const char *iFormat, ... );
 */
#define com_realloct( ADDR, UNIT, COUNT, RESIZE, ... ) \
    com_realloctFunc( (ADDR), (UNIT), (COUNT), (RESIZE), \
                      COM_FILELOC, __VA_ARGS__ )

/* 処理実体関数 */
BOOL com_realloctFunc(
        void *ioAddr, size_t iUnit, long *ioCount, long iResize,
        COM_FILEPRM, const char *iFormat, ... );

/*
 * メモリ再捕捉(位置指定/アドレス返却)  com_reallocAddr()
 *   再捕捉後、追加の場合は追加したデータの先頭アドレスを返す。
 *   削除の場合は一番最後のデータのアドレスを返す。
 *   再捕捉に失敗した場合は NULLを返す。
 *   再捕捉したアドレスは、必ず com_free()で解放すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iPos >= *ioCount
 *   メモリ再捕捉のための com_realloct()によるエラー
 *   監視情報登録時の com_addMemInfo()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * com_realloct()の機能限定版。テーブルのサイズ拡縮後、該当データのアドレスを
 * 返す。iPosは他の com_realloc系I/Fには無い引数なので注意すること。それ以外の
 * 引数は com_realloct()と同じものなので、そちらの説明を参照。実際のテーブル
 * 拡縮処理もこのI/Fを呼んで実施する。
 *
 * iPosは追加/削除を開始するテーブル位置を指定する。COM_TABLEEND もしくは
 * 0～(*ioCount - 1)の範囲でなければならない。範囲外はエラーとなる。
 * (位置指定にこだわりが無いなら、COM_TABLEENDを一貫して使うことで問題ない)
 *
 * テーブル拡張の場合、iPosの位置にデータを追加するようにスペースを空け、
 * 内容はオール0で初期化する。例えば iPos に 0 が指定されたテーブル拡張なら、
 * 新規スペースがテーブルの先頭に作られ、元のデータは順送りにされる。
 * テーブル末尾にデータを追加したいだけの場合は COM_TABLEENDを指定すれば良い。
 *
 * テーブル縮小の場合、iPosが削除されるデータの先頭となる。iPosからの縮小数が
 * 実際のテーブルの個数を超えた場合、テーブル最後までの縮小に補正する。
 * COM_TABLEENDが指定された場合は、テーブル末尾から指定した数(iResize)のデータを
 * 削除するようにテーブルのサイズを縮小する。
 *
 * 最終的に iPosで指定した位置のデータのアドレスを返す。ただしテーブル縮小して
 * iPosのデータがもう無い場合は、テーブル末尾のデータアドレスを返す。
 * iPosに COM_TABLEENDを指定していた場合は、テーブル拡張なら 追加されたデータの
 * 先頭アドレス、テーブル縮小およびサイズ変更なしなら、テーブル末尾のデータの
 * アドレスを返す。
 */

#define  COM_TABLEEND  -1   // テーブル末尾を変動させるための iPos指定値

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void *com_reallocAddr(
 *       void *iAddr, size_t iUnit, long iPos, long *ioCount, long iResize,
 *       const char *iFormat, ... );
 */
#define com_reallocAddr( ADDR, UNIT, POS, COUNT, RESIZE, ... ) \
    com_reallocAddrFunc( (ADDR), (UNIT), (POS), (COUNT), (RESIZE), \
                         COM_FILELOC, __VA_ARGS__ )

/* 処理実体関数 */
void *com_reallocAddrFunc(
        void *iAddr, size_t iUnit, long iPos, long *ioCount, long iReszize,
        COM_FILEPRM, const char *iFormat, ... );

/*
 * 文字列複製  com_strdup()・com_strndup()
 *   strdup()と同じように捕捉したアドレスを返す。処理NGのときは NULLを返す。
 *   捕捉したアドレスは、必ず com_free()で解放すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iString
 *   COM_ERR_NOMEMORY: メモリ捕捉NG
 *   監視情報登録時の com_addMemInfo()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iStringは複製対象文字列。
 * com_strndup()の場合、何文字複製するかを iSizeで指定する。iStringの実際の
 * 文字列長より大きかった場合、実際の文字列長サイズのメモリ捕捉になる。
 *
 * iFormat以降はメモリ捕捉する変数名等をデバッグ表示するために使用する。
 * printf()と同じ書式が使える。何も入れない場合は NULLを指定する。
 * メモリ捕捉に失敗したときにエラーメッセージとともに出力する他、
 * デバッグ機能でメモリ捕捉監視している時も、情報の一つとして保持する。
 *
 * 注意：strdup()は C99の標準にはない GNU拡張関数となる。
 *       このため本I/Fも strdup()は直接使わず、calloc() -> strcpy() という
 *       一般的な手順に差し替えているが、I/Fの呼び手が特に気にする必要はない。
 *
 * ＊com_debugMemoryErrorOn()を使うことで、わざと結果をNGにすることが可能。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   char *com_strdup( const char *iString, const char *iFormat, ... );
 *   char *com_strndup( const char *iString, size_t iSize,
 *                      const char *iFormat, ... );
 */
#define com_strdup( STRING, ... ) \
    com_strdupFunc( (STRING), COM_FILELOC, __VA_ARGS__ )

#define com_strndup( STRING, SIZE, ... ) \
    com_strndupFunc( (STRING), (SIZE), COM_FILELOC, __VA_ARGS__ )

/* 処理実体関数 */
char *com_strdupFunc(
        const char *iString, COM_FILEPRM,
        const char *iFormat, ... );

char *com_strndupFunc(
        const char *iString, size_t iSize, COM_FILEPRM,
        const char *iFormat, ... );

/*
 * メモリ解放  com_free()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !(&ioAddr)
 *   監視情報削除時の com_delMemInfo()によるエラー(COM_DOUBLEFREE)
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * free()と同じ要領で使う。つまり ioAddrはアドレスを保持したポインタ変数を想定。
 * メモリ解放後、指定した変数には NULLを代入する。
 * ioAddrが元々NULLの場合は何もしない。
 * 特殊な事情がない限り、com_freeFunc()ではなく、com_free()を使用すること。
 *
 * com_free()は関数マクロで、実体が com_freeFunc()になる。マクロ定義で
 * com_free()の引数は & を付けたポインタとして com_freeFunc()に渡しており、
 * これによりメモリ解放とともに元のアドレスのポインタへの NULL格納を実現する。
 *
 * この事情により、メモリ解放は出来ても NULLにならないという事象が起こり得る。
 * 例えば以下のようなコードでそれは起きる。
 *
 *     void sample( void ) {
 *         char* name = com_malloc( 16, NULL );
 *         ～ 中略 ～
 *         freeResource( name );
 *     }
 *
 *     void freeResource( char *ioName ) {
 *         com_free( ioName );
 *     }
 *
 * 上記だと freeResource()により sample()で確保された nameのメモリは解放するが
 * NULLが入るのは飽くまで freeResource()の自動変数 ioNameであり、sample()の
 * name は何も値が変わらない(メモリ解放という最大の仕事は果たすが)。
 * sample()の nameにも NULLが入るようにしたければ、
 *
 *     void sample( void ) {
 *         char* name = com_malloc( 16, NULL );
 *         ～ 中略 ～
 *         freeResource( &name );
 *     }
 *
 *     void freeResource( char **ioName ) {
 *         com_free( *ioName );
 *     }
 *     
 * というように、com_freeFunc()に渡して通用するアドレスにする必要がある。
 * こうすれば com_freeFunc()には &(*ioName)・・つまり ioNameが渡され、
 * 正しく sample()の nameが指すメモリを解放するとともに name に NULLが入る。
 *
 * メモリ監視のデバッグ機能が動作している場合、解放しようとしたメモリアドレスが
 * 監視情報に入っていない場合は COM_DOUBLEFREEのエラーを出す。
 * このエラーが出る原因は2つ考えられ、その判別ができない。
 *  (1)本当に二重解放している場合
 *     --> 早急に対処が必要 (二重解放でプログラムが落ちる)
 *  (2)デバッグ情報が保持できる限界を超えたため、デバッグ情報が足りない場合
 *     --> どうしようもない (この場合 プログラムは落ちないことも)
 * もしも(2)のケースの場合、デバッグ情報のメモリ捕捉失敗の COM_ERR_DEBUGNGの
 * エラーメッセージがメモリ捕捉時に先に出ている。
 * そこに捕捉アドレスも出ているため、そのアドレスと解放しようとしていた
 * アドレスが一致するならば、(2)のケースである。
 *
 * (1)のケースは、メモリ監視動作中であれば落とさないようにすることも可能だが、
 * (2)のケースとの区別をつけるのが難しいため、現状は free()を実施しようとして
 * プログラムが確実に落ちる。そうならないようにコードを作るべきだろう。
 */
                      
/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_free( void *ioAddr);
 */
#define com_free( ADDR ) \
    com_freeFunc( &(ADDR), COM_FILELOC )

/* 処理実体関数 */
void com_freeFunc( void *ioAddr, COM_FILEPRM );



/*
 *****************************************************************************
 * COMBUF:文字列バッファ関連I/F (com_proc.c)
 *****************************************************************************
 */

/*
 * 文字列バッファとは、文字列を保持するためのバッファで、実際の文字(data)と
 * そのバッファサイズ(size)を組にして保持するデータの形式。
 * 保持しようとした文字列長が現在のバッファサイズより大きい場合、
 * その長さを保持できるようにバッファのサイズを変更する。
 * 小さい場合に縮小はしないので、バッファサイズ＝文字列長とは限らない点に注意。
 * 文字列長は素直に strlen()で取得すること。
 */

/* 文字列バッファデータ型 */
typedef struct {
    char*    data;     // 実際のデータ
    size_t   size;     // バッファサイズ(データサイズとは限らない)
} com_buf_t;

/*
 * 文字列バッファI/Fを使うパターンについて
 *  文字列バッファを実体定義するかどうかで、使用するI/Fが変わってくる。
 *
 *                      実体定義時  ポインタ定義時
 *  com_createBuffer()     不要       最初に必要
 *  com_initBuffer()    最初に必要       不要 (com_createBuffer()で実施する為)
 *  com_setBuffer()      使用可能      使用可能
 *  com_setBufferSize()  使用可能      使用可能
 *  com_addBuffer()      使用可能      使用可能
 *  com_addBufferSize()  使用可能      使用可能
 *  com_copyBuffer()     使用可能      使用可能
 *  com_clearBuffer()    使用可能      使用可能
 *  com_resetBuffer()   最後に必要       不要 (com_freeBuffer()で実施する為)
 *  com_freeBuffer()       不要       最後に必要
 */

/*
 * 文字列バッファ生成  com_createBuffer()
 *   処理成否を true/false で返す。
 *   falseを返した場合 *oBufの内容は無効。
 *   生成したバッファは、必ず com_freeBuffer()で解放すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oBuf
 *   文字列バッファ確保のための com_malloc()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 文字列バッファのポインタ変数に対して、バッファ生成と初期化を行う。
 * oBufは com_buf_t*型で定義したポインタ変数のアドレスを設定し、それに対して
 * 文字列バッファを生成する。
 *
 * iSizeは最初のバッファサイズを示すが、0でも問題ない。
 * 0 を指定した場合、バッファ自体もメモリ解放し、*oBufには NULLを格納する。
 *
 * iFormat以降で初期化文字列を指定し、格納失敗したときは falseを返す。
 * 最初から文字列を保持する必要がない場合、iFormatには NULLを指定する。
 *   ＊初期化には com_initBuffer()を内部使用し、その結果を返す。
 *
 * 以後、バッファに文字列を格納するときは
 *   com_setBuffer()・com_setBufferSize()・com_addBuffer()・com_addBufferSize()
 * を使用する。他にも幾つか使用できるI/Fはあるので、これ以降を参照。
 *
 * そして使用し終えた文字列バッファは com_freeBuffer()で解放すること。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_createrBuffer( com_buf_t **oBuf, size_t iSize,
 *                           const char *iFormat, ... );
 */
#define com_createBuffer( BUF, SIZE, ... ) \
    com_createBufferFunc( (BUF), (SIZE), COM_FILELOC, __VA_ARGS__ )

/* 処理実体関数 */
BOOL com_createBufferFunc(
        com_buf_t **oBuf, size_t iSize, COM_FILEPRM, const char *iFormat, ... );

/*
 * 文字列バッファ初期化  com_initBuffer()
 *   処理成否を true/false で返す。
 *   falseを返しても、文字列バッファ自体を解放はしない。
 *   最後に必ず com_resetBuffer()で文字列確保を解放すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oBuf
 *   文字列バッファ確保のための com_malloc()によるエラー
 *   初期値設定のための com_reallocf()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 文字列バッファを実体定義した時に、その初期化に用いる。
 * メモリ確保できなかった場合は falseを返す。
 *
 * oBuf にはその文字列バッファ変数のアドレスを設定する。
 *
 * iSizeは最初のバッファサイズを示すが、0でも問題ない。
 *
 * iFormat以降で初期化文字列を指定し、格納失敗したときは falseを返す。
 * 特に最初から文字列を格納する必要がない場合は、iFormatに NULLを指定する。
 *   ＊実際の設定には com_setBuffer()を内部使用し、その結果を返す。
 *
 * com_createBuffer()を使用している場合、バッファ初期化も併せて行うため、
 * 本I/Fで改めて初期化をする必要は無い。
 *
 * 文字列格納済みの文字列バッファに対しては使用しないこと。
 * 既に保持していた文字列のメモリが浮いてしまう。
 * (文字列バッファを初期化無しで実体定義していた場合、その内容は不定のため
 *  oBuf->data != NULL や oBuf->size > 0 で、格納済みかチェックするのは
 *  危険であると判断した)
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_initBuffer( com_buf_t *oBuf, size_t iSize,
 *                        const char *iFormat, ... );
 */
#define com_initBuffer( BUF, SIZE, ... ) \
    com_initBufferFunc( (BUF), (SIZE), COM_FILELOC, __VA_ARGS__ )

/* 処理実体関数 */
BOOL com_initBufferFunc(
        com_buf_t *oBuf, size_t iSize, COM_FILEPRM, const char *iFormat, ... );

/* 処理関数コード (以降で内部使用する。使う側は意識する必要なし) */
typedef enum {
    COM_SET = 0,        // com_setBuffer()
    COM_SETSIZE,        // com_setBufferSize()
    COM_ADD,            // com_addBuffer()
    COM_ADDSIZE,        // com_addBufferSize()
    COM_COPYBUFF,       // com_copyBuffer()
    COM_CREBUFF,        // com_createBuffer()
    COM_INITBUFF        // com_initBuffer()
} COM_SETBUF_t;

/*
 * 文字列バッファ格納  com_setBuffer()・com_setBufferSize()
 *   処理成否を true/false で返す。
 *   falseを返してもバッファ自体を解放はしない。
 *   使用後 com_resetBuffer() または com_freeBuffer()でデータを解放すること。
 *   (文字列バッファを実体定義しているなら com_resetBuffer()を使う、
 *    そうでないなら com_freeBuffer()で文字列バッファ自体とともに解放する)
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oBuf || !iFormat
 *     (com_setBufferSize()のみ)  iSize > COM_DATABUF_SIZE - 1
 *   バッファ拡張のための com_reallocf()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 文字列バッファ oBuf に iFormat以降で指定した文字列を格納する。
 * 対象となる文字列バッファは com_createBuffer()による生成、または
 * com_initBuffer()にょる初期化が済んだものでなければならない。
 *
 * 文字列長がバッファサイズより長いときは、その分 com_reallocf()で拡張する。
 * 文字列長がバッファサイズ以下のときは、バッファをそのまま使用する。
 * つまり格納している文字列長==バッファサイズとは限らないので注意すること。
 *
 * com_setBufferSize()は iFormat以降で指定した文字列の先頭から
 * iSizeの文字数までを指定したバッファにデータとして格納する。
 * iSizeが内部一時バッファのサイズ COM_DATABUF_SIZE - 1 を超えると処理NGになる。
 *
 * この内部バッファは文字列バッファ処理全体での共通バッファで、使用時に
 * mutexロックを掛けて排他処理を実施する。
 *
 * 文字列バッファを実体定義して、データを格納した場合、
 * 必ず com_resetBuffer()でそのデータ部分を解放すること。
 * com_createBuffer()を使ってアドレスで管理している場合は、
 * com_freeBuffer()でデータ部分もメモリ解放を実施する。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_setBuffer( com_buf_t *oBuf, const char *iFormat, ... );
 *
 *   BOOL com_setBufferSize( com_buf_t *oBuf, size_t iSize,
 *                           const char *iFormat, ... );
 */
#define com_setBuffer( BUF, ... ) \
    com_setBufferFunc( (BUF), 0, COM_SET, COM_FILELOC, __VA_ARGS__ )

#define com_setBufferSize( BUF, SIZE, ... ) \
    com_setBufferFunc( (BUF), (SIZE), COM_SETSIZE, COM_FILELOC, __VA_ARGS__ );

/* 処理実体関数 */
BOOL com_setBufferFunc(
        com_buf_t *oBuf, size_t iSize, COM_SETBUF_t iType, COM_FILEPRM,
        const char *iFormat, ... );

/*
 * 文字列バッファ追加  com_addBuffer()・com_addBufferSize()
 *   処理成否を true/false で返す。
 *   falseを返してもバッファ自体を解放はしない。
 *   使用後 com_setBuffer()と同様のデータ開放が必要になる。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oBuf || !iFormat || !(oBuf->data)
 *     (com_addBufferSize()のみ)  iSize > COM_DATABUF_SIZE - 1
 *   文字列設定のための com_setBuffer()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 文字列バッファ oBuf が保持する文字列の末尾に iFormat以降で指定した文字列を
 * 連結する。
 *
 * com_addBufferSize()は iFormat以降で示した文字列の先頭から
 * iSizeの文字数までを指定したバッファに連結する。
 * iSizeが内部一時バッファのサイズ COM_DATABUF_SIZE - 1 を超えると処理NGになる。
 * iSizeが 0指定の場合は、指定された文字列全体を結合する。
 *
 * バッファへの設定可追加家の違いだけで、データの扱い方については
 * com_setBuffer()と同様なので、そちらの説明も参照して欲しい。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_addBuffer( com_buf_t *oBuf, const char *iFormat, ... );
 *
 *   BOOL com_addBufferSize( com_buf_t *oBuf, size_t iSize,
 *                           const char *iFormat, ... );
 */
#define com_addBuffer( BUF, ... ) \
    com_addBufferFunc( (BUF), 0, COM_ADD, COM_FILELOC, __VA_ARGS__ )

#define com_addBufferSize( BUF, SIZE, ... ) \
    com_addBufferFunc( (BUF), (SIZE), COM_ADDSIZE, COM_FILELOC, __VA_ARGS__ )

/* 処理実体関数 */
BOOL com_addBufferFunc(
        com_buf_t *oBuf, size_t iSize, COM_SETBUF_t iType, COM_FILEPRM,
        const char *iFormat, ... );

/*
 * 文字列バッファコピー  com_copyBuffer()
 *   処理成否を true/false で返す。
 *   使用後 com_setBuffer()と同様のデータ開放が必要になる。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oTarget || !iSource
 *   文字列コピーのための com_setBuffer()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 文字列バッファ iSourceの内容を 文字列バッファ oTargetにコピーする。
 * oTargetは com_createBuffer() または com_initBuffer()で初期化しておくこと。
 * oTargetのバッファサイズが、コピーに十分なサイズの場合 変更はしない。
 * (つまり oTarget->size と iSource->sizeが同じになるとは限らない)
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_copyBuffer( com_buf_t *oTarget, const com_buf_t *iSource );
 */
#define com_copyBuffer( TARGET, SOURCE ) \
    com_copyBufferFunc( (TARGET), (SOURCE), COM_FILELOC )

/* 処理実体関数 */
BOOL com_copyBufferFunc(
        com_buf_t *oTarget, const com_buf_t *iSource, COM_FILEPRM );

/*
 * 文字列バッファクリア  com_clearBuffer()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oBuf
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 *   ただし同じ文字列バッファへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、文字列バッファを使う側が排他処理を検討すること。
 * ===========================================================================
 * 内容を 0クリアする。あくまでクリアのみで、メモリ解放はしない。
 * (従って oBuf->size の内容は不変)
 * 本I/Fは実体定義・ポインタ定義、どちらの文字列バッファに対しても使用可能。
 * メモリ解放もしたい場合は com_resetBuffer()を使用する。
 */
void com_clearBuffer( com_buf_t *oBuf );

/*
 * 文字列バッファリセット  com_resetBuffer()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oBuf
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 *   ただし同じ文字列バッファへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、文字列バッファを使う側が排他処理を検討すること。
 * ===========================================================================
 * 文字列保持のためのメモリを解放して初期化する。
 * つまり com_initBuffer()を初期化文字列無しで実施した状態となる。
 * 本I/Fは実体定義・ポインタ定義、どちらの文字列バッファに対しても使用可能で
 * 実体定義の場合の最終的なメモリ解放を実施するI/Fとなる。
 * メモリ解放までしたくない場合は com_clearBuffer() を使用する。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_resetBuffer( com_buf_t *oBuf );
 */
#define com_resetBuffer( BUF ) \
    com_resetBufferFunc( (BUF), COM_FILELOC )

/* 処理実体関数 */
void com_resetBufferFunc( com_buf_t *oBuf, COM_FILEPRM );

/*
 * 文字列バッファ解放  com_freeBuffer()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oBuf
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 *   ただし同じ文字列バッファへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、文字列バッファを使う側が排他処理を検討すること。
 * ===========================================================================
 * com_createBuffer()で生成した文字列バッファをメモリ解放する。
 * 文字列保持のためのバッファももちろん解放する。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_freeBuffer( com_buf_t **oBuf );
 */
#define com_freeBuffer( BUF ) \
    com_freeBufferFunc( (BUF), COM_FILELOC )

/* 処理実体関数 */
void com_freeBufferFunc( com_buf_t **oBuf, COM_FILEPRM );



/*
 *****************************************************************************
 * COMCHAIN:文字列チェーンデータ関連I/F (com_proc.c)
 *****************************************************************************
 */

/*
 * 文字列チェーンデータとは、複数の文字列をチェーン上に保持する形式である。
 * 単純化のため順方向の次データのみでつながるため、逆方向に戻るのは難しい。
 *
 * 簡単なスタックデータの形式でもあることから、スタックとして使う為のI/Fも
 * 用意している。また数値データを文字列変換して保持することにも対応する。
 */

/* 文字列チェーンデータ形式 */
typedef struct com_strChain {
    char *data;                    // 実際の文字列データ
    struct com_strChain *next;     // 次データポインタ
} com_strChain_t;

/*
 * 文字列チェーンデータを使う時は、com_strChain_t型のポインタ変数を用意し、
 * NULLで初期化してから、com_addChainData()/com_sortAddChainData() 等で
 * データを追加する。一度でも追加したら、com_freeChainData()で解放が必要。
 * ただし、com_deleteChainData()や com_popChainData()を使った結果、
 * 全て削除できていた場合は、com_freeChainData()は不要(使っても問題ない)。
 */

/*
 * 文字列チェーンデータ登録  com_addChainData()
 *   処理成否を true/false で返す。
 *   必ず最後に、com_freeChainData()で解放すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioChain || !iFormat
 *   データ追加のための com_malloc()・com_strdup()によるエラー
 * ===========================================================================
 *   mutexによる排他制御が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 文字列チェーンデータ *ioChainの末尾に iFormat以降で指定した文字列をデータと
 * して追加する。
 *
 * ioChainは com_strChain_t*型で定義して、NULL初期化し、&を付けてそのアドレス
 * を第１引数として渡す想定。以後更にデータ操作する時は、同じようにアドレスを
 * 渡す。最初に NULL初期化されていない場合の動作は未保証。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_addChainData( com_strChain_t **ioChain,
 *                          const char *iFormat, ... );
 */
#define com_addChainData( CHAIN, ... ) \
    com_addChainDataFunc( (CHAIN), COM_FILELOC, __VA_ARGS__ )

/* 処理実体関数 */
BOOL com_addChainDataFunc(
        com_strChain_t **ioChain, COM_FILEPRM, const char *iFormat, ... );

/*
 * 文字列チェーンデータ登録(ソートあり)  com_sortAddChainData()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioChain || !iFormat ||
 *                (iSort != COM_SORT_ASCEND && iSort != COM_SORT_DESCEND)
 *   データ追加のための com_malloc()・com_strdup()によるエラー
 * ===========================================================================
 *   mutexによる排他制御が動作するためスレッドセーフとなる。
 * ===========================================================================
 * com_addChainData()のソート機能版。ASCIIで降順か昇順かを iSortで指定し、
 * 文字列的にその順で並ぶように文字列チェーンデータ *ioChainに iFormat以降で
 * 指定した文字列をデータとして追加する。
 *
 * iSortが引数として追加されている以外の使い方は com_addChainData()と同様。
 * そちらの説明も確認すること。
 */

/* ソート順指定用 */
typedef enum {
    COM_SORT_ASCEND,      // 昇順ソート指定
    COM_SORT_DESCEND      // 降順ソート指定
} COM_ADDCHAIN_t;

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_sortAddChainData( com_strChain_t **ioChain, COM_ADDCHAIN_t iSort,
 *                              const char *iFormat, ... );
 */
#define com_sortAddChainData( CHAIN, SORT, ... ) \
    com_sortAddChainDataFunc( (CHAIN), (SORT), COM_FILELOC, __VA_ARGS__ )

/* 処理実体関数 */
BOOL com_sortAddChainDataFunc(
        com_strChain_t **ioData, COM_ADDCHAIN_t iSort, COM_FILEPRM,
        const char *iFormat, ... );

/*
 * 文字列チェーンデータ検索  com_searchChainData()
 *   見つかったデータアドレスを返す。見つからなかったら NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iFormat
 * ===========================================================================
 *   mutexによる排他制御が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 文字列チェーンデータ iChain から、iFormat以降で指定した文字列を検索する。
 * iChainが NULLの場合、NULLを返すのみとなる。
 * iChainは他の文字列チェーンデータ関連I/Fと違い com_strChain_t*型であることに
 * 注意すること。
 *
 * 文字列チェーンデータの中に指定したキー文字列が複数含まれている場合でも
 * 一番最初に見つかったデータのアドレスのみを返す。返ってきたデータの ->nextを
 * 本I/Fに再度渡し、NULLが返るまで処理を続ける、という手法を取ることで、
 * 複数の同一キーデータを全て検索しきることが可能。
 *
 * 検索したデータに対する操作に敢えて規制を加えない目的で、返り値に const は
 * 付加しない。ただ、文字列は 文字列長 + 1 のメモリ捕捉ををして保持するため、
 * 文字列長が変動する(特に大きくなる)ような操作は、文字列データメモリの再捕捉
 * を要することに注意すること。
 * こうした変更はあまり推奨はしないが必要であれば躊躇しなくても良いが、
 * ソートしていた場合、安易な変更はソート状態を崩すことにもなるので注意。
 */
com_strChain_t *com_searchChainData(
        com_strChain_t *iChain, const char *iFormat, ... );

/*
 * 文字列チェーンデータ削除  com_deleteChainData()
 *   削除したデータ数を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioChain || !iFormat
 *   データ削除のための com_free()によるエラー
 * ===========================================================================
 *   mutexによる排他制御が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 文字列チェーンデータ *ioChainの中に iFormat以降で指定した文字列が存在したら、
 * そのデータを削除する。
 *
 * iContinueが trueの場合、複数あれば全て削除する。
 * falseの場合、最初に見つかったデータのみを削除する。複数ある中の最初ではない
 * 特定のデータを削除することを望む場合、自分でチェーンデータを直接操作する
 * コードを書くこと。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   long com_deleteChainData( com_strChain_t **ioChain, BOOL iContinue,
 *                             const char *iFormat, ... );
 */
#define com_deleteChainData( CHAIN, CONTINUE, ... ) \
    com_deleteChainDataFunc( (CHAIN), (CONTINUE), COM_FILELOC, __VA_ARGS__ )

/* 処理実体関数 */
long com_deleteChainDataFunc(
        com_strChain_t **ioChain, BOOL iContinue, COM_FILEPRM,
        const char *iFormat, ... );

/*
 * 文字列チェーンデータプッシュ(スタック疑似)  com_pushChainData()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioChain || !iFormat
 *   データ追加のための com_malloc()・com_strdup() によるエラー
 * ===========================================================================
 *   mutexによる排他制御が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 文字列チェーンデータ *ioChainの先頭に iFormat以降で指定した文字列を追加する。
 * 既にスタックされていたデータは順送りしてチェーン構造を維持する。
 * com_addChainData()は末尾に追加なので、処理が違う点に注意。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_pushChainData( com_strChain_t **ioChain,
 *                           const char *iFormat, ... );
 */
#define com_pushChainData( CHAIN, ... ) \
    com_pushChainDataFunc( (CHAIN), COM_FILELOC, __VA_ARGS__ )

/* 処理実体関数 */
BOOL com_pushChainDataFunc(
        com_strChain_t **ioChain, COM_FILEPRM, const char *iFormat, ... );

/*
 * 文字列チェーンデータポップ (スタック疑似)  com_popChainData()
 *   文字列チェーンデータの先頭データの文字列を返す。
 *   データ無しのときと、ポップ用のメモリが確保できなかった時は NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioChain || !iFormat
 *   ポップ用バッファ格納のための com_setBuffer() によるエラー
 *   データ削除のための com_free() によるエラー
 * ===========================================================================
 *   mutexによる排他制御が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 文字列チェーンデータ *ioChainの先頭データを取り出して渡し、
 * その先頭データは削除する。このため *ioChainが NULLになっていれば、
 * 全てのスタックを吐き出したということになる。
 *
 * 渡されたアドレスの解放は不要だが、共用バッファを使用するため、
 * 保持する必要がある場合は、速やかに別バッファに保持すること。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   char *com_popChainData( com_strChain_t **ioChain );
 */
#define com_popChainData( CHAIN ) \
    com_popChainDataFunc( (CHAIN), COM_FILELOC )

/* 処理実体関数 */
char *com_popChainDataFunc( com_strChain_t **ioData, COM_FILEPRM );

/*
 * 文字列チェーンデータ展開  com_spreadChainData()
 *   生成した文字列長を返す。
 *   iSizeを超える文字列になった場合、COM_SIZE_OVER を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG]
 *                    !oResult || iSize < 2 || iSize >= COM_SIZE_OVER
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 *   ただし同じチェーンデータへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、チェーンデータを使う側が排他処理を検討すること。
 * ===========================================================================
 * 文字列チェーンデータ ioChainに含まれる全ての文字列を、空白1つを区切りにして
 * 順番に並べた文字列を *oResultに格納する。
 * 他の文字列チェーンデータ関連I/Fと異なり、第1引数(iChain)が com_strChani_t*型
 * であることに注意すること(これは誤りではない)。
 *
 * *oResultのサイズを iSizeに設定すること。
 * iSizeは 2以上 COM_SIZE_OVER未満の値にする必要があり、その範囲外ならNGになる。
 *
 * 結果が iSizeを超えた場合。そこまでを連結し、COM_SIZE_OVER を返す。
 * より正確に言うと、文字列終端の \0 の分が必要なため、文字が実際に入る最大の
 * サイズは iSize - 1 になる。
 */

#define COM_SIZE_OVER  ULONG_MAX  // iSizeにこの値を指定するとNGになる

size_t com_spreadChainData(
        const com_strChain_t *iChain, char *oResult, size_t iSize );

/*
 * 文字列チェーンデータ解放  com_freeChaniData()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioChain
 *   データ削除のための com_free() によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 *   ただし同じチェーンデータへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、チェーンデータを使う側が排他処理を検討すること。
 * ===========================================================================
 * 文字列チェーンデータ *ioChain に含まれる全データを解放し、
 * 最後に *ioChain を NULLにして返す。
 */

/*
 * プロトタイプ形式 (この値を使用すること)
 *   void com_freeChainData( com_strChain_t **ioChain );
 */
#define com_freeChainData( CHAIN ) \
    com_freeChainDataFunc( (CHAIN), COM_FILELOC )

/* 処理実体関数 */
void com_freeChainDataFunc( com_strChain_t **ioChain, COM_FILEPRM );

/*
 * 文字列チェーンデータを使用した数値チェーン操作I/F
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iKeyが実体変数のため !iKey の判定は無い。
 *   上記以外は 元I/Fと童謡のエラーが発生し得る。
 * ===========================================================================
 *   マルチスレッド対応状況は元I/Fと同様となる。
 * ===========================================================================
 * I/F名は Data を Num に変えただけで、使い方は元I/Fと同じとなる。
 * 扱うデータ(iKey)が、文字列(char*)ではなく、数値(long)となる。
 * 内部のデータとしては数値を文字列変換し、文字列チェーンデータとして保持する。
 * そのためデータ自体は com_strChain_t*型で定義する。
 *
 * メモリ監視機能で呼び出した位置を正しく捕捉するため、com_searchChainNum()と
 * com_spreadChainNum()以外は、元I/Fと同じように COM_FILELOC を内包したマクロ
 * になっている。基本的に処理実体関数ではなく関数形式マクロの方を使用すること。
 *
 * com_popChainNum()のデータ終了の判定は、返り値が 0かどうかではなく、
 * *ioChainが NULLかどうかで実施すること。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_addChainNum( com_strChain_t **ioChain, long iKey );
 */
#define com_addChainNum( CHAIN, KEY ) \
    com_addChainNumFunc( (CHAIN), (KEY), COM_FILELOC )

/* 処理実体関数 */
BOOL com_addChainNumFunc( com_strChain_t **ioChain, long iKey, COM_FILEPRM );

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_sortAddChainNum( com_strChain_t **ioChain, COM_ADDCHAIN_t iSort,
 *                             long iKey );
 */
#define com_sortAddChainNum( CHAIN, SORT, KEY ) \
    com_sortAddChainNumFunc( (CHAIN), (SORT), (KEY), COM_FILELOC )

/* 処理実体関数 */
BOOL com_sortAddChainNumFunc(
        com_strChain_t **ioChain, COM_ADDCHAIN_t iSort, long iKey,
        COM_FILEPRM );

/* 検索はメモリ操作がないので実体のみ */
com_strChain_t *com_searchChainNum( com_strChain_t *iChain, long iKey );

/*
 * プロトタイプ形式 (この形で使用すること)
 *   long com_deleteChainNum( com_strChain_t **ioChain, BOOL iContinue,
 *                            long iKey );
 */
#define com_deleteChainNum( CHAIN, CONTINUE, KEY ) \
    com_deleteChainNumFunc( (CHAIN), (CONTINUE), (KEY), COM_FILELOC )

/* 処理実体関数 */
long com_deleteChainNumFunc(
        com_strChain_t **ioChain, BOOL iContinue, long iKey, COM_FILEPRM );

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_pushChainNum( com_strChain_t **ioChain, long iKey );
 */
#define com_pushChainNum( CHAIN, KEY ) \
    com_pushChainNumFunc( (CHAIN), (KEY), COM_FILELOC )

/* 処理実体関数 */
BOOL com_pushChainNumFunc( com_strChain_t **ioChain, long iKey, COM_FILEPRM );

/*
 * プロトタイプ形式 (この形で使用すること)
 *   long com_popChainNum( com_strChain_t **ioChain );
 */
#define com_popChainNum( CHAIN ) \
    com_popChainNumFunc( (CHAIN), COM_FILELOC )

/* 処理実体関数 */
long com_popChainNumFunc( com_strChain_t **ioChain, COM_FILEPRM );

size_t com_spreadChainNum(
        const com_strChain_t *iChain, char *oResult, size_t iSize );

/*
 * プロトタイプ形式  (この形で使用すること)
 *   void com_freeChainNum( com_strChain_t **ioChain );
 */
#define com_freeChainNum( CHAIN ) \
    com_freeChainNumFunc( (CHAIN), COM_FILELOC )

/* 処理実体関数 */
void com_freeChainNumFunc( com_strChain_t **ioChain, COM_FILEPRM );



/*
 *****************************************************************************
 * COMHASH:ハッシュテーブル関連I/F (com_proc.c)
 *****************************************************************************
 */

/*
 * ハッシュテーブル登録  comr_registerHash()
 *   登録されたテーブルIDを返す。以後使用するので、必ず保持すること。
 *   (テーブルIDは 0 から始まる値のため、敢えて初期化するなら -1 で)
 * ---------------------------------------------------------------------------
 *   COM_ERR_HUSHNG: ハッシュテーブル登録最大数到達
 *                   テーブル登録のための com_malloc()・com_realloc()で
 *                   エラー発生。この場合、プログラムを強制終了する。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * ハッシュテーブルを使用したいときは、最初にこのI/Fを使用しテーブルIDを取得
 * する必要がある。返ってきたテーブルIDを使って、以後のハッシュ処理は実施する。
 *
 * iTableSizeはテーブルサイズを指定する。
 * これは十分な大きさの素数が望ましいとされる。あまり小さすぎると、
 * ハッシュ値衝突が多発し、チェーン構造が長く伸びて検索効率が落ちる。
 * 「素数表」でWEB検索すれば、素数の一覧を調べることは比較的容易。
 *
 * テーブルサイズは大きいほどメモリ消費も激しくなることに注意。
 * テーブルサイズ * 4byteは最低でも消費する。
 * データが登録されれば、そのサイズも全てメモリ消費となる。
 *
 * iFuncはハッシュ値を計算するための関数を指定する。
 * この関数は入力された keyとそのサイズ、さらに該当ハッシュテーブルサイズを
 * 引数に取り、0～(テーブルサイズ - 1)の値を返す関数としなければならない。
 * NULLならば、comモジュール内の内部関数 calcHashKey()を使用する。
 * (従って calcHashKey()はハッシュ値計算のサンプルともいえる)
 *
 * テーブル生成数は COM_HASHID_MAXまで許容するが、それを超える登録要求時や
 * メモリ不足などでテーブル登録できなかった時は、エラーを出して強制終了する。
 */

typedef long   com_hashId_t;    // ハッシュテーブルID型
typedef ulong  com_hash_t;      // ハッシュ値計算結果型

#define COM_HASHID_MAX  LONG_MAX
#define COM_HASHID_NOTREG  -1  // ハッシュテーブル登録なしの状態(初期状態)

// ハッシュ値計算個別関数プロトタイプ
typedef com_hash_t (*com_calcHashKey)(
        const void *iKey, size_t iKeySize, size_t iTableSize );

/*
 * プロトタイプ形式 (この形で使用すること)
 *   com_hashId_t com_registerHash( size_t iTableSize, com_calcHashKey iFunc );
 */
#define com_registerHash( SIZE, FUNC ) \
    com_registerHashFunc( (SIZE), (FUNC), COM_FILELOC )

com_hashId_t com_registerHashFunc(
        size_t iTableSize, com_calcHashKey iFunc, COM_FILEPRM );

/*
 * ハッシュテーブル解放  com_cancelHash()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] 指定した iIDのハッシュテーブルなし
 *   テーブル解放のための com_free() によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iIDで指定したハッシュテーブルをメモリ解放し削除する。
 * 以後、そのテーブルIDを使ったハッシュ操作は出来なくなる。
 * comモジュール終了処理の中で、それまで生成したハッシュテーブルは
 * 全て自動解放するため、このI/Fによる解放は義務ではない。
 * しかし再利用しないと分かっているハッシュテーブルであれば削除して、
 * メモリを他に充てる方が望ましい。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_cancelHash( com_hashId_t iIF );
 */
#define com_cancelHash( ID ) \
    com_cancelHashFunc( (ID), COM_FILELOC )

void com_cancelHashFunc( com_hashId_t iID, COM_FILEPRM );

/*
 * ハッシュテーブル有効性確認  com_checkHash()
 *   テーブル使用可否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iIDで指定したハッシュテーブルが使用可能なら true、不可能なら falseを返す。
 */
BOOL com_checkHash( com_hashId_t iID );

/*
 * ハッシュデータ追加  com_addHash()
 *   追加できれば COM_HASH_OK、追加できなければ COM_HASH_NGを返す。
 *   同じキーが既にあった場合、COM_HASH_OKの代わりに COM_HASH_OVERWRITE
 *   または COM_HASH_EXIST を返す。詳細は下記参照。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iKey || 指定したiIDのハッシュテーブル無し
 *   新規ハッシュ追加時の com_malloc()によるエラー
 *   ハッシュデータ格納のための com_realloc()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じハッシュIDのテーブルへの複数スレッドによる読み書きは競合の危険
 *   がある。これについては、ハッシュ処理を使う側が排他処理を検討すること。
 * ===========================================================================
 * (iKey, iKeySize)の情報をキーに (iData, iDataSize)のデータを
 * iIDのハッシュテーブルに通過する。キーもデータも先頭アドレスが渡せて、
 * サイズが適切であれば、文字列である必要は無い。文字列を使用する場合は、
 * サイズに終端文字 \0 の分が含まれることを忘れないようにすること。
 *
 * キー・データともに内容をそのままコピーしてテーブルに保持するため、
 * 入力として指定するデータは関数内の自動変数でも問題ない。
 *
 * iOverwriteは同一キーが既にハッシュ登録されていた場合の挙動を決める。
 * ・falseなら、ハッシュ登録なし   (COM_HASH_EXISTを返す)
 * ・trueなら、上書きハッシュ登録  (COM_HASH_OVERWRITEを返す)
 * 同一キーで複数データを登録することは許容しない。
 */

// ハッシュ登録結果定義
typedef enum {
    COM_HASH_NG = 0,      // ハッシュ登録失敗
    COM_HASH_OK,          // ハッシュ登録成功
    COM_HASH_EXIST,       // 同一キーが存在したため、ハッシュ登録なし
    COM_HASH_OVERWRITE    // 同一キーが存在したため、上書きハッシュ登録
} COM_HASH_t;

/*
 * プロトタイプ形式 (この形で使用すること)
 *   COM_HASH_t com_addHash( com_hashId_t iID, BOOL iOverwrite,
 *                           const void *iKey, size_t iKeySize,
 *                           const void *iData, size_t iDataSize );
 */

#define com_addHash( ID, OVERWRITE, KEY, KEYSIZE, DATA, DATASIZE ) \
    com_addHashFunc( (ID), (OVERWRITE), (KEY), (KEYSIZE), (DATA), (DATASIZE), \
                     COM_FILELOC )

COM_HASH_t com_addHashFunc(
        com_hashId_t iID, BOOL iOverwrite, const void *iKey, size_t iKeySize,
        const void *iData, size_t iDataSize, COM_FILEPRM );

/*
 * ハッシュデータ検索  com_searchHash()
 *   データ有無を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iKey || 指定したiIDのハッシュテーブル無し
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じハッシュIDのテーブルへの複数スレッドによる読み書きは競合の危険
 *   がある。これについては、ハッシュ処理を使う側が排他処理を検討すること。
 * ===========================================================================
 * iIDのハッシュテーブルに、(iKey, iKeySize)のキーで保持されているデータの
 * アドレスとサイズを取得し、(*oData, *oDataSize)に格納し、trueを返す。
 * データが無い場合 falseを返し、(*oData, *oDataSize)の内容は変更しない。
 *
 * oData・oDataSizeは個別に NULL指定が可能で、その場合 データ格納しない。
 * 単純にハッシュ登録有無だけをチェックするなら、どちらも NULLで事足りる。
 *
 * oDataは const void *型で変数を定義し、&を付けて指定することを想定。
 * つまり取得したアドレスのデータ内容の変更は推奨されない。
 * 取得したサイズ以下のデータであれば変更は出来なくはないが、変更したいなら
 * 上書き指定をした com_addHash()を使用するべき。
 */
BOOL com_searchHash(
        com_hashId_t iID, const void *iKey, size_t iKeySize,
        const void **oData, size_t *oDataSize );

/*
 * ハッシュデータ削除  com_deleteHash()
 *   削除できたか、もともと無かった場合は trueを返す。
 *   バグ要因と思われる引数NGのときに falseを返す(iKeyが NULL等)
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iKey || 指定したiIDのハッシュテーブル無し
 *   データ削除のための com_free()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じハッシュIDのテーブルへの複数スレッドによる読み書きは競合の危険
 *   がある。これについては、ハッシュ処理を使う側が排他処理を検討すること。
 * ===========================================================================
 * (iKey, iKeySize)のキーで保持されているデータを iIDのテーブルから削除する。
 * 存在しなかった場合、何もせずに trueを返す(つまり異常とはしない)。
 * 存在するかどうか確認したい時は、事前に com_searchHash()を使用すること。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_deleteHash( com_hashId_t iID, const void *iKey, size_t iKeySize );
 */
#define com_deleteHash( ID, KEY, KEYSIZE ) \
    com_deleteHashFunc( (ID), (KEY), (KEYSIZE), COM_FILELOC )

BOOL com_deleteHashFunc(
        com_hashId_t iID, const void *iKey, size_t iKeySize, COM_FILEPRM );



/*
 *****************************************************************************
 * COMSORT:ソートテーブル関連I/F (com_proc.c)
 *****************************************************************************
 */

/*
 * ソートテーブルは、キーとなる数値と紐付けたデータを扱う。
 * このデータはキーを昇順ソートした順で保持しており、テーブル内のデータも
 * キーを使って検索できる。その検索には二分探索法を使用している。
 * 二分探索を実現するために、キーで昇順ソートしてデータを保持する。
 *
 * データは void*型でメモリ確保した領域に そのサイズとともに保持するため、
 * その型は不問となる。使う側でデータ型を意識して使用すれば良い。
 *
 * 実際のソートテーブルは com_sortTable_t型で定義する。
 * データ数(count)と実際のデータテーブル(table)をここに保持する。
 * tableは com_sort_t*型の単純な線形データとなっており、
 * キー(key)・範囲(range)・データ(data)・データのサイズ(size)で構成される。
 * 単純な線形データである故、.table[0]～.table[.count-1] で各要素に直接
 * アクセスすることが可能。内容の参照であれば遠慮なく直接見て良いだろう。
 * その内容を変更したい時は、com_setSortData()の使用が推奨される。
 * このI/Fは既存データ変更時は、そのキーを変えずに処理するようになっている。
 *
 * com_addSortTable()でテーブルデータを追加することにより、キーが小さい順で
 * ソートしたテーブルに配置しており、com_searchSortTable()等の検索処理は、
 * 正しくソートされていることが前提の処理になっている。つまり可能ではあっても
 * 手動でキーを変更してソート順が崩れると、以後正しく動けなくなる。
 *
 * 具体的な使用イメージをを以下に略記する。
 *
 * まずソートテーブルの実体を定義する。
 * (下記ではグローバル変数としているが、そうしなければいけないわけではない)
 *
 * com_sortTable gSample;
 *
 * 以下で初期化実施。下記は同じキーの複数データを許容する設定にしている)
 *
 *     com_initializeSortTable( &gSample, COM_SORT_MULTI );
 *
 * 追加には例えば以下のような関数を使用する。spec_data_t型が保持対象データ。
 * それを含んだ com_sort_t型データを生成し、それをソートテーブルに追加する。
 *
 * BOOL addSample( spec_data_t *iData ) {
 *     com_sort_t tmp = { 1, 1, iData, sizeof(*iData) };
 *     return com_addSortTable( &gSample, &tmp );
 * }
 *
 * これで一番最初のデータは gSample.table[0] に格納される。
 * gSample.table[0].data には上記関数の引数 iData の内容がコピーされており、
 * そのまま参照しても問題ない。コピーしているので iDataの内容が失われても
 * ソートテーブル内のデータは保持できている。
 *
 * その内容を変更したい時は
 *
 *     com_setSortData( &(gSample.table[0]), &tmp );
 *
 * とする。tmpは変更したい内容を addSample()と同じ要領で設定しておく。
 * ただデータサイズ変更が無い単純な変数や構造体を保持しているなら、
 * com_setSortData()を使わず、以下のように直接操作しても問題ない。
 * (心配なら com_setSortData()を使うほうがよいだろう)
 *
 *     spec_data_t* ref = gSample.table[0].data;
 *     ref->member = 1;     // memberというメンバー変数がある構造体のイメージ
 *
 * データ検索の簡単なイメージは以下となる。
 * これはキーが 1のデータを検索する方法のひとつとなる。
 *
 *     com_sort_t** result;
 *     spec_data_t* ref = NULL;
 *
 *     long count = com_searchSortTableByKey( &gSample, 1, &result );
 *     if( count > 0 ) {
 *         ref = result[0]->data;
 *     }
 *
 * これで refには検索がヒットした場合に、その最初のデータアドレスが入る。
 * このアドレスは gSample.table内のデータへの直アドレスなので、
 * そのままテーブル内容の参照や変更が可能となる。
 * (countが 2以上だったら、result[1]...へのアクセスも考慮が必要となるだろう)
 *
 * 生成したソートテーブルは使用終了時には以下のようにメモリ解放すること。
 * 
 *     com_freeSortTable( &gSample );
 */

/*
 * ソートテーブルデータ初期化  com_initializeSortTable()
 *   初期化成否を true/false で返す。
 *   ＊テーブル使用後は com_freeSortTable()でテーブルデータのメモリ解放を
 *     実施すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oTable
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じソートテーブルへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、ソートテーブルを使う側が排他処理を検討すること。
 * ===========================================================================
 * ソートテーブルを利用したい場合、まず com_sortTable_t型の実体を用意し、
 * この I/Fで初期化する。この際、同一キーの追加要求に対するアクションを選び、
 * iAct で指定する。その内容は com_sortTable_t型のメンバー matchActionに入る
 * だけなので、直接テーブル内のデータを変更しても動作は可能(非推奨)。
 * アクションの具体的な処理については、com_addSortTable()の記述を参照。
 *
 * .table[]や その各要素の dataは、データ追加のたびにメモリ捕捉するので、
 * 使用後は com_freeSortTable()でメモリ解放を実施すること。
 */

// ソートテーブル データ型
typedef struct {
    long    key;     // ソート対象となるキー番号
    long    range;   // キー範囲(0以下は 1とみなす)
    void*   data;    // キーに対応するデータ
    size_t  size;    // データサイズ
} com_sort_t;

// 入力キーが既に登録済みの場合の動作(range=1のみ、2以上は全て COM_SORT_SKIP)
typedef enum {
    COM_SORT_OVERWRITE,      // 上書きする
    COM_SORT_MULTI,          // 複数の存在を許容する
    COM_SORT_SKIP            // 追加せずに破棄する
} COM_SORT_MATCH_t;

// ソートテーブル型
typedef struct {
    long              count;          // 現在データ数
    COM_SORT_MATCH_t  matchAction;    // 同一キーの処理方法
    int32_t  dummyFor64bit;
    com_sort_t*       table;          // ソートテーブル本体
    com_sort_t**      search;         // 検索結果出力用
} com_sortTable_t;

void com_initializeSortTable( com_sortTable_t *oTable, COM_SORT_MATCH_t iAct );

/*
 * ソートテーブル データ追加  com_addSortTable()・com_addSortTableByKey()
 *   なんらかのデータ追加/上書きがあったら trueを返す。
 *   テーブルへの変更をしなかった場合は falseを返す。
 *   (重複キースキップ時や、データ追加のためのメモリ不足等)
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oTable || !iData
 *   テーブル領域確保の com_realloc()や、解放の com_free()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じソートテーブルへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、ソートテーブルを使う側が排他処理を検討すること。
 * ===========================================================================
 * oTable に対し、iDataのデータを追加する。
 * この際 iData->keyの値をキーにした昇順ソートした形で oTableに追加する。
 *
 * *oCollisionには同一キーのデータ存在有無が true/false で格納される。
 * oCollisionは NULLにすることも許容し、その場合は 何も格納しない。
 *
 * もし同一キーのデータが既にある場合、com_initializeSortTable()の iAct指定値
 * ..すなわち oTableのメンバー .matchAction により以下のように処理が変わる。
 * ハッシュテーブルと異なり、同一キーでの複数登録を許容できる。
 *
 *   matchAction == COM_SORT_OVRRWRITE  ->既存データに上書きする。
 *   matchAction == COM_SORT_MULTI      ->同一キーで複数追加する。
 *   matchAction == COM_SORT_SKIP       ->何もせずに破棄し falseを返す。
 *
 * 上記の処理が行われたかどうか明示的に知りたい場合は BOOL型変数を用意し、
 * そのアドレスを oCollisionに設定しておくこと。本I/Fから返った後、
 * *oCollisionが trueであれば、matchAction実施を示す。
 *
 * iDataの要素 .rangeを 1未満にした場合、内部で 1に補正する。
 * iDataの要素 .rangeを 1より大きな値にした場合、.keyに指定した値を先頭値とし
 * .rangeの個数までの範囲値をキーとすることが可能になる。1つでもこうした指定を
 * した場合、matchActionは COM_SORT_SKIP に変更する。つまり、以後 範囲重複する
 * ようなデータの追加は行えなくなる。既に登録済みの中で重複しているものは、
 * そのまま残るが、同一キーのデータ検索で思ったとおりに動かない危険がある。
 * そうしたことから、範囲付きデータが入ることが最初から分かっているのであれば、
 * com_initializeSortTable()の段階で、matchActionには COM_SORT_SKIP を設定する
 * 方が望ましい。
 *
 * またテーブル検索を行う時に、その結果を格納するための領域を先に確保する。
 * これは「データポインタ * データ数」のサイズの領域となる。
 * もしこの領域の確保に失敗した場合、メモリ確保失敗のエラーメッセージは出るが、
 * 処理自体は続行を試みる。その後、検索処理を実施したときは、最初にヒットした
 * データを1つだけ返すようになる。
 *
 * com_addSortTableByKey()はキーを直接指定したデータ登録のI/Fとなる。
 * この I/Fでは iDataを自動編集して、com_addSortTable()を実行する。
 * この際の .rangeは無条件で 1となる。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_addSortTable( com_sortTable_t *oTable, com_sort_t *iData,
 *                          BOOL *oCollision );
 *   BOOL com_addSortTableByKey ( com_sortTable_t *oTable, long iKey,
 *                                void *iData, size_t iSize, BOOL *oCollision );
 */
#define com_addSortTable( TABLE, DATA, COLL ) \
    com_addSortTableFunc( (TABLE), (DATA), (COLL), COM_FILELOC )

#define com_addSortTableByKey( TABLE, KEY, DATA, SIZE, COLL ) \
    com_addSortTableByKeyFunc( (TABLE), (KEY), (DATA), (SIZE), (COLL), \
                               COM_FILELOC )

BOOL com_addSortTableFunc(
        com_sortTable_t *oTable, com_sort_t *iData,
        BOOL *oCollision, COM_FILEPRM );

BOOL com_addSortTableByKeyFunc(
        com_sortTable_t *oTable, long iKey, void *iData, size_t iSize,
        BOOL *oCollision, COM_FILEPRM );

/*
 * ソートデータ変更  com_setSortData()
 *   処理の成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oTarget || !iData
 *   テーブル領域確保の com_realloc()や、解放の com_free()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じソートテーブルへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、ソートテーブルを使う側が排他処理を検討すること。
 * ===========================================================================
 * oTargetに指定したソートデータの内容を、iDataの内容に変更する。
 *
 * もし oTargetに既にデータが保持されていた場合、先に com_free()で解放する。
 * その後、com_malloc()で iData->sizeのメモリ領域確保を行い、そこに iData->data
 * の内容をコピーする。iData->sizeはそのまま oTargetでも保持する。
 *
 * iData->keyについては、oTargetが何もデータなしだった場合は、oTargetにて
 * そのまま保持するが、データありの場合(上記の com_free()が動作するケース)は
 * 元々の oTarget->keyの内容がそのまま残り、iData->keyの内容は無視する。
 *
 * これは com_addSortTable()のデータ書き込みでも使用されている処理となる。
 *
 * なお実際に保持するデータ(data)がサイズ変更を伴わないデータ領域の場合、
 * このI/Fを使わず、直接内容を変更しても問題は発生しない。
 * サイズ変更を伴う可能性がある場合のみ、内容変更は本I/Fを使う方が安全。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_setSortData( com_sort_t *oTarget, com_sort_t *iData );
 */
#define com_setSortData( TARGET, DATA ) \
    com_setSortDataFunc( (TARGET), (DATA), COM_FILELOC )

BOOL com_setSortDataFunc(
        com_sort_t *oTarget, com_sort_t *iData, COM_FILEPRM );

/*
 * ソートテーブル データ検索  com_searchSortTable()・com_searchSortTableByKey()
 *   指定したキーのデータ数を返す。0なら該当データ無し。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oTable || !iTarget || !iResult
 *                   ＊com_searchSortTableByKey()の場合 !iTarget は無し
 *   テーブル領域確保の com_realloc()や、解放の com_free()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じソートテーブルへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、ソートテーブルを使う側が排他処理を検討すること。
 * ===========================================================================
 * iTargetで示したデータを iTableの中から検索し、見つかった数を返す。
 * また見つかったデータのアドレスを oResultに格納する。複数ある場合は、
 * 見つかった順に線形で並列する。このアドレスは iTable->table内の実際の
 * データのアドレスであり、直接テーブルにアクセス可能となる。
 *
 * データ変更したい場合は com_setSortData()の使用を推奨するが、
 * データサイズが変わらないのであれば、直接内容変更しても良い。
 *
 * iTargetのうち .keyは必須で設定が必要。
 * .rangeが 1より大きい場合は、その範囲に含まれるデータを全て検索する。
 * .dataが NULLだったら .keyが一致するものを全て検索する。
 * .dataが非NULLの場合は .sizeも設定必須となり .keyが一致するだけでなく
 * データ内容も一致するもののみを検索する。
 *
 * oResultは com_sort_t **型で定義した変数に &を付けたアドレスを想定する。
 * 変数名を result としていた場合、result[0]が見つかった1つ目のデータとなり、
 * 返り値で示した数まで同じ方法で順番にデータアドレスを取り出せる。
 * 返した数を超えた先のデータは未保証なので注意すること。
 * 結果を格納する領域はソートテーブルにデータを追加した時に併せて確保している。
 * そのため本I/F使用時に oResultで得られた領域の解放は不要。もし確保に失敗して
 * いた場合、検索結果が複数あっても最初の1つのみを返すようになる。
 *
 * com_searchSortTableByKey()はキーだけを、iKeyで指定する検索I/Fとなる。
 * I/F内で com_searchSortTable()用の iTargetを自動で準備する。その内容は
 * .keyは iKeyで指定した値、.rangeは 1、.dataは NULL、.sizeは 0となる。
 * それ以外の使い方は com_searchSortTable()と変わらない。
 *
 * 直接データの内容から検索したい場合は、iTable->tableに直接アクセスする処理を
 * 書けば良い。ただ、ソートテーブルという形式は key値を使ったソートにより
 * 検索を高速化することを目指したものなので、検索に keyを使わないのであれば
 * そもそもソートテーブルとしてデータを作る必要すらないかもしれない。
 */

long com_searchSortTable(
        com_sortTable_t *iTable, com_sort_t *iTarget, com_sort_t ***oResult );

long com_searchSortTableByKey(
        com_sortTable_t *iTable, long iKey, com_sort_t ***oResult );

/*
 * ソートテーブル データ削除  com_deleteSortTable()・com_deleteSortTableByKey()
 *   指定した条件で削除したデータ数を返す。0なら該当データ無し。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oTable || !iTarget
 *                   ＊com_deleteSortTableByKey()の場合 !iTarget は無し
 *   データ解放の com_free()や テーブル縮小時の com_realloc()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じソートテーブルへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、ソートテーブルを使う側が排他処理を検討すること。
 * ===========================================================================
 * oTableの中から iTargetで示された条件のデータを検索して削除する。
 * iDeleteAllが trueの場合、条件該当するデータを全て削除する。
 * iDeleteAllが falseの場合、ひとつ削除したら処理終了となる。
 *
 * iTargetのうち .keyは設定必須。.rangeは参照しない(.key一致で削除)。
 * .dataが NULLだったら .keyが一致するものを削除する。
 * .dataが非NULLの場合は .sizeも設定必須となり、.keyが一致するだけでなく、
 * データ内容も一致するもののみを削除する。
 *
 * com_deleteSortTableByKey()はキーだけを iKeyで指定する削除I/Fとなる。
 * I/F内で com_deleteSortTable()用の iTargetを自動で編集する。その内容は
 * .keyは iKeyで指定した値、.rangeは 1、.dataは NULL、.sizeは 0となる。
 * また iDeleteAll は無条件で true を指定する。
 * それ以外の使い方は com_deleteSortTable()と変わらない。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   long com_deleteSortTable( com_sortTable_t *oTable, com_sort_t *iTarget,
 *                             BOOL iDeleteAll );
 *
 *   long com_deleteSortTableByKey( com_sortTable_t *oTable, long iKey );
 */
#define com_deleteSortTable( TABLE, TARGET, DELALL ) \
    com_deleteSortTableFunc( (TABLE), (TARGET), (DELALL), COM_FILELOC )

#define com_deleteSortTableByKey( TABLE, KEY ) \
    com_deleteSortTableByKeyFunc( (TABLE), (KEY), COM_FILELOC )

long com_deleteSortTableFunc(
        com_sortTable_t *oTable, com_sort_t *iTarget, BOOL iDeleteAll,
        COM_FILEPRM );

long com_deleteSortTableByKeyFunc(
        com_sortTable_t *oTable, long iKey, COM_FILEPRM );

/*
 * ソートテーブル解放  com_freeSortTable()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oTable
 *   データ解放の com_free()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じソートテーブルへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、ソートテーブルを使う側が排他処理を検討すること。
 * ===========================================================================
 * oTableで指定されているソートテーブルのデータ保持用メモリを全解放する。
 * ソートテーブルをこれ以上使用しなくなったときに、本I/Fで解放する。
 *
 * oTableそのものを解放はしないので、もし動的メモリ確保していた場合は
 * oTable解放の前に、本I/Fでテーブルデータの全解放を実施すること。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_freeSortTable( com_sortTable_t *oTable );
 */
#define com_freeSortTable( TABLE ) \
    com_freeSortTableFunc( (TABLE), COM_FILELOC )

void com_freeSortTableFunc( com_sortTable_t *oTable, COM_FILEPRM );



/*
 *****************************************************************************
 * COMRING:リングバッファ関連I/F (com_proc.c)
 *****************************************************************************
 */

/*
 * リングバッファは特定サイズのバッファを複数持ったものである。
 * 複数の保持が必要で、かつ保持した順に処理が必要なデータを扱うバッファとなる。
 * バッファサイズが可変になるものは、その最大サイズでリングバッファを生成する
 * ということになるが、それよりはサイズが固定のデータを保持するのに向く。
 * (データモデルの原点はDPDKから。DPDKのリングバッファはもっと複雑で多機能)
 *
 * リングバッファは com_createRingBuf()で生成する。
 *
 * リングバッファへのデータ追加は com_pushRingBuf()で行う。
 * バッファの先頭から順番に、保持中データの末尾にどんどん保持されていく。
 * 受け取ったデータをバッファ内にコピーして保持する。
 * バッファの最後までいったら、先頭に戻って保持を継続する。この動きをするので
 * 「リングバッファ」と呼称している。
 * まだ取り出されていないデータまで到達したら、そこで保持は中断する。
 * (その場合は COM_ERR_RING のエラー(リングバッファFULL)が発生する)
 *
 * ＊中断せずに上書きする設定にも変更できるが、その場合 保持はしたが参照されずに
 *   データが消えることもあり得ることを理解している必要がある。
 *
 * リングバッファからのデータ取出は com_pullRingBuf()で行う。
 * このデータの取出は、必ず保持データの先頭から順番となる(いわゆる先入先出)。
 * 取出が行われたバッファは上書き可能となるが、データ保持が一周して上書きされる
 * までは、内容を保持し続けている(取り出した時にクリアなどしない限り)。
 * 無論、この上書きがいつ発生するかは分からないので、取り出したデータの保持が
 * 必要なら、それは取り出す側で別途領域を確保して、取り出し後即座にコピーする
 * のが一番無難。
 *
 * データ保持が一周したら、データ取り出し済みのバッファは上書きされる。
 * ここで初めて、そのバッファへのデータ保持の前にクリアが行われる。
 * バッファ内に別途メモリ確保するデータが存在する場合、このデータクリア時に
 * それを解放しないと浮きになってしまう。そのため保持するデータの解放処理を
 * 記述した関数を作成し、リング生成時にその関数も指定しなければならない。
 * (単純にバッファを解放するだけで良いなら、NULL指定で良い)
 *
 * 最終的に使い終わったリングバッファは com_freeRingBuf()で解放すること。
 * この際、バッファ内のデータもすべて解放される。
 * 解放関数が登録されている場合は、全バッファにその関数による開放処理が動く。
 *
 * 使用例：  ＊struct items_t は事前に宣言された構造体、
 *             freeItems()はその構造体の中身のメモリ解放を行う関数とする。
 *
 *     com_ringBuf_t* ring = com_createRingBuf( sizeof(struct items_t), 100,
 *                                              false, false, freeItems );
 *
 *     struct items_t item = { (内容は省略) };
 *     com_pushRingBuf( ring, &item, sizeof(item) );
 *
 *     struct items_t* ref = com_pullRingBuf( ring );
 *
 *     com_freeRingBuf( ring );
 *
 * また com_testtos(.c)の test_ringBuffer()も使用サンプルとなっている。
 */

/*
 * リングバッファ 解放関数プロトタイプ宣言 (com_createRingBuf()で設定)
 * 
 * バッファ内のデータをクリアするタイミングで解放関数は呼ばれる。これは
 * ・com_pushRingBuf()で登録が一周して取り出し済みのデータを上書きするとき
 * ・com_freeRingBuf()でリングバッファそのものを解放するとき
 * の2パターンがあり得る。
 *
 * この関数が受け取る引数には、バッファにコピーされた個々のデータのいずれかの
 * 先頭アドレスが渡される。実際のデータは特定の型(構造体等)であることを想定し
 * 関数内ではその型のポインタとして、そのアドレスをキャストすることにより、
 * データアクセスも可能と想定している。
 *
 * その上でメモリ解放が必要なデータについて com_free()を行うことを期待する。
 * データ解放が不要でも消される前に何らかの処理が必要なら、この関数でその必要
 * な処理を行えるだろう。
 */
typedef void(*com_freeRingBuf_t)( void *oData );

// リングバッファ構造
typedef struct {
    void*   buf;         // バッファの先頭アドレス
    size_t  unit;        // 各バッファのサイズ
    size_t  size;        // 総バッファ数
    long    head;        // バッファ先頭位置
    long    tail;        // バッファ最終位置(正確にはそのひとつ先)
    size_t  used;        // 使用中バッファ数
    BOOL    overwrite;   // 取り出し前データへの上書き可否
    BOOL    owNotify;    // 取り出し前データへの上書き実施通知フラグ
    size_t  owCount;     // 取り出し前データの上書き数
    com_freeRingBuf_t  freeFunc;     // データ解放時に呼ぶ個別処理
} com_ringBuf_t;

/*
 * リングバッファ生成  com_createRingBuf()
 *   生成したリングバッファのアドレスを返す。
 *   生成に失敗した場合は NULLを返す。
 *   このリングバッファは使用後に、com_freeRingBuf()で解放が必要。
 *   バッファ内にさらにメモリ捕捉するデータがある場合は、その解放処理を
 *   記述した関数も用意して本I/Fの iFuncで指定する必要がある。
 * ---------------------------------------------------------------------------
 *   COM_ERR_NOMEMORY: リングバッファメモリ捕捉NG
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * iUnitで個々のバッファサイズ、iSizeで総バッファ数を指定することで、
 * その設定に従ったリングバッファを生成し、そのアドレスを返す。
 * 以後、リングバッファに対する操作には必ずそのアドレスを使用するので、
 * com_ringBuf_t*型のポインタ変数を用意して保持すること。
 *
 * iOverwriteが trueの場合、取り出し前のデータが有っても上書きを実施する。
 * 上書きが実施された場合、com_ringBuf_t型の .owCnt にその数が計上される。
 * これはバッファリングされても使われないままデータ消失することを意味する為、
 * 本当にそれで問題がないデータの場合のみ trueを設定すること。
 * falseを指定した場合、エラーメッセージを出力し、それ以上保持を行わない。
 * つまりデータ保持に失敗する。
 *
 * iNotifyOverwriteを trueにした場合、上記の「取り出し前データの上書き」が
 * 発生した時にエラーメッセージを出力して通知する。
 * この通知が不要の場合は falseを設定すること。
 *
 * iFuncはリングバッファのデータを解放するための関数である。
 * 特に開放が必要なデータない場合は NULLを指定すれば良い。
 * この関数の動作については com_freeRingBuf_tの宣言箇所で記載済み。
 *
 * リングバッファの動作仕様については、このI/F説明の直前で記載しているので、
 * 使い方の流れはそちらを参照して欲しい。
 * リングバッファに登録するデータは特定の型(構造体等)とし、その sizeof()値が
 * iUnitに設定され、その総数が iSizeに設定されるという使い方を想定している。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   com_ringBuf_t *com_createRingBuf( size_t iUnit, size_t iSize,
 *                                  BOOL iOverwrite, BOOL iNotifyOverwrite,
 *                                  com_freeRingBuf_t iFunc );
 */
#define com_createRingBuf( UNIT, SIZE, OW, NOTIFOW, FUNC ) \
    com_createRingBufFunc( (UNIT), (SIZE), (OW), (NOTIFOW), (FUNC), \
                           COM_FILELOC )

com_ringBuf_t *com_createRingBufFunc(
        size_t iUnit, size_t iSize, BOOL iOverwrite, BOOL iNotifyOverwrite,
        com_freeRingBuf_t iFunc, COM_FILEPRM );

/*
 * リングバッファ データ登録  com_pushRingBuf()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oRing || !iData
 *   COM_ERR_NOMEMORY: iDataSize > oRing->unit (個々のバッファサイズ)
 *                     バッファがすべて使用中(上書きなしの場合)
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じリングバッファへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、リングバッファを使う側が排他処理を検討すること。
 * ===========================================================================
 * oRingで指定したリングバッファに iDataの内容を登録コピーする。
 * iDataSizeは iDataのサイズで、もしリングバッファ生成時に設定した個々の
 * バッファサイズ(iUnit)より大きい場合はエラーになる。同じである必要はなく、
 * 小さい分には問題ない。
 *
 * 実際の登録の動作は前述した通りとなる。
 * 内容をコピーするので、iData自体は本I/F使用後 破棄されても問題ない。
 *
 * リングバッファが取り出されていないデータで一杯の場合、上書き設定であれば
 * 順に上書き登録し、そうでなければエラーを出して falseを返す。
 * この場合の動作については com_createRingBuf()であらかじめ設定するもので、
 * その詳細はそちらの説明を参照。
 *
 * このI/Fを使わず直接 oRing->buf の内容を見たり書いたりすることも勿論可能。
 * ただその場合、以後の動作は保証できなくなる。
 */
BOOL com_pushRingBuf( com_ringBuf_t *oRing, void *iData, size_t iDataSize );

/*
 * リングバッファ データ取り出し  com_pullRingBuf()
 *   一番先頭の取り出し前データのバッファアドレスを返す。
 *   データがひとつもない時は NULLを返す(エラーにはならない)。
 *   エラー発生時も NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oRing
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じリングバッファへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、リングバッファを使う側が排他処理を検討すること。
 * ===========================================================================
 * oRingで指定したリングバッファに保持された、取り出し前のデータの一番先頭に
 * あるバッファのアドレスを返す。
 *
 * 実際の取り出しの動作は前述した通りとなる。
 * 本I/F使用後、そのバッファは新しいデータで上書き可能となるため、内容の継続
 * 使用があり得る時は、個別にバッファを用意して内容を即座にコピーすること。
 *
 * リングバッファのデータの型(構造体等)のポインタ変数を用意し、返り値をそれで
 * 受けることを想定している。void*型なので、代入時にキャストは不要。
 *
 * このI/Fを使わず直接 oRing->buf の内容を見たり書いたりすることも勿論可能。
 * ただその場合、以後の動作は保証できなくなる。
 */
void *com_pullRingBuf( com_ringBuf_t *oRing );

/*
 * リングバッファ 残りデータ数取得  com_getRestRingBuf()
 *   バッファに残っている取り出し前のデータ数を返す。
 *   エラーが発生した時は 0 を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iRing
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じリングバッファへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、リングバッファを使う側が排他処理を検討すること。
 * ===========================================================================
 * iRingで指定されたリングバッファに保持された取り出し前データ数を返す。
 * といっても現状は iRing->rest の内容を返すだけなんで、本I/Fを使わなくても
 * 簡単に取得できる。ただいつデータ形式が変わるかは分からないので、
 * 記述が iRing->rest を見るより長くなって申し訳ないが、本I/F使用を推奨する。
 */
size_t com_getRestRingBuf( com_ringBuf_t *iRing );

/*
 * リングバッファ解放  com_freeRingBuf()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oRing
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただし同じリングバッファへの複数スレッドによる読み書きは競合の危険がある。
 *   これについては、リングバッファを使う側が排他処理を検討すること。
 * ===========================================================================
 * oRingで指定したリングバッファを解放し。NULLを格納する。
 * 引数がダブルポインタになっていることに注意。
 * com_createRingBuf()で生成した際に使用した変数に & を付ける想定となる。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_freeRingBuf( com_ringBuf_t **oRing );
 */
#define com_freeRingBuf( RING ) \
    com_freeRingBufFunc( (RING), COM_FILELOC )

void com_freeRingBufFunc( com_ringBuf_t **oRing, COM_FILEPRM );



/*
 *****************************************************************************
 * COMCFG:コンフィグ関連I/F (com_proc.c)
 *****************************************************************************
 */

/*
 * バイナリ動作のコンフィグ設定を登録・変更・取得する汎用I/F群。
 * ファイルからの読み込みをしたい場合、com_seekFile() を利用して、
 * ファイル内容を取得して登録を実施することになるだろう。
 *
 * コンフィグ設定は内部で文字列で保持する。
 * 新たな値を設定する際に、その入力が正しいかチェックさせることも可能。
 * チェック用の関数は複数を指定することも出来るし(AND判定になる)、
 * 独自の関数を指定することも可能。ただメモリ管理の都合上、少々複雑な仕様に
 * せざるを得なかったので、以下に続く説明をよく読むこと。
 *
 * 現在のコンフィグ値は、単純にその文字列を取得するだけでなく、
 * 数値変換するなど、幾つかの目的で変換した結果を取得することも可能。
 */

/*
 * コンフィグ登録  com_registerCfg()・
 *                 com_registerCfgDigit()・com_registerCfgUDigit()
 *   登録結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iKey
 *   COM_ERR_CONFIG: 既に iKeyが登録済み/登録に失敗
 *   データ追加のための com_reallocAddr()/com_strdup()のエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * iKeyの文字列をキーとして、iDataをコンフィグ設定に登録する。
 *
 * com_registerCfg()は iDataをそのまま文字列でコンフィグ値として保持する。
 * iDataを NULLにした場合「登録なし」の状態になる。
 *
 * com_registerCfg()以外は iDataを文字列変換してコンフィグ値として保持する。
 * iDataを 0 にしても "0" で登録ありの状態になる。登録なしの状態にしたい場合は
 * com_registerCfg()で iDataを NULL指定すること。
 *
 * 以後、コンフィグ値を変更したいときは com_setCfg～()を使用する。
 * 入力データ形式の違いだけなので、例えば com_registerCfg()で登録しても、
 * com_setCfgDigit()を使用した設定が可能。
 *
 * コンフィグ値設定時にチェック関数を呼び出して内容をチェックすることも可能。
 * もしチェックNGの場合はコンフィグ値設定がエラーになる。ただし、本I/F指定の
 * iDataのチェックは遡ってしないので、それは正しい指定をすること。
 * チェック関数は com_addCfgValidator()で登録する。複数回使用することで、
 * 複数のチェック関数を登録することも可能で、その場合は1つでもチェックNGに
 * なると登録不可になる。
 *
 * 現在の設定値は com_getCfg～()で取得する。
 * 取得形式ごとに異なるI/Fとなっており、com_getCfg()以外の取得I/Fは
 * 文字列を保持しているものを、I/Fごとの指定に沿った形に変換して返すものとなる。
 * 従って、正しく取得できるように、使う側が設定をする必要がある。
 */
BOOL com_registerCfg( char *iKey, char *iData );
BOOL com_registerCfgDigit( char *iKey, long iData );
BOOL com_registerCfgUDigit( char *iKey, ulong iData );

/*
 * コンフィグ設定チェック関数登録  com_addCfgValidator()
 *   処理結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iKey
 *   COM_ERR_CONFIG: iKeyをキーとしたコンフィグ未登録
 *                   必要な条件未設定/条件不要なのに条件設定
 *   データ追加のための com_reallocAddr()/com_malloc()/com_strdup()のエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * iKeyをキーとするコンフィグにチェック関数と、その条件コピー/解放関数を登録
 * する。実際に独自チェック関数を書けるようにするため、少々複雑な仕組みなので
 * 以下の説明をよく確認すること。
 *
 * iCondは iFuncに応じた形式のデータを指定する。指定がある場合 iCopyの処理で
 * 内容をコピーして保持するため、本I/F呼んだ後も iCondの内容を保持する必要は
 * ない(つまり関数の自動変数で作成したものを指定して良い)。
 * 条件を特に指定しない場合(出来る場合でも)、NULLを指定する。
 * iFuncの関数を実行するときの第2引数(iCond)に保持した内容がそのまま指定される。
 *
 * 実際のチェック関数は iFuncで指定する。
 * iCopyは iFuncが必要とする iCondの内容をコンフィグ設定にコピーする処理を
 * 記述する関数で、iFuncごとに決まっている。ない場合は NULLになる。
 * iFreeは iFuncが必要としていた iCondをコンフィグ設定から解放する処理を
 * 記述する関数で、iFuncごとに決まっている。ない場合は NULLになる。
 * 
 * iCopy・iFreeはたとえ iCondに NULLが指定されている場合でも、iFuncごとに
 * 決まったものを指定したほうが安全である。
 * つまり iFunc, iCopy. iFree は3つでセットになっている。本ヘッダでは、
 * この3つを一度に指定するマクロを用意するので、本I/Fを使う際は 個別に指定
 * するのではなく、以下のマクロを使うことを推奨する。
 *
 *      COM_VAL_DIGIT         long型数値チェック
 *      COM_VAL_DGTLIST       long型数値リストチェック
 *      COM_VAL_UDIGIT        ulong型数値チェック
 *      COM_VAL_UDGTLIST      ulong型数値リストチェック
 *      COM_VAL_HEX           16進数値チェック
 *      COM_VAL_HEXLIST       16進数値リストチェック
 *      COM_VAL_STRING        文字列長チェック
 *      COM_VAL_STRLIST       文字列リストチェック
 *      COM_VAL_BOOL          True/Falseの文字列チェック
 *      COM_VAL_YESNO         Yes/Noの文字列チェック
 *      COM_VAL_ONOFF         On/Offの文字列チェック
 *
 * 条件を示す iCond の実際の型と設定方法も、各マクロの説明で記述する。
 *
 *
 * 本I/Fでチェック関数を登録すると、com_setCfg～()の設定I/Fを使用したときに
 * iDataの内容を指定したチェック関数でチェックし、NGだと登録不可と出来る。
 * 同じ iKeyのコンフィグに対し、本I/Fを複数回呼んで 複数のチェック関数を
 * 登録することも可能で、その場合は1つでもチェックNGだと登録不可となる。
 *
 * com_registerCfg～()の iDataをさかのぼってチェックはしないので、
 * 不正となるような値を初期値登録しないように気をつけること。
 *
 *
 * iFuncと iConvをセットにして独自のチェック関数を作成することも可能。
 * そのチェック関数が条件(iConv)を必要としない(常にNULL)のであれば、
 * iCopy・iFuncの関数を不要。上記 COM_VAL_BOOL のように
 *
 *     #define COM_VAL_SAMPLE \
 *         com_valSample, NULL, NULL
 *
 * というようなマクロを用意して、
 *
 *     com_addCfgValidator( "CONFIG_SAMPLE", NULL, COM_VAL_SAMPLE );
 *
 * というように独自チェック関数を登録できる。
 * 実際のチェック関数は、チェックOKなら true、チェックNGなら falseを返す
 * ように記述すること。
*
 * iCondの実際の型は独自に作っても良いし、既存を利用しても良い。
 * 既存を利用するのなら、コピー/解放関数もそのまま使用できる。
 * COM_VAL_HEX は COM_VAL_UDIGIT用の条件型をそのまま利用したケースとなる。
 *
 * iCopyが必要になるのは、iConvに何らかのデータを指定出来る時で、
 * コンフィグ設定のテーブルに iConvの内容をどのようにコピーするか記述する。
 * コピーの成否を true/false で返すようにすること。
 * 単純なコピーで良ければ、その条件データのサイズ分 com_malloc()で確保し
 * iCondの内容をコピーして、*oCondにそのアドレスを書き込むだけで良い。
 * こうしたケースの場合、メモリ解放も単純で済むので iFreeの関数は不要になり
 * NULL指定でよくなる。
 * 例えば COM_VAL_DIGIT がそうしたケースとなっている。
 *
 * iFreeが必要になるのは、条件用データの中でさらにメモリ確保が必要な場合で
 * iCopyで必要なメモリ確保とデータコピーを行うとともに、
 * iFreeでそのメンバーのメモリ解放を記述しなければならない。
 * 例えば COM_VAL_STRLIST がそうしたケースとなっている。
 *
 * iFunc・iCopy・iFreeのプロトタイプは本記述の直下で宣言している。
 * iCopy・iFreeの第1引数が void**型であることに注意。
 */

/*
 * チェック関数プロトタイプ
 *   com_validator_t:
 *     ioData: チェック対象の文字列(チェック関数内で変更可能)。
 *     iCond:  チェック条件。関数登録時に条件も登録。
 *   com_valCondCopy_t:
 *     oCond:  コピー先。動的メモリ確保したアドレスを書き込む想定。
 *     iCond:  コピー元。条件の構造体でキャストする想定。
 *   com_valCondFre_t:
 *     oCond:  解放対象となる条件データ(*oCondの解放は不要)。
 */
typedef BOOL (*com_validator_t)( char *iData, void *iCond );
typedef BOOL (*com_valCondCopy_t)( void **oCond, void *iCond );
typedef void (*com_valCondFree_t)( void **oCond );

BOOL com_addCfgValidator(
        char *iKey, void *iCond, com_validator_t iFunc,
        com_valCondCopy_t iCopy, com_valCondFree_t iFree );

/*
 * 以後 COM_VAL_～ でマクロ宣言されるチェッカーの詳細を記述する。
 * このマクロは com_addCfgValidator()において iFunc, iCopy, iFree を
 * 個別に指定する代わりに使用することを想定している。
 *
 *   例：
 *     #define CFG_REPEAT_DISPLAY  "REPEAT_DISPLAY"
 *
 *     com_addCfgValidator( CFG_REPEAT_DISPLAY, NULL, COM_VAL_ONOFF );
 */

/*
 * long型数値チェック  COM_VAL_DIGIT・COM_VAL_DGTLIST
 *   チェック結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !ioData
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * ioDataが long型の10進数(つまり正負の数値)であるかどうかチェックする。
 * 数値変換できない文字が含まれていた場合は false を返す。
 *
 * iCondで条件を設定する。NULL指定時は上記の数値チェックのみになる。
 *
 * COM_VAL_DIGITの実際のチェック関数は com_valDigit()になる。
 * iCondは com_valCondDigit_t型のアドレスで、数値範囲を指定する。
 *
 * COM_VAL_DGTLISTの実際のチェック関数は com_valDgtList()になる。
 * iCondは com_valCondDgtList_t型のアドレスで、許容数値リストを指定する。
 * リスト個数(.count)が 0の場合は何もしないが、com_addCfgValidator()で登録を
 * するときにそうなっていたら、エラーになる。
 * 例えば以下のように condを定義し、&condを渡すことになるだろう。
 * 
 *     long list[] = { 1, 2, 3, 4 };
 *     com_valCondDgtList_t cond = { COM_ELMCNT(list), list };
 *
 * 実際のコンフィグ取得にあたっては、com_getCfgDigit()を使用することで、
 * 文字列の設定値を数値変換して得ることが出来る。
 */

////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_DIGIT \
    com_valDigit, com_valDigitCondCopy, NULL

// com_valDigit()用 条件指定構造体
typedef struct {
    long    min;     // 最小値 (ない場合は COM_VAL_NO_MINを指定する)
    long    max;     // 最大値 (ない場合は COM_VAL_NO_MAXを指定する)
} com_valCondDigit_t;

// min・maxで値を指定しない場合に使用するマクロ
#define  COM_VAL_NO_MIN   LONG_MIN    // [minに指定] 最小値はチェックしない
#define  COM_VAL_NO_MAX   LONG_MAX    // [maxに指定] 最大値はチェックしない

BOOL com_valDigit( char *ioData, void *iCond );
BOOL com_valDigitCondCopy( void **oCond, void *iCond );
// 解放は単純な com_free()で問題ない


////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_DGTLIST \
    com_valDgtList, com_valDgtListCondCopy, com_valDgtListCondFree

// com_valDgtList()用 条件指定構造体
typedef struct {
    long    count;   // リスト個数
    long*   list;    // 許容数値リスト
} com_valCondDgtList_t;

BOOL com_valDgtList( char *ioData, void *iCond );
BOOL com_valDgtListCondCopy( void **oCond, void *iCond );
void com_valDgtListCondFree( void **oCond );

/*
 * ulong型数値チェック  COM_VAL_UDIGIT・COM_VAL_UDGTLIST
 *   チェック結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !ioData
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * ioDataが ulong型になる以外は COM_VAL_DIGIT・COM_VAL_DGTLIST と同じ動作。
 * 数値変換できない文字が含まれていた場合は false を返す。
 *
 * iCondで条件を設定する。NULL指定時は上記の数値チェックのみになる。
 *
 * COM_VAL_UDIGITの実際のチェック関数は com_valUDigit()になる。
 * iCondは com_valCondUDigit_t型のアドレスで、数値範囲を指定する。
 *
 * COM_VAL_UDGTLISTの実際のチェック関数は com_valUDgtList()になる。
 * iCondは com_valCondUDgtList_t型のアドレスで、許容数値リストを指定する。
 *
 * 実際のコンフィグ取得にあたっては、com_getCfgUDigit()を使用することで、
 * 文字列の設定値を数値変換して得ることが出来る。
 */

////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_UDIGIT \
    com_valUDigit, com_valUDigitCondCopy, NULL

// com_valDigit()用 条件指定構造体
typedef struct {
    ulong   min;     // 最小値 (ない場合は COM_VAL_NO_UMINを指定する)
    ulong   max;     // 最大値 (ない場合は COM_VAL_NO_UMAXを指定する)
} com_valCondUDigit_t;

// min・maxで値を指定しない場合に使用するマクロ
#define  COM_VAL_NO_UMIN   0           // [minに指定] 最小値はチェックしない
#define  COM_VAL_NO_UMAX   ULONG_MAX   // [maxに指定] 最大値はチェックしない

BOOL com_valUDigit( char *ioData, void *iCond );
BOOL com_valUDigitCondCopy( void **oCond, void *iCond );
// 解放は単純な com_free()で問題ない


////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_UDGTLIST \
    com_valUDgtList, com_valUDgtListCondCopy, com_valUDgtListCondFree

// com_valUDgtList()用 条件指定構造体
typedef struct {
    long    count;   // リスト個数
    ulong*  list;    // 許容数値リスト
} com_valCondUDgtList_t;

BOOL com_valUDgtList( char *ioData, void *iCond );
BOOL com_valUDgtListCondCopy( void **oCond, void *iCond );
void com_valUDgtListCondFree( void **oCond );

/*
 * 16進数値チェック  COM_VAL_HEX・COM_VAL_HEXLIST
 *   チェック結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !ioData
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * ioDataが 16進数文字になる以外は COM_VAL_UDIGIT・COM_VAL_UDGTLIST と同じ動作。
 * 16進数値変換できない文字が含まれていた場合は false を返す。
 * 0x は付けても付けなくても認識可能。
 *
 * iCondで条件を設定する。NULL指定時は上記の数値チェックのみになる。
 *
 * COM_VAL_HEXの実際のチェック関数は com_valUDigit()になる。
 * iCondは com_valCondUDigit_t型のアドレスで、数値範囲を指定する。
 * (com_valCondHex_t型というのは無いので注意)
 *
 * COM_VAL_HEXLISTの実際のチェック関数は com_valUDgtList()になる。
 * iCondは com_valCondUDgtList_t型のアドレスで、許容数値リストを指定する。
 * (com_valCondHexList_t型というのは無いので注意)
 *
 * どちらも、コード上は 0x で始まる 16進表記で数値を記述すれば問題ない。
 *
 * 実際のコンフィグ取得にあたっては、com_getCfgUHex()を使用することで、
 * 文字列の設定値を16進数値変換して得ることが出来るが、これは com_setCfg()を
 * 使って設定した場合に限定される。
 * com_setCfgDigit()や com_setCfgUDigit()を使うと、たとえ引数で 0x を付けた
 * 16進表記をしても、あくまで同等の10進数値に変換されて保持になるため、
 * com_getCfgHex()で正しい取得はできず、com_getCfgDigit()や com_getCfgUDigit()
 * を使わないと正しい値が取得できない。
 */

////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_HEX \
    com_valHex, com_valUDigitCondCopy, NULL

// com_valHex()用 条件指定構造体は com_valCondUDigit_tを使用する

BOOL com_valHex( char *ioData, void *iCond );
// コピーは com_valUDigitCondCopy()を使用する
// 解放は単純な com_free()で問題ない


////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_HEXLIST \
    com_valHexList, com_valUDgtListCondCopy, com_valUDgtListCondFree

// com_valUDgtList()用 条件指定構造体は com_valCondUDgtList_tを使用する

BOOL com_valHexList( char *ioData, void *iCond );
// コピーは com_valUDgtListCondCopy()を使用する
// 解放は com_valUDgtListCondFree()を使用する

/*
 * 文字列チェック  COM_VAL_STRING・COM_VAL_STRLIST
 *   チェック結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !ioData
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 */

////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_STRING \
    com_valString, com_valStringCondCopy, NULL

// com_valString()用 条件指定構造体
typedef struct {
    size_t   minLen;   // 文字列最小長 (指定しないときは 0)
    size_t   maxLen;   // 文字列最大長 (指定しないときは 0)
} com_valCondString_t;

BOOL com_valString( char *ioData, void *iCond );
BOOL com_valStringCondCopy( void **oCond, void *iCond );
// 解放は単純な com_free()で問題ない

////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_STRLIST \
    com_valStrList, com_valStrListCondCopy, com_valStrListCondFree

// com_valStrList()用 条件指定構造体
typedef struct {
    BOOL    noCase;  // 大文字小文字を区別しない場合は true
    char**  list;    // 許容する文字列リスト (最後のデータは必ず NULLにする)
} com_valCondStrList_t;

BOOL com_valStrList( char *ioData, void *iCond );
BOOL com_valStrListCondCopy( void **oCond, void *iCond );
void com_valStrListCondFree( void **oCond );

/*
 * 特定文字列チェック  COM_VAL_BOOL・COM_VAL_YESNO・COM_VAL_ONOFF
 *   チェック結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !ioData
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * ioDataの内容が特定の文字列であることをチェックする。
 * 大文字小文字は区別しない。チェック対象文字列であれば true を返す。
 * iCond(条件)は Don't Careとなり、常に NULL指定が望ましい。
 *
 * COM_VAL_BOOLは "TRUE" か "FALSE" であることをチェックする。
 * 実際のチェック関数は com_valBool()になる。
 *
 * COM_VAL_YESNOは "YES" か "NO" であることをチェックする。
 * 実際のチェック関数は com_valYesNo()になる。
 *
 * COM_VAL_ONOFFは "ON" か "OFF" であることをチェックする。
 * 実際のチェック関数は com_valOnOff()になる。
 *
 * 実際のコンフィグ取得にあたっては、com_getCfgBool()を使えば、
 * "TRUE" "YES" "ON" の場合は trueを得られる。
 */

////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_BOOL \
    com_valBool, NULL, NULL

// 条件設定はない

BOOL com_valBool( char *ioData, void *iCond );
// 条件はないので、コピー/解放関数も不要。


////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_YESNO \
    com_valYesNo, NULL, NULL

// 条件設定はない

BOOL com_valYesNo( char *ioData, void *iCond );
// 条件はないので、コピー/解放関数も不要。


////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_ONOFF \
    com_valOnOff, NULL, NULL

// 条件設定はない

BOOL com_valOnOff( char *ioData, void *iCond );
// 条件はないので、コピー/解放関数も不要。

/*
 * コンフィグ設定  com_setCfg()・com_setCfgDigit()・com_setCfgUDigit()
 *   登録が成功した場合 0 を返す。つまり 非0 は登録失敗を意味する。
 *   1以上の数値が返った場合、それは何番目のチェック関数でNGだったかを示す。
 *   -1 が返った場合、メモリが足りなかったために設定できなかったことを示す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iKey
 *   COM_ERR_CONFIG: iKeyのコンフィグ登録なし・チェック関数NG
 *   データ登録時の com_strdup()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * iKeyで指定したコンフィグ設定に iDataで指定したデータを設定する。
 * I/Fの違いは iDataのデータ型だけで、最終的に文字列として保持する。
 *
 * iDataの内容をチェックしたい場合は、あらかじめ com_addCfgValidator()で
 * チェック関数を登録しておく。関数は複数登録可能で、その場合ひとつでもNGだと
 * コンフィグ設定はNGになり、何番目のチェック関数でNGになったかを返す。
 * 本I/F自体は「data is invalid」というエラーしか出力しないため、詳細な理由を
 * 出力したいときは、この返り値からその理由を判断して更に出力すると良い。
 * ANDではなく ORでの判定にしたい場合は、そうしたチェック関数を自作して、
 * チェック関数として登録することを検討する。
 *
 * なお、16進数値については com_setCfg()を使って登録することを推奨する。
 * 他でも出来なくはないが取得時に考慮が必要になる。詳細は COM_VAL_HEX を参照。
 */
long com_setCfg( char *iKey, char *iData );
long com_setCfgDigit( char *iKey, long iData );
long com_setCfgUDigit( char *iKey, ulong iData );

/*
 * コンフィグ取得  com_isEmptyCfg()・com_getCfg()・com_getCfgDigit()・
 *                 com_getCfgUDigit()・com_getCfgHex()・com_getCfgBool()
 *   コンフィグ値の内容を返す(型は各I/Fごとに異なる)
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iKey
 *   COM_ERR_CONFIG: iKeyのコンフィグ登録なし
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * iKeyで指定したコンフィグ設定の内容を取得する。
 * どの取得I/Fを使用するかは使う側が判断する必要がある。
 * 例えば TRUE/FALSE 以外の文字列が入る可能性のあるコンフィグ設定に対し、
 * com_getCfgBool()を使用して、正しい処理が出来る可能性は低いだろう。
 *
 * com_isEmptyCfg()は設定値なしなら true、ありなら falseを返す。
 * コンフィグ登録時に初期値を設定した場合、必ず falseを返す。
 *
 * com_getCfg()は設定されている文字列をそのまま返す。
 * 設定なしの場合 NULLではなく、空文字""を返す。
 * 設定有無を判定したいときは com_isEmptyCfg()を使用すること。
 *
 * com_getCfgDigit()・com_getCfgUDigit()は設定している文字列を数値変換して返す。
 * 数値変換できない文字が含まれていた場合は 0 を返す。
 * 設定なしの場合も 0 を返す。
 *
 * com_getCfgHex()は設定している文字列を16進数値変換して返す。
 * 単純に数値を返すだけなので、それを実際に16進表記で画面出力したいときは
 * %x や %X を出力時の書式に指定すること。
 * 数値変換できない文字が含まれていた場合は 0 を返す。
 * 設定なしの場合も 0 を返す。
 *
 * com_getCfgBool()は設定している文字列が "TRUE" "YES" "Y" "ON" の場合は true、
 * それ以外なら falseを返す。大文字小文字は区別しない。
 * 設定なしの場合は falseを返す。
 */
BOOL com_isEmptyCfg( char *iKey );       // 設定なしなら true
const char *com_getCfg( char *iKey );    // 文字列をそのまま返す
long com_getCfgDigit( char *iKey );      // long型数値に変換する
ulong com_getCfgUDigit( char *iKey );    // ulong型数値に変換する
ulong com_getCfgHex( char *iKey );       // 16進文字として、ulong型に変換する
BOOL com_getCfgBool( char *iKey );       // 内容に応じて true/false

/*
 * コンフィグ全取得  com_getCfgAll()
 *   まだデータ末尾に達していないときは true
 *   データ末尾に達していたら false を返す。
 *   また引数がひとつでも NULLだった場合も false を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !ioCount || !oKey || !oData
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * ioCountには 0で初期化した long型変数のアドレスを指定する。
 * oKeyはキー、oDataはコンフィグ値の文字列の格納先を指定する。格納されるのは
 * 文字列を保持先のアドレスなので、バッファを用意する必要はない。
 *
 * *ioCountをインデックスとしたコンフィグ内容を *oKey・*oDataに格納する。
 * 格納後、*ioCountをインクリメントして tureを返す。*ioCountは変更しないこと。
 * *oKey・*oDataのアドレスを呼び元で解放してはいけない(データが消失する)。
 * 
 * trueを返した場合、*ioCountは変更せずに、そのまま同じI/Fを呼ぶことを想定する。
 * これにより、次のデータが格納される。これ以上データが無ければ falseが返る。
 * 従って falseを返すまで このI/Fを呼び続けるというモデルになる。
 *
 * 以下は本I/Fを使って全コンフィグの文字列を出力するサンプルソースとなる。
 *
 *     long count = 0;
 *     const char *key, *data;
 *     while( com_getCfgAll( &count, &key, &data ) ) {
 *         com_printf( "%s = %s\n", key, data );
 *     }
 *
 * 上記処理だと、最終的な countの値は登録されているコンフィグの数となる。
 */
BOOL com_getCfgAll( long *ioCount, const char **iKey, const char **iData );



/*
 *****************************************************************************
 * COMCONV:データ変換関連I/F (com_proc.c)
 *****************************************************************************
 */

/*
 * 大文字・小文字変換  com_convertUpper()・com_convertLower()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * com_convertUpper()は、ioStringの文字列を一文字ずつ upper()で変換する。
 * com_convertLower()は、ioStringの文字列を一文字ずつ lower()で変換する。
 */
void com_convertUpper( char *ioString );
void com_convertLower( char *ioString );

/*
 * 文字列-数値変換  com_strtoul()・com_strtol()・com_strtof()・com_strtod()
 *   com_strtoul()は strtoul()を使い、結果を unsigned long型で返す。
 *   com_strtol()は strtol()を使い、結果を long型で返す。
 *   com_strtof()は strtof()を使い、結果を float型で返す。
 *   com_strtod()は strtod()を使い、結果を double型で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iString
 *   COM_ERR_CONVERTNG: 変換エラー発生(iError==false時は出ない)
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * iStringの文字列を、基数 iBaseで数値変換する。
 * (com_strtof()と com_strtod()は iBaseを持たない)
 * 呼んだ直後の errnoが 0であれば、エラー無しで変換されたと判定可能。
 *
 * iErrorが trueの場合、変換失敗時にエラー出力する。
 * ・iStringに不正な文字が入っていた場合
 *    本来の strtoul()や strtol()では errnoを設定しないが、
 *    異常検出のために、敢えて errnoに EINVAL を設定する。
 *    iErrorが falseであっても、この errno設定は行う。
 * ・変換時 errnoに 0以外が設定された場合(主に結果が範囲外の時の ERANGE)
 *    その errnoが保持される。
 * 異常を検出するまでに変換された内容は残す。
 *
 * iErrorが falseの場合、上記の状況になってもエラーの出力はしないが、
 * errnoは設定するため、呼び元で変換結果の成否はチェック可能。
 */
ulong  com_strtoul( const char *iString, int iBase, BOOL iError );
long   com_strtol(  const char *iString, int iBase, BOOL iError );
float  com_strtof( const char *iString, BOOL iError );
double com_strtod( const char *iString, BOOL iError );

/*
 * atoi()代替I/F  com_atoi()
 * atol()代替I/F  com_atol()
 * atof()代替I/F  com_atof()
 *   変換結果を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iString
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * 実際は com_strtol()・com_strtof()を内部で使用して、
 * 文字列数値変換を実施し、その結果を返す。基数は 10固定ということになる。
 * 呼んだ直後の errnoを見れば変換NGを検出可能。
 */
int   com_atoi( const char *iString );
long  com_atol( const char *iString );
float com_atof( const char *iString );

/*
 * 文字列-バイナリ数値列変換  com_strtooct()
 *   変換成否を true/false で返す。
 *   結果を格納する *ioOctはメモリ確保した場合、com_free()で解放すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iString || !ioOct || !ioLength
 *   バイナリ数値列格納のための com_malloc()のエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * iStringで与えられた文字列をバイナリのオクテット列に変換し、*ioOctに格納する。
 * ioLengthに変換したオクテット数を格納する。
 *
 * *ioOctが NULLの場合、オクテット列を保持するためのメモリ捕捉を実施する。
 * (本I/F使用後、捕捉したメモリは com_free()で解放すること)
 * *ioOctに自分で用意したバッファのアドレスを入れて使用することも可能で、
 * その場合は *ioLengthにそのバッファサイズを設定しておくこと。この場合、
 * 本I/F内でバッファのメモリ捕捉は実施しない。もし変換したオクテット長が
 * バッファサイズを超えていた場合は、バッファサイズまで保持して残りは捨てる。
 *
 *  ＊＊ 注意 ＊＊
 *   buf[]というバッファを ioOctに指定する場合 &bufは正しくない。
 *   配列変数の場合 buf と指定しても &buf と指定しても同じ意味になり
 *   敢えて言うなら uchar **型ではなく uchar *型扱いになるためである。
 *   これを解決するためには uchar* tmp = buf; というようにポインタ変数を用意し
 *   &tmp を ioOctとして指定する方法が考えられる。
 *   ポインタ変数と配列変数の構造上の違いによる注意事項となる。
 *
 * 正しく変換できない場合は falseを返す。
 * ・16進文字以外が入っていた場合 (空白や制御文字は読み飛ばすのでOK)
 * ・16進文字数が奇数だった場合
 * この場合、一切変換は行われず、*ioOctには NULLが入る。
 *
 * 生成されたバイナリ数値列の出力処理としては com_printBinary()が使える。
 * デバッグ目的であれば com_dump()も使えるだろう。
 * 無論自分で必要な書式に編集して出力する処理を書くのも良い。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_strtooct( const char *iString, uchar **ioOct, size_t *ioLength );
 */
#define com_strtooct( STRING, OCT, LENGTH ) \
    com_strtooctFunc( (STRING), (OCT), (LENGTH), COM_FILELOC )

BOOL com_strtooctFunc(
        const char *iString, uchar **ioOct, size_t *ioLength, COM_FILEPRM );

/*
 * 数値-文字列変換  com_ltostr()・com_ultostr()・com_bintohex()
 *                  COM_LTOSTR()・COM_ULTOSTR()・COM_BINTOHEX()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   (そのためにマクロによる変換格納処理を行っている)
 * ===========================================================================
 * com_ltostr()・com_ultostr()は iValueの数値をそのまま10進数値として文字列
 * 変換して、oBufに格納する。oBufのサイズは COM_LONG_SIZE以上確保すること。
 * (桁数が明確に分かっている場合、そのサイズでも十分という意見もあるだろうが
 *  厳密なコンパイルでは、起こり得るサイズの確保を求められる)
 *
 * com_bintohex()は iBinの数値を16進数値(2桁・0フィル)として文字列変換し、
 * oBufに格納する。oBufのサイズは COM_BIN_SIZE以上確保すること。
 * iCaseが trueの場合 A～F、falseの場合 a～fを使用する。
 *
 * 全て大文字のマクロも用意している。引数は実関数と同じになるが、
 * 第1引数については、定義されていない変数名を指定する必要がある。
 * このマクロの中で、その変数を定義し、そこに結果を格納する。
 *
 * 以下、マクロを使用した時のコードサンプルになる。
 *
 *     long value = sampleCalc();
 *     COM_LTOSTR( result, value );
 *     com_printf( "result = %s\n", result );
 *
 * マクロを使用すると、その場で結果格納用の変数定義文が入る。そのため、C99より
 * 前のC言語規格では、関数冒頭(正確にはブロック冒頭)以外しか変数定義は許容しない
 * ことからコンパイルエラーになる。これを回避するには実関数の使用を検討する。
 */

// 文字列サイズの定数宣言
enum {
    COM_LONG_SIZE = 21,    // long型数値の最大桁数 + 1
    COM_BIN_SIZE  = 3      // 16進2桁 + 1
};

#define COM_LTOSTR( VARNAME, VALUE ) \
    char VARNAME[COM_LONG_SIZE] = {0}; \
    com_ltostr( VARNAME, (VALUE) );

void com_ltostr( char *oBuf, long iValue );

#define COM_ULTOSTR( VARNAME, VALUE ) \
    char VARNAME[COM_LONG_SIZE] = {0}; \
    com_ultostr( VARNAME, (VALUE) );

void com_ultostr( char *oBuf, ulong iValue );

#define COM_BINTOHEX( VARNAME, VALUE, CASE ) \
    char VARNAME[COM_BIN_SIZE] = {0}; \
    com_bintohex( VARNAME, (VALUE), (CASE) );

void com_bintohex( char *oBuf, uchar iValue, BOOL iCase );

/*
 * バイナリ数値列-文字列変換  com_octtostr()
 *   処理結果を true/false で返す。
 *   *oBufは呼び元で使用後に com_free()で解放が必要。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oBuf || !iOct
 *   変換後の文字列格納のための com_malloc()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * バイナリ数値列を iOct、長さを iLengthで与え、文字列変換して *oOctに書き込む。
 * iCaseが trueの場合 16進数で A-F、falseの場合 16進数で a-f を使う。
 *
 * *oBufを NULLにした場合、本I/Fでメモリを確保し、そのアドレスを設定する。
 * この場合 iBufSizeの内容は Don't Careとなる。
 * *oBuf使用後は com_free()で解放すること。
 *
 * 既存バッファを指定することも可能で、その場合は iBufSizeにそのサイズを
 * 設定する。ただし char[]型バッファを oBufに指定するのは工夫が必要。
 * この事情は com_strtooct()の説明で「＊＊ 注意 ＊＊」に記載したとおりで、
 * char*型の一時変数の介在がどうしても必要になる。
 * もしバッファサイズが不足した場合、指定されたサイズの範囲で可能な限り
 * データを出力して falseを返す。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_octtostr( char **oBuf, size_t iBufSize,
 *                      void *iOct, size_t iLength, BOOL iCase );
 */
#define com_octtostr( BUF, BUFSIZE, OCT, LENGTH, CASE ) \
    com_octtostrFunc( (BUF), (BUFSIZE), (OCT), (LENGTH), (CASE), COM_FILELOC )

BOOL com_octtostrFunc(
        char **oBuf, size_t iBufSize, void *iOct, size_t iLength, BOOL iCase,
        COM_FILEPRM );



/*
 *****************************************************************************
 * COMUSTR:文字列ユーティリティI/F (com_proc.c)
 *****************************************************************************
 */

/*
 * 長さ指定文字列コピー  com_strncpy()
 *   バッファサイズを超すコピーが発生した時に falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oTarget || !iSource || iTargetSize <= 1
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * oTargetに iSourceの文字列をコピーする。
 * iTargetSizeは oTargetのバッファサイズ、iCopyLenは iSourceからコピーする
 * 文字列長を指定する。
 * iCopyLenが iSourceの文字列長より大きい場合、iSource全てがコピー対象となり
 * その文字列長を iCopyLenの値として処理を進める。標準関数 strncpy()のように
 * iSource全体をコピーした後 \0 で埋める処理はしない。
 *
 * iTargetSize - 1を超すようなコピーになる場合、iTargetSize - 1文字までを
 * コピーし、最後に '\0' を付与して終端させる。これが発生したら falseを返す。
 * これはメモリ破壊を防ぐための最終手段であり、呼び元はこれが起きないように
 * 十分な大きさの iTargetSizeを持つ oTargetを指定するようにする方が望ましい。
 *
 * ＊標準関数 strncpy()は iTargetSizeの引数を持たず「コピーを受け取るのに
 *   十分な大きさでなければならない」という注意書きのみになっている。
 *   もし大きさが足りなかった場合はメモリ破壊となる。
 */
BOOL com_strncpy(
        char *oTarget, size_t iTargetSize,
        const char *iSource, size_t iCopyLen );

/*
 * 派生マクロ  com_strcpy()
 * ---------------------------------------------------------------------------
 * com_strncpy()を流用して、strcpy()と同じ引数の形で、コピー先バッファサイズを
 * 考慮したコピーを実施する。sizeof(TARGET)で正しくサイズが得られる必要がある為
 * 全ての strcpy()を置き換えられるとは限らない。
 * コピー先バッファサイズが他の手段で与えられている場合、com_strncpy()を使用し、
 * iTargetSizeにそのバッファサイズ、iCopyLenは strlen()で指定すれば良い。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_strcpy( char *oTarget, const char *iSource );
 */
#define com_strcpy( TARGET, SOURCE ) \
    com_strncpy( (TARGET), sizeof(TARGET), (SOURCE), strlen(SOURCE) )

/*
 * 長さ指定文字列連結  com_strncpy()
 *   バッファサイズを超す連結が発生した時に falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG]
 *      !oTarget || !iSource || iTargetSize <= 1 || iCatLen > iTargetSize + 1
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * oTargetの文字列に iSourceの文字列を連結する。
 * iTargetSizeは oTargetのバッファサイズ、iCatLenは iSourceから連結する
 * 文字列長を指定する。iCatLenを 0にすると、iSource全てを連結する。
 * (その場合は、派生マクロ com_strcat()の方が使いやすいかもしれない)
 * iCatLenが iSourceの文字列長より大きい場合、iSource全てが連結対象となり
 * その文字列長を iCopyLenの値として処理を進める。
 *
 * iTargetSize - 1を超すような連結になる場合、iTargetSize - 1文字までを
 * 連結し、最後に '\0' を付与して終端させる。これが発生したら falseを返す。
 * これはメモリ破壊を防ぐための最終手段であり、呼び元はこれが起きないように
 * 十分な大きさの iTargetSizeを持つ oTargetを指定するようにする方が望ましい。
 *
 * ＊標準関数 strncat()は iTargetSizeの引数を持たず「連結後の結果を格納するのに
 *   十分な大きさでなければならない」という注意書きのみになっている。
 */
BOOL com_strncat(
        char *oTarget, size_t iTargetSize,
        const char *iSource, size_t iCatLen );

/*
 * 派生マクロ  com_strcat()
 * ---------------------------------------------------------------------------
 * com_strncat()を流用して、strcat()と同じ引数の形で、連結先バッファサイズを
 * 考慮した連結を実施する。sizeof(TARGET)で正しくサイズが得られる必要がある為
 * 全ての strcat()を置き換えられるとは限らない。
 * 連結先バッファサイズが他の手段で与えられている場合、com_strncat()を使用し、
 * iTargetSizeにそのバッファサイズ、iCopyLenは strlen()で指定すれば良い。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   BOOL com_strcat( char *oTarget, const char *iSource );
 */
#define com_strcat( TARGET, SOURCE ) \
    com_strncat( (TARGET), sizeof(TARGET), (SOURCE), strlen(SOURCE) )

/*
 * 書式指定文字列追加  com_connectString()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oBuf || !iBufSize || !iFormat
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iFormat以下で指定した文字列を oBufに入っている文字列に連結する。
 * iForamt以下の指定は、printf()と同様の出力書式が使用可能。
 * iBufSizeは oBufのバッファサイズで、サイズ内に追加が収まれば trueを返す。
 * それを超えるような追加があった場合、バッファサイズぎりぎりまで追加した上で
 * falseを返す。
 * 実際の文字列の追加には com_strncat()を利用している。
 */
BOOL com_connectString(
        char *oBuf, size_t iBufSize, const char *iFormat, ... );

/*
 * 文字列検索  com_searchString()
 *   見つかった文字列のポインタ一を返す。見つからなかった場合は、NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iSource || !iTarget
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * iSourceの文字列の中にある iTargetの位置をポインタで返す。
 *
 * *ioIndexで数値を指定した場合、複数ある場合にその順番の位置を返す。
 * もし ioIndexが NULL または *ioIndex が 0 か 1 だった場合は、最初に見つかった
 * 位置を返す。これは標準関数の strstr()と同じ結果を意味する。
 *
 * *ioIndexに COM_SEARCH_LASTを設定しておくと、一番最後に見つかった位置を返す。
 *
 * その後 ioIndex指定あり(非NULL)の場合、iTargetが見つかった個数を *ioIndexに
 * 格納して処理を終了する。つまり *ioIndexはI/F呼び出し前と違う値になり得る。
 *
 * iSearchEndに 0以外の値を指定した場合、iSourceの先頭から、そのサイズまでの
 * データを文字列として検索する処理になる。その前に文字列終端(0x00)があったら
 * そこで検索は終了する。
 * 0を指定した場合、iSourceは文字列終端(0x00)で終わることを想定しているが、
 * com_custom.hにて宣言した COM_SEARCH_LIMIT の長さまで検索したら、
 * デバッグNGエラーで警告を出して検索終了とする。明確にそれより大きなサイズの
 * 検索をさせたい場合、COM_SEARCH_LIMIT の値を変更すること。
 *
 * iNoCaseを trueにした場合、英字は大文字・小文字を区別せずに検索する。
 */

#define  COM_SEARCH_LAST  -1   // 最後に見つかった文字列を返して欲しい時の指定

char *com_searchString(
        const char *iSource, const char *iTarget, long *ioIndex,
        size_t iSearchEnd, BOOL iNoCase );

/*
 * 文字列置換  com_replaceString()
 *   置換された数を返す(ひとつも置換されなければ 0)
 *   処理に失敗した場合は COM_REPLACE_NG を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG]
 *           !iSource || !oTarget || !oLength || !iCond
 *           !iCond.replacing || !iCond.replaced ||
 *           !strcmp( iCond.reeplacing, iCond.replaced )
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iSourceで指定した文字列から、iCond.replacingで指定した文字列を検索し、
 * iCond.replacedで指定した文字列に変換する。
 * 置換結果文字列の *oTargetに、その文字列長を *oTargetLengthに格納し、
 * 置換回数を返却する。
 *
 * 置換結果を格納するバッファは共有のため、そのメモリ解放は不要だが、
 * 以後も使用したい場合は、返却後速やかに別バッファに内容をコピーすること。
 * この共有バッファのサイズは COM_TEXTBUF_SIZE であり、置換後の文字列長が
 * これを超える場合、処理は失敗となり、COM_REPLACE_NG を返す。
 *
 * iCond.replacingで示した文字列が複数ある場合、
 * iCond.indexで 1以上が指定されていれば、その順番で見つかったもののみ置換する。
 * 全て置換したい場合は iCond.indexに COM_REPLACE_ALL を指定する。
 *
 * strstr()等の標準関数を使用する都合上、以下の制限がある。
 * ・iSource、iCond.replacing、iCond.replacedは、いずれも '\0'で終端する文字列
 *   になっていなければならない。
 */
enum {
    COM_REPLACE_NG = -1,   // 置換処理でNGが発生したときの返り値
    COM_REPLACE_ALL = 0    // iIndexで全置換を指定するための値
};

typedef struct {
    const char*  replacing;  // 置換前文字列
    const char*  replaced;   // 置換後文字列
    long         index;      // インデックス値
} com_replaceCond_t;

long com_replaceString(
        char **oTarget, size_t *oLength, const char *iSource,
        com_replaceCond_t *iCond );

/*
 * 文字列比較  com_compareString()
 *   一致するなら true、一致しないなら falseを返す。
 * ---------------------------------------------------------------------------
 *   文字列変換のための cmo_strdup()によるエラー (大文字小文字区別しない場合)
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iString1 と iString2 を比較し、一致するかチェックする。
 * iCmpLenが 0なら文字列全体で比較し、1以上なら 先頭からその長さ分を比較する。
 * iNoCaseが trueの場合、大文字小文字を区別せずに比較する。
 */
BOOL com_compareString(
        const char *iString1, const char *iString2,
        size_t iCmpLen, BOOL iNoCase );

/*
 * 先頭文字列位置  com_topString()
 *   ホワイトスペースを除いた文字位置のポインタを返す。
 *   ホワイトスペースのみの場合 NULLを返す。
 *   改行の扱いについては以下を参照。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iString
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iStringで受け取った文字列を先頭からチェックし、空白以外の文字が見つかったら
 * その最初の位置をポインタで返す。
 *
 * この「空白」には スペース ' '、水平タブ '\t'、垂直タブ '\v' が含まれる。
 * iCrLfが trueの場合は '\r' '\n' も含む。
 * iCrLfが falseの場合は 改行文字 '\r' '\n' も空白ではない文字と判定し、
 * それが最初の空白以外の文字ならその位置のポインタを返す。
 *
 * 空白だけで文字列終端まで到達した場合は NULLを返す。
 */
char *com_topString( char *iString, BOOL iCrLf );

/*
 * 複合文字チェック  com_checkString()
 *   条件が合致したら true、条件が合致しなかったら falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iString || !iCheckFuncs
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * ctype.hで宣言された文字チェック関数を複合して、文字列のチェックを実施する。
 * iStringがチェック対象の文字列となる。
 *
 * iCheckFuncsには int is～(int c); で宣言されたチェック関数(isalnum()等)を
 * そのまま並べて、最後をNULLをにした線形データを指定する。これにより複数の
 * チェック関数を指定可能となる。以下が設定例：
 *
 *     com_isFunc_t funcs[] = { isalnum, isblank, NULL };
 *     (この funcsをそのまま iCheckFuncsに指定する)
 *
 * これにより iStringを先頭から一文字ずつ指定したチェック関数に掛ける。
 * プロトタイプが com_isFunc_t と一致していれば、独自関数を使っても良い。
 *
 * iOpはチェック内容を指定する(チェック内容は COM_CHECK_OP_tの宣言を山椒)。
 * 条件が満たされれば trueを返し、満たされなければ falseを返す。
 *
 * なお、文字列内容のチェックという機能に関しては、コンフィグ機能で定義した
 * チェック関数(com_val～()という名前の関数群)も使用できる。
 * 例えば targetが数値文字列かチェックしたければ、
 *
 *     com_valDigit( target, NULL )
 * 
 * が使用できるだろう。
 */

// チェック関数プロトタイプ宣言
typedef int(*com_isFunc_t)( int c );

// チェック内容指定値
typedef enum {
    COM_CHECKOP_AND,    // 全て合致したら 真
    COM_CHECKOP_OR,     // どれかひとつでも合致したら 真
    COM_CHECKOP_NOT     // どれにも合致しなかったら 真
} COM_CHECKOP_t;

BOOL com_checkString(
        const char *iString, com_isFunc_t *iCheckFuncs, COM_CHECKOP_t iOp );

/*
 * 出力書式文字列-文字列ポインタ  com_getString()
 *   生成した文字列へのポインタを返す。(解放不要：下記参照)
 * ---------------------------------------------------------------------------
 *   生成文字列格納用の com_setBuffer()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 引数は printf()等と同じ出力書式を使用できる。それによって生成された文字列を
 * 内部バッファに保持し、その文字列アドレスを返す。このアドレスは解放不要。
 * バッファ数は com_custom.hの COM_FORMSTRING_MAX で宣言しており。その数までは
 * 保持可能。つまり一つの処理文の中でも、その回数までは並列使用が可能。
 *
 * この関数の引数で指定された書式文字列の編集に使用する文字列バッファサイズは
 * COM_TEXTBUF_SIZEなので、それを超える文字列は編集できない。もし超えた場合、
 * 超えた部分はカットされる(メモリ破壊はしない)。
 *
 * ＊本I/Fは 文字列(char *型)が求められている箇所に
 *   snprintf()等を使って整形した文字列を指定したい、というニーズに応える。
 *   文字列バッファを用意して、snprintf()を使った結果を格納し、その文字列
 *   バッファを指定する代わりに、本I/Fの返り値がそのまま使える。
 *
 *   ただし整形した結果を更に使いたい(変更することも含む)場合は、
 *   本I/Fを使わずに、そのための文字列バッファを用意し自分で整形した結果を
 *   使うほうが望ましい。本I/Fの内部バッファは数が限られており、他で使われて
 *   内容がいつ変更されるかわからないためである。
 */
char *com_getString( const char *iFormat, ... );

/*
 * 文字列リスト検索  com_searchStrList()
 *   リスト内の文字列と一致する場合、そのインデックス値(先頭が 0)を返す。
 *   見つからない場合は COM_NO_DATA を返す。エラー時も COM_NO_DATA を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iList || !iTarget
 *   COM_ERR_DEBUGNG [com_prmNG] !(*iList)
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iListは最終データが NULLになった文字列ポインタリストを指定する。
 * その中の文字列が iTargetと一致したら、そのインデックス値(リスト先頭が 0)を
 * 返す。この際 iNoCaseが trueなら英文字の大文字小文字を区別しない比較をする。
 */

#define  COM_NO_DATA  -1    // 指定文字列が見つからない時に返す

long com_searchStrList(
        const char **iList, const char *iTarget, BOOL iNoCase );



/*
 *****************************************************************************
 * COMUTIME:時間ユーティリティI/F (com_proc.c)
 *****************************************************************************
 */

/*
 * 2036年問題について
 *   NTPのタイムスタンプは 1900年1月1日00:00:00を起点とした累積秒数である。
 *   このため2036年に32bitの最大値に達して、それ以上増やせなくなる。
 *   SNTPv4を規定した RFC4330で最上位ビットが 0の場合は一回りして、
 *   起点を 2036年2月7日06:28:16 として扱う対処案が示されている。
 *   (今時点で最上位ビットは 1 になっているため区別はつく)
 *
 *   Wiresharkは既にこれを加味したタイムスタンプのデコードを実施しているが
 *   toscomでは標準関数 localtime()や gmtime()が対応しない限り、
 *   この対処案は実施されない。2036年にこれらが使われているかどうかも疑問。
 *
 *   ちなみに UNIXの時刻は 1970年1月1日00:00:00 を起点とした累積秒数で、
 *   こちらは 2038年に同様の問題が発生することになる。
 */

/*
 * GMT と UTC
 *   GMT＝グリニッジ標準時(Greenwitch meen time)は、経度0度の英国グリニッジの
 *   地方平均時を指す。これが世界的な標準時間とされてきた。
 *
 *   今はうるう秒を使って調整する原子時計による標準時間が制定されており、
 *   これが UTC＝世界協定時(coordinated universal time)と呼ばれている。
 *   (うるう秒を使った調整が必要なのは、地球の自転により原子時計といえど狂う為)
 *
 *   実質的な時間は GMT と UTC で同じになる。
 *   UNIX/Linuxの時間に関連する標準関数は古くから使われているものなので、
 *   基本的に GMT という表記を使用している。
 *
 *   ちなみに日本標準時(JST)は GMT/UTC +09:00 となる。
 */

/*
 * 日時取得  com_getCurrentTime()・com_getCurrentTimeGmt()
 *   処理成否を true/false で返す。
 *   falseが返った場合、データは一切設定されず不定なので使わないこと。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG]
 *          iType != COM_FORM_DETAIL && iType != COM_FORM_SIMPLE
 *   COM_ERR_TIMENG: gettimeofday()失敗
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iTypeで出力書式を指定。
 * oDateは日付文字列、oTimeは時刻文字列、oValは数値データの格納先を指定する。
 * (yearの値は詳細出力(4桁)/簡易出力(2桁)に示された形式の数値となる)
 * 3つのデータ全てが必要ではない場合、不要なデータには NULLを設定する。
 *
 * oDateと oTimeの必要サイズは下記に記載するので呼ぶ前に確保しておくこと。
 * oValは任意の書式で日時を出力したいときなどに使用することを想定し、データを
 * 格納するための実体を呼び元で用意し、そのアドレスを指定する。
 *
 * com_getCurrentTime()は localtime()を使うため、日本標準時になるはず。
 * com_getCurrentTimeGmt()は gmtime()を使うため、日本標準時 - 9時間 になる。
 *
 * gettimeofday()の生データが欲しい場合、com_gettimeofday()の使用を検討する。
 */

// 出力書式
typedef enum {
    COM_FORM_DETAIL,  // 詳細出力：日付 YYYY/MM/DD・時刻 hh:mm:ss.uuuuuu
    COM_FORM_SIMPLE   // 簡易出力：日付 YYMMDD・時刻 hhmmss
} COM_TIMEFORM_TYPE_t;

// 日時格納データサイズ(出力書式によって変わる)
enum {
    // COM_FORM_DETAIL時のサイズ
    COM_DATE_DSIZE = 11,   // oDateに必要なサイズ
    COM_TIME_DSIZE = 16,   // oTimeに必要なサイズ
    // COM_FORM_SIMPLE時のサイズ
    COM_DATE_SSIZE = 7,    // oDateに必要なサイズ
    COM_TIME_SSIZE = 7     // oTimeに必要なサイズ
};

// 共通日時データ(struct tmとは 年・月の値の持ち方が違う点に注意
//  time_t型・suseconds_t型のどちらも long型のため
//  出力時の書式設定は %d ではなく %ld が適正になる。
typedef struct {
    time_t  year;         // 年 (1900-)
    time_t  mon;          // 月 (1-12)
    time_t  day;          // 日 (1-31)
    time_t  wday;         // 曜日 (後述の COM_WD_TYPE_t が使用可能
    time_t  hour;         // 時 (0-23)
    time_t  min;          // 分 (0-59)
    time_t  sec;          // 秒 (0-59)
    suseconds_t  usec;    // マイクロ秒 (Cent OS 6.2では long型)
} com_time_t;

BOOL com_getCurrentTime(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal );

BOOL com_getCurrentTimeGmt(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal );

/*
 * gettimeofday代替関数  com_gettimeofday()
 *   gettimeofday()が 0を返したら true、-1を返したら falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oTime
 *   COM_ERR_TIMENG: gettimeofday()失敗
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * gettimeofday()を実施し、その結果が oTimeに格納される。
 * もしこれが -1を返した場合、エラーとなり、その旨のメッセージが
 * "fail to get ～[error値]" となる。iLabelがこの ～ の部分の文字列となり、
 * NULL指定時は "current time" となる。
 *
 * 本I/Fは gettimeofday()により struct timeval型の生データを取得したいときに
 * 使用する。これで取得したデータは com_setTimeval()で整形可能ではあるが、
 * そうするぐらいなら、com_getCurrentTime()を使った方が楽かもしれない。
 * こちらは内部で本I/Fを呼んで、データ整形している。
 * それ以外の目的で struct timeval型のデータが必要なときに本I/Fを使う想定。
 */
BOOL com_gettimeofday( struct timeval *oTime, const char *iLabel );

/*
 * 任意時間データ整形  com_setTime()・com_setTimeval()
 *                     com_setTimeGmt()・com_setTimevalGmt()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG]
 *          iType != COM_FORM_DETAIL && iType != COM_FORM_SIMPLE
 *   COM_ERR_TIMENG: localtime()失敗
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * 使い方は com_getCurrentTime()と同じ。
 * 対象が現在日時ではなく、引数 iTime または iTv で与えられるデータになる点が
 * 異なる。
 *
 * com_setTime()・com_setTimeGmt()は time_t型の iTimeを整形する。
 * 詳細出力(COM_FORM_DETAIL)指定時のマイクロ秒は 0固定となる。
 *
 * com_setTimeval()・com_setTimevalGmt()は struct timeval型の iTvを整形する。
 * こちらはマイクロ秒の出力にも対応する。
 *
 * com_setTime()・com_setTimeval()は localtime()を使った変換を行い、
 * com_setTimeGmt()・com_setTimevalGmt()は gmtime()を使った変換を行う。
 */
void com_setTime(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal,
        const time_t *iTime );

void com_setTimeval(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal,
        const struct timeval *iTv );

void com_setTimeGmt(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal,
        const time_t *iTime );

void com_setTimevalGmt(
        COM_TIMEFORM_TYPE_t iType, char *oDate, char *oTime, com_time_t *oVal,
        const struct timeval *iTv );

/*
 * 時間計測開始  com_startStopwatch()
 *   処理成否を true/false で返す。
 *   falseが返った場合、そのoWatchを引数に com_checkStopwatch()を使わないこと。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oTime
 *   COM_ERR_TIMENG: gettimeofday()失敗
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * com_stopwatch_t型のデータを定義し、そのアドレスを oWatchに設定する。
 * 本I/Fを呼んだ時点の時間を開始時間として oWatch->startに格納する。
 * 既に過去にデータ格納済みの場合は、上書きする。
 *
 * oWatch->startは struct timeval型で、これを通常の日時などに変換するには
 * com_setTimeval()が使用できる。
 */

// 時間計測データ
typedef struct {
    struct timeval  start;      // 開始時間
    struct timeval  passed;     // 経過時間
} com_stopwatch_t;

BOOL com_startStopwatch( com_stopwatch_t *oWatch );

/*
 * 経過時間計測  com_checkStopwatch()
 *   処理結果を true/false で返す。
 *   falseが返った場合、oWatch->passedの値は不定なので使ってはいけない。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oTime
 *   COM_ERR_TIMENG: gettimeofday()失敗
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * com_startStopwatch()で使用したのと同じデータアドレスを oWatchに指定する。
 * com_startStopwatch()で記録した開始時間から、経過した時間を oWatch->passedに
 * 格納する。複数回呼べば、その都度 開始時間からのその時点の値で上書きする。
 *
 * oWatch->passedも struct timeval型だが、oWatch->startから経過した時間
 * という表現になるため、com_setTimeval()でのぞみの結果が得られるかは疑わしい。
 * この構造体の定義は
 *     struct timeval {
 *         time_t       tv_sec;
 *         suseconds_t  tv_usec;
 *     };
 * で、秒(tv_sec)とマイクロ秒(tv_usec)という2つのデータで構成される。
 * 単純に秒数を出力するだけであれば、com_printf()等を使い、
 *     "%ld.%06ld", watch.passed.tv_sec, watch.passed.tv_usec
 * という出力書式で正しく表示が可能となる(watchが com_stopwatch_t型の場合)。
 *
 * com_convertSecond()を使うことで、日時＋時分秒＋マイクロ秒 の変換計算が
 * 可能なので、必要があれば使うと良い。
 */
BOOL com_checkStopwatch( com_stopwatch_t *ioWatch );

/*
 * 秒-日時＋時間変換  com_convertSecond()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iTimevalで渡された 秒＋マイクロ秒のデータを計算して、
 * 日数＋時分秒＋マイクロ秒 のデータに変換する。
 * 出力先の *oTimeのメンバーのうち、year・month・wday は常に 0になり、
 * day・hour・min・sec・usec に変換結果を格納する。
 */
void com_convertSecond( com_time_t *oTime, const struct timeval *iTimeval );

/*
 * 曜日計算  com_getDotw()
 *   正しく計算できた場合は COM_WD_SUNDAY～COM_WD_SATURDAY を返す。
 *   入力に問題があった場合(下記参照)は COM_WD_NG を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG]
 *       iYear <= 1752 || iMonth < 1 || iMonth > 12 || iDay < 1 || iDay > 31
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iYear・iMonth・iDayで指定した年月日の曜日(Dotw = Day of the week)を計算する。
 * ＊これらの変数を敢えて time_t型にしているのは、com_time_t型に合わせるため。
 *   time_t型は long型で typedefされているため、32bit/64bitでサイズが変わる。
 *
 * iYearは 1753年以上でなければならない。
 * 1752以下を指定した場合は COM_WD_NG を返す。
 * ありえない日付のチェックはそこまで厳密にはしておらず、正しい年月日が指定
 * されることを期待する。
 */

typedef enum {
    COM_WD_SUNDAY    = 0,  // 日曜日
    COM_WD_MONDAY    = 1,  // 月曜日
    COM_WD_TUESDAY   = 2,  // 火曜日
    COM_WD_WEDNESDAY = 3,  // 水曜日
    COM_WD_THURSDAY  = 4,  // 木曜日
    COM_WD_FRIDAY    = 5,  // 金曜日
    COM_WD_SATURDAY  = 6,  // 土曜日

    COM_WD_NG = -1     // 入力引数に問題があり計算ができない場合の返り値
} COM_WD_TYPE_t;

COM_WD_TYPE_t com_getDotw( time_t iYear, time_t iMonth, time_t iDay );

/*
 * 曜日文字列取得  com_strDotw()
 *   曜日に対応する文字列ポインタを返す(解放は不要)。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG]
 *       iDotw < COM_WD_SUNDAY || iDotw > COM_WD_SATURDAY
 *       iType < COM_WDSTR_JP1 || iType > COM_WDSTR_EN2
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iDotwで示した曜日(Dorw = Day of the week)に対応する文字列を返す。
 * 文字列のセットは4種類あり(下記定義参照)、iTypeで指定する。
 */

// 曜日文字列種別
typedef enum {
    COM_WDSTR_JP1 = 1,
       // 日,月,火,水,木,金,土
    COM_WDSTR_JP2 = 2,
       // 日曜日,月曜日,火曜日,水曜日,木曜日,金曜日,土曜日
    COM_WDSTR_EN1 = 3,
       // Sun,Mon,Tue,Wed,Thu,Fri,Sat
    COM_WDSTR_EN2 = 4
       // Sundat,Monday,Tuesday,Wednesday,Thursday,Friday,Saturday
} COM_WDSTR_TYPE_t;

const char *com_strDotw( COM_WD_TYPE_t iDotw, COM_WDSTR_TYPE_t iType );



/*
 *****************************************************************************
 * COMFILE:ファイル/ディレクトリ関連I/F (com_proc.c)
 *****************************************************************************
 */

/*
 * ファイルオープン  com_fopen()
 *   オープンしたファイルポインタを返す。オープンに失敗したら NULLを返す。
 *   オープン後、必ず com_fclose()でクローズすること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath || !iMode
 *   COM_ERR_FILEDIRNG: fopen()失敗
 *   監視情報追加のための com_addFileInfo()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iPathに対象ファイル名、iModeにオープモードを指定し、ファイルをオープンする。
 * fopen()に失敗した場合は、エラーメッセージを出力する。
 *
 * ＊com_debugFopenErrorOn() を使うことで、わざと結果をNGにすることが可能。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   FILE *com_fopen( const char *iPath, const char *iMode );
 */
#define com_fopen( PATH, MODE ) \
    com_fopenFunc( (PATH), (MODE), COM_FILELOC )

FILE *com_fopenFunc( const char *iPath, const char *iMode, COM_FILEPRM );

/*
 * ファイルクローズ  com_fclose()
 *   fclose()が正常終了したときは 0を返す。
 *   fclose()でエラーが発生した場合は その errno値を返す。
 *   デバッグエラーの場合は -1 を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !ioFp
 *   COM_ERR_FILEDIRNG: fclose()失敗
 *   監視情報追加のための com_deleteFileInfo()によるエラー (COM_ERR_DOUBLEFREE)
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * ioFpで指定したファイルをクローズする。
 * 特殊な事情がない限り、com_fcloseFunc()ではなく com_fclose()を使用すること。
 * fclose()と同じ要領で使う。つまり FPの型は FILE *型を期待している。
 * ファイルクローズ後、FPには NULLを格納する。
 *
 * com_fclose()は関数マクロで、実体は com_fcloseFunc()になる。マクロ定義で
 * com_fclose()の引数は & を付けたポインタとして com_fcloseFunc()に渡しており
 * これによりクローズとともに、元のアドレスの変数への NULL格納を実現する。
 *
 * この事情により、クローズは出来ても NULLにならないという事象が起こり得る。
 * com_free()の説明でも com_freeFunc()との関係で、同様の事象を例示しているので
 * 正しく動かすためのコードの書き方の参考にできるだろう。
 *
 * fclose()は返り値を持つ。0以外だとエラー発生を意味し、errnoを設定する。
 * com_fclose()は fclose()の返り値が 0以外のときにエラーメッセージを出力するが
 * fpには NULL格納するし、デバッグ情報は削除する。その上で返り値をerrnoにする。
 *
 * その場合もクローズ自体は行われるし、エラーに対して直接できることは実は無い。
 * (再クローズを試みた場合の結果は、規格でも未定義となっており、事実上非許容)
 * それでも fclose()で NGが出たことをチェックする必要があるケースがあり得る、
 * と考え、返り値の形でチェックできるようにした。
 *
 * ファイル監視のデバッグ機能が動作している場合、ファイル監視情報が無い ioFpを
 * 指定したときに COM_ERR_DOUBLEFREE のエラーを出す。
 * このエラーがでる原因は2つ考えられる。
 *
 * (1)本当に二重クローズしている場合
 *    -->早急に対処が必要
 *
 * (2)デバッグ情報が保持できる限界を超えたため、デバッグ情報が足りない場合
 *    -->どうしようもない
 *
 * もしも(2)のケースの場合、デバッグ情報のメモリ捕捉失敗を示す COM_ERR_DEBUGNGの
 * エラーメッセージがファイルオープン時に先に出ている。
 * さらに ioFpのアドレスも出しているため、そのアドレスと開放しようとしていた
 * アドレスが一致するならば、(2)のケースである。
 *
 * ＊com_debugFcloseErrorOn() を使うことで、わざと結果をNGにすることが可能。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   int com_fclose( FILE *ioFp );
 */
#define com_fclose( FP ) \
    com_fcloseFunc( &(FP), COM_FILELOC )

int com_fcloseFunc( FILE **ioFp, COM_FILEPRM );

/*
 * ファイル存在チェック  com_checkExistFile()・com_checkExistFiles()
 *   ファイル有無を true/false で返す。
 * ---------------------------------------------------------------------------
 *  com_checkExistFile()の場合
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath
 *  com_checkExistFiles()の場合
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath || !iDelim
 *   ファイル名取得のための com_strdup()によるエラー
 *   上記で確保したメモリの com_free()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * iPathでチェックしたいファイル名を指定すると、そのファイルが存在するか
 * チェックし、その有無を true/false で返す。
 * 本I/F使用前に iPathのファイルをオープンする必要はない。
 *
 * com_checkExistFile()は単一ファイルを対象とする。
 * com_checkExistFiles()は複数ファイルが対象で、iDelimでファイル名を区切る
 * 文字列を指定する。それぞれのファイル有無をチェックし、全て存在すれば
 * true、ひとつでも存在しないものがあれば falseを返す。
 * (iPathに iDelimで区切ったファイル名が複数列挙した文字列を指定する想定)
 *
 * stat()を使い、エラー無く動いた場合に「ファイルあり」と判定する。
 * 従ってファイルではなくディレクトリであった場合も、trueを返す。
 * ディレクトリかどうかを知りたい場合は com_checkIsDir()を併用する。
 *
 * stat()で取得した情報自体が欲しい場合は、com_getFileInfo()を使うか、
 * 自分で直接 stat()を使う。
 */
BOOL com_checkExistFile( const char *iPath );
BOOL com_checkExistFiles( const char *iPath, const char *iDelim );

/*
 * ファイル情報取得  com_getFileInfo()
 *   ファイル有無を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * iPathで指定したファイルの情報を取得し、*oInfoに格納する。
 * oInfoが NULLだった場合、何も格納せず、com_checkExistFile()と同じ動作になる。
 * iLinkが trueの場合は lstat()、falseの場合は stat()で情報を取得する。これは
 * 対象がシンボリックリンクの時に異なる結果を返す。
 *
 * ファイルの情報自体は標準関数を使用して取得するだけなので、本I/Fを使わず
 * 標準関数を直接使用して、struct st型データから情報を見ても、何も問題ない。
 *
 * なお、time_t型時刻データの情報整形には com_setTime()が使えるだろう。
 */

typedef struct {                        // 型定義は cygwin [Windows 19042]で確認
    dev_t    device;    // デバイスID              (unsigned long/int）
      uint dev_major;   //  major(device)の結果
      uint dev_minor;   //  minor(device)の結果
#ifndef LINUXOS   // Cygwin環境下でダミー必要 (dev_tの定義差分)
    int dummy1;
#endif
    ino_t    inode;     // inode番号               (unsigned long/int)
    mode_t   mode;      // アクセスモード          (int)
    uint dummy2;
      BOOL isReg;       // 通常ファイルか？       (S_ISREG(mdoe)の結果）
      BOOL isDir;       // ディレクトリか？       (S_ISDIR(mode)の結果）
      BOOL isLink;      // シンボリックリンクか？ (S_ISLINK(mode)の結果）
    uid_t    uid;       // 所有者ユーザーID        (unsigned int/int)
    gid_t    gid;       // 所有者グループID        (unsigned int/int)
    off_t    size;      // ファイルサイズ(Byte)    (long int)
    time_t   atime;     // 最終アクセス日時        (long int)
    time_t   mtime;     // 最終修正日時            (long int)
    time_t   ctime;     // 最終状態変更日時        (long int)
} com_fileInfo_t;

BOOL com_getFileInfo( com_fileInfo_t *oInfo, const char *iPath, BOOL iLink );

/*
 * ファイル名文字列取得  com_getFileName()・com_getFilePath()・com_getFileExt()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oName/oPath/oExt || !iBufSize || !iPath
 * ===========================================================================
 *   同一の結果出力バッファを使わない限り、マルチスレッドで影響は無い。
 * ===========================================================================
 * パス込みのファイル名文字列 iPathから
 *  com_getFileName() -->ファイル名だけを出力する。
 *                       (iPathが / を含まない場合は iPathそのものを出力)
 *  com_getFilePath() -->ファイルパスだけを出力する。
 *                       (iPathが / を含まない場合は falseを返す
 *                        iPathの最後が / の場合は iPathそのものを出力)
 *  com_getFileExt()  -->ファイル拡張子だけを出力する。
 *                       (iPathが . を含まない場合 または
 *                        iPathの最後が . の場合は falseを返す)
 *
 * 出力先は iBufSizeのサイズの oBufになる。oBufのサイズが足りない場合は、
 * 入れられるだけ格納した上で falseを返す。
 */
BOOL com_getFileName( char *oBuf, size_t iBufSize, const char *iPath );
BOOL com_getFilePath( char *oBuf, size_t iBufSize, const char *iPath );
BOOL com_getFileExt(  char *oBuf, size_t iBufSize, const char *iPath );

/*
 * ファイル行走査  com_seekFile()
 *   ファイルを最後まで走査できたら true を返す。
 *   ファイルが存在しないとき/オープン出来ないときは falseを返す。
 *   コールバック関数で falseを返した時も falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath || !iFunc
 *   指定ファイルの com_fopen()・com_fclose()のエラー
 *   コールバック関数 iFunc内でエラーが起こることもあり得るだろう
 * ===========================================================================
 *   oBufを個別に用意するなら、複数スレッドでの同時使用も可能。
 * ===========================================================================
 * テキストファイルを想定して、iPathで指定したファイルを開き、1行ずつ読み込み、
 * oBufで指定したバッファに格納する。iBufSizeはそのバッファサイズを指定する。
 *
 * oBufは NULLが指定可能で、その場合 本I/F用の共通バッファを使用する。
 * この共通バッファのサイズは COM_LINEBUF_SIZE で、iBufSizeは無視される。
 * 本I/Fが並行で複数動く状況になることが無いなら、これを使っても問題ない。
 * もっと大きなサイズの読み込みを行いたい時も個別にバッファを用意すること。
 *
 * バッファ格納後、その内容を iFuncで示すコールバック関数によって通知する。
 * ioUserDataはコールバック時に iFuncに渡されるデータアドレスで、本I/Fの呼び元
 * から iFuncに渡したいデータがあれば設定する想定。本I/Fでは内容に一切触れずに
 * 引き渡し、どう使うかは iFuncに委ねる。
 * ・例えば読み込んだ内容を加工して別ファイルに書き出したい時に
 *   その出力先のファイルを開き、そのファイルポインタを ioUserDataで渡せば
 *   iFuncの処理内でそのファイルに書き出しが出来るだろう。
 * ・複数データを渡したいときは、そのための構造体を宣言してデータを定義し、
 *   そのアドレスを ioUserData で渡せば良い。
 * 何も渡す必要がないなら NULL指定で良い。
 *
 * 本I/F内でループ処理が行われ、1行読むたびにコールバック関数を呼ぶ。
 * iPathで指定したファイルの開閉は本I/F内で行われるので、呼ぶ側は意識不要。
 *
 * iFuncの引数 iInfで読み込んだデータ等を通知する。
 * iInf->line には実際に1行読み込んだ状態のバッファアドレスを格納する。
 * 次行を読む時に上書きクリアされるので、内容を変更しても問題ない。
 * iInf->fp には実際に読み込んでいるファイルのファイルポインタを格納する。
 * 標準関数の rewind()・fgetpos()・fsetpos()を使いたい時の対象に出来る。
 * (なお fseek()・ftell()は使わず、fgetpos()・fsetpos()を使うことを推奨する)
 * iInf->userDataには 本I/Fの引数 ioUserDataをそのまま格納する。void*型なので
 * コールバックした関数内で、ioUserDataに指定したデータ型のポインタ変数に
 * 格納して使用するとよいだろう。
 *
 * iFuncで falseを返すと、そこでファイル読み込みを中断し falseを返す。
 * ファイルを最後まで読み込み、最後の iFuncコールバックも trueが返ったら、
 * 本I/Fは trueを返して処理を終了する。
 */

// コールバック関数で渡すデータ構造体
typedef struct {
    char*  line;       // 読み込んだ1行分の文字列データ
    FILE*  fp;         // ファイルポインタ
    void*  userData;   // ユーザーデータ
} com_seekFileResult_t;

// ファイル読み込みコールバック関数プロトタイプ
typedef BOOL(*com_seekFileCB_t)( com_seekFileResult_t *iInf );

BOOL com_seekFile(
        const char *iPath, com_seekFileCB_t iFunc,
        void *ioUserData, char *oBuf, size_t iBufSize );

/*
 * ファイルのバイナリ読み込み  com_seekBinary()
 *   ファイルを最後まで走査できたら true を返す。
 *   ファイルが存在しないとき/オープン出来ないときは falseを返す。
 *   コールバック関数で 0を返した時も falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath || !iNextSize || !iFunc
 *                               iNextSize > バッファサイズ(下記参照)
 *   指定ファイルの com_fopen()・com_fclose()のエラー
 *   コールバック関数 iFunc内でエラーが起こることもあり得るだろう
 * ===========================================================================
 *   oBufを個別に用意するなら、複数スレッドでの同時使用も可能。
 * ===========================================================================
 * iPathで指定したファイルを開き、iNextSize分のデータをバイナリとして読み込み
 * oBufで指定したバッファに格納する。iBufSizeはそのバッファサイズを指定する。
 * そしてその内容を iFuncで指定したコールバック関数を呼んで通知する。
 * ioUserDataは そのまま iFuncにより通知され、その使い方は任される。本I/F内で
 * その内容を参照したり変更したりはしない。何もないなら NULL指定で良い。
 *
 * oBufは NULL指定が可能で、その場合 本I/F用の共通バッファを使用する。
 * この共通バッファのサイズは 16 で、iBufSizeは無視される。
 * 本I/Fが並行で複数動く状況になることが無いなら、これを使っても問題ない。
 * もっと大きなサイズの読み込みを行いたい時も個別にバッファを用意すること。
 *
 * iNextSizeは最初に読み込むデータサイズを指定する。0は許容されない。
 * また iBufSizeを超える指定も許容されない。
 * (oBufを NULL指定した場合は、上述の通り 16 を超えてはいけないことになる)
 *
 * iFuncの引数 iInfで読み込んだ結果を通知する。
 * 読み込んだ内容が data と length で通知される点以外は、com_seekFile()と同様。
 * このコールバック関数の返り値は、次に読み込むデータサイズになる。つまり、
 * com_seekBinary()の iNextSizeと同様の上限を持つが、0返却は可能となる。
 * 0返却は読込中断の意味となり、com_seekBinary()は処理中断して falseを返す。
 *
 * もしバイナリデータを出力したいのであれば、com_printBinary()が使えるだろう。
 * デバッグ出力をしたいのであれば com_dump()になる。
 */

// コールバック関数で渡すデータ構造体
typedef struct {
    uchar*  data;       // 読み込んだバイナリデータ
    size_t  length;     // 読み込んだ長さ
    FILE*   fp;         // ファイルポインタ
    void*   userData;   // ユーザーデータ
} com_seekBinResult_t;

// ファイル読み込みコールバック関数プロトタイプ
typedef size_t(*com_seekBinCB_t)( com_seekBinResult_t *iInf );

BOOL com_seekBinary(
        const char *iPath, size_t iNextSize, com_seekBinCB_t iFunc,
        void *ioUserData, uchar *oBuf, size_t iBufSize );

/*
 * コマンド実行結果走査  com_pipeCommand()
 *   コマンド実行を最後まで走査できたら trueを返す。
 *   パイプが生成できないときは falseを返す。
 *   コールバック関数で falseを返した時も、falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iCommand || !iFunc
 *   コールバック関数 iFunc内でエラーが起こることもあり得るだろう
 * ===========================================================================
 *   oBufを個別に用意するなら、複数スレッドでの同時使用も可能。
 * ===========================================================================
 * popen()を利用してシェル起動、iCommandで指定されたコマンドラインを実行し、
 * oBufで指定したバッファに出力内容を格納する。iBugSizeはそのバッファサイズ。
 * その出力内容を1行ずつ iFuncで示すコールバック関数によって通知する。
 * ioUserDataはコールバック時に iFuncに渡されるデータアドレス。本I/Fでは内容に
 * 一切触れずに引き渡し、どう扱うかは iFuncに委ねられる。不要なら NULL指定する。
 *
 * oBufは NULLが指定可能で、その場合 本I/F用の共通バッファを使用する。
 * この共通バッファのサイズは COM_LINEBUF_SIZE で、iBufSizeは無視される。
 * 本I/Fが並行で複数動く状況になることが無いなら、これを使っても問題ない。
 *
 * 本I/F内でループ処理が行われ、1行読むたびにコールバック関数を呼ぶ。
 * パイプの開閉はI/F内で実施するので、呼ぶ側の意識は不要。
 *
 * iFuncと ioUserDataの扱い方は、同じ型を利用する com_seekFile()と全く同様なので
 * そちらの説明記述を参照されたい。
 * 本I/Fは指定したコマンドの実行結果テキストが対象となり、
 * com_seekFile()は指定したファイル内容のテキストが対象となる点のみ異なる。
 *
 * popen()によって開かれたファイルポインタから取得できるのは、標準出力の内容に
 * 限られるため、標準エラー出力の内容も取得したいと考える場合、bashであれば、
 * iCommandの中で「 2> stdout」というように付記して、エラー出力をリダイレクトし
 * ファイルに保存する・・といった工夫をすることは出来るだろう。
 * ・使用するシェルによって、リダイレクトの記述方法が異なる
 * ・コマンドラインで | (パイプ)を使う場合、リダイレクト指定位置特定が困難
 * といった理由から、本I/Fによるリダイレクト指定は準備していない。
 */
BOOL com_pipeCommand(
        const char *iCommand, com_seekFileCB_t iFunc,
        void *ioUserData, char *oBuf, size_t iBufSize );

/*
 * テキスト行単位走査  com_seekTextLine()
 *   テキストを最後まで走査できたら trueを返す。
 *   コールバック関数で falseが返ったら、falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iText || !iFunc
 *   コールバック関数 iFunc内でエラーが起こることもあり得るだろう
 * ===========================================================================
 *   oBufを個別に用意するなら、複数スレッドでの同時使用も可能。
 * ===========================================================================
 * iTextで与えられるテキストデータを改行(\n)があるごとに区切り、
 * oBufで指定したバッファにその内容を格納する。iBufSizeはそのバッファサイズ。
 * その出力内容を1行ずつ iFuncで示すコールバック関数によって通知する。
 * ioUserDataはコールバック時に iFuncに渡されるデータアドレス。本I/Fでは内容に
 * 一切触れずに引き渡し、どう扱うかは iFuncに委ねられる。不要なら NULL指定する。
 *
 * iTextは入力となるテキストデータで、iTextSizeは走査するサイズを指定する。
 * iTextは void*型になっているが、基本的には char*型を期待する。
 * ただ uchar*型であっても受け入れることを可能とするため、void*型とした。
 * iTextが \0 で終端する文字列データであるならば、iTextSizeに 0を指定可能で
 * その場合は自動的に文字列データ全体長を itextSizeとして処理する。
 * 走査したい範囲を絞りたいときや、\0終端が保証できないデータの場合は、
 * そのサイズを正しく iTextSizeに指定する必要がある。
 *
 * oBufは NULLが指定可能で、その場合 本I/F用の共通バッファを使用する。
 * この共通バッファのサイズは COM_LINEBUF_SIZE で、iBufSizeは無視される。
 * 本I/Fが並行で複数動く状況になることが無いなら、これを使っても問題ない。
 *
 * iNeedLfは行ごとの改行コード(\n)を格納バッファに含めるかどうかの指定。
 * trueなら \nありのデータ、falseなら \nなしのデータを格納する。
 *
 * 本I/F内でループ処理が行われ、1行読むたびにコールバック関数を呼ぶ。
 * パイプの開閉はI/F内で実施するので、呼ぶ側の意識は不要。
 *
 * iFuncと ioUserDataの扱い方は、同じ型を利用する com_seekFile()と全く同様なので
 * そちらの説明記述を参照されたい。
 * 通知する情報 iInfのうち fpは常に NULLとなる(そもそもファイルは開かない)。
 */
BOOL com_seekTextLine(
        const void *iText, size_t iTextSize, BOOL iNeedLf,
        com_seekFileCB_t iFunc, void *ioUserData, char *oBuf, size_t iBufSize );

/*
 * ディレクトリ存在チェック  com_checkIsDir()
 *   ディレクトリ有無を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iPathでチェックしたいディレクトリパスを指定する。
 * opendir()で開けるかどうかでチェックをしている。
 * stat()の情報からもチェック可能で、その方法を取りたいなら com_getFileInfo()を
 * 使い、取得する情報 com_fileInfo_t型のメンバー .isDir で判断する。
 */
BOOL com_checkIsDir( const char *iPath );

/*
 * ディレクトリ作成  com_makeDir()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath
 *   COM_ERR_FILEDIRNG: 同名ファイル存在/mkdir()失敗
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iPathで指定したディレクトリを作成する。
 * com_checkExistFile()と com_checkIsDir()を最初に内部で呼ぶ。
 * 既に指定されたディレクトリが存在する場合は、何もせずに trueを返す。
 * 同名ファイルが存在する場合 falseを返す。
 * 同名ファイルが存在しなかったら、指定パスのディレクトリを作成する。
 * 複数階層を一度に指定した場合、階層ごとになければディレクトリを作成する。
 */
BOOL com_makeDir( const char *iPath );

/*
 * ディレクトリ削除  com_removeDir()
 *   削除できたら、削除したファイル/サブディレクトリ数を返す(最低1にはなる)。
 *   指定されたディレクトリが存在しない場合は、何もせずに 0を返す。
 *   処理NGが発生した場合は負数を返す(enum宣言値あり)。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath
 *   COM_ERR_FILEDIRNG: rmdir()失敗
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iPathで指定したディレクトリを削除する。
 * ディレクトリが存在しない場合、何もせずに 0を返す。
 * 存在する場合、指定ディレクトリ内のファイル・サブディレクトリを全て削除し
 * その後 指定ディレクトリ自体を削除する。最終的に削除した数を返す。
 * 成否判定としては返り値が 0以上であれば OKとなる。
 *
 * 削除処理に失敗した場合は負数を返すが、そこまでに削除成功したものは
 * もちろん削除したままなので、結果として中途半端な状態で残ることになる。
 */

// com_removeDir()の処理NG返り値
enum {
    COM_RMDIR_PRMNG       = -1,  // 関数引数NG(デバッグNGになる)
    COM_RMDIR_SEEKFILE_NG = -2,  // 削除対象ファイル走査NG
    COM_RMDIR_SEEKDIR_NG  = -3,  // 削除対象サブディレクトリ走査NG
    COM_RMDIR_RMDIR_NG    = -4   // rmdir()の処理NG
};

long com_removeDir( const char *iPath );

/*
 * ディレクトリ走査  com_seekDir()
 *   走査が全て完了したら true、NGが発生したら falseを返す。
 *   コールバック関数で falseを返した時も、falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath || (!iType & COM_SEEK_BOTH) || !iFunc
 *   COM_ERR_FILEDIRNG: scandir()失敗
 *   ディレクトリ情報生成のための com_strdup()によるエラー
 *   サブディレクトリ一覧作成のための com_addChainData()によるエラー
 *   コールバック関数内の処理によってはエラーが発生し得る
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * ioPathで指定したディレクトリを走査する。実際の走査には scandir()を使用し
 * 処理対象となるエントリーが見つかったら、コールバック関数 iFuncでひとつずつ
 * 通知する。
 *
 * iFilterは走査されたエントリーを処理対象とするかチェックするフィルター関数で
 * NULLを指定した場合は . と .. 以外を処理対象とする toscom内部のデフォルト
 * フィルター関数が動作する(つまりディレクトリ内の全エントリーが対象になる)。
 * フィルター関数はエントリー(const struct dirent*型)を引数で受け取るので、
 * その内容をチェックし、処理対象なら非0、非対象なら 0を返す処理とすれば良い。
 * (ファイル名を見たいならば、引数の iEntry->d_name を使用する)
 * ファイルかサブディレクトリかのチェックは iTypeで行うため、フィルター関数で
 * 敢えてチェックをする必要は無い。
 *
 * iTypeで走査種別を指定する。
 * COM_SEEK_FILEであればファイル、COM_SEEK_DIRであればサブディレクトリ、
 * COM_SEEK_BOTHであればファイル/サブディレクトリの双方が走査対象となる。
 *
 * iCheckChildを trueにするとサブディレクトリについても順次走査する。
 *
 * 走査対象が見つかった場合、iFuncで指定したコールバック関数で通知する。
 * 引数 iInfで対象となるエントリーをひとつずつ通知する。
 * iInf->path と iInf->entryは本I/Fで保持していて解放は不要だが、
 * コールバックのたびに内容は変わるので、保持が必要なら iFunc内で保存作を用意
 * してコピーすること。
 * iInf->pathLenは iInf->pathの長さではなく、そのうちの最後のエントリー名を
 * 省いたパス長を格納する。走査中のパスを明示するのがその目的となる。
 * iInf->isDirはそのエントリーがディレクトリかどうかを示す。
 * iInf->userDataには本I/Fの引数 ioUserDataをそのまま格納する。
 *
 * iFuncで falseを返すと、そこでディレクトリ走査を中断し、本I/Fは falseを返す。
 * ディレクトリを最後まで走査し、iFuncも trueを返せば、本I/Fは trueを返す。
 *
 * 実際の走査には scandir()を利用しており、
 * 引数 filterは 本I/Fで指定した iFilterになる(NULLの場合は内部関数が使われる)。
 * 引数 comparは無条件で alphasort を指定するため、ディレクトリ単位で
 * アルファベット順にソートされた結果が返る。
 * もし scandir()で返されたエントリーを直接使用したいなら、com_scanDir()の
 * 使用を検討するとよいだろう。詳細は com_scanDir()の説明を確認すること。
 *
 * com_countFile()は本I/Fを使うサンプルともなっている。
 * ioUserDataも駆使して結果を集計している。
 */

// ディレクトリ走査フィルター関数プロトタイプ
typedef int(*com_seekFilter_t)( const struct dirent *iEntry );

// 走査種別
typedef enum {
    COM_SEEK_FILE = 1,                             // ファイルのみ検索
    COM_SEEK_DIR  = 2,                             // サブディレクトリのみ検索
    COM_SEEK_BOTH = (COM_SEEK_FILE | COM_SEEK_DIR) // 両方とも検索
} COM_SEEK_TYPE_t;

// ディレクトリ走査情報
typedef struct {
    char*           path;      // エントリーのパス名
    size_t          pathLen;   // エントリー名を除いたパス名長
    struct dirent*  entry;     // エントリー生データ (->d_nameがエントリー名)
    BOOL            isDir;     // エントリーがディレクトリなら true
    void*           userData;  // com_seekDir()で渡したユーザーデータ
} com_seekDirResult_t;

// ディレクトリ走査コールバック関数プロトタイプ
typedef BOOL(*com_seekDirCB_t)( const com_seekDirResult_t *iInf );

BOOL com_seekDir(
        const char *iPath, com_seekFilter_t iFilter, COM_SEEK_TYPE_t iType,
        BOOL iCheckChild, com_seekDirCB_t iFunc, void *ioUserData );

/*
 * ディレクトリ走査軽量版  com_seekDir2()
 *   走査が全て完了したら true、NGが発生したら falseを返す。
 *   コールバック関数で falseを返した時も、falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath || (!iType & COM_SEEK_BOTH) || !iFunc
 *   COM_ERR_FILEDIRNG: opendir()失敗・readdir()失敗
 *   コールバック関数内の処理によってはエラーが発生し得る
 * ===========================================================================
 *   スレッドセーフと言われる readdir_r()が使用非推奨のため readdir()を使用。
 *   必要があれば排他制御を別途実施すること。
 * ===========================================================================
 * メモリ捕捉/解放を行わないように考慮した軽量版のディレクトリ走査を実施する。
 * scandir()は使用せず、opendir()～readdir()～closedir()とI/F内で処理し、
 * iFilterで条件合致するエントリーがあったら iFuncをコールバックして通知する。
 * (iFilterが NULLの場合、com_seekDir()と同様 . と .. 以外を全て通知する)
 *
 * 使い方は com_seekDir()と同様だが、以下が異なる。
 * ・scandir()は使わないので、エントリー名がソートされずに通知される。
 * ・iCheckChildが存在しない。つまりサブディレクトリの再帰走査は未対応。
 * フィルター関数 iFilter・コールバック関数 iFuncの作り方については、
 * com_seekDir()の説明を参照すること。
 */
BOOL com_seekDir2(
        const char *iPath, com_seekFilter_t iFilter,
        COM_SEEK_TYPE_t iType,  // iCheckChild は無い
        com_seekDirCB_t iFunc, void *ioUserData );

/*
 * ディレクトリ直走査  com_scanDir()
 *   scandir()と同じ返り値になる。
 *   つまり走査したエントリー数を返す。エラー時は -1 を返す。
 *   oListは使用後、com_freeScanDir()で一気に解放可能(解放すること)。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath || !oList
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iPathで指定したディレクトリを scandir()で走査し、iFilterで指定したフィルター
 * 関数に従ってリストにして oListに格納し、そのエントリー数を返す。
 * サブディレクトリの中は見ない。
 * oListは本I/Fを使用する側で、必ず com_freeScanDir()を使って解放すること。
 * (メモリ監視機能の対象ともなっている)
 *
 * oListには例えば
 *     struct dirent** nameList;
 * と定義して、&nameList を指定するイメージとなる。
 * 本I/Fの返り値の数のエントリーがこの中に格納されるので、
 * 例えば3番目のエントリーのファイル名を見たいのであれば、
 *     nameList[2]->d_name
 * を見れば良いことになる。使い終わったら
 *     com_freeScanDir( count, nameList );
 * などで解放すること(count に com_scanDir()の返り値が入っている場合)。
 *
 * scandir()の使い方は com_seekDir()と同様になる。
 * iFilterで指定する関数の構造や NULL指定時のデフォルト関数も全く同じで、
 * scandir()の引数 filterに指定される。comparに固定で alphasort を使うのも
 * 同様で。それ以外を使いたい場合は scandir()を直接使用するしか無い。
 *
 * 本I/Fは scandir()で生成されるリスト(oList)を直接使ったほうが処理しやすい
 * という時のためのもので、一つ一つ処理することに問題がないのであれば、
 * com_seekDir()を使うほうが扱いは楽なはずである。こちらはサブディレクトリも
 * 含めた走査が可能だし、scandir()の後のメモリ解放も自動で行う。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   int com_scanDir( const char *iPath, com_seekFilter_t iFilter,
 *                    struct dirent ***oList );
 */
#define com_scanDir( PATH, FILTER, LIST ) \
    com_scanDirFunc( (PATH), (FILTER), (LIST), COM_FILELOC )

int com_scanDirFunc(
        const char *iPath, com_seekFilter_t iFilter,
        struct dirent ***oList, COM_FILEPRM );

/*
 * ディレクトリ直走査情報解放  com_freeScanDir()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !oList
 *   com_free()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * com_scanDir()の中で scandir()が捕捉したメモリを全て解放する。
 * iCountは com_scanDir()の返り値(＝scandir()の返り値)をそのまま設定する。
 * oListも com_scanDir()と同じ要領で指定すれば良い。
 *
 * scandir()のオンラインマニュアルに書かれている方法で独自に解放してもよいが
 * その場合も com_free()を使用すること。そうしなければメモリ監視機能で浮きと
 * 判定される。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_freeScanDir( int iCount, struct dirent ***pList );
 */
#define com_freeScanDir( COUNT, LIST ) \
    com_freeScanDirFunc( (COUNT), (LIST), COM_FILELOC )

void com_freeScanDirFunc( int iCount, struct dirent ***oList, COM_FILEPRM );

/*
 * ディレクトリ内ファイル数カウント  com_countFiles()
 *   処理成否を true/false で返す。
 *   (ディレクトリ処理のサンプルコードも兼ねる)
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iPath
 *   com_seekDir()によるエラー
 *   COM_ERR_FILEDIRNG: サイズ取得のための stat()でNG
 * ===========================================================================
 *   内部で com_seekDir()を使っており、これは排他処理を含む。
 *   それ以外でマルチスレッドによる影響はなく、スレッドセーフとなっている。
 * ===========================================================================
 * iPathで指定したパスがディレクトリだった場合に
 *   *oFileCountに その中にあるファイル数
 *   *oDirCountに その中にあるサブディレクトリ数
 *   *oTotalSizeに ファイルサイズ総計
 * を格納して trueを返す。個別にNULL指定が可能で、その場合計算しない。
 * *oTotalSizeはオーバーフローが発生したときのことは無考慮。
 * iPathのディレクトリが存在しないときや、開けないときは falseを返す。
 *
 * iCheckChildを trueにした場合、サブディレクトリも対象になる。
 *
 * ディレクトリ走査には com_seekDir()を使用し、その使用サンプルともなる。
 *
 * ファイルサイズの取得に com_getFileInfo()を通して lstat()を使用している。
 * これがNGになると、そのファイルの情報は何も得られず、計算にも反映されない。
 * ただその場合も走査処理自体は続行する。これが発生すると、サイズ計算結果は
 * du等を使用した時と異なる値になる可能性が高い。エラーが発生しない場合
 *     du -b (対象ディレクトリ)
 * で出た結果と同じサイズが算出されるはず。
 */
BOOL com_countFiles(
        long *oFileCount, long *oDirCount, off_t *oTotalSize,
        const char *iPath, BOOL iCheckChild );

/*
 * ファイル圧縮  com_zipFile()
 *   処理成否を true/false で返す。
 *   zipがインストールされていなければ正しく動作しない(普通はされているはず)。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG]
 *           !iSource
 *           !iArchive && iNoZip
 *           !iArchive && (iSourceに * か ? か空白が含まれる時)
 *           圧縮用の zipコマンドライン生成失敗
 *   COM_ERR_FILEDIRNG: iArchiveと同名ファイルが存在
 *   COM_ERR_ARCHIVENG: zipコマンドでエラー または .zipの除去失敗
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iSourceで指定したファイルまたはディレクトリを、zipを使って圧縮する。
 * 文字列を単純に zipに渡すのみなので、複数が対象ならスペースで区切れば良いし
 * ワイルドカードの使用も可能。対象となるファイルが存在しない場合、zipで処理NG
 * となり、falseを返すことになる。
 *
 * iArchiveで指定したアーカイブファイル名で保存する。
 * 既に同名ファイルがある場合 NGとなり、falseを返す。
 * これに NULLを指定すると、iSourceに .zip を付与したファイル名が
 * アーカイブファイル名になる(この場合 iNoZipに trueは指定不可)。ただし、
 * iSourceにワイルドカード(*?)が使われている場合 .zip を付与すると、
 * 扱いが難しいファイル名になってしまうため、処理NGにする。
 * 同様の理由でスペースで区切って複数ファイルを指定した場合も 処理NGとする。
 * iArchiveでパス付きのファイル名が指定されていない場合は、同じ場所への
 * アーカイブファイル生成を試みる。
 *
 * iKeyはパスワード(-P で指定する内容)となる。不要なら NULLを指定する。
 * 展開時に同じパスワードを指定しなければならなくなる。
 *
 * iNoZipを trueにした場合、生成後のアーカイブファイル名から .zip を除去する。
 * (iArchiveに拡張子が付いていない場合、zipは自動で .zip を付与する)
 *
 * iOverwriteを trueにした場合、同じファイルが存在したら上書きする。
 * falseにした場合、その状況になるとエラーになる。
 */
BOOL com_zipFile(
        const char *iArchive, const char *iSource, const char *iKey,
        BOOL iNoZip, BOOL iOverwrite );

/*
 * ファイル展開  com_unzipFile()
 *   処理成否を true/false で返す。
 *   unzipがインストールされていなければ正しく動作しない(普通はされているはず)。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG [com_prmNG] !iArchive
 *                               展開用の unzipコマンドライン生成失敗
 *   COM_ERR_ARCHIVENG: unzipコマンドでエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iArchiveで指定したアーカイブファイルを unzipを使って展開する。
 *
 * iTargetPathで、展開先のディレクトリを指定する(-d で指定する内容となる)。
 * NULLならアーカイブファイルと同じディレクトリに展開する。
 *
 * iKeyはパスワード(-P で指定する内容となる)。不要なら NULLを指定する。
 *
 * iDeleteを trueにすると、展開できたらアーカイブファイルを削除する。
 */
BOOL com_unzipFile(
        const char *iArchive, const char *iTargetPath, const char *iKey,
        BOOL iDelete );

/*
 * 大したことではないけれど・・
 * 本来、ファイルの「圧縮」と「解凍」は正しく対応しない言葉である。
 *   凍結 -->複数のファイルを1つにまとめる
 *   圧縮 -->サイズを小さくする
 * ということであり、それを復元する動作については
 *   凍結 --> 解凍
 *   圧縮 --> 展開/伸長
 * とそれぞれ別の言葉が当てられている。
 * が、トレンドとしては「圧縮/解凍」がよく使われる組み合わせのように思う。
 * (さすがに Windowsは「圧縮/展開」を正しく組み合わせて使っていた)
 *
 * zip は 圧縮凍結を一緒にする代表。
 * UNIX/Linuxだと、tarが凍結、compressや gzipが圧縮、というように別々。
 *
 * とはいえ、もはや言葉尻を捕まえても仕方なく、意味は伝わっているのだから、
 * 「圧縮/解凍」でいいのかなあ？という風潮になっている。
 */



/*
 *****************************************************************************
 * COMTHRD:スレッド関連I/F (com_procThread.c)
 *****************************************************************************
 */

/*
 * 排他ロック/アンロック  com_mutexLock()・com_mutexUnlock()
 *   pthread_mutex_lock() または pthead_mutex_unlock() の返り値をそのまま返す。
 *   (正常なら 0、エラーなら 非0)
 * ---------------------------------------------------------------------------
 *   COM_ERR_MLOCKNG: pthread_mutex_lock()でエラー EINVAL発生
 *   COM_ERR_MUNLOCKNG: pthread_mutex_unlock()でエラー EINVAL発生
 * ===========================================================================
 *   自身の排他は検討対象外とする。
 * ===========================================================================
 * 排他制御のために mutexを使用する。
 * com_mutexLock()がロック、com_mutexUnlock()がアンロックのI/Fとなる。
 * ioMutexは PTHREAD_MUTEX_INITIALIZER で初期化した pthread_mutext_t型変数の
 * アドレスを渡す想定(もちろんロック/アンロックで同じものを指定する)だが、
 * それ以外の種別で初期化しても、本I/Fの返り値でその後の動作記述は可能なはず。
 *
 * 返り値が EINVALのときのみ本I/Fはエラー出力する。
 * iFormat以降はエラー発生時のデバッグ出力情報となる。
 * com_mutexUnlock()では iFormatを NULLにした場合、com_mutexLock()で設定した
 * デバッグ出力情報をそのまま使用できる。
 *
 * 本I/Fは toscom内で排他制御が必要な箇所で多数使用されている。
 * その際の ioMutexは toscom内部で固有定義したものであるため、toscom使用者と
 * デッドロックすることは無い。ただ処理上の事実として、mutexロックが各所で
 * 行われていることは認識しておくこと。
 */
int com_mutexLock(   pthread_mutex_t *ioMutex, const char *iFormat, ... );
int com_mutexUnlock( pthread_mutex_t *ioMutex, const char *iFormat, ... );


/*
 * !!!!! 試験実装中 !!!!!
 * 子スレッドの生成と管理をするためのI/F群。
 *
 * スレッドについての習作という側面が強いため、mutexの排他処理以外は、
 * 無理に使う必要は無い。ある程度管理はしやすく設計はしているが、
 * 子スレッドの数に制限があるなど、ニーズにマッチしない可能性がある。
 *
 * 生成可能な子スレッドの数は com_custom.h の COM_THREAD_MAX で宣言する。
 * 増減したいときは、この宣言の記述を修正すること。
 *
 * この宣言をした数の管理エリアを静的に定義している。
 * 端的に言えば、その数まで com_createThread()を受け付ける。
 * その後、終了したスレッドについて com_freeThread()を使うことで、
 * 管理状態を空きに戻すことが出来、そうすると新たな別スレッド生成に
 * 利用できるようになる(つまりそれまで管理情報は残ったままとなる)。
 *
 * ちなみにスレッド/プロセス間のデータ授受には、セレクト機能の UNIXドメイン
 * ソケットを使うという選択肢が今はあることを書き添えておく。
 */

/*
 * 子スレッド生成  com_createThread()
 *   生成成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_THREADNG: pthread系ライブラリ関数の実行NG
 *   管理情報生成のための com_strdup()・com_malloc()・com_free()によるエラー
 *   排他のための com_mutexLock()・com_mutexUnlock()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * POSIXスレッドを生成する。同時生成数の上限は COM_THREAD_MAX で宣言される。
 * oPtidは生成したスレッドIDの格納先を指定する。NULLなら何も格納しない。
 * (以後のスレッド関連I/Fでスレッドを指定するのに使用する。NULLにした場合
 *  生成後のスレッド操作を放棄したことになる。作りっぱなしで良ければ、
 *  それでも問題は発生しない)
 * iBootはスレッド生成した後、起動する関数を必須で指定する。
 * iUserDataは起動関数に渡すユーザーデータで、iSizeはそのサイズを指定する。
 * iFuncはスレッド終了を通知するコールバック関数(通知不要なら NULLで良い)
 * iFormat以降はスレッドを表す文字列で、デバッグ表示等に使用する。
 *
 * 以後、本I/Fを使った側を「親スレッド」、生成したスレッドを「子スレッド」と
 * 説明の中で呼称する。複数スレッドで本I/Fでにより子スレッドを生成することは
 * 現状規制していないが、動作的に不具合が発生する可能性が高く推奨しない。
 *
 * 子スレッドは PTHREAD_CREATE_DETACHED を属性指定する。
 * つまり pthread_join()を使った合流は出来ない。そのため子スレッド生成後、
 * 起動関数(iBoot)終了で、子スレッドのリソースは全破棄になることを意味する。
 *
 * 起動関数(iBoot)の記述に置ける注意点は、本I/Fのプロトタイプ宣言直下に、
 * 更に追記があるので、そちらも確認して欲しい。
 *
 * 一般的な PTHREAD_CREATE_JOINABLE での子スレッド生成を望む場合は
 * 本I/Fは使わずに pthread_create()・pthread_join() を直接コード記述すること。
 */

// スレッド管理ID  （使う側はあまり意識しなくても良い)
typedef  long  com_threadId_t;

enum {
    COM_NO_THREAD = -1    // スレッド生成失敗表示
};

// スレッド用ユーザー情報
typedef struct {
    com_threadId_t  thId;     // スレッド管理ID
    void*           data;     // スレッド生成時に受け取ったユーザーデータ
    size_t          dataSize; // ユーザーデータのサイズ
} com_threadInf_t;

// スレッド起動関数プロトタイプ
typedef void*(*com_thBoot_t)( void *ioInf );

// スレッド終了通知関数プロトタイプ
typedef void(*com_thNotifyCB_t)( com_threadInf_t *iInf );

BOOL com_createThread(
        pthread_t *oPtid, com_thBoot_t iBoot,
        const void *ioUserData, size_t iSize, com_thNotifyCB_t iFunc,
        const char *iFormat, ... );

/*
 * スレッド起動関数(iBoot)内の処理記述に関する注意事項
 *
 * 渡される引数 ioInf について
 *   ioInf->thId は、その子スレッドの管理IDが格納される。pthread_create()の
 *   返り値ではない点に注意すること(iBoot関数内では意識不要だが変更は厳禁)。
 *
 *   ioInf->data は com_crreateThread()の引数 ioUserDataの内容コピーになる。
 *   ioUserDataのアドレスをそのまま渡すわけではないことに注意すること。
 *
 *   ioInf->dataSize は com_createThread()の引数 iSizeがそのまま格納される。
 *
 *   ioInfの dataと dataSizeについては iBoot関数内で変更しても問題ない。
 *   data自体は void*型で保持しているので、呼び元に併せた型のポインタ変数に
 *   そのまま代入してデータとして利用できるだろう。
 *   ただし、dataは com_malloc()で確保したメモリ領域のアドレスなので、
 *   そのサイズ変更が必要なら com_realloc()等の使用を検討すること。
 *
 * iBoot関数の開始時は com_readyThread()を必ず呼ぶこと。
 * iBoot関数の終了時は com_finishThread()を必ず呼ぶこと。
 * (それぞれの引数は iBoot関数の引数 ioInf をそのまま指定する)
 *
 * 終了時の return文については
 *     return com_finishThread( ioInf );
 * と書けるようにI/Fを設計している。ioInf->dataの内容が変更されていた場合、
 * この終了I/Fを使用することで、その内容を親スレッドに通知できる。
 *
 * 親スレッドでは com_checkThread() または com_watchThread() を使用して、
 * 子スレッドの終了を監視する(この2つを以後「監視I/F」と呼称する)。
 * これらの監視I/Fでは、子スレッドの com_finishThread()使用を検出し、
 * com_createThread()の iFuncで指定した関数をコールバックして、終了を通知する。
 * この際、iBoot関数と同じ引数を渡すので、最終的なユーザーデータの内容も
 * 受け取れる。
 *
 * ＊＊＊注意＊＊＊
 *   toscomのI/Fはスレッドセーフと出来るように考慮を進めているが、
 *   完全に動作の確認ができているわけではない。排他処理については導入を勧め
 *   com_if.h については基本的にスレッドセーフとなるように考慮済み。
 *   ただ「問題発生するケースはない」と言い切れる状態ではない点は御了承を。
 */

/*
 * 子スレッド開始通知  com_readyThread()
 * ---------------------------------------------------------------------------
 *   排他のための com_mutexLock()・com_mutexUnlock()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 子スレッドの開始を親スレッドに通知する。
 * 本I/Fは子スレッドの起動関数(com_createThread()の iBootで指定した関数)冒頭で
 * 呼ばれることを前提としている。
 * 本I/Fの引数 iInf は、起動関数の引数 ioInfをそのまま指定することを想定する。
 *
 * 本I/Fにより、親スレッドの管理情報で当該子スレッドの状態が「走行中」になる。
 * また com_finishThread()で、親スレッドに子スレッド終了を通知可能になる。
 */
void com_readyThread( com_threadInf_t *iInf );

/*
 * 子スレッド終了通知  com_finishThread()
 *   固定で NULLを返す。
 * ---------------------------------------------------------------------------
 *   排他のための com_mutexLock()・com_mutexUnlock()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 子スレッドの終了を親スレッドに通知する。
 * 本I/Fは子スレッドの起動関数(com_createThread()の iBootで指定した関数)最後で
 * 呼ばれることを前提としている。処理終了ルートが複数ある場合、その全てで記述が
 * あることを期待する。
 * 本I/Fの引数 iInf は、起動関数の引数 ioInfをそのまま指定することを想定する。
 *
 * 本I/Fにより、親スレッドの管理情報で当該子スレッドの状態は、監視I/Fによる
 * 子スレッド終了検出待ちになる。
 * 最終的なユーザーデータの内容も管理情報に親スレッドに通知される。
 * このユーザーデータのメモリは、監視I/Fが終了検出して処理をした後、解放される。
 * 仮に残っていたとしても、プログラム終了時に強制解放する。
 *
 * 固定で NULLを返すが、これは子スレッドの起動関数の返り値に合わせた結果で、
 * 起動関数の処理終了時に
 *     return com_finishThread( ioInf );
 * という書き方が出来るようにするのが、目的となっている。
 * 返り値の型が合致しない別関数内で本I/Fを使う場合は、(void)を付ける等の工夫も
 * 必要になるだろう(コンパイラが黙殺するなら何もしなくても良いが)。
 *
 * 親スレッドが子スレッド終了の検出をしなくてもよい場合は、本I/Fも不要。
 */
void *com_finishThread( com_threadInf_t *iInf );

/*
 * 子スレッド状態取得(監視I/F)  com_checkThread()
 *   当該子スレッドの状態を返す。
 * ---------------------------------------------------------------------------
 *   排他のための com_mutexLock()・com_mutexUnlock()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iPtidで指定した子スレッドの状態を返す。
 * もし子スレッドの狩猟を検出した場合、終了関数のコールバックを実施する。
 * 本I/Fは指定した子スレッドのみのチェックだが、com_watchThread()は全ての
 * 子スレッドの状態をチェックする。
 *
 * COM_THST_NOTEXIST が返った場合、指定IDの子スレッドは管理情報内に存在しない
 * ことを示す。
 *
 * COM_THST_CREATED が返った場合、スレッド生成後の初期状態で、子スレッドから
 * com_readyThread()が呼ばれていないことを示す。
 *
 * COM_THST_RUNNINF が返った場合、子スレッドから com_readyThread()が呼ばれ、
 * 走行中の状態であることを示す。
 *
 * COM_THST_FINISHED が返った場合、子スレッドで com_finishThread()が呼ばれ、
 * 子スレッドが終了していることを示す。コールバック関数が指定されている場合、
 * このタイミングで終了関数をコールバックする。
 * 同じスレッドに対して複数回本I/Fが COM_THST_FINISHED を帰す場合でも、
 * コールバックが行われるのは最初の1回目のみとなる。
 *
 * COM_THST_ABORTED が返った場合、子スレッドは com_finishThread()を呼んで
 * いないが、/proc/(親スレッドのプロセスID)/task/ の下に子スレッドの情報が
 * 一切存在していない＝子スレッドが通知なく終了していることを示す。
 * この場合も子スレッドは終了と認識するが、終了関数のコールバックは実施しない。
 * 何らかの処理が必要な場合、本I/Fの呼び元で この返り値を判定する処理を
 * 検討すること。
 */
typedef enum {
    COM_THST_NOTEXIST = 0,  // スレッド無し
    COM_THST_CREATED,       // スレッド生成済
    COM_THST_RUNNING,       // スレッド走行中
    COM_THST_FINISHED,      // スレッド終了(＝コールバック実施)
    COM_THST_ABORTED        // 通知なくスレッド終了
} COM_THRD_STATUS_t;

COM_THRD_STATUS_t com_checkThread( pthread_t iPtid );

/*
 * 子スレッド監視  com_watchThread()
 *   まだ走行中の子スレッドがある場合は true、ない場合は falseを返す。
 * ---------------------------------------------------------------------------
 *   排他のための com_mutexLock()・com_mutexUnlock()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 全ての子スレッドの状態を com_checkThread()と同じ方法でチェックする。
 * 子スレッドの状態が COM_THST_RUNNING なら、走行中スレッドとしてカウントする。
 * 子スレッドの状態が COM_THST_FINISHED になっていたら、最初の検出時のみ
 * コールバック関数を呼んで通知する(コールバック関数指定時のみ)。
 *
 * iBlockが trueの場合、子スレッドの終了を1つ以上検出するか、全ての子スレッドが
 * 終了するまで、待機状態になり返らない。つまり走行中スレッド数に変化があるまで
 * 本I/Fは返ってこないということになる。
 * iBlockが falseの場合、全スレッドの状態を一度チェックしたら返る。
 *
 * 子スレッドが com_finishThread()を使用しない場合、本I/Fで終了は検知できない。
 *
 * trueが返る場合、まだ走行中の子スレッドがあるので、スレッド監視を継続する
 * という動作になることを、基本的には想定している。
 */
BOOL com_watchThread( BOOL iBlock );

/*
 * 子スレッドユーザーデータ取得  com_getThreadUserData()
 *   指定した子スレッドのユーザーデータのアドレスを返す。
 *   子スレッドの管理情報がない場合、NULLを返す。
 * ---------------------------------------------------------------------------
 *   排他のための com_mutexLock()・com_mutexUnlock()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iPtidで指定した子スレッドの管理情報にアクセスし、ユーザーデータを返す。
 * 終了関数コールバック以外のタイミングで、子スレッドのユーザーデータを参照
 * したい時に使う。つまり終了検知された子スレッド(状態が COM_THST_FINISHED)
 * が本I/Fの対象であり、それ以外の状態の子スレッドを指定しても内容は未保証。
 *
 * 子スレッドの管理情報は、能動的な com_freeThread()の使用か、プログラム終了時
 * com_finalize()内でのみ解放される。
 * つまり子スレッドが終了しても、管理情報自体は残り続ける。
 */
void *com_getThreadUserData( pthread_t iPtid );

/*
 * 子スレッド解放  com_freeThread()
 *   処理成否を true/false で返す。
 *   指定した子スレッドの管理情報がない場合、何もせずに trueを返す。
 * ---------------------------------------------------------------------------
 *   排他のための com_mutexLock()・com_mutexUnlock()によるエラー
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iPtidで指定した子スレッドの情報をメモリから解放し、未使用状態にする。
 * 以後、com_createThread()で未使用状態になったエリアを、新たな子スレッド用に
 * 利用できるようになる。
 * つまりスレッド生成数が設定した最大数に達していたとしても、処理が終わった
 * 子スレッドの管理情報を解放することで、新たな子スレッドが生成できる理屈。
 * コードを書く側が子スレッドの状態をきちんと管理できるのであれば、これも可能。
 * (スレッド生成数が決まっているならば、COM_THREAD_MAX をその数に揃えるべき)
 *
 * com_finishThread()による子スレッド終了通知が行われていない場合、本I/Fによる
 * 管理情報のメモリ解放は実施できず、falseを返す。本I/Fを使わなくても、
 * プログラム終了時に、走行中のスレッドの pthread_cancel()による終了を試み、
 * 併せて管理情報も解放する。
 *
 * ＊pthread_cancel()は既に存在しないスレッドを指定した時の動作が未定義で、
 *   Cent6.xだと落ちる。子スレッドは com_readyThread()・com_finishThread()を
 *   必ず使い、親スレッドで状態が把握できるようにすることを強く推奨する。
 *
 * 監視I/F(com_checkThread()や com_watchThresd())で終了を検知した後も、
 * toscomはプログラム終了時以外に子スレッドの管理情報を自動解放はしない。
 * なお排他処理の都合で、コールバックする終了関数の中で、本I/Fを使用すると
 * デッドロックの原因となり得るので避けること。(一応、工夫しないとそうした
 * ことは出来ないようにしてはいるが)
 */
BOOL com_freeThread( pthread_t iPtid );


/*
 *****************************************************************************
 * COMPRINT:画面出力＆ロギング関連I/F (com_debug.c)
 *****************************************************************************
 */

/*
 * デバッグログファイル名設定  com_setLogFile()
 *   ＊com_initializeSpec()の中で呼ぶ必要がある。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない(問題発生時はプログラム強制終了)
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * デバッグログのファイル名を iFileNameの内容で設定する。
 * 本I/Fを使用しない場合、ログファイル名はデフォルト(.(アプリ名).log)になる。
 *
 * 本I/Fはデバッグログがオープンされる前に呼ばなければ何も動作しない。
 * このため、com_initalizeSpec()で呼ぶ必要がある。
 *
 * もし本I/Fで iFileNameをNULLにした場合、デバッグログ作成自体を抑制する。
 * ＊makefileにコンパイルオプション(-DUSEDEBUGLOG)による抑制も可能。
 *   このオプションがコメントアウトされた場合、本I/Fによる抑制は必要無い。
 *   もしコメントアウトされていても、com_initializeSpec()で本I/Fを使い、
 *   ファイル名を指定したら、そのファイル名でデバッグログを作成する。
 * 
 * もし既に同じ名前のファイルがあった場合は、日時を末尾に付与してリネームし、
 * 指定された名前のファイルが常に最新のログとなるように動作する。
 *
 * ファイル名を変更した場合、make logclean でデバッグログが削除不可になるので
 * ・logcleanに記述しているデバッグログファイル名パターンを変更する。
 * ・speccleanに変更後のファイル名パターンを追記する。
 * のどちらかで対応して、ファイルの完全なクリーンナップが出来るようになる。
 */
void com_setLogFile( const char *iFileName );

/*
 * タイトルの書式設定  com_setTitleForm()
 *   ＊com_initializeSpec()で呼ぶのが一番望ましい。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない(問題発生時はプログラム強制終了)
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * プログラムの開始時と終了時に表示されるメッセージラインの書式を設定する。
 * 以下の順番で書式文字をセットする必要がある。
 *   %s アプリ名
 *   %s バージョン名
 *   %s 開始/終了文字列
 * 空文字 "" を iTitleに設定すると、タイトル出力を抑制できる。
 *
 * タイトルはデバッグログを開くタイミングで出力されるため、本I/Fの変更を
 * 最初から反映させたいならば、com_initializeSpec()で呼ぶ必要がある。
 *
 * 書式用バッファサイズを超える入力が会った場合は、NG出力し、デフォルトのまま。
 * デフォルトは "\n##########   %s ver%s %-8s ##########\n"
 */
void com_setTitleForm( const char *iTitle );

/*
 * エラーコード書式設定  com_setErrorCodeForm()
 *   ＊com_initializeSpec()で呼ぶのが一番望ましい。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない(問題発生時はプログラム強制終了)
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * エラー出力時のエラーコード出力書式を設定する。
 * %ld でエラーコードの書式文字を1つセットする必要がある。
 *
 * エラー処理が始まる前に本I/Fを呼ばなければ、呼ぶ前後でエラー書式が変わる。
 * そのため、com_initializeSpec()で本I/Fを呼ぶのが一番望ましい。
 *
 * 書式用バッファサイズを超える入力が会った場合は、NG出力し、デフォルトのまま。
 * デフォルトは "!%03ld! "
 */
void com_setErrorCodeForm( const char *iError );

/*
 * 画面出力＆ロギング  com_printf()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * printf()と同じ書式で出力する。
 * 内容を標準出力すると同時に、デバッグログ上にも書き出す。
 *
 * 画面出力処理はデバッグログへの出力が密接に絡むため、com_proc.cではなく
 * com_debug.cに記述している。処理を追いたいと思ったときは注意されたい。
 *
 * また、標準関数の printf()のような書式文字列とそれ以降の引数とのチェックが
 * 基本的に行われないため、注意が必要。例えば
 * ・書式文字列の型指定と、対応する値の型が不一致。
 *   (long型変数の出力に対し、%ld ではなく %d を使用、等)
 * ・書式文字列で必要とする継続引数の数と、実際の数が不一致
 *   (2つ必要なのに1つしかない、等)
 * といった問題があった場合、printf()であればコンパイラが警告を出すが、
 * com_printf()では一切警告が出ない。その結果、望む出力が出ない可能性はある。
 *
 * そうした事態の問題分析をするため、makeターゲット checkf を用意した。
 * make checkfと打ってビルドすると、com_printf()のほか、幾つかの出力用I/Fを
 * printf()に置換してコンパイルを試みる。これにより使っている出力書式に対して
 * コンパイラが持つ上記のチェックを実際に掛けることが可能となる。開発中も
 * 図氏子の方法でコンパイルし、エラーにならないようにすることを推奨する。
 * 以下のI/Fが置換される：
 *   com_printf()・com_printCr()・com_printBack()・com_printTag()・
 *   com_printfLogOnly()・com_printfDispOnly()・com_debug()・com_debugFunc()・
 *   com_error()・com_errorExit()・com_errorSelect()・com_system()
 *   com_dbgCom()・com_logCom()  ＊この2つは com_debug.hに記述
 * 以下は iFormatに NULLを許容するなどの理由で置換されない(つまりチェックなし)。
 *   com_dump()・com_dbgFuncCom()・com_dumpCom()・com_prmNG()・
 *   ＊com_dump()以外は com_debug.hに記述
 * make checkfはあくまでコンパイルチェックに使用することのみを想定しており、
 * 生成されたバイナリは正しく動作することを一切保証しない。
 *
 * 出力書式に冠する主な注意事項は以下となる(これは printf()でも同様である)。
 * ・int型は 32/64bitどちらでも 32bitサイズなので %d または %u になる。
 *   enumで宣言した列挙体も int型なので同様となる。
 * ・long型の出力をするときは %ld または %lu を使用する必要がある。
 *   例えばBOOL型は変更していないなら、long型なので %ld になる。
 * ・32/64bitコンパチにする場合、size_t型は %zu、ssize_t型は %zd が適正。
 *   16進表記の %x も long型が対象の場合 %zx を使う方が良い。
 *   これらは変数のサイズが影響を受ける処理になっているため、
 *     単純に %lu・%ld・%lx を使うと 32bit環境で警告
 *     単純に %u・%d・%x を使うと 64bit環境で警告
 *   という事態が避けられなくなる(32/64bitでサイズが変わるのが問題)。
 * ・ポインタ型は 32/64bitでサイズが変わるが、どちらも %p で良い。
 *   ただ void*型でないと警告が出る可能性もあり、その場合は (void*)でキャスト
 *   すると良いだろう。
 * ・関数ポインタは %p でも警告が止まらない。
 *   toscomでは %zx を用い (long)でキャストする方法でしのいでいるが、
 *   intptr型を使うなどしたほうが良いのかもしれない。
 * ・サイズを可変にするため * を使う場合、その数値は int型にする必要がある。
 *   (int)でキャストする方が早いこともあるだろう。
 */
#ifndef CHECK_PRINT_FORMAT
void com_printf( const char *iFormat, ... );
#else  // make checkfを打った時に設定されるマクロ
#define com_printf  printf
#endif

/*
 * 文字の複数入力  com_repeat()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iSource
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iSourceの文字列を iCountの回数連続 com_printf()で出力する。
 * iUseLfを trueにした場合、com_printf()を使って、最後に改行を出力する。
 */
void com_repeat( const char *iSource, long iCount, BOOL iUseLf );

/*
 * 改行のみ出力  com_printLf()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * "\n"だけを com_printf()で出力する。
 * 別に com_printf()を使っても問題はない。
 */
void com_printLf( void );

/*
 * カーソル復帰後、画面出力  com_printCr()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 先に \r を出力して表示位置を行頭に戻す。それから指定された内容を出力する。
 * デバッグログ上は <CR> と出力してから改行する。
 */
#ifndef CHECK_PRINT_FORMAT
void com_printCr( const char *iFormat, ... );
#else  // make checkfを打った時に設定されるマクロ
#define com_printCr  printf
#endif

/*
 * 画面出力後カーソル後退  com_printBack()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 文字を出力後、単純に文字数分 \b を出力して表示位置を戻す。
 * 改行コード(\n)等のエスケープ文字を含まない出力内容とすること。
 * もし改行コードが入っていた場合、その直前までの出力になる。
 * デバッグログ上は改行して <b～> と出力する。～には戻った文字数が入る。
 */
#ifndef CHECK_PRINT_FORMAT
void com_printBack( const char *iFormat, ... );
#else  // make checkfを打った時に設定されるマクロ
#define com_printBack  printf
#endif

/*
 * セパレータタグ文字列出力  com_printTag()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * セパレータタグのような画面出力を実現する。
 * iTagは区切りとなる文字(複数文字も指定可能)で、iWidthがタグの全幅になる。
 * iPosは iFormat以降で指定するラベル文字列の表示位置を指定する。
 *
 * iWidthの範囲で、iPosに沿ったラベル文字列出力を行い、それ以外のスペースを
 * iTagで埋めるという動作になる。ラベル文字列前後は空白を1つ空ける。
 *
 * iWidthよりもラベル文字列の方が長い場合は、単純なラベル文字列出力になる。
 * iWidthの中に収まるようにタグ文字列を生成しようと試みるが、十分なスペースが
 * ない場合、必ずしも iWidth一杯に文字列が入るとは限らない。
 * 特に iTagが複数文字列の場合、iWidthを巧く考慮しないと長さが統一されない
 * という可能性があることに留意すること。
 */

// ラベル文字列の表示位置
typedef enum {
    COM_PTAG_LEFT,    // ラベル左寄せ
    COM_PTAG_CENTER,  // ラベル中央
    COM_PTAG_RIGHT    // ラベル右寄せ
} COM_PTAG_POS_t;

#ifndef CHECK_PRINT_FORMAT
void com_printTag(
        const char *iTag, size_t iWidth, COM_PTAG_POS_t iPos,
        const char *iFormat, ... );
#else  // make checkfを打った時に設定されるマクロ
#define com_printTag( TAG, WIDTH, POS, ... ) \
          COM_UNUSED( (TAG) ); COM_UNUSED( (WIDTH) ); COM_UNUSED( (POS) ); \
          printf( __VA_ARGS__ )
#endif

/*
 * バイナリデータ出力  com_printBinary()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iBinary || !iLength
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * バイナリデータを出力する。
 * com_dump()も存在するが、こちらはデバッグ出力用のため、[DBG]が付与されるし
 * リリース版では出力が抑制される。バイナリデータの出力が本当に必要な場合は
 * 本I/Fを主に用いることが出来るだろう。
 * 本I/Fでは表現しきれない出力を望むのなら、自分で出力処理を書くこと。
 *
 * iBinaryでバイナリデータのアドレス、iLengthでその長さを指定する。
 *  ＊iBinaryが uchar*型ではなく void*型なのは、先頭アドレスを渡されたら
 *    そのデータの型に関係なく、iLengthの長さ分バイナリとして出力する為。
 *
 * iFlagが出力書式指定で、これ自体を NULL指定することも可能。その場合は自動で
 * 下記設定となる。
 *   .topSep = "-",  .prefix = NULL,  .suffix = NULL,
 *   .data = 16,  .colSeq = 4,  .seq = " ",  .secAscii = ": ",
 *   .onlyText = false
 * これによる出力イメージは下記のようになる(-を使ったセパレータを前後に出力、
 * バイナリは1行に 16oct出力・4octごとに空白・右側にアスキー文字列出力)
 * ------------------------------------------------------
 * 12342021 22232437 38394142 43808182 : .4 !#$789ABC...
 * a1a2a3                              : ...
 * ------------------------------------------------------
 * Wiresharkなどで信号を出力した時の表示イメージに近い出力となる。
 * 右側のアスキー出力は有効な文字コードであればその文字を出力し、文字としては
 * 出力できない数値(isprint()で 0になる数値)だった場合は . を出力する。
 *
 * 上記の各要素値は、必ずしもデフォルト値という意味合いではない。
 * 文字列要素(char*型)は NULLだった場合 その要素を出力しないという意味になる。
 * 各要素の値が NULLや 0だった場合の扱いについては com_printBin_t型の宣言で
 * 明記する。
 *
 * .topSepには1文字の文字列を指定することを想定する。生成されるセパレータの
 * 実際の長さは、他の設定値や設定文字列長から自動計算する。
 * もっと固定された長さや 各種文字列を乗せたセパレータにしたい場合、
 * .topSepには NULLにして、本I/Fの呼び出し前後にセパレータ出力の処理を自分で
 * 書く方が良いだろう。
 *
 * 1行書き出すごとに改行するので .prefix や .suffix に改行を含める必要はない。
 * ただし後述する .onlyTextが trueの場合改行は行わない。
 *
 * .cols に iLengthと同じ値を指定すれば、改行無しで1行に全て出力する。
 * .cols と .colSeq を同じ値にすれば、1行のバイナリを詰めて出力可能。
 * (ただし適度に空白で区切ったほうが、見やすいとは思われる)
 *
 * ちなみに com_printBin_t flags = { .colSeq = 1 }; というように .colSeqのみの
 * 指定(それ以外は 0/NULLになる)でも、16octずつ その指定数ごとに空白で区切って
 * バイナリのみ出力という比較的一般的な構成に出来る。
 *
 * .seqAscii を NULLにした場合、アスキー文字列出力自体しなくなる。
 *
 * .onlyText を trueにした場合、逆にバイナリ出力がなくなり、アスキー文字列のみ
 * 出力になる。その他の全要素を全て 0(ポインタ変数はNULL)にすれば、改行も無し
 * の文字列出力だけが出来る。
 */

// バイナリデータ出力書式設定
typedef struct {
    char*   topSep;     // データ前後のセパレータ文字 (NULL時は出力なし)
    char*   prefix;     // 行頭に付与する文字 (NULL時は出力なし)
    char*   suffix;     // 行末に付与する文字 (NULL時は出力なし)
    size_t  cols;       // 1行のデータ数      (0の時は 16として扱う)
    size_t  colSeq;     // データ区切り数     (0の時は 4として扱う)
    char*   seq;        // データ区切り文字列 (NULL時は " "になる)
    char*   seqAscii;   // バイナリとアスキーの区切り (NULL時はアスキーなし)
    BOOL    onlyText;   // trueならアスキー出力のみ
} com_printBin_t;

void com_printBinary(
        const void *iBinary, size_t iLength, com_printBin_t *iFlags );

/*
 * 画面クリア  com_clear()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * デバッグログ上は *****～ が3行入る。
 */
void com_clear( void );

/*
 * ロギングのみ  com_printfLogOnly()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 画面出力はせず、デバッグログにのみ書き出す。
 */
#ifndef CHECK_PRINT_FORMAT
void com_printfLogOnly( const char *iFormat, ... );
#else  // make checkfを打った時に設定されるマクロ
#define com_printfLogOnly  printf
#endif

/*
 * ロギングのみ  com_printfDispOnly()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * com_printfLogOnly()とは逆に、画面出力のみで、デバッグログに書き込まない。
 */
#ifndef CHECK_PRINT_FORMAT
void com_printfDispOnly( const char *iFormat, ... );
#else  // make checkfを打った時に設定されるマクロ
#define com_printfDispOnly  printf
#endif



/*
 *****************************************************************************
 * COMDEBUG:デバッグ関連I/F (com_debug.c)
 *****************************************************************************
 */

// デバッグ情報出力モード
typedef enum {
    COM_DEBUG_OFF    = 0,   // 監視しない
    COM_DEBUG_ON     = 1,   // 監視し、全て画面に出力する
    COM_DEBUG_SILENT = 2    // 監視し、画面出力は最低限に絞る
} COM_DEBUG_MODE_t;

/*
 * デバッグ表示設定  com_setDebugPrint()
 *   ＊COM_DEBUG_SILENTは、デバッグログ出力のみで、画面出力はしない
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * デバッグ表示の要否を iMode で指定し設定する。
 * 実際のデバッグ表示は com_debug()・com_debugFunc()・com_dump()で実施する。
 * デバッグ表示は画面出力・デバッグログ出力ともに [DBG] を付与する。
 * デバッグ表示設定OFFの場合は出力しない情報であることを意識すること。
 *
 * なお、デバッグ表示を行う場合、comモジュールのデバッグ表示も出るようになる。
 * comモジュールのデバッグ表示は [DBG] ではなく [COM] が付与される。
 * これにより、comモジュール使用者の意思で出力したデバッグ表示と区別が出来る
 * ようにしている。このログが不要な場合は com_noComDebugLog() を使用する。
 *
 * プログラム開始当初から設定を反映させたいなら、com_initializeSpec()で
 * 本I/Fを呼ぶ必要がある。本I/F設定が makefileの設定より優先される。
 * (ビルド時に makefileの -DDEBUGPRINT で指定できる)
 */
void com_setDebugPrint( COM_DEBUG_MODE_t iMode );

/*
 * デバッグ表示設定取得  com_getDebugPrint()
 *   現在のデバッグ表示のモードを返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 */
COM_DEBUG_MODE_t com_getDebugPrint( void );

/*
 * デバッグ出力  com_debug()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * printf()と同じ書式で出力内容を指定する。
 * 最後に改行を自動追加する。
 *
 * 実際の出力は先頭に [DBG] を必ず付与する。
 * (通常の画面出力と区別が出来るようにすることを目的としている)
 *
 * デバッグ表示設定によって出力有無が以下のように変わる。
 * ・COM_DEBUG_OFF なら何も出力しない。
 * ・COM_DEBUG_ON なら画面出力＆デバッグログ出力する。
 * ・COM_DEBUG_SILENT ならデバッグログ出力のみする。
 *
 * 出力箇所が特定しやすい com_debubFunc()の方が使いやすいかもしれない。
 */
#ifndef CHECK_PRINT_FORMAT
void com_debug( const char *iFormat, ... );
#else  // make checkfを打った時に設定されるマクロ
#define com_debug  printf
#endif

/*
 * 実行位置付きのデバッグ出力  com_debugFunc()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * デバッグ出力時に、出力を記述したファイル・ライン数・関数名を併せて出力する。
 * デバッグ出力故に com_debug()と同様、[DBG] を各行頭に付加する。
 *
 * 実際の com_debugFunc()はマクロで、実体は com_debugFuncWrap()。
 * iFileには __FILE__、iLineには __LINE__、iFuncには __func__の指定を想定し、
 * 「 関数名() ～ <ファイル名: line ライン数>」という文字列を出力する。
 * ～ には iFormat以降で指定した諸色の文字列が入る(不要なら NULLを指定する)。
 * 最後に改行を自動追加する、という仕様である。
 *
 * デバッグ表示設定による処理の違いは com_debug()と同じになる。
 */

/*
 * プロトタイプ宣言 (この形で使用すること)
 *   void com_debugFunc( const char *iFormat, ... );
 */
#ifndef CHECK_PRINT_FORMAT
#define com_debugFunc( ... ) \
    com_debugFuncWrap( COM_FILELOC, __VA_ARGS__ )
#else  // make checkfを打った時に設定されるマクロ
#define com_debugFunc  printf
#endif

void com_debugFuncWrap( COM_FILEPRM, const char *iFormat, ... );

/*
 * バイナリダンプのデバッグ出力  com_dump()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iAddrから始まる iSizeオクテットのバイナリダンプをデバッグ出力する。
 * iFormat以降はダンプ前に出力する文字列となる。NULL指定が可能で、その場合は
 * iAddrと iSizeの内容を最初に出力する。
 * 最後に改行を自動追加する。
 *
 * デバッグ表示設定による処理の違いは com_debug()と同じになる。
 */
void com_dump( const void *iAddr, size_t iSize, const char *iFormat, ... );

/*
 * comモジュールデバッグログ抑制  com_noComDebugLog()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * iModeに trueを指定すると、comモジュールのデバッグ表示を抑制する。
 * com_setDebugPrint()でデバッグ表示設定を COM_DEBUG_ON または COM_DEBUG_SILENT
 * にしていた時、出力される comモジュールのデバッグ表示が抑制対象で、これにより
 * [COM] で始めるデバッグ表示を一切しなくなる。
 * ユーザー自身のデバッグログを見やすくしたい時に使用することが出来るだろう。
 *
 * この設定を trueにしても、デバッグ監視機能によるプログラム終了時の
 * リソース浮き情報(メモリ/ファイル)については [COM]付きで出力する。
 *
 * 最初は falseが設定されており、本I/Fのみがこれを変更できる。
 * 完全な抑制のためには、デバッグログのオープン前に trueを設定するべきで、
 * そのタイミングとしては com_initializeSpec()内で呼ぶことが推奨される。
 */
void com_noComDebugLog( BOOL iMode );

/*
 * デバッグ監視情報 シーケンス番号最大値
 * -------------------------------------
 * メモリ監視とファイル監視の情報を保持できる最大数をも表す。
 * ただこれを小さくすることで得られるメリットは殆ど無く、変更は非推奨。
 * このため基本的に変更を想定しない com_if.h での宣言とする。
 *
 * もし一度に保持し得る数より少ない設定にしてしまうと、
 * デバッグ情報の追加に失敗して、正しい監視が出来なくなる。
 * 実際に必要なメモリ量は、一度に保持すべき最大数によって決まるものであり、
 * この値を減らしたところでメモリ消費量がそう変わるわけではない。
 *
 * シーケンス番号は long型のため、64bitだと int型以上の数値も使える。
 * ただ列挙体で宣言できるのは int型の範囲に留まるため、マクロ宣言とした。
 */
#define COM_DEBUG_SEQNO_MAX  99999999

/*
 * メモリ監視設定  com_setWatchMemInfo()
 *   ＊COM_DEBUG_SILENTは、プログラム終了時の浮きのみ画面出力する。
 *     (デバッグログには全て出力する)
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * COM_DEBUG_ON/COM_DEBUG_SILENTに設定すると、メモリ捕捉と解放のときにログを
 * 残すようになる。そして解放漏れがあった場合、プログラム終了時に出力する。
 *
 * メモリ捕捉時は
 *  ++Mシーケンス番号 メモリ操作I/F名 アドレス 捕捉サイズ (捕捉サイズ合計)
 *                    I/F指定文字列
 *                    捕捉関数 in ソースファイル line ライン数
 * の形で出力する。シーケンス番号は出力があるたびにインクリメントされ、
 * 最大値(＝COM_DEBUG_SEQNO_MAX)までいったら 1 に戻る。
 * (ただし必ずしも 1になるとは限らない、詳細は後述)
 * シーケンス番号はファイル監視の番号とは別の値になる。
 *
 * I/F指定文字列はメモリ捕捉I/F(例えば com_malloc()等)で指定したデバッグ用の
 * 文字列内容。
 *
 * メモリ解放時の出力内容は、メモリ捕捉時とほぼ同じになる。違うのは
 * ・行頭の ++ が -- になる。
 * ・3行目に表示されるのは捕捉関数ではなく解放関数になる。
 * という2点となる。
 * シーケンス番号は捕捉時と同じ値になる。そのためメモリ解放が行われるまで
 * そのシーケンス番号が別のメモリ監視情報に使われることはない。
 * 従って、最大値まで達して 1に戻るとしても 1がまだ未開放であれば、さらに
 * その後の解放済み(＝監視情報としては未使用の状態)の番号になる。
 *
 * 1～最大値の全シーケンス番号の情報がメモリ解放されていない状態になると、
 * それ以上 メモリ監視情報を追加できなくなり COM_ERR_DEBUGNG のエラーを出す。
 * そうしたメモリを解放するときは情報なしで COM_ERR_DOUBLEFREEのエラーを出す。
 * その場合でもメモリ捕捉/解放の処理自体は行われており、プログラムの実行自体を
 * 中断することはしない。
 * (そこまでメモリを保持したままとなる処理が良いかどうかは議論が分かれる)
 *
 * realloc()を使った場合、先に元のメモリ監視情報を削除し、
 * 再捕捉したアドレスによるメモリ監視情報を改めて追加する動きになる。
 * アドレスに変更がない場合でも、この動きになる。
 *
 * toscomの処理内でも com_malloc()等によるメモリ捕捉は行っているが、
 * 本来見るべきユーザーのデバッグログが見づらくなるため、メモリ監視の出力は
 * 殆ど抑制している(ユーザーによる解放が必要な場合は出力する)。
 * そうした理由から、メモリ監視のシーケンス番号は 1から始まることはなく、
 * 飛び飛びに見えることも多いだろうが、それが正しい動きとなる。
 *
 * ただ問題発生時、抑制しているログが必要になる可能性はゼロではない。
 * そうした場合に備え、makefileには -DNOTSKIP_MEMWATCH というオプションが
 * 用意されている。そのコメントアウトを外すと、メモリ監視のデバッグログ出力を
 * 一切抑制しなくなる。かなりのログが出るが、浮き発生した監視情報のシーケンス
 * 番号がわかれば、そこから捕捉したタイミングを辿れるだろう。toscomの処理で
 * 浮き発生が見つかった場合は、御連絡をお願いする。
 *
 * com_debugMemoryErrorOn()で指定するシーケンス番号を調べる時も、出力抑制を
 * この方法で解除するほうが作業しやすいケースがあると思われる。
 *
 * malloc()等の標準関数を直接使用する場合、当然ながらメモリ監視機能は働かない。
 * 逆に言えばメモリ監視から外したいときは、わざと標準関数を使用する、という
 * 方法が採れなくはないが、あまり推奨はしない。
 *
 * プログラム開始当初から設定を反映させたいなら、com_initializeSpec()で
 * 本I/Fを呼ぶ必要がある。本I/F設定が makefileの設定より優先される。
 * (ビルド時に makefileの -DWATCHMEM で指定できる)
 */
void com_setWatchMemInfo( COM_DEBUG_MODE_t iMode );

/*
 * メモリ監視設定取得  com_getWatchMemInfo()
 *   現在のメモリ監視設定のモードを返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 */
COM_DEBUG_MODE_t com_getWatchMemInfo( void );

/*
 * メモリ捕捉NG発生ON  com_debugMemoryErrorOn()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * iSeqnoにメモリ監視情報のシーケンス番号を設定すると、
 * そのシーケンス番号とそれ以降のメモリ捕捉で NGを返すようになる。
 * (つまり 0 か 1 を指定すると、最初から NGになる)
 */
void com_debugMemoryErrorOn( long iSeqno );

/*
 * メモリ捕捉NG発生OFF  com_debugMemoryErrorOff()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * メモリ捕捉を通常通り行うように戻す。
 */
void com_debugMemoryErrorOff( void );

/*
 * ファイル監視設定  com_setWatchFileInfo()
 *   ＊COM_DEBUG_SILENTは、プログラム終了時の浮きのみ画面出力する。
 *     (デバッグログには全て出力する)
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * COM_DEBUG_ON/COM_DEBUG_SILENTに設定すると、ファイルのオープン/クローズ時に
 * ログを残すようになる。そしてクローズ漏れがあった場合、プログラム終了時に
 * 出力する。
 *
 * ファイルオープン時は
 *  ++Fシーケンス番号 ファイル操作I/F名 アドレス (現ファイルオープン数)
 *                    ファイル名
 *                    オープン関数 in ソースファイル line ライン数
 * の形で出力する。シーケンス番号は出力があるたびにインクリメントされ、
 * 最大値(＝COM_DEBUG_SEQNO_MAX)までいったら 1 に戻る。
 * (ただし必ずしも 1になるとは限らない、詳細は後述)
 * シーケンス番号はメモリ監視の番号とは別の値になる。
 *
 * ファイルクローズ時の出力内容は、オープン時とほぼ同じになる。違うのは
 * ・行頭の ++ が -- になる。
 * ・3行目に表示されるのはオープン関数ではなくクローズ関数になる。
 * という2点となる。
 * シーケンス番号はオープン時と同じ値になる。そのためクローズが行われるまで
 * そのシーケンス番号が別のファイル監視情報に使われることはない。
 * 従って、最大値まで達して 1に戻るとしても 1がまだ未クローズであれば、さらに
 * その後のクローズ済み(＝監視情報としては未使用の状態)の番号になる。
 *
 * 1～最大値の全シーケンス番号の情報がファイルクローズされていない状態になると、
 * それ以上 ファイル監視情報を追加できなくなり COM_ERR_DEBUGNG のエラーを出す。
 * そうしたファイルのクローズ時は情報なしで COM_ERR_DOUBLEFREEのエラーを出す。
 * その場合でもファイルオープン/クローズの処理自体は行われており、
 * プログラムの実行自体を中断することはしない。
 * (そこまでメモリを保持したままとなる処理が良いかどうかは議論が分かれる)
 *
 * toscomの処理内でも com_fopen()等によるファイルオープンは行っているが、
 * 本来見るべきユーザーのデバッグログが見づらくなるため、ファイル監視の出力は
 * 殆ど抑制している(ユーザーによるクローズが必要な場合は出力する)。
 * そうした理由から、ファイル監視のシーケンス番号は 1から始まることはなく、
 * 飛び飛びに見えることも多いだろうが、それが正しい動きとなる。
 *
 * ただ問題発生時、抑制しているログが必要になる可能性はゼロではない。
 * そうした場合に備え、makefileには -DNOTSKIP_FILEWATCH というオプションが
 * 用意されている。そのコメントアウトを外すと、ファイル監視のデバッグログ出力を
 * 一切抑制しなくなる。かなりのログが出るが、浮き発生した監視情報のシーケンス
 * 番号がわかれば、そこから捕捉したタイミングを辿れるだろう。toscomの処理で
 * 浮き発生が見つかった場合は、御連絡をお願いする。
 *
 * com_debugFopenErrorOn()等で指定するシーケンス番号を調べる時も、出力抑制を
 * この方法で解除するほうが作業しやすいケースがあると思われる。
 *
 * fopen()等の標準関数を直接使用する場合、当然ながらファイル監視機能は働かない。
 * 逆に言えばファイル監視から外したいときは、わざと標準関数を使用する、という
 * 方法が採れなくはないが、あまり推奨はしない。
 *
 * プログラム開始当初から設定を反映させたいなら、com_initializeSpec()で
 * 本I/Fを呼ぶ必要がある。本I/F設定が makefileの設定より優先される。
 * (ビルド時に makefileの -DWATCHFILE で指定できる)
 */
void com_setWatchFileInfo( COM_DEBUG_MODE_t iMode );

/*
 * ファイル監視設定取得  com_getWatchFileInfo()
 *   現在のファイル監視設定のモードを返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 */
COM_DEBUG_MODE_t com_getWatchFileInfo( void );

/*
 * ファイルオープンNG発生ON  com_debugFopenErrorOn()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * iSeqnoにファイル監視情報のシーケンス番号を設定すると、
 * そのシーケンス番号とそれ以降のメモリオープンで NGを返すようになる。
 * (つまり 0 か 1 を指定すると、最初から NGになる)
 */
void com_debugFopenErrorOn( long iSeqno );

/*
 * ファイルオープンNG発生OFF  com_debugFopenErrorOff()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * ファイルオープンを通常通り行うように戻す。
 */
void com_debugFopenErrorOff( void );

/*
 * ファイルクローズNG発生ON  com_debugFcloseErrorOn()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * iSeqnoにファイル監視情報のシーケンス番号を設定すると、
 * そのシーケンス番号のファイルクローズで NGを返すようになる。
 * (他のNG発生I/Fと異なり、当該シーケンス番号のみが対象であることに注意)
 * iSeqnoに 0を指定すると、全てのファイルクローズでNGを返すようになる。
 *
 * 本機能によりファイルクローズNGになった場合、com_fclose()は EINVALを返す。
 * このエラー番号は通常の fclose()が設定しないものである。
 */
void com_debugFcloseErrorOn( long iSeqno );

/*
 * ファイルクローズNG発生OFF  com_debugFcloseErrorOff()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   複数スレッドで呼ばれることは想定しない。
 * ===========================================================================
 * ファイルクローズを通常通り行うように戻す。
 */
void com_debugFcloseErrorOff( void );


/*
 * エラーコード登録  com_registerErrorCode()
 *   もしエラーが発生した場合、プログラムは強制終了する。
 * ---------------------------------------------------------------------------
 *   登録時の com_realloc()によるエラー (強制終了対象)
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iListで示すエラーコードと名前文字列を comモジュールに登録する。
 * iListは配列の形で表現できる線形データを想定しており、その最後のデータは
 * 少なくとも code が COM_ERR_END となっている必要がある。これにより本I/Fは
 * エラーコードのリスト終了を検出するためである。
 *
 * 実際のデータはエラーコードを keyにしたソートテーブルに保持している。
 * そのためどんな順番で登録してもエラーコードを昇順ソートした状態で
 * 登録されていく。ソートテーブル生成時に COM_SKIP_SORTを指定しているため
 * 同一key＝エラーコードの登録が既にあったらそれは破棄して処理続行する。
 * 破棄された旨の画面出力は行うが、そういった事が起きないように作ること。
 *   ＊ソートテーブルの詳細は本ファイルの COMSORTセクションを参照。
 *
 * エラーコードのうち個別に使用できるのは 1～899 とする。
 * 900以降は comモジュールが使用するために予約されている。
 * 敢えて 900以降を指定しても comモジュールで登録済みなら破棄されるので、
 * 意図した処理にはならないだろう。
 *
 * エラーコードは comモジュールでは COM_ERR_～ という列挙体宣言としている。
 * プログラム個別のエラーコードについては、com_spec.h で宣言しており、その
 * エラーコードと名前文字列を com_spec.c の gErrorNameSpec[]に追加すれば良い。
 * 後は com_initialize()実行時に com_registerErrorCodeSpec()が呼ばれるため、
 * comモジュールに正しく登録される。
 *
 * 他の独自モジュールからエラーコードを追加しても問題ないが、
 * プログラムの起動処理内でその登録は行うことを推奨する。
 * 登録用 iListの作り方サンプルは com_spec.cの gErrorNameSpec[]となる。
 *
 * エラーコードの宣言や設定が様々なヘッダやソースに散らばる結果となり、
 * 一覧がしづらくなっているが、モジュール独自のエラーコードも存在するので、
 * このような方法とした。既に登録されている一覧については、プログラムの中で
 * com_outputErrorCount()を使えば、出力することが可能。
 *
 * 本I/Fで一切エラーコードを登録していない状態の場合、エラー表示はするが、
 * エラーカウントの処理は行わない(処理上 不具合になるわけではない)。
 * toscomモジュールの初期化処理を未実施の時に起こり得る。
 */

// エラーコード登録用データ
typedef struct {
    long   code;   // エラーコード (64bit時を考慮し、long型で定義)
    char*  name;   // エラー名(画面出力用)
} com_dbgErrName_t;

void com_registerErrorCode( const com_dbgErrName_t *iList );

/*
 * エラーメッセージ出力  com_error()・com_errorExit()・com_errorSelect()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * iCodeで指定したエラー出力を行う。iFormat以降は出力する付加情報となる。
 * com_errorExit()はエラー出力後、プログラムを強制終了する。
 *
 * com_errorSelect()だけは iReturn の引数があり、これを falseにすると、
 * エラー出力後、プログラムを強制終了する。
 *
 * いずれのエラー出力I/Fも、使用するとエラー回数をエラーごとにカウントし、
 * ひとつでもカウントがあれば、プログラム終了時にその内容を出力する。
 * この出力有無は、デバッグ表示設定(com_setDebugPrint())に依存する。
 * これが COM_DEBUG_OFFなら何も出力しない。COM_DEBUG_ONなら画面とデバッグログに
 * 出力し、COM_DEBUG_SILENTならデバッグログにのみ出力する。
 *
 * com_registerErrorCode()で登録していないエラーコードを iCodeに指定した場合
 * それによって処理に不具合が起こることは無いが、エラー回数のカウントはしない
 * (というか、できない)。
 *
 * 一番最後に発生したエラーコードは保持しており、com_getLastError()で取得可能。
 *
 * 用法を理解しているなら、com_errorFunc()を直接利用しても特に問題は無いが、
 * 適切なマクロを使った方が記述は圧倒的に楽なはず。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_error( long iCode, const char *iFormat, ... );
 */
#ifndef CHECK_PRINT_FORMAT
#define com_error( CODE, ... ) \
    com_errorFunc( (CODE), true, COM_FILELOC, __VA_ARGS__ )
#else  // make checkfを打った時に設定されるマクロ
#define com_error( CODE, ... ) \
    COM_UNUSED( CODE ); \
    printf( __VA_ARGS__ )
#endif

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_errorExit( long iCode, const char *iFormat, ... );
 */
#ifndef CHECK_PRINT_FORMAT
#define com_errorExit( CODE, ... ) \
    com_errorFunc( (CODE), false, COM_FILELOC, __VA_ARGS__ )
#else  // make checkfを打った時に設定されるマクロ
#define com_errorExit( CODE, ... ) \
    COM_UNUSED( CODE ); \
    printf( __VA_ARGS__ )
#endif

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_errorSelect( long iCode, BOOL iReturn, const char *iFormat, ... );
 */
#ifndef CHECK_PRINT_FORMAT
#define com_errorSelect( CODE, RETURN, ... ) \
    com_errorFunc( (CODE), (RETURN), COM_FILELOC, __VA_ARGS__ )
#else  // make checkfを打った時に設定されるマクロ
#define com_errorSelect( CODE, RETURN, ... ) \
    COM_UNUSED( CODE ); \
    COM_UNUSED( RETURN ); \
    printf( __VA_ARGS__ )
#endif

void com_errorFunc(
        long iCode, BOOL iReturn, COM_FILEPRM, const char *iFormat, ... );

/*
 * エラー出力先切り替え  com_useStderr()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   データをスレッドごとに持っており、スレッドセーフとなる。
 * ===========================================================================
 * 前述のエラー出力I/Fを使用した時の画面出力先を切り替える。
 * iUseに trueを指定した場合は stderr、falseを指定した場合は stdout になる。
 * 初期状態では stdout になる。
 *
 * プログラム開始当初から設定を反映させたいなら、com_initializeSpec()で
 * 本I/Fを呼ぶ必要がある。
 */
void com_useStderr( BOOL iUse );

/*
 * 最新エラーコード取得  com_getLastError()
 *   一番最後に発生したエラーコードを返す。
 *   何もエラーが発生していない場合は COM_NO_ERROR を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   データをスレッドごとに持っており、スレッドセーフとなる。
 * ===========================================================================
 * 何らかのエラーが発生した時に、何のエラーだったか参照する目的で使用する。
 *
 * たいていのI/Fは処理結果を true/falseで返すのみのため、falseだったときに
 * もしエラーが発生していれば、本I/Fで最新エラーコードをチェックすることにより
 * そのNG要因を呼び元が特定しやすくなる。
 *
 * イメージとしては標準ライブラリの errno と同じ扱いになる。
 * これを使うつもりなのであれば、対象となる処理の前に com_resetLastError()で
 * 最新エラーコードの情報を初期化することを強く推奨する。
 */
long com_getLastError( void );

/*
 * 最新エラーコード初期化  com_resetLastError()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   データをスレッドごとに持っており、スレッドセーフとなる。
 * ===========================================================================
 * com_getLastError()で返すエラーコードを COM_NO_ERROR で初期化する。
 *
 * 最新エラーコードは基本的にエラーが発生した時に参照するものだが、
 * 過去のエラーが残ったままだと問題が分かりづらくなるという状況の時に
 * 本I/Fで先に初期化しておくと良い。
 */
void com_resetLastError( void );

/*
 * エラー文字列取得  com_strerror()
 *   取得した文字列をバッファに格納し、そのアドレスをそのまま帰す。
 *   取得NGの場合は "<strerror_r NG>" を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * エラー番号(errno)からエラー内容の文字列を取得する標準関数 strerror()は
 * スレッドセーフではないため、strerror_r()を使った処理で文字列を返す。
 * 返す文字列バッファはスレッドごとに保持した共有バッファであるため、
 * その文字列が継続して必要な場合は、速やかに別のバッファにコピーすること。
 *
 * iErrnoに入るのは標準ヘッダ変数の errno であり、com_getLastError()で取得した
 * 値が対象ではないことに注意すること。
 */
char *com_strerror( int iErrno );

/*
 * エラーフック  com_hookError()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * エラーが発生したときに、まず iFuncをコールバックするようになる。
 *
 * iFuncは発生したエラーについての情報を引数で渡されるので、
 * 独自のエラー処理を動作させることが可能になる。
 *
 * また iFuncの返り値により、通常のエラー処理を継続するかどうかも指定可能。
 * 特に注意するべきは iFuncが COM_HOOKERROR_SILENT を返す場合。
 * これは通常のエラー出力はしないが、エラー発生数のカウントは実施する。
 * そのためコールバックされた iFuncでは何らかのエラー発生を示す出力を出すことが
 * 強く推奨される。そうしなければ、プログラム終了後に、なぜエラーが発生したのか
 * 分からなくなってしまう。
 *
 * iFuncの引数 iInfに含まれるデータの内容は com_hookErrInf_t の宣言を参照。
 */

// フック関数の返り値
typedef enum {
    COM_HOOKERR_EXEC,     // エラー処理をそのまま実行する。
    COM_HOOKERR_SKIP,     // エラー処理をスキップする。
    COM_HOOKERR_SILENT    // エラー処理はするが、エラー出力はしない。
} COM_HOOKERR_ACTION_t;

// フック関数で渡す情報構造体
typedef struct {
    long         code;       // 発生エラーコード
    char*        errLog;     // 出力予定エラーメッセージ
    const char*  fileName;   // 発生ソースファイル名
    long         line;       // 発生ライン数
    const char*  funcName;   // 発生関数名
} com_hookErrInf_t;

// フック関数のプロトタイプ宣言
typedef COM_HOOKERR_ACTION_t (*com_hookErrCB_t)( com_hookErrInf_t *iInf );

void com_hookError( com_hookErrCB_t iFunc );

/*
 * 個別エラーカウント取得  com_getErrorCount()
 *   指定されたエラーコードのエラーカウントを返す。
 *   存在しないエラーコードの場合は 0を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 画面出力は何もせず、単純に iCodeで指定されたエラーコードのエラーカウントを
 * 返す。特定のエラーが発生しているかチェックしたい時や、独自のエラーカウント
 * 出力のためにエラーカウントを取得したいときに使用する想定。
 */
ulong com_getErrorCount( long iCode );

/*
 * 現在のエラーカウント出力  com_outputErrorCount()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   mutexによる排他処理が動作するためスレッドセーフとなる。
 * ===========================================================================
 * 現在のエラーカウント一覧を出力する。
 * ここに望みのエラーコードが出力されない場合、com_registerErrorCode()による
 * エラーコード登録が足りていない。
 *
 * デバッグ表示設定が COM_DEBUG_OFF以外の場合、一つでも何かエラー出力があったら
 * プログラム終了時に本I/Fによるエラー一覧を出力する。
 */
void com_outputErrorCount( void );


#ifdef USE_FUNCTRACE     // コンパイルオプション指定時のみ有効なI/F

/*
 * 関数呼出トレース情報出力  com_dispFuncTrace()
 *   ＊本機能は Linux上ではある程度動くことが確認できていますが、
 *     Cygwinでは動かずにプログラムが落ちる、というベータ版機能です。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドについては考慮済み。
 * ===========================================================================
 * 関数呼出トレース機能を使用しているときに、現在保持している情報を出力する。
 * この機能を使用するためには、コンパイルオプションを複数指定する必要がある。
 * 添付の makefileでは「デバッグオプション」の場所に「関数呼出トレース使用」
 * とコメントして必要なオプションを書いた行があるので、そのコメントを外す。
 * デバッグ機能であるため、必要がないときは動かさない想定となる。
 *
 * 必要なコンパイルオプションを付けてビルドすると、無条件で関数呼出トレース
 * 機能は動作し、関数呼出するたびにその関数名などをトレース情報として収集する。
 * トレース情報の最大数は com_custom.h で COM_FUNCTRACE_MAX で宣言しており、
 * 最初は 1000 となる。この値は任意で変更しても問題ない。最大数を超えたら、
 * 先頭に戻って情報を上書きしていく。不足を感じるときは最大数を増やすと良い。
 *
 * トレースは main()から開始するが、以下については内部の関数呼出トレースを
 * スキップする。
 * ・初期化関数  com_initialize～()
 * ・終了関数    finalize～()
 * ・画面出力＆ロギング関数  com_printf()等
 * ・デバッグ用関数
 * これらを全てトレースすると、内容が冗長になり、そして殆ど意味がない為である。
 * 逆に言うと、上に挙げていない toscomのI/Fは内部関数呼出も含めて全てトレース
 * するので、時に不要な情報が大量にあふれる可能性もある。自作した処理内でも
 * 「トレースは不要」と思う箇所があることもあるだろう。そうした処理の前後で
 *     com_setFuncTrace( false );  // 関数呼出トレースOFF
 *     (トレースを省略したい処理)
 *     com_setFuncTrace( true );   // 関数呼出トレースON
 * と記述することで、関数呼出トレースの対象から外すことが可能となる。
 *
 * あるいは最初から false を指定しておき、本当に見たい部分だけ true にする、
 * という方が問題を発見しやすいかもしれない。この場合、com_initializeSpec()に
 * com_setFuncTrace( false ); を記述するのがタイミング的に一番良い。
 * com_setFuncTrace()の詳細は、後述のI/F説明を確認すること。
 *
 * 関数呼出トレース使用時は、プログラム終了時にトレース情報を出力する。
 * トレース情報が保持するのは、呼び出した関数のアドレスとネストになる。
 * 添付の makefileでは、関数呼出トレースに必要なオプションを設定した場合、
 * ビルド時に .namelist.toscom というファイルを作る。これは実行用バイナリに
 * nmコマンドを使った出力結果で、これがあればアドレスから関数名とファイル位置を
 * 取得できるようになる。ファイル名は COM_NAMELISTで宣言しており変更可能。
 *
 * このファイルがない場合、内部の情報から関数名を引き出そうとするが、
 * ・外部公開間数しか名前が取得できない。
 *   (static定義された関数は <<internal function>> と代替出力する)
 * ・ファイル位置までは分からない。
 * という違いがあるため .namelist.toscom を使用することが推奨される。
 * 添付の makefileを変更しなければ、このファイルは自動で生成されるし、
 * make cleanで削除もされる。(名前を変更した場合 makefileも要修正)
 */
void com_dispFuncTrace( void );

/*
 * 関数呼出トレース機能ON/OFF  com_setFuncTrace()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドについては考慮済み。
 * ===========================================================================
 * 関数呼出トレース機能を使うためのコンパイルオプションを使用時で
 * (makefileのデバッグ機能の記述の中に、このオプションも含まれている)
 * 一時的に機能をOFFにしたい場合に使用する。その時は iModeに falseを指定する。
 * trueを指定すると、再び機能をONにできる。
 * 本I/Fを何も使っていない最初の状態は ON(true)である。
 *
 * 入れ子の構造にも対応する。つまり、一度 falseを指定した状態で、
 * 更に falseを指定した場合、次に trueを指定しても、最初の false指定が有効で
 * まだ関数呼出トレースは OFFのままとなる。もう一度 trueを指定したら ONになる。
 *
 * toscomの幾つかのI/Fは内部で本I/Fを使って、内部の関数呼出トレースを抑制する。
 * 現状は初期化/終了・画面出力とデバッグ機能のI/Fがその対象となっている。
 * それ以外のI/Fは基本的に機能ONのままなので、一部のI/Fはトレース情報が大量に
 * 生成される要因となり得る。sonoI/F使用に問題がない場合は、本I/Fを使って、
 * トレース機能をOFFにすると情報がみやすくなる。
 */

/*
 * プロトタイプ形式 (この形で使用すること)
 *   void com_setFuncTrace( BOOL iMode );
 */
#define com_setFuncTrace( MODE ) \
    com_setFuncTraceReal( (MODE), COM_FILELOC )

void com_setFuncTraceReal( BOOL iMode, COM_FILEPRM );

#else   // USE_FUNCTRACE未指定時に、関数記述自体を消して、動作させなくする

#define com_dispFuncTrace()
#define com_setFuncTrace( MODE )

#endif  // USE_FUNCTRACE

/*
 * ネームリストアドレス検索  com_seekNameList()
 *   見つかった場合は、名前とファイル位置を示す文字列を返す。
 *   見つからなかった場合は NULLを返す。
 *   ネームリストがない場合も NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iAddr
 * ===========================================================================
 *   マルチスレッドについては考慮済み。
 * ===========================================================================
 * com_cutom.h の COM_NAMELIST で指定されたネームリストが生成されている場合に
 * iAddrで示されたアドレスを検索し、対応する関数名/データ名とファイル位置を
 * 文字列として返す。
 * ただし「アドレス タイプ トークン名 ファイル位置」という書式になっていない
 * アドレスがヒットした場合、ファイル位置が正しく取得できない。基本的に
 * 本I/Fで対象となるのは、関数とグローバル変数である。
 *
 * なお本I/Fは関数呼出トレース機能で使用されるが、関数呼出トレース機能の
 * コンパイルオプションを指定していなくても使用可能。COM_NAMELISTで宣言された
 * 名前で所定のネームリストが作られていれば、いつでも機能する。
 */
const char *com_seekNameList( void *iAddr );



/*
 *****************************************************************************
 * COMTEST:テスト関連I/F (ベースは com_test.c だが 個別にファイル作成を推奨)
 *****************************************************************************
 */

/*
 * テストコードモード  com_testCode()
 * ---------------------------------------------------------------------------
 *   エラー発生はテストコードの内容に依存。
 * ===========================================================================
 *   マルチスレッドについて考慮するかどうかは、コードの書き手の判断に依存。
 * ===========================================================================
 * 個人用のテストコードを走行させることが可能。
 * com_～.c の形式のファイル名でソースファイルを作り、
 * 起点としてこの関数を書く。後はテスト用にどんなコードを書いても良い。
 * 実行時のパラメータが入力として渡されるので、処理の切り分けはそれで
 * 実施することが出来るだろう。
 *
 * テストコードを使わない場合、本I/Fを呼ばないという選択肢もあり得る。
 * 試験方法が他にあるならば、それでも何ら問題はない。
 *
 * toscomで開発環境を作った場合、com_test.c というファイルが存在するはずで
 * そこに本I/Fも記述している。具体的な使い方については com_test.c を参照。
 */
void com_testCode( int iArgc, char **iArgv );


/*****************************************************************************/
/* 共通で使用できる独自I/Fの宣言 */
#include "com_spec.h"
/*
 * 末尾でのインクルードとしているのは、com_if.hで宣言した様々な構造体等を
 * com_spec.h でも使用できるようにするため。
 */

