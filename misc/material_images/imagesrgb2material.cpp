#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>

// this is a weird mix of C and C++, sorry world

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
    fprintf(stderr,
            "USAGE: %s {sym}\n\n"
            "  Creates bindata_{sym}.cpp from the contents of {sym}_r.jpg (etc)\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  const char* sym = argv[1];

  char symfile[256];
  snprintf(symfile, sizeof(symfile), "bindata_%s.cpp", sym);

  FILE* out = open_or_exit(symfile, "w");
  fprintf(out, "#include <array>\n");

  fprintf(out, "#include \"polyscope/render/material_defs.h\"\n");
  fprintf(out, "namespace polyscope { \n");
  fprintf(out, "namespace render { \n\n");
  
  fprintf(out, "// clang-format off \n");


  for (int iComp = 0; iComp < 4; iComp++) {

    std::string postfix;
    if (iComp == 0) postfix = "_r";
    if (iComp == 1) postfix = "_g";
    if (iComp == 2) postfix = "_b";
    if (iComp == 3) postfix = "_k";


    char inFilename[256];
    snprintf(inFilename, sizeof(inFilename), "%s%s.hdr", sym, postfix.c_str());

    FILE* in = open_or_exit(inFilename, "r");
    unsigned char buf[256];
    std::vector<unsigned char> bytes;
    size_t nread = 0;
    do {
      nread = fread(buf, 1, sizeof(buf), in);
      size_t i;
      for (i = 0; i < nread; i++) {
        bytes.push_back(buf[i]);
      }
    } while (nread > 0);
    
    fprintf(out, "const std::array<unsigned char, %i> bindata_%s%s = {\n", (int)bytes.size(), sym, postfix.c_str());
    
    printf("extern const std::array<unsigned char, %i> bindata_%s%s;\n", (int)bytes.size(), sym, postfix.c_str());

    for (size_t iB = 0; iB < bytes.size(); iB++) {
      fprintf(out, "0x%02x, ", bytes[iB]);
      if (iB % 10 == 9) {
        fprintf(out, "\n");
      }
    }

    fprintf(out, "\n  };\n\n");
    fclose(in);
  }
  
  fprintf(out, "// clang-format on \n");
  
  fprintf(out, "}\n");
  fprintf(out, "}\n");

  fclose(out);

  return EXIT_SUCCESS;
}
