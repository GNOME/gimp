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

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpchannel-select.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-projection.h"
#include "core/gimpselection.h"
#include "core/gimptoolinfo.h"

#include "tools/gimpselectionoptions.h"

#include "gimpselectioneditor.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimppreview.h"
#include "gimppreviewrenderer.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_selection_editor_class_init (GimpSelectionEditorClass *klass);
static void   gimp_selection_editor_init       (GimpSelectionEditor      *selection_editor);

static void   gimp_selection_editor_set_image      (GimpImageEditor     *editor,
                                                    GimpImage           *gimage);

static void   gimp_selection_editor_invert_clicked (GtkWidget           *widget,
                                                    GimpImageEditor     *editor);
static void   gimp_selection_editor_all_clicked    (GtkWidget           *widget,
                                                    GimpImageEditor     *editor);
static void   gimp_selection_editor_none_clicked   (GtkWidget           *widget,
                                                    GimpImageEditor     *editor);
static void   gimp_selection_editor_save_clicked   (GtkWidget           *widget,
                                                    GimpImageEditor     *editor);
static void   gimp_selection_editor_path_clicked   (GtkWidget           *widget,
                                                    GimpImageEditor     *editor);
static void   gimp_selection_editor_path_extended_clicked (GtkWidget    *widget,
                                                    guint                state,
                                                    GimpImageEditor     *editor);
static void   gimp_selection_editor_stroke_clicked (GtkWidget           *widget,
                                                    GimpImageEditor     *editor);

static gboolean gimp_selection_preview_button_press(GtkWidget           *widget,
                                                    GdkEventButton      *bevent,
                                                    GimpSelectionEditor *editor);
static void   gimp_selection_editor_drop_color     (GtkWidget           *widget,
                                                    const GimpRGB       *color,
                                                    gpointer             data);

static void   gimp_selection_editor_mask_changed   (GimpImage           *gimage,
                                                    GimpSelectionEditor *editor);


static GimpImageEditorClass *parent_class = NULL;


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

      editor_type = g_type_register_static (GIMP_TYPE_IMAGE_EDITOR,
                                            "GimpSelectionEditor",
                                            &editor_info, 0);
    }

  return editor_type;
}

static void
gimp_selection_editor_class_init (GimpSelectionEditorClass* klass)
{
  GimpImageEditorClass *image_editor_class;

  image_editor_class = GIMP_IMAGE_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  image_editor_class->set_image = gimp_selection_editor_set_image;
}

static void
gimp_selection_editor_init (GimpSelectionEditor *selection_editor)
{
  GtkWidget *frame;
  gchar     *str;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (selection_editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  selection_editor->selection_to_vectors_func = NULL;

  selection_editor->preview = gimp_preview_new_by_types (GIMP_TYPE_PREVIEW,
                                                         GIMP_TYPE_SELECTION,
                                                         GIMP_PREVIEW_SIZE_HUGE,
                                                         0, TRUE);
  gtk_widget_set_size_request (selection_editor->preview,
                               GIMP_PREVIEW_SIZE_HUGE, GIMP_PREVIEW_SIZE_HUGE);
  gimp_preview_set_expand (GIMP_PREVIEW (selection_editor->preview), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), selection_editor->preview);
  gtk_widget_show (selection_editor->preview);

  g_signal_connect (selection_editor->preview, "button_press_event",
                    G_CALLBACK (gimp_selection_preview_button_press),
                    selection_editor);

  gimp_dnd_color_dest_add (selection_editor->preview,
                           gimp_selection_editor_drop_color,
                           selection_editor);

  selection_editor->all_button =
    gimp_editor_add_button (GIMP_EDITOR (selection_editor),
                            GIMP_STOCK_SELECTION_ALL, _("Select All"),
                            GIMP_HELP_SELECTION_ALL,
                            G_CALLBACK (gimp_selection_editor_all_clicked),
                            NULL,
                            selection_editor);

  selection_editor->none_button =
    gimp_editor_add_button (GIMP_EDITOR (selection_editor),
                            GIMP_STOCK_SELECTION_NONE, _("Select None"),
                            GIMP_HELP_SELECTION_NONE,
                            G_CALLBACK (gimp_selection_editor_none_clicked),
                            NULL,
                            selection_editor);

  selection_editor->invert_button =
    gimp_editor_add_button (GIMP_EDITOR (selection_editor),
                            GIMP_STOCK_INVERT, _("Invert Selection"),
                            GIMP_HELP_SELECTION_INVERT,
                            G_CALLBACK (gimp_selection_editor_invert_clicked),
                            NULL,
                            selection_editor);

  selection_editor->save_button =
    gimp_editor_add_button (GIMP_EDITOR (selection_editor),
                            GIMP_STOCK_SELECTION_TO_CHANNEL,
                            _("Save Selection to Channel"),
                            GIMP_HELP_SELECTION_TO_CHANNEL,
                            G_CALLBACK (gimp_selection_editor_save_clicked),
                            NULL,
                            selection_editor);

  str = g_strdup_printf (_("Selection to Path\n"
                           "%s  Advanced Options"),
                         gimp_get_mod_name_shift ());

  selection_editor->path_button =
    gimp_editor_add_button (GIMP_EDITOR (selection_editor),
                            GIMP_STOCK_SELECTION_TO_PATH, str,
                            GIMP_HELP_SELECTION_TO_PATH,
                            G_CALLBACK (gimp_selection_editor_path_clicked),
                            G_CALLBACK (gimp_selection_editor_path_extended_clicked),
                            selection_editor);

  g_free (str);

  selection_editor->stroke_button =
    gimp_editor_add_button (GIMP_EDITOR (selection_editor),
                            GIMP_STOCK_SELECTION_STROKE,
                            _("Stroke Selection"),
                            GIMP_HELP_SELECTION_STROKE,
                            G_CALLBACK (gimp_selection_editor_stroke_clicked),
                            NULL,
                            selection_editor);


  gtk_widget_set_sensitive (GTK_WIDGET (selection_editor), FALSE);
}

