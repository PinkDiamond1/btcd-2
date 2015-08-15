# This file is part of MXE.
# See index.html for further information.

PKG             := bdb48
$(PKG)_IGNORE   :=
$(PKG)_VERSION  := 4.8.30.NC
$(PKG)_CHECKSUM := ff754f92ccfae2f9d3527d6d6ea43add5b852456
$(PKG)_SUBDIR   := db-$($(PKG)_VERSION)
$(PKG)_FILE     := db-$($(PKG)_VERSION).tar.gz
$(PKG)_URL      := http://download.oracle.com/berkeley-db/$($(PKG)_FILE)
$(PKG)_DEPS     :=

#define $(PKG)_UPDATE
#    $(WGET) -q -O- 'http://curl.haxx.se/download/?C=M;O=D' | \
#    $(SED) -n 's,.*curl-\([0-9][^"]*\)\.tar.*,\1,p' | \
#    head -1
#endef

define $(PKG)_BUILD
    cd '$(1)'/build_unix && ../dist/configure \
        $(MXE_CONFIGURE_OPTS) \
	--enable-mingw \
	--enable-cxx \
	--disable-shared \
	--disable-replication
	
	make	
#    $(MAKE) -C '$(1)' -j '$(JOBS)' install $(MXE_DISABLE_DOCS)
#    ln -sf '$(PREFIX)/$(TARGET)/bin/curl-config' '$(PREFIX)/bin/$(TARGET)-curl-config'
#
#    '$(TARGET)-gcc' \
#        -W -Wall -Werror -ansi -pedantic \
#        '$(2).c' -o '$(PREFIX)/$(TARGET)/bin/test-curl.exe' \
#        `'$(TARGET)-pkg-config' libcurl --cflags --libs`
endef
