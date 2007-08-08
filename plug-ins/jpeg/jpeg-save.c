/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include <jpeglib.h>
#include <jerror.h>

#ifdef HAVE_EXIF
#include <libexif/exif-data.h>
#endif /* HAVE_EXIF */

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "jpeg.h"
#include "jpeg-icc.h"
#include "jpeg-save.h"


#define SCALE_WIDTH         125

/* See bugs #63610 and #61088 for a discussion about the quality settings */
#define DEFAULT_QUALITY     85.0
#define DEFAULT_SMOOTHING   0.0
#define DEFAULT_OPTIMIZE    TRUE
#define DEFAULT_PROGRESSIVE FALSE
#define DEFAULT_BASELINE    TRUE
#define DEFAULT_SUBSMP      0
#define DEFAULT_RESTART     0
#define DEFAULT_DCT         0
#define DEFAULT_PREVIEW     FALSE
#define DEFAULT_EXIF        TRUE
#define DEFAULT_THUMBNAIL   FALSE
#define DEFAULT_XMP         TRUE

#define JPEG_DEFAULTS_PARASITE  "jpeg-save-defaults"


typedef struct
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  gint          tile_height;
  FILE         *outfile;
  gboolean      has_alpha;
  gint          rowstride;
  guchar       *temp;
  guchar       *data;
  guchar       *src;
  GimpDrawable *drawable;
  GimpPixelRgn  pixel_rgn;
  const gchar  *file_name;
  gboolean      abort_me;
} PreviewPersistent;

/*le added : struct containing pointers to save dialog*/
typedef struct
{
  gboolean       run;
  GtkWidget     *use_restart_markers;   /*checkbox setting use restart markers*/
  GtkTextBuffer *text_buffer;
  GtkObject     *scale_data;            /*for restart markers*/
  gulong        handler_id_restart;

  GtkObject     *quality;               /*quality slidebar*/
  GtkObject     *smoothing;             /*smoothing slidebar*/
  GtkWidget     *optimize;              /*optimize togle*/
  GtkWidget     *progressive;           /*progressive toggle*/
  GtkWidget     *subsmp;                /*subsampling side select*/
  GtkWidget     *baseline;              /*baseling toggle*/
  GtkWidget     *restart;               /*spinner for setting frequency restart markers*/
  GtkWidget     *dct;                   /*DCT side select*/
  GtkWidget     *preview;               /*show preview toggle checkbox*/
  GtkWidget     *save_exif;
  GtkWidget     *save_thumbnail;
  GtkWidget     *save_xmp;
} JpegSaveGui;

static void  make_preview        (void);

static void  save_restart_update (GtkAdjustment *adjustment,
                                  GtkWidget     *toggle);
static void  subsampling_changed (GtkWidget     *combo,
                                  GtkObject     *entry);

#ifdef HAVE_EXIF

static gint  create_thumbnail    (gint32         image_ID,
                                  gint32         drawable_ID,
                                  gdouble        quality,
                                  guchar       **thumbnail_buffer);

#endif /* HAVE_EXIF */


static GtkWidget *restart_markers_scale = NULL;
static GtkWidget *restart_markers_label = NULL;
static GtkWidget *preview_size          = NULL;
static gboolean  *abort_me              = NULL;

static void   save_dialog_response (GtkWidget   *widget,
                                    gint         response_id,
                                    gpointer     data);

static void   load_gui_defaults    (JpegSaveGui *pg);
static void   save_defaults        (void);


/*
 * sg - This is the best I can do, I'm afraid... I think it will fail
 * if something bad really happens (but it might not). If you have a
 * better solution, send it ;-)
 */
static void
background_error_exit (j_common_ptr cinfo)
{
  if (abort_me)
    *abort_me = TRUE;
  (*cinfo->err->output_message) (cinfo);
}

