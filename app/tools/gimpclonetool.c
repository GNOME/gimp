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

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimptoolinfo.h"

#include "paint/gimpclone.h"

#include "display/gimpdisplay.h"

#include "gimpclonetool.h"
#include "paint_options.h"

#include "libgimp/gimpintl.h"


#define TARGET_WIDTH  15
#define TARGET_HEIGHT 15

#define CLONE_DEFAULT_TYPE     IMAGE_CLONE
#define CLONE_DEFAULT_ALIGNED  ALIGN_NO


static void   gimp_clone_tool_class_init       (GimpCloneToolClass *klass);
static void   gimp_clone_tool_init             (GimpCloneTool      *tool);

static void   gimp_clone_tool_button_press     (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                guint32          time,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);
static void   gimp_clone_tool_motion           (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                guint32          time,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);

static void   gimp_clone_tool_cursor_update    (GimpTool        *tool,
                                                GimpCoords      *coords,
                                                GdkModifierType  state,
                                                GimpDisplay     *gdisp);

static void   gimp_clone_tool_draw             (GimpDrawTool    *draw_tool);

static void   gimp_clone_init_callback         (GimpClone       *clone,
                                                gpointer         data);
static void   gimp_clone_finish_callback       (GimpClone       *clone,
                                                gpointer         data);
static void   gimp_clone_pretrace_callback     (GimpClone       *clone,
                                                gpointer         data);
static void   gimp_clone_posttrace_callback    (GimpClone       *clone,
                                                gpointer         data);

static GimpToolOptions * clone_options_new     (GimpToolInfo    *tool_info);
static void              clone_options_reset   (GimpToolOptions *options);


static GimpPaintToolClass *parent_class;


/* public functions  */

void
gimp_clone_tool_register (Gimp                     *gimp,
                          GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_CLONE_TOOL,
                clone_options_new,
                TRUE,
                "gimp:clone_tool",
                _("Clone"),
                _("Paint using Patterns or Image Regions"),
                N_("/Tools/Paint Tools/Clone"), "C",
                NULL, "tools/clone.html",
                GIMP_STOCK_TOOL_CLONE);
}

GType
gimp_clone_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpCloneToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_clone_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpCloneTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_clone_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_PAINT_TOOL,
					  "GimpCloneTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

/* static functions  */

static void
gimp_clone_tool_class_init (GimpCloneToolClass *klass)
{
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->button_press  = gimp_clone_tool_button_press;
  tool_class->motion        = gimp_clone_tool_motion;
  tool_class->cursor_update = gimp_clone_tool_cursor_update;

  draw_tool_class->draw     = gimp_clone_tool_draw;
}

static void
gimp_clone_tool_init (GimpCloneTool *clone)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;
  GimpClone     *clone_core;

  tool       = GIMP_TOOL (clone);
  paint_tool = GIMP_PAINT_TOOL (clone);

  tool->tool_cursor = GIMP_CLONE_TOOL_CURSOR;

  clone_core = g_object_new (GIMP_TYPE_CLONE, NULL);

  clone_core->init_callback      = gimp_clone_init_callback;
  clone_core->finish_callback    = gimp_clone_finish_callback;
  clone_core->pretrace_callback  = gimp_clone_pretrace_callback;
  clone_core->posttrace_callback = gimp_clone_posttrace_callback;
  clone_core->callback_data      = clone;

  paint_tool->core = GIMP_PAINT_CORE (clone_core);
}

static void
gimp_clone_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpPaintTool *paint_tool;

  paint_tool = GIMP_PAINT_TOOL (tool);

  if (state & GDK_CONTROL_MASK)
    {
      GIMP_CLONE (paint_tool->core)->set_source = TRUE;
    }
  else
    {
      GIMP_CLONE (paint_tool->core)->set_source = FALSE;
    }

  GIMP_TOOL_CLASS (parent_class)->button_press (tool,
                                                coords,
                                                time,
                                                state,
                                                gdisp);
}

