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

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimptoolinfo.h"
#include "core/gimpitem-align.h"

#include "display/gimpdisplay.h"

#include "widgets/gimphelp-ids.h"

#include "gimpdrawtool.h"
#include "gimpalignoptions.h"
#include "gimpaligntool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GObject * gimp_align_tool_constructor (GType              type,
                                              guint              n_params,
                                              GObjectConstructParam *params);
static void   gimp_align_tool_dispose        (GObject           *object);
static gboolean gimp_align_tool_initialize   (GimpTool          *tool,
                                              GimpDisplay       *gdisp);
static void   gimp_align_tool_finalize       (GObject           *object);

static void   gimp_align_tool_button_press   (GimpTool          *tool,
                                              GimpCoords        *coords,
                                              guint32            time,
                                              GdkModifierType    state,
                                              GimpDisplay       *gdisp);
static void   gimp_align_tool_cursor_update  (GimpTool          *tool,
                                              GimpCoords        *coords,
                                              GdkModifierType    state,
                                              GimpDisplay       *gdisp);

static void   gimp_align_tool_draw           (GimpDrawTool      *draw_tool);

static GtkWidget *button_with_stock          (GimpAlignmentType  action,
                                              GimpAlignTool     *align_tool);
static GtkWidget *gimp_align_tool_controls   (GimpAlignTool     *align_tool);
static void   set_action                     (GtkWidget         *widget,
                                              gpointer           data);
static void   do_horizontal_alignment        (GtkWidget         *widget,
                                              gpointer           data);
static void   do_vertical_alignment          (GtkWidget         *widget,
                                              gpointer           data);
static void   clear_reference                (GimpItem          *reference_item,
                                              GimpAlignTool     *align_tool);
static void   clear_target                   (GimpItem          *target_item,
                                              GimpAlignTool     *align_tool);


G_DEFINE_TYPE (GimpAlignTool, gimp_align_tool, GIMP_TYPE_DRAW_TOOL);

#define parent_class gimp_align_tool_parent_class


void
gimp_align_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_ALIGN_TOOL,
                GIMP_TYPE_ALIGN_OPTIONS,
                gimp_align_options_gui,
                0,
                "gimp-align-tool",
                _("Align"),
                _("Align layers & other items"),
                N_("_Align"), "Q",
                NULL, GIMP_HELP_TOOL_MOVE,
                GIMP_STOCK_CENTER,
                data);
}

static void
gimp_align_tool_class_init (GimpAlignToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = gimp_align_tool_finalize;
  object_class->constructor  = gimp_align_tool_constructor;
  object_class->dispose      = gimp_align_tool_dispose;

  tool_class->initialize     = gimp_align_tool_initialize;
  tool_class->button_press   = gimp_align_tool_button_press;
  tool_class->cursor_update  = gimp_align_tool_cursor_update;

  draw_tool_class->draw      = gimp_align_tool_draw;
}

static void
gimp_align_tool_init (GimpAlignTool *align_tool)
{
  GimpTool *tool = GIMP_TOOL (align_tool);

  align_tool->controls        = NULL;

  align_tool->target_item     = NULL;
  align_tool->reference_item  = NULL;

  align_tool->select_reference = FALSE;

  align_tool->target_horz_align_type = GIMP_ALIGN_LEFT;
  align_tool->ref_horz_align_type    = GIMP_ALIGN_LEFT;
  align_tool->target_vert_align_type = GIMP_ALIGN_TOP;
  align_tool->ref_vert_align_type    = GIMP_ALIGN_TOP;

  align_tool->horz_offset = 0;
  align_tool->vert_offset = 0;

  gimp_tool_control_set_snap_to     (tool->control, FALSE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_MOVE);

}

static GObject *
gimp_align_tool_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject       *object;
  GimpTool      *tool;
  GimpAlignTool *align_tool;
  GtkContainer  *controls_container;
  GObject       *options;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  tool       = GIMP_TOOL (object);
  align_tool = GIMP_ALIGN_TOOL (object);

  g_assert (GIMP_IS_TOOL_INFO (tool->tool_info));

  options = G_OBJECT (tool->tool_info->tool_options);

  controls_container = GTK_CONTAINER (g_object_get_data (options,
                                                         "controls-container"));

  align_tool->controls = gimp_align_tool_controls (align_tool);
  gtk_container_add (controls_container, align_tool->controls);

  return object;
}

