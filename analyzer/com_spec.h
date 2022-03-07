/*
 *****************************************************************************
 *
 * 共通モジュール用 独自I/F宣言 by TOS
 *
 *  com_if.h でインクルードされるため、基本的に全ソース共通となる。
 *  それ以外のソースやヘッダで、本ファイルをインクルードしないこと。
 *  com_spec.c で定義する関数について、本ファイルに記載する。
 *
 *****************************************************************************
 */

#pragma once

/* 独自インクルードファイル ------------------------------------------------*/




/* 基本I/f -----------------------------------------------------------------*/

/* 個別エラーコード  ＊こちらを変えたら、連動テーブル gErrorNameSpec[] も直す */
enum {
    /* 開発するプログラムごとに自由に設定可能なエラーコード (001-899) */
    COM_ERR_DUMMY      =   1,   // これはサンプルなので削除変更OK
};

/*
 * 個別エラーコード設定I/F
 *  COMモジュール内部で呼び出すだけのため、特に触れる必要はない。
 *  使う側は上記のエラーコード宣言と gErrorNameSpec[]を変更するのみで良い
 */
void com_registerErrorCodeSpec( void );

/*
 * 個別部初期化  com_initializeSpec()
 * ---------------------------------------------------------------------------
 *   現時点ではエラーは発生しない。(処理を修正した場合は、この内容も修正)
 * ===========================================================================
 * この処理は comモジュールの一番最初の処理で動作するようになっているため、
 * デバッグモードなど最初に変更したいものについて、記述できる。
 * 起動オプションを見たい場合は、com_getCommandLine()で取得すれば良い。
 * (このI/Fは com_sepc.hで宣言されている)
 *
 * このI/Fはロギングやデバッグ機能の動作開始前に呼ばれる。
 * このため、com_if.hの COMPRINT・COMDEBUGセクションの画面/ログ出力を伴いI/Fは
 * 使用できない(例えば、com_printf()・com_debug()・com_error()等)
 * これらのI/Fは他I/Fからも多数呼ばれるため、com_if.h内の以下I/F以外は
 * 使わないようにしたほうが良い。
 *   com_getAplName()・com_getVersion()・com_getCommandLine()
 *   com_setLogFile()・com_setTitleForm()・com_setErrorCodeForm()
 *   com_setDebugPrint()
 *   com_setWatchMemInfo()
 *   com_debugMemoryErrorOn()・com_debugMemoryErrorOff()
 *   com_setWatchFileInfo()
 *   com_debugFopenErrorOn()・com_debugFopenErrorOff()
 *   com_debugFcloseErrorOn()・com_debugFcloseErrorOff()
 *   com_noComDebugLog()
 * これらのI/Fは主に本I/Fは内で使用することを想定したものが多い。
 *
 * 画面出力が必要な場合、標準関数 printf()を使用すること(ログ出力は不可)。
 * プログラム終了が必要な場合は、exit()を使用すること。
 *
 * もちろん独自処理に必要な初期化処理があるなら、本I/Fに記述すべきである。
 *
 * com_malloc()のようなメモリ確保が必要な初期設定をしたい場合、
 * 本I/F内ではそうしたI/Fは使用不可能なので、初期設定用の関数を別途用意し、
 * com_initialize()の後に呼ぶようにするか、推奨はしないが、本I/F内で 直接
 * malloc()などの標準関数を使用する。その場合、デバッグ機能によるメモリ浮きの
 * チェックはできなくなる。
 */
void com_initializeSpec( void );

/*
 * 個別部終了  com_finalizeSpec()
 * ---------------------------------------------------------------------------
 *   現時点ではエラーは発生しない。(処理を修正した場合は、この内容も修正)
 * ===========================================================================
 * 個別部処理の中で、プログラム終了時に片付けが必要な処理があれば記述できる。
 * このI/Fは com_finalize()の中で、一番最初に実行される。
 */
void com_finalizeSpec( void );


/* 独自I/F -----------------------------------------------------------------*/

/*
 * (サンプル) エンディアン判定  com_isBigEndian()・com_isLittleEndian()
 *   チェック成否を true/falseで返す。
 * ---------------------------------------------------------------------------
 *   現時点ではエラーは発生しない。(処理を修正した場合は、この内容も修正)
 * ===========================================================================
 * 自身の処理系のエンディアンをチェックし、その結果を返す。
 * ビッグエンディアンでなければ、自動的にリトルエンディアンと言えるだろう。
 * その逆も同様となる。
 *
 * 本I/Fはあくまで個別処理記述のサンプルなので、より適切なコードがあるならば
 * そのコードに書き換えたほうが良いだろう。
 */
BOOL com_isBigEndian( void );
BOOL com_isLittleEndian( void );

/*
 * (サンプル) 32bit/64bit OS判定  com_is32bitOS()・com_is64bitOS()
 *   チェック成否を true/falseで返す。
 * ---------------------------------------------------------------------------
 *   現時点ではエラーは発生しない。(処理を修正した場合は、この内容も修正)
 * ===========================================================================
 * 自身の OSが 32bit または 64bit かどうかをチェックし、その結果を返す。
 * 今のところ、この2つは対であり、falseであれば他方であると判断できる。
 * (com_is32bitOS()が falseを返した＝64bit OS、と言えるということである)
 *
 * 判定型には long型と int型のサイズが等しければ 32bit、異なれば 64bit、
 * という論理を使用している。
 *
 * 本I/Fはあくまで個別処理記述のサンプルなので、より適切なコードがあるならば
 * そのコードに書き換えたほうが良いだろう。
 */
BOOL com_is32bitOS( void );
BOOL com_is64bitOS( void );

/*
 * (サンプル) 動作環境取得  com_getEnvName()
 *   OS種別を返す。
 *   未サポートOSの場合 COM_OS_NOT_SUPPORTED を返す。
 * ---------------------------------------------------------------------------
 *   現時点ではエラーは発生しない。(処理を修正した場合は、この内容も修正)
 * ===========================================================================
 * コンパイル時の実行環境を示すマクロから、現在のOSが何かを返す。
 * 現状対応しているのは、Linux と Cygwin のみ。
 */
typedef enum {
    COM_OS_NOT_SUPPORTED = 0,
    COM_OS_LINUX  = 1,
    COM_OS_CYGWIN = 2
} COM_OS_TYPE_t;

COM_OS_TYPE_t com_getEnvName( void );
