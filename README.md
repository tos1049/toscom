                     ---------------------------------
                     ### toscom grimoire    by TOS ###
                     ---------------------------------

    これは C言語のコーディングの練習を兼ねて、何年か色々手を加えながら、
    普段何かのコードを書く時に使えそうな機能を関数I/Fとして切り分けて作った
    ライブラリのようなものです。
    また、メモリ解放漏れやファイルのクローズ漏れを監視も出来るようになります。
  
    大量のファイルがあるため、どうしたものかと思われそうですが、
    そこまで面倒な手順は掛けずに使えるようにはしているつもりです。
    一応個人的にサブルーチンとしてまとめておきたいコードはだいぶ書けており
    これを使って簡単なツールを幾つか作って作業量軽減になっています。
  
    ただ、いささか分量的に肥大化したことは否めなく(純コードは20KLぐらい)
    他の人に使えなくなる代物になるのも残念なので、公表を試みています。
    バグが無いわけではないので、何か問題を見つけたら遠慮なく御連絡下さい。
    
    Windows上の Cygwin 及び Linux(Cent OS 4.x～7.x)での動作を確認しています。
    (ただし幾つか導入が必要なパッケージは存在します)
    
    以下の Wikiにて使用方法などをまとめているので必ず御確認下さい。
       https://github.com/tos1049/toscom/wiki


!!!!! 使い方 !!!!!

    アーカイブファイルを展開すると、toscom/ がディレクトリ生成されます。
    toscom/BUILD の下で make を打つことで、コンパイル自体は可能ですが、
    オプション機能を全て含めた最大構成になっているため、そのOS環境下に
    curses系ライブラリがインストールされていない場合、エラーが出ます。

    実際の開発環境を構築する newenv.tar.gz というアーカイブを toscom/下に
    用意しているので、最初からこれを使用することを推奨しています。


    一番簡単な開発環境構築方法は toscom/のディレクトリがある場所で、
      > tar xvfz toscom/newenv.tar.gz
       (MEWEMV/ が展開される)
      > cd NEWENV
      > ./newenv
    とコマンドを投入することです。
    これで基本機能のみの開発環境になります。

    そのまま NEWENVの下の BUILDに入って makeを打てばビルドも可能です。
    NEWENVは開発するプログラムの名前などで、適宜変更する想定です。
    この環境なら、変更をいくらしても toscom/下のファイルは影響を受けません。
    もちろんシンボリックリンクになっているものは修正をしない、という前提です。

    シェルスクリプト newenvの詳しい使い方については、上記に示した Wikiの
    「3.各種シェル」->「開発環境作成シェル」を御覧ください。


    toscomのアーカイブには基本セットの他にオプション機能として
      エキストラ機能  (com_extra.*)    文字入力・乱数・統計計算等
      セレクト機能    (com_select.*)   ソケットを利用した通信
      ウィンドウ機能  (com_window.*)   cursesを利用したウィンドウ描画
    というのが最初から入っています。
    これらの開発環境への取り込み方については、開発環境作成シェルの説明を
    Wikiで御確認下さい。

    オプション機能はいつでも追加/削除が可能です。


    toscomがアップデートされたときも、最初と同じように toscom.*.tar.gzを展開
    する必要があります。この際 toscom/下の main.c を変更していたら、それは上書き
    されてしまいますが、toscom/newenv.tar.gz を使って、別途 開発環境を作り、
    そちらの main.cを変更していたなら、上書きされる心配はなくなります。

    「ほぼ反映される」と書いたのは、そうでない部分が入る可能性があるから
    なのですが、それについては 開発環境作成シェルの説明を Wikiで御確認下さい。


!!!!! あとは作ってみるのみ !!!!!

    toscomの処理については com_ で始まる関数として定義しています。
    構造体などの型名も com_ で始まるトークンで宣言しています。
    マクロや共用体などは COM_ で始まるトークンで宣言しています。

    どんなI/Fが用意されているかは、toscom/aboutIf.txt で一覧しています。
    アップデートによるI/Fの追加/変更/削除についての情報も併記しています。
    個々の説明は com_if.h 等のヘッダファイルに記載しています。


    まずは main.cの main()で自分独自の処理関数を呼べば良いようにしているので
    お試しなら main()内に、例えば以下のように書いてみるとよいでしょう。
        com_printf( "Hello, World\n" );
    ビルドして(makeを打って) 生成された ./test を実行したら画面出力があり、
    .test.log というのも出来ていて、画面出力をロギングされています。
    com_printf()など toscomの画面出力I/Fは 画面出力をすると同時に、
    ログファイルに書き込むことも処理として最初から入っています。

    ./test を何度も実行すると .test.log.* がどんどん生成されます。
    ./test -c と打つと .test.log 以外の履歴ログは全て削除できます。
    これは main.cの main()で呼んでいる com_getOption()の働きです。


    試しに main()に以下のように記述して(メモリ捕捉だけして、わざと解放しない)
        char* memtest = com_malloc( 30, "malloc test" );
        memset( memtest, 0xff, 30 );  // 変数の未使用警告対策
    ビルドして実行したら「not freed memory list」というのが出て、
    解放されずに浮いたままのメモリをプログラム終了時に出力します。
        com_free( memtest );
    と更にその後ろに足せば、何も言われなくなります。

    これが toscomのメモリ監視機能です。
    com_malloc()や com_free()を使うと、メモリ監視機能は動作します。
    メモリの確保には com_realloc()等、幾つかバリエーションが存在します。
    逆に標準関数の malloc() や free()ではこの機能は動作しません。


    試しに main()に以下のように記述して(オープンだけして、わざとクローズしない)
        FILE* fp = com_fopen( "makefile", "r" );
        fgetc( fp );    // 変数fpの未使用警告対策
    プログラムを終了時に「not closed file list」というのが出ます。
        com_fclose( fp );
    と更にその後ろに足せば、何も言われなくなります。

    これが toscomのファイル監視機能です。
    com_fopen()や com_fclose()を使うと、ファイル監視機能は動作します。
    逆に標準関数の fopen()や fclose()ではこの機能は動作しません。


!!!!! grimoire？ !!!!!

    grimoire は「グリモア」とか「グリモワール」などと読まれています。
    「魔術書」を意味するフランス語です。
    ウィザードと呼ばれる人たちが、よく手に持っている道具です。

    この場合の「ウィザード」は、優れたコンピュータ技術者を指す言葉として
    捉えていただき、その作業のための道具になれば、という思いを込めて
    「toscom griomire」というのを、このライブラリもどきの名前としています。

    ソースに書かれている関数は呪文で、それを組み合わせて魔術を構成する・・
    なんて考えると、プログラムも楽しくなるかなと。

