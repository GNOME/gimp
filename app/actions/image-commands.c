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

#include "libgimpbase/gimputils.h"
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

#include "widgets/gimpdock.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpprogress.h"

#include "gui/convert-dialog.h"
#include "gui/grid-dialog.h"
#include "gui/resize-dialog.h"

#include "actions.h"
#include "image-commands.h"

#include "gimp-intl.h"


typedef struct
{
  ResizeDialog *dialog;
  GimpDisplay  *gdisp;
  GimpImage    *gimage;
} ImageResize;


/*  local function prototypes  */

static void     image_resize_callback     (GtkWidget   *widget,
				           gpointer     data);
static void     image_scale_callback      (GtkWidget   *widget,
				           gpointer     data);
static void     image_scale_warn          (ImageResize *image_scale,
                                           const gchar *warning_title,
                                           const gchar *warning_message);
static void     image_scale_warn_callback (GtkWidget   *widget,
				           gboolean     do_scale,
				           gpointer     data);
static void     image_scale_implement     (ImageResize *image_scale);


/*  public functions  */

void
image_convert_rgb_cmd_callback (GtkAction *action,
				gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  gimp_image_convert (gimage, GIMP_RGB, 0, 0, FALSE, FALSE, 0, NULL);
  gimp_image_flush (gimage);
}

void
image_convert_grayscale_cmd_callback (GtkAction *action,
				      gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  gimp_image_convert (gimage, GIMP_GRAY, 0, 0, FALSE, FALSE, 0, NULL);
  gimp_image_flush (gimage);
}

void
image_convert_indexed_cmd_callback (GtkAction *action,
				    gpointer   data)
{
  GimpImage *gimage;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  dialog = convert_dialog_new (gimage, widget);
  gtk_widget_show (dialog);
}

void
image_resize_cmd_callback (GtkAction *action,
			   gpointer   data)
{
  GimpDisplay *gdisp;
  GimpImage   *gimage;
  ImageResize *image_resize;
  return_if_no_display (gdisp, data);

  gimage = gdisp->gimage;

  image_resize = g_new0 (ImageResize, 1);

  image_resize->gdisp  = gdisp;
  image_resize->gimage = gimage;

  image_resize->dialog =
    resize_dialog_new (GIMP_VIEWABLE (gimage), gdisp->shell,
                       RESIZE_DIALOG,
                       gimage->width,
                       gimage->height,
                       gimage->xresolution,
                       gimage->yresolution,
                       gimage->unit,
                       GIMP_DISPLAY_SHELL (gdisp->shell)->dot_for_dot,
                       G_CALLBACK (image_resize_callback),
                       image_resize);

  g_signal_connect_object (gdisp, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           image_resize->dialog->shell,
                           G_CONNECT_SWAPPED);

  g_object_weak_ref (G_OBJECT (image_resize->dialog->shell),
		     (GWeakNotify) g_free,
		     image_resize);

  gtk_widget_show (image_resize->dialog->shell);
}

void
image_scale_cmd_callback (GtkAction *action,
			  gpointer   data)
{
  GimpDisplay *gdisp;
  GimpImage   *gimage;
  ImageResize *image_scale;
  return_if_no_display (gdisp, data);

  gimage = gdisp->gimage;

  image_scale = g_new0 (ImageResize, 1);

  image_scale->gdisp  = gdisp;
  image_scale->gimage = gimage;

  image_scale->dialog =
    resize_dialog_new (GIMP_VIEWABLE (gimage), gdisp->shell,
                       SCALE_DIALOG,
                       gimage->width,
                       gimage->height,
                       gimage->xresolution,
                       gimage->yresolution,
                       gimage->unit,
                       GIMP_DISPLAY_SHELL (gdisp->shell)->dot_for_dot,
                       G_CALLBACK (image_scale_callback),
                       image_scale);

  g_signal_connect_object (gdisp, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           image_scale->dialog->shell,
                           G_CONNECT_SWAPPED);

  g_object_weak_ref (G_OBJECT (image_scale->dialog->shell),
		     (GWeakNotify) g_free,
		     image_scale);

  gtk_widget_show (image_scale->dialog->shell);
}

