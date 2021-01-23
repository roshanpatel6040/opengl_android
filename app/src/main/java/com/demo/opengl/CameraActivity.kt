package com.demo.opengl

import android.content.Context
import android.graphics.Bitmap
import android.graphics.SurfaceTexture
import android.opengl.GLES11Ext.GL_TEXTURE_EXTERNAL_OES
import android.opengl.GLES20
import android.opengl.GLES20.*
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.view.*
import android.widget.FrameLayout
import android.widget.SeekBar
import androidx.appcompat.app.AppCompatActivity
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import java.io.BufferedOutputStream
import java.io.File
import java.io.FileOutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10


class CameraActivity : AppCompatActivity(), SeekBar.OnSeekBarChangeListener {

    companion object {
        private const val BRIGHTNESS = "C_B"
        private const val CONTRAST = "C_C"
        private const val SATURATION = "C_S"
        private const val EXPOSURE = "C_E"
    }

    init {
        System.loadLibrary("cameraCpp")
    }

    lateinit var gl: GL
    lateinit var frameLayout: FrameLayout

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setFlags(
            WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
            WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
        )
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        val flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE or
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
                View.SYSTEM_UI_FLAG_FULLSCREEN or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        window.decorView.systemUiVisibility = flags

        gl = GL(this)
        gl.fitsSystemWindows = false
        frameLayout = FrameLayout(this)
        frameLayout.addView(gl)

        addSeekBar(SATURATION, 5, 0, 10, 0)
        addSeekBar(CONTRAST, 10, 0, 20, 150)
        addSeekBar(BRIGHTNESS, 5, 0, 10, 300)

        initialize()

        setContentView(frameLayout)

//        val file = File(Environment.getExternalStorageDirectory(),"default.png");
//        Log.e("File","${file.absolutePath}")
    }

    private fun addSeekBar(tag: String, progress: Int, min: Int, max: Int, margin: Int) {
        val seekBar = SeekBar(this)
        seekBar.max = max
        seekBar.min = min
        seekBar.progress = progress
        seekBar.tag = tag
        val layoutParams = FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.WRAP_CONTENT
        )
        layoutParams.setMargins(10, 10, 10, 10 + margin)
        layoutParams.gravity = Gravity.BOTTOM
        seekBar.layoutParams = layoutParams
        seekBar.setOnSeekBarChangeListener(this)
        frameLayout.addView(seekBar)
    }

    private fun addExposureTime(min: Long, max: Long) {
        addSeekBar(
            EXPOSURE,
            min.toInt() + ((max.toInt() - min.toInt()) / 2),
            min.toInt(),
            max.toInt(),
            450
        )
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
    private external fun changeExposure(exposure: Int)
    private external fun destroy()

    class GL @JvmOverloads constructor(context: Context) : GLSurfaceView(context) {

        private var renderer = Render(context)

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

        override fun onTouchEvent(event: MotionEvent?): Boolean {
            val index = event?.actionIndex
            val pointerId = event?.getPointerId(index!!)
            when (event?.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    renderer.captureImage(true)
                }
                MotionEvent.ACTION_MOVE -> {
                }
            }
            return true
        }

        private external fun capture()

        class Render(context: Context) : Renderer {

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
                    Log.d("Renderer", "saveImage() Image captured")
                } catch (e: Exception) {
                    Log.e("Renderer", "saveImage() $e")
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

                onDrawFrame(texMatrix, saturation, contrast, brightness)

                if (capture) {
                    captureImage(false)
                    synchronized(captureLock) {
                        val pixelBuffer: ByteBuffer =
                            ByteBuffer.allocateDirect(4 * width * height)
                        pixelBuffer.order(ByteOrder.LITTLE_ENDIAN)

                        glReadPixels(
                            0,
                            0,
                            width,
                            height,
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            pixelBuffer
                        )
                        CoroutineScope(Dispatchers.Default).launch {
                            saveImage(pixelBuffer)
                        }
                    }
                }
            }

            override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
                onSurfaceChanged(width, height)
                this.width = width
                this.height = height
            }

            override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
                // Prepare texture and surface
                val textureBuffer = IntArray(1)
                GLES20.glGenTextures(1, textureBuffer, 0)
                GLES20.glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureBuffer[0])

                surfaceTexture = SurfaceTexture(textureBuffer[0])
                surfaceTexture.setDefaultBufferSize(cameraWidth, cameraHeight)
                surfaceTexture.setOnFrameAvailableListener {
                    synchronized(lock) {
                        frameAvailable = true
                    }
                }

                val surface = Surface(surfaceTexture)

                onSurfaceCreated(textureBuffer[0], surface, cameraWidth, cameraHeight)
            }

            private external fun onSurfaceCreated(
                buffer: Int,
                surface: Surface,
                width: Int,
                height: Int
            )


            fun changeSaturation(value: Float) {
                saturation = value
            }

            fun changeContrast(value: Float) {
                contrast = value
            }

            fun changeBrightness(value: Float) {
                brightness = value
            }

            private external fun onSurfaceChanged(width: Int, height: Int)
            private external fun onDrawFrame(
                texMat: FloatArray,
                saturation: Float,
                contrast: Float,
                brightness: Float
            )

        }
    }

    override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
        when (seekBar?.tag) {
            SATURATION -> {
                gl.changeSaturation(progress.div(10.0f))
            }
            CONTRAST -> {
                gl.changeContrast(progress.div(10.0f))
            }
            BRIGHTNESS -> {
                gl.changeBrightness(progress.div(10.0f).minus(0.5f))
            }
            EXPOSURE -> {
                changeExposure(seekBar.progress)
            }
        }
    }

    override fun onStartTrackingTouch(seekBar: SeekBar?) {
    }

    override fun onStopTrackingTouch(seekBar: SeekBar?) {
    }
}