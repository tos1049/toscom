/*
 *****************************************************************************
 *
 * 共通スレッド処理   by TOS
 *   ＊外部公開I/Fの使い方については com_if.h に記述しているので、必読。
 *
 *****************************************************************************
 */
#include "com_if.h"
#include "com_debug.h"


/* スレッド関連データ定義 ***************************************************/

// toscom内mutex定義
static pthread_mutex_t  gMutex = PTHREAD_MUTEX_INITIALIZER;

// スレッド管理情報
typedef struct {
    BOOL isUse;
    BOOL isRunning;
    BOOL isFinished;
    BOOL isConfirmed;
    com_threadInf_t inf;
    pthread_t ptid;
    com_thNotifyCB_t func;
    const char* label;
} threadManage_t;

static threadManage_t  gThreadList[COM_THREAD_MAX];

enum { COM_ALL_THREAD = -1 };



/* スレッド生成処理 *********************************************************/

static com_threadId_t searchEmpty( void )
{
    for( com_threadId_t id = 0;  id < COM_THREAD_MAX;  id++ ) {
        if( !(gThreadList[id].isUse) ) {return id;}
    }
    com_error( COM_ERR_THREADNG, "cannot create more thread" );
    return COM_NO_THREAD;
}

static threadManage_t *makeNewThreadInf(
        const void *iData, size_t iSize, com_thNotifyCB_t iFunc,
        const char *iLabel )
{
    com_threadId_t  newId = searchEmpty();
    if( newId == COM_NO_THREAD ) {return NULL;}
    char*  label = com_strdup( iLabel, NULL );
    if( !label ) {return NULL;}
    char*  userdata = NULL;
    if( iData ) {
        userdata = com_malloc( iSize, "thread(%s) userdata", label );
        if( !userdata ) {com_free( label );  return NULL;}
        memcpy( userdata, iData, iSize );
    }
    gThreadList[newId] = (threadManage_t){
        .isUse = true,
        .inf = (com_threadInf_t){ newId, userdata, iSize },
        .func = iFunc,   .label = label
    };
    return &(gThreadList[newId]);
}

static void freeThreadInfProc( threadManage_t *oInf )
{
    com_skipMemInfo( true );
    com_free( oInf->inf.data );
    com_free( oInf->label );
    oInf->isUse = false;
    com_skipMemInfo( false );
}

static BOOL countTasks( const com_seekDirResult_t *iInf )
{
    long*  count = iInf->userData;
    (*count)++;
    return true;
}

// マルチスレッド動作しているかどうか、自プロセスの情報から確認
static BOOL existThreads( void )
{
    char  path[COM_LINEBUF_SIZE] = {0};
    int  pid = getpid();
    snprintf( path, sizeof(path), "/proc/%d/task", pid );

    long  taskCount = 0;
    if( !com_seekDir2(path, NULL, COM_SEEK_DIR, countTasks, &taskCount ) ) {
        com_errorExit( COM_ERR_THREADNG, "proc cannot read" );
    }
    if( taskCount == 1 ) {return false;}  // 自分以外にスレッド無し
    return true;
}

static void freeThreadInf( com_threadId_t iId )
{
    com_skipMemInfo( true );
    for( com_threadId_t id = 0;  id < COM_THREAD_MAX;  id++ ) {
        threadManage_t*  mng = &(gThreadList[id]);

        if( iId == COM_ALL_THREAD || id == iId ) {
            if( mng->isUse && !mng->isFinished && existThreads() ) {
                pthread_cancel( mng->ptid );
                com_dbgCom( "forced to cancel thread (%ld)", id );
            }
            freeThreadInfProc( mng );
        }
    }
    pthread_testcancel();
    com_skipMemInfo( false );
}

static BOOL createThread(
        threadManage_t *oMng, com_thBoot_t iBoot, const char *iLabel )
{
    pthread_attr_t  attr;
    if( pthread_attr_init( &attr ) ) {
        com_error( COM_ERR_THREADNG, "fail to init thread attr(%s)", iLabel );
        return false;
    }
    if( pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED ) ) {
        com_error( COM_ERR_THREADNG, "fail to set thread attr(%s)", iLabel );
        return false;
    }
    if( pthread_create( &(oMng->ptid), &attr, iBoot, &(oMng->inf) ) ) {
        com_error( COM_ERR_THREADNG, "fail to create thread(%s)", iLabel );
        return false;
    }
    return true;
}

