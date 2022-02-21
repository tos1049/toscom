/*
 *****************************************************************************
 *
 * 共通部シグナル機能     by TOS
 *
 *   シグナル機能が必要な場合のみ、このヘッダファイルをインクルードし、
 *   更に以下のファイルをコンパイル対象のファイルとして含めること。
 *
 *   このファイルより先に com_if.h をインクルードすること。
 *   また、シグナル機能の追加セットを使いたい場合も、本ファイルのインクルードが
 *   先に必要となる。
 *
 *
 *   本ヘッダに記述したI/F群はシグナル機能全体で共通で使用するもののうち、
 *   直接の解析処理ではなく、その前後で必要な処理やデータ登録を行うものとなる。
 *
 *   本ヘッダの末尾で下記のヘッダもインクルードしている。
 *     com_signalCom.h:  プロトコルの解析/デコードに使う共通処理、
 *     com_signalPrt1.h: プロトコルごとの解析/デコード処理
 *   これらのヘッダに記述したI/Fも必要に応じて使用していくことになる。
 *
 *
 *   現在のシグナル機能
 *   ・COMSIGBASE:  シグナル機能基本I/F
 *   ・COMSIGTYPE:  プロトコル種別判定情報I/F
 *   ・COMSIGANLZ:  プロトコル解析情報I/F
 *   ・COMSIGREAD:  信号読込I/F
 *   ・COMSIGDEBUG: シグナル機能デバッグ用I/F
 *
 *
 *   シグナル機能では定数のマクロ/列挙体の名前について、下記の法則がある。
 *   これはシグナル機能に属する全てのソース共通のグランドルールとなる。
 *     COM_CAP_～：信号に実際に乗る値
 *     COM_SIG_～：解析処理内で正規化されて整理された値
 *
 *   特に各プロトコルを示す正規化された値を COM_SIG_～で宣言しており、
 *   その宣言は本ヘッダ以外のヘッダファイルでも定義されている。
 *   その記述の際は必ず PROTOCOL_AVAILABLE というコメントが入っており、
 *   com_signalPrt?.h では対応するプロトコルがこの宣言に一覧される。
 *
 *   本ヘッダや com_signalCom.h でも PROTOCOL_AVAILABLE を付けた宣言があるが
 *   これはプロトコル種別というよりは、解析処理で使用する数値を示したり、
 *   今は未対応だがいずれ対応を検討したいプロトコルの例示に使われている。
 *
 *****************************************************************************
 */
#pragma once

#include <stdint.h>

/*
 *****************************************************************************
 * COMSIGBASE: シグナル機能基本I/F
 *****************************************************************************
 */

// シグナル機能エラーコード定義  ＊連動テーブルは gErrorNameSignal[]
enum {
    COM_ERR_NOSUPPORT   = 960,       // サポートしていないプロトコル
    COM_ERR_NOMOREDATA  = 961,       // ファイル走査が既に終了している
    COM_ERR_NOTPROTO    = 962,       // 非プロトコルデータ
    COM_ERR_INCORRECT   = 963,       // 不正値
    COM_ERR_ILLSIZE     = 964,       // サイズNG
    COM_ERR_NODATA      = 965,       // 必要データ不足
    COM_ERR_ANALYZENG   = 966        // その他解析NG
};

/*
 * シグナル機能初期化  com_initializeSignal()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 * com_initialize()の後に、これも実行すること。
 * 本I/Fにより com_signalCom.h の com_initializeAnalyzer() と
 * com_signalPrt1.h の com_initializeSigPrt1() も実行する。
 * 
 * COM_INITIALIZE()マクロを使用する場合、直接 本I/Fの記述は必要なく、マクロを
 * 記述しているソース冒頭で、本ヘッダファイルをインクルードするだけで良い。
 * (通常、この方法を推奨する)
 */
void com_initializeSignal( void );

/*
 * 解析処理の流れ
 *
 *  (1)解析I/Fの入力となる信号データを生成する。
 *
 *    入力元となる候補に応じて、使用するI/Fを選択する。
 *    どの方法を取るにせよ com_sigInf_t型のデータが必要になる。
 *    各I/Fの詳細については、本ヘッダーファイルの個々の説明を確認すること。
 *
 *    Wiresharkや tcpdumpを使って収集したバイナリのキャプチャデータの場合：
 *    ・com_readCapFile()を使って信号データを生成できる。
 *      oCapInf->signal が必要な信号データになる。
 *
 *    tsharkや tcpdumpで -X指定時のキャプチャテキスト出力の場合
 *    ・com_readCapLog()を使って信号データを生成できる。
 *      oCapInf->signal が必要な信号データになる。
 *    ・com_seekCapLog()を使う方法も検討できるが、これを使う方法は
 *      このI/Fの説明記述を参照されたい。
 *
 *    16進数のバイナリ数字列の場合
 *    ・com_setSigInf()や com_addSigInf()を使って信号データを生成できる。
 *      oTargetが そのまま必要な信号データとなる。
 *
 *    16進数文字列のテキストデータの場合
 *    ・com_strtooct()を使えば、バイナリ数字列に変換できる。
 *      これに com_setSigInf()や com_addSigInf()を使えば良い。
 *
 *
 *  (2)解析I/Fにかける
 *
 *    何のプロトコルから始まるのか分かっているのであれば、
 *    com_signalPrt?.h に記述された そのプロトコルの解析I/Fを直接使っても良い。
 *
 *    入力となる com_sigInf_t型のデータのうち解析の入力となるのは以下。
 *      .sig :   信号のバイナリデータ
 *               .topが先頭アドレス  .lenがデータ長  .ptypeがプロトコル種別
 *      .order : ネットワークバイトオーダーかどうかの BOOL型数値
 *               Linuxの場合は true となる理解で良い。
 *
 *    (1)で示したI/Fを使ってデータを取得した場合 .sig.ptype 以外は設定される。
 *    .sig.ptype については解析I/Fを呼ぶ前に自分で設定する必要がある。
 *    com_signalPrt?.h の PROTOCOL_AVAILABLE で宣言された値を想定する。
 *
 *    プロトコル解析I/Fを通し、正しく処理されると(＝trueを返した時)
 *    com_sigInf_t型データは解析結果を反映して変更される。
 *    主な変更ポイントは以下となる。ただしプロトコルによって設定しないものも
 *    あるので、解析I/Fの個別説明を必ず確認すること。
 *      .sig.len :   解析できたプロトコル部分のレングス
 *      .sig.ptype : 解析できたプロトコル種別
 *      .multi :     プロトコル個別の信号データ(複数信号が入っていた時など)
 *      .prm :       解析されたパラメータのリスト
 *      .ext :       拡張で更に解析されたデータ
 *      .next :      次スタックの情報
 *
 *    .next.cnt は 続く次プロトコルのデータ個数になり、
 *    .next.stack[] が実際の次プロトコルデータとなる。これも com_sigInf_t型で
 *    対応するプロトコルの解析I/Fを使うことで、信号解析を繰り返す手順となる。
 *
 *    プロトコルによっては複数のデータが続くこともあるので注意すること。
 *    0だった場合、それ以上 続くプロトコルはなく、解析終了を意味する。
 *
 *    次プロトコル種別は .next.stack[].sig.ptype を確認する。
 *      COM_SIG_END だった場合：
 *        最後まで解析が完了したことを示す。
 *      COM_SIG_CONTINUE だった場合：
 *        次プロトコルが解析できなかったことを示す。
 *        使う側で何のプロトコルか分かっているなら、
 *        そのプロトコルの値に変更することで、解析を継続できる可能性がある。
 *
 *    また、そもそもの問題として、解析I/Fの返り値が falseだった場合は、
 *    何らかの理由で解析に失敗しているので、そこで解析中断しなければならない。
 * 
 *
 *  (3)プロトコルの判定を自動で行わせる解析I/Fもある
 *  
 *    順を追って説明するが、一番最後の com_analyzeSignalToLast()を使うのが
 *    一番簡単な解析処理の進め方になる。
 * 
 *    プロトコル固有の解析I/F (com_signalPrt?.hに記述) を直接呼ぶことで
 *    そのプロトコルとしての解析が可能。これが一番原始的な方法。
 *    次プロトコルがあれば、そのプロトコルの解析I/Fを呼ぶ・・を繰り返す。
 *
 *    com_searchSigAnalyzer() を使えば、プロトコル種別をキーにして、
 *    その解析I/Fを取得できるので、それをそのまま使えば良い。
 *    一度解析をして .next.stack[].sig.ptype にプロトコル種別が入っていれば
 *    その値をキーにして、次の解析I/F取得が可能ということになる。
 *
 *    最初のプロトコルがリンク層(Ether2か SLL)だと分かっているのであれば、
 *    com_analyzeAsLinkLayer() を使うと どちらか適切な方を判別して解析可能。
 *    最初のプロトコルがネットワーク層(IP等)だと分かっているのであれば、
 *    com_analyzeAsNetworkLayer()を使うと、適切なプロトコルを判別して解析可能。
 *
 *    com_analyzeSignal() を使えば、入力となる com_sigInf_t型データから
 *    .sig.ptypeを参照してプロトコル解析I/Fを取得し、解析してくれる。
 *    最初からプロトコルが分かっているなら .sig.ptype にそのプロトコルの値を
 *    設定して このI/Fを使うことにより、解析がスムーズに進められる。
 *    もし プロトコル種別が 0 (COM_SIG_UNKNOWN)だったら、プロトコル自動判定も
 *    試みる。ただ、com_analyzeAsLinkLayer()と com_analyzeAsNetworkLayer()を
 *    使った判別なのでこの2つで判別できないプロトコルの場合、解析は出来ない。
 *
 *    com_analyzeSignalToLast() を使えば、com_analyzeSignal() による
 *    自動プロトコル判定をしつつ .next がある限り 次々とプロトコル解析を
 *    進めてくれる。基本的にこのI/Fを使うのが一番簡単な解析処理となる。
 *
 *
 *  (4)簡易デコード
 *
 *    実際に生成された解析結果をどのように使うかは使う側に委ねられる。
 *    内容精査のサンプルとして、解析結果から信号内容を簡易出力する機能はある。
 *
 *    com_decodeSignal() は 解析を終えた com_sigInf_t型データを入力とし、
 *    適切なプロトコルのデコードI/Fを呼んで出力する。
 *    プロトコルスタックが続く場合 .next.cnt を参照して、その数だけ
 *    com_decodeSignal()に .next.stack[] をかけていく・・という繰り返しになる。
 *    (一部をデコード不要と判断してスキップする選択肢もあるだろう)
 *
 *    もちろん com_signalPrt?.h に記述された各プロトコルごとのデコードI/Fを
 *    直接呼んでも問題ない(もちろん正しいプロトコルのデータであれば、だが)。
 *    com_searchSigDecoder()で プロトコル種別(.sig.ptypeに格納)をキーにして
 *    デコードI/Fを検索可能。
 */



