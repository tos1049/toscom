/*
 *****************************************************************************
 *
 * 共通部エキストラ機能     by TOS
 *
 *   エキストラ機能が必要な場合のみ、このヘッダファイルをインクルードし、
 *   com_extra.cをコンパイルするソースに含めること。
 *
 *   このファイルより先に com_if.h をインクルードすること。
 *
 *   現在のエキストラ機能
 *   ・COMEXBASE:  エキストラ機能基本I/F
 *   ・COMEXINPUT: キー入力関連I/F
 *   ・COMEXSTAT:  統計情報/数学計算I/F
 *   ・COMEXRAND:  乱数関連I/F
 *   ・COMEXSIG:   シグナル関連I/F
 *   ・COMEXPACK:  データパッケージ関連I/F
 *   ・COMEXREGXP: 正規表現関連I/F
 *
 *   なお、本ヘッダファイルで float.h と math.h をインクルードするので、
 *   倍精度変数と各種数学関数も副次的に使用可能となる。
 *
 *****************************************************************************
 */

#pragma once

#include <signal.h>
#include <regex.h>

/*
 *****************************************************************************
 * COMEXBASE: エキストラ機能基本I/F
 *****************************************************************************
 */

// エキストラ機能エラーコード定義  ＊連動テーブルは gErrorNameExtra[]
enum {
    COM_ERR_READTXTNG  = 930,        // 複数行テキスト読み込みNG
    COM_ERR_RANDOMIZE  = 931,        // 乱数生成NG
    COM_ERR_SIGNALNG   = 932,        // シグナル関連NG
    COM_ERR_REGXP      = 934,        // 正規表現処理NG
};

/*
 * エキストラ機能初期化  com_initializeExtra()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない
 * ===========================================================================
 *   マルチスレッドで起動することは想定していない。
 * ===========================================================================
 * com_initialize()の後に、これも実行すること。
 *
 * COM_INITIALIZE()マクロを使用する場合、直接 本I/Fの記述は必要なく、マクロを
 * 記述しているソース冒頭で、本ヘッダファイルをインクルードするだけで良い。
 * (通常、この方法を推奨する)
 */
void com_initializeExtra( void );



/*
 *****************************************************************************
 * COMEXINPUT: キー入力関連I/F
 *****************************************************************************
 */

/*
 * com_input()で使用する入力文字のチェック関数は com_if.h にて宣言された
 * コンフィグ値のチェック関数(com_validator_t)をそのまま使用可能。
 * このチェック関数の iCond には、各関数用の条件データを指定する。
 *
 * チェック結果は true (チェックOK)/false (チェックNG) で返す。
 * com_input()では、falseが返ったら再入力となる。
 * -------------------------------------------------------------------------
 * 一般的なパターンとして下記のチェック関数は com_if.h に存在する。
 *    com_valDigit()        long型数値範囲チェック
 *    com_valDgtList()      long型数値リストチェック
 *    com_valUDigit()       ulong型数値範囲チェック
 *    com_valUDgtList()     ulong型数値リストチェック
 *    com_valHex()          16進数範囲チェック
 *    com_valString()       文字列長範囲チェック
 *    com_valStrList()      文字列リストチェック
 *    com_valBool()         TRUE か FALSE (大文字/小文字区別なし)
 *    com_valYesNo()        YES か NO (大文字/小文字区別なし)
 *    com_valOnOff()        ON か OFF (大文字/小文字区別なし)
 *
 * com_extra.hで、更に以下のチェック関数を用意する。
 *    com_valOnlyEnter()    Enterキーのみ押下チェック
 *    com_valSaveFile()     保存ファイル名入力(既存なら上書き確認)
 *    com_valLoadFile()     読込ファイル名入力(存在するものを入力)
 *
 * Yes/No に関しては入力処理をよりサポートするために以下を用意する。
 * 以下の関数はチェック関数ではないので、com_input()の代わりに使用する。
 *    com_isYes()           文字列が YES か Y であることを判定
 *    com_askYesNo()        Yes/Noの入力を求め、true/falseで結果を返す。
 *
 * また com_select.h には以下のチェック関数が用意されている。
 *    com_valIpAddress()    IPアドレス(v4/v6指定可能)チェック
 *
 * 独自に com_validator_t型のチェック関数を自作しても、何も問題ない。
 * 例えば、上記のチェック関数を複数使った判定をしたいと思う場合、
 * 複数を呼んで判定する新たな関数を用意する必要がある。
 */

/*
 * エンターのみ入力チェック  com_valOnlyEnter()
 *   チェック結果を true/false で返す。
 *   com_input()で使用時は、falseが返ったら再入力となる。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 * ===========================================================================
 * 表示のウェイトのときなどに使用することを想定。
 * com_input()で iFlag->enterSkipを false にした場合、永久に抜けられなくなる
 * ので、その指定はしないように注意すること。
 *
 * 特に条件指定は無いため、com_input()での条件指定は NULL とする。
 *
 * リターン待ち処理を共通化した com_waitEnter() もあるので、特に問題が無ければ
 * そちらを使うことで、処理記述を簡略化出来るだろう。
 *
 * マクロ COM_VAL_ONLYENTER は
 * com_addCfgValidator()で本I/Fを使用するための宣言となる。
 * 詳細は com_addCfgValidator() の説明を参照。
 */

////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_ONLYENTER \
    com_valOnlyEnter, NULL, NULL

BOOL com_valOnlyEnter( char *ioData, void *iCond );
// 条件はないので、コピー/解放関数も不要。

/*
 * 保存ファイル名チェック  com_valSaveFile()
 *   チェック結果を true/false で返す。
 *   com_input()で使用時は、falseが返ったら再入力となる。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただしファイル操作が別プロセスで行われることには対応できない。
 * ===========================================================================
 * ioDataをファイル名として扱い、ファイルが無ければ true を返す。
 * ファイルがあった場合は、上書き確認をし、Yesなら true、Noなら false を返す。
 *
 * iCondで条件を設定する。NULL指定時は条件は全て falseとして動作する。
 * iCondは com_valCondSave_t型のアドレスで、既存ファイルありのときの処理を
 * 制御する。
 * .fileExist は既存ファイルがあったときのメッセージ内容を指定する。
 *  ファイル名に当たる %s を必ず1回 文中に含めること。NULL指定時は
 *  "%s already exists. are you sure to overwrite?\n" となる。
 * .forceRerry を trueにすると、確認無しで、無条件 falseを返す。
 * .forceOverwrite を trueにすると、確認無しで 無条件 trueを返す。
 * もし .forceRetry と .forceOverwrite のどちらも true にした場合は、
 * .forceRetry が優先され、確認無しで falseを返す動作となる。
 * どちらかが true の場合 .fileEixst に設定した文字列は使用機会無しとなる。
 *
 * COM_VAL_SAVEFILE・com_valSaveFileCondCopy()・com_valSaveFileCondFree()は
 * com_addCfgValidator()で本I/Fを使用するための宣言となる。
 * 詳細は com_addCfgValidator() の説明を参照。
 */

////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_SAVEFILE \
    com_valSaveFile, com_valSaveFileCondCopy, com_valSaveFileCondFree

// com_valSaveFile()用 条件指定構造体
typedef struct {
    char*  fileExist;        // ファイル存在時に出力する内容
    BOOL   forceRetry;       // true ならファイル存在時 確認無しで falseを返す
    BOOL   forceOverwrite;   // true ならファイル存在時 確認無しで trueを返す
} com_valCondSave_t;

BOOL com_valSaveFile( char *ioData, void *iCond );
// 以下は com_addCfgValidator() でのみ使用する
BOOL com_valSaveFileCondCopy( void **oCond, void *iCond );
void com_valSaveFileCondFree( void **oCond );

