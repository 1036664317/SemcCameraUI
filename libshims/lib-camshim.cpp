#include <gui/Surface.h>
#include <binder/Parcel.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <log/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <errno.h>
#include <new>

#undef LOG_TAG
#define LOG_TAG "lib-camshim"

namespace android {

extern "C" {

bool _ZN7android22checkCallingPermissionERKNS_8String16E(const String16& permission) {
    ALOGI("checkCallingPermission intercepted for: %s -> granting true",
          String8(permission).c_str());
    return true;
}

bool _ZN7android22checkCallingPermissionERKNS_8String16EPiS3_(
    const String16& permission, int32_t* outPid, int32_t* outUid) {
    ALOGI("checkCallingPermission(pid/uid) intercepted for: %s -> granting true",
          String8(permission).c_str());
    if (outPid) *outPid = -1;
    if (outUid) *outUid = -1;
    return true;
}

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

// ---------------------------------------------------------------------------
// Cacao getCaps Shim
// Resolves RefBase virtual inheritance ABI incompatibility in libcacao_client.so
// ---------------------------------------------------------------------------

struct CapsBufferStruct {
    int32_t numFds;
    int32_t fds[96];
    uint32_t size;
    void* buffer;
    uint32_t offset;
    uint32_t reserved;
};

#if defined(__LP64__)
static_assert(sizeof(CapsBufferStruct) == 408, "CapsBufferStruct must be 408 bytes on 64-bit");
#else
static_assert(sizeof(CapsBufferStruct) == 404, "CapsBufferStruct must be 404 bytes on 32-bit");
#endif

static inline uint32_t caps_getSize(void* caps) {
    typedef uint32_t (*GetSizeFn)(void*);
    void** vptr = *(void***)caps;
    GetSizeFn fn = (GetSizeFn)(vptr[4]);
    return fn(caps);
}

static inline int caps_serialize(void* caps, CapsBufferStruct* buf) {
    typedef int (*SerializeFn)(void*, CapsBufferStruct*);
    void** vptr = *(void***)caps;
    SerializeFn fn = (SerializeFn)(vptr[5]);
    return fn(caps, buf);
}

static inline int caps_deserialize(void* caps, CapsBufferStruct* buf) {
    typedef int (*DeserializeFn)(void*, CapsBufferStruct*);
    void** vptr = *(void***)caps;
    DeserializeFn fn = (DeserializeFn)(vptr[6]);
    return fn(caps, buf);
}

int _ZN7android5Cacao7getCapsERKN5cacao15ProcessCtrlCaps11CameraIndexEPNS1_4CapsE(
    const void* cameraIndex, void* caps) {
    int32_t camIdx = cameraIndex ? *(const int32_t*)cameraIndex : -1;
    ALOGI("Cacao::getCaps invoked: cameraIndex=%d, caps=%p", camIdx, caps);

    // 1. Initialize Cacao service connection in libcacao_client.so if available
    typedef void (*GetServiceFn)();
    GetServiceFn getServiceFn = (GetServiceFn)dlsym(RTLD_DEFAULT, "_ZN7android5Cacao10getServiceEv");
    if (getServiceFn) {
        getServiceFn();
    } else {
        ALOGW("Cacao::getService symbol not found");
    }

    // 2. Check mServicePid: if caller is cacao service itself, return 0
    pid_t* pServicePid = (pid_t*)dlsym(RTLD_DEFAULT, "_ZN7android5Cacao11mServicePidE");
    if (pServicePid && *pServicePid == getpid()) {
        ALOGI("Caller is cacao service itself (pid=%d), returning 0", getpid());
        return 0;
    }

    // 3. Check mService: if null, return 0
    void** pService = (void**)dlsym(RTLD_DEFAULT, "_ZN7android5Cacao8mServiceE");
    if (pService && *pService == nullptr) {
        ALOGW("Cacao::mService is null, returning 0");
        return 0;
    }

    if (caps == nullptr) {
        ALOGE("Cacao::getCaps: caps is null");
        return -103;
    }

    uint32_t capsSize = caps_getSize(caps);
    ALOGI("Cacao::getCaps: capsSize=%u", capsSize);

    // 4. Allocate MemoryHeapBase and MemoryBase with Android 16 ABI
    sp<MemoryHeapBase> heap = new MemoryHeapBase(capsSize, 0, "cacao_caps");
    if (heap == nullptr || heap->getBase() == MAP_FAILED) {
        ALOGE("Failed to allocate MemoryHeapBase of size %u", capsSize);
        return -103;
    }

    sp<MemoryBase> mem = new MemoryBase(heap, 0, capsSize);
    if (mem == nullptr) {
        ALOGE("Failed to allocate MemoryBase of size %u", capsSize);
        return -103;
    }

    // 5. Prepare CapsBufferStruct and serialize caps into shared memory
    CapsBufferStruct buf;
    memset(&buf, 0, sizeof(buf));
    buf.size = mem->size();
    buf.buffer = mem->unsecurePointer();
    buf.offset = 0;

    int serRet = caps_serialize(caps, &buf);
    if (serRet < 0) {
        ALOGE("caps_serialize failed: %d", serRet);
        return serRet;
    }

    // 6. Obtain cacao service binder
    sp<IServiceManager> sm = defaultServiceManager();
    if (sm == nullptr) {
        ALOGE("defaultServiceManager() returned null");
        return -103;
    }

    sp<IBinder> binder = sm->checkService(String16("cacao"));
    if (binder == nullptr) {
        binder = sm->getService(String16("cacao"));
    }
    if (binder == nullptr) {
        ALOGE("Failed to retrieve cacao service binder");
        return -103;
    }

    // 7. Perform transaction 3 (getCaps) to cacaoserver
    Parcel data, reply;
    data.writeInterfaceToken(String16("com.sonymobile.cacao.ICacaoService"));
    data.writeInt32(camIdx);
    data.writeStrongBinder(IInterface::asBinder(mem));
    data.writeInt32(buf.numFds);
    for (int i = 0; i < buf.numFds; i++) {
        data.writeFileDescriptor(buf.fds[i], false);
    }

    status_t status = binder->transact(3, data, &reply, 0);
    if (status != OK) {
        ALOGE("cacao transact(3) failed: %d", status);
        return (status == -110) ? -111 : status;
    }

    int32_t res = reply.readInt32();
    if (res != 0) {
        ALOGE("cacao service returned error: %d", res);
        return (res == -110) ? -111 : res;
    }

    // 8. Deserialize result back into caps
    buf.offset = 0;
    int desRet = caps_deserialize(caps, &buf);
    ALOGI("caps_deserialize completed successfully, returned %d", desRet);
    return desRet;
}

int _ZN5cacao22ProcessCtrlCapsFactory7getCapsERKNS_15ProcessCtrlCaps11CameraIndexEPNS_4CapsE(
    const void* cameraIndex, void* caps) {
    return _ZN7android5Cacao7getCapsERKN5cacao15ProcessCtrlCaps11CameraIndexEPNS1_4CapsE(cameraIndex, caps);
}

} // extern "C"

class Cacao {
public:
    class CacaoClient {
    public:
        static sp<IMemory> allocMemory(size_t size);
        static void freeMemory(sp<IMemory>& mem);
    };
};

sp<IMemory> Cacao::CacaoClient::allocMemory(size_t size) {
    ALOGI("CacaoClient::allocMemory invoked: size=%zu", size);
    sp<MemoryHeapBase> heap = new MemoryHeapBase(size, 0, "cacao_client");
    if (heap == nullptr || heap->getBase() == MAP_FAILED) {
        ALOGE("CacaoClient::allocMemory: failed to allocate MemoryHeapBase of size %zu", size);
        return nullptr;
    }
    sp<MemoryBase> mem = new MemoryBase(heap, 0, size);
    if (mem == nullptr) {
        ALOGE("CacaoClient::allocMemory: failed to allocate MemoryBase of size %zu", size);
        return nullptr;
    }
    ALOGI("CacaoClient::allocMemory succeeded: mem=%p, unsecurePointer=%p",
          mem.get(), mem->unsecurePointer());
    return mem;
}

void Cacao::CacaoClient::freeMemory(sp<IMemory>& mem) {
    ALOGI("CacaoClient::freeMemory invoked, mem=%p", mem.get());
    mem.clear();
}

// ---------------------------------------------------------------------------
// SomcMemoryHeap Shim
// Resolves RefBase / BnMemoryHeap virtual inheritance ABI incompatibility
// ---------------------------------------------------------------------------
class __attribute__((visibility("default"))) SomcMemoryHeap : public virtual BnMemoryHeap {
public:
    SomcMemoryHeap(size_t size, uint32_t flags, char const* name);

    int getHeapID() const override { return mHeap->getHeapID(); }
    void* getBase() const override { return mHeap->getBase(); }
    size_t getSize() const override { return mHeap->getSize(); }
    uint32_t getFlags() const override { return mHeap->getFlags(); }
    off_t getOffset() const override { return mHeap->getOffset(); }

private:
    sp<MemoryHeapBase> mHeap;
};

SomcMemoryHeap::SomcMemoryHeap(size_t size, uint32_t flags, char const* name)
    : mHeap(new MemoryHeapBase(size, flags, name)) {}

#if defined(__LP64__)
static_assert(sizeof(SomcMemoryHeap) <= 0x70, "SomcMemoryHeap outgrew the blob's operator new");
#else
static_assert(sizeof(SomcMemoryHeap) <= 0x38, "SomcMemoryHeap outgrew the blob's operator new");
#endif

} // namespace android

