/* The GIMP -- an image manipulation program
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
#include "dialogs/image-scale-dialog.h"
#include "dialogs/print-size-dialog.h"
#include "dialogs/resize-dialog.h"

#include "actions.h"
#include "image-commands.h"

#include "gimp-intl.h"


typedef struct _ImageResizeOptions ImageResizeOptions;

struct _ImageResizeOptions
{
  GimpContext  *context;
  GimpDisplay  *gdisp;
};


/*  local function prototypes  */

static void   image_resize_callback        (GtkWidget              *dialog,
                                            GimpViewable           *viewable,
                                            gint                    width,
                                            gint                    height,
                                            gint                    offset_x,
                                            gint                    offset_y,
                                            gpointer                data);
static void   image_print_size_callback    (GtkWidget              *dialog,
                                            GimpImage              *image,
                                            gdouble                 xresolution,
                                            gdouble                 yresolution,
                                            GimpUnit                resolution_unit,
                                            gpointer                data);
static void   image_scale_callback         (ImageScaleDialog       *dialog);

static void   image_merge_layers_response  (GtkWidget              *widget,
                                            gint                    response_id,
                                            ImageMergeLayersDialog *dialog);


/*  private variables  */

static GimpMergeType image_merge_layers_type = GIMP_EXPAND_AS_NECESSARY;


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
      GimpImage *gimage = action_data_get_image (data);

      image_new_dialog_set (dialog, gimage, NULL);

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

void
image_convert_cmd_callback (GtkAction *action,
                            gint       value,
                            gpointer   data)
{
  GimpImage   *gimage;
  GtkWidget   *widget;
  GimpDisplay *gdisp;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);
  return_if_no_display (gdisp, data);

  switch ((GimpImageBaseType) value)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      gimp_image_convert (gimage, (GimpImageBaseType) value,
                          0, 0, FALSE, FALSE, 0, NULL, NULL);
      gimp_image_flush (gimage);
      break;

    case GIMP_INDEXED:
      gtk_widget_show (convert_dialog_new (gimage, widget,
                                           GIMP_PROGRESS (gdisp)));
      break;
    }
}

void
image_resize_cmd_callback (GtkAction *action,
			   gpointer   data)
{
  ImageResizeOptions *options;
  GimpImage          *gimage;
  GtkWidget          *widget;
  GimpDisplay        *gdisp;
  GtkWidget          *dialog;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);
  return_if_no_display (gdisp, data);

  options = g_new0 (ImageResizeOptions, 1);

  options->gdisp   = gdisp;
  options->context = action_data_get_context (data);

  dialog = resize_dialog_new (GIMP_VIEWABLE (gimage),
                              _("Set Image Canvas Size"), "gimp-image-resize",
                              widget,
                              gimp_standard_help_func, GIMP_HELP_IMAGE_RESIZE,
                              GIMP_DISPLAY_SHELL (gdisp->shell)->unit,
                              image_resize_callback,
                              options);

  g_signal_connect_object (gdisp, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  g_object_weak_ref (G_OBJECT (dialog),
		     (GWeakNotify) g_free, options);

  gtk_widget_show (dialog);
}

void
image_resize_to_layers_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpDisplay  *gdisp;
  GimpProgress *progress;

  return_if_no_display (gdisp, data);

  progress = gimp_progress_start (GIMP_PROGRESS (gdisp),
                                  _("Resizing..."), FALSE);

  gimp_image_resize_to_layers (gdisp->gimage,
                               action_data_get_context (data),
                               progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (gdisp->gimage);
}

void
image_print_size_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GtkWidget   *dialog;
  GimpDisplay *gdisp;
  GtkWidget   *widget;
  return_if_no_display (gdisp, data);
  return_if_no_widget (widget, data);

  dialog = print_size_dialog_new (gdisp->gimage,
                                  _("Set Image Print Resolution"),
                                  "gimp-image-print-size",
                                  widget,
                                  gimp_standard_help_func,
                                  GIMP_HELP_IMAGE_PRINT_SIZE,
                                  image_print_size_callback,
                                  NULL);

  g_signal_connect_object (gdisp, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  gtk_widget_show (dialog);
}

void
image_scale_cmd_callback (GtkAction *action,
			  gpointer   data)
{
  ImageScaleDialog *dialog;
  GimpDisplay      *gdisp;
  GtkWidget        *widget;
  return_if_no_display (gdisp, data);
  return_if_no_widget (widget, data);

  dialog = image_scale_dialog_new (gdisp->gimage, gdisp,
                                   action_data_get_context (data),
                                   widget,
                                   image_scale_callback);

  g_signal_connect_object (gdisp, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog->dialog, G_CONNECT_SWAPPED);

  gtk_widget_show (dialog->dialog);
}