static void
gimp_clone_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *gdisp)
{
  GimpPaintTool *paint_tool;

  paint_tool = GIMP_PAINT_TOOL (tool);

  if (state & GDK_CONTROL_MASK)
    {
      GIMP_CLONE (paint_tool->core)->set_source = TRUE;
    }
  else
    {
      GIMP_CLONE (paint_tool->core)->set_source = FALSE;
    }

  GIMP_TOOL_CLASS (parent_class)->motion (tool,
                                          coords,
                                          time,
                                          state,
                                          gdisp);
}

void
gimp_clone_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
			       GdkModifierType  state,
			       GimpDisplay     *gdisp)
{
  CloneOptions  *options;
  GimpLayer     *layer;
  GdkCursorType  ctype = GIMP_MOUSE_CURSOR;

  options = (CloneOptions *) tool->tool_info->tool_options;

  if ((layer = gimp_image_get_active_layer (gdisp->gimage))) 
    {
      gint off_x, off_y;

      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

      if (coords->x >= off_x &&
          coords->y >= off_y &&
	  coords->x < (off_x + gimp_drawable_width (GIMP_DRAWABLE (layer))) &&
	  coords->y < (off_y + gimp_drawable_height (GIMP_DRAWABLE (layer))))
	{
	  /*  One more test--is there a selected region?
	   *  if so, is cursor inside?
	   */
	  if (gimp_image_mask_is_empty (gdisp->gimage))
	    ctype = GIMP_MOUSE_CURSOR;
	  else if (gimp_image_mask_value (gdisp->gimage, coords->x, coords->y))
	    ctype = GIMP_MOUSE_CURSOR;
	}
    }

  if (options->type == IMAGE_CLONE)
    {
      if (state & GDK_CONTROL_MASK)
	ctype = GIMP_CROSSHAIR_SMALL_CURSOR;
      else if (! GIMP_CLONE (GIMP_PAINT_TOOL (tool)->core)->src_drawable)
	ctype = GIMP_BAD_CURSOR;
    }

  tool->cursor = ctype;

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_clone_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (draw_tool);

  if (tool->state == ACTIVE)
    {
      CloneOptions *options;

      options = (CloneOptions *) tool->tool_info->tool_options;

      if (draw_tool->gdisp && options->type == IMAGE_CLONE)
        {
          GimpClone *clone;

          clone = GIMP_CLONE (GIMP_PAINT_TOOL (draw_tool)->core);

          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_CROSS,
                                      clone->src_x,
                                      clone->src_y,
                                      TARGET_WIDTH, TARGET_WIDTH,
                                      GTK_ANCHOR_CENTER,
                                      TRUE);
        }
    }
  else
    {
      GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
    }
}

static void
gimp_clone_init_callback (GimpClone *clone,
                          gpointer   data)
{
  GimpCloneTool *clone_tool;

  clone_tool = GIMP_CLONE_TOOL (data);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (clone_tool),
                        GIMP_TOOL (clone_tool)->gdisp);
}

static void
gimp_clone_finish_callback (GimpClone *clone,
                            gpointer   data)
{
  GimpCloneTool *clone_tool;

  clone_tool = GIMP_CLONE_TOOL (data);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (clone_tool));
}

static void
gimp_clone_pretrace_callback (GimpClone *clone,
                              gpointer   data)
{
  GimpCloneTool *clone_tool;

  clone_tool = GIMP_CLONE_TOOL (data);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (clone_tool));
}

static void
gimp_clone_posttrace_callback (GimpClone *clone,
                               gpointer   data)
{
  GimpCloneTool *clone_tool;

  clone_tool = GIMP_CLONE_TOOL (data);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (clone_tool));
}


#if 0

static gpointer
gimp_clone_tool_non_gui_paint_func (GimpPaintTool    *paint_tool,
				    GimpDrawable     *drawable,
				    PaintState        state)
{
  gimp_clone_tool_motion (paint_tool, drawable, non_gui_src_drawable,
			  &non_gui_pressure_options,
			  non_gui_type, non_gui_offset_x, non_gui_offset_y);

  return NULL;
}

