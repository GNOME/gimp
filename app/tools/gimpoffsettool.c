/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligmachannel.h"
#include "core/ligmadrawable.h"
#include "core/ligmadrawablefilter.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmalayermask.h"

#include "widgets/ligmahelp-ids.h"

#include "display/ligmadisplay.h"
#include "display/ligmatoolgui.h"

#include "ligmaoffsettool.h"
#include "ligmafilteroptions.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static gboolean   ligma_offset_tool_initialize            (LigmaTool              *tool,
                                                          LigmaDisplay           *display,
                                                          GError               **error);
static void       ligma_offset_tool_control               (LigmaTool              *tool,
                                                          LigmaToolAction         action,
                                                          LigmaDisplay           *display);
static void       ligma_offset_tool_button_press          (LigmaTool              *tool,
                                                          const LigmaCoords      *coords,
                                                          guint32                time,
                                                          GdkModifierType        state,
                                                          LigmaButtonPressType    press_type,
                                                          LigmaDisplay           *display);
static void       ligma_offset_tool_button_release        (LigmaTool              *tool,
                                                          const LigmaCoords      *coords,
                                                          guint32                time,
                                                          GdkModifierType        state,
                                                          LigmaButtonReleaseType  release_type,
                                                          LigmaDisplay           *display);
static void       ligma_offset_tool_motion                (LigmaTool              *tool,
                                                          const LigmaCoords      *coords,
                                                          guint32                time,
                                                          GdkModifierType        state,
                                                          LigmaDisplay           *display);
static void       ligma_offset_tool_oper_update           (LigmaTool              *tool,
                                                          const LigmaCoords      *coords,
                                                          GdkModifierType        state,
                                                          gboolean               proximity,
                                                          LigmaDisplay           *display);
static void       ligma_offset_tool_cursor_update         (LigmaTool              *tool,
                                                          const LigmaCoords      *coords,
                                                          GdkModifierType        state,
                                                          LigmaDisplay           *display);

static gchar    * ligma_offset_tool_get_operation         (LigmaFilterTool        *filter_tool,
                                                          gchar                **description);
static void       ligma_offset_tool_dialog                (LigmaFilterTool        *filter_tool);
static void       ligma_offset_tool_config_notify         (LigmaFilterTool        *filter_tool,
                                                          LigmaConfig            *config,
                                                          const GParamSpec      *pspec);
static void       ligma_offset_tool_region_changed        (LigmaFilterTool        *filter_tool);

static void       ligma_offset_tool_offset_changed        (LigmaSizeEntry         *se,
                                                          LigmaOffsetTool        *offset_tool);

static void       ligma_offset_tool_half_xy_clicked       (GtkButton             *button,
                                                          LigmaOffsetTool        *offset_tool);
static void       ligma_offset_tool_half_x_clicked        (GtkButton             *button,
                                                          LigmaOffsetTool        *offset_tool);
static void       ligma_offset_tool_half_y_clicked        (GtkButton             *button,
                                                          LigmaOffsetTool        *offset_tool);

static void       ligma_offset_tool_edge_behavior_toggled (GtkToggleButton       *toggle,
                                                          LigmaOffsetTool        *offset_tool);

static void       ligma_offset_tool_background_changed    (LigmaContext           *context,
                                                          const LigmaRGB         *color,
                                                          LigmaOffsetTool        *offset_tool);

static gint       ligma_offset_tool_get_width             (LigmaOffsetTool        *offset_tool);
static gint       ligma_offset_tool_get_height            (LigmaOffsetTool        *offset_tool);

static void       ligma_offset_tool_update                (LigmaOffsetTool        *offset_tool);

static void       ligma_offset_tool_halt                  (LigmaOffsetTool        *offset_tool);


G_DEFINE_TYPE (LigmaOffsetTool, ligma_offset_tool,
               LIGMA_TYPE_FILTER_TOOL)

#define parent_class ligma_offset_tool_parent_class