// 基本データ宣言 ///////////////////////////////////////////////////////////

// 信号を扱うためのデータ型
typedef  uchar    com_bin;   // バイナリ数値データ型
typedef  size_t   com_off;   // オフセット位置/サイズ用データ型

// ビットごとのサイズ
enum {
    COM_8BIT_SIZE  = 1,    //  8bitのオクテットサイズ
    COM_16BIT_SIZE = 2,    // 16bitのオクテットサイズ
    COM_32BIT_SIZE = 4,    // 32bitのオクテットサイズ
};

// 変数を特定の型でキャストして、新たな変数として定義iするマクロ
#define COM_CAST_HEAD( TYPE, HEAD, TARGET ) \
    TYPE*  HEAD = (TYPE*)(TARGET)

// PROTOCOL_AVAILABLE
//   ファイルタイプに属する2つは、ファイルを読み込んで解析して導き出された、
//   キャプチャデータの形式を示す。
//   それ以外のものは、個別のプロトコルを示す、というよりは解析処理の中で
//   共通的に使用する値となる。
//
//   解析が可能なプロトコルの値については com_signalPrt?.h に宣言する。
enum {
    /***** ファイルタイプ (1-99) *****/
    COM_SIG_PCAPNG     = 1,    // pcap new generation (wiresharkの主な形式)
    COM_SIG_LIBPCAP    = 2,    // 旧来の pcap (tcpdumpがこの形式)
    /***** Link層 (101-199) *****/
    COM_SIG_LINK_LAYER = 100,       // リンク層プロトコルを示す識別値
    // com_signalPrt1.h: 101-102
    // com_signalPrt3.h: 111
    /***** Network層 (201-299) *****/
    COM_SIG_NETWORK_LAYER = 200,    // ネットワーク層プロトコルを示す識別値
    // com_signalPrt1.h: 201-205
    // com_signalPrt3.h: 211
    /***** Transport層 (301-399) *****/
    COM_SIG_TRANSPORT_LAYER = 300,  // トランスポート層プロトコルを示す識別値
    // com_signalPrt1.h: 301-303
    /***** No.7(SIGTRAN) (401-499) *****/
    COM_SIG_NO7_LAYER = 400,        // No.7系のプロトコルを示す識別値
    // com_signalPrt2.h: 401-406
    /***** Application層 (501-   ) *****/
    COM_SIG_APP_LAYER = 500,        // アプリケーション層プロトコルを示す識別値
    // com_signalPrt1.h: 501-508
    // com_signalPrt2.h: 511-514
    /***** プロトコル認識のみ (601-    ) *****/
    // com_signalPrt3.h: 611-614
    /***** 内部処理用定数 *****/
    COM_SIG_TYPEMAX    = 999,    // プロトコル種別の最大値
    COM_SIG_UNKNOWN    = 0,      // 処理ができないプロトコル
    COM_SIG_END        = -1,     // 一覧の最後
    COM_SIG_CONTINUE   = -2,     // 解析が継続可能なプロトコル
    COM_SIG_FRAGMENT   = -3,     // フラグメントデータ
    COM_SIG_ERROR      = -4,     // 信号処理エラー
    COM_SIG_EXTENSION  = -5,     // 拡張データ/ヘッダ
    COM_SIG_ALLZERO    = -6      // オールゼロデータ
};

/*
 * レイヤー種別取得  com_getLayerType()
 *   プロトコルのレイヤー値(上記で宣言された COM_SIG_～_LAYER)を返す。
 *   0～999の範囲外だったら COM_SIG_UNKNOWN を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * COM_SIG_～で宣言されたプロトコル値を iTypeに指定することで、
 * それがどのレイヤーのプロトコルなのかを返す。
 */
long com_getLayerType( long iType );


// 解析用の信号情報データ構造一式 ////////////////////////////////////////////
/*
 * com_sigInf_t型が解析に使用する情報となる。
 * 信号を解析する大まかな流れは以下のようになる。
 *
 * (1)com_sigInf_t型の実体データを定義。
 * (2)com_initSigInf()で初期化。
 * (3)com_setSigInf()や com_addSigInf()を使って信号バイナリデータを設定。
 *    信号バイナリデータは .sig に格納する。
 * (4)解析I/Fに渡して、その結果を使用 あるいは更に解析。
 * (5)解析終了したら com_freeSigInf()でメモリ解放。
 * 
 * 解析対象がキャプチャデータのバイナリやテキストの場合は、
 * そこから信号データを取得する必要がある。この取得処理をするにあたっては
 * com_capInf_t型データを使用する。この中にも com_sigInf_t型の .signal 
 * というメンバがあり、ここに取得した信号データが入るので、これを使用する。
 *
 * 解析I/Fでは .sig.top と .sig.len を信号データの大本として使用し、
 * それ以降のデータでは、このデータに対するポインタで相対的に保持する。
 * 従って、解析処理が終了するまで .sig.top と .sig.len は変更しないように
 * することを大前提とする。
 *
 * ちなみに .sig.ptype は COM_SIG_～ で宣言されたプロトコル種別値が入ることを
 * 期待している。この宣言は複数ヘッダで行われるため、特定の列挙体型にはせず
 * 単純な long型で定義している。
 */

// 信号バイナリデータ構造
typedef struct {
    com_bin*            top;         // バイナリ先頭アドレス
    com_off             len;         // バイナリ長
    long                ptype;       // バイナリのプロトコル種別
} com_sigBin_t;

