/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolbutton.c
 * Copyright (C) 2020 Ell
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
#include "libgimpwidgets/gimpwidgets-private.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimp-gui.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdisplay.h"
#include "core/gimptoolgroup.h"
#include "core/gimptoolinfo.h"

#include "actions/tools-commands.h"

#include "gimpaccellabel.h"
#include "gimpaction.h"
#include "gimpdock.h"
#include "gimptoolbox.h"
#include "gimptoolbutton.h"
#include "gimpwidgets-utils.h"
#include "gimpwindowstrategy.h"

#include "gimp-intl.h"


#define ARROW_SIZE   0.125 /* * 100%       */
#define ARROW_BORDER 3     /* px           */

#define MENU_TIMEOUT 250   /* milliseconds */


enum
{
  PROP_0,
  PROP_TOOLBOX,
  PROP_TOOL_ITEM
};


struct _GimpToolButtonPrivate
{
  GimpToolbox  *toolbox;
  GimpToolItem *tool_item;

  GtkWidget    *palette;

  GtkWidget    *tooltip_widget;

  GtkWidget    *menu;
  GHashTable   *menu_items;
  gint          menu_timeout_id;
  GdkEvent     *menu_timeout_event;
};


/*  local function prototypes  */

static void         gimp_tool_button_constructed         (GObject             *object);
static void         gimp_tool_button_dispose             (GObject             *object);
static void         gimp_tool_button_set_property        (GObject             *object,
                                                          guint                property_id,
                                                          const GValue        *value,
                                                          GParamSpec          *pspec);
static void         gimp_tool_button_get_property        (GObject             *object,
                                                          guint                property_id,
                                                          GValue              *value,
                                                          GParamSpec          *pspec);

static void         gimp_tool_button_hierarchy_changed   (GtkWidget           *widget,
                                                          GtkWidget           *previous_toplevel);
static gboolean     gimp_tool_button_draw                (GtkWidget           *widget,
                                                          cairo_t             *cr);
static gboolean     gimp_tool_button_query_tooltip       (GtkWidget           *widget,
                                                          gint                 x,
                                                          gint                 y,
                                                          gboolean             keyboard_mode,
                                                          GtkTooltip          *tooltip);

static void         gimp_tool_button_toggled             (GtkToggleToolButton *toggle_tool_button);

static gboolean     gimp_tool_button_button_press        (GtkWidget           *widget,
                                                          GdkEventButton      *event,
                                                          GimpToolButton      *tool_button);
static gboolean     gimp_tool_button_button_release      (GtkWidget           *widget,
                                                          GdkEventButton      *event,
                                                          GimpToolButton      *tool_button);
static gboolean     gimp_tool_button_scroll              (GtkWidget           *widget,
                                                          GdkEventScroll      *event,
                                                          GimpToolButton      *tool_button);

static void         gimp_tool_button_tool_changed        (GimpContext         *context,
                                                          GimpToolInfo        *tool_info,
                                                          GimpToolButton      *tool_button);

static void         gimp_tool_button_active_tool_changed (GimpToolGroup       *tool_group,
                                                          GimpToolButton      *tool_button);

static void         gimp_tool_button_tool_add            (GimpContainer       *container,
                                                          GimpToolInfo        *tool_info,
                                                          GimpToolButton      *tool_button);
static void         gimp_tool_button_tool_remove         (GimpContainer       *container,
                                                          GimpToolInfo        *tool_info,
                                                          GimpToolButton      *tool_button);
static void         gimp_tool_button_tool_reorder        (GimpContainer       *container,
                                                          GimpToolInfo        *tool_info,
                                                          gint                 old_index,
                                                          gint                 new_index,
                                                          GimpToolButton      *tool_button);

static void         gimp_tool_button_icon_size_notify    (GtkToolPalette      *palette,
                                                          const GParamSpec    *pspec,
                                                          GimpToolButton      *tool_button);

static void         gimp_tool_button_menu_item_activate  (GtkMenuItem         *menu_item,
                                                          GimpAction          *action);

static gboolean     gimp_tool_button_menu_leave_notify   (GtkMenu             *menu,
                                                          GdkEventCrossing    *event,
                                                          GimpToolButton      *tool_button);

static gboolean     gimp_tool_button_menu_timeout        (GimpToolButton      *tool_button);

static void         gimp_tool_button_update              (GimpToolButton      *tool_button);
static void         gimp_tool_button_update_toggled      (GimpToolButton      *tool_button);
static void         gimp_tool_button_update_menu         (GimpToolButton      *tool_button);

static void         gimp_tool_button_add_menu_item       (GimpToolButton      *tool_button,
                                                          GimpToolInfo        *tool_info,
                                                          gint                 index);
