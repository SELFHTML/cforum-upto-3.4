/**
 * \file handlerrunners.c
 * \author Christian Kruse, <cjk@wwwtech.de>
 * \brief server communication functions
 *
 * This file contains functions to communicate with the forum server
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
#include "cfconfig.h"
#include "defines.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef CF_SHARED_MEM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#endif

#include <inttypes.h>
#include <sys/types.h>

#include "hashlib.h"
#include "utils.h"
#include "cfgcomp.h"
#include "template.h"
#include "readline.h"
#include "charconvert.h"
#include "clientlib.h"
/* }}} */

/* {{{ cf_get_threadlist */
#ifndef CF_SHARED_MEM
int cf_get_threadlist(cf_array_t *ary,int sock,rline_t *tsd)
#else
int cf_get_threadlist(cf_array_t *ary,int shmids[3],void *ptr)
#endif
{
  cf_cl_thread_t thread;
  cf_array_init(ary,sizeof(thread),(void (*)(void *))cf_cleanup_thread);

  #ifndef CF_SHARED_MEM
  while(cf_get_next_thread_through_sock(sock,tsd,&thread) == 0)
  #else
  while((ptr = cf_get_next_thread_through_shm(ptr,shmids,&thread)) != NULL)
  #endif
  {
    cf_array_push(ary,&thread);
    memset(&thread,0,sizeof(thread));
  }

  return 0;
}
/* }}} */


/* {{{ cf_get_next_thread_through_sock */
int cf_get_next_thread_through_sock(int sock,rline_t *tsd,cf_cl_thread_t *thr) {
  u_char *line,*chtmp;
  int shallRun = 1;
  cf_message_t *x;
  int ok = 0;
  cf_post_flag_t flag;

  memset(thr,0,sizeof(*thr));

  /* now lets read the threadlist */
  while(shallRun) {
    line = readline(sock,tsd);

    if(line) {
      line[tsd->rl_len-1] = '\0';

      if(*line == '\0') shallRun = 0;
      else if(cf_strncmp(line,"THREAD t",8) == 0) {
        chtmp = strstr(line,"m");

        thr->messages           = cf_alloc(NULL,1,sizeof(*thr->messages),CF_ALLOC_CALLOC);
        thr->last               = thr->messages;
        thr->newest             = thr->messages;
        thr->last->mid          = cf_str_to_uint64(chtmp+1);
        thr->messages->may_show = 1;
        thr->msg_len            = 1;
        thr->tid                = cf_str_to_uint64(line+8);

        cf_tpl_var_init(&thr->messages->hashvar,TPL_VARIABLE_HASH);
      }
      else if(cf_strncmp(line,"MSG m",5) == 0) {
        x = thr->last;

        if(thr->last) {
          thr->last->next = cf_alloc(NULL,1,sizeof(*thr->last->next),CF_ALLOC_CALLOC);
          thr->last->next->prev = thr->last;
          thr->last       = thr->last->next;
        }
        else {
          thr->messages = cf_alloc(NULL,1,sizeof(thr->messages),CF_ALLOC_CALLOC);
          thr->last     = thr->messages;
        }

        thr->last->mid      = cf_str_to_uint64(line+5);
        thr->last->may_show = 1;

        thr->msg_len++;

        cf_tpl_var_init(&thr->last->hashvar,TPL_VARIABLE_HASH);
      }
      else if(cf_strncmp(line,"Flag:",5) == 0) {
        chtmp = strstr(line+5,"=");

        flag.name = strndup(line+5,chtmp-line-5);
        flag.val  = strndup(chtmp+1,strlen(chtmp+1));

        cf_list_append(&thr->last->flags,&flag,sizeof(flag));
      }
      else if(cf_strncmp(line,"Date:",5) == 0) {
        thr->last->date = strtoul(line+5,NULL,10);
        if(thr->last->date > thr->newest->date) thr->newest = thr->last;
      }
      else if(cf_strncmp(line,"Author:",7) == 0)   cf_str_char_set(&thr->last->author,line+7,tsd->rl_len-8);
      else if(cf_strncmp(line,"Subject:",8) == 0)  cf_str_char_set(&thr->last->subject,line+8,tsd->rl_len-9);
      else if(cf_strncmp(line,"Category:",9) == 0) cf_str_char_set(&thr->last->category,line+9,tsd->rl_len-10);
      else if(cf_strncmp(line,"Level:",6) == 0)    thr->last->level = atoi(line+6);
      else if(cf_strncmp(line,"Content:",8) == 0)  cf_str_char_set(&thr->last->content,line+8,tsd->rl_len-9);
      else if(cf_strncmp(line,"Homepage:",9) == 0) cf_str_char_set(&thr->last->hp,line+9,tsd->rl_len-10);
      else if(cf_strncmp(line,"Image:",6) == 0)    cf_str_char_set(&thr->last->img,line+6,tsd->rl_len-7);
      else if(cf_strncmp(line,"EMail:",6) == 0)    cf_str_char_set(&thr->last->email,line+6,tsd->rl_len-7);
      else if(cf_strncmp(line,"Votes-Good:",11) == 0) thr->last->votes_good = strtoul(line+11,NULL,10);
      else if(cf_strncmp(line,"Votes-Bad:",10) == 0)  thr->last->votes_bad = strtoul(line+10,NULL,10);
      else if(cf_strncmp(line,"Remote-Addr:",12) == 0) cf_str_char_set(&thr->last->remote_addr,line+12,tsd->rl_len-13);
      else if(cf_strncmp(line,"Visible:",8) == 0) {
        thr->last->invisible = line[8] == '0';
        if(thr->last->invisible) thr->msg_len--;
      }
      else if(cf_strncmp(line,"END",3) == 0) {
        shallRun = 0;
        ok = 1;
      }

      free(line);
    }
    else {
      shallRun = 0; /* connection broke */
      strcpy(ErrorString,"E_COMMUNICATE");
    }
  }

  if(ok) {
    /* {{{ build hierarchical structure */
    thr->ht = cf_alloc(NULL,1,sizeof(*thr->ht),CF_ALLOC_CALLOC);
    thr->ht->msg = thr->messages;

    if(thr->messages->next) cf_msg_build_hierarchical_structure(thr->ht,thr->messages->next);
    /* }}} */

    return 0;
  }
  return -1;
}
/* }}} */

