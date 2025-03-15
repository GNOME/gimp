/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * file-icns-export.c
 * Copyright (C) 2004 Brion Vibber <brion@pobox.com>
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

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "file-icns.h"
#include "file-icns-data.h"
#include "file-icns-load.h"
#include "file-icns-export.h"

#include "libgimp/stdplugins-intl.h"

GtkWidget *        icns_dialog_new       (IcnsSaveInfo         *info,
                                          GimpImage            *image,
                                          GimpProcedure        *procedure,
                                          GimpProcedureConfig  *config);

static gboolean    icns_save_dialog      (IcnsSaveInfo         *info,
                                          GimpImage            *image,
                                          GimpProcedure        *procedure,
                                          GimpProcedureConfig  *config);

void               icns_dialog_add_icon  (GtkWidget            *dialog,
                                          GimpDrawable         *layer,
                                          gint                  layer_num,
                                          gint                  duplicates[]);

static GtkWidget * icns_preview_new      (GimpDrawable         *layer);

static GtkWidget * icns_create_icon_item (GtkWidget            *icon_preview,
                                          GimpDrawable         *layer,
                                          gint                  layer_num,
                                          IcnsSaveInfo         *info,
                                          gint                  duplicates[]);

static gint        icns_find_type        (gint                  width,
                                          gint                  height);
static gboolean    icns_check_dimensions (gint                  width,
                                          gint                  height);
static gboolean    icns_check_compat     (GtkWidget            *dialog,
                                          IcnsSaveInfo         *info);

GimpPDBStatusType  icns_export_image     (GFile                *file,
                                          IcnsSaveInfo         *info,
                                          GimpImage            *image,
                                          gboolean              include_color_profile,
                                          GError              **error);

static guchar    * icns_compress         (guint                 width,
                                          guint                 height,
                                          guchar               *rgba,
                                          gint                 *out_size);

static void        icns_save_info_free   (IcnsSaveInfo *info);

/* Referenced from plug-ins/file-ico/ico-dialog.c */
void
icns_dialog_add_icon (GtkWidget    *dialog,
                      GimpDrawable *layer,
                      gint          layer_num,
                      gint          duplicates[])
{
  GtkWidget    *flowbox;
  GtkWidget    *vbox_item;
  GtkWidget    *preview;
  gchar         key[ICNS_MAXBUF];
  IcnsSaveInfo *info;

  flowbox = g_object_get_data (G_OBJECT (dialog), "icons_flowbox");
  info    = g_object_get_data (G_OBJECT (dialog), "save_info");

  preview   = icns_preview_new (layer);
  vbox_item = icns_create_icon_item (preview, layer, layer_num, info,
                                     duplicates);
  gtk_flow_box_insert (GTK_FLOW_BOX (flowbox), vbox_item, -1);
  gtk_widget_show (vbox_item);

  /* Let's make the vbox_item accessible through the layer ID */
  g_snprintf (key, sizeof (key), "layer_%i_hbox",
              gimp_item_get_id (GIMP_ITEM (layer)));
  g_object_set_data (G_OBJECT (dialog), key, vbox_item);

  icns_check_compat (dialog, info);
}

static GtkWidget *
icns_preview_new (GimpDrawable *layer)
{
  GtkWidget *image;
  GdkPixbuf *pixbuf;
  gint       width  = gimp_drawable_get_width (layer);
  gint       height = gimp_drawable_get_height (layer);

  pixbuf = gimp_drawable_get_thumbnail (layer,
                                        MIN (width, 128), MIN (height, 128),
                                        GIMP_PIXBUF_SMALL_CHECKS);
  image = gtk_image_new_from_pixbuf (pixbuf);

  g_object_unref (pixbuf);

  return image;
}

static gint
icns_find_type (gint width,
                gint height)
{
  gint match = -1;

  for (gint j = 0; iconTypes[j].type; j++)
    {
      /* TODO: Currently, this chooses the first "modern" ICNS format for a
       * ICNS file. This is because newer formats are not supported well in
       * non-native MacOS programs like Inkscape. It'd be nice to design
       * a GUI with enough information for users to make their own decisions
       */
      if (iconTypes[j].width == width   &&
          iconTypes[j].height == height &&
          iconTypes[j].isModern)
        {
          match = j;
          break;
        }
    }

  return match;
}

