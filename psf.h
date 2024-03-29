#ifndef _PSF_H
#define _PSF_H

#include <stdio.h>
#include <stdint.h>


struct psf_tag {
  char *key;
  char *value;
};


#pragma pack(1)
struct psf_file_header {
  uint8_t version;
  uint32_t reserved_size;
  uint32_t compressed_size;
  uint32_t compressed_crc;
};


struct psf_file {
  char *fn;
  FILE *fd;
  // header fields from file
  struct psf_file_header header;
  // reserved data area and uncompressed contents
  void *reserved_data;
  size_t data_size;
  void *data;
  // parsed tags
  int num_tags;
  struct psf_tag **tags;
  // loaded libs
  int num_libs;
  struct psf_file **libs;
  int lib_index;
};


// file opening
extern struct psf_file *psf_open_alloc(const char *path);
extern int psf_open(const char *path, struct psf_file *psf);

// closes openend PSF file and frees allocated resources
extern void psf_close(struct psf_file *);

// read PSF header, data and tags
extern int psf_read(struct psf_file *);

// read libs referenced in the PSF (mainly for mini-PSF)
extern int psf_read_libs(struct psf_file *);


// returns string description of PSF version byte
extern const char *psf_version_string(uint8_t version);


#endif