/* {{{ cf_get_next_thread_through_shm */
#ifdef CF_SHARED_MEM
void *cf_get_next_thread_through_shm(int shmids[3],void *shm_ptr,cf_cl_thread_t *thr) {
  register void *ptr = shm_ptr;
  u_int32_t post,i,val,val1;
  struct shmid_ds shm_buf;
  void *ptr1 = cf_get_shm_ptr(shmids);
  u_int32_t msglen;
  cf_post_flag_t flag;

  memset(thr,0,sizeof(*thr));

  if(shmctl(shm_id,IPC_STAT,&shm_buf) != 0) {
    strcpy(ErrorString,"E_CONFIG_ERR");
    return NULL;
  }

  /*
   *
   * CAUTION! Deep magic begins here! Do not edit if you don't know
   * what you are doing!
   *
   */

  /* Uh, oh, we are at the end of the segment */
  if(ptr >= ptr1 + shm_buf.shm_segsz) return NULL;

  /* thread id */
  thr->tid = *((u_int64_t *)ptr);
  ptr += sizeof(u_int64_t);

  /* message id */
  thr->msg_len = msglen = *((u_int32_t *)ptr);
  ptr += sizeof(u_int32_t);

  for(post = 0;post < msglen;++post) {
    if(thr->last == NULL) thr->messages = thr->last = cf_alloc(NULL,1,sizeof(*thr->last),CF_ALLOC_CALLOC);
    else {
      thr->last->next = cf_alloc(NULL,1,sizeof(*thr->last->next),CF_ALLOC_CALLOC);
      thr->last->next->prev = thr->last;
      thr->last       = thr->last->next;
    }

    if(!thr->newest) thr->newest = thr->last;
    cf_tpl_var_init(&thr->last->hashvar,TPL_VARIABLE_HASH);

    /* {{{ message id */
    thr->last->mid = *((u_int64_t *)ptr);
    ptr += sizeof(u_int64_t);
    /* }}} */

    /* {{{ flags */
    val = *((u_int32_t *)ptr);
    ptr += sizeof(u_int32_t);

    for(i=0;i<val;++i) {
      val1 = *((u_int32_t *)ptr);
      ptr += sizeof(u_int32_t);
      flag.name = strndup(ptr,val1);
      ptr += val1;

      val1 = *((u_int32_t *)ptr);
      ptr += sizeof(u_int32_t);
      flag.val = strndup(ptr,val1);
      ptr += val1;

      cf_list_append(&thr->last->flags,&flag,sizeof(flag));
    }
    /* }}} */

    /* {{{ subject */
    /* length of subject */
    val = *((u_int32_t *)ptr) - 1;
    ptr += sizeof(u_int32_t);

    /* subject */
    cf_str_char_set(&thr->last->subject,ptr,val);
    ptr += val + 1;
    /* }}} */

    /* {{{ category */
    /* length of category */
    val = *((u_int32_t *)ptr) - 1;
    ptr += sizeof(u_int32_t);

    /* category */
    if(val) {
      cf_str_char_set(&thr->last->category,ptr,val);
      ptr += val + 1;
    }
    /* }}} */

    /* {{{ the remote address */
    val = *((u_int32_t *)ptr) - 1;
    ptr += sizeof(u_int32_t);
    cf_str_char_set(&thr->last->remote_addr,ptr,val);
    ptr += val + 1;
    /* }}} */

    /* {{{ content */
    /* content length */
    val = *((u_int32_t *)ptr) - 1;
    ptr += sizeof(u_int32_t);

    /* content */
    cf_str_char_set(&thr->last->content,ptr,val);
    ptr += val + 1;
    /* }}} */

    /* {{{ date */
    thr->last->date = *((time_t *)ptr);
    ptr += sizeof(time_t);

    if(thr->last->date > thr->newest->date) thr->newest = thr->last;
    /* }}} */

    /* {{{ level */
    thr->last->level = *((u_int16_t *)ptr);
    ptr += sizeof(u_int16_t);
    /* }}} */

    /* {{{ invisible */
    thr->last->invisible = *((u_int16_t *)ptr);
    thr->last->may_show  = 1;
    ptr += sizeof(u_int16_t);
    /* }}} */

    if(thr->last->invisible) thr->msg_len--;

    /* {{{ votings */
    thr->last->votes_good = *((u_int32_t *)ptr);
    ptr += sizeof(u_int32_t);

    thr->last->votes_bad = *((u_int32_t *)ptr);
    ptr += sizeof(u_int32_t);
    /* }}} */

    /* {{{ author */
    /* author length */
    val = *((u_int32_t *)ptr) - 1;
    ptr += sizeof(u_int32_t);

    /* author */
    cf_str_char_set(&thr->last->author,ptr,val);
    ptr += val + 1;
    /* }}} */

    /* {{{ email */
    /* email length */
    val = *((u_int32_t *)ptr);
    ptr += sizeof(u_int32_t);

    /* email */
    if(val) {
      cf_str_char_set(&thr->last->email,ptr,val-1);
      ptr += val;
    }
    /* }}} */

    /* {{{ homepage */
    /* homepage length */
    val = *((u_int32_t *)ptr);
    ptr += sizeof(u_int32_t);

    /* homepage */
    if(val) {
      cf_str_char_set(&thr->last->hp,ptr,val-1);
      ptr += val;
    }
    /* }}} */

    /* {{{ image */
    /* image length */
    val = *((u_int32_t *)ptr);
    ptr += sizeof(u_int32_t);

    /* image */
    if(val) {
      cf_str_char_set(&thr->last->img,ptr,val-1);
      ptr += val;
    }
    /* }}} */
  }

  /* {{{ build hierarchical structure */
  thr->ht = cf_alloc(NULL,1,sizeof(*thr->ht),CF_ALLOC_CALLOC);
  thr->ht->msg = thr->messages;

  if(thr->messages->next) cf_msg_build_hierarchical_structure(thr->ht,thr->messages->next);
  /* }}} */

  return ptr;
}
#endif
/* }}} */

