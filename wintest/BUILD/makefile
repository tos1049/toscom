#
# makefile サンプル
#   makeのみ       ->開発中のコンパイル(ついでにタグ生成も)
#   make clean     ->ビルド生成物のクリア
#   make logclean  ->ログやcoreファイルなどをクリア(speccleanも実施)
#   make specclean ->プログラム個別の生成物をクリア
#   make allclean  ->生成物やログをすべてクリア(cleanと logcleanを実施)
#   make init      ->make cleanしてからの全コンパイル
#   make checkf    ->書式文字列チェック (詳細は com_if.hの com_printf()参照)
#   make rel       ->リリース物件のビルド(make initでコンパイルする)
#   make tags      ->タグ生成のみ
# というイメージで作ったサンプル
#

# 実行用ファイル名とバージョン設定
COMMAND := wintest
VERSION := 1.0





# コンパイラ指定(gcc以外での動作は未保証)
CC := gcc

# C99を標準としたコンパイルとする
# コンパイルオプション切り替えは、こちらを変更して実施すること(以後 追加あり)
CFLAGS := -std=c99 -g3


# 緩やかチェック (最低限これで警告は出ないようにすること)
# -pedantic
#   gcc拡張構文は無効にして警告
# -Wall
#   一般的にダメそうな記述に対して警告
# -Wextra
#   -Wallよりもう少し厳し目にチェックをする
# -Wshadow
#   変数名がシステム等と被っている場合に警告
# -Wdisabled-optimization
#   コードが長過ぎたり複雑過ぎたりして最適化出来ない場合に警告
# -Wfloat-equal
#   浮動小数値を == や != で比較している場合に警告
# -Wold-style-definitino
#   K&Rスタイルの関数定義に対して警告
# -Wstrict-prototypes
#   引数の型が示されていない(＝K&Rスタイルの)プロトタイプ宣言に対して警告
#
CFLAGS += -pedantic -Wall -Wextra -Wshadow -Wdisabled-optimization \
          -Wfloat-equal -Wold-style-definition -Wstrict-prototypes

# 追加の厳し目チェック (これで警告が出るコードは基本的に避けたほうが良い)
# -Waggregate-return
#   関数が配列や構造体を返すときに警告
# -Wbad-function-cast
#   関数呼び出しがマッチしない型にキャストされている場合に警告
# -Wcast-align
#   アライメントが変わるようなポインタのキャストに警告
# -Winline
#   inline指定関数が、長過ぎる等の理由でインライン展開しなかった場合に警告
# -Wlogical-op
#   論理演算子の間違っているかもしれない使用に対して警告
# -Wmultichar
#   複数の文字を含む文字リテラルに対して警告
# -Wpadded
#   構造体定義でパディングが挿入されたら警告
# -Wpointer-arith
#   関数型や void型に対する sizeofの使用に警告(gccは独自に 1を返すが)
# -Wsequence-point
#   C言語で規定されていない計算の順序などで問題が起こりそうなら警告
#
CFLAGS += -Waggregate-return -Wbad-function-cast -Wcast-align -Winline \
          -Wlogical-op -Wmultichar -Wpadded -Wpointer-arith -Wsequence-point

# 関数の未使用引数を警告されるのが鬱陶しい？
#    警告オプション -Wno-unused-parameter を追加する前に
#    com_if.h で宣言しているマクロ COM_UNUSED() の使用を検討して欲しい


# 基本機能で使うライブラリ指定
CFLAGS += -lpthread
# com_extra.h(エキストラ機能)を使用する場合、このライブラリ使用が必要
CFLAGS += -lm
# com_window.h(ウィンドウ機能）を使用する場合、このライブラリ使用が必要
CFLAGS += -lpanel -lncursesw
# Electric Fenceを使ったデバッグがしたい時のみ入れるオプション
#CFLAGS += -lefence

# 最新gccではセットされているコンパイルオプション
CFLAGS += -Wformat-security




LDFLAGS := -r

RELOPT := -DAPLNAME=\"$(COMMAND)\" -DVERSION=\"$(VERSION)\"
RELOPT += -D_BSD_SOURCE
RELOPT += -D_DEFAULT_SOURCE
RELOPT += -D_POSIX_C_SOURCE=200809L










# リリース用コンパイルオプションで初期化 (デバッグオプションを更に以降で追加)
OPTION := $(RELOPT)

