/*
 *****************************************************************************
 *
 * comモジュール  個別I/F定義  by TOS
 *
 *   comモジュールの処理の中でも、アプリごとに個別の定義が必要なものを、
 *   このソースに追加する。もちろんさらに別のソースとしても問題はない。
 *   本ファイル内の関数プロトタイプ等は com_spec.h に記載すること。
 *
 *   com_spec.h自体は com_if.hでインクルードしているため、インクルード文を
 *   さらに記述する必要はない。com_if.hは全ソースで最初にインクルードすることが
 *   前提となっているため、どのソースでも本ソースのI/Fが使えることになる。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"

/* サンプル個別処理 *********************************************************/

static BOOL checkEndian( void )
{
    // この判定方法は C言語FAQ 20.9 に記載されている
    int x = 1;
    if( *(char*)&x == 1 ) { return false; }  // リトルエンディアン
    return true;  // ビッグエンディアン
}

BOOL com_isBigEndian( void )
{
    return checkEndian();
}

BOOL com_isLittleEndian( void )
{
    return !checkEndian();
}

BOOL com_is32bitOS( void )
{
    return (sizeof(long) == sizeof(int));
}

BOOL com_is64bitOS( void )
{
    return (sizeof(long) != sizeof(int));
}



/* 個別エラー定義 ***********************************************************/

// com_spec.hのエラー定義を変更したら、こちらも連動で変更する
static com_dbgErrName_t gErrorNameSpec[] = {
    { COM_ERR_DUMMY,        "COM_ERR_DUMMY" },  // これは自由に削除可能

    { COM_ERR_END, "" }  // 最後は必ずこれで(絶対削除しないこと)
};

void com_registerErrorCodeSpec( void )
{
    com_registerErrorCode( gErrorNameSpec );   // 本行は削除禁止
}



/* 個別初期化処理 ***********************************************************/

// com_spec.hのプロトタイプ宣言に付したI/F説明をよく読んでから変更すること
void com_initializeSpec( void )
{
}



/* 個別終了処理 *************************************************************/

void com_finalizeSpec( void )
{
}

