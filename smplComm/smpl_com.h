
#pragma once

// モジュール内共通の定数宣言
enum {
    SMPL_HANDLE_MAX = 16,         // ハンドルネームのバッファサイズ(文字数は-1)
    SMPL_ADDR_SIZE = 48,          // アドレスのバッファサイズ
    SMPL_PORT_MIN = 0,            // 自ポートで指定可能な最小値
    SMPL_PORT_MAX = 65535,        // 自ポートで指定可能な最大値
    SMPL_RETRY_MAX = 10,          // 自分から接続するときのリトライ回数
    SMPL_CHATLOG_SIZE = 32,       // チャットログファイル名のバッファサイズ
};

// SMPL_HANDLE_MAXを拡張する場合、HELLOメッセージのサイズが変わる。
// sendHELLO()の msg[]のサイズや、SMPL_SYSMSG_SIZEが溢れないか要チェック。



////////// smpl_main.c モジュール内公開I/F ///////////////////////////////////

char *smpl_makeDstLabel( int iNum, const char *iHandle );

BOOL smpl_procStdin( char *iData, size_t iDataSize );

BOOL smpl_sendStdin( char *iData, size_t iDataSize );

void smpl_switchWideMode( void );



////////// smpl_spec.c モジュール内公開I/F ///////////////////////////////////
//
// 以下の関数は Ver1・Ver2の処理差分を関数化したもの。
// 従って、どちらのソースにも存在し、その内容にいずれも差分があるものとなる。
// 今後も処理差分が出来た場合は、ここに追加となるだろう。
//
// 関数名は必ず smpl_spec～()とすること。
//
// どちらかのサンプルにしか無いものは smpl_spec.h に記述すること。
// その場合の関数名は smpl_spec～()にしないこと。

void smpl_specStart( void );

void smpl_specEnd( void );

BOOL smpl_specStdin( com_getStdinCB_t iRecvFunc, com_selectId_t *oStdinId );

// プロンプト表示用に渡す構造体
typedef struct {
    char* myHandle;
    char* dstHandle;
    long  dstId;
    BOOL  usePrompt;
    BOOL  wideMode;
    BOOL  broadcast;
    BOOL  wideBroadcast;
} smpl_promptInf_t;

void smpl_specPrompt( const smpl_promptInf_t *iInf );

BOOL smpl_specWait( const smpl_promptInf_t *iInf );

void smpl_specDispMessage( const char *iMsg );

void smpl_specInvalidMessage( const char *iMsg );

void smpl_specRecvMessage(
        const char *iMsg, const char *iDst, ssize_t iSize, const char *iTime );

void smpl_specCommand( const char *iData );

// ワイドモード判定用に渡す構造体
typedef struct {
    BOOL* widemode;
    BOOL* broadcast;
    BOOL* wideBroadcast;
    BOOL  withBcCmd;
} smpl_wideInf_t;

BOOL smpl_specWideInput( smpl_wideInf_t *ioInf );

void smpl_specInitialize( void );

void smpl_specFinalize( void );