/* {{{ cf_get_message_through_sock */
int cf_get_message_through_sock(int sock,rline_t *tsd,cf_cl_thread_t *thr,u_int64_t tid,u_int64_t mid,int del) {
  size_t len;
  u_char buff[128],*line;
  cf_message_t *msg;
  u_char *forum_name = cf_hash_get(GlobalValues,"FORUM_NAME",10);

  memset(thr,0,sizeof(*thr));

  len = snprintf(buff,128,"SELECT %s\n",forum_name);
  writen(sock,buff,len);

  line = readline(sock,tsd);
  if(line && cf_strncmp(line,"200 Ok",6) == 0) {
    free(line);

    len  = snprintf(buff,128,"GET POSTING t%" PRIu64 " m%" PRIu64 " invisible=%d\n",tid,mid,del);
    writen(sock,buff,len);

    line = readline(sock,tsd);

    if(line && cf_strncmp(line,"200 Ok",6) == 0) {
      free(line);

      thr->tid      = tid;
      thr->messages = NULL;
      thr->ht       = NULL;
      thr->last     = NULL;

      if(cf_get_next_thread_through_sock(sock,tsd,thr) < 0 && *ErrorString) {
        strcpy(ErrorString,"E_COMMUNICATION");
        return -1;
      }
      else {
        /* set thread message pointer to the right message */
        if(mid == 0) thr->threadmsg = thr->messages;
        else {
          for(msg = thr->messages;msg;msg=msg->next) {
            if(msg->mid == mid) {
              thr->threadmsg = msg;
              break;
            }
          }
        }
      }
    }
    else {
      /* bye, bye */
      if(line) {
        snprintf(ErrorString,50,"E_FO_%d",atoi(line));
        free(line);
      }
      else strcpy(ErrorString,"E_COMMUNICATION");

      return -1;
    }
  }
  else {
    if(line) {
      snprintf(ErrorString,50,"E_FO_%d",atoi(line));
      free(line);
    }
    else strcpy(ErrorString,"E_COMMUNICATION");

    return -1;
  }

  return 0;
}
/* }}} */

