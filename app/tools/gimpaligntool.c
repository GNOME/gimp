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
#include "core/gimpimage-item-list.h"
#include "core/gimplayer.h"
#include "core/gimptoolinfo.h"
#include "core/gimplist.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpalignoptions.h"
#include "gimpaligntool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GObject * gimp_align_tool_constructor (GType              type,
                                              guint              n_params,
                                              GObjectConstructParam *params);
static gboolean gimp_align_tool_initialize   (GimpTool          *tool,
                                              GimpDisplay       *display);
static void   gimp_align_tool_finalize       (GObject           *object);

static void   gimp_align_tool_control        (GimpTool          *tool,
                                              GimpToolAction     action,
                                              GimpDisplay       *display);
static void   gimp_align_tool_button_press   (GimpTool          *tool,
                                              GimpCoords        *coords,
                                              guint32            time,
                                              GdkModifierType    state,
                                              GimpDisplay       *display);
static void   gimp_align_tool_button_release (GimpTool          *tool,
                                              GimpCoords        *coords,
                                              guint32            time,
                                              GdkModifierType    state,
                                              GimpDisplay       *display);
static void   gimp_align_tool_motion         (GimpTool          *tool,
                                              GimpCoords        *coords,
                                              guint32            time,
                                              GdkModifierType    state,
                                              GimpDisplay       *display);
static void   gimp_align_tool_cursor_update  (GimpTool          *tool,
                                              GimpCoords        *coords,
                                              GdkModifierType    state,
                                              GimpDisplay       *display);

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
static void   clear_selected                 (GimpItem          *item,
                                              GimpAlignTool     *align_tool);
static void   clear_selected_items           (GimpAlignTool     *align_tool);
static GimpLayer *select_layer_by_coords     (GimpImage         *image,
                                              gint               x,
                                              gint               y);

G_DEFINE_TYPE (GimpAlignTool, gimp_align_tool, GIMP_TYPE_DRAW_TOOL)

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
                _("Align or arrange layers and other items"),
                N_("_Align"), "Q",
                NULL, GIMP_HELP_TOOL_MOVE,
                GIMP_STOCK_TOOL_ALIGN,
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

  tool_class->initialize     = gimp_align_tool_initialize;
  tool_class->control        = gimp_align_tool_control;
  tool_class->button_press   = gimp_align_tool_button_press;
  tool_class->button_release = gimp_align_tool_button_release;
  tool_class->motion         = gimp_align_tool_motion;
  tool_class->cursor_update  = gimp_align_tool_cursor_update;

  draw_tool_class->draw      = gimp_align_tool_draw;
}

static void
gimp_align_tool_init (GimpAlignTool *align_tool)
{
  GimpTool *tool = GIMP_TOOL (align_tool);

  align_tool->controls        = NULL;

  align_tool->selected_items  = NULL;

  align_tool->horz_align_type = GIMP_ALIGN_LEFT;
  align_tool->vert_align_type = GIMP_ALIGN_TOP;

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
                            GimpDisplay *display)
{
  if (tool->display != display)
    {
    }

  return TRUE;
}