/*
 * 読込ファイル名チェック  com_valLoadFile()
 *   com_input()で使用時は、falseが返ったら再入力となる。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理は無い。
 *   ただしファイル操作が別プロセスで行われることには対応できない。
 * ===========================================================================
 * ioDataをファイル名として扱い、ファイルがあったら true を返す。
 * ファイルが無ければ false を返す。
 * "-" を入力した場合も、特別に true を返す。これは入力キャンセルを想定した
 * 動作となるが、使い方は呼び元に委ねられる。
 *
 * iCondで条件を設定する。NULL指定時は条件は全て falseとして動作する。
 * iCondは com_valCondLoad_t型のアドレスで、既存ファイルありのときの処理を
 * 制御する。
 * .cancel は"-"の代わりに許容する文字列を指定する(入力キャンセル用想定)。
 *  NULL指定時は上記説明通り "-" 入力時に trueを返すようになる。
 *  その後 呼び元で "-" だったら処理キャンセルとなるようにコードを書くこと。
 * .noFile はファイルが無かった時に出力する文字列を指定する。
 *  ファイル名に当たる %s を必ず1回 文中に含めること。NULL指定時は
 *  "%s not exist...\n" となる。
 * .noDisp が trueの場合、ファイルが無かったら何も出力せず falseを返す。
 *  この場合 .noFile の設定を使用する機会はなくなる。
 *  
 * COM_VAL_LOADFILE・com_valLoadFileCondCopy()・com_valLoadFileCondFree()は
 * com_addCfgValidator()で本I/Fを使用するための宣言となる。
 * 詳細は com_addCfgValidator() の説明を参照。
 */

////////// com_addCfgValidator()で使用できる関数名マクロ //////////
#define COM_VAL_LOADFILE \
    com_valLoadFile, com_valLoadFileCondCopy, com_valLoadFileCondFree

// com_valLoadFile用 条件指定構造体
typedef struct {
    char*  cancel;   // 入力キャンセルに使用する文字列(デフォルトは "-")
    char*  noFile;   // ファイルが無いときに出力する内容
    BOOL   noDisp;   // trueの場合 ファイルが無いときに 無条件で falseを返す。
} com_valCondLoad_t;

BOOL com_valLoadFile( char *ioData, void *iCond );
// 以下は com_addCfgValidator() でのみ使用する
BOOL com_valLoadFileCondCopy( void **oCond, void *iCond );
void com_valLoadFileCondFree( void **oCond );

/*
 * 文字列入力  com_input()
 *   有効な入力文字数を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oData
 * ===========================================================================
 *   排他制御はしていないため、必要に応じて呼び元で実施すること。
 *   入力用バッファを分けたい時は com_inputVar()の使用を検討する。
 * ===========================================================================
 * oData で入力文字列バッファアドレス格納用のポインタを指定する。
 * (char*型で変数を定義し、&を付けて引数にすることを想定)
 * 内部定義の入力用共通バッファのアドレスを *oData に格納して返す。
 *
 * iVal は入力内容のチェック方法のデータアドレスを設定する。
 * チェック不要なら iVal 自体を NULL指定する。
 *  .func は使用するチェック関数を指定する。com_val～() を使用可能なほか、
 *   同じプロトタイプで自作した関数を指定しても問題ない。
 *  .cond はそのチェック関数で使用する条件データのアドレスを指定する。
 *   どんな型のデータかはチェック関数ごとに決まっているので、その型で
 *   データを作成し、そのアドレスを渡せば良い。条件不要なら NULL にする。
 *   void*型にしているので、キャスト無しでアドレスを直接渡せる。
 *
 * iFlag は入力プロンプトの挙動指定のデータアドレスを設定する。
 * フラグの内容については、構造体宣言を参照。
 * NULLを指定した場合、全て falseという扱いになる。
 *
 * iFormat以降は入力プロンプト文の文字列となる。書式文字列が指定可能。
 * 入力プロンプトが不要の場合 NULL を指定する。
 *
 * oDataで返されるデータバッファは解放不要だが、com_input()が呼ばれるたびに
 * クリアされて再利用されるため、入力後も内容保持が必要な場合は、com_input()
 * から返った後、速やかに別バッファにコピーすること。
 * もしくは com_inputVar()で格納先を直接指定する方が簡単かもしれない。
 */

// 入力チェック内容データ構造体
typedef struct {
    com_validator_t  func;    // 使用する入力チェック関数
    void*            cond;    // チェック条件
} com_valFunc_t;

// 入力時の動作フラグ設定構造体
typedef struct {
    BOOL   clear;       // true ならプロンプト表示前に画面クリア
    BOOL   enterSkip;   // true なら Enterのみで入力スキップ(0を返す)
} com_actFlag_t;

size_t com_input(
        char **oData, const com_valFunc_t *iVal,
        const com_actFlag_t *iFlag, const char *iFormat, ... );

/*
 * 文字列入力 (格納変数指定型)  com_inputVar()
 *   有効な入力文字数を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oData
 * ===========================================================================
 *   排他制御はしていないため、必要に応じて呼び元で実施すること。
 * ===========================================================================
 * 機能は com_input()とほぼ同じで、入力内容の格納先を自分で指定出来る点が
 * 異なる。格納先のアドレスを oDataで指定し、そのサイズを iSizeで指定する。
 * 文字列を入れるので「最大文字数 + 1」のサイズが必要になることに注意。
 * サイズを超えた文字列入力は切り捨てる。
 * 最初に指定されたバッファの0クリアを行ってから、格納する。
 */
size_t com_inputVar(
        char *oData, size_t iSize, const com_valFunc_t *iVal,
        const com_actFlag_t *iFlag, const char *iFormat, ... );

/*
 * 複数行テキスト入力  com_inputMultiLine()
 *   入力されたデータサイズを返す。
 *   0 なら、データ読み出しでNGが発生したことを示す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oData
 *   COM_ERR_READTXTNG: テキスト読み込みの fread() でエラー発生
 * ===========================================================================
 *   排他制御はしていないため、必要に応じて呼び元で実施すること。
 * ===========================================================================
 * oDataは入力文字列格納用バッファ、iSizeはそのサイズを指定する。
 * その内容解釈は呼び元に委ねることとし、内容チェックは行わない。
 * 改行込みのテキストを受付、oDataに格納する。
 *
 * CTRL+D を押すと入力終了になる。
 * ただし最後が改行ではない場合、CTRD+D は2回入力する必要がある。
 *
 * iFormat以降は先に表示する入力プロンプトの内容となる。書式文字列が使用可能。
 * 入力プロンプト不要の場合、NULLを指定する。
 */
size_t com_inputMultiLine(
        char *oData, size_t iSize, const char *iFormat, ... );

/*
 * Yes/No入力支援  com_isYes()・com_askYesNo()
 *   "YES" か "Y" なら true、"NO" か "N" なら false を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない
 * ===========================================================================
 *   排他制御はしていないため、必要に応じて呼び元で実施すること。
 * ===========================================================================
 * com_isYes()は iData が "YES" か "Y" なら true、それ以外なら false を返す。
 * 大文字/小文字は区別しない。主に com_input()で受け取った入力文字列を対象と
 * して想定しているが、他の文字列を対象にしても問題ない。
 *
 * com_askYesNo()は 内部で com_input()を使った文字入力を促し、
 * その入力結果が "YES" か "Y" なら true、"NO" か "N" なら false を返す。
 * 大文字/小文字は区別しない。
 * iFlag、iFormatとそれ以降は、そのまま com_input()に渡す。
 * iFormatを NULLにした場合、"(Yes/No)" という入力プロンプトになる。
 */
BOOL com_isYes( const char *iData );
BOOL com_askYesNo( const com_actFlag_t *iFlag, const char *iFormat, ... );

/*
 * 共通用エンター待機  com_waitEnter()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない
 * ===========================================================================
 *   排他制御はしていないため、必要に応じて呼び元で実施すること。
 * ===========================================================================
 * iFormat以降は入力プロンプトの文字列となる。画面クリアはしない。
 * (クリアしたいなら、先に com_clear() を使うと良い)
 * iFormat を NULL指定した場合 "--- HIT ENTER KEY ---" をプロンプトとする。
 */
void com_waitEnter( const char *iFormat, ... );

