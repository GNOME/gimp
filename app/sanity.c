/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <fontconfig/fontconfig.h>
#include <pango/pango.h>
#include <pango/pangoft2.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <lcms2.h>
#include <gexiv2/gexiv2.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "sanity.h"

#include "gimp-intl.h"


/*  early-stage tests  */
static gchar * sanity_check_gimp              (void);
static gchar * sanity_check_glib              (void);
static gchar * sanity_check_cairo             (void);
static gchar * sanity_check_pango             (void);
static gchar * sanity_check_fontconfig        (void);
static gchar * sanity_check_freetype          (void);
static gchar * sanity_check_gdk_pixbuf        (void);
static gchar * sanity_check_lcms              (void);
static gchar * sanity_check_gexiv2            (void);
static gchar * sanity_check_babl              (void);
static gchar * sanity_check_gegl              (void);
static gchar * sanity_check_filename_encoding (void);

/*  late-stage tests  */
static gchar * sanity_check_gegl_ops          (void);


/*  public functions  */

/* early-stage sanity check, performed before the call to app_run(). */
const gchar *
sanity_check_early (void)
{
  gchar *abort_message = NULL;

  if (! abort_message)
    abort_message = sanity_check_gimp ();

  if (! abort_message)
    abort_message = sanity_check_glib ();

  if (! abort_message)
    abort_message = sanity_check_cairo ();

  if (! abort_message)
    abort_message = sanity_check_pango ();

  if (! abort_message)
    abort_message = sanity_check_fontconfig ();

  if (! abort_message)
    abort_message = sanity_check_freetype ();

  if (! abort_message)
    abort_message = sanity_check_gdk_pixbuf ();

  if (! abort_message)
    abort_message = sanity_check_lcms ();

  if (! abort_message)
    abort_message = sanity_check_gexiv2 ();

  if (! abort_message)
    abort_message = sanity_check_babl ();

  if (! abort_message)
    abort_message = sanity_check_gegl ();

  if (! abort_message)
    abort_message = sanity_check_filename_encoding ();

  return abort_message;
}

/* late-stage sanity check, performed during app_run(), after the user
 * configuration has been loaded.
 */
const gchar *
sanity_check_late (void)
{
  gchar *abort_message = NULL;

  /* the gegl ops test initializes all gegl ops; in particular, it initializes
   * all the strings used by their properties, which appear in the ui.  it
   * must be run after we've called language_init(), potentially overriding
   * LANGUAGE according to the user config, or else all affected strings would
   * use the translation corresponding to the system locale, regardless.
   */
  if (! abort_message)
    abort_message = sanity_check_gegl_ops ();

  return abort_message;
}


/*  private functions  */


/*  early-stage tests  */

static gboolean
sanity_check_version (guint major_version, guint required_major,
                      guint minor_version, guint required_minor,
                      guint micro_version, guint required_micro)
{
  if (major_version > required_major)
    return TRUE;

  if (major_version < required_major)
    return FALSE;

  if (minor_version > required_minor)
    return TRUE;

  if (minor_version < required_minor)
    return FALSE;

  if (micro_version >= required_micro)
    return TRUE;

  return FALSE;
}

static gchar *
sanity_check_gimp (void)
{
  if (GIMP_MAJOR_VERSION != gimp_major_version ||
      GIMP_MINOR_VERSION != gimp_minor_version ||
      GIMP_MICRO_VERSION != gimp_micro_version)
    {
      return g_strdup_printf
        ("Libgimp version mismatch!\n\n"
         "The GIMP binary cannot run with a libgimp version\n"
         "other than its own. This is GIMP %d.%d.%d, but the\n"
         "libgimp version is %d.%d.%d.\n\n"
         "Maybe you have GIMP versions in both /usr and /usr/local ?",
         GIMP_MAJOR_VERSION, GIMP_MINOR_VERSION, GIMP_MICRO_VERSION,
         gimp_major_version, gimp_minor_version, gimp_micro_version);
    }

  return NULL;
}