static gboolean
icns_check_dimensions (gint width,
                       gint height)
{
  gboolean isValid = TRUE;

  if (width != height)
    {
      /* Only valid non-square size is 16x12 */
      if (! (width == 16 && height == 12))
        isValid = FALSE;
    }
  else
    {
      /* Valid square ICNS sizes */
      if (width != 16   &&
          width != 18   &&
          width != 24   &&
          width != 32   &&
          width != 36   &&
          width != 48   &&
          width != 64   &&
          width != 128  &&
          width != 256  &&
          width != 512  &&
          width != 1024)
        isValid = FALSE;
    }

  return isValid;
}

static GtkWidget *
icns_create_icon_item (GtkWidget    *icon_preview,
                       GimpDrawable *layer,
                       gint          layer_num,
                       IcnsSaveInfo *info,
                       gint          duplicates[])
{
  static GtkSizeGroup *size = NULL;

  GtkWidget *vbox_item;
  GtkWidget *frame;
  gchar     *frame_header;
  gint       match  = -1;
  gint       width  = gimp_drawable_get_width (layer);
  gint       height = gimp_drawable_get_height (layer);

  vbox_item = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

  /* To make life easier for the callbacks, we store the
     layer's ID and stacking number with vbox_item. */

  g_object_set_data (G_OBJECT (vbox_item),
                     "icon_layer", layer);
  g_object_set_data (G_OBJECT (vbox_item),
                     "icon_layer_num", GINT_TO_POINTER (layer_num));

  frame_header = g_strdup_printf ("%dx%d", width, height);

  frame = gimp_frame_new (frame_header);
  gtk_box_pack_start (GTK_BOX (vbox_item), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  g_free (frame_header);

  g_object_set_data (G_OBJECT (vbox_item), "icon_preview", icon_preview);
  gtk_container_add (GTK_CONTAINER (frame), icon_preview);
  gtk_widget_show (icon_preview);

  if (! size)
    size = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

  gtk_size_group_add_widget (size, icon_preview);

  match = icns_find_type (gimp_drawable_get_width (layer),
                          gimp_drawable_get_height (layer));

  if (! icns_check_dimensions (width, height) ||
      (match != -1 && duplicates[match] != 0))
    {
      GtkWidget *label;
      gchar     *warning;
      gchar     *markup;

      if (! icns_check_dimensions (width, height))
        warning = g_strdup_printf (_("Invalid icon size. \n"
                                     "It will not be exported"));
      else
        warning = g_strdup_printf (_("Duplicate layer size. \n"
                                     "It will not be exported"));

      markup = g_strdup_printf ("<i>%s</i>", warning);

      label = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (label), markup);
      g_free (markup);
      g_free (warning);

      gtk_box_pack_start (GTK_BOX (vbox_item), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
      gtk_style_context_add_class (gtk_widget_get_style_context (vbox_item),
                                   "background");
    }

  if (match != -1)
    duplicates[match] = 1;

  return vbox_item;
}

static gboolean
icns_check_compat (GtkWidget    *dialog,
                   IcnsSaveInfo *info)
{
  GtkWidget *warning;
  GList     *iter;
  gboolean   warn = FALSE;
  gint       i;

  for (iter = info->layers, i = 0; iter; iter = iter->next, i++)
    {
      gint width  = gimp_drawable_get_width (iter->data);
      gint height = gimp_drawable_get_height (iter->data);

      warn = ! icns_check_dimensions (width, height);
      if (warn)
        break;
    }

  if (dialog)
    {
      warning = g_object_get_data (G_OBJECT (dialog), "warning");
      gtk_widget_set_visible (warning, warn);
    }

  return ! warn;
}

GtkWidget *
icns_dialog_new (IcnsSaveInfo        *info,
                 GimpImage           *image,
                 GimpProcedure       *procedure,
                 GimpProcedureConfig *config)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *frame;
  GtkWidget     *scrolled_window;
  GtkWidget     *viewport;
  GtkWidget     *flowbox;
  GtkWidget     *warning;

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             config, image);

  g_object_set_data (G_OBJECT (dialog), "save_info", info);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (main_vbox, TRUE);

  warning = g_object_new (GIMP_TYPE_HINT_BOX,
                          "icon-name", GIMP_ICON_DIALOG_WARNING,
                          "hint",
                          _("Valid ICNS icons sizes are:\n "
                            "16x12, 16x16, 18x18, 24x24, 32x32, 36x36, 48x48,\n"
                            "64x64, 128x128, 256x256, 512x512, and 1024x1024.\n"
                            "Any other sized layers will be ignored on export."),
                          NULL);
  gtk_box_pack_end (GTK_BOX (main_vbox), warning, FALSE, FALSE, 12);
  /* Don't show warning by default */

  frame = gimp_frame_new (_("Export Icons"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 4);
  gtk_widget_set_visible (frame, TRUE);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
  gtk_widget_set_size_request (scrolled_window, -1, 256);
  gtk_widget_set_visible (scrolled_window, TRUE);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window), viewport);
  gtk_widget_set_visible (viewport, TRUE);

  flowbox = gtk_flow_box_new ();
  gtk_flow_box_set_column_spacing (GTK_FLOW_BOX (flowbox), 6);
  gtk_flow_box_set_row_spacing (GTK_FLOW_BOX (flowbox), 6);
  gtk_flow_box_set_selection_mode (GTK_FLOW_BOX (flowbox), GTK_SELECTION_NONE);
  g_object_set_data (G_OBJECT (dialog), "icons_flowbox", flowbox);
  gtk_container_add (GTK_CONTAINER (viewport), flowbox);
  gtk_widget_set_visible (flowbox, TRUE);

  g_object_set_data (G_OBJECT (dialog), "warning", warning);

  return dialog;
}

