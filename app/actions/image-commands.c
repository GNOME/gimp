/* LIGMA - The GNU Image Manipulation Program
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmadialogconfig.h"

#include "gegl/ligma-babl.h"

#include "core/core-enums.h"
#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-color-profile.h"
#include "core/ligmaimage-convert-indexed.h"
#include "core/ligmaimage-convert-precision.h"
#include "core/ligmaimage-convert-type.h"
#include "core/ligmaimage-crop.h"
#include "core/ligmaimage-duplicate.h"
#include "core/ligmaimage-flip.h"
#include "core/ligmaimage-merge.h"
#include "core/ligmaimage-resize.h"
#include "core/ligmaimage-rotate.h"
#include "core/ligmaimage-scale.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaitem.h"
#include "core/ligmapickable.h"
#include "core/ligmapickable-auto-shrink.h"
#include "core/ligmaprogress.h"

#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"

#include "dialogs/dialogs.h"
#include "dialogs/color-profile-dialog.h"
#include "dialogs/convert-indexed-dialog.h"
#include "dialogs/convert-precision-dialog.h"
#include "dialogs/grid-dialog.h"
#include "dialogs/image-merge-layers-dialog.h"
#include "dialogs/image-new-dialog.h"
#include "dialogs/image-properties-dialog.h"
#include "dialogs/image-scale-dialog.h"
#include "dialogs/print-size-dialog.h"
#include "dialogs/resize-dialog.h"

#include "actions.h"
#include "image-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   image_convert_rgb_callback       (GtkWidget                *dialog,
                                                LigmaImage                *image,
                                                LigmaColorProfile         *new_profile,
                                                GFile                    *new_file,
                                                LigmaColorRenderingIntent  intent,
                                                gboolean                  bpc,
                                                gpointer                  user_data);

static void   image_convert_gray_callback      (GtkWidget                *dialog,
                                                LigmaImage                *image,
                                                LigmaColorProfile         *new_profile,
                                                GFile                    *new_file,
                                                LigmaColorRenderingIntent  intent,
                                                gboolean                  bpc,
                                                gpointer                  user_data);

static void   image_convert_indexed_callback   (GtkWidget              *dialog,
                                                LigmaImage              *image,
                                                LigmaConvertPaletteType  palette_type,
                                                gint                    max_colors,
                                                gboolean                remove_duplicates,
                                                LigmaConvertDitherType   dither_type,
                                                gboolean                dither_alpha,
                                                gboolean                dither_text_layers,
                                                LigmaPalette            *custom_palette,
                                                gpointer                user_data);

static void   image_convert_precision_callback (GtkWidget              *dialog,
                                                LigmaImage              *image,
                                                LigmaPrecision           precision,
                                                GeglDitherMethod        layer_dither_method,
                                                GeglDitherMethod        text_layer_dither_method,
                                                GeglDitherMethod        mask_dither_method,
                                                gpointer                user_data);

static void   image_profile_assign_callback    (GtkWidget                *dialog,
                                                LigmaImage                *image,
                                                LigmaColorProfile         *new_profile,
                                                GFile                    *new_file,
                                                LigmaColorRenderingIntent  intent,
                                                gboolean                  bpc,
                                                gpointer                  user_data);

static void   image_profile_convert_callback   (GtkWidget                *dialog,
                                                LigmaImage                *image,
                                                LigmaColorProfile         *new_profile,
                                                GFile                    *new_file,
                                                LigmaColorRenderingIntent  intent,
                                                gboolean                  bpc,
                                                gpointer                  user_data);

static void   image_resize_callback            (GtkWidget              *dialog,
                                                LigmaViewable           *viewable,
                                                LigmaContext            *context,
                                                gint                    width,
                                                gint                    height,
                                                LigmaUnit                unit,
                                                gint                    offset_x,
                                                gint                    offset_y,
                                                gdouble                 xres,
                                                gdouble                 yres,
                                                LigmaUnit                res_unit,
                                                LigmaFillType            fill_type,
                                                LigmaItemSet             layer_set,
                                                gboolean                resize_text_layers,
                                                gpointer                user_data);

static void   image_print_size_callback        (GtkWidget              *dialog,
                                                LigmaImage              *image,
                                                gdouble                 xresolution,
                                                gdouble                 yresolution,
                                                LigmaUnit                resolution_unit,
                                                gpointer                user_data);

static void   image_scale_callback             (GtkWidget              *dialog,
                                                LigmaViewable           *viewable,
                                                gint                    width,
                                                gint                    height,
                                                LigmaUnit                unit,
                                                LigmaInterpolationType   interpolation,
                                                gdouble                 xresolution,
                                                gdouble                 yresolution,
                                                LigmaUnit                resolution_unit,
                                                gpointer                user_data);

static void   image_merge_layers_callback      (GtkWidget              *dialog,
                                                LigmaImage              *image,
                                                LigmaContext            *context,
                                                LigmaMergeType           merge_type,
                                                gboolean                merge_active_group,
                                                gboolean                discard_invisible,
                                                gpointer                user_data);

static void   image_softproof_profile_callback  (GtkWidget                *dialog,
                                                 LigmaImage                *image,
                                                 LigmaColorProfile         *new_profile,
                                                 GFile                    *new_file,
                                                 LigmaColorRenderingIntent  intent,
                                                 gboolean                  bpc,
                                                 gpointer                  user_data);



/*  private variables  */

