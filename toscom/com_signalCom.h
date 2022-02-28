/*
 *****************************************************************************
 *
 * 共通部シグナル機能 (解析/デコード共通処理)    by TOS
 *
 *   シグナル機能の、プロトコルの解析/デコードで使用する共通処理をまとめた。
 *   本ヘッダは com_signal.h でインクルードされているため、
 *   個別にインクルードするのではなく com_signal.h のみインクルードして、
 *   開発を進めること。
 *
 *   現在含まれている機能
 *   ・COMSIGCOM:     共通基本I/F
 *   ・COMSIGCOMANLZ: 解析I/F向け共通処理I/F
 *   ・COMSIGCOMDECO: デコードI/F向け共通処理I/F
 *   ・COMSIGCOMFRAG: 信号フラグメント処理I/F
 * 
 *****************************************************************************
 */

#pragma once

/*
 *****************************************************************************
 * COMSIGCOM: 共通定義I/F
 *****************************************************************************
 */

// PROTOCOL_AVAILABLE
//   AVAILABLEと書いておきながら、下記に示すプロトコルは、現状未対応。
//   今後 解析処理を書けたら・・という研究課題を示している。
enum {
    /*** ファイルタイプ (1-99) ***/
    COM_SIG_PPP           = 99,     // DLT_PPP (0x09) RFC1661
    /*** Link層 (101-199) ***/
    /*** Network層 (201-299) ***/
    /*** Transport層 (301-399) ***/
    /*** No.7(SIGTRAN) (401-499) ***/
    COM_SIG_ANSITCAP      = 499,
    COM_SIG_ANSIMAP       = 498,
    /*** Application層 (501-599) ***/
    COM_SIG_HTTP2         = 589,    //            RFC7540/7541
    COM_SIG_RADIUS        = 599,    // port 1645  RFC2865
    /*** プロトコル認識のみ (601-699) ***/
    /*** 内部処理用 ***/
    COM_SIG_ONLYHEAD =  -999     // ヘッダ編集のみ(com_setHeadInf()で使用)
};

/*
 * シグナル機能解析処理初期化  com_initializeAnalyzer()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 * com_initializeSignal() から呼ばれるため、本I/Fを直接使用する必要はない。
 */
void com_initializeAnalyzer( void );



// 解析/デコード起点処理 /////////////////////////////////////////////////////

/*
 * 汎用信号解析  com_analyzeSignal()
 *   解析結果を true/false で返す。
 *   ioHeadは不要になったら com_freeSigInf()で解放する必要がある。
 * ---------------------------------------------------------------------------
 *   解析I/F検索のための com_searchSigAnalyzer()によるエラー
 *   実際に実行されたプロトコル解析I/Fによるエラー
 *   com_analyzeAsLinkLayer()・com_analyzeAsNetworkLayer() によるエラー
 * ===========================================================================
 *   マルチスレッドによる影響は無い。
 * ===========================================================================
 * COM_ANALYZER_PRM は解析I/Fで共通の引数マクロで、その実体は
 *    com_sigInf_t *ioHead, BOOL iDecode
 * となる。引数にこのマクロが使われていると 解析I/F と見分けられる。
 *
 * ioHeadで与えられた信号データを解析する。
 * iDecode(解析結果を出力するかどうか)は解析I/Fにそのまま渡される。
 *
 * もし ioHead->sig.ptype にプロトコル種別が既に指定されていたら、
 * そのプロトコルの解析I/Fを呼び出す。直接呼ぶのと変わらない動作となる。
 * 特定のプロトコルとして解析させたいなら、ioHead->sig.ptype にその種別値を
 * 設定して、本I/Fを呼ぶことで対応した解析I/Fにかけられる。
 * (この方法はどちらかというと com_analyzeSignalToLast()で使われるだろう)
 * 
 * ioHead->sig.ptypeが COM_SIG_UNKNOWN だったら、
 * まず com_analyzeAsLinkLayer()による解析を試み、上手くいかなかったら
 * com_analyzeAsNetworkLayer()による解析を試みる。これも上手くいかなかったら
 * false を返して処理を終了する。
 */
BOOL com_analyzeSignal( COM_ANALYZER_PRM );

/*
 * 汎用連続信号解析  com_analyzeSignalToLast()
 *   処理結果を true/false で返す。
 *   ioHeadは不要になったら com_freeSigInf()で解放する必要がある。
 * ---------------------------------------------------------------------------
 *   com_analyzeSignal()によるエラー
 * ===========================================================================
 *   マルチスレッドによる影響は無い。
 * ===========================================================================
 * COM_ANALYZER_PRM は解析I/Fで共通の引数マクロで、その実体は
 *    com_sigInf_t *ioHead, BOOL iDecode
 * となる。引数にこのマクロが使われていると 解析I/F と見分けられる。
 *
 * ioHeadで与えられた信号データを com_analyzeSignal()を使って解析する。
 * iDecodeは com_analyzeSignal()にそのまま渡す。
 *
 * 解析の結果、次プロトコルスタックが存在する場合(ioHead->next.cnt > 0の時)は
 * その次プロトコルスタックを com_analyzeSignal()を使って解析する。
 * こうして信号の最後まで(もしくは解析が中断するまで)連続解析を実行する。
 *
 * com_analyzeSignal()で自動判別できるプロトコル(SLL/Ether2/ARP/IPv4/IPv6)
 * ではない場合、ioHead->sig.ptype にそのプロトコル種別を設定する必要がある。
 *
 * 解析した結果 ioHead に対しては、com_detectProtocol()を使うことで、
 * 特定プロトコルが含まれているかどうかや、最初のプロトコル種別が何かを
 * チェックすることが可能。
 */
BOOL com_analyzeSignalToLast( COM_ANALYZER_PRM );

/*
 * 特定レイヤー信号解析  com_analyzeAsLinkLayer()・com_analyzeAsNetworkLayer()
 *   処理結果を true/false で返す
 *   ioHeadは不要になったら com_freeSigInf()で解放する必要がある。
 * ---------------------------------------------------------------------------
 *   各プロトコルの解析I/Fによるエラー
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 * COM_ANALYZER_PRM は解析I/Fで共通の引数マクロで、その実体は
 *    com_sigInf_t *ioHead, BOOL iDecode
 * となる。引数にこのマクロが使われていると 解析I/F と見分けられる。
 *
 * ioHeadで与えられた信号データを解析する。
 * iDecode(解析結果を出力するかどうか)は解析I/Fにそのまま渡される。
 *
 * com_analyzeAsLinkLayer()は ioHeadをリンク層(SLL または Ether2)と仮定して
 * 解析I/Fにかける。次プロトコルとして IPや ARPといった解析可能なプロトコルを
 * 示す値が見つかれば、解析はできたと判断する。
 *
 * com_analyzeAsNetworkLayer()は ioHeadをネットワーク層(IP または ARP)と仮定
 * して解析I/Fにかける。先頭オクテットから IPバージョンと思われる数値(4か6)が
 * 取得できたら、そのバージョンのIPヘッダと判断して解析する。
 * うまくいかなければ、ARPと仮定して解析を試みる。
 *
 * com_analyzeSignal()はプロトコルが特定されていない ioHeadを渡された場合
 * (ioHead->sig.ptype が COM_SIG_UNKNOWN (0)の場合)、
 * 本I/Fを使ってプロトコル特定を試みるが、SLL/Ether2/IPv4/IPv6/ARP 以外の
 * プロトコルのデータだった場合は、自動判定は出来ない。
 */
BOOL com_analyzeAsLinkLayer( COM_ANALYZER_PRM );
BOOL com_analyzeAsNetworkLayer( COM_ANALYZER_PRM );

/*
 * 特定プロトコル検出  com_detectProtocol()
 *   指定したプロトコルの検出数を返す。見つからなければ 0 を返す。
 *   *oResultは使い終わったら com_free()で解放すること。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iHead
 *   oResultのメモリ確保のための com_realloc()によるエラー
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * 解析I/Fをかけた後のデータを ioHeadに指定し、iProtocolで指定したプロトコルの
 * データがあるかどうかを検索し、見つけた数を返す。
 * iHead と その次以降のデータから検索する。もし iHead->prev が非NULLでも
 * (＝前データがある場合でも)、その方向にさかのぼって検索はしない。
 *
 * iProtocolに指定できるのは COM_SIG_～ で宣言されたプロトコル名マクロ。
 *
 * oResult を NULLにした場合は、それ以上のことはしないので、
 * 単純に指定したプロトコルがデータに含まれている数を調べるI/Fとなる。
 *
 * oResultに指定するのは、com_sigInf_t**型変数を NULL初期化して定義し、
 * & を付与したものを想定する。この指定をしている場合、検出したプロトコルの
 * データをポイントしたアドレスを並列して設定していく。
 * あくまで iHeadの情報内のアドレスを集めた配列にしかならないので、
 * メモリ解放は com_free()で問題ないし、解放したからと言って 元のデータ(iHead)
 * が壊れるわけでもない。
 *
 * 以下、使用例となる：
 *     com_siginf_t**  list = NULL;
 *     long  cnt = com_detectProtocol( &list, headData, COM_SIG_END );
 *     if( cnt > 0 ) {
 *         // list[0],list[1],... で検出したデータにアクセスできる
 *         com_free( list );  // 使い終わったら解放
 *     }
 */
