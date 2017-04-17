//
// Created by Administrator on 2016/4/10 0010.
//

#include "NativeMedia.h"

#define LOG_TAG "NativeVideo"

#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

JavaVM *gJavaVM;
NativeMedia *gNativeMedia;

pthread_mutex_t gMutex;

static void checkGlError(const char *op) {
    for (GLint error = glGetError(); error; error = glGetError()) {
        LOG("after %s() glError (0x%x)\n", op, error);
    }
}

static const char gVertexShader[] =
        "attribute vec4 aPosition;\n"
                "attribute vec4 aTexCoordinate;\n"
                "uniform mat4 texTransform;\n"
                "varying vec2 v_TexCoordinate;\n"
                "void main() {\n"
                "v_TexCoordinate = (texTransform * aTexCoordinate).xy;\n"
                "gl_Position = aPosition;\n"
                "}\n";

static const char gFragmentShader[] =
        "#extension GL_OES_EGL_image_external : require\n"
                "precision mediump float;\n"
                "uniform samplerExternalOES  texture;\n"
                "varying vec2 v_TexCoordinate;\n"
                "void main() {\n"
                "vec4 color = texture2D(texture,v_TexCoordinate);\n"
                "gl_FragColor = color;\n"
                "}\n";

GLuint loadShader(GLenum shaderType, const char *pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char *buf = (char *) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOG("Could not compile shader %d:\n%s\n",
                        shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char *pVertexSource, const char *pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char *buf = (char *) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOG("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

NativeMedia::NativeMedia():
        jni(NULL),
        javaVM(NULL),
        javaSurfaceTextureObj(NULL),
        nanoTimeStamp(0),
        updateTexImageMethodId(NULL),
        getTimestampMethodId(NULL),
        setDefaultBufferSizeMethodId(NULL),
        getTransformMtxId(NULL)
{
    pthread_mutex_init(&gMutex, 0);
}

NativeMedia::~NativeMedia() {

    pthread_mutex_destroy(&gMutex);

    if (javaSurfaceTextureObj) {
        jni->DeleteGlobalRef( javaSurfaceTextureObj );
        javaSurfaceTextureObj = 0;
    }
}

jobject NativeMedia::getSurfaceTextureObject() {
    if (javaSurfaceTextureObj == NULL) {
        LOG_ERROR("SurfaceTexture not be NULL");
        return NULL;
    }

    return javaSurfaceTextureObj;
}

static const GLfloat texTransform[16] = {
       1.0f,   0.0f,   0.0f,   0.0f,
       0.0f,   -1.0f,  0.0f,   0.0f,
       0.0f,   0.0f,   1.0f,   0.0f,
       0.0f,   1.0f,   0.0f,   1.0f
};


GLuint texId;
GLuint shaderProgram;
GLuint positionHandle;
GLint textureParamHandle;
GLuint textureCoordHandle;
GLuint textureTranformHandle;

int width, height;

unsigned int  vao;

unsigned int  vb;
unsigned int  ib;

void NativeMedia::setupGraphics(int w, int h) {
    width  = w;
    height = h;

    LOG_INFO("set meidia graphics w: %d, h: %d", w, h);

    shaderProgram = createProgram(gVertexShader, gFragmentShader);
    LOG("setupGraphics: %d \n", shaderProgram);

    if (!shaderProgram) {
        LOG_ERROR("gProgram: %d  (%s)\n", shaderProgram, "createProgram error");
        return;
    }

    textureParamHandle = glGetUniformLocation(shaderProgram, "texture");
    checkGlError("glGetUniformLocation");
    LOG("glGetUniformLocation(\"texturen\") = %d\n", textureParamHandle);

    positionHandle = glGetAttribLocation(shaderProgram, "aPosition");
    checkGlError("glGetAttribLocation");
    LOG("glGetAttribLocation(\"positionHandle\") = %d\n", positionHandle);

    textureCoordHandle = glGetAttribLocation(shaderProgram, "aTexCoordinate");
    checkGlError("glGetAttribLocation");
    LOG("glGetAttribLocation(\"aTexCoordinater\") = %d\n", textureCoordHandle);

    textureTranformHandle = glGetUniformLocation(shaderProgram, "texTransform");
    checkGlError("glGetAttribLocation");
    LOG("glGetUniformLocation(\"texTransform\") = %d\n", textureTranformHandle);


    struct Vertices
    {
        float positions[4][4];
        float texCoords[4][4];
    };

    const float x = 0.5f;
    const float y = 0.5f;
    const float z = -1.0f;

    static const Vertices vertices =
    {
        // positions
        {
            -x,    y,    z,
            -x,   -y,    z,
             x,   -y,    z,
             x,    y,    z
        },
        // texCoords
        {
             0.0f, 1.0f, 0.0f, 1.0f,
             0.0f, 0.0f, 0.0f, 1.0f,
             1.0f, 0.0f, 0.0f, 1.0f,
             1.0f, 1.0f, 0.0f, 1.0f
        },
    };

    static const unsigned short indices[] = {
      0, 1, 2,  0, 2 ,3
    };

     glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ib );
     glBindVertexArray( 0 ) ;

     glGenVertexArrays( 1, &vao );
     glBindVertexArray( vao );

     glGenBuffers( 1, &vb );
     glBindBuffer( GL_ARRAY_BUFFER, vb );
     glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), &vertices, GL_STATIC_DRAW );

     glGenBuffers( 1, &ib ) ;
     glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ib );
     glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );


    glEnableVertexAttribArray ( positionHandle );
    glVertexAttribPointer ( positionHandle, 3, GL_FLOAT, false, 0, (const GLvoid *)offsetof( Vertices, positions ));

    glEnableVertexAttribArray ( textureCoordHandle );
    glVertexAttribPointer ( textureCoordHandle, 4, GL_FLOAT, false, 0, (const GLvoid *)offsetof( Vertices, texCoords ));


    glBindVertexArray( 0 );

    glDisableVertexAttribArray( positionHandle ) ;
    glDisableVertexAttribArray( textureCoordHandle );
}


