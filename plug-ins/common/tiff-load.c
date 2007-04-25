/* tiff loading for the GIMP
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

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-tiff-load"
#define PLUG_IN_BINARY "tiff-load"


typedef struct
{
  gint      compression;
  gint      fillorder;
  gboolean  save_transp_pixels;
} TiffSaveVals;

typedef struct
{
  gint32        ID;
  GimpDrawable *drawable;
  GimpPixelRgn  pixel_rgn;
  guchar       *pixels;
  guchar       *pixel;
} channel_data;

typedef struct
{
  gint  n_pages;
  gint *pages;
} TiffSelectedPages;

/* Declare some local functions.
 */
static void   query     (void);
static void   run       (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static gboolean  load_dialog   (TIFF              *tif,
                                TiffSelectedPages *pages);

static gint32    load_image    (const gchar       *filename,
                                TIFF              *tif,
                                TiffSelectedPages *pages);

static void      load_rgba     (TIFF         *tif,
                                channel_data *channel);
static void      load_lines    (TIFF         *tif,
                                channel_data *channel,
                                gushort       bps,
                                gushort       photomet,
                                gboolean      alpha,
                                gboolean      is_bw,
                                gint          extra);
static void      load_tiles    (TIFF         *tif,
                                channel_data *channel,
                                gushort       bps,
                                gushort       photomet,
                                gboolean      alpha,
                                gboolean      is_bw,
                                gint          extra);
static void      load_paths    (TIFF         *tif,
                                gint          image);

static void      read_separate (const guchar *source,
                                channel_data *channel,
                                gushort       bps,
                                gushort       photomet,
                                gint          startcol,
                                gint          startrow,
                                gint          rows,
                                gint          cols,
                                gboolean      alpha,
                                gint          extra,
                                gint          sample);
static void      read_16bit    (const guchar *source,
                                channel_data *channel,
                                gushort       photomet,
                                gint          startcol,
                                gint          startrow,
                                gint          rows,
                                gint          cols,
                                gboolean      alpha,
                                gint          extra,
                                gint          align);
static void      read_8bit     (const guchar *source,
                                channel_data *channel,
                                gushort       photomet,
                                gint          startcol,
                                gint          startrow,
                                gint          rows,
                                gint          cols,
                                gboolean      alpha,
                                gint          extra,
                                gint          align);
static void      read_bw       (const guchar *source,
                                channel_data *channel,
                                gint          startcol,
                                gint          startrow,
                                gint          rows,
                                gint          cols,
                                gint          align);
static void      read_default  (const guchar *source,
                                channel_data *channel,
                                gushort       bps,
                                gushort       photomet,
                                gint          startcol,
                                gint          startrow,
                                gint          rows,
                                gint          cols,
                                gboolean      alpha,
                                gint          extra,
                                gint          align);

static void      fill_bit2byte          (void);

static void      tiff_warning           (const gchar *module,
                                         const gchar *fmt,
                                         va_list      ap);
static void      tiff_error             (const gchar *module,
                                         const gchar *fmt,
                                         va_list      ap);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static TiffSaveVals tsvals =
{
  COMPRESSION_NONE,    /*  compression    */
  TRUE,                /*  alpha handling */
};


static GimpRunMode             run_mode      = GIMP_RUN_INTERACTIVE;
static GimpPageSelectorTarget  target        = GIMP_PAGE_SELECTOR_TARGET_LAYERS;

static guchar       bit2byte[256 * 8];


MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "loads files of the tiff file format",
                          "FIXME: write help for tiff_load",
                          "Spencer Kimball, Peter Mattis & Nick Lamb",
                          "Nick Lamb <njl195@zepler.org.uk>",
                          "1995-1996,1998-2003",
                          N_("TIFF image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/tiff");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "tif,tiff",
                                    "",
                                    "0,string,II*\\0,0,string,MM\\0*");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image;
  TiffSelectedPages  pages;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  TIFFSetWarningHandler (tiff_warning);
  TIFFSetErrorHandler (tiff_error);

  if (strcmp (name, LOAD_PROC) == 0)
    {
      TIFF       *tif;

      gimp_get_data (LOAD_PROC, &target);

      tif = TIFFOpen (param[1].data.d_string, "r");

      if (! tif)
        {
          g_message (_("Could not open '%s' for reading: %s"),
                     gimp_filename_to_utf8 (param[1].data.d_string), g_strerror (errno));
          status = GIMP_PDB_EXECUTION_ERROR;
        }
      else
        {
          pages.n_pages = TIFFNumberOfDirectories (tif);

          if (pages.n_pages == 0)
            {
              g_message (_("TIFF '%s' does not contain any directories"),
                         gimp_filename_to_utf8 (param[1].data.d_string));

              status = GIMP_PDB_EXECUTION_ERROR;
            }
          else
            {
              gboolean run_it = FALSE;
              gint     i;

              if (run_mode != GIMP_RUN_INTERACTIVE)
                {
                  pages.pages = g_new (gint, pages.n_pages);

                  for (i = 0; i < pages.n_pages; i++)
                    pages.pages[i] = i;

                  run_it = TRUE;
                }

              if (pages.n_pages == 1)
                {
                  target = GIMP_PAGE_SELECTOR_TARGET_LAYERS;
                  pages.pages = g_new0 (gint, pages.n_pages);

                  run_it = TRUE;
                }

              if ((! run_it) && (run_mode == GIMP_RUN_INTERACTIVE))
                run_it = load_dialog (tif, &pages);

              if (run_it)
                {
                  gimp_set_data (LOAD_PROC, &target, sizeof (target));

                  image = load_image (param[1].data.d_string, tif, &pages);

                  g_free (pages.pages);

                  if (image != -1)
                    {
                      *nreturn_vals = 2;
                      values[1].type         = GIMP_PDB_IMAGE;
                      values[1].data.d_image = image;
                    }
                  else
                    {
                      status = GIMP_PDB_EXECUTION_ERROR;
                    }
                }
              else
                status = GIMP_PDB_CANCEL;
            }
          TIFFClose (tif);
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static void
tiff_warning (const gchar *module,
              const gchar *fmt,
              va_list      ap)
{
  va_list ap_test;

  /* Workaround for: http://bugzilla.gnome.org/show_bug.cgi?id=131975 */
  /* Ignore the warnings about unregistered private tags (>= 32768) */
  if (! strcmp (fmt, "%s: unknown field with tag %d (0x%x) encountered"))
    {
      G_VA_COPY (ap_test, ap);
      if (va_arg (ap_test, char *));  /* ignore first argument */
      if (va_arg (ap_test, int) >= 32768)
        return;
    }
  /* for older versions of libtiff? */
  else if (! strcmp (fmt, "unknown field with tag %d (0x%x) ignored"))
    {
      G_VA_COPY (ap_test, ap);
      if (va_arg (ap_test, int) >= 32768)
        return;
    }

  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, fmt, ap);
}

static void
tiff_error (const gchar *module,
            const gchar *fmt,
            va_list      ap)
{
  /* Workaround for: http://bugzilla.gnome.org/show_bug.cgi?id=132297 */
  /* Ignore the errors related to random access and JPEG compression */
  if (! strcmp (fmt, "Compression algorithm does not support random access"))
    return;
  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, fmt, ap);
}