static gboolean
icns_save_dialog (IcnsSaveInfo        *info,
                  GimpImage           *image,
                  GimpProcedure       *procedure,
                  GimpProcedureConfig *config)
{
  GtkWidget *dialog;
  GList     *iter;
  gint       i;
  gint       j;
  gboolean   response;
  gint       duplicates[ICNS_TYPE_NUM];
  gint       ordered[12] =
    {12, 16, 18, 24, 32, 36, 48, 64, 128, 256, 512, 1024};

  gimp_ui_init (PLUG_IN_BINARY);

  for (i = 0; i < ICNS_TYPE_NUM; i++)
    duplicates[i] = 0;

  dialog = icns_dialog_new (info, image, procedure, config);

  /* Add icons in order, smallest to largest */
  for (i = 0; i < 12; i++)
    {
      for (iter = info->layers, j = 0;
           iter;
           iter = g_list_next (iter), j++)
        {
          /* Put the icons in order in dialog */
          gint width  = gimp_drawable_get_width (iter->data);
          gint height = gimp_drawable_get_height (iter->data);

          if (height != ordered[i] || ! icns_check_dimensions (width, height))
            continue;

          icns_dialog_add_icon (dialog, iter->data, i, duplicates);
        }
    }

  /* Add any invalid icons at the end */
  for (iter = info->layers, i = 0;
       iter;
       iter = g_list_next (iter), i++)
    {
      if (! icns_check_dimensions (gimp_drawable_get_width (iter->data),
                                   gimp_drawable_get_height (iter->data)))
        icns_dialog_add_icon (dialog, iter->data, i, duplicates);
    }

  /* Scale the thing to approximately fit its content, but not too large ... */
  gtk_window_set_default_size (GTK_WINDOW (dialog),
                               200 + (info->num_icons > 4 ?
                                      500 : info->num_icons * 120),
                               200 + (info->num_icons > 4 ?
                                      250 : info->num_icons * 60));

  gtk_widget_set_visible (dialog, TRUE);

  response = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return response;
}

