/* LIBGIMP - The GIMP Library
 *
 * gimpmd5.h
 *
 * Use of this code is deprecated! Use %GChecksum from GLib instead.
 */

#if !defined (__GIMP_MATH_H_INSIDE__) && !defined (GIMP_MATH_COMPILATION)
#error "Only <libgimpmath/gimpmath.h> can be included directly."
#endif

#ifndef __GIMP_MD5_H__
#define __GIMP_MD5_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

GIMP_DEPRECATED_FOR(GChecksum)
void gimp_md5_get_digest (const gchar *buffer,
                          gint         buffer_size,
                          guchar       digest[16]);

G_END_DECLS

#endif  /* __GIMP_MD5_H__ */