static gboolean
background_jpeg_save (PreviewPersistent *pp)
{
  guchar *t;
  guchar *s;
  gint    i, j;
  gint    yend;

  if (pp->abort_me || (pp->cinfo.next_scanline >= pp->cinfo.image_height))
    {
      /* clean up... */
      if (pp->abort_me)
        {
          jpeg_abort_compress (&(pp->cinfo));
        }
      else
        {
          jpeg_finish_compress (&(pp->cinfo));
        }

      fclose (pp->outfile);
      jpeg_destroy_compress (&(pp->cinfo));

      g_free (pp->temp);
      g_free (pp->data);

      if (pp->drawable)
        gimp_drawable_detach (pp->drawable);

      /* display the preview stuff */
      if (!pp->abort_me)
        {
          struct stat buf;
          gchar       temp[128];

          g_stat (pp->file_name, &buf);
          g_snprintf (temp, sizeof (temp),
                      _("File size: %02.01f kB"),
                      (gdouble) (buf.st_size) / 1024.0);
          gtk_label_set_text (GTK_LABEL (preview_size), temp);

          /* and load the preview */
          load_image (pp->file_name, GIMP_RUN_NONINTERACTIVE, TRUE);
        }

      /* we cleanup here (load_image doesn't run in the background) */
      g_unlink (pp->file_name);

      if (abort_me == &(pp->abort_me))
        abort_me = NULL;

      g_free (pp);

      gimp_displays_flush ();
      gdk_flush ();

      return FALSE;
    }
  else
    {
      if ((pp->cinfo.next_scanline % pp->tile_height) == 0)
        {
          yend = pp->cinfo.next_scanline + pp->tile_height;
          yend = MIN (yend, pp->cinfo.image_height);
          gimp_pixel_rgn_get_rect (&pp->pixel_rgn, pp->data, 0,
                                   pp->cinfo.next_scanline,
                                   pp->cinfo.image_width,
                                   (yend - pp->cinfo.next_scanline));
          pp->src = pp->data;
        }

      t = pp->temp;
      s = pp->src;
      i = pp->cinfo.image_width;

      while (i--)
        {
          for (j = 0; j < pp->cinfo.input_components; j++)
            *t++ = *s++;
          if (pp->has_alpha)  /* ignore alpha channel */
            s++;
        }

      pp->src += pp->rowstride;
      jpeg_write_scanlines (&(pp->cinfo), (JSAMPARRAY) &(pp->temp), 1);

      return TRUE;
    }
}

gboolean
save_image (const gchar *filename,
            gint32       image_ID,
            gint32       drawable_ID,
            gint32       orig_image_ID,
            gboolean     preview)
{
  GimpPixelRgn   pixel_rgn;
  GimpDrawable  *drawable;
  GimpImageType  drawable_type;
  GimpParasite  *parasite;
  struct jpeg_compress_struct cinfo;
  struct my_error_mgr         jerr;
  FILE     * volatile outfile;
  guchar   *temp, *t;
  guchar   *data;
  guchar   *src, *s;
  gboolean  has_alpha;
  gint      rowstride, yend;
  gint      i, j;
#ifdef HAVE_EXIF
  guchar   *thumbnail_buffer        = NULL;
  gint      thumbnail_buffer_length = 0;
  ExifData *exif_data_tmp           = NULL;
#endif

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable,
                       0, 0, drawable->width, drawable->height, FALSE, FALSE);

  if (!preview)
    gimp_progress_init_printf (_("Saving '%s'"),
                               gimp_filename_to_utf8 (filename));

  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  outfile = NULL;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
    {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      jpeg_destroy_compress (&cinfo);
      if (outfile)
        fclose (outfile);
      if (drawable)
        gimp_drawable_detach (drawable);

      return FALSE;
    }

  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress (&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Here we use the library-supplied code to send compressed data to a
   * stdio stream.  You can also write your own code to do something else.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */
  if ((outfile = g_fopen (filename, "wb")) == NULL)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  jpeg_stdio_dest (&cinfo, outfile);

  /* Get the input image and a pointer to its data.
   */
  switch (drawable_type)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_GRAY_IMAGE:
      /* # of color components per pixel */
      cinfo.input_components = drawable->bpp;
      has_alpha = FALSE;
      break;

    case GIMP_RGBA_IMAGE:
    case GIMP_GRAYA_IMAGE:
      /* # of color components per pixel (minus the GIMP alpha channel) */
      cinfo.input_components = drawable->bpp - 1;
      has_alpha = TRUE;
      break;

    case GIMP_INDEXED_IMAGE:
      return FALSE;

    default:
      return FALSE;
    }

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  /* image width and height, in pixels */
  cinfo.image_width  = drawable->width;
  cinfo.image_height = drawable->height;
  /* colorspace of input image */
  cinfo.in_color_space = (drawable_type == GIMP_RGB_IMAGE ||
                          drawable_type == GIMP_RGBA_IMAGE)
    ? JCS_RGB : JCS_GRAYSCALE;
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults (&cinfo);

  jpeg_set_quality (&cinfo, (gint) (jsvals.quality + 0.5), jsvals.baseline);
  cinfo.optimize_coding = jsvals.optimize;

  /*  smoothing is not supported with nonstandard sampling ratios  */
  if (jsvals.subsmp != 1)
    cinfo.smoothing_factor = (gint) (jsvals.smoothing * 100);

  if (jsvals.progressive)
    {
      jpeg_simple_progression (&cinfo);
    }

  switch (jsvals.subsmp)
    {
    case 0:
    default:
      cinfo.comp_info[0].h_samp_factor = 2;
      cinfo.comp_info[0].v_samp_factor = 2;
      cinfo.comp_info[1].h_samp_factor = 1;
      cinfo.comp_info[1].v_samp_factor = 1;
      cinfo.comp_info[2].h_samp_factor = 1;
      cinfo.comp_info[2].v_samp_factor = 1;
      break;

    case 1:
      cinfo.comp_info[0].h_samp_factor = 2;
      cinfo.comp_info[0].v_samp_factor = 1;
      cinfo.comp_info[1].h_samp_factor = 1;
      cinfo.comp_info[1].v_samp_factor = 1;
      cinfo.comp_info[2].h_samp_factor = 1;
      cinfo.comp_info[2].v_samp_factor = 1;
      break;

    case 2:
      cinfo.comp_info[0].h_samp_factor = 1;
      cinfo.comp_info[0].v_samp_factor = 1;
      cinfo.comp_info[1].h_samp_factor = 1;
      cinfo.comp_info[1].v_samp_factor = 1;
      cinfo.comp_info[2].h_samp_factor = 1;
      cinfo.comp_info[2].v_samp_factor = 1;
      break;

    case 3:
      cinfo.comp_info[0].h_samp_factor = 1;
      cinfo.comp_info[0].v_samp_factor = 2;
      cinfo.comp_info[1].h_samp_factor = 1;
      cinfo.comp_info[1].v_samp_factor = 1;
      cinfo.comp_info[2].h_samp_factor = 1;
      cinfo.comp_info[2].v_samp_factor = 1;
      break;
    }

  cinfo.restart_interval = 0;
  cinfo.restart_in_rows = jsvals.restart;

  switch (jsvals.dct)
    {
    case 0:
    default:
      cinfo.dct_method = JDCT_ISLOW;
      break;

    case 1:
      cinfo.dct_method = JDCT_IFAST;
      break;

    case 2:
      cinfo.dct_method = JDCT_FLOAT;
      break;
    }

  {
    gdouble xresolution;
    gdouble yresolution;

    gimp_image_get_resolution (orig_image_ID, &xresolution, &yresolution);

    if (xresolution > 1e-5 && yresolution > 1e-5)
      {
        gdouble factor;

        factor = gimp_unit_get_factor (gimp_image_get_unit (orig_image_ID));

        if (factor == 2.54 /* cm */ ||
            factor == 25.4 /* mm */)
          {
            cinfo.density_unit = 2;  /* dots per cm */

            xresolution /= 2.54;
            yresolution /= 2.54;
          }
        else
          {
            cinfo.density_unit = 1;  /* dots per inch */
          }

        cinfo.X_density = xresolution;
        cinfo.Y_density = yresolution;
      }
  }

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress (&cinfo, TRUE);