void
image_flip_cmd_callback (GtkAction *action,
                         gint       value,
                         gpointer   data)
{
  GimpDisplay  *gdisp;
  GimpProgress *progress;
  return_if_no_display (gdisp, data);

  progress = gimp_progress_start (GIMP_PROGRESS (gdisp),
                                  _("Flipping..."), FALSE);

  gimp_image_flip (gdisp->gimage, action_data_get_context (data),
                   (GimpOrientationType) value, progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (gdisp->gimage);
}

void
image_rotate_cmd_callback (GtkAction *action,
                           gint       value,
                           gpointer   data)
{
  GimpDisplay  *gdisp;
  GimpProgress *progress;
  return_if_no_display (gdisp, data);

  progress = gimp_progress_start (GIMP_PROGRESS (gdisp),
                                  _("Rotating..."), FALSE);

  gimp_image_rotate (gdisp->gimage, action_data_get_context (data),
                     (GimpRotationType) value, progress);

  if (progress)
    gimp_progress_end (progress);

  gimp_image_flush (gdisp->gimage);
}

void
image_crop_cmd_callback (GtkAction *action,
                         gpointer   data)
{
  GimpImage *gimage;
  gint       x1, y1, x2, y2;
  return_if_no_image (gimage, data);

  if (! gimp_channel_bounds (gimp_image_get_mask (gimage),
                             &x1, &y1, &x2, &y2))
    {
      g_message (_("Cannot crop because the current selection is empty."));
      return;
    }

  gimp_image_crop (gimage, action_data_get_context (data),
                   x1, y1, x2, y2, FALSE, TRUE);
  gimp_image_flush (gimage);
}

void
image_duplicate_cmd_callback (GtkAction *action,
			      gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  GimpImage        *new_gimage;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  new_gimage = gimp_image_duplicate (gdisp->gimage);

  gimp_create_display (new_gimage->gimp,
                       new_gimage,
                       shell->unit, shell->scale);

  g_object_unref (new_gimage);
}

void
image_merge_layers_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  ImageMergeLayersDialog *dialog;
  GimpImage              *gimage;
  GtkWidget              *widget;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  dialog = image_merge_layers_dialog_new (gimage,
                                          action_data_get_context (data),
                                          widget,
                                          image_merge_layers_type);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (image_merge_layers_response),
                    dialog);

  gtk_widget_show (dialog->dialog);
}

void
image_flatten_image_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  gimp_image_flatten (gimage, action_data_get_context (data));
  gimp_image_flush (gimage);
}

void
image_configure_grid_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  GimpImage        *gimage;
  return_if_no_display (gdisp, data);

  shell  = GIMP_DISPLAY_SHELL (gdisp->shell);
  gimage = gdisp->gimage;

  if (! shell->grid_dialog)
    {
      shell->grid_dialog = grid_dialog_new (gdisp->gimage, gdisp->shell);

      gtk_window_set_transient_for (GTK_WINDOW (shell->grid_dialog),
                                    GTK_WINDOW (gdisp->shell));
      gtk_window_set_destroy_with_parent (GTK_WINDOW (shell->grid_dialog),
                                          TRUE);

      g_object_add_weak_pointer (G_OBJECT (shell->grid_dialog),
                                 (gpointer *) &shell->grid_dialog);
    }

  gtk_window_present (GTK_WINDOW (shell->grid_dialog));
}


/*  private functions  */

static void
image_resize_callback (GtkWidget    *dialog,
                       GimpViewable *viewable,
                       gint          width,
                       gint          height,
                       gint          offset_x,
                       gint          offset_y,
                       gpointer      data)
{
  ImageResizeOptions *options = data;

  if (width > 0 && height > 0)
    {
      GimpImage    *image   = GIMP_IMAGE (viewable);
      GimpDisplay  *gdisp   = options->gdisp;
      GimpContext  *context = options->context;
      GimpProgress *progress;

      gtk_widget_destroy (dialog);

      if (width == image->width && height == image->height)
        return;

      progress = gimp_progress_start (GIMP_PROGRESS (gdisp),
                                      _("Resizing..."), FALSE);

      gimp_image_resize (image,
                         context,
                         width, height, offset_x, offset_y,
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
image_scale_callback (ImageScaleDialog  *dialog)
{
  GimpImage *image = dialog->gimage;

  if (dialog->width           == image->width           &&
      dialog->height          == image->height          &&
      dialog->xresolution     == image->xresolution     &&
      dialog->yresolution     == image->yresolution     &&
      dialog->resolution_unit == image->resolution_unit)
    return;

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_SCALE,
                               _("Scale Image"));

  gimp_image_set_resolution (image,
                             dialog->xresolution, dialog->yresolution);
  gimp_image_set_unit (image, dialog->resolution_unit);

  if (dialog->width != image->width || dialog->height != image->height)
    {
      if (dialog->width > 0 && dialog->height > 0)
        {
          GimpProgress *progress;

          progress = gimp_progress_start (GIMP_PROGRESS (dialog->gdisp),
                                          _("Scaling..."), FALSE);

          gimp_image_scale (image,
                            dialog->width,
                            dialog->height,
                            dialog->interpolation,
                            progress);

          if (progress)
            gimp_progress_end (progress);
        }
      else
        {
          g_warning ("Scale Error: "
                     "Both width and height must be greater than zero.");
        }
    }

  gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

static void
image_merge_layers_response (GtkWidget              *widget,
                             gint                    response_id,
                             ImageMergeLayersDialog *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      image_merge_layers_type = dialog->merge_type;

      gimp_image_merge_visible_layers (dialog->gimage,
                                       dialog->context,
                                       image_merge_layers_type);
      gimp_image_flush (dialog->gimage);
    }

  gtk_widget_destroy (widget);
}
