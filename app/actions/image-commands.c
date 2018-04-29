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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpdialogconfig.h"

#include "gegl/gimp-babl.h"

#include "core/core-enums.h"
#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-color-profile.h"
#include "core/gimpimage-convert-indexed.h"
#include "core/gimpimage-convert-precision.h"
#include "core/gimpimage-convert-type.h"
#include "core/gimpimage-crop.h"
#include "core/gimpimage-duplicate.h"
#include "core/gimpimage-flip.h"
#include "core/gimpimage-merge.h"
#include "core/gimpimage-resize.h"
#include "core/gimpimage-rotate.h"
#include "core/gimpimage-scale.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitem.h"
#include "core/gimppickable.h"
#include "core/gimppickable-auto-shrink.h"
#include "core/gimpprogress.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

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

#include "gimp-intl.h"


/*  local function prototypes  */

static void   image_convert_rgb_callback       (GtkWidget                *dialog,
                                                GimpImage                *image,
                                                GimpColorProfile         *new_profile,
                                                GFile                    *new_file,
                                                GimpColorRenderingIntent  intent,
                                                gboolean                  bpc,
                                                gpointer                  user_data);

static void   image_convert_gray_callback      (GtkWidget                *dialog,
                                                GimpImage                *image,
                                                GimpColorProfile         *new_profile,
                                                GFile                    *new_file,
                                                GimpColorRenderingIntent  intent,
                                                gboolean                  bpc,
                                                gpointer                  user_data);

static void   image_convert_indexed_callback   (GtkWidget              *dialog,
                                                GimpImage              *image,
                                                GimpConvertPaletteType  palette_type,
                                                gint                    max_colors,
                                                gboolean                remove_duplicates,
                                                GimpConvertDitherType   dither_type,
                                                gboolean                dither_alpha,
                                                gboolean                dither_text_layers,
                                                GimpPalette            *custom_palette,
                                                gpointer                user_data);

static void   image_convert_precision_callback (GtkWidget              *dialog,
                                                GimpImage              *image,
                                                GimpPrecision           precision,
                                                GeglDitherMethod        layer_dither_method,
                                                GeglDitherMethod        text_layer_dither_method,
                                                GeglDitherMethod        mask_dither_method,
                                                gpointer                user_data);

static void   image_profile_assign_callback    (GtkWidget                *dialog,
                                                GimpImage                *image,
                                                GimpColorProfile         *new_profile,
                                                GFile                    *new_file,
                                                GimpColorRenderingIntent  intent,
                                                gboolean                  bpc,
                                                gpointer                  user_data);

static void   image_profile_convert_callback   (GtkWidget                *dialog,
                                                GimpImage                *image,
                                                GimpColorProfile         *new_profile,
                                                GFile                    *new_file,
                                                GimpColorRenderingIntent  intent,
                                                gboolean                  bpc,
                                                gpointer                  user_data);

static void   image_resize_callback            (GtkWidget              *dialog,
                                                GimpViewable           *viewable,
                                                GimpContext            *context,
                                                gint                    width,
                                                gint                    height,
                                                GimpUnit                unit,
                                                gint                    offset_x,
                                                gint                    offset_y,
                                                GimpFillType            fill_type,
                                                GimpItemSet             layer_set,
                                                gboolean                resize_text_layers,
                                                gpointer                user_data);

static void   image_print_size_callback        (GtkWidget              *dialog,
                                                GimpImage              *image,
                                                gdouble                 xresolution,
                                                gdouble                 yresolution,
                                                GimpUnit                resolution_unit,
                                                gpointer                user_data);

static void   image_scale_callback             (GtkWidget              *dialog,
                                                GimpViewable           *viewable,
                                                gint                    width,
                                                gint                    height,
                                                GimpUnit                unit,
                                                GimpInterpolationType   interpolation,
                                                gdouble                 xresolution,
                                                gdouble                 yresolution,
                                                GimpUnit                resolution_unit,
                                                gpointer                user_data);

static void   image_merge_layers_callback      (GtkWidget              *dialog,
                                                GimpImage              *image,
                                                GimpContext            *context,
                                                GimpMergeType           merge_type,
                                                gboolean                merge_active_group,
                                                gboolean                discard_invisible);