BOOL com_createThread(
        pthread_t *oPtid, com_thBoot_t iBoot,
        const void *iUserData, size_t iSize, com_thNotifyCB_t iFunc,
        const char *iFormat, ... )
{
    if( !iFunc ) {COM_PRMNG(false);}

    char  buff[COM_LINEBUF_SIZE];
    COM_SET_FORMAT( buff );
    if( !iFormat ) {(void)com_strcpy( buff, "createThread" );}
    const char  CREATE[] = "CREATE(%s)";
    com_mutexLock( &gMutex, CREATE, buff );
    com_skipMemInfo( true );
    threadManage_t*  newInf = makeNewThreadInf(iUserData,iSize,iFunc,buff);
    com_skipMemInfo( false );
    if( newInf ) {
        if( !createThread( newInf, iBoot, buff ) ) {
            freeThreadInfProc( newInf );
            newInf = NULL;
        }
    }
    com_mutexUnlock( &gMutex, CREATE, buff );
    if( !newInf ) {return false;}
    COM_SET_IF_EXIST( oPtid, newInf->ptid );
    return true;
}



/* スレッド情報取得処理 *****************************************************/

#define THREAD_LABEL( ID )   gThreadList[ID].label

static com_threadId_t getThreadId( pthread_t iPtid )
{
    for( com_threadId_t id = 0;  id < COM_THREAD_MAX;  id++ ) {
        threadManage_t*  mng = &(gThreadList[id]);
        if( mng->isUse && mng->ptid == iPtid ) {return id;}
    }
    return COM_NO_THREAD;
}

static threadManage_t *getThreadInf(
        com_threadId_t iThId, const char *iFuncName )
{
    if( iThId < 0 || iThId > COM_THREAD_MAX ) {COM_PRMNGF(iFuncName,NULL);}
    threadManage_t*  result = &(gThreadList[iThId]);
    if( !(result->isUse) ) {return NULL;}
    return result;
}

void *com_getThreadUserData( pthread_t iPtid )
{
    com_threadId_t  thId = getThreadId( iPtid );
    if( thId == COM_NO_THREAD ) {return NULL;}

    const char  GETDATA[] = "GETDATA(%s)";
    com_mutexLock( &gMutex, GETDATA, THREAD_LABEL(thId) );
    threadManage_t*  mng = getThreadInf( thId, __func__ );
    com_mutexUnlock( &gMutex, GETDATA, THREAD_LABEL(thId ) );
    if( !mng ) {return NULL;}
    return mng->inf.data;
}



/* 子スレッド専用処理 *******************************************************/

void com_readyThread( com_threadInf_t *iInf )
{
    const char  READY[] = "READY(%s)";
    com_mutexLock( &gMutex, READY, THREAD_LABEL(iInf->thId) );
    threadManage_t*  mng = &(gThreadList[iInf->thId]);
    mng->isRunning = true;
    mng->isFinished = false;
    mng->isConfirmed = false;
    com_mutexUnlock( &gMutex, READY, THREAD_LABEL(iInf->thId) );
}

void *com_finishThread( com_threadInf_t *iInf )
{
    const char  FINISH[] = "FINISH(%s)";
    com_mutexLock( &gMutex, FINISH, THREAD_LABEL(iInf->thId) );
    threadManage_t*  mng = &(gThreadList[iInf->thId]);
    mng->inf = *iInf;
    mng->isFinished = true;
    com_mutexUnlock( &gMutex, FINISH, THREAD_LABEL(iInf->thId) );
    return NULL;
}



/* スレッド終了と片付け *****************************************************/

static COM_THRD_STATUS_t confirmFinish( threadManage_t *oMng )
{
    if( oMng->isConfirmed ) {return COM_THST_FINISHED;}
    if( oMng->func ) {(oMng->func)( &(oMng->inf) );}
    oMng->isConfirmed = true;
    return COM_THST_FINISHED;
}

