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

/*
 * tifftopnm.c - converts a Tagged Image File to a portable anymap
 *
 * Derived by Jef Poskanzer from tif2ras.c, which is:
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <tiffio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "file-tiff-load.h"

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_ROLE "gimp-file-tiff-load"


typedef struct
{
  gint      compression;
  gint      fillorder;
  gboolean  save_transp_pixels;
} TiffSaveVals;

typedef struct
{
  gint32      ID;
  GeglBuffer *buffer;
  const Babl *format;
  guchar     *pixels;
  guchar     *pixel;
} ChannelData;


/* Declare some local functions */

static GimpColorProfile * load_profile     (TIFF         *tif);

static void               load_rgba        (TIFF         *tif,
                                            ChannelData  *channel);
static void               load_contiguous  (TIFF         *tif,
                                            ChannelData  *channel,
                                            const Babl   *type,
                                            gushort       bps,
                                            gushort       spp,
                                            gboolean      is_bw,
                                            gint          extra);
static void               load_separate    (TIFF         *tif,
                                            ChannelData  *channel,
                                            const Babl   *type,
                                            gushort       bps,
                                            gushort       spp,
                                            gboolean      is_bw,
                                            gint          extra);
static void               load_paths       (TIFF         *tif,
                                            gint          image);

static void               fill_bit2byte    (void);
static void               convert_bit2byte (const guchar *src,
                                            guchar       *dest,
                                            gint          width,
                                            gint          height);


static TiffSaveVals tsvals =
{
  COMPRESSION_NONE,    /*  compression    */
  TRUE,                /*  alpha handling */
};


/* returns a pointer into the TIFF */
static const gchar *
tiff_get_page_name (TIFF *tif)
{
  static gchar *name;

  if (TIFFGetField (tif, TIFFTAG_PAGENAME, &name) &&
      g_utf8_validate (name, -1, NULL))
    {
      return name;
    }

  return NULL;
}

gboolean
load_dialog (TIFF              *tif,
             const gchar       *help_id,
             TiffSelectedPages *pages)
{
  GtkWidget  *dialog;
  GtkWidget  *vbox;
  GtkWidget  *selector;
  gint        i;
  gboolean    run;

  dialog = gimp_dialog_new (_("Import from TIFF"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, help_id,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Import"), GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /* Page Selector */
  selector = gimp_page_selector_new ();
  gtk_widget_set_size_request (selector, 300, 200);
  gtk_box_pack_start (GTK_BOX (vbox), selector, TRUE, TRUE, 0);

  gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (selector),
                                  pages->n_pages);
  gimp_page_selector_set_target (GIMP_PAGE_SELECTOR (selector), pages->target);

  for (i = 0; i < pages->n_pages; i++)
    {
      const gchar *name = tiff_get_page_name (tif);

      if (name)
        gimp_page_selector_set_page_label (GIMP_PAGE_SELECTOR (selector),
                                           i, name);

      TIFFReadDirectory (tif);
    }

  g_signal_connect_swapped (selector, "activate",
                            G_CALLBACK (gtk_window_activate_default),
                            dialog);

  gtk_widget_show (selector);

  /* Setup done; display the dialog */
  gtk_widget_show (dialog);

  /* run the dialog */
  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      pages->target =
        gimp_page_selector_get_target (GIMP_PAGE_SELECTOR (selector));

      pages->pages =
        gimp_page_selector_get_selected_pages (GIMP_PAGE_SELECTOR (selector),
                                               &pages->n_pages);

      /* select all if none selected */
      if (pages->n_pages == 0)
        {
          gimp_page_selector_select_all (GIMP_PAGE_SELECTOR (selector));

          pages->pages =
            gimp_page_selector_get_selected_pages (GIMP_PAGE_SELECTOR (selector),
                                                   &pages->n_pages);
        }
    }

  return run;
}