/* ファイル入力とオープン  com_askFile()
 *   入力された名前のファイルを com_fopen()で開き、その結果をそのまま返す。
 *   (ファイルが開ければそのファイルポインタ、開けなければNULLを返す)
 *   開いたファイルは com_fclose()で閉じる必要がある。
 * ---------------------------------------------------------------------------
 *   com_fopen()によるエラー
 * ===========================================================================
 *   排他制御はしていないため、必要に応じて呼び元で実施すること。
 * ===========================================================================
 * com_input()の処理を利用して、ファイル名入力を実施し、そのファイルを開く。
 * 読込/書込2種類の用法がある。
 *
 * iFlagsはファイル名入力のための com_input()にそのまま渡すフラグとなる。
 *
 * iAskCondはファイル名問い合わせ条件で、以下の値を想定する。
 *  .type  読込目的なら COM_ASK_LOADFILE、書込目的なら COM_ASK_SAVEFILE
 *  .valCond.load  読込目的時に com_input()に渡す入力条件
 *                 ＊入力文字列のチェックに com_valLoadFile()を使用する。
 *                   構造体の内容については、本ファイルのそちらの説明を参照。
 *  .valCond.save  書込目的時に com_input()に渡す入力条件
 *                 ＊入力文字列のチェックに com_valSaveFile()を使用する。
 *                   構造体の内容については、本ファイルのそちらの説明を参照。
 * 読込か書込かで .valCondも設定内容が変わることに注意。
 *
 * iFormat以降で、ファイル名入力時のプロンプト文字列を指定する。
 *
 * com_input()の処理を使ったファイル名入力が適正であれば、
 * (読込で指定ファイルが無い場合は不適切、書込で指定ファイルがある場合、
 *  その上書きを許容するかどうかは .valCond.save の内容次第となる)
 * com_fopen()を使用して、そのファイルを開き、その結果をそのまま返す。
 *
 * ファイルを開くことになるので、呼び元で適切な機会に com_fclose()で閉じること。
 */

// 取得ファイル種別
typedef enum {
    COM_ASK_LOADFILE,       // ファイル読み込み
    COM_ASK_SAVEFILE        // ファイル書き込み
} COM_ASK_TYPE_t;

typedef struct {
    long   type;            // 取得ファイル種別 (COM_ASK_TYPE_tの値を指定)
    union {
        com_valCondLoad_t load;   // ファイル読み込み時の入力条件
        com_valCondSave_t save;   // ファイル書き込み時の入力条件
    } valCond;              // ファイル名入力条件
} com_askFile_t;

FILE *com_askFile( const com_actFlag_t *iFlags, const com_askFile_t *iAskCond,
                   const char *iFormat, ... );

/*
 * セレクター動作  com_execSelector()
 *   メニュー実行結果を返す
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iSelector
 *   メニュー内容保持のための com_realloct() によるエラー(プログラム強制終了)
 * ===========================================================================
 *   排他制御はしていないため、必要に応じて呼び元で実施すること。
 * ===========================================================================
 * iSelectorで定義されたメニューデータからメニューテキストを生成出力し、
 * 選択項目の数値入力待ちになる。数値入力すると、それに対応する関数を実行する。
 *
 * iSelector は com_selector_t型の線形データの先頭アドレスを想定する。
 * .code が 0になったデータを最後にすることで、そこまでの複数データを
 * メニューとして出力するようになる。
 *   .code はメニュー番号(0より大きな値で、2桁以内の数値を推奨)
 *   .label はメニュー項目の文字列(60文字未満を推奨)
 *   .check に関数が指定されている場合、その関数が trueを返したらメニュー表示、
 *    falseなら そのメニュー部分は空欄となる。チェック不要なら NULL指定する。
 *    空欄とする場合 .label の文字列長に合わせた空白が入る。
 *   .func は そのメニュー番号を入力された時に実行する関数で、その返り値を
 *    com_execSelector() は返す。NULL指定時は無条件で trueを返す。
 *
 *  ＊なお .check や .func は引数なしの関数であるため、
 *    何らかの引数が必要な処理にしたい場合 com_execSelector() は使えない。
 *    やり取りするデータをグローバル変数定義するか、データ取得関数を作成し
 *    それを呼び出す、といった工夫が必要になる。
 *
 * iPromptはメニューの書式を設定したデータのアドレスになる。
 *   .head はメニュー先頭に表示するヘッダ文字列(NULLなら何も表示しない)
 *   .foot はメニュー末尾に表示するフッタ文字列(NULLなら何も表示しない)
 *    メニュー本体は改行しないので、必要なら .foot に  "\n"を指定すると良い。
 *   .borderLf はメニュー番号による改行単位を指定する。
 *    例えば 10 にすると 1～10、11～20、21～30・・という単位で改行を入れる。
 *    正確な改行のタイミングは (メニュー番号 - 1) / (.borderLf値) と
 *    (直前のメニュー番号 - 1) / (.borderLf値) が異なる場合、となる。
 *    .borderLf を 10にしている場合、出力するメニューの番号が、
 *    例えば 1, 2, 90, 3, 4, 12, 13 の順だった場合、2・90・4 の後で改行する。
 *    間に 90 が入らない場合 2 の後に改行はしない。
 *   .prompt はメニュー各行の先頭に付する文字列を指定する。
 *    NULLなら何も付けずにメニュー表示を始める。
 *
 * メニュー表示後 com_input()を使って数値入力を行う。
 * iFlagは その com_input()にそのまま渡す動作フラグとなる。
 * メニューで表示した項目の数字だけを許容するようにチェック関数を設定する。
 *
 * 繰り返しになるが、数字を入力したら その番号の iSelector[]->func を
 * 関数コールし、その返り値をそのまま返す。
 *
 * 入力された番号に対応した関数が呼ばれることで、大抵は事足りるはずだが、
 * 実際に入力された番号が必要になった場合は、com_getSelectedMenu()を使う。
 *
 * また Enterのみ押した時にデフォルト入力されるメニュー番号を設定したいときは
 * com_setDefaultMenu()を使う。具体的な使い方の注意については、このI/Fの
 * 説明記述を確認すること。
 */

// メニュー処理関数プロトタイプ
//   ユーザーにより入力されたメニュー番号を入力として渡す。
//   それ以外に渡すデータが必要な場合グローバル変数使用を検討する。
//   主に処理成否の意味で true/false を返すが、その使い方は呼び元次第。
typedef BOOL (*com_selectFunc_t)( long iCode );

// メニュー項目データ構造体
//   com_selector_t型の配列変数を定義する想定で、
//   その最後のデータは 必ず .code が 0以下となるようにする。
//   (その場合の .label・.check・.func の内容は不問。NULLが無難)
//   このデータがなければ com_execSelector()はメニュー項目データの終わりを
//   検出できない。
typedef struct {
    long               code;    // メニュー番号
    char*              label;   // メニュー項目名
    com_selectFunc_t   check;   // メニュー出現チェック関数(trueで出現)
    com_selectFunc_t   func;    // メニュー実行関数
} com_selector_t;

// プロンプト書式構造体
typedef struct {
    char*   head;       // メニューヘッダ
    char*   foot;       // メニューフッタ
    long    borderLf;   // メニュー番号による改行単位
    char*   prompt;     // 各行のプロンプト
} com_selPrompt_t;

BOOL com_execSelector(
        const com_selector_t *iSelector, const com_selPrompt_t *iPrompt,
        const com_actFlag_t *iFlag );


/*
 * セレクター入力メニュー番号取得  com_getSelectedMenu()
 *   com_execSelector()で入力されたメニュー番号を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   排他制御は考慮不要。
 * ===========================================================================
 * 一番最後に使用した com_execSelector()で実際に入力されたメニュー番号を
 * そのまま返す。
 */
long com_getSelectedMenu( void );

/*
 * セレクターデフォルトメニュー番号設定  com_setDefaultMenu()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   排他制御は考慮不要。
 * ===========================================================================
 * com_execSelector()で Enterのみ入力された時のメニュー番号を設定する。
 * 本I/F使用後の com_execSelector()呼出では Enterのみの入力を受け付けるために、
 * 第3引数 iFlag の .enterSkip を trueにする必要がある。
 *
 * 以下の場合は Enterのみ押しても 0 が入力された扱いとなり、何も処理はしない。
 * ・iFlagの .enterSkip を trueにしていても、本I/Fでデフォルト設定していない。
 * ・実際のメニューに存在しない番号をデフォルト設定している。
 */
void com_setDefaultMenu( long iCode );



/*
 *****************************************************************************
 * COMEXSTAT: 統計情報/数学計算I/F
 *****************************************************************************
 */

