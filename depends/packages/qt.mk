PACKAGE=qt
$(package)_version=5.13.2
$(package)_download_path=https://download.qt.io/archive/qt/5.13/$($(package)_version)/submodules
$(package)_suffix=everywhere-src-$($(package)_version).tar.xz
$(package)_file_name=qtbase-$($(package)_suffix)
$(package)_sha256_hash=26b6b686d66a7ad28eaca349e55e2894e5a735f3831e45f2049e93b1daa92121
$(package)_dependencies=zlib
$(package)_linux_dependencies=fontconfig libxcb libxkbcommon libxcb_util libxcb_util_render libxcb_util_keysyms libxcb_util_image libxcb_util_wm
$(package)_qt_libs=corelib network widgets gui plugins testlib
$(package)_patches=fix_qt_pkgconfig.patch mac-qmake.conf fix_no_printer.patch no-xlib.patch
$(package)_patches+= fix_android_jni_static.patch dont_hardcode_pwd.patch
$(package)_patches+= drop_lrelease_dependency.patch fix_qpainter_non_determinism.patch
$(package)_patches+= fix_android_qmake_conf.patch fix_android_pch.patch
$(package)_patches+= fix_mingw_cross_compile.patch fix_xkbcommon.patch
$(package)_patches+= .configure qt.pro .gitmodules fix_git_modules.patch
$(package)_patches+= fix_declarative.patch fix_bigsur_drawing.patch fix_lib_paths.patch
$(package)_patches+= qtbase-moc-ignore-gcc-macro.patch no_sdk_version_check.patch fix_linux_opengl.patch

$(package)_qttranslations_file_name=qttranslations-$($(package)_suffix)
$(package)_qttranslations_sha256_hash=25755941a2525de2d7ae48e0011d04db7cc09e4e73fe83293206ceafa0aa82d9

$(package)_qttools_file_name=qttools-$($(package)_suffix)
$(package)_qttools_sha256_hash=919a2713b6d2d7873a09ad85bd93cf4282606e5bf84d5884250f665a253ec06e

$(package)_qtdeclarative_file_name=qtdeclarative-$($(package)_suffix)
$(package)_qtdeclarative_sha256_hash=d9a524f45fe9e136cda2252f9d7013ec17046d907e3f39606db920987c22d1fd

$(package)_qtcharts_file_name=qtcharts-$($(package)_suffix)
$(package)_qtcharts_sha256_hash=3bad81c3cfb32cf72fb0ce2ac2794d031cf78a3902b4715f89c09b2d0e041e87

$(package)_qtgraphicaleffects_file_name=qtgraphicaleffects-$($(package)_suffix)
$(package)_qtgraphicaleffects_sha256_hash=297a89bb6c771f849c4ce866e5c98dadf665163b3dab03bc48a58f51424e7e66

$(package)_qtquickcontrols2_file_name=qtquickcontrols2-$($(package)_suffix)
$(package)_qtquickcontrols2_sha256_hash=90ee8be7b66cc65f3f22e71a0b35adab5c169ac4f8ebc6f9e7685228bf8a7d70

$(package)_qtandroidextras_file_name=qtandroidextras-$($(package)_suffix)
$(package)_qtandroidextras_sha256_hash=403e8f463552564333b1a4d2e0a52c28b27296d096737e55a2327642a7277af8

$(package)_extra_sources  = $($(package)_qttranslations_file_name)
$(package)_extra_sources += $($(package)_qttools_file_name)
$(package)_extra_sources += $($(package)_qtcharts_file_name)
$(package)_extra_sources += $($(package)_qtandroidextras_file_name)
$(package)_extra_sources += $($(package)_qtgraphicaleffects_file_name)
$(package)_extra_sources += $($(package)_qtdeclarative_file_name)
$(package)_extra_sources += $($(package)_qtquickcontrols2_file_name)

