package com.demo.opengl.provider

import java.io.File

interface CaptureListener {

    fun onImageCaptured(imageFile: File)

    fun onCapturedFailed(e: Exception)

}