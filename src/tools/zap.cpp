/* zap.cpp -- NEW FILE (2026-10-12), Stellar Frontier Godot port.
   Rebuilds the "zap" bitmap archives that the original build appended to
   frontier.exe, server.exe and spy.exe (build.bat: zap + zapexe), using
   slib's own S_ZArchive code so the format is exactly the original one.
   The port has no real .exe, so the archive is appended to a small stub;
   S_ZArchive::Open() only needs the trailing exe-size field.

   usage: zap OUTFILE FILE...   (FILE names are stored as given; run it in
                                 the directory that holds them) */
#include "S_OS.h"
#include "S_ZArchive.h"

int main(int argc, char *argv[])
{
  if (argc < 3) { fprintf(stderr, "usage: zap OUTFILE FILE...\n"); return 2; }
  static const char stub[] = "Stellar Frontier port: zap archive (see src/tools/zap.cpp)\n";
  S_ZArchive arc;
  for (int i = 2; i < argc; i++) {
    S_ZFile *f = new S_ZFile();
    if (f->Read(argv[i]) != S_ZFILE_OK) { fprintf(stderr, "cannot read %s\n", argv[i]); return 1; }
    f->Compress();
    arc.AddChild(f);
  }
  arc.exeSize = sizeof(stub) - 1;
  char tmp[1024];
  sprintf(tmp, "%s.tmp", argv[1]);
  if (arc.Write(tmp) != S_ZFILE_OK) { fprintf(stderr, "write failed\n"); return 1; }
  FILE *in = fopen(tmp, "rb");
  FILE *out = fopen(argv[1], "wb");
  if (!in || !out) return 1;
  fwrite(stub, 1, sizeof(stub) - 1, out);
  char buf[65536];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), in)) > 0) fwrite(buf, 1, n, out);
  fclose(in);
  fclose(out);
  remove(tmp);
  return 0;
}
