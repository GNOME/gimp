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

#include "tools-types.h"

#include "core/gimptoolinfo.h"

#include "paint/gimperaser.h"

#include "gimperasertool.h"
#include "paint_options.h"

#include "libgimp/gimpintl.h"


#define ERASER_DEFAULT_HARD        FALSE
#define ERASER_DEFAULT_INCREMENTAL FALSE
#define ERASER_DEFAULT_ANTI_ERASE  FALSE


static void   gimp_eraser_tool_class_init    (GimpEraserToolClass  *klass);
static void   gimp_eraser_tool_init          (GimpEraserTool       *eraser);

static void   gimp_eraser_tool_modifier_key  (GimpTool             *tool,
                                              GdkModifierType       key,
                                              gboolean              press,
                                              GdkModifierType       state,
                                              GimpDisplay          *gdisp);
static void   gimp_eraser_tool_cursor_update (GimpTool             *tool,
                                              GimpCoords           *coords,
                                              GdkModifierType       state,
                                              GimpDisplay          *gdisp);

static GimpToolOptions * gimp_eraser_tool_options_new   (GimpToolInfo    *tool_info);
static void              gimp_eraser_tool_options_reset (GimpToolOptions *tool_options);


static GimpPaintToolClass *parent_class = NULL;


void
gimp_eraser_tool_register (Gimp                     *gimp,
                           GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_ERASER_TOOL,
                gimp_eraser_tool_options_new,
                TRUE,
                "gimp:eraser_tool",
                _("Eraser"),
                _("Paint fuzzy brush strokes"),
                N_("/Tools/Paint Tools/Eraser"), "<shift>E",
                NULL, "tools/eraser.html",
                GIMP_STOCK_TOOL_ERASER);
}

GType
gimp_eraser_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpEraserToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_eraser_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpEraserTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_eraser_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_PAINT_TOOL,
					  "GimpEraserTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_eraser_tool_class_init (GimpEraserToolClass *klass)
{
  GimpToolClass *tool_class;

  tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->modifier_key  = gimp_eraser_tool_modifier_key;
  tool_class->cursor_update = gimp_eraser_tool_cursor_update;
}

static void
gimp_eraser_tool_init (GimpEraserTool *eraser)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (eraser);
  paint_tool = GIMP_PAINT_TOOL (eraser);

  tool->tool_cursor            = GIMP_ERASER_TOOL_CURSOR;
  tool->cursor_modifier        = GIMP_CURSOR_MODIFIER_NONE;
  tool->toggle_tool_cursor     = GIMP_ERASER_TOOL_CURSOR;
  tool->toggle_cursor_modifier = GIMP_CURSOR_MODIFIER_MINUS;

  paint_tool->core = g_object_new (GIMP_TYPE_ERASER, NULL);
}

static void
gimp_eraser_tool_modifier_key (GimpTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               GimpDisplay     *gdisp)
{
  EraserOptions *options;

  options = (EraserOptions *) tool->tool_info->tool_options;

  if ((key == GDK_CONTROL_MASK) &&
      ! (state & GDK_SHIFT_MASK)) /* leave stuff untouched in line draw mode */
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->anti_erase_w),
                                    ! options->anti_erase);
    }
}
  
static void
gimp_eraser_tool_cursor_update (GimpTool        *tool,
                                GimpCoords      *coords,
                                GdkModifierType  state,
                                GimpDisplay     *gdisp)
{
  EraserOptions *options;

  options = (EraserOptions *) tool->tool_info->tool_options;

  tool->toggled = options->anti_erase;

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}


#if 0

gboolean
eraser_non_gui_default (GimpDrawable *drawable,
			gint          num_strokes,
			gdouble      *stroke_array)
{
  GimpToolInfo  *tool_info;
  EraserOptions *options;
  gboolean       hard        = ERASER_DEFAULT_HARD;
  gboolean       incremental = ERASER_DEFAULT_INCREMENTAL;
  gboolean       anti_erase  = ERASER_DEFAULT_ANTI_ERASE;

  tool_info = tool_manager_get_info_by_type (drawable->gimage->gimp,
                                             GIMP_TYPE_ERASER_TOOL);

  options = (EraserOptions *) tool_info->tool_options;

  if (options)
    {
      hard        = options->hard;
      incremental = options->paint_options.incremental;
      anti_erase  = options->anti_erase;
    }

  return eraser_non_gui (drawable, num_strokes, stroke_array,
			 hard, incremental, anti_erase);
}

gboolean
eraser_non_gui (GimpDrawable *drawable,
    		gint          num_strokes,
		gdouble      *stroke_array,
		gint          hard,
		gint          incremental,
		gboolean      anti_erase)
{
  static GimpEraserTool *non_gui_eraser = NULL;

  GimpPaintTool *paint_tool;
  gint           i;
  
  if (! non_gui_eraser)
    {
      non_gui_eraser = g_object_new (GIMP_TYPE_ERASER_TOOL, NULL);
    }

  paint_tool = GIMP_PAINT_TOOL (non_gui_eraser);

  if (gimp_paint_tool_start (paint_tool, drawable,
			     stroke_array[0],
			     stroke_array[1]))
    {
      non_gui_hard        = hard;
      non_gui_incremental = incremental;
      non_gui_anti_erase  = anti_erase;

      paint_tool->start_coords.x = paint_tool->last_coords.x = stroke_array[0];
      paint_tool->start_coords.y = paint_tool->last_coords.y = stroke_array[1];

      gimp_paint_tool_paint (paint_tool, drawable, MOTION_PAINT);

      for (i = 1; i < num_strokes; i++)
	{
          paint_tool->cur_coords.x = stroke_array[i * 2 + 0];
          paint_tool->cur_coords.y = stroke_array[i * 2 + 1];

          gimp_paint_tool_interpolate (paint_tool, drawable);

          paint_tool->last_coords.x = paint_tool->cur_coords.x;
          paint_tool->last_coords.y = paint_tool->cur_coords.y;
	}

      gimp_paint_tool_finish (paint_tool, drawable);

      return TRUE;
    }

  return FALSE;
}

#endif


/*  tool options stuff  */

static GimpToolOptions *
gimp_eraser_tool_options_new (GimpToolInfo *tool_info)
{
  EraserOptions *options;
  GtkWidget     *vbox;

  options = g_new0 (EraserOptions, 1);

  paint_options_init ((PaintOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = gimp_eraser_tool_options_reset;

  options->hard        = options->hard_d        = ERASER_DEFAULT_HARD;
  options->anti_erase  = options->anti_erase_d  = ERASER_DEFAULT_ANTI_ERASE;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  /* the hard toggle */
  options->hard_w = gtk_check_button_new_with_label (_("Hard Edge"));
  gtk_box_pack_start (GTK_BOX (vbox), options->hard_w, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (options->hard_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->hard);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->hard_w),
				options->hard_d);
  gtk_widget_show (options->hard_w);

  /* the anti_erase toggle */
  options->anti_erase_w =
    gtk_check_button_new_with_label (_("Anti Erase (<Ctrl>)"));
  gtk_box_pack_start (GTK_BOX (vbox), options->anti_erase_w, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (options->anti_erase_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->anti_erase);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->anti_erase_w),
				options->anti_erase_d);
  gtk_widget_show (options->anti_erase_w);

  return (GimpToolOptions *) options;
}

static void
gimp_eraser_tool_options_reset (GimpToolOptions *tool_options)
{
  EraserOptions *options;

  options = (EraserOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->hard_w),
				options->hard_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->anti_erase_w),
				options->anti_erase_d);
}
