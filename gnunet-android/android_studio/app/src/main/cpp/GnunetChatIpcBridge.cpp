#include <jni.h>
#include <map>
#include <mutex>
#include <memory>
#include <atomic>
#include <string>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "gnunet_chat_lib.h"
#include "gnunet_util_lib.h"


#define LOG_TAG "GnunetChatIpcBridge"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ======================================================
// Globale Verwaltung
// ======================================================

static JavaVM* g_vm = nullptr;

// Session-Map: handle (jlong) -> ChatSession
static std::mutex g_mapMtx;
struct ChatSession;
static std::map<long, std::unique_ptr<ChatSession>> g_sessions;
static std::atomic_long g_nextHandle{1};

static const struct GNUNET_CONFIGURATION_Handle* g_cfg = nullptr;
static std::atomic_bool g_cfgInited{false};

// Hilfsfunktion: Thread-sicheres JNIEnv-Beschaffen + optionales Detach
struct ScopedEnv {
    JNIEnv* env = nullptr;
    bool didAttach = false;

    ScopedEnv() {
        if (!g_vm) return;
        if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || !env) {
            if (g_vm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
                didAttach = true;
            } else {
                env = nullptr;
            }
        }
    }
    ~ScopedEnv() {
        if (didAttach && g_vm) {
            g_vm->DetachCurrentThread();
        }
    }
    JNIEnv* get() const { return env; }
};

// ======================================================
// Session-Struktur mit GNUnet-Handle
// ======================================================

struct ChatSession {
    jlong handle = 0;
    // Optional: Java-Callback-Objekt (IChatCallback Implementierung aus deinem Service)
    jobject cbGlobal = nullptr;

    // Beispielhafte Session-Daten
    std::string profileName = "GNUnet";
    bool connected = false;

    // GNUnet Chat Handle
    struct GNUNET_CHAT_Handle* chatHandle = nullptr;

    ChatSession() = default;

    ~ChatSession() {
        // GlobalRef bereinigen
        if (cbGlobal) {
            ScopedEnv senv;
            if (auto* env = senv.get()) {
                env->DeleteGlobalRef(cbGlobal);
            }
            cbGlobal = nullptr;
        }

        // GNUnet Chat stoppen
        if (chatHandle) {
            LOGD("Stopping GNUNET_CHAT handle for session %ld", (long)handle);
            GNUNET_CHAT_disconnect(chatHandle);
            chatHandle = nullptr;
        }
    }
};

// Helper: Session lookup
static ChatSession* findSessionOrNull(long h) {
    std::lock_guard<std::mutex> lk(g_mapMtx);
    auto it = g_sessions.find(h);
    return (it == g_sessions.end()) ? nullptr : it->second.get();
}

// ======================================================
// JNI Setup
// ======================================================

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

// ======================================================
// GNUnet Callback (eingehende Nachrichten)
// -> Beispiel: ruft eine Methode am Java-Callback auf (onMessageReceived(String))
// ======================================================

static JNIEnv* GetEnv() {
    JNIEnv* env = nullptr;
    if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
    }
    return env;
}

// ---- kleine Helfer ----
static void fillChatContextDto(JNIEnv* env, jobject jCtx, struct GNUNET_CHAT_Context* ctx) {
    if (!env || !jCtx) return;
    jclass cls = env->GetObjectClass(jCtx);
    if (!cls) return;

    jfieldID fType    = env->GetFieldID(cls, "chatContextType", "I");
    jfieldID fIsGroup = env->GetFieldID(cls, "isGroup", "Z");
    jfieldID fIsPlat  = env->GetFieldID(cls, "isPlatform", "Z");
    jfieldID fUserPtr = env->GetFieldID(cls, "userPointer", "Ljava/lang/String;");

    int type = 0;
    bool isGroup = false;
    bool isPlatform = false;
    // Heuristik: Ist eine Gruppe am Context?
    if (ctx) {
        if (GNUNET_CHAT_context_get_group(ctx)) { isGroup = true; type = 1; }
        // platform: falls du später einen Indikator hast → hier setzen
    }

    env->SetIntField(jCtx, fType, (jint) type);
    env->SetBooleanField(jCtx, fIsGroup, (jboolean) isGroup);
    env->SetBooleanField(jCtx, fIsPlat, (jboolean) isPlatform);
    env->SetObjectField(jCtx, fUserPtr, nullptr);
}

