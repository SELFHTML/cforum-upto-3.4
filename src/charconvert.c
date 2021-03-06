/**
 * \file charconvert.c
 * \author Christian Kruse, <cjk@wwwtech.de>
 * \brief character conversion functions
 *
 */

/* {{{ Initial headers */
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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <iconv.h>
#include <string.h>

#include <sys/types.h>

#include "entitytable.h"

#include "utils.h"
/* }}} */

/* {{{ utf8_to_unicode */
int utf8_to_unicode(const u_char *s,size_t n,u_int32_t *num) {
  u_char c = s[0];

  if(c < 0x80) {
    *num = c;
    return 1;
  }
  else if(c < 0xc2) return EILSEQ;
  else if(c < 0xe0) {
    if(n < 2) return EILSEQ;

    if(!((s[1] ^ 0x80) < 0x40)) return EILSEQ;
    *num = ((u_int32_t)(c & 0x1f) << 6) | (u_int32_t)(s[1] ^ 0x80);
    return 2;
  }
  else if(c < 0xf0) {
    if(n < 3) return EILSEQ;
    if(!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40 && (c >= 0xe1 || s[1] >= 0xa0))) return EILSEQ;

    *num = ((u_int32_t)(c & 0x0f) << 12) | ((u_int32_t)(s[1] ^ 0x80) << 6) | (u_int32_t)(s[2] ^ 0x80);
    return 3;
  }
  else if(c < 0xf8) {
    if(n < 4) return EILSEQ;
    if(!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40 && (s[3] ^ 0x80) < 0x40 && (c >= 0xf1 || s[1] >= 0x90))) return EILSEQ;

    *num = ((u_int32_t)(c & 0x07) << 18) | ((u_int32_t)(s[1] ^ 0x80) << 12) | ((u_int32_t)(s[2] ^ 0x80) << 6) | (u_int32_t)(s[3] ^ 0x80);
    return 4;
  }
  else if(c < 0xfc) {
    if(n < 5) return EILSEQ;
    if(!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40 && (s[3] ^ 0x80) < 0x40 && (s[4] ^ 0x80) < 0x40 && (c >= 0xf9 || s[1] >= 0x88))) return EILSEQ;

    *num = ((u_int32_t)(c & 0x03) << 24) | ((u_int32_t)(s[1] ^ 0x80) << 18) | ((u_int32_t)(s[2] ^ 0x80) << 12) | ((u_int32_t)(s[3] ^ 0x80) << 6) | (u_int32_t)(s[4] ^ 0x80);
    return 5;
  }
  else if(c < 0xfe) {
    if(n < 6) return EILSEQ;
    if(!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40 && (s[3] ^ 0x80) < 0x40 && (s[4] ^ 0x80) < 0x40 && (s[5] ^ 0x80) < 0x40 && (c >= 0xfd || s[1] >= 0x84))) return EILSEQ;

    *num = ((u_int32_t)(c & 0x01) << 30) | ((u_int32_t)(s[1] ^ 0x80) << 24) | ((u_int32_t)(s[2] ^ 0x80) << 18) | ((u_int32_t)(s[3] ^ 0x80) << 12) | ((u_int32_t)(s[4] ^ 0x80) << 6) | (u_int32_t)(s[5] ^ 0x80);
    return 6;
  }
  else return EILSEQ;

}
/* }}} */

