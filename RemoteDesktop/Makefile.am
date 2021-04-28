AM_CFLAGS = $(PROJECTOR_CFLAGS) $(GRABBER_INCLUDES)
AM_CXXFLAGS = $(PROJECTOR_CFLAGS) $(GRABBER_INCLUDES)
LIBS = $(PROJECTOR_LIBS) $(GRABBER_LIBS)

bin_PROGRAMS = lilitun-remote-control

if TARGET_LINUX
lilitun_remote_control_SOURCES = app.cxx jpegenc.c main.cxx projector.c list.c xcbgrabber.c base64.c getrandom.c ../thirdparty/lz4/lz4.c pseudofs.c

LINUXDEPLOY = linuxdeploy-devbuild-$(SYSTEM_ARCH).AppImage

$(HOME)/bin/$(LINUXDEPLOY):
	mkdir -p $(HOME)/bin
	wget https://www.lilitun.net/downloads/tools/$(LINUXDEPLOY) -O $(HOME)/bin/$(LINUXDEPLOY)
	chmod 755 $@

lilitun-remote-control-$(VERSION)-$(SYSTEM_ARCH).AppImage: $(HOME)/bin/$(LINUXDEPLOY)
	-chrpath -c lilitun-remote-control
	convert $(top_srcdir)/lilitun-remote-desktop-color.png -resize 96x96\> lilitun-remote-desktop-color.png
	VERSION=$(VERSION) $(HOME)/bin/$(LINUXDEPLOY)	\
	    --appdir AppDir				\
	    -e lilitun-remote-control			\
	    --custom-apprun=$(top_srcdir)/linux/AppRun	\
	    -d linux/lilitun-remote-control.desktop	\
	    -i lilitun-remote-desktop-color.png
	VERSION=$(VERSION) $(HOME)/bin/$(LINUXDEPLOY)	\
	    --appdir AppDir				\
	    -o appimage

install-exec-hook: lilitun-remote-control-$(VERSION)-$(SYSTEM_ARCH).AppImage
	$(INSTALL) -D -m 755 $^ $(DESTDIR)$(bindir)/$^
endif

if TARGET_WINDOWS
lilitun_remote_control_SOURCES = app.cxx jpegenc.c main.cxx projector.c wingrabber.cxx base64.c getrandom.c ../thirdparty/lz4/lz4.c pseudofs.c resources.rc
endif

if TARGET_MACOS
resourcesdir = $(prefix)/LiliTun-remote-control.app/Contents/Resources
resources_DATA = osx/Projector.icns

contentsdir = $(prefix)/LiliTun-remote-control.app/Contents
contents_DATA = osx/PkgInfo osx/Info.plist

lilitun_remote_control_SOURCES = app.cxx jpegenc.c main.cxx projector.c macgrabber.c base64.c getrandom.c ../thirdparty/lz4/lz4.c pseudofs.c
endif

.rc.o:
	$(RC) $^ -o $@
%.o : %.rc
	$(RC) $^ -o $@

pseudofs.c:
	$(top_srcdir)/WebClient/make_fs.sh $(top_srcdir)/WebClient index.html lilitun-remote-client.js lz4dec.js local.js style.css lilitun-remote-desktop-color.png > $@

clean-local:
	rm -f pseudofs.c
