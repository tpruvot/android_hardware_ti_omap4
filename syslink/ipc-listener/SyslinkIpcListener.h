#ifndef ANDROID_SYSLINKIPCLISTENER_H
#define ANDROID_SYSLINKIPCLISTENER_H

#include "ISyslinkIpcListener.h"

namespace android {
class SyslinkListenerThread : public Thread
{
    public:
        SyslinkListenerThread() : Thread(false) {}
        virtual bool threadLoop();
        virtual void onFirstRef()
        {
            run("SyslinkListenerThread", PRIORITY_NORMAL);
        }
};

class SyslinkIpcListener : public BnSyslinkIpcListener
{
    public:
        SyslinkIpcListener();
        virtual int sendSyslinkMessage(int msgId);
    private:
};



}; // namespace android
#endif // ANDROID_SYSLINKIPCLISTENER_H
