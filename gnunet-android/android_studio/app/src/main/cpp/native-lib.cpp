#include <fstream>
#include <iostream>
#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "gnunet_util_lib.h"
#include <ltdl.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>

#define TAG "GNUNET"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,    TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,     TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,     TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,    TAG, __VA_ARGS__)

extern "C" {
    #include <unistd.h>
    #include <gnunet_util_lib.h>
}

static int pfd[2];
static pthread_t thr;
static const char *tag = "GNUNET";

void *thread_func(void*)
{
    static const size_t READ_CHUNK = 1024;
    static const size_t MAX_LINE   = 8192; // eigene Obergrenze pro „logische“ Zeile
    std::string acc; acc.reserve(MAX_LINE);
    std::vector<char> tmp(READ_CHUNK);

    ssize_t n;
    while ((n = read(pfd[0], tmp.data(), tmp.size())) > 0) {
        acc.append(tmp.data(), n);

        size_t pos;
        while ((pos = acc.find('\n')) != std::string::npos) {
            std::string line = acc.substr(0, pos);
            acc.erase(0, pos + 1);

            // Leer \r am Ende weg
            if (!line.empty() && line.back() == '\r') line.pop_back();

            // In handliche Teile splitten, um log-Limit/ANSI zu vermeiden
            const size_t CHUNK = 1000;
            for (size_t i = 0; i < line.size(); i += CHUNK) {
                std::string_view part(line.c_str() + i, std::min(CHUNK, line.size() - i));
                __android_log_write(ANDROID_LOG_DEBUG, tag, std::string(part).c_str());
            }
        }

        // Falls Zeile extrem lang wird, „notfall“ flushen:
        if (acc.size() > MAX_LINE) {
            __android_log_write(ANDROID_LOG_DEBUG, tag, "[truncated: line too long]");
            acc.clear();
        }
    }

    // Rest ohne Newline flushen
    if (!acc.empty()) {
        __android_log_write(ANDROID_LOG_DEBUG, tag, acc.c_str());
    }
    return nullptr;
}

int start_logger(const char *app_name)
{
    tag = app_name;

    /* make stdout line-buffered and stderr unbuffered */
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    /* create the pipe and redirect stdout and stderr */
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    /* spawn the logging thread */
    if(pthread_create(&thr, 0, thread_func, 0) == -1)
        return -1;
    pthread_detach(thr);
    return 0;
}

/** ------------------ Milestone 6-7 code. -------------------------------
static void
run (void *cls,
     char *const *args,
     const char *cfgfile,
     const struct GNUNET_CONFIGURATION_Handle *c)
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        LOGE ("ERROR opening socket");
        perror("ERROR opening socket");
        goto shutdown;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 8081;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        goto shutdown;
    }
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,
                       (struct sockaddr *) &cli_addr,
                       &clilen);
    if (newsockfd < 0)
    {
        perror("ERROR on accept");
        goto shutdown;
    }
    bzero(buffer,256);
    n = read(newsockfd,buffer,255);
    if (n < 0)
    {
        perror("ERROR reading from socket");
        goto shutdown;
    }
    printf("Here is the message: %s\n",buffer);
    n = write(newsockfd,"I got your message",18);
    if (n < 0)
    {
        perror("ERROR writing to socket");
        goto shutdown;
    }
    close(newsockfd);
    close(sockfd);

shutdown:
    GNUNET_SCHEDULER_shutdown();
}*/

// END ------------------------------ Milestone 6 - 7 Code ---------------------

static jobject android_java_asset_manager = NULL;
static struct GNUNET_ARM_Handle *h;
static struct GNUNET_CONFIGURATION_Handle *cfg;
static struct GNUNET_ARM_Operation *op;