static gboolean
load_dialog (TIFF              *tif,
             TiffSelectedPages *pages)
{
  GtkWidget  *dialog;
  GtkWidget  *vbox;
  GtkWidget  *selector;
  gint        i;
  gboolean    run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Import from TIFF"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            _("_Import"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  /* Page Selector */
  selector = gimp_page_selector_new ();
  gtk_box_pack_start (GTK_BOX (vbox), selector, TRUE, TRUE, 0);

  gimp_page_selector_set_n_pages (GIMP_PAGE_SELECTOR (selector), pages->n_pages);
  gimp_page_selector_set_target (GIMP_PAGE_SELECTOR (selector), target);

  for (i = 0; i < pages->n_pages; i++)
    {
      gchar *name;

      if (! TIFFGetField (tif, TIFFTAG_PAGENAME, &name) ||
          ! g_utf8_validate (name, -1, NULL))
        {
          name = NULL;
        }

      if (name)
        gimp_page_selector_set_page_label (GIMP_PAGE_SELECTOR (selector), i, name);

      g_free (name);

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
    target = gimp_page_selector_get_target (GIMP_PAGE_SELECTOR (selector));

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

  return run;
}

static gint32
load_image (const gchar       *filename,
            TIFF              *tif,
            TiffSelectedPages *pages)
{
  gushort       bps, spp, photomet;
  guint16       orientation;
  gint          cols, rows;
  gboolean      alpha;
  gint          image = 0, image_type = GIMP_RGB;
  gint          layer, layer_type     = GIMP_RGB_IMAGE;
  float         layer_offset_x        = 0.0;
  float         layer_offset_y        = 0.0;
  gint          layer_offset_x_pixel  = 0;
  gint          layer_offset_y_pixel  = 0;
  gint          min_row = G_MAXINT;
  gint          min_col = G_MAXINT;
  gint          max_row = 0;
  gint          max_col = 0;
  gushort       extra, *extra_types;
  channel_data *channel = NULL;

  gushort      *redmap, *greenmap, *bluemap;
  GimpRGB       color;
  guchar        cmap[768];

  gboolean      is_bw;

  gint          i, j;
  gint          ilayer;
  gboolean      worst_case = FALSE;

  TiffSaveVals  save_vals;
  GimpParasite *parasite;
  guint16       tmp;

  gchar        *name;

  GList        *images_list = NULL, *images_list_temp;
  gboolean      do_images;
  gint          li;

  gboolean      flip_horizontal = FALSE;
  gboolean      flip_vertical   = FALSE;

#ifdef TIFFTAG_ICCPROFILE
  uint32        profile_size;
  guchar       *icc_profile;
#endif

  gimp_rgb_set (&color, 0.0, 0.0, 0.0);

  if ((pages->n_pages > 1) && (target == GIMP_PAGE_SELECTOR_TARGET_IMAGES))
    do_images = TRUE;
  else
    do_images = FALSE;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));
  /* We will loop through the all pages in case of multipage TIFF
     and load every page as a separate layer. */

  ilayer = 0;

  for (li = 0; li < pages->n_pages; li++)
    {
      TIFFSetDirectory (tif, pages->pages[li]);
      ilayer = pages->pages[li];

      gimp_progress_update (0.0);

      TIFFGetFieldDefaulted (tif, TIFFTAG_BITSPERSAMPLE, &bps);

      if (bps > 8 && bps != 16)
        worst_case = TRUE; /* Wrong sample width => RGBA */

      TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &spp);

      if (!TIFFGetField (tif, TIFFTAG_EXTRASAMPLES, &extra, &extra_types))
        extra = 0;

      if (!TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &cols))
        {
          g_message ("Could not get image width from '%s'",
                     gimp_filename_to_utf8 (filename));
          return -1;
        }

      if (!TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &rows))
        {
          g_message ("Could not get image length from '%s'",
                     gimp_filename_to_utf8 (filename));
          return -1;
        }

      if (!TIFFGetField (tif, TIFFTAG_PHOTOMETRIC, &photomet))
        {
          uint16 compress;
          if (TIFFGetField(tif, TIFFTAG_COMPRESSION, &compress) &&
              (compress == COMPRESSION_CCITTFAX3 ||
               compress == COMPRESSION_CCITTFAX4 ||
               compress == COMPRESSION_CCITTRLE ||
               compress == COMPRESSION_CCITTRLEW))
            {
              g_message ("Could not get photometric from '%s'. "
                         "Image is CCITT compressed, assuming min-is-white",
                         filename);
              photomet = PHOTOMETRIC_MINISWHITE;
            }
          else
            {
              g_message ("Could not get photometric from '%s'. "
                         "Assuming min-is-black",
                         filename);
              /* old AppleScan software misses out the photometric tag (and
               * incidentally assumes min-is-white, but xv assumes
               * min-is-black, so we follow xv's lead.  It's not much hardship
               * to invert the image later). */
              photomet = PHOTOMETRIC_MINISBLACK;
            }
        }

      /* test if the extrasample represents an associated alpha channel... */
      if (extra > 0 && (extra_types[0] == EXTRASAMPLE_ASSOCALPHA))
        {
          alpha = TRUE;
          tsvals.save_transp_pixels = FALSE;
          --extra;
        }
      else if (extra > 0 && (extra_types[0] == EXTRASAMPLE_UNASSALPHA))
        {
          alpha = TRUE;
          tsvals.save_transp_pixels = TRUE;
          --extra;
        }
      else if (extra > 0 && (extra_types[0] == EXTRASAMPLE_UNSPECIFIED))
        {
          /* assuming unassociated alpha if unspecified */
          g_message ("alpha channel type not defined for file %s. "
                     "Assuming alpha is not premultiplied",
                     gimp_filename_to_utf8 (filename));
          alpha = TRUE;
          tsvals.save_transp_pixels = TRUE;
          --extra;
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
          if (bps == 1 && !alpha && spp == 1)
            {
              image_type = GIMP_INDEXED;
              layer_type = GIMP_INDEXED_IMAGE;

              is_bw = TRUE;
              fill_bit2byte ();
            }
          else
            {
              image_type = GIMP_GRAY;
              layer_type = (alpha) ? GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE;
            }
          break;

        case PHOTOMETRIC_RGB:
          image_type = GIMP_RGB;
          layer_type = (alpha) ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE;
          break;

        case PHOTOMETRIC_PALETTE:
          image_type = GIMP_INDEXED;
          layer_type = (alpha) ? GIMP_INDEXEDA_IMAGE : GIMP_INDEXED_IMAGE;
          break;

        default:
          worst_case = TRUE;
          break;
        }

      if (worst_case)
        {
          image_type = GIMP_RGB;
          layer_type = GIMP_RGBA_IMAGE;
        }

      if (do_images || (! image))
        {
          if ((image = gimp_image_new (cols, rows, image_type)) == -1)
            {
              g_message ("Could not create a new image");
              return -1;
            }

          gimp_image_undo_disable (image);

          if (do_images)
            {
              gchar *fname;
              fname = g_strdup_printf ("%s-%d", filename, ilayer);
              gimp_image_set_filename (image, fname);
              g_free (fname);

              images_list = g_list_append (images_list, GINT_TO_POINTER (image));
            }
          else
            {
              gimp_image_set_filename (image, filename);
            }
        }


      /* attach a parasite containing an ICC profile - if found in the TIFF */

