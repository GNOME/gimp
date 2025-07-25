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
#include "jpeg-export.h"
#include "jpeg-settings.h"

/* See bugs #63610 and #61088 for a discussion about the quality settings */
#define DEFAULT_RESTART_MCU_ROWS 16


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
  GFile        *file;
  gboolean      abort_me;
  guint         source_id;
} PreviewPersistent;


static void  make_preview              (GimpProcedureConfig *config);

static void  quality_changed           (GimpProcedureConfig *config);
static void  subsampling_changed       (GimpProcedureConfig *config,
                                        const GParamSpec    *pspec,
                                        GtkWidget           *smoothing_scale);
static void  use_orig_qual_changed     (GimpProcedureConfig *config);
static void  use_orig_qual_changed_rgb (GimpProcedureConfig *config);


static GtkWidget         *preview_size  = NULL;
static PreviewPersistent *prev_p        = NULL;


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
      if (! pp->abort_me)
        {
          GFileInfo *info;
          gchar     *text;
          GError    *error = NULL;

          info = g_file_query_info (pp->file,
                                    G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                    G_FILE_QUERY_INFO_NONE,
                                    NULL, &error);

          if (info)
            {
              goffset  size = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_STANDARD_SIZE);
              gchar   *size_text;

              size_text = g_format_size (size);
              text = g_strdup_printf (_("File size without metadata: %s"), size_text);
              g_free (size_text);

              g_object_unref (info);
            }
          else
            {
              text = g_strdup_printf (_("File size without metadata: %s"), error->message);
              g_clear_error (&error);
            }

          gtk_label_set_text (GTK_LABEL (preview_size), text);
          g_free (text);

          /* and load the preview */
          load_image (pp->file, GIMP_RUN_NONINTERACTIVE,
                      TRUE, NULL, NULL, NULL);
        }

      /* we cleanup here (load_image doesn't run in the background) */
      g_file_delete (pp->file, NULL, NULL);
      g_object_unref (pp->file);

      g_free (pp);
      prev_p = NULL;

      gimp_displays_flush ();
      gdk_display_flush (gdk_display_get_default ());

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
export_image (GFile                *file,
              GimpProcedureConfig  *config,
              GimpImage            *image,
              GimpDrawable         *drawable,
              GimpImage            *orig_image,
              gboolean              preview,
              GError              **error)
{
  static struct jpeg_compress_struct cinfo;
  static struct my_error_mgr         jerr;

  GimpImageType     drawable_type;
  GeglBuffer       *buffer;
  const gchar      *encoding;
  const Babl       *format;
  const Babl       *space;
  JpegSubsampling   subsampling;
  FILE             * volatile outfile;
  guchar           *data;
  guchar           *src;
  GimpColorProfile *profile      = NULL;
  GimpColorProfile *cmyk_profile = NULL;

  gboolean         has_alpha;
  gboolean         out_linear = FALSE;
  gint             rowstride, yend;

  gint             quality;
  gdouble          dquality      = 1.0;
  gdouble          smoothing;
  gboolean         optimize;
  gboolean         progressive;
  gboolean         cmyk;
  gint             subsmp;
  gboolean         baseline;
  gint             restart;
  gint             dct;
  gboolean         save_profile          = TRUE;
  gboolean         save_comment;
  gboolean         use_orig_quality      = FALSE;
  gint             orig_num_quant_tables = -1;
  gboolean         use_arithmetic_coding = FALSE;
  gboolean         use_restart           = FALSE;
  gchar           *comment;

  g_object_get (config,
                "quality",                   &dquality,
                "smoothing",                 &smoothing,
                "optimize",                  &optimize,
                "progressive",               &progressive,
                "cmyk",                      &cmyk,
                "baseline",                  &baseline,
                "restart",                   &restart,

                /* Original quality settings. */
                "use-original-quality",      &use_orig_quality,
                "original-num-quant-tables", &orig_num_quant_tables,

                "use-arithmetic-coding",     &use_arithmetic_coding,
                "use-restart",               &use_restart,

                "include-color-profile",     &save_profile,
                "include-comment",           &save_comment,
                "gimp-comment",              &comment,

                NULL);
  dct    = gimp_procedure_config_get_choice_id (config, "dct");
  subsmp = gimp_procedure_config_get_choice_id (config, "sub-sampling");

  quality = (gint) (dquality * 100.0 + 0.5);

  drawable_type = gimp_drawable_type (drawable);
  buffer = gimp_drawable_get_buffer (drawable);
  space = gimp_drawable_get_format (drawable);

  if (! preview)
    gimp_progress_init_printf (_("Exporting '%s'"),
                               gimp_file_get_utf8_name (file));

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
  outfile = g_fopen (g_file_peek_path (file), "wb");

  if (! outfile)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  /* When we don't save profiles, we convert data to sRGB because
   * that's what most/all readers expect on a no-profile JPEG.
   * If we save an assigned profile, let's just follow its TRC.
   * If we save the default linear profile (i.e. no assigned
   * profile), we convert it to sRGB, except when it is 8-bit linear.
   */
  if (save_profile)
    {
      profile = gimp_image_get_color_profile (orig_image);

      /* If a profile is explicitly set, follow its TRC, whatever the
       * storage format.
       */
      if (profile && gimp_color_profile_is_linear (profile))
        out_linear = TRUE;

      if (! profile)
        {
          /* There is always an effective profile. */
          profile = gimp_image_get_effective_color_profile (orig_image);

          if (gimp_color_profile_is_linear (profile))
            {
              if (gimp_image_get_precision (image) != GIMP_PRECISION_U8_LINEAR)
                {
                  GimpColorProfile *saved_profile;

                  saved_profile = gimp_color_profile_new_srgb_trc_from_color_profile (profile);
                  g_object_unref (profile);
                  profile = saved_profile;
                }
              else
                {
                  /* Keep linear profile as-is for 8-bit linear image. */
                  out_linear = TRUE;
                }
            }
        }
      space = gimp_color_profile_get_space (profile,
                                            gimp_image_get_simulation_intent (image),
                                            error);
      if (error && *error)
        {
          /* XXX: the profile space should normally be the same one as
           * the drawable's so let's continue with it. We were mostly
           * getting the profile space to be complete. Still let's
           * display the error to standard error channel because if the
           * space could not be extracted, there is a problem somewhere!
           */
          g_printerr ("%s: error getting the profile space: %s",
                     G_STRFUNC, (*error)->message);
          g_clear_error (error);
          space = gimp_drawable_get_format (drawable);
        }
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

      if (out_linear)
        encoding = "RGB u8";
      else
        encoding = "R'G'B' u8";
      break;

    case GIMP_GRAY_IMAGE:
      /* # of color components per pixel */
      cinfo.input_components = 1;
      has_alpha = FALSE;

      if (out_linear)
        encoding = "Y u8";
      else
        encoding = "Y' u8";
      break;

    case GIMP_RGBA_IMAGE:
      /* # of color components per pixel (minus the GIMP alpha channel) */
      cinfo.input_components = 4 - 1;
      has_alpha = TRUE;

      if (out_linear)
        encoding = "RGB u8";
      else
        encoding = "R'G'B' u8";
      break;

    case GIMP_GRAYA_IMAGE:
      /* # of color components per pixel (minus the GIMP alpha channel) */
      cinfo.input_components = 2 - 1;
      has_alpha = TRUE;
      if (out_linear)
        encoding = "Y u8";
      else
        encoding = "Y' u8";
      break;

    case GIMP_INDEXED_IMAGE:
    default:
      return FALSE;
    }

  if (cmyk)
    {
      if (save_profile)
        {
          GError *err = NULL;

          cmyk_profile = gimp_image_get_simulation_profile (image);
          if (! cmyk_profile && err)
            g_printerr ("%s: no soft-proof profile: %s\n", G_STRFUNC, err->message);

          if (cmyk_profile && ! gimp_color_profile_is_cmyk (cmyk_profile))
            g_clear_object (&cmyk_profile);

          g_clear_error (&err);
        }

      /* As far as I know, without access to JPEG specifications, we
       * should encode as proper "CMYK" encoding scheme. But every other
       * program where I test this JPEG, the color are inverted, so I
       * use the "cmyk" encoding where 0.0 is full ink coverage vs. 1.0
       * being no ink.
       * libjpeg-turbo says that Photoshop is wrongly inverting the data
       * in JPEG files in a 1994 commit! We might imagine that since
       * then it became the de-facto encoding?
       * See: https://github.com/libjpeg-turbo/libjpeg-turbo/blob/dfc63d42ee3d1ae8eacb921e89e64ac57861dff6/libjpeg.txt#L1425-L1438
       */
      encoding = "cmyk u8";

      if (cmyk_profile)
        space = gimp_color_profile_get_space (cmyk_profile,
                                              GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                              error);
      else
        /* The NULL space will fallback to a naive CMYK conversion. */
        space = NULL;
    }

  format = babl_format_with_space (encoding, space);

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  /* image width and height, in pixels */
  cinfo.image_width  = gegl_buffer_get_width (buffer);
  cinfo.image_height = gegl_buffer_get_height (buffer);
  /* colorspace of input image */
  if (cmyk)
    {
      cinfo.input_components = 4;
      cinfo.in_color_space   = JCS_CMYK;
      cinfo.jpeg_color_space = JCS_CMYK;
    }
  else
    {
      cinfo.in_color_space = (drawable_type == GIMP_RGB_IMAGE ||
                              drawable_type == GIMP_RGBA_IMAGE)
        ? JCS_RGB : JCS_GRAYSCALE;
    }
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults (&cinfo);

  if (cmyk_profile)
    jpeg_set_colorspace (&cinfo, JCS_CMYK);

  jpeg_set_quality (&cinfo, quality, baseline);

  if (use_orig_quality && orig_num_quant_tables > 0)
    {
      guint **quant_tables;
      gint    t;

      /* override tables generated by jpeg_set_quality() with custom tables */
      quant_tables = jpeg_restore_original_tables (image, orig_num_quant_tables);
      if (quant_tables)
        {
          for (t = 0; t < orig_num_quant_tables; t++)
            {
              jpeg_add_quant_table (&cinfo, t, quant_tables[t],
                                    100, baseline);
              g_free (quant_tables[t]);
            }
          g_free (quant_tables);
        }
    }

#ifdef C_ARITH_CODING_SUPPORTED
  cinfo.arith_code = use_arithmetic_coding;
  if (! use_arithmetic_coding)
    cinfo.optimize_coding = optimize;
#else
  cinfo.optimize_coding = optimize;
#endif

  subsampling = (gimp_drawable_is_rgb (drawable) ?
                 subsmp : JPEG_SUBSAMPLING_1x1_1x1_1x1);

  /*  smoothing is not supported with nonstandard sampling ratios  */
  if (subsampling != JPEG_SUBSAMPLING_2x1_1x1_1x1 &&
      subsampling != JPEG_SUBSAMPLING_1x2_1x1_1x1)
    {
      cinfo.smoothing_factor = (gint) (smoothing * 100);
    }

  if (progressive)
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
  cinfo.restart_in_rows = use_restart ? restart : 0;

  switch (dct)
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

    gimp_image_get_resolution (orig_image, &xresolution, &yresolution);

    if (xresolution > 1e-5 && yresolution > 1e-5)
      {
        gdouble factor;

        factor = gimp_unit_get_factor (gimp_image_get_unit (orig_image));

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
  if (save_comment && comment && *comment)
    {
#ifdef GIMP_UNSTABLE
      g_print ("jpeg-export: saving image comment (%d bytes)\n",
               (int) strlen (comment));
#endif
      jpeg_write_marker (&cinfo, JPEG_COM,
                         (guchar *) comment, strlen (comment));
    }

  /* Step 4.2: store the color profile */
  if (save_profile &&
      /* XXX Only case when we don't save a profile even though the
       * option was requested is if we store as CMYK without setting a
       * profile. It would actually be better to generate a profile
       * corresponding to the "naive" CMYK space we use in such case.
       * But it doesn't look like babl can do this yet.
       */
      (! cmyk || cmyk_profile != NULL))
    {
      const guint8 *icc_data;
      gsize         icc_length;

      icc_data = gimp_color_profile_get_icc_profile (cmyk_profile ? cmyk_profile : profile, &icc_length);
      jpeg_icc_write_profile (&cinfo, icc_data, icc_length);
    }
  g_clear_object (&profile);
  g_clear_object (&cmyk_profile);

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
      pp->file        = g_object_ref (file);
      pp->abort_me    = FALSE;

      g_warn_if_fail (prev_p == NULL);
      prev_p = pp;

      pp->cinfo.err = jpeg_std_error(&(pp->jerr));
      pp->jerr.error_exit = background_error_exit;

      gtk_label_set_text (GTK_LABEL (preview_size),
                          _("Calculating approximate file size..."));

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
make_preview (GimpProcedureConfig *config)
{
  gboolean show_preview;

  destroy_preview ();

  g_object_get (config, "show-preview", &show_preview, NULL);

  if (show_preview)
    {
      GFile *file = gimp_temp_file ("jpeg");

      if (! undo_touched)
        {
          /* we freeze undo saving so that we can avoid sucking up
           * tile cache with our unneeded preview steps. */
          gimp_image_undo_freeze (preview_image);

          undo_touched = TRUE;
        }

      export_image (file, config,
                    preview_image,
                    drawable_global,
                    orig_image_global,
                    TRUE, NULL);

      g_object_unref (file);

      if (separate_display && ! display)
        display = gimp_display_new (preview_image);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (preview_size), _("File size without metadata: unknown"));

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

  if (gimp_image_is_valid (preview_image) &&
      gimp_item_is_valid (GIMP_ITEM (preview_layer)))
    {
      /* assuming that reference counting is working correctly, we do
       * not need to delete the layer, removing it from the image
       * should be sufficient
       */
      gimp_image_remove_layer (preview_image, preview_layer);

      preview_layer = NULL;
    }
}

gboolean
save_dialog (GimpProcedure       *procedure,
             GimpProcedureConfig *config,
             GimpDrawable        *drawable,
             GimpImage           *image)
{
  GtkWidget        *dialog;
  GtkWidget        *widget;
  GtkWidget        *box;
  GtkWidget        *profile_label;
  GimpColorProfile *cmyk_profile = NULL;
  gint              orig_quality;
  gint              restart;
  gboolean          run;

  g_object_get (config,
                "original-quality", &orig_quality,
                "restart",          &restart,
                NULL);

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             gimp_item_get_image (GIMP_ITEM (drawable)));

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "option-title", _("Options"),
                                   FALSE, FALSE);

  /* custom quantization tables - now used also for original quality */
  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                       "use-original-quality",
                                       (orig_quality > 0), NULL, NULL, FALSE);

  /* Quality as a GimpScaleEntry. */
  gimp_procedure_dialog_get_spin_scale (GIMP_PROCEDURE_DIALOG (dialog), "quality", 100.0);

  /* changing quality disables custom quantization tables, and vice-versa */
  g_signal_connect (config, "notify::quality",
                    G_CALLBACK (quality_changed),
                    NULL);
  g_signal_connect (config, "notify::use-original-quality",
                    G_CALLBACK (use_orig_qual_changed),
                    NULL);

  /* File size label. */
  preview_size = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                                  "preview-size", _("File size without metadata: unknown"),
                                                  FALSE, FALSE);
  gtk_label_set_xalign (GTK_LABEL (preview_size), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (preview_size), PANGO_ELLIPSIZE_END);
  gimp_label_set_attributes (GTK_LABEL (preview_size),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gimp_help_set_help_data (preview_size,
                           _("Enable preview to obtain the approximate file size."), NULL);


  /* Profile label. */
  profile_label = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                                   "profile-label", _("No soft-proofing profile"),
                                                   FALSE, FALSE);
  gtk_label_set_xalign (GTK_LABEL (profile_label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (profile_label), PANGO_ELLIPSIZE_END);
  gimp_label_set_attributes (GTK_LABEL (profile_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gimp_help_set_help_data (profile_label,
                           _("Name of the color profile used for CMYK export."), NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "cmyk-frame", "cmyk", FALSE,
                                    "profile-label");
  cmyk_profile = gimp_image_get_simulation_profile (image);
  if (cmyk_profile)
    {
      if (gimp_color_profile_is_cmyk (cmyk_profile))
        {
          gchar *label_text;

          label_text = g_strdup_printf (_("Profile: %s"),
                                        gimp_color_profile_get_label (cmyk_profile));
          gtk_label_set_text (GTK_LABEL (profile_label), label_text);
          gimp_label_set_attributes (GTK_LABEL (profile_label),
                                     PANGO_ATTR_STYLE, PANGO_STYLE_NORMAL,
                                     -1);
          g_free (label_text);
        }
      g_object_unref (cmyk_profile);
    }

#ifdef C_ARITH_CODING_SUPPORTED
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "arithmetic-frame", "use-arithmetic-coding", TRUE,
                                    "optimize");
#endif

  /* Restart marker. */
  /* TODO: apparently when toggle is unchecked, we want to show the
   * scale as 0.
   */
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "restart-frame", "use-restart", FALSE,
                                    "restart");
  if (restart == 0)
    g_object_set (config,
                  "restart",     DEFAULT_RESTART_MCU_ROWS,
                  "use-restart", FALSE,
                  NULL);

  /* Subsampling */
  widget = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                             "sub-sampling", G_TYPE_NONE);

  if (! gimp_drawable_is_rgb (drawable))
    {
      g_object_set (config, "sub-sampling", "sub-sampling-1x1", NULL);
      gtk_widget_set_sensitive (widget, FALSE);
    }

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "advanced-title", _("Advanced Options"),
                                   FALSE, FALSE);
  widget = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                             "smoothing", GIMP_TYPE_SPIN_SCALE);
  gimp_help_set_help_data (widget, NULL, "file-jpeg-save-smoothing");

  /* Add some logics for "Use original quality". */
  if (gimp_drawable_is_rgb (drawable))
    {
      g_signal_connect (config, "notify::sub-sampling",
                        G_CALLBACK (subsampling_changed),
                        widget);
      subsampling_changed (config, NULL, widget);
      g_signal_connect (config, "notify::use-original-quality",
                        G_CALLBACK (use_orig_qual_changed_rgb),
                        NULL);
    }

  /* Put options in two column form so the dialog fits on
   * smaller screens. */
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "options",
                                  "quality",
                                  "use-original-quality",
                                  "preview-size",
                                  "show-preview",
                                  "progressive",
                                  "cmyk-frame",
                                  NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "option-frame", "option-title", FALSE,
                                    "options");

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "advanced-options",
                                  "smoothing",
