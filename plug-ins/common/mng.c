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
 * THIS SOURCE CODE DOES NOT INCLUDE ANY FUNCTIONALITY FOR READING
 * OR WRITING CONTENT IN THE GIF IMAGE FORMAT.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#define PLUG_IN_BINARY "mng"
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

struct mng_data_t mng_data =
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


/* The output FILE pointer which is used by libmng;
 * passed around as user data. */
struct mnglib_userdata_t
{
  FILE *fp;
};


/*
 * Function prototypes
 */

static mng_ptr   myalloc                   (mng_size_t        size);
static void      myfree                    (mng_ptr           ptr,
                                            mng_size_t        size);
static mng_bool  myopenstream              (mng_handle        handle);
static mng_bool  myclosestream             (mng_handle        handle);
static mng_bool  mywritedata               (mng_handle        handle,
                                            mng_ptr           buf,
                                            mng_uint32        size,
                                            mng_uint32       *written_size);

static gint32    parse_chunks_type_from_layer_name   (const gchar *str);
static gint32    parse_disposal_type_from_layer_name (const gchar *str);
static gint32    parse_ms_tag_from_layer_name        (const gchar *str);

static gint      find_unused_ia_colour     (guchar           *pixels,
                                            gint              numpixels,
                                            gint             *colors);
static gboolean  ia_has_transparent_pixels (guchar           *pixels,
                                            gint              numpixels);
static gboolean  respin_cmap               (png_structp       png_ptr,
                                            png_infop         png_info_ptr,
                                            guchar           *remap,
                                            gint32            image_id,
                                            GimpDrawable     *drawable);

static gint      mng_save_image            (const gchar      *filename,
                                            gint32            image_id,
                                            gint32            drawable_id,
                                            gint32            original_image_id);
static gint      mng_save_dialog           (gint32            image_id);

