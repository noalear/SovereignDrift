/* ************************************************************************* */
/* w32_text.cpp -- GDI fonts and text output (CreateFont, TextOut,           */
/* GetTextExtentPoint32, GetTextMetrics, DrawText).                          */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/* Glyphs come from sf_fonts.inc (tools/gen_fonts.py, Liberation fonts).    */
/* ************************************************************************* */
#include "w32_internal.h"
#include <cstring>
#include <cstdlib>

using namespace w32;

namespace w32 {
#include "sf_fonts.inc"
}

namespace w32 {

static const SfFontData *find_font(const char *fam, int height, int width, int weight)
{
  bool bold = weight >= 600;
  const SfFontData *best = nullptr;
  int bestScore = 1 << 30;
  for (int i = 0; i < sf_num_fonts; i++) {
    const SfFontData &f = sf_fonts[i];
    if (strcmp(f.family, fam) != 0) continue;
    int score = 0;
    score += std::abs(f.req_height - height) * 100;
    score += (f.req_width == width) ? 0 : (f.req_width == 0 ? 10 : 50);
    score += ((f.weight >= 600) == bold) ? 0 : 5;
    if (score < bestScore) { bestScore = score; best = &f; }
  }
  return best ? best : &sf_fonts[0];
}

const SfFontData *font_default_gui()      { return find_font("sans", 13, 0, 400); }
const SfFontData *font_default_gui_bold() { return find_font("sans", 16, 0, 700); }

const SfFontData *font_match(const LOGFONT &lf)
{
  char face[64];
  snprintf(face, sizeof(face), "%s", lf.lfFaceName);
  for (char *p = face; *p; p++) *p = (char)tolower((unsigned char)*p);
  const char *fam = "sans";
  if (strstr(face, "courier") || strstr(face, "lucida console") ||
      strstr(face, "fixedsys") || strstr(face, "terminal") || strstr(face, "mono"))
    fam = "mono";
  int height = lf.lfHeight;
  if (height == 0) height = 16;
  if (height < 0) {
    /* character height (em size): find the cell whose ppem matches */
    int em = -height;
    const SfFontData *best = nullptr;
    for (int i = 0; i < sf_num_fonts; i++) {
      const SfFontData &f = sf_fonts[i];
      if (strcmp(f.family, fam) || f.req_width) continue;
      if ((f.weight >= 600) != (lf.lfWeight >= 600)) continue;
      if (!best || std::abs(f.ppem - em) < std::abs(best->ppem - em)) best = &f;
    }
    if (best) return best;
    height = em + em / 5;
  }
  if (strstr(face, "small fonts")) return find_font("sans", 12, 4, 400);
  return find_font(fam, height, lf.lfWidth, lf.lfWeight);
}

int text_width(const SfFontData *fd, const char *s, int n)
{
  int w = 0;
  for (int i = 0; i < n; i++) w += fd->glyphs[(uint8_t)s[i]].adv;
  return w;
}

void text_draw(Target &t, const SfFontData *fd, int x, int y, const char *s, int n, uint32_t color)
{
  for (int i = 0; i < n; i++) {
    const SfGlyph &g = fd->glyphs[(uint8_t)s[i]];
    if (g.w && g.h) {
      const unsigned char *b = fd->bits + g.off;
      int rb = (g.w + 7) / 8;
      int gx = x + g.x, gy = y + fd->ascent - g.y;
      for (int r = 0; r < g.h; r++)
        for (int c = 0; c < g.w; c++)
          if (b[r * rb + (c >> 3)] & (0x80 >> (c & 7)))
            plot(t, gx + c, gy + r, color);
    }
    x += g.adv;
  }
}

}  /* namespace w32 */

static const SfFontData *dc_font(DCObj *dc)
{
  if (dc && dc->font && dc->font->fd) return dc->font->fd;
  return font_default_gui_bold();
}

