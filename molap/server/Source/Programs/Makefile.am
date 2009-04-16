AM_CPPFLAGS = -I${top_srcdir}

noinst_PROGRAMS = \
	pdc \
	palo



palo_DEPENDENCIES =
palo_LDADD =

if ENABLE_HTTP
if ENABLE_HTTP_MODULE
else
  palo_DEPENDENCIES += ../libhttp.la
  palo_LDADD        += ../libhttp.la
endif
endif

if ENABLE_HTTPS
if ENABLE_HTTPS_MODULE
else
  palo_DEPENDENCIES += ../libhttps.la
  palo_LDADD        += ../libhttps.la
endif
endif

if ENABLE_LEGACY
if ENABLE_LEGACY_MODULE
else
  palo_DEPENDENCIES += ../liblegacy.la
  palo_LDADD        += ../liblegacy.la
endif
endif

palo_DEPENDENCIES += ../libpalo.la
palo_LDADD        += ../libpalo.la
palo_SOURCES       = palo.cpp extension.cpp Palo1Converter.cpp

pdc_DEPENDENCIES = ../libpalo.la
pdc_LDADD        = ../libpalo.la
pdc_SOURCES      = pdc.cpp Palo1Converter.cpp