// mutexロックされた中で呼ばれることを前提とする
static COM_THRD_STATUS_t checkThread( com_threadId_t iThId )
{
    threadManage_t*  mng = getThreadInf( iThId, __func__ );
    if( !mng ) {return COM_THST_NOTEXIST;}
    if( !(mng->isRunning) ) {return COM_THST_CREATED;}
    if( mng->isFinished ) {return confirmFinish( mng );}
    if( !existThreads() ) {return COM_THST_ABORTED;}
    return COM_THST_RUNNING;
}

COM_THRD_STATUS_t com_checkThread( pthread_t iPtid )
{
    com_threadId_t  thId = getThreadId( iPtid );
    if( thId == COM_NO_THREAD ) {return COM_THST_NOTEXIST;}
    const char CHECK[] = "CHECK(%s)";
    com_mutexLock( &gMutex, CHECK, THREAD_LABEL(thId) );
    COM_THRD_STATUS_t  result = checkThread( thId );
    com_mutexUnlock( &gMutex, CHECK, THREAD_LABEL(thId) );
    return result;
}

BOOL com_watchThread( BOOL iBlock )
{
    const char  WATCH[] = "WATCH";
    long  count = 0;
    BOOL  detectFinish = false;
    do {
        count = 0;
        com_mutexLock( &gMutex, WATCH );
        for( com_threadId_t id = 0;  id < COM_THREAD_MAX;  id++ ) {
            COM_THRD_STATUS_t st = checkThread( id );
            if( st == COM_THST_CREATED || st == COM_THST_RUNNING ) {
                count++;
            }
            if( st == COM_THST_FINISHED || st == COM_THST_ABORTED ) {
                detectFinish = true;
            }
        }
        com_mutexUnlock( &gMutex, WATCH );
    } while( iBlock && count && !detectFinish );
    return (count != 0);
}

BOOL com_freeThread( pthread_t iPtid )
{
    com_threadId_t  thId = getThreadId( iPtid );
    if( thId == COM_NO_THREAD ) {return true;}
    const char  FREE[] = "FREE(%s)";
    com_mutexLock( &gMutex, FREE, THREAD_LABEL(thId) );
    BOOL  result = false;
    threadManage_t*  mng = getThreadInf( thId, __func__ );
    if( mng ) {
        if( mng->isFinished ) {freeThreadInfProc( mng );  result = true;}
    } 
    com_mutexUnlock( &gMutex, FREE, THREAD_LABEL(thId) );
    return result;
}



/* 排他処理(Mutex) **********************************************************/

static __thread char  gMutexBuf[COM_LINEBUF_SIZE];

int com_mutexLock( pthread_mutex_t *ioMutex, const char *iFormat, ... )
{
    int  result = pthread_mutex_lock( ioMutex );
    COM_SET_FORMAT( gMutexBuf );
    if( result == EINVAL ) {
        com_error( COM_ERR_MLOCKNG, "fail to mutex lock by %s", gMutexBuf );
    }
    return result;
}

int com_mutexUnlock( pthread_mutex_t *ioMutex, const char *iFormat, ... )
{
    int  result = pthread_mutex_unlock( ioMutex );
    // iForamt が NULLの場合、gMutexBufの内容を保持して使用する
    if( iFormat ) {COM_SET_FORMAT( gMutexBuf );}
    if( result == EINVAL ) {
        com_error( COM_ERR_MUNLOCKNG, "fail to mutex unlock by %s", gMutexBuf );
    }
    return result;
}



/* 初期化処理 ***************************************************************/

void com_initializeThread( void )
{
    // com_initialize()から呼ばれる限り、COM_DEBUG_AVOID_～は不要
    // 現状は処理なし
}



/* 終了処理 *****************************************************************/

void com_finalizeThread( void )
{
    // finalizeCom()から呼ばれる限り、COM_DEBUG_AVOID_～は不要
    freeThreadInf( COM_ALL_THREAD );
}