#ifdef TIFFTAG_ICCPROFILE
      /* If TIFFTAG_ICCPROFILE is defined we are dealing with a libtiff version
       * that can handle ICC profiles. Otherwise just ignore this section. */
      if (TIFFGetField (tif, TIFFTAG_ICCPROFILE, &profile_size, &icc_profile))
        {
          parasite = gimp_parasite_new ("icc-profile", 0,
                                        profile_size, icc_profile);
          gimp_image_parasite_attach (image, parasite);
          gimp_parasite_free (parasite);
        }
#endif

      /* attach a parasite containing the compression */
      if (!TIFFGetField (tif, TIFFTAG_COMPRESSION, &tmp))
        {
          save_vals.compression = COMPRESSION_NONE;
        }
      else
        {
          switch (tmp)
            {
            case COMPRESSION_NONE:
            case COMPRESSION_LZW:
            case COMPRESSION_PACKBITS:
            case COMPRESSION_DEFLATE:
            case COMPRESSION_JPEG:
            case COMPRESSION_CCITTFAX3:
            case COMPRESSION_CCITTFAX4:
              save_vals.compression = tmp;
              break;

            default:
              save_vals.compression = COMPRESSION_NONE;
              break;
            }
        }

      parasite = gimp_parasite_new ("tiff-save-options", 0,
                                    sizeof (save_vals), &save_vals);
      gimp_image_parasite_attach (image, parasite);
      gimp_parasite_free (parasite);

      /* Attach a parasite containing the image description.  Pretend to
       * be a gimp comment so other plugins will use this description as
       * an image comment where appropriate. */
      {
        const gchar *img_desc;

        if (TIFFGetField (tif, TIFFTAG_IMAGEDESCRIPTION, &img_desc) &&
            g_utf8_validate (img_desc, -1, NULL))
          {
            parasite = gimp_parasite_new ("gimp-comment",
                                          GIMP_PARASITE_PERSISTENT,
                                          strlen (img_desc) + 1, img_desc);
            gimp_image_parasite_attach (image, parasite);
            gimp_parasite_free (parasite);
          }
      }

      /* any resolution info in the file? */
      {
        gfloat   xres = 72.0, yres = 72.0;
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
                        g_message ("Warning: resolution units meaningless");
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
                  { /* no res unit tag */
                    /* old AppleScan software produces these */
                    g_message ("Warning: resolution specified without "
                               "any units tag, assuming dpi");
                  }
              }
            else
              { /* xres but no yres */
                g_message ("Warning: no y resolution info, assuming same as x");
                yres = xres;
              }

            /* now set the new image's resolution info */

            /* If it is invalid, instead of forcing 72dpi, do not set the
               resolution at all. Gimp will then use the default set by
               the user */
            if (read_unit != RESUNIT_NONE)
              {
                gimp_image_set_resolution (image, xres, yres);
                if (unit != GIMP_UNIT_PIXEL)
                  gimp_image_set_unit (image, unit);
              }
          }

        /* no x res tag => we assume we have no resolution info, so we
         * don't care.  Older versions of this plugin used to write files
         * with no resolution tags at all. */

        /* TODO: haven't caught the case where yres tag is present, but
           not xres.  This is left as an exercise for the reader - they
           should feel free to shoot the author of the broken program
           that produced the damaged TIFF file in the first place. */

        /* handle layer offset */
        if (!TIFFGetField (tif, TIFFTAG_XPOSITION, &layer_offset_x))
          layer_offset_x = 0.0;
        if (!TIFFGetField (tif, TIFFTAG_YPOSITION, &layer_offset_y))
          layer_offset_y = 0.0;

        /* round floating point position to integer position
           required by GIMP */
        layer_offset_x_pixel = ROUND(layer_offset_x * xres);
        layer_offset_y_pixel = ROUND(layer_offset_y * yres);
      }


      /* Install colormap for INDEXED images only */
      if (image_type == GIMP_INDEXED)
        {
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
              if (!TIFFGetField (tif, TIFFTAG_COLORMAP,
                                 &redmap, &greenmap, &bluemap))
                {
                  g_message ("Could not get colormaps from '%s'",
                             gimp_filename_to_utf8 (filename));
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

      /* Allocate channel_data for all channels, even the background layer */
      channel = g_new0 (channel_data, extra + 1);

      /* try and use layer name from tiff file */
      if (! TIFFGetField (tif, TIFFTAG_PAGENAME, &name) ||
          ! g_utf8_validate (name, -1, NULL))
        {
          name = NULL;
        }

      if (name)
        {
          layer = gimp_layer_new (image, name,
                                  cols, rows,
                                  layer_type, 100, GIMP_NORMAL_MODE);
        }
      else
        {
          if (ilayer == 0)
            name = g_strdup (_("Background"));
          else
            name = g_strdup_printf (_("Page %d"), ilayer);

          layer = gimp_layer_new (image, name,
                                  cols, rows,
                                  layer_type, 100, GIMP_NORMAL_MODE);
          g_free (name);
        }

      channel[0].ID       = layer;
      channel[0].drawable = gimp_drawable_get (layer);

      if (extra > 0 && !worst_case)
        {
          /* Add alpha channels as appropriate */
          for (i = 1; i <= extra; ++i)
            {
              channel[i].ID = gimp_channel_new (image, _("TIFF Channel"),
                                                cols, rows,
                                                100.0, &color);
              gimp_image_add_channel (image, channel[i].ID, 0);
              channel[i].drawable = gimp_drawable_get (channel[i].ID);
            }
        }

      if (bps == 16)
        g_message (_("Warning:\n"
                     "The image you are loading has 16 bits per channel. GIMP "
                     "can only handle 8 bit, so it will be converted for you. "
                     "Information will be lost because of this conversion."));

      if (worst_case)
        {
          load_rgba (tif, channel);
        }
      else if (TIFFIsTiled (tif))
        {
          load_tiles (tif, channel, bps, photomet, alpha, is_bw, extra);
        }
      else
        { /* Load scanlines in tile_height chunks */
          load_lines (tif, channel, bps, photomet, alpha, is_bw, extra);
        }

      if (TIFFGetField (tif, TIFFTAG_ORIENTATION, &orientation))
        {
          switch (orientation)
            {
            case ORIENTATION_TOPLEFT:
              flip_horizontal = FALSE;
              flip_vertical   = FALSE;
              break;
            case ORIENTATION_TOPRIGHT:
              flip_horizontal = TRUE;
              flip_vertical   = FALSE;
              break;
            case ORIENTATION_BOTRIGHT:
              flip_horizontal = TRUE;
              flip_vertical   = TRUE;
              break;
            case ORIENTATION_BOTLEFT:
              flip_horizontal = FALSE;
              flip_vertical   = TRUE;
              break;
            default:
              flip_horizontal = FALSE;
              flip_vertical   = FALSE;
              g_warning ("Orientation %d not handled yet!", orientation);
              break;
            }

          if (flip_horizontal)
            gimp_drawable_transform_flip_simple (layer,
                                                 GIMP_ORIENTATION_HORIZONTAL,
                                                 TRUE, 0.0, TRUE);

          if (flip_vertical)
            gimp_drawable_transform_flip_simple (layer,
                                                 GIMP_ORIENTATION_VERTICAL,
                                                 TRUE, 0.0, TRUE);
        }

      gimp_drawable_flush (channel[0].drawable);
      gimp_drawable_detach (channel[0].drawable);

      for (i = 1; !worst_case && i < extra; ++i)
        {
          gimp_drawable_flush (channel[i].drawable);
          gimp_drawable_detach (channel[i].drawable);
        }

      g_free (channel);
      channel = NULL;

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
      if (layer_offset_x_pixel > 0 || layer_offset_y_pixel > 0)
        {
          gimp_layer_set_offsets (layer,
                                  layer_offset_x_pixel, layer_offset_y_pixel);
        }


      if (ilayer > 0 && !alpha)
        gimp_layer_add_alpha (layer);

      gimp_image_add_layer (image, layer, -1);

      if (do_images)
        {
          gimp_image_undo_enable (image);
          gimp_image_clean_all (image);
        }

      gimp_progress_update (1.0);
    }

  if (! do_images)
    {
      /* resize image to bounding box of all layers */
      gimp_image_resize (image,
                     max_col - min_col, max_row - min_row, -min_col, -min_row);

      gimp_image_undo_enable (image);
    }
  else
    {
      images_list_temp = images_list;

      if (images_list)
        {
          image = GPOINTER_TO_INT (images_list->data);
          images_list = images_list->next;
        }

      while (images_list)
        {
          gimp_display_new (GPOINTER_TO_INT (images_list->data));
          images_list = images_list->next;
        }

      g_list_free (images_list_temp);
    }

  return image;
}

