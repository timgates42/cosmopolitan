/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2021 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/
#include "libc/assert.h"
#include "libc/dce.h"
#include "libc/macros.internal.h"
#include "libc/mem/mem.h"
#include "libc/stdio/append.internal.h"
#include "libc/str/str.h"

#define W sizeof(size_t)

/**
 * Resets length of append buffer, e.g.
 *
 *     char *b = 0;
 *     appends(&b, "hello");
 *     appendr(&b, 1);
 *     assert(!strcmp(b, "h"));
 *     appendr(&b, 0);
 *     assert(!strcmp(b, ""));
 *     free(b);
 *
 * If `i` is greater than the current length then the extra bytes are
 * filled with NUL characters.
 *
 * The resulting buffer is guarranteed to be NUL-terminated, i.e.
 * `!b[appendz(b).i]` will be the case.
 *
 * @return `i` or -1 if `ENOMEM`
 * @see appendz(b).i to get buffer length
 */
ssize_t appendr(char **b, size_t i) {
  char *p;
  struct appendz z;
  assert(b);
  z = appendz((p = *b));
  z.n = ROUNDUP(i + 1, 8) + W;
  if ((p = realloc(p, z.n))) {
    z.n = malloc_usable_size(p);
    assert(!(z.n & (W - 1)));
    *b = p;
  } else {
    return -1;
  }
  if (i > z.i) {
    memset(p, z.i, i - z.i);
  }
  z.i = i;
  p[z.i] = 0;
  if (!IsTiny() && W == 8) {
    z.i |= (size_t)APPEND_COOKIE << 48;
  }
  *(size_t *)(p + z.n - W) = z.i;
  return i;
}
