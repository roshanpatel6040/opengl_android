package com.demo.opengl.ui

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.view.isVisible
import com.demo.opengl.R
import com.google.android.material.snackbar.Snackbar
import kotlinx.android.synthetic.main.activity_intro.*

class IntroActivity : AppCompatActivity() {

    companion object {
        private const val PERMISSION = 100
        private const val PERMISSION_RESULT = 101
    }

    private var handler = Handler(Looper.getMainLooper())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_intro)
        window.setFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS, WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS)

        if (!checkCamera()) {
            cl_permission.isVisible = true
            txt_info.text = getString(R.string.message_camera_not_supported)
            btn_permission.isVisible = false
            txt_launching.isVisible = false
            return
        }
        if (permissionGranted()) {
            openCameraScreen(2000)
        } else {
            cl_permission.isVisible = true
            txt_launching.isVisible = false
        }

        btn_permission.setOnClickListener {
            val perm = arrayOf(Manifest.permission.CAMERA, Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE)
            ActivityCompat.requestPermissions(this, perm, PERMISSION)
        }

        txt_launching.setOnClickListener {
            handler.removeCallbacksAndMessages(null)
            val intent = Intent(this, MainActivity::class.java)
            startActivity(intent)
            finishAffinity()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        handler.removeCallbacksAndMessages(null)
    }

    private fun openCameraScreen(delay: Long) {
        cl_permission.isVisible = false
        txt_launching.isVisible = true
        handler.postDelayed({
            val intent = Intent(this, CameraActivity::class.java)
            startActivity(intent)
            finishAffinity()
        }, delay)
    }

    private fun permissionGranted(): Boolean {
        return !(ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED || ActivityCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED ||
                ActivityCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED)

    }

    private fun checkCamera(): Boolean {
        return packageManager.hasSystemFeature(PackageManager.FEATURE_CAMERA_LEVEL_FULL)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == PERMISSION_RESULT) {
            if (permissionGranted()) {
                openCameraScreen(1000)
            }
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == PERMISSION) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED && grantResults[1] == PackageManager.PERMISSION_GRANTED && grantResults[2] == PackageManager.PERMISSION_GRANTED) {
                openCameraScreen(1000)
            } else {
                for (i in permissions) {
                    val boolean = ActivityCompat.shouldShowRequestPermissionRationale(this, i)
                    if (!boolean) {
                        val snackBar = Snackbar.make(findViewById(android.R.id.content), "Permission forcefully denied.", Snackbar.LENGTH_LONG)
                        snackBar.setAction("Go to settings.") {
                            val intent = Intent()
                            intent.action = Settings.ACTION_APPLICATION_DETAILS_SETTINGS
                            intent.data = Uri.parse("package:$packageName")
                            startActivityForResult(intent, PERMISSION_RESULT)
                        }
                        snackBar.show()
                        return
                    }
                }

                Snackbar.make(findViewById(android.R.id.content), "Permission denied.", Snackbar.LENGTH_LONG).show()
            }
        }
    }
}