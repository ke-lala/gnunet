package org.gnu.gnunet.core

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.res.AssetManager
import android.os.Build
import android.os.Handler
import android.os.HandlerThread
import android.os.IBinder
import android.util.Log
import androidx.core.app.NotificationCompat
import org.gnu.gnunet.R
import org.gnu.gnunet.MainActivity
import org.gnunet.gnunetmessenger.ipc.IAccountCallback
import org.gnunet.gnunetmessenger.ipc.IChatCallback


object NativeBridge {
    init {
        System.loadLibrary("gnunet")
    }
    external fun stringFromJNI(assets: AssetManager, path: String): String
    @JvmStatic external fun initCfgFromAsset(assetManager: AssetManager, assetName: String): Long
    @JvmStatic external fun destroyCfg(cfgPtr: Long)
    @JvmStatic external fun nativeStartChatUsingCfg(cfgPtr: Long, appName: String, cb: IChatCallback): Long
    @JvmStatic external fun nativeDisconnect(sessionHandle: Long)
    @JvmStatic external fun nativeGetProfileName(sessionHandle: Long): String
    @JvmStatic external fun nativeSetProfileName(sessionHandle: Long, name: String)
    @JvmStatic external fun nativeCreateAccount(sessionHandle: Long, name: String): Int
    @JvmStatic external fun nativeConnect(sessionHandle: Long, accountKey: String, accountName: String)
    @JvmStatic external fun nativeIterateAccounts(sessionHandle: Long, cb: IAccountCallback)
}

class GnunetCoreService : Service() {

    companion object {
        private const val TAG = "GnunetCoreService"
        private const val CHANNEL_ID = "gnunet_core_channel"
        private const val NOTIF_ID = 1001
    }

    private lateinit var thread: HandlerThread
    private lateinit var handler: Handler

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()

        // Vordergrunddienst starten (ab Android 8 Pflicht, wenn dauerhaft läuft)
        startForeground(NOTIF_ID, buildOngoingNotification())

        // Eigener Looper-Thread für GNUnet
        thread = HandlerThread("gnunet-core")
        thread.start()
        handler = Handler(thread.looper)

        // GNUnet-Start in den Worker-Thread verschieben (niemals im UI-Thread!)
        handler.post {
            try {
                val res = NativeBridge.stringFromJNI(assets, filesDir.absolutePath)
                Log.d(TAG, "GNUnet started: $res")
            } catch (t: Throwable) {
                Log.e(TAG, "GNUnet start failed", t)
                stopSelf()
            }
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        // dauerhaft laufen lassen
        return START_STICKY
    }

    override fun onDestroy() {
        super.onDestroy()
        if (this::thread.isInitialized) {
            thread.quitSafely()
        }
    }

    override fun onBind(intent: Intent?): IBinder? = null

    // ---- Notification Helfer ----

    private fun buildOngoingNotification(): Notification {
        // Optional: Tap öffnet deine App
        val contentIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.mipmap.ic_launcher) // eigenes Icon verwenden
            .setContentTitle("GNUnet läuft")
            .setContentText("Der GNUnet-Dienst ist aktiv.")
            .setOngoing(true)
            .setContentIntent(contentIntent)
            .build()
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val mgr = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            val ch = NotificationChannel(
                CHANNEL_ID,
                "GNUnet Core",
                NotificationManager.IMPORTANCE_LOW
            )
            mgr.createNotificationChannel(ch)
        }
    }
}