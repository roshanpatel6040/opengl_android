package com.demo.opengl.provider

import android.graphics.Rect
import android.view.Surface

object CameraInterface {

    external fun initialize()
    external fun openCamera()
    external fun closeCamera()
    external fun destroy()
    external fun capture()
    external fun changeExposure(exposure: Int)
    external fun onSurfaceChanged(width: Int, height: Int)
    external fun onDrawFrame(texMat: FloatArray, saturation: Float, contrast: Float, brightness: Float, highlight: Float, shadow: Float, orientation: FloatArray)
    external fun onSurfaceCreated(buffer: Int, surface: Surface, width: Int, height: Int)
    external fun changeMode(mode: Int)
    external fun boundingBox(rect: Rect)

}