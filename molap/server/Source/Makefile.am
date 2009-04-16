all-recursive: 

SUBDIRS = . Programs UnitTests

lib_LTLIBRARIES = libpalo.la
pkglib_LTLIBRARIES = 

libpalo_la_LDFLAGS = -static
libpalo_la_SOURCES = \
	Collections/StringUtils.cpp \
	Collections/md5.c \
	InputOutput/Condition.cpp \
	InputOutput/Documentation.cpp \
	InputOutput/FileDocumentation.cpp \
	InputOutput/FileReader.cpp \
	InputOutput/FileWriter.cpp \
	InputOutput/HtmlFormatter.cpp \
	InputOutput/HttpFileRequestHandler.cpp \
	InputOutput/HttpRequest.cpp \
	InputOutput/HttpRequestHandler.cpp \
	InputOutput/HttpResponse.cpp \
	InputOutput/HttpServer.cpp \
	InputOutput/HttpServerTask.cpp \
	InputOutput/JournalFileReader.cpp \
	InputOutput/JournalFileWriter.cpp \
	InputOutput/ListenTask.cpp \
	InputOutput/Logger.cpp \
	InputOutput/LoginWorker.cpp \
	InputOutput/ReadTask.cpp \
	InputOutput/Scheduler.cpp \
	InputOutput/Statistics.cpp \
	InputOutput/StatisticsDocumentation.cpp \
	InputOutput/Task.cpp \
	InputOutput/Worker.cpp \
	InputOutput/WorkerTask.cpp \
	InputOutput/WriteTask.cpp \
	Olap/AliasDimension.cpp \
	Olap/AreaPage.cpp \
	Olap/AreaStorage.cpp \
	Olap/AttributedDimension.cpp \
	Olap/AttributesCube.cpp \
	Olap/AttributesDimension.cpp \
	Olap/BasicDimension.cpp \
	Olap/CacheConsolidationsPage.cpp \
	Olap/CacheConsolidationsStorage.cpp \
	Olap/CachePage.cpp \
	Olap/CacheRulesPage.cpp \
	Olap/CacheRulesStorage.cpp \
	Olap/CacheStorage.cpp \
	Olap/CellPath.cpp \
	Olap/ConfigurationCube.cpp \
	Olap/ConfigurationDimension.cpp \
	Olap/Cube.cpp \
	Olap/CubeDimension.cpp \
	Olap/CubePage.cpp \
	Olap/CubeStorage.cpp \
	Olap/CubeWorker.cpp \
	Olap/Database.cpp \
	Olap/Dimension.cpp \
	Olap/DimensionDimension.cpp \
	Olap/Element.cpp \
	Olap/ExportPage.cpp \
	Olap/ExportStorage.cpp \
	Olap/GoalSeekSolver.cpp \
	Olap/Lock.cpp \
	Olap/MarkerStorage.cpp \
	Olap/NormalCube.cpp \
	Olap/NormalDatabase.cpp \
	Olap/NormalDimension.cpp \
	Olap/PaloSession.cpp \
	Olap/RightsCube.cpp \
	Olap/RightsDimension.cpp \
	Olap/RollbackPage.cpp \
	Olap/RollbackStorage.cpp \
	Olap/Rule.cpp \
	Olap/RuleMarker.cpp \
	Olap/Server.cpp \
	Olap/SubsetViewCube.cpp \
	Olap/SubsetViewDimension.cpp \
	Olap/SystemCube.cpp \
	Olap/SystemDatabase.cpp \
	Olap/User.cpp \
	Olap/UserInfoCube.cpp \
	Olap/UserInfoDimension.cpp \
	Options/OptionSpecification.cpp \
	Options/Options.cpp \
	Options/OptionsFileIterator.cpp \
	Parser/AreaNode.cpp \
	Parser/DestinationNode.cpp \
	Parser/DoubleNode.cpp \
	Parser/FunctionNode.cpp \
	Parser/FunctionNodeComparison.cpp \
	Parser/FunctionNodeSimple.cpp \
	Parser/FunctionNodeSimpleSingle.cpp \
	Parser/PaloFunctionNodeFactory.cpp \
	Parser/ParseFunctionNodeFactory.cpp \
	Parser/RuleNode.cpp \
	Parser/RuleOptimizer.cpp \
	Parser/RuleParser.cpp \
	Parser/RuleParserDriver.cpp \
	Parser/RuleScanner.cpp \
	Parser/SourceNode.cpp \
	Parser/StringNode.cpp \
	Parser/VariableNode.cpp