static void
gimp_align_tool_dispose (GObject *object)
{
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (object);

  if (align_tool->reference_item)
    {
      g_signal_handlers_disconnect_by_func (align_tool->reference_item,
                                            G_CALLBACK (clear_reference),
                                            (gpointer) align_tool);
      align_tool->reference_item = NULL;
    }

  if (align_tool->target_item)
    {
      g_signal_handlers_disconnect_by_func (align_tool->target_item,
                                            G_CALLBACK (clear_target),
                                            (gpointer) align_tool);
      align_tool->target_item = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_align_tool_finalize (GObject *object)
{
  GimpTool      *tool       = GIMP_TOOL (object);
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (object);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (object)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (object));

  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);

  if (align_tool->controls)
    {
      gtk_widget_destroy (align_tool->controls);
      align_tool->controls = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_align_tool_initialize (GimpTool    *tool,
                            GimpDisplay *gdisp)
{
  GimpAlignTool    *align_tool = GIMP_ALIGN_TOOL (tool);

  if (tool->gdisp != gdisp)
    {
/*       align_tool->target_item     = NULL; */
/*       align_tool->reference_item  = NULL; */
    }

  return TRUE;
}

static void
gimp_align_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpAlignTool    *align_tool  = GIMP_ALIGN_TOOL (tool);
  GimpAlignOptions *options     = GIMP_ALIGN_OPTIONS (tool->tool_info->tool_options);
  GimpItem         *item        = NULL;

  /*  If the tool was being used in another image...reset it  */

  if (gdisp != tool->gdisp)
    {
      if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
        gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

      align_tool->target_item     = NULL;
      align_tool->reference_item  = NULL;
    }

  if (! gimp_tool_control_is_active (tool->control))
    gimp_tool_control_activate (tool->control);

  tool->gdisp = gdisp;

  if (! gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (options->align_type == GIMP_TRANSFORM_TYPE_PATH)
    {
      GimpVectors *vectors;

      if (gimp_draw_tool_on_vectors (GIMP_DRAW_TOOL (tool), gdisp,
                                     coords, 7, 7,
                                     NULL, NULL, NULL, NULL, NULL,
                                     &vectors))
        {
          item = GIMP_ITEM (vectors);
        }
    }
  else if (options->align_type == GIMP_TRANSFORM_TYPE_LAYER)
    {

      GimpLayer *layer;

      if ((layer = gimp_image_pick_correlate_layer (gdisp->gimage,
                                                         coords->x,
                                                         coords->y)))
        {
          item = GIMP_ITEM (layer);
        }
    }

  if (item)
    {
      if (state & GDK_CONTROL_MASK || align_tool->select_reference)
        {
          if (align_tool->reference_item)
            g_signal_handlers_disconnect_by_func (align_tool->reference_item,
                                                  G_CALLBACK (clear_reference),
                                                  (gpointer) align_tool);
          align_tool->reference_item = item;
          g_signal_connect (item, "removed",
                            G_CALLBACK (clear_reference), (gpointer) align_tool);
        }
      else
        {
          if (align_tool->target_item)
            g_signal_handlers_disconnect_by_func (align_tool->target_item,
                                                  G_CALLBACK (clear_target),
                                                  (gpointer) align_tool);
          align_tool->target_item = item;
          g_signal_connect (item, "removed",
                            G_CALLBACK (clear_target), (gpointer) align_tool);
        }
    }

  if (align_tool->target_item)
    {
      gint i;

      for (i = 0; i < ALIGN_TOOL_NUM_BUTTONS; i++)
        gtk_widget_set_sensitive (align_tool->button[i], TRUE);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_align_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
                               GdkModifierType  state,
                               GimpDisplay     *gdisp)
{
  GimpAlignOptions  *options = GIMP_ALIGN_OPTIONS (tool->tool_info->tool_options);

  GimpCursorType     cursor      = GIMP_CURSOR_BAD;
  GimpToolCursorType tool_cursor = GIMP_TOOL_CURSOR_MOVE;
  GimpCursorModifier modifier    = GIMP_CURSOR_MODIFIER_NONE;

  if (options->align_type == GIMP_TRANSFORM_TYPE_PATH)
    {
      tool_cursor = GIMP_TOOL_CURSOR_PATHS;
      modifier    = GIMP_CURSOR_MODIFIER_MOVE;

      if (gimp_draw_tool_on_vectors (GIMP_DRAW_TOOL (tool), gdisp,
                                     coords, 7, 7,
                                     NULL, NULL, NULL, NULL, NULL, NULL))
        {
          cursor      = GIMP_CURSOR_MOUSE;
          tool_cursor = GIMP_TOOL_CURSOR_HAND;
        }
    }
  else
    {
      GimpLayer *layer;

      if ((layer = gimp_image_pick_correlate_layer (gdisp->gimage,
                                                         coords->x, coords->y)))
	{
	  /*  if there is a floating selection, and this aint it...  */
	  if (gimp_image_floating_sel (gdisp->gimage) &&
	      ! gimp_layer_is_floating_sel (layer))
	    {
              cursor      = GIMP_CURSOR_MOUSE;
              tool_cursor = GIMP_TOOL_CURSOR_MOVE;
              modifier    = GIMP_CURSOR_MODIFIER_ANCHOR;
	    }
	  else if (layer == gimp_image_get_active_layer (gdisp->gimage))
	    {
              cursor = GIMP_CURSOR_MOUSE;
	    }
	  else
	    {
              cursor      = GIMP_CURSOR_MOUSE;
              tool_cursor = GIMP_TOOL_CURSOR_HAND;
              modifier    = GIMP_CURSOR_MODIFIER_MOVE;
	    }
	}
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_align_tool_draw (GimpDrawTool *draw_tool)
{
  GimpAlignTool *align = GIMP_ALIGN_TOOL (draw_tool);
  GimpItem      *item;

  if ((item = align->target_item))
    {
      gimp_draw_tool_draw_rectangle (draw_tool, FALSE,
                                     item->offset_x + 1, item->offset_y + 1,
                                     item->width - 2, item->height - 2,
                                     FALSE);
    }

  if ((item = align->reference_item))
    {
      gimp_draw_tool_draw_rectangle (draw_tool, FALSE,
                                     item->offset_x, item->offset_y,
                                     item->width, item->height,
                                     FALSE);
      gimp_draw_tool_draw_dashed_line (draw_tool,
                                       item->offset_x,
                                       item->offset_y,
                                       item->offset_x + item->width,
                                       item->offset_y + item->height,
                                       FALSE);
      gimp_draw_tool_draw_dashed_line (draw_tool,
                                       item->offset_x,
                                       item->offset_y + item->height,
                                       item->offset_x + item->width,
                                       item->offset_y,
                                       FALSE);
    }
}


static GtkWidget *
gimp_align_tool_controls (GimpAlignTool *align_tool)
{
  GtkWidget *main_vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *spinbutton;
  gint       row, col, n;

  hbox = gtk_hbox_new (FALSE, 0);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), main_vbox, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_widget_show (main_vbox);

  vbox2 = gimp_int_radio_group_new (FALSE, NULL,
                                    G_CALLBACK (gimp_radio_button_update),
                                    &align_tool->select_reference, FALSE,

                                    _("Select target"),
                                    FALSE, NULL,

                                    _("Select reference (Ctrl)"),
                                    TRUE, NULL,
                                    NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  table = gtk_table_new (7, 9, FALSE);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 5);
  gtk_widget_show (table);

  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 10);

  row = col = 0;

  /* Top row */
  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox2, 0, 8, row, row + 1);
  gtk_widget_show (hbox2);
  label = gtk_label_new (_("Horizontal"));
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  row++;

  /* second row */

  col = 1;
  n = 0;

  button = button_with_stock (GIMP_ALIGN_LEFT, align_tool);
  gtk_table_attach_defaults (GTK_TABLE (table), button, col, col + 2, row, row + 1);
  gimp_help_set_help_data (button, _("Align left edge of target"), NULL);
  align_tool->button[n++] = button;
  ++col;
  ++col;

  button = button_with_stock (GIMP_ALIGN_CENTER, align_tool);
  gtk_table_attach_defaults (GTK_TABLE (table), button, col, col + 2, row, row + 1);
  gimp_help_set_help_data (button, _("Align center of target"), NULL);
  align_tool->button[n++] = button;
  ++col;
  ++col;

  button = button_with_stock (GIMP_ALIGN_RIGHT, align_tool);
  gtk_table_attach_defaults (GTK_TABLE (table), button, col, col + 2, row, row + 1);
  gimp_help_set_help_data (button, _("Align right edge of target"), NULL);
  align_tool->button[n++] = button;
  ++col;
  ++col;

  row++;

  /* next row */
  label = gtk_label_new (_("Offset:"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 3, row, row + 1);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&align_tool->horz_offset_adjustment,
                                     0,
                                     -100000.,
                                     100000.,
                                     1., 20., 20., 1., 0);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 3, 7, row, row + 1);
  g_signal_connect (align_tool->horz_offset_adjustment, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &align_tool->horz_offset);
  gtk_widget_show (spinbutton);

  row++;

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox2, 0, 8, row, row + 1);
  gtk_widget_show (hbox2);
  label = gtk_label_new (_("Vertical"));
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  row++;

  /* second row */

  col = 1;

  button = button_with_stock (GIMP_ALIGN_TOP, align_tool);
  gtk_table_attach_defaults (GTK_TABLE (table), button, col, col + 2, row, row + 1);
  gimp_help_set_help_data (button, _("Align top edge of target"), NULL);
  align_tool->button[n++] = button;
  ++col;
  ++col;

  button = button_with_stock (GIMP_ALIGN_MIDDLE, align_tool);
  gtk_table_attach_defaults (GTK_TABLE (table), button, col, col + 2, row, row + 1);
  gimp_help_set_help_data (button, _("Align middle of target"), NULL);
  align_tool->button[n++] = button;
  ++col;
  ++col;

  button = button_with_stock (GIMP_ALIGN_BOTTOM, align_tool);
  gtk_table_attach_defaults (GTK_TABLE (table), button, col, col + 2, row, row + 1);
  gimp_help_set_help_data (button, _("Align bottom of target"), NULL);
  align_tool->button[n++] = button;
  ++col;
  ++col;

  row++;

  /* next row */
  label = gtk_label_new (_("Offset:"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 3, row, row + 1);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&align_tool->vert_offset_adjustment,
                                     0,
                                     -100000.,
                                     100000.,
                                     1., 20., 20., 1., 0);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 3, 7, row, row + 1);
  g_signal_connect (align_tool->vert_offset_adjustment, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &align_tool->vert_offset);
  gtk_widget_show (spinbutton);

  gtk_widget_show (hbox);
  return hbox;
}


