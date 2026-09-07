plugins {
    java
    id("io.quarkus") version "3.39.2"
}

repositories {
    mavenCentral()
}

dependencies {
    implementation(enforcedPlatform("io.quarkus.platform:quarkus-bom:3.39.2"))
    implementation("io.quarkus:quarkus-arc")
    implementation("io.quarkus:quarkus-jackson")

    testImplementation("io.quarkus:quarkus-junit5")
}

group = "org.visor"
version = "1.0.0-SNAPSHOT"

java {
    sourceCompatibility = JavaVersion.VERSION_21
    targetCompatibility = JavaVersion.VERSION_21
}

tasks.withType<JavaCompile> {
    options.encoding = "UTF-8"
    // プレビュー機能の有効化とパラメータ保持を1つのブロックにまとめました
    options.compilerArgs.addAll(listOf("-parameters", "--enable-preview"))
    options.release.set(21)
}

tasks.withType<io.quarkus.gradle.tasks.QuarkusDev> {
    jvmArgs = listOf("--enable-preview")
}