static void
gimp_selection_editor_set_image (GimpImageEditor *image_editor,
                                 GimpImage       *gimage)
{
  GimpSelectionEditor *editor;

  editor = GIMP_SELECTION_EDITOR (image_editor);

  if (image_editor->gimage)
    {
      g_signal_handlers_disconnect_by_func (image_editor->gimage,
					    gimp_selection_editor_mask_changed,
					    editor);
    }

  GIMP_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, gimage);

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
    }
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

  gimp_preview_renderer_set_background (GIMP_PREVIEW (editor->preview)->renderer,
                                        GIMP_STOCK_TEXTURE);

  if (gimage)
    gimp_image_editor_set_image (GIMP_IMAGE_EDITOR (editor), gimage);

  return GTK_WIDGET (editor);
}

static void
gimp_selection_editor_invert_clicked (GtkWidget       *widget,
                                      GimpImageEditor *editor)
{
  if (editor->gimage)
    {
      gimp_channel_invert (gimp_image_get_mask (editor->gimage), TRUE);
      gimp_image_flush (editor->gimage);
    }
}

static void
gimp_selection_editor_all_clicked (GtkWidget       *widget,
                                   GimpImageEditor *editor)
{
  if (editor->gimage)
    {
      gimp_channel_all (gimp_image_get_mask (editor->gimage), TRUE);
      gimp_image_flush (editor->gimage);
    }
}

static void
gimp_selection_editor_none_clicked (GtkWidget       *widget,
                                    GimpImageEditor *editor)
{
  if (editor->gimage)
    {
      gimp_channel_clear (gimp_image_get_mask (editor->gimage), NULL, TRUE);
      gimp_image_flush (editor->gimage);
    }
}

static void
gimp_selection_editor_save_clicked (GtkWidget       *widget,
                                    GimpImageEditor *editor)
{
  if (editor->gimage)
    {
      gimp_selection_save (gimp_image_get_mask (editor->gimage));
    }
}

static void
gimp_selection_editor_path_clicked (GtkWidget       *widget,
                                    GimpImageEditor *editor)
{
  gimp_selection_editor_path_extended_clicked (widget, 0, editor);
}

static void
gimp_selection_editor_path_extended_clicked (GtkWidget       *widget,
                                             guint            state,
                                             GimpImageEditor *editor)
{
  if (editor->gimage)
    {
      GimpSelectionEditor *sel_editor = GIMP_SELECTION_EDITOR (editor);

      if (sel_editor->selection_to_vectors_func)
        sel_editor->selection_to_vectors_func (editor->gimage,
                                               (state & GDK_SHIFT_MASK) != 0);
    }
}