HFONT CreateFontIndirect(const LOGFONT *lf)
{
  gdi_init();
  if (!lf) return nullptr;
  FontObj *f = new FontObj();
  f->lf = *lf;
  f->fd = font_match(*lf);
  return (HFONT)hnew(f, HT_FONT);
}

HFONT CreateFont(int h, int w, int esc, int orient, int weight, DWORD italic,
                 DWORD underline, DWORD strike, DWORD charset, DWORD outp,
                 DWORD clip, DWORD quality, DWORD pitch, LPCSTR face)
{
  LOGFONT lf;
  memset(&lf, 0, sizeof(lf));
  lf.lfHeight = h;
  lf.lfWidth = w;
  lf.lfEscapement = esc;
  lf.lfOrientation = orient;
  lf.lfWeight = weight;
  lf.lfItalic = (BYTE)italic;
  lf.lfUnderline = (BYTE)underline;
  lf.lfStrikeOut = (BYTE)strike;
  lf.lfCharSet = (BYTE)charset;
  lf.lfOutPrecision = (BYTE)outp;
  lf.lfClipPrecision = (BYTE)clip;
  lf.lfQuality = (BYTE)quality;
  lf.lfPitchAndFamily = (BYTE)pitch;
  if (face) snprintf(lf.lfFaceName, sizeof(lf.lfFaceName), "%s", face);
  return CreateFontIndirect(&lf);
}

BOOL GetTextMetrics(HDC h, LPTEXTMETRIC tm)
{
  DCObj *dc = dc_from(h);
  if (!dc || !tm) return FALSE;
  const SfFontData *fd = dc_font(dc);
  memset(tm, 0, sizeof(*tm));
  tm->tmHeight = fd->height;
  tm->tmAscent = fd->ascent;
  tm->tmDescent = fd->descent;
  tm->tmInternalLeading = fd->intlead;
  tm->tmExternalLeading = 0;
  tm->tmAveCharWidth = fd->avgw;
  tm->tmMaxCharWidth = fd->maxw;
  tm->tmWeight = fd->weight;
  tm->tmDigitizedAspectX = 96;
  tm->tmDigitizedAspectY = 96;
  tm->tmFirstChar = 32;
  tm->tmLastChar = 255;
  tm->tmDefaultChar = 31;
  tm->tmBreakChar = 32;
  /* TMPF_FIXED_PITCH set means *variable* pitch (sic) */
  bool fixed = strcmp(fd->family, "mono") == 0;
  tm->tmPitchAndFamily = (BYTE)((fixed ? 0 : TMPF_FIXED_PITCH) | TMPF_TRUETYPE | TMPF_VECTOR |
                                (fixed ? FF_MODERN : FF_SWISS));
  tm->tmCharSet = ANSI_CHARSET;
  return TRUE;
}

BOOL GetTextExtentPoint32(HDC h, LPCSTR s, int n, LPSIZE sz)
{
  DCObj *dc = dc_from(h);
  if (!dc || !sz) return FALSE;
  const SfFontData *fd = dc_font(dc);
  sz->cx = s ? text_width(fd, s, n) : 0;
  sz->cy = fd->height;
  return TRUE;
}

