package com.demo.opengl.ui

import android.os.Bundle
import android.view.View
import android.view.WindowManager
import android.widget.SeekBar
import androidx.appcompat.app.AppCompatActivity
import com.demo.opengl.R
import com.demo.opengl.provider.CameraInterface
import com.demo.opengl.provider.CameraModeImpl
import com.demo.opengl.provider.ProviderConst
import com.google.android.material.bottomsheet.BottomSheetBehavior
import kotlinx.android.synthetic.main.activity_camera.*

class CameraActivity : AppCompatActivity(), SeekBar.OnSeekBarChangeListener, View.OnClickListener {

    init {
        System.loadLibrary("cameraCpp")
    }

    lateinit var sheet: BottomSheetBehavior<View>

    private val cameraModeImpl: CameraModeImpl by lazy { CameraModeImpl() }

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

        // Set camera mode to auto initially
        changeMode(ProviderConst.AUTO_MODE)

        cv_capture.setOnClickListener(this)
        seekBar_Brightness.setOnSeekBarChangeListener(this)
        seekBar_Contrast.setOnSeekBarChangeListener(this)
        seekBar_saturation.setOnSeekBarChangeListener(this)
        seekBar_highlight.setOnSeekBarChangeListener(this)
        seekBar_shadow.setOnSeekBarChangeListener(this)
        btn_mode.setOnClickListener(this)
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

    /**
     * Change camera mode
     * @param type camera mode [ProviderConst]
     */
    private fun changeMode(mode: Int) {
        when (mode) {
            ProviderConst.AUTO_MODE -> {
                btn_mode.text = getString(R.string.label_auto)
            }
            ProviderConst.DETECTION_MODE -> {
                btn_mode.text = getString(R.string.label_detection)
            }
        }
        cameraModeImpl.changeMode(mode)
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
            R.id.seekBar_highlight -> {
                surface.changeHighlight(progress.div(10.0f))
            }
            R.id.seekBar_shadow -> {
                surface.changeShadow(progress.div(10.0f))
            }
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
            R.id.btn_mode -> {
                var mode = cameraModeImpl.getSettings().mode
                if (mode == ProviderConst.AUTO_MODE) {
                    mode = ProviderConst.DETECTION_MODE
                } else if (mode == ProviderConst.DETECTION_MODE) {
                    mode = ProviderConst.AUTO_MODE
                }
                changeMode(mode)
            }
        }
    }
}