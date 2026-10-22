#include "fm1_adapter.h"
#include <string.h>

static bool string(const uint8_t *d, unsigned n, unsigned *at, char *out,
                   unsigned cap) {
  unsigned used = 0;
  while (*at < n && d[*at]) {
    unsigned c = d[(*at)++];
    if (c < 32 || c > 126)
      return false;
    if (used + 1 < cap)
      out[used++] = (char)c;
  }
  if (*at >= n)
    return false;
  (*at)++;
  out[used] = 0;
  return true;
}
bool fm1_adapter_parse_info(fm1_adapter_state_t *s, const uint8_t *d,
                            unsigned n) {
  unsigned p = 0;
  char version[32] = {0};
  if (!string(d, n, &p, version, sizeof(version)) || n - p < 5)
    return false;
  unsigned engines = d[p++], params = d[p++], globals = d[p++], steps = d[p++],
           base = d[p++];
  if (engines > 32 || params > 127 || globals > 127 || steps > 127 ||
      base > params)
    return false;
  char names[768] = {0};
  unsigned used = 0;
  for (unsigned i = 0; i < engines; i++) {
    char name[40];
    if (!string(d, n, &p, name, sizeof(name)))
      return false;
    unsigned len = strlen(name);
    if (used + len + 2 >= sizeof(names))
      return false;
    if (i)
      names[used++] = '\n';
    memcpy(names + used, name, len);
    used += len;
  }
  unsigned tracks = p < n ? d[p++] : 1, protocol = p < n ? d[p++] : 1,
           menus = 0;
  if (tracks > 8)
    return false;
  if (s->profile == FW_FELUCCA) {
    /* INFO uses tagged capability records, not SLOOP's protocol number. */
    for (unsigned i = p; i + 2 < n; i++)
      if (d[i] == 0x4e && d[i + 1] == 1) {
        menus = d[i + 2];
        break;
      }
    protocol = 7;
  }
  memcpy(s->version, version, sizeof(version));
  memcpy(s->engine_names, names, sizeof(names));
  s->engines = engines;
  s->parameters = params;
  s->globals = globals;
  s->steps = steps;
  s->engine_base = base;
  s->tracks = tracks;
  s->protocol = protocol;
  s->menu_count = menus;
  s->ready = true;
  return true;
}
bool fm1_adapter_parse_desc(fm1_remote_parameter_t *out, const uint8_t *d,
                            unsigned n, bool menu) {
  if (n < 9)
    return false;
  fm1_remote_parameter_t p = {0};
  p.id = d[1];
  p.scope = menu ? 2 : d[0];
  p.index = menu ? d[0] : d[1];
  p.format = menu ? (d[2] == 0 ? 8 : 0) : d[2];
  unsigned at = 9;
  if (menu) {
    p.value = fm1_v14(d + 3);
    p.minimum = fm1_v14(d + 5);
    p.maximum = fm1_v14(d + 7);
    p.known = true;
  } else {
    p.minimum = fm1_v14(d + 3);
    p.maximum = fm1_v14(d + 5);
    p.initial = fm1_v14(d + 7);
  }
  if (p.minimum > p.maximum || !string(d, n, &at, p.name, sizeof(p.name)))
    return false;
  if (!menu || p.format != 8)
    if (!string(d, n, &at, p.unit, sizeof(p.unit)))
      return false;
  if (p.format == 8) {
    unsigned used = 0;
    for (int i = p.minimum; i <= p.maximum; i++) {
      /* Firmware caps enum names (24/64); missing whole names fall back to
       * a numeric control, while an unterminated name is still invalid. */
      if (at == n) {
        p.options[0] = 0;
        break;
      }
      char option[40];
      if (!string(d, n, &at, option, sizeof(option)))
        return false;
      unsigned len = strlen(option);
      if (used + len + 2 >= sizeof(p.options)) {
        p.options[0] = 0;
        break;
      }
      if (i != p.minimum)
        p.options[used++] = '\n';
      memcpy(p.options + used, option, len);
      used += len;
    }
  }
  p.described = true;
  *out = p;
  return true;
}
