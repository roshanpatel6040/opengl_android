package com.demo.opengl.ui

import android.content.res.AssetManager
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.demo.opengl.gl.GlSurface

class MainActivity : AppCompatActivity() {

    init {
        System.loadLibrary("opengl")
    }

    private var surface: GlSurface? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        assetsManager(assets)
        surface = GlSurface(this)
        setContentView(surface)
    }

    override fun onResume() {
        super.onResume()
        surface?.onResume()
    }

    override fun onPause() {
        super.onPause()
        surface?.onPause()
    }

    private external fun assetsManager(manager : AssetManager)

}