#!/bin/bash

API_LEVEL="30"
NDK_ROOT="/opt/android-ndk"
SDK_ROOT="/opt/android-sdk"

CC="$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/clang"
CFLAGS="-g3 -std=c11 -Wall -Wextra -Wno-unused-parameter -Werror -I$NDK_ROOT/sources/android/native_app_glue"
LDFLAGS="-shared -llog -landroid -lGLESv2 -lEGL"

set -e
mkdir -p build/lib/{armeabi-v7a,arm64-v8a,x86,x86_64}
cd build

$CC $CFLAGS -target arm-linux-androideabi$API_LEVEL -o lib/armeabi-v7a/libmyapp.so ../src/main.c $LDFLAGS &
$CC $CFLAGS -target aarch64-linux-android$API_LEVEL -o lib/arm64-v8a/libmyapp.so   ../src/main.c $LDFLAGS &
$CC $CFLAGS -target  x86_64-linux-android$API_LEVEL -o lib/x86_64/libmyapp.so      ../src/main.c $LDFLAGS &
$CC $CFLAGS -target    i686-linux-android$API_LEVEL -o lib/x86/libmyapp.so         ../src/main.c $LDFLAGS -fPIC

[ -f "debug.keystore" ] || keytool -genkey -validity 100000 \
	-dname "CN=Android Debug,O=Android,C=US" -keystore debug.keystore \
	-alias debugkey -storepass android -keypass android -keyalg RSA

rm -f myapp.apk temp.apk
aapt2 link --manifest ../AndroidManifest.xml --target-sdk-version $API_LEVEL \
	-I $SDK_ROOT/platforms/android-$API_LEVEL/android.jar -o temp.apk
zip -qr temp.apk lib
jarsigner -storepass android -keystore debug.keystore temp.apk debugkey >/dev/null
zipalign 4 temp.apk myapp.apk
apksigner sign --key-pass pass:android --ks-pass pass:android --ks debug.keystore myapp.apk
