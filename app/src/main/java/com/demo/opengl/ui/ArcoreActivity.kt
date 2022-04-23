package com.demo.opengl.ui

import android.opengl.GLSurfaceView
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.GestureDetector
import android.view.MotionEvent
import com.demo.opengl.R
import com.demo.opengl.provider.ArcoreInterface
import kotlinx.android.synthetic.main.activity_arcore.*
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class ArcoreActivity : AppCompatActivity(), GLSurfaceView.Renderer {
    private lateinit var gestureDetector: GestureDetector
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_arcore)

        setUpGlSurface()
        ArcoreInterface.onCreate(assets)

        gestureDetector = GestureDetector(this, object : GestureDetector.SimpleOnGestureListener() {
            override fun onSingleTapUp(e: MotionEvent): Boolean {
                surfaceView.queueEvent {
                    ArcoreInterface.onTouched(e.x, e.y)
                }
                return true
            }

            override fun onDown(e: MotionEvent?): Boolean {
                return true
            }
        })

        surfaceView.setOnTouchListener { _, motionEvent -> gestureDetector.onTouchEvent(motionEvent) }
    }

    private fun setUpGlSurface() {
        surfaceView.preserveEGLContextOnPause = true
        surfaceView.setEGLContextClientVersion(2)
        surfaceView.setEGLConfigChooser(8, 8, 8, 8, 16, 0) // Alpha used for plane blending.
        surfaceView.setRenderer(this)
        surfaceView.renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY
        surfaceView.setWillNotDraw(false)
    }

    override fun onSurfaceCreated(p0: GL10?, p1: EGLConfig?) {
        ArcoreInterface.onSurfaceCreated()
    }

    override fun onSurfaceChanged(p0: GL10?, p1: Int, p2: Int) {
        val displayRotation = windowManager.defaultDisplay.rotation
        ArcoreInterface.onSurfaceChanged(displayRotation, p1, p2)
    }

    override fun onDrawFrame(p0: GL10?) {
        ArcoreInterface.onDrawFrame()
    }

    override fun onResume() {
        super.onResume()
        ArcoreInterface.onResume(this, this)
    }

    override fun onPause() {
        super.onPause()
        ArcoreInterface.onPause()
    }

    override fun onDestroy() {
        super.onDestroy()
        ArcoreInterface.onDestroy()
    }
}