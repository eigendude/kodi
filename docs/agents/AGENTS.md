# Agent Setup

To build Kodi in this container you'll need numerous development packages. Install them with `apt`:

```bash
sudo apt-get update
sudo apt-get install -y \
  debhelper autoconf automake autopoint gettext autotools-dev cmake curl default-jre \
  doxygen gawk gcc gdc gperf libasound2-dev libass-dev libavahi-client-dev \
  libavahi-common-dev libbluetooth-dev libbluray-dev libbz2-dev libcdio-dev \
  libp8-platform-dev libcrossguid-dev libcurl4-openssl-dev libcwiid-dev \
  libdbus-1-dev libdrm-dev libegl1-mesa-dev libenca-dev libexiv2-dev libflac-dev \
  libfmt-dev libfontconfig-dev libfreetype6-dev libfribidi-dev libfstrcmp-dev \
  libgcrypt-dev libgif-dev libgles2-mesa-dev libgl1-mesa-dev libglu1-mesa-dev \
  libgnutls28-dev libgpg-error-dev libgtest-dev libiso9660-dev libjpeg-dev \
  liblcms2-dev libltdl-dev liblzo2-dev libmicrohttpd-dev libmysqlclient-dev \
  libnfs-dev libogg-dev libpcre2-dev libplist-dev libpng-dev libpulse-dev \
  libshairplay-dev libsmbclient-dev libspdlog-dev libsqlite3-dev libssl-dev \
  libtag1-dev libtiff5-dev libtinyxml-dev libtinyxml2-dev libtool libudev-dev \
  libunistring-dev libva-dev libvdpau-dev libvorbis-dev libxmu-dev libxrandr-dev \
  libxslt1-dev libxt-dev lsb-release meson nasm ninja-build nlohmann-json3-dev \
  python3-dev python3-pil python3-pip swig unzip uuid-dev zip zlib1g-dev
```

Adjust the package list if any dependencies are unavailable on your platform.