static LigmaUnit               image_resize_unit  = LIGMA_UNIT_PIXEL;
static LigmaUnit               image_scale_unit   = LIGMA_UNIT_PIXEL;
static LigmaInterpolationType  image_scale_interp = -1;
static LigmaPalette           *image_convert_indexed_custom_palette = NULL;


/*  public functions  */

void
image_new_cmd_callback (LigmaAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_widget (widget, data);

  dialog = ligma_dialog_factory_dialog_new (ligma_dialog_factory_get_singleton (),
                                           ligma_widget_get_monitor (widget),
                                           NULL /*ui_manager*/,
                                           widget,
                                           "ligma-image-new-dialog", -1, FALSE);

  if (dialog)
    {
      LigmaImage *image = action_data_get_image (data);

      image_new_dialog_set (dialog, image, NULL);

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

void
image_duplicate_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaDisplay      *display;
  LigmaImage        *image;
  LigmaDisplayShell *shell;
  LigmaImage        *new_image;
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);
  shell = ligma_display_get_shell (display);

  new_image = ligma_image_duplicate (image);

  ligma_create_display (new_image->ligma, new_image, shell->unit,
                       ligma_zoom_model_get_factor (shell->zoom),
                       G_OBJECT (ligma_widget_get_monitor (GTK_WIDGET (shell))));

  g_object_unref (new_image);
}

void
image_convert_base_type_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaImage         *image;
  LigmaDisplay       *display;
  GtkWidget         *widget;
  LigmaDialogConfig  *config;
  GtkWidget         *dialog;
  LigmaImageBaseType  base_type;
  GError            *error = NULL;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  base_type = (LigmaImageBaseType) g_variant_get_int32 (value);

  if (base_type == ligma_image_get_base_type (image))
    return;

#define CONVERT_TYPE_DIALOG_KEY "ligma-convert-type-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), CONVERT_TYPE_DIALOG_KEY);

  if (dialog)
    {
      gtk_widget_destroy (dialog);
      dialog = NULL;
    }

  config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  switch (base_type)
    {
    case LIGMA_RGB:
    case LIGMA_GRAY:
      if (ligma_image_get_color_profile (image))
        {
          ColorProfileDialogType    dialog_type;
          LigmaColorProfileCallback  callback;
          LigmaColorProfile         *current_profile;
          LigmaColorProfile         *default_profile;
          LigmaTRCType               trc;

          current_profile =
            ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));

          trc = ligma_babl_trc (ligma_image_get_precision (image));

          if (base_type == LIGMA_RGB)
            {
              dialog_type = COLOR_PROFILE_DIALOG_CONVERT_TO_RGB;
              callback    = image_convert_rgb_callback;

              default_profile = ligma_babl_get_builtin_color_profile (LIGMA_RGB,
                                                                     trc);
            }
          else
            {
              dialog_type = COLOR_PROFILE_DIALOG_CONVERT_TO_GRAY;
              callback    = image_convert_gray_callback;

              default_profile = ligma_babl_get_builtin_color_profile (LIGMA_GRAY,
                                                                     trc);
            }

          dialog = color_profile_dialog_new (dialog_type,
                                             image,
                                             action_data_get_context (data),
                                             widget,
                                             current_profile,
                                             default_profile,
                                             0, 0,
                                             callback,
                                             display);
        }
      else if (! ligma_image_convert_type (image, base_type, NULL, NULL, &error))
        {
          ligma_message_literal (image->ligma,
                                G_OBJECT (widget), LIGMA_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
        }
      break;

    case LIGMA_INDEXED:
      dialog = convert_indexed_dialog_new (image,
                                           action_data_get_context (data),
                                           widget,
                                           config->image_convert_indexed_palette_type,
                                           config->image_convert_indexed_max_colors,
                                           config->image_convert_indexed_remove_duplicates,
                                           config->image_convert_indexed_dither_type,
                                           config->image_convert_indexed_dither_alpha,
                                           config->image_convert_indexed_dither_text_layers,
                                           image_convert_indexed_custom_palette,
                                           image_convert_indexed_callback,
                                           display);
      break;
    }

  if (dialog)
    {
      dialogs_attach_dialog (G_OBJECT (image),
                             CONVERT_TYPE_DIALOG_KEY, dialog);
      gtk_window_present (GTK_WINDOW (dialog));
    }

  /*  always flush, also when only the indexed dialog was shown, so
   *  the menu items get updated back to the current image type
   */
  ligma_image_flush (image);
}

void
image_convert_precision_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaImage         *image;
  LigmaDisplay       *display;
  GtkWidget         *widget;
  LigmaDialogConfig  *config;
  GtkWidget         *dialog;
  LigmaComponentType  component_type;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  component_type = (LigmaComponentType) g_variant_get_int32 (value);

  if (component_type == ligma_image_get_component_type (image))
    return;