// 信号フラグメント セグメント構造
typedef struct {
    long                seg;         // セグメント値
    com_sigBin_t        bin;         // 実際のセグメント情報
} com_sigSeg_t;

// 信号フラグメントデータ構造
typedef struct {
    long                segMax;      // フラグメント数
    void*               ext;         // 追加情報
    long                cnt;         // 収集済みセグメント数
    com_sigSeg_t*       inf;         // 収集したセグメント情報
} com_sigFrg_t;

// 信号パラメータデータ構造
typedef struct {
    com_sigBin_t        tagBin;      // 複数オクテットのタグに備える
    com_off             tag;         // 実際のタグ値
    com_sigBin_t        lenBin;      // 複数オクテットのレングスに備える
    com_off             len;         // 実際に計算されたレングス値
    com_bin*            value;       // レングス分の実際のバリュー
} com_sigTlv_t;

// 信号パラメータリストデータ構造
typedef struct {
    long                cnt;         // 解析分解したパラメータ数
    com_sigTlv_t*       list;        // 解析分解したパラメータ
    void*               spec;        // プロトコル固有情報
} com_sigPrm_t;

// 信号スタックデータ構造
struct com_sig_t;    // 実体定義はこの後(com_sigStk_t内で使うため仮定義
typedef struct {
    long                cnt;         // 信号個数
    struct com_sig_t*   stack;       // 信号スタック情報
} com_sigStk_t;

// 信号情報データ構造 (com_sigInf_t = struct com_sig_t)
typedef struct com_sig_t {
    com_sigBin_t        sig;         // 信号データ
    BOOL                isFragment;  // フラグメント断片かどうか
    com_sigBin_t        ras;         // フラグメント結合後の信号データ
    BOOL                order;       // バイトオーダー
    com_sigStk_t        multi;       // 複数スタックを分解した信号データ
    com_sigPrm_t        prm;         // ヘッダパラメータ
    void*               ext;         // 拡張情報
    com_sigStk_t        next;        // 次スタック情報
    struct com_sig_t*   prev;        // 前スタック情報
} com_sigInf_t;

/*
 * 信号情報の初期化/解放  com_initSigInf()/com_makeSigInf()/com_freeSigInf()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [COM_PRMNG] !oTarget
 *   com_freeSigInf(): メモリ解放の com_free()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * com_initSigInf()は oTargetで指定された信号情報を初期化する。
 * 母体となる信号情報がある場合は iOrgに指定する。
 * 指定がされていれば
 *   oTarget->order : iOrg->order の値
 *   pTarget->prev  : iOrgの値
 * となる。母体がない場合、iOrg はNULL設定でよく、その場合は
 *   oTarget->order : true固定
 *   oTarget->prev  : NULL
 * という格納になる。
 * この初期化は com_sigInf_t型データを使うときに必ず実施が必要。
 *
 * com_makeSigInf()は com_initSigInf( oTarget, NULL )を実行後、
 * oTarget->sig を以下のように設定する。
 *   oTarget->sig.top   : iData
 *   oTarget->sig.len   : iSize
 *   oTarget->sig.ptype : COM_SIG_UNKNOWN (0)
 *
 * com_freeSigInf()は oTarget内をメモリ解放する(oTarget自体の解放はしない)。
 * iBinが trueであれば oTarget->sig.top を com_free()でメモリ解放する。
 * これは信号データの格納に動的メモリ確保をしていない場合に配慮している。
 */
void com_initSigInf( com_sigInf_t *oTarget, com_sigInf_t *iOrg );
void com_makeSigInf( com_sigInf_t *oTarget, void *iData, com_off iSize );
void com_freeSigInf( com_sigInf_t *oTarget, BOOL iBin );

/*
 * 信号データセット  com_setSigInf()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   com_err_debugng: [com_prmng] !otarget || !isignal
 *   信号データ生成のための com_malloc()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSignalと iSizeで与えられた信号バイナリデータを oTarget->sigに設定する。
 * iOrder は ネットワークバイトオーダーかどうかを指定し、通常は true にする。
 * 
 * oTargetには以下のような格納が行われる：
 *   .sig.top : com_copySigBin()で iSignalの内容を複製コピー。
 *   .bin.len : iSizeの値を設定
 *   .order   : iOrderの値を設定
 *   .next    : oStaticNext指定時は com_setStaticStk()で静的領域設定
 *
 * oStaticNextは次プロトコルデータ格納先を指定する。
 * oTargetを使ったプロトコル解析後、次プロトコルスタックを .next.stackに
 * 格納する時、メモリの動的確保をせず、指定したバッファに格納する。
 * この静的領域指定は com_setStaticStk() で行われる処理で、
 * 注意事項があるので、そちらの動作説明記述も必ず参照すること。
 * 静的領域を使うつもりが無い場合 oStaticNext は NULLを指定すること。
 *
 * 信号データ格納に際し、メモリを動的確保することから、oTargetの解放時は
 * com_freeSigInf()で iBin を trueにする必要がある。
 */
BOOL com_setSigInf(
        com_sigInf_t *oTarget, com_sigInf_t *oStaticNext,
        const uchar *iSignal, size_t iSize, BOOL iOrder );

/*
 * 信号データ追加  com_addSigInf()
 *   処理の成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [COM_PRMNG] !oTarget || !iSignal
 *   信号データ追加のための com_realloc()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSignalと iSizeで与えられた信号バイナリデータを oTarget->sigに追加設定する。
 * 既に設定されているデータの後に追加する。
 *
 * そのために com_realloc()で保持領域のサイズを拡張する都合から、
 * 信号保持領域は動的確保したものでなければならない。
 * com_setSigInf()であれば、この条件に合致する。
 */
BOOL com_addSigInf( com_sigBin_t *oTarget, const uchar *iSignal, size_t iSize );

/*
 * 信号バイナリ複製  com_copySigBni()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   com_err_debugng: [com_prmng] !otarget || !iSource
 *   信号バイナリ複製のための com_malloc()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSourceの内容を oTargetにコピーする。
 *
 * oTarget->top が NULLの場合、
 * iSource->len分のメモリを確保し、そのアドレスを設定する。
 * そして iSource->topの内容をそこにコピーする。
 *
 * oTarget->top が 非NULLの場合、メモリ確保は行わず
 * oTarget->len のサイズまで、iSignal->top から内容をコピーする。
 * oTargetに元から入っていたデータは当然失われる。
 */
BOOL com_copySigBin( com_sigBin_t *oTarget, const com_sigBin_t *iSource );

/*
 * 静的領域のスタック指定  com_setStaticStk()
 * ---------------------------------------------------------------------------
 *   com_err_debugng: [com_prmng] !otarget || !iSource
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * com_sigInf_t型データのメンバー .next に対して使用されるI/F。
 *
 * プロトコル解析後、さらに上位のプロトコルスタックがある場合、
 * その内容を .next に設定する。com_initSigInf()を介して com_initSigStk()で
 * 初期化されていた場合、.next は0クリアされており、その状態の場合、
 * .next.stack[]は動的にメモリ確保され、そのスタック数を .next.cntに格納する。
 *
 * 本I/Fは .nextのために静的バッファを設定する。その場合
 * .next.cnt は COM_SIG_STATIC (-1) という特別な値に代わり、
 * .next.stack に指定された iSourceのアドレスが設定される。
 *
 * この状態になった場合、次プロトコルスタックの情報を格納するときは
 * 動的メモリ確保をせず、設定されたアドレスのバッファに内容を格納する。
 * .next.cntは変更せず COM_SIG_STATIC のままになる。
 * つまり、スタック数を表さなくなるため、この静的領域を設定した側が
 * バッファにどんな内容を書き込むのか把握して、その後の処理を進めること。
 */

// 静的領域設定を示す指標
enum { COM_SIG_STATIC = -1 };

void com_setStaticStk( com_sigStk_t *oTarget, com_sigInf_t *iSource );

