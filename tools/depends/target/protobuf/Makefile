-include ../../Makefile.include
DEPS=Makefile 0001-CMake-Provide-missing-Findandroid_log.cmake.patch

# lib name, version
LIBNAME=protobuf
LANG=cpp
VERSION=3.11.2
#SOURCE=$(LIBNAME)-$(LANG)-$(VERSION)
SOURCE=v$(VERSION)-p0
ARCHIVE=$(SOURCE).tar.gz

# The build directory
BUILDDIR=$(PLATFORM)/build-cmake # 'build' conflicts with file BUILD on case-insensitive FS

# The compiled library
LIBDYLIB=$(BUILDDIR)/cmake/lib$(LIBNAME).a

ifeq ($(PLATFORM),)
	# Building stand-alone
	ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
	PLATFORM = native
	TARBALLS_LOCATION = $(ROOT_DIR)
	BASE_URL := http://mirrors.kodi.tv/build-deps/sources
	RETRIEVE_TOOL := curl -Ls --create-dirs -f -O
	ARCHIVE_TOOL := tar --strip-components=1 -xf
else
	# Building as part of depends
	DEPS += ../../Makefile.include
	CMAKE_OPTIONS := \
	  -Dprotobuf_BUILD_PROTOC_BINARIES=OFF \
	  $(CMAKE_OPTIONS)
endif

# TODO
#BASE_URL=https://github.com/protocolbuffers/protobuf/releases/download/v$(VERSION)
BASE_URL=https://github.com/cpp-pm/protobuf/archive

CMAKE_OPTIONS := \
  -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
  -DHUNTER_ENABLED=OFF \
  -Dprotobuf_BUILD_TESTS=OFF \
  $(CMAKE_OPTIONS)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../0001-CMake-Provide-missing-Findandroid_log.cmake.patch
endif

$(LIBDYLIB): $(PLATFORM)
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..
	$(MAKE) -C $(BUILDDIR) -j$(getconf _NPROCESSORS_ONLN)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
