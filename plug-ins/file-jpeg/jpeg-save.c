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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "jpeg.h"
#include "jpeg-icc.h"
#include "jpeg-load.h"
#include "jpeg-save.h"
#include "jpeg-settings.h"

#ifdef C_ARITH_CODING_SUPPORTED
static gboolean arithc_supported = TRUE;
#else
static gboolean arithc_supported = FALSE;
#endif

#define SCALE_WIDTH         125

/* See bugs #63610 and #61088 for a discussion about the quality settings */
#define DEFAULT_QUALITY          90.0
#define DEFAULT_SMOOTHING        0.0
#define DEFAULT_OPTIMIZE         TRUE
#define DEFAULT_ARITHMETIC_CODING FALSE
#define DEFAULT_PROGRESSIVE      TRUE
#define DEFAULT_BASELINE         TRUE
#define DEFAULT_SUBSMP           JPEG_SUBSAMPLING_1x1_1x1_1x1
#define DEFAULT_RESTART          0
#define DEFAULT_RESTART_MCU_ROWS 16
#define DEFAULT_DCT              0
#define DEFAULT_PREVIEW          FALSE
#define DEFAULT_EXIF             TRUE
#define DEFAULT_THUMBNAIL        FALSE
#define DEFAULT_XMP              TRUE
#define DEFAULT_IPTC             TRUE
#define DEFAULT_USE_ORIG_QUALITY FALSE

#define JPEG_DEFAULTS_PARASITE  "jpeg-save-defaults"


typedef struct
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  gint          tile_height;
  FILE         *outfile;
  gboolean      has_alpha;
  gint          rowstride;
  guchar       *data;
  guchar       *src;
  GeglBuffer   *buffer;
  const Babl   *format;
  const gchar  *file_name;
  gboolean      abort_me;
  guint         source_id;
} PreviewPersistent;

/*le added : struct containing pointers to export dialog*/
typedef struct
{
  gboolean       run;
  GtkWidget     *use_restart_markers;   /*checkbox setting use restart markers*/
  GtkTextBuffer *text_buffer;
  GtkAdjustment *scale_data;            /*for restart markers*/
  gulong         handler_id_restart;

  GtkAdjustment *quality;               /*quality slidebar*/
  GtkAdjustment *smoothing;             /*smoothing slidebar*/
  GtkWidget     *optimize;              /*optimize toggle*/
  GtkWidget     *arithmetic_coding;     /*arithmetic coding toggle*/
  GtkWidget     *progressive;           /*progressive toggle*/
  GtkWidget     *subsmp;                /*subsampling side select*/
  GtkWidget     *restart;               /*spinner for setting frequency restart markers*/
  GtkWidget     *dct;                   /*DCT side select*/
  GtkWidget     *preview;               /*show preview toggle checkbox*/
  GtkWidget     *save_exif;
  GtkWidget     *save_thumbnail;
  GtkWidget     *save_xmp;
  GtkWidget     *save_iptc;
  GtkWidget     *use_orig_quality;      /*quant tables toggle*/
} JpegSaveGui;

static void  make_preview           (void);

static void  save_restart_update    (GtkAdjustment *adjustment,
                                     GtkWidget     *toggle);
static void  subsampling_changed    (GtkWidget     *combo,
                                     GtkAdjustment *entry);
static void  quality_changed        (GtkAdjustment *scale_entry,
                                     GtkWidget     *toggle);
static void  subsampling_changed2   (GtkWidget     *combo,
                                     GtkWidget     *toggle);
static void  use_orig_qual_changed  (GtkWidget     *toggle,
                                     GtkAdjustment *scale_entry);
static void  use_orig_qual_changed2 (GtkWidget     *toggle,
                                     GtkWidget     *combo);


static GtkWidget *restart_markers_scale = NULL;
static GtkWidget *restart_markers_label = NULL;
static GtkWidget *preview_size          = NULL;
static PreviewPersistent *prev_p        = NULL;

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
  if (prev_p)
    prev_p->abort_me = TRUE;
  (*cinfo->err->output_message) (cinfo);
}