/* {{{ unicode_to_utf8 */
int unicode_to_utf8(u_int32_t num, u_char *c,size_t n) {
  size_t count;

  if(num < 0x80)             count = 1;
  else if(num < 0x800)       count = 2;
  else if(num < 0x10000)     count = 3;
  else if(num < 0x200000)    count = 4;
  else if(num < 0x4000000)   count = 5;
  else if(num <= 0x7FFFFFFF) count = 6;
  else return EINVAL;

  if (n < count) return EINVAL;

  switch(count) {
    case 6:
      c[5] = 0x80 | (num & 0x3F);
      num = (num >> 6) | 0x4000000;

    case 5:
      c[4] = 0x80 | (num & 0x3F);
      num = (num >> 6) | 0x200000;

    case 4:
      c[3] = 0x80 | (num & 0x3F);
      num = (num >> 6) | 0x10000;

    case 3:
      c[2] = 0x80 | (num & 0x3F);
      num = (num >> 6) | 0x800;

    case 2:
      c[1] = 0x80 | (num & 0x3F);
      num = (num >> 6) | 0xC0;

    case 1:
      c[0] = num;
  }

  return count;
}
/* }}} */

/* {{{ is_valid_utf8_string */
int is_valid_utf8_string(const u_char *str,size_t len) {
  register u_char *ptr = (u_char *)str;
  int x;
  int ret;

  for(;*ptr && len > 0;) {
    if((ret = utf8_to_unicode(ptr,len,&x)) == EILSEQ) {
      return -1;
    }

    ptr += ret;
    len -= ret;
  }

  return 0;
}
/* }}} */

/* {{{ charset_convert
 * Returns: u_char * (NULL on failure, a string-array on success)
 * Parameters:
 *   - const u_char *toencode     the string which has to be encoded
 *   - size_t in_len              the length of the string to convert
 *   - const u_char *from_charset the starting charset
 *   - const u_char *to_charset   the target charset
 *   - size_t *out_len_p          pointer to a variable; will be filled with the length of the output
 *
 * this function tries to convert a string from one charset to another charset
 *
 */
u_char *charset_convert(const u_char *toencode,size_t in_len,const u_char *from_charset,const u_char *to_charset,size_t *out_len_p) {
  iconv_t cd;
  size_t in_left, out_size, out_left;
  u_char *out_p, *out_buf, *tmp_buf;
  size_t bsz, result = 0;

  cd = iconv_open(to_charset,from_charset);

  if(cd == (iconv_t)(-1)) {
    return NULL;
  }

  in_left  = in_len;
  out_left = in_len + 32; /* avoids realloc() in most cases */
  out_size = 0;
  bsz      = out_left;
  out_buf  = cf_alloc(NULL,bsz+1,1,CF_ALLOC_MALLOC);
  out_p    = out_buf;

  while(in_left > 0) {
    result = iconv(cd,(char **)&toencode,&in_left,(char **)&out_p,&out_left);
    out_size = bsz - out_left;
    if(result == (size_t)(-1)) {
      if(errno == E2BIG && in_left > 0) {
        /* converted string is longer than out buffer */
        bsz += in_len;

        /* tmp_buf cannot be NULL because if memory allocation failes, cf_alloc calls exit() */
        tmp_buf = (u_char *)cf_alloc(out_buf, bsz+1,1,CF_ALLOC_REALLOC);

        out_buf  = tmp_buf;
        out_p    = out_buf + out_size;
        out_left = bsz - out_size;
        continue;
      }
    }

    break;
  }

  iconv_close(cd);

  if(result == (size_t)(-1)) {
    free(out_buf);
    return NULL;
  }

  *out_p = '\0';
  if(out_len_p) *out_len_p = (size_t)(out_p - out_buf);
  return out_buf;
}
/* }}} */

/* {{{ get_named_entity */
u_int32_t get_named_entity(const u_char *entity) {
  u_int32_t low = 0;
  u_int32_t high = entity_table_length - 1;
  u_int32_t m;
  int c;

  while(low <= high) {
    m = (low + high) / 2;
    c = strcmp(entity,entity_table_entities[m]);

    if(c < 0) high = m - 1;
    else if(c > 0) low = m + 1;
    else {
      return entity_table_codes[m];
    }
  }

  return 0;
}
/* }}} */

