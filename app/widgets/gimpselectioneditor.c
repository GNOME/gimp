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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#ifdef __GNUC__
#warning #include "tools/tools-types.h"
#endif

#include "tools/tools-types.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimpimage-mask-select.h"
#include "core/gimpimage-projection.h"
#include "core/gimptoolinfo.h"

#include "tools/gimpbycolorselecttool.h"
#include "tools/selection_options.h"
#include "tools/tool_manager.h"

#include "gimpselectioneditor.h"
#include "gimpdnd.h"
#include "gimppreview.h"

#include "libgimp/gimpintl.h"


static void   gimp_selection_editor_class_init (GimpSelectionEditorClass *klass);
static void   gimp_selection_editor_init       (GimpSelectionEditor      *selection_editor);

static void   gimp_selection_editor_destroy        (GtkObject           *object);

static void   gimp_selection_editor_abox_resized   (GtkWidget           *widget,
                                                    GtkAllocation       *allocation,
                                                    GimpSelectionEditor *editor);
static void   gimp_selection_editor_invert_clicked (GtkWidget           *widget,
                                                    GimpSelectionEditor *editor);
static void   gimp_selection_editor_all_clicked    (GtkWidget           *widget,
                                                    GimpSelectionEditor *editor);
static void   gimp_selection_editor_none_clicked   (GtkWidget           *widget,
                                                    GimpSelectionEditor *editor);
static void   gimp_selection_editor_save_clicked   (GtkWidget           *widget,
                                                    GimpSelectionEditor *editor);

static gboolean gimp_selection_preview_button_press(GtkWidget           *widget,
                                                    GdkEventButton      *bevent,
                                                    GimpSelectionEditor *editor);
static void   gimp_selection_editor_drop_color     (GtkWidget           *widget,
                                                    const GimpRGB       *color,
                                                    gpointer             data);

static void   gimp_selection_editor_mask_changed   (GimpImage           *gimage,
                                                    GimpSelectionEditor *editor);


static GimpEditorClass *parent_class = NULL;


GType
gimp_selection_editor_get_type (void)
{
  static GType editor_type = 0;

  if (! editor_type)
    {
      static const GTypeInfo editor_info =
      {
        sizeof (GimpSelectionEditorClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_selection_editor_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpSelectionEditor),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_selection_editor_init,
      };

      editor_type = g_type_register_static (GIMP_TYPE_EDITOR,
                                            "GimpSelectionEditor",
                                            &editor_info, 0);
    }

  return editor_type;
}

static void
gimp_selection_editor_class_init (GimpSelectionEditorClass* klass)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy = gimp_selection_editor_destroy;
}

static void
gimp_selection_editor_init (GimpSelectionEditor *selection_editor)
{
  GtkWidget       *frame;
  GtkWidget       *abox;
  /* FIXME: take value from GimpGuiConfig */ 
  GimpPreviewSize  nav_preview_size = GIMP_PREVIEW_SIZE_HUGE;

  selection_editor->gimage = NULL;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (selection_editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);

  gtk_widget_set_size_request (abox, nav_preview_size, nav_preview_size);

  g_signal_connect (abox, "size_allocate",
                    G_CALLBACK (gimp_selection_editor_abox_resized),
                    selection_editor);

  selection_editor->preview = gimp_preview_new_by_type (GIMP_TYPE_DRAWABLE,
                                                        nav_preview_size,
                                                        0,
                                                        FALSE);
  gtk_container_add (GTK_CONTAINER (abox), selection_editor->preview);
  gtk_widget_show (selection_editor->preview);

  g_signal_connect (selection_editor->preview, "button_press_event",
                    G_CALLBACK (gimp_selection_preview_button_press),
                    selection_editor);

  gimp_dnd_color_dest_add (selection_editor->preview,
                           gimp_selection_editor_drop_color,
                           selection_editor);

  selection_editor->invert_button =
    gimp_editor_add_button (GIMP_EDITOR (selection_editor),
                            GIMP_STOCK_INVERT,
                            _("Invert Selection"), NULL,
                            G_CALLBACK (gimp_selection_editor_invert_clicked),
                            NULL,
                            selection_editor);

  selection_editor->all_button =
    gimp_editor_add_button (GIMP_EDITOR (selection_editor),
                            GIMP_STOCK_ALL,
                            _("Select All"), NULL,
                            G_CALLBACK (gimp_selection_editor_all_clicked),
                            NULL,
                            selection_editor);

  selection_editor->none_button =
    gimp_editor_add_button (GIMP_EDITOR (selection_editor),
                            GIMP_STOCK_NONE,
                            _("Select None"), NULL,
                            G_CALLBACK (gimp_selection_editor_none_clicked),
                            NULL,
                            selection_editor);

  selection_editor->save_button =
    gimp_editor_add_button (GIMP_EDITOR (selection_editor),
                            GIMP_STOCK_SELECTION_TO_CHANNEL,
                            _("Save Selection to Channel"), NULL,
                            G_CALLBACK (gimp_selection_editor_save_clicked),
                            NULL,
                            selection_editor);

  gtk_widget_set_sensitive (GTK_WIDGET (selection_editor), FALSE);
}