long com_detectProtocol(
        com_sigInf_t ***oResult, com_sigInf_t *iHead, long iProtocol );

/*
 * 特定プロトコル取得  com_getProtocol()
 *   検出したプロトコルデータへのアドレスを返す。
 *   検出できなかったら NULL を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iHead
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * com_detectProtocol()と似ているが、こちらは iHeadで指定した解析結果中で
 * iProtocolで指定したデータがあるなら、最初に見つけたデータへのアドレスを
 * 直接返す。こちらで返すアドレスは、使用後の解放は不要。
 *
 * iProtocolに指定できるのは COM_SIG_～ で宣言されたプロトコル名マクロ。
 *
 * もし同じプロトコルのデータが複数乗り得る場合は、com_detectProtocol()を
 * 使用すること。
 */
com_sigInf_t *com_getProtocol( com_sigInf_t *iHead, long iProtocol );

/*
 * 汎用信号でコードI/F  com_decodeSignal()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iHead
 * ===========================================================================
 *   マルチスレッドで影響は受けない。
 * ===========================================================================
 * COM_DECODER_PRM はデコードI/Fで共通の引数マクロで、その実体は
 *    com_sigInf_t *iHead
 * となる。引数にこのマクロが使われていたら デコードI/F と見分けられる。
 *
 * 解析I/Fに掛けた後の iHeadを渡すと、iHead->sig.ptype に応じたプロトコルの
 * デコードI/Fを呼び出して、解析内容を簡易出力する。実際の出力前に
 * com_setDebugSignal( true ); でシグナル機能のデバッグを有効にする必要がある。
 *
 * 解析I/Fで iDecodeを trueにした場合、解析後にそのままデコードI/Fも呼ぶので
 * その場合、本I/Fを呼ぶ必要は基本的になくなる。
 */
void com_decodeSignal( COM_DECODER_PRM );



/*
 * 解析I/F・デコードI/F・拡張情報解放I/F について
 *
 *   本ファイルでは汎用の com_analyzeSignal() や com_decodeSignal() を示す。
 *   ただこれらのI/Fはプロトコルごとに登録された個別のI/Fを呼ぶ処理をするので
 *   いわば「起点となるI/F」に過ぎない。
 *
 *   プロトコルごとの個別I/Fについては com_signalSet?.c にて定義する。
 *   この個別I/Fには幾つか書き方のフォーマットがあるので、それを下記に示す。
 *
 * (1)解析I/F
 *
 *   プロトタイプは com_analyzeSig_t になる(com_signal.hにて宣言)。
 *   処理成否を true/false で返し、その引数は COM_ANALYZER_PRM で指定する。
 *   関数内で引数をそのまま渡したい時は COM_ANALYZER_VAR を使用すると良い。
 *
 *   その関数の最初は COM_ANALYZER_START() で必ず始め、COM_ANALYZER_END で
 *   必ず終わらなければならない。関数の途中で return を記述するのは非推奨で
 *   必ず末尾の COM_ANALYZER_END を通るようにすること。
 *
 *   COM_ANALYZER_START() は そのプロトコルのヘッダサイズをマクロ引数で渡す。
 *     この値は size_t型の HDRSIZE という変数に定義され、自由に使用できる。
 *
 *     まず引数チェックを行っており、ioHeadの内容に以下の不正があると弾く。
 *     ・ioHeadが NULL
 *     ・ioHead->sig.topが NULL
 *     ・ioHead->next.cntが COM_SIG_STATIC なのに ioHead->next.stack が NULL
 *     これらはいずれも解析処理の継続が非常に困難になる不正である。
 *
 *     ioHead->isFragmentが true (フラグメントデータと特定済み)の場合は
 *     全てのフラグメントデータが揃うまで何も出来ないので、何もせずに
 *     true を返す。
 *
 *     ioHead->sig で指定されたデータが全て 0 だった場合、解析は出来ないので
 *     オール0データと解析し、trueを返す。
 *
 *     ioHead->sig.len が HDRSIZE(COM_ANALYZER_STARTで渡した引数)より小さい
 *     場合、不正なデータと判断し、false を返す。
 *
 *     上記のチェックを全て終えたら、解析処理の対象と判断する。
 *     BOOL型の result を定義するので、解析結果を必ず格納する。
 *
 *     他に orgHead というデータがあるが、これは巻き戻し用の変数で、
 *     COM_ANALZYER_START と COM_ANALYZER_END の中で自動使用されるので、
 *     使う側が何か意識をする必要はない。
 *
 *   あとは ioHead->sig に格納されたバイナリデータを解析する処理を記述する。
 *   解析結果を必ず result に設定して、関数末尾に処理を移すようにする。
 *   (result自体は前述したように COM_ANALYZER_START内で自動定義している。
 *    HDRSIZEも同様に自動定義しており、自由に使えるが内容変更は非推奨)
 *   
 *   解析が正常にできた場合、
 *     ioHead->sig.len は解析を済ませたデータ長
 *     ioHead->sig.ptype は解析したプロトコル値
 *   に変更する。
 *   (これらの格納には com_setHeadInf()が使える)
 *
 *   ioHead->sig.top など使用頻度の高いものは COM_SGTOP というように
 *   短縮マクロを用意している。(もちろん内部定義を増やしても問題ない)
 *
 *   バイトオーダーを意識した数値の取得/設定には
 *   com_getVal16()・com_getVal32()・com_setVal16()・com_setVal32()が使える。
 *   サイズが不定の場合、com_calcValue()が使える。
 *   これは ioHead->order を引数に指定することで信号全体のオーダーを考慮する。
 *
 *   データの取得に使える共通処理も本ファイルに幾つか用意されている。
 *   また HTTPとそれに類似したプロトコルなら テキストベース用の共通処理、
 *   (SIPや HTTPの解析I/Fを見ると分かる通り、相当に記述を簡略化出来る)
 *   ASN.1を使用するプロトコルなら、ASN.1用の共通処理が併せて使えるだろう。
 *
 *   同じプロトコルの信号が複数まとまっていた場合 ioHead->multi に格納する。
 *   (その格納には com_stackSigMulti() を使うと良い)
 *   ヘッダ内容等を分解して解析した結果は ioHead->prm に格納する。
 *   (その格納には com_addPrm～()を使うと良い。入力に応じて幾つかI/Fがある)
 *   他にもプロトコル固有の情報を何か格納したい場合 ioHead->ext (拡張情報)の
 *   使用を検討するが、メモリを動的に確保して格納することになるので、
 *   単純な com_free()での解放ができない場合、拡張情報解放I/Fも必要になる。
 *
 *   更に解析が必要なデータが続く場合 ioHead->next に設定する。
 *   複数データがある場合も、このデータなら格納が可能。
 *   ioHead->next.stack[].prev には ioHeadを格納しておく。これにより、
 *   ioHead->next.stack[] のプロトコル情報から前にさかのぼることが可能になる。
 *   (ioHead->nextへのデータ設定は１つだけなら com_setHeadInf()で可能。
 *    複数ある場合は com_stackSigNext() を使用する)
 *
 *   COM_ANALYZER_END は解析結果に沿った後片付けを実施する。
 *     resultが trueだった場合、
 *       ioHead->ras にデータ設定がある場合、それを ioHead->nextに引き継ぐ。
 *       iDecodeが trueの場合、com_decodeSignal()を呼び、解析結果を出力する。
 *     returnが falseだった場合、
 *       orgHead(処理前の ioHeadの内容が入っている)の内容で、
 *       ioHeadの内容をロールバックする。つまり解析前の状態に戻す。
 *       iDecodeが trueの場合、ERROR というデータとして出力する。
 *   
 *   上述した解析I/Fで使える共通処理は、これ以降の COMSIGCOMANLZ セクションに
 *   まとめている。
 *   また。フラグメントを扱う共通処理については、COMSIGCOMFRAG セクションに
 *   まとめている。IPv4/IPv6 や TCP でこの処理を使っている。
 *
 *
 * (2)デコードI/F
 *
 *   プロトタイプは com_decodeSig_t になる(com_signal.hにて宣言)。
 *   返り値はなく、その引数は COM_DECODER_PRM で指定する。
 *   関数内で引数をそのまま渡したい時は COM_DECODER_VAR を使用すると良い。
 *
 *   その関数の最初は COM_DECODER_START() で必ず始め、COM_DECODER_END で
 *   必ず終わらなければならない。関数の途中で return を記述するのは非推奨で
 *   必ず末尾の COM_DECODER_END を通るようにすること。
 *
 *   COM_DECODER_START は以下のような引数のチェックを実施している。
 *     ・iHead が NULL
 *     ・iHead->isFragment が true
 *     の場合は、何もせずに処理を終える。
 *     
 *     ・iHead->sig.ptypeが COM_SIG_ALLZERO の場合は、
 *     オール0のデータであることを示す出力を行い、処理を終える。
 *
 *     上記のケースの場合 COM_DECODER_ENDを通さず関数リターンするが、
 *     これは問題ない。
 *
 *   あとは与えられた iHeadの内容を出力する処理を記述する。
 *   必ず 最初は # で始める必要がある。その後のインデント数は、
 *   既存デコードI/Fの動きを見て合わせると良いだろう。
 *
 *   iHead->sig.top など使用頻度の高いものは COM_ISGTOP というように
 *   短縮マクロを用意している。(もちろん内部定義を増やしても問題ない)
 *
 *   バイトオーダーを意識した数値の取得/設定には
 *   com_getVal16()・com_getVal32()・com_setVal16()・com_setVal32()が使える。
 *   サイズが不定の場合、com_calcValue()が使える。
 *   これは ioHead->order を引数に指定することで信号全体のオーダーを考慮する。
 *
 *   出力の汎用的なI/Fは com_dispDec() で、単純な文字列出力となる。
 *   信号データのダンプも含めたい場合 com_dispSig() を使用すると良い。
 *
 *   iHead->prm の汎用的な出力には com_dispPrmList()が使えるが、
 *   書式をいろいろ考えたい場合は、自分で作ることになるだろう。
 *   
 *   com_dispVal()・com_dispPrm()・com_dispBin() も特定の形式のデータ出力に
 *   使用できる。特に com_dispPrm()は iSizeの数値次第で出力内容が変わり、
 *   デコーダーらしい出力ができるようになっている。
 *
 *   一番最後に 次プロトコルの情報を出力するが、それには com_dispNext()を
 *   基本的に使うと良い。
 *
 *   上記に示した com_disp*()は 行頭に # を出す共通動作となっており、
 *   出力内容の統一感を出しやすくなる。com_printf()を使っても問題ないが
 *   出力内容の行頭は # にすると統一感が保たれるので留意すること。
 *
 *   COM_DECODE_END は全てのデコード出力が完了した後の共通処理となる。
 *     ここで iHead->next に次データが格納されており、
 *     なおかつ その iHead->next.stack[].sig.ptype が COM_SIG_CONTINUE の場合
 *     UNKNOKWN プロトコルデータとして併せて出力する。
 *
 *   デコードI/Fで使用する共通処理は COMSIGCOMDECO セクションにまとめる。
 *
 *
 * (3)拡張情報解放I/F
 *
 *   プロトタイプは com_freeSig_t になる(com_signal.hにて宣言)。
 *   返り値はなく、その引数は解放対象となる .ext も含む com_sigInf_t*型の
 *   データになる。この関数は com_freeSigInfExt()で使用されるが、
 *   使う側がこれを直接呼ぶケースは無く、com_freeSigInf() を呼んだときに、
 *   併せて内部で使用されるのがメインの使われ方となる。
 *
 *   解析I/Fで ioHead->ext にデータを設定した場合、これは動的確保なので、
 *   メモリ解放も考慮しなければならない。
 *   単純に com_free()で良い場合は、改めてI/Fを用意する必要はないので、
 *   拡張情報を設定するときに、そのように検討するのも一つの道だろう。
 *
 *   解放I/Fは粛々と解放処理を書くだけなので、引数や処理のマクロは無い。
 *   com_signalSet1.c の com_freeDns() が解放I/Fのサンプルとなる。
 *   他にも拡張情報を使うプロトコルはあるが、特に難しいデータ保持はせず
 *   com_free()で単純に解放出来るものとなっている。
 */

