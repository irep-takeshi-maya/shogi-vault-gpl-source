plugins {
    id("com.android.application")
    // The Flutter Gradle Plugin must be applied after the Android and Kotlin Gradle plugins.
    id("dev.flutter.flutter-gradle-plugin")
}

android {
    namespace = "com.shogiairesearch.shogi_ai_research"
    compileSdk = flutter.compileSdkVersion
    ndkVersion = flutter.ndkVersion

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    defaultConfig {
        // TODO: Specify your own unique Application ID (https://developer.android.com/studio/build/application-id.html).
        applicationId = "com.shogiairesearch.shogi_ai_research"
        // You can update the following values to match your application needs.
        // For more information, see: https://flutter.dev/to/review-gradle-config.
        minSdk = flutter.minSdkVersion
        targetSdk = flutter.targetSdkVersion
        versionCode = flutter.versionCode
        versionName = flutter.versionName

        // 組込みエンジン(libyaneuraou.so)を実機(arm64-v8a)とエミュレータ(x86_64)の
        // 両方に同梱する。docs/ios_engine_integration_research.md §4.5。
        ndk {
            abiFilters += listOf("arm64-v8a", "x86_64")
        }

        // CMake の externalNativeBuild が走る ABI を明示的に限定する。
        // これを指定しないと 32bit(armeabi-v7a/x86)でもビルドされ、YaneuraOu の
        // __int128(64bit専用)でコンパイルに失敗する。
        externalNativeBuild {
            cmake {
                abiFilters += listOf("arm64-v8a", "x86_64")
            }
        }
    }

    // ネイティブの組込みエンジンを CMake でビルドして APK に同梱する。
    externalNativeBuild {
        cmake {
            path = file("../../native/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildTypes {
        release {
            // TODO: Add your own signing config for the release build.
            // Signing with the debug keys for now, so `flutter run --release` works.
            signingConfig = signingConfigs.getByName("debug")
        }
    }
}

kotlin {
    compilerOptions {
        jvmTarget = org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_17
    }
}

flutter {
    source = "../.."
}