gint32
load_image (GFile              *file,
            TIFF               *tif,
            TiffSelectedPages  *pages,
            gboolean           *resolution_loaded,
            GError            **error)
{
  GList *images_list      = NULL;
  gint   image            = 0;
  gint   first_image_type = GIMP_RGB;
  gint   min_row          = G_MAXINT;
  gint   min_col          = G_MAXINT;
  gint   max_row          = 0;
  gint   max_col          = 0;
  gint   li;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  /* We will loop through the all pages in case of multipage TIFF
   * and load every page as a separate layer.
   */
  for (li = 0; li < pages->n_pages; li++)
    {
      gint              ilayer;
      gushort           bps;
      gushort           spp;
      gushort           photomet;
      gshort            sampleformat;
      GimpColorProfile *profile;
      gboolean          profile_linear = FALSE;
      GimpPrecision     image_precision;
      const Babl       *type;
      const Babl       *base_format = NULL;
      guint16           orientation;
      gint              cols;
      gint              rows;
      gboolean          alpha;
      gint              image_type           = GIMP_RGB;
      gint              layer;
      gint              layer_type           = GIMP_RGB_IMAGE;
      float             layer_offset_x       = 0.0;
      float             layer_offset_y       = 0.0;
      gint              layer_offset_x_pixel = 0;
      gint              layer_offset_y_pixel = 0;
      gushort           extra;
      gushort          *extra_types;
      ChannelData      *channel = NULL;
      uint16            planar  = PLANARCONFIG_CONTIG;
      gboolean          is_bw;
      gint              i;
      gboolean          worst_case = FALSE;
      TiffSaveVals      save_vals;
      const gchar      *name;

      TIFFSetDirectory (tif, pages->pages[li]);
      ilayer = pages->pages[li];

      gimp_progress_update (0.0);

      TIFFGetFieldDefaulted (tif, TIFFTAG_BITSPERSAMPLE, &bps);

      TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLEFORMAT, &sampleformat);

      profile = load_profile (tif);
      if (profile)
        profile_linear = gimp_color_profile_is_linear (profile);

      if (bps > 8 && bps != 8 && bps != 16 && bps != 32 && bps != 64)
        worst_case = TRUE; /* Wrong sample width => RGBA */

      switch (bps)
        {
        case 1:
        case 8:
          if (profile_linear)
            image_precision = GIMP_PRECISION_U8_LINEAR;
          else
            image_precision = GIMP_PRECISION_U8_GAMMA;

          type = babl_type ("u8");
          break;

        case 16:
          if (sampleformat == SAMPLEFORMAT_IEEEFP)
            {
              if (profile_linear)
                image_precision = GIMP_PRECISION_HALF_LINEAR;
              else
                image_precision = GIMP_PRECISION_HALF_GAMMA;

              type = babl_type ("half");
            }
          else
            {
              if (profile_linear)
                image_precision = GIMP_PRECISION_U16_LINEAR;
              else
                image_precision = GIMP_PRECISION_U16_GAMMA;

              type = babl_type ("u16");
            }
          break;

        case 32:
          if (sampleformat == SAMPLEFORMAT_IEEEFP)
            {
              if (profile_linear)
                image_precision = GIMP_PRECISION_FLOAT_LINEAR;
              else
                image_precision = GIMP_PRECISION_FLOAT_GAMMA;

              type = babl_type ("float");
            }
          else
            {
              if (profile_linear)
                image_precision = GIMP_PRECISION_U32_LINEAR;
              else
                image_precision = GIMP_PRECISION_U32_GAMMA;

              type = babl_type ("u32");
            }
          break;

        case 64:
          if (profile_linear)
            image_precision = GIMP_PRECISION_DOUBLE_LINEAR;
          else
            image_precision = GIMP_PRECISION_DOUBLE_GAMMA;

          type = babl_type ("double");
          break;

        default:
          if (profile_linear)
            image_precision = GIMP_PRECISION_U16_LINEAR;
          else
            image_precision = GIMP_PRECISION_U16_GAMMA;

          type = babl_type ("u16");
        }

      g_printerr ("bps: %d\n", bps);

      TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &spp);

      if (! TIFFGetField (tif, TIFFTAG_EXTRASAMPLES, &extra, &extra_types))
        extra = 0;

      if (! TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &cols))
        {
          g_message ("Could not get image width from '%s'",
                     gimp_file_get_utf8_name (file));
          return -1;
        }

      if (! TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &rows))
        {
          g_message ("Could not get image length from '%s'",
                     gimp_file_get_utf8_name (file));
          return -1;
        }

      if (! TIFFGetField (tif, TIFFTAG_PHOTOMETRIC, &photomet))
        {
          guint16 compression;

          if (TIFFGetField (tif, TIFFTAG_COMPRESSION, &compression) &&
              (compression == COMPRESSION_CCITTFAX3 ||
               compression == COMPRESSION_CCITTFAX4 ||
               compression == COMPRESSION_CCITTRLE  ||
               compression == COMPRESSION_CCITTRLEW))
            {
              g_message ("Could not get photometric from '%s'. "
                         "Image is CCITT compressed, assuming min-is-white",
                         gimp_file_get_utf8_name (file));
              photomet = PHOTOMETRIC_MINISWHITE;
            }
          else
            {
              g_message ("Could not get photometric from '%s'. "
                         "Assuming min-is-black",
                         gimp_file_get_utf8_name (file));

              /* old AppleScan software misses out the photometric tag
               * (and incidentally assumes min-is-white, but xv
               * assumes min-is-black, so we follow xv's lead.  It's
               * not much hardship to invert the image later).
               */
              photomet = PHOTOMETRIC_MINISBLACK;
            }
        }

      /* test if the extrasample represents an associated alpha channel... */
      if (extra > 0 && (extra_types[0] == EXTRASAMPLE_ASSOCALPHA))
        {
          alpha = TRUE;
          tsvals.save_transp_pixels = FALSE;
          extra--;
        }
      else if (extra > 0 && (extra_types[0] == EXTRASAMPLE_UNASSALPHA))
        {
          alpha = TRUE;
          tsvals.save_transp_pixels = TRUE;
          extra--;
        }
      else if (extra > 0 && (extra_types[0] == EXTRASAMPLE_UNSPECIFIED))
        {
          /* assuming unassociated alpha if unspecified */
          g_message ("alpha channel type not defined for file %s. "
                     "Assuming alpha is not premultiplied",
                     gimp_file_get_utf8_name (file));
          alpha = TRUE;
          tsvals.save_transp_pixels = TRUE;
          extra--;
        }
      else
        {
          alpha = FALSE;
        }

      if (photomet == PHOTOMETRIC_RGB && spp > 3 + extra)
        {
          alpha = TRUE;
          extra = spp - 4;
        }
      else if (photomet != PHOTOMETRIC_RGB && spp > 1 + extra)
        {
          alpha = TRUE;
          extra = spp - 2;
        }

      is_bw = FALSE;

      switch (photomet)
        {
        case PHOTOMETRIC_MINISBLACK:
        case PHOTOMETRIC_MINISWHITE:
          if (bps == 1 && ! alpha && spp == 1)
            {
              image_type = GIMP_INDEXED;
              layer_type = GIMP_INDEXED_IMAGE;

              is_bw = TRUE;
              fill_bit2byte ();
            }
          else
            {
              image_type = GIMP_GRAY;
              layer_type = alpha ? GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE;

              if (alpha)
                {
                  if (tsvals.save_transp_pixels)
                    {
                      if (profile_linear)
                        {
                          base_format = babl_format_new (babl_model ("YA"),
                                                         type,
                                                         babl_component ("Y"),
                                                         babl_component ("A"),
                                                         NULL);
                        }
                      else
                        {
                          base_format = babl_format_new (babl_model ("Y'A"),
                                                         type,
                                                         babl_component ("Y'"),
                                                         babl_component ("A"),
                                                         NULL);
                        }
                    }
                  else
                    {
                      if (profile_linear)
                        {
                          base_format = babl_format_new (babl_model ("YaA"),
                                                         type,
                                                         babl_component ("Ya"),
                                                         babl_component ("A"),
                                                         NULL);
                        }
                      else
                        {
                          base_format = babl_format_new (babl_model ("Y'aA"),
                                                         type,
                                                         babl_component ("Y'a"),
                                                         babl_component ("A"),
                                                         NULL);
                        }
                    }
                }
              else
                {
                  if (profile_linear)
                    {
                      base_format = babl_format_new (babl_model ("Y'"),
                                                     type,
                                                     babl_component ("Y'"),
                                                     NULL);
                    }
                  else
                    {
                      base_format = babl_format_new (babl_model ("Y'"),
                                                     type,
                                                     babl_component ("Y'"),
                                                     NULL);
                    }
                }
            }
          break;

        case PHOTOMETRIC_RGB:
          image_type = GIMP_RGB;
          layer_type = alpha ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE;

          if (alpha)
            {
              if (tsvals.save_transp_pixels)
                {
                  if (profile_linear)
                    {
                      base_format = babl_format_new (babl_model ("RGBA"),
                                                     type,
                                                     babl_component ("R"),
                                                     babl_component ("G"),
                                                     babl_component ("B"),
                                                     babl_component ("A"),
                                                     NULL);
                    }
                  else
                    {
                      base_format = babl_format_new (babl_model ("R'G'B'A"),
                                                     type,
                                                     babl_component ("R'"),
                                                     babl_component ("G'"),
                                                     babl_component ("B'"),
                                                     babl_component ("A"),
                                                     NULL);
                    }
                }
              else
                {
                  if (profile_linear)
                    {
                      base_format = babl_format_new (babl_model ("RaGaBaA"),
                                                     type,
                                                     babl_component ("Ra"),
                                                     babl_component ("Ga"),
                                                     babl_component ("Ba"),
                                                     babl_component ("A"),
                                                     NULL);
                    }
                  else
                    {
                      base_format = babl_format_new (babl_model ("R'aG'aB'aA"),
                                                     type,
                                                     babl_component ("R'a"),
                                                     babl_component ("G'a"),
                                                     babl_component ("B'a"),
                                                     babl_component ("A"),
                                                     NULL);
                    }
                }
            }
          else
            {
              if (profile_linear)
                {
                  base_format = babl_format_new (babl_model ("RGB"),
                                                 type,
                                                 babl_component ("R"),
                                                 babl_component ("G"),
                                                 babl_component ("B"),
                                                 NULL);
                }
              else
                {
                  base_format = babl_format_new (babl_model ("R'G'B'"),
                                                 type,
                                                 babl_component ("R'"),
                                                 babl_component ("G'"),
                                                 babl_component ("B'"),
                                                 NULL);
                }
            }
          break;

        case PHOTOMETRIC_PALETTE:
          image_type = GIMP_INDEXED;
          layer_type = alpha ? GIMP_INDEXEDA_IMAGE : GIMP_INDEXED_IMAGE;
          break;

        default:
          g_printerr ("photomet: %d (%d)\n", photomet, PHOTOMETRIC_PALETTE);
          worst_case = TRUE;
          break;
        }

      /* attach a parasite containing the compression */
      {
        guint16 compression = COMPRESSION_NONE;

        if (TIFFGetField (tif, TIFFTAG_COMPRESSION, &compression))
          {
            switch (compression)
              {
              case COMPRESSION_NONE:
              case COMPRESSION_LZW:
              case COMPRESSION_PACKBITS:
              case COMPRESSION_DEFLATE:
              case COMPRESSION_ADOBE_DEFLATE:
              case COMPRESSION_JPEG:
              case COMPRESSION_CCITTFAX3:
              case COMPRESSION_CCITTFAX4:
                break;

              case COMPRESSION_OJPEG:
                worst_case  = TRUE;
                compression = COMPRESSION_JPEG;
                break;

              default:
                compression = COMPRESSION_NONE;
                break;
              }
          }

        save_vals.compression = compression;
      }

      if (worst_case)
        {
          image_type  = GIMP_RGB;
          layer_type  = GIMP_RGBA_IMAGE;

          if (profile_linear)
            {
              base_format = babl_format_new (babl_model ("RaGaBaA"),
                                             type,
                                             babl_component ("Ra"),
                                             babl_component ("Ga"),
                                             babl_component ("Ba"),
                                             babl_component ("A"),
                                             NULL);
            }
          else
            {
              base_format = babl_format_new (babl_model ("R'aG'aB'aA"),
                                             type,
                                             babl_component ("R'a"),
                                             babl_component ("G'a"),
                                             babl_component ("B'a"),
                                             babl_component ("A"),
                                             NULL);
            }
        }

      if (pages->target == GIMP_PAGE_SELECTOR_TARGET_LAYERS)
        {
          if (li == 0)
            {
              first_image_type = image_type;
            }
          else if (image_type != first_image_type)
            {
              continue;
            }
        }

      if ((pages->target == GIMP_PAGE_SELECTOR_TARGET_IMAGES) || (! image))
        {
          image = gimp_image_new_with_precision (cols, rows, image_type,
                                                 image_precision);

          if (image < 1)
            {
              g_message ("Could not create a new image: %s",
                         gimp_get_pdb_error ());
              return -1;
            }

          gimp_image_undo_disable (image);

          if (pages->target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
            {
              gchar *fname = g_strdup_printf ("%s-%d", g_file_get_uri (file),
                                              ilayer);

              gimp_image_set_filename (image, fname);
              g_free (fname);

              images_list = g_list_prepend (images_list,
                                            GINT_TO_POINTER (image));
            }
          else if (pages->o_pages != pages->n_pages)
            {
              gchar *fname = g_strdup_printf (_("%s-%d-of-%d-pages"),
                                              g_file_get_uri (file),
                                              pages->n_pages, pages->o_pages);

              gimp_image_set_filename (image, fname);
              g_free (fname);
            }
          else
            {
              gimp_image_set_filename (image, g_file_get_uri (file));
            }
        }

      /* attach color profile */

      if (profile)
        {
          gimp_image_set_color_profile (image, profile);
          g_object_unref (profile);
        }

      /* attach parasites */
      {
        GimpParasite *parasite;
        const gchar  *img_desc;

        parasite = gimp_parasite_new ("tiff-save-options", 0,
                                      sizeof (save_vals), &save_vals);
        gimp_image_attach_parasite (image, parasite);
        gimp_parasite_free (parasite);

        /* Attach a parasite containing the image description.
         * Pretend to be a gimp comment so other plugins will use this
         * description as an image comment where appropriate.
         */
        if (TIFFGetField (tif, TIFFTAG_IMAGEDESCRIPTION, &img_desc) &&
            g_utf8_validate (img_desc, -1, NULL))
          {
            parasite = gimp_parasite_new ("gimp-comment",
                                          GIMP_PARASITE_PERSISTENT,
                                          strlen (img_desc) + 1, img_desc);
            gimp_image_attach_parasite (image, parasite);
            gimp_parasite_free (parasite);
          }
      }

      /* any resolution info in the file? */
      {
        gfloat   xres = 72.0;
        gfloat   yres = 72.0;
        gushort  read_unit;
        GimpUnit unit = GIMP_UNIT_PIXEL; /* invalid unit */

        if (TIFFGetField (tif, TIFFTAG_XRESOLUTION, &xres))
          {
            if (TIFFGetField (tif, TIFFTAG_YRESOLUTION, &yres))
              {
                if (TIFFGetFieldDefaulted (tif, TIFFTAG_RESOLUTIONUNIT,
                                           &read_unit))
                  {
                    switch (read_unit)
                      {
                      case RESUNIT_NONE:
                        /* ImageMagick writes files with this silly resunit */
                        break;

                      case RESUNIT_INCH:
                        unit = GIMP_UNIT_INCH;
                        break;

                      case RESUNIT_CENTIMETER:
                        xres *= 2.54;
                        yres *= 2.54;
                        unit = GIMP_UNIT_MM; /* this is our default metric unit */
                        break;

                      default:
                        g_message ("File error: unknown resolution "
                                   "unit type %d, assuming dpi", read_unit);
                        break;
                      }
                  }
                else
                  {
                    /* no res unit tag */

                    /* old AppleScan software produces these */
                    g_message ("Warning: resolution specified without "
                               "any units tag, assuming dpi");
                  }
              }
            else
              {
                /* xres but no yres */

                g_message ("Warning: no y resolution info, assuming same as x");
                yres = xres;
              }

            /* now set the new image's resolution info */

            /* If it is invalid, instead of forcing 72dpi, do not set
             * the resolution at all. Gimp will then use the default
             * set by the user
             */
            if (read_unit != RESUNIT_NONE)
              {
                gimp_image_set_resolution (image, xres, yres);
                if (unit != GIMP_UNIT_PIXEL)
                  gimp_image_set_unit (image, unit);

                *resolution_loaded = TRUE;
              }
          }

        /* no x res tag => we assume we have no resolution info, so we
         * don't care.  Older versions of this plugin used to write
         * files with no resolution tags at all.
         */

        /* TODO: haven't caught the case where yres tag is present,
         * but not xres.  This is left as an exercise for the reader -
         * they should feel free to shoot the author of the broken
         * program that produced the damaged TIFF file in the first
         * place.
         */

        /* handle layer offset */
        if (! TIFFGetField (tif, TIFFTAG_XPOSITION, &layer_offset_x))
          layer_offset_x = 0.0;

        if (! TIFFGetField (tif, TIFFTAG_YPOSITION, &layer_offset_y))
          layer_offset_y = 0.0;

        /* round floating point position to integer position required
         * by GIMP
         */
        layer_offset_x_pixel = ROUND (layer_offset_x * xres);
        layer_offset_y_pixel = ROUND (layer_offset_y * yres);
      }

      /* Install colormap for INDEXED images only */
      if (image_type == GIMP_INDEXED)
        {
          guchar cmap[768];

          if (is_bw)
            {
              if (photomet == PHOTOMETRIC_MINISWHITE)
                {
                  cmap[0] = cmap[1] = cmap[2] = 255;
                  cmap[3] = cmap[4] = cmap[5] = 0;
                }
              else
                {
                  cmap[0] = cmap[1] = cmap[2] = 0;
                  cmap[3] = cmap[4] = cmap[5] = 255;
                }
            }
          else
            {
              gushort *redmap;
              gushort *greenmap;
              gushort *bluemap;
              gint     i, j;

              if (! TIFFGetField (tif, TIFFTAG_COLORMAP,
                                  &redmap, &greenmap, &bluemap))
                {
                  g_message ("Could not get colormaps from '%s'",
                             gimp_file_get_utf8_name (file));
                  return -1;
                }

              for (i = 0, j = 0; i < (1 << bps); i++)
                {
                  cmap[j++] = redmap[i] >> 8;
                  cmap[j++] = greenmap[i] >> 8;
                  cmap[j++] = bluemap[i] >> 8;
                }
            }

          gimp_image_set_colormap (image, cmap, (1 << bps));
        }

      load_paths (tif, image);

      /* Allocate ChannelData for all channels, even the background layer */
      channel = g_new0 (ChannelData, extra + 1);

      /* try and use layer name from tiff file */
      name = tiff_get_page_name (tif);

      if (name)
        {
          layer = gimp_layer_new (image, name,
                                  cols, rows,
                                  layer_type,
                                  100,
                                  gimp_image_get_default_new_layer_mode (image));
        }
      else
        {
          gchar *name;

          if (ilayer == 0)
            name = g_strdup (_("Background"));
          else
            name = g_strdup_printf (_("Page %d"), ilayer);

          layer = gimp_layer_new (image, name,
                                  cols, rows,
                                  layer_type,
                                  100,
                                  gimp_image_get_default_new_layer_mode (image));
          g_free (name);
        }

      if (! base_format && image_type == GIMP_INDEXED)
        {
          /* can't create the palette format here, need to get it from
           * an existing layer
           */
          base_format = gimp_drawable_get_format (layer);
        }

      channel[0].ID     = layer;
      channel[0].buffer = gimp_drawable_get_buffer (layer);
      channel[0].format = base_format;

      if (extra > 0 && ! worst_case)
        {
          /* Add extra channels as appropriate */
          for (i = 1; i <= extra; i++)
            {
              GimpRGB color;

              gimp_rgb_set (&color, 0.0, 0.0, 0.0);

              channel[i].ID = gimp_channel_new (image, _("TIFF Channel"),
                                                cols, rows,
                                                100.0, &color);
              gimp_image_insert_channel (image, channel[i].ID, -1, 0);
              channel[i].buffer = gimp_drawable_get_buffer (channel[i].ID);
              channel[i].format = babl_format_new (babl_model ("Y'"),
                                                   type,
                                                   babl_component ("Y'"),
                                                   NULL);
            }
        }

      TIFFGetField (tif, TIFFTAG_PLANARCONFIG, &planar);

      if (worst_case)
        {
          load_rgba (tif, channel);
        }
      else if (planar == PLANARCONFIG_CONTIG)
        {
          load_contiguous (tif, channel, type, bps, spp, is_bw, extra);
        }
      else
        {
          load_separate (tif, channel, type, bps, spp, is_bw, extra);
        }

      if (TIFFGetField (tif, TIFFTAG_ORIENTATION, &orientation))
        {
          gboolean flip_horizontal = FALSE;
          gboolean flip_vertical   = FALSE;

          switch (orientation)
            {
            case ORIENTATION_TOPLEFT:
              break;

            case ORIENTATION_TOPRIGHT:
              flip_horizontal = TRUE;
              break;

            case ORIENTATION_BOTRIGHT:
              flip_horizontal = TRUE;
              flip_vertical   = TRUE;
              break;

            case ORIENTATION_BOTLEFT:
              flip_vertical = TRUE;
              break;

            default:
              g_warning ("Orientation %d not handled yet!", orientation);
              break;
            }

          if (flip_horizontal)
            gimp_item_transform_flip_simple (layer,
                                             GIMP_ORIENTATION_HORIZONTAL,
                                             TRUE /* auto_center */,
                                             -1.0 /* axis */);

          if (flip_vertical)
            gimp_item_transform_flip_simple (layer,
                                             GIMP_ORIENTATION_VERTICAL,
                                             TRUE /* auto_center */,
                                             -1.0 /* axis */);
        }

      for (i = 0; i <= extra; i++)
        {
          if (channel[i].buffer)
            g_object_unref (channel[i].buffer);
        }

      g_free (channel);
      channel = NULL;


      /* TODO: in GIMP 2.6, use a dialog to selectively enable the
       * following code, as the save plug-in will then save layer offsets
       * as well.
       */

      /* compute bounding box of all layers read so far */
      if (min_col > layer_offset_x_pixel)
        min_col = layer_offset_x_pixel;
      if (min_row > layer_offset_y_pixel)
        min_row = layer_offset_y_pixel;

      if (max_col < layer_offset_x_pixel + cols)
        max_col = layer_offset_x_pixel + cols;
      if (max_row < layer_offset_y_pixel + rows)
        max_row = layer_offset_y_pixel + rows;

      /* position the layer */
      if (layer_offset_x_pixel > 0 ||
          layer_offset_y_pixel > 0)
        {
          gimp_layer_set_offsets (layer,
                                  layer_offset_x_pixel, layer_offset_y_pixel);
        }

      gimp_image_insert_layer (image, layer, -1, -1);

      if (pages->target == GIMP_PAGE_SELECTOR_TARGET_IMAGES)
        {
          gimp_image_undo_enable (image);
          gimp_image_clean_all (image);
        }

      gimp_progress_update (1.0);
    }

  if (pages->target != GIMP_PAGE_SELECTOR_TARGET_IMAGES)
    {
      /* resize image to bounding box of all layers */
      gimp_image_resize (image,
                         max_col - min_col, max_row - min_row,
                         -min_col, -min_row);

      gimp_image_undo_enable (image);
    }
  else
    {
      GList *list = images_list;

      if (list)
        {
          image = GPOINTER_TO_INT (list->data);

          list = g_list_next (list);
        }

      for (; list; list = g_list_next (list))
        {
          gimp_display_new (GPOINTER_TO_INT (list->data));
        }

      g_list_free (images_list);
    }

  return image;
}