/*  private variables  */

static GimpUnit               image_resize_unit  = GIMP_UNIT_PIXEL;
static GimpUnit               image_scale_unit   = GIMP_UNIT_PIXEL;
static GimpInterpolationType  image_scale_interp = -1;
static GimpPalette           *image_convert_indexed_custom_palette = NULL;


/*  public functions  */

void
image_new_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_widget (widget, data);

  dialog = gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                           gimp_widget_get_monitor (widget),
                                           NULL /*ui_manager*/,
                                           "gimp-image-new-dialog", -1, FALSE);

  if (dialog)
    {
      GimpImage *image = action_data_get_image (data);

      image_new_dialog_set (dialog, image, NULL);

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

void
image_duplicate_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpDisplay      *display;
  GimpImage        *image;
  GimpDisplayShell *shell;
  GimpImage        *new_image;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);
  shell = gimp_display_get_shell (display);

  new_image = gimp_image_duplicate (image);

  gimp_create_display (new_image->gimp, new_image, shell->unit,
                       gimp_zoom_model_get_factor (shell->zoom),
                       G_OBJECT (gimp_widget_get_monitor (GTK_WIDGET (shell))));

  g_object_unref (new_image);
}

void
image_convert_base_type_cmd_callback (GtkAction *action,
                                      GtkAction *current,
                                      gpointer   data)
{
  GimpImage         *image;
  GimpDisplay       *display;
  GtkWidget         *widget;
  GimpDialogConfig  *config;
  GtkWidget         *dialog;
  GimpImageBaseType  value;
  GError            *error = NULL;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value == gimp_image_get_base_type (image))
    return;

#define CONVERT_TYPE_DIALOG_KEY "gimp-convert-type-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), CONVERT_TYPE_DIALOG_KEY);

  if (dialog)
    {
      gtk_widget_destroy (dialog);
      dialog = NULL;
    }

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  switch (value)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      if (gimp_image_get_color_profile (image))
        {
          ColorProfileDialogType    dialog_type;
          GimpColorProfileCallback  callback;
          GimpColorProfile         *current_profile;
          GimpColorProfile         *default_profile;
          const Babl               *format;

          current_profile =
            gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));

          if (value == GIMP_RGB)
            {
              dialog_type = COLOR_PROFILE_DIALOG_CONVERT_TO_RGB;
              callback    = image_convert_rgb_callback;

              format = gimp_babl_format (GIMP_RGB,
                                         gimp_image_get_precision (image),
                                         TRUE);
              default_profile = gimp_babl_format_get_color_profile (format);
            }
          else
            {
              dialog_type = COLOR_PROFILE_DIALOG_CONVERT_TO_GRAY;
              callback    = image_convert_gray_callback;

              format = gimp_babl_format (GIMP_GRAY,
                                         gimp_image_get_precision (image),
                                         TRUE);
              default_profile = gimp_babl_format_get_color_profile (format);
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
      else if (! gimp_image_convert_type (image, value, NULL, NULL, &error))
        {
          gimp_message_literal (image->gimp,
                                G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
        }
      break;

    case GIMP_INDEXED:
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
  gimp_image_flush (image);
}

void
image_convert_precision_cmd_callback (GtkAction *action,
                                      GtkAction *current,
                                      gpointer   data)
{
  GimpImage         *image;
  GimpDisplay       *display;
  GtkWidget         *widget;
  GimpDialogConfig  *config;
  GtkWidget         *dialog;
  GimpComponentType  value;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value == gimp_image_get_component_type (image))
    return;

#define CONVERT_PRECISION_DIALOG_KEY "gimp-convert-precision-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), CONVERT_PRECISION_DIALOG_KEY);

  if (dialog)
    {
      gtk_widget_destroy (dialog);
      dialog = NULL;
    }

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  dialog = convert_precision_dialog_new (image,
                                         action_data_get_context (data),
                                         widget,
                                         value,
                                         config->image_convert_precision_layer_dither_method,
                                         config->image_convert_precision_text_layer_dither_method,
                                         config->image_convert_precision_channel_dither_method,
                                         image_convert_precision_callback,
                                         display);

  dialogs_attach_dialog (G_OBJECT (image),
                         CONVERT_PRECISION_DIALOG_KEY, dialog);

  gtk_window_present (GTK_WINDOW (dialog));

  /*  see comment above  */
  gimp_image_flush (image);
}

