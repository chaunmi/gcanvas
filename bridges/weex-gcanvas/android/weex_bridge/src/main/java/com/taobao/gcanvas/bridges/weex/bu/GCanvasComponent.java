package com.taobao.gcanvas.bridges.weex.bu;

import android.app.Activity;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.text.TextUtils;
import android.view.Display;
import android.view.TextureView;

import com.alibaba.weex.plugin.annotation.WeexComponent;
import com.taobao.gcanvas.GCanvasJNI;
import com.taobao.gcanvas.bridges.spec.module.IGBridgeModule;
import com.taobao.gcanvas.surface.GTextureView;
import com.taobao.gcanvas.util.GLog;
import com.taobao.weex.annotation.Component;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * @author ertong
 */

@WeexComponent(names = {"gcanvas"})
@Component(lazyload = false)
public class GCanvasComponent  implements TextureView.SurfaceTextureListener {
    private GTextureView mSurfaceView;
    public AtomicBoolean mSended = new AtomicBoolean(false);
    public IGBridgeModule.ContextType mType;
    private Context context = null;
    private SurfaceOk  surfaceInterface= null;
    private static final String TAG = GCanvasComponent.class.getSimpleName();
    public String getRef(){
        return "999";
    }
    public GTextureView addGCanvasView() {
        String backgroundColor = "rgba(255,255,255,255)";
        mSurfaceView = new GTextureView(this.context, this.getRef());
//        GCanvasJNI.registerWXCallNativeFunc(this.context);
        if (TextUtils.isEmpty(backgroundColor)) {
            backgroundColor = "rgba(255,255,255,255)";
        }
        mSurfaceView.setBackgroundColor(backgroundColor);
        mSurfaceView.addSurfaceTextureListener(this);
        return mSurfaceView;
    }
    public String getCanvasKey(){
        return mSurfaceView.getCanvasKey();
    }
    static interface SurfaceOk{
        void available();
    }
    public GCanvasComponent(Context context,SurfaceOk sufaceCheck) {
        this.context = context;
        this.surfaceInterface = sufaceCheck;
    }


    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
      this.surfaceInterface.available();
    }

    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
        Context ctx = this.context;
        if (ctx == null) {
            GLog.e(TAG, "setDevicePixelRatio error ctx == null");
            return;
        }

        Display display = ((Activity) ctx).getWindowManager().getDefaultDisplay();

        int screenWidth = display.getWidth();
        double devicePixelRatio = screenWidth / 750.0;

        GLog.d(TAG, "enable width = " + screenWidth + ",devicePixelRatio = " + devicePixelRatio);

        GCanvasJNI.setWrapperDevicePixelRatio(this.getRef(), devicePixelRatio);
    }

    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        return true;
    }

    public void onSurfaceTextureUpdated(SurfaceTexture surface) {
    }

    public GTextureView getSurfaceView() {
        return mSurfaceView;
    }

}