define $(package)_set_vars
$(package)_config_opts_release = -release
$(package)_config_opts_debug = -debug
$(package)_config_opts += -bindir $(build_prefix)/bin
$(package)_config_opts += -c++std c++1z
$(package)_config_opts += -confirm-license
$(package)_config_opts += -hostprefix $(build_prefix)
$(package)_config_opts += -no-compile-examples
$(package)_config_opts += -no-cups
$(package)_config_opts += -no-egl
$(package)_config_opts += -no-eglfs
$(package)_config_opts += -no-gif
$(package)_config_opts += -no-glib
$(package)_config_opts += -no-icu
$(package)_config_opts += -no-ico
$(package)_config_opts += -no-iconv
$(package)_config_opts += -no-kms
$(package)_config_opts += -no-linuxfb
$(package)_config_opts += -no-libjpeg
$(package)_config_opts += -no-libproxy
$(package)_config_opts += -no-libudev
$(package)_config_opts += -no-mtdev
$(package)_config_opts += -no-openssl
$(package)_config_opts += -no-openvg
$(package)_config_opts += -no-reduce-relocations
$(package)_config_opts += -no-schannel
$(package)_config_opts += -no-sctp
$(package)_config_opts += -no-securetransport
$(package)_config_opts += -no-sql-db2
$(package)_config_opts += -no-sql-ibase
$(package)_config_opts += -no-sql-oci
$(package)_config_opts += -no-sql-tds
$(package)_config_opts += -no-sql-mysql
$(package)_config_opts += -no-sql-odbc
$(package)_config_opts += -no-sql-psql
$(package)_config_opts += -no-sql-sqlite
$(package)_config_opts += -no-sql-sqlite2
$(package)_config_opts += -no-system-proxies
$(package)_config_opts += -no-use-gold-linker
$(package)_config_opts += -no-zstd
$(package)_config_opts += -nomake examples
$(package)_config_opts += -nomake tests
$(package)_config_opts += -opensource
$(package)_config_opts += -pch
$(package)_config_opts += -pkg-config
$(package)_config_opts += -prefix $(host_prefix)
$(package)_config_opts += -qt-libpng
$(package)_config_opts += -qt-pcre
$(package)_config_opts += -qt-harfbuzz
$(package)_config_opts += -system-zlib
$(package)_config_opts += -static
$(package)_config_opts += -silent
$(package)_config_opts += -v
$(package)_config_opts += -no-feature-bearermanagement
$(package)_config_opts += -no-feature-colordialog
$(package)_config_opts += -no-feature-concurrent
$(package)_config_opts += -no-feature-dial
$(package)_config_opts += -no-feature-filesystemwatcher
$(package)_config_opts += -no-feature-ftp
$(package)_config_opts += -no-feature-image_heuristic_mask
$(package)_config_opts += -no-feature-keysequenceedit
$(package)_config_opts += -no-feature-lcdnumber
$(package)_config_opts += -no-feature-networkdiskcache
$(package)_config_opts += -no-feature-pdf
$(package)_config_opts += -no-feature-printdialog
$(package)_config_opts += -no-feature-printer
$(package)_config_opts += -no-feature-printpreviewdialog
$(package)_config_opts += -no-feature-printpreviewwidget
$(package)_config_opts += -no-feature-sessionmanager
$(package)_config_opts += -no-feature-statemachine
$(package)_config_opts += -no-feature-syntaxhighlighter
$(package)_config_opts += -no-feature-textodfwriter
$(package)_config_opts += -no-feature-topleveldomain
$(package)_config_opts += -no-feature-undocommand
$(package)_config_opts += -no-feature-undogroup
$(package)_config_opts += -no-feature-undostack
$(package)_config_opts += -no-feature-undoview
$(package)_config_opts += -no-feature-vnc

$(package)_config_opts_darwin = -no-dbus
$(package)_config_opts_darwin += -no-opengl
$(package)_config_opts_darwin += -pch
$(package)_config_opts_darwin += QMAKE_MACOSX_DEPLOYMENT_TARGET=$(OSX_MIN_VERSION)

