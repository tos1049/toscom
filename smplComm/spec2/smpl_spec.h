/*
 *****************************************************************************
 *
 * サンプルコミュニケーター  個別ヘッダ (ver2)
 *
 *   Ver1とは内容が異なるので注意。
 *   Ver1と Ver2のどちらにも存在するI/Fは smpl_com.h に記載する。
 *
 *****************************************************************************
 */

#pragma once

#include "com_window.h"


/*
 * ログウィンドウへの文字出力  smpl_printLog()
 *   iAttrには出力時に設定する属性値(文字装飾)を指定する。
 *   iFormat以降は実際の出力内容
 */
void smpl_printLog( int iAttr, const char *iFormat, ... );


// 画面出力マクロ関数  SMPL_PRINT()
//   Ver1と Ver2で画面出力処理が変わるのでこれで吸収する。
#define SMPL_PRINT( ... )  smpl_printLog( 0, __VA_ARGS__ )