/*
 *****************************************************************************
 * COMSIGCOMANLZ: 解析I/F向け共通処理
 *****************************************************************************
 */

// 解析I/F内で使用する変数マクロ
#define COM_SG       (ioHead->sig)
#define COM_SGTOP    (ioHead->sig.top)
#define COM_SGLEN    (ioHead->sig.len)
#define COM_SGTYPE   (ioHead->sig.ptype)
#define COM_ORDER    (ioHead->order)
#define COM_NEXTCNT  (ioHead->next.cnt)
#define COM_NEXTSTK  (ioHead->next.stack)
#define COM_APRM     (&(ioHead->prm))

// 解析開始マクロ用処理 //////////////////////////////////////////////////////

// 解析I/Fの先頭は必ずこのマクロ
#define COM_ANALYZER_START( MIN_LEN ) \
    size_t  HDRSIZE = (MIN_LEN); \
    if( !com_checkAnalyzerPrm( COM_FILELOC, ioHead ) ) {return false;} \
    if( ioHead->isFragment ) {return true;} \
    if( com_isAllZeroBinary( COM_ANALYZER_VAR ) ) {return true;} \
    if( COM_SGLEN < HDRSIZE ) { \
        com_error( COM_ERR_ILLSIZE, "too short data length(%zu/%zu)", \
                   COM_SGLEN, HDRSIZE ); \
        return false; \
    } \
    BOOL  result = false; \
    com_sigInf_t  orgHead = *ioHead; \
    com_dummyAnlzPrm( COM_ANALYZER_VAR ); \
    do { \
        orgHead = *ioHead; \
        com_skipMemInfo( true ); \
    }while(0)

// COM_ANALYZER_START()で特にサイズ関係ないときに指定する引数マクロ
enum { COM_NO_MIN_LEN = 1 };

// 解析I/Fの最後は必ずこのマクロ
#define COM_ANALYZER_END \
    do { \
        if( result ) { \
            com_inheritRasData( ioHead ); \
            if( iDecode ) {com_decodeSignal( ioHead );} \
        } \
        else { \
            com_rollbackSignal( ioHead, &orgHead ); \
            if( iDecode ) { \
                com_dispSig( "ERORR", COM_SGTYPE, &ioHead->sig ); \
            } \
        } \
        com_skipMemInfo( false ); \
        return result; \
    }while(0)

/*
 * 解析I/F共通マクロ用 内部処理関数群
 *   基本的に COM_ANALYZER_START か COM_ANALYZER_END で使うもので、
 *   他の用途で使うことはない。
 * ---------------------------------------------------------------------------
 *   說明記述を参照。
 * ===========================================================================
 *   マルチスレッドの影響は受けない。
 * ===========================================================================
 * COM_ANALYZER_START で使用する内部処理関数
 *   com_checkAnalyzerPrm()
 *     ioHeadに不正なデータがないかチェックし、問題があったら COM_ERR_DEBUGNG
 *     を出して falseを返す。(その後 解析I/Fも falseを返して終了する)
 *     問題がなければ trueを返す。
 *
 *   com_isAllZeroBinary()
 *     ioHead->sig の内容がオール0だったら、その 0の数を出力して falseを返す。
 *     (その後 解析I/Fは trueを返して終了する)
 *     問題がなければ trueを返す。
 *
 *   com_dummyAnlzPrm()
 *     解析I/Fの仮引数が使われない可能性を考慮したダミー処理。
 *
 * COM_ANALYZER_END で使用する内部処理関数
 *   com_inheritRasData()
 *     result(解析結果)が trueだった時、ioHead->ras を ioHead->next.stack[] に継
 *     承する。ただし解析されたプロトコルと一致した時は、解析のために使用済み
 *     となるので何もしない。
 *
 *   com_rollbackSignal()
 *     result(解析結果)が falseだった時、ioHeadの内容を一度クリアし、
 *     関数開始時の内容に巻き戻す。 
 */