static gboolean
background_jpeg_save (PreviewPersistent *pp)
{
  gint yend;

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

      g_free (pp->data);

      if (pp->buffer)
        g_object_unref (pp->buffer);

      /* display the preview stuff */
      if (!pp->abort_me)
        {
          GFile     *file = g_file_new_for_path (pp->file_name);
          GFileInfo *info;
          gchar     *text;
          GError    *error = NULL;

          info = g_file_query_info (file,
                                    G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                    G_FILE_QUERY_INFO_NONE,
                                    NULL, &error);

          if (info)
            {
              goffset  size = g_file_info_get_size (info);
              gchar   *size_text;

              size_text = g_format_size (size);
              text = g_strdup_printf (_("File size: %s"), size_text);
              g_free (size_text);

              g_object_unref (info);
            }
          else
            {
              text = g_strdup_printf (_("File size: %s"), error->message);
              g_clear_error (&error);
            }

          gtk_label_set_text (GTK_LABEL (preview_size), text);
          g_free (text);

          g_object_unref (file);

          /* and load the preview */
          load_image (pp->file_name, GIMP_RUN_NONINTERACTIVE, TRUE, NULL, NULL);
        }

      /* we cleanup here (load_image doesn't run in the background) */
      g_unlink (pp->file_name);

      g_free (pp);
      prev_p = NULL;

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
          gegl_buffer_get (pp->buffer,
                           GEGL_RECTANGLE (0, pp->cinfo.next_scanline,
                                           pp->cinfo.image_width,
                                           (yend - pp->cinfo.next_scanline)),
                           1.0,
                           pp->format,
                           pp->data,
                           GEGL_AUTO_ROWSTRIDE,
                           GEGL_ABYSS_NONE);
          pp->src = pp->data;
        }

      jpeg_write_scanlines (&(pp->cinfo), (JSAMPARRAY) &(pp->src), 1);
      pp->src += pp->rowstride;

      return TRUE;
    }
}

