/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Multiple-image Network Graphics (MNG) plug-in
 *
 * Copyright (C) 2002 Mukund Sivaraman <muks@mukund.org>
 * Portions are copyright of the authors of the file-gif-save, file-png-save
 * and file-jpeg-save plug-ins' code. The exact ownership of these code
 * fragments has not been determined.
 *
 * This work was sponsored by Xinit Systems Limited, UK.
 * http://www.xinitsystems.com/
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * --
 *
 * For now, this MNG plug-in can only save images. It cannot load images.
 * Save your working copy as .xcf and use this for "exporting" your images
 * to MNG. Supports animation the same way as animated GIFs. Supports alpha
 * transparency. Uses the libmng library (http://www.libmng.com/).
 * The MIME content-type for MNG images is video/x-mng for now. Make sure
 * your web-server is configured appropriately if you want to serve MNG
 * images.
 *
 * Since libmng cannot write PNG, JNG and delta PNG chunks at this time
 * (when this text was written), this plug-in uses libpng and jpeglib to
 * create the data for the chunks.
 *
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib/gstdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


/* libpng and jpeglib are currently used in this plug-in. */

#include <png.h>
#include <jpeglib.h>


/* Grrr. The grrr is because the following have to be defined
 * by the application as well for some reason, although they
 * were enabled when libmng was compiled. The authors of libmng
 * must look into this. */

#if !defined(MNG_SUPPORT_FULL)
#define MNG_SUPPORT_FULL 1
#endif

#if !defined(MNG_SUPPORT_READ)
#define MNG_SUPPORT_READ 1
#endif

#if !defined(MNG_SUPPORT_WRITE)
#define MNG_SUPPORT_WRITE 1
#endif

#if !defined(MNG_SUPPORT_DISPLAY)
#define MNG_SUPPORT_DISPLAY 1
#endif

#if !defined(MNG_ACCESS_CHUNKS)
#define MNG_ACCESS_CHUNKS 1
#endif

#include <libmng.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimp/stdplugins-intl.h"


#define SAVE_PROC      "file-mng-save"
#define PLUG_IN_BINARY "file-mng"
#define PLUG_IN_ROLE   "gimp-file-mng"
#define SCALE_WIDTH    125

enum
{
  CHUNKS_PNG_D,
  CHUNKS_JNG_D,
  CHUNKS_PNG,
  CHUNKS_JNG
};

enum
{
  DISPOSE_COMBINE,
  DISPOSE_REPLACE
};


/* The contents of this struct remain static among multiple
 * invocations of the plug-in. */

/* TODO: describe the members of the struct */

struct mng_data_t
{
  gint32 interlaced;
  gint32 bkgd;
  gint32 gama;
  gint32 phys;
  gint32 time;
  gint32 default_chunks;

  gfloat quality;
  gfloat smoothing;

  gint32 compression_level;

  gint32 loop;
  gint32 default_delay;
  gint32 default_dispose;
};

/* Values of the instance of the above struct when the plug-in is
 * first invoked. */

static struct mng_data_t mng_data =
{
  FALSE,                        /* interlaced */
  FALSE,                        /* bkgd */
  FALSE,                        /* gama */
  TRUE,                         /* phys */
  TRUE,                         /* time */
  CHUNKS_PNG_D,                 /* default_chunks */

  0.75,                         /* quality */
  0.0,                          /* smoothing */

  9,                            /* compression_level */

  TRUE,                         /* loop */
  100,                          /* default_delay */
  DISPOSE_COMBINE               /* default_dispose */
};


/* These are not saved or restored. */

struct mng_globals_t
{
  gboolean   has_trns;
  png_bytep  trans;
  int        num_trans;
  gboolean   has_plte;
  png_colorp palette;
  int        num_palette;
};

static struct mng_globals_t mngg;


/* The output FILE pointer which is used by libmng;
 * passed around as user data. */
struct mnglib_userdata_t
{
  FILE *fp;
};


/*
 * Function prototypes
 */

static mng_ptr   myalloc       (mng_size_t  size);
static void      myfree        (mng_ptr     ptr,
                                mng_size_t  size);
static mng_bool  myopenstream  (mng_handle  handle);
static mng_bool  myclosestream (mng_handle  handle);
static mng_bool  mywritedata   (mng_handle  handle,
                                mng_ptr     buf,
                                mng_uint32  size,
                                mng_uint32 *written_size);


static gint32    parse_chunks_type_from_layer_name   (const gchar *str);
static gint32    parse_disposal_type_from_layer_name (const gchar *str);
static gint32    parse_ms_tag_from_layer_name        (const gchar *str);
static gint      find_unused_ia_color               (guchar      *pixels,
                                                      gint         numpixels,
                                                      gint        *colors);
static gboolean  ia_has_transparent_pixels           (guchar      *pixels,
                                                      gint         numpixels);

static gboolean  respin_cmap     (png_structp       png_ptr,
                                  png_infop         png_info_ptr,
                                  guchar           *remap,
                                  gint32            image_id,
                                  GeglBuffer       *buffer,
                                  int              *bit_depth);

static gboolean  mng_save_image  (const gchar      *filename,
                                  gint32            image_id,
                                  gint32            drawable_id,
                                  gint32            original_image_id,
                                  GError          **error);
static gboolean  mng_save_dialog (gint32            image_id);

static void      query           (void);
static void      run             (const gchar      *name,
                                  gint              nparams,
                                  const GimpParam  *param,
                                  gint             *nreturn_vals,
                                  GimpParam       **return_vals);


/*
 * Callbacks for libmng
 */

static mng_ptr
myalloc (mng_size_t size)
{
  gpointer ptr;

  ptr = g_try_malloc (size);

  if (ptr != NULL)
    memset (ptr, 0, size);

  return ((mng_ptr) ptr);
}

static void
myfree (mng_ptr    ptr,
        mng_size_t size)
{
  g_free (ptr);
}

static mng_bool
myopenstream (mng_handle handle)
{
  return MNG_TRUE;
}

static mng_bool
myclosestream (mng_handle handle)
{
  return MNG_TRUE;
}

static mng_bool
mywritedata (mng_handle  handle,
             mng_ptr     buf,
             mng_uint32  size,
             mng_uint32 *written_size)
{
  struct mnglib_userdata_t *userdata =
    (struct mnglib_userdata_t *) mng_get_userdata (handle);


  *written_size = (mng_uint32) fwrite ((void *) buf, 1,
                                       (size_t) size, userdata->fp);

  return MNG_TRUE;
}


/* Parses which output chunk type to use for this layer
 * from the layer name. */

static gint32
parse_chunks_type_from_layer_name (const gchar *str)
{
  guint i;

  for (i = 0; (i + 5) <= strlen (str); i++)
    {
      if (g_ascii_strncasecmp (str + i, "(png)", 5) == 0)
        return CHUNKS_PNG;
      else if (g_ascii_strncasecmp (str + i, "(jng)", 5) == 0)
        return CHUNKS_JNG;
    }

  return mng_data.default_chunks;
}


/* Parses which disposal type to use for this layer
 * from the layer name. */

static gint32
parse_disposal_type_from_layer_name (const gchar *str)
{
  guint i;

  for (i = 0; (i + 9) <= strlen (str); i++)
    {
      if (g_ascii_strncasecmp (str + i, "(combine)", 9) == 0)
        return DISPOSE_COMBINE;
      else if (g_ascii_strncasecmp (str + i, "(replace)", 9) == 0)
        return DISPOSE_REPLACE;
    }

  return mng_data.default_dispose;
}


/* Parses the millisecond delay to use for this layer
 * from the layer name. */

static gint32
parse_ms_tag_from_layer_name (const gchar *str)
{
  guint offset = 0;
  gint32 sum = 0;
  guint length = strlen (str);

  while (TRUE)
    {
      while ((offset < length) && (str[offset] != '('))
        offset++;

      if (offset >= length)
        return mng_data.default_delay;

      offset++;

      if (g_ascii_isdigit (str[offset]))
        break;
    }

  do
    {
      sum *= 10;
      sum += str[offset] - '0';
      offset++;
    }
  while ((offset < length) && (g_ascii_isdigit (str[offset])));

  if ((length - offset) <= 2)
    return mng_data.default_delay;

  if ((g_ascii_toupper (str[offset]) != 'M') ||
      (g_ascii_toupper (str[offset + 1]) != 'S'))
    return mng_data.default_delay;

  return sum;
}


/* Try to find a color in the palette which isn't actually
 * used in the image, so that we can use it as the transparency
 * index. Taken from png.c */
static gint
find_unused_ia_color (guchar *pixels,
                       gint    numpixels,
                       gint   *colors)
{
  gint     i;
  gboolean ix_used[256];
  gboolean trans_used = FALSE;

  for (i = 0; i < *colors; i++)
    {
      ix_used[i] = FALSE;
    }

  for (i = 0; i < numpixels; i++)
    {
      /* If alpha is over a threshold, the color index in the
       * palette is taken. Otherwise, this pixel is transparent. */
      if (pixels[i * 2 + 1] > 127)
        ix_used[pixels[i * 2]] = TRUE;
      else
        trans_used = TRUE;
    }

  /* If there is no transparency, ignore alpha. */
  if (trans_used == FALSE)
    return -1;

  for (i = 0; i < *colors; i++)
    {
      if (ix_used[i] == FALSE)
        {
          return i;
        }
    }

  /* Couldn't find an unused color index within the number of
     bits per pixel we wanted.  Will have to increment the number
     of colors in the image and assign a transparent pixel there. */
  if ((*colors) < 256)
    {
      (*colors)++;
      return ((*colors) - 1);
    }

  return -1;
}


static gboolean
ia_has_transparent_pixels (guchar *pixels,
                           gint    numpixels)
{
  while (numpixels --)
    {
      if (pixels [1] <= 127)
        return TRUE;
      pixels += 2;
    }
  return FALSE;
}

static int
get_bit_depth_for_palette (int num_palette)
{
  if (num_palette <= 2)
    return 1;
  else if (num_palette <= 4)
    return 2;
  else if (num_palette <= 16)
    return 4;
  else
    return 8;
}

/* Spins the color map (palette) putting the transparent color at
 * index 0 if there is space. If there isn't any space, warn the user
 * and forget about transparency. Returns TRUE if the colormap has
 * been changed and FALSE otherwise.
 */

static gboolean
respin_cmap (png_structp  pp,
             png_infop    info,
             guchar      *remap,
             gint32       image_id,
             GeglBuffer  *buffer,
             int         *bit_depth)
{
  static guchar  trans[] = { 0 };
  guchar        *before;
  guchar        *pixels;
  gint           numpixels;
  gint           colors;
  gint           transparent;
  gint           cols, rows;

  before = gimp_image_get_colormap (image_id, &colors);

  /* Make sure there is something in the colormap */
  if (colors == 0)
    {
      before = g_newa (guchar, 3);
      memset (before, 0, sizeof (guchar) * 3);

      colors = 1;
    }

  cols      = gegl_buffer_get_width  (buffer);
  rows      = gegl_buffer_get_height (buffer);
  numpixels = cols * rows;

  pixels = (guchar *) g_malloc (numpixels * 2);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, cols, rows), 1.0,
                   NULL, pixels,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (ia_has_transparent_pixels (pixels, numpixels))
    {
      transparent = find_unused_ia_color (pixels, numpixels, &colors);

      if (transparent != -1)
        {
          static png_color palette[256] = { {0, 0, 0} };
          gint i;

          /* Set tRNS chunk values for writing later. */
          mngg.has_trns = TRUE;
          mngg.trans = trans;
          mngg.num_trans = 1;

          /* Transform all pixels with a value = transparent to
           * 0 and vice versa to compensate for re-ordering in palette
           * due to png_set_tRNS().
           */

          remap[0] = transparent;
          remap[transparent] = 0;

          /* Copy from index 0 to index transparent - 1 to index 1 to
           * transparent of after, then from transparent+1 to colors-1
           * unchanged, and finally from index transparent to index 0.
           */

          for (i = 1; i < colors; i++)
            {
              palette[i].red = before[3 * remap[i]];
              palette[i].green = before[3 * remap[i] + 1];
              palette[i].blue = before[3 * remap[i] + 2];
            }

          /* Set PLTE chunk values for writing later. */
          mngg.has_plte = TRUE;
          mngg.palette = palette;
          mngg.num_palette = colors;

          *bit_depth = get_bit_depth_for_palette (colors);

          return TRUE;
        }
      else
        {
          g_message (_("Couldn't losslessly save transparency, "
                       "saving opacity instead."));
        }
    }

  mngg.has_plte = TRUE;
  mngg.palette = (png_colorp) before;
  mngg.num_palette = colors;
  *bit_depth = get_bit_depth_for_palette (colors);

  return FALSE;
}

