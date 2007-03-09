/* LIBGIMP - The GIMP Library
 *
 * gimpmd5.c
 *
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * GIMPified 2002 by Sven Neumann <sven@gimp.org>
 */

/* parts of this file are :
 * Written March 1993 by Branko Lankester
 * Modified June 1993 by Colin Plumb for altered md5.c.
 * Modified October 1995 by Erik Troan for RPM
 */

#include "config.h"

#include <string.h>

#include <glib.h>

#include "gimpmd5.h"

typedef struct _GimpMD5Context GimpMD5Context;

struct _GimpMD5Context
{
  guint32 buf[4];
  guint32 bits[2];
  guchar  in[64];
};


static void  gimp_md5_init      (GimpMD5Context *ctx);
static void  gimp_md5_transform (guint32         buf[4],
                                 const guint32   in[16]);
static void  gimp_md5_update    (GimpMD5Context *ctx,
                                 const gchar    *buf,
                                 guint32         len);
static void  gimp_md5_final     (GimpMD5Context *ctx,
                                 guchar          digest[16]);


/**
 * gimp_md5_get_digest:
 * @buffer: byte buffer
 * @buffer_size: buffer size (in bytes) or -1 if @buffer is nul-terminated.
 * @digest: 16 bytes buffer receiving the hash code.
 *
 * Get the md5 hash of a buffer. The result is put in the 16 bytes
 * buffer @digest.
 *
 * The MD5 algorithm takes as input a message of arbitrary length and
 * produces as output a 128-bit "fingerprint" or "message digest" of
 * the input.  It is conjectured that it is computationally infeasible
 * to produce two messages having the same message digest, or to
 * produce any message having a given prespecified target message
 * digest. For more information see RFC 1321.
 **/
void
gimp_md5_get_digest (const gchar *buffer,
                     gint         buffer_size,
                     guchar       digest[16])
{
  GimpMD5Context ctx;

  g_return_if_fail (buffer != NULL);
  g_return_if_fail (digest != NULL);

  if (buffer_size < 0)
    buffer_size = strlen (buffer);

  gimp_md5_init (&ctx);
  gimp_md5_update (&ctx, buffer, buffer_size);
  gimp_md5_final (&ctx, digest);
}

static inline void
byte_reverse (guint32 *buf,
              guint32  longs)
{
#if G_BYTE_ORDER != G_LITTLE_ENDIAN
  do
    {
      *buf = GINT32_TO_LE (*buf);
      buf++;
    }
  while (--longs);
#endif
}

static void
gimp_md5_init (GimpMD5Context *ctx)
{
  ctx->buf[0] = 0x67452301;
  ctx->buf[1] = 0xefcdab89;
  ctx->buf[2] = 0x98badcfe;
  ctx->buf[3] = 0x10325476;

  ctx->bits[0] = 0;
  ctx->bits[1] = 0;
}

static void
gimp_md5_update (GimpMD5Context *ctx,
                 const gchar    *buf,
                 guint32         len)
{
  guint32 t;

  /* Update bitcount */
  t = ctx->bits[0];
  if ((ctx->bits[0] = t + ((guint32) len << 3)) < t)
    ctx->bits[1]++;             /* Carry from low to high */
  ctx->bits[1] += len >> 29;

  t = (t >> 3) & 0x3f;

  /* Handle any leading odd-sized chunks */
  if (t)
    {
      guchar *p = (guchar *) ctx->in + t;

      t = 64 - t;
      if (len < t)
        {
          memcpy (p, buf, len);
          return;
        }
      memcpy (p, buf, t);

      byte_reverse ((guint32 *) ctx->in, 16);

      gimp_md5_transform (ctx->buf, (guint32 *) ctx->in);
      buf += t;
      len -= t;
    }

  /* Process data in 64-byte chunks */
  while (len >= 64)
    {
      memcpy (ctx->in, buf, 64);
      byte_reverse ((guint32 *) ctx->in, 16);
      gimp_md5_transform (ctx->buf, (guint32 *) ctx->in);
      buf += 64;
      len -= 64;
    }

  /* Handle any remaining bytes of data. */
  memcpy (ctx->in, buf, len);
}

static void
gimp_md5_final (GimpMD5Context *ctx,
                guchar          digest[16])
{
  guint32 count;
  guchar *p;

  /* Compute number of bytes mod 64 */
  count = (ctx->bits[0] >> 3) & 0x3F;

  /* Set the first char of padding to 0x80.  This is safe since there is
     always at least one byte free */
  p = ctx->in + count;
  *p++ = 0x80;

  /* Bytes of padding needed to make 64 bytes */
  count = 64 - 1 - count;

  /* Pad out to 56 mod 64 */
  if (count < 8)
    {
      /* Two lots of padding:  Pad the first block to 64 bytes */
      memset (p, 0, count);
      byte_reverse ((guint32 *) ctx->in, 16);
      gimp_md5_transform (ctx->buf, (guint32 *) ctx->in);

      /* Now fill the next block with 56 bytes */
      memset (ctx->in, 0, 56);
    }
  else
    {
      /* Pad block to 56 bytes */
      memset (p, 0, count - 8);
    }

  byte_reverse ((guint32 *) ctx->in, 14);

  /* Append length in bits and transform */
  ((guint32 *) ctx->in)[14] = ctx->bits[0];
  ((guint32 *) ctx->in)[15] = ctx->bits[1];

  gimp_md5_transform (ctx->buf, (guint32 *) ctx->in);
  byte_reverse (ctx->buf, 4);
  memcpy (digest, ctx->buf, 16);
}


