/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolbutton.c
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"
#include "libligmawidgets/ligmawidgets-private.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligma-gui.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadisplay.h"
#include "core/ligmatoolgroup.h"
#include "core/ligmatoolinfo.h"

#include "actions/tools-commands.h"

#include "ligmaaccellabel.h"
#include "ligmaaction.h"
#include "ligmadock.h"
#include "ligmatoolbox.h"
#include "ligmatoolbutton.h"
#include "ligmauimanager.h"
#include "ligmawidgets-utils.h"
#include "ligmawindowstrategy.h"

#include "ligma-intl.h"


#define ARROW_SIZE   0.125 /* * 100%       */
#define ARROW_BORDER 3     /* px           */

#define MENU_TIMEOUT 250   /* milliseconds */


enum
{
  PROP_0,
  PROP_TOOLBOX,
  PROP_TOOL_ITEM
};


struct _LigmaToolButtonPrivate
{
  LigmaToolbox  *toolbox;
  LigmaToolItem *tool_item;

  GtkWidget    *palette;

  GtkWidget    *tooltip_widget;

  GtkWidget    *menu;
  GHashTable   *menu_items;
  gint          menu_timeout_id;
  GdkEvent     *menu_timeout_event;
};


/*  local function prototypes  */

static void         ligma_tool_button_constructed         (GObject             *object);
static void         ligma_tool_button_dispose             (GObject             *object);
static void         ligma_tool_button_set_property        (GObject             *object,
                                                          guint                property_id,
                                                          const GValue        *value,
                                                          GParamSpec          *pspec);
static void         ligma_tool_button_get_property        (GObject             *object,
                                                          guint                property_id,
                                                          GValue              *value,
                                                          GParamSpec          *pspec);

static void         ligma_tool_button_hierarchy_changed   (GtkWidget           *widget,
                                                          GtkWidget           *previous_toplevel);
static gboolean     ligma_tool_button_draw                (GtkWidget           *widget,
                                                          cairo_t             *cr);
static gboolean     ligma_tool_button_query_tooltip       (GtkWidget           *widget,
                                                          gint                 x,
                                                          gint                 y,
                                                          gboolean             keyboard_mode,
                                                          GtkTooltip          *tooltip);

static void         ligma_tool_button_toggled             (GtkToggleToolButton *toggle_tool_button);

static gboolean     ligma_tool_button_button_press        (GtkWidget           *widget,
                                                          GdkEventButton      *event,
                                                          LigmaToolButton      *tool_button);
static gboolean     ligma_tool_button_button_release      (GtkWidget           *widget,
                                                          GdkEventButton      *event,
                                                          LigmaToolButton      *tool_button);
static gboolean     ligma_tool_button_scroll              (GtkWidget           *widget,
                                                          GdkEventScroll      *event,
                                                          LigmaToolButton      *tool_button);

static void         ligma_tool_button_tool_changed        (LigmaContext         *context,
                                                          LigmaToolInfo        *tool_info,
                                                          LigmaToolButton      *tool_button);

static void         ligma_tool_button_active_tool_changed (LigmaToolGroup       *tool_group,
                                                          LigmaToolButton      *tool_button);

static void         ligma_tool_button_tool_add            (LigmaContainer       *container,
                                                          LigmaToolInfo        *tool_info,
                                                          LigmaToolButton      *tool_button);
static void         ligma_tool_button_tool_remove         (LigmaContainer       *container,
                                                          LigmaToolInfo        *tool_info,
                                                          LigmaToolButton      *tool_button);
static void         ligma_tool_button_tool_reorder        (LigmaContainer       *container,
                                                          LigmaToolInfo        *tool_info,
                                                          gint                 new_index,
                                                          LigmaToolButton      *tool_button);

static void         ligma_tool_button_icon_size_notify    (GtkToolPalette      *palette,
                                                          const GParamSpec    *pspec,
                                                          LigmaToolButton      *tool_button);

static gboolean     ligma_tool_button_menu_leave_notify   (GtkMenu             *menu,
                                                          GdkEventCrossing    *event,
                                                          LigmaToolButton      *tool_button);

static gboolean     ligma_tool_button_menu_timeout        (LigmaToolButton      *tool_button);