/*
 * 信号情報従属データの初期化/解放
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [COM_PRMNG] !oTarget
 *   com_freeSig～(): メモリ解放の com_free()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * com_sigInf_t型のデータが保持する従属データ型の初期化/解放を行う。
 * 個別に使うケースは少ないかもしれないが、部品I/Fとして外部公開する。
 *
 * com_freeSigStk()の iBin を true に設定したときは、
 * oTarget->stack[].sig.top に対して com_free() を実施する。
 *
 * com_freeSigInfExt()は oTarget->ext のメモリ解放を行うが、
 * その解放方法はプロトコルごとに定められているため、oTarget->sig,ptype を
 * 参照し、プロトコルごとに定められた解放関数を呼ぶ処理となる。
 * (解放関数は com_searchSigFreer()で取得している)
 * 解放関数がない(NULL)場合は、単純に com_free()で解放するのみとなる。
 *
 * 上記以外のI/Fは oTarget を対象とした単純な処理となるので、詳細な説明は省く。
 */
void com_initSigBin( com_sigBin_t *oTarget );
void com_freeSigBin( com_sigBin_t *oTarget );

void com_initSigFrg( com_sigFrg_t *oTarget );
void com_freeSigFrg( com_sigFrg_t *oTarget );

void com_initSigPrm( com_sigPrm_t *oTarget );
void com_freeSigPrm( com_sigPrm_t *oTarget );

void com_initSigStk( com_sigStk_t *oTarget );
void com_freeSigStk( com_sigStk_t *oTarget, BOOL iBin );

void com_freeSigInfExt( com_sigInf_t *oTarget );



/*
 *****************************************************************************
 * COMSIGTYPE: プロトコル種別判定情報I/F
 *****************************************************************************
 */

/*
 * プロトコル種別判定情報登録  com_setPrtclType()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   COM_ERR_DEBUGNG: [COM_PRMNG] !iList
 *   com_realloct()によるエラー (その場合プログラム強制終了)
 * ===========================================================================
 * 特定の数値(ポート番号等)または文字列を、プロトコル種別と関連付けて登録する。
 * プロトコル種別以外の数値と関連付けることもある(使う側で意識すればよい)。
 *
 * iBaseで指定した判定種別データに iListの内容を追加登録する。
 * iListは com_sigPrtclType_t型の線形データになる。C言語の言語仕様から
 * .targetは type(数値)で初期設定するときは {100} という書き方で構わないが、
 * label(文字列)で初期化したいときは {.label="SAMPLE"} と書く必要がある。
 *  (バイナリベース/テキストベース、どちらのプロトコルでも使えるように考慮)
 * .codeは .targetが示すプロトコル種別などの数値を設定する。
 *
 * iListの 最終データは
 *   { {COM_PRTCLTYPE_END}, デフォルト値 }
 * という形式に必ずすること。
 * .target.type が COM_PRTCLTYPE_END であることで最終データと判定する。
 * また その時の .type.code は その iBaseの判定種別でのデフォルト値を示す
 * ものになるので、内容はよく検討すること。
 *
 * このデフォルト値して扱われるのは 最初の COM_PRTCLTYPE_END検出時のみ。
 * 同じ iBase に対して複数箇所から複数リストの登録を受け付けることが可能だが
 * 2回目以降の登録時の COM_PRTCLTYPE_END時の .target.code は Don'tCare。
 *
 *
 * 登録例は com_signalPrt1.cの gLinkNex1[]・gSctpNext1[]・gSipNext[] になる。
 * 特に gSipNext[]は .target.label での初期化例となる。
 * com_signalPrt2.cでは gSctpNext2[]・gSipNext2[] の登録があり、
 * これは com_signalPrt1.cで登録したものへの追加となる。
 *
 * 新たなプロトコル解析に対応しても追加が用意になるように、本I/Fは作られた。
 * 今後もこうした判定が必要なデータが新たに増えるときは、COM_PRTCLTYPE_tに
 * 新たな数値を追加し、com_setPrtclType()で登録できるように対応する。
 */

// プロトコル種別判定ベース
typedef enum {
    COM_NOT_USE      = 0,
    /*** com_signal.h用 ***/
    COM_FILENEXT     = 1,     // ファイルヘッダ解析 次プロトコル値
    /*** com_signalPrt1.h用 ***/
    COM_LINKNEXT     = 100,   // リンク層ヘッダ解析 次プロトコル値
    COM_VLANTAG,              // VLANタグ値
    COM_IPNEXT,               // IPヘッダ解析 次プロトコル値
    COM_IPOPTLEN,             // IPv4解析 オプションレングス
    COM_IP6XHDR,              // IPv6解析 拡張ヘッダ判定
    COM_IPPORT,               // トランスポート層 プロトコル対応ポート番号
    COM_TCPOPTLEN,            // TCP解析 オプションレングス
    COM_SCTPNEXT,             // SCTP DATAヘッダ解析 次プロトコル値
    COM_SIPNEXT,              // SIPヘッダ解析 次プロトコル値
    COM_DIAMAVPG,             // Diameter解析 Grouped属性AVP判定
    /*** com_signalPrt2.h用 ***/
    COM_SCCPSSN      = 200,   // SCCP解析 サブシステム番号プロトコル対応
    COM_TCAPSSN,              // TCAP解析 サブシステム番号プロトコル対応
    /*** 共通のデータ終了識別子 ***/
    COM_PRTCLTYPE_END = -1
} COM_PRTCLTYPE_t;

// プロトコル種別判定データ構造
typedef struct {
    union {
        long   type;    // 次プロトコルを示す数値データ
        char*  label;   // 次プロトコルを示す文字データ
    } target;
    long   code;        // targetに対応するプロトコル識別値(COM_SIG_～)
} com_sigPrtclType_t;

void com_setPrtclType( COM_PRTCLTYPE_t iBase, com_sigPrtclType_t *iList );

/*
 * プロトコル種別判定取得  com_getPrtclType()・com_getPrtclLabel()
 *   対応する種別値を返す。
 *   該当する登録がなかった場合は デフォルト値を返す。
 *    (デフォルト値の設定方法は com_setPrtclType()を参照)
 *   iBaseが COM_PRTCLTYPE_tにない値の場合、エラーとなり
 *   無条件で COM_SIG_UNKNOWN (0) を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [COM_PRMNG] !oTarget
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iBaseで指定した種別で登録した情報を取得する。
 * 
 * com_getPrtclType()は iType が com_setPrtclType()で登録した .target.typwと
 * 一致したら、その時の .code を返す。
 *
 * com_getPrtclLabel()は iLabel が com_setPrtclType()で登録した .target.labelと
 * 一致したら、その時の .code を返す。
 */
long com_getPrtclType( COM_PRTCLTYPE_t iBase, long iType );
long com_getPrtclLabel( COM_PRTCLTYPE_t iBase, char *iLabel );

/*
 * BOOL型判定データ登録  com_setBoolTable()
 * ---------------------------------------------------------------------------
 *   入力データ生成時の com_malloc()でエラーになった場合プログラム強制終了。
 *   com_setPrtclType()でエラーになった場合も、プログラム強制終了。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iTypes は long型数値のリストで、iTypeCnt はその個数を示す。
 * リストで指定された数値をそれぞれ .target.type とし .target.code を true 
 * とする com_sigPrtclType_t型の線形リストを生成し、デフォルト値は falseで
 * com_setPrtclType()を使って iBaseで指定した判定情報データとして登録する。
 *
 * com_getPrtclType()で数値を iTypeに指定することにより、
 * それが true か false かを判定することが可能になる。
 * (登録されていない数値なら false が返る)
 */
void com_setBoolTable( COM_PRTCLTYPE_t iBase, long *iTypes, long iTypeCnt );

/*
 * プロトコル種別判定情報解放  com_freePrtclType()
 * ---------------------------------------------------------------------------
 *   com_free()によるエラー。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * com_setPrtclType()で登録した全データをメモリ解放する。
 * これはプログラム終了時専用の処理であり、それ以外での用途は無い。
 */
void com_freePrtclType( void );



/*
 *****************************************************************************
 * COMSIGANLZ: プロトコル解析情報I/F
 *****************************************************************************
 */

