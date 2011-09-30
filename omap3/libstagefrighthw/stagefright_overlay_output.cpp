#include "TIHardwareRenderer.h"

#include <media/stagefright/HardwareAPI.h>
#include <utils/Log.h>

#define LOG_TAG "TIHardwareRenderer"

using android::sp;
using android::ISurface;
using android::VideoRenderer;

VideoRenderer *createRenderer(
        const sp<ISurface> &surface,
        const char *componentName,
        OMX_COLOR_FORMATTYPE colorFormat,
        size_t displayWidth, size_t displayHeight,
        size_t decodedWidth, size_t decodedHeight, int32_t numOfOpBuffers) {
    using android::TIHardwareRenderer;

    TIHardwareRenderer *renderer =
        new TIHardwareRenderer(
                surface, displayWidth, displayHeight,
                decodedWidth, decodedHeight,
                colorFormat,  false, numOfOpBuffers);

    if (renderer->initCheck() != android::OK) {
        delete renderer;
        renderer = NULL;
    }

    return renderer;
}

