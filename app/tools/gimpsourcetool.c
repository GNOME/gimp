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

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimptoolinfo.h"

#include "paint/gimpclone.h"
#include "paint/gimpcloneoptions.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimpclonetool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define TARGET_WIDTH  15
#define TARGET_HEIGHT 15


static void   gimp_clone_tool_button_press  (GimpTool        *tool,
                                             GimpCoords      *coords,
                                             guint32          time,
                                             GdkModifierType  state,
                                             GimpDisplay     *gdisp);
static void   gimp_clone_tool_motion        (GimpTool        *tool,
                                             GimpCoords      *coords,
                                             guint32          time,
                                             GdkModifierType  state,
                                             GimpDisplay     *gdisp);

static void   gimp_clone_tool_cursor_update (GimpTool        *tool,
                                             GimpCoords      *coords,
                                             GdkModifierType  state,
                                             GimpDisplay     *gdisp);
static void   gimp_clone_tool_oper_update   (GimpTool        *tool,
                                             GimpCoords      *coords,
                                             GdkModifierType  state,
                                             GimpDisplay     *gdisp);

static void   gimp_clone_tool_draw          (GimpDrawTool    *draw_tool);

static GtkWidget * gimp_clone_options_gui   (GimpToolOptions *tool_options);


G_DEFINE_TYPE (GimpCloneTool, gimp_clone_tool, GIMP_TYPE_PAINT_TOOL);

#define parent_class gimp_clone_tool_parent_class


void
gimp_clone_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_CLONE_TOOL,
                GIMP_TYPE_CLONE_OPTIONS,
                gimp_clone_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |
                GIMP_CONTEXT_PATTERN_MASK,
                "gimp-clone-tool",
                _("Clone"),
                _("Paint using Patterns or Image Regions"),
                N_("_Clone"), "C",
                NULL, GIMP_HELP_TOOL_CLONE,
                GIMP_STOCK_TOOL_CLONE,
                data);
}

static void
gimp_clone_tool_class_init (GimpCloneToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->button_press  = gimp_clone_tool_button_press;
  tool_class->motion        = gimp_clone_tool_motion;
  tool_class->cursor_update = gimp_clone_tool_cursor_update;
  tool_class->oper_update   = gimp_clone_tool_oper_update;

  draw_tool_class->draw     = gimp_clone_tool_draw;
}

static void
gimp_clone_tool_init (GimpCloneTool *clone)
{
  GimpTool *tool = GIMP_TOOL (clone);

  gimp_tool_control_set_tool_cursor     (tool->control,
                                         GIMP_TOOL_CURSOR_CLONE);
  gimp_tool_control_set_action_object_2 (tool->control,
                                         "context/context-pattern-select-set");
}

static void
gimp_clone_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpPaintTool    *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpCloneTool    *clone_tool = GIMP_CLONE_TOOL (tool);
  GimpCloneOptions *options;

  options = GIMP_CLONE_OPTIONS (tool->tool_info->tool_options);

  paint_tool->core->use_saved_proj = FALSE;

  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
    {
      GIMP_CLONE (paint_tool->core)->set_source = TRUE;

      if (gdisp != clone_tool->src_gdisp)
        {
          if (clone_tool->src_gdisp)
            g_object_remove_weak_pointer (G_OBJECT (clone_tool->src_gdisp),
                                          (gpointer *) &clone_tool->src_gdisp);

          clone_tool->src_gdisp = gdisp;
          g_object_add_weak_pointer (G_OBJECT (gdisp),
                                     (gpointer *) &clone_tool->src_gdisp);
        }
    }
  else
    {
      GIMP_CLONE (paint_tool->core)->set_source = FALSE;

      if (options->clone_type == GIMP_IMAGE_CLONE &&
          options->sample_merged                  &&
          gdisp == clone_tool->src_gdisp)
        {
          paint_tool->core->use_saved_proj = TRUE;
        }
    }

  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                gdisp);
}

static void
gimp_clone_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *gdisp)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);

  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
    GIMP_CLONE (paint_tool->core)->set_source = TRUE;
  else
    GIMP_CLONE (paint_tool->core)->set_source = FALSE;

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, gdisp);
}