#ifdef HAVE_EXIF

  /* Create the thumbnail JPEG in a buffer */
  if (jsvals.save_exif || jsvals.save_thumbnail)
    {
      guchar *exif_buf = NULL;
      guint   exif_buf_len;
      gdouble quality  = MIN (75.0, jsvals.quality);

      if ( (! jsvals.save_exif) || (! exif_data))
        exif_data_tmp = exif_data_new ();
      else
        exif_data_tmp = exif_data;

      /* avoid saving markers longer than 65533, gradually decrease
       * quality in steps of 5 until exif_buf_len is lower than that.
       */
      for (exif_buf_len = 65535;
           exif_buf_len > 65533 && quality > 0.0;
           quality -= 5.0)
        {
          if (jsvals.save_thumbnail)
            thumbnail_buffer_length = create_thumbnail (image_ID, drawable_ID,
                                                        quality,
                                                        &thumbnail_buffer);

          exif_data_tmp->data = thumbnail_buffer;
          exif_data_tmp->size = thumbnail_buffer_length;

          if (exif_buf)
            free (exif_buf);

          exif_data_save_data (exif_data_tmp, &exif_buf, &exif_buf_len);
        }

      if (exif_buf_len > 65533)
        {
          /* last attempt with quality 0.0 */
          if (jsvals.save_thumbnail)
            thumbnail_buffer_length = create_thumbnail (image_ID, drawable_ID,
                                                        0.0,
                                                        &thumbnail_buffer);
          exif_data_tmp->data = thumbnail_buffer;
          exif_data_tmp->size = thumbnail_buffer_length;

          if (exif_buf)
            free (exif_buf);

          exif_data_save_data (exif_data_tmp, &exif_buf, &exif_buf_len);
        }

      if (exif_buf_len > 65533)
        {
          /* still no go? save without thumbnail */
          exif_data_tmp->data = NULL;
          exif_data_tmp->size = 0;

          if (exif_buf)
            free (exif_buf);

          exif_data_save_data (exif_data_tmp, &exif_buf, &exif_buf_len);
        }

      g_print ("jpeg-save: saving EXIF block (%d bytes)\n", exif_buf_len);
      jpeg_write_marker (&cinfo, JPEG_APP0 + 1, exif_buf, exif_buf_len);

      if (exif_buf)
        free (exif_buf);
    }
