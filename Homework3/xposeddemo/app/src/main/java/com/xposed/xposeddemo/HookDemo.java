package com.xposed.xposeddemo;

import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.XposedHelpers;
import de.robv.android.xposed.callbacks.XC_LoadPackage.LoadPackageParam;

public class HookDemo implements IXposedHookLoadPackage {
    public void handleLoadPackage(LoadPackageParam lpparam) throws Throwable {
        XposedBridge.log("Loaded app: " + lpparam.packageName);
//        hook 进入com.example.x86demo
        if (lpparam.packageName.equals("com.example.x86demo")) {
//            获取MainActivity类
            Class clazz = lpparam.classLoader.loadClass(
                    "com.example.x86demo.MainActivity");
//            对stringFromJNI函数进行hook
            XposedHelpers.findAndHookMethod(clazz, "stringFromJNI", new XC_MethodHook() {
                protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                    super.beforeHookedMethod(param);
                }
                protected void afterHookedMethod(MethodHookParam param) throws Throwable {
//                    修改返回值
                    param.setResult("Hello from fqh");
                }
            });
        }
    }
}
