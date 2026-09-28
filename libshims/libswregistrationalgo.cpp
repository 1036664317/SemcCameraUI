#include <android/log.h>
#include <string.h>

#define LOG_TAG "SWREG_ALGO"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" {

int register_mf(
    void* src_buf,
    void* ref_buf,
    int width,
    int height,
    int stride,
    int arg5,
    float* homography,
    int* arg7,
    int arg8,
    int arg9,
    int center_x,
    int center_y,
    double scale_x,
    double scale_y)
{
    ALOGI("register_mf called: %dx%d, stride=%d, homography=%p", width, height, stride, homography);
    if (homography != nullptr) {
        // Provide identity homography matrix (no warp / baseline alignment)
        homography[0] = 1.0f; homography[1] = 0.0f; homography[2] = 0.0f;
        homography[3] = 0.0f; homography[4] = 1.0f; homography[5] = 0.0f;
        homography[6] = 0.0f; homography[7] = 0.0f; homography[8] = 1.0f;
    }
    if (arg7 != nullptr) {
        *arg7 = 0;
    }
    return 0; // Success
}

}
