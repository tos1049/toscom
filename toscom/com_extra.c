/*
 *****************************************************************************
 *
 * 共通処理エキストラ機能    by TOS
 *
 *   外部公開I/Fの使用方法については com_extra.h を必読。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"
#include "com_extra.h"




// 初期化/終了処理 ///////////////////////////////////////////////////////////

static com_dbgErrName_t gErrorNameExtra[] = {
    { COM_ERR_READTXTNG,   "COM_ERR_READTXTNG" },
    { COM_ERR_SIGNALING,   "COM_ERR_SIGNALING" },
    { COM_ERR_END,         "" }  // 最後は必ずこれで
};

static void finalizeExtra( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

void com_initializeExtra( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    atexit( finalizeExtra );
    com_registerErrorCode( gErrorNameExtra );
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

