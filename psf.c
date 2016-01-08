#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "psf.h"


struct psf_file *psf_open_alloc(const char *path) {
  struct psf_file *psf = malloc(sizeof (struct psf_file));

  if (!psf) {
    // error allocating memory
    return NULL;
  }

  memset(psf, 0, sizeof (struct psf_file));
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

  psf->fd = fopen(path, "r");
  if (!psf->fd) {
    // error opening file
    return -2;
  }

  return 0;
}


void psf_close(struct psf_file *psf) {
  if (!psf) {
    return;
  }

  if (psf->fd) {
    fclose(psf->fd);
  }

  if (psf->reserved_data) {
    free(psf->reserved_data);
  };

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

  memset(psf, 0, sizeof (struct psf_file));
};


int psf_read(struct psf_file *psf) {
  char tag_buffer[1024];
  char magic_buffer[5];
  char *buffer;

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
  if (fread(&psf->version, 1, 13, psf->fd) < 13) {
    return -4;
  }

  // read reserved area data
  psf->reserved_data = NULL;
  if (psf->reserved_size > 0) {
    psf->reserved_data = malloc(psf->reserved_size);
    if (!psf->reserved_data) {
      return -7;
    }
    if (fread(psf->reserved_data, 1, psf->reserved_size, psf->fd) < psf->reserved_size) {
      return -8;
    }
  }

  // read compressed data
  if (psf->compressed_size > 0) {
    buffer = malloc(psf->compressed_size);
    if (!buffer) {
      return -5;
    }
    if (fread(buffer, 1, psf->compressed_size, psf->fd) < psf->compressed_size) {
      free(buffer);
      return -6;
    }

    // TODO: Check CRC
    free(buffer);
  }

  psf->num_tags = 0;
  psf->tag_size = 0;
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
  while (fgets(tag_buffer, sizeof(tag_buffer), psf->fd)) {
    char *separator, *end;
    char *key, *value;
    struct psf_tag *tag;
    struct psf_tag **tags;

    // find and remove line end
    end = tag_buffer;
    while (*end != '\0' && *end != '\n')
      end++;
    *end = '\0';

    // find separator
    separator = strchr(tag_buffer, '=');
    if (!separator) {
      // invalid line
      continue;
    }
    *separator++ = 0;

    // TODO: Remove all whitespace (0x01 - 0x20)
    // TODO: Check if key is valid (valid C identifier, <0x7F)
    //printf("Reading tag: '%s' = '%s'\n", tag_buffer, separator);

    // create a copy of key and value
    key = strdup(tag_buffer);
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

