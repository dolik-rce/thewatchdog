DESTDIR:=

SERVER_DEPS:= uppsrc/Core uppsrc/Skylark uppsrc/MySql uppsrc/Sql uppsrc/plugin/z uppsrc/uppconfig.h
CLIENT_DEPS:= uppsrc/Core uppsrc/plugin/z uppsrc/uppconfig.h
UPPSVN=http://upp-mirror.googlecode.com/svn/trunk

all: bin/wds bin/wdc

bin/wds: $(SERVER_DEPS)
	$(MAKE) -f src/mkfile PKG=Watchdog/Server NESTS="src uppsrc" OUT=obj BIN=bin COLOR=0 SHELL=bash FLAGS="GCC SSE2 MT" TARGET=$@

bin/wdc: $(CLIENT_DEPS)
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
	install -d $(DESTDIR)/usr/bin
	install bin/* $(DESTDIR)/usr/bin/
	install -D src/Watchdog/Server/Server.ini $(DESTDIR)/etc/watchdog/wds.ini
	install -D src/Watchdog/Client/Client.ini $(DESTDIR)/etc/watchdog/wdc.ini
	install -d $(DESTDIR)/usr/share/watchdog/Watchdog/Server
	install src/Watchdog/Server/static/* $(DESTDIR)/usr/share/watchdog/
	install src/Watchdog/Server/*.witz $(DESTDIR)/usr/share/watchdog/Watchdog/Server/
	install -d $(DESTDIR)/var/log/watchdog

uninstall:
	rm $(DESTDIR)/usr/bin/wdc
	rm $(DESTDIR)/usr/bin/wds
	rm -r $(DESTDIR)/etc/watchdog
	rm -r $(DESTDIR)/usr/share/watchdog
	rm -r $(DESTDIR)/var/log/watchdog

.PHONY: all clean dist-clean update-uppsrc install uninstall
