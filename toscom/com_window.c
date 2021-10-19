/*
 *****************************************************************************
 *
 * 共通部ウィンドウ機能    by TOS
 *
 *   外部公開I/Fの使用方法については com_window.h を必読。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"
#include "com_window.h"







// 初期化処理 ----------------------------------------------------------------

static com_dbgErrName_t  gErrorNameWindow[] = {
    { COM_ERR_WINDOWNG,     "COM_ERR_WINDOWNG" },
    { COM_ERR_END,          "" }  // 最後は必ずこれで
};

static void finalizeWindow( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

void com_initializeWindow( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    atexit( finalizeWindow );
    com_registerErrorCode( gErrorNameWindow );
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

