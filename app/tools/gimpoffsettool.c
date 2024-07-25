/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolgui.h"

#include "gimpoffsettool.h"
#include "gimpfilteroptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static gboolean   gimp_offset_tool_initialize            (GimpTool              *tool,
                                                          GimpDisplay           *display,
                                                          GError               **error);
static void       gimp_offset_tool_control               (GimpTool              *tool,
                                                          GimpToolAction         action,
                                                          GimpDisplay           *display);
static void       gimp_offset_tool_button_press          (GimpTool              *tool,
                                                          const GimpCoords      *coords,
                                                          guint32                time,
                                                          GdkModifierType        state,
                                                          GimpButtonPressType    press_type,
                                                          GimpDisplay           *display);
static void       gimp_offset_tool_button_release        (GimpTool              *tool,
                                                          const GimpCoords      *coords,
                                                          guint32                time,
                                                          GdkModifierType        state,
                                                          GimpButtonReleaseType  release_type,
                                                          GimpDisplay           *display);
static void       gimp_offset_tool_motion                (GimpTool              *tool,
                                                          const GimpCoords      *coords,
                                                          guint32                time,
                                                          GdkModifierType        state,
                                                          GimpDisplay           *display);
static void       gimp_offset_tool_oper_update           (GimpTool              *tool,
                                                          const GimpCoords      *coords,
                                                          GdkModifierType        state,
                                                          gboolean               proximity,
                                                          GimpDisplay           *display);
static void       gimp_offset_tool_cursor_update         (GimpTool              *tool,
                                                          const GimpCoords      *coords,
                                                          GdkModifierType        state,
                                                          GimpDisplay           *display);

static gchar    * gimp_offset_tool_get_operation         (GimpFilterTool        *filter_tool,
                                                          gchar                **description);
static void       gimp_offset_tool_dialog                (GimpFilterTool        *filter_tool);
static void       gimp_offset_tool_config_notify         (GimpFilterTool        *filter_tool,
                                                          GimpConfig            *config,
                                                          const GParamSpec      *pspec);
static void       gimp_offset_tool_region_changed        (GimpFilterTool        *filter_tool);

static void       gimp_offset_tool_offset_changed        (GimpSizeEntry         *se,
                                                          GimpOffsetTool        *offset_tool);

static void       gimp_offset_tool_half_xy_clicked       (GtkButton             *button,
                                                          GimpOffsetTool        *offset_tool);
static void       gimp_offset_tool_half_x_clicked        (GtkButton             *button,
                                                          GimpOffsetTool        *offset_tool);
static void       gimp_offset_tool_half_y_clicked        (GtkButton             *button,
                                                          GimpOffsetTool        *offset_tool);

static void       gimp_offset_tool_edge_behavior_toggled (GtkToggleButton       *toggle,
                                                          GimpOffsetTool        *offset_tool);

static void       gimp_offset_tool_background_changed    (GimpContext           *context,
                                                          GeglColor             *color,
                                                          GimpOffsetTool        *offset_tool);

static gint       gimp_offset_tool_get_width             (GimpOffsetTool        *offset_tool);
static gint       gimp_offset_tool_get_height            (GimpOffsetTool        *offset_tool);

static void       gimp_offset_tool_update                (GimpOffsetTool        *offset_tool);

static void       gimp_offset_tool_halt                  (GimpOffsetTool        *offset_tool);


G_DEFINE_TYPE (GimpOffsetTool, gimp_offset_tool,
               GIMP_TYPE_FILTER_TOOL)

#define parent_class gimp_offset_tool_parent_class


void
gimp_offset_tool_register (GimpToolRegisterCallback callback,
                           gpointer                 data)
{
  (* callback) (GIMP_TYPE_OFFSET_TOOL,
                GIMP_TYPE_FILTER_OPTIONS, NULL,
                GIMP_CONTEXT_PROP_MASK_BACKGROUND,
                "gimp-offset-tool",
                _("Offset"),
                _("Shift the pixels, optionally wrapping them at the borders"),
                N_("_Offset..."), NULL,
                NULL, GIMP_HELP_TOOL_OFFSET,
                GIMP_ICON_TOOL_OFFSET,
                data);
}

