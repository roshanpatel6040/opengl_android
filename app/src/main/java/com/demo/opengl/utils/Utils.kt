package com.demo.opengl.utils

import android.content.Context
import android.content.res.AssetManager
import android.util.Log
import java.io.*
import java.nio.file.Path

class Utils {

    companion object {
        fun copyAssets(context: Context, path: String) {
            val baseDirectory = File(context.getExternalFilesDir(null), "models")
            if (!baseDirectory.exists()) {
                baseDirectory.mkdirs()
            }
            val assetManager: AssetManager = context.assets
            var files: Array<String>? = null
            try {
                files = assetManager.list(path)
            } catch (e: IOException) {
                Log.e("tag", "Failed to get asset file list.", e)
            }
            if (files != null) for (filename in files) {
                if (!filename.contains(".")) {
                    copyAssets(context, "$path/$filename")
                    continue
                }
                var `in`: InputStream? = null
                var out: OutputStream? = null
                try {
                    `in` = assetManager.open("$path/$filename")
                    val outFile = File(baseDirectory, filename)
                    out = FileOutputStream(outFile)
                    copyFile(`in`, out)
                } catch (e: IOException) {
                    Log.e("tag", "Failed to copy asset file: $filename", e)
                } finally {
                    if (`in` != null) {
                        try {
                            `in`.close()
                        } catch (e: IOException) {
                            // NOOP
                        }
                    }
                    if (out != null) {
                        try {
                            out.close()
                        } catch (e: IOException) {
                            // NOOP
                        }
                    }
                }
            }
        }

        @kotlin.jvm.Throws(IOException::class)
        private fun copyFile(`in`: InputStream, out: OutputStream) {
            val buffer = ByteArray(1024)
            var read: Int
            while (`in`.read(buffer).also { read = it } != -1) {
                out.write(buffer, 0, read)
            }
        }
    }

}