static GimpColorProfile *
load_profile (TIFF *tif)
{
  GimpColorProfile *profile = NULL;

#ifdef TIFFTAG_ICCPROFILE
  /* If TIFFTAG_ICCPROFILE is defined we are dealing with a
   * libtiff version that can handle ICC profiles. Otherwise just
   * return a NULL profile.
   */
  uint32  profile_size;
  guchar *icc_profile;

  /* set the ICC profile - if found in the TIFF */
  if (TIFFGetField (tif, TIFFTAG_ICCPROFILE, &profile_size, &icc_profile))
    {
      profile = gimp_color_profile_new_from_icc_profile (icc_profile,
                                                         profile_size,
                                                         NULL);
    }
#endif

  return profile;
}

static void
load_rgba (TIFF        *tif,
           ChannelData *channel)
{
  guint32  image_width;
  guint32  image_height;
  guint32  row;
  guint32 *buffer;

  g_printerr ("%s\n", __func__);

  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH,  &image_width);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &image_height);

  buffer = g_new (uint32, image_width * image_height);

  if (! TIFFReadRGBAImage (tif, image_width, image_height, buffer, 0))
    g_message ("Unsupported layout, no RGBA loader");

  for (row = 0; row < image_height; row++)
    {
#if G_BYTE_ORDER != G_LITTLE_ENDIAN
      /* Make sure our channels are in the right order */
      guint32 row_start = row * image_width;
      guint32 row_end   = row_start + image_width;
      guint32 i;

      for (i = row_start; i < row_end; i++)
        buffer[i] = GUINT32_TO_LE (buffer[i]);
#endif

      gegl_buffer_set (channel[0].buffer,
                       GEGL_RECTANGLE (0, image_height - row - 1,
                                       image_width, 1),
                       0, channel[0].format,
                       ((guchar *) buffer) + row * image_width * 4,
                       GEGL_AUTO_ROWSTRIDE);

      if ((row % 32) == 0)
        gimp_progress_update ((gdouble) row / (gdouble) image_height);
    }

  g_free (buffer);
}