void
image_flip_cmd_callback (GtkAction *action,
                         gint       value,
                         gpointer   data)
{
  GimpDisplay  *gdisp;
  GimpProgress *progress;
  return_if_no_display (gdisp, data);

  progress = gimp_progress_start (gdisp, _("Flipping..."), TRUE, NULL, NULL);

  gimp_image_flip (gdisp->gimage, gimp_get_user_context (gdisp->gimage->gimp),
                   (GimpOrientationType) value,
                   gimp_progress_update_and_flush, progress);

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

  progress = gimp_progress_start (gdisp, _("Rotating..."), TRUE, NULL, NULL);

  gimp_image_rotate (gdisp->gimage, gimp_get_user_context (gdisp->gimage->gimp),
                     (GimpRotationType) value,
                     gimp_progress_update_and_flush, progress);

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

  gimp_image_crop (gimage, gimp_get_user_context (gimage->gimp),
                   x1, y1, x2, y2, FALSE, TRUE);
  gimp_image_flush (gimage);
}

void
image_duplicate_cmd_callback (GtkAction *action,
			      gpointer   data)
{
  GimpImage *gimage;
  GimpImage *new_gimage;
  return_if_no_image (gimage, data);

  new_gimage = gimp_image_duplicate (gimage);

  gimp_create_display (new_gimage->gimp, new_gimage, 1.0);

  g_object_unref (new_gimage);
}

void
image_merge_layers_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpImage *gimage;
  GtkWidget *widget;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  image_layers_merge_query (gimage, TRUE, widget);
}

void
image_flatten_image_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  gimp_image_flatten (gimage, gimp_get_user_context (gimage->gimp));
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


/****************************/
/*  The layer merge dialog  */
/****************************/

typedef struct _LayerMergeOptions LayerMergeOptions;

struct _LayerMergeOptions
{
  GtkWidget     *query_box;
  GimpImage     *gimage;
  gboolean       merge_visible;
  GimpMergeType  merge_type;
};

static void
image_layers_merge_query_response (GtkWidget         *widget,
                                   gint               response_id,
                                   LayerMergeOptions *options)
{
  GimpImage *gimage = options->gimage;

  if (! gimage)
    return;

  if (response_id == GTK_RESPONSE_OK)
    {
      if (options->merge_visible)
        gimp_image_merge_visible_layers (gimage,
                                         gimp_get_user_context (gimage->gimp),
                                         options->merge_type);

      gimp_image_flush (gimage);
    }

  gtk_widget_destroy (options->query_box);
}

