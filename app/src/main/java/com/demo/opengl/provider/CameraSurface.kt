package com.demo.opengl.provider

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.view.MotionEvent

class CameraSurface @JvmOverloads constructor(context: Context, attributeSet: AttributeSet) : GLSurfaceView(context, attributeSet) {

    private var renderer = CameraRenderer(context)

    init {
        setEGLContextClientVersion(3)
        setRenderer(renderer)
    }

    fun changeSaturation(value: Float) {
        renderer.changeSaturation(value)
    }

    fun changeContrast(value: Float) {
        renderer.changeContrast(value)
    }

    fun changeBrightness(value: Float) {
        renderer.changeBrightness(value)
    }

    fun changeHighlight(value: Float) {
        renderer.changeHighlight(value)
    }

    fun changeShadow(value: Float) {
        renderer.changeShadow(value)
    }

    override fun onTouchEvent(event: MotionEvent?): Boolean {
        val index = event?.actionIndex
        val pointerId = event?.getPointerId(index!!)
        when (event?.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
            }
            MotionEvent.ACTION_MOVE -> {
            }
        }
        return true
    }

    fun capture(cap: Boolean) {
        renderer.captureImage(cap)
    }

}
