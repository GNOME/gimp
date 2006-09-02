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

#include "paint/gimpheal.h"
#include "paint/gimphealoptions.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimphealtool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define TARGET_WIDTH  15
#define TARGET_HEIGHT 15


static gboolean      gimp_heal_tool_has_display   (GimpTool        *tool,
                                                   GimpDisplay     *display);
static GimpDisplay * gimp_heal_tool_has_image     (GimpTool        *tool,
                                                   GimpImage       *image);

static void          gimp_heal_tool_button_press  (GimpTool        *tool,
                                                   GimpCoords      *coords,
                                                   guint32          time,
                                                   GdkModifierType  state,
                                                   GimpDisplay     *display);

static void          gimp_heal_tool_control       (GimpTool        *tool,
                                                   GimpToolAction   action,
                                                   GimpDisplay     *display);

static void          gimp_heal_tool_motion        (GimpTool        *tool,
                                                   GimpCoords      *coords,
                                                   guint32          time,
                                                   GdkModifierType  state,
                                                   GimpDisplay     *display);

static void          gimp_heal_tool_cursor_update (GimpTool        *tool,
                                                   GimpCoords      *coords,
                                                   GdkModifierType  state,
                                                   GimpDisplay     *display);

static void          gimp_heal_tool_oper_update   (GimpTool        *tool,
                                                   GimpCoords      *coords,
                                                   GdkModifierType  state,
                                                   gboolean         proximity,
                                                   GimpDisplay     *display);

static void          gimp_heal_tool_draw          (GimpDrawTool    *draw_tool);

static GtkWidget   * gimp_heal_options_gui        (GimpToolOptions *tool_options);


G_DEFINE_TYPE (GimpHealTool, gimp_heal_tool, GIMP_TYPE_BRUSH_TOOL)


void
gimp_heal_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_HEAL_TOOL,
                GIMP_TYPE_HEAL_OPTIONS,
                gimp_heal_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK,
                "gimp-heal-tool",
                _("Heal"),
                _("Heal image irregularities"),
                N_("_Heal"),
                "H",
                NULL,
                GIMP_HELP_TOOL_HEAL,
                GIMP_STOCK_TOOL_HEAL,
                data);
}

static void
gimp_heal_tool_class_init (GimpHealToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->has_display   = gimp_heal_tool_has_display;
  tool_class->has_image     = gimp_heal_tool_has_image;
  tool_class->control       = gimp_heal_tool_control;
  tool_class->button_press  = gimp_heal_tool_button_press;
  tool_class->motion        = gimp_heal_tool_motion;
  tool_class->cursor_update = gimp_heal_tool_cursor_update;
  tool_class->oper_update   = gimp_heal_tool_oper_update;

  draw_tool_class->draw     = gimp_heal_tool_draw;
}

static void
gimp_heal_tool_init (GimpHealTool *heal)
{
  GimpTool      *tool       = GIMP_TOOL (heal);
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_HEAL);

  paint_tool->status      = _("Click to heal.");
  paint_tool->status_ctrl = _("%s to set a new heal source");
}

static gboolean
gimp_heal_tool_has_display (GimpTool    *tool,
                            GimpDisplay *display)
{
  GimpHealTool *heal_tool = GIMP_HEAL_TOOL (tool);

  return (display == heal_tool->src_display ||
          GIMP_TOOL_CLASS (gimp_heal_tool_parent_class)->has_display (tool,
                                                                      display));
}

static GimpDisplay *
gimp_heal_tool_has_image (GimpTool  *tool,
                          GimpImage *image)
{
  GimpHealTool *heal_tool = GIMP_HEAL_TOOL (tool);
  GimpDisplay  *display;

  display = GIMP_TOOL_CLASS (gimp_heal_tool_parent_class)->has_image (tool,
                                                                      image);

  if (! display && heal_tool->src_display)
    {
      if (image && heal_tool->src_display->image == image)
        display = heal_tool->src_display;

      /*  NULL image means any display  */
      if (! image)
        display = heal_tool->src_display;
    }

  return display;
}