void
image_convert_gamma_cmd_callback (GtkAction *action,
                                  GtkAction *current,
                                  gpointer   data)
{
  GimpImage     *image;
  GimpDisplay   *display;
  gboolean       value;
  GimpPrecision  precision;
  return_if_no_image (image, data);
  return_if_no_display (display, data);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value == gimp_babl_format_get_linear (gimp_image_get_layer_format (image,
                                                                         FALSE)))
    return;

  precision = gimp_babl_precision (gimp_image_get_component_type (image),
                                   value);

  gimp_image_convert_precision (image, precision,
                                GEGL_DITHER_NONE,
                                GEGL_DITHER_NONE,
                                GEGL_DITHER_NONE,
                                GIMP_PROGRESS (display));
  gimp_image_flush (image);
}

void
image_color_management_enabled_cmd_callback (GtkAction *action,
                                             gpointer   data)
{
  GimpImage *image;
  gboolean   enabled;
  return_if_no_image (image, data);

  enabled = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (enabled != gimp_image_get_is_color_managed (image))
    {
      gimp_image_set_is_color_managed (image, enabled, TRUE);
      gimp_image_flush (image);
    }
}

void
image_color_profile_assign_cmd_callback (GtkAction *action,
                                         gpointer   data)
{
  GimpImage   *image;
  GimpDisplay *display;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

#define PROFILE_ASSIGN_DIALOG_KEY "gimp-profile-assign-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PROFILE_ASSIGN_DIALOG_KEY);

  if (! dialog)
    {
      GimpColorProfile *current_profile;
      GimpColorProfile *default_profile;

      current_profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));
      default_profile = gimp_image_get_builtin_color_profile (image);

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
image_color_profile_convert_cmd_callback (GtkAction *action,
                                          gpointer   data)
{
  GimpImage   *image;
  GimpDisplay *display;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

#define PROFILE_CONVERT_DIALOG_KEY "gimp-profile-convert-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PROFILE_CONVERT_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
      GimpColorProfile *current_profile;
      GimpColorProfile *default_profile;

      current_profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));
      default_profile = gimp_image_get_builtin_color_profile (image);

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
image_color_profile_discard_cmd_callback (GtkAction *action,
                                          gpointer   data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  gimp_image_set_color_profile (image, NULL, NULL);
  gimp_image_flush (image);
}