/* {{{ cf_get_message_through_shm */
#ifdef CF_SHARED_MEM
int cf_get_message_through_shm(int shmids[3],void *shm_ptr,cf_cl_thread_t *thr,u_int64_t tid,u_int64_t mid,int del) {
  struct shmid_ds shm_buf;
  register void *ptr1;
  u_int64_t val = 0;
  size_t posts,post,x,flagnum;
  cf_message_t *msg;

  if(shmctl(shm_id,IPC_STAT,&shm_buf) != 0) {
    strcpy(ErrorString,"E_CONFIG_ERR");
    return -1;
  }

  /*
   *
   * CAUTION! Deep magic begins here! Do not edit if you don't know
   * what you are doing
   *
   */

  for(ptr1=shm_ptr+sizeof(time_t);ptr1 < shm_ptr+shm_buf.shm_segsz;) {
    /* first: tid */
    val   = *((u_int64_t *)ptr1);

    if(val == tid) break;

    ptr1 += sizeof(u_int64_t);

    /* after that: number of postings */
    posts = *((int32_t *)ptr1);
    ptr1 += sizeof(int32_t);

    for(post = 0;post < posts;post++) {
      /* then: message id */
      ptr1 += sizeof(u_int64_t);

      /* number of flags */
      flagnum = *((u_int32_t *)ptr1);
      ptr1 += sizeof(u_int32_t);

      for(x=0;x<flagnum;++x) {
        /* size of name */
        val = *((u_int32_t *)ptr1);
        ptr1 += sizeof(u_int32_t) + val;

        val = *((u_int32_t *)ptr1);
        ptr1 += sizeof(u_int32_t) + val;
      }

      /* then: length of the subject + subject */
      val = *((u_int32_t *)ptr1);
      ptr1 += sizeof(u_int32_t) + val;

      /* length of the category + category */
      val = *((u_int32_t *)ptr1);
      ptr1 += sizeof(u_int32_t);
      if(val) ptr1 += val;

      /* remote address */
      val = *((u_int32_t *)ptr1);
      ptr1 += sizeof(u_int32_t) + val;

      /* length of the content + content */
      val = *((u_int32_t *)ptr1);
      ptr1 += sizeof(u_int32_t) + val;

      /* date, level, invisible, votes good, votes bad */
      ptr1 += sizeof(time_t) + sizeof(u_int16_t) + sizeof(u_int16_t) + sizeof(u_int32_t) + sizeof(u_int32_t);

      /* user name length + user name */
      val = *((u_int32_t *)ptr1);
      ptr1 += sizeof(u_int32_t) + val;

      /* email length + email */
      val = *((u_int32_t *)ptr1);
      ptr1 += sizeof(u_int32_t);
      if(val) ptr1 += val;

      /* homepage length + homepage */
      val = *((u_int32_t *)ptr1);
      ptr1 += sizeof(u_int32_t);
      if(val) ptr1 += val;

      /* image length + image */
      val = *((u_int32_t *)ptr1);
      ptr1 += sizeof(u_int32_t);
      if(val) ptr1 += val;
    }
  }

  /*
   *
   * Phew, deep magic ended
   *
   */

  if(ptr1 >= shm_ptr + shm_buf.shm_segsz) {
    strcpy(ErrorString,"E_FO_404");
    return -1;
  }

  if((ptr1 = cf_get_next_thread_through_shm(shmids,ptr1,thr)) == NULL) return -1;

  if(mid == 0) thr->threadmsg = thr->messages;
  else {
    for(msg=thr->messages;msg;msg=msg->next) {
      if(msg->mid == mid) {
        thr->threadmsg = msg;
        break;
      }
    }
  }

  if(!thr->threadmsg) {
    strcpy(ErrorString,"E_FO_404");
    return -1;
  }

  if((thr->messages->invisible == 1 || thr->threadmsg->invisible == 1) && del == CF_KILL_DELETED) {
    strcpy(ErrorString,"E_FO_404");
    return -1;
  }

  return 0;
}
#endif
/* }}} */

/* eof */