static jlong getTimestampMillis(struct GNUNET_CHAT_Message* msg) {
    if (!msg) return 0;
    time_t ts = GNUNET_CHAT_message_get_timestamp(msg);
    if (ts <= 0) return 0;
    return (jlong) ts * 1000LL;
}

// ---- Haupt-Callback: nur die Felder, die du brauchst ----
static static enum GNUNET_GenericReturnValue chat_message_cb(void* cls,
                            struct GNUNET_CHAT_Context* ctx,
                            struct GNUNET_CHAT_Message* msg) {
    auto* sess = static_cast<ChatSession*>(cls);
    if (!sess || !sess->cbGlobal || !msg) {
        return GNUNET_YES; // niemals Scheduler blockieren
    }

    JNIEnv* env = GetEnv();
    if (!env) return GNUNET_YES;

    jclass clsCb  = env->GetObjectClass(sess->cbGlobal);
    jclass clsCtx = env->FindClass("org/gnunet/gnunetmessenger/ipc/ChatContextDto");
    jclass clsMsg = env->FindClass("org/gnunet/gnunetmessenger/ipc/ChatMessageDto");
    if (!clsCb || !clsCtx || !clsMsg) return GNUNET_YES;

    jmethodID ctorCtx = env->GetMethodID(clsCtx, "<init>", "()V");
    jmethodID ctorMsg = env->GetMethodID(clsMsg, "<init>", "()V");
    if (!ctorCtx || !ctorMsg) return GNUNET_YES;

    jobject jCtx = env->NewObject(clsCtx, ctorCtx);
    jobject jMsg = env->NewObject(clsMsg, ctorMsg);
    if (!jCtx || !jMsg) return GNUNET_YES;

    // -> Kontext füllen
    fillChatContextDto(env, jCtx, ctx); // deine Helper-Funktion

    // -> ChatMessageDto-Felder setzen (minimal TEXT/WARNING)
    jfieldID fKind      = env->GetFieldID(clsMsg, "kind", "I");
    jfieldID fTimestamp = env->GetFieldID(clsMsg, "timestamp", "J");
    jfieldID fText      = env->GetFieldID(clsMsg, "text", "Ljava/lang/String;");
    if (!fKind || !fTimestamp || !fText) {
        env->DeleteLocalRef(jCtx);
        env->DeleteLocalRef(jMsg);
        return GNUNET_YES;
    }

    const int kind = (int) GNUNET_CHAT_message_get_kind(msg);
    env->SetIntField(jMsg, fKind, (jint) kind);
    env->SetLongField(jMsg, fTimestamp, (jlong) getTimestampMillis(msg)); // dein Helper

    switch (kind) {
        case GNUNET_CHAT_KIND_WARNING:
        case GNUNET_CHAT_KIND_TEXT: {
            const char* t = GNUNET_CHAT_message_get_text(msg);
            jstring jT = t ? env->NewStringUTF(t) : nullptr;
            env->SetObjectField(jMsg, fText, jT);
            if (jT) env->DeleteLocalRef(jT);
            break;
        }
        case GNUNET_CHAT_KIND_INVITATION:
        default:
            env->SetObjectField(jMsg, fText, (jobject) nullptr);
            break;
    }

    // Java-Callback: void onMessage(ChatContextDto, ChatMessageDto)
    jmethodID mOnMessage = env->GetMethodID(
            clsCb,
            "onMessage",
            "(Lorg/gnunet/gnunetmessenger/ipc/ChatContextDto;"
            "Lorg/gnunet/gnunetmessenger/ipc/ChatMessageDto;)V"
    );
    if (mOnMessage) {
        env->CallVoidMethod(sess->cbGlobal, mOnMessage, jCtx, jMsg);
    }

    env->DeleteLocalRef(jCtx);
    env->DeleteLocalRef(jMsg);
    return GNUNET_YES;
}



