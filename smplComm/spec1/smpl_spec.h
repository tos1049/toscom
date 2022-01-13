/*
 *****************************************************************************
 *
 * smpl_spec.cの個別定義ヘッダ (Ver1)
 * 
 *   Ver2とは内容が異なるので注意。
 *   Ver1と Ver2のどちらにも存在するI/Fは smpl_com.h に記載する。
 *
 *****************************************************************************
 */

#pragma once


// 画面出力マクロ関数  SMPL_PRINT()
//   Ver1と Ver2で画面出力処理が変わるのでこれで吸収する。
#define SMPL_PRINT( ... )  com_printf( __VA_ARGS__ )

