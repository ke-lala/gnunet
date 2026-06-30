package org.gnu.gnunet

import android.content.Intent
import android.content.res.AssetManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import org.gnu.gnunet.core.GnunetCoreService
import org.gnu.gnunet.databinding.ActivityMainBinding
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.io.InputStream

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val dataPath: String = filesDir.getAbsolutePath()
        /*try {
            System.loadLibrary("gnunetutil") // Muss ganz früh geladen sein
            Log.d("DEBUG", "libgnunetutil.so geladen")
        } catch (e: UnsatisfiedLinkError) {
            Log.e("DEBUG", "Fehler beim Laden libgnunetutil", e)
        }

        try {
            System.loadLibrary("gnunet_plugin_peerstore_sqlite") // Muss ganz früh geladen sein
            Log.d("DEBUG", "libgnunet_plugin_peerstore_sqlite.so geladen")
        } catch (e: UnsatisfiedLinkError) {
            Log.e("DEBUG", "Fehler beim Laden libgnunet_plugin_peerstore_sqlite", e)
        }*/

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        var isSuccess = copyFileFromAssetsToInternalStorage("plugins/libgnunet_plugin_peerstore_sqlite.so", "libgnunet_plugin_peerstore_sqlite.so")
        if (isSuccess) {
            Log.d("GNUNET", "onCreate: file libgnunet_plugin_peerstore_sqlite.so copied successfully")
        } else {
            Log.e("GNUNET", "onCreate: file libgnunet_plugin_peerstore_sqlite.so not copied :(")
        }

        isSuccess = copyFileFromAssetsToInternalStorage("private_key.ecc", "private_key.ecc")
        if (isSuccess) {
            Log.d("GNUNET", "onCreate: file private_key.ecc copied successfully")
        } else {
            Log.e("GNUNET", "onCreate: file private_key.ecc not copied :(")
        }

        Log.d("GNUNET", "is asset null? assets = $assets")
        // Example of a call to a native method
        val intent = Intent(this, GnunetCoreService::class.java)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent)
        } else {
            startService(intent)
        }
        //binding.sampleText.text = stringFromJNI(assets, dataPath)
    }

    private fun copyFileFromAssetsToInternalStorage(fileName: String, outputFileName: String): Boolean {
        var inputStream: InputStream? = null
        var outputStream: FileOutputStream? = null

        return try {
            // Access the file from the assets
            inputStream = assets.open(fileName)

            // Create the destination file in internal storage
            val outputFile = File(filesDir, outputFileName)
            Log.d("GNUNET", outputFile.path)
            outputStream = FileOutputStream(outputFile)

            // Buffer to hold data during copy
            val buffer = ByteArray(1024)
            var length: Int

            // Copy the file contents from assets to the destination file
            while (inputStream.read(buffer).also { length = it } > 0) {
                outputStream.write(buffer, 0, length)
            }

            outputFile.setReadable(true, false)
            outputFile.setExecutable(true, false)
            //Success
            true
        } catch (e: IOException) {
            e.printStackTrace()

            // Failure
            false
        } finally {
            inputStream?.close()
            outputStream?.close()
        }
    }
    /**
     * A native method that is implemented by the 'gnunet' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(assets: AssetManager,
                               path: String): String

    companion object {
        // Used to load the 'gnunet' library on application startup.
        init {
            /*try {


            System.loadLibrary("sqliteX");
            System.loadLibrary("gnunetsq")
            System.loadLibrary("gnunetutil")
            System.loadLibrary("gnunetblock")
            System.loadLibrary("gnunetregexblock")
            System.loadLibrary("gnunetblockgroup")
            System.loadLibrary("gnunetgnsrecord")
            System.loadLibrary("gnunetjson")
            System.loadLibrary("gnunethello")
            System.loadLibrary("gnunet_plugin_peerstore_sqlite");
            System.loadLibrary("gnunet")
            } catch (e: UnsatisfiedLinkError) {
                Log.e("NativeLoader", "Failed to load native library", e);
            }*/
            System.loadLibrary("gnunetti")
        }
    }
}