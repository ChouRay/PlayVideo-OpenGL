package com.example.jarry;

import android.graphics.SurfaceTexture;
import android.media.MediaPlayer;
import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.Surface;

import java.io.IOException;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * NativeMediaActivity
 *
 * if you want to render movie rightly on this activity,
 * you should set your environment for ANDROID_NDK or your local.properties contains ndk.dir
 */
public class NativeMediaActivity extends AppCompatActivity implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener{
//    public static final String videoPath = Environment.getExternalStorageDirectory().getPath()+"/Movies/不将就.mp4";
    public static final String videoPath = "http://www.w3school.com.cn/example/html5/mov_bbb.mp4";

    private SurfaceTexture videoTexture;
    private GLSurfaceView glView;
    private MediaPlayer mediaPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        NativeMediaWrapper.nativeOnCreate();

        glView = new GLSurfaceView(this);
        setContentView(glView);
        glView.setEGLContextClientVersion(2);
        glView.setRenderer(this);

    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        NativeMediaWrapper.nativeSurfaceCreated();
        videoTexture = NativeMediaWrapper.nativeGetSurfaceTexture();
        if (videoTexture != null) {
            videoTexture.setOnFrameAvailableListener(this);
        }
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        NativeMediaWrapper.nativeSurfaceChanged(width, height);

        playVideo();
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        NativeMediaWrapper.nativeDrawFrame();
    }


    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        NativeMediaWrapper.nativeFrameAailable();
    }


    private void playVideo() {
        if (mediaPlayer == null) {
            mediaPlayer = new MediaPlayer();
            mediaPlayer.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
                @Override
                public void onPrepared(MediaPlayer mp) {
                    mp.start();
                }
            });
            if (videoTexture == null) {
                return;
            }
            Surface surface = new Surface(videoTexture);
            mediaPlayer.setSurface(surface);
            surface.release();
            try {
                mediaPlayer.setDataSource(videoPath);
                mediaPlayer.prepareAsync();
            } catch (IOException e) {
                e.printStackTrace();
            }
        } else {
            mediaPlayer.start();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mediaPlayer != null) {
            mediaPlayer.pause();
        }
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        NativeMediaWrapper.nativeOnDestroy();

        if (mediaPlayer != null) {
            mediaPlayer.stop();
            mediaPlayer.release();
            mediaPlayer = null;
        }
    }

}