static void text_out(DCObj *dc, int x, int y, const char *s, int n, const RECT *opaque, UINT eto)
{
  const SfFontData *fd = dc_font(dc);
  int w = text_width(fd, s, n);
  if (dc->align & TA_UPDATECP) { x = dc->cur.x; y = dc->cur.y; }
  int left = x, top = y;
  switch (dc->align & (TA_LEFT | TA_RIGHT | TA_CENTER)) {
  case TA_RIGHT:  left = x - w; break;
  case TA_CENTER: left = x - w / 2; break;
  default:        left = x; break;
  }
  switch (dc->align & (TA_TOP | TA_BOTTOM | TA_BASELINE)) {
  case TA_BOTTOM:   top = y - fd->height; break;
  case TA_BASELINE: top = y - fd->ascent; break;
  default:          top = y; break;
  }
  if (dc->align & TA_UPDATECP) {
    if ((dc->align & (TA_LEFT | TA_RIGHT | TA_CENTER)) == TA_LEFT) dc->cur.x = x + w;
    else if ((dc->align & (TA_LEFT | TA_RIGHT | TA_CENTER)) == TA_RIGHT) dc->cur.x = x - w;
  }
  Target t;
  if (!resolve_dc(dc, t)) return;
  if (opaque && (eto & 2 /*ETO_OPAQUE*/))
    fill_rect(t, opaque->left + t.ox, opaque->top + t.oy, opaque->right + t.ox,
              opaque->bottom + t.oy, cref_to_px(dc->bk));
  if (eto & 4 /*ETO_CLIPPED*/ && opaque) {
    std::vector<RECT> out;
    RECT o = { opaque->left + t.ox, opaque->top + t.oy, opaque->right + t.ox, opaque->bottom + t.oy };
    for (auto &c : t.clips) {
      RECT i = { std::max(o.left, c.left), std::max(o.top, c.top),
                 std::min(o.right, c.right), std::min(o.bottom, c.bottom) };
      if (i.left < i.right && i.top < i.bottom) out.push_back(i);
    }
    t.clips.swap(out);
    if (t.clips.empty()) return;
  }
  if (dc->bkMode == OPAQUE)
    fill_rect(t, left + t.ox, top + t.oy, left + w + t.ox, top + fd->height + t.oy, cref_to_px(dc->bk));
  text_draw(t, fd, left + t.ox, top + t.oy, s, n, cref_to_px(dc->text));
  screen_dirty();
}

BOOL TextOut(HDC h, int x, int y, LPCSTR s, int n)
{
  DCObj *dc = dc_from(h);
  if (!dc || !s) return FALSE;
  if (n > 0) text_out(dc, x, y, s, n, nullptr, 0);
  return TRUE;
}

BOOL ExtTextOut(HDC h, int x, int y, UINT opts, const RECT *r, LPCSTR s, UINT n, const INT *)
{
  DCObj *dc = dc_from(h);
  if (!dc) return FALSE;
  text_out(dc, x, y, s ? s : "", s ? (int)n : 0, r, opts);
  return TRUE;
}

/* ------------------------------- DrawText -------------------------------- */
namespace {
struct Line { std::string s; int prefixPos; };

std::vector<Line> layout(const SfFontData *fd, const char *s, int n, int width, UINT f)
{
  std::vector<Line> lines;
  std::string text;
  std::vector<int> prefix;   /* underline positions in text */
  for (int i = 0; i < n; i++) {
    char c = s[i];
    if (!(f & DT_NOPREFIX) && c == '&') {
      if (i + 1 < n && s[i + 1] == '&') { text.push_back('&'); i++; continue; }
      prefix.push_back((int)text.size());
      continue;
    }
    if (c == '\t' && (f & DT_EXPANDTABS)) { text.append(8 - (text.size() % 8), ' '); continue; }
    text.push_back(c);
  }
  auto push = [&](const std::string &l) { lines.push_back(Line{l, -1}); };
  if (f & DT_SINGLELINE) {
    std::string l;
    for (char c : text) if (c != '\r' && c != '\n') l.push_back(c);
    push(l);
    return lines;
  }
  size_t i = 0;
  while (i <= text.size()) {
    size_t e = text.find('\n', i);
    std::string para = text.substr(i, e == std::string::npos ? std::string::npos : e - i);
    if (!para.empty() && para.back() == '\r') para.pop_back();
    if (!(f & DT_WORDBREAK) || width <= 0) push(para);
    else {
      /* greedy word wrap */
      std::string cur;
      size_t p = 0;
      if (para.empty()) push("");
      while (p < para.size()) {
        size_t sp = para.find(' ', p);
        std::string word = para.substr(p, sp == std::string::npos ? std::string::npos : sp - p + 1);
        std::string trial = cur + word;
        std::string trimmed = trial;
        while (!trimmed.empty() && trimmed.back() == ' ') trimmed.pop_back();
        if (!cur.empty() && text_width(fd, trimmed.c_str(), (int)trimmed.size()) > width) {
          while (!cur.empty() && cur.back() == ' ') cur.pop_back();
          push(cur);
          cur = word;
        } else {
          cur = trial;
        }
        p = (sp == std::string::npos) ? para.size() : sp + 1;
      }
      if (!cur.empty()) { while (!cur.empty() && cur.back() == ' ') cur.pop_back(); push(cur); }
    }
    if (e == std::string::npos) break;
    i = e + 1;
  }
  if (!prefix.empty() && !lines.empty()) lines[0].prefixPos = prefix[0];
  return lines;
}
}  /* namespace */

