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


static gboolean      gimp_clone_tool_has_display   (GimpTool        *tool,
                                                    GimpDisplay     *display);
static GimpDisplay * gimp_clone_tool_has_image     (GimpTool        *tool,
                                                    GimpImage       *image);
static void          gimp_clone_tool_control       (GimpTool        *tool,
                                                    GimpToolAction   action,
                                                    GimpDisplay     *display);
static void          gimp_clone_tool_button_press  (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *display);
static void          gimp_clone_tool_motion        (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *display);
static void          gimp_clone_tool_cursor_update (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *display);
static void          gimp_clone_tool_modifier_key  (GimpTool        *tool,
                                                    GdkModifierType  key,
                                                    gboolean         press,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *display);
static void          gimp_clone_tool_oper_update   (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    GdkModifierType  state,
                                                    gboolean         proximity,
                                                    GimpDisplay     *display);

static void          gimp_clone_tool_draw          (GimpDrawTool    *draw_tool);

static GtkWidget   * gimp_clone_options_gui        (GimpToolOptions *tool_options);


G_DEFINE_TYPE (GimpCloneTool, gimp_clone_tool, GIMP_TYPE_BRUSH_TOOL)

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

  tool_class->has_display   = gimp_clone_tool_has_display;
  tool_class->has_image     = gimp_clone_tool_has_image;
  tool_class->control       = gimp_clone_tool_control;
  tool_class->button_press  = gimp_clone_tool_button_press;
  tool_class->motion        = gimp_clone_tool_motion;
  tool_class->modifier_key  = gimp_clone_tool_modifier_key;
  tool_class->oper_update   = gimp_clone_tool_oper_update;
  tool_class->cursor_update = gimp_clone_tool_cursor_update;

  draw_tool_class->draw     = gimp_clone_tool_draw;
}

static void
gimp_clone_tool_init (GimpCloneTool *clone)
{
  GimpTool      *tool       = GIMP_TOOL (clone);
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);

  gimp_tool_control_set_tool_cursor     (tool->control,
                                         GIMP_TOOL_CURSOR_CLONE);
  gimp_tool_control_set_action_object_2 (tool->control,
                                         "context/context-pattern-select-set");

  paint_tool->status      = _("Click to clone.");
  paint_tool->status_ctrl = _("%s to set a new clone source");
}

static gboolean
gimp_clone_tool_has_display (GimpTool    *tool,
                             GimpDisplay *display)
{
  GimpCloneTool *clone_tool = GIMP_CLONE_TOOL (tool);

  return (display == clone_tool->src_display ||
          GIMP_TOOL_CLASS (parent_class)->has_display (tool, display));
}

static GimpDisplay *
gimp_clone_tool_has_image (GimpTool  *tool,
                           GimpImage *image)
{
  GimpCloneTool *clone_tool = GIMP_CLONE_TOOL (tool);
  GimpDisplay   *display;

  display = GIMP_TOOL_CLASS (parent_class)->has_image (tool, image);

  if (! display && clone_tool->src_display)
    {
      if (image && clone_tool->src_display->image == image)
        display = clone_tool->src_display;

      /*  NULL image means any display  */
      if (! image)
        display = clone_tool->src_display;
    }

  return display;
}