static void
load_rgba (TIFF         *tif,
           channel_data *channel)
{
  uint32  imageWidth, imageLength;
  uint32  row;
  uint32 *buffer;

  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &imageLength);

  gimp_tile_cache_ntiles (1 + imageWidth / gimp_tile_width ());

  gimp_pixel_rgn_init (&(channel[0].pixel_rgn), channel[0].drawable,
                       0, 0, imageWidth, imageLength, TRUE, FALSE);

  buffer = g_new (uint32, imageWidth * imageLength);
  channel[0].pixels = (guchar*) buffer;

  if (!TIFFReadRGBAImage (tif, imageWidth, imageLength, buffer, 0))
    g_message ("Unsupported layout, no RGBA loader");

  for (row = 0; row < imageLength; ++row)
    {
#if G_BYTE_ORDER != G_LITTLE_ENDIAN
      /* Make sure our channels are in the right order */
      uint32 i;
      uint32 rowStart = row * imageWidth;
      uint32 rowEnd   = rowStart + imageWidth;

      for (i = rowStart; i < rowEnd; i++)
        buffer[i] = GUINT32_TO_LE (buffer[i]);
#endif
      gimp_pixel_rgn_set_rect (&(channel[0].pixel_rgn),
                               channel[0].pixels + row * imageWidth * 4,
                               0, imageLength -row -1, imageWidth, 1);

      gimp_progress_update ((gdouble) row / (gdouble) imageLength);
    }
}

