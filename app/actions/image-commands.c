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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpguiconfig.h"

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
#include "widgets/gimpviewabledialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "dialogs/convert-dialog.h"
#include "dialogs/dialogs.h"
#include "dialogs/grid-dialog.h"
#include "dialogs/image-new-dialog.h"
#include "dialogs/resize-dialog.h"

#include "actions.h"
#include "image-commands.h"

#include "gimp-intl.h"


typedef struct _ImageResizeOptions ImageResizeOptions;

struct _ImageResizeOptions
{
  ResizeDialog *dialog;

  GimpContext  *context;
  GimpDisplay  *gdisp;
  GimpImage    *gimage;
};


typedef struct _LayerMergeOptions LayerMergeOptions;

struct _LayerMergeOptions
{
  GtkWidget     *query_box;

  GimpContext   *context;
  GimpImage     *gimage;
  GimpMergeType  merge_type;
};


/*  local function prototypes  */

static void   image_resize_callback       (GtkWidget          *widget,
                                           gpointer            data);
static void   image_scale_callback        (GtkWidget          *widget,
                                           gpointer            data);
static void   image_scale_warn            (ImageResizeOptions *options,
                                           const gchar        *warning_title,
                                           const gchar        *warning_message);
static void   image_scale_warn_callback   (GtkWidget          *widget,
                                           gboolean            do_scale,
                                           gpointer            data);
static void   image_scale_implement       (ImageResizeOptions *options);
static void   image_merge_layers_response (GtkWidget          *widget,
                                           gint                response_id,
                                           LayerMergeOptions  *options);


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

      if (gimage)
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
  GimpDisplay        *gdisp;
  GimpImage          *gimage;
  return_if_no_display (gdisp, data);

  gimage = gdisp->gimage;

  options = g_new0 (ImageResizeOptions, 1);

  options->context = action_data_get_context (data);
  options->gdisp   = gdisp;
  options->gimage  = gimage;

  options->dialog =
    resize_dialog_new (GIMP_VIEWABLE (gimage), gdisp->shell,
                       RESIZE_DIALOG,
                       gimage->width,
                       gimage->height,
                       gimage->xresolution,
                       gimage->yresolution,
                       GIMP_DISPLAY_SHELL (gdisp->shell)->unit,
                       G_CALLBACK (image_resize_callback),
                       options);

  g_signal_connect_object (gdisp, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           options->dialog->shell,
                           G_CONNECT_SWAPPED);

  g_object_weak_ref (G_OBJECT (options->dialog->shell),
		     (GWeakNotify) g_free,
		     options);

  gtk_widget_show (options->dialog->shell);
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
image_scale_cmd_callback (GtkAction *action,
			  gpointer   data)
{
  ImageResizeOptions *options;
  GimpDisplay        *gdisp;
  GimpImage          *gimage;
  return_if_no_display (gdisp, data);

  gimage = gdisp->gimage;

  options = g_new0 (ImageResizeOptions, 1);

  options->context = action_data_get_context (data);
  options->gdisp   = gdisp;
  options->gimage  = gimage;

  options->dialog =
    resize_dialog_new (GIMP_VIEWABLE (gimage), gdisp->shell,
                       SCALE_DIALOG,
                       gimage->width,
                       gimage->height,
                       gimage->xresolution,
                       gimage->yresolution,
                       GIMP_DISPLAY_SHELL (gdisp->shell)->unit,
                       G_CALLBACK (image_scale_callback),
                       options);

  g_signal_connect_object (gdisp, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           options->dialog->shell,
                           G_CONNECT_SWAPPED);

  g_object_weak_ref (G_OBJECT (options->dialog->shell),
		     (GWeakNotify) g_free,
		     options);

  gtk_widget_show (options->dialog->shell);
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
  GimpImage        *gimage;
  GimpImage        *new_gimage;
  return_if_no_display (gdisp, data);

  shell  = GIMP_DISPLAY_SHELL (gdisp->shell);
  gimage = gdisp->gimage;

  new_gimage = gimp_image_duplicate (gimage);

  gimp_create_display (new_gimage->gimp,
                       new_gimage,
                       shell->unit, shell->scale);

  g_object_unref (new_gimage);
}

void
image_merge_layers_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  LayerMergeOptions *options;
  GimpImage         *gimage;
  GtkWidget         *widget;
  GtkWidget         *frame;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  options = g_new0 (LayerMergeOptions, 1);

  options->context    = action_data_get_context (data);
  options->gimage     = gimage;
  options->merge_type = GIMP_EXPAND_AS_NECESSARY;

  options->query_box =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (gimage),
                              _("Merge Layers"), "gimp-image-merge-layers",
                              GIMP_STOCK_MERGE_DOWN,
                              _("Layers Merge Options"),
                              widget,
                              gimp_standard_help_func,
                              GIMP_HELP_IMAGE_MERGE_LAYERS,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (image_merge_layers_response),
                    options);

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free,
		     options);

  frame = gimp_int_radio_group_new (TRUE, _("Final, Merged Layer should be:"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &options->merge_type, options->merge_type,

                                    _("Expanded as necessary"),
                                    GIMP_EXPAND_AS_NECESSARY, NULL,

                                    _("Clipped to image"),
                                    GIMP_CLIP_TO_IMAGE, NULL,

                                    _("Clipped to bottom layer"),
                                    GIMP_CLIP_TO_BOTTOM_LAYER, NULL,

                                    NULL);

  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     frame);
  gtk_widget_show (frame);

  gtk_widget_show (options->query_box);
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
image_resize_callback (GtkWidget *widget,
		       gpointer   data)
{
  ImageResizeOptions *options = data;

  gtk_widget_set_sensitive (options->dialog->shell, FALSE);

  if (options->dialog->width  > 0 &&
      options->dialog->height > 0)
    {
      GimpProgress *progress;

      progress = gimp_progress_start (GIMP_PROGRESS (options->gdisp),
                                      _("Resizing..."), FALSE);

      gimp_image_resize (options->gimage,
                         options->context,
			 options->dialog->width,
			 options->dialog->height,
			 options->dialog->offset_x,
			 options->dialog->offset_y,
                         progress);

      if (progress)
        gimp_progress_end (progress);

      gimp_image_flush (options->gimage);
    }
  else
    {
      g_message (_("Resize Error: Both width and height must be "
		   "greater than zero."));
    }

  gtk_widget_destroy (options->dialog->shell);
}