/* {{{ htmlentities_decode */
u_char *htmlentities_decode(const u_char *string,size_t *slen) {
  u_char *ptr;
  cf_string_t new_str;
  u_int32_t num;

  u_char buff[8],*val,*safe;
  int len;

  cf_str_init(&new_str);
  memset(buff,0,sizeof(buff));

  if(!string) return NULL;

  for(ptr=(u_char *)string;*ptr;++ptr) {
    if(*ptr == '&') {
      /* decimal entitiy reference */
      if(*(ptr+1) == '#') {
        safe = ptr;

        if(*(ptr+2) == 'x' || *(ptr+2) == 'X') num = strtol(ptr+3,(char **)&ptr,16);
        else num = strtol(ptr+2,(char **)&ptr,10);

        if(num == 0) {
          cf_str_char_append(&new_str,*safe);
          ptr = safe;
          continue;
        }

        if((len = unicode_to_utf8(num,buff,7)) == EINVAL) {
          cf_str_char_append(&new_str,*safe);
          ptr = safe;
          continue;
        }

        cf_str_chars_append(&new_str,buff,len);
      }
      else {
        for(safe=ptr,val=ptr+1;*ptr && *ptr != ';';++ptr);
        val = strndup(val,ptr-val);
        num = get_named_entity(val);
        free(val);

        if(num == 0) {
          cf_str_char_append(&new_str,*safe);
          ptr = safe;
          continue;
        }

        if((len = unicode_to_utf8(num,buff,7)) == EINVAL) {
          cf_str_char_append(&new_str,*safe);
          ptr = safe;
          continue;
        }

        cf_str_chars_append(&new_str,buff,len);
      }
    }
    else cf_str_char_append(&new_str,*ptr);
  }

  if(slen) *slen = new_str.len;
  return cf_alloc(new_str.content,new_str.len+1,1,CF_ALLOC_REALLOC);
}
/* }}} */

/* {{{ htmlentities
 * Returns: u_char * (NULL on failure, a string-array on success)
 * Parameters:
 *   - const u_char *string  the string which has to be encoded
 *
 * this function converts HTML named characters to their entities
 *
 */
u_char *htmlentities(const u_char *string,int sq) {
  register u_char *ptr;
  cf_string_t new_str;

  cf_str_init(&new_str);

  if(!string) {
    return NULL;
  }

  for(ptr=(u_char *)string;*ptr;ptr++) {
    switch(*ptr) {
      case '>':
        cf_str_chars_append(&new_str,"&gt;",4);
        break;
      case '<':
        cf_str_chars_append(&new_str,"&lt;",4);
        break;
      case '&':
        cf_str_chars_append(&new_str,"&amp;",5);
        break;
      case '"':
        cf_str_chars_append(&new_str,"&quot;",6);
        break;
      case '\'':
        if(sq) {
          cf_str_chars_append(&new_str,"&#39;",5);
        }
        else {
          cf_str_char_append(&new_str,*ptr);
        }
        break;
      default:
        cf_str_char_append(&new_str,*ptr);
        break;
    }
  }

  return cf_alloc(new_str.content,new_str.len+1,1,CF_ALLOC_REALLOC);
}
/* }}} */

/* {{{ print_htmlentities_encoded */
size_t print_htmlentities_encoded(const u_char *string,int sq,FILE *handle) {
  register u_char *ptr;
  register size_t written = 0,s = 0;

  if(!string) return 0;

  for(ptr=(u_char *)string;*ptr;ptr++) {
    switch(*ptr) {
      case 34:
        s = fwrite("&quot;",1,6,handle);
        break;
      case 38:
        s = fwrite("&amp;",1,5,handle);
        break;
      case 39:
        if(sq) {
          s = fwrite("&#39;",1,5,handle);
        }
        else {
          s = fwrite(ptr,1,1,handle);
        }
        break;
      case 60:
        s = fwrite("&lt;",1,4,handle);
        break;
      case 62:
        s = fwrite("&gt;",1,4,handle);
        break;
      default:
        fputc(*ptr,handle);
        s = 1;
        break;
    }

    if(s <= 0) {
      return written;
    }

    written += s;
  }

  return written;
}
/* }}} */

