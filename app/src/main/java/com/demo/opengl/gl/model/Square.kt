package com.demo.opengl.gl.model

import android.opengl.GLES20
import com.demo.opengl.gl.GLHelper
import com.demo.opengl.gl.Shader
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer
import java.nio.ShortBuffer

class Square {

    private val position = floatArrayOf(
        -0.5f, 0.5f, 0F,
        -0.5f, -0.5f, 0f,
        0.5f, -0.5f, 0f,
        0.5f, 0.5f, 0f
    )

    private val drawOrder = shortArrayOf(0, 1, 2, 0, 2, 3)

    private val positionBuffer: FloatBuffer = ByteBuffer.allocateDirect(position.size * 4).run {
        order(ByteOrder.nativeOrder())
        asFloatBuffer().apply {
            put(position)
            position(0)
        }
    }

    private val drawBuffer: ShortBuffer = ByteBuffer.allocateDirect(drawOrder.size * 2).run {
        order(ByteOrder.nativeOrder())
        asShortBuffer().apply {
            put(drawOrder)
            position(0)
        }
    }

    private val color = floatArrayOf(1f, 0.5f, 0.3f, 1.0f)

    private val vertexShader = "" +
            "attribute vec4 position;" +
            "void main()" +
            "{" +
            "gl_Position = position;" +
            "}"

    private val fragmentShader = "" +
            "uniform vec4 color;" +
            "void main()" +
            "{" +
            "gl_FragColor = color;" +
            "}"

    fun draw() {
        val program = GLES20.glCreateProgram().also {
            GLES20.glAttachShader(it, Shader.loadShader(GLES20.GL_VERTEX_SHADER, vertexShader))
            GLES20.glAttachShader(it, Shader.loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentShader))
            GLES20.glLinkProgram(it)
            GLES20.glUseProgram(it)
        }

        val positionHandle = GLES20.glGetAttribLocation(program, "position").also {
            GLES20.glEnableVertexAttribArray(it)
           GLHelper.call(Runnable {
               GLES20.glVertexAttribPointer(it, 3, GLES20.GL_FLOAT, false, 0, positionBuffer)
           })

            val colorHandle = GLES20.glGetUniformLocation(program, "color").also { colorH ->
                GLES20.glUniform4fv(colorH, 1, color, 0)
            }

            GLHelper.call(Runnable {
                GLES20.glDrawElements(GLES20.GL_TRIANGLES, drawOrder.size, GLES20.GL_UNSIGNED_SHORT, drawBuffer)
            })
            GLES20.glDisableVertexAttribArray(it)
        }

        GLES20.glDeleteShader(GLES20.GL_VERTEX_SHADER)
        GLES20.glDeleteShader(GLES20.GL_FRAGMENT_SHADER)
    }

}