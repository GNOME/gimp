/* LIBGIMP - The GIMP Library
 *
 * gimpmd5.h
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

#ifndef __GIMP_MD5_H__
#define __GIMP_MD5_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

void gimp_md5_get_digest (const gchar *buffer,
                          gint         buffer_size,
                          guchar       digest[16]);


G_END_DECLS

#endif  /* __GIMP_MD5_H__ */
