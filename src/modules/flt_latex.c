/**
 * \file flt_latex.c
 * \author Christian Kruse, <ckruse@wwwtech.de>
 *
 * This plugin handles lot of standard directives
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
#include <time.h>

#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/wait.h>

#include <openssl/md5.h>

#include "readline.h"
#include "hashlib.h"
#include "utils.h"
#include "configparser.h"
#include "cfcgi.h"
#include "template.h"
#include "clientlib.h"
#include "charconvert.h"
#include "validate.h"
#include "htmllib.h"
/* }}} */

#define FLT_LATEX_PNG    0
#define FLT_LATEX_OBJECT 1
#define FLT_LATEX_INLINE 2

static struct {
  u_char *cache_path;
  u_char *tmp_path;
  u_char *texvc;
  u_char *path_env;
  u_char *uri;
  int mode;
} flt_latex_cfg = { NULL, NULL, NULL, NULL, NULL, FLT_LATEX_PNG };

/* {{{ flt_latex_create_cache */
int flt_latex_create_cache(const u_char *cnt,const u_char *our_sum) {
  u_char *envp[] = {
    flt_latex_cfg.path_env,
    NULL
  };

  int fds[2];
  char c,sum[33],buff[512];
  ssize_t n;
  t_string mml,path,path1;

  FILE *fd;
  pid_t pid;

  sum[32] = '\0';

  if(pipe(fds) < 0) return -1;

  switch(pid = fork()) {
    case -1:
      return -1;
    case 0:
      dup2(fds[0],STDIN_FILENO);
      dup2(fds[1],STDOUT_FILENO);
      close(STDERR_FILENO);

      if(execle(flt_latex_cfg.texvc,"texvc",flt_latex_cfg.tmp_path,flt_latex_cfg.cache_path,cnt,"UTF-8",(char *)NULL,envp) < 0) {
        perror("texvc");
        exit(-1);
      }
      exit(0);
  }

  close(fds[1]);

  n = read(fds[0],&c,1);

  if(!isalpha(c) || !isupper(c) || c == 'F' || c == 'S' || c == 'E') {
    close(fds[0]);
    waitpid(pid,NULL,0);
    return -1;
  }

  read(fds[0],sum,32);

  switch(c) {
    case 'C':
    case 'M':
    case 'L':
      while(read(fds[0],&c,1) > 0 && c != '\0');

      /* something went wrong */
      if(c != '\0') {
        close(fds[0]);
        waitpid(pid,NULL,0);
        return -1;
      }
  }

  str_init(&mml);
  str_init(&path);
  str_init(&path1);

  while((n = read(fds[0],buff,512)) > 0) str_chars_append(&mml,buff,n);

  /* something went wrong */
  if(n < 0) {
    str_cleanup(&mml);

    close(fds[0]);
    waitpid(pid,NULL,0);
    return -1;
  }

  str_char_set(&path,flt_latex_cfg.cache_path,strlen(flt_latex_cfg.cache_path));
  str_char_set(&path1,flt_latex_cfg.cache_path,strlen(flt_latex_cfg.cache_path));
  str_chars_append(&path,our_sum,32);
  str_chars_append(&path1,sum,32);
  str_chars_append(&path,".mml",4);
  str_chars_append(&path1,".png",4);

  if((fd = fopen(path.content,"w")) == NULL) {
    str_cleanup(&mml);
    str_cleanup(&path);
    str_cleanup(&path1);

    close(fds[0]);
    waitpid(pid,NULL,0);
    return -1;
  }

  fwrite(mml.content,1,mml.len,fd);
  fclose(fd);

  path.len -= 4;
  str_chars_append(&path,".png",4);
  rename(path1.content,path.content);

  str_cleanup(&mml);
  str_cleanup(&path);
  str_cleanup(&path1);

  close(fds[0]);
  waitpid(pid,NULL,0);

  return 0;
}
/* }}} */

/* {{{ flt_latex_create_md5_sum */
void flt_latex_create_md5_sum(u_char *str,size_t len,u_char *res) {
  u_char sum[16];
  int i;

  MD5(str,len,sum);

  for(i=0;i<16;++i,res+=2) sprintf(res,"%02x",sum[i]);
  *res = '\0';
}
/* }}} */

/* {{{ flt_latex_get_mml */
int flt_latex_get_mml(t_string *which,u_char *sum) {
  t_string path;
  FILE *fd;
  u_char buff[512];
  size_t n = 1;

  str_init(&path);
  str_char_set(&path,flt_latex_cfg.cache_path,strlen(flt_latex_cfg.cache_path));
  str_chars_append(&path,sum,32);
  str_chars_append(&path,".mml",4);

  if((fd = fopen(path.content,"r")) == NULL) return -1;

  while(!feof(fd) && n > 0) {
    n = fread(buff,1,512,fd);
    if(n > 0) str_chars_append(which,buff,n);
  }

  if(!feof(fd)) {
    fclose(fd);
    return -1;
  }

  fclose(fd);

  return 0;
}
/* }}} */

