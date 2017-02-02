## top directory

#build in these directories:
SUBDIRS = libgmonopd

EXTRA_DIST = libgmonopd.m4

# install the m4 macros to be used by autoconf
m4datadir = $(datadir)/aclocal
m4data_DATA = libgmonopd.m4

DIST_SUBDIRS = libgmonopd
bin_SCRIPTS = libgmonopd-config

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/conf; then \
	    $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/conf; \
	    for confdir in $(srcdir)/conf/*; do \
		if test -d $$confdir; then \
		    $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/$$confdir; \
		    for conffile in $(srcdir)/$$confdir/*; do \
			if test -f $$conffile; then \
			    $(INSTALL_DATA) $$conffile $(DESTDIR)$(pkgdatadir)/$$confdir; \
			fi \
		    done \
		fi \
	    done \
	fi


dist-hook:
	if test -d conf; then \
	  mkdir $(distdir)/conf; \
	  for directory in conf/*; do \
	    if test -d $$directory; then \
	      cp -a $$directory $(distdir)/conf; \
	    fi \
	  done \
	fi
