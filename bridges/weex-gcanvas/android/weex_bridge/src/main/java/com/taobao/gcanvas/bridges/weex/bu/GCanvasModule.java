package com.taobao.gcanvas.bridges.weex.bu;

import android.app.Activity;
import android.content.Context;
import android.graphics.Point;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.Log;
import android.view.Display;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;

import com.alibaba.weex.plugin.annotation.WeexModule;
import com.taobao.gcanvas.GCanvasJNI;
import com.taobao.gcanvas.adapters.img.impl.fresco.GCanvasFrescoImageLoader;
import com.taobao.gcanvas.bridges.spec.bridge.IJSCallbackDataFactory;
import com.taobao.gcanvas.bridges.spec.module.AbsGBridgeModule;
import com.taobao.gcanvas.bridges.spec.module.IGBridgeModule;
import com.taobao.gcanvas.bridges.weex.bridge.WeexJSCallbackDataFactory;
import com.taobao.gcanvas.surface.GTextureView;
import com.taobao.gcanvas.util.GLog;
import com.taobao.weex.annotation.JSMethod;
import com.taobao.weex.bridge.JSCallback;
import com.taobao.weex.common.Destroyable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.lang.ref.WeakReference;
import java.util.HashMap;

import static com.taobao.gcanvas.bridges.spec.module.IGBridgeModule.ContextType._2D;


/**
 * @author ertong
 *         create at 2017/8/17
 */

@WeexModule(name = "gcanvas")
public class GCanvasModule implements Destroyable, GCanvasComponent.SurfaceOk {
    public Context context;
    private static final String TAG = "GCanvasModule";
    private GCanvasComponent component = null;
    private ModuleImpl mImpl;

