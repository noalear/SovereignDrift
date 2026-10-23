/* sf_fs.cpp -- see sf_fs.h.  Added 2026-10-12 for the Godot port. */
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include "sf_fs.h"

namespace fs = std::filesystem;

static bool wild_match(const char *p, const char *s) {
  /* Win32-style: '*' any run, '?' one char, case-insensitive */
  const char *star = nullptr, *ss = nullptr;
  while (*s) {
    if (*p == '?' || std::tolower((unsigned char)*p) == std::tolower((unsigned char)*s)) { p++; s++; continue; }
    if (*p == '*') { star = p++; ss = s; continue; }
    if (star) { p = star + 1; s = ++ss; continue; }
    return false;
  }
  while (*p == '*') p++;
  /* Win32 quirk: "*.*" matches names without a dot too */
  if (*p == '.' && p[1] == '*' && p[2] == 0) return true;
  return *p == 0;
}

static std::string lower(std::string s) {
  for (auto &c : s) c = (char)std::tolower((unsigned char)c);
  return s;
}

extern "C" int sf_fs_glob(const char *pattern, int kinds, char ***out) {
  *out = nullptr;
  if (!pattern) return 0;
  std::string pat(pattern);
  for (auto &c : pat) if (c == '\\') c = '/';
  std::string dir, mask;
  size_t slash = pat.find_last_of('/');
  if (slash == std::string::npos) { dir = ""; mask = pat; }
  else { dir = pat.substr(0, slash + 1); mask = pat.substr(slash + 1); }
  std::vector<std::string> names;
  std::error_code ec;
  fs::path root = dir.empty() ? fs::path(".") : fs::path(dir);
  for (fs::directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
    std::string name = it->path().filename().string();
    if (!wild_match(mask.c_str(), name.c_str())) continue;
    std::error_code ec2;
    bool isdir = it->is_directory(ec2);
    if ((isdir && (kinds & SF_FS_DIR)) || (!isdir && (kinds & SF_FS_FILE)))
      names.push_back(name);
  }
  std::sort(names.begin(), names.end(),
            [](const std::string &a, const std::string &b) { return lower(a) < lower(b); });
  if (names.empty()) return 0;
  char **list = (char **)std::malloc(sizeof(char *) * names.size());
  for (size_t i = 0; i < names.size(); i++) {
    std::string full = dir + names[i];
    list[i] = (char *)std::malloc(full.size() + 1);
    std::memcpy(list[i], full.c_str(), full.size() + 1);
  }
  *out = list;
  return (int)names.size();
}

extern "C" void sf_fs_glob_free(char **list, int count) {
  if (!list) return;
  for (int i = 0; i < count; i++) std::free(list[i]);
  std::free(list);
}

extern "C" const char *sf_fs_resolve_case(const char *path, char *buf, int buflen) {
  if (!path || !buf || buflen <= 0) return path;
  std::string p(path);
  for (auto &c : p) if (c == '\\') c = '/';
  std::error_code ec;
  if (fs::exists(p, ec)) { std::snprintf(buf, buflen, "%s", p.c_str()); return buf; }
  /* walk components, matching case-insensitively */
  fs::path in(p), cur = in.is_absolute() ? in.root_path() : fs::path(".");
  bool first = true;
  for (auto &comp : in.relative_path()) {
    std::string want = lower(comp.string());
    fs::path next = cur / comp;
    if (!fs::exists(next, ec)) {
      bool found = false;
      for (fs::directory_iterator it(cur, ec), end; !ec && it != end; it.increment(ec)) {
        if (lower(it->path().filename().string()) == want) { next = it->path(); found = true; break; }
      }
      if (!found) { std::snprintf(buf, buflen, "%s", p.c_str()); return buf; }
    }
    cur = next; first = false;
  }
  (void)first;
  std::string r = cur.generic_string();
  if (r.rfind("./", 0) == 0 && !(p.rfind("./", 0) == 0)) r = r.substr(2);
  std::snprintf(buf, buflen, "%s", r.c_str());
  return buf;
}