void
image_layers_merge_query (GimpImage   *gimage,
                          /*  if FALSE, anchor active layer  */
                          gboolean     merge_visible,
                          GtkWidget   *parent)
{
  LayerMergeOptions *options;
  GtkWidget         *frame;

  /*  The new options structure  */
  options = g_new (LayerMergeOptions, 1);
  options->gimage        = gimage;
  options->merge_visible = merge_visible;
  options->merge_type    = GIMP_EXPAND_AS_NECESSARY;

  /* The dialog  */
  options->query_box =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (gimage),
                              _("Merge Layers"), "gimp-image-merge-layers",
                              GIMP_STOCK_MERGE_DOWN,
                              _("Layers Merge Options"),
                              parent,
                              gimp_standard_help_func,
                              GIMP_HELP_IMAGE_MERGE_LAYERS,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (image_layers_merge_query_response),
                    options);

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free,
		     options);

  frame =
    gimp_int_radio_group_new (TRUE,
                              merge_visible ?
                                _("Final, Merged Layer should be:") :
                                _("Final, Anchored Layer should be:"),
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


/*  private functions  */

static void
image_resize_callback (GtkWidget *widget,
		       gpointer   data)
{
  ImageResize *image_resize = data;

  g_assert (image_resize != NULL);
  g_assert (image_resize->gimage != NULL);

  gtk_widget_set_sensitive (image_resize->dialog->shell, FALSE);

  if (image_resize->dialog->width  > 0 &&
      image_resize->dialog->height > 0)
    {
      GimpProgress *progress;

      progress = gimp_progress_start (image_resize->gdisp,
                                      _("Resizing..."),
                                      TRUE, NULL, NULL);

      gimp_image_resize (image_resize->gimage,
                         gimp_get_user_context (image_resize->gimage->gimp),
			 image_resize->dialog->width,
			 image_resize->dialog->height,
			 image_resize->dialog->offset_x,
			 image_resize->dialog->offset_y,
                         gimp_progress_update_and_flush, progress);

      gimp_progress_end (progress);

      gimp_image_flush (image_resize->gimage);
    }
  else
    {
      g_message (_("Resize Error: Both width and height must be "
		   "greater than zero."));
    }

  gtk_widget_destroy (image_resize->dialog->dialog_shell);
}

static void
image_scale_callback (GtkWidget *widget,
		      gpointer   data)
{
  ImageResize             *image_scale = data;
  GimpImageScaleCheckType  scale_check;
  gint64                   new_memsize;
  gchar                   *warning_message;

  g_assert (image_scale != NULL);
  g_assert (image_scale->gimage != NULL);

  gtk_widget_set_sensitive (image_scale->dialog->shell, FALSE);

  scale_check = gimp_image_scale_check (image_scale->gimage,
                                        image_scale->dialog->width,
                                        image_scale->dialog->height,
                                        &new_memsize);
  switch (scale_check)
    {
    case GIMP_IMAGE_SCALE_TOO_BIG:
      {
        gchar *size_str;
        gchar *max_size_str;

        size_str     = gimp_memsize_to_string (new_memsize);
        max_size_str = gimp_memsize_to_string
          (GIMP_GUI_CONFIG (image_scale->gimage->gimp->config)->max_new_image_size);

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

        image_scale_warn (image_scale, _("Image exceeds maximum image size"),
                          warning_message);

        g_free (warning_message);
      }
      break;

    case GIMP_IMAGE_SCALE_TOO_SMALL:
      warning_message = _("The chosen image size will shrink "
                          "some layers completely away. "
                          "Is this what you want?");

      image_scale_warn (image_scale, _("Layer Too Small"),
                        warning_message);
      break;

    case GIMP_IMAGE_SCALE_OK:
      /* If all is well, return directly after scaling image. */
      image_scale_implement (image_scale);
      gtk_widget_destroy (image_scale->dialog->shell);
      break;
    }
}

static void
image_scale_warn (ImageResize *image_scale,
                  const gchar *warning_title,
                  const gchar *warning_message)
{
  GtkWidget *dialog;

  dialog = gimp_query_boolean_box (warning_title,
                                   image_scale->dialog->shell,
                                   gimp_standard_help_func,
                                   GIMP_HELP_IMAGE_SCALE_WARNING,
                                   GTK_STOCK_DIALOG_QUESTION,
                                   warning_message,
                                   GTK_STOCK_OK, GTK_STOCK_CANCEL,
                                   G_OBJECT (image_scale->dialog->shell),
                                   "destroy",
                                   image_scale_warn_callback,
                                   image_scale);
  gtk_widget_show (dialog);
}

static void
image_scale_warn_callback (GtkWidget *widget,
			   gboolean   do_scale,
			   gpointer   data)
{
  ImageResize *image_scale = data;

  if (do_scale) /* User doesn't mind losing layers or
                 * creating huge image... */
    {
      image_scale_implement (image_scale);
      gtk_widget_destroy (image_scale->dialog->shell);
    }
  else
    {
      gtk_widget_set_sensitive (image_scale->dialog->shell, TRUE);
    }
}

static void
image_scale_implement (ImageResize *image_scale)
{
  GimpImage *gimage;

  g_assert (image_scale != NULL);
  g_assert (image_scale->gimage != NULL);

  gimage = image_scale->gimage;

  if (image_scale->dialog->resolution_x == gimage->xresolution &&
      image_scale->dialog->resolution_y == gimage->yresolution &&
      image_scale->dialog->unit         == gimage->unit        &&
      image_scale->dialog->width        == gimage->width       &&
      image_scale->dialog->height       == gimage->height)
    return;

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_IMAGE_SCALE,
                               _("Scale Image"));

  gimp_image_set_resolution (gimage,
                             image_scale->dialog->resolution_x,
                             image_scale->dialog->resolution_y);
  gimp_image_set_unit (gimage, image_scale->dialog->unit);

  if (image_scale->dialog->width  != gimage->width ||
      image_scale->dialog->height != gimage->height)
    {
      if (image_scale->dialog->width  > 0 &&
	  image_scale->dialog->height > 0)
	{
          GimpProgress *progress;

          progress = gimp_progress_start (image_scale->gdisp,
                                          _("Scaling..."),
                                          TRUE, NULL, NULL);

	  gimp_image_scale (gimage,
			    image_scale->dialog->width,
			    image_scale->dialog->height,
                            image_scale->dialog->interpolation,
                            gimp_progress_update_and_flush, progress);

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
