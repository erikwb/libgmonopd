## source directory

CPPFLAGS= -DGMONOPD_DATA=\"$(pkgdatadir)\"

h_sources = const.h network.h player.h input.h event.h game.h trade.h estate.h card.h auction.h debt.h libgmonopd.h
c_sources = network.c player.c input.c event.c game.c trade.c estate.c card.c auction.c debt.c libgmonopd.c

library_includedir=$(includedir)/libgmonopd
library_include_HEADERS = libgmonopd.h

INCLUDES = -I$(top_srcdir)

lib_LTLIBRARIES = libgmonopd.la
libgmonopd_la_SOURCES = $(h_sources) $(c_sources)
libgmonopd_la_LDFLAGS = -version-info $(LIBGMONOPD_LIBRARY_VERSION) -release $(LIBGMONOPD_RELEASE)