BOOL com_checkAnalyzerPrm( COM_FILEPRM, com_sigInf_t *ioHead );
BOOL com_isAllZeroBinary( COM_ANALYZER_PRM );
void com_dummyAnlzPrm( COM_ANALYZER_PRM );
void com_inheritRasData( com_sigInf_t *ioHead );
void com_rollbackSignal( com_sigInf_t *oHead, com_sigInf_t *iOrgHead );


// プロトコル解析用共通処理 //////////////////////////////////////////////////

/*
 * 解析結果設定  com_setHeadInf()
 *   iNextTypeが COM_SIG_UNKNOWN でなければ true、そうであれば false を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioHead
 *   COM_ERR_ILLSIZE: ioHead.sig.len < iLength
 *   com_reallocAddr()によるエラー
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * ioHeadに解析結果の基本データを設定する。具体的には下記のように設定する。
 *   ioHead->sig.len:    iLength を設定
 *   ioHead->sig.ptype:  iType を設定
 * iNextTypeが COM_SIG_ONLYHEADの場合、ここで処理は終了となる。
 *
 * iNextTypeが COM_SIG_ONLYHEAD以外(特定のプロトコル)の場合、
 *   ioHead->next.cnt:   +1
 *   ioHead->next.stack[].sig.top:    ioHead->sig.top + iLength
 *   ioHead->next.stack[].sig.len:    ioHead->sig.len - iLength
 *   ioHead->next.stack[].sig.ptype:  iNextType を設定
 *   ioHead->next.stack[].order:      ioHead->order を設定
 *   ioHead->next.stack[].prev:       ioHead を設定
 * iLengthが解析済みサイズとなり、まだ未解析のデータ部分を nextに設定する想定。
 *
 * 本I/Fによる ioHead->next の設定は、次プロトコルスタックが複数あると使えない。
 * (これは同一レイヤで複数あることを意味する。例えば
 *  ・sctp信号で chunkが複数あり、それぞれに次プロトコルデータがある
 *    (複数chunkある場合、そのヘッダ自体は ioHead->multi に設定する)
 *  ・SIP信号でボディ部がマルチパートになっている
 *  といったパターンが考えられる)
 * その場合は iNextTypeに COM_SIG_ONLYHEAD を設定してヘッダ部分をまず設定し
 * 複数ある次プロトコルスタックは別処理でデータを作成し com_stackSigNext()を
 * 使って設定すると良い。
 * 
 * ioHead->next.stack[]に設定する際は、メモリを動的確保する。
 * しかし、ioHead->next.sig が COM_SIG_STATIC だった場合、
 * ioHead->next.stack には静的領域が既に設定されていると判断し、
 * 動的確保はせずに、そこに設定されたバッファへのデータ設定となる。
 * 静的領域の設定は com_setStaticStk() で行う。
 * この方式も次プロトコルデータが複数ある場合は使えないので注意すること。
 */
BOOL com_setHeadInf(
        com_sigInf_t *ioHead, com_off iLength, long iType, long iNextType );

/*
 * ポインタとレングスの連動動作  com_advancePtr()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oTarget
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * oTarget には対象となるポインタ、oLengthにはその現在長を格納した変数に &を
 * 付けて、そのアドレスを指定する。iAdd はポインタを進めるサイズを指定する。
 *
 * これにより *oTarget を iAdd分 加算し、*oLength は iAdd分 減算して、
 * trueを返す。
 *
 * なお、*oTargetは NULLを許容し、その場合 iAdd加算は行わない。
 * (oTargetが NULLの場合は COM_ERR_DEBUGNGのエラーになる)
 * 同様に oLengthも NULLを許容し、その場合 iAdd減算は行わない。
 *
 * *oLength が iAddより少ない時は false を返す。
 * その場合 *oTarget や *oLength は変化しない。
 */
BOOL com_advancePtr( com_bin **oTarget, com_off *oLength, com_off iAdd );

/*
 * バイトオーダー変換
 *   変換結果を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSourceで与えられた数値を、バイトオーダーに応じた変換を行う。
 *
 * iOrderはネットワークバイトオーダーなら trueを設定するが、
 * com_sigInf_t型の .order をそのまま指定することを想定している。
 * (解析I/Fなら ioHead、デコードI/Fなら iHead がこの型である)
 *
 * com_getVal16()・com_getVal32() は信号データから数値を取得するときに使う。
 * 実際の変換は iOrder が true のときのみ実施し ntohs()・ntohl() を使う。
 * iOrderが falseのときは何もせずに そのまま iSourceの値を返す。
 * 信号データから 16bit/32bit の数値を取得した時に使うことを強く推奨する。
 *
 * com_setVal16()・com_setVal32() は信号データとして数値を変換するときに使う。
 * 実際の変換は iOrder が true のときのみ実施し htons()・htonl() を使う。
 * iOrderが falseのときは何もせずに そのまま iSourceの値を返す。
 * 現状、解析の中で com_setVal16()や com_setVal32()を使う機会は少ないだろう。
 */
uint16_t com_getVal16( uint16_t iSource, BOOL iOrder );
uint32_t com_getVal32( uint32_t iSource, BOOL iOrder );

uint16_t com_setVal16( uint16_t iSource, BOOL iOrder );
uint32_t com_setVal32( uint32_t iSource, BOOL iOrder );

/*
 * オクテット列数値変換  com_calcValue()
 *   変換結果を返す。
 *   処理NG時は 0 を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iTopで与えられたバイナリ列から iSize分取得して 32bit数値に変換する。
 * 最終結果が最大32bitなので、iSizeは 1～4 の範囲でなければならない。
 * もし iSizeが 4より大きい場合、4として扱う。
 *
 * iTopが NULLだったり、iSizeが 0だったりしたら、0を返す(エラーにはしない)。
 */
uint32_t com_calcValue( const void *iTop, com_off iSize );

/*
 * ビットフィールド値取得  com_getBitField()
 *   取得した値を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSourceで与えられた 32bit数値から、iMaskでマスクしたビットの値を返す。
 *
 * iShift > 0 なら、その分 右にビットシフトする。
 * iShift < 0 なら、その分 左にビットシフトする。
 * iShift = 0 なら、ビットシフトは行わない。
 * ビットシフトによるオーバーフローは考慮しないため、呼ぶ側で注意すること。
 */
ulong com_getBitField( ulong iSource, ulong iMask, long iShift );

/*
 * 次スタック登録  com_stackSigNext()
 *   実際に追加されたスタック情報(ioHead->next.stack[])のアドレスを返す。
 *   登録に失敗した時は NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioHead || !iSource
 *   com_reallocAddr()によるエラー。
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * ioHeadで与えられた信号情報に対して、次プロトコル情報を設定する。
 *   ioHead->next.cnt:           +1
 *   ioHead->next.stack[]:       メモリを確保して iSourceの内容を設定
 *   ioHead->next.stack[].prev:  ioHeadを設定
 *
 * iSourceに設定する情報の内容は、com_setHeadInf()が ioHead->next.stack[]に
 * 設定する内容に準じたものとするべき。ただ次プロトコルの解析で困らないなら
 * それに固執する必要もない。
 * 
 * 本I/Fは解析の結果、次プロトコルデータが複数あり、それを設定するのに使う。
 * 次プロトコルデータが１つだけの場合、com_setHeadInf()で大抵の場合事足りる。
 *
 * ちなみに次プロトコルデータではなく、現在解析中のスタック内で複数信号がある
 * (例えば sctpで複数chunkが含まれている場合が該当する)場合は、
 * 本I/Fではなく com_stackSigMulti()で ioHead->multiに設定する。
 * そして、その複数chunkに次プロトコルデータがそれぞれ存在する場合、
 * それは本I/Fで ioHead->nextに設定する。
 */
com_sigInf_t *com_stackSigNext( com_sigInf_t *ioHead, com_sigInf_t *iSource );

/*
 * 最新のスタック情報取得  com_getRecentStack()
 *   スタックの最終データのアドレスを返す。
 *   iStk->cntが 0の場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iStk
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iStk->stack[iStk->cnt - 1] のアドレスを返す。
 * これはスタックの最終データを指す。
 *
 * iStk->cntが 0の場合は NULLを返す。
 *
 * com_sigStk_t*型データの代表は com_sigInf_t型の next(次プロトコルデータ)。
 * また com_sigInf_t型では multi(信号の複数スタック) も com_sigStk_t*型である。
 */
com_sigInf_t *com_getRecentStack( com_sigStk_t *iStk );