// 統計データ計算用構造体
typedef struct {
    long   count;            // データ件数
    BOOL   needList;         // リスト保持要否
    BOOL   existNegative;    // 負数データ有無(負数が入力されると true)
    long*  list;             // データリスト (.needList が true時に生成)
    double total;            // 合計値     (統計計算に使用)
    double lgtotal;          // 対数合計値 (統計計算に使用)
    double rvtotal;          // 逆数合計値 (統計計算に使用)
    double sqtotal;          // 平方合計値 (統計計算に使用)
    double cbtotal;          // 立法合計値 (統計計算に使用)
    double fototal;          // ４乗合計値 (統計計算に使用)

    // 以下が統計計算の結果となる
    // .existNegative が true の場合、.hrmavr・.geoavr は計算せず 0固定

    long   max;              // 最大値 (maximum)
    long   min;              // 最小値 (minimum)
    double average;          // 算術平均値 (arithmetic average)
    double geoavr;           // 幾何平均値 (geometric average)
    double hrmavr;           // 調和平均値 (hermonic average)
    double variance;         // 分散 (variance)
    double stddev;           // 標準偏差 (standard deviation)
    double coevar;           // 変動係数 (coefficent of variation)
    double skewness;         // 歪度 (skewness)
    double kurtosis;         // 尖度 (kurtosis)
} com_calcStat_t;

/*
 * 統計データ計算開始  com_readyStat()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oStat
 * ===========================================================================
 *   マルチスレッドの影響は受けない
 * ===========================================================================
 * 統計情報の計算データを初期化する。
 * com_calcStat_t型の変数を定義し、そのアドレスを oStatで渡す想定。
 *
 * iNeedListを true にすると、入力された数値データを oStat->list に保持する。
 * false の場合 入力データの保持が必要なら呼び元で実施すること。
 *
 * その後、com_inputStat() で ひとつずつ数値を入力し、その都度計算する。
 * 全ての処理が終わったら com_finishStat() で統計データのメモリ解放を実施する。
 *
 * 実際に算出されるデータが何かは com_calcStat_t型の説明を参照。
 */
void com_readyStat( com_calcStat_t *oStat, BOOL iNeedList );

/*
 * 統計データ入力＆計算  com_inputStat()
 *   処理成否を true/false で返す。
 *   falseになるのは、合計値や平方合計値でオーバーフローが発生した時、
 *   または 入力データを保持するメモリが確保できなかった時。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oStat
 *   入力値を保持する場合(oStat->needList==true)のみ com_reallloct()のエラー
 * ===========================================================================
 *   マルチスレッドの影響は受けない
 * ===========================================================================
 * oStat は com_readyStat()で初期化済みのデータを指定する。
 * iData は実際の入力数値で、それを oStat内部で統計情報として計算し、
 * 結果を保持する。つまり .max以降のデータが計算保持された状態となっている。
 * 入力数値ひとつずつ 本I/Fを使用する必要がある。
 *
 * 統計情報の計算結果は oStatに格納されるだけで他に出力はしないので、
 * 画面出力は必要に応じて呼び元が構造体メンバーを指定して実施すること。
 * double型の数値出力時の書式文字列は %lf ではなく %f であることに注意。
 *
 * com_readyStat() で iNeedList を trueにしていた場合、
 * 入力された iDataをひとつずつメモリ確保して oData->list に保持する。
 * このメモリ確保は com_finishStat() を使用することで解放される。
 */
BOOL com_inputStat( com_calcStat_t *oStat, long iData );

/*
 * 統計データ計算終了  com_finishStat()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oStat
 * ===========================================================================
 *   マルチスレッドの影響は受けない
 * ===========================================================================
 * 統計データのメモリ解放を実施する。統計情報 oStat自体の解放は行わないので、
 * 動的に確保していた場合は、呼び元で責任を持って解放すること。
 */
void com_finishStat( com_calcStat_t *oStat );

/*
 * 素数判定  com_isPrime()
 *   素数であれば true、素数でなければ false を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドの影響は受けない
 * ===========================================================================
 * iNumberが素数かどうかを、試し割り法で判定する。(つまり最も原始的手法)
 * 従って iNumber が大きくなればなるほど判定に時間がかかる。
 * ・・とはいうものの、例えば uint型の範囲の数値であれば、どんなに大きくても
 * ミリ秒オーダーであり、実用には足ることが多いと思われる。
 */
BOOL com_isPrime( ulong iNumber );



/*
 *****************************************************************************
 * COMEXRAND: 乱数関連I/F
 *****************************************************************************
 */

/*
 * 乱数生成  com_rand()
 *   生成した乱数(1以上)を返す。
 *   Linuxでは int最大値を超える乱数は生成できない。
 *   処理NGが発生した場合は、無条件で 0 を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_RANDOMIZE: 標準関数(initstate_r/srandom_r/random_r)でNG発生
 *                      Windows(Cygwin)では srandom/random でNG発生
 * ===========================================================================
 *   マルチスレッドの影響は受けない処理を使用している。
 *   ただし Windows(Cygwin)では ～_r の標準関数が使用できないため、
 *   マルチスレッドではうまく動作しない可能性がある。
 * ===========================================================================
 * 1～iMaxまでの乱数を返す。
 * iMaxを負数にした場合は、-1～-iMaxまでの乱数を返す。
 * iMaxを 0にした場合は 0しか返さない。
 * 本I/Fを一番最初に呼んだ時に、現在時刻を srandom()にシードとして与える。
 *
 * --- Linux向け固有情報 ---
 * 本I/Fを一番最初に呼んだ時に、現在時刻を srandom_r()にシードとして与える。
 * このI/Fを使うための状態情報バッファはスレッドごとに保持する。
 * また、random_r()が int型を返却するため、iMax は long型だが、int型の
 * 最大値/最小値を超える値を設定しても、最大値/最小値に補正されるので注意。
 */
long com_rand( long iMax );

/*
 * 確率判定  com_checkChance()
 *   判定結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   com_rand()で発生するエラー
 * ===========================================================================
 *   マルチスレッドの影響は受けない処理を使用している。
 * ===========================================================================
 * iChanceでパーセンテージ(0～100)を指定すると、その確率成否を返す。
 * com_rand()で1～100の乱数を生成し、iChance以下なら true、大きかったら false
 * を返すというロジックになる。
 * なお「確率論で信用できるのは 0 か 100 だけ」である。
 */
BOOL com_checkChance( long iChance );

/*
 * ダイスロール  com_rollDice()
 *   結果を数値で返す。
 * ---------------------------------------------------------------------------
 *   com_rand()で発生するエラー
 * ===========================================================================
 *   マルチスレッドの影響は受けない処理を使用している。
 *   ただ、iDisp に true を指定した場合、com_printf()を使用する都合から
 *   com_printf()を始めとする画面出力I/Fの引数に本I/Fを直接使用すると
 *   デッドロックが発生する(普通はやらない処理記述とは思われる)。
 * ===========================================================================
 * iDiceが振る個数、iSideがダイス面数、iAdjustを増減補正値として、com_rand()を
 * 使ってダイスを振って計算した値を返す。例えば iDice=2、iSide=6、iAdjust=2 なら
 * 6面ダイス(通常のサイコロ)を2個振って合計した値に ＋2 して返す。
 *
 * iDispを trueにした場合、結果を画面出力する。
 */
long com_rollDice( long iDice, long iSide, long iAdjust, BOOL iDisp );



/*
 *****************************************************************************
 * COMEXSIG: シグナル関連I/F
 *****************************************************************************
 */

// シグナルハンドラー構造体
typedef struct {
    long               signal;   // sigaction()に渡す時は intでキャストする
    struct sigaction   action;
} com_sigact_t;

// struct sigaction について   (詳細は man 2 sigactionも参照)
//   toscomではシグナルハンドラーの指定としてこの構造体を使用する。
//   元来は signal() で設定してきたが、これを使うことは非推奨となってきており
//   代わりに sigaction()が推奨されているため、これに倣っている。
//
//       struct sigaction {
//           void      (*sa_handler)(int);
//           void      (*sa_sigaction)(int, siginfo_t *, void *);
//           sigset_t    sa_mask;
//           int         sa_flags;
//           void      (*sa_restorer)(void);
//       };
//
//   伝統的な signal()で登録していたシグナルハンドラーは .sa_handler になる。
//   返り値は返さず(void)、引数は int型一つで 受信したシグナル番号が入る。
//
//   さらに機能強化された .sa_sigaction も用意されているが、処理系によっては
//   unionで同じ保持領域となる可能性もあるため、両方とも指定するというのは
//   避けたほうが良い模様(どのみち どちらか片方しか支えない)。
//
//   .sa_mask はシグナルハンドラー実行中にブロックすべきシグナルを指定する。
//   .sa_flags は動作に関するフラグを設定する。もしシグナルハンドラーとして
//   元来の .sa_handler ではなく .sa_sigaction を使用したい場合、最低でも
//   このフラグに SA_SIGINFO を設定する必要がある。
//   .sa_restorer は「使用すべきではない」とされているので説明は省略する。

