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

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "cursorutil.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "paint_funcs.h"
#include "selection.h"
#include "temp_buf.h"

#include "gimperasertool.h"
#include "paint_options.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"

#include "pixmaps2.h"


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


static void   gimp_eraser_tool_class_init   (GimpEraserToolClass  *klass);
static void   gimp_eraser_tool_init         (GimpEraserTool       *eraser);

static void   gimp_eraser_tool_modifier_key (GimpTool             *tool,
                                             GdkEventKey          *kevent,
                                             GDisplay             *gdisp);

static void   gimp_eraser_tool_paint        (GimpPaintTool        *paint_tool,
                                             GimpDrawable         *drawable,
                                             PaintState            state);

static void   gimp_eraser_tool_motion       (GimpPaintTool        *paint_tool,
                                             GimpDrawable         *drawable,
                                             PaintPressureOptions *pressure_options,
                                             gboolean              hard,
                                             gboolean              incremental,
                                             gboolean              anti_erase);

static EraserOptions * gimp_eraser_tool_options_new   (void);
static void            gimp_eraser_tool_options_reset (void);



/*  local variables  */
static gboolean   non_gui_hard        = ERASER_DEFAULT_HARD;
static gboolean   non_gui_incremental = ERASER_DEFAULT_INCREMENTAL;
static gboolean	  non_gui_anti_erase  = ERASER_DEFAULT_ANTI_ERASE;

static EraserOptions *eraser_options = NULL;

static GimpPaintToolClass *parent_class = NULL;


/*  functions  */

void
gimp_eraser_tool_register (void)
{
  tool_manager_register_tool (GIMP_TYPE_ERASER_TOOL,
			      TRUE,
  			      "gimp:eraser_tool",
  			      _("Eraser"),
  			      _("Paint fuzzy brush strokes"),
      			      N_("/Tools/Paint Tools/Eraser"), "<shift>E",
  			      NULL, "tools/eraser.html",
			      (const gchar **) erase_bits);
}

GtkType
gimp_eraser_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpEraserTool",
        sizeof (GimpEraserTool),
        sizeof (GimpEraserToolClass),
        (GtkClassInitFunc) gimp_eraser_tool_class_init,
        (GtkObjectInitFunc) gimp_eraser_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        NULL
      };

      tool_type = gtk_type_unique (GIMP_TYPE_PAINT_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_eraser_tool_class_init (GimpEraserToolClass *klass)
{
  GimpToolClass      *tool_class;
  GimpPaintToolClass *paint_tool_class;

  tool_class       = (GimpToolClass *) klass;
  paint_tool_class = (GimpPaintToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PAINT_TOOL);

  tool_class->modifier_key = gimp_eraser_tool_modifier_key;

  paint_tool_class->paint  = gimp_eraser_tool_paint;
}

static void
gimp_eraser_tool_init (GimpEraserTool *eraser)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (eraser);
  paint_tool = GIMP_PAINT_TOOL (eraser);

  if (! eraser_options)
    {
      eraser_options = gimp_eraser_tool_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_ERASER_TOOL,
                                          (ToolOptions *) eraser_options);
    }

  tool->tool_cursor = GIMP_ERASER_TOOL_CURSOR;

  paint_tool->flags |= TOOL_CAN_HANDLE_CHANGING_BRUSH;
}

static void
gimp_eraser_tool_modifier_key (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GDisplay    *gdisp)
{
  switch (kevent->keyval)
    {
    case GDK_Alt_L: 
    case GDK_Alt_R:
      break;
    case GDK_Shift_L: 
    case GDK_Shift_R:
      if (kevent->state & GDK_CONTROL_MASK)    /* reset tool toggle */
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (eraser_options->anti_erase_w),
	   ! eraser_options->anti_erase);
      break;
    case GDK_Control_L: 
    case GDK_Control_R:
      if (!(kevent->state & GDK_SHIFT_MASK)) /* shift enables line draw mode */
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (eraser_options->anti_erase_w),
	   ! eraser_options->anti_erase);
      break;
    }

  tool->toggled = eraser_options->anti_erase;
}
  