static void      query                     (void);
static void      run                       (const gchar      *name,
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
  ptr = g_try_malloc ((gulong) size);

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


/* Try to find a colour in the palette which isn't actually
 * used in the image, so that we can use it as the transparency
 * index. Taken from png.c */
static gint
find_unused_ia_colour (guchar *pixels,
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
      /* If alpha is over a threshold, the colour index in the
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

  /* Couldn't find an unused colour index within the number of
     bits per pixel we wanted.  Will have to increment the number
     of colours in the image and assign a transparent pixel there. */
  if ((*colors) < 256)
    {
      (*colors)++;
      return ((*colors) - 1);
    }

  return (-1);
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


/* Spins the color map (palette) putting the transparent color at
 * index 0 if there is space. If there isn't any space, warn the user
 * and forget about transparency. Returns TRUE if the colormap has
 * been changed and FALSE otherwise.
 */

static gboolean
respin_cmap (png_structp  png_ptr,
             png_infop    png_info_ptr,
             guchar       *remap,
             gint32       image_id,
             GimpDrawable *drawable)
{
  static guchar  trans[] = { 0 };
  guchar        *before;
  guchar        *pixels;
  gint           numpixels;
  gint           colors;
  gint           transparent;
  gint           cols, rows;
  GimpPixelRgn   pixel_rgn;

  before = gimp_image_get_colormap (image_id, &colors);

  /*
   * Make sure there is something in the colormap.
   */
  if (colors == 0)
    {
      before = g_new0 (guchar, 3);
      colors = 1;
    }

  cols      = drawable->width;
  rows      = drawable->height;
  numpixels = cols * rows;

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                       drawable->width, drawable->height, FALSE, FALSE);

  pixels = (guchar *) g_malloc (numpixels * 2);

  gimp_pixel_rgn_get_rect (&pixel_rgn, pixels, 0, 0,
                           drawable->width, drawable->height);

  if (ia_has_transparent_pixels (pixels, numpixels))
    {
      transparent = find_unused_ia_colour (pixels, numpixels, &colors);

      if (transparent != -1)
        {
          png_color palette[256];
          gint i;

          png_set_tRNS (png_ptr, png_info_ptr, (png_bytep) trans, 1, NULL);

          /* Transform all pixels with a value = transparent to
           * 0 and vice versa to compensate for re-ordering in palette
           * due to png_set_tRNS() */

          remap[0] = transparent;
          remap[transparent] = 0;

          /* Copy from index 0 to index transparent - 1 to index 1 to
           * transparent of after, then from transparent+1 to colors-1
           * unchanged, and finally from index transparent to index 0. */

          for (i = 0; i < colors; i++)
            {
              palette[i].red = before[3 * remap[i]];
              palette[i].green = before[3 * remap[i] + 1];
              palette[i].blue = before[3 * remap[i] + 2];
            }

          png_set_PLTE (png_ptr, png_info_ptr, (png_colorp) palette, colors);

          return TRUE;
        }
      else
        g_message (_("Couldn't losslessly save transparency, "
                     "saving opacity instead."));
    }

  png_set_PLTE (png_ptr, png_info_ptr, (png_colorp) before, colors);

  return FALSE;
}


static gint
mng_save_image (const gchar *filename,
                gint32       image_id,
                gint32       drawable_id,
                gint32       original_image_id)
{
  gint            rows, cols;
  volatile gint   i;
  time_t          t;
  struct tm      *gmt;

  gint            num_layers;
  gint32         *layers;

  struct mnglib_userdata_t *userdata;
  mng_handle      handle;
  mng_retcode     ret;
  guint32         mng_ticks_per_second;
  guint32         mng_simplicity_profile;

  layers = gimp_image_get_layers (image_id, &num_layers);

  if (num_layers < 1)
    return 0;

  if (num_layers > 1)
    mng_ticks_per_second = 1000;
  else
    mng_ticks_per_second = 0;

  rows = gimp_image_height (image_id);
  cols = gimp_image_width (image_id);

  mng_simplicity_profile =
    (MNG_SIMPLICITY_VALID | MNG_SIMPLICITY_SIMPLEFEATURES |
     MNG_SIMPLICITY_COMPLEXFEATURES);
  mng_simplicity_profile |= (MNG_SIMPLICITY_JNG | MNG_SIMPLICITY_DELTAPNG);     /* JNG and delta-PNG chunks exist */

  for (i = 0; i < num_layers; i++)
    if (gimp_drawable_has_alpha (layers[i]))
      {
        mng_simplicity_profile |= MNG_SIMPLICITY_TRANSPARENCY;  /* internal transparency exists */
        mng_simplicity_profile |= 0x00000040;   /* validity of following */
        mng_simplicity_profile |= 0x00000100;   /* semi-transparency exists */
        mng_simplicity_profile |= 0x00000080;   /* background transparency should happen */
        break;
      }

  userdata = g_new0 (struct mnglib_userdata_t, 1);

  if ((userdata->fp = g_fopen (filename, "wb")) == NULL)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      g_free (userdata);
      return 0;
    }

  if ((handle =
       mng_initialize ((mng_ptr) userdata, myalloc, myfree,
                       MNG_NULL)) == NULL)
    {
      g_warning ("Unable to mng_initialize() in mng_save_image()");
      fclose (userdata->fp);
      g_free (userdata);
      return 0;
    }

  if (((ret = mng_setcb_openstream (handle, myopenstream)) != MNG_NOERROR) ||
      ((ret = mng_setcb_closestream (handle, myclosestream)) != MNG_NOERROR)
      || ((ret = mng_setcb_writedata (handle, mywritedata)) != MNG_NOERROR))
    {
      g_warning ("Unable to setup callbacks in mng_save_image()");
      mng_cleanup (&handle);
      fclose (userdata->fp);
      g_free (userdata);
      return 0;
    }

  if ((ret = mng_create (handle)) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_create() image in mng_save_image()");
      mng_cleanup (&handle);
      fclose (userdata->fp);
      g_free (userdata);
      return 0;
    }

  if ((ret =
       mng_putchunk_mhdr (handle, cols, rows, mng_ticks_per_second, 1,
                          num_layers, mng_data.default_delay,
                          mng_simplicity_profile)) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_putchunk_mhdr() in mng_save_image()");
      mng_cleanup (&handle);
      fclose (userdata->fp);
      g_free (userdata);
      return 0;
    }

  if ((num_layers > 1) && (mng_data.loop))
    {
      if ((ret =
           mng_putchunk_term (handle, MNG_TERMACTION_REPEAT,
                              MNG_ITERACTION_LASTFRAME,
                              parse_ms_tag_from_layer_name
                              (gimp_drawable_get_name (layers[0])),
                              0x7fffffff)) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_term() in mng_save_image()");
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }
    }
  else
    {
      if ((ret =
           mng_putchunk_term (handle, MNG_TERMACTION_LASTFRAME,
                              MNG_ITERACTION_LASTFRAME,
                              parse_ms_tag_from_layer_name
                              (gimp_drawable_get_name (layers[0])),
                              0x7fffffff)) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_term() in mng_save_image()");
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }
    }


  /* For now, we hardwire a comment */

  if ((ret =
       mng_putchunk_text (handle, strlen (MNG_TEXT_TITLE), MNG_TEXT_TITLE, 22,
                          "Created using GIMP")) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_putchunk_text() in mng_save_image()");
      mng_cleanup (&handle);
      fclose (userdata->fp);
      g_free (userdata);
      return 0;
    }

#if 0
        /* how do we get this to work? */

        if (mng_data.bkgd)
        {
                GimpRGB bgcolor;
                guchar red, green, blue;

                gimp_context_get_background(&bgcolor);
                gimp_rgb_get_uchar(&bgcolor, &red, &green, &blue);

                if ((ret = mng_putchunk_back(handle, red, green, blue,
                                             MNG_BACKGROUNDCOLOR_MANDATORY,
                                             0, MNG_BACKGROUNDIMAGE_NOTILE)) != MNG_NOERROR)
                {
                        g_warning("Unable to mng_putchunk_back() in mng_save_image()");
                        mng_cleanup(&handle);
                        fclose(userdata->fp);
                        g_free(userdata);
                        return 0;
                }

                if ((ret = mng_putchunk_bkgd(handle, MNG_FALSE, 2, 0,
                                             gimp_rgb_luminance_uchar(&bgcolor),
                                             red, green, blue)) != MNG_NOERROR)
                {
                        g_warning("Unable to mng_putchunk_bkgd() in mng_save_image()");
                        mng_cleanup(&handle);
                        fclose(userdata->fp);
                        g_free(userdata);
                        return 0;
                }
        }
#endif

  if (mng_data.gama)
    {
      if ((ret =
           mng_putchunk_gama (handle, MNG_FALSE,
                              (1.0 / (gimp_gamma ()) * 100000))) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_gama() in mng_save_image()");
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }
    }

#if 0
        /* how do we get this to work? */

        if (mng_data.phys)
        {
                gimp_image_get_resolution(original_image_id, &xres, &yres);

                if ((ret = mng_putchunk_phyg(handle, MNG_FALSE, (mng_uint32) (xres * 39.37), (mng_uint32) (yres * 39.37), 1)) != MNG_NOERROR)
                {
                        g_warning("Unable to mng_putchunk_phyg() in mng_save_image()");
                        mng_cleanup(&handle);
                        fclose(userdata->fp);
                        g_free(userdata);
                        return 0;
                }

                if ((ret = mng_putchunk_phys(handle, MNG_FALSE, (mng_uint32) (xres * 39.37), (mng_uint32) (yres * 39.37), 1)) != MNG_NOERROR)
                {
                        g_warning("Unable to mng_putchunk_phys() in mng_save_image()");
                        mng_cleanup(&handle);
                        fclose(userdata->fp);
                        g_free(userdata);
                        return 0;
                }
        }
#endif

  if (mng_data.time)
    {
      t = time (NULL);
      gmt = gmtime (&t);

      if ((ret =
           mng_putchunk_time (handle, gmt->tm_year + 1900, gmt->tm_mon + 1,
                              gmt->tm_mday, gmt->tm_hour, gmt->tm_min,
                              gmt->tm_sec)) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_time() in mng_save_image()");
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }
    }

  if (gimp_image_base_type (image_id) == GIMP_INDEXED)
    {
      guchar *palette;
      gint    numcolors;

      palette = gimp_image_get_colormap (image_id, &numcolors);

      if (numcolors != 0 &&
          (ret = mng_putchunk_plte (handle, numcolors, (mng_palette8e *) palette))
          != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_plte() in mng_save_image()");
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }
    }

  for (i = (num_layers - 1); i >= 0; i--)
    {
      gint            num_colors;
      GimpImageType   layer_drawable_type;
      GimpDrawable   *layer_drawable;
      gint            layer_offset_x, layer_offset_y;
      gint            layer_rows, layer_cols;
      gchar          *layer_name;
      gint            layer_chunks_type;
      volatile gint   layer_bpp;
      GimpPixelRgn    layer_pixel_rgn;

      guint8          layer_mng_colortype;
      guint8          layer_mng_compression_type;
      guint8          layer_mng_interlace_type;
      gboolean        layer_has_unique_palette;

      gchar           frame_mode;
      int             frame_delay;
      gchar          *temp_file_name;
      png_structp     png_ptr;
      png_infop       png_info_ptr;
      FILE           *infile, *outfile;
      int             num_passes;
      int             tile_height;
      guchar        **layer_pixels, *layer_pixel;
      int             pass, j, k, begin, end, num;
      guchar         *fixed;
      guchar          layer_remap[256];


      layer_name          = gimp_drawable_get_name (layers[i]);
      layer_chunks_type   = parse_chunks_type_from_layer_name (layer_name);
      layer_drawable_type = gimp_drawable_type (layers[i]);

      layer_drawable      = gimp_drawable_get (layers[i]);
      layer_rows          = layer_drawable->height;
      layer_cols          = layer_drawable->width;
      gimp_drawable_offsets (layers[i], &layer_offset_x, &layer_offset_y);

      layer_has_unique_palette = TRUE;

      for (j = 0; j < 256; j++)
        layer_remap[j] = j;

      switch (layer_drawable_type)
        {
        case GIMP_RGB_IMAGE:
          layer_bpp = 3;
          layer_mng_colortype = MNG_COLORTYPE_RGB;
          break;
        case GIMP_RGBA_IMAGE:
          layer_bpp = 4;
          layer_mng_colortype = MNG_COLORTYPE_RGBA;
          break;
        case GIMP_GRAY_IMAGE:
          layer_bpp = 1;
          layer_mng_colortype = MNG_COLORTYPE_GRAY;
          break;
        case GIMP_GRAYA_IMAGE:
          layer_bpp = 2;
          layer_mng_colortype = MNG_COLORTYPE_GRAYA;
          break;
        case GIMP_INDEXED_IMAGE:
          layer_bpp = 1;
          layer_mng_colortype = MNG_COLORTYPE_INDEXED;
          break;
        case GIMP_INDEXEDA_IMAGE:
          layer_bpp = 2;
          layer_mng_colortype = MNG_COLORTYPE_INDEXED | MNG_COLORTYPE_GRAYA;
          break;
        default:
          g_warning ("Unsupported GimpImageType in mng_save_image()");
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }

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
          g_warning
            ("Huh? Programmer stupidity error with 'layer_chunks_type'");
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }

      if ((i == (num_layers - 1))
          || (parse_disposal_type_from_layer_name (layer_name) !=
              DISPOSE_COMBINE))
        frame_mode = MNG_FRAMINGMODE_3;
      else
        frame_mode = MNG_FRAMINGMODE_1;

      frame_delay = parse_ms_tag_from_layer_name (layer_name);

      if ((ret = mng_putchunk_fram (handle, MNG_FALSE, frame_mode, 0, NULL,
                                    MNG_CHANGEDELAY_DEFAULT,
                                    MNG_CHANGETIMOUT_NO,
                                    MNG_CHANGECLIPPING_DEFAULT,
                                    MNG_CHANGESYNCID_NO,
                                    frame_delay, 0, 0,
                                    layer_offset_x,
                                    layer_offset_x + layer_cols,
                                    layer_offset_y,
                                    layer_offset_y + layer_rows,
                                    0, 0)) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_fram() in mng_save_image()");
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }

      if ((layer_offset_x != 0) || (layer_offset_y != 0))
        if ((ret =
             mng_putchunk_defi (handle, 0, 0, 1, 1, layer_offset_x,
                                layer_offset_y, 1, layer_offset_x,
                                layer_offset_x + layer_cols, layer_offset_y,
                                layer_offset_y + layer_rows)) != MNG_NOERROR)
          {
            g_warning ("Unable to mng_putchunk_defi() in mng_save_image()");
            mng_cleanup (&handle);
            fclose (userdata->fp);
            g_free (userdata);
            return 0;
          }

      if ((temp_file_name = gimp_temp_name ("mng")) == NULL)
        {
          g_warning ("gimp_temp_name() failed in mng_save_image(");
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }

      if ((outfile = g_fopen (temp_file_name, "wb")) == NULL)
        {
          g_message (_("Could not open '%s' for writing: %s"),
                     gimp_filename_to_utf8 (temp_file_name),
                     g_strerror (errno));
          g_unlink (temp_file_name);
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }

      if ((png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING,
                                              (png_voidp) NULL,
                                              (png_error_ptr) NULL,
                                              (png_error_ptr) NULL)) == NULL)
        {
          g_warning
            ("Unable to png_create_write_struct() in mng_save_image()");
          fclose (outfile);
          g_unlink (temp_file_name);
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }

      if ((png_info_ptr = png_create_info_struct (png_ptr)) == NULL)
        {
          g_warning
            ("Unable to png_create_info_struct() in mng_save_image()");
          png_destroy_write_struct (&png_ptr, (png_infopp) NULL);
          fclose (outfile);
          g_unlink (temp_file_name);
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }

      if (setjmp (png_ptr->jmpbuf) != 0)
        {
          g_warning ("HRM saving PNG in mng_save_image()");
          png_destroy_write_struct (&png_ptr, (png_infopp) NULL);
          fclose (outfile);
          g_unlink (temp_file_name);
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
        }

      png_init_io (png_ptr, outfile);
      png_set_compression_level (png_ptr, mng_data.compression_level);

      png_info_ptr->width = layer_cols;
      png_info_ptr->height = layer_rows;
      png_info_ptr->interlace_type = ((mng_data.interlaced == 0) ? 0 : 1);
      png_info_ptr->bit_depth = 8;

      switch (layer_drawable_type)
        {
        case GIMP_RGB_IMAGE:
          png_info_ptr->color_type = PNG_COLOR_TYPE_RGB;
          break;
        case GIMP_RGBA_IMAGE:
          png_info_ptr->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
          break;
        case GIMP_GRAY_IMAGE:
          png_info_ptr->color_type = PNG_COLOR_TYPE_GRAY;
          break;
        case GIMP_GRAYA_IMAGE:
          png_info_ptr->color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
          break;
        case GIMP_INDEXED_IMAGE:
          png_info_ptr->color_type = PNG_COLOR_TYPE_PALETTE;
          png_info_ptr->valid |= PNG_INFO_PLTE;
          png_info_ptr->palette =
            (png_colorp) gimp_image_get_colormap (image_id, &num_colors);
          png_info_ptr->num_palette = num_colors;
          break;
        case GIMP_INDEXEDA_IMAGE:
          png_info_ptr->color_type = PNG_COLOR_TYPE_PALETTE;
          layer_has_unique_palette =
            respin_cmap (png_ptr, png_info_ptr, layer_remap,
                         image_id, layer_drawable);
          break;
        default:
          g_warning ("This can't be!\n");
          return 0;
        }

      if ((png_info_ptr->valid & PNG_INFO_PLTE) == PNG_INFO_PLTE)
        {
          if (png_info_ptr->num_palette <= 2)
            png_info_ptr->bit_depth = 1;
          else if (png_info_ptr->num_palette <= 4)
            png_info_ptr->bit_depth = 2;
          else if (png_info_ptr->num_palette <= 16)
            png_info_ptr->bit_depth = 4;
        }

      png_write_info (png_ptr, png_info_ptr);

      if (mng_data.interlaced != 0)
        num_passes = png_set_interlace_handling (png_ptr);
      else
        num_passes = 1;

      if ((png_info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
          && (png_info_ptr->bit_depth < 8))
        png_set_packing (png_ptr);

      tile_height = gimp_tile_height ();
      layer_pixel = g_new (guchar, tile_height * layer_cols * layer_bpp);
      layer_pixels = g_new (guchar *, tile_height);

      for (j = 0; j < tile_height; j++)
        layer_pixels[j] = layer_pixel + (layer_cols * layer_bpp * j);

      gimp_pixel_rgn_init (&layer_pixel_rgn, layer_drawable, 0, 0, layer_cols,
                           layer_rows, FALSE, FALSE);

      for (pass = 0; pass < num_passes; pass++)
        {
          for (begin = 0, end = tile_height; begin < layer_rows;
               begin += tile_height, end += tile_height)
            {
              if (end > layer_rows)
                end = layer_rows;

              num = end - begin;
              gimp_pixel_rgn_get_rect (&layer_pixel_rgn, layer_pixel, 0,
                                       begin, layer_cols, num);

              if ((png_info_ptr->valid & PNG_INFO_tRNS) == PNG_INFO_tRNS)
                {
                  for (j = 0; j < num; j++)
                    {
                      fixed = layer_pixels[j];

                      for (k = 0; k < layer_cols; k++)
                        fixed[k] =
                          ((fixed[k * 2 + 1] > 127) ? layer_remap[fixed[k * 2]] : 0);
                    }
                }
              else
                if (((png_info_ptr->valid & PNG_INFO_PLTE) == PNG_INFO_PLTE)
                    && (layer_bpp == 2))
                {
                  for (j = 0; j < num; j++)
                    {
                      fixed = layer_pixels[j];

                      for (k = 0; k < layer_cols; k++)
                        fixed[k] = fixed[k * 2];
                    }
                }

              png_write_rows (png_ptr, layer_pixels, num);
            }
        }

      png_write_end (png_ptr, png_info_ptr);
      png_destroy_write_struct (&png_ptr, &png_info_ptr);

      g_free (layer_pixels);
      g_free (layer_pixel);

      fclose (outfile);

      if ((infile = g_fopen (temp_file_name, "rb")) == NULL)
        {
          g_message (_("Could not open '%s' for reading: %s"),
                     gimp_filename_to_utf8 (temp_file_name),
                     g_strerror (errno));
          g_unlink (temp_file_name);
          mng_cleanup (&handle);
          fclose (userdata->fp);
          g_free (userdata);
          return 0;
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

          chunksize = (chunksize_chars[0] << 24) | (chunksize_chars[1] << 16)
            | (chunksize_chars[2] << 8) | chunksize_chars[3];

          chunkbuffer = NULL;

          if (chunksize > 0)
            {
              chunkbuffer = g_new (guchar, chunksize);

              if (fread (chunkbuffer, 1, chunksize, infile) != chunksize)
                break;
            }

          if (strncmp (chunkname, "IHDR", 4) == 0)
            {
              chunkwidth = (chunkbuffer[0] << 24) | (chunkbuffer[1] << 16)
                | (chunkbuffer[2] << 8) | chunkbuffer[3];

              chunkheight = (chunkbuffer[4] << 24) | (chunkbuffer[5] << 16)
                | (chunkbuffer[6] << 8) | chunkbuffer[7];

              chunkbitdepth = chunkbuffer[8];
              chunkcolortype = chunkbuffer[9];
              chunkcompression = chunkbuffer[10];
              chunkfilter = chunkbuffer[11];
              chunkinterlaced = chunkbuffer[12];

              if ((ret =
                   mng_putchunk_ihdr (handle, chunkwidth, chunkheight,
                                      chunkbitdepth, chunkcolortype,
                                      chunkcompression, chunkfilter,
                                      chunkinterlaced)) != MNG_NOERROR)
                {
                  g_warning
                    ("Unable to mng_putchunk_ihdr() in mng_save_image()");
                  mng_cleanup (&handle);
                  fclose (userdata->fp);
                  g_free (userdata);
                  return 0;
                }
            }
          else if (strncmp (chunkname, "IDAT", 4) == 0)
            {
              if ((ret =
                   mng_putchunk_idat (handle, chunksize,
                                      chunkbuffer)) != MNG_NOERROR)
                {
                  g_warning
                    ("Unable to mng_putchunk_idat() in mng_save_image()");
                  mng_cleanup (&handle);
                  fclose (userdata->fp);
                  g_free (userdata);
                  return 0;
                }
            }
          else if (strncmp (chunkname, "IEND", 4) == 0)
            {
              if ((ret = mng_putchunk_iend (handle)) != MNG_NOERROR)
                {
                  g_warning
                    ("Unable to mng_putchunk_iend() in mng_save_image()");
                  mng_cleanup (&handle);
                  fclose (userdata->fp);
                  g_free (userdata);
                  return 0;
                }
            }
          else if (strncmp (chunkname, "PLTE", 4) == 0)
            {
              /* if this frame's palette is the same as the global palette,
                 write a 0-color palette chunk */
              if ((ret =
                   mng_putchunk_plte (handle,
                                      layer_has_unique_palette ? (chunksize / 3) : 0,
                                      (mng_palette8e *) chunkbuffer))
                  != MNG_NOERROR)
                {
                  g_warning
                    ("Unable to mng_putchunk_plte() in mng_save_image()");
                  mng_cleanup (&handle);
                  fclose (userdata->fp);
                  g_free (userdata);
                  return 0;
                }
            }
          else if (strncmp (chunkname, "tRNS", 4) == 0)
            {
              if ((ret =
                   mng_putchunk_trns (handle, 0, 0, 3, chunksize,
                                      (mng_uint8 *) chunkbuffer, 0, 0, 0, 0,
                                      0,
                                      (mng_uint8 *) chunkbuffer)) !=
                  MNG_NOERROR)
                {
                  g_warning
                    ("Unable to mng_putchunk_trns() in mng_save_image()");
                  mng_cleanup (&handle);
                  fclose (userdata->fp);
                  g_free (userdata);
                  return 0;
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

  if ((ret = mng_putchunk_mend (handle)) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_putchunk_mend() in mng_save_image()");
      mng_cleanup (&handle);
      fclose (userdata->fp);
      g_free (userdata);
      return 0;
    }

  if ((ret = mng_write (handle)) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_write() the image in mng_save_image()");
      mng_cleanup (&handle);
      fclose (userdata->fp);
      g_free (userdata);
      return 0;
    }

  mng_cleanup (&handle);
  fclose (userdata->fp);
  g_free (userdata);

  return TRUE;
}


/* The interactive dialog. */

static gint
mng_save_dialog (gint32 image_id)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkWidget *hbox;
  GtkWidget *combo;
  GtkWidget *label;
  GtkWidget *scale;
  GtkObject *scale_adj;
  GtkWidget *spinbutton;
  GtkObject *spinbutton_adj;
  gint       num_layers;
  gboolean   run;


  dialog = gimp_dialog_new (_("Save as MNG"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, SAVE_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);

  frame = gimp_frame_new (_("MNG Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 6);
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

  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_adj));
  gtk_widget_set_size_request (scale, SCALE_WIDTH, -1);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
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

  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_adj));
  gtk_widget_set_size_request (scale, SCALE_WIDTH, -1);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_widget_set_sensitive (scale, FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                             _("JPEG compression quality:"), 0.0, 0.9,
                             scale, 1, FALSE);

  g_signal_connect (scale_adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &mng_data.quality);

  scale_adj = gtk_adjustment_new (mng_data.smoothing,
                                  0.0, 1.0, 0.01, 0.01, 0.0);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_adj));
  gtk_widget_set_size_request (scale, SCALE_WIDTH, -1);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
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

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label (_("Loop"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mng_data.loop);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                mng_data.loop);

  gtk_widget_show (toggle);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Default frame delay:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&spinbutton_adj,
                                     mng_data.default_delay,
                                     0, 65000, 10, 100, 0, 1, 0);

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

  gtk_widget_set_sensitive (frame, num_layers > 1);

  gtk_widget_show (main_vbox);
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}


