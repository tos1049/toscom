/*
 *****************************************************************************
 *
 * 共通処理セレクト機能    by TOS
 *
 *   外部公開I/Fの使用方法については com_select.h を必読。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"
#include "com_select.h"

#ifdef LINUXOS
#include <linux/rtnetlink.h>
#include <net/if_arp.h>
#endif





// 初期化処理 ----------------------------------------------------------------

static com_dbgErrName_t gErrorNameSelect[] = {
    { COM_ERR_SOCKETNG,     "COM_ERR_SOCKETNG" },
    { COM_ERR_BINDNG,       "COM_ERR_BINDNG" },
    { COM_ERR_CONNECTNG,    "COM_ERR_CONNECTNG" },
    { COM_ERR_LISTENNG,     "COM_ERR_LISTENNG" },
    { COM_ERR_ACCEPTNG,     "COM_ERR_ACCEPTNG" },
    { COM_ERR_SENDNG,       "COM_ERR_SENDNG" },
    { COM_ERR_RECVNG,       "COM_ERR_RECVNG" },
    { COM_ERR_SELECTNG,     "COM_ERR_SELECTNG" },
    { COM_ERR_CLOSENG,      "COM_ERR_CLOSENG" },
    { COM_ERR_GETIFINFO,    "COM_ERR_GETIFINFO" },
    { COM_ERR_END,          "" }  // 最後は必ずこれで
};

static void finalizeSelect( void ) {
    COM_DEBUG_AVOID_START( COM_NO_SKIPMEM );
    com_skipMemInfo( true );
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

void com_initializeSelect( void ) {
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    atexit( finalizeSelect );
    com_registerErrorCode( gErrorNameSelect );
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

