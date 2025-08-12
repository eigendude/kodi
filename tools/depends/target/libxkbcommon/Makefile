include ../../Makefile.include
DEPS =../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libxkbcommon
VERSION=1.11.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=f434c816c1212bd27e259bba22b0db7de7c58a0f22a1d4aef387515d9fb9595d2f0e515642a287a6f2791bd439ccf771771b76780ab4f564172c04c4d6a48b11
FULL_URL=https://github.com/xkbcommon/libxkbcommon/archive/refs/tags/xkbcommon-$(VERSION).tar.gz
include ../../download-files.include

# configuration settings
MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

# configuration settings
CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson \
                --buildtype=$(MESON_BUILD_TYPE) \
                --prefix=$(PREFIX) \
                -Ddefault_library=static \
                -Denable-docs=false \
                -Denable-x11=false \
                -Denable-wayland=false \
                -Denable-xkbregistry=false \
                -Dxkb-config-root=/usr/share/X11/xkb

ifeq ($(CROSS_COMPILING), yes)
CONFIGURE += --cross-file $(PREFIX)/share/cross-file.meson
endif

LIBDYLIB=$(PLATFORM)/build/libxkbcommon.a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE) . build

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
