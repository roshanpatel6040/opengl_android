package com.demo.opengl.ui

import android.os.Bundle
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
        setContentView(R.layout.activity_preview)
        window.setFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS, WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        getBundle()
    }

    private fun getBundle() {
        if (intent.hasExtra(Constants.PREVIEW_PATH)) {
            val image = intent.getStringExtra(Constants.PREVIEW_PATH)
            if (image != null) {
                img_preview.setImageURI(File(image).toUri())
            }
        }
    }
}