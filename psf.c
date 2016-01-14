#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libgen.h>
#include <zlib.h>

#include "psf.h"


struct psf_file *psf_open_alloc(const char *path) {
  struct psf_file *psf = malloc(sizeof (struct psf_file));

  if (!psf) {
    // error allocating memory
    return NULL;
  }

  if (psf_open(path, psf) < 0) {
    // error opening psf file
    free(psf);
    return NULL;
  }

  return psf;
}


int psf_open(const char *path, struct psf_file *psf) {
  if (!psf) {
    return -1;
  }

  memset(psf, 0, sizeof (struct psf_file));

  // store file name
  psf->fn = strdup(path);
  if (!psf->fn) {
    return -2;
  }

  // open file
  psf->fd = fopen(path, "r");
  if (!psf->fd) {
    // error opening file
    free(psf->fn);
    return -3;
  }

  return 0;
}


void psf_close(struct psf_file *psf) {
  int i;

  if (!psf) {
    return;
  }

  if (psf->fn) {
    free(psf->fn);
  }
  if (psf->fd) {
    fclose(psf->fd);
  }

  if (psf->reserved_data) {
    free(psf->reserved_data);
  };
  if (psf->data) {
    free(psf->data);
  }

  // free tags
  if (psf->tags) {
    int i;
    for (i = 0; i < psf->num_tags; i++) {
      struct psf_tag *tag = psf->tags[i];
      if (!tag)
        continue;

      if (tag->key)
        free(tag->key);
      if (tag->value)
        free(tag->value);

      free(tag);
    }

    free(psf->tags);
  }

  // close and free libs
  if (psf->libs) {
    for (i = 0; i < psf->num_libs; i++) {
      psf_close(psf->libs[i]);
    }

    free(psf->libs);
  }

  memset(psf, 0, sizeof (struct psf_file));
};


int psf_read(struct psf_file *psf) {
  char tag_buffer[1024];
  char magic_buffer[5];
  unsigned char *buffer;
  void *data_buffer;
  uint32_t crc;
  int ret;
  struct psf_tag *tag;

  if (!psf) {
    return -1;
  }

  // read magic bytes
  if (fread(magic_buffer, 1, 3, psf->fd) < 3) {
    return -2;
  }
  if (magic_buffer[0] != 'P' || magic_buffer[1] != 'S' || magic_buffer[2] != 'F') {
    // magic bytes don't match
    return -3;
  }

  // read header
  // this assumes that the host machine uses little-endian byte ordering
  if (fread(&psf->header, 1, sizeof psf->header, psf->fd) < sizeof psf->header) {
    return -4;
  }

  // read reserved area data
  psf->reserved_data = NULL;
  if (psf->header.reserved_size > 0) {
    psf->reserved_data = malloc(psf->header.reserved_size);
    if (!psf->reserved_data) {
      return -7;
    }
    if (fread(psf->reserved_data, 1, psf->header.reserved_size, psf->fd) < psf->header.reserved_size) {
      return -8;
    }
  }

  psf->data_size = 0;
  psf->data = NULL;

  // read and uncompress data
  if (psf->header.compressed_size > 0) {
    buffer = malloc(psf->header.compressed_size);
    if (!buffer) {
      return -5;
    }
    if (fread(buffer, 1, psf->header.compressed_size, psf->fd) < psf->header.compressed_size) {
      free(buffer);
      return -6;
    }

    // check CRC of compressed data with zlib
    crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, buffer, psf->header.compressed_size);
    if (crc != psf->header.compressed_crc) {
      free(buffer);
      return -11;
    }

    // uncompress data
    do {
      // exponentially grow output buffer until it is large enough to hold data
      if (psf->data_size) {
        psf->data_size = psf->data_size * 2;
      } else {
        // initial size is twice the compressed size rounded down to 256 byte increments
        psf->data_size = (psf->header.compressed_size >> 8) << 9;
        if (!psf->data_size) {
          psf->data_size = 256;
        }
      }

      psf->data = malloc(psf->data_size);
      if (!psf->data) {
        free(buffer);
        return -12;
      }

      // uncompress data with zlib
      ret = uncompress(psf->data, &psf->data_size, buffer, psf->header.compressed_size);
    }  while (ret == Z_BUF_ERROR);

    // release input buffer and handle zlib errors
    free(buffer);
    if (ret != Z_OK) {
      psf->data_size = 0;
      free(psf->data);

      if (ret == Z_MEM_ERROR) {
        return -13;
      } else if (ret == Z_DATA_ERROR) {
        return -14;
      }
      return -15;
    }

    // shrink data buffer
    data_buffer = realloc(psf->data, psf->data_size);
    if (!data_buffer) {
      psf->data_size = 0;
      free(psf->data);
      return -16;
    }
    psf->data = data_buffer;
  }

  psf->num_tags = 0;
  psf->tags = NULL;

  // read and check tag magic
  if (fread(magic_buffer, 1, 5, psf->fd) < 5) {
    // no tags
    return 0;
  }
  if (magic_buffer[0] != '[' || magic_buffer[1] != 'T' || magic_buffer[2] != 'A'
      || magic_buffer[3] != 'G' || magic_buffer[4] != ']') {
    // wrong magic, no tags
    return 0;
  }

  // read and parse tags
  tag = NULL;
  while (fgets(tag_buffer, sizeof(tag_buffer), psf->fd)) {
    char *separator, *end;
    char *key_start, *key_end;
    char *key, *value;
    struct psf_tag **tags;

    // sanitze string and parse line
    end = key_start = key_end = tag_buffer;
    separator = NULL;
    while (*end != '\0' && *end != '\n') {
      if (key_start == end) {
        // advance key_start until first non-whitespace character is reached
        if (*end <= 0x20) {
          key_start++;
        }
        key_end = key_start + 1;
      } else if (key_end == end) {
        // advance key_end until separator or first whitespace character is reached
        if (*end > 0x20 && *end != '=') {
          key_end++;
        }
        if (*end >= 0x7F) {
          // key contains invalid character, abort parsing
          key_end = NULL;
          break;
        }
      }

      // find key/value separator
      if (*end == '=') {
        separator = end;
      }

      // replace control characters with spaces (\0 and \n are catched by the loop)
      if (*end < 0x20) {
        *end = 0x20;
      }
      end++;
    }
    *end = '\0';

    // check key_end
    if (!key_end) {
      // invalid line
      continue;
    }

    // check for separator and split string
    if (!separator) {
      // invalid line
      continue;
    }

    // split buffer into multiple strings
    *key_end = 0;
    *separator++ = 0;

    // check for and handle multi-line tag
    if (tag && strcmp(key_start, tag->key) == 0) {
      // grow value string to hold old value, newline character, current value and \0
      value = realloc(tag->value, strlen(tag->value) + strlen(separator) + 2);
      if (!value) {
        return -17;
      }
      tag->value = value;

      // concatenate strings
      strcat(value, "\n");
      strcat(value, separator);
      continue;
    }

    // create a copy of key and value
    key = strdup(key_start);
    if (!key) {
      return -7;
    }
    value = strdup(separator);
    if (!value) {
      free(key);
      return -8;
    }

    // allocate new tag struct
    tag = malloc(sizeof (struct psf_tag));
    if (!tag) {
      free(key);
      free(value);
      return -9;
    }
    tag->key = key;
    tag->value = value;

    // grow tag array
    tags = realloc(psf->tags, (psf->num_tags + 1) * sizeof (struct psf_tag **));
    if (!tags) {
      free(key);
      free(value);
      return -10;
    }
    tags[psf->num_tags] = tag;
    psf->tags = tags;
    psf->num_tags++;
  }

  psf->num_libs = 0;
  psf->libs = NULL;

  return 0;
}


