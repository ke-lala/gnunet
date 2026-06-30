package org.gnu.gnunet.ipc

import android.app.*
import android.content.Intent
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat

class GnunetForegroundService : Service() {
    override fun onCreate() {
        super.onCreate()
        if (Build.VERSION.SDK_INT >= 26) {
            getSystemService(NotificationManager::class.java)
                .createNotificationChannel(NotificationChannel("gnunet_srv","GNUnet", NotificationManager.IMPORTANCE_LOW))
        }
        val notif = NotificationCompat.Builder(this, "gnunet_srv")
            .setContentTitle("GNUnet läuft")
            .setContentText("Bereit für Client-Verbindungen")
            .setSmallIcon(android.R.drawable.stat_sys_download_done)
            .build()
        startForeground(1, notif)
    }
    override fun onBind(intent: Intent?): IBinder? = null
}