void
ligma_offset_tool_register (LigmaToolRegisterCallback callback,
                           gpointer                 data)
{
  (* callback) (LIGMA_TYPE_OFFSET_TOOL,
                LIGMA_TYPE_FILTER_OPTIONS, NULL,
                LIGMA_CONTEXT_PROP_MASK_BACKGROUND,
                "ligma-offset-tool",
                _("Offset"),
                _("Shift the pixels, optionally wrapping them at the borders"),
                N_("_Offset..."), NULL,
                NULL, LIGMA_HELP_TOOL_OFFSET,
                LIGMA_ICON_TOOL_OFFSET,
                data);
}

static void
ligma_offset_tool_class_init (LigmaOffsetToolClass *klass)
{
  LigmaToolClass       *tool_class        = LIGMA_TOOL_CLASS (klass);
  LigmaFilterToolClass *filter_tool_class = LIGMA_FILTER_TOOL_CLASS (klass);

  tool_class->initialize            = ligma_offset_tool_initialize;
  tool_class->control               = ligma_offset_tool_control;
  tool_class->button_press          = ligma_offset_tool_button_press;
  tool_class->button_release        = ligma_offset_tool_button_release;
  tool_class->motion                = ligma_offset_tool_motion;
  tool_class->oper_update           = ligma_offset_tool_oper_update;
  tool_class->cursor_update         = ligma_offset_tool_cursor_update;

  filter_tool_class->get_operation  = ligma_offset_tool_get_operation;
  filter_tool_class->dialog         = ligma_offset_tool_dialog;
  filter_tool_class->config_notify  = ligma_offset_tool_config_notify;
  filter_tool_class->region_changed = ligma_offset_tool_region_changed;
}

static void
ligma_offset_tool_init (LigmaOffsetTool *offset_tool)
{
  LigmaTool *tool = LIGMA_TOOL (offset_tool);

  ligma_tool_control_set_scroll_lock (tool->control, TRUE);
  ligma_tool_control_set_precision   (tool->control,
                                     LIGMA_CURSOR_PRECISION_PIXEL_CENTER);
}

static gboolean
ligma_offset_tool_initialize (LigmaTool     *tool,
                             LigmaDisplay  *display,
                             GError      **error)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaOffsetTool *offset_tool = LIGMA_OFFSET_TOOL (tool);
  LigmaContext    *context     = LIGMA_CONTEXT (LIGMA_TOOL_GET_OPTIONS (tool));
  LigmaImage      *image;
  LigmaDrawable   *drawable;
  gdouble         xres;
  gdouble         yres;

  if (! LIGMA_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    return FALSE;

  if (g_list_length (tool->drawables) != 1)
    {
      if (g_list_length (tool->drawables) > 1)
        ligma_tool_message_literal (tool, display,
                                   _("Cannot modify multiple drawables. Select only one."));
      else
        ligma_tool_message_literal (tool, display, _("No selected drawables."));

      return FALSE;
    }

  drawable = tool->drawables->data;

  image = ligma_item_get_image (LIGMA_ITEM (drawable));

  ligma_image_get_resolution (image, &xres, &yres);

  g_signal_handlers_block_by_func (offset_tool->offset_se,
                                   ligma_offset_tool_offset_changed,
                                   offset_tool);

  ligma_size_entry_set_resolution (
    LIGMA_SIZE_ENTRY (offset_tool->offset_se), 0,
    xres, FALSE);
  ligma_size_entry_set_resolution (
    LIGMA_SIZE_ENTRY (offset_tool->offset_se), 1,
    yres, FALSE);

  if (LIGMA_IS_LAYER (drawable))
    ligma_tool_gui_set_description (filter_tool->gui, _("Offset Layer"));
  else if (LIGMA_IS_LAYER_MASK (drawable))
    ligma_tool_gui_set_description (filter_tool->gui, _("Offset Layer Mask"));
  else if (LIGMA_IS_CHANNEL (drawable))
    ligma_tool_gui_set_description (filter_tool->gui, _("Offset Channel"));
  else
    g_warning ("%s: unexpected drawable type", G_STRFUNC);

  gtk_widget_set_sensitive (offset_tool->transparent_radio,
                            ligma_drawable_has_alpha (drawable));

  g_signal_handlers_unblock_by_func (offset_tool->offset_se,
                                     ligma_offset_tool_offset_changed,
                                     offset_tool);

  gegl_node_set (
    filter_tool->operation,
    "context", context,
    NULL);

  g_signal_connect (context, "background-changed",
                    G_CALLBACK (ligma_offset_tool_background_changed),
                    offset_tool);

  ligma_offset_tool_update (offset_tool);

  return TRUE;
}

