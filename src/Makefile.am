
SUBDIRS=. parsetpl
#modules swig perl

bin_PROGRAMS= \
	fo_server \
	fo_view \
	fo_vote \
	cf-tid_index \
	fo_post \
	cf-recovery \
	fo_arcview \
	fo_threadlookup \
	fo_feeds
#	fo_userconf \
#	fo_usermanagement \
#	fo_xmlrpc

pkglib_LTLIBRARIES= \
	libcfcharconvert.la \
	libcfreadline.la \
	libcfhashlib.la \
	libcfutils.la \
	libcfvalidate.la \
	libcfcgi.la \
	libcfconfigparser.la \
	libcftemplate.la \
	libcfclientlib.la \
	libcfhtml.la \
	libcfserverutils.la \
	libcfserverlib.la
#	libcfuserconfig.la

libcfcharconvert_la_SOURCES=charconvert.c
libcfcharconvert_la_LIBADD=$(ICONV)
libcfcharconvert_la_LDFLAGS=-export-dynamic -avoid-version

libcfreadline_la_SOURCES=readline.c
libcfreadline_la_LDFLAGS=-export-dynamic -avoid-version

libcfhashlib_la_SOURCES=hashlib.c
libcfhashlib_la_LDFLAGS=-export-dynamic -avoid-version

libcfutils_la_SOURCES=utils.c arrayutils.c listutils.c memoryutils.c stringutils.c treeutils.c cacheutils.c datautils.c ipcutils.c wwwutils.c
libcfutils_la_LIBADD=libcfhashlib.la libcfcharconvert.la -lz $(CURL_LIBS)
libcfutils_la_LDFLAGS=-export-dynamic -avoid-version

libcfcgi_la_SOURCES=cfcgi.c
libcfcgi_la_LIBADD=libcfhashlib.la
libcfcgi_la_LDFLAGS=-export-dynamic -avoid-version

libcfvalidate_la_SOURCES=validate.c
libcfvalidate_la_LIBADD=libcfutils.la $(IDN)
libcfvalidate_la_LDFLAGS=-export-dynamic -avoid-version

libcftemplate_la_SOURCES=template.c
libcftemplate_la_LIBADD=libcfutils.la libcfhashlib.la libcfcharconvert.la $(DL)
libcftemplate_la_LDFLAGS=-export-dynamic -avoid-version

libcfclientlib_la_SOURCES=clientlib.c handlerrunners.c communication.c messageutils.c semaphores.c
libcfclientlib_la_LIBADD=libcfconfigparser.la libcfcharconvert.la libcfreadline.la libcfutils.la libcfhashlib.la libcfcgi.la libcftemplate.la $(BDB_LIB)
libcfclientlib_la_LDFLAGS=-export-dynamic -avoid-version

libcfhtml_la_SOURCES=htmllib.c
libcfhtml_la_LIBADD=libcfutils.la libcfcharconvert.la libcfclientlib.la
libcfhtml_la_LDFLAGS=-export-dynamic -avoid-version

libcfconfigparser_la_SOURCES=cfgcomp_assembler.c cfgcomp.c cfgcomp_codegenerator.c cfgcomp_lexer.c cfgcomp_parser.c cfgcomp_vm.c
libcfconfigparser_la_LIBADD=libcfutils.la libcfcharconvert.la $(DL)
libcfconfigparser_la_LDFLAGS=-export-dynamic -avoid-version

libcfserverutils_la_SOURCES=serverutils.c cf_pthread.c
libcfserverutils_la_LDFLAGS=-export-dynamic -avoid-version

libcfserverlib_la_SOURCES=serverlib.c shm_locking.c semaphores.c archiver.c cf_pthread.c
libcfserverlib_la_LIBADD=libcfhashlib.la libcfutils.la libcfreadline.la $(PTHREAD_LIB)
libcfserverlib_la_LDFLAGS=-export-dynamic -avoid-version