/* -------------- Multi library code -----------------------------
/*static void
start_callback (void *cls,
                enum GNUNET_ARM_RequestStatus rs,
                enum GNUNET_ARM_Result result)
{
    (void) cls;
    op = NULL;
    if (GNUNET_ARM_REQUEST_SENT_OK != rs)
    {
        LOGE("Failed to start the ARM service.");
        GNUNET_SCHEDULER_shutdown ();
        return;
    }
    if ((GNUNET_ARM_RESULT_STARTING != result) &&
        (GNUNET_ARM_RESULT_IS_STARTED_ALREADY != result))
    {
        LOGE("Failed to start the ARM service.");
        GNUNET_SCHEDULER_shutdown ();
        return;
    }
    LOGD ("ARM service [re]start successful");
}


static void
shutdown_task (void *cls)
{
    (void) cls;
    if (NULL != op)
    {
        GNUNET_ARM_operation_cancel (op);
        op = NULL;
    }
    if (NULL != h)
    {
        GNUNET_ARM_disconnect (h);
        h = NULL;
    }
    GNUNET_CONFIGURATION_destroy (cfg);
    cfg = NULL;
}*/


/**
 * Function called whenever we connect to or disconnect from ARM.
 * Termiantes the process if we fail to connect to the service on
 * our first attempt.
 *
 * @param cls closure
 * @param connected #GNUNET_YES if connected, #GNUNET_NO if disconnected,
 *                  #GNUNET_SYSERR on error.
 */
/*static void
conn_status (void *cls,
             enum GNUNET_GenericReturnValue connected)
{
    static int once;

    (void) cls;
    if ( (GNUNET_SYSERR == connected) &&
         (0 == once) )
    {
        LOGE("Fatal error initializing ARM API.");
        GNUNET_SCHEDULER_shutdown ();
        return;
    }
    once = 1;
}



static void
run (void *cls,
     char *const *args,
     const char *cfgfile,
     const struct GNUNET_CONFIGURATION_Handle *c)
{
    start_logger(tag);
    printf("Some message.\n");
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Simulate error message. We do not no what is the actual log level.\n");
    cfg =  GNUNET_CONFIGURATION_dup (c);
    AAssetManager *mgr = static_cast<AAssetManager *>(cls);
    AAsset *asset = AAssetManager_open(mgr, "gnunet.conf", AASSET_MODE_BUFFER);
    char buf[AAsset_getLength(asset)];

    AAsset_read(asset, buf, AAsset_getLength(asset));

    if (GNUNET_OK != GNUNET_CONFIGURATION_deserialize (cfg, buf, strlen(buf), NULL))
    {
        LOGE ("Deserialization of configuration failed!");
    }

    AAsset_close(asset);


    if (NULL == (h = GNUNET_ARM_connect (cfg,
                                         &conn_status,
                                         NULL)))
        return;

    op = GNUNET_ARM_request_service_start (h,
                                           "arm",
                                           GNUNET_OS_INHERIT_STD_NONE,
                                           &start_callback,
                                           NULL);


}
 END -------------- Multi library code ----------------------------- */

void ensure_directory_exists(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        if (mkdir(path, 0700) != 0) {
            __android_log_print(ANDROID_LOG_ERROR, "APP", "Failed to create dir: %s (%s)", path, strerror(errno));
        } else {
            __android_log_print(ANDROID_LOG_DEBUG, "APP", "Created dir: %s", path);
        }
    } else {
        __android_log_print(ANDROID_LOG_DEBUG, "APP", "Dir already exists: %s", path);
    }
}

void ensure_file_exists(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        // Datei existiert nicht → versuchen, anzulegen
        int fd = open(path, O_CREAT | O_WRONLY, 0600);
        if (fd < 0) {
            __android_log_print(ANDROID_LOG_ERROR, "APP", "Failed to create file: %s (%s)", path, strerror(errno));
        } else {
            __android_log_print(ANDROID_LOG_DEBUG, "APP", "Created file: %s", path);
            close(fd);
        }
    } else {
        __android_log_print(ANDROID_LOG_DEBUG, "APP", "File already exists: %s", path);
    }
}