#endif /* HAVE_EXIF */

  /* Step 4.1: Write the comment out - pw */
  if (image_comment && *image_comment)
    {
      g_print ("jpeg-save: saving image comment (%d bytes)\n",
               (int) strlen (image_comment));
      jpeg_write_marker (&cinfo, JPEG_COM,
                         (guchar *) image_comment, strlen (image_comment));
    }

  /* Step 4.2: Write the XMP packet in an APP1 marker */
  if (jsvals.save_xmp)
    {
      /* FIXME: temporary hack until the right thing is done by a library */
      parasite = gimp_image_parasite_find (orig_image_ID, "gimp-metadata");
      if (parasite)
        {
          const gchar *xmp_data;
          glong        xmp_data_size;
          guchar      *app_block;

          xmp_data = ((const gchar *) gimp_parasite_data (parasite)) + 10;
          xmp_data_size = gimp_parasite_data_size (parasite) - 10;
          g_print ("jpeg-save: saving XMP packet (%d bytes)\n",
                   (int) xmp_data_size);
          app_block = g_malloc (sizeof (JPEG_APP_HEADER_XMP) + xmp_data_size);
          memcpy (app_block, JPEG_APP_HEADER_XMP,
                  sizeof (JPEG_APP_HEADER_XMP));
          memcpy (app_block + sizeof (JPEG_APP_HEADER_XMP), xmp_data,
                  xmp_data_size);
          jpeg_write_marker (&cinfo, JPEG_APP0 + 1, app_block,
                             sizeof (JPEG_APP_HEADER_XMP) + xmp_data_size);
          g_free (app_block);
          gimp_parasite_free (parasite);
        }
    }

  /* Step 4.3: store the color profile if there is one */
  parasite = gimp_image_parasite_find (orig_image_ID, "icc-profile");
  if (parasite)
    {
      jpeg_icc_write_profile (&cinfo,
                              gimp_parasite_data (parasite),
                              gimp_parasite_data_size (parasite));
      gimp_parasite_free (parasite);
    }

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  /* JSAMPLEs per row in image_buffer */
  rowstride = drawable->bpp * drawable->width;
  temp = g_new (guchar, cinfo.image_width * cinfo.input_components);
  data = g_new (guchar, rowstride * gimp_tile_height ());

  /* fault if cinfo.next_scanline isn't initially a multiple of
   * gimp_tile_height */
  src = NULL;

  /*
   * sg - if we preview, we want this to happen in the background -- do
   * not duplicate code in the future; for now, it's OK
   */

  if (preview)
    {
      PreviewPersistent *pp = g_new (PreviewPersistent, 1);

      /* pass all the information we need */
      pp->cinfo       = cinfo;
      pp->tile_height = gimp_tile_height();
      pp->data        = data;
      pp->outfile     = outfile;
      pp->has_alpha   = has_alpha;
      pp->rowstride   = rowstride;
      pp->temp        = temp;
      pp->data        = data;
      pp->drawable    = drawable;
      pp->pixel_rgn   = pixel_rgn;
      pp->src         = NULL;
      pp->file_name   = filename;

      pp->abort_me    = FALSE;
      abort_me = &(pp->abort_me);

      pp->cinfo.err = jpeg_std_error(&(pp->jerr));
      pp->jerr.error_exit = background_error_exit;

      gtk_label_set_text (GTK_LABEL (preview_size),
                          _("Calculating file size..."));

      g_idle_add ((GSourceFunc) background_jpeg_save, pp);

      /* background_jpeg_save() will cleanup as needed */
      return TRUE;
    }

  while (cinfo.next_scanline < cinfo.image_height)
    {
      if ((cinfo.next_scanline % gimp_tile_height ()) == 0)
        {
          yend = cinfo.next_scanline + gimp_tile_height ();
          yend = MIN (yend, cinfo.image_height);
          gimp_pixel_rgn_get_rect (&pixel_rgn, data,
                                   0, cinfo.next_scanline,
                                   cinfo.image_width,
                                   (yend - cinfo.next_scanline));
          src = data;
        }

      t = temp;
      s = src;
      i = cinfo.image_width;

      while (i--)
        {
          for (j = 0; j < cinfo.input_components; j++)
            *t++ = *s++;
          if (has_alpha)  /* ignore alpha channel */
            s++;
        }

      src += rowstride;
      jpeg_write_scanlines (&cinfo, (JSAMPARRAY) &temp, 1);

      if ((cinfo.next_scanline % 32) == 0)
        gimp_progress_update ((gdouble) cinfo.next_scanline /
                              (gdouble) cinfo.image_height);
    }

  /* Step 6: Finish compression */
  jpeg_finish_compress (&cinfo);
  /* After finish_compress, we can close the output file. */
  fclose (outfile);

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress (&cinfo);
  /* free the temporary buffer */
  g_free (temp);
  g_free (data);

  /* And we're done! */
  /*gimp_do_progress (1, 1);*/

  gimp_drawable_detach (drawable);

  return TRUE;
}

static void
make_preview (void)
{
  destroy_preview ();

  if (jsvals.preview)
    {
      gchar *tn = gimp_temp_name ("jpeg");

      if (! undo_touched)
        {
          /* we freeze undo saving so that we can avoid sucking up
           * tile cache with our unneeded preview steps. */
          gimp_image_undo_freeze (preview_image_ID);

          undo_touched = TRUE;
        }

      save_image (tn,
                  preview_image_ID,
                  drawable_ID_global,
                  orig_image_ID_global,
                  TRUE);

      if (display_ID == -1)
        display_ID = gimp_display_new (preview_image_ID);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (preview_size), _("File size: unknown"));

      gimp_displays_flush ();
    }
}