ifneq ($(build_os),darwin)
$(package)_config_opts_darwin += -xplatform macx-clang-linux
$(package)_config_opts_darwin += -device-option MAC_SDK_PATH=$(OSX_SDK)
$(package)_config_opts_darwin += -device-option MAC_SDK_VERSION=$(OSX_SDK_VERSION)
$(package)_config_opts_darwin += -device-option CROSS_COMPILE="$(host)-"
$(package)_config_opts_darwin += -device-option MAC_TARGET=$(host)
$(package)_config_opts_darwin += -device-option XCODE_VERSION=$(XCODE_VERSION)
endif

# for macOS on Apple Silicon (ARM) see https://bugreports.qt.io/browse/QTBUG-85279
$(package)_config_opts_arm_darwin += -device-option QMAKE_APPLE_DEVICE_ARCHS=arm64

$(package)_config_opts_linux = -xcb
$(package)_config_opts_linux += -no-xcb-xlib
$(package)_config_opts_linux += -no-feature-xlib
$(package)_config_opts_linux += -system-freetype
$(package)_config_opts_linux += -opengl desktop
$(package)_config_opts_linux += -fontconfig
$(package)_config_opts_linux += -no-feature-vulkan
$(package)_config_opts_linux += -dbus-runtime
$(package)_config_opts_linux += -L $(host_prefix)/lib
$(package)_config_opts_linux += -I $(host_prefix)/include

$(package)_config_opts_arm_linux += -platform linux-g++ -xplatform crown-linux-g++
$(package)_config_opts_i686_linux  = -xplatform linux-g++-32
$(package)_config_opts_x86_64_linux = -xplatform linux-g++-64
$(package)_config_opts_aarch64_linux = -xplatform linux-aarch64-gnu-g++
$(package)_config_opts_powerpc64_linux = -platform linux-g++ -xplatform crown-linux-g++
$(package)_config_opts_powerpc64le_linux = -platform linux-g++ -xplatform crown-linux-g++
$(package)_config_opts_riscv64_linux = -platform linux-g++ -xplatform crown-linux-g++
$(package)_config_opts_s390x_linux = -platform linux-g++ -xplatform crown-linux-g++

$(package)_config_opts_mingw32 = -opengl desktop
$(package)_config_opts_mingw32 += -no-dbus
$(package)_config_opts_mingw32 += -xplatform win32-g++
$(package)_config_opts_mingw32 += -no-freetype
$(package)_config_opts_mingw32 += -device-option CROSS_COMPILE="$(host)-"
$(package)_config_opts_mingw32 += -L $(host_prefix)/lib
$(package)_config_opts_mingw32 += -I $(host_prefix)/include
$(package)_config_opts_mingw32 += -pch

$(package)_config_opts_android = -xplatform android-clang
$(package)_config_opts_android += -android-sdk $(ANDROID_SDK)
$(package)_config_opts_android += -android-ndk $(ANDROID_NDK)
$(package)_config_opts_android += -android-ndk-platform android-$(ANDROID_API_LEVEL)
$(package)_config_opts_android += -device-option CROSS_COMPILE="$(host)-"
$(package)_config_opts_android += -egl
$(package)_config_opts_android += -qpa xcb
$(package)_config_opts_android += -no-eglfs
$(package)_config_opts_android += -no-dbus
$(package)_config_opts_android += -opengl es2
$(package)_config_opts_android += -qt-freetype
$(package)_config_opts_android += -no-fontconfig
$(package)_config_opts_android += -L $(host_prefix)/lib
$(package)_config_opts_android += -I $(host_prefix)/include
$(package)_config_opts_android += -pch

$(package)_config_opts_aarch64_android += -android-arch arm64-v8a
$(package)_config_opts_armv7a_android += -android-arch armeabi-v7a
$(package)_config_opts_x86_64_android += -android-arch x86_64
$(package)_config_opts_i686_android += -android-arch i686