extern "C" JNIEXPORT jboolean JNICALL
Java_org_gnunet_gnunetmessenger_ipc_NativeBridge_initCfgFromAsset(
        JNIEnv* env, jclass,
        jobject jAssetManager, jstring jAssetName)
{
    if (g_cfgInited.load()) return JNI_TRUE;

    AAssetManager* mgr = AAssetManager_fromJava(env, jAssetManager);
    if (!mgr) return JNI_FALSE;

    const char* assetName = env->GetStringUTFChars(jAssetName, nullptr);
    AAsset* asset = AAssetManager_open(mgr, assetName, AASSET_MODE_BUFFER);
    env->ReleaseStringUTFChars(jAssetName, assetName);
    if (!asset) return JNI_FALSE;

    off_t sz = AAsset_getLength(asset);
    if (sz <= 0) { AAsset_close(asset); return JNI_FALSE; }

    std::vector<char> buf;
    buf.resize(static_cast<size_t>(sz));
    const int readBytes = AAsset_read(asset, buf.data(), sz);
    AAsset_close(asset);
    if (readBytes != sz) return JNI_FALSE;

    // Config-Handle mit Projekt-Daten erzeugen (Android-Build hat die GNUnet-Projektinfos)
    struct GNUNET_CONFIGURATION_Handle* cfg =
            GNUNET_CONFIGURATION_create(GNUNET_OS_project_data_gnunet());
    if (!cfg) return JNI_FALSE;

    // Wichtig: deserialize erwartet die exakte Datenlänge (ohne extra NUL)
    if (GNUNET_OK != GNUNET_CONFIGURATION_deserialize(cfg, buf.data(),
                                                      static_cast<size_t>(sz), nullptr)) {
        GNUNET_CONFIGURATION_destroy(cfg);
        return JNI_FALSE;
    }

    g_cfg = cfg;
    g_cfgInited.store(true);
    return JNI_TRUE;
}

// ======================================================
// JNI: startChat
// Signature in Kotlin/Java (Server):
//     private external fun nativeStartChat(appName: String, callback: IChatCallback): Long
// Package/Cls: org.gnu.gnunet.ipc.GnunetChatIpcService
// ======================================================

// Ergänzung: zum Freigeben der Config
extern "C" JNIEXPORT void JNICALL
Java_org_gnunet_gnunetmessenger_ipc_NativeBridge_destroyCfg(
        JNIEnv*, jclass, jlong cfgPtr) {
    auto* cfg = reinterpret_cast<GNUNET_CONFIGURATION_Handle*>(cfgPtr);
    if (cfg) GNUNET_CONFIGURATION_destroy(cfg);
}

// Start mit bereits erstellter cfg
extern "C" JNIEXPORT jlong JNICALL
Java_org_gnunet_gnunetmessenger_ipc_NativeBridge_nativeStartChatUsingCfg(
        JNIEnv* env, jclass,
        jlong cfgPtr, jstring jAppName, jobject jCallback) {

    const char* appName = env->GetStringUTFChars(jAppName, nullptr);
    LOGD("nativeStartChat(%s)", appName ? appName : "(null)");

    // Neue Session anlegen
    auto sess = std::make_unique<ChatSession>();
    sess->handle = g_nextHandle++;
    if (jCallback) {
        sess->cbGlobal = env->NewGlobalRef(jCallback); // global ref behalten
    }

    // --- GNUnet Chat starten ---
    sess->chatHandle = GNUNET_CHAT_start(
            reinterpret_cast<const GNUNET_CONFIGURATION_Handle *>(cfgPtr), &chat_message_cb, sess.get());
    GNUNET_CONFIGURATION_destroy(reinterpret_cast<GNUNET_CONFIGURATION_Handle *>(cfgPtr));

    if (!sess->chatHandle) {
        LOGE("GNUNET_CHAT_start failed");
        if (sess->cbGlobal) {
            env->DeleteGlobalRef(sess->cbGlobal);
            sess->cbGlobal = nullptr;
        }
        env->ReleaseStringUTFChars(jAppName, appName);
        return 0;
    }

    // Session registrieren
    long handle = (long)sess->handle;
    {
        std::lock_guard<std::mutex> lk(g_mapMtx);
        g_sessions.emplace(handle, std::move(sess));
    }

    env->ReleaseStringUTFChars(jAppName, appName);
    LOGD("Session created with handle=%ld", handle);
    return static_cast<jlong>(handle);
}