static void
gimp_heal_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  GimpHealTool *heal_tool = GIMP_HEAL_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      heal_tool->src_display = NULL;
      g_object_set (GIMP_PAINT_TOOL (tool)->core,
                    "src-drawable", NULL,
                    NULL);
      break;
    }

  GIMP_TOOL_CLASS (gimp_heal_tool_parent_class)->control (tool, action, display);
}


static void
gimp_heal_tool_button_press (GimpTool        *tool,
                             GimpCoords      *coords,
                             guint32          time,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
  GimpPaintTool   *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpHealTool    *heal_tool  = GIMP_HEAL_TOOL (tool);
  GimpHeal        *heal       = GIMP_HEAL (paint_tool->core);
  GimpHealOptions *options;

  options = GIMP_HEAL_OPTIONS (tool->tool_info->tool_options);

  /* pause the tool before drawing */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /* use_saved_proj tells whether we keep the unmodified pixel projection
   * around.  in this case no.  */
  paint_tool->core->use_saved_proj = FALSE;

  /* state holds a set of bit-flags to indicate the state of modifier keys and
   * mouse buttons in various event types. Typical modifier keys are Shift,
   * Control, Meta, Super, Hyper, Alt, Compose, Apple, CapsLock or ShiftLock.
   * Part of gtk -> GdkModifierType */
  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
    /* we enter here only if CTRL is pressed */
    {
      /* mark that the source display has been set.  defined in gimpheal.h */
      heal->set_source = TRUE;

      /* if currently active display != the heal tools source display */
      if (display != heal_tool->src_display)
        {
          /* check if the heal tools src display has been previously set */
          if (heal_tool->src_display)
            /* if so remove the pointer to the display */
            g_object_remove_weak_pointer (G_OBJECT (heal_tool->src_display),
                                          (gpointer *) &heal_tool->src_display);

          /* set the heal tools source display to the currently active
           * display */
          heal_tool->src_display = display;

          /* add a pointer to the display */
          g_object_add_weak_pointer (G_OBJECT (display),
                                     (gpointer *) &heal_tool->src_display);
        }
    }
  else
    {
      /* note that the source is not being set */
      heal->set_source = FALSE;

      if ((options->sample_merged) && (display == heal_tool->src_display))
        {
          /* keep unmodified projection around */
          paint_tool->core->use_saved_proj = TRUE;
        }
    }

  /* chain up to call the parents functions */
  GIMP_TOOL_CLASS (gimp_heal_tool_parent_class)->button_press (tool, coords,
                                                               time, state,
                                                               display);

  /* set the tool display's source position to match the current heal
   * implementation source position */
  heal_tool->src_x = heal->src_x;
  heal_tool->src_y = heal->src_y;

  /* resume drawing */
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_heal_tool_motion (GimpTool        *tool,
                       GimpCoords      *coords,
                       guint32          time,
                       GdkModifierType  state,
                       GimpDisplay     *display)
{
  GimpHealTool  *heal_tool  = GIMP_HEAL_TOOL (tool);
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpHeal      *heal       = GIMP_HEAL (paint_tool->core);

  /* pause drawing */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /* check if CTRL is pressed */
  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
    /* if yes the source has been set */
    heal->set_source = TRUE;
  else
    /* if no the source has not been set */
    heal->set_source = FALSE;

  /* chain up to the parent classes motion function */
  GIMP_TOOL_CLASS (gimp_heal_tool_parent_class)->motion (tool, coords, time,
                                                         state, display);

  /* set the tool display's source to be the same as the heal implementation
   * source */
  heal_tool->src_x = heal->src_x;
  heal_tool->src_y = heal->src_y;

  /* resume drawing */
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_heal_tool_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpHealOptions    *options;
  GimpCursorType      cursor   = GIMP_CURSOR_MOUSE;
  GimpCursorModifier  modifier = GIMP_CURSOR_MODIFIER_NONE;

  options = GIMP_HEAL_OPTIONS (tool->tool_info->tool_options);

  /* if the cursor is in an active area */
  if (gimp_image_coords_in_active_pickable (display->image, coords,
                                            FALSE, TRUE))
    {
      /* set the cursor to the normal cursor */
      cursor = GIMP_CURSOR_MOUSE;
    }

  /* if CTRL is pressed, change cursor to cross-hair */
  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
    {
      cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
    }
  /* otherwise, let the cursor now that we can't paint */
  else if (! GIMP_HEAL (GIMP_PAINT_TOOL (tool)->core)->src_drawable)
    {
      modifier = GIMP_CURSOR_MODIFIER_BAD;
    }

  /* set the cursor and the modifier cursor */
  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  /* chain up to the parent class */
  GIMP_TOOL_CLASS (gimp_heal_tool_parent_class)->cursor_update (tool, coords,
                                                                state, display);
}

static void
gimp_heal_tool_oper_update (GimpTool        *tool,
                            GimpCoords      *coords,
                            GdkModifierType  state,
                            gboolean         proximity,
                            GimpDisplay     *display)
{
  GimpHealTool    *heal_tool = GIMP_HEAL_TOOL (tool);
  GimpHealOptions *options;

  options = GIMP_HEAL_OPTIONS (tool->tool_info->tool_options);

  if (proximity)
    {
      GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);

      paint_tool->status_ctrl = _("%s to set a new heal source");
    }

  /* chain up to the parent class */
  GIMP_TOOL_CLASS (gimp_heal_tool_parent_class)->oper_update (tool, coords,
                                                              state, proximity,
                                                              display);

  if (proximity)
    {
      GimpHeal *heal = GIMP_HEAL (GIMP_PAINT_TOOL (tool)->core);

      if (heal->src_drawable == NULL)
        {
          if (state & GDK_CONTROL_MASK)
            /* if we haven't set the source drawable yet, make a notice to do so */
            gimp_tool_replace_status (tool, display,
                                      _("Ctrl-Click to set a heal source."));
          else
            {
              gchar *status;
              status = g_strdup_printf (_("%s%sClick to set a heal source."),
                                        gimp_get_mod_name_control (),
                                        gimp_get_mod_separator ());
              gimp_tool_replace_status (tool, display, status);
              g_free (status);
            }
        }
      else
        {
          /* pause drawing */
          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

          /* set the tool source to match the implementation source */
          heal_tool->src_x = heal->src_x;
          heal_tool->src_y = heal->src_y;

          if (! heal->first_stroke)
            {
              /* set the coordinates based on the alignment type */
              if (options->align_mode == GIMP_HEAL_ALIGN_YES)
                {
                  heal_tool->src_x = coords->x + heal->offset_x;
                  heal_tool->src_y = coords->y + heal->offset_y;
                }
            }

          /* resume drawing */
          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
        }
    }
}