$(package)_build_env  = QT_RCC_TEST=1
$(package)_build_env += QT_RCC_SOURCE_DATE_OVERRIDE=1
endef

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttranslations_file_name),$($(package)_qttranslations_file_name),$($(package)_qttranslations_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttools_file_name),$($(package)_qttools_file_name),$($(package)_qttools_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtcharts_file_name),$($(package)_qtcharts_file_name),$($(package)_qtcharts_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtandroidextras_file_name),$($(package)_qtandroidextras_file_name),$($(package)_qtandroidextras_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtgraphicaleffects_file_name),$($(package)_qtgraphicaleffects_file_name),$($(package)_qtgraphicaleffects_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtdeclarative_file_name),$($(package)_qtdeclarative_file_name),$($(package)_qtdeclarative_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtquickcontrols2_file_name),$($(package)_qtquickcontrols2_file_name),$($(package)_qtquickcontrols2_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttranslations_sha256_hash)  $($(package)_source_dir)/$($(package)_qttranslations_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttools_sha256_hash)  $($(package)_source_dir)/$($(package)_qttools_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtdeclarative_sha256_hash)  $($(package)_source_dir)/$($(package)_qtdeclarative_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtcharts_sha256_hash)  $($(package)_source_dir)/$($(package)_qtcharts_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtandroidextras_sha256_hash)  $($(package)_source_dir)/$($(package)_qtandroidextras_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtgraphicaleffects_sha256_hash)  $($(package)_source_dir)/$($(package)_qtgraphicaleffects_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtquickcontrols2_sha256_hash)  $($(package)_source_dir)/$($(package)_qtquickcontrols2_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir qtbase && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source) -C qtbase && \
  mkdir qttranslations && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttranslations_file_name) -C qttranslations && \
  mkdir qttools && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttools_file_name) -C qttools && \
  mkdir qtdeclarative && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtdeclarative_file_name) -C qtdeclarative && \
  mkdir qtcharts && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtcharts_file_name) -C qtcharts && \
  mkdir qtandroidextras && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtandroidextras_file_name) -C qtandroidextras && \
  mkdir qtgraphicaleffects && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtgraphicaleffects_file_name) -C qtgraphicaleffects && \
  mkdir qtquickcontrols2 && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtquickcontrols2_file_name) -C qtquickcontrols2
endef