void
destroy_preview (void)
{
  if (abort_me)
    *abort_me = TRUE;   /* signal the background save to stop */

  if (drawable_global)
    {
      gimp_drawable_detach (drawable_global);
      drawable_global = NULL;
    }

  g_printerr ("destroy_preview (%d, %d)\n", preview_image_ID, preview_layer_ID);

  if (gimp_image_is_valid (preview_image_ID) &&
      gimp_drawable_is_valid (preview_layer_ID))
    {
      /*  assuming that reference counting is working correctly,
          we do not need to delete the layer, removing it from
          the image should be sufficient  */
      gimp_image_remove_layer (preview_image_ID, preview_layer_ID);

      preview_layer_ID = -1;
    }
}

gboolean
save_dialog (void)
{
  JpegSaveGui    pg;
  GtkWidget     *dialog;
  GtkWidget     *vbox;
  GtkObject     *entry;
  GtkWidget     *table;
  GtkWidget     *table2;
  GtkWidget     *tabledefaults;
  GtkWidget     *expander;
  GtkWidget     *frame;
  GtkWidget     *toggle;
  GtkWidget     *spinbutton;
  GtkWidget     *ebox;
  GtkWidget     *label;
  GtkWidget     *combo;
  GtkWidget     *text_view;
  GtkTextBuffer *text_buffer;
  GtkWidget     *scrolled_window;
  GtkWidget     *button;

  GimpImageType  dtype;
  gchar         *text;


  dialog = gimp_dialog_new (_("Save as JPEG"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, SAVE_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (save_dialog_response),
                    &pg);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  pg.quality = entry = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                             _("_Quality:"),
                                             SCALE_WIDTH, 0, jsvals.quality,
                                             0.0, 100.0, 1.0, 10.0, 0,
                                             TRUE, 0.0, 0.0,
                                             _("JPEG quality parameter"),
                                             "file-jpeg-save-quality");

  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &jsvals.quality);
  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (make_preview),
                    NULL);

  ebox = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), ebox, FALSE, FALSE, 0);
  gtk_widget_show (ebox);

  gimp_help_set_help_data (ebox,
                           _("Enable preview to obtain the file size."), NULL);

  preview_size = gtk_label_new (_("File size: unknown"));
  gtk_misc_set_alignment (GTK_MISC (preview_size), 0.0, 0.5);
  gimp_label_set_attributes (GTK_LABEL (preview_size),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_container_add (GTK_CONTAINER (ebox), preview_size);
  gtk_widget_show (preview_size);

  pg.preview = toggle =
    gtk_check_button_new_with_mnemonic (_("Show _preview in image window"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), jsvals.preview);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.preview);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (make_preview),
                    NULL);

  text = g_strdup_printf ("<b>%s</b>", _("_Advanced Options"));
  expander = gtk_expander_new_with_mnemonic (text);
  gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
  g_free (text);

  gtk_box_pack_start (GTK_BOX (vbox), expander, TRUE, TRUE, 0);
  gtk_widget_show (expander);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (expander), vbox);
  gtk_widget_show (vbox);

  frame = gimp_frame_new ("<expander>");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 7, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 12);
  gtk_container_add (GTK_CONTAINER (frame), table);

  table2 = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 6);
  gtk_table_attach (GTK_TABLE (table), table2,
                    2, 6, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (table2);

  pg.smoothing = entry = gimp_scale_entry_new (GTK_TABLE (table2), 0, 0,
                                               _("_Smoothing:"),
                                               100, 0, jsvals.smoothing,
                                               0.0, 1.0, 0.01, 0.1, 2,
                                               TRUE, 0.0, 0.0,
                                               NULL,
                                               "file-jpeg-save-smoothing");
  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &jsvals.smoothing);
  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (make_preview),
                    NULL);

  restart_markers_label = gtk_label_new (_("Frequency (rows):"));
  gtk_misc_set_alignment (GTK_MISC (restart_markers_label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), restart_markers_label, 4, 5, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (restart_markers_label);

  /*pg.scale_data = scale_data;*/
  pg.restart = restart_markers_scale = spinbutton =
    gimp_spin_button_new (&pg.scale_data,
                          (jsvals.restart == 0) ? 1 : jsvals.restart,
                          1.0, 64.0, 1.0, 1.0, 64.0, 1.0, 0);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 5, 6, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  pg.use_restart_markers = toggle =
    gtk_check_button_new_with_label (_("Use restart markers"));
  gtk_table_attach (GTK_TABLE (table), toggle, 2, 4, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), jsvals.restart);

  gtk_widget_set_sensitive (restart_markers_label, jsvals.restart);
  gtk_widget_set_sensitive (restart_markers_scale, jsvals.restart);

  g_signal_connect (pg.scale_data, "value-changed",
                    G_CALLBACK (save_restart_update),
                    toggle);
  pg.handler_id_restart = g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (save_restart_update),
                            pg.scale_data);

  pg.optimize = toggle = gtk_check_button_new_with_label (_("Optimize"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.optimize);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (make_preview),
                    NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), jsvals.optimize);

  pg.progressive = toggle = gtk_check_button_new_with_label (_("Progressive"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.progressive);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (make_preview),
                    NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                jsvals.progressive);

  pg.baseline = toggle =
    gtk_check_button_new_with_label (_("Force baseline JPEG"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 2, 3, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.baseline);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (make_preview),
                    NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                jsvals.baseline);