static gchar *
sanity_check_glib (void)
{
#define GLIB_REQUIRED_MAJOR 2
#define GLIB_REQUIRED_MINOR 70
#define GLIB_REQUIRED_MICRO 0

  const gchar *mismatch = glib_check_version (GLIB_REQUIRED_MAJOR,
                                              GLIB_REQUIRED_MINOR,
                                              GLIB_REQUIRED_MICRO);

  if (mismatch)
    {
      return g_strdup_printf
        ("%s\n\n"
         "GIMP requires GLib version %d.%d.%d or later.\n"
         "Installed GLib version is %d.%d.%d.\n\n"
         "Somehow you or your software packager managed\n"
         "to install GIMP with an older GLib version.\n\n"
         "Please upgrade to GLib version %d.%d.%d or later.",
         mismatch,
         GLIB_REQUIRED_MAJOR, GLIB_REQUIRED_MINOR, GLIB_REQUIRED_MICRO,
         glib_major_version, glib_minor_version, glib_micro_version,
         GLIB_REQUIRED_MAJOR, GLIB_REQUIRED_MINOR, GLIB_REQUIRED_MICRO);
    }

#undef GLIB_REQUIRED_MAJOR
#undef GLIB_REQUIRED_MINOR
#undef GLIB_REQUIRED_MICRO

  return NULL;
}

static gchar *
sanity_check_cairo (void)
{
#define CAIRO_REQUIRED_MAJOR 1
#define CAIRO_REQUIRED_MINOR 14
#define CAIRO_REQUIRED_MICRO 0

  if (cairo_version () < CAIRO_VERSION_ENCODE (CAIRO_REQUIRED_MAJOR,
                                               CAIRO_REQUIRED_MINOR,
                                               CAIRO_REQUIRED_MICRO))
    {
      return g_strdup_printf
        ("The Cairo version being used is too old!\n\n"
         "GIMP requires Cairo version %d.%d.%d or later.\n"
         "Installed Cairo version is %s.\n\n"
         "Somehow you or your software packager managed\n"
         "to install GIMP with an older Cairo version.\n\n"
         "Please upgrade to Cairo version %d.%d.%d or later.",
         CAIRO_REQUIRED_MAJOR, CAIRO_REQUIRED_MINOR, CAIRO_REQUIRED_MICRO,
         cairo_version_string (),
         CAIRO_REQUIRED_MAJOR, CAIRO_REQUIRED_MINOR, CAIRO_REQUIRED_MICRO);
    }

#undef CAIRO_REQUIRED_MAJOR
#undef CAIRO_REQUIRED_MINOR
#undef CAIRO_REQUIRED_MICRO

  return NULL;
}

static gchar *
sanity_check_pango (void)
{
#define PANGO_REQUIRED_MAJOR 1
#define PANGO_REQUIRED_MINOR 44
#define PANGO_REQUIRED_MICRO 0

  const gchar *mismatch = pango_version_check (PANGO_REQUIRED_MAJOR,
                                               PANGO_REQUIRED_MINOR,
                                               PANGO_REQUIRED_MICRO);

  if (mismatch)
    {
      const gint pango_major_version = pango_version () / 100 / 100;
      const gint pango_minor_version = pango_version () / 100 % 100;
      const gint pango_micro_version = pango_version () % 100;

      return g_strdup_printf
        ("%s\n\n"
         "GIMP requires Pango version %d.%d.%d or later.\n"
         "Installed Pango version is %d.%d.%d.\n\n"
         "Somehow you or your software packager managed\n"
         "to install GIMP with an older Pango version.\n\n"
         "Please upgrade to Pango version %d.%d.%d or later.",
         mismatch,
         PANGO_REQUIRED_MAJOR, PANGO_REQUIRED_MINOR, PANGO_REQUIRED_MICRO,
         pango_major_version, pango_minor_version, pango_micro_version,
         PANGO_REQUIRED_MAJOR, PANGO_REQUIRED_MINOR, PANGO_REQUIRED_MICRO);
    }

#undef PANGO_REQUIRED_MAJOR
#undef PANGO_REQUIRED_MINOR
#undef PANGO_REQUIRED_MICRO

  return NULL;
}

