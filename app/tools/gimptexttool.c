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
#include <errno.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimptoolinfo.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-utils.h"

#include "text/gimptext.h"
#include "text/gimptext-vectors.h"
#include "text/gimptextlayer.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimptexteditor.h"

#include "display/gimpdisplay.h"

#include "gimptextoptions.h"
#include "gimptexttool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_text_tool_class_init     (GimpTextToolClass *klass);
static void   gimp_text_tool_init           (GimpTextTool      *tool);

static void   gimp_text_tool_control        (GimpTool          *tool,
					     GimpToolAction     action,
					     GimpDisplay       *gdisp);
static void   gimp_text_tool_button_press   (GimpTool          *tool,
					     GimpCoords        *coords,
					     guint32            time,
					     GdkModifierType    state,
					     GimpDisplay       *gdisp);
static void   gimp_text_tool_cursor_update  (GimpTool          *tool,
					     GimpCoords        *coords,
					     GdkModifierType    state,
					     GimpDisplay       *gdisp);

static void   gimp_text_tool_connect        (GimpTextTool      *tool,
					     GimpText          *text);

static void   gimp_text_tool_create_vectors (GimpTextTool      *text_tool);
static void   gimp_text_tool_create_layer   (GimpTextTool      *text_tool);

static void   gimp_text_tool_editor         (GimpTextTool      *text_tool);
static void   gimp_text_tool_text_changed   (GimpTextEditor    *editor,
					     GimpTextTool      *text_tool);
static void   gimp_text_tool_layer_changed  (GimpImage         *image,
                                             GimpTextTool      *text_tool);


/*  local variables  */

static GimpToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_text_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_TEXT_TOOL,
                GIMP_TYPE_TEXT_OPTIONS,
                gimp_text_options_gui,
                GIMP_CONTEXT_FOREGROUND_MASK | GIMP_CONTEXT_FONT_MASK,
                "gimp-text-tool",
                _("Text"),
                _("Add text to the image"),
                N_("/Tools/Te_xt"), "T",
                NULL, GIMP_HELP_TOOL_TEXT,
                GIMP_STOCK_TOOL_TEXT,
                data);
}

GType
gimp_text_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpTextToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_text_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpTextTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_text_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TOOL,
					  "GimpTextTool",
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_text_tool_class_init (GimpTextToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->control       = gimp_text_tool_control;
  tool_class->button_press  = gimp_text_tool_button_press;
  tool_class->cursor_update = gimp_text_tool_cursor_update;
}

static void
gimp_text_tool_init (GimpTextTool *text_tool)
{
  GimpTool *tool = GIMP_TOOL (text_tool);

  text_tool->text  = NULL;
  text_tool->layer = NULL;

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TEXT_TOOL_CURSOR);
}

static void
gimp_text_tool_control (GimpTool       *tool,
			GimpToolAction  action,
			GimpDisplay    *gdisp)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      if (text_tool->editor)
        gtk_widget_destroy (text_tool->editor);

      gimp_text_tool_set_layer (text_tool, NULL);
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_text_tool_button_press (GimpTool        *tool,
			     GimpCoords      *coords,
			     guint32          time,
			     GdkModifierType  state,
			     GimpDisplay     *gdisp)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (tool);
  GimpDrawable *drawable;

  gimp_tool_control_activate (tool->control);
  tool->gdisp = gdisp;

  text_tool->x1 = coords->x;
  text_tool->y1 = coords->y;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (GIMP_IS_TEXT_LAYER (drawable))
    {
      GimpItem *item = GIMP_ITEM (drawable);

      coords->x -= item->offset_x;
      coords->y -= item->offset_y;

      if (coords->x > 0 && coords->x < item->width &&
          coords->y > 0 && coords->y < item->height)
        {
          GimpLayer *layer = GIMP_LAYER (drawable);
          gboolean   edit  = (text_tool->layer == layer);

          gimp_text_tool_set_layer (text_tool, layer);

          if (edit)
            gimp_text_tool_editor (text_tool);

          return;
        }
    }

  gimp_text_tool_set_layer (text_tool, NULL);
  gimp_text_tool_editor (text_tool);
}

static void
gimp_text_tool_cursor_update (GimpTool        *tool,
			      GimpCoords      *coords,
			      GdkModifierType  state,
			      GimpDisplay     *gdisp)
{
  /* FIXME: should do something fancy here... */

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_text_tool_create_vectors (GimpTextTool *text_tool)
{
  GimpTool    *tool   = GIMP_TOOL (text_tool);
  GimpImage   *gimage = tool->gdisp->gimage;
  GimpVectors *vectors;

  if (! text_tool->text)
    return;

  gimp_tool_control_set_preserve (tool->control, TRUE);

  vectors = gimp_text_vectors_new (gimage, text_tool->text);

  if (text_tool->layer)
    {
      gint x, y;

      gimp_item_offsets (GIMP_ITEM (text_tool->layer), &x, &y);
      gimp_item_translate (GIMP_ITEM (vectors), x, y, FALSE);
    }

  gimp_image_add_vectors (gimage, vectors, -1);

  gimp_tool_control_set_preserve (tool->control, FALSE);

  gimp_image_flush (gimage);
}

static void
gimp_text_tool_create_layer (GimpTextTool *text_tool)
{
  GimpTool        *tool = GIMP_TOOL (text_tool);
  GimpTextOptions *options;
  GimpImage       *gimage;
  GimpText        *text;
  GimpLayer       *layer;

  g_return_if_fail (text_tool->text == NULL);

  options = GIMP_TEXT_OPTIONS (GIMP_TOOL (text_tool)->tool_info->tool_options);

  gimage = tool->gdisp->gimage;

  text = gimp_text_options_create_text (options);

  g_object_set (text,
                "text",
                gimp_text_editor_get_text (GIMP_TEXT_EDITOR (text_tool->editor)),
                NULL);

  layer = gimp_text_layer_new (gimage, text);
  g_object_unref (text);

  if (! layer)
    return;

  gimp_text_tool_connect (text_tool, text);

  gimp_tool_control_set_preserve (tool->control, TRUE);

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_TEXT,
                               _("Add Text Layer"));

  if (gimp_image_floating_sel (gimage))
    floating_sel_anchor (gimp_image_floating_sel (gimage));

  GIMP_ITEM (layer)->offset_x = text_tool->x1;
  GIMP_ITEM (layer)->offset_y = text_tool->y1;

  gimp_image_add_layer (gimage, layer, -1);

  gimp_image_undo_group_end (gimage);

  gimp_tool_control_set_preserve (tool->control, FALSE);

  gimp_image_flush (gimage);

  gimp_text_tool_set_layer (text_tool, layer);
}