// プロトコルごとに少なくとも 解析I/F と デコードI/F は作成する。
// (toscomで作ったものは com_signalPrt?.c に定義している)
// これらの I/Fは仮引数を統一する目的で、マクロ化しており、
// com_analyzeSig_t や com_decodeSIg_t に示したようなプロトタイプとなる。
// 
// また解析した結果 拡張情報を編集するプロトコルは、
// com_sigInf_t型のメモリ解放を実施するときに .ext の解放も必要になる。
// 単純に com_free()で解放すればよいなら、特に検討は必要ないが、
// ここに構造体のデータを格納し、単純な com_free()だけでは解放しきれない場合
// その拡張情報のメモリ解放を行う関数を用意する必要がある。
// これを解放関数と呼んでおり、com_freeSig_t でプロトタイプを示している。
// (toscomでもそこまでしているプロトコルはかなり数が限られてはいる)
//
// 解析I/F・デコードI/F・解放I/Fの実際の処理の書き方は、
// また幾つかパターンがあるが、それについては com_signalCom.h にて記述する。
// プロトタイプなどの宣言が以下に記述されているのは、プロトコル解析情報に
// これらの関数アドレスが含まれており、そのデータ型定義が必要なためである。


// プロトコル解析I/F 仮引数マクロ
#define COM_ANALYZER_PRM  \
    com_sigInf_t *ioHead, BOOL iDecode

// プロトコル解析I/F 実引数マクロ
#define COM_ANALYZER_VAR  \
    ioHead, iDecode

// プロトコル解析I/Fプロトタイプ
typedef BOOL(*com_analyzeSig_t)( COM_ANALYZER_PRM );

// プロトコルデコードI/F 仮引数マクロ
#define COM_DECODER_PRM \
    com_sigInf_t *iHead

// プロトコルデコードI/Fプロトタイプ
typedef void(*com_decodeSig_t)( COM_DECODER_PRM );

// プロトコル拡張情報解放I/Fプロトタイプ
typedef void(*com_freeSig_t)( com_sigInf_t *oTarget );

/*
 * プロトコル解析情報登録  com_registerAnalyzer() 
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [COM_PRMNG] !iFunc
 *   com_addSortTableByKey()によるエラー (ただしその場合プログラム強制終了)
 * ===========================================================================
 *   マルチスレッドで動作することを考慮済み。
 * ===========================================================================
 * iList で指定したプロトコル情報を登録する。
 * 各プロトコルのポート番号やプロトコル文字名や解析I/F・デコードI/F・
 * 拡張情報解放I/F(これは必要な場合のみ)がデータとして含まれる。
 *
 * iListは com_analyzeFuncList_t型の線形リストで、その最終データは
 * .typeが COM_SIG_END となっていなければならない。
 * (最終データのそれ以外のメンバーの値は Don't Care)
 *
 * 登録する内容のうち .port だけは少し特殊な任意情報となる。
 * この値が 0 以外になっている場合、com_setPrtclType()を内部で使用し、
 * iNumSys の種別に対して .port の値を .type のプロトコルとして登録する。
 * 本I/Fで com_setPrtclType()を使って登録できるのは、各プロトコルごとに
 * .oprtで指定した一つの値だけなので、同一プロトコルに複数を登録したい場合、
 * 別途 com_setPrtclType()で登録する必要がある。
 *
 * 本I/Fの具体的な使用例は com_initializeSigPrt1() を参照。
 * gFuncSignal1[] の情報を登録しており、iNamSys は COM_IPPORT を指定することで
 * そのプロトコルを示すポート番号を併せて登録するようにしている。
 * ただし、全てのプロトコルでポート番号を持つわけではないので、
 * ポート番号を持たないものは .port を 0 で定義しているのが分かるだろう。
 */

// プロトコル解析用情報データ構造
typedef struct {
    long               type;        // プロトコル種別(COM_SIG_～)
    long               port;        // 該当ポート番号
    char*              label;       // プロトコル名文字列
    com_analyzeSig_t   func;        // 解析I/F
    com_decodeSig_t    decoFunc;    // デコードI/F
    com_freeSig_t      freeFunc;    // 解放I/F
} com_analyzeFuncList_t;

BOOL com_registerAnalyzer(
        com_analyzeFuncList_t *iList, COM_PRTCLTYPE_t iNumSys );

/*
 * プロトコル解析情報取得
 *   指定されたプロトコル種別に対応するデータを返す。
 *   登録されていないプロトコル種別だった場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   データ検索時の com_searchSortTableByKey()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iTypeでプロトコル種別(COM_SIG_～)を指定することにより、
 * com_registerAnalyzer()で登録した情報を取得する。
 * 指定されたプロトコルの情報が登録されていない場合は NULLを返す。
 *
 * com_searchSigProtocol()   プロトコル名文字列
 * com_searchSigAnalyzer()   解析I/F
 * com_searchSigDecoder()    デコードI/F
 * com_searchSigFreer()      拡張情報解放I/F
 */
char *com_searchSigProtocol( long iType );
com_analyzeSig_t com_searchSigAnalyzer( long iType );
com_decodeSig_t com_searchSigDecoder( long iType );
com_freeSig_t com_searchSigFreer( long iType );

/*
 * プロトコル名文字列から種別取得  com_searchPrtclByLabel()
 *   対応するプロトコル種別を返す。
 *   登録がない場合は、COM_SIG_UNKNOWN (0) を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [COM_PRMNG] !iLabel
 *   データ検索時の com_searchSortTableByKey()によるエラー
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iLabelで指定した文字列を登録しているプロトコル種別値(COM_SIG_～)を返す。
 * 登録がない場合は COM_SIG_UNKNOWN を返す。
 */
long com_searchPrtclByLabel( const char *iLabel );

/*
 * 使用可能なプロトコル取得  com_showAvailProtocols()
 *   使用可能なプロトコル数を返す。
 *   oListでアドレスを指定した場合、使用後に com_free()で解放すること。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * com_registerAnalyzer()で登録したプロトコル数を返す。
 * oListが 非NULLの場合 *oListにスペースで区切った全プロトコル名の文字列を
 * 格納する。このエリアは動的に確保するので、呼び元で解放する必要がある。
 *
 * oListを NULLにした場合、全プロトコル名の文字列を画面出力する。
 */
long com_showAvailProtocols( long **oList );



/*
 *****************************************************************************
 * COMSIGREAD: 信号読込I/F
 *****************************************************************************
 */

// tcpdumpや Wiresharkでキャプチャしたデータを入力としたいときに、
// そのパケットデータを取得するために、以後に示すI/Fを使用する。
//
// その流れは以下のようになる。
//
// (1)com_capInf_t型の実体データ定義。
// (2)com_initCapInf()で (1)のデータを初期化。
// (3)取得I/Fによる信号データ抽出。
//     com_readCapFile()    バイナリのファイルを読み込む
//     com_readCapLog()     テキストのログを読み込む
//     com_seekCapLog()     com_seekFile()を使って テキストのログを読み込む
// (4)取得した内容は (1)のデータの .signal に格納されるので。
//    それをそのまま解析I/Fにかけることが可能。
// (5)続けて信号データを読み込みたいときは (3)(4) を繰り返す。
// (6)全て読み込んで処理をしたら com_freeCapInf()で (1)のデータを解放。
//
// com_readCapLog()と com_seekCapLog()のどちらを使うのかは、
// 純粋に使う側の選択であり、得られる情報に差分はない。


// 信号キャプチャ取得用データ構造
typedef struct {
    long            cause;       // 処理結果
    char*           fileName;    // 読込中ファイル名
    FILE*           fp;          // 読込中ファイルポインタ
    com_sigInf_t    head;        // ファイルヘッダ
    com_sigStk_t    ifs;         // I/F情報
    com_sigInf_t    signal;      // 信号全体
    BOOL            hasRas;      // Reassembledデータ読込有無
} com_capInf_t;

// 処理結果(NG要因)  (com_capInf_tの .causeに設定)
typedef enum {
    COM_CAPERR_NOERROR = 0,   // エラー無し
    COM_CAPERR_NOMOREDATA,    // ファイル走査は既に終了している
    COM_CAPERR_COPYNAME,      // ファイル名コピー失敗
    COM_CAPERR_OPENFILE,      // キャプチャファイルのオープンに失敗
    COM_CAPERR_GETHEAD,       // ファイルヘッダ解析NG
    COM_CAPERR_GETLINK,       // リンクタイプ解析NG
    COM_CAPERR_GETSIGNAL      // 信号抽出NG
} COM_CAPERR_t;