static gchar *
sanity_check_fontconfig (void)
{
  const gint fc_version = FcGetVersion ();

#define FC_REQUIRED_MAJOR 2
#define FC_REQUIRED_MINOR 12
#define FC_REQUIRED_MICRO 4

  if (fc_version < ((FC_REQUIRED_MAJOR * 10000) +
                    (FC_REQUIRED_MINOR *   100) +
                    (FC_REQUIRED_MICRO *     1)))
    {
      const gint fc_major_version = fc_version / 100 / 100;
      const gint fc_minor_version = fc_version / 100 % 100;
      const gint fc_micro_version = fc_version % 100;

      return g_strdup_printf
        ("The Fontconfig version being used is too old!\n\n"
         "GIMP requires Fontconfig version %d.%d.%d or later.\n"
         "The Fontconfig version loaded by GIMP is %d.%d.%d.\n\n"
         "This may be caused by another instance of libfontconfig.so.1\n"
         "being installed in the system, probably in /usr/X11R6/lib.\n"
         "Please correct the situation or report it to someone who can.",
         FC_REQUIRED_MAJOR, FC_REQUIRED_MINOR, FC_REQUIRED_MICRO,
         fc_major_version, fc_minor_version, fc_micro_version);
    }

#undef FC_REQUIRED_MAJOR
#undef FC_REQUIRED_MINOR
#undef FC_REQUIRED_MICRO

  return NULL;
}

static gchar *
sanity_check_freetype (void)
{
  FT_Library ft_library;
  FT_Int     ft_major_version;
  FT_Int     ft_minor_version;
  FT_Int     ft_micro_version;
  FT_Int     ft_version;

#define FT_REQUIRED_MAJOR 2
#define FT_REQUIRED_MINOR 1
#define FT_REQUIRED_MICRO 7

  if (FT_Init_FreeType (&ft_library) != 0)
    g_error ("FT_Init_FreeType() failed");

  FT_Library_Version (ft_library,
                      &ft_major_version,
                      &ft_minor_version,
                      &ft_micro_version);

  if (FT_Done_FreeType (ft_library) != 0)
    g_error ("FT_Done_FreeType() failed");

  ft_version = (ft_major_version * 10000 +
                ft_minor_version *   100 +
                ft_micro_version *     1);

  if (ft_version < ((FT_REQUIRED_MAJOR * 10000) +
                    (FT_REQUIRED_MINOR *   100) +
                    (FT_REQUIRED_MICRO *     1)))
    {
      return g_strdup_printf
        ("FreeType version too old!\n\n"
         "GIMP requires FreeType version %d.%d.%d or later.\n"
         "Installed FreeType version is %d.%d.%d.\n\n"
         "Somehow you or your software packager managed\n"
         "to install GIMP with an older FreeType version.\n\n"
         "Please upgrade to FreeType version %d.%d.%d or later.",
         FT_REQUIRED_MAJOR, FT_REQUIRED_MINOR, FT_REQUIRED_MICRO,
         ft_major_version, ft_minor_version, ft_micro_version,
         FT_REQUIRED_MAJOR, FT_REQUIRED_MINOR, FT_REQUIRED_MICRO);
    }

#undef FT_REQUIRED_MAJOR
#undef FT_REQUIRED_MINOR
#undef FT_REQUIRED_MICRO

  return NULL;
}

static gchar *
sanity_check_gdk_pixbuf (void)
{
#define GDK_PIXBUF_REQUIRED_MAJOR 2
#define GDK_PIXBUF_REQUIRED_MINOR 30
#define GDK_PIXBUF_REQUIRED_MICRO 8

  if (! sanity_check_version (gdk_pixbuf_major_version, GDK_PIXBUF_REQUIRED_MAJOR,
                              gdk_pixbuf_minor_version, GDK_PIXBUF_REQUIRED_MINOR,
                              gdk_pixbuf_micro_version, GDK_PIXBUF_REQUIRED_MICRO))
    {
      return g_strdup_printf
        ("GdkPixbuf version too old!\n\n"
         "GIMP requires GdkPixbuf version %d.%d.%d or later.\n"
         "Installed GdkPixbuf version is %d.%d.%d.\n\n"
         "Somehow you or your software packager managed\n"
         "to install GIMP with an older GdkPixbuf version.\n\n"
         "Please upgrade to GdkPixbuf version %d.%d.%d or later.",
         GDK_PIXBUF_REQUIRED_MAJOR, GDK_PIXBUF_REQUIRED_MINOR, GDK_PIXBUF_REQUIRED_MICRO,
         gdk_pixbuf_major_version, gdk_pixbuf_minor_version, gdk_pixbuf_micro_version,
         GDK_PIXBUF_REQUIRED_MAJOR, GDK_PIXBUF_REQUIRED_MINOR, GDK_PIXBUF_REQUIRED_MICRO);
    }

#undef GDK_PIXBUF_REQUIRED_MAJOR
#undef GDK_PIXBUF_REQUIRED_MINOR
#undef GDK_PIXBUF_REQUIRED_MICRO

  return NULL;
}

