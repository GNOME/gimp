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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpcoreconfig.h"

#include "core/core-enums.h"
#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-convert.h"
#include "core/gimpimage-crop.h"
#include "core/gimpimage-duplicate.h"
#include "core/gimpimage-flip.h"
#include "core/gimpimage-merge.h"
#include "core/gimpimage-resize.h"
#include "core/gimpimage-rotate.h"
#include "core/gimpimage-scale.h"
#include "core/gimpimage-undo.h"
#include "core/gimpprogress.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "dialogs/convert-dialog.h"
#include "dialogs/dialogs.h"
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

static GimpMergeType         image_merge_layers_type = GIMP_EXPAND_AS_NECESSARY;
static gboolean              image_merge_layers_discard_invisible = FALSE;
static GimpUnit              image_resize_unit       = GIMP_UNIT_PIXEL;
static GimpUnit              image_scale_unit        = GIMP_UNIT_PIXEL;
static GimpInterpolationType image_scale_interp      = -1;


/*  public functions  */

void
image_new_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_widget (widget, data);

  dialog = gimp_dialog_factory_dialog_new (global_dialog_factory,
                                           gtk_widget_get_screen (widget),
                                           "gimp-image-new-dialog", -1, FALSE);

  if (dialog)
    {
      image_new_dialog_set (dialog, NULL, NULL);

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

void
image_new_from_image_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_widget (widget, data);

  dialog = gimp_dialog_factory_dialog_new (global_dialog_factory,
                                           gtk_widget_get_screen (widget),
                                           "gimp-image-new-dialog", -1, FALSE);

  if (dialog)
    {
      GimpImage *image = action_data_get_image (data);

      image_new_dialog_set (dialog, image, NULL);

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

static void
image_convert_dialog_unset (GtkWidget *widget)
{
  g_object_set_data (G_OBJECT (widget), "image-convert-dialog", NULL);
}

void
image_convert_cmd_callback (GtkAction *action,
                            GtkAction *current,
                            gpointer   data)
{
  GimpImage         *image;
  GtkWidget         *widget;
  GimpDisplay       *display;
  GimpImageBaseType  value;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);
  return_if_no_display (display, data);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value == gimp_image_base_type (image))
    return;

  switch (value)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      gimp_image_convert (image, value, 0, 0, FALSE, FALSE, 0, NULL, NULL);
      break;

    case GIMP_INDEXED:
      {
        GtkWidget *dialog;

        dialog = g_object_get_data (G_OBJECT (widget), "image-convert-dialog");

        if (! dialog)
          {
            dialog = convert_dialog_new (image,
                                         action_data_get_context (data),
                                         widget,
                                         GIMP_PROGRESS (display));

            g_object_set_data (G_OBJECT (widget),
                               "image-convert-dialog", dialog);

            g_signal_connect_object (dialog, "destroy",
                                     G_CALLBACK (image_convert_dialog_unset),
                                     widget, G_CONNECT_SWAPPED);
          }

        gtk_window_present (GTK_WINDOW (dialog));
      }
      break;
    }

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
    image_resize_unit = GIMP_DISPLAY_SHELL (display->shell)->unit;

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
  GimpProgress *progress;
  return_if_no_display (display, data);

  progress = gimp_progress_start (GIMP_PROGRESS (display),
                                  _("Resizing"), FALSE);

  gimp_image_resize_to_layers (display->image,
                               action_data_get_context (data),
                               progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (display->image);
}

void
image_resize_to_selection_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpDisplay  *display;
  GimpProgress *progress;
  return_if_no_display (display, data);

  progress = gimp_progress_start (GIMP_PROGRESS (display),
                                  _("Resizing"), FALSE);

  gimp_image_resize_to_selection (display->image,
                                  action_data_get_context (data),
                                  progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (display->image);
}