gboolean
save_image (const gchar  *filename,
            gint32        image_ID,
            gint32        drawable_ID,
            gint32        orig_image_ID,
            gboolean      preview,
            GError      **error)
{
  static struct jpeg_compress_struct cinfo;
  static struct my_error_mgr         jerr;

  GimpImageType    drawable_type;
  GeglBuffer      *buffer;
  const Babl      *format;
  JpegSubsampling  subsampling;
  FILE            * volatile outfile;
  guchar          *data;
  guchar          *src;
  gboolean         has_alpha;
  gint             rowstride, yend;

  drawable_type = gimp_drawable_type (drawable_ID);
  buffer = gimp_drawable_get_buffer (drawable_ID);

  if (! preview)
    gimp_progress_init_printf (_("Exporting '%s'"),
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
      if (buffer)
        g_object_unref (buffer);

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
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  jpeg_stdio_dest (&cinfo, outfile);

  /* Get the input image and a pointer to its data.
   */
  switch (drawable_type)
    {
    case GIMP_RGB_IMAGE:
      /* # of color components per pixel */
      cinfo.input_components = 3;
      has_alpha = FALSE;
      format = babl_format ("R'G'B' u8");
      break;

    case GIMP_GRAY_IMAGE:
      /* # of color components per pixel */
      cinfo.input_components = 1;
      has_alpha = FALSE;
      format = babl_format ("Y' u8");
      break;

    case GIMP_RGBA_IMAGE:
      /* # of color components per pixel (minus the GIMP alpha channel) */
      cinfo.input_components = 4 - 1;
      has_alpha = TRUE;
      format = babl_format ("R'G'B' u8");
      break;

    case GIMP_GRAYA_IMAGE:
      /* # of color components per pixel (minus the GIMP alpha channel) */
      cinfo.input_components = 2 - 1;
      has_alpha = TRUE;
      format = babl_format ("Y' u8");
      break;

    case GIMP_INDEXED_IMAGE:
    default:
      return FALSE;
    }

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  /* image width and height, in pixels */
  cinfo.image_width  = gegl_buffer_get_width (buffer);
  cinfo.image_height = gegl_buffer_get_height (buffer);
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

  if (jsvals.use_orig_quality && num_quant_tables > 0)
    {
      guint **quant_tables;
      gint    t;

      /* override tables generated by jpeg_set_quality() with custom tables */
      quant_tables = jpeg_restore_original_tables (image_ID, num_quant_tables);
      if (quant_tables)
        {
          for (t = 0; t < num_quant_tables; t++)
            {
              jpeg_add_quant_table (&cinfo, t, quant_tables[t],
                                    100, jsvals.baseline);
              g_free (quant_tables[t]);
            }
          g_free (quant_tables);
        }
    }

  if (arithc_supported)
    {
      cinfo.arith_code = jsvals.arithmetic_coding;
      if (!jsvals.arithmetic_coding)
        cinfo.optimize_coding = jsvals.optimize;
    }
  else
    cinfo.optimize_coding = jsvals.optimize;

  subsampling = (gimp_drawable_is_rgb (drawable_ID) ?
                 jsvals.subsmp : JPEG_SUBSAMPLING_1x1_1x1_1x1);

  /*  smoothing is not supported with nonstandard sampling ratios  */
  if (subsampling != JPEG_SUBSAMPLING_2x1_1x1_1x1 &&
      subsampling != JPEG_SUBSAMPLING_1x2_1x1_1x1)
    {
      cinfo.smoothing_factor = (gint) (jsvals.smoothing * 100);
    }

  if (jsvals.progressive)
    {
      jpeg_simple_progression (&cinfo);
    }

  switch (subsampling)
    {
    case JPEG_SUBSAMPLING_2x2_1x1_1x1:
    default:
      cinfo.comp_info[0].h_samp_factor = 2;
      cinfo.comp_info[0].v_samp_factor = 2;
      cinfo.comp_info[1].h_samp_factor = 1;
      cinfo.comp_info[1].v_samp_factor = 1;
      cinfo.comp_info[2].h_samp_factor = 1;
      cinfo.comp_info[2].v_samp_factor = 1;
      break;

    case JPEG_SUBSAMPLING_2x1_1x1_1x1:
      cinfo.comp_info[0].h_samp_factor = 2;
      cinfo.comp_info[0].v_samp_factor = 1;
      cinfo.comp_info[1].h_samp_factor = 1;
      cinfo.comp_info[1].v_samp_factor = 1;
      cinfo.comp_info[2].h_samp_factor = 1;
      cinfo.comp_info[2].v_samp_factor = 1;
      break;

    case JPEG_SUBSAMPLING_1x1_1x1_1x1:
      cinfo.comp_info[0].h_samp_factor = 1;
      cinfo.comp_info[0].v_samp_factor = 1;
      cinfo.comp_info[1].h_samp_factor = 1;
      cinfo.comp_info[1].v_samp_factor = 1;
      cinfo.comp_info[2].h_samp_factor = 1;
      cinfo.comp_info[2].v_samp_factor = 1;
      break;

    case JPEG_SUBSAMPLING_1x2_1x1_1x1:
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

  /* Step 4.1: Write the comment out - pw */
  if (image_comment && *image_comment)
    {
#ifdef GIMP_UNSTABLE
      g_print ("jpeg-save: saving image comment (%d bytes)\n",
               (int) strlen (image_comment));
#endif
      jpeg_write_marker (&cinfo, JPEG_COM,
                         (guchar *) image_comment, strlen (image_comment));
    }

  /* Step 4.2: store the color profile if there is one */
  {
    GimpColorProfile *profile = gimp_image_get_color_profile (orig_image_ID);

    if (profile)
      {
        const guint8 *icc_data;
        gsize         icc_length;

        icc_data = gimp_color_profile_get_icc_profile (profile, &icc_length);

        jpeg_icc_write_profile (&cinfo, icc_data, icc_length);

        g_object_unref (profile);
    }
  }

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  /* JSAMPLEs per row in image_buffer */
  rowstride = cinfo.input_components * cinfo.image_width;
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
      pp->data        = data;
      pp->buffer      = buffer;
      pp->format      = format;
      pp->src         = NULL;
      pp->file_name   = filename;
      pp->abort_me    = FALSE;

      g_warn_if_fail (prev_p == NULL);
      prev_p = pp;

      pp->cinfo.err = jpeg_std_error(&(pp->jerr));
      pp->jerr.error_exit = background_error_exit;

      gtk_label_set_text (GTK_LABEL (preview_size),
                          _("Calculating file size..."));

      pp->source_id = g_idle_add ((GSourceFunc) background_jpeg_save, pp);

      /* background_jpeg_save() will cleanup as needed */
      return TRUE;
    }

  while (cinfo.next_scanline < cinfo.image_height)
    {
      if ((cinfo.next_scanline % gimp_tile_height ()) == 0)
        {
          yend = cinfo.next_scanline + gimp_tile_height ();
          yend = MIN (yend, cinfo.image_height);
          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (0, cinfo.next_scanline,
                                           cinfo.image_width,
                                           (yend - cinfo.next_scanline)),
                           1.0,
                           format,
                           data,
                           GEGL_AUTO_ROWSTRIDE,
                           GEGL_ABYSS_NONE);
          src = data;
        }

      jpeg_write_scanlines (&cinfo, (JSAMPARRAY) &src, 1);
      src += rowstride;

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
  g_free (data);

  /* And we're done! */
  gimp_progress_update (1.0);

  g_object_unref (buffer);

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
                  TRUE, NULL);

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
  if (prev_p && !prev_p->abort_me)
    {
      guint id = prev_p->source_id;
      prev_p->abort_me = TRUE;   /* signal the background save to stop */
      background_jpeg_save (prev_p);
      g_source_remove (id);
    }

  if (gimp_image_is_valid (preview_image_ID) &&
      gimp_item_is_valid (preview_layer_ID))
    {
      /*  assuming that reference counting is working correctly,
          we do not need to delete the layer, removing it from
          the image should be sufficient  */
      gimp_image_remove_layer (preview_image_ID, preview_layer_ID);

      preview_layer_ID = -1;
    }
}