#ifdef C_ARITH_CODING_SUPPORTED
                                  "arithmetic-frame",
#else
                                  "optimize",
#endif
                                  "restart-frame",
                                  "sub-sampling",
                                  "dct",
                                  NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "advanced-frame", "advanced-title", FALSE,
                                    "advanced-options");

  box = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                        "jpeg-hbox", "option-frame",
                                        "advanced-frame", NULL);
  gtk_box_set_spacing (GTK_BOX (box), 12);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_HORIZONTAL);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "jpeg-hbox", NULL);

  /* Run make_preview() when various config are changed. */
  g_signal_connect (config, "notify",
                    G_CALLBACK (make_preview),
                    NULL);

  make_preview (config);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  destroy_preview ();

  return run;
}

static void
quality_changed (GimpProcedureConfig *config)
{
  gboolean use_orig_quality;
  gdouble  quality;
  gint     orig_quality;

  g_object_get (config,
                "use-original-quality",  &use_orig_quality,
                "original-quality",      &orig_quality,
                "quality",               &quality,
                NULL);

  if (use_orig_quality && (gint) (quality * 100.0) != orig_quality)
    g_object_set (config, "use-original-quality", FALSE, NULL);
}

static void
subsampling_changed (GimpProcedureConfig *config,
                     const GParamSpec    *pspec,
                     GtkWidget           *smoothing_scale)
{
  gboolean use_orig_quality;
  gint     orig_subsmp;
  gint     subsmp;

  g_object_get (config,
                "use-original-quality",  &use_orig_quality,
                "original-sub-sampling", &orig_subsmp,
                NULL);
  subsmp = gimp_procedure_config_get_choice_id (config, "sub-sampling");

  /*  smoothing is not supported with nonstandard sampling ratios  */
  gtk_widget_set_sensitive (smoothing_scale,
                            subsmp != JPEG_SUBSAMPLING_2x1_1x1_1x1 &&
                            subsmp != JPEG_SUBSAMPLING_1x2_1x1_1x1);

  if (use_orig_quality && orig_subsmp != subsmp)
    g_object_set (config, "use-original-quality", FALSE, NULL);
}

