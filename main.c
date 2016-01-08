#include <stdlib.h>
#include <stdio.h>

#include "psf.h"


void usage(const char *argv[]) {
  printf("Usage: %s file\n", argv[0]);
}


void print_psf(const char *fn, const struct psf_file *f) {
  int i;

  printf("PSF information (for '%s'):\n", fn);
  printf("\n");
  printf("Version: %s (0x%02x)\n", psf_version_string(f->version), f->version);
  printf("Data Size: Reserved area: %4u Byte\n"
         "           Compressed:    %4u Byte\n"
         "           Uncompressed:  %4u Byte\n", f->reserved_size, f->compressed_size, f->data_size);
  printf("Number of tags: %d\n", f->num_tags);
  printf("\n");

  // print tags
  if (f->num_tags) {
    printf("Tags:\n");

    for (i = 0; i < f->num_tags; i++) {
      struct psf_tag *tag = f->tags[i];
      printf("  %8s = %s\n", tag->key, tag->value);
    }
  }
}


int main(int argc, const char *argv[]) {
  int ret = 0;

  const char *fn;
  struct psf_file *psf;

  if (argc < 2) {
    usage(argv);
    return -1;
  }

  fn = argv[1];

  psf = psf_open_alloc(fn);
  if (!psf) {
    printf("Error opening file '%s'\n", fn);
    return -2;
  }

  if (psf_read(psf) < 0) {
    printf("Error reading PSF contents\n");
    ret = -3;
    goto end;
  }

  print_psf(fn, psf);

end:
  psf_close(psf);
  free(psf);
  return ret;
}