#libcfuserconfig_la_SOURCES=userconf.c xml_handling.c
#libcfuserconfig_la_LIBADD=libcfhashlib.la libcfutils.la libcfconfigparser.la libcfclientlib.la libcfvalidate.la $(PCRE_LIBS) $(GDOME_LIBS)
#libcfuserconfig_la_LDFLAGS=-export-dynamic -avoid-version

fo_threadlookup_SOURCES=fo_threadlookup.c
fo_threadlookup_LDADD=libcfclientlib.la libcfutils.la libcfconfigparser.la libcfcgi.la $(BDB_LIB)
fo_threadlookup_LDFLAGS=-export-dynamic

#fo_xmlrpc_SOURCES=fo_xmlrpc.c
#fo_xmlrpc_LDADD=libcfclientlib.la libcfutils.la libcfconfigparser.la libcfcgi.la $(XMLRPC_C_LIBS)
#fo_xmlrpc_LDFLAGS=-export-dynamic

fo_arcview_SOURCES=fo_arcview.c
fo_arcview_LDADD=libcfconfigparser.la libcftemplate.la libcfcharconvert.la libcfutils.la libcfhashlib.la libcfclientlib.la libcfcgi.la libcfhtml.la
fo_arcview_LDFLAGS=-export-dynamic

cf_tid_index_SOURCES=cf-tid_index.c
cf_tid_index_LDADD=libcfutils.la libcfconfigparser.la $(BDB_LIB)
cf_tid_index_LDFLAGS=-export-dynamic

cf_recovery_SOURCES=cf-recovery.c
cf_recovery_LDADD=libcfcharconvert.la libcfreadline.la libcfutils.la libcfserverlib.la libcfconfigparser.la libcfserverutils.la libcfhashlib.la
cf_recovery_LDFLAGS=-export-dynamic

fo_vote_SOURCES=fo_vote.c
fo_vote_LDADD=libcfutils.la libcfhashlib.la libcfreadline.la libcfconfigparser.la libcfclientlib.la libcfcgi.la libcftemplate.la $(BDB_LIB)
fo_vote_LDFLAGS=-export-dynamic

#fo_userconf_SOURCES=fo_userconf.c
#fo_userconf_LDADD=libcfuserconfig.la libcfutils.la libcfhashlib.la libcfreadline.la libcfconfigparser.la libcfclientlib.la libcfcgi.la libcftemplate.la
#fo_userconf_LDFLAGS=-export-dynamic
#
#fo_usermanagement_SOURCES=fo_usermanagement.c
#fo_usermanagement_LDADD=libcfutils.la libcfhashlib.la libcfreadline.la libcfconfigparser.la libcfclientlib.la libcfcgi.la libcftemplate.la
#fo_usermanagement_LDFLAGS=-export-dynamic

fo_post_SOURCES=fo_post.c
fo_post_LDADD=libcfconfigparser.la libcftemplate.la libcfcharconvert.la libcfreadline.la libcfutils.la libcfhashlib.la libcfclientlib.la libcfcgi.la libcfhtml.la libcfvalidate.la $(PCRE_LIBS)
fo_post_LDFLAGS=-export-dynamic

fo_feeds_SOURCES=fo_feeds.c
fo_feeds_LDADD=libcfconfigparser.la libcftemplate.la libcfcharconvert.la libcfreadline.la libcfutils.la libcfhashlib.la libcfclientlib.la libcfcgi.la libcfhtml.la
fo_feeds_LDFLAGS=-export-dynamic

fo_view_SOURCES=fo_view.c
fo_view_LDADD=libcfconfigparser.la libcftemplate.la libcfcharconvert.la libcfreadline.la libcfutils.la libcfhashlib.la libcfclientlib.la libcfcgi.la libcfhtml.la
fo_view_LDFLAGS=-export-dynamic

fo_server_SOURCES=fo_server.c
fo_server_LDADD=libcfcharconvert.la libcfreadline.la libcfutils.la libcfserverlib.la libcfconfigparser.la libcfserverutils.la libcfhashlib.la $(PTHREAD_LIB)
fo_server_LDFLAGS=-export-dynamic

# eof
