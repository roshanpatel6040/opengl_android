package com.demo.opengl.provider

import android.content.Context
import android.graphics.Bitmap
import android.graphics.SurfaceTexture
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.opengl.GLES11Ext
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.os.Environment
import android.util.Log
import android.view.Surface
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


class CameraRenderer(var context: Context) : GLSurfaceView.Renderer {

    companion object {
        private const val TAG = "CameraRenderer"

        private const val ALPHA = 0.8F // if alpha is 1 and 0 then no filter applies
    }

    private val rotationMatrix = FloatArray(16)
    private val remappedRotationMatrix = FloatArray(16)
    private val orientations = FloatArray(3)
    private var rotationVectorValues = FloatArray(5)

    private var lastTime: Long = 0

    fun lowPass(input: FloatArray, output: FloatArray): FloatArray {
        if (output.isEmpty()) {
            return input
        }

        for (i in input.indices) {
            output[i] = ALPHA * output[i] + (1 - ALPHA) * input[i]
        }
        return output
    }

    private var listener = object : SensorEventListener {
        override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {
        }

        override fun onSensorChanged(event: SensorEvent) {
            if (event.sensor.type == Sensor.TYPE_GAME_ROTATION_VECTOR) {
                val currentTime = System.currentTimeMillis()
                if ((currentTime - lastTime) > 0) {
                    rotationVectorValues = lowPass(event.values.clone(), rotationVectorValues)
                    SensorManager.getRotationMatrixFromVector(rotationMatrix, rotationVectorValues)
                    SensorManager.remapCoordinateSystem(
                        rotationMatrix, SensorManager.AXIS_X, SensorManager.AXIS_Z,
                        remappedRotationMatrix
                    )
                    SensorManager.getOrientation(remappedRotationMatrix, orientations)
                    lastTime = currentTime
                }
            }
            if (event.sensor.type == Sensor.TYPE_LINEAR_ACCELERATION) {
                val x = event.values[0]
                val y = event.values[1]
                val z = event.values[2]
            }
            if (event.sensor.type == Sensor.TYPE_ACCELEROMETER) {
                val x = event.values[0]
                val y = event.values[1]
                val z = event.values[2]
            }
            if (event.sensor.type == Sensor.TYPE_GYROSCOPE) {
                val x = event.values[0]
                val y = event.values[1]
                val z = event.values[2]
            }
            if (event.sensor.type == Sensor.TYPE_MAGNETIC_FIELD) {
                val x = event.values[0]
                val y = event.values[1]
                val z = event.values[2]

//                Log.e("TAG","magnetic Field x: $x")
//                Log.e("TAG","magnetic Field y: $y")
//                Log.e("TAG","magnetic Field z: $z")
            }
            if (event.sensor.type == Sensor.TYPE_GRAVITY) {
                val x = event.values[0]
                val y = event.values[1]
                val z = event.values[2]
            }
        }
    }

    init {
        val sensorManager: SensorManager = context.applicationContext.getSystemService(Context.SENSOR_SERVICE) as SensorManager
        sensorManager.registerListener(listener, sensorManager.getDefaultSensor(Sensor.TYPE_GAME_ROTATION_VECTOR), 1000)
        sensorManager.registerListener(listener, sensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION), 1)
        sensorManager.registerListener(listener, sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER), 1)
        sensorManager.registerListener(listener, sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE), 1)
        sensorManager.registerListener(listener, sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD), 1)
        sensorManager.registerListener(listener, sensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY), 1)
    }

    private var captureListener: CaptureListener? = null

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
    private var highlight = 0.0f
    private var shadow = 0.0f

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
        val directory = File(Environment.getExternalStorageDirectory(), "Pro")
        if (!directory.exists()) {
            directory.mkdirs()
        }
        val file = File(directory.absolutePath, "${System.currentTimeMillis()}.jpg")
        if (!file.exists()) {
            file.createNewFile()
        }
        try {
            bos = BufferedOutputStream(FileOutputStream(file))
            val bmp =
                Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
            bmp.copyPixelsFromBuffer(pixelBuffer)
            bmp.compress(Bitmap.CompressFormat.JPEG, 100, bos)
            bmp.recycle()
            Log.d(TAG, "saveImage() Image captured")
            if (captureListener == null) {
                throw Exception("Set capture image listener")
            } else {
                captureListener?.onImageCaptured(file)
            }
        } catch (e: Exception) {
            Log.e(TAG, "saveImage() $e")
            if (captureListener == null) {
                throw Exception("Set capture image listener")
            } else {
                captureListener?.onCapturedFailed(e)
            }
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

        CameraInterface.onDrawFrame(texMatrix, saturation, contrast, brightness, highlight, shadow, orientations)

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

    fun changeHighlight(value: Float) {
        highlight = value
    }

    fun changeShadow(value: Float) {
        shadow = value
    }

    fun setCaptureListener(listener: CaptureListener?) {
        this.captureListener = listener
    }

}