static void
load_paths (TIFF *tif,
            gint  image)
{
  gint   width;
  gint   height;
  gsize  n_bytes;
  gchar *bytes;
  gint   path_index;
  gsize  pos;

  width  = gimp_image_width (image);
  height = gimp_image_height (image);

  if (! TIFFGetField (tif, TIFFTAG_PHOTOSHOP, &n_bytes, &bytes))
    return;

  path_index = 0;
  pos        = 0;

  while (pos < n_bytes)
    {
      guint16  id;
      gsize    len;
      gchar   *name;
      guint32 *val32;
      guint16 *val16;

      if (n_bytes-pos < 7 ||
          strncmp (bytes + pos, "8BIM", 4) != 0)
        break;

      pos += 4;

      val16 = (guint16 *) (bytes + pos);
      id = GUINT16_FROM_BE (*val16);
      pos += 2;

      /* g_printerr ("id: %x\n", id); */
      len = (guchar) bytes[pos];

      if (n_bytes - pos < len + 1)
        break;   /* block not big enough */

      /* do we have the UTF-marker? is it valid UTF-8?
       * if so, we assume an utf-8 encoded name, otherwise we
       * assume iso8859-1
       */
      name = bytes + pos + 1;
      if (len >= 3 &&
          name[0] == '\xEF' && name[1] == '\xBB' && name[2] == '\xBF' &&
          g_utf8_validate (name, len, NULL))
        {
          name = g_strndup (name + 3, len - 3);
        }
      else
        {
          name = g_convert (name, len, "utf-8", "iso8859-1", NULL, NULL, NULL);
        }

      if (! name)
        name = g_strdup ("(imported path)");

      pos += len + 1;

      if (pos % 2)  /* padding */
        pos++;

      if (n_bytes - pos < 4)
        break;   /* block not big enough */

      val32 = (guint32 *) (bytes + pos);
      len = GUINT32_FROM_BE (*val32);
      pos += 4;

      if (n_bytes - pos < len)
        break;   /* block not big enough */

      if (id >= 2000 && id <= 2998)
        {
          /* path information */
          guint16   type;
          gint      rec = pos;
          gint32    vectors;
          gdouble  *points          = NULL;
          gint      expected_points = 0;
          gint      pointcount      = 0;
          gboolean  closed          = FALSE;

          vectors = gimp_vectors_new (image, name);
          gimp_image_insert_vectors (image, vectors, -1, path_index);
          path_index++;

          while (rec < pos + len)
            {
              /* path records */
              val16 = (guint16 *) (bytes + rec);
              type = GUINT16_FROM_BE (*val16);

              switch (type)
                {
                case 0:  /* new closed subpath */
                case 3:  /* new open subpath */
                  val16 = (guint16 *) (bytes + rec + 2);
                  expected_points = GUINT16_FROM_BE (*val16);
                  pointcount = 0;
                  closed = (type == 0);

                  if (n_bytes - rec < (expected_points + 1) * 26)
                    {
                      g_printerr ("not enough point records\n");
                      rec = pos + len;
                      continue;
                    }

                  if (points)
                    g_free (points);
                  points = g_new (gdouble, expected_points * 6);
                  break;

                case 1:  /* closed subpath bezier knot, linked */
                case 2:  /* closed subpath bezier knot, unlinked */
                case 4:  /* open subpath bezier knot, linked */
                case 5:  /* open subpath bezier knot, unlinked */
                  /* since we already know if the subpath is open
                   * or closed and since we don't differentiate between
                   * linked and unlinked, just treat all the same...  */

                  if (pointcount < expected_points)
                    {
                      gint j;

                      for (j = 0; j < 6; j++)
                        {
                          gdouble f;
                          guint32 coord;

                          val32 = (guint32 *) (bytes + rec + 2 + j * 4);
                          coord = GUINT32_FROM_BE (*val32);

                          f = (double) ((gchar) ((coord >> 24) & 0xFF)) +
                              (double) (coord & 0x00FFFFFF) /
                              (double) 0xFFFFFF;

                          /* coords are stored with vertical component
                           * first, gimp expects the horizontal
                           * component first. Sigh.
                           */
                          points[pointcount * 6 + (j ^ 1)] = f * (j % 2 ? width : height);
                        }

                      pointcount++;

                      if (pointcount == expected_points)
                        {
                          gimp_vectors_stroke_new_from_points (vectors,
                                                               GIMP_VECTORS_STROKE_TYPE_BEZIER,
                                                               pointcount * 6,
                                                               points,
                                                               closed);
                        }
                    }
                  else
                    {
                      g_printerr ("Oops - unexpected point record\n");
                    }

                  break;

                case 6:  /* path fill rule record */
                case 7:  /* clipboard record (?) */
                case 8:  /* initial fill rule record (?) */
                  /* we cannot use this information */

                default:
                  break;
                }

              rec += 26;
            }

          if (points)
            g_free (points);
        }

      pos += len;

      if (pos % 2)  /* padding */
        pos++;

      g_free (name);
    }
}


