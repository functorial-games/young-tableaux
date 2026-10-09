# ICK compiles maintained app C; the declared NDK boundary owns the Lua runtime,
# unmodified NativeActivity glue, assembly and platform linking.
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
AICI_ICK_ROOT ?= $(ROOT)/.ai-ci-ick
ICK_STAGE ?= $(ROOT)/.ick-stages/$(ABI)
BUILD ?= $(ROOT)/build/ick-objects/$(ABI)
OUT ?= $(BUILD)/libyoungtableaux.so
FORTIFY_SOURCE = 2
include $(AICI_ICK_ROOT)/ick-android/Makefile
.DEFAULT_GOAL := native
ifneq ($(filter $(ABI),armeabi-v7a arm64-v8a),$(ABI))
$(error Young Tableaux packages require armeabi-v7a or arm64-v8a)
endif
TARGET_FLAGS_armeabi-v7a = -marm -march=armv7-a -mfpu=neon -mfloat-abi=softfp
GLUE = $(NDK)/sources/android/native_app_glue
SOURCES = core/parse core/diagram core/partition core/tableau core/rsk core/jeu_de_taquin core/algebra core/combinatorics ui/controls console/console console/nearby render/raster render/paint lua/bridge android/native_main
OBJECTS = $(addprefix $(BUILD)/,$(addsuffix .o,$(SOURCES))) $(BUILD)/lua-runtime.o $(BUILD)/native_app_glue.o
INCLUDES = $(addprefix -I"$(ROOT)/app/,$(addsuffix ",core ui console render lua)) -I"$(ROOT)/third_party/lua" -isystem "$(GLUE)"
FLAGS = -std=c17 -O2 -fPIC -ffunction-sections -fdata-sections -fstack-protector-strong -Wall -Wextra -Werror -Wpedantic -Wshadow
.PHONY: native check-owned-producer
native: $(OUT)
	"$(NDK_READELF)" -h -l "$(OUT)"
	sha256sum "$(OUT)"

check-owned-producer:
	test -x "$(ICK_COMPILER)"
	test "$$("$(ICK_COMPILER)" $(ICK_EXTRA_FLAGS) -dumpmachine)" = "$(EXPECTED_TARGET)"
	test -f "$(ICK_BUILTIN_INCLUDE)/stdatomic.h"
	test -f "$(ICK_BUILTIN_INCLUDE)/stddef.h"
	test "$$(git -C "$(ROOT)/third_party/lua" rev-parse HEAD)" = 87306483cec50f8c750a22dda1d0742246fad756
	test "$$(git -C "$(ROOT)/third_party/lua" rev-parse 'HEAD^{tree}')" = 6b2968928611542c8f82448f49c32ea8bc98b559
	test -z "$$(git -C "$(ROOT)/third_party/lua" status --porcelain)"

$(BUILD)/%.s: $(ROOT)/app/%.c FORCE | check-owned-producer
	mkdir -p "$(@D)"
	"$(ICK_COMPILER)" $(ICK_EXTRA_FLAGS) $(ICK_NDK_FLAGS) $(FLAGS) -D__ANDROID_API__=26 -D__ANDROID_MIN_SDK_VERSION__=26 $(INCLUDES) -S "$<" -o "$@"

$(BUILD)/%.o: $(BUILD)/%.s
	"$(NDK_CC)" $(TARGET_FLAGS) --sysroot="$(NDK_SYSROOT)" -c "$<" -o "$@"

$(BUILD)/lua-runtime.o: $(ROOT)/third_party/lua/onelua.c FORCE | check-owned-producer
	mkdir -p "$(@D)"
	"$(NDK_CC)" $(TARGET_FLAGS) --sysroot="$(NDK_SYSROOT)" -std=c17 -O2 -fPIC -DMAKE_LIB -w -I"$(ROOT)/third_party/lua" -c "$<" -o "$@"

$(BUILD)/native_app_glue.o: FORCE | check-owned-producer
	mkdir -p "$(@D)"
	"$(NDK_CC)" $(TARGET_FLAGS) --sysroot="$(NDK_SYSROOT)" -std=c17 -O2 -fPIC -isystem "$(GLUE)" -c "$(GLUE)/android_native_app_glue.c" -o "$@"

$(OUT): $(OBJECTS)
	mkdir -p "$(@D)"
	"$(NDK_CC)" $(TARGET_FLAGS) --sysroot="$(NDK_SYSROOT)" -shared -Wl,--no-undefined -Wl,--gc-sections -Wl,-z,relro,-z,now -Wl,-z,max-page-size=16384 -Wl,-u,ANativeActivity_onCreate $(OBJECTS) -landroid -llog -lm -o "$@"