static void
ligma_offset_tool_control (LigmaTool       *tool,
                          LigmaToolAction  action,
                          LigmaDisplay    *display)
{
  LigmaOffsetTool *offset_tool = LIGMA_OFFSET_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_offset_tool_halt (offset_tool);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gchar *
ligma_offset_tool_get_operation (LigmaFilterTool  *filter_tool,
                                gchar          **description)
{
  return g_strdup ("ligma:offset");
}

static void
ligma_offset_tool_button_press (LigmaTool            *tool,
                               const LigmaCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               LigmaButtonPressType  press_type,
                               LigmaDisplay         *display)
{
  LigmaOffsetTool *offset_tool = LIGMA_OFFSET_TOOL (tool);

  offset_tool->dragging = ! ligma_filter_tool_on_guide (LIGMA_FILTER_TOOL (tool),
                                                       coords, display);

  if (! offset_tool->dragging)
    {
      LIGMA_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
  else
    {
      offset_tool->x = coords->x;
      offset_tool->y = coords->y;

      g_object_get (LIGMA_FILTER_TOOL (tool)->config,
                    "x", &offset_tool->offset_x,
                    "y", &offset_tool->offset_y,
                    NULL);

      tool->display = display;

      ligma_tool_control_activate (tool->control);

      ligma_tool_pop_status (tool, display);

      ligma_tool_push_status_coords (tool, display,
                                    LIGMA_CURSOR_PRECISION_PIXEL_CENTER,
                                    _("Offset: "),
                                    0,
                                    ", ",
                                    0,
                                    NULL);
    }
}

static void
ligma_offset_tool_button_release (LigmaTool              *tool,
                                 const LigmaCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 LigmaButtonReleaseType  release_type,
                                 LigmaDisplay           *display)
{
  LigmaOffsetTool *offset_tool = LIGMA_OFFSET_TOOL (tool);

  if (! offset_tool->dragging)
    {
      LIGMA_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
  else
    {
      ligma_tool_control_halt (tool->control);

      offset_tool->dragging = FALSE;

      if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
        {
          g_object_set (LIGMA_FILTER_TOOL (tool)->config,
                        "x", offset_tool->offset_x,
                        "y", offset_tool->offset_y,
                        NULL);
        }
    }
}

static void
ligma_offset_tool_motion (LigmaTool         *tool,
                         const LigmaCoords *coords,
                         guint32           time,
                         GdkModifierType   state,
                         LigmaDisplay      *display)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaOffsetTool *offset_tool = LIGMA_OFFSET_TOOL (tool);

  if (! offset_tool->dragging)
    {
      LIGMA_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
  else
    {
      LigmaOffsetType type;
      gint           offset_x;
      gint           offset_y;
      gint           x;
      gint           y;
      gint           width;
      gint           height;

      g_object_get (filter_tool->config,
                    "type", &type,
                    NULL);

      offset_x = RINT (coords->x - offset_tool->x);
      offset_y = RINT (coords->y - offset_tool->y);

      x = offset_tool->offset_x + offset_x;
      y = offset_tool->offset_y + offset_y;

      width  = ligma_offset_tool_get_width  (offset_tool);
      height = ligma_offset_tool_get_height (offset_tool);

      if (type == LIGMA_OFFSET_WRAP_AROUND)
        {
          x %= MAX (width,  1);
          y %= MAX (height, 1);
        }
      else
        {
          x = CLAMP (x, -width,  +width);
          y = CLAMP (y, -height, +height);
        }

      g_object_set (filter_tool->config,
                    "x", x,
                    "y", y,
                    NULL);

      ligma_tool_pop_status (tool, display);

      ligma_tool_push_status_coords (tool, display,
                                    LIGMA_CURSOR_PRECISION_PIXEL_CENTER,
                                    _("Offset: "),
                                    offset_x,
                                    ", ",
                                    offset_y,
                                    NULL);
    }
}

static void
ligma_offset_tool_oper_update (LigmaTool         *tool,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              LigmaDisplay      *display)
{
  if (! tool->drawables ||
      ligma_filter_tool_on_guide (LIGMA_FILTER_TOOL (tool),
                                 coords, display))
    {
      LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
  else
    {
      ligma_tool_pop_status (tool, display);

      ligma_tool_push_status (tool, display, "%s",
                             _("Click-Drag to offset drawable"));
    }
}

static void
ligma_offset_tool_cursor_update (LigmaTool         *tool,
                                const LigmaCoords *coords,
                                GdkModifierType   state,
                                LigmaDisplay      *display)
{
  if (! tool->drawables ||
      ligma_filter_tool_on_guide (LIGMA_FILTER_TOOL (tool),
                                 coords, display))
    {
      LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                     display);
    }
  else
    {
      ligma_tool_set_cursor (tool, display,
                            LIGMA_CURSOR_MOUSE,
                            LIGMA_TOOL_CURSOR_MOVE,
                            LIGMA_CURSOR_MODIFIER_NONE);
    }
}

static void
ligma_offset_tool_dialog (LigmaFilterTool *filter_tool)
{
  LigmaOffsetTool *offset_tool = LIGMA_OFFSET_TOOL (filter_tool);
  GtkWidget      *main_vbox;
  GtkWidget      *vbox;
  GtkWidget      *hbox;
  GtkWidget      *button;
  GtkWidget      *spinbutton;
  GtkWidget      *frame;
  GtkAdjustment  *adjustment;

  main_vbox = ligma_filter_tool_dialog_get_vbox (filter_tool);

  /*  The offset frame  */
  frame = ligma_frame_new (_("Offset"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  adjustment = (GtkAdjustment *)
    gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinbutton = ligma_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

  offset_tool->offset_se = ligma_size_entry_new (1, LIGMA_UNIT_PIXEL, "%a",
                                                TRUE, TRUE, FALSE, 10,
                                                LIGMA_SIZE_ENTRY_UPDATE_SIZE);

  ligma_size_entry_add_field (LIGMA_SIZE_ENTRY (offset_tool->offset_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_grid_attach (GTK_GRID (offset_tool->offset_se), spinbutton, 1, 0, 1, 1);
  gtk_widget_show (spinbutton);

  ligma_size_entry_attach_label (LIGMA_SIZE_ENTRY (offset_tool->offset_se),
                                _("_X:"), 0, 0, 0.0);
  ligma_size_entry_attach_label (LIGMA_SIZE_ENTRY (offset_tool->offset_se),
                                _("_Y:"), 1, 0, 0.0);

  gtk_box_pack_start (GTK_BOX (vbox), offset_tool->offset_se, FALSE, FALSE, 0);
  gtk_widget_show (offset_tool->offset_se);

  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (offset_tool->offset_se),
                            LIGMA_UNIT_PIXEL);

  g_signal_connect (offset_tool->offset_se, "refval-changed",
                    G_CALLBACK (ligma_offset_tool_offset_changed),
                    offset_tool);
  g_signal_connect (offset_tool->offset_se, "value-changed",
                    G_CALLBACK (ligma_offset_tool_offset_changed),
                    offset_tool);

  button = gtk_button_new_with_mnemonic (_("By width/_2, height/2"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_offset_tool_half_xy_clicked),
                    offset_tool);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("By _width/2"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_offset_tool_half_x_clicked),
                    offset_tool);

  button = gtk_button_new_with_mnemonic (_("By _height/2"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_offset_tool_half_y_clicked),
                    offset_tool);

  /*  The edge behavior frame  */
  frame = ligma_int_radio_group_new (TRUE, _("Edge Behavior"),

                                    G_CALLBACK (ligma_offset_tool_edge_behavior_toggled),
                                    offset_tool, NULL,

                                    LIGMA_OFFSET_WRAP_AROUND,

                                    _("W_rap around"),
                                    LIGMA_OFFSET_WRAP_AROUND, NULL,

                                    _("Fill with _background color"),
                                    LIGMA_OFFSET_BACKGROUND, NULL,

                                    _("Make _transparent"),
                                    LIGMA_OFFSET_TRANSPARENT,
                                    &offset_tool->transparent_radio,
                                    NULL);

  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
}

static void
ligma_offset_tool_config_notify (LigmaFilterTool   *filter_tool,
                                LigmaConfig       *config,
                                const GParamSpec *pspec)
{
  ligma_offset_tool_update (LIGMA_OFFSET_TOOL (filter_tool));

  LIGMA_FILTER_TOOL_CLASS (parent_class)->config_notify (filter_tool,
                                                        config, pspec);
}

static void
ligma_offset_tool_region_changed (LigmaFilterTool *filter_tool)
{
  ligma_offset_tool_update (LIGMA_OFFSET_TOOL (filter_tool));
}

static void
ligma_offset_tool_offset_changed (LigmaSizeEntry  *se,
                                 LigmaOffsetTool *offset_tool)
{
  g_object_set (LIGMA_FILTER_TOOL (offset_tool)->config,
                "x", (gint) ligma_size_entry_get_refval (se, 0),
                "y", (gint) ligma_size_entry_get_refval (se, 1),
                NULL);
}

static void
ligma_offset_tool_half_xy_clicked (GtkButton      *button,
                                  LigmaOffsetTool *offset_tool)
{
  g_object_set (LIGMA_FILTER_TOOL (offset_tool)->config,
                "x", ligma_offset_tool_get_width  (offset_tool) / 2,
                "y", ligma_offset_tool_get_height (offset_tool) / 2,
                NULL);
}

static void
ligma_offset_tool_half_x_clicked (GtkButton      *button,
                                 LigmaOffsetTool *offset_tool)
{
  g_object_set (LIGMA_FILTER_TOOL (offset_tool)->config,
                "x", ligma_offset_tool_get_width (offset_tool) / 2,
                NULL);
}

static void
ligma_offset_tool_half_y_clicked (GtkButton      *button,
                                 LigmaOffsetTool *offset_tool)
{
  g_object_set (LIGMA_FILTER_TOOL (offset_tool)->config,
                "y", ligma_offset_tool_get_height (offset_tool) / 2,
                NULL);
}

static void
ligma_offset_tool_edge_behavior_toggled (GtkToggleButton *toggle,
                                        LigmaOffsetTool  *offset_tool)
{
  if (gtk_toggle_button_get_active (toggle))
    {
      LigmaOffsetType type;

      type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (toggle),
                                                 "ligma-item-data"));

      g_object_set (LIGMA_FILTER_TOOL (offset_tool)->config,
                    "type", type,
                    NULL);
    }
}

static void
ligma_offset_tool_background_changed (LigmaContext    *context,
                                     const LigmaRGB  *color,
                                     LigmaOffsetTool *offset_tool)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (offset_tool);
  LigmaOffsetType  type;

  g_object_get (filter_tool->config,
                "type", &type,
                NULL);

  if (type == LIGMA_OFFSET_BACKGROUND)
    {
      gegl_node_set (filter_tool->operation,
                     "context", context,
                     NULL);

      ligma_drawable_filter_apply (filter_tool->filter, NULL);
    }
}

static gint
ligma_offset_tool_get_width (LigmaOffsetTool *offset_tool)
{
  GeglRectangle drawable_area;
  gint          drawable_offset_x;
  gint          drawable_offset_y;

  if (ligma_filter_tool_get_drawable_area (LIGMA_FILTER_TOOL (offset_tool),
                                          &drawable_offset_x,
                                          &drawable_offset_y,
                                          &drawable_area) &&
      ! gegl_rectangle_is_empty (&drawable_area))
    {
      return drawable_area.width;
    }

  return 0;
}

static gint
ligma_offset_tool_get_height (LigmaOffsetTool *offset_tool)
{
  GeglRectangle drawable_area;
  gint          drawable_offset_x;
  gint          drawable_offset_y;

  if (ligma_filter_tool_get_drawable_area (LIGMA_FILTER_TOOL (offset_tool),
                                          &drawable_offset_x,
                                          &drawable_offset_y,
                                          &drawable_area) &&
      ! gegl_rectangle_is_empty (&drawable_area))
    {
      return drawable_area.height;
    }

  return 0;
}

static void
ligma_offset_tool_update (LigmaOffsetTool *offset_tool)
{
  LigmaTool       *tool        = LIGMA_TOOL (offset_tool);
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (offset_tool);
  LigmaOffsetType  orig_type;
  gint            orig_x;
  gint            orig_y;
  LigmaOffsetType  type;
  gint            x;
  gint            y;
  gint            width;
  gint            height;

  g_object_get (filter_tool->config,
                "type", &orig_type,
                "x",    &orig_x,
                "y",    &orig_y,
                NULL);

  width  = ligma_offset_tool_get_width  (offset_tool);
  height = ligma_offset_tool_get_height (offset_tool);

  x = CLAMP (orig_x, -width,  +width);
  y = CLAMP (orig_y, -height, +height);

  type = orig_type;

  if (tool->drawables                            &&
      ! ligma_drawable_has_alpha (tool->drawables->data) &&
      type == LIGMA_OFFSET_TRANSPARENT)
    {
      type = LIGMA_OFFSET_BACKGROUND;
    }

  if (x    != orig_x ||
      y    != orig_y ||
      type != orig_type)
    {
      g_object_set (filter_tool->config,
                    "type", type,
                    "x",    x,
                    "y",    y,
                    NULL);
    }

  if (offset_tool->offset_se)
    {
      gint width  = ligma_offset_tool_get_width  (offset_tool);
      gint height = ligma_offset_tool_get_height (offset_tool);

      g_signal_handlers_block_by_func (offset_tool->offset_se,
                                       ligma_offset_tool_offset_changed,
                                       offset_tool);

      ligma_size_entry_set_refval_boundaries (
        LIGMA_SIZE_ENTRY (offset_tool->offset_se), 0,
        -width, +width);
      ligma_size_entry_set_refval_boundaries (
        LIGMA_SIZE_ENTRY (offset_tool->offset_se), 1,
        -height, +height);

      ligma_size_entry_set_size (
        LIGMA_SIZE_ENTRY (offset_tool->offset_se), 0,
        0, width);
      ligma_size_entry_set_size (
        LIGMA_SIZE_ENTRY (offset_tool->offset_se), 1,
        0, height);

      ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (offset_tool->offset_se), 0,
                                  x);
      ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (offset_tool->offset_se), 1,
                                  y);

      g_signal_handlers_unblock_by_func (offset_tool->offset_se,
                                         ligma_offset_tool_offset_changed,
                                         offset_tool);
    }

  if (offset_tool->transparent_radio)
    {
      ligma_int_radio_group_set_active (
        GTK_RADIO_BUTTON (offset_tool->transparent_radio),
        type);
    }
}

static void
ligma_offset_tool_halt (LigmaOffsetTool *offset_tool)
{
  LigmaContext *context = LIGMA_CONTEXT (LIGMA_TOOL_GET_OPTIONS (offset_tool));

  offset_tool->offset_se         = NULL;
  offset_tool->transparent_radio = NULL;

  g_signal_handlers_disconnect_by_func (
    context,
    ligma_offset_tool_background_changed,
    offset_tool);
}
