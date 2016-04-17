
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := native_media

LOCAL_CFLAGS	:= -DANDROID_NDK
LOCAL_CFLAGS	+= -Werror			# error on warnings
LOCAL_CFLAGS	+= -Wall
LOCAL_CFLAGS	+= -Wextra
#LOCAL_CFLAGS	+= -Wlogical-op		# not part of -Wall or -Wextra
#LOCAL_CFLAGS	+= -Weffc++			# too many issues to fix for now
LOCAL_CFLAGS	+= -Wno-strict-aliasing		# TODO: need to rewrite some code
LOCAL_CFLAGS	+= -Wno-unused-parameter
LOCAL_CFLAGS	+= -Wno-missing-field-initializers	# warns on this: SwipeAction	ret = {}
LOCAL_CFLAGS	+= -Wno-multichar	# used in internal Android headers:  DISPLAY_EVENT_VSYNC = 'vsyn',
LOCAL_CPPFLAGS := -Wno-type-limits
LOCAL_CPPFLAGS += -Wno-invalid-offsetof
LOCAL_CPPFLAGS += -Wno-unused-function


LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv2 -lGLESv3

LOCAL_SRC_FILES  := NativeMedia.cpp

include $(BUILD_SHARED_LIBRARY)


