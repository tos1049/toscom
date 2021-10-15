/*
 *****************************************************************************
 *
 * 共通モジュール用 カスタマイズ宣言ヘッダ by TOS
 *
 *  com_if.h で インクルードされるため、基本的に全ソース共通となる。
 *  それ以外のソースやヘッダで、本ファイルをインクルードしないこと。
 *
 *****************************************************************************
 */

#pragma once

/* toscom動作のカスタマイズ宣言 --------------------------------------------*/

/*
 * ブーリアン値については、C99標準の bool型値 true/false を使用する。
 * しかし bool型はサイズが 1 のため、構造体のメンバーとしては非常に使いづらい。
 * (toscomは構造体メンバーでパディングが発生したら警告する設定でコンパイルする)
 * そこで long型の BOOL という型を敢えて typedefしておく。
 * これは「long型だが、true/falseを入れることを期待する」という意味になる。
 *
 * 開発環境によって、ブーリアン値は独自宣言したくなるかもしれないので、
 * com_spec.h にて宣言を記述する。ただい BOOL型は以下の条件に合致するように宣言
 * しなければ、toscomが正常にビルド/動作できなくなる。
 * ・偽は必ず 0、真は 0以外の値とする (これはC言語の仕様でもある)
 * ・32bit OSの場合は 32bit、64bit OSの場合は 64bitのサイズにする
 * 例えば typedef enum { FALSE, TRUE } BOOL; と宣言することは 32bit OSなら可能。
 * しかし列挙体は int型＝32bitのため、64bit OSでこの宣言にした場合、toscomは
 * ビルド時に「構造体定義でパディング発生」の警告が多発し、データの使い方次第で
 * 正しく動作もしなくなるだろう。
 */
typedef   long    BOOL;

/*
 * 以降は、変更の前に宣言を利用するI/Fの説明記述を確認すること。
 * 最大/最小の定義であれば、1未満の値の設定は基本的に危険と考えること。
 */

// com_searchString()で指定がない場合の検索限界長
enum {
    COM_SEARCH_LIMIT = 4096
};

// com_getString()で使用する文字列バッファの保持数
enum {
    COM_FORMSTRING_MAX = 5
};

//com_createThread()で参照する子プロセス最大生成数
enum {
    COM_THREAD_MAX = 16
};

// デバッグ監視 情報ラベルサイズ (8の倍数にすること)
enum {
    COM_DEBUGINFO_LABEL = 64
};

// 関数呼出トレース 情報最大数
enum {
    COM_FUNCTRACE_MAX = 1000
};

// 関数名解析用のネームリストファイル名
//  実行用バイナリのあるディレクトリで
//    nm -nl 実行用ファイル名 > 以下で宣言するファイル名
//  のコマンドによりファイルを作っておく必要がある。
//  添付の makefile使用時は、-DUSE_FUNCTRACE を指定してビルドすると自動で作る。
//  下記ファイル名を変更した場合、makefileの方の変数 NAMELIST も併せて修正する。
#define COM_NAMELIST  ".namelist.toscom"