/*
 * 信号マルチスタック登録  com_stackSigMulti()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioHead || !iLabel || !iData
 *   com_reallocAddr()によるエラー
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * ioHead->multi に iDataで示した信号バイナリデータを登録する。
 * iLabelはメモリ確保のための com_reallocAddr()に渡すデバッグ用の文字列。
 *
 * 以下のようなデータ設定を行う
 *   ioHead->multi.cnt:            +1
 *   ioHead->multi.stack[].sig:    iData を設定
 *   ioHead->multi.stack[].order:  ioHead->order を設定
 *   ioHead->multi.stack[].prev:   ioHead を設定
 *
 * 本I/Fは1つの信号の中に同レイヤの信号が複数スタックされているときに使う。
 * 次プロトコルデータが複数ある場合、その設定は com_stackSigNext()を使う。
 */
BOOL com_stackSigMulti(
        com_sigInf_t *ioHead, const char *iLabel, com_sigBin_t *iData );

/*
 * パラメータリスト追加  com_addPrm()・com_addPrmDirect()・com_addPrmTlv()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oTatget
 *   COM_ERR_DEBUGNG: [com_prmNG] !iPrm  (com_addPrmTlv()のみ)
 *   com_reallocAddr()によるエラー
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * oTargetのリストにパラメータを追加する。具体的には
 *   oTarget->cnt:     +1
 *   oTarget->list[]:  指定されたデータを追加
 * となる。
 *
 * 入力データによりI/Fが変わるが、上記の処理は共通。
 * oTarget->list[] に設定する内容が下記のように異なる。
 * .tag .len .value については値を保証できるようにしている。
 * (バリュー値を取得したい時は .valueが先頭で そのサイズが .len となる)
 *
 * com_addPrm()は入力から oTarget->list[]を以下のように設定する。
 *   .tagBin.top : iTag      (タグ値のバイナリ)
 *   .tagBin.len : iTagSize  (タグ値のバイナリサイズ)
 *   .tag        : iTagと iTagSizeから算出した値
 *   .lenBin.top : iLen      (レングス値のバイナリ)
 *   .lenbin.len : iLenSize  (レングス値のバイナリサイズ)
 *   .len        : iLenと iLenSizeから算出した値
 *   .value      : iValue    (バリューのバイナリ)
 * この場合 .tag と .len は元のバイナリから計算されるが、これらの変数は
 * サイズが32bitのため iTagSize や iLenSize が 4を超えている場合、
 * 正しい値が格納できない制限事項がある。
 * 64bitまでであれば、呼び元で再計算をする余地はあると思われる。
 * 
 * com_addPrmDirect()は入力から oTarget->list[]を以下のように設定する。
 *   .tagBin.top : NULL
 *   .tagBin.len : 0
 *   .tag        : iTag      (実際のタグ値)
 *   .lenBin.top : NULL
 *   .lenbin.len : 0
 *   .len        : iLenk     (実際のレングス値)
 *   .value      : iValue    (バリューのバイナリ)
 * バイナリデータが不要な場合、こちらを使っても良いだろう。
 *
 * com_addPrmTlv()は iPrmの内容を oTarget->list[] に設定する。
 */
BOOL com_addPrm(
        com_sigPrm_t *oTarget, void *iTag, com_off iTagSize,
        void *iLen, com_off iLenSize, void *iValue );
BOOL com_addPrmDirect(
        com_sigPrm_t *oTarget, com_off iTag, com_off iLen, void *iValue );
BOOL com_addPrmTlv( com_sigPrm_t *oTarget, com_sigTlv_t *iPrm );

/*
 * 最新のパラメータ情報取得  com_getRecentPrm()
 *   パラメータの最終データのアドレスを返す。
 *   iPrm->cntが 0の場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iPrm
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iPrm->list[iPrm->cnt - 1] のアドレスを返す。
 * これはパラメータリストの最終データを指す。
 *
 * iPrm->cntが 0の場合は NULLを返す。
 */
com_sigTlv_t *com_getRecentPrm( com_sigPrm_t *iPrm );

/*
 * パラメータリスト検索  com_searchPrm()
 *   最初に見つかったパラメータのアドレスを返す。
 *   見つからなかった場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iTarget
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iTargetで指定したパラメータリストから、タグ値が iTag となっているものを
 * 検索し、そのアドレスを返す。
 *
 * データが見つからなかった場合は NULLを返す。
 * iTarget->cnt が 0の場合も NULLを返す(エラーにはならない)。
 *
 * 同じタグ値で複数のパラメータが存在する場合は、本I/Fで見つかった後の
 * パラメータを先頭とする com_sigPrm_t*型データを渡して 本I/Fを再度呼ぶ、
 * といった工夫が必要になるだろう。
 */
com_sigTlv_t *com_searchPrm( com_sigPrm_t *iTarget, com_off iTag );

/*
 * バイナリ列比較  com_matchSigBin()
 *   バイナリ列が一致する場合は true、一致しない場合は false を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iBin1 と iBin2 のバイナリ内容が完全一致するなら trueを返す。
 * 比較するのは .top がポイントするデータ内容で .len がそのサイズになる。
 * .ptype は比較対象としない(つまり結果が trueでも .ptypeは違う可能性がある)。
 *
 * iBin1 と iBin2 がどちらも NULLだった場合は trueを返す。
 * (片方だけ NULLの場合は 不一致とみなして falseを返す)
 *
 * iBun1->len と iBin2->len が異なる場合、その時点で false を返す。
 *
 * iBin1->top と iBin2->top がどちらも NULLだった場合も trueを返す。
 * (片方だけ NULLの場合は 不一致とみなして falseを返す)
 *
 * あとは iBin1->top と iBin2->top が指すデータ内容を比較した結果を返す。
 */
BOOL com_matchSigBin( const com_sigBin_t *iBin1, const com_sigBin_t *iBin2 );

/*
 * パラメータ比較  com_matchPrm()
 *   内容が一致していたら true、一致していなかったら false を返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iTlv1 と iTlv2 の内容を比較し、その結果を返す。
 * 双方が NULLだった場合は trueを返す。
 *
 * .tagBin.topが設定ある場合 .tagBin.len のサイズ分の比較となる。
 * .lenBin.topが設定ある場合 .lenBin.len のサイズ分の比較となる。
 * .value については .len のサイズ分の比較となる。
 * 上記はポイント先の内容を比較するので、アドレス自体が異なっていても、
 * 内容が同じなら一致と判定する。
 * 
 * .tag は一致するかどうかの比較となる。
 * .len は一致するかどうかの比較となる。
 */
BOOL com_matchPrm( const com_sigTlv_t *iTlv1, const com_sigTlv_t *iTlv2 );

/*
 * パラメータ複製  com_dupPrm()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iTarget || !iSource
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * iSourceの内容を oTargetに複製する。
 * ただし単純なコピーではないので、下記の説明を確認すること。
 * (端的に言うと、oTargetは設定後、信号データとは別に解放が必要になる)
 *
 * oTargetには以下のような設定を行う
 *   .oTarget->tagBin.top  : .iSource->tagBin.len分のメモリを確保し
 *                           .iSource->tagBin.topの内容をコピー。
 *   .oTarget->lenBin.top  : .iSource->lenBin.len分のメモリを確保し
 *                           .iSource->lenBin.topの内容をコピー。
 *   .oTarget->value       : .iSource->len分のメモリを確保し
 *                           .iSource->valueの内容をコピー。
 *   上記以外は iSourceからそのままコピー。
 *
 * つまり、動的メモリ確保が3ヶ所行われるため、使用後の解放が必要になる。
 * ただ元々 ここに設定するのは 元の信号データの相対位置であり、
 * そうする限りはメモリの確保は必要ない(単なる元データ参照で済むため)。
 *
 * このため com_freeSigPrm()で com_sigTlv_t*型のデータを解放するときも、
 * 上記のメンバーを個別に解放することはしていない(する必要がない)。
 * そもそもその場合は単純な構造体コピー( *oTarget = *iSource; )で良い。
 * 本来はそのような運用をするべきで、本I/Fによりデータを複製するのは、
 * かなり特殊な事例となる。
 */
BOOL com_dupPrm( com_sigTlv_t *oTarget, const com_sigTlv_t *iSource );

