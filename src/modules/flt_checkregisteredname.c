/*
 * \file flt_preview.c
 * \author Christian Kruse, <ckruse@wwwtech.de>
 * \brief Posting preview
 *
 * This file is a plugin for fo_post. It gives the user the
 * ability to preview his postings.
 *
 */

/* {{{ Initial comments */
/*
 * $LastChangedDate$
 * $LastChangedRevision$
 * $LastChangedBy$
 *
 */
/* }}} */

/* {{{ Includes */
#include "config.h"
#include "defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>

#include <time.h>

#include <errno.h>

#include "readline.h"
#include "hashlib.h"
#include "utils.h"
#include "configparser.h"
#include "cfcgi.h"
#include "template.h"
#include "clientlib.h"
#include "htmllib.h"
#include "fo_post.h"
/* }}} */

/* {{{ flt_checkregisteredname_execute */
#ifdef CF_SHARED_MEM
int flt_checkregisteredname_execute(t_cf_hash *head,t_configuration *dc,t_configuration *pc,t_message *p,void *ptr,int sock,int mode)
#else
int flt_checkregisteredname_execute(t_cf_hash *head,t_configuration *dc,t_configuration *pc,t_message *p,int sock,int mode)
#endif
{
  u_char *username = cf_hash_get(GlobalValues,"UserName",8);
  u_char *forum_name = cf_hash_get(GlobalValues,"FORUM_NAME",10);
  u_char *name,*line;

  rline_t rsd;

  t_string str;

  if(!head) return FLT_DECLINE;
  memset(&rsd,0,sizeof(rsd));

  name = cf_cgi_get(head,"Name");
  str_init(&str);

  str_chars_append(&str,"SELECT ",7);
  str_chars_append(&str,forum_name,strlen(forum_name));
  str_chars_append(&str,"\nAUTH CHECK\nName: ",18);
  str_chars_append(&str,name,strlen(name));
  str_char_append(&str,'\n');

  if(username) {
    str_chars_append(&str,"Pass: ",6);
    str_chars_append(&str,username,strlen(username));
    str_char_append(&str,'\n');
  }

  str_char_append(&str,'\n');

  writen(sock,str.content,str.len);
  str_cleanup(&str);

  if((line = readline(sock,&rsd)) == NULL) return FLT_DECLINE;
  free(line);
  if((line = readline(sock,&rsd)) == NULL) return FLT_DECLINE;

  if(cf_strncmp(line,"200",3) != 0) {
    strcpy(ErrorString,"E_auth_required");
    display_posting_form(head,p);

    free(line);
    return FLT_EXIT;
  }

  free(line);
  return FLT_OK;
}
/* }}} */

t_conf_opt flt_checkregisteredname_config[] = {
  { NULL, NULL, 0, NULL }
};

t_handler_config flt_checkregisteredname_handlers[] = {
  { NEW_POST_HANDLER,  flt_checkregisteredname_execute },
  { 0, NULL }
};

t_module_config flt_checkregisteredname = {
  flt_checkregisteredname_config,
  flt_checkregisteredname_handlers,
  NULL,
  NULL,
  NULL,
  NULL
};

/* eof */