/* {{{ htmlentities_charset_convert
 * This function converts a string between to charsets and encodes it
 * as html (this means, " to &quot;, < to &lt;, > to &gt; and & to &amp;). If a
 * sequence cannot be converted to the target charset, it will be converted to
 * a named entity (if given) or a unicode entity (&#<number>;)
 */
u_char *htmlentities_charset_convert(const u_char *toencode, const u_char *from, const u_char *to,size_t *outlen,int sq) {
  register u_char *ptr;
  u_char *in_ptr,*entity,buff[15];
  cf_string_t new_str;

  iconv_t cd;
  size_t in_left, out_size, out_left,in_len,elen;
  u_char *out_p, *out_buf, *tmp_buf;
  size_t bsz, result = 0;
  int unicode,ret;

  cd = iconv_open(to,from);

  if(cd == (iconv_t)(-1)) {
    return NULL;
  }

  /* first phase: encode html active characters */
  cf_str_init(&new_str);

  for(ptr=(u_char *)toencode;*ptr;ptr++) {
    switch(*ptr) {
      case '>':
        cf_str_chars_append(&new_str,"&gt;",4);
        break;
      case '<':
        cf_str_chars_append(&new_str,"&lt;",4);
        break;
      case '&':
        cf_str_chars_append(&new_str,"&amp;",5);
        break;
      case '"':
        cf_str_chars_append(&new_str,"&quot;",6);
        break;
      case '\'':
        if(sq) {
          cf_str_chars_append(&new_str,"&#39;",5);
        }
        else {
          cf_str_char_append(&new_str,*ptr);
        }
        break;
      default:
        cf_str_char_append(&new_str,*ptr);
        break;
    }
  }

  /* second phase: convert string to charset */
  in_len   = new_str.len;
  in_ptr   = new_str.content;
  in_left  = new_str.len;
  out_left = in_len + 32; /* avoids realloc() in most cases */
  out_size = 0;
  bsz      = out_left;
  out_buf  = cf_alloc(NULL,bsz+1,1,CF_ALLOC_MALLOC);
  out_p    = out_buf;

  while(in_left > 0) {
    result = iconv(cd,(char **)&in_ptr,&in_left,(char **)&out_p,&out_left);
    out_size = bsz - out_left;
    if(result == (size_t)(-1)) {
      if(errno == E2BIG && in_left > 0) {
        /* converted string is longer than out buffer */
        bsz += in_len;

        /* tmp_buf cannot be NULL because if memory allocation failes, cf_alloc calls exit() */
        tmp_buf = (u_char *)cf_alloc(out_buf, bsz+1,1,CF_ALLOC_REALLOC);

        out_buf  = tmp_buf;
        out_p    = out_buf + out_size;
        out_left = bsz - out_size;
        continue;
      }
      else if(errno == EILSEQ) {
        /* ok, we got an illegal sequence... lets convert it to an entity */
        if((ret = utf8_to_unicode(in_ptr,in_left,&unicode)) <= 0) {
          cf_str_cleanup(&new_str);
          free(out_buf);
          return NULL;
        }

        /* longest entity is about 19 bytes; we need more space if buffer is shorter */
        if(out_left < 20) {
          bsz += in_len;
          /* tmp_buf cannot be NULL because if memory allocation failes, cf_alloc calls exit() */
          tmp_buf = (u_char *)cf_alloc(out_buf, bsz+1,1,CF_ALLOC_REALLOC);

          out_buf  = tmp_buf;
          out_p    = out_buf + out_size;
          out_left = bsz - out_size;
        }

        /* get named enity (if available) */
        if((entity = (u_char *)entity_lookup(unicode)) == NULL) {
          elen = snprintf(buff,15,"&#%d;",unicode);
        }
        else {
          elen = snprintf(buff,15,"&%s;",entity);
        }

        /* copy entity to buffer */
        strncpy(out_p,buff,elen);

        /* go to next sequence */
        in_left -= ret;
        in_ptr += ret;

        /* go to free space */
        out_p    += elen;
        out_left -= elen;

        if(in_left <= 0) {
          result = 0;
          break;
        }

        continue;
      }
    }

    break;
  }

  iconv_close(cd);
  cf_str_cleanup(&new_str);

  if(result == (size_t)(-1)) {
    free(out_buf);
    return NULL;
  }

  *out_p = '\0';
  if(outlen) *outlen = (size_t)(out_p - out_buf);
  return out_buf;
}
/* }}} */