// ======================================================
// JNI: createAccount
// Signature in Kotlin/Java:
//     private external fun nativeCreateAccount(handle: Long, name: String): Int
// ======================================================

extern "C" JNIEXPORT jint JNICALL
Java_org_gnu_gnunet_ipc_GnunetChatIpcService_nativeCreateAccount(
        JNIEnv* env, jobject /*thiz*/, jlong jHandle, jstring jName) {

    const char* name = env->GetStringUTFChars(jName, nullptr);
    long h = static_cast<long>(jHandle);
    LOGD("nativeCreateAccount(handle=%ld, name=%s)", h, name ? name : "(null)");

    ChatSession* sess = findSessionOrNull(h);
    if (!sess || !sess->chatHandle) {
        env->ReleaseStringUTFChars(jName, name);
        return -1; // GNUNET_SYSERR analog
    }

    const char* nameC = env->GetStringUTFChars(jName, nullptr);
    if (!nameC) {
        LOGE("nativeCreateAccount: failed to get UTF chars");
        return GNUNET_SYSERR;
    }

    int rc = GNUNET_CHAT_account_create(sess->chatHandle, nameC);

    env->ReleaseStringUTFChars(jName, name);
    return rc;
}

// ======================================================
// JNI: connect
// Signature in Kotlin/Java:
//     private external fun nativeConnect(handle: Long, accountKey: String, accountName: String)
// Du kannst die Parameterform anpassen – aktuell minimal.
// ======================================================

extern "C" JNIEXPORT void JNICALL
Java_org_gnu_gnunet_ipc_GnunetChatIpcService_nativeConnect(
        JNIEnv* env, jobject /*thiz*/, jlong jHandle,
        jstring jAccountKey, jstring jAccountName) {

    long h = static_cast<long>(jHandle);
    const char* key = jAccountKey ? env->GetStringUTFChars(jAccountKey, nullptr) : "";
    const char* name = jAccountName ? env->GetStringUTFChars(jAccountName, nullptr) : "";
    struct GNUNET_CHAT_Account *account;
    LOGD("nativeConnect(handle=%ld, key=%s, name=%s)", h, key, name);

    ChatSession* sess = findSessionOrNull(h);
    if (!sess || !sess->chatHandle) {
        if (jAccountKey) env->ReleaseStringUTFChars(jAccountKey, key);
        if (jAccountName) env->ReleaseStringUTFChars(jAccountName, name);
        return;
    }

    account = GNUNET_CHAT_find_account(
            sess->chatHandle,
            name
    );

    GNUNET_CHAT_connect(sess->chatHandle, account);

    sess->connected = true; // Platzhalter

    if (jAccountKey) env->ReleaseStringUTFChars(jAccountKey, key);
    if (jAccountName) env->ReleaseStringUTFChars(jAccountName, name);
}

// ======================================================
// JNI: disconnect
// Signature in Kotlin/Java:
//     private external fun nativeDisconnect(handle: Long)
// ======================================================

extern "C" JNIEXPORT void JNICALL
Java_org_gnu_gnunet_ipc_GnunetChatIpcService_nativeDisconnect(
        JNIEnv* /*env*/, jobject /*thiz*/, jlong jHandle) {

    long h = static_cast<long>(jHandle);
    LOGD("nativeDisconnect(handle=%ld)", h);

    ChatSession* sess = findSessionOrNull(h);
    if (!sess || !sess->chatHandle) return;

    if (sess->chatHandle) {
        GNUNET_CHAT_disconnect(sess->chatHandle); // ✅
        sess->chatHandle = nullptr;
        sess->connected  = false;
    }
    sess->connected = false; // Platzhalter
}

// ======================================================
// JNI: getProfileName
// Signature in Kotlin/Java:
//     private external fun nativeGetProfileName(handle: Long): String
// ======================================================