GimpPDBStatusType
icns_export_image (GFile        *file,
                   IcnsSaveInfo *info,
                   GimpImage    *image,
                   gboolean      include_color_profile,
                   GError      **error)
{
  FILE           *fp;
  GList          *iter;
  gint            i;
  guint32         file_size   = 8;
  gint            duplicates[ICNS_TYPE_NUM];

  for (i = 0; i < ICNS_TYPE_NUM; i++)
    duplicates[i] = 0;

  fp = g_fopen (g_file_peek_path (file), "wb");

  if (! fp)
    {
      icns_save_info_free (info);
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return GIMP_PDB_EXECUTION_ERROR;
    }

  /* Write Header */
  fwrite ("icns", sizeof (gchar), 4, fp);
  fwrite ("\0\0\0\0", sizeof (gchar), 4, fp);  /* will be filled in later */

  /* Write Icon Data */
  for (iter = info->layers, i = 0;
       iter;
       iter = g_list_next (iter), i++)
    {
      gint match  = -1;
      gint width  = gimp_drawable_get_width (iter->data);
      gint height = gimp_drawable_get_height (iter->data);

      /* Don't export icons with invalid dimensions */
      if (! icns_check_dimensions (width, height))
        continue;

      match = icns_find_type (width, height);

      /* MacOS X format icons */
      if (match != -1 && duplicates[match] == 0)
        {
          gint temp_size;
          gint macos_size;

          /* icp4 - 6 types (16x16, 32x32 and 48x48 icons) do not render well
           * in applications if saved as PNGs. Therefore, we will save those
           * in the older format for compatibility. */
          if (! g_strcmp0 (iconTypes[match].type, "icp4") ||
              ! g_strcmp0 (iconTypes[match].type, "icp5") ||
              ! g_strcmp0 (iconTypes[match].type, "icp6"))
            {
              GeglBuffer *buffer;
              guchar     *pixels;
              guchar     *alpha     = NULL;
              guchar     *output    = NULL;
              gint        compat_id = -1;

              macos_size = 0;

              for (compat_id = 0; iconTypes[compat_id].type; compat_id++)
                {
                  if (iconTypes[compat_id].width == width   &&
                      iconTypes[compat_id].height == height &&
                      iconTypes[compat_id].bits == 32)
                    break;
                }

              fwrite (iconTypes[compat_id].type, sizeof (gchar), 4, fp);
              temp_size = width * height * 4;

              buffer = gimp_drawable_get_buffer (iter->data);
              pixels = g_malloc (temp_size);
              alpha  = g_malloc (width * height);

              gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height),
                               1.0, babl_format ("R'G'B'A u8"), pixels,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
              gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height),
                               1.0, babl_format ("A u8"), alpha,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

              output = icns_compress (width, height, pixels, &macos_size);

              /* ---------------------- */

              temp_size = GUINT32_TO_BE (macos_size + 8);
              fwrite (&temp_size, sizeof (temp_size), 1, fp);

              if (fwrite (output, 1, macos_size, fp) < macos_size)
                {
                  icns_save_info_free (info);
                  g_set_error (error, G_FILE_ERROR,
                               g_file_error_from_errno (errno),
                               _("Error writing icns: %s"),
                               g_strerror (errno));
                  return GIMP_PDB_EXECUTION_ERROR;
                }

              file_size += macos_size + 8;

              /* Write uncompressed mask */
              fwrite (iconTypes[compat_id].mask, sizeof (gchar), 4, fp);
              macos_size = GUINT32_TO_BE ((width * height) + 8);
              fwrite (&macos_size, sizeof (macos_size), 1, fp);
              macos_size = width * height;

              if (fwrite (alpha, 1, macos_size, fp) < macos_size)
                {
                  icns_save_info_free (info);
                  g_set_error (error, G_FILE_ERROR,
                               g_file_error_from_errno (errno),
                               _("Error writing icns: %s"),
                               g_strerror (errno));
                  return GIMP_PDB_EXECUTION_ERROR;
                }

              file_size += (width * height) + 8;

              g_free (pixels);
              g_free (alpha);
              g_object_unref (buffer);
            }
          else
            {
              GimpProcedure  *procedure;
              GimpValueArray *return_vals;
              GimpImage      *temp_image;
              GimpLayer      *temp_layer;
              GFile          *temp_file = NULL;
              FILE           *temp_fp;

              temp_file  = gimp_temp_file ("png");

              /* TODO: Use GimpExportOptions for this when available */
              temp_image = gimp_image_new (width, height,
                                           gimp_image_get_base_type (image));
              if (gimp_image_get_base_type (image) == GIMP_INDEXED)
                gimp_image_set_palette (temp_image,
                                        gimp_image_get_palette (image));

              temp_layer = gimp_layer_new_from_drawable (GIMP_DRAWABLE (iter->data),
                                                         temp_image);
              gimp_image_insert_layer (temp_image, temp_layer, NULL, 0);

              if (include_color_profile &&
                  gimp_image_get_color_profile (image))
                {
                  GimpColorProfile *profile;

                  profile =  gimp_image_get_color_profile (image);
                  gimp_image_set_color_profile (temp_image, profile);
                }

              procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (), "file-png-export");
              return_vals = gimp_procedure_run (procedure,
                                                "run-mode",              GIMP_RUN_NONINTERACTIVE,
                                                "image",                 temp_image,
                                                "file",                  temp_file,
                                                "interlaced",            FALSE,
                                                "compression",           9,
                                                "bkgd",                  FALSE,
                                                "offs",                  FALSE,
                                                "phys",                  FALSE,
                                                "time",                  FALSE,
                                                "save-transparent",      FALSE,
                                                "optimize-palette",      FALSE,
                                                "include-color-profile", include_color_profile,
                                                NULL);
              gimp_image_delete (temp_image);

              if (GIMP_VALUES_GET_ENUM (return_vals, 0) != GIMP_PDB_SUCCESS)
                {
                  icns_save_info_free (info);
                  g_set_error (error, 0, 0,
                               "Running procedure 'file-png-export' "
                               "for icns export failed: %s",
                               gimp_pdb_get_last_error (gimp_get_pdb ()));

                  return GIMP_PDB_EXECUTION_ERROR;
                }

              temp_fp = g_fopen (g_file_peek_path (temp_file), "rb");
              fseek (temp_fp, 0L, SEEK_END);
              temp_size = ftell (temp_fp);
              fseek (temp_fp, 0L, SEEK_SET);

              g_file_delete (temp_file, NULL, NULL);
              g_object_unref (temp_file);

              fwrite (iconTypes[match].type, sizeof (gchar), 4, fp);
              macos_size = GUINT32_TO_BE (temp_size + 8);
              fwrite (&macos_size, sizeof (macos_size), 1, fp);

              if (temp_size > 0)
                {
                  guchar buf[temp_size];

                  fread (buf, 1, sizeof (buf), temp_fp);

                  if (fwrite (buf, 1, temp_size, fp) < temp_size)
                    {
                      icns_save_info_free (info);
                      g_set_error (error, G_FILE_ERROR,
                                   g_file_error_from_errno (errno),
                                   _("Error writing icns: %s"),
                                   g_strerror (errno));
                      return GIMP_PDB_EXECUTION_ERROR;
                    }
                }
              fclose (temp_fp);

              file_size += temp_size + 8;
          }
          duplicates[match] = 1;
        }

      gimp_progress_update (i / info->num_icons);
    }

  /* Update header with full file size */
  file_size = GUINT32_TO_BE (file_size);
  fseek (fp, 4L, SEEK_SET);
  fwrite (&file_size, sizeof (file_size), 1, fp);

  gimp_progress_update (1.0);

  icns_save_info_free (info);
  fclose (fp);
  return GIMP_PDB_SUCCESS;
}

