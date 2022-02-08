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

    (最新の情報は、上記の Wikiで御確認を御願いします)

    アーカイブファイルを展開すると、toscom/ がディレクトリ生成されます。
    実際のビルドは BUILD/に移動し、makeを打つことで実行できます。
      > cd toscom/BUILD
      > make
    これにより test (Cygwinの場合は test.exe)が生成されます。

    また curses系のライブラリがインストールされていない場合、エラーになります。
    別ディレクトリに開発環境を構築するための newenv.tar.gz を用意しているので
    これを利用して最小構成でのビルドを試みて下さい。

    最小構成での開発環境構築方法は toscom/のディレクトリがある場所で、
      > tar xvfz toscom/newenv.tar.gz
      > cd NEWENV
      > ./newenv
    とコマンドを投入することです。
    そのまま NEWENVの下の BUILDに入って makeを打てばビルドも可能です。
    この環境なら、変更をいくらしても toscom/下のファイルは影響を受けません。
    もちろんシンボリックリンクになっているものは修正をしない、という前提です。

    シェルスクリプト newenvの詳しい使い方については、上記に示した Wikiの
    「3.各種シェル」->「開発環境作成シェル」を御覧ください。
    (newenvを viなどで開くと、冒頭コメントでも説明記述していますが、
     最新の情報は Wikiの方になります)

    toscomがアップデートされたときも、最初と同じように toscom.*.tar.gzを展開
    するだけで、ほぼ反映される、という仕掛けです。main.cに変更があっても、
    開発環境を作ってあれば、上書きされる心配もありません。 (toscom/下で
    main.cを変えていても、アーカイブを展開した時に上書きされてしまいます)
    「ほぼ反映される」と書いているのは、そうでない部分が入る可能性があるから
    なのですが、それについては 開発環境作成シェルの説明を Wikiで御確認下さい。

    toscomのアーカイブには基本セットの他にオプション機能として
      エキストラ機能  (com_extra.*)    文字入力・乱数・統計計算等
      セレクト機能    (com_select.*)   ソケットを利用した通信
      ウィンドウ機能  (com_window.*)   cursesを利用したウィンドウ描画
    というのが最初から入っています。
    これらの開発環境への取り込み方については、開発環境作成シェルの説明を
    Wikiで御確認下さい。オプション機能は後から追加/削除が可能です。


!!!!! あとは作ってみて下さい !!!!!

    あとは main.cの main()で自分独自の処理関数を呼べば、好きな処理が作れます。
    お試しなら main()内で適当に書いてみても良いでしょう。例えば・・
        com_printf( "Hello, World\n" );
    などなど。
    ビルドして(makeを打って) 生成された ./test を実行したら画面出力があり、
    .test.log というのも出来ていて、画面出力をロギングしているはずです。

    ./test を何度も実行すると .test.log.* がどんどん生成されます。
    ./test -c と打つと .test.log 以外の履歴ログは全て削除できます。
    これは main.cの main()で呼んでいる com_getOption()の働きです。

    以下のような記述をすると(メモリ捕捉だけして、わざと解放しない)
        char* z = com_malloc( 30, "malloc test" );
        memset( z, 0xff, 30 );    // 変数zの未使用警告対策
    ビルドして実行したら「not freed memory list」というのが出て、
    解放されずに浮いたままのメモリをプログラム終了時に出力します。
    com_free( z ); と文言を更にその後ろに足せば、何も言われなくなります。
    これが toscomのメモリ監視機能です。

    以下のような記述をすると(ファイルオープンだけして、わざとクローズしない)
        FILE* fp = com_fopen( "makefile", "r" );
        fgetc( fp );    // 変数fpの未使用警告対策
    プログラムを終了時に「not closed file list」というのが出ます。
    com_fclose( fp ); と更にその後ろに足せば、何も言われなくなります。
    これが toscomのファイル監視機能です。


!!!!! grimoire？ !!!!!

    grimoire は「グリモア」とか「グリモワール」などと読まれています。
    「魔術書」を意味するフランス語です。
    ウィザードと呼ばれる人たちが、よく手に持っている道具です。

    この場合の「ウィザード」は、優れたコンピュータ技術者を指す言葉として
    捉えていただき、その作業のための道具になれば、という思いを込めて
    「toscom griomire」というのを、このライブラリもどきの名前としています。

    ソースに書かれている関数は呪文で、それを組み合わせて魔術を構成する・・
    なんて考えると、プログラムも楽しくなるかなと。

