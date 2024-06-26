/*
 *****************************************************************************
 *
 * 共通部シグナル機能 (プロトコルセット3)    by TOS
 *
 *   外部公開I/Fの使用方法については com_signalSet3.h を参照。
 *
 *****************************************************************************
 */

#include "com_if.h"
#include "com_debug.h"
#include "com_signal.h"
#include "com_signalSet3.h"


void com_initializeSigSet3( void )
{
    COM_DEBUG_AVOID_START( COM_PROC_ALL );
    com_setInitStage( COM_INIT_STAGE_PROCCESSING, false );
    // まずは箱だけ
    com_setInitStage( COM_INIT_STAGE_FINISHED, false );
    COM_DEBUG_AVOID_END( COM_PROC_ALL );
}

