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
#include <gdk/gdkkeysyms.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "gimperasertool.h"
#include "paint_options.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"


#define ERASER_DEFAULT_HARD        FALSE
#define ERASER_DEFAULT_INCREMENTAL FALSE
#define ERASER_DEFAULT_ANTI_ERASE  FALSE


typedef struct _EraserOptions EraserOptions;

struct _EraserOptions
{
  PaintOptions  paint_options;

  gboolean      hard;
  gboolean      hard_d;
  GtkWidget    *hard_w;

  gboolean	anti_erase;
  gboolean	anti_erase_d;
  GtkWidget    *anti_erase_w;
};


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

static void   gimp_eraser_tool_paint         (GimpPaintTool        *paint_tool,
                                              GimpDrawable         *drawable,
                                              PaintState            state);

static void   gimp_eraser_tool_motion        (GimpPaintTool        *paint_tool,
                                              GimpDrawable         *drawable,
                                              PaintPressureOptions *pressure_options,
                                              gboolean              hard,
                                              gboolean              incremental,
                                              gboolean              anti_erase);

static GimpToolOptions * gimp_eraser_tool_options_new   (GimpToolInfo    *tool_info);
static void              gimp_eraser_tool_options_reset (GimpToolOptions *tool_options);


/*  local variables  */
static gboolean   non_gui_hard        = ERASER_DEFAULT_HARD;
static gboolean   non_gui_incremental = ERASER_DEFAULT_INCREMENTAL;
static gboolean	  non_gui_anti_erase  = ERASER_DEFAULT_ANTI_ERASE;

static GimpPaintToolClass *parent_class = NULL;


/*  functions  */

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
  GimpToolClass      *tool_class;
  GimpPaintToolClass *paint_tool_class;

  tool_class       = GIMP_TOOL_CLASS (klass);
  paint_tool_class = GIMP_PAINT_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->modifier_key  = gimp_eraser_tool_modifier_key;
  tool_class->cursor_update = gimp_eraser_tool_cursor_update;

  paint_tool_class->paint   = gimp_eraser_tool_paint;
}

static void
gimp_eraser_tool_init (GimpEraserTool *eraser)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (eraser);
  paint_tool = GIMP_PAINT_TOOL (eraser);

  tool->tool_cursor = GIMP_ERASER_TOOL_CURSOR;

  paint_tool->flags |= TOOL_CAN_HANDLE_CHANGING_BRUSH;
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

  tool->toggled = options->anti_erase;
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

static void
gimp_eraser_tool_paint (GimpPaintTool *paint_tool,
                        GimpDrawable  *drawable,
                        PaintState     state)
{
  EraserOptions        *options;
  PaintPressureOptions *pressure_options;
  gboolean              hard;
  gboolean              incremental;
  gboolean              anti_erase;

  options = (EraserOptions *) GIMP_TOOL (paint_tool)->tool_info->tool_options;

  if (options)
    {
      pressure_options = options->paint_options.pressure_options;
      hard             = options->hard;
      incremental      = options->paint_options.incremental;
      anti_erase       = options->anti_erase;
    }
  else
    {
      pressure_options = &non_gui_pressure_options;
      hard             = non_gui_hard;
      incremental      = non_gui_incremental;
      anti_erase       = non_gui_anti_erase;
    }

  switch (state)
    {
    case INIT_PAINT:
      break;

    case MOTION_PAINT:
      gimp_eraser_tool_motion (paint_tool,
                               drawable,
                               pressure_options,
                               hard,
                               incremental,
                               anti_erase);
      break;

    case FINISH_PAINT:
      break;

    default:
      break;
    }
}

static void
gimp_eraser_tool_motion (GimpPaintTool        *paint_tool,
                         GimpDrawable         *drawable,
                         PaintPressureOptions *pressure_options,
                         gboolean              hard,
                         gboolean              incremental,
                         gboolean              anti_erase)
{
  GimpImage   *gimage;
  GimpContext *context;
  gint         opacity;
  TempBuf     *area;
  guchar       col[MAX_CHANNELS];
  gdouble      scale;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  gimp_image_get_background (gimage, drawable, col);

  context = gimp_get_current_context (gimage->gimp);

  if (pressure_options->size)
    scale = paint_tool->cur_coords.pressure;
  else
    scale = 1.0;

  /*  Get a region which can be used to paint to  */
  if (! (area = gimp_paint_tool_get_paint_area (paint_tool, drawable, scale)))
    return;

  /*  set the alpha channel  */
  col[area->bytes - 1] = OPAQUE_OPACITY;

  /*  color the pixels  */
  color_pixels (temp_buf_data (area), col,
		area->width * area->height, area->bytes);

  opacity = 255 * gimp_context_get_opacity (context);

  if (pressure_options->opacity)
    opacity = opacity * 2.0 * paint_tool->cur_coords.pressure;

  /* paste the newly painted canvas to the gimage which is being
   * worked on  */
  gimp_paint_tool_paste_canvas (paint_tool, drawable, 
				MIN (opacity, 255),
				gimp_context_get_opacity (context) * 255,
				anti_erase ? ANTI_ERASE_MODE : ERASE_MODE,
				hard ? HARD : (pressure_options->pressure ? PRESSURE : SOFT),
				scale,
				incremental ? INCREMENTAL : CONSTANT);
}


/*  non-gui stuff  */

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
