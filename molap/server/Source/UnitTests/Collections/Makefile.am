AM_CPPFLAGS = -I${top_srcdir}

noinst_PROGRAMS = \
	TestAssociativeArray TestMD5



TestAssociativeArray_DEPENDENCIES = ../../libpalo.la
TestAssociativeArray_LDADD        = ../../libpalo.la
TestAssociativeArray_SOURCES      = TestAssociativeArray.cpp

TestMD5_DEPENDENCIES = ../../libpalo.la
TestMD5_LDADD        = ../../libpalo.la -lm
TestMD5_SOURCES      = md5main.c