static void
gimp_clone_tool_control (GimpTool       *tool,
                         GimpToolAction  action,
                         GimpDisplay    *display)
{
  GimpCloneTool *clone_tool = GIMP_CLONE_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      clone_tool->src_display = NULL;
      g_object_set (GIMP_PAINT_TOOL (tool)->core,
                    "src-drawable", NULL,
                    NULL);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_clone_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpPaintTool    *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpCloneTool    *clone_tool = GIMP_CLONE_TOOL (tool);
  GimpClone        *clone      = GIMP_CLONE (paint_tool->core);
  GimpCloneOptions *options;

  options = GIMP_CLONE_OPTIONS (tool->tool_info->tool_options);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  paint_tool->core->use_saved_proj = FALSE;

  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
    {
      clone->set_source = TRUE;

      clone_tool->src_display = display;
    }
  else
    {
      clone->set_source = FALSE;

      if (options->clone_type == GIMP_IMAGE_CLONE &&
          options->sample_merged                  &&
          display == clone_tool->src_display)
        {
          paint_tool->core->use_saved_proj = TRUE;
        }
    }

  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                display);

  clone_tool->src_x = clone->src_x;
  clone_tool->src_y = clone->src_y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_clone_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *display)
{
  GimpCloneTool *clone_tool = GIMP_CLONE_TOOL (tool);
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpClone     *clone      = GIMP_CLONE (paint_tool->core);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
    clone->set_source = TRUE;
  else
    clone->set_source = FALSE;

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  clone_tool->src_x = clone->src_x;
  clone_tool->src_y = clone->src_y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_clone_tool_modifier_key (GimpTool        *tool,
                              GdkModifierType  key,
                              gboolean         press,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpPaintTool    *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpCloneOptions *options;

  options = GIMP_CLONE_OPTIONS (tool->tool_info->tool_options);

  if ( (! (state & GDK_BUTTON1_MASK)) &&
       options->clone_type == GIMP_IMAGE_CLONE
       && key == GDK_CONTROL_MASK)
    {
      if (press)
        paint_tool->status = _("Click to set the clone source.");
      else
        paint_tool->status = _("Click to clone.");
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);

}

static void
gimp_clone_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  GimpCloneOptions   *options;
  GimpCursorType      cursor   = GIMP_CURSOR_MOUSE;
  GimpCursorModifier  modifier = GIMP_CURSOR_MODIFIER_NONE;

  options = GIMP_CLONE_OPTIONS (tool->tool_info->tool_options);

  if (gimp_image_coords_in_active_pickable (display->image, coords,
                                            FALSE, TRUE))
    {
      cursor = GIMP_CURSOR_MOUSE;
    }

  if (options->clone_type == GIMP_IMAGE_CLONE)
    {
      if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
        {
          cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
        }
      else if (! GIMP_CLONE (GIMP_PAINT_TOOL (tool)->core)->src_drawable)
        {
          modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_clone_tool_oper_update (GimpTool        *tool,
                             GimpCoords      *coords,
                             GdkModifierType  state,
                             gboolean         proximity,
                             GimpDisplay     *display)
{
  GimpCloneTool    *clone_tool = GIMP_CLONE_TOOL (tool);
  GimpCloneOptions *options;

  options = GIMP_CLONE_OPTIONS (tool->tool_info->tool_options);

  if (proximity)
    {
      GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);

      if (options->clone_type == GIMP_IMAGE_CLONE)
        paint_tool->status_ctrl = _("%s to set a new clone source");
      else
        paint_tool->status_ctrl = NULL;
    }

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  if (options->clone_type == GIMP_IMAGE_CLONE && proximity)
    {
      GimpClone *clone = GIMP_CLONE (GIMP_PAINT_TOOL (tool)->core);

      if (clone->src_drawable == NULL)
        {
          if (state & GDK_CONTROL_MASK)
            gimp_tool_replace_status (tool, display,
                                      _("Click to set the clone source."));
          else
            {
              gchar *status;
              status = g_strdup_printf (_("%s%sClick to set a clone source."),
                                        gimp_get_mod_name_control (),
                                        gimp_get_mod_separator ());
              gimp_tool_replace_status (tool, display, status);
              g_free (status);
            }
        }
      else
        {
          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

          clone_tool->src_x = clone->src_x;
          clone_tool->src_y = clone->src_y;

          if (! clone->first_stroke)
            {
              if (options->align_mode == GIMP_CLONE_ALIGN_YES)
                {
                  clone_tool->src_x = coords->x + clone->offset_x;
                  clone_tool->src_y = coords->y + clone->offset_y;
                }
              else if (options->align_mode == GIMP_CLONE_ALIGN_REGISTERED)
                {
                  clone_tool->src_x = coords->x;
                  clone_tool->src_y = coords->y;
                }
            }

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
        }
    }
}

static void
gimp_clone_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool         *tool       = GIMP_TOOL (draw_tool);
  GimpCloneTool    *clone_tool = GIMP_CLONE_TOOL (draw_tool);
  GimpClone        *clone      = GIMP_CLONE (GIMP_PAINT_TOOL (tool)->core);
  GimpCloneOptions *options;

  options = GIMP_CLONE_OPTIONS (tool->tool_info->tool_options);

  if (options->clone_type == GIMP_IMAGE_CLONE &&
      clone->src_drawable && clone_tool->src_display)
    {
      GimpDisplay *tmp_display;
      gint         off_x;
      gint         off_y;

      gimp_item_offsets (GIMP_ITEM (clone->src_drawable), &off_x, &off_y);

      tmp_display = draw_tool->display;
      draw_tool->display = clone_tool->src_display;

      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_CROSS,
                                  clone_tool->src_x + off_x,
                                  clone_tool->src_y + off_y,
                                  TARGET_WIDTH, TARGET_WIDTH,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);

      draw_tool->display = tmp_display;
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