static void
gimp_offset_tool_class_init (GimpOffsetToolClass *klass)
{
  GimpToolClass       *tool_class        = GIMP_TOOL_CLASS (klass);
  GimpFilterToolClass *filter_tool_class = GIMP_FILTER_TOOL_CLASS (klass);

  tool_class->initialize            = gimp_offset_tool_initialize;
  tool_class->control               = gimp_offset_tool_control;
  tool_class->button_press          = gimp_offset_tool_button_press;
  tool_class->button_release        = gimp_offset_tool_button_release;
  tool_class->motion                = gimp_offset_tool_motion;
  tool_class->oper_update           = gimp_offset_tool_oper_update;
  tool_class->cursor_update         = gimp_offset_tool_cursor_update;

  filter_tool_class->get_operation  = gimp_offset_tool_get_operation;
  filter_tool_class->dialog         = gimp_offset_tool_dialog;
  filter_tool_class->config_notify  = gimp_offset_tool_config_notify;
  filter_tool_class->region_changed = gimp_offset_tool_region_changed;
}

static void
gimp_offset_tool_init (GimpOffsetTool *offset_tool)
{
  GimpTool *tool = GIMP_TOOL (offset_tool);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_precision   (tool->control,
                                     GIMP_CURSOR_PRECISION_PIXEL_CENTER);
}

static gboolean
gimp_offset_tool_initialize (GimpTool     *tool,
                             GimpDisplay  *display,
                             GError      **error)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpOffsetTool *offset_tool = GIMP_OFFSET_TOOL (tool);
  GimpContext    *context     = GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (tool));
  GimpImage      *image;
  GimpDrawable   *drawable;
  gdouble         xres;
  gdouble         yres;

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    return FALSE;

  if (g_list_length (tool->drawables) != 1)
    {
      if (g_list_length (tool->drawables) > 1)
        gimp_tool_message_literal (tool, display,
                                   _("Cannot modify multiple drawables. Select only one."));
      else
        gimp_tool_message_literal (tool, display, _("No selected drawables."));

      return FALSE;
    }

  drawable = tool->drawables->data;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  gimp_image_get_resolution (image, &xres, &yres);

  g_signal_handlers_block_by_func (offset_tool->offset_se,
                                   gimp_offset_tool_offset_changed,
                                   offset_tool);

  gimp_size_entry_set_resolution (
    GIMP_SIZE_ENTRY (offset_tool->offset_se), 0,
    xres, FALSE);
  gimp_size_entry_set_resolution (
    GIMP_SIZE_ENTRY (offset_tool->offset_se), 1,
    yres, FALSE);

  if (GIMP_IS_LAYER (drawable))
    gimp_tool_gui_set_description (filter_tool->gui, _("Offset Layer"));
  else if (GIMP_IS_LAYER_MASK (drawable))
    gimp_tool_gui_set_description (filter_tool->gui, _("Offset Layer Mask"));
  else if (GIMP_IS_CHANNEL (drawable))
    gimp_tool_gui_set_description (filter_tool->gui, _("Offset Channel"));
  else
    g_warning ("%s: unexpected drawable type", G_STRFUNC);

  gtk_widget_set_sensitive (offset_tool->transparent_radio,
                            gimp_drawable_has_alpha (drawable));

  g_signal_handlers_unblock_by_func (offset_tool->offset_se,
                                     gimp_offset_tool_offset_changed,
                                     offset_tool);

  gegl_node_set (
    filter_tool->operation,
    "context", context,
    NULL);

  g_signal_connect (context, "background-changed",
                    G_CALLBACK (gimp_offset_tool_background_changed),
                    offset_tool);

  gimp_offset_tool_update (offset_tool);

  return TRUE;
}