static void
load_paths (TIFF *tif, gint image)
{
  guint16 id;
  guint32 len, n_bytes, pos;
  gchar *bytes, *name;
  guint32 *val32;
  guint16 *val16;

  gint width, height;

  width = gimp_image_width (image);
  height = gimp_image_height (image);

#if 0
  /* TIFFTAG_CLIPPATH seems to be basically unused */

  if (TIFFGetField (tif, TIFFTAG_CLIPPATH, &n_bytes, &bytes) &&
      TIFFGetField (tif, TIFFTAG_XCLIPPATHUNITS, &xclipunits) &&
      TIFFGetField (tif, TIFFTAG_YCLIPPATHUNITS, &yclipunits))
    {
      /* Tiff clipping path */

      g_printerr ("Tiff clipping path, %d - %d\n", xclipunits, yclipunits);
    }
#endif

  if (!TIFFGetField (tif, TIFFTAG_PHOTOSHOP, &n_bytes, &bytes))
    return;

  pos = 0;

  while (pos < n_bytes)
    {
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

      name = g_strndup (bytes + pos + 1, len);
      if (!g_utf8_validate (name, -1, NULL))
        name = g_strdup ("imported path");
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
          guint16 type;
          gint rec = pos;
          gint32   vectors;
          gdouble *points = NULL;
          gint     expected_points = 0;
          gint     pointcount = 0;
          gboolean closed = FALSE;

          vectors = gimp_vectors_new (image, name);
          gimp_image_add_vectors (image, vectors, -1);

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
                   * or closed and since we don't differenciate between
                   * linked and unlinked, just treat all the same...  */

                  if (pointcount < expected_points)
                    {
                      gint    j;
                      gdouble f;
                      guint32 coord;

                      for (j = 0; j < 6; j++)
                        {
                          val32 = (guint32 *) (bytes + rec + 2 + j * 4);
                          coord = GUINT32_FROM_BE (*val32);

                          f = (double) ((coord >> 24) & 0x7F) +
                                 (double) (coord & 0x00FFFFFF) /
                                 (double) 0xFFFFFF;
                          if (coord & 0x80000000)
                            f *= -1;

                          /* coords are stored with vertical component
                           * first, gimp expects the horizontal component
                           * first. Sigh.  */
                          points[pointcount * 6 + (j ^ 1)] = f * (j % 2 ? width : height);
                        }

                      pointcount ++;

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
load_tiles (TIFF         *tif,
            channel_data *channel,
            gushort       bps,
            gushort       photomet,
            gboolean      alpha,
            gboolean      is_bw,
            gint          extra)
{
  uint16  planar = PLANARCONFIG_CONTIG;
  uint32  imageWidth, imageLength;
  uint32  tileWidth, tileLength;
  uint32  x, y, rows, cols;
  guchar *buffer;
  gdouble progress = 0.0, one_row;
  gint    i;

  TIFFGetField (tif, TIFFTAG_PLANARCONFIG, &planar);
  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &imageLength);
  TIFFGetField (tif, TIFFTAG_TILEWIDTH, &tileWidth);
  TIFFGetField (tif, TIFFTAG_TILELENGTH, &tileLength);

  if (tileWidth > gimp_tile_width () || tileLength > gimp_tile_height ())
    {
      gimp_tile_cache_ntiles ((1 + tileWidth / gimp_tile_width ()) *
                              (1 + tileLength / gimp_tile_width ()));
    }

  one_row = (gdouble) tileLength / (gdouble) imageLength;
  buffer = g_malloc (TIFFTileSize (tif));

  for (i = 0; i <= extra; ++i)
    {
      channel[i].pixels = g_new (guchar,
                                 tileWidth * tileLength *
                                 channel[i].drawable->bpp);
    }

  for (y = 0; y < imageLength; y += tileLength)
    {
      for (x = 0; x < imageWidth; x += tileWidth)
        {
          gimp_progress_update (progress + one_row *
                                ( (gdouble) x / (gdouble) imageWidth));

          TIFFReadTile (tif, buffer, x, y, 0, 0);

          cols = MIN (imageWidth - x, tileWidth);
          rows = MIN (imageLength - y, tileLength);

          if (bps == 16)
            {
              read_16bit (buffer, channel, photomet, y, x, rows, cols, alpha,
                          extra, tileWidth - cols);
            }
          else if (bps == 8)
            {
              read_8bit (buffer, channel, photomet, y, x, rows, cols, alpha,
                         extra, tileWidth - cols);
            }
          else if (is_bw)
            {
              read_bw (buffer, channel, y, x, rows, cols, tileWidth - cols);
            }
          else
            {
              read_default (buffer, channel, bps, photomet, y, x, rows, cols,
                            alpha, extra, tileWidth - cols);
            }
        }

      progress += one_row;
    }

  for (i = 0; i <= extra; ++i)
    g_free(channel[i].pixels);

  g_free(buffer);
}

static void
load_lines (TIFF         *tif,
            channel_data *channel,
            gushort       bps,
            gushort       photomet,
            gboolean      alpha,
            gboolean      is_bw,
            gint          extra)
{
  uint16  planar = PLANARCONFIG_CONTIG;
  uint32  imageLength, lineSize, cols, rows;
  guchar *buffer;
  gint    i, y;
  gint    tile_height = gimp_tile_height ();

  TIFFGetField (tif, TIFFTAG_PLANARCONFIG, &planar);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &imageLength);
  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &cols);

  lineSize = TIFFScanlineSize (tif);

  for (i = 0; i <= extra; ++i)
    {
      channel[i].pixels = g_new (guchar,
                                 tile_height * cols * channel[i].drawable->bpp);
    }

  gimp_tile_cache_ntiles (1 + cols / gimp_tile_width ());

  buffer = g_malloc (lineSize * tile_height);

  if (planar == PLANARCONFIG_CONTIG)
    {
      for (y = 0; y < imageLength; y += tile_height )
        {
          gimp_progress_update ((gdouble) y / (gdouble) imageLength);

          rows = MIN (tile_height, imageLength - y);

          for (i = 0; i < rows; ++i)
            TIFFReadScanline (tif, buffer + i * lineSize, y + i, 0);

          if (bps == 16)
            {
              read_16bit (buffer, channel, photomet, y, 0, rows, cols,
                          alpha, extra, 0);
            }
          else if (bps == 8)
            {
              read_8bit (buffer, channel, photomet, y, 0, rows, cols,
                         alpha, extra, 0);
            }
          else if (is_bw)
            {
              read_bw (buffer, channel, y, 0, rows, cols, 0);
            }
          else
            {
              read_default (buffer, channel, bps, photomet, y, 0, rows, cols,
                            alpha, extra, 0);
            }
        }
    }
  else
    { /* PLANARCONFIG_SEPARATE  -- Just say "No" */
      uint16 s, samples;

      TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samples);

      for (s = 0; s < samples; ++s)
        {
          for (y = 0; y < imageLength; y += tile_height )
            {
              gimp_progress_update ((gdouble) y / (gdouble) imageLength);

              rows = MIN (tile_height, imageLength - y);
              for (i = 0; i < rows; ++i)
                TIFFReadScanline(tif, buffer + i * lineSize, y + i, s);

              read_separate (buffer, channel, bps, photomet,
                             y, 0, rows, cols, alpha, extra, s);
            }
        }
    }

  for (i = 0; i <= extra; ++i)
    g_free(channel[i].pixels);

  g_free(buffer);
}