#ifdef HAVE_EXIF
  pg.save_exif = toggle = gtk_check_button_new_with_label (_("Save EXIF data"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 3, 4, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.save_exif);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (make_preview),
                    NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                jsvals.save_exif && exif_data);

  gtk_widget_set_sensitive (toggle, exif_data != NULL);

  pg.save_thumbnail = toggle =
    gtk_check_button_new_with_label (_("Save thumbnail"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 4, 5, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.save_thumbnail);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (make_preview),
                    NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                jsvals.save_thumbnail);
#endif /* HAVE_EXIF */

  /* XMP metadata */
  pg.save_xmp = toggle = gtk_check_button_new_with_label (_("Save XMP data"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 5, 6, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.save_xmp);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                jsvals.save_xmp && has_metadata);

  gtk_widget_set_sensitive (toggle, has_metadata);

  /* Subsampling */
  label = gtk_label_new (_("Subsampling:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  pg.subsmp =
    combo = gimp_int_combo_box_new (_("1x1,1x1,1x1 (best quality)"),  2,
                                    _("2x1,1x1,1x1 (4:2:2)"),         1,
                                    _("1x2,1x1,1x1"),                 3,
                                    _("2x2,1x1,1x1 (smallest file)"), 0,
                                    NULL);
  gtk_table_attach (GTK_TABLE (table), combo, 3, 6, 2, 3,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), jsvals.subsmp,
                              G_CALLBACK (subsampling_changed),
                              entry);

  dtype = gimp_drawable_type (drawable_ID_global);
  if (dtype != GIMP_RGB_IMAGE && dtype != GIMP_RGBA_IMAGE)
    gtk_widget_set_sensitive (combo, FALSE);

  /* DCT method */
  label = gtk_label_new (_("DCT method:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  pg.dct = combo = gimp_int_combo_box_new (_("Fast Integer"),   1,
                                           _("Integer"),        0,
                                           _("Floating-Point"), 2,
                                           NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), jsvals.dct);
  gtk_table_attach (GTK_TABLE (table), combo, 3, 6, 3, 4,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &jsvals.dct);
  g_signal_connect (combo, "changed",
                    G_CALLBACK (make_preview),
                    NULL);

  frame = gimp_frame_new (_("Comment"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (scrolled_window, 250, 50);
  gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
  gtk_widget_show (scrolled_window);

  pg.text_buffer = text_buffer = gtk_text_buffer_new (NULL);
  if (image_comment)
    gtk_text_buffer_set_text (text_buffer, image_comment, -1);

  text_view = gtk_text_view_new_with_buffer (text_buffer);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);

  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  g_object_unref (text_buffer);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  tabledefaults = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (tabledefaults), 6);
  gtk_box_pack_start (GTK_BOX (vbox), tabledefaults, FALSE, FALSE, 0);
  gtk_widget_show (tabledefaults);

  button = gtk_button_new_with_mnemonic (_("_Load Defaults"));
  gtk_table_attach (GTK_TABLE (tabledefaults),
                    button, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (load_gui_defaults),
                            &pg);

  button = gtk_button_new_with_mnemonic (_("S_ave Defaults"));
  gtk_table_attach (GTK_TABLE (tabledefaults),
                    button, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (save_defaults),
                            &pg);
  gtk_widget_show (frame);

  gtk_widget_show (table);
  gtk_widget_show (dialog);

  make_preview ();

  pg.run = FALSE;

  gtk_main ();

  destroy_preview ();

  return pg.run;
}

static void
save_dialog_response (GtkWidget *widget,
                      gint       response_id,
                      gpointer   data)
{
  JpegSaveGui *pg = data;
  GtkTextIter  start_iter;
  GtkTextIter  end_iter;

  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      gtk_text_buffer_get_bounds (pg->text_buffer, &start_iter, &end_iter);
      image_comment = gtk_text_buffer_get_text (pg->text_buffer,
                                                &start_iter, &end_iter, FALSE);
      pg->run = TRUE;
      /* fallthrough */

    default:
      gtk_widget_destroy (widget);
      break;
    }
}