void copy_plugins(AAssetManager* mgr, const char* dest_dir) {
    // Zielverzeichnis erstellen
    mkdir(dest_dir, 0700);  // Ignoriert Fehler, wenn schon existiert

    // Asset-Verzeichnis öffnen
    AAssetDir* assetDir = AAssetManager_openDir(mgr, "plugins");
    if (!assetDir) {
        LOGE("Could not open assets/plugins/");
        return;
    }

    const char* filename = nullptr;
    while ((filename = AAssetDir_getNextFileName(assetDir)) != nullptr) {
        LOGD("Copying %s...", filename);

        // Pfad innerhalb von Assets
        std::string asset_path = std::string("plugins/") + filename;

        AAsset* asset = AAssetManager_open(mgr, asset_path.c_str(), AASSET_MODE_STREAMING);
        if (!asset) {
            LOGE("Failed to open asset %s", asset_path.c_str());
            continue;
        }

        // Zielpfad vorbereiten
        std::string out_path = std::string(dest_dir) + "/" + filename;
        int out_fd = open(out_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0700);
        if (out_fd < 0) {
            LOGE("Could not create output file %s", out_path.c_str());
            AAsset_close(asset);
            continue;
        }

        // Kopieren
        char buffer[4096];
        int read_bytes;
        while ((read_bytes = AAsset_read(asset, buffer, sizeof(buffer))) > 0) {
            write(out_fd, buffer, read_bytes);
        }

        close(out_fd);
        AAsset_close(asset);
        LOGD("Copied to %s", out_path.c_str());
    }

    AAssetDir_close(assetDir);
}