static mng_retcode
mng_putchunk_plte_wrapper (mng_handle    handle,
                           gint          numcolors,
                           const guchar *colormap)
{
  mng_palette8 palette;

  memset (palette, 0, sizeof palette);
  if (0 < numcolors)
    memcpy (palette, colormap, numcolors * sizeof palette[0]);

  return mng_putchunk_plte (handle, numcolors, palette);
}

static mng_retcode
mng_putchunk_trns_wrapper (mng_handle    handle,
                           gint          n_alphas,
                           const guchar *buffer)
{
  const mng_bool mng_global = TRUE;
  const mng_bool mng_empty  = TRUE;
  mng_uint8arr   alphas;

  memset (alphas, 0, sizeof alphas);
  if (buffer && 0 < n_alphas)
    memcpy (alphas, buffer, n_alphas * sizeof alphas[0]);

  return mng_putchunk_trns (handle,
                            ! mng_empty,
                            ! mng_global,
                            MNG_COLORTYPE_INDEXED,
                            n_alphas,
                            alphas,
                            0, 0, 0, 0, 0, alphas);
}

static gboolean
mng_save_image (const gchar  *filename,
                gint32        image_id,
                gint32        drawable_id,
                gint32        original_image_id,
                GError      **error)
{
  gboolean        ret = FALSE;
  gint            rows, cols;
  volatile gint   i;
  time_t          t;
  struct tm      *gmt;

  gint            num_layers;
  gint32         *layers;

  struct mnglib_userdata_t *userdata;
  mng_handle      handle;
  guint32         mng_ticks_per_second;
  guint32         mng_simplicity_profile;

  layers = gimp_image_get_layers (image_id, &num_layers);

  if (num_layers < 1)
    return FALSE;

  if (num_layers > 1)
    mng_ticks_per_second = 1000;
  else
    mng_ticks_per_second = 0;

  rows = gimp_image_height (image_id);
  cols = gimp_image_width (image_id);

  mng_simplicity_profile = (MNG_SIMPLICITY_VALID |
                            MNG_SIMPLICITY_SIMPLEFEATURES |
                            MNG_SIMPLICITY_COMPLEXFEATURES);

  /* JNG and delta-PNG chunks exist */
  mng_simplicity_profile |= (MNG_SIMPLICITY_JNG |
                             MNG_SIMPLICITY_DELTAPNG);

  for (i = 0; i < num_layers; i++)
    if (gimp_drawable_has_alpha (layers[i]))
      {
        /* internal transparency exists */
        mng_simplicity_profile |= MNG_SIMPLICITY_TRANSPARENCY;

        /* validity of following flags */
        mng_simplicity_profile |= 0x00000040;

        /* semi-transparency exists */
        mng_simplicity_profile |= 0x00000100;

        /* background transparency should happen */
        mng_simplicity_profile |= 0x00000080;

        break;
      }

  userdata = g_new0 (struct mnglib_userdata_t, 1);
  userdata->fp = g_fopen (filename, "wb");

  if (NULL == userdata->fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      goto err;
    }

  handle = mng_initialize ((mng_ptr) userdata, myalloc, myfree, NULL);
  if (NULL == handle)
    {
      g_warning ("Unable to mng_initialize() in mng_save_image()");
      goto err2;
    }

  if ((mng_setcb_openstream (handle, myopenstream) != MNG_NOERROR) ||
      (mng_setcb_closestream (handle, myclosestream) != MNG_NOERROR) ||
      (mng_setcb_writedata (handle, mywritedata) != MNG_NOERROR))
    {
      g_warning ("Unable to setup callbacks in mng_save_image()");
      goto err3;
    }

  if (mng_create (handle) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_create() image in mng_save_image()");
      goto err3;
    }

  if (mng_putchunk_mhdr (handle, cols, rows, mng_ticks_per_second, 1,
                         num_layers, mng_data.default_delay,
                         mng_simplicity_profile) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_putchunk_mhdr() in mng_save_image()");
      goto err3;
    }

  if ((num_layers > 1) && (mng_data.loop))
    {
      gint32 ms =
        parse_ms_tag_from_layer_name (gimp_item_get_name (layers[0]));

      if (mng_putchunk_term (handle, MNG_TERMACTION_REPEAT,
                             MNG_ITERACTION_LASTFRAME,
                             ms, 0x7fffffff) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_term() in mng_save_image()");
          goto err3;
        }
    }
  else
    {
      gint32 ms =
        parse_ms_tag_from_layer_name (gimp_item_get_name (layers[0]));

      if (mng_putchunk_term (handle, MNG_TERMACTION_LASTFRAME,
                             MNG_ITERACTION_LASTFRAME,
                             ms, 0x7fffffff) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_term() in mng_save_image()");
          goto err3;
        }
    }


  /* For now, we hardwire a comment */

  if (mng_putchunk_text (handle,
                         strlen (MNG_TEXT_TITLE), MNG_TEXT_TITLE,
                         18, "Created using GIMP") != MNG_NOERROR)
    {
      g_warning ("Unable to mng_putchunk_text() in mng_save_image()");
      goto err3;
    }

#if 0

  /* how do we get this to work? */
  if (mng_data.bkgd)
    {
      GimpRGB bgcolor;
      guchar red, green, blue;

      gimp_context_get_background (&bgcolor);
      gimp_rgb_get_uchar (&bgcolor, &red, &green, &blue);

      if (mng_putchunk_back (handle, red, green, blue,
                             MNG_BACKGROUNDCOLOR_MANDATORY,
                             0, MNG_BACKGROUNDIMAGE_NOTILE) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_back() in mng_save_image()");
          goto err3;
        }

      if (mng_putchunk_bkgd (handle, MNG_FALSE, 2, 0,
                             gimp_rgb_luminance_uchar (&bgcolor),
                             red, green, blue) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_bkgd() in mng_save_image()");
          goto err3;
        }
    }

#endif

  if (mng_data.gama)
    {
      if (mng_putchunk_gama (handle, MNG_FALSE,
                             (1.0 / (gimp_gamma ()) * 100000)) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_gama() in mng_save_image()");
          goto err3;
        }
    }

#if 0

  /* how do we get this to work? */
  if (mng_data.phys)
    {
      gimp_image_get_resolution(original_image_id, &xres, &yres);

      if (mng_putchunk_phyg (handle, MNG_FALSE,
                             (mng_uint32) (xres * 39.37),
                             (mng_uint32) (yres * 39.37), 1) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_phyg() in mng_save_image()");
          goto err3;
        }

      if (mng_putchunk_phys (handle, MNG_FALSE,
                             (mng_uint32) (xres * 39.37),
                             (mng_uint32) (yres * 39.37), 1) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_phys() in mng_save_image()");
          goto err3;
        }
    }

#endif

  if (mng_data.time)
    {
      t = time (NULL);
      gmt = gmtime (&t);

      if (mng_putchunk_time (handle, gmt->tm_year + 1900, gmt->tm_mon + 1,
                             gmt->tm_mday, gmt->tm_hour, gmt->tm_min,
                             gmt->tm_sec) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_time() in mng_save_image()");
          goto err3;
        }
    }

  if (gimp_image_base_type (image_id) == GIMP_INDEXED)
    {
      guchar *palette;
      gint    numcolors;

      palette = gimp_image_get_colormap (image_id, &numcolors);

      if ((numcolors != 0) &&
          (mng_putchunk_plte_wrapper (handle, numcolors,
                                      palette) != MNG_NOERROR))
        {
          g_warning ("Unable to mng_putchunk_plte() in mng_save_image()");
          goto err3;
        }
    }

  for (i = (num_layers - 1); i >= 0; i--)
    {
      GimpImageType   layer_drawable_type;
      GeglBuffer     *layer_buffer;
      gint            layer_offset_x, layer_offset_y;
      gint            layer_rows, layer_cols;
      gchar          *layer_name;
      gint            layer_chunks_type;
      const Babl     *layer_format;
      volatile gint   layer_bpp;

      guint8          __attribute__((unused))layer_mng_colortype;
      guint8          __attribute__((unused))layer_mng_compression_type;
      guint8          __attribute__((unused))layer_mng_interlace_type;
      gboolean        layer_has_unique_palette;

      gchar           frame_mode;
      int             frame_delay;
      gchar          *temp_file_name;
      png_structp     pp;
      png_infop       info;
      FILE           *infile, *outfile;
      int             num_passes;
      int             tile_height;
      guchar        **layer_pixels, *layer_pixel;
      int             pass, j, k, begin, end, num;
      guchar         *fixed;
      guchar          layer_remap[256];
      int             color_type;
      int             bit_depth;

      layer_name          = gimp_item_get_name (layers[i]);
      layer_chunks_type   = parse_chunks_type_from_layer_name (layer_name);
      layer_drawable_type = gimp_drawable_type (layers[i]);

      layer_buffer        = gimp_drawable_get_buffer (layers[i]);
      layer_rows          = gegl_buffer_get_width  (layer_buffer);
      layer_cols          = gegl_buffer_get_height (layer_buffer);

      gimp_drawable_offsets (layers[i], &layer_offset_x, &layer_offset_y);
      layer_has_unique_palette = TRUE;

      for (j = 0; j < 256; j++)
        layer_remap[j] = j;

      switch (layer_drawable_type)
        {
        case GIMP_RGB_IMAGE:
          layer_format        = babl_format ("R'G'B' u8");
          layer_mng_colortype = MNG_COLORTYPE_RGB;
          break;
        case GIMP_RGBA_IMAGE:
          layer_format        = babl_format ("R'G'B'A u8");
          layer_mng_colortype = MNG_COLORTYPE_RGBA;
          break;
        case GIMP_GRAY_IMAGE:
          layer_format        = babl_format ("Y' u8");
          layer_mng_colortype = MNG_COLORTYPE_GRAY;
          break;
        case GIMP_GRAYA_IMAGE:
          layer_format        = babl_format ("Y'A u8");
          layer_mng_colortype = MNG_COLORTYPE_GRAYA;
          break;
        case GIMP_INDEXED_IMAGE:
          layer_format        = gegl_buffer_get_format (layer_buffer);
          layer_mng_colortype = MNG_COLORTYPE_INDEXED;
          break;
        case GIMP_INDEXEDA_IMAGE:
          layer_format        = gegl_buffer_get_format (layer_buffer);
          layer_mng_colortype = MNG_COLORTYPE_INDEXED | MNG_COLORTYPE_GRAYA;
          break;
        default:
          g_warning ("Unsupported GimpImageType in mng_save_image()");
          goto err3;
        }

      layer_bpp = babl_format_get_bytes_per_pixel (layer_format);

      /* Delta PNG chunks are not yet supported */

      /* if (i == (num_layers - 1)) */
      {
        if (layer_chunks_type == CHUNKS_JNG_D)
          layer_chunks_type = CHUNKS_JNG;
        else if (layer_chunks_type == CHUNKS_PNG_D)
          layer_chunks_type = CHUNKS_PNG;
      }

      switch (layer_chunks_type)
        {
        case CHUNKS_PNG_D:
          layer_mng_compression_type = MNG_COMPRESSION_DEFLATE;
          if (mng_data.interlaced != 0)
            layer_mng_interlace_type = MNG_INTERLACE_ADAM7;
          else
            layer_mng_interlace_type = MNG_INTERLACE_NONE;
          break;
        case CHUNKS_JNG_D:
          layer_mng_compression_type = MNG_COMPRESSION_DEFLATE;
          if (mng_data.interlaced != 0)
            layer_mng_interlace_type = MNG_INTERLACE_ADAM7;
          else
            layer_mng_interlace_type = MNG_INTERLACE_NONE;
          break;
        case CHUNKS_PNG:
          layer_mng_compression_type = MNG_COMPRESSION_DEFLATE;
          if (mng_data.interlaced != 0)
            layer_mng_interlace_type = MNG_INTERLACE_ADAM7;
          else
            layer_mng_interlace_type = MNG_INTERLACE_NONE;
          break;
        case CHUNKS_JNG:
          layer_mng_compression_type = MNG_COMPRESSION_BASELINEJPEG;
          if (mng_data.interlaced != 0)
            layer_mng_interlace_type = MNG_INTERLACE_PROGRESSIVE;
          else
            layer_mng_interlace_type = MNG_INTERLACE_SEQUENTIAL;
          break;
        default:
          g_warning ("Huh? Programmer stupidity error "
                     "with 'layer_chunks_type'");
          goto err3;
        }

      if ((i == (num_layers - 1)) ||
          (parse_disposal_type_from_layer_name (layer_name) != DISPOSE_COMBINE))
        frame_mode = MNG_FRAMINGMODE_3;
      else
        frame_mode = MNG_FRAMINGMODE_1;

      frame_delay = parse_ms_tag_from_layer_name (layer_name);

      if (mng_putchunk_fram (handle, MNG_FALSE, frame_mode, 0, NULL,
                             MNG_CHANGEDELAY_DEFAULT,
                             MNG_CHANGETIMOUT_NO,
                             MNG_CHANGECLIPPING_DEFAULT,
                             MNG_CHANGESYNCID_NO,
                             frame_delay, 0, 0,
                             layer_offset_x,
                             layer_offset_x + layer_cols,
                             layer_offset_y,
                             layer_offset_y + layer_rows,
                             0, NULL) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_fram() in mng_save_image()");
          goto err3;
        }

      if ((layer_offset_x != 0) || (layer_offset_y != 0))
        {
          if (mng_putchunk_defi (handle, 0, 0, 1, 1, layer_offset_x,
                                 layer_offset_y, 1, layer_offset_x,
                                 layer_offset_x + layer_cols, layer_offset_y,
                                 layer_offset_y + layer_rows) != MNG_NOERROR)
            {
              g_warning ("Unable to mng_putchunk_defi() in mng_save_image()");
              goto err3;
            }
        }

      if ((temp_file_name = gimp_temp_name ("mng")) == NULL)
        {
          g_warning ("gimp_temp_name() failed in mng_save_image()");
          goto err3;
        }

      if ((outfile = g_fopen (temp_file_name, "wb")) == NULL)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Could not open '%s' for writing: %s"),
                       gimp_filename_to_utf8 (temp_file_name),
                       g_strerror (errno));
          g_unlink (temp_file_name);
          goto err3;
        }

      pp = png_create_write_struct (PNG_LIBPNG_VER_STRING,
                                    NULL, NULL, NULL);
      if (NULL == pp)
        {
          g_warning ("Unable to png_create_write_struct() in mng_save_image()");
          fclose (outfile);
          g_unlink (temp_file_name);
          goto err3;
        }

      info = png_create_info_struct (pp);
      if (NULL == info)
        {
          g_warning
            ("Unable to png_create_info_struct() in mng_save_image()");
          png_destroy_write_struct (&pp, NULL);
          fclose (outfile);
          g_unlink (temp_file_name);
          goto err3;
        }

      if (setjmp (png_jmpbuf (pp)) != 0)
        {
          g_warning ("HRM saving PNG in mng_save_image()");
          png_destroy_write_struct (&pp, &info);
          fclose (outfile);
          g_unlink (temp_file_name);
          goto err3;
        }

      png_init_io (pp, outfile);

      bit_depth = 8;

      switch (layer_drawable_type)
        {
        case GIMP_RGB_IMAGE:
          color_type = PNG_COLOR_TYPE_RGB;
          break;
        case GIMP_RGBA_IMAGE:
          color_type = PNG_COLOR_TYPE_RGB_ALPHA;
          break;
        case GIMP_GRAY_IMAGE:
          color_type = PNG_COLOR_TYPE_GRAY;
          break;
        case GIMP_GRAYA_IMAGE:
          color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
          break;
        case GIMP_INDEXED_IMAGE:
          color_type = PNG_COLOR_TYPE_PALETTE;
          mngg.has_plte = TRUE;
          mngg.palette = (png_colorp)
            gimp_image_get_colormap (image_id, &mngg.num_palette);
          bit_depth = get_bit_depth_for_palette (mngg.num_palette);
          break;
        case GIMP_INDEXEDA_IMAGE:
          color_type = PNG_COLOR_TYPE_PALETTE;
          layer_has_unique_palette =
            respin_cmap (pp, info, layer_remap,
                         image_id, layer_buffer,
                         &bit_depth);
          break;
        default:
          g_warning ("This can't be!\n");
          png_destroy_write_struct (&pp, &info);
          fclose (outfile);
          g_unlink (temp_file_name);
          goto err3;
        }

      /* Note: png_set_IHDR() must be called before any other
         png_set_*() functions. */
      png_set_IHDR (pp, info, layer_cols, layer_rows,
                    bit_depth,
                    color_type,
                    mng_data.interlaced ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
                    PNG_COMPRESSION_TYPE_BASE,
                    PNG_FILTER_TYPE_BASE);

      if (mngg.has_trns)
        {
          png_set_tRNS (pp, info, mngg.trans, mngg.num_trans, NULL);
        }

      if (mngg.has_plte)
        {
          png_set_PLTE (pp, info, mngg.palette, mngg.num_palette);
        }

      png_set_compression_level (pp, mng_data.compression_level);

      png_write_info (pp, info);

      if (mng_data.interlaced != 0)
        num_passes = png_set_interlace_handling (pp);
      else
        num_passes = 1;

      if ((color_type == PNG_COLOR_TYPE_PALETTE) &&
          (bit_depth < 8))
        png_set_packing (pp);

      tile_height = gimp_tile_height ();
      layer_pixel = g_new (guchar, tile_height * layer_cols * layer_bpp);
      layer_pixels = g_new (guchar *, tile_height);

      for (j = 0; j < tile_height; j++)
        layer_pixels[j] = layer_pixel + (layer_cols * layer_bpp * j);

      for (pass = 0; pass < num_passes; pass++)
        {
          for (begin = 0, end = tile_height;
               begin < layer_rows;
               begin += tile_height, end += tile_height)
            {
              if (end > layer_rows)
                end = layer_rows;

              num = end - begin;

              gegl_buffer_get (layer_buffer,
                               GEGL_RECTANGLE (0, begin, layer_cols, num), 1.0,
                               layer_format, layer_pixel,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

              if (png_get_valid (pp, info, PNG_INFO_tRNS))
                {
                  for (j = 0; j < num; j++)
                    {
                      fixed = layer_pixels[j];

                      for (k = 0; k < layer_cols; k++)
                        fixed[k] = (fixed[k * 2 + 1] > 127) ?
                          layer_remap[fixed[k * 2]] : 0;
                    }
                }
              else if (png_get_valid (pp, info, PNG_INFO_PLTE)
                       && (layer_bpp == 2))
                {
                  for (j = 0; j < num; j++)
                    {
                      fixed = layer_pixels[j];

                      for (k = 0; k < layer_cols; k++)
                        fixed[k] = fixed[k * 2];
                    }
                }

              png_write_rows (pp, layer_pixels, num);
            }
        }

      g_object_unref (layer_buffer);

      png_write_end (pp, info);
      png_destroy_write_struct (&pp, &info);

      g_free (layer_pixels);
      g_free (layer_pixel);

      fclose (outfile);

      infile = g_fopen (temp_file_name, "rb");
      if (NULL == infile)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Could not open '%s' for reading: %s"),
                       gimp_filename_to_utf8 (temp_file_name),
                       g_strerror (errno));
          g_unlink (temp_file_name);
          goto err3;
        }

      fseek (infile, 8L, SEEK_SET);

      while (!feof (infile))
        {
          guchar  chunksize_chars[4];
          gulong  chunksize;
          gchar   chunkname[5];
          guchar *chunkbuffer;
          glong   chunkwidth;
          glong   chunkheight;
          gchar   chunkbitdepth;
          gchar   chunkcolortype;
          gchar   chunkcompression;
          gchar   chunkfilter;
          gchar   chunkinterlaced;


          if (fread (chunksize_chars, 1, 4, infile) != 4)
            break;

          if (fread (chunkname, 1, 4, infile) != 4)
            break;

          chunkname[4] = 0;

          chunksize = ((chunksize_chars[0] << 24) |
                       (chunksize_chars[1] << 16) |
                       (chunksize_chars[2] << 8) |
                       (chunksize_chars[3]));

          chunkbuffer = NULL;

          if (chunksize > 0)
            {
              chunkbuffer = g_new (guchar, chunksize);

              if (fread (chunkbuffer, 1, chunksize, infile) != chunksize)
                break;
            }

          if (strncmp (chunkname, "IHDR", 4) == 0)
            {
              chunkwidth = ((chunkbuffer[0] << 24) |
                            (chunkbuffer[1] << 16) |
                            (chunkbuffer[2] << 8) |
                            (chunkbuffer[3]));

              chunkheight = ((chunkbuffer[4] << 24) |
                             (chunkbuffer[5] << 16) |
                             (chunkbuffer[6] << 8) |
                             (chunkbuffer[7]));

              chunkbitdepth = chunkbuffer[8];
              chunkcolortype = chunkbuffer[9];
              chunkcompression = chunkbuffer[10];
              chunkfilter = chunkbuffer[11];
              chunkinterlaced = chunkbuffer[12];

              if (mng_putchunk_ihdr (handle, chunkwidth, chunkheight,
                                     chunkbitdepth, chunkcolortype,
                                     chunkcompression, chunkfilter,
                                     chunkinterlaced) != MNG_NOERROR)
                {
                  g_warning ("Unable to mng_putchunk_ihdr() "
                             "in mng_save_image()");
                  goto err3;
                }
            }
          else if (strncmp (chunkname, "IDAT", 4) == 0)
            {
              if (mng_putchunk_idat (handle, chunksize,
                                     chunkbuffer) != MNG_NOERROR)
                {
                  g_warning ("Unable to mng_putchunk_idat() "
                             "in mng_save_image()");
                  goto err3;
                }
            }
          else if (strncmp (chunkname, "IEND", 4) == 0)
            {
              if (mng_putchunk_iend (handle) != MNG_NOERROR)
                {
                  g_warning ("Unable to mng_putchunk_iend() "
                             "in mng_save_image()");
                  goto err3;
                }
            }
          else if (strncmp (chunkname, "PLTE", 4) == 0)
            {
              /* If this frame's palette is the same as the global palette,
               * write a 0-color palette chunk.
               */
              if (mng_putchunk_plte_wrapper (handle,
                                             (layer_has_unique_palette ?
                                              (chunksize / 3) : 0),
                                             chunkbuffer) != MNG_NOERROR)
                {
                  g_warning ("Unable to mng_putchunk_plte() "
                             "in mng_save_image()");
                  goto err3;
                }
            }
          else if (strncmp (chunkname, "tRNS", 4) == 0)
            {
              if (mng_putchunk_trns_wrapper (handle,
                                             chunksize,
                                             chunkbuffer) != MNG_NOERROR)
                {
                  g_warning ("Unable to mng_putchunk_trns() "
                             "in mng_save_image()");
                  goto err3;
                }
            }

          if (chunksize > 0)
            g_free (chunkbuffer);

          /* read 4 bytes after the chunk */

          fread (chunkname, 1, 4, infile);
        }

      fclose (infile);
      g_unlink (temp_file_name);
    }

  if (mng_putchunk_mend (handle) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_putchunk_mend() in mng_save_image()");
      goto err3;
    }

  if (mng_write (handle) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_write() the image in mng_save_image()");
      goto err3;
    }

  ret = TRUE;

 err3:
  mng_cleanup (&handle);
 err2:
  fclose (userdata->fp);
 err:
  g_free (userdata);

  return ret;
}