void
load_save_defaults (void)
{
  GimpParasite *parasite;
  gchar        *def_str;
  JpegSaveVals  tmpvals;
  gint          num_fields;

  jsvals.quality        = DEFAULT_QUALITY;
  jsvals.smoothing      = DEFAULT_SMOOTHING;
  jsvals.optimize       = DEFAULT_OPTIMIZE;
  jsvals.progressive    = DEFAULT_PROGRESSIVE;
  jsvals.baseline       = DEFAULT_BASELINE;
  jsvals.subsmp         = DEFAULT_SUBSMP;
  jsvals.restart        = DEFAULT_RESTART;
  jsvals.dct            = DEFAULT_DCT;
  jsvals.preview        = DEFAULT_PREVIEW;
  jsvals.save_exif      = DEFAULT_EXIF;
  jsvals.save_thumbnail = DEFAULT_THUMBNAIL;
  jsvals.save_xmp       = DEFAULT_XMP;

#ifdef HAVE_EXIF
  if (exif_data && (exif_data->data))
    jsvals.save_thumbnail = TRUE;
#endif /* HAVE_EXIF */

  parasite = gimp_parasite_find (JPEG_DEFAULTS_PARASITE);

  if (! parasite)
    return;

  def_str = g_strndup (gimp_parasite_data (parasite),
                       gimp_parasite_data_size (parasite));

  gimp_parasite_free (parasite);

  num_fields = sscanf (def_str, "%lf %lf %d %d %d %d %d %d %d %d %d %d",
                       &tmpvals.quality,
                       &tmpvals.smoothing,
                       &tmpvals.optimize,
                       &tmpvals.progressive,
                       &tmpvals.subsmp,
                       &tmpvals.baseline,
                       &tmpvals.restart,
                       &tmpvals.dct,
                       &tmpvals.preview,
                       &tmpvals.save_exif,
                       &tmpvals.save_thumbnail,
                       &tmpvals.save_xmp);

  if (num_fields == 12)
    memcpy (&jsvals, &tmpvals, sizeof (tmpvals));
}

static void
save_defaults (void)
{
  GimpParasite *parasite;
  gchar        *def_str;

  def_str = g_strdup_printf ("%lf %lf %d %d %d %d %d %d %d %d %d %d",
                             jsvals.quality,
                             jsvals.smoothing,
                             jsvals.optimize,
                             jsvals.progressive,
                             jsvals.subsmp,
                             jsvals.baseline,
                             jsvals.restart,
                             jsvals.dct,
                             jsvals.preview,
                             jsvals.save_exif,
                             jsvals.save_thumbnail,
                             jsvals.save_xmp);
  parasite = gimp_parasite_new (JPEG_DEFAULTS_PARASITE,
                                GIMP_PARASITE_PERSISTENT,
                                strlen (def_str), def_str);

  gimp_parasite_attach (parasite);

  gimp_parasite_free (parasite);
  g_free (def_str);
}

static void
load_gui_defaults (JpegSaveGui *pg)
{
  GtkAdjustment *restart_markers;

  load_save_defaults ();

#define SET_ACTIVE_BTTN(field) \
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pg->field), jsvals.field)

  SET_ACTIVE_BTTN (optimize);
  SET_ACTIVE_BTTN (progressive);
  SET_ACTIVE_BTTN (baseline);
  SET_ACTIVE_BTTN(preview);
#ifdef HAVE_EXIF
  SET_ACTIVE_BTTN(save_exif);
  SET_ACTIVE_BTTN(save_thumbnail);
#endif
  SET_ACTIVE_BTTN(save_xmp);

#undef SET_ACTIVE_BTTN

/*spin button stuff*/
  g_signal_handler_block (pg->use_restart_markers, pg->handler_id_restart);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pg->use_restart_markers),
                                jsvals.restart);
  restart_markers = GTK_ADJUSTMENT (pg->scale_data);
  gtk_adjustment_set_value (restart_markers, jsvals.restart);
  g_signal_handler_unblock (pg->use_restart_markers, pg->handler_id_restart);


  gtk_adjustment_set_value (GTK_ADJUSTMENT (pg->quality),
                            jsvals.quality);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (pg->smoothing),
                            jsvals.smoothing);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (pg->subsmp),
                                 jsvals.subsmp);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (pg->dct),
                                 jsvals.dct);

}

static void
save_restart_update (GtkAdjustment *adjustment,
                     GtkWidget     *toggle)
{
  jsvals.restart = GTK_TOGGLE_BUTTON (toggle)->active ? adjustment->value : 0;

  gtk_widget_set_sensitive (restart_markers_label, jsvals.restart);
  gtk_widget_set_sensitive (restart_markers_scale, jsvals.restart);

  make_preview ();
}

static void
subsampling_changed (GtkWidget *combo,
                     GtkObject *entry)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &jsvals.subsmp);

  /*  smoothing is not supported with nonstandard sampling ratios  */
  gimp_scale_entry_set_sensitive (entry, jsvals.subsmp != 1);

  make_preview ();
}

#ifdef HAVE_EXIF

static guchar *tbuffer = NULL;
static guchar *tbuffer2 = NULL;

static gint tbuffer_count = 0;

