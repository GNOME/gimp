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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
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

#include "gimptexttool.h"
#include "tool_options.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


/*  the text tool structures  */

typedef struct _TextOptions TextOptions;

struct _TextOptions
{
  GimpToolOptions  tool_options;

  GimpText        *text;
  GtkTextBuffer   *buffer;
};


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

static GimpToolOptions * text_tool_options_new   (GimpToolInfo    *tool_info);
static void              text_tool_options_reset (GimpToolOptions *tool_options);

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
                text_tool_options_new,
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
  TextOptions *options;
  GimpImage   *gimage;
  GimpText    *text;
  GimpLayer   *layer;

  g_return_if_fail (text_tool->text == NULL);

  gimage = text_tool->gdisp->gimage;

  options = (TextOptions *) GIMP_TOOL (text_tool)->tool_info->tool_options;

  text = GIMP_TEXT (gimp_config_duplicate (G_OBJECT (options->text)));

  gimp_text_tool_connect (text_tool, text);

  layer = gimp_text_layer_new (gimage, text);

  g_object_unref (text);

  if (!layer)
    return;

  undo_push_group_start (gimage, TEXT_UNDO_GROUP);

  GIMP_DRAWABLE (layer)->offset_x = text_tool->click_x;
  GIMP_DRAWABLE (layer)->offset_y = text_tool->click_y;

  gimp_image_add_layer (gimage, layer, -1);

  undo_push_group_end (gimage);

  gimp_image_flush (gimage);
}

static void
gimp_text_tool_notify (GObject    *tool,
		       GParamSpec *param_spec,
		       GObject    *text)
{
  GValue value = { 0, };

  g_value_init (&value, param_spec->value_type);

  g_object_get_property (tool, param_spec->name, &value);
  g_object_set_property (text, param_spec->name, &value);

  g_value_unset (&value);
}

static void
gimp_text_tool_connect (GimpTextTool *tool,
                        GimpText     *text)
{
  TextOptions *options;

  if (tool->text == text)
    return;

  options = (TextOptions *) GIMP_TOOL (tool)->tool_info->tool_options;

  if (tool->text)
    {
      g_signal_handlers_disconnect_by_func (options->text,
					    gimp_text_tool_notify,
					    tool->text);

      g_object_unref (tool->text);
      tool->text = NULL;
    }

  if (text)
    {
      tool->text = g_object_ref (text);
      
      gimp_config_copy_properties (G_OBJECT (tool->text),
				   G_OBJECT (options->text));

      g_signal_connect (options->text, "notify",
			G_CALLBACK (gimp_text_tool_notify),
			tool->text);
    }
}

/*  tool options stuff  */

static GimpToolOptions *
text_tool_options_new (GimpToolInfo *tool_info)
{
  TextOptions *options;
  GObject     *text;
  GtkWidget   *vbox;
  GtkWidget   *table;
  GtkWidget   *button;
  GtkWidget   *unit_menu;
  GtkWidget   *font_selection;
  GtkWidget   *spin_button;

  options = g_new0 (TextOptions, 1);

  tool_options_init ((GimpToolOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = text_tool_options_reset;

  text = g_object_new (GIMP_TYPE_TEXT, NULL);

  options->text = GIMP_TEXT (text);

  options->buffer = gimp_prop_text_buffer_new (text, "text", -1);

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  table = gtk_table_new (4, 5, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacing  (GTK_TABLE (table), 3, 12);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (table), FALSE, FALSE, 0);
  gtk_widget_show (table);

  font_selection = gimp_font_selection_new (NULL);
  gimp_font_selection_set_fontname (GIMP_FONT_SELECTION (font_selection),
				    options->text->font);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Font:"), 1.0, 0.5,
                             font_selection, 2, FALSE);
  gtk_widget_set_sensitive (font_selection, FALSE);

  gimp_prop_scale_entry_new (text, "font-size",
			     GTK_TABLE (table), 0, 1,
			     _("_Size:"), 1.0, 50.0, 1);

  unit_menu = gimp_unit_menu_new ("%a", GIMP_TEXT (text)->font_size_unit,
				  TRUE, FALSE, TRUE);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Unit:"), 1.0, 0.5, unit_menu, 2, TRUE);
  gtk_widget_set_sensitive (unit_menu, FALSE);

#if 0
  g_object_set_data (G_OBJECT (unit_menu), "set_digits",
                     size_spinbutton);

  g_signal_connect (options->unit_w, "unit_changed",
                    G_CALLBACK (gimp_unit_menu_update),
                    &unit);
#endif

  button = gimp_prop_color_button_new (text, "color", _("Text Color"),
				       48, 24, GIMP_COLOR_AREA_FLAT); 
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                             _("Color:"), 1.0, 0.5, button, 2, TRUE);

  spin_button = gimp_prop_spin_button_new (text, "letter-spacing", 0.1, 1.0, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spin_button), 5);
  gimp_table_attach_stock (GTK_TABLE (table), 0, 4,
                           GIMP_STOCK_LETTER_SPACING, spin_button);

  spin_button = gimp_prop_spin_button_new (text, "line-spacing", 0.1, 1.0, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spin_button), 5);
  gimp_table_attach_stock (GTK_TABLE (table), 0, 5,
                           GIMP_STOCK_LINE_SPACING, spin_button);

  return (GimpToolOptions *) options;
}

static void
text_tool_options_reset (GimpToolOptions *tool_options)
{
  TextOptions *options = (TextOptions *) tool_options;

  gimp_config_reset (G_OBJECT (options->text));
}


/* text editor */

static void
text_tool_editor (GimpTextTool *text_tool)
{
  TextOptions *options;

  if (text_tool->editor)
    {
      gtk_window_present (GTK_WINDOW (text_tool->editor));
      return;
    }

  options = (TextOptions *) GIMP_TOOL (text_tool)->tool_info->tool_options;

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