static void
gimp_offset_tool_control (GimpTool       *tool,
                          GimpToolAction  action,
                          GimpDisplay    *display)
{
  GimpOffsetTool *offset_tool = GIMP_OFFSET_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_offset_tool_halt (offset_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gchar *
gimp_offset_tool_get_operation (GimpFilterTool  *filter_tool,
                                gchar          **description)
{
  return g_strdup ("gimp:offset");
}

static void
gimp_offset_tool_button_press (GimpTool            *tool,
                               const GimpCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               GimpButtonPressType  press_type,
                               GimpDisplay         *display)
{
  GimpOffsetTool *offset_tool = GIMP_OFFSET_TOOL (tool);

  offset_tool->dragging = ! gimp_filter_tool_on_guide (GIMP_FILTER_TOOL (tool),
                                                       coords, display);

  if (! offset_tool->dragging)
    {
      GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
  else
    {
      offset_tool->x = coords->x;
      offset_tool->y = coords->y;

      g_object_get (GIMP_FILTER_TOOL (tool)->config,
                    "x", &offset_tool->offset_x,
                    "y", &offset_tool->offset_y,
                    NULL);

      tool->display = display;

      gimp_tool_control_activate (tool->control);

      gimp_tool_pop_status (tool, display);

      gimp_tool_push_status_coords (tool, display,
                                    GIMP_CURSOR_PRECISION_PIXEL_CENTER,
                                    _("Offset: "),
                                    0,
                                    ", ",
                                    0,
                                    NULL);
    }
}

static void
gimp_offset_tool_button_release (GimpTool              *tool,
                                 const GimpCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 GimpButtonReleaseType  release_type,
                                 GimpDisplay           *display)
{
  GimpOffsetTool *offset_tool = GIMP_OFFSET_TOOL (tool);

  if (! offset_tool->dragging)
    {
      GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
  else
    {
      gimp_tool_control_halt (tool->control);

      offset_tool->dragging = FALSE;

      if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
        {
          g_object_set (GIMP_FILTER_TOOL (tool)->config,
                        "x", offset_tool->offset_x,
                        "y", offset_tool->offset_y,
                        NULL);
        }
    }
}

static void
gimp_offset_tool_motion (GimpTool         *tool,
                         const GimpCoords *coords,
                         guint32           time,
                         GdkModifierType   state,
                         GimpDisplay      *display)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (tool);
  GimpOffsetTool *offset_tool = GIMP_OFFSET_TOOL (tool);

  if (! offset_tool->dragging)
    {
      GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
  else
    {
      GimpOffsetType type;
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

      width  = gimp_offset_tool_get_width  (offset_tool);
      height = gimp_offset_tool_get_height (offset_tool);

      if (type == GIMP_OFFSET_WRAP_AROUND)
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

      gimp_tool_pop_status (tool, display);

      gimp_tool_push_status_coords (tool, display,
                                    GIMP_CURSOR_PRECISION_PIXEL_CENTER,
                                    _("Offset: "),
                                    offset_x,
                                    ", ",
                                    offset_y,
                                    NULL);
    }
}

static void
gimp_offset_tool_oper_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              GimpDisplay      *display)
{
  if (! tool->drawables ||
      gimp_filter_tool_on_guide (GIMP_FILTER_TOOL (tool),
                                 coords, display))
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
  else
    {
      gimp_tool_pop_status (tool, display);

      gimp_tool_push_status (tool, display, "%s",
                             _("Click-Drag to offset drawable"));
    }
}

static void
gimp_offset_tool_cursor_update (GimpTool         *tool,
                                const GimpCoords *coords,
                                GdkModifierType   state,
                                GimpDisplay      *display)
{
  if (! tool->drawables ||
      gimp_filter_tool_on_guide (GIMP_FILTER_TOOL (tool),
                                 coords, display))
    {
      GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                     display);
    }
  else
    {
      gimp_tool_set_cursor (tool, display,
                            GIMP_CURSOR_MOUSE,
                            GIMP_TOOL_CURSOR_MOVE,
                            GIMP_CURSOR_MODIFIER_NONE);
    }
}

static void
gimp_offset_tool_dialog (GimpFilterTool *filter_tool)
{
  GimpOffsetTool *offset_tool = GIMP_OFFSET_TOOL (filter_tool);
  GtkWidget      *main_vbox;
  GtkWidget      *vbox;
  GtkWidget      *hbox;
  GtkWidget      *button;
  GtkWidget      *spinbutton;
  GtkWidget      *frame;
  GtkAdjustment  *adjustment;

  main_vbox = gimp_filter_tool_dialog_get_vbox (filter_tool);

  /*  The offset frame  */
  frame = gimp_frame_new (_("Offset"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  adjustment = (GtkAdjustment *)
    gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

  offset_tool->offset_se = gimp_size_entry_new (1, gimp_unit_pixel (), "%a",
                                                TRUE, TRUE, FALSE, 10,
                                                GIMP_SIZE_ENTRY_UPDATE_SIZE);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (offset_tool->offset_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_grid_attach (GTK_GRID (offset_tool->offset_se), spinbutton, 1, 0, 1, 1);
  gtk_widget_show (spinbutton);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (offset_tool->offset_se),
                                _("_X:"), 0, 0, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (offset_tool->offset_se),
                                _("_Y:"), 1, 0, 0.0);

  gtk_box_pack_start (GTK_BOX (vbox), offset_tool->offset_se, FALSE, FALSE, 0);
  gtk_widget_show (offset_tool->offset_se);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (offset_tool->offset_se),
                            gimp_unit_pixel ());

  g_signal_connect (offset_tool->offset_se, "refval-changed",
                    G_CALLBACK (gimp_offset_tool_offset_changed),
                    offset_tool);
  g_signal_connect (offset_tool->offset_se, "value-changed",
                    G_CALLBACK (gimp_offset_tool_offset_changed),
                    offset_tool);

  button = gtk_button_new_with_mnemonic (_("By width/_2, height/2"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_offset_tool_half_xy_clicked),
                    offset_tool);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("By _width/2"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_offset_tool_half_x_clicked),
                    offset_tool);

  button = gtk_button_new_with_mnemonic (_("By _height/2"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_offset_tool_half_y_clicked),
                    offset_tool);

  /*  The edge behavior frame  */
  frame = gimp_int_radio_group_new (TRUE, _("Edge Behavior"),

                                    G_CALLBACK (gimp_offset_tool_edge_behavior_toggled),
                                    offset_tool, NULL,

                                    GIMP_OFFSET_WRAP_AROUND,

                                    _("W_rap around"),
                                    GIMP_OFFSET_WRAP_AROUND, NULL,

                                    _("Fill with _background color"),
                                    GIMP_OFFSET_BACKGROUND, NULL,

                                    _("Make _transparent"),
                                    GIMP_OFFSET_TRANSPARENT,
                                    &offset_tool->transparent_radio,
                                    NULL);

  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
}

static void
gimp_offset_tool_config_notify (GimpFilterTool   *filter_tool,
                                GimpConfig       *config,
                                const GParamSpec *pspec)
{
  gimp_offset_tool_update (GIMP_OFFSET_TOOL (filter_tool));

  GIMP_FILTER_TOOL_CLASS (parent_class)->config_notify (filter_tool,
                                                        config, pspec);
}

static void
gimp_offset_tool_region_changed (GimpFilterTool *filter_tool)
{
  gimp_offset_tool_update (GIMP_OFFSET_TOOL (filter_tool));
}

static void
gimp_offset_tool_offset_changed (GimpSizeEntry  *se,
                                 GimpOffsetTool *offset_tool)
{
  g_object_set (GIMP_FILTER_TOOL (offset_tool)->config,
                "x", (gint) gimp_size_entry_get_refval (se, 0),
                "y", (gint) gimp_size_entry_get_refval (se, 1),
                NULL);
}

static void
gimp_offset_tool_half_xy_clicked (GtkButton      *button,
                                  GimpOffsetTool *offset_tool)
{
  g_object_set (GIMP_FILTER_TOOL (offset_tool)->config,
                "x", gimp_offset_tool_get_width  (offset_tool) / 2,
                "y", gimp_offset_tool_get_height (offset_tool) / 2,
                NULL);
}

static void
gimp_offset_tool_half_x_clicked (GtkButton      *button,
                                 GimpOffsetTool *offset_tool)
{
  g_object_set (GIMP_FILTER_TOOL (offset_tool)->config,
                "x", gimp_offset_tool_get_width (offset_tool) / 2,
                NULL);
}

static void
gimp_offset_tool_half_y_clicked (GtkButton      *button,
                                 GimpOffsetTool *offset_tool)
{
  g_object_set (GIMP_FILTER_TOOL (offset_tool)->config,
                "y", gimp_offset_tool_get_height (offset_tool) / 2,
                NULL);
}

static void
gimp_offset_tool_edge_behavior_toggled (GtkToggleButton *toggle,
                                        GimpOffsetTool  *offset_tool)
{
  if (gtk_toggle_button_get_active (toggle))
    {
      GimpOffsetType type;

      type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (toggle),
                                                 "gimp-item-data"));

      g_object_set (GIMP_FILTER_TOOL (offset_tool)->config,
                    "type", type,
                    NULL);
    }
}