#define CONVERT_PRECISION_DIALOG_KEY "ligma-convert-precision-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), CONVERT_PRECISION_DIALOG_KEY);

  if (dialog)
    {
      gtk_widget_destroy (dialog);
      dialog = NULL;
    }

  config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  dialog = convert_precision_dialog_new (image,
                                         action_data_get_context (data),
                                         widget,
                                         component_type,
                                         config->image_convert_precision_layer_dither_method,
                                         config->image_convert_precision_text_layer_dither_method,
                                         config->image_convert_precision_channel_dither_method,
                                         image_convert_precision_callback,
                                         display);

  dialogs_attach_dialog (G_OBJECT (image),
                         CONVERT_PRECISION_DIALOG_KEY, dialog);

  gtk_window_present (GTK_WINDOW (dialog));

  /*  see comment above  */
  ligma_image_flush (image);
}

void
image_convert_trc_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaImage     *image;
  LigmaDisplay   *display;
  LigmaTRCType    trc_type;
  LigmaPrecision  precision;
  return_if_no_image (image, data);
  return_if_no_display (display, data);

  trc_type = (LigmaTRCType) g_variant_get_int32 (value);

  if (trc_type == ligma_babl_format_get_trc (ligma_image_get_layer_format (image,
                                                                         FALSE)))
    return;

  precision = ligma_babl_precision (ligma_image_get_component_type (image),
                                   trc_type);

  ligma_image_convert_precision (image, precision,
                                GEGL_DITHER_NONE,
                                GEGL_DITHER_NONE,
                                GEGL_DITHER_NONE,
                                LIGMA_PROGRESS (display));
  ligma_image_flush (image);
}

void
image_color_profile_use_srgb_cmd_callback (LigmaAction *action,
                                           GVariant   *value,
                                           gpointer    data)
{
  LigmaImage *image;
  gboolean   use_srgb;
  return_if_no_image (image, data);

  use_srgb = g_variant_get_boolean (value);

  if (use_srgb != ligma_image_get_use_srgb_profile (image, NULL))
    {
      ligma_image_set_use_srgb_profile (image, use_srgb);
      ligma_image_flush (image);
    }
}

void
image_color_profile_assign_cmd_callback (LigmaAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  LigmaImage   *image;
  LigmaDisplay *display;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

#define PROFILE_ASSIGN_DIALOG_KEY "ligma-profile-assign-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PROFILE_ASSIGN_DIALOG_KEY);

  if (! dialog)
    {
      LigmaColorProfile *current_profile;
      LigmaColorProfile *default_profile;

      current_profile = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));
      default_profile = ligma_image_get_builtin_color_profile (image);

      dialog = color_profile_dialog_new (COLOR_PROFILE_DIALOG_ASSIGN_PROFILE,
                                         image,
                                         action_data_get_context (data),
                                         widget,
                                         current_profile,
                                         default_profile,
                                         0, 0,
                                         image_profile_assign_callback,
                                         display);

      dialogs_attach_dialog (G_OBJECT (image),
                             PROFILE_ASSIGN_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_color_profile_convert_cmd_callback (LigmaAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  LigmaImage   *image;
  LigmaDisplay *display;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

#define PROFILE_CONVERT_DIALOG_KEY "ligma-profile-convert-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PROFILE_CONVERT_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
      LigmaColorProfile *current_profile;
      LigmaColorProfile *default_profile;

      current_profile = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));
      default_profile = ligma_image_get_builtin_color_profile (image);

      dialog = color_profile_dialog_new (COLOR_PROFILE_DIALOG_CONVERT_TO_PROFILE,
                                         image,
                                         action_data_get_context (data),
                                         widget,
                                         current_profile,
                                         default_profile,
                                         config->image_convert_profile_intent,
                                         config->image_convert_profile_bpc,
                                         image_profile_convert_callback,
                                         display);

      dialogs_attach_dialog (G_OBJECT (image),
                             PROFILE_CONVERT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_color_profile_discard_cmd_callback (LigmaAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  LigmaImage *image;
  return_if_no_image (image, data);

  ligma_image_assign_color_profile (image, NULL, NULL, NULL);
  ligma_image_flush (image);
}

static void
image_profile_save_dialog_response (GtkWidget *dialog,
                                    gint       response_id,
                                    LigmaImage *image)
{
  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      LigmaColorProfile *profile;
      GFile            *file;
      GError           *error = NULL;

      profile = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));
      file    = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

      if (! file)
        return;

      if (! ligma_color_profile_save_to_file (profile, file, &error))
        {
          ligma_message (image->ligma, NULL,
                        LIGMA_MESSAGE_WARNING,
                        _("Saving color profile failed: %s"),
                        error->message);
          g_clear_error (&error);
          g_object_unref (file);
          return;
        }

      g_object_unref (file);
    }

  gtk_widget_destroy (dialog);
}

void
image_color_profile_save_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaImage   *image;
  LigmaDisplay *display;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