static void
load_contiguous (TIFF        *tif,
                 ChannelData *channel,
                 const Babl  *type,
                 gushort      bps,
                 gushort      spp,
                 gboolean     is_bw,
                 gint         extra)
{
  guint32     image_width;
  guint32     image_height;
  guint32     tile_width;
  guint32     tile_height;
  gint        bytes_per_pixel;
  const Babl *src_format;
  guchar     *buffer;
  guchar     *bw_buffer = NULL;
  gdouble     progress  = 0.0;
  gdouble     one_row;
  guint32     y;
  gint        i;

  g_printerr ("%s\n", __func__);

  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH,  &image_width);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &image_height);

  tile_width = image_width;

  if (TIFFIsTiled (tif))
    {
      TIFFGetField (tif, TIFFTAG_TILEWIDTH,  &tile_width);
      TIFFGetField (tif, TIFFTAG_TILELENGTH, &tile_height);

      buffer = g_malloc (TIFFTileSize (tif));
    }
  else
    {
      tile_width  = image_width;
      tile_height = 1;

      buffer = g_malloc (TIFFScanlineSize (tif));
    }

  if (is_bw)
    bw_buffer = g_malloc (tile_width * tile_height);

  one_row = (gdouble) tile_height / (gdouble) image_height;

  src_format = babl_format_n (type, spp);

  /* consistency check */
  bytes_per_pixel = 0;
  for (i = 0; i <= extra; i++)
    bytes_per_pixel += babl_format_get_bytes_per_pixel (channel[i].format);

  g_printerr ("bytes_per_pixel: %d, format: %d\n",
              bytes_per_pixel,
              babl_format_get_bytes_per_pixel (src_format));

  for (y = 0; y < image_height; y += tile_height)
    {
      guint32 x;

      for (x = 0; x < image_width; x += tile_width)
        {
          GeglBuffer *src_buf;
          guint32     rows;
          guint32     cols;
          gint        offset;

          gimp_progress_update (progress + one_row *
                                ((gdouble) x / (gdouble) image_width));

          if (TIFFIsTiled (tif))
            TIFFReadTile (tif, buffer, x, y, 0, 0);
          else
            TIFFReadScanline (tif, buffer, y, 0);

          cols = MIN (image_width  - x, tile_width);
          rows = MIN (image_height - y, tile_height);

          if (is_bw)
            convert_bit2byte (buffer, bw_buffer, cols, rows);

          src_buf = gegl_buffer_linear_new_from_data (is_bw ? bw_buffer : buffer,
                                                      src_format,
                                                      GEGL_RECTANGLE (0, 0, cols, rows),
                                                      tile_width * bytes_per_pixel,
                                                      NULL, NULL);

          offset = 0;

          for (i = 0; i <= extra; i++)
            {
              GeglBufferIterator *iter;
              gint                src_bpp;
              gint                dest_bpp;

              src_bpp  = babl_format_get_bytes_per_pixel (src_format);
              dest_bpp = babl_format_get_bytes_per_pixel (channel[i].format);

              iter = gegl_buffer_iterator_new (src_buf,
                                               GEGL_RECTANGLE (0, 0, cols, rows),
                                               0, NULL,
                                               GEGL_ACCESS_READ,
                                               GEGL_ABYSS_NONE);
              gegl_buffer_iterator_add (iter, channel[i].buffer,
                                        GEGL_RECTANGLE (x, y, cols, rows),
                                        0, channel[i].format,
                                        GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

              while (gegl_buffer_iterator_next (iter))
                {
                  guchar *s      = iter->data[0];
                  guchar *d      = iter->data[1];
                  gint    length = iter->length;

                  s += offset;

                  while (length--)
                    {
                      memcpy (d, s, dest_bpp);
                      d += dest_bpp;
                      s += src_bpp;
                    }
                }

              offset += dest_bpp;
            }

          g_object_unref (src_buf);
        }

      progress += one_row;
    }

  g_free (buffer);
  g_free (bw_buffer);
}


