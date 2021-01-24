package com.demo.opengl.gl

import android.opengl.GLES20

class Shader {

    companion object {

        fun loadShader(type: Int, shaderSource: String): Int {
            return GLES20.glCreateShader(type).also {
                GLES20.glShaderSource(it, shaderSource)
                GLES20.glCompileShader(it)
            }
        }

    }

}