static int psf_read_lib(struct psf_file *psf, int index, const char *file) {
  char *fn;
  struct psf_file *f, **libs;

  // get path of parent file
  fn = malloc(strlen(psf->fn) + strlen(file) + 2);
  if (!fn) {
    return -4;
  }
  strcpy(fn, psf->fn);
  if (dirname(fn) != fn) {
    // filename of parent PSF must be invalid
    free(fn);
    return -5;
  }

  // append lib name
  strcat(fn, "/");
  strcat(fn, file);

  // try to open and parse file
  f = psf_open_alloc(fn);
  free(fn);
  if (f == NULL) {
    return -1;
  }
  f->lib_index = index;
  if (psf_read(f) < 0) {
    psf_close(f);
    return -2;
  }

  // grow and append to libs array of parent file
  libs = realloc(psf->libs, (psf->num_libs + 1) * sizeof (struct psf_file **));
  if (!libs) {
    psf_close(f);
    return -3;
  }
  libs[psf->num_libs] = f;
  psf->libs = libs;
  psf->num_libs++;
  return 0;
}


int psf_read_libs(struct psf_file *psf) {
  int i;
  unsigned int index;
  struct psf_tag *tag;

  if (!psf)
    return -1;
  if (!psf->tags)
    return -2;

  // find tags that reference libraries
  for (i = 0; i < psf->num_tags; i++) {
    tag = psf->tags[i];
    if (!tag)
      return -3;

    if (strcmp(tag->key, "_lib") == 0) {
      if (psf_read_lib(psf, 1, tag->value) < 0)
        return -4;
    } else if (sscanf(tag->key, "_lib%d", &index) == 1) {
      if (psf_read_lib(psf, index, tag->value) < 0)
        return -5;
    }
  }

  return 0;
}


const char *psf_version_string(uint8_t version) {
  switch (version) {
    case 0x01:  return "Playstation (PSF1)";
    case 0x02:  return "Playstation 2 (PSF2)";
    case 0x11:  return "Saturn (SSF)";
    case 0x12:  return "Dreamcast (DSF)";
    case 0x13:  return "Sega Genesis";
    case 0x21:  return "Nintendo 64 (USF)";
    case 0x22:  return "GameBoy Advance (GSF)";
    case 0x23:  return "Super NES (SNSF)";
    case 0x41:  return "Capcom QSound (QSF)";
  }
  return NULL;
}

