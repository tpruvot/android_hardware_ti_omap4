/*
**
** Copyright 2010, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>

#include <utils/Errors.h>  // for status_t
#include <utils/Log.h>

#include "ISyslinkIpcListener.h"

namespace android {

enum {
    MSG_ID = IBinder::FIRST_CALL_TRANSACTION
};

class BpSyslinkIpcListener: public BpInterface<ISyslinkIpcListener>
{
public:
    BpSyslinkIpcListener(const sp<IBinder>& impl)
        : BpInterface<ISyslinkIpcListener>(impl)
    {
    }

    virtual int sendSyslinkMessage(int msgId)
    {
        Parcel data, reply;
        data.writeInt32(msgId);
        remote()->transact(MSG_ID, data, &reply, IBinder::FLAG_ONEWAY);
        return 0;
    }
};

IMPLEMENT_META_INTERFACE(SyslinkIpcListener, "ISyslinkIpcListener");

// ----------------------------------------------------------------------

status_t BnSyslinkIpcListener::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case MSG_ID: {
            int msgId = data.readInt32();
            sendSyslinkMessage(msgId);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android