/*
 * キャプチャ情報初期化/解放  com_initCapInf()/com_freeCapInf()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [COM_PRMNG] !oTarget
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 * com_initCapInf()で oTargetの内容を初期化する。
 * com_capInf_t型のデータは使用前にかならず初期化する想定となる。
 *
 * com_freeCapInf()で oTargetの内容をメモリ解放する。
 * oTarget自身は解放しないので、必要であれば呼び元で解放すること。
 * (老婆心ながら実体定義しているなら解放は不要)
 */
void com_initCapInf( com_capInf_t *oTarget );
void com_freeCapInf( com_capInf_t *oTarget );


// キャプチャデータ形式 (リンクタイプ)
//   具体的にパケットキャプチャのヘッダに乗る値。
//   toscom内部ではこれを以下のプロトコル種別に正規化する。
//     COM_CAP_ETHER   ->  COM_SIG_ETHER2
//     COM_CAP_SLL     ->  COM_SIG_SLL
//   上記のプロトコル種別(COM_SIG_～)は、解析/デコードI/Fとともに
//   com_signalPrt1.h にて宣言している。
//   
//   libpcapがインストールされている場合、<pcap/bpf.h>で宣言された値が使えるが、
//   未インストールの環境を考慮して使用しない。
//   (実際に宣言されているのは下記のコメントで示す DLT_～ のマクロ名)

enum {
    COM_CAP_ETHER     = 0x01,    // DLT_EN10MB     DIX or IEEE803.3
    COM_CAP_SLL       = 0x71     // DLT_LINUX_SLL: Linux cooked capture
};


// キャプチャファイルの先頭4byteの値から、形式を特定する。
//   0a0d0d0a -> pcapng形式 (主にWireshark)
//   a1b2c3d4 -> libpcap形式 (主にtcpdump)
//
//   libpcap形式はリトルエンディアンの場合 d4c3b2a1 となる。
//   pcapng形式はエンディアンに関係なく同じ値となるように工夫した値となる。
//   (後発の形式なので、そういった考慮もされている。pcapng形式では
//    エンディアン(バイトオーダー)を確認するために SHB に Byte Order Magic
//    というパラメータ(メンバー名 .boMagic)が用意されている)


// pcapng形式のブロックヘッダ値
//   先頭は必ず SHB (Section Header Block) となり、
//   その Block Type値(0a0d0d0a)は pcapng形式の判定値でもある。
//
//   pcapng形式のブロック構造は以下で統一されている
//     Block Type          (32bit)
//     Block Total Length  (32bit)  Block Typeも含めた長さ
//    (Block Body)                  32bitアラインで、パディングは 0x00
//     Block Total Length  (32bit)
//
//   Block Total Length がブロックの最初と末尾に存在する理由は、
//   前のブロックに戻りやすくするため。
//
//   ブロックによってはオプションパラメータが可変個数設定されることがある。
//   メンバ名は options[]で その構造は以下となる。
//     Option Code   (16bit)
//     Option Length (16bit)
//     Option Value   32bitアラインで、パディングは 0x00
//   コメントでオプション情報を記述するときは
//     オプションコード(オプション名)レングス
//   というフォーマットとしている。レングスが可変の場合は v としている。
//   toscomではオプションの詳細内容は Don't Careとなる。

#define COM_CAP_FILE_SHB    0x0a0d0d0a    // PcapNg SHB Block Type
#define COM_CAP_FILE_IDB    0x00000001    // PcapNg IDB Block Type
#define COM_CAP_FILE_EPB    0x00000006    // PcapNg EPB Block Type
#define COM_CAP_FILE_SPB    0x00000003    // PcapNg SPB Block Type

// 以下のブロックはブロックとしては認識するが内容は Don't Careとなる
#define COM_CAP_FILE_NRB    0x00000004    // PcapNg NRB Block Type
#define COM_CAP_FILE_ISB    0x00000005    // PcapNg ISB Block Type
#define COM_CAP_FILE_CB1    0x00000BAD    // PcapNg Custom Block Type(1)
#define COM_CAP_FILE_CB2    0x40000BAD    // PcapNg Custom Block Type(2)

// Section Header Block (0x0a0d0d0a) 構造 (ファイルの最初にある想定)
typedef struct {
    uint32_t   blockType;
    uint32_t   blockLen;
    uint32_t   boMagic;     // 0x1a2b3c4d でエンディアン判定に使われる
    uint16_t   majVer;      // 現状 1 固定 (majVer+minVer で 1.0版)
    uint16_t   minVer;      // 現状 0 固定
    uint64_t   sectorLen;   // Wiresharkはセクション数1固定なので Don't Care
    uint8_t    options[];   // オプション情報については下記にコメント
} com_pcapngShb_t;
// オプション情報
//   2(shb_hardware)v  3(shb_os)v  4(shb_userappl)v

// Interface Description Block (0x00000001) 構造 (複数乗ることもあり得る)
typedef struct {
    uint32_t   blockType;
    uint32_t   blockLen;
    uint16_t   linkType;    // これがリンクヘッダのプロトコルを示す
    uint16_t   reserve;
    uint32_t   snapLen;     // 各パケットの最大キャプチャ長(0なら無制限)
    uint8_t    options[];   // オプション情報については下記にコメント
} com_pcapngIdb_t;
// オプション情報
//   2(if_name)v  3(if_description)v  4(if_IPv4addr)8  5(if_IPv6addr)17
//   6(if_MACaddr)6  7(if_EUIaddr)8  8(if_speed)8  9(if_tsresol)1  10(if_tzone)4
//   11(if_filter)v  12(if_os)v  13(if_fcslen)1  14(if_tsoffset)8

// Enhanced Packet Block (0x00000006) 構造 (実際のパケットデータ)
typedef struct {
    uint32_t   blockType;
    uint32_t   blockLen;
    uint32_t   ifID;        // これがどのInterfaceかを示す(IDBと対応)
    uint32_t   tStamp_h;
    uint32_t   tStamp_l;
    uint32_t   capPktLen;   // パケットデータ長
    uint32_t   orgPktLen;   // 元々のパケットデータ長
    uint8_t    pktData[];   // 実際のパケットデータ
      // 本当はこの後にオプションが着くが、この構造では含められない。
      // 必要なら pktData + capPktLen が options[] となるはず
      // 念の為オプション情報については下記にコメント
} com_pcapngEpb_t;
// オプション情報
//   2(epb_flags)4  3(epb_hash)v  4(epb_dropcount)8

// Simple Packet Block (0x00000003) 構造 (こちらもパケットデータ)
typedef struct {
    uint32_t   blockType;
    uint32_t   blockLen;
    uint32_t   orgPktLen;   // パケットデータ長
    uint8_t    pktData[];   // 実際のパケットデータ
} com_pcapngSpb_t;

// EPBと SPBの大きな違いは ifIDの有無。
// pcapng形式では EPBが新設され どのインターフェースのパケットなのかを
// 判別できるようになった。(結果 libpcap形式との完全な互換性は無くなった)
// pcapng形式->libpcap形式への変換が Wireshark/tsharkのツールで可能だが、
// その場合 このインターフェースの情報は IDBとともに失われる。


// libpcap形式の Magic Number
//   ファイルの先頭 4byteがこの値になっている場合、libpcap形式と判断する。
//   本来はデータのバイトオーダー(エンディアン)を確認するための値だったが、
//   pcapng形式はこの仕組みを利用しつつ新形式として判定できるようにした。
//
//   libpcap形式では最初にこの Magic Numberを含むヘッダ情報があり、
//   その後は パケットデータが タイムスタンプ＋パケット長＋パケットデータ
//   の形で続くものとなっている。
//   (pcapng形式ではヘッダもパケットデータも「ブロック」という単位として
//    再定義し、様々な追加情報を保持できるようにした。
//    また、それ以外のデータを示すブロックも幾つか新設された)

#define COM_CAP_FILE_PCAP     0xa1b2c3d4   // libpcap Magic Number
#define COM_CAP_FILE_PCAP_R   0xd4c3b2a1   // libpcap Magic Number

