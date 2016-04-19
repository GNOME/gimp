/* tiff loading for GIMP
 *  -Peter Mattis
 *
 * The TIFF loading code has been completely revamped by Nick Lamb
 * njl195@zepler.org.uk -- 18 May 1998
 * And it now gains support for tiles (and doubtless a zillion bugs)
 * njl195@zepler.org.uk -- 12 June 1999
 * LZW patent fuss continues :(
 * njl195@zepler.org.uk -- 20 April 2000
 * The code for this filter is based on "tifftopnm" and "pnmtotiff",
 *  2 programs that are a part of the netpbm package.
 * khk@khk.net -- 13 May 2000
 * Added support for ICCPROFILE tiff tag. If this tag is present in a
 * TIFF file, then a parasite is created and vice versa.
 * peter@kirchgessner.net -- 29 Oct 2002
 * Progress bar only when run interactive
 * Added support for layer offsets - pablo.dangelo@web.de -- 7 Jan 2004
 * Honor EXTRASAMPLES tag while loading images with alphachannel
 * pablo.dangelo@web.de -- 16 Jan 2004
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <tiffio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "file-tiff-io.h"


static void   tiff_warning (const gchar *module,
                            const gchar *fmt,
                            va_list      ap) G_GNUC_PRINTF (2, 0);
static void   tiff_error   (const gchar *module,
                            const gchar *fmt,
                            va_list      ap) G_GNUC_PRINTF (2, 0);


TIFF *
tiff_open (GFile        *file,
           const gchar  *mode,
           GError      **error)
{
  gchar *filename = g_file_get_path (file);

  TIFFSetWarningHandler (tiff_warning);
  TIFFSetErrorHandler (tiff_error);

#ifdef G_OS_WIN32
  gunichar2 *utf16_filename = g_utf8_to_utf16 (filename, -1, NULL, NULL, error);

  if (utf16_filename)
    {
      TIFF *tif = TIFFOpenW (utf16_filename, mode);

      g_free (utf16_filename);

      return tif;
    }

  return NULL;
#else
  return TIFFOpen (filename, mode);
#endif
}

static void
tiff_warning (const gchar *module,
              const gchar *fmt,
              va_list      ap)
{
  gint tag = 0;

  if (! strcmp (fmt, "%s: unknown field with tag %d (0x%x) encountered"))
    {
      va_list ap_test;

      G_VA_COPY (ap_test, ap);

      va_arg (ap_test, const char *); /* ignore first arg */

      tag = va_arg (ap_test, int);
    }
  /* for older versions of libtiff? */
  else if (! strcmp (fmt, "unknown field with tag %d (0x%x) ignored"))
    {
      va_list ap_test;

      G_VA_COPY (ap_test, ap);

      tag = va_arg (ap_test, int);
    }

  /* Workaround for: http://bugzilla.gnome.org/show_bug.cgi?id=131975
   * Ignore the warnings about unregistered private tags (>= 32768).
   */
  if (tag >= 32768)
    return;

  /* Other unknown fields are only reported to stderr. */
  if (tag > 0)
    {
      gchar *msg = g_strdup_vprintf (fmt, ap);

      g_printerr ("%s\n", msg);
      g_free (msg);

      return;
    }

  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, fmt, ap);
}

static void
tiff_error (const gchar *module,
            const gchar *fmt,
            va_list      ap)
{
  /* Workaround for: http://bugzilla.gnome.org/show_bug.cgi?id=132297
   * Ignore the errors related to random access and JPEG compression
   */
  if (! strcmp (fmt, "Compression algorithm does not support random access"))
    return;

  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, fmt, ap);
}