void set_executable_permissions(const char* path) {
    if (chmod(path, 0700) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, "GNUNET", "chmod failed for %s: %s", path, strerror(errno));
    } else {
        __android_log_print(ANDROID_LOG_DEBUG, "GNUNET", "chmod OK: %s", path);
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_gnu_gnunet_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */,
        jobject assets,
        jstring path) {

    jboolean isCopy = JNI_FALSE;
    const char *c_install_path = env->GetStringUTFChars(path, &isCopy);
    const struct GNUNET_OS_ProjectData *default_pd = GNUNET_OS_project_data_gnunet();
    const struct GNUNET_OS_ProjectData my_pd = {
            .libname = "libgnunetutil",
            .project_dirname = "gnunet",
            .binary_name = "gnunet-arm",
            .version = default_pd->version,
            .env_varname = "GNUNET_PREFIX",
            .base_config_varname = "GNUNET_BASE_CONFIG",
            .bug_email = "gnunet-developers@gnu.org",
            .homepage = "http://www.gnu.org/s/gnunet/",
            .config_file = "gnunet.conf",
            .user_config_file = "~/.config/gnunet.conf",
            .is_gnu = 1,
            .gettext_domain = "gnunet",
            .gettext_path = NULL,
            .agpl_url = GNUNET_AGPL_URL,
            .install_path_override = c_install_path
    };

    void* handle_gnunet_util = dlopen("libgnunetutil.so", RTLD_NOW);
    if (!handle_gnunet_util) return env->NewStringUTF("libgnunetutil.so not loaded");
   /* void* handle_plugin_peerstore_sqlite = dlopen("libgnunet_plugin_peerstore_sqlite.so", RTLD_NOW);
    if (!handle_plugin_peerstore_sqlite) return env->NewStringUTF("libgnunet_plugin_peerstore_sqlite.so not loaded");*/
    android_java_asset_manager = (*env).NewGlobalRef(assets);
    AAssetManager *mgr = AAssetManager_fromJava(env, android_java_asset_manager);

    std::string install_path_str(c_install_path);
    ensure_directory_exists((install_path_str + "/gnunet").c_str());
    ensure_directory_exists((install_path_str + "/tmp").c_str());
    ensure_directory_exists((install_path_str + "/share").c_str());
    ensure_directory_exists((install_path_str + "/share/gnunet").c_str());
    ensure_directory_exists((install_path_str + "/share/gnunet/hellos").c_str());
    ensure_directory_exists((install_path_str + "/home").c_str());
    ensure_directory_exists((install_path_str + "/home/data").c_str());
    ensure_file_exists((install_path_str + "/home/data/statistics.dat").c_str());
    ensure_directory_exists((install_path_str + "/home/data/nse").c_str());
    set_executable_permissions((install_path_str + "/home/data/nse").c_str());
    ensure_file_exists((install_path_str + "/home/data/nse/proof.dat").c_str());
    set_executable_permissions((install_path_str + "/home/data/nse/proof.dat").c_str());
    ensure_directory_exists((install_path_str + "/lib").c_str());
    ensure_directory_exists((install_path_str + "/lib/gnunet").c_str());
    copy_plugins(mgr, "/data/user/0/org.gnu.gnunet/files/lib/gnunet");
    set_executable_permissions("/data/user/0/org.gnu.gnunet/files/lib/gnunet/libgnunet_plugin_datacache_heap.so");

    struct stat st;
    if (0 == stat("/data/user/0/org.gnu.gnunet/files/home/data/nse/proof.dat", &st)) {
        __android_log_print(ANDROID_LOG_DEBUG, "GNUNET", "proof.dat size = %lld", (long long)st.st_size);
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "GNUNET", "proof.dat not found after creation");
    }

    const char* proof_path = "/data/user/0/org.gnu.gnunet/files/home/data/nse/proof.dat";
    FILE* f = fopen(proof_path, "a");
    if (!f) {
        __android_log_print(ANDROID_LOG_ERROR, "GNUNET", "fopen() failed on %s", proof_path);
        perror("fopen");
    } else {
        fprintf(f, "Write test at %ld\n", time(nullptr));
        fclose(f);
        __android_log_print(ANDROID_LOG_DEBUG, "GNUNET", "Write to %s successful", proof_path);
    }

    void* plugin = dlopen("/data/user/0/org.gnu.gnunet/files/lib/gnunet/libgnunet_plugin_datacache_heap.so", RTLD_NOW);

    if (!plugin)
        __android_log_print(ANDROID_LOG_ERROR, "DLTEST", "dlopen failed: %s", dlerror());
    else
        __android_log_print(ANDROID_LOG_DEBUG, "DLTEST", "dlopen SUCCESS for libgnunet_plugin_datacache_heap");

    set_executable_permissions("/data/user/0/org.gnu.gnunet/files/lib/gnunet/libgnunet_plugin_datastore.so");

    void* plugin2 = dlopen("/data/user/0/org.gnu.gnunet/files/lib/gnunet/libgnunet_plugin_datastore.so", RTLD_NOW);

    if (!plugin2)
        __android_log_print(ANDROID_LOG_ERROR, "DLTEST", "dlopen failed: %s", dlerror());
    else
        __android_log_print(ANDROID_LOG_DEBUG, "DLTEST", "dlopen SUCCESS for libgnunet_plugin_datastore");

    void* handle_util = dlopen("/data/user/0/org.gnu.gnunet/files/lib/gnunet/libgnunetutil.so", RTLD_NOW | RTLD_GLOBAL);
    if (!handle_util) {
        __android_log_print(ANDROID_LOG_ERROR, "DLTEST", "Failed to load libgnunetutil.so: %s", dlerror());
    } else {
        __android_log_print(ANDROID_LOG_DEBUG, "DLTEST", "Loaded libgnunetutil.so successfully");
    }

    const char* tmp = GNUNET_DISK_mktemp(&my_pd, "test");
    if (tmp == nullptr) {
        // Fehlerbehandlung
        __android_log_print(ANDROID_LOG_ERROR, "GNUNET", "mktemp failed!");
        return env->NewStringUTF("mktemp failed");
    }
    std::string tmp_file = tmp;
    LOGD ("Temp file is here: %s", tmp_file.c_str());
    int ok = lt_dlinit();
    LOGD("lt_dlinit return code: %d", ok);
    LOGD ("current ltdl search path: %s", lt_dlgetsearchpath());

    lt_dlsetsearchpath ("/data/user/0/org.gnu.gnunet/files/lib/gnunet");

    LOGD ("current ltdl search path afterwards: %s", lt_dlgetsearchpath());
    char *const non_const_ptr = const_cast<char*>(tmp_file.c_str());

    char *const argvx[] = {
            "BuggerAll",
            nullptr
    };


    static const struct GNUNET_GETOPT_CommandLineOption options[] = {
            GNUNET_GETOPT_OPTION_END
    };

    start_logger(tag);
    printf("Some message.\n");
    /*
    GNUNET_log (GNUNET_ERROR_TYPE_ERROR,
                "Simulate error message. We do not no what is the actual log level.\n");
                */

    /*AAsset *plugin_asset = AAssetManager_open(mgr, "libgnunet_plugin_peerstore_sqlite.so", AASSET_MODE_BUFFER);
    const off_t &length = AAsset_getLength(plugin_asset);
    char plugin_buf[length];
    LOGD("libgnunet_plugin_peerstore_sqlite.so size is %ld bytes", length);
    AAsset_read(plugin_asset, plugin_buf, length);
    enum GNUNET_DISK_AccessPermissions permission = static_cast<GNUNET_DISK_AccessPermissions>(
            GNUNET_DISK_PERM_USER_READ
            | GNUNET_DISK_PERM_USER_WRITE
            | GNUNET_DISK_PERM_USER_EXEC);

    // This code fails to get a fileHandle. TODO work out why not. It means the write doesn't happen.
    struct GNUNET_DISK_FileHandle *fh = GNUNET_DISK_file_open ("/data/user/0/org.gnu.gnunet/files/libgnunet_plugin_peerstore_sqlite_gnunet.so",
                           GNUNET_DISK_OPEN_WRITE,
                           permission );
    GNUNET_DISK_file_write (fh,
                            plugin_buf,
                            length);
    GNUNET_DISK_file_close (fh);*/

    // The following code does work; so does the kotlin code that copies the .so file from the assets folder.
    std::vector<int> data = {1, 2, 3, 4, 5, 6};
    std::ofstream outfile("/data/user/0/org.gnu.gnunet/files/output.bin", std::ios::binary);
    if (!outfile) {
        LOGE("Could not open output.bin for writing");
    }

    outfile.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(int ));
    if (!outfile) {
        LOGE("Write failed for output.bin");
    }
    outfile.close();


    cfg = GNUNET_CONFIGURATION_create (GNUNET_OS_project_data_gnunet());
    if (cfg == nullptr) {
        LOGE("Failed to create GNUNET configuration object");
        return env->NewStringUTF("Failed to create GNUNET configuration object");
    }
    AAsset *asset = AAssetManager_open(mgr, "gnunet.conf", AASSET_MODE_BUFFER);
    off_t config_file_size = AAsset_getLength(asset);
    LOGD("gnunet.conf is %ld bytes long", config_file_size);


    std::vector<char> buf(config_file_size);

    int actually_read = AAsset_read(asset, buf.data(), config_file_size);
    LOGD("actually read %d bytes", actually_read);

    // Sicherstellen, dass der Puffer nullterminiert ist
    buf.push_back('\0');

    if (GNUNET_OK != GNUNET_CONFIGURATION_deserialize(cfg, buf.data(), config_file_size, NULL))
    {
        LOGE ("Deserialization of configuration failed!");
    }
    AAsset_close(asset);
    LOGD("about to run GNUNET_PROGRAM_monolith_main");

    GNUNET_PROGRAM_monolith_main (&my_pd,
                                  1,
                        argvx,
                        cfg);

    /*GNUNET_PROGRAM_run (4,
                        argvx,
                        "native-lib",
                        "native-lib",
                        options,
                        &run,
                        mgr);*/
    std::string hello = "Hello from C++. Temp file is here: " + tmp_file;

    return env->NewStringUTF(hello.c_str());
}