/*
 * ノード情報取得  com_getNodeInf()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iHead || !oInf
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * 指定された iHeadから下位データ(.prev)をたどり、
 * TCP/UDP/SCTP のデータから送信元(src)/送信先(dst)のポート番号を取得する。
 * さらに下位データをさかのぼり、IPv4/IPv6のデータがあったら、そこから
 * 送信元(src)/送信先(dst)のIPアドレスを取得する。
 *
 * 取得したポート番号とアドレスを oInfに設定して trueを返す。
 * oInf->ptype には COM_SIG_IPV4 か COM_SIG_IPV6 を設定し、IPアドレスの種別を
 * 特定できるようにする。
 *
 * ポート番号やアドレスが取得できなかった場合、falseを返す。
 */

// ノード情報データ構造
enum {
    COM_NODEADDR_SIZE = 16,
    COM_NODEPORT_SIZE = 8
};

typedef struct {
    long     ptype;                         // プロトコル種別
    com_bin  srcAddr[COM_NODEADDR_SIZE];    // 送信元IPアドレス
    com_bin  dstAddr[COM_NODEADDR_SIZE];    // 送信元ポート番号
    com_bin  srcPort[COM_NODEPORT_SIZE];    // 送信先IPアドレス
    com_bin  dstPort[COM_NODEPORT_SIZE];    // 送信先ポート番号
} com_nodeInf_t;

BOOL com_getNodeInf( com_sigInf_t *iHead, com_nodeInf_t *oInf );

/*
 * ノード情報比較  com_matchNodeInf()
 *   比較結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iInf1 || !iInf2
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iInf1 と iInf2 の内容が一致するかチェックし、その結果を返す。
 */
BOOL com_matchNodeInf( com_nodeInf_t *iInf1, com_nodeInf_t *iInf2 );

/*
 * ノード情報反転  com_reverseNodeInf()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioInf
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * ioInfで指定したノード情報の 送信元(src)と送信先(dst)を入れ替える。
 * 具体的には
 *   .srcAddr と .dstAddr の内容を入れ替え
 *   .srcPort と .dstPort の内容を入れ替え
 *   .ptype は変化なし
 * となる。
 */
void com_reverseNodeInf( com_nodeInf_t *ioInf );


// テキストベースプロトコル解析用共通処理 ////////////////////////////////////

/*
 * テキストベースプロトコルの共通解析処理  com_analyzeTxtBase()
 *   解析成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioHead || !iConLen || !iConType || !iMulti
 *   com_malloc()・com_realloc()によるエラー
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * テキストベースと呼んでいるのは、HTTPとそれをベースとするプロトコルを指す。
 * (ヘッダが並び、空行をおいてボディが続く、という構成のプロトコル)
 *
 * そうしたプロトコルの解析I/Fにおいて、解析処理全般を代替する。
 * 一連の解析処理で、本I/F以降に挙げるテキストベースプロトコル解析用の
 * その他のI/Fを駆使しており、本I/Fの引数もそうしたI/Fに渡す目的のものが多い。
 *
 * ioHeadは解析対象のデータとなる。
 * iTypeは想定するプロトコル種別値を指定する。
 * iNextはポート番号を判定するデータ種を指定する。
 * iConLenはボディ部のサイズを記述しているヘッダ名、
 * iConTypeはボディ部の種類を記述しているヘッダ名、
 * iMultiはボディ部がマルチパートになることを示すラベルとなる。
 *
 * iNext・iConLen・iConType・iMulti は本I/Fの内部処理の中で、
 * com_getTxtBody()への入力としてそのまま渡す。その使い方については、
 * こちらの説明記述も参照。
 *
 * com_signalSet1.cでは com_analyzeSip()がこれを使っている例となる。
 * (com_analyzeSip()は本I/Fを呼ぶだけで、解析処理を全面的に委ねている)
 *
 * 本I/Fで解析したioHead は com_decodeTxtBase()でデコード可能。
 *
 * もし本I/Fで解析が難しい場合、以降に示す共通処理も駆使して、
 * 独自の解析処理を記述する必要があるだろう。
 */
BOOL com_analyzeTxtBase(
        com_sigInf_t *ioHead, long iType, COM_PRTCLTYPE_t iNext,
        const char *iConLen, const char *iConType, const char *iMulti );

/*
 * テキストヘッダ取得  com_getTxtHeader()
 *   ヘッダ取得後、その先の信号データポインタを返す。
 *   処理NGだった場合 NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioHead || !oHdrSize
 *   com_reallocAddr()によるエラー
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * ioHeadをテキストデータベースのプロトコルとみなし、
 * そのヘッダー一覧を作成して、ioHead->prmに格納する。
 * ヘッダ部分のサイズを *oHdrSize に格納する。格納不要の場合は NULLを指定する。
 *
 * ヘッダの終了は空行(\r\n)で判断する。
 * 最終的な返し値は、この空行の直後を指すポインターとなる。
 * ボディがある場合は、ボディの先頭部分ということになる。
 */

// 内部で使用するデータ構造
typedef struct {
    com_sigInf_t*  head;
    com_off*       size;
} com_sigTextSeek_t;

char *com_getTxtHeader( com_sigInf_t *ioHead, com_off *oHdrSize );

/*
 * テキストヘッダ設定  com_setTxtHeaderVal()
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !oTlv || !iHdr
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iHdrで指定する文字列が「ヘッダ名 iSep指定文字 値」という形式であると期待し、
 * oTlvにそのヘッダ内容を格納する。
 * iSepには例えば ':' や '=' を設定する。前後の空白は無視して取得する。
 *
 * *oTlvへには以下のような設定を行う。
 *   .tagBin:   .topにヘッダ名先頭位置、.lenに iSep文字までの長さ
 *   .tag:      0固定
 *   .lenBin:   オール0固定
 *   .len:      値(iSep文字以降の非空白から始まる)のサイズ
 *   .value     値(iSep文字以降の非空白から始まる)の先頭位置
 * 
 * ヘッダ名を得たい時は .tagBinから取得、値は .valueと .lenから取得する。
 */
void com_setTxtHeaderVal( com_sigTlv_t *oTlv, char *iHdr, int iSep );

/*
 * テキストヘッダ取得  com_getTxtHeaderVal()
 *   指定したヘッダの値の文字列を返す。
 *   指定したヘッダが無い場合は NULLを返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iPrm || !iHeader
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * com_getTxtHeader()で取得したヘッダリスト(ioHead->prm)から、
 * ヘッダ名を iHeaderで指定し、その値を返す。内部一時バッファのアドレスなので
 * 継続してその文字列を使用したい場合は、別バッファに即座にコピーすること。
 *
 * ヘッダリストの中に指定した名前のヘッダが無い場合は NULLを返す。
 */
char *com_getTxtHeaderVal( com_sigPrm_t *iPrm, const char *iHeader );

/*
 * テキストボディ取得  com_getTxtBody()
 *   処理結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioHead || !iConLen || !iConType || !iMulti
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * ioHeadに com_getTxtHeader()でヘッダ部分を解析したデータが入っていると想定し
 * そのボディ部分の情報を ioHead->next に設定する。
 *
 * iNextはボディ部のプロトコル判定に使用するプロトコル情報種別を指定する。
 * iHdrSizeはヘッダ部分のサイズを設定する(com_getTxtHeader()で設定可能)。
 * iConLenはボディ部の長さを示すヘッダ名、
 * iConTypeはボディ部のプロトコル判定に使用するヘッダ名、
 * iMultiはボディ部がマルチパートであることを示す文字列を設定する。
 *
 * 例えばSIPの場合、iNextは COM_SIPNEXT になる。
 * iConLenは  "Content-Length"  (COM_CAP_SIPHDR_CLENGTH のマクロ)
 * iConTypeは "Content-Type"    (COM_CAP_SIPHDR_CTYPE のマクロ)
 * iMultiは   "multipart-mixed" (下記 COM_CAP_CTYPE_MULTI のマクロ)
 * を設定する。ヘッダ名はだいたいどのプロトコルでも共通かもしれない。
 *
 * なお下位プロトコルに TCP が含まれ、Content-Lengthに示されたサイズのデータが
 * 無い場合は、TCPセグメンテーションが発生したと認識し、そのサイズのデータが
 * 後続パケットで揃うまで、解析処理を保留する。全て揃ったら結合して、
 * そのデータで解析を行う。
 */

// Content-Type用 汎用文字列定数
#define COM_CAP_CTYPE_MULTI     "multipart/mixed"
#define COM_CAP_CTYPE_APPETC    "application/"
#define COM_CAP_CTYPE_TXTETC    "text/"

BOOL com_getTxtBody(
        com_sigInf_t *ioHead, COM_PRTCLTYPE_t iNext, com_off iHdrSize,
        const char *iConLen, const char *iConType, const char *iMulti );

