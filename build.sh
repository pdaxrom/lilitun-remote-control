#!/bin/bash

TARGET_OS=$1
shift

if [ "$TARGET_OS" = "" ]; then
    TARGET_OS="Linux"
fi

case "$TARGET_OS" in
Windows) ;;
MacOS) ;;
Linux) ;;
*) echo "Unknown target"
    exit 1;;
esac


NPROC=9
TOPDIR=$PWD

if [ "$INSIDE_DOCKER" = "" ]; then
    BUILD=${TOPDIR}/Out-${TARGET_OS}/Build

    TARGET=${TOPDIR}/Out-${TARGET_OS}/Target
else
    BUILD=/root/Out-${TARGET_OS}/Build

    TARGET=/root/Out-${TARGET_OS}/Target
fi
export PKG_CONFIG_PATH=${TARGET}/lib/pkgconfig:${TARGET}/lib64/pkgconfig

error() {
    echo "ERROR: $@"
    exit 1
}

create_directory() {
    test -d ${1} || mkdir -p ${1}
}

build_cmake() {
    pushd ${BUILD}
    local SRC=${1}
    shift
    mkdir -p ${SRC}
    cd ${SRC}
    cmake -DCMAKE_INSTALL_PREFIX=${TARGET} ${TOPDIR}/${SRC} $@ || error "Configure component $SRC"
    make -j${NPROC} || error "Build component $SRC"
    make install || error "Install component $SRC"
    popd
}

build_autotools() {
    pushd ${BUILD}
    local SRC=${1}
    shift

#    if [ ! -f ${TOPDIR}/${SRC}/configure ]; then
	pushd ${TOPDIR}/${SRC}
	if [ -f ${TOPDIR}/${SRC}/autogen.sh ]; then
	    ./autogen.sh || error "Autogenerating autotools files"
	else
	    autoreconf -i || error "Reconfiguring autotools files"
	fi

	test -f Makefile && make distclean

#	make distclean
	popd
#    fi

    mkdir -p ${SRC}
    cd ${SRC}
    ${TOPDIR}/${SRC}/configure --prefix=${TARGET} $@ || error "Configure component $SRC"
    make -j${NPROC} || error "Build component $SRC"
    make install || error "Install component $SRC"
    popd
}

build_broken_autotools() {
    pushd ${BUILD}
    local SRC=${1}
    shift

    mkdir -p $(dirname ${SRC})
    cp -R ${TOPDIR}/${SRC} $(dirname ${SRC})
    cd ${SRC}
    if [ ! -f ./configure ]; then
	if [ -f ./autogen.sh ]; then
	    ./autogen.sh || error "Autogenerating autotools files"
	else
	    autoreconf -i || error "Reconfiguring autotools files"
	fi
#	make distclean
#	popd
    fi

    ./configure --prefix=${TARGET} $@ || error "Configure component $SRC"
    make -j${NPROC} || error "Build component $SRC"
    make install || error "Install component $SRC"
    popd
}

build_openssl() {
    local SRC=openssl-1.1.1k
    local C=$1
    shift
    pushd ${BUILD}/thirdparty

    test -f install.openssl && return

    rm -rf $SRC
    cp -R ${TOPDIR}/thirdparty/${SRC} .

    cd $SRC

    ./Configure --cross-compile-prefix=${C} --prefix=${TARGET} no-shared $@ || error "Configure openssl"

#    make CC=${C}gcc AR=${C}ar RANLIB=${C}ranlib RC=${C}windres -j ${NPROC} || error "Build openssl"
    make -j ${NPROC} || error "Build openssl"

    make install || error "Install openssl"

    touch ../install.openssl

    popd
}

