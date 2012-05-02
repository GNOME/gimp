/* LIBGIMP - The GIMP Library
 *
 * gimpmd5.c
 *
 * Use of this code is deprecated! Use %GChecksum from GLib instead.
 */

#include "config.h"

#include <glib-object.h>

#include "gimpmathtypes.h"

#include "gimpmd5.h"


/**
 * SECTION: gimpmd5
 * @title: GimpMD5
 * @short_description: The MD5 message-digest algorithm
 *
 * The MD5 message-digest algorithm
 **/


/**
 * gimp_md5_get_digest:
 * @buffer:      byte buffer
 * @buffer_size: buffer size (in bytes) or -1 if @buffer is nul-terminated.
 * @digest:      16 bytes buffer receiving the hash code.
 *
 * This function is deprecated! Use %GChecksum from GLib instead.
 *
 * Get the md5 hash of a buffer. The result is put in the 16 bytes
 * buffer @digest. For more information see RFC 1321.
 **/
void
gimp_md5_get_digest (const gchar *buffer,
                     gint         buffer_size,
                     guchar       digest[16])
{
  GChecksum *checksum;
  gsize      len = 16;

  g_return_if_fail (buffer != NULL);
  g_return_if_fail (digest != NULL);

  checksum = g_checksum_new (G_CHECKSUM_MD5);

  g_checksum_update (checksum, (const guchar *) buffer, buffer_size);
  g_checksum_get_digest (checksum, digest, &len);
  g_checksum_free (checksum);
}