static void
gimp_text_tool_connect (GimpTextTool *text_tool,
                        GimpText     *text)
{
  GimpTool        *tool = GIMP_TOOL (text_tool);
  GimpTextOptions *options;
  GtkWidget       *button;

  if (text_tool->text == text)
    return;

  options = GIMP_TEXT_OPTIONS (tool->tool_info->tool_options);

  button = g_object_get_data (G_OBJECT (options), "gimp-text-to-vectors");

  if (text_tool->text)
    {
      if (button)
        {
          gtk_widget_set_sensitive (button, FALSE);
          g_signal_handlers_disconnect_by_func (button,
                                                gimp_text_tool_create_vectors,
                                                text_tool);
        }

      gimp_text_options_disconnect_text (options, text_tool->text);

      g_object_unref (text_tool->text);
      text_tool->text = NULL;

      text_tool->layer = NULL;
    }

  if (text)
    {
      text_tool->text = g_object_ref (text);

      gimp_text_options_connect_text (options, text);

      if (button)
        {
          g_signal_connect_swapped (button, "clicked",
                                    G_CALLBACK (gimp_text_tool_create_vectors),
                                    text_tool);
          gtk_widget_set_sensitive (button, TRUE);
        }
    }
}

static void
gimp_text_tool_editor (GimpTextTool *text_tool)
{
  GimpTextOptions *options;

  if (text_tool->editor)
    {
      gtk_window_present (GTK_WINDOW (text_tool->editor));
      return;
    }

  options = GIMP_TEXT_OPTIONS (GIMP_TOOL (text_tool)->tool_info->tool_options);

  text_tool->editor = gimp_text_options_editor_new (options,
                                                    _("GIMP Text Editor"));

  g_object_add_weak_pointer (G_OBJECT (text_tool->editor),
			     (gpointer *) &text_tool->editor);

  gimp_dialog_factory_add_foreign (gimp_dialog_factory_from_name ("toplevel"),
                                   "gimp-text-tool-dialog",
                                   text_tool->editor);

  if (text_tool->text)
    gimp_text_editor_set_text (GIMP_TEXT_EDITOR (text_tool->editor),
                               text_tool->text->text, -1);

  g_signal_connect_object (text_tool->editor, "text_changed",
                           G_CALLBACK (gimp_text_tool_text_changed),
                           text_tool, 0);

  gtk_widget_show (text_tool->editor);
}

static void
gimp_text_tool_text_changed (GimpTextEditor *editor,
                             GimpTextTool   *text_tool)
{
  if (text_tool->text)
    {
      gchar *text;

      text = gimp_text_editor_get_text (GIMP_TEXT_EDITOR (text_tool->editor));

      g_object_set (text_tool->text,
                    "text", text,
                    NULL);

      g_free (text);
    }
  else
    {
      gimp_text_tool_create_layer (text_tool);
    }
}

static void
gimp_text_tool_layer_changed (GimpImage    *image,
                              GimpTextTool *text_tool)
{
  gimp_text_tool_set_layer (text_tool, gimp_image_get_active_layer (image));
}

void
gimp_text_tool_set_layer (GimpTextTool *text_tool,
                          GimpLayer    *layer)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_TEXT_TOOL (text_tool));

  if (layer && GIMP_IS_TEXT_LAYER (layer) && GIMP_TEXT_LAYER (layer)->text)
    {
      gimp_text_tool_connect (text_tool, GIMP_TEXT_LAYER (layer)->text);

      if (text_tool->layer == layer)
        return;

      if (text_tool->layer)
        {
          image = gimp_item_get_image (GIMP_ITEM (text_tool->layer));
          g_signal_handlers_disconnect_by_func (image,
                                                gimp_text_tool_layer_changed,
                                                text_tool);
        }

      text_tool->layer = layer;

      image = gimp_item_get_image (GIMP_ITEM (text_tool->layer));
      g_signal_connect_object (image, "active_layer_changed",
                               G_CALLBACK (gimp_text_tool_layer_changed),
                               text_tool, 0);
    }
  else
    {
      gimp_text_tool_connect (text_tool, NULL);

      if (text_tool->layer)
        {
          image = gimp_item_get_image (GIMP_ITEM (text_tool->layer));
          g_signal_handlers_disconnect_by_func (image,
                                                gimp_text_tool_layer_changed,
                                                text_tool);
          text_tool->layer = NULL;
        }
    }
}
