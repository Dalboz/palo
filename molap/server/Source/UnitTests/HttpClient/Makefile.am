AM_CPPFLAGS =

noinst_PROGRAMS = \
	HttpClient

HttpClient_DEPENDENCIES =
HttpClient_LDADD        =
HttpClient_SOURCES      = TestHttpClient.cpp HttpClient.cpp

.PHONY: dist_src

dist_src:
	make clean
	tar cvzf dist_src-`date +%Y-%m-%d`.tar.gz \
		MS.NET-2003/HttpClient.{sln,vcproj} \
		*.{h,cpp}
