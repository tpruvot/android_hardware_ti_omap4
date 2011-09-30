
#define LOG_TAG "SyslinkIpcListener"
#include <utils/Log.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include "SyslinkIpcListener.h"
#include "syslink_ipc_listener.h"

namespace android {
SyslinkIpcListener::SyslinkIpcListener()
{
    LOGD("Created");
}

int SyslinkIpcListener::sendSyslinkMessage(int msgId)
{
    if (msgId == 543210)
    {
        LOGD("Got abort message, stopping process");
        IPCThreadState::self()->stopProcess();
    }
    return 0;
}

bool SyslinkListenerThread::threadLoop()
{
    status_t ret = NO_ERROR;
    int retryCount = 0;
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();
    LOGD("Got serviceManager: %p", sm.get());

    do
    {
        if (retryCount > 0)
        {
            LOGD("Attempting to add service again, %d try", retryCount);
            sleep(1);
        }
        ret = sm->addService(String16("omap4.syslink_recovery"), new SyslinkIpcListener());
        retryCount++;
    } while ( ret != NO_ERROR && retryCount < 6);

    if (ret == NO_ERROR)
    {
        ProcessState::self()->startThreadPool();
        LOGD("Launching thread pool");
        IPCThreadState::self()->joinThreadPool();
        LOGW("Aborting syslink daemon");
        raise(SIGTERM);
    }

    return false;
}

};

using namespace android;
static android::sp<SyslinkListenerThread> listenerThread;

extern "C" {

void startSyslinkListenerThread()
{
    if (listenerThread.get() == NULL)
        listenerThread = new SyslinkListenerThread();
    else
        LOGE("Listener thread already started, not expecting this...");
}

void clearSyslinkListenerThread()
{
    listenerThread.clear();
}

}

