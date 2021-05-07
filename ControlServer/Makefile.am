AM_CFLAGS = $(CONFERENCEROOM_CFLAGS)
AM_CXXFLAGS = $(CONFERENCEROOM_CFLAGS)
LIBS = $(CONFERENCEROOM_LIBS)

bin_PROGRAMS = controlserver

controlserver_SOURCES = base64.c getrandom.c list.c main.c messaging.c utils.c config.c

#contentsdir = $(prefix)/Projector.app/Contents
#contents_DATA = osx/PkgInfo osx/Info.plist

pemdir = $(sysconfdir)/controlserver
pem_DATA = cert/README

.rc.o:
	$(RC) $^ -o $@
%.o : %.rc
	$(RC) $^ -o $@
