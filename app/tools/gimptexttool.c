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
#include "core/gimptoolinfo.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-utils.h"

#include "text/gimptext.h"
#include "text/gimptextlayer.h"

#include "widgets/gimpfontselection.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimptextoptions.h"
#include "gimptexttool.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void     gimp_text_tool_class_init    (GimpTextToolClass *klass);
static void     gimp_text_tool_init          (GimpTextTool      *tool);

static void     text_tool_control              (GimpTool        *tool,
                                                GimpToolAction   action,
                                                GimpDisplay     *gdisp);
static void     text_tool_button_press         (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                guint32          time,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);
static void     text_tool_button_release       (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                guint32          time,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);
static void     text_tool_cursor_update        (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);

static void     gimp_text_tool_connect         (GimpTextTool    *tool,
                                                GimpText        *text);

static void     text_tool_create_layer         (GimpTextTool    *text_tool);

static void     text_tool_editor               (GimpTextTool    *text_tool);
static void     text_tool_editor_response      (GtkWidget       *editor,
						GtkResponseType  response,
						gpointer         data);


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
                FALSE,
                "gimp-text-tool",
                _("Text"),
                _("Add text to the image"),
                N_("/Tools/Text"), "T",
                NULL, "tools/text.html",
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
  GimpToolClass *tool_class;

  tool_class   = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->control        = text_tool_control;
  tool_class->button_press   = text_tool_button_press;
  tool_class->button_release = text_tool_button_release;
  tool_class->cursor_update  = text_tool_cursor_update;
}

static void
gimp_text_tool_init (GimpTextTool *text_tool)
{
  GimpTool *tool = GIMP_TOOL (text_tool);

  text_tool->text = NULL;

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TEXT_TOOL_CURSOR);
}

static void
text_tool_control (GimpTool       *tool,
		   GimpToolAction  action,
		   GimpDisplay    *gdisp)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      if (GIMP_TEXT_TOOL (tool)->editor)
        gtk_widget_destroy (GIMP_TEXT_TOOL (tool)->editor);

      gimp_text_tool_connect (GIMP_TEXT_TOOL (tool), NULL);
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
text_tool_button_press (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
			GdkModifierType  state,
			GimpDisplay     *gdisp)
{
  GimpTextTool *text_tool;
  GimpDrawable *drawable;
  GimpText     *text = NULL;

  text_tool = GIMP_TEXT_TOOL (tool);

  text_tool->gdisp = gdisp;

  gimp_tool_control_activate (tool->control);
  tool->gdisp = gdisp;

  text_tool->click_x = coords->x;
  text_tool->click_y = coords->y;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (drawable && GIMP_IS_TEXT_LAYER (drawable))
    {
      coords->x -= drawable->offset_x;
      coords->y -= drawable->offset_y;

      if (coords->x > 0 && coords->x < drawable->width &&
          coords->y > 0 && coords->y < drawable->height)
        {
          text = gimp_text_layer_get_text (GIMP_TEXT_LAYER (drawable));
        }
    }

  if (!text || text == text_tool->text)
    text_tool_editor (text_tool);

  gimp_text_tool_connect (GIMP_TEXT_TOOL (tool), text);
}

static void
text_tool_button_release (GimpTool        *tool,
                          GimpCoords      *coords,
                          guint32          time,
			  GdkModifierType  state,
			  GimpDisplay     *gdisp)
{
/*    gimp_tool_control_halt (tool->control); */
}

static void
text_tool_cursor_update (GimpTool        *tool,
                         GimpCoords      *coords,
			 GdkModifierType  state,
			 GimpDisplay     *gdisp)
{
  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
text_tool_create_layer (GimpTextTool *text_tool)
{
  GimpTextOptions *options;
  GimpImage       *gimage;
  GimpText        *text;
  GimpLayer       *layer;

  g_return_if_fail (text_tool->text == NULL);

  options = GIMP_TEXT_OPTIONS (GIMP_TOOL (text_tool)->tool_info->tool_options);

  gimage = text_tool->gdisp->gimage;

  text = GIMP_TEXT (gimp_config_duplicate (G_OBJECT (options->text)));

  gimp_text_tool_connect (text_tool, text);

  layer = gimp_text_layer_new (gimage, text);

  g_object_unref (text);

  if (!layer)
    return;

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_TEXT,
                               _("Add Text Layer"));

  GIMP_DRAWABLE (layer)->offset_x = text_tool->click_x;
  GIMP_DRAWABLE (layer)->offset_y = text_tool->click_y;

  gimp_image_add_layer (gimage, layer, -1);

  gimp_image_undo_group_end (gimage);

  gimp_image_flush (gimage);
}

static void
gimp_text_tool_connect (GimpTextTool *tool,
                        GimpText     *text)
{
  GimpTextOptions *options;

  if (tool->text == text)
    return;

  options = GIMP_TEXT_OPTIONS (GIMP_TOOL (tool)->tool_info->tool_options);

  if (tool->text)
    {
      gimp_config_disconnect (G_OBJECT (options->text),
                              G_OBJECT (tool->text));

      g_object_unref (tool->text);
      tool->text = NULL;
    }

  if (text)
    {
      tool->text = g_object_ref (text);
      
      gimp_config_copy_properties (G_OBJECT (tool->text),
				   G_OBJECT (options->text));

      gimp_config_connect (G_OBJECT (options->text),
                           G_OBJECT (tool->text));
    }
}


/* text editor */

static void
text_tool_editor (GimpTextTool *text_tool)
{
  GimpTextOptions *options;

  if (text_tool->editor)
    {
      gtk_window_present (GTK_WINDOW (text_tool->editor));
      return;
    }

  options = GIMP_TEXT_OPTIONS (GIMP_TOOL (text_tool)->tool_info->tool_options);

  text_tool->editor = gimp_text_editor_new (options->buffer,
					    _("GIMP Text Editor"),
					    text_tool_editor_response,
					    text_tool);

  g_object_add_weak_pointer (G_OBJECT (text_tool->editor),
			     (gpointer *) &text_tool->editor);

  gtk_widget_show (text_tool->editor);
}

static void
text_tool_editor_response (GtkWidget       *editor,
			   GtkResponseType  response,
			   gpointer         data)
{
  gtk_widget_destroy (editor);

  switch (response)
    {
    case GTK_RESPONSE_OK:
      {
	GimpTextTool *text_tool = GIMP_TEXT_TOOL (data);

	if (! text_tool->text)
	  text_tool_create_layer (text_tool);
      }
      break;
    default:
      break;
    }
}