static gchar *
sanity_check_lcms (void)
{
#define LCMS_REQUIRED_MAJOR 2
#define LCMS_REQUIRED_MINOR 8

  gint lcms_version = cmsGetEncodedCMMversion ();

  if (lcms_version < (LCMS_REQUIRED_MAJOR * 1000 +
                      LCMS_REQUIRED_MINOR * 10))
    {
      const gint lcms_major_version = lcms_version / 1000;
      const gint lcms_minor_version = lcms_version % 1000 / 10;

      return g_strdup_printf
        ("Liblcms2 version too old!\n\n"
         "GIMP requires LittleCMS version %d.%d or later.\n"
         "Installed LittleCMS version is %d.%d.\n\n"
         "Somehow you or your software packager managed\n"
         "to install GIMP with an older LittleCMS version.\n\n"
         "Please upgrade to LittleCMS version %d.%d or later.",
         LCMS_REQUIRED_MAJOR, LCMS_REQUIRED_MINOR,
         lcms_major_version, lcms_minor_version,
         LCMS_REQUIRED_MAJOR, LCMS_REQUIRED_MINOR);
    }

#undef LCMS_REQUIRED_MAJOR
#undef LCMS_REQUIRED_MINOR

  return NULL;
}

static gchar *
sanity_check_gexiv2 (void)
{
#ifdef GEXIV2_MAJOR_VERSION

#define GEXIV2_REQUIRED_MAJOR 0
#define GEXIV2_REQUIRED_MINOR 14
#define GEXIV2_REQUIRED_MICRO 0

  gint gexiv2_version = gexiv2_get_version ();

  if (gexiv2_version < (GEXIV2_REQUIRED_MAJOR * 100 * 100 +
                        GEXIV2_REQUIRED_MINOR * 100 +
                        GEXIV2_REQUIRED_MICRO))
    {
      const gint gexiv2_major_version = gexiv2_version / 100 / 100;
      const gint gexiv2_minor_version = gexiv2_version / 100 % 100;
      const gint gexiv2_micro_version = gexiv2_version % 100;

      return g_strdup_printf
        ("gexiv2 version too old!\n\n"
         "GIMP requires gexiv2 version %d.%d.%d or later.\n"
         "Installed gexiv2 version is %d.%d.%d.\n\n"
         "Somehow you or your software packager managed\n"
         "to install GIMP with an older gexiv2 version.\n\n"
         "Please upgrade to gexiv2 version %d.%d.%d or later.",
         GEXIV2_REQUIRED_MAJOR, GEXIV2_REQUIRED_MINOR, GEXIV2_REQUIRED_MICRO,
         gexiv2_major_version, gexiv2_minor_version, gexiv2_micro_version,
         GEXIV2_REQUIRED_MAJOR, GEXIV2_REQUIRED_MINOR, GEXIV2_REQUIRED_MICRO);
    }

#undef GEXIV2_REQUIRED_MAJOR
#undef GEXIV2_REQUIRED_MINOR
#undef GEXIV2_REQUIRED_MICRO

#endif

  return NULL;
}