/* The interactive dialog. */

static gboolean
mng_save_dialog (gint32 image_id)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *table;
  GtkWidget     *toggle;
  GtkWidget     *hbox;
  GtkWidget     *combo;
  GtkWidget     *label;
  GtkWidget     *scale;
  GtkAdjustment *scale_adj;
  GtkWidget     *spinbutton;
  GtkAdjustment *spinbutton_adj;
  gint           num_layers;
  gboolean       run;

  dialog = gimp_export_dialog_new (_("MNG"), PLUG_IN_BINARY, SAVE_PROC);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      main_vbox, TRUE, TRUE, 0);

  frame = gimp_frame_new (_("MNG Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label (_("Interlace"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mng_data.interlaced);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                mng_data.interlaced);

  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Save background color"));
  gtk_widget_set_sensitive (toggle, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mng_data.bkgd);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mng_data.bkgd);

  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Save gamma"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mng_data.gama);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mng_data.gama);

  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Save resolution"));
  gtk_widget_set_sensitive (toggle, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mng_data.phys);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mng_data.phys);

  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Save creation time"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mng_data.time);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mng_data.time);

  gtk_widget_show (toggle);

  table = gtk_table_new (2, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  gimp_image_get_layers (image_id, &num_layers);

  if (num_layers == 1)
    combo = gimp_int_combo_box_new (_("PNG"), CHUNKS_PNG_D,
                                    _("JNG"), CHUNKS_JNG_D,
                                    NULL);
  else
    combo = gimp_int_combo_box_new (_("PNG + delta PNG"), CHUNKS_PNG_D,
                                    _("JNG + delta PNG"), CHUNKS_JNG_D,
                                    _("All PNG"),         CHUNKS_PNG,
                                    _("All JNG"),         CHUNKS_JNG,
                                    NULL);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 mng_data.default_chunks);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &mng_data.default_chunks);

  gtk_widget_set_sensitive (combo, FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Default chunks type:"), 0.0, 0.5,
                             combo, 1, FALSE);

  combo = gimp_int_combo_box_new (_("Combine"), DISPOSE_COMBINE,
                                  _("Replace"), DISPOSE_REPLACE,
                                  NULL);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 mng_data.default_dispose);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &mng_data.default_dispose);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Default frame disposal:"), 0.0, 0.5,
                             combo, 1, FALSE);

  scale_adj = gtk_adjustment_new (mng_data.compression_level,
                                  0.0, 9.0, 1.0, 1.0, 0.0);

  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, scale_adj);
  gtk_widget_set_size_request (scale, SCALE_WIDTH, -1);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("PNG compression level:"), 0.0, 0.9,
                             scale, 1, FALSE);

  g_signal_connect (scale_adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &mng_data.compression_level);

  gimp_help_set_help_data (scale,
                           _("Choose a high compression level "
                             "for small file size"),
                           NULL);

  scale_adj = gtk_adjustment_new (mng_data.quality,
                                  0.0, 1.0, 0.01, 0.01, 0.0);

  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, scale_adj);
  gtk_widget_set_size_request (scale, SCALE_WIDTH, -1);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_widget_set_sensitive (scale, FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                             _("JPEG compression quality:"), 0.0, 0.9,
                             scale, 1, FALSE);

  g_signal_connect (scale_adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &mng_data.quality);

  scale_adj = gtk_adjustment_new (mng_data.smoothing,
                                  0.0, 1.0, 0.01, 0.01, 0.0);

  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, scale_adj);
  gtk_widget_set_size_request (scale, SCALE_WIDTH, -1);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_widget_set_sensitive (scale, FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
                             _("JPEG smoothing factor:"), 0.0, 0.9,
                             scale, 1, FALSE);

  g_signal_connect (scale_adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &mng_data.smoothing);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  frame = gimp_frame_new (_("Animated MNG Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label (_("Loop"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mng_data.loop);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                mng_data.loop);

  gtk_widget_show (toggle);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Default frame delay:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton_adj = gtk_adjustment_new (mng_data.default_delay,
                                       0, 65000, 10, 100, 0);
  spinbutton = gtk_spin_button_new (spinbutton_adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  g_signal_connect (spinbutton_adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &mng_data.default_delay);

  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  gtk_widget_show (spinbutton);

  label = gtk_label_new (_("milliseconds"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  if (num_layers <= 1)
    {
      gtk_widget_set_sensitive (frame, FALSE);
      gimp_help_set_help_data (frame,
                               _("These options are only available when "
                                 "the exported image has more than one "
                                 "layer. The image you are exporting only has "
                                 "one layer."),
                               NULL);
    }

  gtk_widget_show (main_vbox);
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}


/* GIMP calls these methods. */

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",        "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",           "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",        "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",        "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename",    "The name of the file to save the image in" },

    { GIMP_PDB_INT32,    "interlace",       "Use interlacing" },
    { GIMP_PDB_INT32,    "compression",     "PNG deflate compression level (0 - 9)" },
    { GIMP_PDB_FLOAT,    "quality",         "JPEG quality factor (0.00 - 1.00)" },
    { GIMP_PDB_FLOAT,    "smoothing",       "JPEG smoothing factor (0.00 - 1.00)" },
    { GIMP_PDB_INT32,    "loop",            "(ANIMATED MNG) Loop infinitely" },
    { GIMP_PDB_INT32,    "default-delay",   "(ANIMATED MNG) Default delay between frames in milliseconds" },
    { GIMP_PDB_INT32,    "default-chunks",  "(ANIMATED MNG) Default chunks type (0 = PNG + Delta PNG; 1 = JNG + Delta PNG; 2 = All PNG; 3 = All JNG)" },
    { GIMP_PDB_INT32,    "default-dispose", "(ANIMATED MNG) Default dispose type (0 = combine; 1 = replace)" },
    { GIMP_PDB_INT32,    "bkgd",            "Write bKGD (background color) chunk" },
    { GIMP_PDB_INT32,    "gama",            "Write gAMA (gamma) chunk"},
    { GIMP_PDB_INT32,    "phys",            "Write pHYs (image resolution) chunk" },
    { GIMP_PDB_INT32,    "time",            "Write tIME (creation time) chunk" }
  };

  gimp_install_procedure (SAVE_PROC,
                          "Saves images in the MNG file format",
                          "This plug-in saves images in the Multiple-image "
                          "Network Graphics (MNG) format which can be used as "
                          "a replacement for animated GIFs, and more.",
                          "Mukund Sivaraman <muks@mukund.org>",
                          "Mukund Sivaraman <muks@mukund.org>",
                          "November 19, 2002",
                          N_("MNG animation"),
                          "RGB*,GRAY*,INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/x-mng");
  gimp_register_save_handler (SAVE_PROC, "mng", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam values[2];

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  if (strcmp (name, SAVE_PROC) == 0)
    {
      GimpRunMode      run_mode          = param[0].data.d_int32;
      gint32           image_id          = param[1].data.d_int32;
      gint32           original_image_id = image_id;
      gint32           drawable_id       = param[2].data.d_int32;
      GimpExportReturn export            = GIMP_EXPORT_IGNORE;

      if (run_mode == GIMP_RUN_INTERACTIVE ||
          run_mode == GIMP_RUN_WITH_LAST_VALS)
        {
          gimp_procedural_db_get_data (SAVE_PROC, &mng_data);

          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_id, &drawable_id, "MNG",
                                      GIMP_EXPORT_CAN_HANDLE_RGB     |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                      GIMP_EXPORT_CAN_HANDLE_LAYERS);
        }

      if (export == GIMP_EXPORT_CANCEL)
        {
          values[0].data.d_status = GIMP_PDB_CANCEL;
        }
      else if (export == GIMP_EXPORT_IGNORE || export == GIMP_EXPORT_EXPORT)
        {
          if (run_mode == GIMP_RUN_INTERACTIVE)
            {
              if (mng_save_dialog (image_id) == 0)
                values[0].data.d_status = GIMP_PDB_CANCEL;
            }
          else if (run_mode == GIMP_RUN_NONINTERACTIVE)
            {
              if (nparams != 17)
                {
                  g_message ("Incorrect number of parameters "
                             "passed to file-mng-save()");
                  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
                }
              else
                {
                  mng_data.interlaced = param[5].data.d_int32;
                  mng_data.compression_level = param[6].data.d_int32;
                  mng_data.quality = param[7].data.d_float;
                  mng_data.smoothing = param[8].data.d_float;
                  mng_data.loop = param[9].data.d_int32;
                  mng_data.default_delay = param[10].data.d_int32;
                  mng_data.default_chunks = param[11].data.d_int32;
                  mng_data.default_dispose = param[12].data.d_int32;
                  mng_data.bkgd = param[13].data.d_int32;
                  mng_data.gama = param[14].data.d_int32;
                  mng_data.phys = param[15].data.d_int32;
                  mng_data.time = param[16].data.d_int32;

                  if ((mng_data.compression_level < 0)
                      || (mng_data.compression_level > 9))
                    {
                      g_warning ("Parameter 'compression_level' passed to "
                                 "file-mng-save() must be in the range 0 - 9; "
                                 "Clamping it to the default value of 6.");
                      mng_data.compression_level = 6;
                    }

                  if ((mng_data.quality < ((float) 0))
                      || (mng_data.quality > ((float) 1)))
                    {
                      g_warning ("Parameter 'quality' passed to "
                                 "file-mng-save() must be in the range "
                                 "0.00 - 1.00; Clamping it to the "
                                 "default value of 0.75.");
                      mng_data.quality = 0.75;
                    }

                  if ((mng_data.smoothing < ((float) 0))
                      || (mng_data.smoothing > ((float) 1)))
                    {
                      g_warning ("Parameter 'smoothing' passed to "
                                 "file-mng-save() must be in the "
                                 "range 0.00 - 1.00; Clamping it to "
                                 "the default value of 0.00.");
                      mng_data.smoothing = 0.0;
                    }

                  if ((mng_data.default_chunks < 0)
                      || (mng_data.default_chunks > 3))
                    {
                      g_warning ("Parameter 'default_chunks' passed to "
                                 "file-mng-save() must be in the range 0 - 2.");
                      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
                    }

                  if ((mng_data.default_dispose < 0)
                      || (mng_data.default_dispose > 1))
                    {
                      g_warning ("Parameter 'default_dispose' passed to "
                                 "file-mng-save() must be in the range 0 - 1.");
                      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
                    }
                }
            }

          if (values[0].data.d_status == GIMP_PDB_SUCCESS)
            {
              GError *error = NULL;

              if (mng_save_image (param[3].data.d_string,
                                  image_id, drawable_id,
                                  original_image_id, &error))
                {
                  gimp_set_data (SAVE_PROC, &mng_data, sizeof (mng_data));
                }
              else
                {
                  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

                  if (error)
                    {
                      *nreturn_vals = 2;
                      values[1].type          = GIMP_PDB_STRING;
                      values[1].data.d_string = error->message;
                    }
                }
            }

          if (export == GIMP_EXPORT_EXPORT)
            gimp_image_delete (image_id);
        }

    }
  else
    {
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
    }
}
