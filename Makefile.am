ACLOCAL_AMFLAGS = -I m4

# This minimal Makefile.am is created primarily to satisfy glibtoolize requirements
# The actual build process still uses the custom Makefile.in

SUBDIRS = 
EXTRA_DIST = Makefile.in bootstrap configure.ac

# Dummy target to prevent automake from interfering with the custom build system
all-local:
	@echo "Using custom Makefile.in build system"
	$(MAKE) -f Makefile

install-exec-local:
	$(MAKE) -f Makefile install

clean-local:
	$(MAKE) -f Makefile clean

distclean-local:
	$(MAKE) -f Makefile distclean