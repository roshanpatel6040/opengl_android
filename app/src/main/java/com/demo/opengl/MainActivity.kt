package com.demo.opengl

import android.content.res.AssetManager
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle

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