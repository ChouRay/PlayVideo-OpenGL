//
// Created by Jarry on 2016/4/10 0010.
//
#ifndef PLAYVIDEO_TEXUTURE_NATIVEVIDEO_H
#define PLAYVIDEO_TEXUTURE_NATIVEVIDEO_H


#include <pthread.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <pthread.h>

#include <jni.h>
#include <strings.h>
#include <android/log.h>


#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)


class NativeMedia {

public:
    NativeVideo();
    virtual ~NativeVideo();

    bool setupEGL();
    void setupSurfaceTexture(int texId);

    // For some java-side uses, you can set the size
    // of the buffer before it is used to control how
    // large it is.  Video decompression and camera preview
    // always override the size automatically.
    void SetDefaultBufferSizse(const int width, const int height);

    // This can only be called with an active GL context.
    // As a side effect, the textureId will be bound to the
    // GL_TEXTURE_EXTERNAL_OES target of the currently active
    // texture unit.
    void Update();

    unsigned GetTextureId();
    jobject GetJavaObject();
    long long GetNanoTimeStamp();
    JNIEnv *AttachJava();
    void renderLoop();
    void renderFrame();
    bool setupGraphics(int w, int h);
    void setFrameAvailable(bool const available);

    static void *threadCallback(void *);

    void setNativeWindow(ANativeWindow *window);
    void setActivity(jobject obj);
    void destroy();

private:
    JNIEnv *jni;
    JavaVM *javaVM;
    jobject activity;
    ANativeWindow* nativeWindow;
    pthread_t pthreadId;
    pthread_mutex_t mutex;

    /*about EGL*/
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

    bool running;
    bool fameAvailable;


    /**about---surfaceTexture*/
    unsigned textureId;
    jobject javaObject;

    // Updated when Update() is called, can be used to
    // check if a new frame is available and ready
    // to be processed / mipmapped by other code.
    long long nanoTimeStamp;

    jmethodID updateTexImageMethodId;
    jmethodID getTimestampMethodId;
    jmethodID setDefaultBufferSizeMethodId;


};

#endif //PLAYVIDEO_TEXUTURE_NATIVEVIDEO_H
