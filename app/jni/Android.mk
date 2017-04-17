
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := native_media

LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv2 -lGLESv3

LOCAL_SRC_FILES  := NativeMedia.cpp

include $(BUILD_SHARED_LIBRARY)