gboolean
gimp_clone_tool_non_gui_default (GimpDrawable *drawable,
				 gint          num_strokes,
				 gdouble      *stroke_array)
{
  GimpDrawable *src_drawable = NULL;
  CloneType     clone_type = CLONE_DEFAULT_TYPE;
  gdouble       local_src_x = 0.0;
  gdouble       local_src_y = 0.0;
  CloneOptions *options = clone_options;

  if (options)
    {
      clone_type   = options->type;
      src_drawable = the_src_drawable;
      local_src_x  = src_x;
      local_src_y  = src_y;
    }
  
  return gimp_clone_tool_non_gui (drawable,
				  src_drawable,
				  clone_type,
				  local_src_x,local_src_y,
				  num_strokes, stroke_array);
}

gboolean
gimp_clone_tool_non_gui (GimpDrawable *drawable,
			 GimpDrawable *src_drawable,
			 CloneType     clone_type,
			 gdouble       src_x,
			 gdouble       src_y,
			 gint          num_strokes,
			 gdouble      *stroke_array)
{
  gint i;

  if (gimp_paint_tool_start (&non_gui_paint_tool, drawable,
			     stroke_array[0], stroke_array[1]))
    {
      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = clone_non_gui_paint_func;
      
      non_gui_type = clone_type;

      non_gui_src_drawable = src_drawable;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      non_gui_offset_x = (int) (src_x - non_gui_paint_core.start_coords.x);
      non_gui_offset_y = (int) (src_y - non_gui_paint_core.start_coords.y);

      clone_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.cur_coords.x = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cur_coords.y = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.last_coords.x = non_gui_paint_core.cur_coords.x;
	  non_gui_paint_core.last_coords.y = non_gui_paint_core.cur_coords.y;
	}

      /* Finish the painting */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /* Cleanup */
      paint_core_cleanup ();
      return TRUE;
    }
  else
    return FALSE;
}

#endif


/*  too options stuff  */

static GimpToolOptions *
clone_options_new (GimpToolInfo *tool_info)
{
  CloneOptions *options;
  GtkWidget    *vbox;
  GtkWidget    *frame;

  options = g_new0 (CloneOptions, 1);

  paint_options_init ((PaintOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = clone_options_reset;

  options->type    = options->type_d    = CLONE_DEFAULT_TYPE;
  options->aligned = options->aligned_d = CLONE_DEFAULT_ALIGNED;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  frame = gimp_radio_group_new2 (TRUE, _("Source"),
				 G_CALLBACK (gimp_radio_button_update),
				 &options->type,
                                 GINT_TO_POINTER (options->type),

				 _("Image Source"),
                                 GINT_TO_POINTER (IMAGE_CLONE),
				 &options->type_w[0],

				 _("Pattern Source"),
                                 GINT_TO_POINTER (PATTERN_CLONE),
				 &options->type_w[1],

				 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame = gimp_radio_group_new2 (TRUE, _("Alignment"),
				 G_CALLBACK (gimp_radio_button_update),
				 &options->aligned,
                                 GINT_TO_POINTER (options->aligned),

				 _("Non Aligned"),
                                 GINT_TO_POINTER (ALIGN_NO),
				 &options->aligned_w[0],

				 _("Aligned"),
                                 GINT_TO_POINTER (ALIGN_YES),
				 &options->aligned_w[1],

				 _("Registered"),
                                 GINT_TO_POINTER (ALIGN_REGISTERED),
				 &options->aligned_w[2],

				 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  
  return (GimpToolOptions *) options;
}

static void
clone_options_reset (GimpToolOptions *tool_options)
{
  CloneOptions *options;

  options = (CloneOptions *) tool_options;

  paint_options_reset (tool_options);

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w[0]),
                               GINT_TO_POINTER (options->type_d));
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->aligned_w[0]),
                               GINT_TO_POINTER (options->aligned_d));
}