static void
load_separate (TIFF        *tif,
               ChannelData *channel,
               const Babl  *type,
               gushort      bps,
               gushort      spp,
               gboolean     is_bw,
               gint         extra)
{
  guint32     image_width;
  guint32     image_height;
  guint32     tile_width;
  guint32     tile_height;
  gint        bytes_per_pixel;
  const Babl *src_format;
  guchar     *buffer;
  guchar     *bw_buffer = NULL;
  gdouble     progress  = 0.0;
  gdouble     one_row;
  gint        i, compindex;

  g_printerr ("%s\n", __func__);

  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH,  &image_width);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &image_height);

  tile_width = image_width;

  if (TIFFIsTiled (tif))
    {
      TIFFGetField (tif, TIFFTAG_TILEWIDTH,  &tile_width);
      TIFFGetField (tif, TIFFTAG_TILELENGTH, &tile_height);

      buffer = g_malloc (TIFFTileSize (tif));
    }
  else
    {
      tile_width  = image_width;
      tile_height = 1;

      buffer = g_malloc (TIFFScanlineSize (tif));
    }

  if (is_bw)
    bw_buffer = g_malloc (tile_width * tile_height);

  one_row = (gdouble) tile_height / (gdouble) image_height;

  src_format = babl_format_n (type, 1);

  /* consistency check */
  bytes_per_pixel = 0;
  for (i = 0; i <= extra; i++)
    bytes_per_pixel += babl_format_get_bytes_per_pixel (channel[i].format);

  g_printerr ("bytes_per_pixel: %d, format: %d\n",
              bytes_per_pixel,
              babl_format_get_bytes_per_pixel (src_format));

  compindex = 0;

  for (i = 0; i <= extra; i++)
    {
      gint n_comps;
      gint src_bpp;
      gint dest_bpp;
      gint offset;
      gint j;

      n_comps  = babl_format_get_n_components (channel[i].format);
      src_bpp  = babl_format_get_bytes_per_pixel (src_format);
      dest_bpp = babl_format_get_bytes_per_pixel (channel[i].format);

      offset = 0;

      for (j = 0; j < n_comps; j++)
        {
          guint32 y;

          for (y = 0; y < image_height; y += tile_height)
            {
              guint32 x;

              for (x = 0; x < image_width; x += tile_width)
                {
                  GeglBuffer         *src_buf;
                  GeglBufferIterator *iter;
                  guint32             rows;
                  guint32             cols;

                  gimp_progress_update (progress + one_row *
                                        ((gdouble) x / (gdouble) image_width));

                  if (TIFFIsTiled (tif))
                    TIFFReadTile (tif, buffer, x, y, 0, compindex);
                  else
                    TIFFReadScanline (tif, buffer, y, compindex);

                  cols = MIN (image_width  - x, tile_width);
                  rows = MIN (image_height - y, tile_height);

                  if (is_bw)
                    convert_bit2byte (buffer, bw_buffer, cols, rows);

                  src_buf = gegl_buffer_linear_new_from_data (is_bw ? bw_buffer : buffer,
                                                              src_format,
                                                              GEGL_RECTANGLE (0, 0, cols, rows),
                                                              GEGL_AUTO_ROWSTRIDE,
                                                              NULL, NULL);

                  iter = gegl_buffer_iterator_new (src_buf,
                                                   GEGL_RECTANGLE (0, 0, cols, rows),
                                                   0, NULL,
                                                   GEGL_ACCESS_READ,
                                                   GEGL_ABYSS_NONE);
                  gegl_buffer_iterator_add (iter, channel[i].buffer,
                                            GEGL_RECTANGLE (x, y, cols, rows),
                                            0, channel[i].format,
                                            GEGL_ACCESS_READWRITE,
                                            GEGL_ABYSS_NONE);

                  while (gegl_buffer_iterator_next (iter))
                    {
                      guchar *s      = iter->data[0];
                      guchar *d      = iter->data[1];
                      gint    length = iter->length;

                      d += offset;

                      while (length--)
                        {
                          memcpy (d, s, src_bpp);
                          d += dest_bpp;
                          s += src_bpp;
                        }
                    }

                  g_object_unref (src_buf);
                }
            }

          offset += src_bpp;
          compindex++;
        }

      progress += one_row;
    }

  g_free (buffer);
  g_free (bw_buffer);
}


static guchar bit2byte[256 * 8];

static void
fill_bit2byte (void)
{
  static gboolean filled = FALSE;

  guchar *dest;
  gint    i, j;

  if (filled)
    return;

  dest = bit2byte;

  for (j = 0; j < 256; j++)
    for (i = 7; i >= 0; i--)
      *(dest++) = ((j & (1 << i)) != 0);

  filled = TRUE;
}

static void
convert_bit2byte (const guchar *src,
                  guchar       *dest,
                  gint          width,
                  gint          height)
{
  gint y;

  for (y = 0; y < height; y++)
    {
      gint x = width;

      while (x >= 8)
        {
          memcpy (dest, bit2byte + *src * 8, 8);
          dest += 8;
          x -= 8;
          src++;
        }

      if (x > 0)
        {
          memcpy (dest, bit2byte + *src * 8, x);
          dest += x;
          src++;
        }
    }
}