static void
gimp_clone_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
                               GdkModifierType  state,
                               GimpDisplay     *gdisp)
{
  GimpCloneOptions *options;
  GimpCursorType    ctype = GIMP_CURSOR_MOUSE;

  options = (GimpCloneOptions *) tool->tool_info->tool_options;

  if (gimp_image_coords_in_active_drawable (gdisp->gimage, coords))
    {
      GimpChannel *selection = gimp_image_get_mask (gdisp->gimage);

      /*  One more test--is there a selected region?
       *  if so, is cursor inside?
       */
      if (gimp_channel_is_empty (selection) ||
          gimp_pickable_get_opacity_at (GIMP_PICKABLE (selection),
                                        coords->x, coords->y))
        ctype = GIMP_CURSOR_MOUSE;
    }

  if (options->clone_type == GIMP_IMAGE_CLONE)
    {
      if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
        ctype = GIMP_CURSOR_CROSSHAIR_SMALL;
      else if (! GIMP_CLONE (GIMP_PAINT_TOOL (tool)->core)->src_drawable)
        ctype = GIMP_CURSOR_BAD;
    }

  gimp_tool_control_set_cursor (tool->control, ctype);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_clone_tool_oper_update (GimpTool        *tool,
                             GimpCoords      *coords,
                             GdkModifierType  state,
                             GimpDisplay     *gdisp)
{
  GimpToolOptions *options = tool->tool_info->tool_options;

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, gdisp);

  if (GIMP_CLONE_OPTIONS (options)->clone_type == GIMP_IMAGE_CLONE &&
      GIMP_CLONE (GIMP_PAINT_TOOL (tool)->core)->src_drawable == NULL)
    {
      gimp_tool_replace_status (tool, gdisp,
                                _("Ctrl-Click to set a clone source."));
    }
}

static void
gimp_clone_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool *tool = GIMP_TOOL (draw_tool);

  if (gimp_tool_control_is_active (tool->control))
    {
      GimpClone        *clone = GIMP_CLONE (GIMP_PAINT_TOOL (draw_tool)->core);
      GimpCloneOptions *options;

      options = (GimpCloneOptions *) tool->tool_info->tool_options;

      if (options->clone_type == GIMP_IMAGE_CLONE && clone->src_drawable)
        {
          gint           off_x;
          gint           off_y;
          GimpDisplay   *tmp_gdisp;
          GimpCloneTool *clone_tool = GIMP_CLONE_TOOL (draw_tool);

          gimp_item_offsets (GIMP_ITEM (clone->src_drawable), &off_x, &off_y);

          tmp_gdisp = draw_tool->gdisp;
          draw_tool->gdisp = clone_tool->src_gdisp;

          if (draw_tool->gdisp)
            gimp_draw_tool_draw_handle (draw_tool,
                                        GIMP_HANDLE_CROSS,
                                        clone->src_x + off_x,
                                        clone->src_y + off_y,
                                        TARGET_WIDTH, TARGET_WIDTH,
                                        GTK_ANCHOR_CENTER,
                                        FALSE);

          draw_tool->gdisp = tmp_gdisp;
        }
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}


/*  tool options stuff  */

static GtkWidget *
gimp_clone_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *combo;

  vbox = gimp_paint_options_gui (tool_options);

  frame = gimp_prop_enum_radio_frame_new (config, "clone-type",
                                          _("Source"), 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  button = gimp_prop_check_button_new (config, "sample-merged",
                                       _("Sample merged"));
  gimp_enum_radio_frame_add (GTK_FRAME (frame), button, GIMP_IMAGE_CLONE);

  hbox = gimp_prop_pattern_box_new (NULL, GIMP_CONTEXT (tool_options), 2,
                                    "pattern-view-type", "pattern-view-size");
  gimp_enum_radio_frame_add (GTK_FRAME (frame), hbox, GIMP_PATTERN_CLONE);

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  combo = gimp_prop_enum_combo_box_new (config, "align-mode", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Alignment:"), 0.0, 0.5,
                             combo, 1, FALSE);

  return vbox;
}