/* GIMP calls these methods. */

static void
query (void)
{
  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",        "Interactive, non-interactive" },
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
  static GimpParam values[1];

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  if (strcmp (name, SAVE_PROC) == 0)
    {
      GimpRunMode      run_mode;
      gint32           image_id, original_image_id;
      gint32           drawable_id;
      GimpExportReturn export = GIMP_EXPORT_IGNORE;

      run_mode = param[0].data.d_int32;
      image_id = original_image_id = param[1].data.d_int32;
      drawable_id = param[2].data.d_int32;

      if ((run_mode == GIMP_RUN_INTERACTIVE)
          || (run_mode == GIMP_RUN_WITH_LAST_VALS))
        {
          gimp_procedural_db_get_data (SAVE_PROC, &mng_data);

          gimp_ui_init (PLUG_IN_BINARY, FALSE);
          export = gimp_export_image (&image_id, &drawable_id, "MNG",
                                      (GIMP_EXPORT_CAN_HANDLE_RGB |
                                       GIMP_EXPORT_CAN_HANDLE_GRAY |
                                       GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                       GIMP_EXPORT_CAN_HANDLE_ALPHA |
                                       GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION));
        }

      if (export == GIMP_EXPORT_CANCEL)
        values[0].data.d_status = GIMP_PDB_CANCEL;
      else if ((export == GIMP_EXPORT_IGNORE)
               || (export == GIMP_EXPORT_EXPORT))
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
                      g_warning ("Parameter 'compression_level' passed to file-mng-save() must be in the range 0 - 9; Clamping it to the default value of 6.");
                      mng_data.compression_level = 6;
                    }

                  if ((mng_data.quality < ((float) 0))
                      || (mng_data.quality > ((float) 1)))
                    {
                      g_warning ("Parameter 'quality' passed to file-mng-save() must be in the range 0.00 - 1.00; Clamping it to the default value of 0.75.");
                      mng_data.quality = 0.75;
                    }

                  if ((mng_data.smoothing < ((float) 0))
                      || (mng_data.smoothing > ((float) 1)))
                    {
                      g_warning ("Parameter 'smoothing' passed to file-mng-save() must be in the range 0.00 - 1.00; Clamping it to the default value of 0.00.");
                      mng_data.smoothing = 0.0;
                    }

                  if ((mng_data.default_chunks < 0)
                      || (mng_data.default_chunks > 3))
                    {
                      g_warning ("Parameter 'default_chunks' passed to file-mng-save() must be in the range 0 - 2.");
                      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
                    }

                  if ((mng_data.default_dispose < 0)
                      || (mng_data.default_dispose > 1))
                    {
                      g_warning ("Parameter 'default_dispose' passed to file-mng-save() must be in the range 0 - 1.");
                      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
                    }
                }
            }

          if (values[0].data.d_status == GIMP_PDB_SUCCESS)
            {
              if (mng_save_image
                  (param[3].data.d_string, image_id, drawable_id,
                   original_image_id) != 0)
                gimp_set_data (SAVE_PROC, &mng_data, sizeof (mng_data));
              else
                values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
            }

          if (export == GIMP_EXPORT_EXPORT)
            gimp_image_delete (image_id);
        }

    }
  else
    values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
}


/* Only query and run are implemented by this plug-in. */

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};

MAIN ()
