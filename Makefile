DESTDIR:=

SERVER_DEPS:= uppsrc/Core uppsrc/Skylark uppsrc/MySql uppsrc/Sql uppsrc/plugin/z uppsrc/uppconfig.h
CLIENT_DEPS:= uppsrc/Core uppsrc/plugin/z uppsrc/uppconfig.h
UPPSVN=http://upp-mirror.googlecode.com/svn/trunk

all: bin/wds bin/wdc

bin/wds: $(SERVER_DEPS) FORCE
	$(MAKE) -f src/mkfile PKG=Watchdog/Server NESTS="src uppsrc" OUT=obj BIN=bin COLOR=0 SHELL=bash FLAGS="GCC SSE2 MT" TARGET=$@

bin/wdc: $(CLIENT_DEPS) FORCE
	$(MAKE) -f src/mkfile PKG=Watchdog/Client NESTS="src uppsrc" OUT=obj BIN=bin COLOR=0 SHELL=bash FLAGS="GCC SSE2 MT" TARGET=$@

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
	install -d $(DESTDIR)/etc/watchdog \
	           $(DESTDIR)/usr/bin \
	           $(DESTDIR)/usr/share/watchdog/output \
	           $(DESTDIR)/usr/share/watchdog/css \
	           $(DESTDIR)/usr/share/watchdog/img \
	           $(DESTDIR)/usr/share/watchdog/js \
	           $(DESTDIR)/usr/share/watchdog/templates \
	           $(DESTDIR)/usr/share/watchdog/usermods/css \
	           $(DESTDIR)/usr/share/watchdog/usermods/img \
	           $(DESTDIR)/usr/share/watchdog/usermods/js \
	           $(DESTDIR)/usr/share/watchdog/usermods/templates \
	           $(DESTDIR)/var/log/watchdog
	install bin/* $(DESTDIR)/usr/bin/
	install src/Watchdog/Server/Server.ini $(DESTDIR)/etc/watchdog/wds.ini
	install src/Watchdog/Client/Client.ini $(DESTDIR)/etc/watchdog/wdc.ini
	install src/Watchdog/Server/css/* $(DESTDIR)/usr/share/watchdog/css
	install src/Watchdog/Server/img/* $(DESTDIR)/usr/share/watchdog/img
	install src/Watchdog/Server/js/* $(DESTDIR)/usr/share/watchdog/js
	install src/Watchdog/Server/misc/* $(DESTDIR)/usr/share/watchdog/misc
	install src/Watchdog/Server/templates/* $(DESTDIR)/usr/share/watchdog/templates

uninstall:
	rm $(DESTDIR)/usr/bin/wdc
	rm $(DESTDIR)/usr/bin/wds
	rm -r $(DESTDIR)/etc/watchdog
	rm -r $(DESTDIR)/usr/share/watchdog
	rm -r $(DESTDIR)/var/log/watchdog

FORCE::

.PHONY: all clean dist-clean update-uppsrc install uninstall