void NativeMedia::setFrameAvailable(bool const available) {
    pthread_mutex_lock(&gMutex);
    fameAvailable = available;
    pthread_mutex_unlock(&gMutex);
}

void NativeMedia::renderFrame() {

    pthread_mutex_lock(&gMutex);

    if (fameAvailable) {
        Update();
    }

    pthread_mutex_unlock(&gMutex);


    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    checkGlError("glClearColor");

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(shaderProgram);
    checkGlError("glUseProgram");

    glActiveTexture(GL_TEXTURE0) ;
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texId) ;

    glUniform1i(textureParamHandle, 0);

    glUniformMatrix4fv(textureTranformHandle, 1, GL_FALSE, texTransform);
    checkGlError("texTransform, glUniformMatrix4fv");

    glBindVertexArray( vao );
    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void *)0);
    checkGlError("glDrawElements");

    glBindVertexArray( 0 );
    glUseProgram(0);
}


void NativeMedia::destroy() {
    LOG_INFO("native destroy");
    glDeleteVertexArrays( 1, &vao);
    glDeleteBuffers( 1, &vb );
    glDeleteBuffers( 1, &ib );
    glDeleteProgram(shaderProgram);
}

JNIEnv *AttachJava()
{
    JavaVMAttachArgs args = {JNI_VERSION_1_4, 0, 0};
    JNIEnv* jni;
    int status = gJavaVM->AttachCurrentThread( &jni, &args);
    if (status < 0) {
        LOG_ERROR("<SurfaceTexture> faild to attach current thread!");
        return NULL;
    }
    return jni;
}

void NativeMedia::setupSurfaceTexture() {

    glGenTextures(1, &texId);
    checkGlError("glGenTextures");

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texId);
    checkGlError("glBindTexture");

    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    jni = AttachJava();

    if ( jni == NULL){
        LOG_ERROR("AttachJava");
        return;
    }

    const char *stClassPath = "android/graphics/SurfaceTexture";
    const jclass surfaceTextureClass = jni->FindClass(stClassPath);
    if (surfaceTextureClass == 0) {
        LOG_ERROR("FindClass (%s) failed", stClassPath);
    }