static void
use_orig_qual_changed (GimpProcedureConfig *config)
{
  gboolean use_orig_quality;
  gint     orig_quality;

  g_object_get (config,
                "use-original-quality", &use_orig_quality,
                "original-quality",     &orig_quality,
                NULL);

  if (use_orig_quality && orig_quality > 0)
    {
      g_signal_handlers_block_by_func (config, quality_changed, NULL);
      g_object_set (config, "quality", orig_quality / 100.0, NULL);
      g_signal_handlers_unblock_by_func (config, quality_changed, NULL);
    }
}

static void
use_orig_qual_changed_rgb (GimpProcedureConfig *config)
{
  gboolean use_orig_quality;
  gint     orig_quality;
  gint     orig_subsmp;

  g_object_get (config,
                "use-original-quality",  &use_orig_quality,
                "original-sub-sampling", &orig_subsmp,
                "original-quality",      &orig_quality,
                NULL);

  /* the test is (orig_quality > 0), not (orig_subsmp > 0) - this is normal */
  if (use_orig_quality && orig_quality > 0)
    {
      switch (orig_subsmp)
        {
        case JPEG_SUBSAMPLING_1x1_1x1_1x1:
          g_object_set (config, "sub-sampling", "sub-sampling-1x1", NULL);
          break;

        case JPEG_SUBSAMPLING_2x1_1x1_1x1:
          g_object_set (config, "sub-sampling", "sub-sampling-2x1", NULL);
          break;

        case JPEG_SUBSAMPLING_1x2_1x1_1x1:
          g_object_set (config, "sub-sampling", "sub-sampling-1x2", NULL);
          break;

        case JPEG_SUBSAMPLING_2x2_1x1_1x1:
          g_object_set (config, "sub-sampling", "sub-sampling-2x2", NULL);
          break;
        }
    }
}