# Preprocessing steps work as follows:
#
# 1. Apply our patches to the extracted source. See each patch for more info.
#
# 2. Point to lrelease in qttools/bin/lrelease; otherwise Qt will look for it in
# $(host)/native/bin/lrelease and not find it.
#
# 3. Create a macOS-Clang-Linux mkspec using our mac-qmake.conf.
#
# 4. After making a copy of the mkspec for the linux-arm-gnueabi host, named
# crown-linux-g++, replace instances of linux-arm-gnueabi with $(host). This
# way we can generically support hosts like riscv64-linux-gnu, which Qt doesn't
# ship a mkspec for. See it's usage in config_opts_* above.
#
# 5. Put our C, CXX and LD FLAGS into gcc-base.conf. Only used for non-host builds.
#
# 6. Do similar for the win32-g++ mkspec.
#
# 7. In clang.conf, swap out clang & clang++, for our compiler + flags. See #17466.
#
# 8. Adjust a regex in toolchain.prf, to accomodate Guix's usage of
# CROSS_LIBRARY_PATH. See #15277.
define $(package)_preprocess_cmds
  patch -p1 -i $($(package)_patch_dir)/drop_lrelease_dependency.patch && \
  patch -p1 -i $($(package)_patch_dir)/dont_hardcode_pwd.patch && \
  patch -p1 -i $($(package)_patch_dir)/fix_qt_pkgconfig.patch && \
  patch -p1 -i $($(package)_patch_dir)/fix_android_qmake_conf.patch && \
  patch -p1 -i $($(package)_patch_dir)/fix_android_jni_static.patch && \
  patch -p1 -i $($(package)_patch_dir)/fix_android_pch.patch && \
  patch -p1 -i $($(package)_patch_dir)/no-xlib.patch && \
  patch -p1 -i $($(package)_patch_dir)/fix_mingw_cross_compile.patch && \
  patch -p1 -i $($(package)_patch_dir)/fix_xkbcommon.patch && \
  patch -p1 -i $($(package)_patch_dir)/fix_declarative.patch && \
  patch -p1 -i $($(package)_patch_dir)/fix_bigsur_drawing.patch && \
  patch -p1 -i $($(package)_patch_dir)/fix_lib_paths.patch && \
  patch -p1 -i $($(package)_patch_dir)/qtbase-moc-ignore-gcc-macro.patch && \
  patch -p1 -i $($(package)_patch_dir)/no_sdk_version_check.patch && \
  patch -p1 -i $($(package)_patch_dir)/fix_linux_opengl.patch && \
  sed -i.old "s|updateqm.commands = \$$$$\$$$$LRELEASE|updateqm.commands = $($(package)_extract_dir)/qttools/bin/lrelease|" qttranslations/translations/translations.pro && \
  mkdir -p qtbase/mkspecs/macx-clang-linux &&\
  cp -f qtbase/mkspecs/macx-clang/qplatformdefs.h qtbase/mkspecs/macx-clang-linux/ &&\
  cp -f $($(package)_patch_dir)/qt.pro . && \
  cp -f $($(package)_patch_dir)/.gitmodules . && \
  cp -f $($(package)_patch_dir)/.configure configure && \
  patch -p0 -i $($(package)_patch_dir)/fix_git_modules.patch && \
  cp -f $($(package)_patch_dir)/mac-qmake.conf qtbase/mkspecs/macx-clang-linux/qmake.conf && \
  cp -r qtbase/mkspecs/linux-arm-gnueabi-g++ qtbase/mkspecs/crown-linux-g++ && \
  sed -i.old "s/arm-linux-gnueabi-/$(host)-/g" qtbase/mkspecs/crown-linux-g++/qmake.conf && \
  echo "!host_build: QMAKE_CFLAGS     += $($(package)_cflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "!host_build: QMAKE_CXXFLAGS   += $($(package)_cxxflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "!host_build: QMAKE_LFLAGS     += $($(package)_ldflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  sed -i.old "s|QMAKE_CFLAGS           += |!host_build: QMAKE_CFLAGS            = $($(package)_cflags) $($(package)_cppflags) |" qtbase/mkspecs/win32-g++/qmake.conf && \
  sed -i.old "s|QMAKE_CXXFLAGS         += |!host_build: QMAKE_CXXFLAGS            = $($(package)_cxxflags) $($(package)_cppflags) |" qtbase/mkspecs/win32-g++/qmake.conf && \
  sed -i.old "0,/^QMAKE_LFLAGS_/s|^QMAKE_LFLAGS_|!host_build: QMAKE_LFLAGS            = $($(package)_ldflags)\n&|" qtbase/mkspecs/win32-g++/qmake.conf && \
  sed -i.old "s|QMAKE_CC                = clang|QMAKE_CC                = $($(package)_cc)|" qtbase/mkspecs/common/clang.conf && \
  sed -i.old "s|QMAKE_CXX               = clang++|QMAKE_CXX               = $($(package)_cxx)|" qtbase/mkspecs/common/clang.conf && \
  sed -i.old "s/LIBRARY_PATH/(CROSS_)?\0/g" qtbase/mkspecs/features/toolchain.prf
endef

define $(package)_config_cmds
  export PKG_CONFIG_SYSROOT_DIR=/ && \
  export PKG_CONFIG_LIBDIR=$(host_prefix)/lib/pkgconfig && \
  export PKG_CONFIG_PATH=$(host_prefix)/share/pkgconfig  && \
  ./configure $($(package)_config_opts) && \
  echo "host_build: QT_CONFIG ~= s/system-zlib/zlib" >> qtbase/mkspecs/qconfig.pri && \
  echo "CONFIG += force_bootstrap" >> qtbase/mkspecs/qconfig.pri
endef

define $(package)_build_cmds
  $(MAKE) -C .
endef

define $(package)_stage_cmds
  $(MAKE) -C . INSTALL_ROOT=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf native/mkspecs/ native/lib/ lib/cmake/ && \
  rm -f lib/lib*.la lib/*.prl plugins/*/*.prl
endef