if ENABLE_HTTP
pkglib_LTLIBRARIES += libhttp.la

if ENABLE_HTTP_MODULE
  libhttp_la_LDFLAGS = -module
else
  libhttp_la_LDFLAGS = -static
endif

libhttp_la_SOURCES = \
	HttpServer/BrowserDocumentation.cpp \
	HttpServer/CubeBrowserDocumentation.cpp \
	HttpServer/CubeBrowserHandler.cpp \
	HttpServer/DatabaseBrowserDocumentation.cpp \
	HttpServer/DimensionBrowserDocumentation.cpp \
	HttpServer/ElementBrowserDocumentation.cpp \
	HttpServer/ErrorCodeBrowserDocumentation.cpp \
	HttpServer/PaloHttpBrowserHandler.cpp \
	HttpServer/PaloHttpHandler.cpp \
	HttpServer/RuleBrowserDocumentation.cpp \
	HttpServer/RuleBrowserHandler.cpp \
	HttpServer/EventBeginHandler.cpp \
	HttpServer/MemoryStatisticsBrowserDocumentation.cpp \
	HttpServer/ServerBrowserDocumentation.cpp
endif



if ENABLE_HTTPS
pkglib_LTLIBRARIES += libhttps.la

if ENABLE_HTTPS_MODULE
  libhttps_la_LDFLAGS = -module @OPENSSL_LIBS@
else
  libhttps_la_LDFLAGS = -static @OPENSSL_LIBS@
endif

libhttps_la_SOURCES = \
	HttpsServer/HttpsServerTask.cpp \
	HttpsServer/PaloHttpsServer.cpp
endif



if ENABLE_LEGACY
pkglib_LTLIBRARIES += liblegacy.la

if ENABLE_LEGACY_MODULE
  liblegacy_la_LDFLAGS = -module
else
  liblegacy_la_LDFLAGS = -static
endif

liblegacy_la_SOURCES = \
	LegacyServer/ArgumentDouble.cpp \
	LegacyServer/ArgumentInt32.cpp \
	LegacyServer/ArgumentMulti.cpp \
	LegacyServer/ArgumentString.cpp \
	LegacyServer/ArgumentError.cpp \
	LegacyServer/ArgumentUInt32.cpp \
	LegacyServer/ArgumentVoid.cpp \
	LegacyServer/LegacyArgument.cpp \
	LegacyServer/LegacyRequestHandler.cpp \
	LegacyServer/LegacyServer.cpp \
	LegacyServer/LegacyServerTask.cpp
endif



if ENABLE_BISON
@top_srcdir@/Parser/RuleParser.cpp: @top_srcdir@/Parser/RuleParser.yy
	$(BISON) -l -d -ra -S lalr1.cc -o @top_srcdir@/Parser/RuleParser.cpp @top_srcdir@/Parser/RuleParser.yy
	@mv @top_srcdir@/Parser/RuleParser.hpp @top_srcdir@/Parser/RuleParser.h
	sed -e 's:\.hpp:.h:' < @top_srcdir@/Parser/RuleParser.cpp > @top_srcdir@/Parser/RuleParser.cpp.tmp \
	  && mv @top_srcdir@/Parser/RuleParser.cpp.tmp @top_srcdir@/Parser/RuleParser.cpp
	sed -e 's:return seq_.size ();:return (unsigned int) seq_.size ();:' < @top_srcdir@/Parser/stack.hh > @top_srcdir@/Parser/stack.hh.tmp \
	  && mv @top_srcdir@/Parser/stack.hh.tmp @top_srcdir@/Parser/stack.hh
endif



if ENABLE_FLEX
@top_srcdir@/Parser/RuleScanner.cpp: @top_srcdir@/Parser/RuleScanner.ll
	$(LEX) --nounistd -L -o@top_srcdir@/Parser/RuleScanner.cpp @top_srcdir@/Parser/RuleScanner.ll
	sed -e 's:\.hpp:.h:' < @top_srcdir@/Parser/RuleScanner.cpp > @top_srcdir@/Parser/RuleScanner.cpp.tmp \
	  && mv @top_srcdir@/Parser/RuleScanner.cpp.tmp @top_srcdir@/Parser/RuleScanner.cpp
endif



-include @top_srcdir@/Makefile.triagens



.PHONY: doxygen

doxygen: Doxygen/palo.doxy
	cd Doxygen && doxygen palo.doxy