static void
gimp_heal_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool        *tool      = GIMP_TOOL (draw_tool);
  GimpHealTool    *heal_tool = GIMP_HEAL_TOOL (draw_tool);
  GimpHeal        *heal      = GIMP_HEAL (GIMP_PAINT_TOOL (tool)->core);
  GimpHealOptions *options;

  options = GIMP_HEAL_OPTIONS (tool->tool_info->tool_options);

  /* If we have a source drawable and display we can do the drawing */
  if ((heal->src_drawable) && (heal_tool->src_display))
    {
      /* make a temporary display and keep track of offsets */
      GimpDisplay *tmp_display;
      gint         off_x;
      gint         off_y;

      /* gimp_item_offsets reveals the X and Y offsets of the first parameter.
       * this gets the location of the drawable. */
      gimp_item_offsets (GIMP_ITEM (heal->src_drawable), &off_x, &off_y);

      /* store the display for later */
      tmp_display = draw_tool->display;

      /* set the display */
      draw_tool->display = heal_tool->src_display;

      /* convenience function to simplify drawing */
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_CROSS,
                                  heal_tool->src_x + off_x,
                                  heal_tool->src_y + off_y,
                                  TARGET_WIDTH, TARGET_HEIGHT,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);

      /* restore settings after drawing */
      draw_tool->display = tmp_display;
    }

  /* chain up to the parent draw function */
  GIMP_DRAW_TOOL_CLASS (gimp_heal_tool_parent_class)->draw (draw_tool);
}

static GtkWidget *
gimp_heal_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *combo;

  /* create and attach the sample merged checkbox */
  button = gimp_prop_check_button_new (config, "sample-merged",
                                       _("Sample merged"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* create and attach the alignment options to a table */
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
