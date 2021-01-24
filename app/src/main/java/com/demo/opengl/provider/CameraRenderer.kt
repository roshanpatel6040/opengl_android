package com.demo.opengl.provider

import android.content.Context
import android.graphics.Bitmap
import android.graphics.SurfaceTexture
import android.opengl.GLES11Ext
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.os.Environment
import android.util.Log
import android.view.Surface
import android.widget.Toast
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.BufferedOutputStream
import java.io.File
import java.io.FileOutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class CameraRenderer(var context: Context) : GLSurfaceView.Renderer {

    companion object {
        private const val TAG = "CameraRenderer"
    }

    private var capture = false

    private lateinit var surfaceTexture: SurfaceTexture
    private val texMatrix = FloatArray(16)

    @Volatile
    private var frameAvailable: Boolean = false

    private val lock = Object()
    private val captureLock = Object()

    // Camera filter parameter values
    private var saturation = 0.5f
    private var contrast = 1.0f
    private var brightness = 0.0f

    private val cameraWidth = 1920
    private val cameraHeight = 1080
    private var width = context.resources.displayMetrics.widthPixels
    private var height = context.resources.displayMetrics.heightPixels

    fun captureImage(cap: Boolean) {
        capture = cap
    }

    private fun saveImage(pixelBuffer: ByteBuffer) {
        var i = 0
        val tmp = ByteArray(width * 4)
        while (i++ < height / 2) {
            pixelBuffer.get(tmp)
            System.arraycopy(
                pixelBuffer.array(),
                pixelBuffer.limit() - pixelBuffer.position(),
                pixelBuffer.array(),
                pixelBuffer.position() - width * 4,
                width * 4
            )
            System.arraycopy(
                tmp,
                0,
                pixelBuffer.array(),
                pixelBuffer.limit() - pixelBuffer.position(),
                width * 4
            )
        }

        pixelBuffer.rewind()

        var bos: BufferedOutputStream? = null
        val file = File(Environment.getExternalStorageDirectory(), "/pro/opengl.png")
        if (!file.exists()) {
            file.createNewFile()
        }
        try {
            bos = BufferedOutputStream(FileOutputStream(file))
            val bmp =
                Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
            bmp.copyPixelsFromBuffer(pixelBuffer)
            bmp.compress(Bitmap.CompressFormat.PNG, 100, bos)
            bmp.recycle()
            Log.d(TAG, "saveImage() Image captured")
        } catch (e: Exception) {
            Log.e(TAG, "saveImage() $e")
        } finally {
            bos?.close()
        }
    }

    override fun onDrawFrame(gl: GL10?) {
        synchronized(lock) {
            if (frameAvailable) {
                surfaceTexture.updateTexImage()
                surfaceTexture.getTransformMatrix(texMatrix)
                frameAvailable = false
            }
        }

        CameraInterface.onDrawFrame(texMatrix, saturation, contrast, brightness)

        if (capture) {
            captureImage(false)
            synchronized(captureLock) {
                val pixelBuffer: ByteBuffer =
                    ByteBuffer.allocateDirect(4 * width * height)
                pixelBuffer.order(ByteOrder.LITTLE_ENDIAN)

                GLES20.glReadPixels(
                    0,
                    0,
                    width,
                    height,
                    GLES20.GL_RGBA,
                    GLES20.GL_UNSIGNED_BYTE,
                    pixelBuffer
                )
                CoroutineScope(Dispatchers.Default).launch {
                    saveImage(pixelBuffer)
                    withContext(Dispatchers.Main) {
                        Toast.makeText(context, "Captured", Toast.LENGTH_SHORT).show()
                    }
                }
            }
        }
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        CameraInterface.onSurfaceChanged(width, height)
        this.width = width
        this.height = height
    }

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        // Prepare texture and surface
        val textureBuffer = IntArray(1)
        GLES20.glGenTextures(1, textureBuffer, 0)
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureBuffer[0])

        surfaceTexture = SurfaceTexture(textureBuffer[0])
        surfaceTexture.setDefaultBufferSize(cameraWidth, cameraHeight)
        surfaceTexture.setOnFrameAvailableListener {
            synchronized(lock) {
                frameAvailable = true
            }
        }

        val surface = Surface(surfaceTexture)

        CameraInterface.onSurfaceCreated(textureBuffer[0], surface, cameraWidth, cameraHeight)
    }

    fun changeSaturation(value: Float) {
        saturation = value
    }

    fun changeContrast(value: Float) {
        contrast = value
    }

    fun changeBrightness(value: Float) {
        brightness = value
    }

}