/**
 * \file defines.h
 * \author Christian Kruse, <cjk@wwwtech.de>
 * \brief the standard defines
 *
 * This file contains some standard definitions needed in nearly any source file of the Classic Forum.
 */

/* {{{ Initial headers */
/*
 * $LastChangedDate$
 * $LastChangedRevision$
 * $LastChangedBy$
 *
 */
/* }}} */

#ifndef _CF_DEFINES_H
#define _CF_DEFINES_H

/*
 * Magic numbers change with every API change. Binary backwards compatibility
 * leads to a minor change, backwards incompatibility leads to a major change
 */
#define MODULE_MAGIC_NUMBER_MAJOR ((u_int16_t)0x4) /* upper 16 bit */
#define MODULE_MAGIC_NUMBER_MINOR ((u_int16_t)0x1) /* lower 16 bit */
#define MODULE_MAGIC_COOKIE ((u_int32_t)((MODULE_MAGIC_NUMBER_MAJOR << 16) | (MODULE_MAGIC_NUMBER_MINOR)))

#define CF_SORT_ASCENDING 1
#define CF_SORT_DESCENDING 2
#define CF_SORT_NEWESTFIRST 3

#define CF_VERSION "4.0"

#define CF_BUFSIZ BUFSIZ

#define CF_ERR (1<<1) /**< Used by the logging function. Log an error. */
#define CF_STD (1<<2) /**< Used by the logging function. Log a standard message. */
#define CF_DBG (1<<3) /**< Used by the logging function. Log a debugging message. */
#define CF_FLSH (1<<4) /**< Used by the logging fuction. If bit is set, stream will be flusehd */

#define TIMER 5L /**< Timer value. Check every 5 seconds. */

/* some limits */
#define PRERESERVE 5 /**< Limit in lists or for bigger arrays to reserve before used (to avoid malloc() calls) */
#define MAXLINE    BUFSIZ /**< The size of a buffer for lines */

#define LISTENQ 1024 /**< The size of the listen() queque. */

#define INIT_HANDLER             1 /**< Handler hook number. Look in the programmers manual section for more information. */
#define VIEW_HANDLER             2 /**< Handler hook number. Look in the programmers manual section for more information. */
#define VIEW_INIT_HANDLER        3 /**< Handler hook number. Look in the programmers manual section for more information. */
#define VIEW_LIST_HANDLER        4 /**< Handler hook number. Look in the programmers manual section for more information. */
#define POSTING_HANDLER          5 /**< Handler hook number. Look in the programmers manual section for more information. */
#define CONNECT_INIT_HANDLER     6 /**< Handler hook number. Look in the programmers manual section for more information. */
#define AUTH_HANDLER             7 /**< Handler hook number. Look in the programmers manual section for more information. */
#define ARCHIVE_HANDLER          8 /**< Handler hook number. Look in the programmers manual section for more information. */
#define NEW_POST_HANDLER         9 /**< Handler hook number. Look in the programmers man... argh, fuck off, you know what I mean */
#define AFTER_POST_HANDLER      10 /**< Handler hook number. .... */
#define HANDLE_404_HANDLER      11 /**< Handler hook .... */
#define DIRECTIVE_FILTER        12 /**< Handler hook */
#define PRE_CONTENT_FILTER      13 /**< Handler hook */
#define POST_CONTENT_FILTER     14 /**< Handler hook */
#define NEW_THREAD_HANDLER      15 /**< Handler h... */
#define DATA_LOADING_HANDLER    16 /**< ... */
#define THRDLST_WRITE_HANDLER   17 /**< ... */
#define ARCHIVE_THREAD_HANDLER  18 /**< ... */
#define SORTING_HANDLER         19 /**< ... */
#define POST_DISPLAY_HANDLER    20 /**< ... */
#define URL_REWRITE_HANDLER     21
#define THREAD_SORTING_HANDLER  22
#define THRDLST_WRITTEN_HANDLER 23
#define PERPOST_VAR_HANDLER     24
#define RM_COLLECTORS_HANDLER   25
#define REMOVE_THREAD_HANDLER   26
#define UCONF_WRITE_HANDLER     27
#define UCONF_DISPLAY_HANDLER   28

#define MOD_MAX                 28 /**< The maximum hook value. */

#define FLT_OK       0 /**< Returned by a plugin function if everything as ok. */
#define FLT_DECLINE -1 /**< Returned by a plugin function if this request is not for the plugin. */
#define FLT_EXIT    -2 /**< Context dependend. */
#define FLT_ERROR   -3 /**< Error value */

#define init_modules() memset(&Modules,0,(MOD_MAX+1) * sizeof(cf_array_t)) /**< Initialization macro for the plugins */

#define CF_KILL_DELETED 0 /**< kill deleted messages constant */
#define CF_KEEP_DELETED 1 /**< keep deleted messages constant */

#ifndef HAVE_U_CHAR
typedef unsigned char u_char; /**< We only use unsigned char (u_char) instead of char */
#endif

#endif

/* eof */