/*
 * シグナルハンドラー設定  com_setSignalAction()
 *   設定のための sigaction()に全て成功したら trueを返す。
 *   1つでも失敗したら、全ての登録をもとに戻して falseを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_SIGNALING: sigaction()の処理NG
 *   COM_ERR_DEBUGNG: 既に登録済みのシグナルを更に登録しようとした
 *                    シグナル番号(com_sigact_t の .signum)が INT_MAXを超えた
 *   変更前設定保持のための com_addSortTableFunc()によるエラー
 * ===========================================================================
 *   データはスレッドごとに保持するため、スレッドセーフとなる。
 * ===========================================================================
 * iSigListでフレーム起動時のシグナルハンドラーを指定する。線形データで、
 * 最後のデータの .signum を COM_SIGACT_END とすることでリスト終了を示す。
 *
 * データの定義例としては以下のようになる：
 *     static com_sigact_t gSigHandler[] = {
 *         { SIGINT,         { .sa_handler = quitPorc },
 *         { SIGTSTP,        { .sa_handler = SIG_IGN },
 *         { SIGUSR1,        { .sa_handler = SIG_IGN },
 *         { SIGUSR2,        { .sa_handler = SIG_IGN },
 *         { COM_SIGACT_END, { .sa_handler = NULL } }
 *     };
 * この gSigHandler はそのまま iSigList として指定可能である。
 * (参考までに CTRL+C で SIGINT、CTRL+Z で SIGTSTP のシグナルを発行する)
 * .sa_handler に関数ではなく SIG_IGN を指定すると「シグナル無視」となる。
 *
 * sigaction()により、変更前の設定を取得できるので、それを内部で保持する。
 * これによりシグナルハンドラーの設定を元に戻すことが可能になる。
 * (能動的に戻したい時は com_resumeSignalAction() を使用する)
 * 既に変更前設定を保持済みのシグナルが、更に指定された場合、最初のデータを
 * そのまま保持するが、警告として COM_ERR_DEBUGNG のエラーを出す。
 * これは予期せぬ重複登録になっている可能性への警告である。
 *
 * シグナル名のマクロ宣言は signal.h に記述があるが /usr/include/signal.h
 * ではなく そこから参照される
 *     /usr/include/bits/signal.h
 *     /usr/include/sys/signal.h
 * などに実際の記述があるかもしれない。
 */
BOOL com_setSignalAction( const com_sigact_t *iSigList );

enum {
    COM_SIGACT_END = -1     // シグナルハンドラーリストの終端指定
};

/*
 * シグナルハンドラー復旧  com_resumeSignalAction()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: 登録されていないシグナルを指定した
 *                    iSignum > INT_MAX
 *   データ削除に伴う com_deleteSortTable()や com_freeSortTable()によるエラー
 * ===========================================================================
 *   データはスレッドごとに保持するため、スレッドセーフとなる。
 * ===========================================================================
 * iSignumで指定したシグナルハンドラーを変更前の状態に戻す。
 * これにより 変更前の設定も削除されるので com_setSignalAction()で
 * 別のシグナルハンドラーをエラー無しに設定し直せるようになる。
 *
 * iSignum に COM_SIGACT_ALL を指定すると全てのシグナルハンドラーを元に戻し
 * 変更前の設定を保持する内部データも全てメモリ解放する。
 * この内部データは処理終了時に自動解放も試みるため、放置しても良い。
 *
 * ただ、複数スレッドで個別に設定をしている場合、各スレッドで最後に本I/Fを
 * 使って元に戻したほうが良いかもしれない。
 * (スレッド終了時のリソース放棄があるので、恐らく放置しても良いが)
 */
void com_resumeSignalAction( long iSugnum );

enum {
    COM_SIGACT_ALL = -1      // シグナル全指定
};



/*
 *****************************************************************************
 * COMEXPACK: データパッケージ関連I/F
 *****************************************************************************
 */

// データパッケージ管理情報構造体
typedef struct {
    char*    filename;         // 書込/読込するファイル名
    BOOL     writeFile;        // true:書込、false:読込
    BOOL     byText;           // true:テキスト保存、false:バイナリ保存
    BOOL     useZip;           // 圧縮有無
    char*    archive;          // 圧縮ファイル名 (com_zipFile()に渡す)
    char*    key;              // パスワード (com_zipFile/com_unzipFileに渡す)
    BOOL     noZip;            // .zip付与       (com_zipFile()に渡す)
    BOOL     delZip;           // 展開後削除     (com_unzipFile()に渡す)
    ///// 以降はI/F内部用データ
    FILE*    fp;
} com_packInf_t;

/*
 * データパッケージ開始  com_readyPack()
 *   処理成否を true/false で返す。
 *   com_finishPack()による明示的な処理終了が必要なので注意すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioInf || !ioInf->filename
 *   com_fopen()・com_fclose()・com_unzipFile() によるエラー
 * ===========================================================================
 *   スレッドのことは現状無考慮。
 *   ただ複数スレッドで同一ファイルを操作することがなければ、問題ない想定。
 * ===========================================================================
 * データファイルの書込/読込をするための処理を開始する。
 * 書込/読込どちらであっても、本I/Fが起点となる。
 * zip/unzipを使ったファイルの圧縮/展開も可能とする。
 *
 * 具体的な設定は iInfにて通知する。その実データを作りアドレスを渡す想定。
 * *ioInfの内容は書込時と読込時で同じにする必要がある(.writeText と .fp以外は)。
 *
 * .filename はデータファイル名(圧縮時はさらに名前を変えることも可能)。
 * ファイルは必ず新規作成とし、既存ファイルへの追加書込には対応しない。
 * .writeText が true であれば書込、false であれば読込の処理となる。
 * .byText が true であればテキスト書込、false であればバイナリ書込 となる。
 *
 * .useZip が true であれば、書込時は最後に zip圧縮、読込時は最初に unzip展開
 * を実施する。それ以降のメンバーは主に圧縮/展開用のパラメータである。
 *
 * 書込時の zip圧縮には com_zipFile()を使用する。これに渡す引数が
 *   .archive  ->  iArchive
 *   .filename ->  iSource
 *   .key      ->  iKey
 *   .noZip    ->  iNoZip
 *   true固定  ->  iOverwrite
 * となる。com_zipFile()の引数についての説明はそちらのI/F説明を参照すること。
 * com_zipFile()の引数のうち、iOverwriteは true固定ではあるが、
 * 本I/Fの iConfirm に文字列を設定すると、.filename のファイルが既に存在時、
 * その文字列をプロンプトとして使用して上書き確認を実施する。
 * iConfitm を NULL にした場合、無条件で上書きする。
 *
 * 読込時の unzip展開には com_unzipFile()を使用する。これに渡す引数が
 *   .archive/.filename/.noZip から生成したファイル名  -> iArchive
 *   NULL固定  ->  iTargetPath
 *   .key      ->  iKey
 *   .delZip   ->  iDelete
 * となる。com_unzipFile()の引数についての説明はそちらのI/F説明を参照すること。
 * com_unzipFile()の引数の内、iTargetPathは NULL固定となる。
 * 展開の結果 .filename で指定した名前のファイルが生成されることを期待する。
 * もし違う場合は処理NGとなる。圧縮時と同じ .archive/.filename/.noZip の
 * 設定になっていれば、このNGにはならない。
 *
 * 本I/F使用後、実際のデータ書込は com_writePack() か com_writePackDirect()、
 * 実際のデータ読込は com_readPack() か com_readPackDirect() を使用する。
 * 全ての処理を終えたら、com_finishPack()を使用する必要もある。
 * 書込時のファイル圧縮を実際に行うのは com_finishPack() になる。
 */
BOOL com_readyPack( com_packInf_t *ioInf, const char *iConfirm );