typedef struct
{
  struct jpeg_destination_mgr  pub;   /* public fields */
  guchar                      *buffer;
  gint                         size;
} my_destination_mgr;

typedef my_destination_mgr *my_dest_ptr;

static void
init_destination (j_compress_ptr cinfo)
{
}

static gboolean
empty_output_buffer (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

  tbuffer_count = tbuffer_count + 16384;
  tbuffer = g_renew (guchar, tbuffer, tbuffer_count);
  g_memmove (tbuffer + tbuffer_count - 16384, tbuffer2, 16384);

  dest->pub.next_output_byte = tbuffer2;
  dest->pub.free_in_buffer   = 16384;

  return TRUE;
}

static void
term_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

  tbuffer_count = (tbuffer_count + 16384) - (dest->pub.free_in_buffer);

  tbuffer = g_renew (guchar, tbuffer, tbuffer_count);
  g_memmove (tbuffer + tbuffer_count - (16384 - dest->pub.free_in_buffer),
             tbuffer2, 16384 - dest->pub.free_in_buffer);
}

static gint
create_thumbnail (gint32    image_ID,
                  gint32    drawable_ID,
                  gdouble   quality,
                  guchar  **thumbnail_buffer)
{
  GimpDrawable  *drawable;
  gint           req_width, req_height, bpp, rbpp;
  guchar        *thumbnail_data = NULL;
  struct jpeg_compress_struct cinfo;
  struct my_error_mgr         jerr;
  my_dest_ptr dest;
  gboolean  alpha = FALSE;
  JSAMPROW  scanline[1];
  guchar   *buf = NULL;
  gint      i;

  drawable = gimp_drawable_get (drawable_ID);

  req_width  = 196;
  req_height = 196;

  if (MIN (drawable->width, drawable->height) < 196)
    req_width = req_height = MIN(drawable->width, drawable->height);

  thumbnail_data = gimp_drawable_get_thumbnail_data (drawable_ID,
                                                     &req_width, &req_height,
                                                     &bpp);

  if (! thumbnail_data)
    return 0;

  rbpp = bpp;

  if ((bpp == 2) || (bpp == 4))
    {
      alpha = TRUE;
      rbpp = bpp - 1;
    }

  buf = g_new (guchar, req_width * bpp);
  tbuffer2 = g_new (guchar, 16384);

  tbuffer_count = 0;

  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
    {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, free memory, and return.
       */
      jpeg_destroy_compress (&cinfo);

      if (thumbnail_data)
        {
          g_free (thumbnail_data);
          thumbnail_data = NULL;
        }

      if (buf)
        {
          g_free (buf);
          buf = NULL;
        }

      if (tbuffer2)
        {
          g_free (tbuffer2);
          tbuffer2 = NULL;
        }

      if (drawable)
        gimp_drawable_detach (drawable);

      return 0;
    }

  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress (&cinfo);

  if (cinfo.dest == NULL)
    cinfo.dest = (struct jpeg_destination_mgr *)
      (*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
                                 sizeof(my_destination_mgr));

  dest = (my_dest_ptr) cinfo.dest;
  dest->pub.init_destination    = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination    = term_destination;

  dest->pub.next_output_byte = tbuffer2;
  dest->pub.free_in_buffer   = 16384;

  dest->buffer = tbuffer2;
  dest->size   = 16384;

  cinfo.input_components = rbpp;
  cinfo.image_width      = req_width;
  cinfo.image_height     = req_height;

  /* colorspace of input image */
  cinfo.in_color_space = (rbpp == 3) ? JCS_RGB : JCS_GRAYSCALE;

  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults (&cinfo);

  jpeg_set_quality (&cinfo, (gint) (quality + 0.5), jsvals.baseline);

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress (&cinfo, TRUE);

  while (cinfo.next_scanline < (unsigned int) req_height)
    {
      for (i = 0; i < req_width; i++)
        {
          buf[(i * rbpp) + 0] = thumbnail_data[(cinfo.next_scanline * req_width * bpp) + (i * bpp) + 0];

          if (rbpp == 3)
            {
              buf[(i * rbpp) + 1] = thumbnail_data[(cinfo.next_scanline * req_width * bpp) + (i * bpp) + 1];
              buf[(i * rbpp) + 2] = thumbnail_data[(cinfo.next_scanline * req_width * bpp) + (i * bpp) + 2];
            }
        }

      scanline[0] = buf;
      jpeg_write_scanlines (&cinfo, scanline, 1);
  }

  /* Step 6: Finish compression */
  jpeg_finish_compress (&cinfo);

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress (&cinfo);

  /* And we're done! */

  if (thumbnail_data)
    {
      g_free (thumbnail_data);
      thumbnail_data = NULL;
    }

  if (buf)
    {
      g_free (buf);
      buf = NULL;
    }

  if (drawable)
    {
      gimp_drawable_detach (drawable);
      drawable = NULL;
    }

  *thumbnail_buffer = tbuffer;

  return tbuffer_count;
}

#endif /* HAVE_EXIF */