static void
gimp_selection_editor_stroke_clicked (GtkWidget       *widget,
				      GimpImageEditor *editor)
{
  if (editor->gimage)
    {
      GimpSelectionEditor *sel_editor = GIMP_SELECTION_EDITOR (editor);

      if (sel_editor->stroke_item_func)
        sel_editor->stroke_item_func (GIMP_ITEM (gimp_image_get_mask (editor->gimage)),
                                      GTK_WIDGET (editor));
    }
}

static gboolean
gimp_selection_preview_button_press (GtkWidget           *widget,
                                     GdkEventButton      *bevent,
                                     GimpSelectionEditor *editor)
{
  GimpImageEditor      *image_editor;
  GimpPreviewRenderer  *renderer;
  GimpToolInfo         *tool_info;
  GimpSelectionOptions *options;
  GimpDrawable         *drawable;
  SelectOps             operation = SELECTION_REPLACE;
  gint                  x, y;
  guchar               *col;
  GimpRGB               color;

  image_editor = GIMP_IMAGE_EDITOR (editor);

  if (! image_editor->gimage)
    return TRUE;

  renderer = GIMP_PREVIEW (editor->preview)->renderer;

  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (image_editor->gimage->gimp->tool_info_list,
                                      "gimp-by-color-select-tool");

  if (! tool_info)
    return TRUE;

  options = GIMP_SELECTION_OPTIONS (tool_info->tool_options);

  drawable = gimp_image_active_drawable (image_editor->gimage);

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

  x = image_editor->gimage->width  * bevent->x / renderer->width;
  y = image_editor->gimage->height * bevent->y / renderer->height;

  if (options->sample_merged)
    {
      if (x < 0 || y < 0 ||
          x >= image_editor->gimage->width ||
          y >= image_editor->gimage->height)
	return TRUE;

      col = gimp_image_projection_get_color_at (image_editor->gimage, x, y);
    }
  else
    {
      GimpItem *item;
      gint      off_x, off_y;

      item = GIMP_ITEM (drawable);

      gimp_item_offsets (item, &off_x, &off_y);

      x -= off_x;
      y -= off_y;

      if (x < 0 || y < 0               ||
	  x >= gimp_item_width  (item) ||
          y >= gimp_item_height (item))
	return TRUE;

      col = gimp_drawable_get_color_at (drawable, x, y);
    }

  gimp_rgba_set_uchar (&color, col[0], col[1], col[2], col[3]);

  g_free (col);

  gimp_channel_select_by_color (gimp_image_get_mask (image_editor->gimage),
                                drawable,
                                options->sample_merged,
                                &color,
                                options->threshold,
                                options->select_transparent,
                                operation,
                                options->antialias,
                                options->feather,
                                options->feather_radius,
                                options->feather_radius);
  gimp_image_flush (image_editor->gimage);

  return TRUE;
}

static void
gimp_selection_editor_drop_color (GtkWidget     *widget,
                                  const GimpRGB *color,
                                  gpointer       data)
{
  GimpToolInfo         *tool_info;
  GimpSelectionOptions *options;
  GimpDrawable         *drawable;
  GimpImageEditor      *editor;

  editor = GIMP_IMAGE_EDITOR (data);

  if (! editor->gimage)
    return;

  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (editor->gimage->gimp->tool_info_list,
                                      "gimp-by-color-select-tool");

  if (! tool_info)
    return;

  options = GIMP_SELECTION_OPTIONS (tool_info->tool_options);

  drawable = gimp_image_active_drawable (editor->gimage);

  if (! drawable)
    return;

  gimp_channel_select_by_color (gimp_image_get_mask (editor->gimage),
                                drawable,
                                options->sample_merged,
                                color,
                                options->threshold,
                                options->select_transparent,
                                options->operation,
                                options->antialias,
                                options->feather,
                                options->feather_radius,
                                options->feather_radius);
  gimp_image_flush (editor->gimage);
}

static void
gimp_selection_editor_mask_changed (GimpImage           *gimage,
                                    GimpSelectionEditor *editor)
{
  gimp_preview_renderer_invalidate (GIMP_PREVIEW (editor->preview)->renderer);
}
