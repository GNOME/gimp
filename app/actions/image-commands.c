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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-babl.h"

#include "core/core-enums.h"
#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-color-profile.h"
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

#include "dialogs/color-profile-dialog.h"
#include "dialogs/convert-precision-dialog.h"
#include "dialogs/convert-type-dialog.h"
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


#define IMAGE_CONVERT_PRECISION_DIALOG_KEY "image-convert-precision-dialog"
#define IMAGE_CONVERT_TYPE_DIALOG_KEY      "image-convert-type-dialog"
#define IMAGE_PROFILE_CONVERT_DIALOG_KEY   "image-profile-convert-dialog"
#define IMAGE_PROFILE_ASSIGN_DIALOG_KEY    "image-profile-assign-dialog"


typedef struct
{
  GimpContext *context;
  GimpDisplay *display;
} ImageResizeOptions;


/*  local function prototypes  */

static void   image_resize_callback        (GtkWidget              *dialog,
                                            GimpViewable           *viewable,
                                            gint                    width,
                                            gint                    height,
                                            GimpUnit                unit,
                                            gint                    offset_x,
                                            gint                    offset_y,
                                            GimpItemSet             layer_set,
                                            gboolean                resize_text_layers,
                                            gpointer                data);
static void   image_resize_options_free    (ImageResizeOptions     *options);

static void   image_print_size_callback    (GtkWidget              *dialog,
                                            GimpImage              *image,
                                            gdouble                 xresolution,
                                            gdouble                 yresolution,
                                            GimpUnit                resolution_unit,
                                            gpointer                data);

static void   image_scale_callback         (GtkWidget              *dialog,
                                            GimpViewable           *viewable,
                                            gint                    width,
                                            gint                    height,
                                            GimpUnit                unit,
                                            GimpInterpolationType   interpolation,
                                            gdouble                 xresolution,
                                            gdouble                 yresolution,
                                            GimpUnit                resolution_unit,
                                            gpointer                user_data);

static void   image_merge_layers_response  (GtkWidget              *widget,
                                            gint                    response_id,
                                            ImageMergeLayersDialog *dialog);


/*  private variables  */

static GimpMergeType         image_merge_layers_type               = GIMP_EXPAND_AS_NECESSARY;
static gboolean              image_merge_layers_merge_active_group = TRUE;
static gboolean              image_merge_layers_discard_invisible  = FALSE;
static GimpUnit              image_resize_unit                     = GIMP_UNIT_PIXEL;
static GimpUnit              image_scale_unit                      = GIMP_UNIT_PIXEL;
static GimpInterpolationType image_scale_interp                    = -1;


/*  public functions  */

void
image_new_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_widget (widget, data);

  dialog = gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                           gtk_widget_get_screen (widget),
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

static void
image_convert_type_dialog_unset (GimpImage *image)
{
  g_object_set_data (G_OBJECT (image), IMAGE_CONVERT_TYPE_DIALOG_KEY, NULL);
}

void
image_convert_base_type_cmd_callback (GtkAction *action,
                                      GtkAction *current,
                                      gpointer   data)
{
  GimpImage         *image;
  GimpDisplay       *display;
  GtkWidget         *widget;
  GtkWidget         *dialog;
  GimpImageBaseType  value;
  GError            *error = NULL;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value == gimp_image_get_base_type (image))
    return;

  dialog = g_object_get_data (G_OBJECT (image),
                              IMAGE_CONVERT_TYPE_DIALOG_KEY);

  switch (value)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      if (dialog)
        gtk_widget_destroy (dialog);

      if (! gimp_image_convert_type (image, value, NULL, NULL, &error))
        {
          gimp_message_literal (image->gimp,
                                G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
          return;
        }
      break;

    case GIMP_INDEXED:
      if (! dialog)
        {
          dialog = convert_type_dialog_new (image,
                                            action_data_get_context (data),
                                            widget,
                                            GIMP_PROGRESS (display));

          g_object_set_data (G_OBJECT (image),
                             IMAGE_CONVERT_TYPE_DIALOG_KEY, dialog);

          g_signal_connect_object (dialog, "destroy",
                                   G_CALLBACK (image_convert_type_dialog_unset),
                                   image, G_CONNECT_SWAPPED);
        }

      gtk_window_present (GTK_WINDOW (dialog));
      break;
    }

  /*  always flush, also when only the indexed dialog was shown, so the
   *  menu items get updated back to the current image type
   */
  gimp_image_flush (image);
}

static void
image_convert_precision_dialog_unset (GimpImage *image)
{
  g_object_set_data (G_OBJECT (image), IMAGE_CONVERT_PRECISION_DIALOG_KEY, NULL);
}

