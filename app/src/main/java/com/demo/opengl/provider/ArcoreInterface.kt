package com.demo.opengl.provider

import android.app.Activity
import android.content.Context
import android.content.res.AssetManager

object ArcoreInterface {

    init {
        System.loadLibrary("arcoreImpld")
    }

    external fun onSurfaceCreated()
    external fun onSurfaceChanged(rotation: Int, width: Int, height: Int)
    external fun onDrawFrame()
    external fun onTouched(x: Float, y: Float)
    external fun onMove(x : Float, y : Float)
    external fun onCreate(manager: AssetManager)
    external fun onResume(context: Context, activity: Activity)
    external fun onPause()
    external fun onDestroy()

}