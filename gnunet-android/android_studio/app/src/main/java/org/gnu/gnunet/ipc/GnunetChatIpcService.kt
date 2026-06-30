package org.gnu.gnunet.ipc

import android.app.Service
import android.content.Intent
import android.os.IBinder
import android.os.RemoteException
import kotlinx.coroutines.*
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.channels.Channel
import org.gnunet.gnunetmessenger.ipc.ChatAccountDto
import org.gnunet.gnunetmessenger.ipc.ChatContextDto
import org.gnunet.gnunetmessenger.ipc.ChatMessageDto
import org.gnunet.gnunetmessenger.ipc.IAccountCallback
import org.gnunet.gnunetmessenger.ipc.IChatCallback
import org.gnunet.gnunetmessenger.ipc.IGnunetChat
import android.util.Log
import org.gnu.gnunet.core.NativeBridge

class GnunetChatIpcService : Service() {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Default)
    private val sessions = mutableMapOf<Long, Session>()
    private var nextHandle = 1L
    @Volatile private var cfgPtr: Long = 0L

    data class Session(
        val handle: Long,
        val cb: IChatCallback,
        val chan: Channel<Pair<ChatContextDto, ChatMessageDto>>,
        val worker: Job,
        var profileName: String = "GNUnet",
        var connected: Boolean = false,
        val accounts: MutableList<ChatAccountDto> = mutableListOf()
    )

    companion object {
        private const val TAG = "GnunetChatIpcService"
        private const val OK = 0
        private const val ERR_NO_SESSION = 1
    }

    override fun onCreate() {
        super.onCreate()
        // Optional: dauerhaft im Hintergrund
        // startService(Intent(this, GnunetForegroundService::class.java))
    }

    override fun onBind(intent: Intent): IBinder {
        return object : IGnunetChat.Stub() {
            override fun getApiVersion(): Int = 100

            override fun startChat(messengerApp: String, cb: IChatCallback): Long {
                applicationContext.enforceAllowedCaller() // ggf. auskommentieren bis Whitelist steht

                // 1) Channel + Worker: entkoppelt native Events vom Binder zum Client
                val handle = nextHandle++  // lokaler (Java/Kotlin) Handle – wir überschreiben ihn gleich mit dem nativen, wenn vorhanden
                val chan = Channel<Pair<ChatContextDto, ChatMessageDto>>(
                    capacity = 256,
                    onBufferOverflow = BufferOverflow.DROP_OLDEST
                )
                val worker = scope.launch(Dispatchers.IO) {
                    for ((ctx, msg) in chan) {
                        val ok = try {
                            cb.onMessage(ctx, msg)  // an den echten Client-Binder weiterreichen
                            true
                        } catch (t: Throwable) {
                            Log.e(TAG, "Remote callback to client failed", t)
                            false
                        }
                        if (!ok) {
                            stopSession(handle)     // Session aufräumen, wenn Client tot ist
                            break
                        }
                    }
                }

                // 2) Interner Callback für die NATIVEN Events: schiebt nur in den Channel
                val nativeCb = object : IChatCallback.Stub() {
                    override fun onMessage(context: ChatContextDto, message: ChatMessageDto) {
                        // leichtgewichtig, kein UI, kein Binder zurück zum Client
                        if (!chan.trySend(context to message).isSuccess) {
                            Log.w(TAG, "Inbound buffer full; dropping oldest")
                        }
                    }
                }

                val cfg = try { ensureCfg() }
                catch (t: Throwable) {
                    Log.e(TAG, "ensureCfg failed", t)
                    worker.cancel()
                    chan.close()
                    return 0L
                }

                // 4) Native Start – nutzt dieselbe Config und registriert unseren internen Callback
                val nativeHandle = try {
                    NativeBridge.nativeStartChatUsingCfg(cfg, messengerApp, nativeCb)
                } catch (t: Throwable) {
                    Log.e(TAG, "nativeStartChatUsingCfg failed", t)
                    NativeBridge.destroyCfg(cfg)
                    worker.cancel()
                    chan.close()
                    return 0L
                } finally {
                    // Falls du die cfg NICHT global wiederverwenden willst, hier freigeben:
                    NativeBridge.destroyCfg(cfg)
                }

                if (nativeHandle == 0L) {
                    Log.e(TAG, "GNUNET_CHAT_start returned 0 (failed to start)")
                    worker.cancel()
                    chan.close()
                    return 0L
                }

                // 5) Session registrieren (wir speichern den NATIVEN Handle)
                sessions[nativeHandle] = Session(
                    handle = nativeHandle,
                    cb = cb,                    // Binder zum Client
                    chan = chan,                // Inbound-Puffer
                    worker = worker             // Forwarder-Job
                    // profileName / connected / accounts bleiben wie bei dir
                )

                try {
                    cb.asBinder().linkToDeath(
                        { stopSession(handle) },  // wird aufgerufen, wenn Client stirbt
                        0
                    )
                } catch (t: RemoteException) {
                    // falls der Binder schon tot war
                    stopSession(handle)
                }

                Log.d(TAG, "startChat ok -> nativeHandle=$nativeHandle, app=$messengerApp")
                return nativeHandle
            }

            override fun iterateAccounts(handle: Long, cb: IAccountCallback) {
                Log.d(TAG, "iterateAccounts(handle=$handle)")

                applicationContext.enforceAllowedCaller()
                Log.d(TAG, "hasSession=${sessions.containsKey(handle)}")
                val sess = sessions[handle]
                if (sess == null) {
                    cb.onError(404, "Unknown handle")
                    return
                }

                // Nicht auf dem Main-Thread iterieren
                scope.launch(Dispatchers.IO) {
                    try {
                        // TODO: Hier die echte GNUnet-Iteration aufrufen.
                        // Fürs erste: Dummy oder aus deiner GNUnet-Brücke lesen.
                        // Beispiel-Dummy:
                        val list = listOf(
                            ChatAccountDto().apply { key="acc-1"; name="Alice" },
                            ChatAccountDto().apply { key="acc-2"; name="Bob" }
                        )

                        for (dto in list) {
                            try {
                                cb.onAccount(dto) // oneway: non-blocking
                            } catch (t: Throwable) {
                                Log.e(TAG, "iterateAccounts: callback failed", t)
                                cb.onError(500, "callback failed: ${t.message}")
                                return@launch
                            }
                        }
                        cb.onDone()
                    } catch (t: Throwable) {
                        Log.e(TAG, "iterateAccounts failed", t)
                        cb.onError(500, t.message ?: "unknown error")
                    }
                }
            }



            override fun createAccount(handle: Long, name: String): Int {
                Log.d(TAG, "createAccount(handle=$handle)")
                applicationContext.enforceAllowedCaller()

                val sess = sessions[handle] ?: return ERR_NO_SESSION

                // Später: echten GNUnet-Account erstellen (JNI) und Ergebnis zurückgeben.
                // Vorläufig: "Account" lokal vormerken
                val account = ChatAccountDto().apply {
                    key = "acc:${System.currentTimeMillis()}"
                    this.name = name
                }
                sess.accounts += account

                // Beispiel: optional dem Client via Callback signalisieren (nicht zwingend)
                scope.launch(Dispatchers.IO) {
                    runCatching {
                        val ctx = ChatContextDto().apply {
                            chatContextType = 0; userPointer = null; isGroup = false; isPlatform = false
                        }
                        val msg = ChatMessageDto().apply {
                            text = "Account '$name' erstellt"
                            timestamp = System.currentTimeMillis()
                            kind = 0; type = -1; senderKey = "server"
                        }
                        sess.chan.trySend(ctx to msg)
                        Log.d(TAG, "createAccount send")
                    }
                }

                return OK
            }

            override fun connect(handle: Long, account: ChatAccountDto?) {
                applicationContext.enforceAllowedCaller()
                val sess = sessions[handle] ?: return
                // Hier später GNUNET_CHAT_connect(handle, account) aufrufen
                sess.connected = true
                // Optional: Event rausschicken
                val ctx = ChatContextDto().apply {
                    chatContextType = 0; userPointer = null; isGroup = false; isPlatform = false
                }
                val msg = ChatMessageDto().apply {
                    text = "Verbunden mit Account: ${account?.name ?: "(null)"}"
                    timestamp = System.currentTimeMillis()
                    kind = 0; type = -1; senderKey = "server"
                }
                scope.launch { sess.chan.trySend(ctx to msg) }
            }

            override fun disconnect(handle: Long) {
                applicationContext.enforceAllowedCaller()
                stopSession(handle) // bestehende Utility bei dir, die worker cancelt, chan schließt, Map-Eintrag entfernt
            }

            override fun getProfileName(handle: Long): String {
                applicationContext.enforceAllowedCaller()
                val sess = sessions[handle] ?: return ""
                return sess.profileName
            }

            override fun setProfileName(handle: Long, name: String) {
                applicationContext.enforceAllowedCaller()
                val sess = sessions[handle] ?: return
                sess.profileName = name

                // optional: Event an Client
                scope.launch(Dispatchers.IO) {
                    runCatching {
                        val ctx = ChatContextDto().apply {
                            chatContextType = 0; userPointer = null; isGroup = false; isPlatform = false
                        }
                        val msg = ChatMessageDto().apply {
                            text = "Profilname gesetzt: $name"
                            timestamp = System.currentTimeMillis()
                            kind = 0; type = -1; senderKey = "server"
                        }
                        sess.chan.trySend(ctx to msg)
                    }
                }
            }
        }
    }

    private fun stopSession(handle: Long) {
        val sess = sessions.remove(handle) ?: return // idempotent
        try {
            // 1) JNI: native Seite trennen & Session freigeben
            runCatching { NativeBridge.nativeDisconnect(handle) }
                .onFailure { Log.w(TAG, "nativeDisconnect failed for $handle", it) }

            // 2) Worker/Channel schließen
            sess.worker.cancel()
            sess.chan.close()

        } catch (t: Throwable) {
            Log.w(TAG, "stopSession cleanup error", t)
        }
    }

    override fun onDestroy() {
        val ptr = cfgPtr
        if (ptr != 0L) {
            runCatching { NativeBridge.destroyCfg(ptr) }
            cfgPtr = 0L
        }
        sessions.keys.toList().forEach { stopSession(it) }
        scope.cancel()
        super.onDestroy()
    }

    // --- Öffentliche Hilfsfunktion für deine GNUnet-Bridge ---
    fun emitToHandle(handle: Long, ctx: ChatContextDto, msg: ChatMessageDto) {
        sessions[handle]?.chan?.trySend(ctx to msg)
    }

    private fun ensureCfg(): Long {
        var local = cfgPtr
        if (local != 0L) return local
        synchronized(this) {
            if (cfgPtr == 0L) {
                cfgPtr = NativeBridge.initCfgFromAsset(applicationContext.assets, "gnunet.conf")
                require(cfgPtr != 0L) { "initCfgFromAsset failed (cfgPtr==0)" }
            }
            return cfgPtr
        }
    }
}