extern "C" JNIEXPORT jstring JNICALL
Java_org_gnu_gnunet_ipc_GnunetChatIpcService_nativeGetProfileName(
        JNIEnv* env, jobject /*thiz*/, jlong jHandle) {

    long h = static_cast<long>(jHandle);
    ChatSession* sess = findSessionOrNull(h);
    if (!sess) return env->NewStringUTF("");

    sess->profileName = GNUNET_CHAT_get_name (sess->chatHandle);
    return env->NewStringUTF(sess->profileName.c_str());
}

// ======================================================
// JNI: setProfileName
// Signature in Kotlin/Java:
//     private external fun nativeSetProfileName(handle: Long, name: String)
// ======================================================

extern "C" JNIEXPORT void JNICALL
Java_org_gnu_gnunet_ipc_GnunetChatIpcService_nativeSetProfileName(
        JNIEnv* env, jobject /*thiz*/, jlong jHandle, jstring jName) {

    long h = static_cast<long>(jHandle);
    const char* name = env->GetStringUTFChars(jName, nullptr);

    ChatSession* sess = findSessionOrNull(h);
    if (sess) {
        GNUNET_CHAT_set_name (sess->chatHandle ,name);
        sess->profileName = name ? name : "";
    }

    env->ReleaseStringUTFChars(jName, name);
}

// --------- Hilfsfunktion: ChatAccountDto bauen ---------
static jobject buildChatAccountDto(JNIEnv* env,
                                   const char* key,
                                   const char* name)
{
    if (!env) return nullptr;

    jclass clsDto = env->FindClass("org/gnunet/gnunetmessenger/ipc/ChatAccountDto");
    if (!clsDto) return nullptr;

    jmethodID ctor = env->GetMethodID(clsDto, "<init>", "()V");
    if (!ctor) return nullptr;

    jobject dto = env->NewObject(clsDto, ctor);
    if (!dto) return nullptr;

    jfieldID fKey  = env->GetFieldID(clsDto, "key",  "Ljava/lang/String;");
    jfieldID fName = env->GetFieldID(clsDto, "name", "Ljava/lang/String;");
    if (!fKey || !fName) return dto; // Felder optional setzen

    jstring jKey  = key  ? env->NewStringUTF(key)  : nullptr;
    jstring jName = name ? env->NewStringUTF(name) : nullptr;

    env->SetObjectField(dto, fKey,  jKey);
    env->SetObjectField(dto, fName, jName);

    if (jKey)  env->DeleteLocalRef(jKey);
    if (jName) env->DeleteLocalRef(jName);

    return dto;
}

// --------- Daten für die Iteration + JNI Callback IDs cachen ---------
struct IterateAccountsCtx {
    jclass cbClass = nullptr;
    jobject cbGlobal = nullptr;
    jmethodID onAccount = nullptr; // (LChatAccountDto;)V
    jmethodID onDone    = nullptr; // ()V
    jmethodID onError   = nullptr; // (ILjava/lang/String;)V
};

// --------- GNUnet-Callback: wird je Account aufgerufen ---------
static enum GNUNET_GenericReturnValue
iterateAccountsCb(void* cls,
                  GNUNET_CHAT_Handle* /*handle*/,
                  GNUNET_CHAT_Account* account)
{
    auto* ictx = static_cast<IterateAccountsCtx*>(cls);
    if (!ictx) return GNUNET_YES;

    ScopedEnv senv;
    JNIEnv* env = senv.get();
    if (!env || !ictx->cbGlobal || !ictx->onAccount) return GNUNET_YES;

    const char* name = GNUNET_CHAT_account_get_name(account);
    const char* key  = "";          // Platzhalter
    //const char* name = "Account";   // Platzhalter

    jobject dto = buildChatAccountDto(env, key, name);
    if (dto) {
        env->CallVoidMethod(ictx->cbGlobal, ictx->onAccount, dto);
        env->DeleteLocalRef(dto);
    }
    // Weiter iterieren
    return GNUNET_YES;
}

