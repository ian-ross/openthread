/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements the OpenThread Instance class.
 */

#include "instance.hpp"

#include <openthread/platform/misc.h>

#include "common/logging.hpp"
#include "common/new.hpp"
#include "thread/router_table.hpp"

namespace ot {

#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

// Define the raw storage used for OpenThread instance (in single-instance case).
otDEFINE_ALIGNED_VAR(gInstanceRaw, sizeof(Instance), uint64_t);

#endif

Instance::Instance(void)
    : mTaskletScheduler()
    , mTimerMilliScheduler(*this)
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    , mTimerMicroScheduler(*this)
#endif
#if OPENTHREAD_MTD || OPENTHREAD_FTD
#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    , mHeap()
#endif
    , mMbedTls()
#endif // #if OPENTHREAD_MTD || OPENTHREAD_FTD
    , mRandomManager()
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    , mNotifier(*this)
    , mSettings(*this)
    , mMessagePool(*this)
    , mActiveScanCallback(NULL)
    , mActiveScanCallbackContext(NULL)
    , mEnergyScanCallback(NULL)
    , mEnergyScanCallbackContext(NULL)
    , mIp6(*this)
    , mThreadNetif(*this)
#if OPENTHREAD_ENABLE_APPLICATION_COAP
    , mApplicationCoap(*this)
#endif
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    , mApplicationCoapSecure(*this, /* aLayerTwoSecurity */ true)
#endif
#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
    , mChannelMonitor(*this)
#endif
#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
    , mChannelManager(*this)
#endif
#if OPENTHREAD_CONFIG_ANNOUNCE_SENDER_ENABLE
    , mAnnounceSender(*this)
#endif
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    , mLinkRaw(*this)
#endif
#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    , mLogLevel(static_cast<otLogLevel>(OPENTHREAD_CONFIG_INITIAL_LOG_LEVEL))
#endif
#if OPENTHREAD_ENABLE_VENDOR_EXTENSION
    , mExtension(Extension::ExtensionBase::Init(*this))
#endif
#if OPENTHREAD_ENABLE_DIAG
    , mDiags(*this)
#endif
    , mIsInitialized(false)
{
}

#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

Instance &Instance::InitSingle(void)
{
    Instance *instance = &Get();

    VerifyOrExit(instance->mIsInitialized == false);

    instance = new (&gInstanceRaw) Instance();

    instance->AfterInit();

exit:
    return *instance;
}

Instance &Instance::Get(void)
{
    void *instance = &gInstanceRaw;

    return *static_cast<Instance *>(instance);
}

#else // #if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

Instance *Instance::Init(void *aBuffer, size_t *aBufferSize)
{
    Instance *instance = NULL;

    VerifyOrExit(aBufferSize != NULL);

    // Make sure the input buffer is big enough
    VerifyOrExit(sizeof(Instance) <= *aBufferSize, *aBufferSize = sizeof(Instance));

    VerifyOrExit(aBuffer != NULL);

    instance = new (aBuffer) Instance();

    instance->AfterInit();

exit:
    return instance;
}

#endif // OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

void Instance::Reset(void)
{
    otPlatReset(this);
}

void Instance::AfterInit(void)
{
    mIsInitialized = true;
#if OPENTHREAD_MTD || OPENTHREAD_FTD

    // Restore datasets and network information

    Get<Settings>().Init();
    Get<Mle::MleRouter>().Restore();

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_VENDOR_EXTENSION
    Get<Extension::ExtensionBase>().SignalInstanceInit();
#endif
}

void Instance::Finalize(void)
{
    VerifyOrExit(mIsInitialized == true);

    mIsInitialized = false;

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    IgnoreReturnValue(otThreadSetEnabled(this, false));
    IgnoreReturnValue(otIp6SetEnabled(this, false));
    IgnoreReturnValue(otLinkSetEnabled(this, false));

    Get<Settings>().Deinit();
#endif

#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

    /**
     * Object was created on buffer, so instead of deleting
     * the object we call destructor explicitly.
     */
    this->~Instance();

#endif // !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

exit:
    return;
}

#if OPENTHREAD_MTD || OPENTHREAD_FTD
void Instance::FactoryReset(void)
{
    Get<Settings>().Wipe();
    otPlatReset(this);
}

otError Instance::ErasePersistentInfo(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);
    Get<Settings>().Wipe();

exit:
    return error;
}

void Instance::RegisterActiveScanCallback(otHandleActiveScanResult aCallback, void *aContext)
{
    mActiveScanCallback        = aCallback;
    mActiveScanCallbackContext = aContext;
}

void Instance::InvokeActiveScanCallback(otActiveScanResult *aResult) const
{
    if (mActiveScanCallback != NULL)
    {
        mActiveScanCallback(aResult, mActiveScanCallbackContext);
    }
}

void Instance::RegisterEnergyScanCallback(otHandleEnergyScanResult aCallback, void *aContext)
{
    mEnergyScanCallback        = aCallback;
    mEnergyScanCallbackContext = aContext;
}

void Instance::InvokeEnergyScanCallback(otEnergyScanResult *aResult) const
{
    if (mEnergyScanCallback != NULL)
    {
        mEnergyScanCallback(aResult, mEnergyScanCallbackContext);
    }
}
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

} // namespace ot