static void
toggle_arithmetic_coding (GtkToggleButton *togglebutton,
                          gpointer         user_data)
{
  GtkWidget *optimize = GTK_WIDGET (user_data);

  gtk_widget_set_sensitive (optimize,
                            !gtk_toggle_button_get_active (togglebutton));
}

gboolean
save_dialog (void)
{
  JpegSaveGui    pg;
  GtkWidget     *dialog;
  GtkWidget     *vbox;
  GtkAdjustment *entry;
  GtkWidget     *table;
  GtkWidget     *table2;
  GtkWidget     *tabledefaults;
  GtkWidget     *expander;
  GtkWidget     *frame;
  GtkWidget     *toggle;
  GtkWidget     *spinbutton;
  GtkWidget     *label;
  GtkWidget     *combo;
  GtkWidget     *text_view;
  GtkTextBuffer *text_buffer;
  GtkWidget     *scrolled_window;
  GtkWidget     *button;
  gchar         *text;
  gint           row;

  dialog = gimp_export_dialog_new (_("JPEG"), PLUG_IN_BINARY, SAVE_PROC);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (save_dialog_response),
                    &pg);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  pg.quality = entry = (GtkAdjustment *)
                       gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
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

  preview_size = gtk_label_new (_("File size: unknown"));
  gtk_label_set_xalign (GTK_LABEL (preview_size), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (preview_size), PANGO_ELLIPSIZE_END);
  gimp_label_set_attributes (GTK_LABEL (preview_size),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), preview_size, FALSE, FALSE, 0);
  gtk_widget_show (preview_size);

  gimp_help_set_help_data (preview_size,
                           _("Enable preview to obtain the file size."), NULL);

  pg.preview = toggle =
    gtk_check_button_new_with_mnemonic (_("Sho_w preview in image window"));
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

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_add (GTK_CONTAINER (expander), vbox);
  gtk_widget_show (vbox);

  frame = gimp_frame_new ("<expander>");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 8, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 12);
  gtk_container_add (GTK_CONTAINER (frame), table);

  table2 = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 6);
  gtk_table_attach (GTK_TABLE (table), table2,
                    2, 6, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (table2);

  pg.smoothing = entry = (GtkAdjustment *)
                         gimp_scale_entry_new (GTK_TABLE (table2), 0, 0,
                                               _("S_moothing:"),
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

  restart_markers_label = gtk_label_new (_("Interval (MCU rows):"));
  gtk_label_set_xalign (GTK_LABEL (restart_markers_label), 1.0);
  gtk_table_attach (GTK_TABLE (table), restart_markers_label, 4, 5, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (restart_markers_label);

  pg.scale_data = (GtkAdjustment *)
    gtk_adjustment_new (((jsvals.restart == 0) ?
                         DEFAULT_RESTART_MCU_ROWS : jsvals.restart),
                        1.0, 64.0, 1.0, 1.0, 0);
  pg.restart = restart_markers_scale = spinbutton =
    gtk_spin_button_new (pg.scale_data, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 5, 6, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  pg.use_restart_markers = toggle =
    gtk_check_button_new_with_mnemonic (_("Use _restart markers"));
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

  row = 0;

  /* Optimize */
  pg.optimize = toggle = gtk_check_button_new_with_mnemonic (_("_Optimize"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1,
                    row, row + 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.optimize);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (make_preview),
                    NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), jsvals.optimize);

  if (arithc_supported)
    gtk_widget_set_sensitive (toggle, !jsvals.arithmetic_coding);

  row++;

  if (arithc_supported)
    {
      /* Arithmetic coding */
      pg.arithmetic_coding = toggle = gtk_check_button_new_with_mnemonic
        (_("Use arithmetic _coding"));
      gtk_widget_set_tooltip_text
        (toggle, _("Older software may have trouble opening "
                   "arithmetic-coded images"));
      gtk_table_attach (GTK_TABLE (table), toggle, 0, 1,
                        row, row + 1, GTK_FILL, 0, 0, 0);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &jsvals.arithmetic_coding);
      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (make_preview),
                        NULL);
      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (toggle_arithmetic_coding),
                        pg.optimize);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    jsvals.arithmetic_coding);

      row++;
    }

  /* Progressive */
  pg.progressive = toggle =
    gtk_check_button_new_with_mnemonic (_("_Progressive"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1,
                    row, row + 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.progressive);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (make_preview),
                    NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                jsvals.progressive);

  row++;

  /* Save EXIF data */
  pg.save_exif = toggle =
    gtk_check_button_new_with_mnemonic (_("Save _Exif data"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), jsvals.save_exif);
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1,
                    row, row + 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.save_exif);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (make_preview),
                    NULL);

  gtk_widget_set_sensitive (toggle, TRUE);

  row++;

  /* Save thumbnail */
  pg.save_thumbnail = toggle =
    gtk_check_button_new_with_mnemonic (_("Save _thumbnail"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), jsvals.save_thumbnail);
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1,
                    row, row + 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.save_thumbnail);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (make_preview),
                    NULL);

  row++;

  /* XMP metadata */
  pg.save_xmp = toggle =
    gtk_check_button_new_with_mnemonic (_("Save _XMP data"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), jsvals.save_xmp);
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1,
                    row, row + 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.save_xmp);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (make_preview),
                    NULL);

  gtk_widget_set_sensitive (toggle, TRUE);

  row++;

  /* IPTC metadata */
  pg.save_iptc = toggle =
    gtk_check_button_new_with_mnemonic (_("Save _IPTC data"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), jsvals.save_iptc);
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1,
                    row, row + 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.save_iptc);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (make_preview),
                    NULL);

  gtk_widget_set_sensitive (toggle, TRUE);

  row++;

  /* custom quantization tables - now used also for original quality */
  pg.use_orig_quality = toggle =
    gtk_check_button_new_with_mnemonic (_("_Use quality settings from original "
                                          "image"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 4,
                    row, row + 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show (toggle);

  gimp_help_set_help_data (toggle,
                           _("If the original image was loaded from a JPEG "
                             "file using non-standard quality settings "
                             "(quantization tables), enable this option to "
                             "get almost the same quality and file size."),
                           NULL);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &jsvals.use_orig_quality);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                jsvals.use_orig_quality
                                && (orig_quality > 0)
                                && (orig_subsmp == jsvals.subsmp)
                               );
  gtk_widget_set_sensitive (toggle, (orig_quality > 0));

  /* changing quality disables custom quantization tables, and vice-versa */
  g_signal_connect (pg.quality, "value-changed",
                    G_CALLBACK (quality_changed),
                    pg.use_orig_quality);
  g_signal_connect (pg.use_orig_quality, "toggled",
                    G_CALLBACK (use_orig_qual_changed),
                    pg.quality);

  /* Subsampling */
  label = gtk_label_new_with_mnemonic (_("Su_bsampling:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  pg.subsmp =
    combo = gimp_int_combo_box_new (_("4:4:4 (best quality)"),
                                    JPEG_SUBSAMPLING_1x1_1x1_1x1,
                                    _("4:2:2 horizontal (chroma halved)"),
                                    JPEG_SUBSAMPLING_2x1_1x1_1x1,
                                    _("4:2:2 vertical (chroma halved)"),
                                    JPEG_SUBSAMPLING_1x2_1x1_1x1,
                                    _("4:2:0 (chroma quartered)"),
                                    JPEG_SUBSAMPLING_2x2_1x1_1x1,
                                    NULL);
  gtk_table_attach (GTK_TABLE (table), combo, 3, 6, 2, 3,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  if (gimp_drawable_is_rgb (drawable_ID_global))
    {
      gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                  jsvals.subsmp,
                                  G_CALLBACK (subsampling_changed),
                                  entry);
      g_signal_connect (pg.subsmp, "changed",
                        G_CALLBACK (subsampling_changed2),
                        pg.use_orig_quality);
      g_signal_connect (pg.use_orig_quality, "toggled",
                        G_CALLBACK (use_orig_qual_changed2),
                        pg.subsmp);
    }
  else
    {
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                     JPEG_SUBSAMPLING_1x1_1x1_1x1);

      gtk_widget_set_sensitive (combo, FALSE);
    }


  /* DCT method */
  label = gtk_label_new_with_mnemonic (_("_DCT method:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
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

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

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

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      vbox, TRUE, TRUE, 0);
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

  button = gtk_button_new_with_mnemonic (_("Sa_ve Defaults"));
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
load_defaults (void)
{
  GimpParasite *parasite;
  gchar        *def_str;
  JpegSaveVals  tmpvals;
  gint          num_fields;
  gint          subsampling;

  jsvals.quality          = DEFAULT_QUALITY;
  jsvals.smoothing        = DEFAULT_SMOOTHING;
  jsvals.optimize         = DEFAULT_OPTIMIZE;
  jsvals.arithmetic_coding= DEFAULT_ARITHMETIC_CODING;
  jsvals.progressive      = DEFAULT_PROGRESSIVE;
  jsvals.baseline         = DEFAULT_BASELINE;
  jsvals.subsmp           = DEFAULT_SUBSMP;
  jsvals.restart          = DEFAULT_RESTART;
  jsvals.dct              = DEFAULT_DCT;
  jsvals.preview          = DEFAULT_PREVIEW;
  jsvals.save_exif        = DEFAULT_EXIF;
  jsvals.save_thumbnail   = DEFAULT_THUMBNAIL;
  jsvals.save_xmp         = DEFAULT_XMP;
  jsvals.save_iptc        = DEFAULT_IPTC;
  jsvals.use_orig_quality = DEFAULT_USE_ORIG_QUALITY;

  parasite = gimp_get_parasite (JPEG_DEFAULTS_PARASITE);

  if (! parasite)
    return;

  def_str = g_strndup (gimp_parasite_data (parasite),
                       gimp_parasite_data_size (parasite));

  gimp_parasite_free (parasite);

  /* Initialize tmpvals in case fewer fields exist in the parasite
     (e.g., when importing from a previous version of GIMP). */
  memcpy(&tmpvals, &jsvals, sizeof jsvals);

  num_fields = sscanf (def_str,
                       "%lf %lf %d %d %d %d %d %d %d %d %d %d %d %d %d",
                       &tmpvals.quality,
                       &tmpvals.smoothing,
                       &tmpvals.optimize,
                       &tmpvals.progressive,
                       &subsampling,
                       &tmpvals.baseline,
                       &tmpvals.restart,
                       &tmpvals.dct,
                       &tmpvals.preview,
                       &tmpvals.save_exif,
                       &tmpvals.save_thumbnail,
                       &tmpvals.save_xmp,
                       &tmpvals.use_orig_quality,
                       &tmpvals.save_iptc,
                       &tmpvals.arithmetic_coding);

  tmpvals.subsmp = subsampling;

  if (num_fields == 13 || num_fields == 15)
    {
      memcpy (&jsvals, &tmpvals, sizeof (tmpvals));
    }

  g_free (def_str);
}

static void
save_defaults (void)
{
  GimpParasite *parasite;
  gchar        *def_str;

  def_str = g_strdup_printf ("%lf %lf %d %d %d %d %d %d %d %d %d %d %d %d %d",
                             jsvals.quality,
                             jsvals.smoothing,
                             jsvals.optimize,
                             jsvals.progressive,
                             (gint) jsvals.subsmp,
                             jsvals.baseline,
                             jsvals.restart,
                             jsvals.dct,
                             jsvals.preview,
                             jsvals.save_exif,
                             jsvals.save_thumbnail,
                             jsvals.save_xmp,
                             jsvals.use_orig_quality,
                             jsvals.save_iptc,
                             jsvals.arithmetic_coding);
  parasite = gimp_parasite_new (JPEG_DEFAULTS_PARASITE,
                                GIMP_PARASITE_PERSISTENT,
                                strlen (def_str), def_str);

  gimp_attach_parasite (parasite);

  gimp_parasite_free (parasite);
  g_free (def_str);
}

static void
load_gui_defaults (JpegSaveGui *pg)
{
  GtkAdjustment *restart_markers;

  load_defaults ();

#define SET_ACTIVE_BTTN(field) \
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pg->field), jsvals.field)

  SET_ACTIVE_BTTN (optimize);
  SET_ACTIVE_BTTN (progressive);
  SET_ACTIVE_BTTN (use_orig_quality);
  SET_ACTIVE_BTTN (preview);
  SET_ACTIVE_BTTN (save_exif);
  SET_ACTIVE_BTTN (save_thumbnail);
  SET_ACTIVE_BTTN (save_xmp);
  SET_ACTIVE_BTTN (save_iptc);

#undef SET_ACTIVE_BTTN

/*spin button stuff*/
  g_signal_handler_block (pg->use_restart_markers, pg->handler_id_restart);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pg->use_restart_markers),
                                jsvals.restart);
  restart_markers = pg->scale_data;
  gtk_adjustment_set_value (restart_markers, jsvals.restart);
  g_signal_handler_unblock (pg->use_restart_markers, pg->handler_id_restart);

  /* Don't override quality and subsampling setting if we alredy set it from original */
  if (!jsvals.use_orig_quality)
    {
      gtk_adjustment_set_value (pg->quality, jsvals.quality);

      if (gimp_drawable_is_rgb (drawable_ID_global))
        {
          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (pg->subsmp),
                                         jsvals.subsmp);
        }
    }

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (pg->dct),
                                 jsvals.dct);
}

