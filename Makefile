all:
	gcc -g -Wall -Wextra main.c ring_buffer.c -o pkt-rotator

android:
	CC=/path/to/android-ndk-standalone-toolchain/bin/arm-linux-androideabi-gcc
	CFLAGS = -fPIC -fvisibility=default -pie -fPIE -mbionic -march=armv5te -mfloat-abi=soft -mfpu=vfp -mtls-dialect=gnu
	$(CC) -g -Wall -Wextra $(CFLAGS) main.c ring_buffer.c -o pkt-rotator-android