// struct pcap_file_header型の定義
typedef struct {
    uint32_t   magic;          // 0xa1b2c3d4 でエンディアン判定に使われる
    uint16_t   version_major;  // 現状 2 固定 (version_minorと併せて 2.4版)
    uint16_t   version_minor;  // 現状 4 固定
    uint32_t   thiszone;
    uint32_t   sigfigs;
    uint32_t   snaplen;
    uint32_t   linktype;       // これがリンクヘッダのプロトコルを示す
} com_pcapHead_t;

// struct pcap_pkthdr型の定義 (この後に実際のパケットデータが続く)
typedef struct {
    uint8_t    ts[8];     // タイムスタンプ
    uint32_t   capLen;    // パケットデータ長
    uint32_t   len;       // 元々のパケットデータ長
} com_pcapPkthdr_t;

/*
 * キャプチャのバイナリ読込  com_readCapFile()
 *   読込成否を true/false で返す。
 *   trueが返った場合、読み込んだ信号データが oCapInf->signal に格納されるので
 *   それを解析I/Fに掛けることが可能。
 *   falseが返った場合、そこで読込は中断すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [COM_PRMNG] !oCapInf
 *   ファイルオープンのための com_fopen()によるエラー
 *   メモリ捕捉のための com_strdup()・com_reallocf()・com_malloc()によるエラー
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 * tcpdumpや Wiresharkでパケットキャプチャしたバイナリファイルを読み込む。
 * iPathで指定されたキャプチャファイルを読み込み、パケットデータを走査する。
 * oCapInfには 予め com_initCapInf()で初期化されたデータのアドレスを渡す。
 *
 * パケットデータを一つ読み取れたら oCapInf->signal に格納して trueを返す。
 * oCapInf->signal をプロトコル解析I/Fに掛けることが可能となる。
 *
 * さらに続けてパケットデータを読み込みたいときは iPathは NULL指定し、
 * oCapInfは最初と同じものをそのまま指定し続ける。
 * oCapInf->signalの内容はその都度 解放/捕捉していくため、
 * 信号データとして保持したい場合は、呼び元で別バッファにコピーすること。
 *
 * 読み取れるパケットがない場合や、処理NGが発生した場合は false を返し、
 * NG要因を oCapInf->cause に設定する。
 * ただ、false が返ったら、そこで処理終了となることに変わりはない。
 * falseを返すときは、キャプチャファイルをクローズし、oCapInfも解放する。
 *
 * falseを返す前に処理を中断したいときは、com_freeCapInf()で oCapInfの解放を
 * 必ず実施すること。
 *
 * oCapInfに設定する内容について、以下に簡単に記述する。
 *
 *   .cause      falseを返すときにその要因を設定。
 *               読み込むデータが無くなったら falseを返す。
 *               (.cause=COM_CAPERR_NOMOREDATA)
 *               ただファイル末尾まで読み切った場合も、これには含まれる。
 *   .fileName   iPathの内容をコピー。
 *               メモリ確保に失敗したら falseを返す(.cause=COM_CAPERR_COPYNAME)
 *   .fp         iPathで指定したファイルのファイルポインタ
 *               ファイルオープンNG時は falseを返す(.cause=COM_CAPERR_OPENFILE)
 *   .head       ファイルヘッダ内容を格納。
 *                 .head.sig        ヘッダ内容を格納
 *                 .head.sig.ptype  ファイル形式(COM_SIG_PCAPNG>COM_SIG_LIBPCAP)
 *                 .head.order      ネットワークバイトオーダーかどうか設定
 *                                  (ヘッダの Magic Number から判断)
 *               ヘッダ情報取得NG時は falseを返す(.cause=COM_CAPERR_GETHEAD)
 *   .ifs[]      pcapng形式の場合、IDBの個数とその内容を格納する。
 *               libpcap形式の場合、個数は 1固定。内容はほぼ0固定。
 *               どちらの形式でも .ifs[].sig.ptype に次のリンクプロトコル値設定
 *               (COM_SIG_ETHER2/COM_SIG_SLL)
 *               リンクタイプ取得NG時は falseを返す(.cause=COM_CAPERR_GETLINK)
 *   .signal.sig 取得したパケット1つ分のデータを格納。(true返送時)
 *               .signal.sig.ptype には .ifs[].sig.ptype と同値を設定する。
 *               パケット取得NG時は falseを返す(.cause=COM_CAPERR_GETSIGNAL)
 *               iPathを NULL指定して継続読込した場合、一度内容を解放して
 *               新たにメモリ確保を実施する。
 *   .hasRas     falseを固定設定。
 */
BOOL com_readCapFile( const char *iPath, com_capInf_t *oCapInf );

/*
 * キャプチャのテキスト読込  com_raadCapLog()
 *   読込成否を true/false で返す。
 *   trueが返った場合、読み込んだ信号データが oCapInf->signal に格納されるので
 *   それを解析I/Fに掛けることが可能。
 *   falseが返った場合、そこで読込は中断すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [COM_PRMNG] !oCapInf
 *   ファイルオープンのための com_fopen()によるエラー
 *   メモリ捕捉のための com_strdup()・com_reallocf()・com_malloc()によるエラー
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 * tsharkや Wiresharkで -x または -X を指定し、パケット内容を出力させたテキスト
 * からパケットデータを読み込む。
 * iPathで指定されたキャプチャファイルを読み込み、パケットデータを走査する。
 * oCapInfには 予め com_initCapInf()で初期化されたデータのアドレスを渡す。
 *
 * またネットワークバイトオーダーかどうか iOrderで指定する(Linux なら true)。
 * バイナリデータと違い、これを判断する材料が無いため、この指定が必要になる。
 * 
 * それ以外の使い方は com_readCapFile() と同じなので、その説明を参照。
 *
 * 処理的に大きな差分となる点は大きく２つある。
 * 
 * (1)テキストログの場合、ファイルヘッダが確認出来ないため、
 *    バイトオーダーや 次のリンクタイプ(Ether2/SLL)が特定できない。
 *
 *      従ってヘッダ情報を格納する領域(.head と .ifs[])には何も格納しない。
 *      そういった理由でバイトオーダーは iOrder で呼び元が指定する。
 *      リンクタイプは不明なので signal.sig.ptype は COM_SIG_UNKNOWN になる。
 *      例えば com_analyzeSignal() や com_analyzeSignalToLast() を使えば
 *      Ether2か SLLかは自動判定を試みる。もしそのデータがその上のIPヘッダ等
 *      から始まるものであっても対応は可能。
 *      もちろんプロトコルがなにか分かっているのなら、そのプロトコルの解析I/Fを
 *      すぐに呼んでも問題ない。
 *
 * (2)結合データの読込に対応する。
 *
 *      IPフラグメント等が行われている場合、テキストでパケットデータを出すと
 *      全てのフラグメントが見つかった時点で、それらを結合したデータが、
 *      テキストの中に出力される。toscomではこのデータも読み込んで使うことで
 *      フラグメントデータの結合処理を省略している。
 *
 * oCapInfに設定する内容は差分があるので、それも含めて以下に示す。
 * com_readCapFile()と差分があるものに [差分あり] と付記する。
 *   .cause      falseを返すときにその要因を設定。
 *               読み込むデータが無くなったら falseを返す。
 *               (.cause=COM_CAPERR_NOMOREDATA)
 *               ただファイル末尾まで読み切った場合も、これには含まれる。
 *   .fileName   iPathの内容をコピー。
 *               メモリ確保に失敗したら falseを返す(.cause=COM_CAPERR_COPYNAME)
 *   .fp         iPathで指定したファイルのファイルポインタ
 *               ファイルオープンNG時は falseを返す(.cause=COM_CAPERR_OPENFILE)
 *   .head       [差分あり] データは何も設定しない。
 *   .ifs[]      [差分あり] データは何も設定しない。
 *   .signal.sig 取得したパケット1つ分のデータを格納。(true返送時)
 *               [差分あり] .signal.sig.ptype には COM_SIG_UNKNOWNを設定。
 *               パケット取得NG時は falseを返す(.cause=COM_CAPERR_GETSIGNAL)
 *               iPathを NULL指定して継続読込した場合、一度内容を解放して
 *               新たにメモリ確保を実施する。
 *   .signal.ras [差分あり] Reassembled で記述された結合データを検出したら
 *               その内容を格納する。解析I/Fは必要があればこちらを自動使用する。
 *               取得NG時は falseを返す(.cause=COM_CAPERR_GETSIGNAL)
 *   .hasRas     [差分あり] .signal.ras にデータ格納があったら trueを設定。
 */