/* {{{ charset_convert_entities
 * This function converts a string between to charsets; every entity which cannot be shown
 * in the corresponding charset will be converted to HTML entities (named or UTF8-reference)
 */
u_char *charset_convert_entities(const u_char *toencode, size_t in_len,const u_char *from, const u_char *to,size_t *outlen) {
  u_char *in_ptr,*entity,buff[15];

  iconv_t cd;
  size_t in_left, out_size, out_left,elen;
  u_char *out_p, *out_buf, *tmp_buf;
  size_t bsz, result = 0;
  int unicode,ret;

  cd = iconv_open(to,from);

  if(cd == (iconv_t)(-1)) {
    return NULL;
  }

  /* second phase: convert string to charset */
  in_ptr   = (u_char *)toencode;
  in_left  = in_len;
  out_left = in_len + 32; /* avoids realloc() in most cases */
  out_size = 0;
  bsz      = out_left;
  out_buf  = cf_alloc(NULL,bsz+1,1,CF_ALLOC_MALLOC);
  out_p    = out_buf;

  while(in_left > 0) {
    result = iconv(cd,(char **)&in_ptr,&in_left,(char **)&out_p,&out_left);
    out_size = bsz - out_left;
    if(result == (size_t)(-1)) {
      if(errno == E2BIG && in_left > 0) {
        /* converted string is longer than out buffer */
        bsz += in_len;

        /* tmp_buf cannot be NULL because if memory allocation failes, cf_alloc calls exit() */
        tmp_buf = (u_char *)cf_alloc(out_buf, bsz+1,1,CF_ALLOC_REALLOC);

        out_buf  = tmp_buf;
        out_p    = out_buf + out_size;
        out_left = bsz - out_size;
        continue;
      }
      else if(errno == EILSEQ) {
        /* ok, we got an illegal sequence... lets convert it to an entity */
        if((ret = utf8_to_unicode(in_ptr,in_left,&unicode)) <= 0) {
          free(out_buf);
          return NULL;
        }

        /* longest entity is about 19 bytes; we need more space if buffer is shorter */
        if(out_left < 20) {
          bsz += in_len;
          /* tmp_buf cannot be NULL because if memory allocation failes, cf_alloc calls exit() */
          tmp_buf = (u_char *)cf_alloc(out_buf, bsz+1,1,CF_ALLOC_REALLOC);

          out_buf  = tmp_buf;
          out_p    = out_buf + out_size;
          out_left = bsz - out_size;
        }

        /* get named enity (if available) */
        if((entity = (u_char *)entity_lookup(unicode)) == NULL) {
          elen = snprintf(buff,15,"&#%d;",unicode);
        }
        else {
          elen = snprintf(buff,15,"&%s;",entity);
        }

        /* copy entity to buffer */
        strncpy(out_p,buff,elen);

        /* go to next sequence */
        in_left -= ret;
        in_ptr += ret;

        /* go to free space */
        out_p    += elen;
        out_left -= elen;

        if(in_left <= 0) {
          result = 0;
          break;
        }

        continue;
      }
    }

    break;
  }

  iconv_close(cd);

  if(result == (size_t)(-1)) {
    free(out_buf);
    return NULL;
  }

  *out_p = '\0';
  if(outlen) *outlen = (size_t)(out_p - out_buf);
  return out_buf;
}
/* }}} */

/* eof */
