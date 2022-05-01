#define JANPATCH_STREAM  struct memfile

#include <stdio.h>
#include "janpatch.h"

#define MAX_MEM_SIZE (16 * 1024 * 1024)

struct memfile {
  char *mem;
  off_t pos;
  size_t size;
};

char old_mem[MAX_MEM_SIZE];
struct memfile old_mf = { old_mem };

char patch_mem[MAX_MEM_SIZE];
struct memfile patch_mf = { patch_mem };

char target_mem[MAX_MEM_SIZE];
struct memfile target_mf = { target_mem };

size_t mf_write(const void *ptr, size_t size, size_t nmemb, JANPATCH_STREAM *stream)
{
  size_t i, j;
  const char *cptr = ptr;

  for (j = 0; j < nmemb; j++) {
    for (i = 0; i < size; i++) {
      stream->mem[stream->pos++] = *cptr;
      cptr++;
    }
  }

  if (stream->pos > stream->size)
       stream->size = stream->pos;

  return nmemb;
}

size_t mf_read(void *ptr, size_t size, size_t nmemb, JANPATCH_STREAM *stream)
{
  size_t i, j;
  char *cptr = ptr;

  for (j = 0; j < nmemb; j++) {
      for (i = 0; i < size; i++) {
        if (stream->pos == stream->size)
          return j;
        *cptr = stream->mem[stream->pos++];
        cptr++;
      }
  }

  return nmemb;
}

int mf_seek(JANPATCH_STREAM *stream, long offset, int whence)
{
  if (whence == SEEK_SET) {
    stream->pos = offset;
  } else if (whence == SEEK_CUR) {
    stream->pos += offset;
  } else if (whence == SEEK_END) {
    printf("not supported\n");
    return -1;
  }
  return 0;
}

long mf_tell(JANPATCH_STREAM *stream)
{
  return stream->pos;
}

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        printf("Usage: janpatch-cli [old-file] [patch-file] ([new-file])\n");
        return 1;
    }

    // Open streams
    FILE *old = fopen(argv[1], "rb");
    if (!old) {
        printf("Could not open '%s'\n", argv[1]);
        return 1;
    }
    old_mf.size = fread(old_mf.mem, 1, MAX_MEM_SIZE, old);
    if (ferror(old)) {
        printf("error reading from old file\n");
        return 1; 
    }
            
    FILE *patch = fopen(argv[2], "rb");
    if (!patch) {
        printf("Could not open '%s'\n", argv[2]);
        return 1;
    }
    patch_mf.size = fread(patch_mf.mem, 1, MAX_MEM_SIZE, patch);
    if (ferror(patch)) {
        printf("error reading from patch file\n");
        return 1;
    }
    
    // janpatch_ctx contains buffers, and references to the file system functions
    janpatch_ctx ctx = {
        { (unsigned char*)malloc(1024), 1024 }, // source buffer
        { (unsigned char*)malloc(1024), 1024 }, // patch buffer
        { (unsigned char*)malloc(1024), 1024 }, // target buffer

        &mf_read,
        &mf_write,
        &mf_seek,
        &mf_tell
    };

    int ret = janpatch(ctx, &old_mf, &patch_mf, &target_mf);

    FILE *target = argc == 4 ? fopen(argv[3], "wb") : stdout;
    if (!target) {
        printf("Could not open '%s'\n", argv[3]);
        return 1;
    }

    fwrite(target_mf.mem, target_mf.size, 1, target);
    if (ferror(target)) {
        printf("error writing to target file\n");
        return 1;
    }

    return ret;
}