static gchar *
sanity_check_babl (void)
{
  gint babl_major_version;
  gint babl_minor_version;
  gint babl_micro_version;

#define BABL_REQUIRED_MAJOR 0
#define BABL_REQUIRED_MINOR 1
#define BABL_REQUIRED_MICRO 116

  babl_get_version (&babl_major_version,
                    &babl_minor_version,
                    &babl_micro_version);

  if (! sanity_check_version (babl_major_version, BABL_REQUIRED_MAJOR,
                              babl_minor_version, BABL_REQUIRED_MINOR,
                              babl_micro_version, BABL_REQUIRED_MICRO))
    {
      return g_strdup_printf
        ("BABL version too old!\n\n"
         "GIMP requires BABL version %d.%d.%d or later.\n"
         "Installed BABL version is %d.%d.%d.\n\n"
         "Somehow you or your software packager managed\n"
         "to install GIMP with an older BABL version.\n\n"
         "Please upgrade to BABL version %d.%d.%d or later.",
         BABL_REQUIRED_MAJOR, BABL_REQUIRED_MINOR, BABL_REQUIRED_MICRO,
         babl_major_version, babl_minor_version, babl_micro_version,
         BABL_REQUIRED_MAJOR, BABL_REQUIRED_MINOR, BABL_REQUIRED_MICRO);
    }

#undef BABL_REQUIRED_MAJOR
#undef BABL_REQUIRED_MINOR
#undef BABL_REQUIRED_MICRO

  return NULL;
}

static gchar *
sanity_check_gegl (void)
{
  gint gegl_major_version;
  gint gegl_minor_version;
  gint gegl_micro_version;

#define GEGL_REQUIRED_MAJOR 0
#define GEGL_REQUIRED_MINOR 4
#define GEGL_REQUIRED_MICRO 64

  gegl_get_version (&gegl_major_version,
                    &gegl_minor_version,
                    &gegl_micro_version);

  if (! sanity_check_version (gegl_major_version, GEGL_REQUIRED_MAJOR,
                              gegl_minor_version, GEGL_REQUIRED_MINOR,
                              gegl_micro_version, GEGL_REQUIRED_MICRO))
    {
      return g_strdup_printf
        ("GEGL version too old!\n\n"
         "GIMP requires GEGL version %d.%d.%d or later.\n"
         "Installed GEGL version is %d.%d.%d.\n\n"
         "Somehow you or your software packager managed\n"
         "to install GIMP with an older GEGL version.\n\n"
         "Please upgrade to GEGL version %d.%d.%d or later.",
         GEGL_REQUIRED_MAJOR, GEGL_REQUIRED_MINOR, GEGL_REQUIRED_MICRO,
         gegl_major_version, gegl_minor_version, gegl_micro_version,
         GEGL_REQUIRED_MAJOR, GEGL_REQUIRED_MINOR, GEGL_REQUIRED_MICRO);
    }

#undef GEGL_REQUIRED_MAJOR
#undef GEGL_REQUIRED_MINOR
#undef GEGL_REQUIRED_MICRO

  return NULL;
}

static gchar *
sanity_check_filename_encoding (void)
{
  gchar  *result;
  GError *error = NULL;

  result = g_filename_to_utf8 ("", -1, NULL, NULL, &error);

  if (! result)
    {
      gchar *msg =
        g_strdup_printf
        (_("The configured filename encoding cannot be converted to UTF-8: "
           "%s\n\n"
           "Please check the value of the environment variable "
           "G_FILENAME_ENCODING."),
         error->message);

      g_error_free (error);

      return msg;
    }

  g_free (result);

  result = g_filename_to_utf8 (gimp_directory (), -1, NULL, NULL, &error);

  if (! result)
    {
      gchar *msg =
        g_strdup_printf
        (_("The name of the directory holding the GIMP user configuration "
           "cannot be converted to UTF-8: "
           "%s\n\n"
           "Your filesystem probably stores files in an encoding "
           "other than UTF-8 and you didn't tell GLib about this. "
           "Please set the environment variable G_FILENAME_ENCODING."),
         error->message);

      g_error_free (error);

      return msg;
    }

  g_free (result);

  return NULL;
}


/*  late-stage tests  */