static void
read_16bit (const guchar *source,
            channel_data *channel,
            gushort       photomet,
            gint          startrow,
            gint          startcol,
            gint          rows,
            gint          cols,
            gboolean      alpha,
            gint          extra,
            gint          align)
{
  guchar *dest;
  gint    gray_val, red_val, green_val, blue_val, alpha_val;
  gint    col, row, i;

  for (i = 0; i <= extra; ++i)
    {
      gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                           startcol, startrow, cols, rows, TRUE, FALSE);
    }

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  source++; /* offset source once, to look at the high byte */
#endif

  for (row = 0; row < rows; ++row)
    {
      dest = channel[0].pixels + row * cols * channel[0].drawable->bpp;

      for (i = 1; i <= extra; ++i)
        channel[i].pixel = channel[i].pixels + row * cols;

      for (col = 0; col < cols; col++)
        {
          switch (photomet)
            {
            case PHOTOMETRIC_MINISBLACK:
              if (alpha)
                {
                  if (tsvals.save_transp_pixels)
                    {
                      *dest++ = *source; source += 2;
                      *dest++ = *source; source += 2;
                    }
                  else
                    {
                      gray_val  = *source; source += 2;
                      alpha_val = *source; source += 2;
                      gray_val  = MIN (gray_val, alpha_val);

                      if (alpha_val)
                        *dest++ = gray_val * 255 / alpha_val;
                      else
                        *dest++ = 0;

                      *dest++ = alpha_val;
                    }
                }
              else
                {
                  *dest++ = *source; source += 2;
                }
              break;

            case PHOTOMETRIC_MINISWHITE:
              if (alpha)
                {
                  if (tsvals.save_transp_pixels)
                    {
                      *dest++ = *source; source += 2;
                      *dest++ = *source; source += 2;
                    }
                  else
                    {
                      gray_val  = *source; source += 2;
                      alpha_val = *source; source += 2;
                      gray_val  = MIN (gray_val, alpha_val);

                      if (alpha_val)
                        *dest++ = ((alpha_val - gray_val) * 255) / alpha_val;
                      else
                        *dest++ = 0;

                      *dest++ = alpha_val;
                    }
                }
              else
                {
                  *dest++ = ~(*source); source += 2;
                }
              break;

            case PHOTOMETRIC_PALETTE:
              *dest++= *source; source += 2;
              if (alpha) *dest++= *source; source += 2;
              break;

            case PHOTOMETRIC_RGB:
              if (alpha)
                {
                  if (tsvals.save_transp_pixels)
                    {
                      *dest++ = *source; source += 2;
                      *dest++ = *source; source += 2;
                      *dest++ = *source; source += 2;
                      *dest++ = *source; source += 2;
                    }
                  else
                    {
                      red_val   = *source; source += 2;
                      green_val = *source; source += 2;
                      blue_val  = *source; source += 2;
                      alpha_val = *source; source += 2;
                      red_val   = MIN (red_val,   alpha_val);
                      green_val = MIN (green_val, alpha_val);
                      blue_val  = MIN (blue_val,  alpha_val);
                      if (alpha_val)
                        {
                          *dest++ = (red_val   * 255) / alpha_val;
                          *dest++ = (green_val * 255) / alpha_val;
                          *dest++ = (blue_val  * 255) / alpha_val;
                        }
                      else
                        {
                          *dest++ = 0;
                          *dest++ = 0;
                          *dest++ = 0;
                        }
                      *dest++ = alpha_val;
                    }
                }
              else
                {
                  *dest++ = *source; source += 2;
                  *dest++ = *source; source += 2;
                  *dest++ = *source; source += 2;
                }
              break;

            default:
              /* This case was handled earlier */
              g_assert_not_reached();
            }

          for (i = 1; i <= extra; ++i)
            {
              *channel[i].pixel++ = *source; source += 2;
            }
        }

      if (align)
        {
          switch (photomet)
            {
            case PHOTOMETRIC_MINISBLACK:
            case PHOTOMETRIC_MINISWHITE:
            case PHOTOMETRIC_PALETTE:
              source += align * (1 + alpha + extra) * 2;
              break;
            case PHOTOMETRIC_RGB:
              source += align * (3 + alpha + extra) * 2;
              break;
            }
        }
    }

  for (i = 0; i <= extra; ++i)
    gimp_pixel_rgn_set_rect (&(channel[i].pixel_rgn), channel[i].pixels,
                             startcol, startrow, cols, rows);
}

