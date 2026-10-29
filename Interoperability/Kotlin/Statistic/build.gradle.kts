// Statistic/build.gradle.kts
//
// Kotlin/Native (linuxX64) application that calls libstatslib.so through cinterop.
//
// Flow:  cmakeConfigure -> cmakeBuild -> (cinterop + compile) -> link -> run/test
//   * cmakeBuild compiles native/ and drops libstatslib.so into ./lib
//   * cinterop reads native/include/statistic/stats_c_api.h (see statslib.def)
//   * the final executable links against ./lib/libstatslib.so
//
// Useful commands:
//   ./gradlew runReleaseExecutableLinuxX64            run on data/sample.csv
//   ./gradlew runReleaseExecutableLinuxX64 -Pstatistic.csv=data/other.csv
//   ./gradlew linuxX64Test                            Kotlin interop tests
//   ./gradlew nativeTest                              C++ tests (ctest)
//   ./gradlew check                                   both test suites

plugins {
    kotlin("multiplatform") version "2.4.0"
}

group = "statistic"
version = "1.0.0"

repositories {
    mavenCentral()
}

// ---------------------------------------------------------------------------
// Paths and optional switches
// ---------------------------------------------------------------------------
val nativeDir: File = layout.projectDirectory.dir("native").asFile
val nativeBuildDir: File = nativeDir.resolve("build")
val libDir: File = layout.projectDirectory.dir("lib").asFile
val libFile: File = libDir.resolve("libstatslib.so")

// -Pstatistic.staticCxx=ON bundles libstdc++ into the .so (see native/CMakeLists.txt).
val staticCxx: String = providers.gradleProperty("statistic.staticCxx").getOrElse("OFF")
// CSV used by the run task; relative paths are resolved against the project dir.
val csvPath: String = providers.gradleProperty("statistic.csv").getOrElse("data/sample.csv")

// ---------------------------------------------------------------------------
// C++ build (CMake) wired into Gradle
// ---------------------------------------------------------------------------
val cmakeConfigure = tasks.register("cmakeConfigure", Exec::class) {
    group = "native"
    description = "Configures the C++ project with CMake."
    inputs.file(nativeDir.resolve("CMakeLists.txt"))
    inputs.property("staticCxx", staticCxx)
    outputs.file(nativeBuildDir.resolve("CMakeCache.txt"))
    commandLine(
        "cmake",
        "-S", nativeDir.absolutePath,
        "-B", nativeBuildDir.absolutePath,
        "-DCMAKE_BUILD_TYPE=Release",
        "-DSTATISTIC_LIB_OUTPUT_DIR=${libDir.absolutePath}",
        "-DSTATISTIC_STATIC_CXX_RUNTIME=$staticCxx",
    )
}

val cmakeBuild = tasks.register("cmakeBuild", Exec::class) {
    group = "native"
    description = "Builds libstatslib.so into ./lib."
    dependsOn(cmakeConfigure)
    inputs.files(fileTree(nativeDir) { include("CMakeLists.txt", "include/**", "src/**") })
    inputs.property("staticCxx", staticCxx)
    outputs.file(libFile)
    commandLine("cmake", "--build", nativeBuildDir.absolutePath, "--target", "statslib", "--parallel")
}

val nativeTest = tasks.register("nativeTest", Exec::class) {
    group = "verification"
    description = "Builds and runs the C++ tests (ctest)."
    dependsOn(cmakeConfigure)
    commandLine(
        "sh", "-c",
        "cmake --build '${nativeBuildDir.absolutePath}' --parallel && " +
                "ctest --test-dir '${nativeBuildDir.absolutePath}' --output-on-failure",
    )
}

// ---------------------------------------------------------------------------
// Kotlin/Native
// ---------------------------------------------------------------------------
kotlin {
    // Every cinterop-generated declaration is marked @ExperimentalForeignApi.
    compilerOptions {
        optIn.add("kotlinx.cinterop.ExperimentalForeignApi")
    }

    linuxX64 {

        compilations.getByName("main") {
            cinterops {
                // Name == src/nativeInterop/cinterop/statslib.def (default lookup).
                create("statslib") {
                    // Resolves "statistic/stats_c_api.h" from the .def file.
                    includeDirs(nativeDir.resolve("include"))
                }
            }
        }

        binaries {
            executable {
                baseName = "statistic"
                entryPoint = "statistic.main"
            }

            all {
                linkerOpts(
                    "-L${libDir.absolutePath}",
                    "-Wl,-rpath,${libDir.absolutePath}"
                )
            }
        }
    }

    sourceSets {
        named("linuxX64Main") {
            kotlin.srcDir("src/main/kotlin")
            resources.srcDir("src/main/resources")
        }

        named("linuxX64Test") {
            kotlin.srcDir("src/test/kotlin")
            resources.srcDir("src/test/resources")
            dependencies {
                implementation(kotlin("test"))
            }
        }
    }
}

// The .so must exist before any native link step (executable and tests).
tasks.withType<org.jetbrains.kotlin.gradle.tasks.CInteropProcess>().configureEach {
    dependsOn(cmakeBuild)
}

// Run tasks execute from the project dir so "data/sample.csv" resolves, and receive the CSV path.
tasks.withType<Exec>()
    .matching { it.name.startsWith("run") && it.name.endsWith("ExecutableLinuxX64") }
    .configureEach {
        workingDir = projectDir
        args(csvPath)
    }

tasks.matching { it.name == "check" }.configureEach {
    dependsOn(nativeTest)
}

// `clean` also removes the CMake build tree and the generated .so.
tasks.withType<Delete>().matching { it.name == "clean" }.configureEach {
    delete(nativeBuildDir, libFile)
}