static void
gimp_align_tool_control (GimpTool       *tool,
                         GimpToolAction  action,
                         GimpDisplay    *display)
{
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      clear_selected_items (align_tool);
      gimp_tool_pop_status (tool, display);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_align_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpAlignTool    *align_tool  = GIMP_ALIGN_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  If the tool was being used in another image...reset it  */

  if (display != tool->display)
    {
      if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
        gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

      clear_selected_items (align_tool);
    }

  if (! gimp_tool_control_is_active (tool->control))
    gimp_tool_control_activate (tool->control);

  tool->display = display;

  align_tool->x1 = align_tool->x0 = coords->x;
  align_tool->y1 = align_tool->y0 = coords->y;

  if (! gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_align_tool_button_release (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
                                GdkModifierType  state,
                                GimpDisplay     *display)
{
  GimpAlignTool    *align_tool  = GIMP_ALIGN_TOOL (tool);
  GimpAlignOptions *options     = GIMP_ALIGN_OPTIONS (tool->tool_info->tool_options);
  GimpItem         *item        = NULL;
  GimpImage        *image       = display->image;
  gint              i;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (! (state & GDK_SHIFT_MASK))
    clear_selected_items (align_tool);

  if (coords->x == align_tool->x0 && coords->y == align_tool->y0)
    {
      if (options->align_type == GIMP_TRANSFORM_TYPE_PATH)
        {
          GimpVectors *vectors;

          if (gimp_draw_tool_on_vectors (GIMP_DRAW_TOOL (tool), display,
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

          if ((layer = select_layer_by_coords (display->image,
                                               coords->x, coords->y)))
            {
              item = GIMP_ITEM (layer);
            }
        }

      if (item)
        {
          if (! g_list_find (align_tool->selected_items, item))
            {
              align_tool->selected_items = g_list_append (align_tool->selected_items, item);
              g_signal_connect (item, "removed", G_CALLBACK (clear_selected),
                                (gpointer) align_tool);
            }
        }
    }
  else
    {
      gint   X0    = MIN (coords->x, align_tool->x0);
      gint   X1    = MAX (coords->x, align_tool->x0);
      gint   Y0    = MIN (coords->y, align_tool->y0);
      gint   Y1    = MAX (coords->y, align_tool->y0);
      GList *list;

      for (list = GIMP_LIST (image->layers)->list;
           list;
           list = g_list_next (list))
        {
          GimpLayer *layer = list->data;
          gint       x0, y0, x1, y1;

          if (! gimp_item_get_visible (GIMP_ITEM (layer)))
            continue;

          gimp_item_offsets (GIMP_ITEM (layer), &x0, &y0);
          x1 = x0 + gimp_item_width (GIMP_ITEM (layer));
          y1 = y0 + gimp_item_height (GIMP_ITEM (layer));

          if (x0 < X0 || y0 < Y0 || x1 > X1 || y1 > Y1)
            continue;

          if (g_list_find (align_tool->selected_items, layer))
            continue;

          align_tool->selected_items = g_list_append (align_tool->selected_items, layer);
          g_signal_connect (layer, "removed", G_CALLBACK (clear_selected),
                            (gpointer) align_tool);
        }
    }

  for (i = 0; i < ALIGN_TOOL_NUM_BUTTONS; i++)
    gtk_widget_set_sensitive (align_tool->button[i],
                              (align_tool->selected_items != NULL));

  align_tool->x1 = align_tool->x0;
  align_tool->y1 = align_tool->y0;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_align_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *display)
{
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  align_tool->x1 = coords->x;
  align_tool->y1 = coords->y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_align_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  GimpAlignOptions  *options;
  GimpCursorType     cursor      = GIMP_CURSOR_MOUSE;
  GimpToolCursorType tool_cursor = GIMP_TOOL_CURSOR_MOVE;
  GimpCursorModifier modifier    = GIMP_CURSOR_MODIFIER_NONE;

  options = GIMP_ALIGN_OPTIONS (tool->tool_info->tool_options);

  if (options->align_type == GIMP_TRANSFORM_TYPE_PATH)
    {
      tool_cursor = GIMP_TOOL_CURSOR_PATHS;

      if (gimp_draw_tool_on_vectors (GIMP_DRAW_TOOL (tool), display,
                                     coords, 7, 7,
                                     NULL, NULL, NULL, NULL, NULL, NULL))
        {
          tool_cursor = GIMP_TOOL_CURSOR_HAND;
          modifier    = GIMP_CURSOR_MODIFIER_MOVE;
        }
      else
        {
          modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
    }
  else
    {
      GimpLayer *layer = gimp_image_pick_correlate_layer (display->image,
                                                          coords->x, coords->y);

      if (layer)
        {
          /*  if there is a floating selection, and this aint it...  */
          if (gimp_image_floating_sel (display->image) &&
              ! gimp_layer_is_floating_sel (layer))
            {
              modifier = GIMP_CURSOR_MODIFIER_ANCHOR;
            }
          else if (layer != gimp_image_get_active_layer (display->image))
            {
              tool_cursor = GIMP_TOOL_CURSOR_HAND;
              modifier    = GIMP_CURSOR_MODIFIER_MOVE;
            }
        }
      else
        {
          modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_align_tool_draw (GimpDrawTool *draw_tool)
{
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (draw_tool);
  GList         *list;
  gint           x, y, w, h;

  /* draw rubber-band rectangle */
  x = MIN (align_tool->x1, align_tool->x0);
  y = MIN (align_tool->y1, align_tool->y0);
  w = MAX (align_tool->x1, align_tool->x0) - x;
  h = MAX (align_tool->y1, align_tool->y0) - y;

  gimp_draw_tool_draw_rectangle (draw_tool,
                                 FALSE,
                                 x, y,w, h,
                                 FALSE);

  for (list = g_list_first (align_tool->selected_items); list;
       list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_item_offsets (item, &x, &y);
      w = gimp_item_width (item);
      h = gimp_item_height (item);

      gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                  x, y, 4, 4,
                                  GTK_ANCHOR_NORTH_WEST, FALSE);
      gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                  x + w, y, 4, 4,
                                  GTK_ANCHOR_NORTH_EAST, FALSE);
      gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                  x, y + h, 4, 4,
                                  GTK_ANCHOR_SOUTH_WEST, FALSE);
      gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                  x + w, y + h, 4, 4,
                                  GTK_ANCHOR_SOUTH_EAST, FALSE);
    }
}


static GtkWidget *
gimp_align_tool_controls (GimpAlignTool *align_tool)
{
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *button;
  gint       row, col, n;

  hbox = gtk_hbox_new (FALSE, 0);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), main_vbox, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_widget_show (main_vbox);

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
/*   label = gtk_label_new (_("Offset:")); */
/*   gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 3, row, row + 1); */
/*   gtk_widget_show (label); */

/*   spinbutton = gimp_spin_button_new (&align_tool->horz_offset_adjustment, */
/*                                      0, */
/*                                      -100000., */
/*                                      100000., */
/*                                      1., 20., 20., 1., 0); */
/*   gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 3, 7, row, row + 1); */
/*   g_signal_connect (align_tool->horz_offset_adjustment, "value-changed", */
/*                     G_CALLBACK (gimp_double_adjustment_update), */
/*                     &align_tool->horz_offset); */
/*   gtk_widget_show (spinbutton); */

/*   row++; */

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
/*   label = gtk_label_new (_("Offset:")); */
/*   gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 3, row, row + 1); */
/*   gtk_widget_show (label); */

/*   spinbutton = gimp_spin_button_new (&align_tool->vert_offset_adjustment, */
/*                                      0, */
/*                                      -100000., */
/*                                      100000., */
/*                                      1., 20., 20., 1., 0); */
/*   gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 3, 7, row, row + 1); */
/*   g_signal_connect (align_tool->vert_offset_adjustment, "value-changed", */
/*                     G_CALLBACK (gimp_double_adjustment_update), */
/*                     &align_tool->vert_offset); */
/*   gtk_widget_show (spinbutton); */

  gtk_widget_show (hbox);
  return hbox;
}


static void
do_horizontal_alignment (GtkWidget *widget,
                         gpointer   data)
{
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (data);
  GimpImage     *image;
  GimpItem      *reference;

  /* make sure there is something to align */
  if (! g_list_nth (align_tool->selected_items, 1))
    return;

  image = GIMP_TOOL (align_tool)->display->image;

  reference = align_tool->selected_items->data;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  gimp_image_item_list_align (image, g_list_next (align_tool->selected_items),
                              align_tool->horz_align_type,
                              reference,
                              align_tool->horz_align_type,
                              align_tool->horz_offset);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));

  gimp_image_flush (image);
}


static void
do_vertical_alignment (GtkWidget *widget,
                       gpointer   data)
{
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (data);
  GimpImage     *image;
  GimpItem      *reference;

  /* make sure there is something to align */
  if (g_list_nth (align_tool->selected_items, 1))
    return;

  image = GIMP_TOOL (align_tool)->display->image;

  reference = align_tool->selected_items->data;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  gimp_image_item_list_align (image, g_list_next (align_tool->selected_items),
                              align_tool->vert_align_type,
                              reference,
                              align_tool->vert_align_type,
                              align_tool->vert_offset);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));

  gimp_image_flush (image);
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

  g_signal_connect (button, "clicked",
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
      align_tool->horz_align_type = action;
      do_horizontal_alignment (widget, data);
      break;
    case GIMP_ALIGN_TOP:
    case GIMP_ALIGN_MIDDLE:
    case GIMP_ALIGN_BOTTOM:
      align_tool->vert_align_type = action;
      do_vertical_alignment (widget, data);
      break;
    default:
      break;
    }
}