make_macos_package()
{
#    local BUILD_DIR="/var/www/deploy/TMP/osx-ia32/latest-git"

    local BUILD_DIR=$1
    local APP_DIR=$2

    local BASE_DIR="${BUILD}/tmp"
    local VERSION="1.0"
    local IDENTIFIER="remote.lilitun.net"
    local APPNAME="LiliTun remote control"
    local BACKGROUND="$3"

    rm -rf "${BASE_DIR}/darwin"
    mkdir -p "${BASE_DIR}/darwin/flat/Resources/en.lproj"
    mkdir -p "${BASE_DIR}/darwin/flat/base.pkg/"
    mkdir -p "${BASE_DIR}/darwin/root/Applications"
    cp -R "${BUILD_DIR}/${APP_DIR}" ${BASE_DIR}/darwin/root/Applications/${APP_DIR}
    cp ${BACKGROUND} ${BASE_DIR}/darwin/flat/Resources/en.lproj/background
    NUM_FILES=$(find ${BASE_DIR}/darwin/root | wc -l)
    INSTALL_KB_SIZE=$(du -k -s ${BASE_DIR}/darwin/root | awk '{print $1}')

cat <<EOF > ${BASE_DIR}/darwin/flat/base.pkg/PackageInfo
<?xml version="1.0" encoding="utf-8" standalone="no"?>
<pkg-info overwrite-permissions="true" relocatable="false" identifier="${IDENTIFIER}" postinstall-action="none" version="${VERSION}" format-version="2" generator-version="InstallCmds-502 (14B25)" auth="root">
 <payload numberOfFiles="${NUM_FILES}" installKBytes="${INSTALL_KB_SIZE}"/>
 <bundle-version>
 <bundle id="${IDENTIFIER}" CFBundleIdentifier="${IDENTIFIER}" path="./Applications/${APPNAME}.app" CFBundleVersion="1.3.0"/>
 </bundle-version>
 <update-bundle/>
 <atomic-update-bundle/>
 <strict-identifier/>
 <relocate/>
 <scripts/>
</pkg-info>
EOF

cat <<EOF > ${BASE_DIR}/darwin/flat/Distribution
<?xml version="1.0" encoding="utf-8"?>
<installer-script minSpecVersion="1.000000" authoringTool="com.apple.PackageMaker" authoringToolVersion="3.0.3" authoringToolBuild="174">
 <title>${APPNAME} ${VERSION}</title>
 <options customize="never" allow-external-scripts="no"/>
 <background file="background" alignment="bottomleft" scaling="none"/>
 <domains enable_anywhere="true"/>
 <installation-check script="pm_install_check();"/>
 <script>function pm_install_check() {
 if(!(system.compareVersions(system.version.ProductVersion,'10.9') >= 0)) {
 my.result.title = 'Failure';
 my.result.message = 'You need at least Mac OS X 10.9 to install ${APPNAME}.';
 my.result.type = 'Fatal';
 return false;
 }
 return true;
 }
 </script>
 <choices-outline>
 <line choice="choice1"/>
 </choices-outline>
 <choice id="choice1" title="base">
 <pkg-ref id="${IDENTIFIER}.base.pkg"/>
 </choice>
 <pkg-ref id="${IDENTIFIER}.base.pkg" installKBytes="${INSTALL_KB_SIZE}" version="${VERSION}" auth="Root">#base.pkg</pkg-ref>
</installer-script>
EOF

    PKG_LOCATION="${BASE_DIR}/${APPNAME}-${VERSION}.pkg"

    ( cd ${BASE_DIR}/darwin/root && find . | cpio -o --format odc --owner 0:80 | gzip -c ) > ${BASE_DIR}/darwin/flat/base.pkg/Payload

    mkbom -u 0 -g 80 ${BASE_DIR}/darwin/root ${BASE_DIR}/darwin/flat/base.pkg/Bom

    ( cd ${BASE_DIR}/darwin/flat/ && xar --compression none -cf "${PKG_LOCATION}" * )

    echo "osx package has been built: ${PKG_LOCATION}"

#    cp -f "${PKG_LOCATION}" $4
    cp -f "${PKG_LOCATION}" ${BUILD_DIR}
}

