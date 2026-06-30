plugins {
    alias(libs.plugins.android.application)
    // id("com.google.gms.google-services") // Disabled for multi-client testing
    kotlin("android")
    id("androidx.navigation.safeargs.kotlin")
    id ("kotlin-parcelize")
}

android {
    namespace = "org.gnunet.gnunetmessenger"
    compileSdk = 35

    defaultConfig {
        applicationId = "org.gnunet.gnunetmessenger"
        minSdk = 24
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    flavorDimensions += "user"
    
    productFlavors {
        create("alice") {
            dimension = "user"
            applicationId = "org.gnunet.gnunetmessenger"
            versionNameSuffix = "-alice"
            resValue("string", "app_name", "GNUnet Messenger")
            resValue("string", "default_account_name", "Alice")
        }

        create("bob") {
            dimension = "user"
            applicationId = "org.gnunet.gnunetmessenger.bob"
            versionNameSuffix = "-bob"
            resValue("string", "app_name", "GNUnet Messenger 2")
            resValue("string", "default_account_name", "Bob")
        }
    }
    
    buildTypes {
        release {
            isMinifyEnabled = false
            signingConfig = signingConfigs.getByName("debug")
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            buildConfigField("boolean", "ALLOW_RESET", "false")
        }
        debug {
            buildConfigField("boolean", "ALLOW_RESET", "true")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    kotlinOptions {
        jvmTarget = "11"
    }
    buildFeatures {
        dataBinding = true
        viewBinding = true
        aidl = true
        buildConfig = true
    }
}

dependencies {

    implementation(libs.camerax.core)
    implementation(libs.camerax.camera2)
    implementation(libs.camerax.lifecycle)
    implementation(libs.camerax.view)
    implementation(libs.mlkit.barcode)
    implementation(libs.zxingcore)
    implementation(libs.zxingandroidembedded)
    implementation(libs.androidx.cardview)
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.androidx.navigation.fragment)
    implementation(libs.androidx.navigation.ui)
    implementation(libs.material)
    implementation(libs.androidx.navigation.fragment.ktx)
    implementation(libs.androidx.navigation.ui.ktx)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.8.1")
    androidTestImplementation("org.jetbrains.kotlinx:kotlinx-coroutines-test:1.8.1")
    testImplementation("junit:junit:4.13.2")
}