void
image_print_size_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GtkWidget   *dialog;
  GimpDisplay *display;
  GtkWidget   *widget;
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  dialog = print_size_dialog_new (display->image,
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
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  if (image_scale_unit != GIMP_UNIT_PERCENT)
    image_scale_unit = GIMP_DISPLAY_SHELL (display->shell)->unit;

  if (image_scale_interp == -1)
    image_scale_interp = display->image->gimp->config->interpolation_type;

  dialog = image_scale_dialog_new (display->image,
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
  GimpProgress *progress;
  return_if_no_display (display, data);

  progress = gimp_progress_start (GIMP_PROGRESS (display),
                                  _("Flipping"), FALSE);

  gimp_image_flip (display->image, action_data_get_context (data),
                   (GimpOrientationType) value, progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (display->image);
}

void
image_rotate_cmd_callback (GtkAction *action,
                           gint       value,
                           gpointer   data)
{
  GimpDisplay  *display;
  GimpProgress *progress;
  return_if_no_display (display, data);

  progress = gimp_progress_start (GIMP_PROGRESS (display),
                                  _("Rotating"), FALSE);

  gimp_image_rotate (display->image, action_data_get_context (data),
                     (GimpRotationType) value, progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (display->image);
}

void
image_crop_cmd_callback (GtkAction *action,
                         gpointer   data)
{
  GimpImage *image;
  GtkWidget *widget;
  gint       x1, y1, x2, y2;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  if (! gimp_channel_bounds (gimp_image_get_mask (image),
                             &x1, &y1, &x2, &y2))
    {
      gimp_message (image->gimp, G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                    _("Cannot crop because the current selection is empty."));
      return;
    }

  gimp_image_crop (image, action_data_get_context (data),
                   x1, y1, x2, y2, FALSE, TRUE);
  gimp_image_flush (image);
}

void
image_duplicate_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  GimpImage        *new_image;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  new_image = gimp_image_duplicate (display->image);

  gimp_create_display (new_image->gimp,
                       new_image,
                       shell->unit,
                       gimp_zoom_model_get_factor (shell->zoom));

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
  return_if_no_image (image, data);

  gimp_image_flatten (image, action_data_get_context (data));
  gimp_image_flush (image);
}

void
image_configure_grid_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  GimpImage        *image;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);
  image = display->image;

  if (! shell->grid_dialog)
    {
      shell->grid_dialog = grid_dialog_new (display->image,
                                            action_data_get_context (data),
                                            display->shell);

      gtk_window_set_transient_for (GTK_WINDOW (shell->grid_dialog),
                                    GTK_WINDOW (display->shell));
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
  GimpDisplayShell *shell;
  GimpImage        *image;
  GtkWidget        *dialog;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);
  image = display->image;

  dialog = image_properties_dialog_new (display->image,
                                        action_data_get_context (data),
                                        display->shell);

  gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                GTK_WINDOW (display->shell));
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

      if (width  == image->width &&
          height == image->height)
        return;

      progress = gimp_progress_start (GIMP_PROGRESS (display),
                                      _("Resizing"), FALSE);

      gimp_image_resize_with_layers (image,
                                     context,
                                     width, height, offset_x, offset_y,
                                     layer_set,
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
  gtk_widget_destroy (dialog);

  if (xresolution     == image->xresolution     &&
      yresolution     == image->yresolution     &&
      resolution_unit == image->resolution_unit)
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

  image_scale_unit   = unit;
  image_scale_interp = interpolation;

  if (width > 0 && height > 0)
    {
      if (width           == image->width           &&
          height          == image->height          &&
          xresolution     == image->xresolution     &&
          yresolution     == image->yresolution     &&
          resolution_unit == image->resolution_unit)
        return;

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_SCALE,
                                   _("Scale Image"));

      gimp_image_set_resolution (image, xresolution, yresolution);
      gimp_image_set_unit (image, resolution_unit);

      if (width  != image->width ||
          height != image->height)
        {
          GimpProgress *progress;

          progress = gimp_progress_start (GIMP_PROGRESS (user_data),
                                          _("Scaling"), FALSE);

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
      image_merge_layers_type              = dialog->merge_type;
      image_merge_layers_discard_invisible = dialog->discard_invisible;

      gimp_image_merge_visible_layers (dialog->image,
                                       dialog->context,
                                       image_merge_layers_type,
                                       image_merge_layers_discard_invisible);

      gimp_image_flush (dialog->image);
    }

  gtk_widget_destroy (widget);
}
