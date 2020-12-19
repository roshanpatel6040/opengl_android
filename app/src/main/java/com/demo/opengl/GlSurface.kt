package com.demo.opengl

import android.content.Context
import android.opengl.GLSurfaceView
import android.view.MotionEvent
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class GlSurface @JvmOverloads constructor(private val mContext: Context) : GLSurfaceView(mContext) {

    init {
        setEGLContextClientVersion(2)
        setRenderer(Render())
        renderMode = RENDERMODE_WHEN_DIRTY
    }

    override fun onTouchEvent(event: MotionEvent?): Boolean {
        val index = event?.actionIndex
        val pointerId = event?.getPointerId(index!!)
        when (event?.actionMasked) {
            MotionEvent.ACTION_MOVE -> {
                val x = event.x
                val y = event.y
                touchPoints(x, y)
            }
        }
        return true
    }

    private external fun touchPoints(x: Float, y: Float)

}

class Render : GLSurfaceView.Renderer {

    private var triangle: Triangle? = null
    private var square: Square? = null

    private var viewMatrix = FloatArray(16)

    override fun onDrawFrame(gl: GL10?) {
        render()
//        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT)
//        square?.draw()
//        triangle?.draw(viewMatrix)
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        onSurfaceChanged(width, height)
//        GLES20.glViewport(0, 0, width, height)
//        Matrix.frustumM()
    }

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        onSurfaceCreated()
//        GLES20.glClearColor(1F, 1F, 0F, 1F)
//        triangle = Triangle()
//        square = Square()
//        Matrix.setLookAtM(viewMatrix, 0, 0f, 1.5f, 0f, 0f, 0f, -5.0f, 0f, 1f, 0f)
    }

    private external fun render()
    private external fun onSurfaceCreated()
    private external fun onSurfaceChanged(width: Int, height: Int)

}