static guchar *
icns_compress (guint   width,
               guint   height,
               guchar *rgba,
               gint   *out_size)
{
  const guint npixels  = width * height;
  const guint max_size = (npixels * 3) + ((npixels * 3) / 4);

  const guint min_run = 3;   /* Shorter run must be stored as uncompressed */
  const guint max_run = 130; /* Longest same-value run that can be stored */
  const guint min_raw = 1;
  const guint max_raw = 128; /* Longest run of non-matching pixels */

  guint   i;
  guint   j;
  guint   size;
  guint   channel;
  guint   run;
  guint   marker;
  guchar *out_data;
  guchar *run_length;

  run_length = g_new (guchar, npixels);
  if (! run_length)
    {
      g_warning ("icns_compress: couldn't allocate run count buffer (%d bytes)", npixels);
      return NULL;
    }

  out_data = g_new (guchar, max_size);
  if (! out_data)
    {
      g_free (run_length);
      return NULL;
    }

  size = 0;
  /* For some reason 128x128 icons have an extra 4 bytes at the start */
  if (width == 128 && height == 128)
    {
      out_data[size++] = 0;
      out_data[size++] = 0;
      out_data[size++] = 0;
      out_data[size++] = 0;
    }

  for (channel = 0; channel < 3; channel++)
    {
      /* Count all run lengths */
      for (i = 0; i < npixels; i++)
        {
          for (run = 1; run < max_run && (run + i - 1) < npixels; run++)
            if (rgba[i * 4 + channel] != rgba[(i + run) * 4 + channel])
              break;

          run_length[i] = run;
        }

      for (i = 0; i < npixels; i++)
        {
          if (run_length[i] >= min_run)
            {
              /* Compressable! Store and skip ahead */
              out_data[size++] = (run_length[i] - min_run) | 0x80;
              out_data[size++] = rgba[i * 4 + channel];
              i += run_length[i] - 1;
            }
          else
            {
              /* Too short: stuff together as many as you can in a raw run */
              marker = size++;
              run = 0;
              while (run < max_raw && i < npixels && run_length[i] < min_run)
                {
                  for (j = 0; j < run_length[i]; j++)
                    {
                      out_data[size++] = rgba[(i + j) * 4 + channel];
                      run++;
                    }
                  i += run_length[i];
                }
              out_data[marker] = run - min_raw;
              i--;
            }
        }
    }

  g_free (run_length);
  *out_size = size;

  return out_data;
}