/*
 * データパッケージ終了  com_finishPack()
 *   処理成否を true/false で返す。
 *   com_finishPack()による明示的な処理終了が必要なので注意すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioInf
 *   com_fclose()・com_zipFile() によるエラー
 * ===========================================================================
 *   スレッドのことは現状無考慮。
 *   ただ複数スレッドで同一ファイルを操作することがなければ、問題ない想定。
 * ===========================================================================
 * ioInf には com_readyPack()を実施したデータのアドレスをそのまま渡し、
 * データパッケージの処理を終了させる。データ書込/読込していたファイルの
 * クローズをここで実施するため、必ず本I/Fを最後に呼ぶ必要がある。
 *
 * iFinal を trueにすると完了処理、false にすると NG完了処理を実施する。
 *
 * 完了処理の場合 開いていたファイルをクローズする。
 * また 書込時でファイル圧縮を指定時(ioInf->useZip == true)であれば、
 * ファイル圧縮を実施する。
 * 書込/読込どちらでも 圧縮/展開をした後の 元ファイルは削除する。
 * (この「元ファイル」とは ioInf->filename で指定されたファイルを指す)
 *
 * NG完了処理の場合 作業対象となっていた ioInf->filename は無条件削除する。
 * 圧縮指定時でも圧縮を行う対象が無いので実施しない。この処理は、
 * データ書込/読込の途中でNGが発生して処理中断する事態を想定している。
 */
BOOL com_finishPack( com_packInf_t *ioInf, BOOL iFinal );

// パッケージアクセス用データ構造体
typedef struct {
    void*    data;      // 書込時：書き込むデータ、読込時：読み出し先
    size_t   size;      // データサイズ
    BOOL     varSize;   // 可変サイズか
} com_packElm_t;

/*
 * データパッケージ書込  com_writePack()・com_writePackDirect()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioInf || !iElm || !iCount  (com_writePack)
 *                                !ioInf || iAddr       (com_writePackDigest)
 * ===========================================================================
 *   スレッドのことは現状無考慮。
 *   ただ複数スレッドで同一ファイルを操作することがなければ、問題ない想定。
 * ===========================================================================
 * com_readyPack()で初期化済みの ioInfによって開かれたデータファイルに対して
 * データ書き込みを実施する。
 * 初期化する時は ioInf->writeText を true にすることが必須となる。
 *
 * 書き込むデータの指定方法でI/Fが２つある。
 *
 * com_writePack()
 *   iElm で書き込むデータを指定する(配列構造で複数指定が可能)。
 *     .data : データの先頭アドレス(書き込みたいデータ変数名に & を付ける想定)
 *             ただしポインタ変数(&は付けない)でデータを指定したい場合
 *             .varSize を true にしてデータサイズも書き込ませること。
 *     .size : データのサイズ
 *     .varSize: trueの場合、データ書込時にデータサイズも含める。
 *   iCount は書き込むデータの個数を指定する。
 *
 * com_writePackDirect()
 *   com_writePack()の iElm の内容を直接個別に設定する。
 *     iData :     com_packElm_t の .data
 *     iSize :     com_packElm_t の .size
 *     iVarSize :  com_packElm_t の .varSize
 *
 * つまり com_writePack()は複数データを一度に指定できるが、
 * com_writePackDirect()は一つのデータのみを指定する、ということになる。
 *
 * データ設定について、もう少し詳細を以下に示す。
 *
 * .data に設定するアドレスが 実体変数名に & を付けていた場合
 * その変数のデータサイズを .size に指定し .varSize は false になる。
 * この場合データファイルに書き込まれるのは 実際の内容のみとなる。
 * 詠込側が同じサイズで(＝特定の型の変数として)詠込むことを期待する。
 *
 * ポインタ変数で示すデータエリアを指定したい場合、
 * .data にはそのポインタ変数名を指定し(&は付けない)、
 * .size はそのデータエリアのサイズを指定し .varSize は true とする。
 * この場合データファイルにはサイズと内容が書き込む。
 * 読込側でもサイズ込みでの詠込むことを期待する。
 *
 * なお .varSize が true の時に書き込めるサイズは 65535(0xFFFF) が上限となる。
 * これを超えて書き込もうとすると、エラーになり書込は行われない。
 * これより大きなサイズのデータを書き込みたい時は、データを幾つかに分割して
 * 書込み可能なサイズにすることを検討する。
 *
 * データの書込方法は com_writePack()・com_writePackDirect() ともに同じなので
 * 読込時に対応するように com_readPack()・com_readPackDirect() を使う必要は
 * なく、書込時と同じ順番・サイズで処理できれば どちらを使っても問題ない。
 */
BOOL com_writePack( com_packInf_t *ioInf, com_packElm_t *iElm, long iCount );
BOOL com_writePackDirect(
        com_packInf_t *ioInf, void *iData, size_t iSize, BOOL iVarSize );

/*
 * データパッケージ読込  com_readPack()・com_readPackFix()・com_readPackVar()
 *   com_readPack()は処理成否を true/false で返す。
 *
 *   com_readPackDirect()は 読み込んだデータを保存したアドレスを返す。
 *   メモリ確保失敗など処理NGが発生した場合は NULL を返す。
 *   (iVarSizeが false の場合 iData がそのまま返る。
 *    iVarSizeが true で iData が 非NULLなら それもそのまま返る。
 *    iVarSizeが true で iData が NULLの場合 確保したアドレスが返る）
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioInf || !oElm || !coun        (com_readPack)
 *                                !ioInf || !iAddr             (com_readPackFix)
 *                                !ioInf || !ioAddr || !ioSize (com_readPackVar)
 *   com_malloc()によるエラー
 * ===========================================================================
 *   スレッドのことは現状無考慮。
 *   ただ複数スレッドで同一ファイルを操作することがなければ、問題ない想定。
 * ===========================================================================
 * com_readyPack()で初期化済みの ioInfによって開かれたデータファイルに対して
 * データ読み込みを実施する。
 * 初期化する時は ioInf->writeText を false にすることが必須となる。
 *
 * 読み込むデータの指定方法でI/Fが２つある。
 *
 * com_readPack()
 *    oElm で読み込むデータを指定する(配列構造で複数指定が可能)。
 *     .data : データの先頭アドレス(読み込みたいデータ変数名に & を付ける想定)
 *             読み込んだデータを格納する。
 *             ただしポインタ変数(&は付けない)でデータを指定したい場合
 *             .varSize を true にしてデータサイズも読み込ませること。
 *             NULLを許容し、その場合はデータサイズ分メモリを確保して、
 *             そのアドレスで上書きする。
 *     .size : データサイズ
 *             .varSizeが trueの場合、読み込んだサイズで値を上書きするので、
 *             最初の設定値は何でも構わない。
 *     .varSize: trueの場合、データ書込時にデータサイズが含まれていると期待し
 *               そのサイズ分データ読込を実施する。
 *   iCount は読み込むデータの個数を指定する。
 *   書込と異なり .data・.sizeは変更の可能性があることに注意すること。
 *   特に .dataの内容が変わる(＝NULL指定して動的確保する)場合は、
 *   読込後、書き換わったアドレスをどこかに保持する必要があるだろう。
 *
 * com_readPackFix()
 *   com_readPack()の iElm の内容を直接個別に設定するが、機能が限定される。
 *     iData :      com_packElm_t の .data    (NULLは非許容)
 *     iSize :      com_packElm_t の .size
 *     false固定 :  com_packElm_t の .varSize
 *   iData(data格納先)に NULLは指定できない。すなわち com_readPack()のように
 *   格納先のメモリを動的に確保することに対応しない。
 *   .varSize は false固定のため、これを trueにして書き込んだデータの読込は
 *   できない。
 *   (これらの制限は引数の指定と処理を複雑にすることを防ぐ目的となる。
 *
 * com_readPackVar()
 *   com_readPackFix() と逆に .varSize が true固定で読み込む。
 *   その代わりに引数の型が com_writePack～系と少し異なる結果となった。
 *     ioData :   com_packElm_t の .data は *ioData  (NULLも許容）
 *                データアドレスを保持するポインタ変数に & を付けたもの
 *                NULLを格納したポインタ変数に & を付ける事が可能で、
 *                その場合は メモリを動的確保し、そのアドレスを格納する。
 *     ioSize :   com_packElm_t の .size は *ioSize
 *                データサイズを保持する変数に & を付けたもの。
 *                iVarSizeが true でサイズを読み込むとその内容で上書きする。
 *     true固定 : com_packElm_tの .varSize
 *   *ioData と *ioSize が実際のデータとなり、I/F内で内容の変更がある得る。
 *   動的確保時(*ioData に NULL格納時)、*ioData は呼び元でメモリ解放が必要な
 *   点も忘れずに。
 *   ioData に配列変数を指定したい場合、単純に & を付けるだけでは不可能。
 *   例えば char sample[] の場合、sample と &sample は同じアドレスになる為で
 *        char* dummy = sample;
 *   というダミー変数を定義し &dummy を ioDataに指定する、という方法はある。
 *
 * つまり com_readPack()は複数データを一度に指定できてメモリ動的確保に対応し
 * com_readPackFix()・com_readPackVar()は一つのデータのみを限定的に扱う、
 * ということになる。
 *
 * ioInfの内容は書込時と同じになっている必要がある。ただし
 *   .writeFile は 書込時は true だが 読込時は false と値が変わる
 *   .fp は 内部で使用する値なので、内容を考慮する必要はない
 * 書き込み時と同じ com_packElm_t の設定を同じ順番で指定すれば読み込める。
 *
 * 読み込んだデータを保持する場所は動的な確保を可能としているので、
 * com_packElm_t型の入力データの内容は書込時と全く同じになるとは限らないが、
 * 書き込んだのと同じ順番で同じサイズのデータを読み込む必要がある。
 */