static void
gimp_selection_editor_destroy (GtkObject *object)
{
  GimpSelectionEditor *editor;

  editor = GIMP_SELECTION_EDITOR (object);

  if (editor->gimage)
    gimp_selection_editor_set_image (editor, NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


/*  public functions  */

#define PREVIEW_WIDTH       256
#define PREVIEW_HEIGHT      256

GtkWidget *
gimp_selection_editor_new (GimpImage *gimage)
{
  GimpSelectionEditor *editor;

  g_return_val_if_fail (! gimage || GIMP_IS_IMAGE (gimage), NULL);

  editor = g_object_new (GIMP_TYPE_SELECTION_EDITOR, NULL);

  gimp_selection_editor_set_image (editor, gimage);

  return GTK_WIDGET (editor);
}

void
gimp_selection_editor_set_image (GimpSelectionEditor *editor,
                                 GimpImage           *gimage)
{
  g_return_if_fail (GIMP_IS_SELECTION_EDITOR (editor));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));

  if (gimage == editor->gimage)
    return;

  if (editor->gimage)
    {
      g_signal_handlers_disconnect_by_func (editor->gimage,
					    gimp_selection_editor_mask_changed,
					    editor);
    }
  else if (gimage)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
    }

  editor->gimage = gimage;

  if (gimage)
    {
      g_signal_connect (gimage, "mask_changed",
			G_CALLBACK (gimp_selection_editor_mask_changed),
			editor);

      gimp_preview_set_viewable (GIMP_PREVIEW (editor->preview),
                                 GIMP_VIEWABLE (gimp_image_get_mask (gimage)));
    }
  else
    {
      gimp_preview_set_viewable (GIMP_PREVIEW (editor->preview), NULL);
      gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
    }
}

static void
gimp_selection_editor_abox_resized (GtkWidget           *widget,
                                    GtkAllocation       *allocation,
                                    GimpSelectionEditor *editor)
{
  GimpPreview *preview;

  preview = GIMP_PREVIEW (editor->preview);

  if (! preview->viewable)
    return;

  if (preview->width  > allocation->width  ||
      preview->height > allocation->height ||
      (preview->width  != allocation->width &&
       preview->height != allocation->height))
    {
      gint     width;
      gint     height;
      gboolean dummy;

      gimp_preview_calc_size (preview,
                              editor->gimage->width,
                              editor->gimage->height,
                              MIN (allocation->width,  GIMP_PREVIEW_MAX_SIZE),
                              MIN (allocation->height, GIMP_PREVIEW_MAX_SIZE),
                              editor->gimage->xresolution,
                              editor->gimage->yresolution,
                              &width,
                              &height,
                              &dummy);

      if (width > allocation->width)
        {
          height = height * allocation->width / width;
          width  = width  * allocation->width / width;
        }
      else if (height > allocation->height)
        {
          width  = width  * allocation->height / height;
          height = height * allocation->height / height;
        }

      gimp_preview_set_size_full (preview, width, height, preview->border_width);
    }
}

static void
gimp_selection_editor_invert_clicked (GtkWidget           *widget,
                                      GimpSelectionEditor *editor)
{
  if (editor->gimage)
    {
      gimp_image_mask_invert (editor->gimage);
      gimp_image_flush (editor->gimage);
    }
}

static void
gimp_selection_editor_all_clicked (GtkWidget           *widget,
                                   GimpSelectionEditor *editor)
{
  if (editor->gimage)
    {
      gimp_image_mask_all (editor->gimage);
      gimp_image_flush (editor->gimage);
    }
}