##############################################################################
################# デバッグオプション(必要に応じて設定を変更) #################
##############################################################################
# デバッグログ作成 (コメントアウトすると、デバッグログを作成しない)
OPTION += -DUSEDEBUGLOG
# テスト関数使用 (コメントアウトすると、ビルド対象からも外れる)
OPTION += -DUSE_TESTFUNC
# 関数呼出トレース使用 (必要な時のみコメントを外す)
#OPTION += -DUSE_FUNCTRACE -D_GNU_SOURCE -fPIC -finstrument-functions -ldl
#
##############################################################################
############ デバッグモードスイッチ(未指定時は COM_DEBUG_OFF扱い) ############
##############################################################################
# デバッグ出力 (COM_DEBUG_OFF/COM_DEBUG_ON/COM_DEBUG_SILENT:ファイル出力のみ)
OPTION += -DDEBUGPRINT=COM_DEBUG_SILENT
# メモリ監視   (COM_DEBUG_OFF/COM_DEBUG_ON/COM_DEBUG_SILENT:最後に浮き出力)
OPTION += -DWATCHMEM=COM_DEBUG_SILENT
# ファイル監視 (COM_DEBUG_OFF/COM_DEBUG_ON/COM_DEBUG_SILENT:最後に浮き出力)
OPTION += -DWATCHFILE=COM_DEBUG_SILENT
##############################################################################
#
# COMモジュールのメモリ監視省略を解除 (浮き確認等デバッグで必要な時のみ使用)
#OPTION += -DNOTSKIP_MEMWATCH
# COMモジュールのファイル監視省略を解除 (浮き確認等デバッグで必要な時のみ使用)
#OPTION += -DNOTSKIP_FILEWATCH
##############################################################################









# ソース/ヘッダは全て同じディレクトリにある想定
SRC := ..

# 共通ヘッダファイル (依存関係の記述に使用)
COMMON_HEAD := $(SRC)/com_*.h

# オブジェクトファイル一式
OBJS = main.o com.o win.o

# ネームリストファイル名 (com_custom.hの COM_NAMELISTと同じ内容にすること)
NAMELIST = .namelist.toscom







########## コンパイル時はタグファイルも自動生成する ##########
$(COMMAND): tags $(OBJS)
	$(CC) $(OPTION) $(OBJS) -o $(CHECKF)$@ $(CFLAGS)
ifneq (,$(findstring USE_FUNCTRACE, $(OPTION)))
	nm -nl $(CHECKF)$(COMMAND) > $(NAMELIST)
endif

main.o: $(SRC)/main.c $(COMMON_HEAD)
	$(CC) $(OPTION) $< -c -o $@ $(CFLAGS)

com.o: $(SRC)/com_*.c $(SRC)/com_*.h
	$(CC) $(OPTION) -c $(wildcard $(SRC)/com_*.c) $(CFLAGS)
	$(LD) $(LDFLAGS) com_*.o -o $@

# 新たに別モジュールを追加したい時は、smpl.oと同じように記述する。
# (smpl.o は初期状態では OBJS に入っていないため、makeを打っても処理されない)
#
# つまり smpl となっている５ヶ所を、そのモジュール名の文字列に置換する。
# そのモジュールのファイルが モジュール名_*.c/h であることが前提となる。
# ＊依存ファイルに 自モジュールのファイルだけでなく $(COMMON_HEAD)を
#   含めることにより、共通処理I/Fの変更を逐一反映出来るように工夫している。
#
# OBJS に オブジェクト名 (モジュール名.o) を追記することも必要。
# またタグ生成の為、tagsにも モジュール名_*.? を追加するとよいだろう。

win.o: $(SRC)/win_*.c $(SRC)/win_*.h $(COMMON_HEAD)
	$(CC) $(OPTION) -c $(wildcard $(SRC)/win_*.c) $(CFLAGS)
	$(LD) $(LDFLAGS) win_*.o -o $@














########## 書式文字列チェックコンパイル (make checkf) ########################
# com_printf()の書式出力パラメータは printf()のようなチェックをコンパイラが
# 実施しないため、正しくない記述でも警告/エラー無しとなってしまう。
# そこで com_printf を printf に変換してコンパイルを実施できるようにする。
# 書式指定に問題がある場合は警告が出て -Werror指定することでそれをエラーと
# して結果を出す。定期的にこのビルドでチェックをすることを推奨する。
# make checkf により生成されるバイナリは CHECKF_PRE に定義された文字列を
# ファイル名の前に付与し、実行属性を落としたものになる。
#
CHECKF_PRE := _CHECKF_

checkf: CFLAGS += -Werror
checkf: CFLAGS += -DCHECK_PRINT_FORMAT
checkf: CHECKF := $(CHECKF_PRE)
checkf: init
	chmod 666 $(CHECKF)$(COMMAND)







