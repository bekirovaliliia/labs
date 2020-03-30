#include "lwip/opt.h"
#include "lwip/def.h"
#include "fs.h"
#include "fsdata.h"
#include <string.h>

#ifndef HTTPD_USE_CUSTUM_FSDATA
#define HTTPD_USE_CUSTUM_FSDATA 0
#endif

#if HTTPD_USE_CUSTUM_FSDATA
#include "fsdata_custom.c"
#else 
#include "fsdata.c"
#endif 

#ifndef LWIP_MAX_OPEN_FILES
#define LWIP_MAX_OPEN_FILES     10
#endif

struct fs_table {
  struct fs_file file;
  u8_t inuse;
};

struct fs_table fs_memory[LWIP_MAX_OPEN_FILES];

#if LWIP_HTTPD_CUSTOM_FILES
int fs_open_custom(struct fs_file *file, const char *name);
void fs_close_custom(struct fs_file *file);
#endif 

static struct fs_file *
fs_malloc(void)
{
  int i;
  for(i = 0; i < LWIP_MAX_OPEN_FILES; i++) {
    if(fs_memory[i].inuse == 0) {
      fs_memory[i].inuse = 1;
      return(&fs_memory[i].file);
    }
  }
  return(NULL);
}

static void
fs_free(struct fs_file *file)
{
  int i;
  for(i = 0; i < LWIP_MAX_OPEN_FILES; i++) {
    if(&fs_memory[i].file == file) {
      fs_memory[i].inuse = 0;
      break;
    }
  }
  return;
}

struct fs_file *
fs_open(const char *name)
{
  struct fs_file *file;
  const struct fsdata_file *f;

  file = fs_malloc();
  if(file == NULL) {
    return NULL;
  }

#if LWIP_HTTPD_CUSTOM_FILES
  if(fs_open_custom(file, name)) {
    file->is_custom_file = 1;
    return file;
  }
  file->is_custom_file = 0;
#endif 

  for(f = FS_ROOT; f != NULL; f = f->next) {
    if (!strcmp(name, (char *)f->name)) {
      file->data = (const char *)f->data;
      file->len = f->len;
      file->index = f->len;
      file->pextension = NULL;
      file->http_header_included = f->http_header_included;
#if HTTPD_PRECALCULATED_CHECKSUM
      file->chksum_count = f->chksum_count;
      file->chksum = f->chksum;
#endif 
#if LWIP_HTTPD_FILE_STATE
      file->state = fs_state_init(file, name);
#endif 
      return file;
    }
  }
  fs_free(file);
  return NULL;
}

void
fs_close(struct fs_file *file)
{
#if LWIP_HTTPD_CUSTOM_FILES
  if (file->is_custom_file) {
    fs_close_custom(file);
  }
#endif 
#if LWIP_HTTPD_FILE_STATE
  fs_state_free(file, file->state);
#endif 
  fs_free(file);
}

int
fs_read(struct fs_file *file, char *buffer, int count)
{
  int read;

  if(file->index == file->len) {
    return -1;
  }

  read = file->len - file->index;
  if(read > count) {
    read = count;
  }

  MEMCPY(buffer, (file->data + file->index), read);
  file->index += read;

  return(read);
}

int fs_bytes_left(struct fs_file *file)
{
  return file->len - file->index;
}