BOOL com_readPack( com_packInf_t *ioInf, com_packElm_t *oElm, long iCount );
BOOL com_readPackFix( com_packInf_t *ioInf, void *iData,  size_t iSize );
BOOL com_readPackVar( com_packInf_t *ioInf, void *ioData, size_t *ioSize );



/*
 *****************************************************************************
 * COMEXREGXP: 正規表現関連I/F
 *****************************************************************************
 */

/*
 * 正規表現は regex.h を利用して実現している。
 * その使用パターンはだいたい以下のようになる。
 *
 * iRegexp が正規表現の文字列、iLineが検索対象の文字列、
 * oMatch と iMatchSize はマッチした文字列を格納するバッファとなる。
 *
 * static BOOL getMatchedString(
 *     const char *iRegexp, const char *iLine, char *oMatch, size_t iMatchSize )
 * {
 *     // 正規表現のコンパイル
 *     com_regcomp_t compInf = { iRegexp, REG_EXTENDED };
 *     com_regex_id_t regexId = com_regcomp( &compInf );
 *     if( regexId == COM_REGXP_NG ) {return false;}
 *
 *     // 正規表現実行用データ作成
 *     com_regexec_t execInf;   
 *     if( !com_makeRegexec( &execInf, iLine, 0, 5, NULL ) ) {return false;}
 *
 *     // マッチング実施
 *     BOOL result = com_regexec( regexId, &execInf );
 *     if( result && oMatch ) {
 *         // マッチしていたら、結果の文字列を取得
 *         size_t matchSize = 0;
 *         char* matched = com_analyzeRegmatch( &execInf, 0, &matchSize );
 *         com_strncpy( oMatch, iMatchSize, matched, matchSize );
 *     }
 *     com_freeRegexec( &execInf );
 *     return result;
 * }
 *
 * もし検索成否が欲しいだけであれば、下記のように もう少しシンプルに出来る。
 *
 * static BOOL getOnlyResult( const char *iRegexp, const char *iLine )
 * {
 *     // 正規表現のコンパイル
 *     com_regcomp_t compInf = { iRegexp, REG_EXTENDED | REG_NOSUB };
 *     com_regex_id_t regexId = com_regcomp( &compInf );
 *     if( regexId == COM_REGXP_NG ) {return false;}
 *
 *     // 正規表現実行用データ作成
 *     com_regexec_t execInf = { iLine, 0, 0, NULL };
 *
 *     // マッチング実施
 *     return com_regexec( regexId, &execInf );
 * }
 *
 * ただ、いずれも同じ正規表現文字列を再度使っても、新たにコンパイルして、
 * どんどん内部データとして保持を続けてしまう。コンパイルした結果は保持されて
 * いるので、それを再利用できるようにする方が望ましい。
 * そうした作りにしたい場合は、コンパイルと実行で処理を分ける必要がある。
 * (先にコンパイルだけ実施し、以後は取得した 正規表現ID を使って検索実行する)
 */

/* 正規表現ID */
typedef long  com_regex_id_t;

#define  COM_REGXP_NG  -1

/*
 * 正規表現コンパイル  com_regcomp()
 *   コンパイルに成功したら、正規表現ID(0以上の値)を返す。
 *   コンパイルに失敗したら、COM_REGXP_NG を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iRegex  !iRegex->regex
 *   COM_ERR_REGXP:  regcomp()でエラー
 *                   コンパイル結果保持のためのメモリ確保失敗
 *   com_realloct()によるエラー
 * ===========================================================================
 *   マルチスレッドについては考慮済み。
 * ===========================================================================
 * regcomp()を使って、正規表現文字列をコンパイルし、その結果を保持する。
 * そして保持したデータのIDを 正規表現ID として返す。このIDは com_regexec()で
 * どの正規表現コンパイル結果を使って検索するか指定するために使用する。
 *
 * com_regcomp_t型のデータのアドレスを iRegexで指定する。
 * この構造体データのメンバーは下記のように設定する。
 *   .regex   実際の正規表現の文字列
 *   .cflags  regcomp()にそのまま渡す処理フラグ値。下記から指定可能。
 *            複数指定したいときは | で繋いで指定する。
 *               REG_EXTENED   regexにPOSIX拡張正規表現を使用
 *               REG_ICASE     大文字小文字の違いを無視
 *               REG_NEWLINE   オペレータに改行をマッチさせない
 *               REG_NOSUB     regexec()の nmatch・pmatch を無視
 *                             (マッチ成否のみ見る)
 *
 * この後、com_regexec()を使って、実際の正規表現マッチを実施する。
 * その時に、本I/Fで返した正規表現IDを使って、どの検索をするのか指定する。
 * 
 * コンパイル結果は toscomの内部データとして、メモリを動的に確保して保持する。
 * 保持したデータは本I/Fで返す 正規表現ID を通して使用可能となっており、
 * プログラム終了時に regfree()を用いて自動解放するので、ユーザーは意識不要。
 *
 * COM_REGXP_NG を返したときは、正規表現の処理は中断するようにコードを書くこと。
 */

// 正規表現コンパイル用データ (regcomp()に渡すデータ)
typedef struct {
    const char *regex;     // 実際の正規表現文字列
    long        cflags;    // regcomp()に渡すフラグ(com_regcomp()の説明参照)
} com_regcomp_t;

com_regex_id_t com_regcomp( com_regcomp_t *iRegex );