/*
 * メソッド/ステータスコード取得  com_getMethodName()
 *   処理成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iTable || !iSig
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * テキストベースのプロトコルの信号データ iSig を対象にして、
 * iTable で与えられるメソッド名リストを元にi、信号メソッド名を取得して
 * *oLabel に設定する。またそのインデックス値を *oTypeに設定する。
 * oLabel・oType は NULLを許容しており、その場合 データの格納を行わない。
 *
 * iTableには信号内で先頭に現れるメソッド名と、対応する数値(99未満の任意値)を
 * 並べたもので .sigType を COM_SIG_METHOD_END にしたデータを必ず最後に配置
 * したテーブルとする。
 *
 * レスポンスの場合、文字列のメソッド名ではなく 100以上のステータスコードが
 * 記述されていることを想定しており、そのステータスコードを取得して、
 * *oTypeにその値を格納して返す。呼んだ側は *oTypeが COM_SIG_METHOD_ENDより
 * 小さければリクエスト、大きければレスポンスと判断できる。
 *
 * iTableの設定例としては com_signalSet1.cの gSipName[] が挙げられる。
 * これは SIPのメソッド名のテーブルとなる。
 */

// 文字列とメソッド名の対応データ構造
typedef struct {
    const char*   token;
    long          sigType;
} com_sigMethod_t;

// com_sigMethod_t型テーブル最終データの .sigTypeに設定する値
enum { COM_SIG_METHOD_END = 99 };

BOOL com_getMethodName(
        com_sigMethod_t *iTable, void *iSig, const char **oLabel, long *oType );

/*
 * テキストヘッダ行残り文字数取得  com_getHeaderRest()
 *   指定テキストヘッダ行の残り文字数を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iHdr
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iHdrはテキストヘッダ内の文字列ポインタで、そこから行末までの文字数を返す。
 * この場合の「行末」とは '\0' '\r' '\n' のいずれかの文字を指しており、
 * その直前までの文字数を返す。
 *
 * iSep に特定の文字コードを設定すると、その文字までの文字数になる。
 * 文字が見つからない場合は 行末までの文字数になる。
 * 指定が不要な場合は iSep に 0 を設定する。
 */
com_off com_getHeaderRest( char *iHdr, int iSep );

/*
 * テキストヘッダ内パラメータ検索  com_seekTxtHeaderPrm()
 *   処理結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iHdr || !iPrm || !oTop || !oLen
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iHdrで指定されたヘッダ文字列から、iPrmで指定されたパラメータ名を検索し、
 * 見つかった場合は その値を *oTop と *oLen に格納して trueを返す。
 * 見つからなかった場合は false を返す。
 */
BOOL com_seekTxtHeaderPrm( char *iHdr, char *iPrm, char **oTop, com_off *oLen );

/*
 * テキストトークン一致チェック  com_matchToken()
 *   一致していた場合は、その次のトークンまでの文字数を返す。
 *   一致していない場合は 0 を返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iSource || !iTarget
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSourceにチェック対象となる文字列から始まるポインタを指定し、
 * iTargetにチェックしたい文字列を指定する。
 *
 * iNoCaseは大文字小文字を区別せずにチェックしたいときに true を指定する。
 *
 * ホワイトスペース以外にトークンを区切る文字を指定したい時は iSpp を使う。
 * 文字指定が不要の場合は 0 を指定する。
 *
 * まず、iSourceから始まる文字列が iTargetと一致するかチェックする。
 * (一致しなかったら、その時点で false を返す)
 * 続いて、iSourceでその次の文字がホワイトスペースか iSep指定文字かをチェックし
 * そうであればトークン一致として trueを返す。そうでなければ falseを返す。
 * (ホワイトスペースとは 空白・タブ文字・改行文字・'\0' である)
 *
 * iSourceが "SAMPLESET" だった場合、iTargetが "SAPMLE" だと falseになる。
 * もし iSourceが "SAMPLE SET" だった場合、trueになることを意味する。
 */
com_off com_matchToken(
        char *iSource, const char *iTarget, BOOL iNoCase, const int iSpec );


// ASN.1形式解析用共通処理 ///////////////////////////////////////////////////

// パラメータ解析用定数：タグクラス (上位2bit)
enum {
    COM_ASN1_UNIVERSAL  = 0x00,    // UNIVERSAL class
    COM_ASN1_APPLI      = 0x40,    // APPLICATION class
    COM_ASN1_CONTEXT    = 0x80,    // CONTEXT class
    COM_ASN1_PRIVATE    = 0xc0     // PRIVATE class
};

// パラメータ解析用定数：タグ形式 (3bit目)
enum {
    COM_ASN1_PRMFORM    = 0x00,    // Primitive Form
    COM_ASN1_CONFORM    = 0x20     // Constructor Form
};

// Constructor Form判定マクロ
#define COM_ASN_ISCONST( TAG ) \
    COM_CHECKBIT( (TAG), COM_ASN1_CONFORM )

// パラメータ解析用定数： indefinite formの EOCタグ＆レングス
enum {
    COM_ASN1_EOC_TAG    = 0x00,
    COM_ASN1_EOC_LENGTH = 0x00
};

// パラメータ解析用定数：ASN.1で定義されたデータ種別
enum {
    COM_ASN1_BOOL       = 0x01,    // BOOLEAN type Tag
    COM_ASN1_INT        = 0x02,    // INTEGER type Tag
    COM_ASN1_BITSTR     = 0x03,    // BIT STRING type Tag
    COM_ASN1_OCTSTR     = 0x04,    // OCTET STRING type Tag
    COM_ASN1_NULL       = 0x05,    // NULL type Tag
    COM_ASN1_OBJID      = 0x06,    // OBJECT IDENTIFIER type Tag
    COM_ASN1_OBJDES     = 0x07,    // OBJECT DESCRIPTOR type Tag
    COM_ASN1_EXT        = 0x08,    // EXTERNAL type Tag
    COM_ASN1_REAL       = 0x09,    // REAL type Tag
    COM_ASN1_ENUM       = 0x0a,    // ENUMRATED type Tag
    COM_ASN1_SEQ        = 0x10,    // SEQUENCE(OF) type Tag
    COM_ASN1_SET        = 0x11     // SET(OF) type Tag
};

/*
 * ASN的な TLV取得  com_getAsnTlv()
 *   処理結果を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !ioSignal || !oPrm
 *   com_reallocAddr()によるエラー
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * 与えられた信号データ *ioSignal を先頭から ASN.1の TLV形式とみなして解析し、
 * *oPrm にパラメータリストとして格納する。
 * 最終的に *ioSignal は解析後の位置に変化する。
 *
 * レングスが indefinite の場合 iPrm->spec を内部的に使用するが、その内容を
 * 呼ぶ側が意識する必要はない。ただし処理的には暫定的なもので、対応しきれない
 * 事態が発生する可能性がある。
 */
BOOL com_getAsnTlv( com_bin **ioSignal, com_sigPrm_t *oPrm );

/*
 * ASN的な TLV検索  com_searchAsnTlv()
 *   検索成否を true/false で返す。
 * ---------------------------------------------------------------------------
 *   COM_ERR_DEBUGNG: [com_prmNG] !iPrm || !iTags || !oValue
 * ===========================================================================
 *   マルチスレッドの影響は考慮済み。
 * ===========================================================================
 * com_getAsnTlv()で作成したリスト iPrm から iTags と iTagSize で指定した
 * タグを検索し、その値を *oValue に格納する。
 * oValue には com_sigBin_t型変数を定義し、& を付けてそのアドレスを渡すことを
 * 想定する。
 */
BOOL com_searchAsnTlv(
        com_sigPrm_t *iPrm, uint32_t *iTags, size_t iTagsSize,
        com_sigBin_t *oValue );

/*
 * ASN的なタグ長取得  com_getTagLength()
 *   指定されたタグの長さを返す。
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響を受ける処理はない。
 * ===========================================================================
 * iSignal をタグ値の先頭とみなし、その長さ(オクテット数を返す)。
 * またそれを加味した実際のタグ値を *oTag に設定する。設定が不要の場合は
 * NULL を指定する。
 *
 * (参考情報) タグ長の考え方
 *   タグ値のオクテット値の最上位ビット(8bit目)が 0 の場合は
 *   そのオクテットのみがタグ値となり、 0x01～0x7f の short formか
 *   0x00 の indefinite form (不定長) となる。
 *   上記のパターンの場合、1 を返し、タグ値をそのまま *oTagに設定する。
 *
 *   最上位ビット(8bit目)が 1 の場合、long form と認識し、
 *   そのビットを除いた 7bit が実際のタグ自体の長さになり、
 *   その後に続く その長さ分のオクテットがタグ値となる。
 *   *oTag には連結した値を格納するが、タグ長が com_offのサイズをを超えると
 *   正しい結果にならないので、呼び元で *oTagは参照しないこと。
 *   (com_offのサイズは 64bitOSなら 64bit (＝8オクテット)、
 *    32bitOSなら 32bit (＝4オクテット) となる)
 *
 *   本I/Fで返すタグ長は 最初のタグ自体の長さも含めるため、
 *   そのタグ自体の長さ + 1 になる。
 *
 *   例えば *iSignal が 12 0a 1c ... というバイナリ列だった場合、
 *   *oTag には 0x0a1c が設定され、返り値自体は 3 となる。
 */
