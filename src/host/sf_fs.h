/* sf_fs.h -- portable file-system helpers for the SFHOST build.
   Added 2026-10-12 for the Stellar Frontier Godot port (new file). */
#ifndef SF_FS_H
#define SF_FS_H
#ifdef __cplusplus
extern "C" {
#endif
#define SF_FS_FILE 1
#define SF_FS_DIR  2
/* Expand a wildcard path (e.g. dir + "/" + "*.scn") like Win32 FindFirstFile: '*' and
   '?' in the last component only, case-insensitive.  Results are full
   paths ("dir/name"), sorted case-insensitively.  Returns the count; free
   with sf_fs_glob_free. */
int  sf_fs_glob(const char *pattern, int kinds, char ***out);
void sf_fs_glob_free(char **list, int count);
/* Case-insensitive fopen fallback for data files whose case differs from
   what the 1997 code asks for (Windows file names were case-insensitive). */
const char *sf_fs_resolve_case(const char *path, char *buf, int buflen);
#ifdef __cplusplus
}
#endif
#endif
