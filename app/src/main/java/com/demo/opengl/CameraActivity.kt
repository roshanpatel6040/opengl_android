package com.demo.opengl

import android.content.Context
import android.graphics.SurfaceTexture
import android.opengl.GLES11Ext.GL_TEXTURE_EXTERNAL_OES
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.util.Log
import android.view.Surface
import android.view.Window
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import java.nio.ByteBuffer
import java.nio.IntBuffer
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class CameraActivity : AppCompatActivity() {

    init {
        System.loadLibrary("cameraCpp")
    }

    lateinit var gl: GL

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setFlags(
            WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
            WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS)
        initialize()
        gl = GL(this)
        gl.fitsSystemWindows = false
        setContentView(gl)
    }

    override fun onResume() {
        super.onResume()
        gl.onResume()
    }

    override fun onPause() {
        super.onPause()
        gl.onPause()
    }

    override fun onDestroy() {
        super.onDestroy()
        destroy()
    }

    private external fun initialize()
    private external fun destroy()

    class GL @JvmOverloads constructor(context: Context) : GLSurfaceView(context) {

        init {
            setEGLContextClientVersion(2)
            setRenderer(Render(context))
        }

        class Render(context: Context) : Renderer {

            private lateinit var surfaceTexture: SurfaceTexture
            private val texMatrix = FloatArray(16)

            @Volatile
            private var frameAvailable: Boolean = false
            private val lock = Object()

            private val width = context.resources.displayMetrics.widthPixels
            private val height = context.resources.displayMetrics.heightPixels

            override fun onDrawFrame(gl: GL10?) {
                synchronized(lock) {
                    if (frameAvailable) {
                        surfaceTexture.updateTexImage()
                        surfaceTexture.getTransformMatrix(texMatrix)
                        frameAvailable = false
                    }
                }

                onDrawFrame(texMatrix)
            }

            override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
                onSurfaceChanged(width, height)
            }

            override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
                // Prepare texture and surface
                val textureBuffer = IntArray(1)
                GLES20.glGenTextures(1, textureBuffer, 0)
                GLES20.glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureBuffer[0])

                surfaceTexture = SurfaceTexture(textureBuffer[0])
                surfaceTexture.setDefaultBufferSize(1920, 1080)
                surfaceTexture.setOnFrameAvailableListener {
                    synchronized(lock) {
                        frameAvailable = true
                    }
                }

                // Choose you preferred preview size here before creating surface
                // val optimalSize = getOptimalSize()
                // surfaceTexture.setDefaultBufferSize(optimalSize.width, optimalSize.height)
                val surface = Surface(surfaceTexture)

                // Pass to native code
                onSurfaceCreated(textureBuffer[0], surface, width, height)
            }

            private external fun onSurfaceCreated(
                buffer: Int,
                surface: Surface,
                width: Int,
                height: Int
            )

            private external fun onSurfaceChanged(width: Int, height: Int)
            private external fun onDrawFrame(texMat: FloatArray)

        }
    }
}