static void
image_profile_save_dialog_response (GtkWidget *dialog,
                                    gint       response_id,
                                    GimpImage *image)
{
  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      GimpColorProfile *profile;
      GFile            *file;
      GError           *error = NULL;

      profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));
      file    = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

      if (! file)
        return;

      if (! gimp_color_profile_save_to_file (profile, file, &error))
        {
          gimp_message (image->gimp, NULL,
                        GIMP_MESSAGE_WARNING,
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
image_color_profile_save_cmd_callback (GtkAction *action,
                                       gpointer   data)
{
  GimpImage   *image;
  GimpDisplay *display;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

#define PROFILE_SAVE_DIALOG_KEY "gimp-profile-save-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PROFILE_SAVE_DIALOG_KEY);

  if (! dialog)
    {
      GtkWindow        *toplevel;
      GimpColorProfile *profile;
      gchar            *basename;

      toplevel = GTK_WINDOW (gtk_widget_get_toplevel (widget));
      profile  = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));

      dialog =
        gimp_color_profile_chooser_dialog_new (_("Save Color Profile"),
                                               toplevel,
                                               GTK_FILE_CHOOSER_ACTION_SAVE);

      gimp_color_profile_chooser_dialog_connect_path (dialog,
                                                      G_OBJECT (image->gimp->config),
                                                      "color-profile-path");

      basename = g_strconcat (gimp_color_profile_get_label (profile),
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
image_resize_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  GimpImage   *image;
  GtkWidget   *widget;
  GimpDisplay *display;
  GtkWidget   *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);
  return_if_no_display (display, data);

#define RESIZE_DIALOG_KEY "gimp-resize-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), RESIZE_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

      if (image_resize_unit != GIMP_UNIT_PERCENT)
        image_resize_unit = gimp_display_get_shell (display)->unit;

      dialog = resize_dialog_new (GIMP_VIEWABLE (image),
                                  action_data_get_context (data),
                                  _("Set Image Canvas Size"),
                                  "gimp-image-resize",
                                  widget,
                                  gimp_standard_help_func,
                                  GIMP_HELP_IMAGE_RESIZE,
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
image_resize_to_layers_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpDisplay  *display;
  GimpImage    *image;
  GimpProgress *progress;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

  progress = gimp_progress_start (GIMP_PROGRESS (display), FALSE,
                                  _("Resizing"));

  gimp_image_resize_to_layers (image,
                               action_data_get_context (data),
                               progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (image);
}

void
image_resize_to_selection_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpDisplay  *display;
  GimpImage    *image;
  GimpProgress *progress;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

  progress = gimp_progress_start (GIMP_PROGRESS (display), FALSE,
                                  _("Resizing"));

  gimp_image_resize_to_selection (image,
                                  action_data_get_context (data),
                                  progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (image);
}

void
image_print_size_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay *display;
  GimpImage   *image;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  image = gimp_display_get_image (display);

#define PRINT_SIZE_DIALOG_KEY "gimp-print-size-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PRINT_SIZE_DIALOG_KEY);

  if (! dialog)
    {
      dialog = print_size_dialog_new (image,
                                      action_data_get_context (data),
                                      _("Set Image Print Resolution"),
                                      "gimp-image-print-size",
                                      widget,
                                      gimp_standard_help_func,
                                      GIMP_HELP_IMAGE_PRINT_SIZE,
                                      image_print_size_callback,
                                      NULL);

      dialogs_attach_dialog (G_OBJECT (image), PRINT_SIZE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_scale_cmd_callback (GtkAction *action,
                          gpointer   data)
{
  GimpDisplay *display;
  GimpImage   *image;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  image = gimp_display_get_image (display);

#define SCALE_DIALOG_KEY "gimp-scale-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), SCALE_DIALOG_KEY);

  if (! dialog)
    {
      if (image_scale_unit != GIMP_UNIT_PERCENT)
        image_scale_unit = gimp_display_get_shell (display)->unit;

      if (image_scale_interp == -1)
        image_scale_interp = display->gimp->config->interpolation_type;

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
image_flip_cmd_callback (GtkAction *action,
                         gint       value,
                         gpointer   data)
{
  GimpDisplay  *display;
  GimpImage    *image;
  GimpProgress *progress;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

  progress = gimp_progress_start (GIMP_PROGRESS (display), FALSE,
                                  _("Flipping"));

  gimp_image_flip (image, action_data_get_context (data),
                   (GimpOrientationType) value, progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (image);
}

void
image_rotate_cmd_callback (GtkAction *action,
                           gint       value,
                           gpointer   data)
{
  GimpDisplay  *display;
  GimpImage    *image;
  GimpProgress *progress;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

  progress = gimp_progress_start (GIMP_PROGRESS (display), FALSE,
                                  _("Rotating"));

  gimp_image_rotate (image, action_data_get_context (data),
                     (GimpRotationType) value, progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (image);
}

void
image_crop_to_selection_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  GimpImage *image;
  GtkWidget *widget;
  gint       x, y;
  gint       width, height;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  if (! gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                          &x, &y, &width, &height))
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            _("Cannot crop because the current selection "
                              "is empty."));
      return;
    }

  gimp_image_crop (image,
                   action_data_get_context (data), GIMP_FILL_TRANSPARENT,
                   x, y, width, height, TRUE);
  gimp_image_flush (image);
}

void
image_crop_to_content_cmd_callback (GtkAction *action,
                                    gpointer   data)
{
  GimpImage *image;
  GtkWidget *widget;
  gint       x, y;
  gint       width, height;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  switch (gimp_pickable_auto_shrink (GIMP_PICKABLE (image),
                                     0, 0,
                                     gimp_image_get_width  (image),
                                     gimp_image_get_height (image),
                                     &x, &y, &width, &height))
    {
    case GIMP_AUTO_SHRINK_SHRINK:
      gimp_image_crop (image,
                       action_data_get_context (data), GIMP_FILL_TRANSPARENT,
                       x, y, width, height, TRUE);
      gimp_image_flush (image);
      break;

    case GIMP_AUTO_SHRINK_EMPTY:
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_INFO,
                            _("Cannot crop because the image has no content."));
      break;

    case GIMP_AUTO_SHRINK_UNSHRINKABLE:
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_INFO,
                            _("Cannot crop because the image is already "
                              "cropped to its content."));
      break;
    }
}

void
image_merge_layers_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GtkWidget *dialog;
  GimpImage *image;
  GtkWidget *widget;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define MERGE_LAYERS_DIALOG_KEY "gimp-merge-layers-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), MERGE_LAYERS_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

      dialog = image_merge_layers_dialog_new (image,
                                              action_data_get_context (data),
                                              widget,
                                              config->layer_merge_type,
                                              config->layer_merge_active_group_only,
                                              config->layer_merge_discard_invisible,
                                              image_merge_layers_callback,
                                              NULL);

      dialogs_attach_dialog (G_OBJECT (image), MERGE_LAYERS_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_flatten_image_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpImage *image;
  GtkWidget *widget;
  GError    *error = NULL;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  if (! gimp_image_flatten (image, action_data_get_context (data), &error))
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
      return;
    }

  gimp_image_flush (image);
}

void
image_configure_grid_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDisplay *display;
  GimpImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

#define GRID_DIALOG_KEY "gimp-grid-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), GRID_DIALOG_KEY);

  if (! dialog)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (display);

      dialog = grid_dialog_new (image,
                                action_data_get_context (data),
                                gtk_widget_get_toplevel (GTK_WIDGET (shell)));

      dialogs_attach_dialog (G_OBJECT (image), GRID_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_properties_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay *display;
  GimpImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

#define PROPERTIES_DIALOG_KEY "gimp-image-properties-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PROPERTIES_DIALOG_KEY);

  if (! dialog)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (display);

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
                            GimpImage                *image,
                            GimpColorProfile         *new_profile,
                            GFile                    *new_file,
                            GimpColorRenderingIntent  intent,
                            gboolean                  bpc,
                            gpointer                  user_data)
{
  GimpProgress *progress = user_data;
  GError       *error    = NULL;

  progress = gimp_progress_start (progress, FALSE,
                                  _("Converting to RGB (%s)"),
                                  gimp_color_profile_get_label (new_profile));

  if (! gimp_image_convert_type (image, GIMP_RGB, new_profile,
                                 progress, &error))
    {
      gimp_message (image->gimp, G_OBJECT (dialog),
                    GIMP_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);

      if (progress)
        gimp_progress_end (progress);

      return;
    }

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (image);

 gtk_widget_destroy (dialog);
}

static void
image_convert_gray_callback (GtkWidget                *dialog,
                             GimpImage                *image,
                             GimpColorProfile         *new_profile,
                             GFile                    *new_file,
                             GimpColorRenderingIntent  intent,
                             gboolean                  bpc,
                             gpointer                  user_data)
{
  GimpProgress *progress = user_data;
  GError       *error    = NULL;

  progress = gimp_progress_start (progress, FALSE,
                                  _("Converting to grayscale (%s)"),
                                  gimp_color_profile_get_label (new_profile));

  if (! gimp_image_convert_type (image, GIMP_GRAY, new_profile,
                                 progress, &error))
    {
      gimp_message (image->gimp, G_OBJECT (dialog),
                    GIMP_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);

      if (progress)
        gimp_progress_end (progress);

      return;
    }

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_convert_indexed_callback (GtkWidget              *dialog,
                                GimpImage              *image,
                                GimpConvertPaletteType  palette_type,
                                gint                    max_colors,
                                gboolean                remove_duplicates,
                                GimpConvertDitherType   dither_type,
                                gboolean                dither_alpha,
                                gboolean                dither_text_layers,
                                GimpPalette            *custom_palette,
                                gpointer                user_data)
{
  GimpDialogConfig *config  = GIMP_DIALOG_CONFIG (image->gimp->config);
  GimpDisplay      *display = user_data;
  GimpProgress     *progress;
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

  progress = gimp_progress_start (GIMP_PROGRESS (display), FALSE,
                                  _("Converting to indexed colors"));

  if (! gimp_image_convert_indexed (image,
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
      gimp_message_literal (image->gimp, G_OBJECT (display),
                            GIMP_MESSAGE_WARNING, error->message);
      g_clear_error (&error);

      if (progress)
        gimp_progress_end (progress);

      return;
    }

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_convert_precision_callback (GtkWidget        *dialog,
                                  GimpImage        *image,
                                  GimpPrecision     precision,
                                  GeglDitherMethod  layer_dither_method,
                                  GeglDitherMethod  text_layer_dither_method,
                                  GeglDitherMethod  channel_dither_method,
                                  gpointer          user_data)
{
  GimpDialogConfig *config   = GIMP_DIALOG_CONFIG (image->gimp->config);
  GimpProgress     *progress = user_data;
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
   *  GimpDialogConfig.
   */

  /* random formats with the right precision */
  old_format = gimp_image_get_layer_format (image, FALSE);
  new_format = gimp_babl_format (GIMP_RGB, precision, FALSE);

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

  gimp_enum_get_value (GIMP_TYPE_PRECISION, precision,
                       NULL, NULL, &enum_desc, NULL);

  progress = gimp_progress_start (progress, FALSE,
                                  _("Converting image to %s"),
                                  enum_desc);

  gimp_image_convert_precision (image,
                                precision,
                                layer_dither_method,
                                text_layer_dither_method,
                                channel_dither_method,
                                progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_profile_assign_callback (GtkWidget                *dialog,
                               GimpImage                *image,
                               GimpColorProfile         *new_profile,
                               GFile                    *new_file,
                               GimpColorRenderingIntent  intent,
                               gboolean                  bpc,
                               gpointer                  user_data)
{
  GError *error = NULL;

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_PARASITE_ATTACH,
                               _("Assign color profile"));

  if (! gimp_image_set_color_profile (image, new_profile, &error))
    {
      gimp_message (image->gimp, G_OBJECT (dialog),
                    GIMP_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);

      gimp_image_undo_group_end (image);
      gimp_image_undo (image);

      return;
    }

  gimp_image_set_is_color_managed (image, TRUE, TRUE);

  /*  omg...  */
  gimp_image_parasite_detach (image, "icc-profile-name");

  gimp_image_undo_group_end (image);

  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_profile_convert_callback (GtkWidget                *dialog,
                                GimpImage                *image,
                                GimpColorProfile         *new_profile,
                                GFile                    *new_file,
                                GimpColorRenderingIntent  intent,
                                gboolean                  bpc,
                                gpointer                  user_data)
{
  GimpDialogConfig *config   = GIMP_DIALOG_CONFIG (image->gimp->config);
  GimpProgress     *progress = user_data;
  GError           *error    = NULL;

  g_object_set (config,
                "image-convert-profile-intent",                   intent,
                "image-convert-profile-black-point-compensation", bpc,
                NULL);

  progress = gimp_progress_start (progress, FALSE,
                                  _("Converting to '%s'"),
                                  gimp_color_profile_get_label (new_profile));

  if (! gimp_image_convert_color_profile (image, new_profile,
                                          config->image_convert_profile_intent,
                                          config->image_convert_profile_bpc,
                                          progress, &error))
    {
      gimp_message (image->gimp, G_OBJECT (dialog),
                    GIMP_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);

      if (progress)
        gimp_progress_end (progress);

      return;
    }

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_resize_callback (GtkWidget    *dialog,
                       GimpViewable *viewable,
                       GimpContext  *context,
                       gint          width,
                       gint          height,
                       GimpUnit      unit,
                       gint          offset_x,
                       gint          offset_y,
                       GimpFillType  fill_type,
                       GimpItemSet   layer_set,
                       gboolean      resize_text_layers,
                       gpointer      user_data)
{
  GimpDisplay *display = user_data;

  image_resize_unit = unit;

  if (width > 0 && height > 0)
    {
      GimpImage        *image  = GIMP_IMAGE (viewable);
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
      GimpProgress     *progress;

      g_object_set (config,
                    "image-resize-fill-type",          fill_type,
                    "image-resize-layer-set",          layer_set,
                    "image-resize-resize-text-layers", resize_text_layers,
                    NULL);

      gtk_widget_destroy (dialog);

      if (width  == gimp_image_get_width  (image) &&
          height == gimp_image_get_height (image))
        return;

      progress = gimp_progress_start (GIMP_PROGRESS (display), FALSE,
                                      _("Resizing"));

      gimp_image_resize_with_layers (image,
                                     context, fill_type,
                                     width, height, offset_x, offset_y,
                                     layer_set,
                                     resize_text_layers,
                                     progress);

      if (progress)
        gimp_progress_end (progress);

      gimp_image_flush (image);
    }
  else
    {
      g_warning ("Resize Error: "
                 "Both width and height must be greater than zero.");
    }
}

static void
image_print_size_callback (GtkWidget *dialog,
                           GimpImage *image,
                           gdouble    xresolution,
                           gdouble    yresolution,
                           GimpUnit   resolution_unit,
                           gpointer   data)
{
  gdouble xres;
  gdouble yres;

  gtk_widget_destroy (dialog);

  gimp_image_get_resolution (image, &xres, &yres);

  if (xresolution     == xres &&
      yresolution     == yres &&
      resolution_unit == gimp_image_get_unit (image))
    return;

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_SCALE,
                               _("Change Print Size"));

  gimp_image_set_resolution (image, xresolution, yresolution);
  gimp_image_set_unit (image, resolution_unit);

  gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

static void
image_scale_callback (GtkWidget              *dialog,
                      GimpViewable           *viewable,
                      gint                    width,
                      gint                    height,
                      GimpUnit                unit,
                      GimpInterpolationType   interpolation,
                      gdouble                 xresolution,
                      gdouble                 yresolution,
                      GimpUnit                resolution_unit,
                      gpointer                user_data)
{
  GimpProgress *progress = user_data;
  GimpImage    *image    = GIMP_IMAGE (viewable);
  gdouble       xres;
  gdouble       yres;

  image_scale_unit   = unit;
  image_scale_interp = interpolation;

  gimp_image_get_resolution (image, &xres, &yres);

  if (width > 0 && height > 0)
    {
      gtk_widget_destroy (dialog);

      if (width           == gimp_image_get_width  (image) &&
          height          == gimp_image_get_height (image) &&
          xresolution     == xres                          &&
          yresolution     == yres                          &&
          resolution_unit == gimp_image_get_unit (image))
        return;

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_SCALE,
                                   _("Scale Image"));

      gimp_image_set_resolution (image, xresolution, yresolution);
      gimp_image_set_unit (image, resolution_unit);

      if (width  != gimp_image_get_width  (image) ||
          height != gimp_image_get_height (image))
        {
          progress = gimp_progress_start (progress, FALSE,
                                          _("Scaling"));

          gimp_image_scale (image, width, height, interpolation, progress);

          if (progress)
            gimp_progress_end (progress);
        }

      gimp_image_undo_group_end (image);

      gimp_image_flush (image);
    }
  else
    {
      g_warning ("Scale Error: "
                 "Both width and height must be greater than zero.");
    }
}

static void
image_merge_layers_callback (GtkWidget     *dialog,
                             GimpImage     *image,
                             GimpContext   *context,
                             GimpMergeType  merge_type,
                             gboolean       merge_active_group,
                             gboolean       discard_invisible)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

  g_object_set (config,
                "layer-merge-type",              merge_type,
                "layer-merge-active-group-only", merge_active_group,
                "layer-merge-discard-invisible", discard_invisible,
                NULL);

  gimp_image_merge_visible_layers (image,
                                   context,
                                   config->layer_merge_type,
                                   config->layer_merge_active_group_only,
                                   config->layer_merge_discard_invisible);

  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}