static void
do_horizontal_alignment (GtkWidget *widget,
                         gpointer   data)
{
  GimpAlignTool *align_tool = data;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  if (align_tool->target_item)
    gimp_item_align (align_tool->target_item,
                     align_tool->target_horz_align_type,
                     align_tool->reference_item,
                     align_tool->ref_horz_align_type,
                     align_tool->horz_offset);

  if (GIMP_TOOL (align_tool)->gdisp)
    gimp_image_flush (GIMP_TOOL (align_tool)->gdisp->gimage);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));
}


static void
do_vertical_alignment (GtkWidget *widget,
                       gpointer   data)
{
  GimpAlignTool *align_tool = data;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  if (align_tool->target_item)
    gimp_item_align (align_tool->target_item,
                     align_tool->target_vert_align_type,
                     align_tool->reference_item,
                     align_tool->ref_vert_align_type,
                     align_tool->vert_offset);

  if (GIMP_TOOL (align_tool)->gdisp)
      gimp_image_flush (GIMP_TOOL (align_tool)->gdisp->gimage);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));
}



static GtkWidget *
button_with_stock (GimpAlignmentType  action,
                   GimpAlignTool     *align_tool)
{
  GtkWidget *button;
  gchar     *stock_id;

  switch (action)
    {
    case GIMP_ALIGN_LEFT:
      stock_id = GIMP_STOCK_GRAVITY_WEST;
      break;
    case GIMP_ALIGN_CENTER:
      stock_id = GIMP_STOCK_HCENTER;
      break;
    case GIMP_ALIGN_RIGHT:
      stock_id = GIMP_STOCK_GRAVITY_EAST;
      break;
    case GIMP_ALIGN_TOP:
      stock_id = GIMP_STOCK_GRAVITY_NORTH;
      break;
    case GIMP_ALIGN_MIDDLE:
      stock_id = GIMP_STOCK_VCENTER;
      break;
    case GIMP_ALIGN_BOTTOM:
      stock_id = GIMP_STOCK_GRAVITY_SOUTH;
      break;
    default:
      stock_id = "?";
      break;
    }

  button = gtk_button_new_from_stock (stock_id);

  g_object_set_data (G_OBJECT (button), "action", GINT_TO_POINTER (action));

  g_signal_connect (button, "pressed",
                    G_CALLBACK (set_action),
                    align_tool);

  gtk_widget_set_sensitive (button, FALSE);
  gtk_widget_show (button);

  return button;
}