static void
gimp_eraser_tool_paint (GimpPaintTool *paint_tool,
                        GimpDrawable  *drawable,
                        PaintState     state)
{
  PaintPressureOptions *pressure_options;
  gboolean              hard;
  gboolean              incremental;
  gboolean              anti_erase;

  if (eraser_options)
    {
      pressure_options = eraser_options->paint_options.pressure_options;
      hard             = eraser_options->hard;
      incremental      = eraser_options->paint_options.incremental;
      anti_erase       = eraser_options->anti_erase;
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
  GimpImage *gimage;
  gint       opacity;
  TempBuf   *area;
  guchar     col[MAX_CHANNELS];
  gdouble    scale;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  gimp_image_get_background (gimage, drawable, col);

  if (pressure_options->size)
    scale = paint_tool->curpressure;
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

  opacity = 255 * gimp_context_get_opacity (NULL);

  if (pressure_options->opacity)
    opacity = opacity * 2.0 * paint_tool->curpressure;

  /*  paste the newly painted canvas to the gimage which is being worked on  */
  gimp_paint_tool_paste_canvas (paint_tool, drawable, 
                                MIN (opacity, 255),
                                gimp_context_get_opacity (NULL) * 255,
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
  gboolean  hard        = ERASER_DEFAULT_HARD;
  gboolean  incremental = ERASER_DEFAULT_INCREMENTAL;
  gboolean  anti_erase  = ERASER_DEFAULT_ANTI_ERASE;

  EraserOptions *options = eraser_options;

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
      non_gui_eraser = gtk_type_new (GIMP_TYPE_ERASER_TOOL);
    }

  paint_tool = GIMP_PAINT_TOOL (non_gui_eraser);

  if (gimp_paint_tool_start (paint_tool, drawable,
			     stroke_array[0],
			     stroke_array[1]))
    {
      non_gui_hard        = hard;
      non_gui_incremental = incremental;
      non_gui_anti_erase  = anti_erase;

      paint_tool->startx = paint_tool->lastx = stroke_array[0];
      paint_tool->starty = paint_tool->lasty = stroke_array[1];

      gimp_paint_tool_paint (paint_tool, drawable, MOTION_PAINT);

      for (i = 1; i < num_strokes; i++)
	{
          paint_tool->curx = stroke_array[i * 2 + 0];
          paint_tool->cury = stroke_array[i * 2 + 1];

          gimp_paint_tool_interpolate (paint_tool, drawable);

          paint_tool->lastx = paint_tool->curx;
          paint_tool->lasty = paint_tool->cury;
	}

      gimp_paint_tool_finish (paint_tool, drawable);

      return TRUE;
    }

  return FALSE;
}


/*  tool options stuff  */

static EraserOptions *
gimp_eraser_tool_options_new (void)
{
  EraserOptions *options;
  GtkWidget     *vbox;

  options = g_new (EraserOptions, 1);

  paint_options_init ((PaintOptions *) options,
		      GIMP_TYPE_ERASER_TOOL,
		      gimp_eraser_tool_options_reset);

  options->hard        = options->hard_d        = ERASER_DEFAULT_HARD;
  options->anti_erase  = options->anti_erase_d  = ERASER_DEFAULT_ANTI_ERASE;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /* the hard toggle */
  options->hard_w = gtk_check_button_new_with_label (_("Hard Edge"));
  gtk_box_pack_start (GTK_BOX (vbox), options->hard_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->hard_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->hard);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->hard_w),
				options->hard_d);
  gtk_widget_show (options->hard_w);

  /* the anti_erase toggle */
  options->anti_erase_w = gtk_check_button_new_with_label (_("Anti Erase"));
  gtk_box_pack_start (GTK_BOX (vbox), options->anti_erase_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->anti_erase_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->anti_erase);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->anti_erase_w),
				options->anti_erase_d);
  gtk_widget_show (options->anti_erase_w);

  return options;
}

static void
gimp_eraser_tool_options_reset (void)
{
  EraserOptions *options = eraser_options;

  paint_options_reset ((PaintOptions *) options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->hard_w),
				options->hard_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->anti_erase_w),
				options->anti_erase_d);
}
