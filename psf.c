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
};


int psf_read(struct psf_file *psf) {
  char magic[3];

  if (!psf) {
    return -1;
  }

  // read magic bytes
  if (fread(magic, 1, 3, psf->fd) < 3) {
    return -2;
  }
  if (magic[0] != 'P' || magic[1] != 'S' || magic[2] != 'F') {
    // magic bytes don't match
    return -3;
  }

  // read header
  // this assumes that both the host machine and the file format use little-endian byte ordering
  if (fread(&psf->version, 1, 13, psf->fd) < 13) {
    return -4;
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

