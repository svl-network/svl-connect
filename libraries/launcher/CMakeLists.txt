cmake_minimum_required(VERSION 3.15)
project(launcher Java)
find_package(Java 1.7 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT net.sunveil.connect.EntryPoint)
set(CMAKE_JAVA_COMPILE_FLAGS -target 8 -source 8)

set(SRC
    net/sunveil/connect/EntryPoint.java
    net/sunveil/connect/SystemProperties.java
    net/sunveil/connect/launcher/Launcher.java
    net/sunveil/connect/launcher/impl/AbstractLauncher.java
    net/sunveil/connect/launcher/impl/StandardLauncher.java
    net/sunveil/connect/exception/ParameterNotFoundException.java
    net/sunveil/connect/exception/ParseException.java
    net/sunveil/connect/utils/Parameters.java
    net/sunveil/connect/utils/ReflectionUtils.java
    net/sunveil/connect/utils/logging/Level.java
    net/sunveil/connect/utils/logging/Log.java
    net/sunveil/connect/legacy/LegacyProxy.java
)

set(LEGACY_SRC
    legacy/net/sunveil/connect/legacy/LegacyFrame.java
    legacy/net/sunveil/connect/legacy/LegacyLauncher.java
    legacy/net/sunveil/connect/legacy/fix/online/Handler.java
    legacy/net/sunveil/connect/legacy/fix/online/OnlineFixes.java
    legacy/net/sunveil/connect/legacy/fix/online/OnlineModeFix.java
    legacy/net/sunveil/connect/legacy/fix/online/SkinFix.java
    legacy/net/sunveil/connect/legacy/utils/Base64.java
    legacy/net/sunveil/connect/legacy/utils/api/MojangApi.java
    legacy/net/sunveil/connect/legacy/utils/api/Texture.java
    legacy/net/sunveil/connect/legacy/utils/json/JsonParseException.java
    legacy/net/sunveil/connect/legacy/utils/json/JsonParser.java
    legacy/net/sunveil/connect/legacy/utils/url/ByteArrayUrlConnection.java
    legacy/net/sunveil/connect/legacy/utils/url/UrlUtils.java
    legacy/net/minecraft/Launcher.java
    legacy/net/sunveil/connect/legacy/LegacyProxy.java
)

add_jar(NewLaunch ${SRC})
add_jar(NewLaunchLegacy ${LEGACY_SRC} INCLUDE_JARS NewLaunch)
install_jar(NewLaunch "${JARS_DEST_DIR}")
install_jar(NewLaunchLegacy "${JARS_DEST_DIR}")