static void
set_action (GtkWidget *widget,
            gpointer   data)
{
  GimpAlignTool      *align_tool    = data;
  GimpAlignmentType   action;

  action = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "action"));

  switch (action)
    {
    case GIMP_ALIGN_LEFT:
    case GIMP_ALIGN_CENTER:
    case GIMP_ALIGN_RIGHT:
      align_tool->ref_horz_align_type = action;
      align_tool->target_horz_align_type = action;
      do_horizontal_alignment (widget, data);
      break;
    case GIMP_ALIGN_TOP:
    case GIMP_ALIGN_MIDDLE:
    case GIMP_ALIGN_BOTTOM:
      align_tool->ref_vert_align_type = action;
      align_tool->target_vert_align_type = action;
      do_vertical_alignment (widget, data);
      break;
    default:
      break;
    }
}

static void
clear_target (GimpItem      *target_item,
              GimpAlignTool *align_tool)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  if (align_tool->target_item)
    g_signal_handlers_disconnect_by_func (align_tool->target_item,
                                          G_CALLBACK (clear_target),
                                          (gpointer) align_tool);

  align_tool->target_item = NULL;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));
}

static void
clear_reference (GimpItem      *reference_item,
                 GimpAlignTool *align_tool)
{

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  if (align_tool->reference_item)
    g_signal_handlers_disconnect_by_func (align_tool->reference_item,
                                          G_CALLBACK (clear_reference),
                                          (gpointer) align_tool);

  align_tool->reference_item = NULL;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));
}
