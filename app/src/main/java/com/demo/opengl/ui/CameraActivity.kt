package com.demo.opengl.ui

import android.os.Bundle
import android.view.View
import android.view.WindowManager
import android.widget.SeekBar
import androidx.appcompat.app.AppCompatActivity
import com.demo.opengl.R
import com.demo.opengl.provider.CameraInterface
import com.google.android.material.bottomsheet.BottomSheetBehavior
import kotlinx.android.synthetic.main.activity_camera.*


class CameraActivity : AppCompatActivity(), SeekBar.OnSeekBarChangeListener, View.OnClickListener {

    companion object {
        private const val EXPOSURE = "C_E"
    }

    init {
        System.loadLibrary("cameraCpp")
    }

    lateinit var sheet: BottomSheetBehavior<View>

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS, WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        val flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or View.SYSTEM_UI_FLAG_FULLSCREEN or
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        window.decorView.systemUiVisibility = flags
        setContentView(R.layout.activity_camera)

        initView()
    }

    private fun initView() {
        CameraInterface.initialize()
        sheet = BottomSheetBehavior.from(ll_bottomSheet)
        sheet.addBottomSheetCallback(bottomSheetCallback)
        cv_capture.setOnClickListener(this)
        seekBar_Brightness.setOnSeekBarChangeListener(this)
        seekBar_Contrast.setOnSeekBarChangeListener(this)
        seekBar_saturation.setOnSeekBarChangeListener(this)
    }

    override fun onResume() {
        super.onResume()
        surface.onResume()
    }

    override fun onPause() {
        super.onPause()
        surface.onPause()
    }

    override fun onDestroy() {
        super.onDestroy()
        CameraInterface.destroy()
        sheet.removeBottomSheetCallback(bottomSheetCallback)
    }

    override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
        when (seekBar?.id) {
            R.id.seekBar_saturation -> {
                surface.changeSaturation(progress.div(10.0f))
            }
            R.id.seekBar_Contrast -> {
                surface.changeContrast(progress.div(10.0f))
            }
            R.id.seekBar_Brightness -> {
                surface.changeBrightness(progress.div(10.0f).minus(0.5f))
            }
//            EXPOSURE -> {
//                CameraInterface.changeExposure(seekBar.progress)
//            }
        }
    }

    override fun onStartTrackingTouch(seekBar: SeekBar?) {
    }

    override fun onStopTrackingTouch(seekBar: SeekBar?) {
    }

    private var bottomSheetCallback = object : BottomSheetBehavior.BottomSheetCallback() {
        override fun onSlide(bottomSheet: View, slideOffset: Float) {

        }

        override fun onStateChanged(bottomSheet: View, newState: Int) {

        }
    }

    override fun onClick(v: View) {
        when (v.id) {
            R.id.cv_capture -> {
                surface.capture(true)
            }
        }
    }
}