static void
clear_selected (GimpItem       *item,
                GimpAlignTool  *align_tool)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  if (align_tool->selected_items)
    g_signal_handlers_disconnect_by_func (item,
                                          G_CALLBACK (clear_selected),
                                          (gpointer) align_tool);

  align_tool->selected_items = g_list_remove (align_tool->selected_items,
                                              item);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));
}

static void
clear_selected_items (GimpAlignTool *align_tool)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (align_tool);

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_pause (draw_tool);

  while (align_tool->selected_items)
    {
      GimpItem *item = g_list_first (align_tool->selected_items)->data;

      g_signal_handlers_disconnect_by_func (item,
                                            G_CALLBACK (clear_selected),
                                            (gpointer) align_tool);

      align_tool->selected_items = g_list_remove (align_tool->selected_items,
                                                  item);
    }

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_resume (draw_tool);
}

static GimpLayer *
select_layer_by_coords (GimpImage *image,
                        gint       x,
                        gint       y)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  for (list = GIMP_LIST (image->layers)->list;
       list;
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;
      gint       off_x, off_y;
      gint       width, height;

      if (! gimp_item_get_visible (GIMP_ITEM (layer)))
        continue;

      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);
      width = gimp_item_width (GIMP_ITEM (layer));
      height = gimp_item_height (GIMP_ITEM (layer));

      if (off_x <= x &&
          off_y <= y &&
          x < off_x + width &&
          y < off_y + height)
        {
          return layer;
        }
    }

  return NULL;
}