/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) \
        ( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  md5_Update blocks
 * the data and converts bytes into longwords for this routine.
 */
static void
gimp_md5_transform (guint32       buf[4],
                    const guint32 in[16])
{
  register guint32 a, b, c, d;

  a = buf[0];
  b = buf[1];
  c = buf[2];
  d = buf[3];

  MD5STEP (F1, a, b, c, d, in[0] + 0xd76aa478, 7);
  MD5STEP (F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
  MD5STEP (F1, c, d, a, b, in[2] + 0x242070db, 17);
  MD5STEP (F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
  MD5STEP (F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
  MD5STEP (F1, d, a, b, c, in[5] + 0x4787c62a, 12);
  MD5STEP (F1, c, d, a, b, in[6] + 0xa8304613, 17);
  MD5STEP (F1, b, c, d, a, in[7] + 0xfd469501, 22);
  MD5STEP (F1, a, b, c, d, in[8] + 0x698098d8, 7);
  MD5STEP (F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
  MD5STEP (F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
  MD5STEP (F1, b, c, d, a, in[11] + 0x895cd7be, 22);
  MD5STEP (F1, a, b, c, d, in[12] + 0x6b901122, 7);
  MD5STEP (F1, d, a, b, c, in[13] + 0xfd987193, 12);
  MD5STEP (F1, c, d, a, b, in[14] + 0xa679438e, 17);
  MD5STEP (F1, b, c, d, a, in[15] + 0x49b40821, 22);

  MD5STEP (F2, a, b, c, d, in[1] + 0xf61e2562, 5);
  MD5STEP (F2, d, a, b, c, in[6] + 0xc040b340, 9);
  MD5STEP (F2, c, d, a, b, in[11] + 0x265e5a51, 14);
  MD5STEP (F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
  MD5STEP (F2, a, b, c, d, in[5] + 0xd62f105d, 5);
  MD5STEP (F2, d, a, b, c, in[10] + 0x02441453, 9);
  MD5STEP (F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
  MD5STEP (F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
  MD5STEP (F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
  MD5STEP (F2, d, a, b, c, in[14] + 0xc33707d6, 9);
  MD5STEP (F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
  MD5STEP (F2, b, c, d, a, in[8] + 0x455a14ed, 20);
  MD5STEP (F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
  MD5STEP (F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
  MD5STEP (F2, c, d, a, b, in[7] + 0x676f02d9, 14);
  MD5STEP (F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

  MD5STEP (F3, a, b, c, d, in[5] + 0xfffa3942, 4);
  MD5STEP (F3, d, a, b, c, in[8] + 0x8771f681, 11);
  MD5STEP (F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
  MD5STEP (F3, b, c, d, a, in[14] + 0xfde5380c, 23);
  MD5STEP (F3, a, b, c, d, in[1] + 0xa4beea44, 4);
  MD5STEP (F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
  MD5STEP (F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
  MD5STEP (F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
  MD5STEP (F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
  MD5STEP (F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
  MD5STEP (F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
  MD5STEP (F3, b, c, d, a, in[6] + 0x04881d05, 23);
  MD5STEP (F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
  MD5STEP (F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
  MD5STEP (F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
  MD5STEP (F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

  MD5STEP (F4, a, b, c, d, in[0] + 0xf4292244, 6);
  MD5STEP (F4, d, a, b, c, in[7] + 0x432aff97, 10);
  MD5STEP (F4, c, d, a, b, in[14] + 0xab9423a7, 15);
  MD5STEP (F4, b, c, d, a, in[5] + 0xfc93a039, 21);
  MD5STEP (F4, a, b, c, d, in[12] + 0x655b59c3, 6);
  MD5STEP (F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
  MD5STEP (F4, c, d, a, b, in[10] + 0xffeff47d, 15);
  MD5STEP (F4, b, c, d, a, in[1] + 0x85845dd1, 21);
  MD5STEP (F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
  MD5STEP (F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
  MD5STEP (F4, c, d, a, b, in[6] + 0xa3014314, 15);
  MD5STEP (F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
  MD5STEP (F4, a, b, c, d, in[4] + 0xf7537e82, 6);
  MD5STEP (F4, d, a, b, c, in[11] + 0xbd3af235, 10);
  MD5STEP (F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
  MD5STEP (F4, b, c, d, a, in[9] + 0xeb86d391, 21);

  buf[0] += a;
  buf[1] += b;
  buf[2] += c;
  buf[3] += d;
}
