if HAVE_DOXYGEN
doxyfile.stamp: Doxyfile
	$(DOXYGEN) Doxyfile
	touch doxyfile.stamp

CLEANFILES = doxyfile.stamp

all-local: doxyfile.stamp
clean-local:
	rm -rf $(top_srcdir)/docs/html

endif