#include <stdio.h>

#include "psf.h"


void usage(const char *argv[]) {
  printf("Usage: %s file\n", argv[0]);
}


void print_psf(const char *fn, const struct psf_file *f) {
  printf("PSF information (for '%s'):\n", fn);
  printf("\n");
  printf("Version: %s (0x%02x)\n", psf_version_string(f->version), f->version);
}


int main(int argc, const char *argv[]) {
  const char *fn;
  struct psf_file psf;

  if (argc < 2) {
    usage(argv);
    return -1;
  }

  fn = argv[1];
  if (psf_open(fn, &psf) < 0) {
    printf("Error opening file '%s'\n", fn);
    return -2;
  }

  if (psf_read(&psf) < 0) {
    printf("Error reading PSF contents\n");
    return -3;
  }

  print_psf(fn, &psf);

  return 0;
}