static void
save_restart_update (GtkAdjustment *adjustment,
                     GtkWidget     *toggle)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)))
    jsvals.restart = gtk_adjustment_get_value (adjustment);
  else
    jsvals.restart = 0;

  gtk_widget_set_sensitive (restart_markers_label, jsvals.restart);
  gtk_widget_set_sensitive (restart_markers_scale, jsvals.restart);

  make_preview ();
}

static void
subsampling_changed (GtkWidget     *combo,
                     GtkAdjustment *entry)
{
  gint value;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &value);

  jsvals.subsmp = value;

  /*  smoothing is not supported with nonstandard sampling ratios  */
  gimp_scale_entry_set_sensitive ((gpointer) entry,
                                  jsvals.subsmp != JPEG_SUBSAMPLING_2x1_1x1_1x1 &&
                                  jsvals.subsmp != JPEG_SUBSAMPLING_1x2_1x1_1x1);

  make_preview ();
}

static void
quality_changed (GtkAdjustment *scale_entry,
                 GtkWidget     *toggle)
{
  if (jsvals.use_orig_quality)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), FALSE);
    }
}

static void
subsampling_changed2 (GtkWidget *combo,
                      GtkWidget *toggle)
{
  if (jsvals.use_orig_quality && orig_subsmp != jsvals.subsmp)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), FALSE);
    }
}

static void
use_orig_qual_changed (GtkWidget     *toggle,
                       GtkAdjustment *scale_entry)
{
  if (jsvals.use_orig_quality && orig_quality > 0)
    {
      g_signal_handlers_block_by_func (scale_entry, quality_changed, toggle);
      gtk_adjustment_set_value (scale_entry, orig_quality);
      g_signal_handlers_unblock_by_func (scale_entry, quality_changed, toggle);
    }
}

static void
use_orig_qual_changed2 (GtkWidget *toggle,
                        GtkWidget *combo)
{
  /* the test is (orig_quality > 0), not (orig_subsmp > 0) - this is normal */
  if (jsvals.use_orig_quality && orig_quality > 0)
    {
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), orig_subsmp);
    }
}
