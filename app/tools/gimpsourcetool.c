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

#include "core/core-types.h"
#include "libgimptool/gimptooltypes.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimptoolinfo.h"

#include "paint/gimpclone.h"

#include "display/gimpdisplay.h"

#include "widgets/gimpenummenu.h"

#include "gimpclonetool.h"
#include "paint_options.h"

#include "libgimp/gimpintl.h"


#define TARGET_WIDTH  15
#define TARGET_HEIGHT 15


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
gimp_clone_tool_register (GimpToolRegisterCallback  callback,
                          Gimp                     *gimp)
{
  (* callback) (GIMP_TYPE_CLONE_TOOL,
                clone_options_new,
                TRUE,
                "gimp-clone-tool",
                _("Clone"),
                _("Paint using Patterns or Image Regions"),
                N_("/Tools/Paint Tools/Clone"), "C",
                NULL, "tools/clone.html",
                GIMP_STOCK_TOOL_CLONE,
                gimp);
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

  tool->control = gimp_tool_control_new  (FALSE,                      /* scroll_lock */
                                          TRUE,                       /* auto_snap_to */
                                          TRUE,                       /* preserve */
                                          FALSE,                      /* handle_empty_image */
                                          GIMP_MOTION_MODE_EXACT,     /* motion_mode */
                                          GIMP_MOUSE_CURSOR,          /* cursor */
                                          GIMP_CLONE_TOOL_CURSOR,     /* tool_cursor */
                                          GIMP_CURSOR_MODIFIER_NONE,  /* cursor_modifier */
                                          GIMP_MOUSE_CURSOR,          /* toggle_cursor */
                                          GIMP_TOOL_CURSOR_NONE,      /* toggle_tool_cursor */
                                          GIMP_CURSOR_MODIFIER_NONE   /* toggle_cursor_modifier */);


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
  GimpCloneOptions *options;
  GimpLayer        *layer;
  GdkCursorType     ctype = GIMP_MOUSE_CURSOR;

  options = (GimpCloneOptions *) tool->tool_info->tool_options;

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

  if (options->type == GIMP_IMAGE_CLONE)
    {
      if (state & GDK_CONTROL_MASK)
	ctype = GIMP_CROSSHAIR_SMALL_CURSOR;
      else if (! GIMP_CLONE (GIMP_PAINT_TOOL (tool)->core)->src_drawable)
	ctype = GIMP_BAD_CURSOR;
    }

  gimp_tool_control_set_cursor(tool->control, ctype);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_clone_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (draw_tool);

  if (gimp_tool_control_is_active(tool->control))
    {
      GimpCloneOptions *options;

      options = (GimpCloneOptions *) tool->tool_info->tool_options;

      if (draw_tool->gdisp && options->type == GIMP_IMAGE_CLONE)
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


/*  tool options stuff  */

static GimpToolOptions *
clone_options_new (GimpToolInfo *tool_info)
{
  GimpCloneOptions *options;
  GtkWidget        *vbox;
  GtkWidget        *frame;

  options = gimp_clone_options_new ();

  paint_options_init ((GimpPaintOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = clone_options_reset;

  /*  the main vbox  */
  vbox = ((GimpToolOptions *) options)->main_vbox;

  frame = gimp_enum_radio_frame_new (GIMP_TYPE_CLONE_TYPE,
                                     gtk_label_new (_("Source")),
                                     2,
                                     G_CALLBACK (gimp_radio_button_update),
                                     &options->type,
                                     &options->type_w);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                               GINT_TO_POINTER (options->type));

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame = gimp_enum_radio_frame_new (GIMP_TYPE_CLONE_ALIGN_MODE,
                                     gtk_label_new (_("Alignment")),
                                     2,
                                     G_CALLBACK (gimp_radio_button_update),
                                     &options->aligned,
                                     &options->aligned_w);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->aligned_w),
                               GINT_TO_POINTER (options->aligned));

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  
  return (GimpToolOptions *) options;
}

static void
clone_options_reset (GimpToolOptions *tool_options)
{
  GimpCloneOptions *options;

  options = (GimpCloneOptions *) tool_options;

  paint_options_reset (tool_options);

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                               GINT_TO_POINTER (options->type_d));
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->aligned_w),
                               GINT_TO_POINTER (options->aligned_d));
}
