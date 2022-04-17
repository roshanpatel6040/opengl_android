package com.demo.opengl.provider

import android.content.Context
import android.graphics.Bitmap
import android.graphics.SurfaceTexture
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.media.MediaPlayer
import android.opengl.GLES11Ext
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.opengl.Matrix
import android.os.Environment
import android.util.Log
import android.view.Surface
import android.view.WindowManager
import com.demo.opengl.R
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

    private lateinit var windowManager: WindowManager
    private lateinit var mediaPlayer: MediaPlayer

    private val rotationMatrix = FloatArray(16)
    private val remappedRotationMatrix = FloatArray(16)
    private val orientations = FloatArray(3)
    private var rotationVectorValues = FloatArray(5)

    private var oldTime: Long = 0
    private val acceleration = FloatArray(3)
    private val velocity = FloatArray(3)
    private val gravity = FloatArray(3)
    private val distance = FloatArray(3)

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
            if (event.sensor.type == Sensor.TYPE_ROTATION_VECTOR) {
                updateOrientation(event.values)
                //                val currentTime = System.currentTimeMillis()
                //                if ((currentTime - lastTime) > 0) {
                //                    rotationVectorValues = lowPass(event.values.clone(), rotationVectorValues)
                //                    SensorManager.getRotationMatrixFromVector(rotationMatrix, rotationVectorValues)
                //                    SensorManager.remapCoordinateSystem(
                //                        rotationMatrix, SensorManager.AXIS_X, SensorManager.AXIS_Z,
                //                        remappedRotationMatrix
                //                    )
                //                    SensorManager.getOrientation(remappedRotationMatrix, orientations)
                //                    lastTime = currentTime
                //                }
            }
            if (event.sensor.type == Sensor.TYPE_LINEAR_ACCELERATION) {
                val x = event.values[0]
                val y = event.values[1]
                val z = event.values[2]
            }
            if (event.sensor.type == Sensor.TYPE_ACCELEROMETER) {
                gravity[0] = ALPHA * gravity[0] + (1 - ALPHA) * event.values[0]
                gravity[1] = ALPHA * gravity[1] + (1 - ALPHA) * event.values[1]
                gravity[2] = ALPHA * gravity[2] + (1 - ALPHA) * event.values[2]

                // Remove the gravity contribution with the high-pass filter.

                // Remove the gravity contribution with the high-pass filter.
                acceleration[0] = event.values[0] - gravity[0]
                acceleration[1] = event.values[1] - gravity[1]
                acceleration[2] = event.values[2] - gravity[2]

                val dtSeconds: Float = (event.timestamp - oldTime) / 1000000000.0f
                oldTime = event.timestamp

                velocity[0] = velocity[0] + acceleration[0] * dtSeconds
                velocity[1] = velocity[1] + acceleration[1] * dtSeconds
                velocity[2] = velocity[2] + acceleration[2] * dtSeconds

                distance[0] = velocity[0] * dtSeconds
                distance[1] = velocity[1] * dtSeconds
                distance[2] = velocity[2] * dtSeconds
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

    fun updateOrientation(rotationVector: FloatArray) {
        val rotationMatrix = FloatArray(9)
        SensorManager.getRotationMatrixFromVector(rotationMatrix, rotationVector)
        var axisX: Int? = null
        var axisY: Int? = null
        when (windowManager.defaultDisplay.rotation) {
            Surface.ROTATION_0 -> {
                axisX = SensorManager.AXIS_X
                axisY = SensorManager.AXIS_Z
            }
            Surface.ROTATION_90 -> {
                axisX = SensorManager.AXIS_Z
                axisY = SensorManager.AXIS_MINUS_X
            }
            Surface.ROTATION_180 -> {
                axisX = SensorManager.AXIS_MINUS_X
                axisY = SensorManager.AXIS_MINUS_Z
            }
            Surface.ROTATION_270 -> {
                axisX = SensorManager.AXIS_Z
                axisY = SensorManager.AXIS_X
            }
        }

        val adjustedRotatedMatrix = FloatArray(9)
        SensorManager.remapCoordinateSystem(rotationMatrix, axisX!!, axisY!!, adjustedRotatedMatrix)

//        val orientation = FloatArray(3)
        SensorManager.getOrientation(adjustedRotatedMatrix, orientations)
        val radToDegree = (-180.0f / Math.PI)
        val yaw = orientations[0] * radToDegree
        val pitch = orientations[1] * radToDegree
        val roll = orientations[2] * radToDegree

    }

    init {
        val sensorManager: SensorManager =
            context.applicationContext.getSystemService(Context.SENSOR_SERVICE) as SensorManager
        windowManager =
            context.applicationContext.getSystemService(Context.WINDOW_SERVICE) as WindowManager
        sensorManager.registerListener(
            listener,
            sensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR),
            1000
        )
        sensorManager.registerListener(
            listener,
            sensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION),
            1
        )
        sensorManager.registerListener(
            listener,
            sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
            1
        )
        sensorManager.registerListener(
            listener,
            sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE),
            1
        )
        sensorManager.registerListener(
            listener,
            sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD),
            1
        )
        sensorManager.registerListener(
            listener,
            sensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY),
            1
        )

        mediaPlayer = MediaPlayer.create(context, R.raw.coin)
        mediaPlayer.isLooping = true
    }

    private var captureListener: CaptureListener? = null

    private var capture = false

    private lateinit var surfaceTexture: SurfaceTexture
    private lateinit var videoSurfaceTexture: SurfaceTexture
    private val texMatrix = FloatArray(16)
    private val videoTexMatrix = FloatArray(16)

    @Volatile
    private var frameAvailable: Boolean = false

    @Volatile
    private var videoFrameAvailable: Boolean = false

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
        val directory = File(context.getExternalFilesDir(null), "Pro")
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

            if (videoFrameAvailable) {
                videoSurfaceTexture.updateTexImage()
            videoSurfaceTexture.getTransformMatrix(videoTexMatrix)
                videoFrameAvailable = false
            }
        }
        CameraInterface.onDrawFrame(
            texMatrix,
            videoTexMatrix,
            saturation,
            contrast,
            brightness,
            highlight,
            shadow,
            orientations,
            distance
        )

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
        val textureBuffer = IntArray(2)
        GLES20.glGenTextures(2, textureBuffer, 0)
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureBuffer[0])
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureBuffer[1])

        surfaceTexture = SurfaceTexture(textureBuffer[0])
        surfaceTexture.setDefaultBufferSize(cameraWidth, cameraHeight)
        surfaceTexture.setOnFrameAvailableListener {
            synchronized(lock) {
                frameAvailable = true
            }
        }

        val surface = Surface(surfaceTexture)

        videoSurfaceTexture = SurfaceTexture(textureBuffer[1])
        videoSurfaceTexture.setDefaultBufferSize(mediaPlayer.videoWidth, mediaPlayer.videoHeight)
        videoSurfaceTexture.setOnFrameAvailableListener {
            synchronized(lock) {
                videoFrameAvailable = true
            }
        }

        // val videoSurface = Surface(videoSurfaceTexture)
        // mediaPlayer.setSurface(videoSurface)
        // mediaPlayer.start()

        CameraInterface.onSurfaceCreated(textureBuffer, surface, cameraWidth, cameraHeight)
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