static gchar *
sanity_check_gegl_ops (void)
{
  static const gchar *required_ops[] =
  {
    "gegl:alien-map",
    "gegl:bayer-matrix",
    "gegl:bloom",
    "gegl:buffer-sink",
    "gegl:buffer-source",
    "gegl:c2g",
    "gegl:cache",
    "gegl:cartoon",
    "gegl:cell-noise",
    "gegl:checkerboard",
    "gegl:color",
    "gegl:color-enhance",
    "gegl:color-exchange",
    "gegl:color-rotate",
    "gegl:color-temperature",
    "gegl:color-to-alpha",
    "gegl:component-extract",
    "gegl:convolution-matrix",
    "gegl:copy-buffer",
    "gegl:crop",
    "gegl:cubism",
    "gegl:deinterlace",
    "gegl:difference-of-gaussians",
    "gegl:diffraction-patterns",
    "gegl:displace",
    "gegl:distance-transform",
    "gegl:dither",
    "gegl:dropshadow",
    "gegl:edge",
    "gegl:edge-laplace",
    "gegl:edge-neon",
    "gegl:edge-sobel",
    "gegl:emboss",
    "gegl:engrave",
    "gegl:exposure",
    "gegl:fattal02",
    "gegl:focus-blur",
    "gegl:fractal-trace",
    "gegl:gaussian-blur",
    "gegl:gaussian-blur-selective",
    "gegl:gegl",
    "gegl:grid",
    "gegl:high-pass",
    "gegl:hue-chroma",
    "gegl:illusion",
    "gegl:image-gradient",
    "gegl:invert-gamma",
    "gegl:invert-linear",
    "gegl:lens-blur",
    "gegl:lens-distortion",
    "gegl:lens-flare",
    "gegl:linear-sinusoid",
    "gegl:long-shadow",
    "gegl:mantiuk06",
    "gegl:map-absolute",
    "gegl:map-relative",
    "gegl:matting-global",
    "gegl:maze",
    "gegl:mean-curvature-blur",
    "gegl:median-blur",
    "gegl:mirrors",
    "gegl:mono-mixer",
    "gegl:mosaic",
    "gegl:motion-blur-circular",
    "gegl:motion-blur-linear",
    "gegl:motion-blur-zoom",
    "gegl:newsprint",
    "gegl:noise-cie-lch",
    "gegl:noise-hsv",
    "gegl:noise-hurl",
    "gegl:noise-pick",
    "gegl:noise-rgb",
    "gegl:noise-slur",
    "gegl:noise-solid",
    "gegl:noise-spread",
    "gegl:normal-map",
    "gegl:npd",
    "gegl:oilify",
    "gegl:opacity",
    "gegl:over",
    "gegl:panorama-projection",
    "gegl:perlin-noise",
    "gegl:photocopy",
    "gegl:pixelize",
    "gegl:polar-coordinates",
    "gegl:recursive-transform",
    "gegl:red-eye-removal",
    "gegl:reinhard05",
    "gegl:rgb-clip",
    "gegl:ripple",
    "gegl:saturation",
    "gegl:scale-ratio",
    "gegl:seamless-clone",
    "gegl:sepia",
    "gegl:shadows-highlights",
    "gegl:shift",
    "gegl:simplex-noise",
    "gegl:shift",
    "gegl:sinus",
    "gegl:slic",
    "gegl:snn-mean",
    "gegl:softglow",
    "gegl:spherize",
    "gegl:spiral",
    "gegl:stereographic-projection",
    "gegl:stretch-contrast",
    "gegl:stretch-contrast-hsv",
    "gegl:stress",
    "gegl:supernova",
    "gegl:threshold",
    "gegl:tile",
    "gegl:tile-paper",
    "gegl:tile-glass",
    "gegl:tile-seamless",
    "gegl:transform",
    "gegl:translate",
    "gegl:unsharp-mask",
    "gegl:value-invert",
    "gegl:value-propagate",
    "gegl:variable-blur",
    "gegl:video-degradation",
    "gegl:vignette",
    "gegl:warp",
    "gegl:waterpixels",
    "gegl:wavelet-blur",
    "gegl:waves",
    "gegl:whirl-pinch",
    "gegl:write-buffer"
  };

  gint i;

  for (i = 0; i < G_N_ELEMENTS (required_ops); i++)
    {
      if (! gegl_has_operation (required_ops[i]))
        {
          return g_strdup_printf
            ("GEGL operation missing!\n\n"
             "GIMP requires the GEGL operation \"%s\".\n"
             "This operation cannot be found. Check your\n"
             "GEGL install and ensure it has been compiled\n"
             "with any dependencies required for GIMP.",
             required_ops [i]);
        }
    }

  return NULL;
}