void
image_convert_precision_cmd_callback (GtkAction *action,
                                      GtkAction *current,
                                      gpointer   data)
{
  GimpImage         *image;
  GimpDisplay       *display;
  GtkWidget         *widget;
  GtkWidget         *dialog;
  GimpComponentType  value;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value == gimp_image_get_component_type (image))
    return;

  dialog = g_object_get_data (G_OBJECT (image),
                              IMAGE_CONVERT_PRECISION_DIALOG_KEY);

  if (! dialog)
    {
      dialog = convert_precision_dialog_new (image,
                                             action_data_get_context (data),
                                             widget,
                                             value,
                                             GIMP_PROGRESS (display));

      g_object_set_data (G_OBJECT (image),
                         IMAGE_CONVERT_PRECISION_DIALOG_KEY, dialog);

      g_signal_connect_object (dialog, "destroy",
                               G_CALLBACK (image_convert_precision_dialog_unset),
                               image, G_CONNECT_SWAPPED);
    }

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

  gimp_image_convert_precision (image, precision, 0, 0, 0,
                                GIMP_PROGRESS (display));
  gimp_image_flush (image);
}

static void
image_profile_assign_dialog_unset (GimpImage *image)
{
  g_object_set_data (G_OBJECT (image), IMAGE_PROFILE_ASSIGN_DIALOG_KEY, NULL);
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

  dialog = g_object_get_data (G_OBJECT (image),
                              IMAGE_PROFILE_ASSIGN_DIALOG_KEY);

  if (! dialog)
    {
      dialog = color_profile_dialog_new (COLOR_PROFILE_DIALOG_ASSIGN_PROFILE,
                                         image,
                                         action_data_get_context (data),
                                         widget,
                                         GIMP_PROGRESS (display));

      g_object_set_data (G_OBJECT (image),
                         IMAGE_PROFILE_ASSIGN_DIALOG_KEY, dialog);

      g_signal_connect_object (dialog, "destroy",
                               G_CALLBACK (image_profile_assign_dialog_unset),
                               image, G_CONNECT_SWAPPED);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
image_profile_convert_dialog_unset (GimpImage *image)
{
  g_object_set_data (G_OBJECT (image), IMAGE_PROFILE_CONVERT_DIALOG_KEY, NULL);
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

  dialog = g_object_get_data (G_OBJECT (image),
                              IMAGE_PROFILE_CONVERT_DIALOG_KEY);

  if (! dialog)
    {
      dialog = color_profile_dialog_new (COLOR_PROFILE_DIALOG_CONVERT_TO_PROFILE,
                                         image,
                                         action_data_get_context (data),
                                         widget,
                                         GIMP_PROGRESS (display));

      g_object_set_data (G_OBJECT (image),
                         IMAGE_PROFILE_CONVERT_DIALOG_KEY, dialog);

      g_signal_connect_object (dialog, "destroy",
                               G_CALLBACK (image_profile_convert_dialog_unset),
                               image, G_CONNECT_SWAPPED);
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

void
image_resize_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  ImageResizeOptions *options;
  GimpImage          *image;
  GtkWidget          *widget;
  GimpDisplay        *display;
  GtkWidget          *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);
  return_if_no_display (display, data);

  options = g_slice_new (ImageResizeOptions);

  options->display = display;
  options->context = action_data_get_context (data);

  if (image_resize_unit != GIMP_UNIT_PERCENT)
    image_resize_unit = gimp_display_get_shell (display)->unit;

  dialog = resize_dialog_new (GIMP_VIEWABLE (image),
                              action_data_get_context (data),
                              _("Set Image Canvas Size"), "gimp-image-resize",
                              widget,
                              gimp_standard_help_func, GIMP_HELP_IMAGE_RESIZE,
                              image_resize_unit,
                              image_resize_callback,
                              options);

  g_signal_connect_object (display, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) image_resize_options_free, options);

  gtk_widget_show (dialog);
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

  dialog = print_size_dialog_new (image,
                                  action_data_get_context (data),
                                  _("Set Image Print Resolution"),
                                  "gimp-image-print-size",
                                  widget,
                                  gimp_standard_help_func,
                                  GIMP_HELP_IMAGE_PRINT_SIZE,
                                  image_print_size_callback,
                                  NULL);

  g_signal_connect_object (display, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  gtk_widget_show (dialog);
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

  g_signal_connect_object (display, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  gtk_widget_show (dialog);
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
  gint       x, y, w, h;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  if (! gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                          &x, &y, &w, &h))
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            _("Cannot crop because the current selection is empty."));
      return;
    }

  gimp_image_crop (image, action_data_get_context (data),
                   x, y, w, h, TRUE);
  gimp_image_flush (image);
}

