#ifndef _PSF_H
#define _PSF_H

#include <stdio.h>
#include <stdint.h>


struct psf_tag {
  const char *key;
  const char *value;
};

#pragma pack(1)
struct psf_file {
  FILE *fd;
  uint8_t version;
  uint32_t reserved_size;
  uint32_t compressed_size;
  uint32_t compressed_crc;
  struct psf_tag *tags[];
};


// file opening
extern struct psf_file *psf_open_alloc(const char *path);
extern int psf_open(const char *path, struct psf_file *psf);

// closes openend PSF file and frees allocated resources
extern void psf_free(struct psf_file *);

// read PSF header, data and tags
extern int psf_read(struct psf_file *);


// returns string description of PSF version byte
extern const char *psf_version_string(uint8_t version);


#endif