static void
gimp_offset_tool_background_changed (GimpContext    *context,
                                     GeglColor      *color,
                                     GimpOffsetTool *offset_tool)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (offset_tool);
  GimpOffsetType  type;

  g_object_get (filter_tool->config,
                "type", &type,
                NULL);

  if (type == GIMP_OFFSET_BACKGROUND)
    {
      gegl_node_set (filter_tool->operation,
                     "context", context,
                     NULL);

      gimp_drawable_filter_apply (filter_tool->filter, NULL);
    }
}

static gint
gimp_offset_tool_get_width (GimpOffsetTool *offset_tool)
{
  GeglRectangle drawable_area;
  gint          drawable_offset_x;
  gint          drawable_offset_y;

  if (gimp_filter_tool_get_drawable_area (GIMP_FILTER_TOOL (offset_tool),
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
gimp_offset_tool_get_height (GimpOffsetTool *offset_tool)
{
  GeglRectangle drawable_area;
  gint          drawable_offset_x;
  gint          drawable_offset_y;

  if (gimp_filter_tool_get_drawable_area (GIMP_FILTER_TOOL (offset_tool),
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
gimp_offset_tool_update (GimpOffsetTool *offset_tool)
{
  GimpTool       *tool        = GIMP_TOOL (offset_tool);
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (offset_tool);
  GimpOffsetType  orig_type;
  gint            orig_x;
  gint            orig_y;
  GimpOffsetType  type;
  gint            x;
  gint            y;
  gint            width;
  gint            height;

  g_object_get (filter_tool->config,
                "type", &orig_type,
                "x",    &orig_x,
                "y",    &orig_y,
                NULL);

  width  = gimp_offset_tool_get_width  (offset_tool);
  height = gimp_offset_tool_get_height (offset_tool);

  x = CLAMP (orig_x, -width,  +width);
  y = CLAMP (orig_y, -height, +height);

  type = orig_type;

  if (tool->drawables                            &&
      ! gimp_drawable_has_alpha (tool->drawables->data) &&
      type == GIMP_OFFSET_TRANSPARENT)
    {
      type = GIMP_OFFSET_BACKGROUND;
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
      gint width  = gimp_offset_tool_get_width  (offset_tool);
      gint height = gimp_offset_tool_get_height (offset_tool);

      g_signal_handlers_block_by_func (offset_tool->offset_se,
                                       gimp_offset_tool_offset_changed,
                                       offset_tool);

      gimp_size_entry_set_refval_boundaries (
        GIMP_SIZE_ENTRY (offset_tool->offset_se), 0,
        -width, +width);
      gimp_size_entry_set_refval_boundaries (
        GIMP_SIZE_ENTRY (offset_tool->offset_se), 1,
        -height, +height);

      gimp_size_entry_set_size (
        GIMP_SIZE_ENTRY (offset_tool->offset_se), 0,
        0, width);
      gimp_size_entry_set_size (
        GIMP_SIZE_ENTRY (offset_tool->offset_se), 1,
        0, height);

      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (offset_tool->offset_se), 0,
                                  x);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (offset_tool->offset_se), 1,
                                  y);

      g_signal_handlers_unblock_by_func (offset_tool->offset_se,
                                         gimp_offset_tool_offset_changed,
                                         offset_tool);
    }

  if (offset_tool->transparent_radio)
    {
      gimp_int_radio_group_set_active (
        GTK_RADIO_BUTTON (offset_tool->transparent_radio),
        type);
    }
}

static void
gimp_offset_tool_halt (GimpOffsetTool *offset_tool)
{
  GimpContext *context = GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (offset_tool));

  offset_tool->offset_se         = NULL;
  offset_tool->transparent_radio = NULL;

  g_signal_handlers_disconnect_by_func (
    context,
    gimp_offset_tool_background_changed,
    offset_tool);
}