int flt_latex_execute(t_configuration *fdc,t_configuration *fvc,const u_char *directive,const u_char **parameters,size_t plen,t_string *bco,t_string *bci,t_string *content,t_string *cite,const u_char *qchars,int sig) {
  u_char *forum_name = cf_hash_get(GlobalValues,"FORUM_NAME",10);
  u_char sum[33];
  t_string str;
  struct stat st;
  int obj;
  t_name_value *xhtml = cfg_get_first_value(fdc,forum_name,"XHTMLMode");
  size_t len;

  if(sig) return FLT_DECLINE;

  str_init(&str);

  flt_latex_create_md5_sum(content->content,content->len,sum);

  str_char_set(&str,flt_latex_cfg.cache_path,strlen(flt_latex_cfg.cache_path));
  str_chars_append(&str,sum,32);
  str_chars_append(&str,".png",4);

  if(stat(str.content,&st) == -1) {
    if(flt_latex_create_cache(content->content,sum) == -1) return FLT_DECLINE;
  }

  switch(flt_latex_cfg.mode) {
    case FLT_LATEX_INLINE:
      len = bco->len;

      str_chars_append(bco,"<math xmlns=\"http://www.w3.org/1998/Math/MathML\">",49);
      if(flt_latex_get_mml(bco,sum) == -1) {
        bco->len = len;
        bco->content[bco->len] = '\0';
        return FLT_DECLINE;
      }
      str_chars_append(bco,"</math>",7);
      break;

    case FLT_LATEX_OBJECT:
      obj = 1;
      str_chars_append(bco,"<object type=\"text/mathml\" data=\"",33);
      str_chars_append(bco,flt_latex_cfg.uri,strlen(flt_latex_cfg.uri));
      str_chars_append(bco,sum,32);
      str_chars_append(bco,".mml\">",6);

    case FLT_LATEX_PNG:
      str_chars_append(bco,"<img src=\"",10);
      str_chars_append(bco,flt_latex_cfg.uri,strlen(flt_latex_cfg.uri));
      str_chars_append(bco,sum,32);
      str_chars_append(bco,".png\"",5);

      if(*xhtml->values[0] == 'y')  str_chars_append(bco," />",3);
      else str_chars_append(bco,">",1);

      if(obj) str_chars_append(bco,"</object>",9);
  }


  return FLT_OK;
}

int flt_latex_init(t_cf_hash *cgi,t_configuration *dc,t_configuration *vc) {
  cf_html_register_directive("latex",flt_latex_execute,CF_HTML_DIR_TYPE_ARG|CF_HTML_DIR_TYPE_BLOCK);

  return FLT_DECLINE;
}

/* {{{ flt_latex_handle */
int flt_latex_handle(t_configfile *cfile,t_conf_opt *opt,const u_char *context,u_char **args,size_t argnum) {
  u_char *fn;

  if(cf_strcmp(opt->name,"LatexCachePath") == 0) {
    fn = cf_hash_get(GlobalValues,"FORUM_NAME",10);
    if(cf_strcmp(context,fn) == 0) {
      if(flt_latex_cfg.cache_path) free(flt_latex_cfg.cache_path);
      flt_latex_cfg.cache_path = strdup(args[0]);
    }
  }
  else if(cf_strcmp(opt->name,"LatexPathEnv") == 0) {
    if(flt_latex_cfg.path_env) free(flt_latex_cfg.path_env);
    flt_latex_cfg.path_env = strdup(args[0]);
  }
  else if(cf_strcmp(opt->name,"LatexTmpPath") == 0) {
    if(flt_latex_cfg.tmp_path) free(flt_latex_cfg.tmp_path);
    flt_latex_cfg.tmp_path = strdup(args[0]);
  }
  else if(cf_strcmp(opt->name,"LatexMode") == 0) {
    fn = cf_hash_get(GlobalValues,"FORUM_NAME",10);
    if(cf_strcmp(context,fn) == 0) {
      if(cf_strcmp(args[0],"INLINE") == 0) flt_latex_cfg.mode = FLT_LATEX_INLINE;
      else if(cf_strcmp(args[0],"OBJECT") == 0) flt_latex_cfg.mode = FLT_LATEX_OBJECT;
      else flt_latex_cfg.mode = FLT_LATEX_PNG;
    }
  }
  else if(cf_strcmp(opt->name,"LatexUri") == 0) {
    fn = cf_hash_get(GlobalValues,"FORUM_NAME",10);
    if(cf_strcmp(context,fn) == 0) {
      if(flt_latex_cfg.uri) free(flt_latex_cfg.uri);
      flt_latex_cfg.uri = strdup(args[0]);
    }
  }
  else {
    if(flt_latex_cfg.texvc) free(flt_latex_cfg.texvc);
    flt_latex_cfg.texvc = strdup(args[0]);
  }

  return 0;
}
/* }}} */

t_conf_opt flt_latex_config[] = {
  { "LatexTmpPath",   flt_latex_handle, CFG_OPT_CONFIG|CFG_OPT_GLOBAL|CFG_OPT_NEEDED, NULL },
  { "LatexTexvcPath", flt_latex_handle, CFG_OPT_CONFIG|CFG_OPT_GLOBAL|CFG_OPT_NEEDED, NULL },
  { "LatexPathEnv",   flt_latex_handle, CFG_OPT_CONFIG|CFG_OPT_GLOBAL|CFG_OPT_NEEDED, NULL },
  { "LatexCachePath", flt_latex_handle, CFG_OPT_CONFIG|CFG_OPT_LOCAL|CFG_OPT_NEEDED,  NULL },
  { "LatexUri",       flt_latex_handle, CFG_OPT_CONFIG|CFG_OPT_LOCAL|CFG_OPT_NEEDED,  NULL },
  { "LatexMode",      flt_latex_handle, CFG_OPT_CONFIG|CFG_OPT_USER|CFG_OPT_LOCAL,    NULL },
  { NULL, NULL, 0, NULL }
};

t_handler_config flt_latex_handlers[] = {
  { INIT_HANDLER, flt_latex_init },
  { 0, NULL }
};

t_module_config flt_latex = {
  flt_latex_config,
  flt_latex_handlers,
  NULL,
  NULL,
  NULL,
  NULL
};


/* eof */