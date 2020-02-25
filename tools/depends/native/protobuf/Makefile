include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS=../../Makefile.include Makefile

# lib name, version
LIBNAME=protobuf
VERSION=3.11.2
SOURCE=v$(VERSION)-p0
ARCHIVE=$(SOURCE).tar.gz
#BASE_URL=http://mirrors.kodi.tv/build-deps/sources
# TODO
BASE_URL=https://github.com/cpp-pm/protobuf/archive

# The build directory
BUILDDIR=$(PLATFORM)/build-cmake # 'build' conflicts with file BUILD on case-insensitive FS

# The generated executable
APP=$(BUILDDIR)/cmake/protoc

# This package uses CMake for build on native, so there is no separate
# toolchain file. Still we have to unset the CMAKE_TOOLCHAIN_FILE, which is
# part of $(CMAKE) and set to the target toolchain file.
CMAKE_OPTIONS := \
  -DCMAKE_TOOLCHAIN_FILE= \
  -DCMAKE_BUILD_TYPE=Release \
  -DHUNTER_ENABLED=OFF \
  -Dprotobuf_BUILD_TESTS=OFF \
  $(CMAKE_OPTIONS)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(APP): $(PLATFORM)
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..
	$(MAKE) -C $(BUILDDIR) -j$(getconf _NPROCESSORS_ONLN)

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