com_off com_getTagLength( com_bin *iSignal, com_off *oTag );



/*
 *****************************************************************************
 * COMSIGCOMDECO: デコードI/F向け共通処理
 *****************************************************************************
 */

// デコードI/F内で使用する変数マクロ
#define COM_ISG       (iHead->sig)
#define COM_ISGTOP    (iHead->sig.top)
#define COM_ISGLEN    (iHead->sig.len)
#define COM_ISGTYPE   (iHead->sig.ptype)
#define COM_IORDER    (iHead->order)
#define COM_IMLTICNT  (iHead->multi.cnt)
#define COM_IMLTISTK  (iHead->multi.stack)
#define COM_INEXTCNT  (iHead->next.cnt)
#define COM_INEXTSTK  (iHead->next.stack)
#define COM_IAPRM     (&(iHead->prm))
#define COM_IPRMCNT   (iHead->prm.cnt)
#define COM_IPRMLST   (iHead->prm.list)

// デコードI/Fの先頭は必ずこのマクロ
#define COM_DECODER_START \
    do { \
        if( !iHead ) {return;} \
        if( iHead->isFragment ) {return;} \
        if( COM_ISGTYPE == COM_SIG_ALLZERO ) { \
            com_decodeData( iHead, "DATA" ); \
            return; \
        } \
    }while(0)

// デコードI/Fの最後は必ずこのマクロ
#define COM_DECODER_END \
    do { \
        for( long i = 0;  i < COM_INEXTCNT;  i++ ) { \
            com_sigInf_t*  stk = &(COM_INEXTSTK[i]); \
            if( stk->sig.ptype == COM_SIG_CONTINUE ) { \
                com_decodeData( stk, "UNKNOWN " ); \
            } \
        } \
    }while(0)

// バイナリデータのダンプマクロ
#define COM_ASCII_DUMP( SIG ) \
    do { \
        com_printBin_t  flags = { .prefix = "#   ", .seqAscii = ": " }; \
        com_printBinary( (SIG)->top, (SIG)->len, &flags ); \
    }while(0)

/*
 * データ出力(オール0考慮)  com_decodeData()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響は考慮済み。
 * ===========================================================================
 * iLabelで示したデータタイトルとデータ長(=iHead.sig.len)を出力後、
 * iHead->sig.top を先頭にして、iHead->sig.len のサイズまでのバイナリデータを
 * そのまま出力する。
 * 本I/Fは主にプロトコルの特定ができなかったデータ出力での使用を想定する。
 *
 * ただ、データが全て 0x00 だった場合はバイナリ出力はせず
 *   #   << all 0x00 binaries >>
 * と出力し、オール0データのダンプは省略する。
 * com_onlyDispSig()との違いは、このオール0時の処理と、
 * iHead->sig.ptype の内容は出力しないこととなる。
 */
void com_decodeData( COM_DECODER_PRM, const char *iLabel );

/*
 * デコードなし信号データ出力  com_onlyDispSig()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響は考慮済み。
 * ===========================================================================
 * iHead->sig.ptype で示されたプロトコル名とプロトコル値(COM_SIG_～の価)と
 * IhEAD->SIG.LEN で示されたデータ長を出力後、iHead->sig.top を先頭とする
 * バイナリデータを iHead->sig.len のサイズ分、何もデコードはせずに、
 * 数値列として出力する。
 *
 * 主に COM_SIG_～が 6xx となるプロトコルで、そのデコードI/Fで使われる。
 * それ以外の用途で使う分には問題ない。
 */
void com_onlyDispSig( COM_DECODER_PRM );

/*
 * ポインタ存在時のみ出力  com_dispIfExist()
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで影響は考慮済み。
 * ===========================================================================
 * iPrmで指定されたリスト一覧から、iTypeで示したタグ価のデータを検索する。
 * 存在しない場合は、それ以上何もせずにリターンする。
 *
 * 存在する場合、iNameがそのパラメータ名として出力される。
 */
void com_dispIfExist( com_sigPrm_t *iPrm, long iType, const char *iName );

/*
 *
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 *
 */
void com_dispDec( const char *iFormat, ... );

void com_dispVal( const char *iName, long iValue );

enum { COM_NO_PTYPE = -1 };
void com_dispSig( const char *iName, long iCode, const com_sigBin_t *iSig );

ulong com_dispPrm( const char *iName, const void *iTop, com_off iSize );

// ペイロードデータ判定関数プロトタイプ
typedef BOOL(*com_judgeDispPrm_t)( com_bin iTag );

void com_dispPrmList(
        com_sigPrm_t *iPrm, long iTagSize, long iLenSize,
        com_judgeDispPrm_t iFunc );

void com_dispBin(
        const char *iName, const void *iTop, com_off iLength,
        const char *iSep, BOOL iHex );

void com_dispNext( long iProtocol, size_t iSize, long iType );

/*
 *
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 *
 */
long com_getSigType( com_sigInf_t *iInf );

/*
 *
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 *
 */

// 文字列情報データ構造
typedef struct {
    long   code;
    char*  name;
} com_decodeName_t;

// リスト最終データマクロ
#define COM_DECODENAME_END    { -1, NULL }

char *com_searchDecodeName( com_decodeName_t *iList, long iType, BOOL iAddNum );

/*
 *
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 *
 */
void com_decodeTxtBase(
        com_sigInf_t *iHead, long iType, const char **iHdrList,
        const char *iSigLabel, long iSigType, const char *iConType );

/*
 *
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 *
 */
void com_dispAsnPrm(
        const com_sigPrm_t *iPrm, const char *iLabel, long iStart );



/*
 *****************************************************************************
 * COMSIGCOMFRAG: 信号フラグメント処理I/F
 *****************************************************************************
 */

// フラグメント処理結果
typedef enum {
    COM_FRG_ERROR = 0,    // フラグメント処理NG
    COM_FRG_OK    = 1,    // フラグメント不要
    COM_FRG_REASM = 2,    // フラグメントデータ結合完了
    COM_FRG_SEG   = 3,    // フラグメントデータ収集
} COM_SIG_FRG_t;

// 分割条件データ構造 (使い方は com_stockFragments()の説明を参照
typedef struct {
    long          type;    // 自プロトコル種別(COM_SIG_～)
    long          id;      // フラグメント識別子(プロトコル固有)
    com_sigBin_t  src;     // 送信元情報(必要があれば保持)
    com_sigBin_t  dst;     // 送信先情報(必要があれば保持)
    long          usr1;    // ユーザー情報1(プロトコル固有)
    long          usr2;    // ユーザー情報2(プロトコル固有)
} com_sigFrgCond_t;

// フラグメント管理情報データ構造
typedef struct {
    long               isUse;
    com_sigFrgCond_t   cond;
    com_sigFrg_t       data;
} com_sigFrgManage_t;

// 上記の .srcや .dstの内容を指定しやすくするマクロ
#define COM_BIN( ADDR ) \
    (com_bin*)( &(ADDR), sizeof(ADDR) )

/*
 *
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 *
 */
void com_freeSigFrgCond( com_sigFrgCond_t *oTarget );

/*
 *
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 *
 */
com_sigFrg_t *com_stockFragments(
        const com_sigFrgCond_t *iCond, long iSeg, com_sigBin_t *iFrag );

/*
 *
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 *
 */
com_sigFrgManage_t *com_searchFragment( const com_sigFrgCond_t *iCond );

/*
 *
 * ---------------------------------------------------------------------------
 *   エラーは発生しない。
 * ===========================================================================
 *   マルチスレッドで動作することは想定していない。
 * ===========================================================================
 *
 */
void com_freeFragments( const com_sigFrgCond_t *iCond );




// 機能導入フラグは USING_COM_SIGNAL1 に含まれるため、ここでは定義なし

