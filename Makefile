# configuration
DESTDIR:=
SHELL:=bash
OBJ:=obj
BIN:=bin
LIB:=lib
TMP:=.
DSQL:=mysql sqlite
UPPVER:=6204
UPPFILE:=upp-x11-src-$(UPPVER)
UPPTAR:=$(TMP)/$(UPPFILE).tar.gz
UPPSRC:=http://ultimatepp.org/downloads/$(UPPTAR)
UPPSVN:=http://upp-mirror.googlecode.com/svn/trunk
USESVN:=$(shell which svn &> /dev/null && echo "true" || echo "false")

# dependencies
COMMON_DEPS:=uppsrc/Core uppsrc/plugin/z uppsrc/uppconfig.h
SERVER_DEPS:=$(COMMON_DEPS) uppsrc/Sql uppsrc/Skylark uppsrc/MySql uppsrc/plugin/sqlite3 uppsrc/plugin/zip uppsrc/plugin/pcre
CLIENT_DEPS:=$(COMMON_DEPS)

#internal variables
DSQL_LIBS:=$(foreach d,$(DSQL), $(LIB)/$d.so)
DSQL_MYSQL_DEPS:=$(COMMON_DEPS) uppsrc/Sql uppsrc/MySql
DSQL_SQLITE_DEPS:=$(COMMON_DEPS) uppsrc/Sql uppsrc/plugin/sqlite3

MAKE_WD=$(MAKE) -f src/mkfile NESTS="src uppsrc" OUT=$(OBJ) BIN=$(BIN) COLOR=0 SHELL=bash FLAGS="GCC SSE2 MT" $(JOBS) TARGET=$@
MAKE_DSQL=$(MAKE_WD) LDFLAGS="-shared -Wl,-O,2 -Wl,--gc-sections -u GetSession" CXX="$(CXX) -fPIC --std=c++11" CC="$(CC) -fPIC"


all: $(BIN)/wds $(BIN)/wdc $(BIN)/wda $(DSQL_LIBS)

ifeq ($(USESVN),true)
  UPPTAR:=
  GETUPPDEP=mkdir -p $@; svn co '$(UPPSVN)/$@' $@;
  GETUPPDEPFILE=mkdir -p uppsrc; svn export --force '$(UPPSVN)/$@' $@
else
  GETUPPDEP=tar -xzmf $(UPPTAR) --mtime=$$(stat -c@%Y $(UPPTAR)) --strip 1 $(UPPFILE)/$@;
  GETUPPDEPFILE=$(GETUPPDEP)
  $(UPPTAR):
	wget -nv -O $@ '$(UPPSRC)'
endif

# enable control of paralel building from dpkg-buildpackage
DEB_BUILD_OPTIONS?=
ifneq (,$(filter parallel=%,$(DEB_BUILD_OPTIONS)))
  JNUM:=$(patsubst parallel=%,%,$(filter parallel=%,$(DEB_BUILD_OPTIONS)))
  JOBS:=JOBS=$(JNUM)
else
  JNUM:=1
  JOBS:=
endif

deb:
	dpkg-buildpackage -j$(JNUM)

$(BIN)/wds: $(SERVER_DEPS) FORCE
	$(MAKE_WD) PKG=Watchdog/Server

$(BIN)/wdc: $(CLIENT_DEPS) FORCE
	$(MAKE_WD) PKG=Watchdog/Client

$(BIN)/wda: $(CLIENT_DEPS) FORCE
	$(MAKE_WD) PKG=Watchdog/Admin

$(LIB)/mysql.so: $(DSQL_MYSQL_DEPS) FORCE
	$(MAKE_DSQL) PKG=DynamicSql/mysql

$(LIB)/sqlite.so: $(DSQL_SQLITE_DEPS) FORCE
	$(MAKE_DSQL) PKG=DynamicSql/sqlite

uppsrc/%: $(UPPTAR)
	$(GETUPPDEP)
	[ -e patch/$*.patch ] && patch --binary -t -l -p0 -duppsrc -i ../patch/$*.patch || true

uppsrc/uppconfig.h: $(UPPTAR)
	$(GETUPPDEPFILE)

update-uppsrc: $(CLIENT_DEPS) $(SERVER_DEPS) $(DSQL_SQLITE_DEPS) $(DSQL_MYSQL_DEPS)
	@$(USESVN) || (echo "ERROR: 'update-uppsrc' goal should be only used when USESVN=true" && exit 1)
	for d in $^; do \
	  [ -d "$$d" ] && svn up "$$d" || svn export --force "$(UPPSVN)/$$d" "$$d"; \
	done;

clean:
	rm -rf $(OBJ) $(BIN) $(LIB)

distclean: clean
	rm -rf uppsrc $(TMP)/upp-x11-src-*.tar.gz

install: all
	install -d $(DESTDIR)/etc/thewatchdog \
	           $(DESTDIR)/usr/bin \
	           $(DESTDIR)/usr/share/thewatchdog/lib \
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
	install $(BIN)/* $(DESTDIR)/usr/bin/
	install $(LIB)/* $(DESTDIR)/usr/share/thewatchdog/lib/
	install src/Watchdog/Server/Server.ini $(DESTDIR)/etc/thewatchdog/wds.ini
	install src/Watchdog/Client/Client.ini $(DESTDIR)/etc/thewatchdog/wdc.ini
	install src/Watchdog/Admin/Admin.ini $(DESTDIR)/etc/thewatchdog/wda.ini
	install src/Watchdog/Server/static/css/* $(DESTDIR)/usr/share/thewatchdog/css
	install src/Watchdog/Server/static/img/* $(DESTDIR)/usr/share/thewatchdog/img
	install src/Watchdog/Server/static/js/* $(DESTDIR)/usr/share/thewatchdog/js
	install src/Watchdog/Server/static/misc/* $(DESTDIR)/usr/share/thewatchdog/misc
	install src/Watchdog/Server/templates/* $(DESTDIR)/usr/share/thewatchdog/templates
	install LICENSE $(DESTDIR)/usr/share/thewatchdog/copyright
	find $(DESTDIR) -type d -exec chmod 755 {} \; -o -type f -exec chmod 644 {} \;

uninstall:
	rm $(DESTDIR)/usr/bin/wds
	rm $(DESTDIR)/usr/bin/wdc
	rm $(DESTDIR)/usr/bin/wda
	rm -r $(DESTDIR)/etc/watchdog
	rm -r $(DESTDIR)/usr/share/thewatchdog
	rm -r $(DESTDIR)/var/log/thewatchdog

FORCE::

.PHONY: all clean dist-clean update-uppsrc install uninstall