//    // find the constructor that takes an int
    const jmethodID constructor = jni->GetMethodID( surfaceTextureClass, "<init>", "(I)V" );
    if (constructor == 0) {
        LOG_ERROR("GetMethonID(<init>) failed");
    }

    jobject  obj = jni->NewObject(surfaceTextureClass, constructor, texId);
    if (obj == 0) {
        LOG_ERROR("NewObject() failed");
    }

    javaSurfaceTextureObj = jni->NewGlobalRef(obj);
    if (javaSurfaceTextureObj == 0) {
        LOG_ERROR("NewGlobalRef() failed");
    }

    //Now that we have a globalRef, we can free the localRef
    jni->DeleteLocalRef(obj);

    updateTexImageMethodId = jni->GetMethodID( surfaceTextureClass, "updateTexImage", "()V");
    if ( !updateTexImageMethodId ) {
        LOG_ERROR("couldn't get updateTexImageMethonId");
    }

    getTimestampMethodId = jni->GetMethodID(surfaceTextureClass, "getTimestamp", "()J");
    if (!getTimestampMethodId) {
        LOG_ERROR("couldn't get TimestampMethodId");
    }

    getTransformMtxId = jni->GetMethodID(surfaceTextureClass, "getTransformMatrix", "([F)V");
    if (!getTransformMtxId) {
        LOG_ERROR("couldn't get getTransformMtxId");
    }

    // jclass objects are loacalRefs that need to be free;
    jni->DeleteLocalRef( surfaceTextureClass );

    LOG_INFO("setupSurfaceTexture success: texId: %d", texId);
}


void NativeMedia::SetDefaultBufferSizse(const int width, const int height) {
    jni->CallVoidMethod(javaSurfaceTextureObj, setDefaultBufferSizeMethodId, width, height);
}



void NativeMedia::Update() {
    //latch the latest movie frame to the texture
    if (!javaSurfaceTextureObj) {
        return;
    }
    jni->CallVoidMethod(javaSurfaceTextureObj, updateTexImageMethodId);
    nanoTimeStamp = jni->CallLongMethod( javaSurfaceTextureObj, getTimestampMethodId);
    LOG("+++++updateTexImage++++++textureId: %p  w:%d, h: %d ", &texId,width, height);

}


extern "C" {

//////////////////////////////////////////////////////////////////////////////////////////
//                      java activity interface
//////////////////////////////////////////////////////////////////////////////////////////
JNIEXPORT void JNICALL Java_com_example_jarry_NativeMediaWrapper_nativeOnCreate(JNIEnv *env, jobject obj) {
    LOG_INFO("nativeOnCreate");
    gNativeMedia = new NativeMedia();
    env->GetJavaVM(&gJavaVM);
}

JNIEXPORT void JNICALL Java_com_example_jarry_NativeMediaWrapper_nativeOnDestroy(JNIEnv *env, jobject obj) {
    LOG_INFO("nativeOnCreate");
    gNativeMedia->destroy();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//                  java gl_surface_view renderer interface
//////////////////////////////////////////////////////////////////////////////////////////////////
JNIEXPORT void JNICALL Java_com_example_jarry_NativeMediaWrapper_nativeSurfaceCreated(JNIEnv *env, jobject obj) {
    LOG_INFO("nativeSurfaceCreated");
    gNativeMedia->setupSurfaceTexture();
}
JNIEXPORT void JNICALL Java_com_example_jarry_NativeMediaWrapper_nativeSurfaceChanged(JNIEnv *env, jobject obj, jint width, jint height) {
    LOG_INFO("nativeSurfaceChanged");

    gNativeMedia->setupGraphics(width, height);
}
JNIEXPORT void JNICALL Java_com_example_jarry_NativeMediaWrapper_nativeDrawFrame(JNIEnv *env, jobject obj) {
//    LOG_INFO("nativeDrawFrame");
    gNativeMedia->renderFrame();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//                  java SurfaceTexture interface
////////////////////////////////////////////////////////////////////////////////////////////////////
JNIEXPORT void JNICALL Java_com_example_jarry_NativeMediaWrapper_nativeFrameAailable(JNIEnv *env, jobject obj) {
    LOG_INFO("nativeFrameAailable");
    gNativeMedia->setFrameAvailable(true);
}

JNIEXPORT jobject JNICALL Java_com_example_jarry_NativeMediaWrapper_nativeGetSurfaceTexture(JNIEnv *env, jobject obj) {
    LOG_INFO("nativeGetSurfaceTexture");
    jobject surfaceTextureObj;
    surfaceTextureObj = gNativeMedia->getSurfaceTextureObject();

    return surfaceTextureObj;
}

}       // end extern ""C




