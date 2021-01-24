package com.demo.opengl.gl

import android.opengl.GLES20
import android.util.Log

class GLHelper {

    companion object {

        private const val TAG = "GLHelper"

        fun clearError() {
            while (GLES20.glGetError() != GLES20.GL_NO_ERROR) {
            }
        }

        fun checkError() {
            var error = GLES20.GL_NO_ERROR
            while (GLES20.glGetError().also { error = it } != GLES20.GL_NO_ERROR) {
                Log.e(TAG, "GL Error $error")
            }
        }

        fun call(runnable: Runnable) {
            clearError()
            runnable.run()
            checkError()
        }

    }

}