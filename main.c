#include <stdlib.h>
#include <stdio.h>

#include "psf.h"


void usage(const char *argv[]) {
  printf("Usage: %s file\n", argv[0]);
}


void print_psf(const struct psf_file *f) {
  int i;

  // print header information
  printf("PSF information (for '%s'):\n", f->fn);
  printf("\n");
  printf("Version: %s (0x%02x)\n", psf_version_string(f->header.version), f->header.version);
  printf("Data Size: Reserved area: %9u Byte\n"
         "           Compressed:    %9u Byte\n"
         "           Uncompressed:  %9u Byte\n",
         f->header.reserved_size, f->header.compressed_size, f->data_size);
  printf("Number of tags: %d\n", f->num_tags);
  printf("Number of libraries: %d\n", f->num_libs);
  printf("Hierarchy index: %d\n", f->lib_index);
  printf("\n");

  // print tags
  if (f->num_tags) {
    printf("Tags:\n");

    for (i = 0; i < f->num_tags; i++) {
      struct psf_tag *tag = f->tags[i];
      printf("  %8s = %s\n", tag->key, tag->value);
    }
    printf("\n");
  }

  // print libs
  if (f->num_libs) {
    printf("Libraries:\n\n");

    for (i = 0; i < f->num_libs; i++) {
      print_psf(f->libs[i]);
    }
    printf("\n");
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

  if (psf_read_libs(psf) < 0) {
    printf("Error reading PSF libs\n");
    ret = -4;
    goto end;
  }

  print_psf(psf);

end:
  psf_close(psf);
  free(psf);
  return ret;
}

