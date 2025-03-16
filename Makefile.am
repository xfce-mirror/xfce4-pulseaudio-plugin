ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS =								\
	icons								\
	panel-plugin 							\
	po

EXTRA_DIST = 								\
	meson.build							\
	meson_options.txt						\
	po/meson.build							\
	xfce-revision.h.in						\
	$(NULL)

distclean-local:
	rm -rf *.cache *~

distuninstallcheck_listfiles = 						\
	find . -type f -print | grep -v ./share/icons/hicolor/icon-theme.cache

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