void
image_crop_to_content_cmd_callback (GtkAction *action,
                                    gpointer   data)
{
  GimpImage *image;
  GtkWidget *widget;
  gint       x1, y1, x2, y2;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  switch (gimp_pickable_auto_shrink (GIMP_PICKABLE (image),
                                     0, 0,
                                     gimp_image_get_width  (image),
                                     gimp_image_get_height (image),
                                     &x1, &y1, &x2, &y2))
    {
    case GIMP_AUTO_SHRINK_SHRINK:
      gimp_image_crop (image, action_data_get_context (data),
                       x1, y1, x2 - x1, y2 - y1, TRUE);
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
                            _("Cannot crop because the image is already cropped to its content."));
      break;
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
                       G_OBJECT (gtk_widget_get_screen (GTK_WIDGET (shell))),
                       gimp_widget_get_monitor (GTK_WIDGET (shell)));

  g_object_unref (new_image);
}

void
image_merge_layers_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  ImageMergeLayersDialog *dialog;
  GimpImage              *image;
  GtkWidget              *widget;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = image_merge_layers_dialog_new (image,
                                          action_data_get_context (data),
                                          widget,
                                          image_merge_layers_type,
                                          image_merge_layers_merge_active_group,
                                          image_merge_layers_discard_invisible);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (image_merge_layers_response),
                    dialog);

  gtk_widget_show (dialog->dialog);
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
  GimpDisplay      *display;
  GimpImage        *image;
  GimpDisplayShell *shell;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);
  shell = gimp_display_get_shell (display);

  if (! shell->grid_dialog)
    {
      shell->grid_dialog = grid_dialog_new (image,
                                            action_data_get_context (data),
                                            GTK_WIDGET (shell));

      gtk_window_set_transient_for (GTK_WINDOW (shell->grid_dialog),
                                    GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (shell))));
      gtk_window_set_destroy_with_parent (GTK_WINDOW (shell->grid_dialog),
                                          TRUE);

      g_object_add_weak_pointer (G_OBJECT (shell->grid_dialog),
                                 (gpointer) &shell->grid_dialog);
    }

  gtk_window_present (GTK_WINDOW (shell->grid_dialog));
}

void
image_properties_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay      *display;
  GimpImage        *image;
  GimpDisplayShell *shell;
  GtkWidget        *dialog;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);
  shell = gimp_display_get_shell (display);

  dialog = image_properties_dialog_new (image,
                                        action_data_get_context (data),
                                        GTK_WIDGET (shell));

  gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (shell))));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog),
                                      TRUE);

  gtk_window_present (GTK_WINDOW (dialog));
}


/*  private functions  */

static void
image_resize_callback (GtkWidget    *dialog,
                       GimpViewable *viewable,
                       gint          width,
                       gint          height,
                       GimpUnit      unit,
                       gint          offset_x,
                       gint          offset_y,
                       GimpItemSet   layer_set,
                       gboolean      resize_text_layers,
                       gpointer      data)
{
  ImageResizeOptions *options = data;

  image_resize_unit = unit;

  if (width > 0 && height > 0)
    {
      GimpImage    *image   = GIMP_IMAGE (viewable);
      GimpDisplay  *display = options->display;
      GimpContext  *context = options->context;
      GimpProgress *progress;

      gtk_widget_destroy (dialog);

      if (width  == gimp_image_get_width  (image) &&
          height == gimp_image_get_height (image))
        return;

      progress = gimp_progress_start (GIMP_PROGRESS (display), FALSE,
                                      _("Resizing"));

      gimp_image_resize_with_layers (image,
                                     context,
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
image_resize_options_free (ImageResizeOptions *options)
{
  g_slice_free (ImageResizeOptions, options);
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
  GimpImage *image = GIMP_IMAGE (viewable);
  gdouble    xres;
  gdouble    yres;

  image_scale_unit   = unit;
  image_scale_interp = interpolation;

  gimp_image_get_resolution (image, &xres, &yres);

  if (width > 0 && height > 0)
    {
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
          GimpProgress *progress;

          progress = gimp_progress_start (GIMP_PROGRESS (user_data), FALSE,
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
image_merge_layers_response (GtkWidget              *widget,
                             gint                    response_id,
                             ImageMergeLayersDialog *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      image_merge_layers_type               = dialog->merge_type;
      image_merge_layers_merge_active_group = dialog->merge_active_group;
      image_merge_layers_discard_invisible  = dialog->discard_invisible;

      gimp_image_merge_visible_layers (dialog->image,
                                       dialog->context,
                                       image_merge_layers_type,
                                       image_merge_layers_merge_active_group,
                                       image_merge_layers_discard_invisible);

      gimp_image_flush (dialog->image);
    }

  gtk_widget_destroy (widget);
}
