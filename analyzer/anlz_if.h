/*
 *****************************************************************************
 *
 * 簡易信号アナライザー Analyzer 外部公開ヘッダ
 *
 *****************************************************************************
 */

#pragma once

#include "com_if.h"
#include "com_extra.h"
#include "com_signal.h"
#include "com_signalSet2.h"
#include "com_signalSet3.h"

/* anlzモジュールの初期化処理 */
void anlz_initialize( void );

/* 解析処理の起点I/F */
void anlz_start( int iArgc, char **iArgv );