    @Override
    public void available() {
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                GCanvasModule.this.setLogLevel("info");
                GCanvasModule.this.setContextType("asdqsadas",component.getRef());
                GCanvasModule.this.setAlpha(component.getRef(),1f);
                GCanvasJNI.setHiQuality(component.getRef(),true);
                GCanvasModule.this.setDevicePixelRatio(component.getRef());
                GCanvasModule.this.render(component.getRef(),null);
            }
        },2000);
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                GCanvasModule.this.render(component.getRef(),null);
            }
        },3000);
    }

    private static class ModuleImpl extends AbsGBridgeModule<JSCallback> {

        private String refId = null;
        private WeakReference<GCanvasModule> mOutRef;

        private WeexJSCallbackDataFactory mFactory = new WeexJSCallbackDataFactory();

        public ModuleImpl(GCanvasModule module) {
            this.mOutRef = new WeakReference<>(module);
        }

        @Override
        public String enable(JSONObject data) {


            try {
                String refId = data.getString("componentId");
                this.refId = refId;
                return Boolean.TRUE.toString();
            } catch (JSONException e) {
                return Boolean.FALSE.toString();
            }
        }

        @Override
        public void setContextType(String canvasId, ContextType type) {
            GCanvasModule module = this.mOutRef.get();
            if (null == module) {
                return;
            }

            Display display = ((Activity) getContext()).getWindowManager().getDefaultDisplay();

            int width = display.getWidth();
            double devicePixelRatio = width / 750.0;

            GLog.d(TAG, "enable width " + width);
            GLog.d(TAG, "enable devicePixelRatio " + devicePixelRatio);

            /**
             * open high Quality default
             */
            GCanvasJNI.setWrapperHiQuality(canvasId, true);
            GCanvasJNI.setWrapperDevicePixelRatio(canvasId, devicePixelRatio);
            GCanvasJNI.setWrapperContextType(canvasId, type.value());

            if (GCanvasJNI.sendEvent(canvasId)) {
                GLog.d("start to send event in module.");
            }
        }

        @Override
        public void setDevicePixelRatio(String canvasId, double ratio) {
            GCanvasJNI.setDevicePixelRatio(canvasId, ratio);
        }

        @Override
        public void render(String canvasId, String cmd) {
            GCanvasJNI.render(canvasId, cmd);
        }

        @Override
        public Context getContext() {
          return this.mOutRef.get().context;
        }

        @Override
        protected void invokeCallback(JSCallback jsCallback, Object args) {
            if (null != jsCallback) {
                jsCallback.invoke(args);
            }
        }

        @Override
        protected IJSCallbackDataFactory getDataFactory() {
            return mFactory;
        }
    }

    public GCanvasModule(Context context) {
        this.context = context;
        mImpl = new ModuleImpl(this);
        mImpl.setImageLoader(new GCanvasFrescoImageLoader());
        component = new GCanvasComponent(context,this);
    }
    public void init(FrameLayout parent){
        parent.addView(this.component.addGCanvasView(),new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    }

    @JSMethod(uiThread = false)
    public void bindImageTexture(final String args, final String refId, final JSCallback callback) {
        String url = null;
        int rid = 0;
        if (!TextUtils.isEmpty(args)) {
            try {
                JSONArray dataArray = new JSONArray(args);
                url = dataArray.getString(0);
                rid = dataArray.getInt(1);
            } catch (Throwable e) {
                GLog.e(TAG, e.getMessage(), e);
            }
        }

        final String src = url;
        final int id = rid;
        mImpl.bindImageTexture(refId, src, id, callback);
    }

    @JSMethod(uiThread = false)
    public void resetComponent(String refId) {
    }

    @JSMethod(uiThread = false)
    public void setup(String args, String refId, JSCallback callback) {
    }


    @JSMethod(uiThread = false)
    public void preLoadImage(String args, final JSCallback callBack) {
        GLog.d(TAG, "preLoadImage() in GCanvasWeexModule,args: " + args);
        if (!TextUtils.isEmpty(args)) {
            try {
                JSONArray dataArray = new JSONArray(args);
                mImpl.preLoadImage(dataArray, callBack);
            } catch (Throwable e) {
                GLog.e(TAG, e.getMessage(), e);
            }
        }
    }

    @JSMethod(uiThread = false)
    public void setHiQuality(String args, String refId) {
    }

    @JSMethod(uiThread = false)
    public void setLogLevel(String args) {
        GLog.d(TAG, "setLogLevel() args: " + args);
        int level = 0;
        try {
            level = Integer.parseInt(args);
        } catch (Throwable throwable) {
            // for old version compatiblity
            switch (args) {
                case "debug":
                    level = 0;
                    break;

                case "info":
                    level = 1;
                    break;

                case "warn":
                    level = 2;
                    break;

                case "error":
                    level = 3;
                    break;
            }
        }
        mImpl.setLogLevel(level);
    }


    public String enable(String args) {
        try {
            JSONObject jo = new JSONObject(args);
            return mImpl.enable(jo);
        } catch (Throwable e) {
            return Boolean.FALSE.toString();
        }
    }

    public void render(String cmd, String refId) {
        GCanvasJNI.render(this.component.getRef(),"W10;F#fdbb56;S#fdbb56;b;g10.00,10.00;i500.00,500.00;x;"  );
    }

    public void getDeviceInfo(String args, JSCallback callBack) {
        if (!TextUtils.isEmpty(args)) {

            HashMap<String, Object> hm = new HashMap<>();

            JSONObject data = new JSONObject();
            try {
                data.put("platform", "Android");
            } catch (JSONException e) {
            }
            hm.put("data", data.toString());
            callBack.invoke(hm);
        }
    }

    public void setContextType(String args, String refId) {
        if (TextUtils.isEmpty(args) || TextUtils.isEmpty(refId)) {
            return;
        }

        Context ctx = this.context;
        if (ctx == null) {
            GLog.e(TAG, "setDevicePixelRatio error ctx == null");
            return;
        }

        IGBridgeModule.ContextType type = _2D;

        mImpl.setContextType(refId, type);
    }


    public void setAlpha(String refId, float alpha) {
        GLog.d("start to set alpha in 3dmodule."+refId);
        if (component != null) {
            GTextureView view = component.getSurfaceView();
            if (view != null) {
                GLog.d("set alpha success in 3dmodule.");
                view.setAlpha(alpha);
            }
        }
        GCanvasJNI.setClearColor(component.getRef(),"rgb(242,12,42)");
    }

    @JSMethod(uiThread = false)
    public void setDevicePixelRatio(String refId) {
        WindowManager windowManager = (WindowManager) this.context.getSystemService(Context.WINDOW_SERVICE);

        Display display = windowManager.getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        double devicePixelRatio = (double) size.x * 1.0D / (double) size.y;
        Log.d("CampHippy", "enable size " + size.toString());
        Log.d("CampHippy", "enable devicePixelRatio " + devicePixelRatio);
        mImpl.setDevicePixelRatio(refId, devicePixelRatio);
    }

    @JSMethod(uiThread = false)
    public String execGcanvaSyncCMD(String refId, String action, String args) {
        return "";
    }

    @Override
    public void destroy() {
//        Iterator iter = mComponentMap.entrySet().iterator();
//        while (iter.hasNext()) {
//            Map.Entry entry = (Map.Entry) iter.next();
//            WXGCanvasWeexComponent val = (WXGCanvasWeexComponent) entry.getValue();
//            GLog.d("component destroy id=" + entry.getKey());
//
//            val.onActivityDestroy();
//        }
//
//        mComponentMap.clear();
    }

    @JSMethod(uiThread = false)
    public void texImage2D(final String refid, final int target, final int level, final int internalformat, final int format, final int type, String path) {
        if (!TextUtils.isEmpty(path)) {
            mImpl.texImage2D(refid, target, level, internalformat, format, type, path);
        }
    }

    @JSMethod(uiThread = false)
    public void texSubImage2D(final String refid, final int target, final int level, final int xoffset, final int yoffset, final int format, final int type, String path) {
        if (!TextUtils.isEmpty(path)) {
            mImpl.texSubImage2D(refid, target, level, xoffset, yoffset, format, type, path);
        }
    }
}