static void
read_8bit (const guchar *source,
           channel_data *channel,
           gushort       photomet,
           gint          startrow,
           gint          startcol,
           gint          rows,
           gint          cols,
           gboolean      alpha,
           gint          extra,
           gint          align)
{
  guchar *dest;
  gint    gray_val, red_val, green_val, blue_val, alpha_val;
  gint    col, row, i;

  for (i = 0; i <= extra; ++i)
    {
      gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                           startcol, startrow, cols, rows, TRUE, FALSE);
    }

  for (row = 0; row < rows; ++row)
    {
      dest = channel[0].pixels + row * cols * channel[0].drawable->bpp;

      for (i = 1; i <= extra; ++i)
        channel[i].pixel = channel[i].pixels + row * cols;

      for (col = 0; col < cols; col++)
        {
          switch (photomet)
            {
            case PHOTOMETRIC_MINISBLACK:
              if (alpha)
                {
                  if (tsvals.save_transp_pixels)
                    {
                      *dest++ = *source; source++;
                      *dest++ = *source; source++;
                    }
                  else
                    {
                      gray_val = *source++;
                      alpha_val = *source++;
                      gray_val = MIN(gray_val, alpha_val);
                      if (alpha_val)
                        *dest++ = gray_val * 255 / alpha_val;
                      else
                        *dest++ = 0;
                      *dest++ = alpha_val;
                    }
                }
              else
                {
                  *dest++ = *source++;
                }
              break;

            case PHOTOMETRIC_MINISWHITE:
              if (alpha)
                {
                  if (tsvals.save_transp_pixels)
                    {
                      *dest++ = *source; source++;
                      *dest++ = *source; source++;
                    }
                  else
                    {
                      gray_val  = *source++;
                      alpha_val = *source++;
                      gray_val  = MIN (gray_val, alpha_val);

                      if (alpha_val)
                        *dest++ = ((alpha_val - gray_val) * 255) / alpha_val;
                      else
                        *dest++ = 0;
                      *dest++ = alpha_val;
                    }
                }
              else
                {
                  *dest++ = ~(*source++);
                }
              break;

            case PHOTOMETRIC_PALETTE:
              *dest++= *source++;
              if (alpha)
                *dest++= *source++;
              break;

            case PHOTOMETRIC_RGB:
              if (alpha)
                {
                  if (tsvals.save_transp_pixels)
                    {
                      *dest++ = *source; source++;
                      *dest++ = *source; source++;
                      *dest++ = *source; source++;
                      *dest++ = *source; source++;
                    }
                  else
                    {
                      red_val   = *source++;
                      green_val = *source++;
                      blue_val  = *source++;
                      alpha_val = *source++;
                      red_val   = MIN (red_val, alpha_val);
                      blue_val  = MIN (blue_val, alpha_val);
                      green_val = MIN (green_val, alpha_val);

                      if (alpha_val)
                        {
                          *dest++ = (red_val   * 255) / alpha_val;
                          *dest++ = (green_val * 255) / alpha_val;
                          *dest++ = (blue_val  * 255) / alpha_val;
                        }
                      else
                        {
                          *dest++ = 0;
                          *dest++ = 0;
                          *dest++ = 0;
                        }
                      *dest++ = alpha_val;
                    }
                }
              else
                {
                  *dest++ = *source++;
                  *dest++ = *source++;
                  *dest++ = *source++;
                }
              break;

            default:
              /* This case was handled earlier */
              g_assert_not_reached();
            }

          for (i = 1; i <= extra; ++i)
            *channel[i].pixel++ = *source++;
        }

      if (align)
        {
          switch (photomet)
            {
            case PHOTOMETRIC_MINISBLACK:
            case PHOTOMETRIC_MINISWHITE:
            case PHOTOMETRIC_PALETTE:
              source += align * (1 + alpha + extra);
              break;
            case PHOTOMETRIC_RGB:
              source += align * (3 + alpha + extra);
              break;
            }
        }
    }

  for (i = 0; i <= extra; ++i)
    gimp_pixel_rgn_set_rect(&(channel[i].pixel_rgn), channel[i].pixels,
                            startcol, startrow, cols, rows);
}

static void
read_bw (const guchar *source,
         channel_data *channel,
         gint          startrow,
         gint          startcol,
         gint          rows,
         gint          cols,
         gint          align)
{
  guchar *dest;
  gint    col, row;

  gimp_pixel_rgn_init (&(channel[0].pixel_rgn), channel[0].drawable,
                       startcol, startrow, cols, rows, TRUE, FALSE);

  for (row = 0; row < rows; ++row)
    {
      dest = channel[0].pixels + row * cols * channel[0].drawable->bpp;

      col = cols;

      while (col >= 8)
        {
          memcpy (dest, bit2byte + *source * 8, 8);
          dest += 8;
          col -= 8;
          source++;
        }

      if (col > 0)
        {
          memcpy (dest, bit2byte + *source * 8, col);
          dest += col;
          source++;
        }

      source += align;
    }

  gimp_pixel_rgn_set_rect(&(channel[0].pixel_rgn), channel[0].pixels,
                          startcol, startrow, cols, rows);
}

/* Step through all <= 8-bit samples in an image */

#define NEXTSAMPLE(var)                       \
  {                                           \
      if (bitsleft == 0)                      \
      {                                       \
          source++;                           \
          bitsleft = 8;                       \
      }                                       \
      bitsleft -= bps;                        \
      var = ( *source >> bitsleft ) & maxval; \
  }

