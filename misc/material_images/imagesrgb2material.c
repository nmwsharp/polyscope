#include <stdio.h>
#include <stdlib.h>

FILE* open_or_exit(const char* fname, const char* mode) {
  FILE* f = fopen(fname, mode);
  if (f == NULL) {
    perror(fname);
    exit(EXIT_FAILURE);
  }
  return f;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "USAGE: %s {rsrc}\n\n"
                    "  Creates bindata_{sym}.cpp from the contents of {rsrc}_r.jpg (etc)\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  const char* sym = argv[1];

  char symfile[256];
  snprintf(symfile, sizeof(symfile), "bindata_%s.cpp", sym);

  FILE* out = open_or_exit(symfile, "w");
  fprintf(out, "#include <vector>\n");

  fprintf(out, "#include \"polyscope/gl/materials/materials.h\"\n");
  fprintf(out, "namespace polyscope { \n");
  fprintf(out, "namespace gl { \n");
  fprintf(out, "const std::vector<std::vector<unsigned char>> bindata_%s = {\n", sym);


  for (int iComp = 0; iComp < 3; iComp++) {

    char inFilename[256];
    if (iComp == 0) snprintf(inFilename, sizeof(inFilename), "%s_r.jpg", sym);
    if (iComp == 1) snprintf(inFilename, sizeof(inFilename), "%s_g.jpg", sym);
    if (iComp == 2) snprintf(inFilename, sizeof(inFilename), "%s_b.jpg", sym);


    fprintf(out, "{\n");

    FILE* in = open_or_exit(inFilename, "r");
    unsigned char buf[256];
    size_t nread = 0;
    size_t linecount = 0;
    do {
      nread = fread(buf, 1, sizeof(buf), in);
      size_t i;
      for (i = 0; i < nread; i++) {
        fprintf(out, "0x%02x, ", buf[i]);
        if (++linecount == 10) {
          fprintf(out, "\n");
          linecount = 0;
        }
      }
    } while (nread > 0);
    if (linecount > 0) fprintf(out, "\n");
    fprintf(out, "},\n");

    fclose(in);
  }

  fprintf(out, "};\n");
  fprintf(out, "}}");
  fclose(out);

  return EXIT_SUCCESS;
}
