#include <gui/Surface.h>
#include <binder/Parcel.h>
#include <new>

namespace android {

extern "C" {

void _ZN7android7SurfaceC1ERKNS_2spINS_22IGraphicBufferProducerEEEb(
    Surface* thiz,
    const sp<IGraphicBufferProducer>& bufferProducer,
    bool controlledByApp) {
    new (thiz) Surface(bufferProducer, controlledByApp, nullptr);
}

void _ZN7android7SurfaceC2ERKNS_2spINS_22IGraphicBufferProducerEEEb(
    Surface* thiz,
    const sp<IGraphicBufferProducer>& bufferProducer,
    bool controlledByApp) {
    new (thiz) Surface(bufferProducer, controlledByApp, nullptr);
}

intptr_t _ZNK7android6Parcel10readIntPtrEv(const Parcel* thiz) {
#if defined(__LP64__)
    return thiz->readInt64();
#else
    return thiz->readInt32();
#endif
}

} // extern "C"
} // namespace android
