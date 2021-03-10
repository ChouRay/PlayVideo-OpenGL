//
// Created by Jarry on 2016/4/10 0010.
//
#ifndef PLAYVIDEO_TEXUTURE_NATIVEVIDEO_H
#define PLAYVIDEO_TEXUTURE_NATIVEVIDEO_H


#include <jni.h>
#include <strings.h>
#include <pthread.h>
#include <android/log.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

class NativeMedia {

public:
    NativeMedia();
    virtual ~NativeMedia();


    void setupSurfaceTexture();

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

    void renderFrame();
    void setupGraphics(int w, int h);
    void setFrameAvailable(bool const available);

    jobject getSurfaceTextureObject();

    void destroy();

private:
    JNIEnv *jni;
    JavaVM *javaVM;

    bool running;
    bool fameAvailable;

    /**about---surfaceTexture*/
    jobject javaSurfaceTextureObj;

    // Updated when Update() is called, can be used to
    // check if a new frame is available and ready
    // to be processed / mipmapped by other code.
    long long nanoTimeStamp;

    jmethodID updateTexImageMethodId;
    jmethodID getTimestampMethodId;
    jmethodID setDefaultBufferSizeMethodId;
    jmethodID getTransformMtxId;
};

#endif //PLAYVIDEO_TEXUTURE_NATIVEVIDEO_H