########## リリース物件生成 (make rel) #######################################
# リリース時はデバッグオプションを落とし、
# ・CFLAGS: -Werror -Wmissing-declarations -O2
# ・OPTIONS: -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3
# を追加する。
#
# -Werrorとすることで警告を全てエラーとしてコンパイル中断する。どうしても
# 仕方なく警告が出るという場合は、オプションを落とすことも視野に入れる。
#
# -Wmissing-declarations は未使用関数でぷるとタイプ宣言がないものを警告する。
# 不要なら関数を削除するかコメントアウトするべきだし、実際の関数名と不整合な
# プロトタイプ宣言があるかもしれないのでチェックをした方が良い。
# (com_testtos.cはテスト関数がこの警告で引っかかるので削除を勧める)
#
# -O2は最適化オプションとなる。このオプションが無ければ
# -Wmissing-declarations や -D_FORTIFY_SOURCE が使えないので注意すること。
#
# -D_FORTIFY_SOURCE=3は特定の標準関数使用時にバッファオーバーフローを検出する
# 軽めのチェックを実施する。コンパイル時のチェックでは警告が出るだけだが、
# 実行時もチェックが行われているらしく、実行時のチェックに引っかかった場合
# 実行エラーになる模様。予想外の挙動の原因となり得るが、基本的にバグであり、
# そうしたエラーとならないようにコードを書くべきである。
#
# ちなみに関数内で初期化漏れ変数を使用しているという警告も make rel時のみ出る。
# これはそのチェックのために最適化オプションで作られる内部データが必要な為で
# そうしたこともあり、開発中も時折 make rel を打つことをお勧めする。
#
# OPTION からはデバッグオプション設定を全て外す(RELOPTの内容のみになる)ので
# 例えばデバッグログ生成が必要ならば -DUSEDEBUGLOG を追加すると良いだろう。
#
# ビルド後、リリース用のアーカイブファイルを生成するように以下は記述している。
# RELDIRで設定したディレクトリにファイルをコピーしてアーカイブする。
# ディレクトリ名やファイル名を変更したい場合や、そもそものファイル作成作業自体を
# 変更したい場合は、以下の記述を適宜修正すること。clean等、他のターゲットにも
# 影響がありそうならば、そちらの修正も必要になるだろう。
#
RELEASE := RELEASE
RELDIR := $(RELEASE)_$(COMMAND)_$(VERSION)

rel: CFLAGS += -Werror
rel: CFLAGS += -Wmissing-declarations -O2
rel: OPTION := $(RELOPT) -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3

rel: init
	mkdir $(RELDIR)
	cp $(COMMAND) $(RELDIR)
	tar cvfz $(COMMAND)_$(VERSION).tar.gz $(RELDIR)



########## ライブラリ作成 ####################################################
#
# toscomをライブラリ化する。＊まだ試験実装のためアーカイブ生成のみ＊
# 生成されるアーカイブファイルは
#   lib/libtoscom.a    (実際のライブラリ)
#   include/*.h        (ヘッダファイル 兼 説明書
# となる。
#
# 他ソースと混ぜる場合のイメージは、ビルドする際、
# 他ソースファイルとともに上記のライブラリファイル指定するというもの。
# -L でライブラリがあるパスを指定後、-ltoscom を指定でも良い。
#
# toscom使用時は、include/下にあるヘッダファイルをインクルードする。
# -I でヘッダファイルのあるパスを指定すると、インクルード時に指定するのが
# ヘッダファイル名のみで良くなる。
#
LIBDIR :=  libtoscom
LIBFILE := libtoscom
LIB := .lib

lib: rel
	ar rcs $(LIBFILE).a         com_proc*.o com_debug.o
	ar rcs ${LIBFILE}_extra.a   com_extra.o
	ar rcs ${LIBFILE}_select.a  com_select.o
	ar rcs ${LIBFILE}_window.a  com_window.o
	ar rcs ${LIBFILE}_signal.a  com_signal*.o
	mkdir $(LIBDIR)
	mkdir $(LIBDIR)/lib
	cp $(LIBFILE)*.a $(LIBDIR)/lib
	mkdir $(LIBDIR)/include
	$(LIB)/collectLibFiles .. $(LIBDIR)/include
	mkdir ${LIBDIR}/files
	cp $(LIB)/gcc_sample $(LIBDIR)/files
	cp ../com_spec.* ${LIBDIR}/files
	cp ../com_test*.c ${LIBDIR}/files
	cp ../main.c     ${LIBDIR}/files
	cp ${LIB}/ready  ${LIBDIR}
	tar cvfz ${LIBDIR}.tar.gz $(LIBDIR)




########## まっさらテストコンパイル (make init) ##############################
init: clean $(COMMAND)

########## ビルドによる直接的生成物の削除 (make clean) #######################
clean:
	$(RM) -fr $(COMMAND) $(COMMAND).exe $(CHECKF_PRE)$(COMMAND)
	$(RM) -fr *.o ../tags $(RELEASE)* $(COMMAND)*.tar.gz $(NAMELIST)
	$(RM) -fr *.a ${LIBDIR}*

########## ログファイルやコアファイルを削除 (make logclean) ##################
logclean: specclean
	$(RM) -fr .$(COMMAND)* core.* *.stackdump

########## プログラム固有のファイルを削除 (make specclean) ###################
# 何か削除する必要のあるものがあれば、削除文をここに追記する。
specclean:

########## 生成物やログを全て削除 (make allclean) ############################
allclean: clean logclean specclean
	$(RM) -fr ../NEWENV/*.tar.gz






########## タグ生成 (make tags) ##############################################
tags:
	cd ..; ctags main*.? com_*.? win_*.?

