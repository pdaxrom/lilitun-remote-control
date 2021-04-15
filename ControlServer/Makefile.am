AM_CFLAGS = $(CONFERENCEROOM_CFLAGS)
AM_CXXFLAGS = $(CONFERENCEROOM_CFLAGS)
LIBS = $(CONFERENCEROOM_LIBS)

bin_PROGRAMS = controlserver

controlserver_SOURCES = base64.c fbops.c getrandom.c jpegdec.c list.c main.c portctrl.c translator.c messaging.c utils.c ../thirdparty/lz4/lz4.c config.c

#contentsdir = $(prefix)/Projector.app/Contents
#contents_DATA = osx/PkgInfo osx/Info.plist

pemdir = $(sysconfdir)/controlserver
pem_DATA = cert/README

install-exec-hook:
	if test ! -e $(DESTDIR)$(libdir)/libvncserver*.so; then \
	    $(INSTALL) -D -m 755 $(LIBVNCSERVER_LIBDIR)/libvncserver-openssl.so.$(LIBVNCSERVER_VERSION) $(DESTDIR)$(libdir)/libvncserver-openssl.so.$(LIBVNCSERVER_VERSION) ; \
	    ln -sf libvncserver-openssl.so.$(LIBVNCSERVER_VERSION) $(DESTDIR)$(libdir)/libvncserver-openssl.so.1 ; \
	    ln -sf libvncserver-openssl.so.1 $(DESTDIR)$(libdir)/libvncserver-openssl.so ; \
	fi
	if test -e $(top_srcdir)/../thirdparty/websockify ; then \
	    mkdir -p $(DESTDIR)$(datadir) ; \
	    cp -R $(top_srcdir)/../thirdparty/websockify $(DESTDIR)$(datadir) ; \
	    cd $(DESTDIR)$(datadir)/websockify && make ; \
	fi

.rc.o:
	$(RC) $^ -o $@
%.o : %.rc
	$(RC) $^ -o $@
