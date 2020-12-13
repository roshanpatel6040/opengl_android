package com.demo.opengl

import android.opengl.GLES20
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer

class Triangle {

    private var position = floatArrayOf(
        0.0f, 0.622008459f, 0.0f,
        -0.5f, -0.311004243f, 0.0f,
        0.5f, -0.311004243f, 0.0f
    )

    private var vertexBuffer: FloatBuffer = ByteBuffer.allocateDirect(position.size * 4).run {
        order(ByteOrder.nativeOrder())
        asFloatBuffer().apply {
            put(position)
            position(0)
        }
    }

    private var color = floatArrayOf(1F, 1F, 1F, 1F)

    private var colorBuffer: FloatBuffer = ByteBuffer.allocateDirect(color.size * 4).run {
        order(ByteOrder.nativeOrder())
        asFloatBuffer().apply {
            put(color)
            position(0)
        }
    }

    private var vertexShader = "" +
            "attribute vec4 position;" +
            "void main()" +
            "{" +
            " gl_Position = position;" +
            "}"

    private var fragmentShader = "" +
            "uniform vec4 color;" +
            "void main()" +
            "{" +
            " gl_FragColor = color;" +
            "}"

    fun draw(viewMatrix: FloatArray) {
        val program = GLES20.glCreateProgram()
        GLES20.glAttachShader(program, Shader.loadShader(GLES20.GL_VERTEX_SHADER, vertexShader))
        GLES20.glAttachShader(program, Shader.loadShader(GLES20.GL_VERTEX_SHADER, vertexShader))
        GLES20.glAttachShader(program, Shader.loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentShader))
        GLES20.glLinkProgram(program)
        GLES20.glUseProgram(program)

        GLES20.glGetAttribLocation(program, "position").also {
            GLES20.glEnableVertexAttribArray(it)
            GLHelper.call(Runnable {
                GLES20.glVertexAttribPointer(it, 3, GLES20.GL_FLOAT, true, 0, vertexBuffer)
            })

            GLES20.glGetUniformLocation(program, "color").also { colorH ->
                GLES20.glUniform4fv(colorH, 1, color, 0)
                if (color[0] == 0.5f) {
                    color[0] += 0.5f
                } else {
                    color[0] -= 0.5f
                }
            }
            GLHelper.call(Runnable { GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, 3) })
            GLES20.glDisableVertexAttribArray(it)
        }
        GLES20.glDeleteShader(GLES20.GL_VERTEX_SHADER)
        GLES20.glDeleteShader(GLES20.GL_FRAGMENT_SHADER)
    }
}