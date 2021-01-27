package com.demo.opengl.ui

import android.os.Bundle
import android.view.View
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import androidx.core.net.toUri
import com.demo.opengl.R
import com.demo.opengl.utils.Constants
import kotlinx.android.synthetic.main.activity_preview.*
import java.io.File

class PreviewActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS, WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        val flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or View.SYSTEM_UI_FLAG_FULLSCREEN or
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        window.decorView.systemUiVisibility = flags
        setContentView(R.layout.activity_preview)
        getBundle()
    }

    fun getBundle() {
        if (intent.hasExtra(Constants.PREVIEW_PATH)) {
            val image = intent.getStringExtra(Constants.PREVIEW_PATH)
            if (image != null) {
                img_preview.setImageURI(File(image).toUri())
            }
        }
    }
}