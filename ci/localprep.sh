export APT_ARGS="-y --no-install-recommends --no-upgrade"
apt-get update && apt-get install $APT_ARGS git wget unzip && rm -rf /var/lib/apt/lists/*
apt-get update && apt-get install $APT_ARGS g++ && rm -rf /var/lib/apt/lists/*
apt-get update && apt-get install $APT_ARGS autotools-dev libtool m4 automake autoconf pkg-config && rm -rf /var/lib/apt/lists/*
apt-get update && apt-get install $APT_ARGS zlib1g-dev libssl1.0-dev curl ccache bsdmainutils cmake && rm -rf /var/lib/apt/lists/*
apt-get update && apt-get install $APT_ARGS python3 python3-dev && rm -rf /var/lib/apt/lists/*
apt-get update && apt-get install $APT_ARGS python python3-pip python3-setuptools && rm -rf /var/lib/apt/lists/*

# Python stuff
pip3 install pyzmq # really needed?
pip3 install jinja2

# Packages needed for all target builds
dpkg --add-architecture i386
apt-get update && apt-get install $APT_ARGS g++-7-multilib && rm -rf /var/lib/apt/lists/*
apt-get update && apt-get install $APT_ARGS g++-arm-linux-gnueabihf && rm -rf /var/lib/apt/lists/*
apt-get update && apt-get install $APT_ARGS g++-mingw-w64-i686 && rm -rf /var/lib/apt/lists/*
apt-get update && apt-get install $APT_ARGS g++-mingw-w64-x86-64 && rm -rf /var/lib/apt/lists/*
apt-get update && apt-get install $APT_ARGS wine-stable wine32 wine64 bc nsis && rm -rf /var/lib/apt/lists/*
apt-get update && apt-get install $APT_ARGS python python3-zmq && rm -rf /var/lib/apt/lists/*
apt-get update && apt-get install $APT_ARGS imagemagick libcap-dev librsvg2-bin libz-dev libbz2-dev libtiff-tools && rm -rf /var/lib/apt/lists/*
apt-get update && apt-get install $APT_ARGS make binutils-gold bsdmainutils patch bison libgmp-dev libgl1-mesa-dev libevent-dev libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-test-dev libboost-thread-dev libboost-program-options-dev curl libminiupnpc-dev screen openjdk-8-jdk openjdk-8-jdk-headless openjdk-8-jre openjdk-8-jre-headless cmake libtinfo5 xorriso && rm -rf /var/lib/apt/lists/*

# This is a hack. It is needed because gcc-multilib and g++-multilib are conflicting with g++-arm-linux-gnueabihf. This is
# due to gcc-multilib installing the following symbolic link, which is needed for -m32 support. However, this causes
# arm builds to also have the asm folder implicitely in the include search path. This is kind of ok, because the asm folder
# for arm has precedence.
ln -s x86_64-linux-gnu/asm /usr/include/asm

# Make sure std::thread and friends is available

  update-alternatives --set i686-w64-mingw32-gcc /usr/bin/i686-w64-mingw32-gcc-posix; 
  update-alternatives --set i686-w64-mingw32-g++  /usr/bin/i686-w64-mingw32-g++-posix; 
  update-alternatives --set x86_64-w64-mingw32-gcc  /usr/bin/x86_64-w64-mingw32-gcc-posix; 
  update-alternatives --set x86_64-w64-mingw32-g++  /usr/bin/x86_64-w64-mingw32-g++-posix; 
 

mkdir /crown-src 
mkdir -p /cache/ccache 
mkdir /cache/depends
mkdir /cache/sdk-sources