BOOL com_readCapLog( const char *iPath, com_capInf_t *oCapInf, BOOL iOrder );


// ＊＊＊ 参考情報 と 制限事項 ＊＊＊
//
//   tsharkと tcpdumpではテキストでのパケット内容打ち出しの書式が若干異なる。
//   (更に今後のバージョンアップでどちらも変更になる可能性がある)
//
//     tsharkの場合
//   0000  01 00 0c cc cc cc 50 1c bf a7 4b c3 01 b4 aa aa   ......P...K.....
//     アドレス情報は16進数4桁。
//     信号バイナリは 1octずつ空白区切り、1行に16個、右端にアスキー文字列出力。
//
//     tcpdumpの場合
//   (TAB)0x0000:  4500 0050 0000 4000 4084 9037 0ab4 ca43  E..P..@.@..7...C
//     先頭に TAB(\t)1つ。
//     アドレスは 0x付きの16進数4桁で : も付与。
//     信号バイナリは 2octずつ空白区切り、1行に16個、右端にアスキー文字列出力。
//     アスキー文字列は -X 指定時のみ出力。-x 指定時は出ない。
//   
//   toscomでは上記どちらの書式でも読み取れるように処理を作り込んでいる。
//   しかし、16oct未満のバイナリを読み込むことが現状できない という制限がある。
//           ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//   これは16個のオクテット(1行の最大数)を取得しなければ、
//   1行ごとのオクテット列のサイズを特定できないから。
//   読み取り処理の中では、そのサイズの間には16進数しか入らないはず、と期待し
//   もしそれ以外の文字が入っている場合は、取得すべきデータではないと判断する。
//
//   例えば下記のような行は、処理的には一見 取得すべきデータに見える。
//     0000 00.. = Differentiated Services Codepoint: Defai;t (0x00)
//   しかしオクテット列があるという範囲内に 16進数文字列以外が入るので、
//   データ取得するべき行とは判定されない。
//
//   一方下記の行は取得するべき行として判断したい。
//     0050  00 01 00 0c 00 06 00 05 00 00                     ..........
//           | <--- この範囲をオクテット列領域と判断  ---> |
//   オクテット列領域は 16進数文字列だけなので、これはデータ取得行と判定したい。
//   
//   こうしたことを判断するにはオクテットが16個並んだ行がどうしても必要になる。
//   それがない場合、うまくデータを取得できない。
//
//   com_readCapLog()・com_seekCapLog() 共通でこの制限事項を持っている。


/*
 * ファイル走査用関数  com_seekCapLog()
 *   true/falseを返して com_seekFile()に処理続行要否を通知する。
 *   ユーザーデータの .notifyで通知関数を指定していた場合、その返り値は、
 *   そのまま本I/Fの返り値になる。
 *   処理終了後、ioInf->capInf は com_freeCapInf()で解放が必要。
 * ---------------------------------------------------------------------------
 *   信号データ領域確保のための com_realloc()によるエラー。
 * ===========================================================================
 *   マルチスレッドによる影響は com_seekFile()に準じる。
 * ===========================================================================
 * com_readCapLog()で読めるテキストログを、com_seekFile()を使って読むためのI/F。
 * com_readCapLog()で事足りるのなら、こちらを使う必要はない。
 * 読込実施時に何か独自の判定などをしたい場合は、こちらの使用を検討する。
 * 
 * com_seekFile()の引数 iFunc に本I/Fを指定する。
 * 更に com_seekCapInf_t型データを用意し、そのアドレスを ioUserDataで指定する。
 * このデータは以下のように設定する：
 *   .capInf     0クリアを想定
 *               処理終了後は com_freeCapInf()で解放もすること。
 *   .isReading  falseを固定設定
 *   .order      ネットワークバイトオーダーを設定(Linuxなら true)
 *   .notify     パケットデータが読み込めたら、それを通知するための関数。
 *               iSignal には読み込んだパケットの情報が格納されている。
 *               (例えばそのデータの解析を行う処理を記述した関数)
 *               その返り値が本I/Fの返り値になる。つまり com_seekFile()で
 *               処理を継続するつもりなら true、中断するなら falseを返すこと。
 *
 *  C99なら、設定例としては
 *  --------------------------------------------------------------------------
 *    com_seekCapInf_t  userData = { .order = true,  .notify = execAnalyze };
 *    (void)com_seekFile( fileName, com_seekCapLog, &userData );
 *    com_freeCapInf( &userData.capInf );
 *  --------------------------------------------------------------------------
 *  となる。(execAnalyzer や fileName は呼ぶ側が別途定義したもの)
 *  こう初期化すれば、userDataのその他のメンバー(.capInf と .isReading)は
 *  0クリアなので、結果として必要な初期化を完了できる。
 *
 * .notifyで指定した引数 iSignalに設定される内容は
 * com_readCapLog()の oCapInf->signal と同じ内容となる。
 * (つまり .sig や .ras が設定される)
 *
 * com_seekFile()で直接本I/Fを指定せず、別関数を指定して、その中で
 * 本I/Fを呼ぶようにすることも出来る。その場合でも ioUserData で上記のデータは
 * 用意するようにして、そのアドレスを本I/Fを呼ぶときに引数指定すること。
 * ioUserDataに別の構造体を指定するときでも、そのメンバに com_seekCapInf_t型の
 * データがある状態にすれば。何も問題ない。そのアドレスを本I/Fに渡せば良い。
 */

// ユーザー情報データ構造
typedef BOOL(*com_notifyPacketCB_t)( com_sigInf_t *iSignal );
typedef struct {
    com_capInf_t           capInf;       // 作業用情報
    BOOL                   isReading;    // 作業用情報
    BOOL                   order;        // ネットワークバイトオーダー
    com_notifyPacketCB_t   notify;       // パケット情報通知CB
} com_seekCapInf_t;

BOOL com_seekCapLog( com_seekFileResult_t *ioInf );



/*
 *****************************************************************************
 * COMSIGDEBUG: シグナル機能デバッグ用I/F
 *****************************************************************************
 */

/*
 * シグナル機能デバッグ設定/取得  com_setDebugSignal()・com_getDebugSignal()
 *   com_getDebugSignal()はデバッグフラグの内容を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 * com_setDebugSignal()で iModeを trueに設定すると、シグナル機能動作時に、
 * デバッグログを幾つか出力するようになる。
 *
 * 主に DEBUGSIG() を使ったデバッグ出力を制御する。
 * com_signal.c は、このモードがON(true)時に画面出力する debugHead() と
 * debugSignal() という内部関数を有している。
 *
 * com_getDebugSignal()は現在のデバッグモードを返す。
 */
void com_setDebugSignal( BOOL iMode );
BOOL com_getDebugSignal( void );

// デバッグ出力マクロ (com_setDebugSignal()で true設定時に出力)
#define DEBUGSIG( ... ) \
    if( com_getDebugSignal() ) {com_printf( __VA_ARGS__ );}

// バイナリダンプマクロ (デバッグモード関係なく使用可能)
#define BINDUMP( TARGET ) \
    com_printBinary( (TARGET)->top, (TARGET)->len, \
                     &(com_printBin_t){ .prefix = "# ", .colSeq = 1 } );



/*
 *****************************************************************************
 * シグナル機能のヘッダファイルインクルードと、使用フラグマクロ設定
 *****************************************************************************
 */
#include "com_signalCom.h"
#include "com_signalSet1.h"

// シグナル機能 導入フラグ ///////////////////////////////////////////////////
#define USING_COM_SIGNAL1
// シグナル機能 初期化関数マクロ (COM_INITIALIZE()で使用) ////////////////////
#ifdef COM_INIT_SIGNAL1
#undef COM_INIT_SIGNAL1
#endif
#define COM_INIT_SIGNAL1  com_initializeSignal()

