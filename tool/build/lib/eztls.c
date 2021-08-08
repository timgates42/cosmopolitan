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
#include "libc/calls/calls.h"
#include "libc/calls/struct/iovec.h"
#include "libc/errno.h"
#include "libc/log/check.h"
#include "libc/log/log.h"
#include "libc/rand/rand.h"
#include "libc/sock/sock.h"
#include "libc/sysv/consts/sig.h"
#include "libc/x/x.h"
#include "third_party/mbedtls/ctr_drbg.h"
#include "third_party/mbedtls/ecp.h"
#include "third_party/mbedtls/error.h"
#include "third_party/mbedtls/ssl.h"
#include "tool/build/lib/eztls.h"
#include "tool/build/lib/psk.h"

struct EzTlsBio ezbio;
mbedtls_ssl_config ezconf;
mbedtls_ssl_context ezssl;
mbedtls_ctr_drbg_context ezrng;

static char *EzTlsError(int r) {
  static char b[128];
  mbedtls_strerror(r, b, sizeof(b));
  return b;
}

static wontreturn void EzTlsDie(const char *s, int r) {
  if (IsTiny()) {
    fprintf(stderr, "error: %s (-0x%04x %s)\n", s, -r, EzTlsError(r));
  } else {
    fprintf(stderr, "error: %s (grep -0x%04x)\n", s, -r);
  }
  exit(1);
}

static int EzGetEntropy(void *c, unsigned char *p, size_t n) {
  CHECK_EQ(n, getrandom(p, n, 0));
  return 0;
}

static void EzInitializeRng(mbedtls_ctr_drbg_context *r) {
  volatile unsigned char b[64];
  mbedtls_ctr_drbg_init(r);
  CHECK(getrandom(b, 64, 0) == 64);
  CHECK(!mbedtls_ctr_drbg_seed(r, EzGetEntropy, 0, b, 64));
  mbedtls_platform_zeroize(b, 64);
}

static ssize_t EzWritevAll(int fd, struct iovec *iov, int iovlen) {
  int i;
  ssize_t rc;
  size_t wrote, total;
  i = 0;
  total = 0;
  do {
    if (i) {
      while (i < iovlen && !iov[i].iov_len) ++i;
      if (i == iovlen) break;
    }
    if ((rc = writev(fd, iov + i, iovlen - i)) != -1) {
      wrote = rc;
      total += wrote;
      do {
        if (wrote >= iov[i].iov_len) {
          wrote -= iov[i++].iov_len;
        } else {
          iov[i].iov_base = (char *)iov[i].iov_base + wrote;
          iov[i].iov_len -= wrote;
          wrote = 0;
        }
      } while (wrote);
    } else if (errno != EINTR) {
      return total ? total : -1;
    }
  } while (i < iovlen);
  return total;
}

int EzTlsFlush(struct EzTlsBio *bio, const unsigned char *buf, size_t len) {
  struct iovec v[2];
  if (len || bio->c > 0) {
    v[0].iov_base = bio->u;
    v[0].iov_len = MAX(0, bio->c);
    v[1].iov_base = buf;
    v[1].iov_len = len;
    if (EzWritevAll(bio->fd, v, 2) != -1) {
      if (bio->c > 0) bio->c = 0;
    } else if (errno == EINTR) {
      return MBEDTLS_ERR_NET_CONN_RESET;
    } else if (errno == EAGAIN) {
      return MBEDTLS_ERR_SSL_TIMEOUT;
    } else if (errno == EPIPE || errno == ECONNRESET || errno == ENETRESET) {
      return MBEDTLS_ERR_NET_CONN_RESET;
    } else {
      WARNF("EzTlsSend error %s", strerror(errno));
      return MBEDTLS_ERR_NET_SEND_FAILED;
    }
  }
  return 0;
}

static int EzTlsSend(void *ctx, const unsigned char *buf, size_t len) {
  int rc;
  struct iovec v[2];
  struct EzTlsBio *bio = ctx;
  if (bio->c >= 0 && bio->c + len <= sizeof(bio->u)) {
    memcpy(bio->u + bio->c, buf, len);
    bio->c += len;
    return len;
  }
  if ((rc = EzTlsFlush(bio, buf, len)) < 0) return rc;
  return len;
}

static int EzTlsRecvImpl(void *ctx, unsigned char *p, size_t n, uint32_t o) {
  int r;
  ssize_t s;
  struct iovec v[2];
  struct EzTlsBio *bio = ctx;
  if ((r = EzTlsFlush(bio, 0, 0)) < 0) return r;
  if (bio->a < bio->b) {
    r = MIN(n, bio->b - bio->a);
    memcpy(p, bio->t + bio->a, r);
    if ((bio->a += r) == bio->b) bio->a = bio->b = 0;
    return r;
  }
  v[0].iov_base = p;
  v[0].iov_len = n;
  v[1].iov_base = bio->t;
  v[1].iov_len = sizeof(bio->t);
  while ((r = readv(bio->fd, v, 2)) == -1) {
    if (errno == EINTR) {
      return MBEDTLS_ERR_SSL_WANT_READ;
    } else if (errno == EAGAIN) {
      return MBEDTLS_ERR_SSL_TIMEOUT;
    } else if (errno == EPIPE || errno == ECONNRESET || errno == ENETRESET) {
      return MBEDTLS_ERR_NET_CONN_RESET;
    } else {
      WARNF("tls read() error %s", strerror(errno));
      return MBEDTLS_ERR_NET_RECV_FAILED;
    }
  }
  if (r > n) bio->b = r - n;
  return MIN(n, r);
}

static int EzTlsRecv(void *ctx, unsigned char *buf, size_t len, uint32_t tmo) {
  return EzTlsRecvImpl(ctx, buf, len, tmo);
}

/*
 * openssl s_client -connect 127.0.0.1:31337 \
 *   -psk $(hex <~/.runit.psk)               \
 *   -psk_identity runit
 */

void SetupPresharedKeySsl(int endpoint) {
  xsigaction(SIGPIPE, SIG_IGN, 0, 0, 0);
  EzInitializeRng(&ezrng);
  mbedtls_ssl_config_defaults(&ezconf, endpoint, MBEDTLS_SSL_TRANSPORT_STREAM,
                              MBEDTLS_SSL_PRESET_SUITEC);
  mbedtls_ssl_conf_rng(&ezconf, mbedtls_ctr_drbg_random, &ezrng);
  DCHECK_EQ(0, mbedtls_ssl_conf_psk(&ezconf, GetRunitPsk(), 32, "runit", 5));
  DCHECK_EQ(0, mbedtls_ssl_setup(&ezssl, &ezconf));
  mbedtls_ssl_set_bio(&ezssl, &ezbio, EzTlsSend, 0, EzTlsRecv);
}