namespace w32 {
void draw_text_in(Target &t, const SfFontData *fd, const char *s, RECT r, UINT f, uint32_t color)
{
  int n = (int)strlen(s);
  std::vector<Line> lines = layout(fd, s, n, r.right - r.left, f);
  int th = (int)lines.size() * fd->height;
  int y = r.top;
  if (f & DT_SINGLELINE) {
    if (f & DT_VCENTER) y = r.top + ((r.bottom - r.top) - fd->height) / 2;
    else if (f & DT_BOTTOM) y = r.bottom - fd->height;
  }
  (void)th;
  for (auto &l : lines) {
    int w = text_width(fd, l.s.c_str(), (int)l.s.size());
    int x = r.left;
    if (f & DT_CENTER) x = r.left + ((r.right - r.left) - w) / 2;
    else if (f & DT_RIGHT) x = r.right - w;
    text_draw(t, fd, x, y, l.s.c_str(), (int)l.s.size(), color);
    if (l.prefixPos >= 0 && l.prefixPos < (int)l.s.size()) {
      int ux = x + text_width(fd, l.s.c_str(), l.prefixPos);
      int uw = fd->glyphs[(uint8_t)l.s[l.prefixPos]].adv;
      hline(t, ux, ux + uw, y + fd->ascent + 1, color);
    }
    y += fd->height;
  }
}
}  /* namespace w32 */

int DrawText(HDC h, LPCSTR s, int n, LPRECT r, UINT f)
{
  DCObj *dc = dc_from(h);
  if (!dc || !s || !r) return 0;
  if (n < 0) n = (int)strlen(s);
  const SfFontData *fd = dc_font(dc);
  std::vector<Line> lines = layout(fd, s, n, r->right - r->left, f);
  int height = (int)lines.size() * fd->height;
  if (f & DT_CALCRECT) {
    int maxw = 0;
    for (auto &l : lines) maxw = std::max(maxw, text_width(fd, l.s.c_str(), (int)l.s.size()));
    r->right = r->left + maxw;
    r->bottom = r->top + height;
    return height;
  }
  Target t;
  if (!resolve_dc(dc, t)) return height;
  RECT sr = { r->left + t.ox, r->top + t.oy, r->right + t.ox, r->bottom + t.oy };
  if (!(f & DT_NOCLIP)) {
    std::vector<RECT> out;
    for (auto &c : t.clips) {
      RECT i = { std::max(sr.left, c.left), std::max(sr.top, c.top),
                 std::min(sr.right, c.right), std::min(sr.bottom, c.bottom) };
      if (i.left < i.right && i.top < i.bottom) out.push_back(i);
    }
    t.clips.swap(out);
  }
  if (!t.clips.empty()) {
    if (dc->bkMode == OPAQUE) {
      int y = sr.top;
      for (auto &l : lines) {
        int w = text_width(fd, l.s.c_str(), (int)l.s.size());
        int x = sr.left;
        if (f & DT_CENTER) x = sr.left + ((sr.right - sr.left) - w) / 2;
        else if (f & DT_RIGHT) x = sr.right - w;
        fill_rect(t, x, y, x + w, y + fd->height, cref_to_px(dc->bk));
        y += fd->height;
      }
    }
    std::string str(s, n);
    draw_text_in(t, fd, str.c_str(), sr, f, cref_to_px(dc->text));
    screen_dirty();
  }
  return height;
}
