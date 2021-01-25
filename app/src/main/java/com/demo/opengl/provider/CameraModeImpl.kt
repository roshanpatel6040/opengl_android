package com.demo.opengl.provider

class CameraModeImpl : CameraMode {

    private val cameraSettingsHolder: CameraSettingsHolder = CameraSettingsHolder()

    override fun changeMode(type: Int) {
        cameraSettingsHolder.mode = type
        CameraInterface.changeMode(type)
    }

    fun getSettings(): CameraSettingsHolder {
        return cameraSettingsHolder
    }

}