static void         gimp_tool_button_remove_menu_item    (GimpToolButton      *tool_button,
                                                          GimpToolInfo        *tool_info);

static void         gimp_tool_button_reconstruct_menu    (GimpToolButton      *tool_button);
static void         gimp_tool_button_destroy_menu        (GimpToolButton      *tool_button);
static gboolean     gimp_tool_button_show_menu           (GimpToolButton      *tool_button,
                                                          const GdkEvent      *trigger_event);

static GimpAction * gimp_tool_button_get_action          (GimpToolButton      *tool_button,
                                                          GimpToolInfo        *tool_info);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolButton, gimp_tool_button,
                            GTK_TYPE_TOGGLE_TOOL_BUTTON)

#define parent_class gimp_tool_button_parent_class


/*  private functions  */

static void
gimp_tool_button_class_init (GimpToolButtonClass *klass)
{
  GObjectClass             *object_class             = G_OBJECT_CLASS (klass);
  GtkWidgetClass           *widget_class             = GTK_WIDGET_CLASS (klass);
  GtkToggleToolButtonClass *toggle_tool_button_class = GTK_TOGGLE_TOOL_BUTTON_CLASS (klass);

  object_class->constructed         = gimp_tool_button_constructed;
  object_class->dispose             = gimp_tool_button_dispose;
  object_class->get_property        = gimp_tool_button_get_property;
  object_class->set_property        = gimp_tool_button_set_property;

  widget_class->hierarchy_changed   = gimp_tool_button_hierarchy_changed;
  widget_class->draw                = gimp_tool_button_draw;
  widget_class->query_tooltip       = gimp_tool_button_query_tooltip;

  toggle_tool_button_class->toggled = gimp_tool_button_toggled;

  g_object_class_install_property (object_class, PROP_TOOLBOX,
                                   g_param_spec_object ("toolbox",
                                                        NULL, NULL,
                                                        GIMP_TYPE_TOOLBOX,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_TOOL_ITEM,
                                   g_param_spec_object ("tool-item",
                                                        NULL, NULL,
                                                        GIMP_TYPE_TOOL_ITEM,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_tool_button_init (GimpToolButton *tool_button)
{
  tool_button->priv = gimp_tool_button_get_instance_private (tool_button);
}

static void
gimp_tool_button_constructed (GObject *object)
{
  GimpToolButton *tool_button = GIMP_TOOL_BUTTON (object);
  GimpContext    *context;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  context = gimp_toolbox_get_context (tool_button->priv->toolbox);

  /* Make sure the toolbox buttons won't grab focus, which has
   * nearly no practical use, and prevents various actions until
   * you click back in canvas.
   */
  gtk_widget_set_can_focus (gtk_bin_get_child (GTK_BIN (tool_button)), FALSE);

  gtk_widget_add_events (gtk_bin_get_child (GTK_BIN (tool_button)),
                         GDK_SCROLL_MASK);

  g_signal_connect (gtk_bin_get_child (GTK_BIN (tool_button)),
                    "button-press-event",
                    G_CALLBACK (gimp_tool_button_button_press),
                    tool_button);
  g_signal_connect (gtk_bin_get_child (GTK_BIN (tool_button)),
                    "button-release-event",
                    G_CALLBACK (gimp_tool_button_button_release),
                    tool_button);
  g_signal_connect (gtk_bin_get_child (GTK_BIN (tool_button)),
                    "scroll-event",
                    G_CALLBACK (gimp_tool_button_scroll),
                    tool_button);

  g_signal_connect_object (context, "tool-changed",
                           G_CALLBACK (gimp_tool_button_tool_changed),
                           tool_button,
                           0);

  gimp_tool_button_update (tool_button);
}

static void
gimp_tool_button_dispose (GObject *object)
{
  GimpToolButton *tool_button = GIMP_TOOL_BUTTON (object);

  gimp_tool_button_set_tool_item (tool_button, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tool_button_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpToolButton *tool_button = GIMP_TOOL_BUTTON (object);

  switch (property_id)
    {
    case PROP_TOOLBOX:
      tool_button->priv->toolbox = g_value_get_object (value);
      break;

    case PROP_TOOL_ITEM:
      gimp_tool_button_set_tool_item (tool_button, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_button_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpToolButton *tool_button = GIMP_TOOL_BUTTON (object);

  switch (property_id)
    {
    case PROP_TOOLBOX:
      g_value_set_object (value, tool_button->priv->toolbox);
      break;

    case PROP_TOOL_ITEM:
      g_value_set_object (value, tool_button->priv->tool_item);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_button_hierarchy_changed (GtkWidget *widget,
                                    GtkWidget *previous_toplevel)
{
  GimpToolButton *tool_button = GIMP_TOOL_BUTTON (widget);
  GtkWidget      *palette;

  if (GTK_WIDGET_CLASS (parent_class)->hierarchy_changed)
    {
      GTK_WIDGET_CLASS (parent_class)->hierarchy_changed (widget,
                                                          previous_toplevel);
    }

  palette = gtk_widget_get_ancestor (GTK_WIDGET (tool_button),
                                     GTK_TYPE_TOOL_PALETTE);

  if (palette != tool_button->priv->palette)
    {
      if (tool_button->priv->palette)
        {
          g_signal_handlers_disconnect_by_func (
            tool_button->priv->palette,
            gimp_tool_button_icon_size_notify,
            tool_button);
        }

      tool_button->priv->palette = palette;

      if (tool_button->priv->palette)
        {
          g_signal_connect (
            tool_button->priv->palette, "notify::icon-size",
            G_CALLBACK (gimp_tool_button_icon_size_notify),
            tool_button);
        }
    }

  gimp_tool_button_update (tool_button);

  gimp_tool_button_reconstruct_menu (tool_button);
}

static gboolean
gimp_tool_button_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  GimpToolButton *tool_button = GIMP_TOOL_BUTTON (widget);

  if (GTK_WIDGET_CLASS (parent_class)->draw)
    GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);

  if (GIMP_IS_TOOL_GROUP (tool_button->priv->tool_item))
    {
      GtkStyleContext *style = gtk_widget_get_style_context (widget);
      GtkStateFlags    state = gtk_widget_get_state_flags (widget);
      GdkRGBA          fg;
      GtkAllocation    allocation;
      gint             size;
      gint             x1, y1;
      gint             x2, y2;

      gtk_style_context_get_color (style, state, &fg);

      gtk_widget_get_allocation (widget, &allocation);

      size = MIN (allocation.width, allocation.height);

      x1 = SIGNED_ROUND (allocation.width  -
                         (ARROW_BORDER + size * ARROW_SIZE));
      y1 = SIGNED_ROUND (allocation.height -
                         (ARROW_BORDER + size * ARROW_SIZE));

      x2 = SIGNED_ROUND (allocation.width  - ARROW_BORDER);
      y2 = SIGNED_ROUND (allocation.height - ARROW_BORDER);

      cairo_save (cr);

      cairo_move_to (cr, x2, y1);
      cairo_line_to (cr, x2, y2);
      cairo_line_to (cr, x1, y2);
      cairo_close_path (cr);

      gdk_cairo_set_source_rgba (cr, &fg);
      cairo_fill (cr);

      cairo_restore (cr);
    }

  return FALSE;
}

static GtkWidget *
gimp_tool_button_query_tooltip_add_tool (GimpToolButton *tool_button,
                                         GtkGrid        *grid,
                                         gint            row,
                                         GimpToolInfo   *tool_info,
                                         const gchar    *label_str,
                                         GtkIconSize     icon_size)
{
  Gimp        *gimp;
  GimpAction  *action = NULL;
  GtkWidget   *label;
  GtkWidget   *image;
  gchar       *name;

  gimp = gimp_toolbox_get_context (tool_button->priv->toolbox)->gimp;
  name = gimp_tool_info_get_action_name (tool_info);

  action = GIMP_ACTION (g_action_map_lookup_action (G_ACTION_MAP (gimp->app), name));
  g_free (name);

  image = gtk_image_new_from_icon_name (
    gimp_viewable_get_icon_name (GIMP_VIEWABLE (tool_info)),
    icon_size);
  gtk_grid_attach (grid,
                   image,
                   0, row,
                   1, 1);
  gtk_widget_show (image);

  label = gtk_label_new (label_str);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (grid,
                   label,
                   1, row,
                   1, 1);
  gtk_widget_show (label);

  if (action)
    {
      GtkWidget *accel_label;

      accel_label = gimp_accel_label_new (action);
      gtk_widget_set_margin_start (accel_label, 16);
      gtk_label_set_xalign (GTK_LABEL (accel_label), 1.0);
      gtk_grid_attach (grid,
                       accel_label,
                       2, row,
                       1, 1);
      gtk_widget_show (accel_label);
    }

  return label;
}

static gboolean
gimp_tool_button_query_tooltip (GtkWidget  *widget,
                                gint        x,
                                gint        y,
                                gboolean    keyboard_mode,
                                GtkTooltip *tooltip)
{
  GimpToolButton *tool_button = GIMP_TOOL_BUTTON (widget);

  if (tool_button->priv->tooltip_widget &&
      GIMP_IS_TOOL_GROUP (tool_button->priv->tool_item))
    g_clear_object (&tool_button->priv->tooltip_widget);

  if (! tool_button->priv->tooltip_widget)
    {
      GimpToolInfo  *tool_info;
      GtkWidget     *grid;
      GtkWidget     *label;
      gchar        **tooltip_labels;
      GtkIconSize    icon_size = GTK_ICON_SIZE_MENU;
      gint           row       = 0;

      tool_info = gimp_tool_button_get_tool_info (tool_button);

      if (! tool_info)
        return FALSE;

      if (tool_button->priv->palette)
        {
          icon_size = gtk_tool_palette_get_icon_size (
            GTK_TOOL_PALETTE (tool_button->priv->palette));
        }

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
      gtk_widget_show (grid);

      tool_button->priv->tooltip_widget = g_object_ref_sink (grid);

      tooltip_labels = g_strsplit (tool_info->tooltip, ": ", 2);

      label = gimp_tool_button_query_tooltip_add_tool (tool_button,
                                                       GTK_GRID (grid),
                                                       row++,
                                                       tool_info,
                                                       tooltip_labels[0],
                                                       icon_size);
      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                 -1);

      if (tooltip_labels[0])
        {
          //gtk_table_set_row_spacing (GTK_TABLE (table), 0, 0);

          label = gtk_label_new (tooltip_labels[1]);
          gtk_label_set_xalign (GTK_LABEL (label), 0.0);
          gtk_grid_attach (GTK_GRID (grid),
                           label,
                           1, row++,
                           1, 1);
          gtk_widget_show (label);
        }

      g_strfreev (tooltip_labels);

      if (GIMP_IS_TOOL_GROUP (tool_button->priv->tool_item))
        {
          GimpContainer *children;
          gint           n_children;

          children = gimp_viewable_get_children (
            GIMP_VIEWABLE (tool_button->priv->tool_item));

          n_children = gimp_container_get_n_children (children);

          if (n_children > 1)
            {
              GtkWidget *label;
              gint       i;

              label = gtk_label_new (_("Also in group:"));
              gtk_widget_set_margin_top (label, 4);
              gtk_label_set_xalign (GTK_LABEL (label), 0.0);
              gimp_label_set_attributes (GTK_LABEL (label),
                                         PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                         -1);
              gtk_grid_attach (GTK_GRID (grid),
                               label,
                               0, row++,
                               3, 1);
              gtk_widget_show (label);

              for (i = 0; i < n_children; i++)
                {
                  GimpToolInfo *other_tool_info;
                  gboolean      visible;

                  other_tool_info = GIMP_TOOL_INFO (
                    gimp_container_get_child_by_index (children, i));

                  visible =
                    gimp_tool_item_get_visible (GIMP_TOOL_ITEM (other_tool_info));
                  if (other_tool_info != tool_info && visible)
                    {
                      gimp_tool_button_query_tooltip_add_tool (
                        tool_button,
                        GTK_GRID (grid),
                        row++,
                        other_tool_info,
                        other_tool_info->label,
                        icon_size);
                    }
                }
            }
        }
    }

  gtk_tooltip_set_custom (tooltip, tool_button->priv->tooltip_widget);

  return TRUE;
}

static void
gimp_tool_button_toggled (GtkToggleToolButton *toggle_tool_button)
{
  GimpToolButton *tool_button = GIMP_TOOL_BUTTON (toggle_tool_button);
  GimpContext    *context;
  GimpToolInfo   *tool_info;

  if (GTK_TOGGLE_TOOL_BUTTON_CLASS (parent_class)->toggled)
    GTK_TOGGLE_TOOL_BUTTON_CLASS (parent_class)->toggled (toggle_tool_button);

  context   = gimp_toolbox_get_context (tool_button->priv->toolbox);
  tool_info = gimp_tool_button_get_tool_info (tool_button);

  if (tool_info)
    {
      GimpDisplay *display;

      if (gtk_toggle_tool_button_get_active (toggle_tool_button))
        gimp_context_set_tool (context, tool_info);
      else if (tool_info == gimp_context_get_tool (context))
        gtk_toggle_tool_button_set_active (toggle_tool_button, TRUE);

      /* Give focus to the main image. */
      if ((display = gimp_context_get_display (context)) &&
          gimp_context_get_image (context))
        {
          gimp_display_grab_focus (display);
        }
    }
  else
    {
      gtk_toggle_tool_button_set_active (toggle_tool_button, FALSE);
    }
}

static gboolean
gimp_tool_button_button_press (GtkWidget      *widget,
                               GdkEventButton *event,
                               GimpToolButton *tool_button)
{
  if (GIMP_IS_TOOL_GROUP (tool_button->priv->tool_item))
    {
      if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
          gimp_tool_button_show_menu (tool_button, (GdkEvent *) event))
        {
          return TRUE;
        }
      else if (event->type == GDK_BUTTON_PRESS && event->button == 1 &&
               ! tool_button->priv->menu_timeout_id)
        {
          GdkEventButton *timeout_event;

          timeout_event        = (GdkEventButton *) gdk_event_copy (
            (GdkEvent *) event);
          timeout_event->time += MENU_TIMEOUT;

          tool_button->priv->menu_timeout_event = (GdkEvent *) timeout_event;

          tool_button->priv->menu_timeout_id = g_timeout_add (
            MENU_TIMEOUT,
            (GSourceFunc) gimp_tool_button_menu_timeout,
            tool_button);
        }
    }

  if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      GimpContext *context;
      GimpDock    *dock;

      context = gimp_toolbox_get_context (tool_button->priv->toolbox);
      dock    = GIMP_DOCK (tool_button->priv->toolbox);

      gimp_window_strategy_show_dockable_dialog (
        GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (context->gimp)),
        context->gimp,
        gimp_dock_get_dialog_factory (dock),
        gimp_widget_get_monitor (widget),
        "gimp-tool-options");

      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_tool_button_button_release (GtkWidget      *widget,
                                 GdkEventButton *event,
                                 GimpToolButton *tool_button)
{
  if (event->button == 1 && tool_button->priv->menu_timeout_id)
    {
      g_source_remove (tool_button->priv->menu_timeout_id);

      tool_button->priv->menu_timeout_id = 0;

      g_clear_pointer (&tool_button->priv->menu_timeout_event, gdk_event_free);
    }

  return FALSE;
}

static gboolean
gimp_tool_button_scroll (GtkWidget      *widget,
                         GdkEventScroll *event,
                         GimpToolButton *tool_button)
{
  GimpToolInfo *tool_info;
  gint          delta;

  tool_info = gimp_tool_button_get_tool_info (tool_button);

  switch (event->direction)
    {
    case GDK_SCROLL_UP:
      delta = -1;
      break;

    case GDK_SCROLL_DOWN:
      delta = +1;
      break;

    default:
      return FALSE;
    }

  if (tool_info && GIMP_IS_TOOL_GROUP (tool_button->priv->tool_item))
    {
      GimpContainer *children;
      gint           n_children;
      gint           index;
      gint           i;

      children = gimp_viewable_get_children (
        GIMP_VIEWABLE (tool_button->priv->tool_item));

      n_children = gimp_container_get_n_children (children);

      index = gimp_container_get_child_index (children,
                                              GIMP_OBJECT (tool_info));

      for (i = 1; i < n_children; i++)
        {
          GimpToolInfo *new_tool_info;
          gint          new_index;

          new_index = (index + i * delta) % n_children;
          if (new_index < 0) new_index += n_children;

          new_tool_info = GIMP_TOOL_INFO (
            gimp_container_get_child_by_index (children, new_index));

          if (gimp_tool_item_get_visible (GIMP_TOOL_ITEM (new_tool_info)))
            {
              gimp_tool_group_set_active_tool_info (
                GIMP_TOOL_GROUP (tool_button->priv->tool_item), new_tool_info);

              break;
            }
        }
    }

  return FALSE;
}

static void
gimp_tool_button_tool_changed (GimpContext    *context,
                               GimpToolInfo   *tool_info,
                               GimpToolButton *tool_button)
{
  gimp_tool_button_update_toggled (tool_button);
}

static void
gimp_tool_button_active_tool_changed (GimpToolGroup  *tool_group,
                                      GimpToolButton *tool_button)
{
  gimp_tool_button_update (tool_button);
}

static void
gimp_tool_button_tool_add (GimpContainer  *container,
                           GimpToolInfo   *tool_info,
                           GimpToolButton *tool_button)
{
  gint index;

  index = gimp_container_get_child_index (container, GIMP_OBJECT (tool_info));

  gimp_tool_button_add_menu_item (tool_button, tool_info, index);

  gimp_tool_button_update (tool_button);
}

static void
gimp_tool_button_tool_remove (GimpContainer  *container,
                              GimpToolInfo   *tool_info,
                              GimpToolButton *tool_button)
{
  gimp_tool_button_remove_menu_item (tool_button, tool_info);

  gimp_tool_button_update (tool_button);
}

static void
gimp_tool_button_tool_reorder (GimpContainer  *container,
                               GimpToolInfo   *tool_info,
                               gint            old_index,
                               gint            new_index,
                               GimpToolButton *tool_button)
{
  gimp_tool_button_remove_menu_item (tool_button, tool_info);
  gimp_tool_button_add_menu_item    (tool_button, tool_info, new_index);

  gimp_tool_button_update (tool_button);
}

static void
gimp_tool_button_icon_size_notify (GtkToolPalette   *palette,
                                   const GParamSpec *pspec,
                                   GimpToolButton   *tool_button)
{
  gimp_tool_button_reconstruct_menu (tool_button);

  gimp_tool_button_update (tool_button);
}

static void
gimp_tool_button_menu_item_activate (GtkMenuItem *menu_item,
                                     GimpAction  *action)
{
  gimp_action_activate (action);
}

static gboolean
gimp_tool_button_menu_leave_notify (GtkMenu          *menu,
                                    GdkEventCrossing *event,
                                    GimpToolButton   *tool_button)
{
  if (event->mode == GDK_CROSSING_NORMAL &&
      gtk_widget_get_visible (tool_button->priv->menu))
    {
      gimp_tool_button_update_menu (tool_button);
    }

  return FALSE;
}

static gboolean
gimp_tool_button_menu_timeout (GimpToolButton *tool_button)
{
  GdkEvent *event = tool_button->priv->menu_timeout_event;

  tool_button->priv->menu_timeout_id    = 0;
  tool_button->priv->menu_timeout_event = NULL;

  gimp_tool_button_show_menu (tool_button, event);

  gdk_event_free (event);

  /* work around gtk not properly redrawing the button in the toggled state */
  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (tool_button)))
    {
      gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (tool_button),
                                         FALSE);
    }

  return G_SOURCE_REMOVE;
}

static void
gimp_tool_button_update (GimpToolButton *tool_button)
{
  GimpToolInfo *tool_info;

  tool_info = gimp_tool_button_get_tool_info (tool_button);

  gtk_tool_button_set_icon_name (
    GTK_TOOL_BUTTON (tool_button),
    tool_info ? gimp_viewable_get_icon_name (GIMP_VIEWABLE (tool_info)) :
                NULL);

  g_clear_object (&tool_button->priv->tooltip_widget);

  if (tool_info)
    {
      gimp_help_set_help_data (GTK_WIDGET (tool_button),
                               tool_info->tooltip, tool_info->help_id);
    }
  else
    {
      gimp_help_set_help_data (GTK_WIDGET (tool_button), NULL, NULL);
    }

  if (tool_info)
    {
      const gchar *tool_name = gimp_object_get_name (tool_info);
      gchar       *identifier;

      if (g_str_has_prefix (tool_name, "gimp-") &&
          g_str_has_suffix (tool_name, "-tool"))
        {
          /* The GimpToolInfo names are of the form "gimp-pencil-tool",
           * and action names are of the form "tools-pencil".
           * To simplify things, I make the tool button identifiers the
           * same as the actions, which make them easier to find.
           */
          gchar *suffix;

          identifier = g_strdup_printf ("tools-%s", tool_name + 5);
          suffix = g_strrstr (identifier, "-tool");
          suffix[0] = '\0';
        }
      else
        {
          identifier = g_strdup (tool_name);
        }

      gimp_widget_set_identifier (GTK_WIDGET (tool_button), identifier);
      g_free (identifier);
    }

  gimp_tool_button_update_toggled (tool_button);
  gimp_tool_button_update_menu    (tool_button);
}

static void
gimp_tool_button_update_toggled (GimpToolButton *tool_button)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;

  context = gimp_toolbox_get_context (tool_button->priv->toolbox);

  tool_info = gimp_tool_button_get_tool_info (tool_button);

  gtk_toggle_tool_button_set_active (
    GTK_TOGGLE_TOOL_BUTTON (tool_button),
    tool_info && tool_info == gimp_context_get_tool (context));
}

static void
gimp_tool_button_update_menu (GimpToolButton *tool_button)
{
  if (tool_button->priv->menu &&
      gtk_widget_get_visible (tool_button->priv->menu))
    {
      GimpToolInfo *tool_info = gimp_tool_button_get_tool_info (tool_button);

      if (tool_info)
        {
          gtk_menu_shell_select_item (
            GTK_MENU_SHELL (tool_button->priv->menu),
            g_hash_table_lookup (tool_button->priv->menu_items, tool_info));
        }
    }
}

static void
gimp_tool_button_add_menu_item (GimpToolButton *tool_button,
                                GimpToolInfo   *tool_info,
                                gint            index)
{
  GimpAction  *action;
  GtkWidget   *item;
  GtkWidget   *hbox;
  GtkWidget   *image;
  GtkWidget   *label;
  GtkIconSize  icon_size = GTK_ICON_SIZE_MENU;

  action = gimp_tool_button_get_action (tool_button, tool_info);

  if (tool_button->priv->palette)
    {
      icon_size = gtk_tool_palette_get_icon_size (
        GTK_TOOL_PALETTE (tool_button->priv->palette));
    }

  item = gtk_menu_item_new ();
  gtk_menu_shell_insert (GTK_MENU_SHELL (tool_button->priv->menu), item, index);
  g_signal_connect (item, "activate",
                    G_CALLBACK (gimp_tool_button_menu_item_activate),
                    action);
  gimp_help_set_help_data (item, tool_info->tooltip, tool_info->help_id);

  g_object_bind_property (tool_info, "visible",
                          item,      "visible",
                          G_BINDING_SYNC_CREATE);

  g_object_set_data (G_OBJECT (item), "gimp-tool-info", tool_info);

  gimp_gtk_container_clear (GTK_CONTAINER (item));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add (GTK_CONTAINER (item), hbox);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_icon_name (
    gimp_viewable_get_icon_name (GIMP_VIEWABLE (tool_info)),
    icon_size);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  label = gtk_accel_label_new (tool_info->label);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  if (action)
    {
      label = gimp_accel_label_new (action);
      gtk_label_set_xalign (GTK_LABEL (label), 1.0);
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
      gtk_widget_show (label);

      /* TODO: while porting to GAction, this part seems useless, as the toolbox
       * is perfectly working. But maybe I'm missing something. So leaving this
       * here for further investigation.
       * We probably want to check what gimp_ui_manager_connect_proxy() does
       * exactly.
       */
#if 0
      if (ui_manager)
        {
          g_signal_emit_by_name (ui_manager, "connect-proxy",
                                 action, item);
        }
#endif
    }

  g_hash_table_insert (tool_button->priv->menu_items, tool_info, item);
}

static void
gimp_tool_button_remove_menu_item (GimpToolButton *tool_button,
                                   GimpToolInfo   *tool_info)
{
  GtkWidget *item;

  item = g_hash_table_lookup (tool_button->priv->menu_items, tool_info);

  gtk_container_remove (GTK_CONTAINER (tool_button->priv->menu), item);

  g_hash_table_remove (tool_button->priv->menu_items, tool_info);
}

static void
gimp_tool_button_reconstruct_menu_add_menu_item (GimpToolInfo   *tool_info,
                                                 GimpToolButton *tool_button)
{
  gimp_tool_button_add_menu_item (tool_button, tool_info, -1);
}

static void
gimp_tool_button_reconstruct_menu (GimpToolButton *tool_button)
{
  gimp_tool_button_destroy_menu (tool_button);

  if (GIMP_IS_TOOL_GROUP (tool_button->priv->tool_item))
    {
      GimpContainer *children;

      children = gimp_viewable_get_children (
        GIMP_VIEWABLE (tool_button->priv->tool_item));

      tool_button->priv->menu = gtk_menu_new ();
      gtk_menu_attach_to_widget (GTK_MENU (tool_button->priv->menu),
                                 GTK_WIDGET (tool_button), NULL);

      g_signal_connect (tool_button->priv->menu, "leave-notify-event",
                        G_CALLBACK (gimp_tool_button_menu_leave_notify),
                        tool_button);

      tool_button->priv->menu_items = g_hash_table_new (g_direct_hash,
                                                        g_direct_equal);

      gimp_container_foreach (
        children,
        (GFunc) gimp_tool_button_reconstruct_menu_add_menu_item,
        tool_button);
    }
}

static void
gimp_tool_button_destroy_menu (GimpToolButton *tool_button)
{
  if (tool_button->priv->menu)
    {
      gtk_menu_detach (GTK_MENU (tool_button->priv->menu));
      tool_button->priv->menu = NULL;

      g_clear_pointer (&tool_button->priv->menu_items, g_hash_table_unref);

      if (tool_button->priv->menu_timeout_id)
        {
          g_source_remove (tool_button->priv->menu_timeout_id);

          tool_button->priv->menu_timeout_id = 0;

          g_clear_pointer (&tool_button->priv->menu_timeout_event,
                           gdk_event_free);
        }
    }
}

static gboolean
gimp_tool_button_show_menu (GimpToolButton *tool_button,
                            const GdkEvent *trigger_event)
{
  if (! tool_button->priv->menu)
    return FALSE;

  gtk_menu_popup_at_widget (
    GTK_MENU (tool_button->priv->menu),
    GTK_WIDGET (tool_button),
    GDK_GRAVITY_NORTH_EAST,
    GDK_GRAVITY_NORTH_WEST,
    trigger_event);

  gimp_tool_button_update_menu (tool_button);

  return TRUE;
}

static GimpAction *
gimp_tool_button_get_action (GimpToolButton *tool_button,
                             GimpToolInfo   *tool_info)
{
  GimpAction *action = NULL;

  if (tool_info)
    {
      GimpContext *context;
      Gimp        *gimp;
      gchar       *name;

      context = gimp_toolbox_get_context (tool_button->priv->toolbox);
      gimp    = context->gimp;
      name    = gimp_tool_info_get_action_name (tool_info);

      action = GIMP_ACTION (g_action_map_lookup_action (G_ACTION_MAP (gimp->app), name));

      g_free (name);
    }

  return action;
}


/*  public functions  */

GtkToolItem *
gimp_tool_button_new (GimpToolbox  *toolbox,
                      GimpToolItem *tool_item)
{
  g_return_val_if_fail (GIMP_IS_TOOLBOX (toolbox), NULL);
  g_return_val_if_fail (tool_item == NULL ||
                        GIMP_IS_TOOL_ITEM (tool_item), NULL);

  return g_object_new (GIMP_TYPE_TOOL_BUTTON,
                       "toolbox",   toolbox,
                       "tool-item", tool_item,
                       NULL);
}

GimpToolbox *
gimp_tool_button_get_toolbox (GimpToolButton *tool_button)
{
  g_return_val_if_fail (GIMP_IS_TOOL_BUTTON (tool_button), NULL);

  return tool_button->priv->toolbox;
}

void
gimp_tool_button_set_tool_item (GimpToolButton *tool_button,
                                GimpToolItem   *tool_item)
{
  g_return_if_fail (GIMP_IS_TOOL_BUTTON (tool_button));
  g_return_if_fail (tool_item == NULL || GIMP_IS_TOOL_ITEM (tool_item));

  if (tool_item != tool_button->priv->tool_item)
    {
      if (GIMP_IS_TOOL_GROUP (tool_button->priv->tool_item))
        {
          GimpContainer *children;

          children = gimp_viewable_get_children (
            GIMP_VIEWABLE (tool_button->priv->tool_item));

          g_signal_handlers_disconnect_by_func (
            tool_button->priv->tool_item,
            gimp_tool_button_active_tool_changed,
            tool_button);

          g_signal_handlers_disconnect_by_func (
            children,
            gimp_tool_button_tool_add,
            tool_button);
          g_signal_handlers_disconnect_by_func (
            children,
            gimp_tool_button_tool_remove,
            tool_button);
          g_signal_handlers_disconnect_by_func (
            children,
            gimp_tool_button_tool_reorder,
            tool_button);

          gimp_tool_button_destroy_menu (tool_button);
        }

      g_set_object (&tool_button->priv->tool_item, tool_item);

      if (GIMP_IS_TOOL_GROUP (tool_button->priv->tool_item))
        {
          GimpContainer *children;

          children = gimp_viewable_get_children (
            GIMP_VIEWABLE (tool_button->priv->tool_item));

          g_signal_connect (
            tool_button->priv->tool_item, "active-tool-changed",
            G_CALLBACK (gimp_tool_button_active_tool_changed),
            tool_button);

          g_signal_connect (
            children, "add",
            G_CALLBACK (gimp_tool_button_tool_add),
            tool_button);
          g_signal_connect (
            children, "remove",
            G_CALLBACK (gimp_tool_button_tool_remove),
            tool_button);
          g_signal_connect (
            children, "reorder",
            G_CALLBACK (gimp_tool_button_tool_reorder),
            tool_button);

          gimp_tool_button_reconstruct_menu (tool_button);
        }

      gimp_tool_button_update (tool_button);

      g_object_notify (G_OBJECT (tool_button), "tool-item");
    }
}

GimpToolItem *
gimp_tool_button_get_tool_item (GimpToolButton *tool_button)
{
  g_return_val_if_fail (GIMP_IS_TOOL_BUTTON (tool_button), NULL);

  return tool_button->priv->tool_item;
}

GimpToolInfo *
gimp_tool_button_get_tool_info (GimpToolButton *tool_button)
{
  g_return_val_if_fail (GIMP_IS_TOOL_BUTTON (tool_button), NULL);

  if (tool_button->priv->tool_item)
    {
      if (GIMP_IS_TOOL_INFO (tool_button->priv->tool_item))
        {
          return GIMP_TOOL_INFO (tool_button->priv->tool_item);
        }
      else
        {
          return gimp_tool_group_get_active_tool_info (
            GIMP_TOOL_GROUP (tool_button->priv->tool_item));
        }
    }

  return NULL;
}