static void
gimp_selection_editor_none_clicked (GtkWidget           *widget,
                                    GimpSelectionEditor *editor)
{
  if (editor->gimage)
    {
      gimp_image_mask_clear (editor->gimage);
      gimp_image_flush (editor->gimage);
    }
}

static void
gimp_selection_editor_save_clicked (GtkWidget           *widget,
                                    GimpSelectionEditor *editor)
{
  if (editor->gimage)
    {
      gimp_image_mask_save (editor->gimage);
    }
}

static gboolean
gimp_selection_preview_button_press (GtkWidget           *widget,
                                     GdkEventButton      *bevent,
                                     GimpSelectionEditor *editor)
{
  GimpToolInfo     *tool_info;
  SelectionOptions *sel_options;
  GimpDrawable     *drawable;
  SelectOps         operation = SELECTION_REPLACE;
  gint              x, y;
  guchar           *col;
  GimpRGB           color;

  if (! editor->gimage)
    return TRUE;

  tool_info = tool_manager_get_info_by_type (editor->gimage->gimp,
                                             GIMP_TYPE_BY_COLOR_SELECT_TOOL);

  sel_options = (SelectionOptions *) tool_info->tool_options;

  drawable = gimp_image_active_drawable (editor->gimage);

  if (! drawable)
    return TRUE;

  if (bevent->state & GDK_SHIFT_MASK)
    {
      if (bevent->state & GDK_CONTROL_MASK)
        {
          operation = SELECTION_INTERSECT;
        }
      else
        {
          operation = SELECTION_ADD;
        }
    }
  else if (bevent->state & GDK_CONTROL_MASK)
    {
      operation = SELECTION_SUBTRACT;
    }

  if (sel_options->sample_merged)
    {
      x = editor->gimage->width  * bevent->x / editor->preview->allocation.width;
      y = editor->gimage->height * bevent->y / editor->preview->allocation.height;

      if (x < 0 || y < 0 ||
          x >= editor->gimage->width ||
          y >= editor->gimage->height)
	return TRUE;

      col = gimp_image_projection_get_color_at (editor->gimage, x, y);
    }
  else
    {
      gint offx, offy;

      gimp_drawable_offsets (drawable, &offx, &offy);

      x = (gimp_drawable_width (drawable) * bevent->x /
           editor->preview->requisition.width - offx);
      y = (gimp_drawable_height (drawable) * bevent->y /
           editor->preview->requisition.height - offy);

      if (x < 0 || y < 0 ||
	  x >= gimp_drawable_width (drawable) ||
          y >= gimp_drawable_height (drawable))
	return TRUE;

      col = gimp_drawable_get_color_at (drawable, x, y);
    }

  gimp_rgba_set_uchar (&color, col[0], col[1], col[2], col[3]);

  g_free (col);

  gimp_image_mask_select_by_color (editor->gimage, drawable,
                                   sel_options->sample_merged,
                                   &color,
                                   sel_options->threshold,
                                   sel_options->select_transparent,
                                   operation,
                                   sel_options->antialias,
                                   sel_options->feather,
                                   sel_options->feather_radius,
                                   sel_options->feather_radius);

  gimp_image_flush (editor->gimage);

  return TRUE;
}

static void
gimp_selection_editor_drop_color (GtkWidget     *widget,
                                  const GimpRGB *color,
                                  gpointer       data)
{
  GimpToolInfo        *tool_info;
  SelectionOptions    *sel_options;
  GimpDrawable        *drawable;
  GimpSelectionEditor *editor;

  editor = GIMP_SELECTION_EDITOR (data);

  if (! editor->gimage)
    return;

  tool_info = tool_manager_get_info_by_type (editor->gimage->gimp,
                                             GIMP_TYPE_BY_COLOR_SELECT_TOOL);

  sel_options = (SelectionOptions *) tool_info->tool_options;

  drawable = gimp_image_active_drawable (editor->gimage);

  if (! drawable)
    return;

  gimp_image_mask_select_by_color (editor->gimage, drawable,
                                   sel_options->sample_merged,
                                   color,
                                   sel_options->threshold,
                                   sel_options->select_transparent,
                                   sel_options->op,
                                   sel_options->antialias,
                                   sel_options->feather,
                                   sel_options->feather_radius,
                                   sel_options->feather_radius);

  gimp_image_flush (editor->gimage);
}

static void
gimp_selection_editor_mask_changed (GimpImage           *gimage,
                                    GimpSelectionEditor *editor)
{
  gimp_preview_update (GIMP_PREVIEW (editor->preview));
}