create_directory ${BUILD}
create_directory ${TARGET}
create_directory ${TARGET}/lib

ln -sf lib ${TARGET}/lib64

CMAKE_OPTS=""
AUTOCONF_OPTS=""

if [ "$TARGET_OS" = "Windows" ]; then
    AUTOCONF_OPTS="--host=i686-w64-mingw32 --disable-shared --enable-static"

    CMAKE_OPTS="-DCMAKE_TOOLCHAIN_FILE=${TOPDIR}/cmake/mingw32.cmake -DFLUID_PATH=${TOPDIR}/Out-Linux/Target/bin/fluid -DCMAKE_SYSTEM_PROCESSOR=x86 -DBUILD_SHARED_LIBS=OFF"
elif [ "$TARGET_OS" = "MacOS" ]; then
    AUTOCONF_OPTS="--host=x86_64-apple-darwin13 --disable-shared --enable-static CC=x86_64-apple-darwin13-clang CXX=x86_64-apple-darwin13-clang++"

    if [ "$INSIDE_DOCKER" = "" ]; then
	export PATH="${HOME}/bin/osxcross/bin:$PATH"

	export SDK_PATH=${HOME}/bin/osxcross/SDK/MacOSX10.9.sdk
    else
	export PATH="/root/osxcross/target/bin:$PATH"

	export SDK_PATH=/root/osxcross/target/SDK/MacOSX10.9.sdk
    fi


    cp -f ${TOPDIR}/cmake/darwin.cmake.in ${TARGET}/darwin.cmake

    sed -i -e "s|@SDK_PATH@|${SDK_PATH}|g" ${TARGET}/darwin.cmake

    CMAKE_OPTS="-DCMAKE_TOOLCHAIN_FILE=${TARGET}/darwin.cmake -DFLUID_PATH=${TOPDIR}/Out-Linux/Target/bin/fluid -DCMAKE_SYSTEM_PROCESSOR=x86_64 -DBUILD_SHARED_LIBS=OFF"
fi

if [ "$TARGET_OS" = "Windows" ]; then
    build_cmake thirdparty/zlib-1.2.11.dfsg $CMAKE_OPTS

    CMAKE_OPTS="$CMAKE_OPTS -DZLIB_INCLUDE_DIR=${TARGET}/include -DZLIB_LIBRARY=${TARGET}/lib/libzlibstatic.a"

    build_cmake thirdparty/libjpeg-turbo-2.0.3 $CMAKE_OPTS -DENABLE_SHARED=FALSE -DENABLE_STATIC=TRUE

    CMAKE_OPTS="$CMAKE_OPTS -DJPEG_INCLUDE_DIR=${TARGET}/include -DJPEG_LIBRARY=${TARGET}/lib/libturbojpeg.a"

    build_cmake thirdparty/libpng1.6-1.6.37 $CMAKE_OPTS

    ln -sf libpng16 ${TARGET}/include/libpng

    CMAKE_OPTS="$CMAKE_OPTS -DPNG_PNG_INCLUDE_DIR=${TARGET}/include -DPNG_LIBRARY=${TARGET}/lib/libpng16.a"

    build_openssl i686-w64-mingw32- mingw

    CMAKE_OPTS="$CMAKE_OPTS -DOPENSSL_ROOT_DIR=${TARGET} -DOPENSSL_INCLUDE_DIR=${TARGET}/include -DOPENSSL_LIBRARIES=${TARGET}/lib"
