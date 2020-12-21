package com.demo.opengl

import org.junit.Test

import org.junit.Assert.*

/**
 * Example local unit test, which will execute on the development machine (host).
 *
 * See [testing documentation](http://d.android.com/tools/testing).
 */
class ExampleUnitTest {
    @Test
    fun addition_isCorrect() {
        assertEquals(4, 2 + 2)
    }

    @Test
    fun test(){
        val ww = 1080
        val wh = 2340
        val cw = 1920
        val ch = 1080

        val war = wh.toFloat() / ww.toFloat()
        val car = ch.toFloat() / cw.toFloat()

        print("${war} $car")
    }
}