#define PROFILE_SAVE_DIALOG_KEY "ligma-profile-save-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PROFILE_SAVE_DIALOG_KEY);

  if (! dialog)
    {
      GtkWindow        *toplevel;
      LigmaColorProfile *profile;
      gchar            *basename;

      toplevel = GTK_WINDOW (gtk_widget_get_toplevel (widget));
      profile  = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));

      dialog =
        ligma_color_profile_chooser_dialog_new (_("Save Color Profile"),
                                               toplevel,
                                               GTK_FILE_CHOOSER_ACTION_SAVE);

      ligma_color_profile_chooser_dialog_connect_path (dialog,
                                                      G_OBJECT (image->ligma->config),
                                                      "color-profile-path");

      basename = g_strconcat (ligma_color_profile_get_label (profile),
                              ".icc", NULL);
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), basename);
      g_free (basename);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (image_profile_save_dialog_response),
                        image);

      dialogs_attach_dialog (G_OBJECT (image), PROFILE_SAVE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_resize_cmd_callback (LigmaAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  LigmaImage   *image;
  GtkWidget   *widget;
  LigmaDisplay *display;
  GtkWidget   *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);
  return_if_no_display (display, data);

#define RESIZE_DIALOG_KEY "ligma-resize-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), RESIZE_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);

      if (image_resize_unit != LIGMA_UNIT_PERCENT)
        image_resize_unit = ligma_display_get_shell (display)->unit;

      dialog = resize_dialog_new (LIGMA_VIEWABLE (image),
                                  action_data_get_context (data),
                                  _("Set Image Canvas Size"),
                                  "ligma-image-resize",
                                  widget,
                                  ligma_standard_help_func,
                                  LIGMA_HELP_IMAGE_RESIZE,
                                  image_resize_unit,
                                  config->image_resize_fill_type,
                                  config->image_resize_layer_set,
                                  config->image_resize_resize_text_layers,
                                  image_resize_callback,
                                  display);

      dialogs_attach_dialog (G_OBJECT (image), RESIZE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_resize_to_layers_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaDisplay  *display;
  LigmaImage    *image;
  LigmaProgress *progress;
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);

  progress = ligma_progress_start (LIGMA_PROGRESS (display), FALSE,
                                  _("Resizing"));

  ligma_image_resize_to_layers (image,
                               action_data_get_context (data),
                               NULL, NULL, NULL, NULL, progress);

  if (progress)
    ligma_progress_end (progress);

  ligma_image_flush (image);
}

void
image_resize_to_selection_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  LigmaDisplay  *display;
  LigmaImage    *image;
  LigmaProgress *progress;
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);

  progress = ligma_progress_start (LIGMA_PROGRESS (display), FALSE,
                                  _("Resizing"));

  ligma_image_resize_to_selection (image,
                                  action_data_get_context (data),
                                  progress);

  if (progress)
    ligma_progress_end (progress);

  ligma_image_flush (image);
}

void
image_print_size_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaDisplay *display;
  LigmaImage   *image;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  image = ligma_display_get_image (display);

#define PRINT_SIZE_DIALOG_KEY "ligma-print-size-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PRINT_SIZE_DIALOG_KEY);

  if (! dialog)
    {
      dialog = print_size_dialog_new (image,
                                      action_data_get_context (data),
                                      _("Set Image Print Resolution"),
                                      "ligma-image-print-size",
                                      widget,
                                      ligma_standard_help_func,
                                      LIGMA_HELP_IMAGE_PRINT_SIZE,
                                      image_print_size_callback,
                                      NULL);

      dialogs_attach_dialog (G_OBJECT (image), PRINT_SIZE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_scale_cmd_callback (LigmaAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  LigmaDisplay *display;
  LigmaImage   *image;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  image = ligma_display_get_image (display);

#define SCALE_DIALOG_KEY "ligma-scale-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), SCALE_DIALOG_KEY);

  if (! dialog)
    {
      if (image_scale_unit != LIGMA_UNIT_PERCENT)
        image_scale_unit = ligma_display_get_shell (display)->unit;

      if (image_scale_interp == -1)
        image_scale_interp = display->ligma->config->interpolation_type;

      dialog = image_scale_dialog_new (image,
                                       action_data_get_context (data),
                                       widget,
                                       image_scale_unit,
                                       image_scale_interp,
                                       image_scale_callback,
                                       display);

      dialogs_attach_dialog (G_OBJECT (image), SCALE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_flip_cmd_callback (LigmaAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  LigmaDisplay         *display;
  LigmaImage           *image;
  LigmaProgress        *progress;
  LigmaOrientationType  orientation;
  return_if_no_display (display, data);

  orientation = (LigmaOrientationType) g_variant_get_int32 (value);

  image = ligma_display_get_image (display);

  progress = ligma_progress_start (LIGMA_PROGRESS (display), FALSE,
                                  _("Flipping"));

  ligma_image_flip (image, action_data_get_context (data),
                   orientation, progress);

  if (progress)
    ligma_progress_end (progress);

  ligma_image_flush (image);
}

void
image_rotate_cmd_callback (LigmaAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  LigmaDisplay      *display;
  LigmaImage        *image;
  LigmaProgress     *progress;
  LigmaRotationType  rotation;
  return_if_no_display (display, data);

  rotation = (LigmaRotationType) g_variant_get_int32 (value);

  image = ligma_display_get_image (display);

  progress = ligma_progress_start (LIGMA_PROGRESS (display), FALSE,
                                  _("Rotating"));

  ligma_image_rotate (image, action_data_get_context (data),
                     rotation, progress);

  if (progress)
    ligma_progress_end (progress);

  ligma_image_flush (image);
}

void
image_crop_to_selection_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaImage *image;
  GtkWidget *widget;
  gint       x, y;
  gint       width, height;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  if (! ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                          &x, &y, &width, &height))
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_WARNING,
                            _("Cannot crop because the current selection "
                              "is empty."));
      return;
    }

  ligma_image_crop (image,
                   action_data_get_context (data), LIGMA_FILL_TRANSPARENT,
                   x, y, width, height, TRUE);
  ligma_image_flush (image);
}

void
image_crop_to_content_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaImage *image;
  GtkWidget *widget;
  gint       x, y;
  gint       width, height;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  switch (ligma_pickable_auto_shrink (LIGMA_PICKABLE (image),
                                     0, 0,
                                     ligma_image_get_width  (image),
                                     ligma_image_get_height (image),
                                     &x, &y, &width, &height))
    {
    case LIGMA_AUTO_SHRINK_SHRINK:
      ligma_image_crop (image,
                       action_data_get_context (data), LIGMA_FILL_TRANSPARENT,
                       x, y, width, height, TRUE);
      ligma_image_flush (image);
      break;

    case LIGMA_AUTO_SHRINK_EMPTY:
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_INFO,
                            _("Cannot crop because the image has no content."));
      break;

    case LIGMA_AUTO_SHRINK_UNSHRINKABLE:
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_INFO,
                            _("Cannot crop because the image is already "
                              "cropped to its content."));
      break;
    }
}

