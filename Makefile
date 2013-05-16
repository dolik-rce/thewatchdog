# configuration
DESTDIR:=

# dependencies
SERVER_DEPS:= uppsrc/Core uppsrc/Skylark uppsrc/MySql uppsrc/Sql uppsrc/plugin/z uppsrc/uppconfig.h
CLIENT_DEPS:= uppsrc/Core uppsrc/plugin/z uppsrc/uppconfig.h
UPPSVN=http://upp-mirror.googlecode.com/svn/trunk

# enable control of paralel building from dpkg-buildpackage
DEB_BUILD_OPTIONS?=
ifneq (,$(filter parallel=%,$(DEB_BUILD_OPTIONS)))
    JOBS:=JOBS=$(patsubst parallel=%,%,$(filter parallel=%,$(DEB_BUILD_OPTIONS)))
else
    JOBS:=
endif

all: bin/wds bin/wdc

deb:
	dpkg-buildpackage -j25

bin/wds: $(SERVER_DEPS) FORCE
	$(MAKE) -f src/mkfile PKG=Watchdog/Server NESTS="src uppsrc" OUT=obj BIN=bin COLOR=0 SHELL=bash FLAGS="GCC SSE2 MT" $(JOBS) TARGET=$@

bin/wdc: $(CLIENT_DEPS) FORCE
	$(MAKE) -f src/mkfile PKG=Watchdog/Client NESTS="src uppsrc" OUT=obj BIN=bin COLOR=0 SHELL=bash FLAGS="GCC SSE2 MT" $(JOBS) TARGET=$@

uppsrc/%:
	mkdir -p $@
	svn co '$(UPPSVN)/$@' $@

uppsrc/uppconfig.h:
	mkdir -p uppsrc
	svn export '$(UPPSVN)/$@' uppsrc/uppconfig.h

update-uppsrc: $(CLIENT_DEPS) $(SERVER_DEPS)
	for d in $$(find uppsrc/ -exec test -d {}/.svn \; -print -prune); do svn up $$d; done;
	svn export --force '$(UPPSVN)/uppsrc/uppconfig.h' uppsrc/uppconfig.h

clean:
	rm -rf obj bin

dist-clean: clean
	rm -rf uppsrc

install: bin/wdc bin/wds
	install -d $(DESTDIR)/etc/thewatchdog \
	           $(DESTDIR)/usr/bin \
	           $(DESTDIR)/usr/share/thewatchdog/output \
	           $(DESTDIR)/usr/share/thewatchdog/css \
	           $(DESTDIR)/usr/share/thewatchdog/img \
	           $(DESTDIR)/usr/share/thewatchdog/js \
	           $(DESTDIR)/usr/share/thewatchdog/misc \
	           $(DESTDIR)/usr/share/thewatchdog/templates \
	           $(DESTDIR)/usr/share/thewatchdog/usermods/css \
	           $(DESTDIR)/usr/share/thewatchdog/usermods/img \
	           $(DESTDIR)/usr/share/thewatchdog/usermods/js \
	           $(DESTDIR)/usr/share/thewatchdog/usermods/misc \
	           $(DESTDIR)/usr/share/thewatchdog/usermods/templates \
	           $(DESTDIR)/var/log/thewatchdog
	install bin/* $(DESTDIR)/usr/bin/
	install src/Watchdog/Server/Server.ini $(DESTDIR)/etc/thewatchdog/wds.ini
	install src/Watchdog/Client/Client.ini $(DESTDIR)/etc/thewatchdog/wdc.ini
	install src/Watchdog/Server/css/* $(DESTDIR)/usr/share/thewatchdog/css
	install src/Watchdog/Server/img/* $(DESTDIR)/usr/share/thewatchdog/img
	install src/Watchdog/Server/js/* $(DESTDIR)/usr/share/thewatchdog/js
	install src/Watchdog/Server/misc/* $(DESTDIR)/usr/share/thewatchdog/misc
	install src/Watchdog/Server/templates/* $(DESTDIR)/usr/share/thewatchdog/templates
	install LICENSE $(DESTDIR)/usr/share/thewatchdog/copyright
	find $(DESTDIR) -type d -exec chmod 755 {} \; -o -type f -exec chmod 644 {} \;

uninstall:
	rm $(DESTDIR)/usr/bin/wdc
	rm $(DESTDIR)/usr/bin/wds
	rm -r $(DESTDIR)/etc/watchdog
	rm -r $(DESTDIR)/usr/share/thewatchdog
	rm -r $(DESTDIR)/var/log/thewatchdog

FORCE::

.PHONY: all clean dist-clean update-uppsrc install uninstall