static void
read_default (const guchar *source,
              channel_data *channel,
              gushort       bps,
              gushort       photomet,
              gint          startrow,
              gint          startcol,
              gint          rows,
              gint          cols,
              gboolean      alpha,
              gint          extra,
              gint          align)
{
  guchar *dest;
  gint    gray_val, red_val, green_val, blue_val, alpha_val;
  gint    col, row, i;
  gint    bitsleft = 8, maxval = (1 << bps) - 1;

  for (i = 0; i <= extra; ++i)
    {
      gimp_pixel_rgn_init (&(channel[i].pixel_rgn), channel[i].drawable,
                           startcol, startrow, cols, rows, TRUE, FALSE);
    }

  for (row = 0; row < rows; ++row)
    {
      dest = channel[0].pixels + row * cols * channel[0].drawable->bpp;

      for (i = 1; i <= extra; ++i)
        channel[i].pixel = channel[i].pixels + row * cols;

      for (col = 0; col < cols; col++)
        {
          switch (photomet)
            {
            case PHOTOMETRIC_MINISBLACK:
              NEXTSAMPLE (gray_val);
              if (alpha)
                {
                  NEXTSAMPLE (alpha_val);
                  if (tsvals.save_transp_pixels)
                    {
                      *dest++ = (gray_val * 255) / maxval;
                      *dest++ = alpha_val;
                    }
                  else
                    {
                      gray_val = MIN (gray_val, alpha_val);

                      if (alpha_val)
                        *dest++ = (gray_val * 65025) / (alpha_val * maxval);
                      else
                        *dest++ = 0;

                      *dest++ = alpha_val;
                    }
                }
              else
                {
                  *dest++ = (gray_val * 255) / maxval;
                }
              break;

            case PHOTOMETRIC_MINISWHITE:
              NEXTSAMPLE (gray_val);
              if (alpha)
                {
                  NEXTSAMPLE (alpha_val);
                  if (tsvals.save_transp_pixels)
                    {
                      *dest++ = ((maxval - gray_val) * 255) / maxval;
                      *dest++ = alpha_val;
                    }
                  else
                    {
                      gray_val = MIN (gray_val, alpha_val);

                      if (alpha_val)
                        *dest++ = ((maxval - gray_val) * 65025) /
                                   (alpha_val * maxval);
                      else
                        *dest++ = 0;

                      *dest++ = alpha_val;
                    }

                }
              else
                {
                  *dest++ = ((maxval - gray_val) * 255) / maxval;
                }
              break;

            case PHOTOMETRIC_PALETTE:
              NEXTSAMPLE (*dest++);
              if (alpha)
                NEXTSAMPLE (*dest++);
              break;

            case PHOTOMETRIC_RGB:
              NEXTSAMPLE (red_val);
              NEXTSAMPLE (green_val);
              NEXTSAMPLE (blue_val);
              if (alpha)
                {
                  NEXTSAMPLE (alpha_val);
                  if (tsvals.save_transp_pixels)
                    {
                      *dest++ = red_val;
                      *dest++ = green_val;
                      *dest++ = blue_val;
                      *dest++ = alpha_val;
                    }
                  else
                    {
                      red_val   = MIN (red_val, alpha_val);
                      blue_val  = MIN (blue_val, alpha_val);
                      green_val = MIN (green_val, alpha_val);

                      if (alpha_val)
                        {
                          *dest++ = (red_val   * 255) / alpha_val;
                          *dest++ = (green_val * 255) / alpha_val;
                          *dest++ = (blue_val  * 255) / alpha_val;
                        }
                      else
                        {
                          *dest++ = 0;
                          *dest++ = 0;
                          *dest++ = 0;
                        }

                      *dest++ = alpha_val;
                    }
                }
              else
                {
                  *dest++ = red_val;
                  *dest++ = green_val;
                  *dest++ = blue_val;
                }
              break;

            default:
              /* This case was handled earlier */
              g_assert_not_reached();
            }

          for (i = 1; i <= extra; ++i)
            {
              NEXTSAMPLE(alpha_val);
              *channel[i].pixel++ = alpha_val;
            }
        }

      if (align)
        {
          switch (photomet)
            {
            case PHOTOMETRIC_MINISBLACK:
            case PHOTOMETRIC_MINISWHITE:
            case PHOTOMETRIC_PALETTE:
              for (i = 0; i < align * (1 + alpha + extra); ++i)
                NEXTSAMPLE (alpha_val);
              break;
            case PHOTOMETRIC_RGB:
              for (i = 0; i < align * (3 + alpha + extra); ++i)
                NEXTSAMPLE (alpha_val);
              break;
            }
        }

      bitsleft = 0;
    }

  for (i = 0; i <= extra; ++i)
    gimp_pixel_rgn_set_rect (&(channel[i].pixel_rgn), channel[i].pixels,
                             startcol, startrow, cols, rows);
}

static void
read_separate (const guchar *source,
               channel_data *channel,
               gushort       bps,
               gushort       photomet,
               gint          startrow,
               gint          startcol,
               gint          rows,
               gint          cols,
               gboolean      alpha,
               gint          extra,
               gint          sample)
{
  guchar *dest;
  gint    col, row, c;
  gint    bitsleft = 8, maxval = (1 << bps) - 1;

  if (bps > 8)
    {
      g_message ("Unsupported layout");
      gimp_quit ();
    }

  if (sample < channel[0].drawable->bpp)
    {
      c = 0;
    }
  else
    {
      c = (sample - channel[0].drawable->bpp) + 4;
      photomet = PHOTOMETRIC_MINISBLACK;
    }

  gimp_pixel_rgn_init (&(channel[c].pixel_rgn), channel[c].drawable,
                         startcol, startrow, cols, rows, TRUE, FALSE);

  gimp_pixel_rgn_get_rect (&(channel[c].pixel_rgn), channel[c].pixels,
                           startcol, startrow, cols, rows);

  for (row = 0; row < rows; ++row)
    {
      dest = channel[c].pixels + row * cols * channel[c].drawable->bpp;

      if (c == 0)
        {
          for (col = 0; col < cols; ++col)
            NEXTSAMPLE(dest[col * channel[0].drawable->bpp + sample]);
        }
      else
        {
          for (col = 0; col < cols; ++col)
            NEXTSAMPLE(dest[col]);
        }
    }

  gimp_pixel_rgn_set_rect (&(channel[c].pixel_rgn), channel[c].pixels,
                           startcol, startrow, cols, rows);
}

static void
fill_bit2byte(void)
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