void
image_merge_layers_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GtkWidget   *dialog;
  LigmaImage   *image;
  LigmaDisplay *display;
  GtkWidget   *widget;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

#define MERGE_LAYERS_DIALOG_KEY "ligma-merge-layers-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), MERGE_LAYERS_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);

      dialog = image_merge_layers_dialog_new (image,
                                              action_data_get_context (data),
                                              widget,
                                              config->layer_merge_type,
                                              config->layer_merge_active_group_only,
                                              config->layer_merge_discard_invisible,
                                              image_merge_layers_callback,
                                              display);

      dialogs_attach_dialog (G_OBJECT (image), MERGE_LAYERS_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_merge_layers_last_vals_cmd_callback (LigmaAction *action,
                                           GVariant   *value,
                                           gpointer    data)
{
  LigmaImage        *image;
  LigmaDisplay      *display;
  LigmaDialogConfig *config;
  return_if_no_image (image, data);
  return_if_no_display (display, data);

  config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  image_merge_layers_callback (NULL,
                               image,
                               action_data_get_context (data),
                               config->layer_merge_type,
                               config->layer_merge_active_group_only,
                               config->layer_merge_discard_invisible,
                               display);
}

void
image_flatten_image_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaImage   *image;
  LigmaDisplay *display;
  GtkWidget   *widget;
  GError      *error = NULL;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  if (! ligma_image_flatten (image, action_data_get_context (data),
                            LIGMA_PROGRESS (display), &error))
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
      return;
    }

  ligma_image_flush (image);
}

void
image_configure_grid_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaDisplay *display;
  LigmaImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);

#define GRID_DIALOG_KEY "ligma-grid-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), GRID_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDisplayShell *shell = ligma_display_get_shell (display);

      dialog = grid_dialog_new (image,
                                action_data_get_context (data),
                                gtk_widget_get_toplevel (GTK_WIDGET (shell)));

      dialogs_attach_dialog (G_OBJECT (image), GRID_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_properties_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaDisplay *display;
  LigmaImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);

#define PROPERTIES_DIALOG_KEY "ligma-image-properties-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PROPERTIES_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDisplayShell *shell = ligma_display_get_shell (display);

      dialog = image_properties_dialog_new (image,
                                            action_data_get_context (data),
                                            gtk_widget_get_toplevel (GTK_WIDGET (shell)));

      dialogs_attach_dialog (G_OBJECT (image), PROPERTIES_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}


/*  private functions  */

static void
image_convert_rgb_callback (GtkWidget                *dialog,
                            LigmaImage                *image,
                            LigmaColorProfile         *new_profile,
                            GFile                    *new_file,
                            LigmaColorRenderingIntent  intent,
                            gboolean                  bpc,
                            gpointer                  user_data)
{
  LigmaProgress *progress = user_data;
  GError       *error    = NULL;

  progress = ligma_progress_start (progress, FALSE,
                                  _("Converting to RGB (%s)"),
                                  ligma_color_profile_get_label (new_profile));

  if (! ligma_image_convert_type (image, LIGMA_RGB, new_profile,
                                 progress, &error))
    {
      ligma_message (image->ligma, G_OBJECT (dialog),
                    LIGMA_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);

      if (progress)
        ligma_progress_end (progress);

      return;
    }

  if (progress)
    ligma_progress_end (progress);

  ligma_image_flush (image);

 gtk_widget_destroy (dialog);
}