// --------- JNI: nativeIterateAccounts(handle, IAccountCallback) ---------
extern "C" JNIEXPORT void JNICALL
Java_org_gnunet_gnunetmessenger_ipc_NativeBridge_nativeIterateAccounts(
        JNIEnv* env, jclass /*clazz*/,
        jlong jHandle,
        jobject jAccountCallback /* IAccountCallback */)
{
    // 1) Session finden
    std::unique_ptr<ChatSession>* sessPtr = nullptr;
    {
        std::lock_guard<std::mutex> lk(g_mapMtx);
        auto it = g_sessions.find((long) jHandle);
        if (it != g_sessions.end()) {
            sessPtr = &it->second;
        }
    }
    if (!sessPtr || !sessPtr->get()) {
        // Callback.onError(...) (falls verfügbar), sonst nur loggen
        jclass cbCls = env->GetObjectClass(jAccountCallback);
        if (cbCls) {
            jmethodID onError = env->GetMethodID(cbCls, "onError", "(ILjava/lang/String;)V");
            if (onError) {
                jstring jMsg = env->NewStringUTF("No such session");
                env->CallVoidMethod(jAccountCallback, onError, (jint) -1, jMsg);
                if (jMsg) env->DeleteLocalRef(jMsg);
            }
            env->DeleteLocalRef(cbCls);
        }
        return;
    }

    ChatSession* sess = sessPtr->get();
    if (!sess->chatHandle) {
        jclass cbCls = env->GetObjectClass(jAccountCallback);
        if (cbCls) {
            jmethodID onError = env->GetMethodID(cbCls, "onError", "(ILjava/lang/String;)V");
            if (onError) {
                jstring jMsg = env->NewStringUTF("Chat not started (chatHandle==null)");
                env->CallVoidMethod(jAccountCallback, onError, (jint) -2, jMsg);
                if (jMsg) env->DeleteLocalRef(jMsg);
            }
            env->DeleteLocalRef(cbCls);
        }
        return;
    }

    // 2) Callback-Methoden auflösen & GlobalRef halten (falls GNUnet anderen Thread nutzt)
    IterateAccountsCtx ictx;

    ictx.cbGlobal = env->NewGlobalRef(jAccountCallback);
    if (!ictx.cbGlobal) return;

    ictx.cbClass = (jclass) env->NewGlobalRef(env->GetObjectClass(jAccountCallback));
    if (!ictx.cbClass) {
        env->DeleteGlobalRef(ictx.cbGlobal);
        return;
    }

    ictx.onAccount = env->GetMethodID(
            ictx.cbClass, "onAccount",
            "(Lorg/gnunet/gnunetmessenger/ipc/ChatAccountDto;)V");
    ictx.onDone  = env->GetMethodID(ictx.cbClass, "onDone", "()V");
    ictx.onError = env->GetMethodID(ictx.cbClass, "onError", "(ILjava/lang/String;)V");

    if (!ictx.onAccount) {
        // Minimal: ohne onAccount können wir nichts tun
        if (ictx.onError) {
            jstring jMsg = env->NewStringUTF("IAccountCallback.onAccount not found");
            env->CallVoidMethod(ictx.cbGlobal, ictx.onError, (jint) -3, jMsg);
            if (jMsg) env->DeleteLocalRef(jMsg);
        }
        env->DeleteGlobalRef(ictx.cbClass);
        env->DeleteGlobalRef(ictx.cbGlobal);
        return;
    }

    // 3) Iteration starten (synchron)
    int rc = GNUNET_CHAT_iterate_accounts(sess->chatHandle, &iterateAccountsCb, &ictx);

    // 4) Abschluss melden
    if (rc < 0) {
        if (ictx.onError) {
            jstring jMsg = env->NewStringUTF("iterate_accounts failed");
            env->CallVoidMethod(ictx.cbGlobal, ictx.onError, (jint) rc, jMsg);
            if (jMsg) env->DeleteLocalRef(jMsg);
        }
    } else {
        if (ictx.onDone) {
            env->CallVoidMethod(ictx.cbGlobal, ictx.onDone);
        }
    }

    // 5) Aufräumen
    env->DeleteGlobalRef(ictx.cbClass);
    env->DeleteGlobalRef(ictx.cbGlobal);
}