/*
 * 正規表現実行  com_regexec()
 *   検索マッチ成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iId < 0 || iId > 最終生成ID
 *                                !ioRegexec   !ioRegexec->target
 * ===========================================================================
 *   スレッドに関する問題は発生しない。
 * ===========================================================================
 * regexec()を使って、実際の正規表現マッチを実施し、その結果を返す。
 *
 * iId には com_regcomp()で返された正規表現IDを指定する。
 * 複数のコンパイルを実施している時、どの正規表現マッチを実施するか識別する。
 *
 * com_regexec_t型のデータのアドレスを ioRegexecに指定する。
 * regcomp()に渡すデータとなり、そのメンバーは以下のように指定する。
 *   .target  検索対象の文字列
 *   .eflags  regexec()にそのまま渡す処理フラグ値。下記から指定可能。
 *            複数指定したいときは | で繋いで指定する。
 *               REG_NOTBOL   行頭にマッチするオペレータは必ずマッチに失敗
 *               REG_NOTEOL   行末にマッチするオペレータは必ずマッチに失敗
 *   .nmatch  この次の .pmatchの要素数。
 *            com_regcomp()で .cflagsに REG_NOSUB を指定時は内容は無視される。
 *   .pmatch  検索結果を格納する配列変数のアドレス。 .nmatch が 0のときと
 *            com_regcomp()で .cflagsに REG_NOSUB を指定時は内容は無視される。
 *
 * 手動であれば、例えば以下のように指定すれば良い。
 *   enum { NMATCH = 5 };
 *   pmatch_t  pmatchInf[NMATCH];
 *   com_regexec_t  exec = { "検索対象文字列", 0, NMATCH, pmatchInf };
 *
 * 検索結果だけで良ければ .nmatch=0 .pmatch=NULL にしてしまっても問題無い。
 * (あるいは com_regcomp()の時点で REG_NOSUB を指定しても良い)
 *
 * 同じ文字列を対象にして、マッチングを複数回行いときは、最低でも
 * .nmatch=1で、それを格納できる .pmatchが必要
 * (単純に pmatch_t pmatchInf; と定義し .pmatch には &pmatchInf を与えれば良い)
 * そうして、検索を行ってマッチングした後は .target に .pmatch[0].rm_eo を加え
 * 新たな .target として検索を実施する。これをマッチングが失敗するまで
 * 次々繰り返すことで、同じ文字列に対する検索を複数回実施できる。
 *
 * com_makeRegexec()を使えば .pmatch をメモリから動的に確保し、
 * com_freeRegexec()で確保したメモリの解放することも可能。
 *
 * .pmatch に格納される情報は
 *   .pmatch[0]  マッチした文字列全体
 *   .pmatch[1]  1つ目のグループマッチした文字列
 *   .pmatch[2]  2つ目のグループマッチした文字列
 *   ....
 * となる。グループマッチは正規表現で () を使ってグループ化したら実施する。
 * 幾つグループ化しているかは明白だから、nmatch でそれが十分入る個数と、
 * その個数を確保した .pmatch を渡す必要がある。
 *
 * 実際の .pmatch[] には 以下の情報が入っている。
 *   .rm_so  マッチした文字列の先頭オフセット
 *   .rm_eo  マッチした文字列の最終オフセット
 * 例えば、以下のコードで文字列を出力できる。
 *   com_regexec_t  exec = ～;   // 構造体の名前は exec とする
 *   (中略)
 *   for( size_t idx = 0;  idx < exec.nmatch;  idx++ ) {
 *       regmatch_t *tmp = &(exec.pmatch[idx]);
 *       for( regoff_t offset = tmp->rm_so; offset < tmp->rm_eo;  offset++ ) {
 *           com_printf( "%c", exec.target[offset];
 *       }
 *       com_printLf();
 *   }
 * 正確に言うと .rm_eo はマッチした文字列の次の文字オフセットなことに注意。
 * それでも上記のような for文で使うのには、そのほうが適しているし、
 * 文字列長を計算したいときも .rm_eo から .rm_so を引くだけで良い。
 */

// 正規表現実行用データ (regexec()に渡すデータ)
typedef struct {
    const char *target;    // 検索対象の文字列
    long        eflags;    // regexec()に渡すフラグ(com_regexec()の説明参照)
    size_t      nmatch;    // マッチさせる数
    regmatch_t *pmatch;    // マッチした内容
} com_regexec_t;

BOOL com_regexec( com_regex_id_t iId, com_regexec_t *ioRegexec );

/*
 * 正規表現実行用データ生成  com_makeRegexec()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] ioRegexec || !iTarget
 *   com_malloc()によるエラー
 * ===========================================================================
 *   スレッドに関する問題は発生しない。
 * ===========================================================================
 * com_regexec()で使用する com_regexec_t型のデータを生成する。
 *
 * oRegexec は実際に設定したい対象の実体データアドレスを指定する。
 * iTarget・iEflags・iNmatch・iPmatch はそのまま *oTarget に設定する内容。
 * 
 * com_regexec()の説明でも記載した通り iPmatch に指定するべき内容は
 *   regmatch_t  pmatch[iNmatchの内容];
 * で定義したもの・・つまり上記であれば pmatch ということになる。
 *
 * iPmatch には 敢えて NULL を指定することが可能。
 * iNmatch が 0ではない場合、com_malloc()を使って、その数のメモリを確保し
 * 確保したメモリのアドレスを iPmatchの値として自動設定する。
 * メモリを動的に確保している場合、処理をすべて終えたら、
 * com_freeRegexec()でメモリ解放を実施しなければならない。
 *
 * 本I/Fを使用することは義務ではない。
 * com_regexec()の説明記述には、ローカル変数を定義して、
 * メモリの動的確保をせずに com_regexec_t型のデータを作成するパターンも
 * 例示している。この方法なら com_freeRegexec()等でメモリ解放する必要もない。
 */
BOOL com_makeRegexec(
        com_regexec_t *oRegexec, const char *iTarget, long iEflags,
        size_t iNmatch, regmatch_t *iPmatch );

/*
 * 正規表現実行用データ解放  com_freeRegexec()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] ioRegexec
 *   com_free()によるエラー
 * ===========================================================================
 *   スレッドに関する問題は発生しない。
 * ===========================================================================
 * ioRegexec->pmatch を動的に確保しているものと想定し、com_free()で解放する。
 * メモリを動的確保していない場合、落ちるので注意すること。
 *
 * com_makeRegexec()で iPmatchを NULL指定し、メモリを自動確保している場合に、
 * このI/Fでそのメモリを開放できる。
 *
 * com_makeRegexec()を使わず手動でメモリ確保していたものの解放にも使用可能だが
 * 解放するのは ioRegexec->pmatch だけであることには注意すること。
 */
void com_freeRegexec( com_regexec_t *ioRegexec );

/*
 * 正規表現検索結果文字列取得
 *   取得した文字列を指すアドレスを返す。
 *   (継続して文字列を使いたいときは、本I/F使用後、別バッファにコピーすること)
 *   文字列が取得できなかった場合は空文字を返す。
 *   エラー発生時は NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iRegexec
 *   COM_ERR_REGXP:  iIndex >= iRegexec->nmatch
 * ===========================================================================
 *   マルチスレッドについては考慮済み。
 * ===========================================================================
 * iRegexcは com_regexec()で検索して得た結果を指定する。
 * iRegexec->pmatch のうち iIndexで指定したデータを iRegexec->target から
 * 文字列として取得し、そのアドレスを返す。
 *
 * oSizeを指定すると、iRegexec->pmatch からマッチした文字列長を格納する。
 * もしエラーの場合(NULLを返す場合)、*oSizeには 0 を格納する。
 * サイズの取得が不要なときは NULL を指定すれば良い。
 *
 * 実際に返すアドレスはスレッドごとに生成された文字列バッファになる。
 * もし複数回 本I/Fを呼んだ場合、そのたびに上書きされるので、
 * 取得した文字列を継続して使いたいときは、I/F使用後 速やかに別バッファに、
 * その文字列をコピーすることを強く推奨する。
 *
 * iIndex = 0 の場合は、マッチした文字列全体を取得できるが、
 * 1以上を指定する場合は、正規表現コンパイル時に 正規表現で () を使って
 * グループ化をしていなければ、意味がないので注意すること。
 * (これは regexec()によるデータ格納の仕様である)
 *
 * 本I/Fで返す文字列バッファのサイズは COM_TEXTBUF_SIZE となっている。
 * そのため *oSizeがその値以上の場合、超えた分は入らない状態の文字列バッファの
 * アドレスが返るので注意すること。(特にエラーや警告は出さない)
 * そうした事態になる場合は、ユーザーが独自に文字列取得の処理を書いて、
 * 取得するようにするしか無い。
 * (そこまで大きなサイズの結果を正規表現で得る処理はレアとは思われるが…)
 */
char *com_analyzeRegmatch(
        com_regexec_t *iRegexec, size_t iIndex, size_t *oSize );

/*
 * 正規表現コンパイル結果取得  com_getRegex()
 *   該当するコンパイル結果を返す。
 *   指定した tIdが不正だった場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] iId < 0 || iId > 最終生成ID
 * ===========================================================================
 *   マルチスレッドについては考慮不要。
 * ===========================================================================
 * iIdで指定した正規表現IDのコンパイル結果(com_regcomp()でコンパイルして、
 * 内部に保持しているもの)を格納するアドレスを返す。
 *
 * com_regexec()など toscomのI/Fを使う限り、regex_t型のデータをユーザーが見る
 * 必要はないが、何らかの理由で必要になったときのために備えるI/Fとなる。
 *
 * com_regcomp()が実行されるたびに、アドレスは変更になる可能性があるので
 * 長期的に取得した内容を使用したいときは、別の regex_t型変数に内容をコピーし
 * そちらを使用することを強く推奨する。
 */
regex_t *com_getRegex( com_regex_id_t iId );



// エキストラ機能 導入フラグ /////////////////////////////////////////////////
#define  USING_COM_EXTRA
// エキストラ機能 初期化関数マクロ (COM_INITIALIZE()で使用) //////////////////
#ifdef   COM_INIT_EXTRA
#undef   COM_INIT_EXTRA
#endif
#define  COM_INIT_EXTRA  com_initializeExtra()