static void
image_scale_callback (GtkWidget *widget,
		      gpointer   data)
{
  ImageResizeOptions      *options = data;
  GimpImageScaleCheckType  scale_check;
  gint64                   max_memsize;
  gint64                   new_memsize;

  gtk_widget_set_sensitive (options->dialog->shell, FALSE);

  max_memsize =
    GIMP_GUI_CONFIG (options->gimage->gimp->config)->max_new_image_size;

  scale_check = gimp_image_scale_check (options->gimage,
                                        options->dialog->width,
                                        options->dialog->height,
                                        max_memsize,
                                        &new_memsize);
  switch (scale_check)
    {
    case GIMP_IMAGE_SCALE_TOO_BIG:
      {
        gchar *size_str;
        gchar *max_size_str;
        gchar *warning_message;

        size_str     = gimp_memsize_to_string (new_memsize);
        max_size_str = gimp_memsize_to_string (max_memsize);

        warning_message = g_strdup_printf
          (_("You are trying to create an image with a size of %s.\n\n"
             "Choose OK to create this image anyway.\n"
             "Choose Cancel if you did not intend to create such a "
             "large image.\n\n"
             "To prevent this dialog from appearing, increase the "
             "\"Maximum Image Size\" setting (currently %s) in the "
             "Preferences dialog."),
           size_str, max_size_str);

        g_free (size_str);
        g_free (max_size_str);

        image_scale_warn (options, _("Image exceeds maximum image size"),
                          warning_message);

        g_free (warning_message);
      }
      break;

    case GIMP_IMAGE_SCALE_TOO_SMALL:
      image_scale_warn (options, _("Layer Too Small"),
                        _("The chosen image size will shrink some layers "
                          "completely away. Is this what you want?"));
      break;

    case GIMP_IMAGE_SCALE_OK:
      /* If all is well, return directly after scaling image. */
      image_scale_implement (options);
      gtk_widget_destroy (options->dialog->shell);
      break;
    }
}

static void
image_scale_warn (ImageResizeOptions *options,
                  const gchar        *warning_title,
                  const gchar        *warning_message)
{
  GtkWidget *dialog;

  dialog = gimp_query_boolean_box (warning_title,
                                   options->dialog->shell,
                                   gimp_standard_help_func,
                                   GIMP_HELP_IMAGE_SCALE_WARNING,
                                   GTK_STOCK_DIALOG_QUESTION,
                                   warning_message,
                                   GTK_STOCK_OK, GTK_STOCK_CANCEL,
                                   G_OBJECT (options->dialog->shell),
                                   "destroy",
                                   image_scale_warn_callback,
                                   options);
  gtk_widget_show (dialog);
}

static void
image_scale_warn_callback (GtkWidget *widget,
			   gboolean   do_scale,
			   gpointer   data)
{
  ImageResizeOptions *options = data;

  if (do_scale)
    {
      image_scale_implement (options);
      gtk_widget_destroy (options->dialog->shell);
    }
  else
    {
      gtk_widget_set_sensitive (options->dialog->shell, TRUE);
    }
}

static void
image_scale_implement (ImageResizeOptions *options)
{
  GimpImage *gimage = options->gimage;

  if (options->dialog->resolution_x == gimage->xresolution     &&
      options->dialog->resolution_y == gimage->yresolution     &&
      options->dialog->unit         == gimage->resolution_unit &&
      options->dialog->width        == gimage->width           &&
      options->dialog->height       == gimage->height)
    return;

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_IMAGE_SCALE,
                               _("Scale Image"));

  gimp_image_set_resolution (gimage,
                             options->dialog->resolution_x,
                             options->dialog->resolution_y);
  gimp_image_set_unit (gimage, options->dialog->unit);

  if (options->dialog->width  != gimage->width ||
      options->dialog->height != gimage->height)
    {
      if (options->dialog->width  > 0 &&
	  options->dialog->height > 0)
	{
          GimpProgress *progress;

          progress = gimp_progress_start (GIMP_PROGRESS (options->gdisp),
                                          _("Scaling..."), FALSE);

	  gimp_image_scale (gimage,
			    options->dialog->width,
			    options->dialog->height,
                            options->dialog->interpolation,
                            progress);

          if (progress)
            gimp_progress_end (progress);
	}
      else
	{
	  g_message (_("Scale Error: Both width and height must be "
		       "greater than zero."));
	  return;
	}
    }

  gimp_image_undo_group_end (gimage);

  gimp_image_flush (gimage);
}

static void
image_merge_layers_response (GtkWidget         *widget,
                             gint               response_id,
                             LayerMergeOptions *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gimp_image_merge_visible_layers (options->gimage,
                                       options->context,
                                       options->merge_type);
      gimp_image_flush (options->gimage);
    }

  gtk_widget_destroy (options->query_box);
}