static void
icns_save_info_free (IcnsSaveInfo *info)
{
  g_list_free (info->layers);
  memset (info, 0, sizeof (IcnsSaveInfo));
}

GimpPDBStatusType
icns_save_image (GFile                *file,
                 GimpImage            *image,
                 GimpProcedure        *procedure,
                 GimpProcedureConfig  *config,
                 gint32                run_mode,
                 GError              **error)
{
  IcnsSaveInfo  info;
  GList        *iter;
  gboolean      isValidLayers         = FALSE;
  gboolean      include_color_profile = FALSE;
  gint          i;

  info.layers    = gimp_image_list_layers (image);
  info.num_icons = g_list_length (info.layers);

  /* Initial check if we have any valid layers to export */
  for (iter = info.layers, i = 0; iter; iter = iter->next, i++)
    {
      gint width  = gimp_drawable_get_width (iter->data);
      gint height = gimp_drawable_get_height (iter->data);

      if (icns_check_dimensions (width, height))
        {
          isValidLayers = TRUE;
          break;
        }
    }
  if (! isValidLayers)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("No valid sized layers. Only valid layer sizes are "
                     "16x12, 16x16, 18x18, 24x24, 32x32, 36x36, 48x48, "
                     "64x64, 128x128, 256x256, 512x512, or 1024x1024."));

      return GIMP_PDB_EXECUTION_ERROR;
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      /* Allow user to override default values */
      if (! icns_save_dialog (&info, image, procedure, config))
        return GIMP_PDB_CANCEL;
    }
  else if (run_mode == GIMP_RUN_NONINTERACTIVE)
    {
      if (! icns_check_compat (NULL, &info))
        {
          g_set_error (error, G_FILE_ERROR, 0,
                       _("Invalid layer size(s). Only valid layer sizes are "
                         "16x12, 16x16, 18x18, 24x24, 32x32, 36x36, 48x48, "
                         "64x64, 128x128, 256x256, 512x512, or 1024x1024."));

          return GIMP_PDB_EXECUTION_ERROR;
        }
    }

  g_object_get (config,
                "include-color-profile", &include_color_profile,
                NULL);

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  return icns_export_image (file, &info, image, include_color_profile, error);
}