static void
image_convert_gray_callback (GtkWidget                *dialog,
                             LigmaImage                *image,
                             LigmaColorProfile         *new_profile,
                             GFile                    *new_file,
                             LigmaColorRenderingIntent  intent,
                             gboolean                  bpc,
                             gpointer                  user_data)
{
  LigmaProgress *progress = user_data;
  GError       *error    = NULL;

  progress = ligma_progress_start (progress, FALSE,
                                  _("Converting to grayscale (%s)"),
                                  ligma_color_profile_get_label (new_profile));

  if (! ligma_image_convert_type (image, LIGMA_GRAY, new_profile,
                                 progress, &error))
    {
      ligma_message (image->ligma, G_OBJECT (dialog),
                    LIGMA_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);

      if (progress)
        ligma_progress_end (progress);

      return;
    }

  if (progress)
    ligma_progress_end (progress);

  ligma_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_convert_indexed_callback (GtkWidget              *dialog,
                                LigmaImage              *image,
                                LigmaConvertPaletteType  palette_type,
                                gint                    max_colors,
                                gboolean                remove_duplicates,
                                LigmaConvertDitherType   dither_type,
                                gboolean                dither_alpha,
                                gboolean                dither_text_layers,
                                LigmaPalette            *custom_palette,
                                gpointer                user_data)
{
  LigmaDialogConfig *config  = LIGMA_DIALOG_CONFIG (image->ligma->config);
  LigmaDisplay      *display = user_data;
  LigmaProgress     *progress;
  GError           *error   = NULL;

  g_object_set (config,
                "image-convert-indexed-palette-type",       palette_type,
                "image-convert-indexed-max-colors",         max_colors,
                "image-convert-indexed-remove-duplicates",  remove_duplicates,
                "image-convert-indexed-dither-type",        dither_type,
                "image-convert-indexed-dither-alpha",       dither_alpha,
                "image-convert-indexed-dither-text-layers", dither_text_layers,
                NULL);

  if (image_convert_indexed_custom_palette)
    g_object_remove_weak_pointer (G_OBJECT (image_convert_indexed_custom_palette),
                                  (gpointer) &image_convert_indexed_custom_palette);

  image_convert_indexed_custom_palette = custom_palette;

  if (image_convert_indexed_custom_palette)
    g_object_add_weak_pointer (G_OBJECT (image_convert_indexed_custom_palette),
                               (gpointer) &image_convert_indexed_custom_palette);

  progress = ligma_progress_start (LIGMA_PROGRESS (display), FALSE,
                                  _("Converting to indexed colors"));

  if (! ligma_image_convert_indexed (image,
                                    config->image_convert_indexed_palette_type,
                                    config->image_convert_indexed_max_colors,
                                    config->image_convert_indexed_remove_duplicates,
                                    config->image_convert_indexed_dither_type,
                                    config->image_convert_indexed_dither_alpha,
                                    config->image_convert_indexed_dither_text_layers,
                                    image_convert_indexed_custom_palette,
                                    progress,
                                    &error))
    {
      ligma_message_literal (image->ligma, G_OBJECT (display),
                            LIGMA_MESSAGE_WARNING, error->message);
      g_clear_error (&error);

      if (progress)
        ligma_progress_end (progress);

      return;
    }

  if (progress)
    ligma_progress_end (progress);

  ligma_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_convert_precision_callback (GtkWidget        *dialog,
                                  LigmaImage        *image,
                                  LigmaPrecision     precision,
                                  GeglDitherMethod  layer_dither_method,
                                  GeglDitherMethod  text_layer_dither_method,
                                  GeglDitherMethod  channel_dither_method,
                                  gpointer          user_data)
{
  LigmaDialogConfig *config   = LIGMA_DIALOG_CONFIG (image->ligma->config);
  LigmaProgress     *progress = user_data;
  const gchar      *enum_desc;
  const Babl       *old_format;
  const Babl       *new_format;
  gint              old_bits;
  gint              new_bits;

  g_object_set (config,
                "image-convert-precision-layer-dither-method",
                layer_dither_method,
                "image-convert-precision-text-layer-dither-method",
                text_layer_dither_method,
                "image-convert-precision-channel-dither-method",
                channel_dither_method,
                NULL);

  /*  we do the same dither method checks here *and* in the dialog,
   *  because the dialog leaves the passed dither methods untouched if
   *  dithering is disabled and passes the original values to the
   *  callback, in order not to change the values saved in
   *  LigmaDialogConfig.
   */

  /* random formats with the right precision */
  old_format = ligma_image_get_layer_format (image, FALSE);
  new_format = ligma_babl_format (LIGMA_RGB, precision, FALSE, NULL);

  old_bits = (babl_format_get_bytes_per_pixel (old_format) * 8 /
              babl_format_get_n_components (old_format));
  new_bits = (babl_format_get_bytes_per_pixel (new_format) * 8 /
              babl_format_get_n_components (new_format));

  if (new_bits >= old_bits ||
      new_bits >  CONVERT_PRECISION_DIALOG_MAX_DITHER_BITS)
    {
      /*  don't dither if we are converting to a higher bit depth,
       *  or to more than MAX_DITHER_BITS.
       */
      layer_dither_method      = GEGL_DITHER_NONE;
      text_layer_dither_method = GEGL_DITHER_NONE;
      channel_dither_method    = GEGL_DITHER_NONE;
    }

  ligma_enum_get_value (LIGMA_TYPE_PRECISION, precision,
                       NULL, NULL, &enum_desc, NULL);

  progress = ligma_progress_start (progress, FALSE,
                                  _("Converting image to %s"),
                                  enum_desc);

  ligma_image_convert_precision (image,
                                precision,
                                layer_dither_method,
                                text_layer_dither_method,
                                channel_dither_method,
                                progress);

  if (progress)
    ligma_progress_end (progress);

  ligma_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_profile_assign_callback (GtkWidget                *dialog,
                               LigmaImage                *image,
                               LigmaColorProfile         *new_profile,
                               GFile                    *new_file,
                               LigmaColorRenderingIntent  intent,
                               gboolean                  bpc,
                               gpointer                  user_data)
{
  GError *error = NULL;

  if (! ligma_image_assign_color_profile (image, new_profile, NULL, &error))
    {
      ligma_message (image->ligma, G_OBJECT (dialog),
                    LIGMA_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);

      return;
    }

  ligma_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_profile_convert_callback (GtkWidget                *dialog,
                                LigmaImage                *image,
                                LigmaColorProfile         *new_profile,
                                GFile                    *new_file,
                                LigmaColorRenderingIntent  intent,
                                gboolean                  bpc,
                                gpointer                  user_data)
{
  LigmaDialogConfig *config   = LIGMA_DIALOG_CONFIG (image->ligma->config);
  LigmaProgress     *progress = user_data;
  GError           *error    = NULL;

  g_object_set (config,
                "image-convert-profile-intent",                   intent,
                "image-convert-profile-black-point-compensation", bpc,
                NULL);

  progress = ligma_progress_start (progress, FALSE,
                                  _("Converting to '%s'"),
                                  ligma_color_profile_get_label (new_profile));

  if (! ligma_image_convert_color_profile (image, new_profile,
                                          config->image_convert_profile_intent,
                                          config->image_convert_profile_bpc,
                                          progress, &error))
    {
      ligma_message (image->ligma, G_OBJECT (dialog),
                    LIGMA_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);

      if (progress)
        ligma_progress_end (progress);

      return;
    }

  if (progress)
    ligma_progress_end (progress);

  ligma_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_resize_callback (GtkWidget    *dialog,
                       LigmaViewable *viewable,
                       LigmaContext  *context,
                       gint          width,
                       gint          height,
                       LigmaUnit      unit,
                       gint          offset_x,
                       gint          offset_y,
                       gdouble       xres,
                       gdouble       yres,
                       LigmaUnit      res_unit,
                       LigmaFillType  fill_type,
                       LigmaItemSet   layer_set,
                       gboolean      resize_text_layers,
                       gpointer      user_data)
{
  LigmaDisplay *display = user_data;

  image_resize_unit = unit;

  if (width > 0 && height > 0)
    {
      LigmaImage        *image  = LIGMA_IMAGE (viewable);
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
      LigmaProgress     *progress;
      gdouble           old_xres;
      gdouble           old_yres;
      LigmaUnit          old_res_unit;
      gboolean          update_resolution;

      g_object_set (config,
                    "image-resize-fill-type",          fill_type,
                    "image-resize-layer-set",          layer_set,
                    "image-resize-resize-text-layers", resize_text_layers,
                    NULL);

      gtk_widget_destroy (dialog);

      if (width  == ligma_image_get_width  (image) &&
          height == ligma_image_get_height (image))
        return;

      progress = ligma_progress_start (LIGMA_PROGRESS (display), FALSE,
                                      _("Resizing"));

      ligma_image_get_resolution (image, &old_xres, &old_yres);
      old_res_unit = ligma_image_get_unit (image);

      update_resolution = xres     != old_xres ||
                          yres     != old_yres ||
                          res_unit != old_res_unit;

      if (update_resolution)
        {
          ligma_image_undo_group_start (image,
                                       LIGMA_UNDO_GROUP_IMAGE_SCALE,
                                       _("Change Canvas Size"));
          ligma_image_set_resolution (image, xres, yres);
          ligma_image_set_unit (image, res_unit);
        }

      ligma_image_resize_with_layers (image,
                                     context, fill_type,
                                     width, height,
                                     offset_x, offset_y,
                                     layer_set,
                                     resize_text_layers,
                                     progress);

      if (progress)
        ligma_progress_end (progress);

      if (update_resolution)
        ligma_image_undo_group_end (image);

      ligma_image_flush (image);
    }
  else
    {
      g_warning ("Resize Error: "
                 "Both width and height must be greater than zero.");
    }
}

static void
image_print_size_callback (GtkWidget *dialog,
                           LigmaImage *image,
                           gdouble    xresolution,
                           gdouble    yresolution,
                           LigmaUnit   resolution_unit,
                           gpointer   data)
{
  gdouble xres;
  gdouble yres;

  gtk_widget_destroy (dialog);

  ligma_image_get_resolution (image, &xres, &yres);

  if (xresolution     == xres &&
      yresolution     == yres &&
      resolution_unit == ligma_image_get_unit (image))
    return;

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_SCALE,
                               _("Change Print Size"));

  ligma_image_set_resolution (image, xresolution, yresolution);
  ligma_image_set_unit (image, resolution_unit);

  ligma_image_undo_group_end (image);

  ligma_image_flush (image);
}

static void
image_scale_callback (GtkWidget              *dialog,
                      LigmaViewable           *viewable,
                      gint                    width,
                      gint                    height,
                      LigmaUnit                unit,
                      LigmaInterpolationType   interpolation,
                      gdouble                 xresolution,
                      gdouble                 yresolution,
                      LigmaUnit                resolution_unit,
                      gpointer                user_data)
{
  LigmaProgress *progress = user_data;
  LigmaImage    *image    = LIGMA_IMAGE (viewable);
  gdouble       xres;
  gdouble       yres;

  image_scale_unit   = unit;
  image_scale_interp = interpolation;

  ligma_image_get_resolution (image, &xres, &yres);

  if (width > 0 && height > 0)
    {
      gtk_widget_destroy (dialog);

      if (width           == ligma_image_get_width  (image) &&
          height          == ligma_image_get_height (image) &&
          xresolution     == xres                          &&
          yresolution     == yres                          &&
          resolution_unit == ligma_image_get_unit (image))
        return;

      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_SCALE,
                                   _("Scale Image"));

      ligma_image_set_resolution (image, xresolution, yresolution);
      ligma_image_set_unit (image, resolution_unit);

      if (width  != ligma_image_get_width  (image) ||
          height != ligma_image_get_height (image))
        {
          progress = ligma_progress_start (progress, FALSE,
                                          _("Scaling"));

          ligma_image_scale (image, width, height, interpolation, progress);

          if (progress)
            ligma_progress_end (progress);
        }

      ligma_image_undo_group_end (image);

      ligma_image_flush (image);
    }
  else
    {
      g_warning ("Scale Error: "
                 "Both width and height must be greater than zero.");
    }
}

static void
image_merge_layers_callback (GtkWidget     *dialog,
                             LigmaImage     *image,
                             LigmaContext   *context,
                             LigmaMergeType  merge_type,
                             gboolean       merge_active_group,
                             gboolean       discard_invisible,
                             gpointer       user_data)
{
  LigmaDialogConfig *config  = LIGMA_DIALOG_CONFIG (image->ligma->config);
  LigmaDisplay      *display = user_data;

  g_object_set (config,
                "layer-merge-type",              merge_type,
                "layer-merge-active-group-only", merge_active_group,
                "layer-merge-discard-invisible", discard_invisible,
                NULL);

  ligma_image_merge_visible_layers (image,
                                   context,
                                   config->layer_merge_type,
                                   config->layer_merge_active_group_only,
                                   config->layer_merge_discard_invisible,
                                   LIGMA_PROGRESS (display));

  ligma_image_flush (image);

  g_clear_pointer (&dialog, gtk_widget_destroy);
}

void
image_softproof_profile_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaImage        *image;
  LigmaDisplayShell *shell;
  GtkWidget        *dialog;
  return_if_no_image (image, data);
  return_if_no_shell (shell, data);

#define SOFTPROOF_PROFILE_DIALOG_KEY "ligma-softproof-profile-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (shell), SOFTPROOF_PROFILE_DIALOG_KEY);

  if (! dialog)
    {
      LigmaColorProfile *current_profile;

      current_profile = ligma_image_get_simulation_profile (image);

      dialog = color_profile_dialog_new (COLOR_PROFILE_DIALOG_SELECT_SOFTPROOF_PROFILE,
                                         image,
                                         action_data_get_context (data),
                                         GTK_WIDGET (shell),
                                         current_profile,
                                         NULL,
                                         0, 0,
                                         image_softproof_profile_callback,
                                         shell);

      dialogs_attach_dialog (G_OBJECT (shell),
                             SOFTPROOF_PROFILE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
image_softproof_profile_callback (GtkWidget                *dialog,
                                  LigmaImage                *image,
                                  LigmaColorProfile         *new_profile,
                                  GFile                    *new_file,
                                  LigmaColorRenderingIntent  intent,
                                  gboolean                  bpc,
                                  gpointer                  user_data)
{
  LigmaDisplayShell *shell = user_data;

  /* Update image's simulation profile */
  ligma_image_set_simulation_profile (image, new_profile);
  ligma_color_managed_simulation_profile_changed (LIGMA_COLOR_MANAGED (shell));

  gtk_widget_destroy (dialog);
}

void
image_softproof_intent_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaImage                 *image;
  LigmaDisplayShell          *shell;
  LigmaColorRenderingIntent   intent;
  return_if_no_image (image, data);
  return_if_no_shell (shell, data);

  intent = (LigmaColorRenderingIntent) g_variant_get_int32 (value);

  if (intent != ligma_image_get_simulation_intent (image))
    {
      ligma_image_set_simulation_intent (image, intent);
      shell->color_config_set = TRUE;
      ligma_color_managed_simulation_intent_changed (LIGMA_COLOR_MANAGED (shell));
    }
}

void
image_softproof_bpc_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaImage                 *image;
  LigmaDisplayShell          *shell;
  gboolean                   bpc;
  return_if_no_image (image, data);
  return_if_no_shell (shell, data);

  bpc = g_variant_get_boolean (value);

  if (bpc != ligma_image_get_simulation_bpc (image))
    {
      ligma_image_set_simulation_bpc (image, bpc);
      shell->color_config_set = TRUE;
      ligma_color_managed_simulation_bpc_changed (LIGMA_COLOR_MANAGED (shell));
    }
}