static void         ligma_tool_button_update              (LigmaToolButton      *tool_button);
static void         ligma_tool_button_update_toggled      (LigmaToolButton      *tool_button);
static void         ligma_tool_button_update_menu         (LigmaToolButton      *tool_button);

static void         ligma_tool_button_add_menu_item       (LigmaToolButton      *tool_button,
                                                          LigmaToolInfo        *tool_info,
                                                          gint                 index);
static void         ligma_tool_button_remove_menu_item    (LigmaToolButton      *tool_button,
                                                          LigmaToolInfo        *tool_info);

static void         ligma_tool_button_reconstruct_menu    (LigmaToolButton      *tool_button);
static void         ligma_tool_button_destroy_menu        (LigmaToolButton      *tool_button);
static gboolean     ligma_tool_button_show_menu           (LigmaToolButton      *tool_button,
                                                          const GdkEvent      *trigger_event);

static LigmaAction * ligma_tool_button_get_action          (LigmaToolButton      *tool_button,
                                                          LigmaToolInfo        *tool_info);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolButton, ligma_tool_button,
                            GTK_TYPE_TOGGLE_TOOL_BUTTON)

#define parent_class ligma_tool_button_parent_class


/*  private functions  */

static void
ligma_tool_button_class_init (LigmaToolButtonClass *klass)
{
  GObjectClass             *object_class             = G_OBJECT_CLASS (klass);
  GtkWidgetClass           *widget_class             = GTK_WIDGET_CLASS (klass);
  GtkToggleToolButtonClass *toggle_tool_button_class = GTK_TOGGLE_TOOL_BUTTON_CLASS (klass);

  object_class->constructed         = ligma_tool_button_constructed;
  object_class->dispose             = ligma_tool_button_dispose;
  object_class->get_property        = ligma_tool_button_get_property;
  object_class->set_property        = ligma_tool_button_set_property;

  widget_class->hierarchy_changed   = ligma_tool_button_hierarchy_changed;
  widget_class->draw                = ligma_tool_button_draw;
  widget_class->query_tooltip       = ligma_tool_button_query_tooltip;

  toggle_tool_button_class->toggled = ligma_tool_button_toggled;

  g_object_class_install_property (object_class, PROP_TOOLBOX,
                                   g_param_spec_object ("toolbox",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_TOOLBOX,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_TOOL_ITEM,
                                   g_param_spec_object ("tool-item",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_TOOL_ITEM,
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_tool_button_init (LigmaToolButton *tool_button)
{
  tool_button->priv = ligma_tool_button_get_instance_private (tool_button);
}

static void
ligma_tool_button_constructed (GObject *object)
{
  LigmaToolButton *tool_button = LIGMA_TOOL_BUTTON (object);
  LigmaContext    *context;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  context = ligma_toolbox_get_context (tool_button->priv->toolbox);

  /* Make sure the toolbox buttons won't grab focus, which has
   * nearly no practical use, and prevents various actions until
   * you click back in canvas.
   */
  gtk_widget_set_can_focus (gtk_bin_get_child (GTK_BIN (tool_button)), FALSE);

  gtk_widget_add_events (gtk_bin_get_child (GTK_BIN (tool_button)),
                         GDK_SCROLL_MASK);

  g_signal_connect (gtk_bin_get_child (GTK_BIN (tool_button)),
                    "button-press-event",
                    G_CALLBACK (ligma_tool_button_button_press),
                    tool_button);
  g_signal_connect (gtk_bin_get_child (GTK_BIN (tool_button)),
                    "button-release-event",
                    G_CALLBACK (ligma_tool_button_button_release),
                    tool_button);
  g_signal_connect (gtk_bin_get_child (GTK_BIN (tool_button)),
                    "scroll-event",
                    G_CALLBACK (ligma_tool_button_scroll),
                    tool_button);

  g_signal_connect_object (context, "tool-changed",
                           G_CALLBACK (ligma_tool_button_tool_changed),
                           tool_button,
                           0);

  ligma_tool_button_update (tool_button);
}

static void
ligma_tool_button_dispose (GObject *object)
{
  LigmaToolButton *tool_button = LIGMA_TOOL_BUTTON (object);

  ligma_tool_button_set_tool_item (tool_button, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_tool_button_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaToolButton *tool_button = LIGMA_TOOL_BUTTON (object);

  switch (property_id)
    {
    case PROP_TOOLBOX:
      tool_button->priv->toolbox = g_value_get_object (value);
      break;

    case PROP_TOOL_ITEM:
      ligma_tool_button_set_tool_item (tool_button, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_tool_button_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaToolButton *tool_button = LIGMA_TOOL_BUTTON (object);

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
ligma_tool_button_hierarchy_changed (GtkWidget *widget,
                                    GtkWidget *previous_toplevel)
{
  LigmaToolButton *tool_button = LIGMA_TOOL_BUTTON (widget);
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
            ligma_tool_button_icon_size_notify,
            tool_button);
        }

      tool_button->priv->palette = palette;

      if (tool_button->priv->palette)
        {
          g_signal_connect (
            tool_button->priv->palette, "notify::icon-size",
            G_CALLBACK (ligma_tool_button_icon_size_notify),
            tool_button);
        }
    }

  ligma_tool_button_update (tool_button);

  ligma_tool_button_reconstruct_menu (tool_button);
}

static gboolean
ligma_tool_button_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  LigmaToolButton *tool_button = LIGMA_TOOL_BUTTON (widget);

  if (GTK_WIDGET_CLASS (parent_class)->draw)
    GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);

  if (LIGMA_IS_TOOL_GROUP (tool_button->priv->tool_item))
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
ligma_tool_button_query_tooltip_add_tool (LigmaToolButton *tool_button,
                                         GtkGrid        *grid,
                                         gint            row,
                                         LigmaToolInfo   *tool_info,
                                         const gchar    *label_str,
                                         GtkIconSize     icon_size)
{
  LigmaUIManager *ui_manager;
  LigmaAction    *action = NULL;
  GtkWidget     *label;
  GtkWidget     *image;

  ui_manager = ligma_dock_get_ui_manager (
    LIGMA_DOCK (tool_button->priv->toolbox));

  if (ui_manager)
    {
      gchar *name;

      name = ligma_tool_info_get_action_name (tool_info);

      action = ligma_ui_manager_find_action (ui_manager, "tools", name);

      g_free (name);
    }

  image = gtk_image_new_from_icon_name (
    ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool_info)),
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

      accel_label = ligma_accel_label_new (action);
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
ligma_tool_button_query_tooltip (GtkWidget  *widget,
                                gint        x,
                                gint        y,
                                gboolean    keyboard_mode,
                                GtkTooltip *tooltip)
{
  LigmaToolButton *tool_button = LIGMA_TOOL_BUTTON (widget);

  if (! tool_button->priv->tooltip_widget)
    {
      LigmaToolInfo  *tool_info;
      GtkWidget     *grid;
      GtkWidget     *label;
      gchar        **tooltip_labels;
      GtkIconSize    icon_size = GTK_ICON_SIZE_MENU;
      gint           row       = 0;

      tool_info = ligma_tool_button_get_tool_info (tool_button);

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

      label = ligma_tool_button_query_tooltip_add_tool (tool_button,
                                                       GTK_GRID (grid),
                                                       row++,
                                                       tool_info,
                                                       tooltip_labels[0],
                                                       icon_size);
      ligma_label_set_attributes (GTK_LABEL (label),
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

      if (LIGMA_IS_TOOL_GROUP (tool_button->priv->tool_item))
        {
          LigmaContainer *children;
          gint           n_children;

          children = ligma_viewable_get_children (
            LIGMA_VIEWABLE (tool_button->priv->tool_item));

          n_children = ligma_container_get_n_children (children);

          if (n_children > 1)
            {
              GtkWidget *label;
              gint       i;

              label = gtk_label_new (_("Also in group:"));
              gtk_widget_set_margin_top (label, 4);
              gtk_label_set_xalign (GTK_LABEL (label), 0.0);
              ligma_label_set_attributes (GTK_LABEL (label),
                                         PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                         -1);
              gtk_grid_attach (GTK_GRID (grid),
                               label,
                               0, row++,
                               3, 1);
              gtk_widget_show (label);

              for (i = 0; i < n_children; i++)
                {
                  LigmaToolInfo *other_tool_info;

                  other_tool_info = LIGMA_TOOL_INFO (
                    ligma_container_get_child_by_index (children, i));

                  if (other_tool_info != tool_info)
                    {
                      ligma_tool_button_query_tooltip_add_tool (
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
ligma_tool_button_toggled (GtkToggleToolButton *toggle_tool_button)
{
  LigmaToolButton *tool_button = LIGMA_TOOL_BUTTON (toggle_tool_button);
  LigmaContext    *context;
  LigmaToolInfo   *tool_info;

  if (GTK_TOGGLE_TOOL_BUTTON_CLASS (parent_class)->toggled)
    GTK_TOGGLE_TOOL_BUTTON_CLASS (parent_class)->toggled (toggle_tool_button);

  context   = ligma_toolbox_get_context (tool_button->priv->toolbox);
  tool_info = ligma_tool_button_get_tool_info (tool_button);

  if (tool_info)
    {
      LigmaDisplay *display;

      if (gtk_toggle_tool_button_get_active (toggle_tool_button))
        ligma_context_set_tool (context, tool_info);
      else if (tool_info == ligma_context_get_tool (context))
        gtk_toggle_tool_button_set_active (toggle_tool_button, TRUE);

      /* Give focus to the main image. */
      if ((display = ligma_context_get_display (context)) &&
          ligma_context_get_image (context))
        {
          ligma_display_grab_focus (display);
        }
    }
  else
    {
      gtk_toggle_tool_button_set_active (toggle_tool_button, FALSE);
    }
}

static gboolean
ligma_tool_button_button_press (GtkWidget      *widget,
                               GdkEventButton *event,
                               LigmaToolButton *tool_button)
{
  if (LIGMA_IS_TOOL_GROUP (tool_button->priv->tool_item))
    {
      if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
          ligma_tool_button_show_menu (tool_button, (GdkEvent *) event))
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
            (GSourceFunc) ligma_tool_button_menu_timeout,
            tool_button);
        }
    }

  if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      LigmaContext *context;
      LigmaDock    *dock;

      context = ligma_toolbox_get_context (tool_button->priv->toolbox);
      dock    = LIGMA_DOCK (tool_button->priv->toolbox);

      ligma_window_strategy_show_dockable_dialog (
        LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (context->ligma)),
        context->ligma,
        ligma_dock_get_dialog_factory (dock),
        ligma_widget_get_monitor (widget),
        "ligma-tool-options");

      return TRUE;
    }

  return FALSE;
}

static gboolean
ligma_tool_button_button_release (GtkWidget      *widget,
                                 GdkEventButton *event,
                                 LigmaToolButton *tool_button)
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
ligma_tool_button_scroll (GtkWidget      *widget,
                         GdkEventScroll *event,
                         LigmaToolButton *tool_button)
{
  LigmaToolInfo *tool_info;
  gint          delta;

  tool_info = ligma_tool_button_get_tool_info (tool_button);

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

  if (tool_info && LIGMA_IS_TOOL_GROUP (tool_button->priv->tool_item))
    {
      LigmaContainer *children;
      gint           n_children;
      gint           index;
      gint           i;

      children = ligma_viewable_get_children (
        LIGMA_VIEWABLE (tool_button->priv->tool_item));

      n_children = ligma_container_get_n_children (children);

      index = ligma_container_get_child_index (children,
                                              LIGMA_OBJECT (tool_info));

      for (i = 1; i < n_children; i++)
        {
          LigmaToolInfo *new_tool_info;
          gint          new_index;

          new_index = (index + i * delta) % n_children;
          if (new_index < 0) new_index += n_children;

          new_tool_info = LIGMA_TOOL_INFO (
            ligma_container_get_child_by_index (children, new_index));

          if (ligma_tool_item_get_visible (LIGMA_TOOL_ITEM (new_tool_info)))
            {
              ligma_tool_group_set_active_tool_info (
                LIGMA_TOOL_GROUP (tool_button->priv->tool_item), new_tool_info);

              break;
            }
        }
    }

  return FALSE;
}

static void
ligma_tool_button_tool_changed (LigmaContext    *context,
                               LigmaToolInfo   *tool_info,
                               LigmaToolButton *tool_button)
{
  ligma_tool_button_update_toggled (tool_button);
}

static void
ligma_tool_button_active_tool_changed (LigmaToolGroup  *tool_group,
                                      LigmaToolButton *tool_button)
{
  ligma_tool_button_update (tool_button);
}

static void
ligma_tool_button_tool_add (LigmaContainer  *container,
                           LigmaToolInfo   *tool_info,
                           LigmaToolButton *tool_button)
{
  gint index;

  index = ligma_container_get_child_index (container, LIGMA_OBJECT (tool_info));

  ligma_tool_button_add_menu_item (tool_button, tool_info, index);

  ligma_tool_button_update (tool_button);
}

static void
ligma_tool_button_tool_remove (LigmaContainer  *container,
                              LigmaToolInfo   *tool_info,
                              LigmaToolButton *tool_button)
{
  ligma_tool_button_remove_menu_item (tool_button, tool_info);

  ligma_tool_button_update (tool_button);
}

static void
ligma_tool_button_tool_reorder (LigmaContainer  *container,
                               LigmaToolInfo   *tool_info,
                               gint            new_index,
                               LigmaToolButton *tool_button)
{
  ligma_tool_button_remove_menu_item (tool_button, tool_info);
  ligma_tool_button_add_menu_item    (tool_button, tool_info, new_index);

  ligma_tool_button_update (tool_button);
}

static void
ligma_tool_button_icon_size_notify (GtkToolPalette   *palette,
                                   const GParamSpec *pspec,
                                   LigmaToolButton   *tool_button)
{
  ligma_tool_button_reconstruct_menu (tool_button);

  ligma_tool_button_update (tool_button);
}

static gboolean
ligma_tool_button_menu_leave_notify (GtkMenu          *menu,
                                    GdkEventCrossing *event,
                                    LigmaToolButton   *tool_button)
{
  if (event->mode == GDK_CROSSING_NORMAL &&
      gtk_widget_get_visible (tool_button->priv->menu))
    {
      ligma_tool_button_update_menu (tool_button);
    }

  return FALSE;
}

static gboolean
ligma_tool_button_menu_timeout (LigmaToolButton *tool_button)
{
  GdkEvent *event = tool_button->priv->menu_timeout_event;

  tool_button->priv->menu_timeout_id    = 0;
  tool_button->priv->menu_timeout_event = NULL;

  ligma_tool_button_show_menu (tool_button, event);

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
ligma_tool_button_update (LigmaToolButton *tool_button)
{
  LigmaToolInfo *tool_info;

  tool_info = ligma_tool_button_get_tool_info (tool_button);

  gtk_tool_button_set_icon_name (
    GTK_TOOL_BUTTON (tool_button),
    tool_info ? ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool_info)) :
                NULL);

  g_clear_object (&tool_button->priv->tooltip_widget);

  if (tool_info)
    {
      ligma_help_set_help_data (GTK_WIDGET (tool_button),
                               tool_info->tooltip, tool_info->help_id);
    }
  else
    {
      ligma_help_set_help_data (GTK_WIDGET (tool_button), NULL, NULL);
    }

  if (tool_info)
    {
      const gchar *tool_name = ligma_object_get_name (tool_info);
      gchar       *identifier;

      if (g_str_has_prefix (tool_name, "ligma-") &&
          g_str_has_suffix (tool_name, "-tool"))
        {
          /* The LigmaToolInfo names are of the form "ligma-pencil-tool",
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

      ligma_widget_set_identifier (GTK_WIDGET (tool_button), identifier);
      g_free (identifier);
    }

  ligma_tool_button_update_toggled (tool_button);
  ligma_tool_button_update_menu    (tool_button);
}

static void
ligma_tool_button_update_toggled (LigmaToolButton *tool_button)
{
  LigmaContext  *context;
  LigmaToolInfo *tool_info;

  context = ligma_toolbox_get_context (tool_button->priv->toolbox);

  tool_info = ligma_tool_button_get_tool_info (tool_button);

  gtk_toggle_tool_button_set_active (
    GTK_TOGGLE_TOOL_BUTTON (tool_button),
    tool_info && tool_info == ligma_context_get_tool (context));
}

static void
ligma_tool_button_update_menu (LigmaToolButton *tool_button)
{
  if (tool_button->priv->menu &&
      gtk_widget_get_visible (tool_button->priv->menu))
    {
      LigmaToolInfo *tool_info = ligma_tool_button_get_tool_info (tool_button);

      if (tool_info)
        {
          gtk_menu_shell_select_item (
            GTK_MENU_SHELL (tool_button->priv->menu),
            g_hash_table_lookup (tool_button->priv->menu_items, tool_info));
        }
    }
}

static void
ligma_tool_button_add_menu_item (LigmaToolButton *tool_button,
                                LigmaToolInfo   *tool_info,
                                gint            index)
{
  LigmaUIManager *ui_manager;
  LigmaAction    *action;
  GtkWidget     *item;
  GtkWidget     *hbox;
  GtkWidget     *image;
  GtkWidget     *label;
  GtkIconSize    icon_size = GTK_ICON_SIZE_MENU;

  ui_manager = ligma_dock_get_ui_manager (
    LIGMA_DOCK (tool_button->priv->toolbox));

  action = ligma_tool_button_get_action (tool_button, tool_info);

  if (tool_button->priv->palette)
    {
      icon_size = gtk_tool_palette_get_icon_size (
        GTK_TOOL_PALETTE (tool_button->priv->palette));
    }

  item = gtk_menu_item_new ();
  gtk_menu_shell_insert (GTK_MENU_SHELL (tool_button->priv->menu), item, index);
  gtk_activatable_set_related_action (GTK_ACTIVATABLE (item),
                                      GTK_ACTION (action));
  ligma_help_set_help_data (item, tool_info->tooltip, tool_info->help_id);

  g_object_bind_property (tool_info, "visible",
                          item,      "visible",
                          G_BINDING_SYNC_CREATE);

  g_object_set_data (G_OBJECT (item), "ligma-tool-info", tool_info);

  ligma_gtk_container_clear (GTK_CONTAINER (item));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add (GTK_CONTAINER (item), hbox);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_icon_name (
    ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool_info)),
    icon_size);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  label = gtk_accel_label_new (tool_info->label);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  if (action)
    {
      gtk_accel_label_set_accel_closure (GTK_ACCEL_LABEL (label),
                                         ligma_action_get_accel_closure (
                                           action));

      if (ui_manager)
        {
          g_signal_emit_by_name (ui_manager, "connect-proxy",
                                 action, item);
        }
    }

  g_hash_table_insert (tool_button->priv->menu_items, tool_info, item);
}

static void
ligma_tool_button_remove_menu_item (LigmaToolButton *tool_button,
                                   LigmaToolInfo   *tool_info)
{
  GtkWidget *item;

  item = g_hash_table_lookup (tool_button->priv->menu_items, tool_info);

  gtk_container_remove (GTK_CONTAINER (tool_button->priv->menu), item);

  g_hash_table_remove (tool_button->priv->menu_items, tool_info);
}

static void
ligma_tool_button_reconstruct_menu_add_menu_item (LigmaToolInfo   *tool_info,
                                                 LigmaToolButton *tool_button)
{
  ligma_tool_button_add_menu_item (tool_button, tool_info, -1);
}

static void
ligma_tool_button_reconstruct_menu (LigmaToolButton *tool_button)
{
  ligma_tool_button_destroy_menu (tool_button);

  if (LIGMA_IS_TOOL_GROUP (tool_button->priv->tool_item))
    {
      LigmaUIManager *ui_manager;
      LigmaContainer *children;

      ui_manager = ligma_dock_get_ui_manager (
        LIGMA_DOCK (tool_button->priv->toolbox));

      children = ligma_viewable_get_children (
        LIGMA_VIEWABLE (tool_button->priv->tool_item));

      tool_button->priv->menu = gtk_menu_new ();
      gtk_menu_attach_to_widget (GTK_MENU (tool_button->priv->menu),
                                 GTK_WIDGET (tool_button), NULL);

      g_signal_connect (tool_button->priv->menu, "leave-notify-event",
                        G_CALLBACK (ligma_tool_button_menu_leave_notify),
                        tool_button);

      if (ui_manager)
        {
          gtk_menu_set_accel_group (
            GTK_MENU (tool_button->priv->menu),
            ligma_ui_manager_get_accel_group (ui_manager));
        }

      tool_button->priv->menu_items = g_hash_table_new (g_direct_hash,
                                                        g_direct_equal);

      ligma_container_foreach (
        children,
        (GFunc) ligma_tool_button_reconstruct_menu_add_menu_item,
        tool_button);
    }
}

static void
ligma_tool_button_destroy_menu (LigmaToolButton *tool_button)
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
ligma_tool_button_show_menu (LigmaToolButton *tool_button,
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

  ligma_tool_button_update_menu (tool_button);

  return TRUE;
}

static LigmaAction *
ligma_tool_button_get_action (LigmaToolButton *tool_button,
                             LigmaToolInfo   *tool_info)
{
  LigmaUIManager *ui_manager;
  LigmaAction    *action = NULL;

  ui_manager = ligma_dock_get_ui_manager (
    LIGMA_DOCK (tool_button->priv->toolbox));

  if (ui_manager && tool_info)
    {
      gchar *name;

      name = ligma_tool_info_get_action_name (tool_info);

      action = ligma_ui_manager_find_action (ui_manager, "tools", name);

      g_free (name);
    }

  return action;
}


/*  public functions  */

GtkToolItem *
ligma_tool_button_new (LigmaToolbox  *toolbox,
                      LigmaToolItem *tool_item)
{
  g_return_val_if_fail (LIGMA_IS_TOOLBOX (toolbox), NULL);
  g_return_val_if_fail (tool_item == NULL ||
                        LIGMA_IS_TOOL_ITEM (tool_item), NULL);

  return g_object_new (LIGMA_TYPE_TOOL_BUTTON,
                       "toolbox",   toolbox,
                       "tool-item", tool_item,
                       NULL);
}

LigmaToolbox *
ligma_tool_button_get_toolbox (LigmaToolButton *tool_button)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_BUTTON (tool_button), NULL);

  return tool_button->priv->toolbox;
}

void
ligma_tool_button_set_tool_item (LigmaToolButton *tool_button,
                                LigmaToolItem   *tool_item)
{
  g_return_if_fail (LIGMA_IS_TOOL_BUTTON (tool_button));
  g_return_if_fail (tool_item == NULL || LIGMA_IS_TOOL_ITEM (tool_item));

  if (tool_item != tool_button->priv->tool_item)
    {
      if (LIGMA_IS_TOOL_GROUP (tool_button->priv->tool_item))
        {
          LigmaContainer *children;

          children = ligma_viewable_get_children (
            LIGMA_VIEWABLE (tool_button->priv->tool_item));

          g_signal_handlers_disconnect_by_func (
            tool_button->priv->tool_item,
            ligma_tool_button_active_tool_changed,
            tool_button);

          g_signal_handlers_disconnect_by_func (
            children,
            ligma_tool_button_tool_add,
            tool_button);
          g_signal_handlers_disconnect_by_func (
            children,
            ligma_tool_button_tool_remove,
            tool_button);
          g_signal_handlers_disconnect_by_func (
            children,
            ligma_tool_button_tool_reorder,
            tool_button);

          ligma_tool_button_destroy_menu (tool_button);
        }

      g_set_object (&tool_button->priv->tool_item, tool_item);

      if (LIGMA_IS_TOOL_GROUP (tool_button->priv->tool_item))
        {
          LigmaContainer *children;

          children = ligma_viewable_get_children (
            LIGMA_VIEWABLE (tool_button->priv->tool_item));

          g_signal_connect (
            tool_button->priv->tool_item, "active-tool-changed",
            G_CALLBACK (ligma_tool_button_active_tool_changed),
            tool_button);

          g_signal_connect (
            children, "add",
            G_CALLBACK (ligma_tool_button_tool_add),
            tool_button);
          g_signal_connect (
            children, "remove",
            G_CALLBACK (ligma_tool_button_tool_remove),
            tool_button);
          g_signal_connect (
            children, "reorder",
            G_CALLBACK (ligma_tool_button_tool_reorder),
            tool_button);

          ligma_tool_button_reconstruct_menu (tool_button);
        }

      ligma_tool_button_update (tool_button);

      g_object_notify (G_OBJECT (tool_button), "tool-item");
    }
}

LigmaToolItem *
ligma_tool_button_get_tool_item (LigmaToolButton *tool_button)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_BUTTON (tool_button), NULL);

  return tool_button->priv->tool_item;
}

LigmaToolInfo *
ligma_tool_button_get_tool_info (LigmaToolButton *tool_button)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_BUTTON (tool_button), NULL);

  if (tool_button->priv->tool_item)
    {
      if (LIGMA_IS_TOOL_INFO (tool_button->priv->tool_item))
        {
          return LIGMA_TOOL_INFO (tool_button->priv->tool_item);
        }
      else
        {
          return ligma_tool_group_get_active_tool_info (
            LIGMA_TOOL_GROUP (tool_button->priv->tool_item));
        }
    }

  return NULL;
}
