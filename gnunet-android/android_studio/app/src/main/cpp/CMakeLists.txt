# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("gnunet")

# configure import libs
set(distribution_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../distribution)

add_library(idn SHARED IMPORTED)
set_target_properties(idn PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/idn/lib/${ANDROID_ABI}/libidn.so)
add_library(sqliteX SHARED IMPORTED)
set_target_properties(sqliteX PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/sqlite/lib/${ANDROID_ABI}/libsqliteX.so)
add_library(gnunetutil SHARED IMPORTED)
set_target_properties(gnunetutil PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gnunetutil/lib/${ANDROID_ABI}/libgnunetutil.so)
add_library(gnunetsq SHARED IMPORTED)
set_target_properties(gnunetsq PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gnunetsq/lib/${ANDROID_ABI}/libgnunetsq.so)
add_library(gnunetjson SHARED IMPORTED)
set_target_properties(gnunetjson PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gnunetjson/lib/${ANDROID_ABI}/libgnunetjson.so)
add_library(gnunetgnsrecord SHARED IMPORTED)
set_target_properties(gnunetgnsrecord PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gnunetgnsrecord/lib/${ANDROID_ABI}/libgnunetgnsrecord.so)
add_library(gnunetblock SHARED IMPORTED)
set_target_properties(gnunetblock PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gnunetblock/lib/${ANDROID_ABI}/libgnunetblock.so)
add_library(gnunetregexblock SHARED IMPORTED)
set_target_properties(gnunetregexblock PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gnunetregexblock/lib/${ANDROID_ABI}/libgnunetregexblock.so)
add_library(gnunetblockgroup SHARED IMPORTED)
set_target_properties(gnunetblockgroup PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gnunetblockgroup/lib/${ANDROID_ABI}/libgnunetblockgroup.so)
add_library(gnunethello SHARED IMPORTED)
set_target_properties(gnunethello PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gnunethello/lib/${ANDROID_ABI}/libgnunethello.so)
#[[add_library(gnunet_plugin_peerstore_sqlite SHARED IMPORTED)
set_target_properties(gnunet_plugin_peerstore_sqlite PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gnunet_plugin_peerstore_sqlite/lib/${ANDROID_ABI}/libgnunet_plugin_peerstore_sqlite.so)]]
add_library(gnunet SHARED IMPORTED)
set_target_properties(gnunet PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gnunet/lib/${ANDROID_ABI}/libgnunet.so)
add_library(gnunetchat SHARED IMPORTED)
set_target_properties(gnunetchat PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gnunetchat/lib/${ANDROID_ABI}/libgnunetchat.so)
add_library(libgpg-error SHARED IMPORTED)
set_target_properties(libgpg-error PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libgpg-error/lib/${ANDROID_ABI}/libgpg-error.so)
add_library(libgcrypt SHARED IMPORTED)
set_target_properties(libgcrypt PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libgcrypt/lib/${ANDROID_ABI}/libgcrypt.so)
add_library(libsodium SHARED IMPORTED)
set_target_properties(libsodium PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libsodium/lib/${ANDROID_ABI}/libsodium.so)
add_library(libtool SHARED IMPORTED)
set_target_properties(libtool PROPERTIES IMPORTED_LOCATION
${distribution_DIR}/libtool/lib/${ANDROID_ABI}/libltdl.so)
add_library(gmp SHARED IMPORTED)
set_target_properties(gmp PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gmp/lib/${ANDROID_ABI}/libgmp.so)
add_library(libunistring SHARED IMPORTED)
set_target_properties(libunistring PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libunistring/lib/${ANDROID_ABI}/libunistring.so)
add_library(libz SHARED IMPORTED)
set_target_properties(libz PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libz/lib/${ANDROID_ABI}/libz.so)
add_library(libiconv SHARED IMPORTED)
set_target_properties(libiconv PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libiconv/lib/${ANDROID_ABI}/libiconv.so)
add_library(libintl SHARED IMPORTED)
set_target_properties(libintl PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libintl/lib/${ANDROID_ABI}/libintl.so)
add_library(libgettext SHARED IMPORTED)
set_target_properties(libgettext PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/gettext/lib/${ANDROID_ABI}/libgettextlib-0.22.5.so)
add_library(jansson SHARED IMPORTED)
set_target_properties(jansson PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/jansson/lib/${ANDROID_ABI}/libjansson.so)
add_library(wolfssl SHARED IMPORTED)
set_target_properties(wolfssl PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/wolfssl/lib/${ANDROID_ABI}/libwolfssl.so)
add_library(libcurl SHARED IMPORTED)
set_target_properties(libcurl PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libcurl/lib/${ANDROID_ABI}/libcurl.so)
add_library(microhttpd SHARED IMPORTED)
set_target_properties(microhttpd PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/microhttpd/lib/${ANDROID_ABI}/libmicrohttpd.so)

# build application's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.
add_library(gnunetti SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        native-lib.cpp
        GnunetChatIpcBridge.cpp
)

target_include_directories(gnunetti PRIVATE
        ${distribution_DIR}/idn/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/sqlite/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/libsodium/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/libgcrypt/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/libgpg-error/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/gmp/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/libunistring/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/libz/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/libtool/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/libiconv/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/libintl/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/gettext/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/gnunetutil/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/gnunet/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/gnunethello/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/gnunet_plugin_peerstore_sqlite/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/gnunetsq/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/gnunetgnsrecord/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/gnunetblock/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/gnunetregexblock/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/gnunetblockgroup/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/gnunetchat/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/wolfssl/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/libcurl/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/jansson/lib/${ANDROID_ABI}/include
        ${distribution_DIR}/microhttpd/lib/${ANDROID_ABI}/include)

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(gnunetti
        # List libraries link to the target library
        android
        libgpg-error
        libgcrypt
        libsodium
        libtool
        idn
        gmp
        libunistring
        libz
        libiconv
        libintl
        libgettext
        wolfssl
        libcurl
        jansson
        microhttpd
        gnunetsq
        gnunetregexblock
        gnunetblock
        gnunetblockgroup
        gnunetgnsrecord
        gnunetjson
        gnunethello
        gnunetutil
        gnunet
        gnunetchat
        sqliteX
        log)

target_link_options(gnunetti PRIVATE "-Wl,-z,max-page-size=16384")