elif [ "$TARGET_OS" = "MacOS" ]; then
#    build_cmake thirdparty/zlib-1.2.11.dfsg $CMAKE_OPTS

    CMAKE_OPTS="$CMAKE_OPTS -DZLIB_INCLUDE_DIR=${SDK_PATH}/usr/include -DZLIB_LIBRARY=${SDK_PATH}/usr/lib/libz.dylib"

    build_cmake thirdparty/libjpeg-turbo-2.0.3 $CMAKE_OPTS

    CMAKE_OPTS="$CMAKE_OPTS -DJPEG_INCLUDE_DIR=${TARGET}/include -DJPEG_LIBRARY=${TARGET}/lib/libturbojpeg.a"

    build_cmake thirdparty/libpng1.6-1.6.37 $CMAKE_OPTS

    ln -sf libpng16 ${TARGET}/include/libpng

    CMAKE_OPTS="$CMAKE_OPTS -DPNG_PNG_INCLUDE_DIR=${TARGET}/include -DPNG_LIBRARY=${TARGET}/lib/libpng16.a"

    build_openssl x86_64-apple-darwin13- darwin64-x86_64-cc

#    CMAKE_OPTS="$CMAKE_OPTS -DOPENSSL_ROOT_DIR=${TARGET} -DOPENSSL_INCLUDE_DIR=${TARGET}/include -DOPENSSL_LIBRARIES=${TARGET}/lib"
fi

build_autotools thirdparty/simple-connection-lib $AUTOCONF_OPTS --disable-shared --enable-static

if [ "$TARGET_OS" != "Windows" -a "$TARGET_OS" != "MacOS" ]; then
    build_autotools ControlServer ${AUTOCONF_OPTS}
fi

if [ "$TARGET_OS" = "MacOS" ]; then
    build_broken_autotools thirdparty/fltk ${AUTOCONF_OPTS} CPPFLAGS="-I${TARGET}/include" LDFLAGS="-L${TARGET}/lib"
else
    build_cmake thirdparty/fltk $CMAKE_OPTS -DOPTION_BUILD_EXAMPLES=OFF
fi

if [ "$TARGET_OS" = "Windows" ]; then
    if [ -d "${TARGET}/CMake" ]; then
	rm -rf ${TARGET}/share/fltk

	cp -R ${TARGET}/CMake ${TARGET}/share/fltk
    fi
fi

if [ "$INSIDE_DOCKER" = "" -o "$TARGET_OS" != "Linux" ]; then
    build_autotools RemoteDesktop ${AUTOCONF_OPTS} FLTK_DIR=${TARGET}/bin/
fi

if [ "$TARGET_OS" = "Linux" ]; then
    if which dpkg-buildpackage &>/dev/null; then
	pushd ControlServer

	dpkg-buildpackage -rfakeroot -b -tc -uc || error "Build server package"

	popd
    fi
fi

RELEASE=${TOPDIR}/release

case "$TARGET_OS" in
Linux)
	if [ -e ${TARGET}/bin/lilitun-remote-control-1.0-x86_64.AppImage ]; then
	    cp -f ${TARGET}/bin/lilitun-remote-control-1.0-x86_64.AppImage ${RELEASE}/linux
	fi
	cp -f ${TOPDIR}/controlserver_1.0_amd64.deb ${RELEASE}/linux
	rm -f controlserver_1.0_*.*
	rm -f controlserver-dbgsym_1.0_*.*
	;;
Windows)
	cp -f ${TARGET}/bin/lilitun-remote-control.exe ${RELEASE}/windows
	i686-w64-mingw32-strip ${RELEASE}/windows/lilitun-remote-control.exe
	;;
MacOS)
	make_macos_package ${TARGET} LiliTun-remote-control.app ${TOPDIR}/RemoteDesktop/lilitun-remote-desktop-color.png
	#${RELEASE}/macos
	pushd ${TARGET}
	zip -r9 ${RELEASE}/macos/lilitun-remote-control.zip LiliTun-remote-control.app
	cp -f *.pkg ${RELEASE}/macos/
	popd
	;;
esac

if [ "$INSIDE_DOCKER" != "" ]; then
    git clean -f -d
    pushd thirdparty/simple-connection-lib
    git clean -f -d
    popd
fi
