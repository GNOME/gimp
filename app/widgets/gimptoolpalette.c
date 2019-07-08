/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolpalette.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "gimptoolbox.h"
#include "gimptoolpalette.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"
#include "gimpwindowstrategy.h"

#include "gimp-intl.h"


#define DEFAULT_TOOL_ICON_SIZE GTK_ICON_SIZE_BUTTON
#define DEFAULT_BUTTON_RELIEF  GTK_RELIEF_NONE

#define TOOL_BUTTON_DATA_KEY   "gimp-tool-palette-item"
#define TOOL_INFO_DATA_KEY     "gimp-tool-info"


typedef struct _GimpToolPalettePrivate GimpToolPalettePrivate;

struct _GimpToolPalettePrivate
{
  GimpToolbox *toolbox;
};

#define GET_PRIVATE(p) ((GimpToolPalettePrivate *) gimp_tool_palette_get_instance_private ((GimpToolPalette *) (p)))


static GtkSizeRequestMode
                gimp_tool_palette_get_request_mode    (GtkWidget       *widget);
static void     gimp_tool_palette_get_preferred_width (GtkWidget       *widget,
                                                       gint            *min_width,
                                                       gint            *pref_width);
static void     gimp_tool_palette_get_preferred_height(GtkWidget       *widget,
                                                       gint            *min_width,
                                                       gint            *pref_width);
static void     gimp_tool_palette_height_for_width    (GtkWidget       *widget,
                                                       gint             width,
                                                       gint            *min_height,
                                                       gint            *pref_height);
static void     gimp_tool_palette_style_updated       (GtkWidget       *widget);
static void     gimp_tool_palette_hierarchy_changed   (GtkWidget       *widget,
                                                       GtkWidget       *previous_toplevel);

static void     gimp_tool_palette_tool_changed        (GimpContext     *context,
                                                       GimpToolInfo    *tool_info,
                                                       GimpToolPalette *palette);
static void     gimp_tool_palette_tool_reorder        (GimpContainer   *container,
                                                       GimpToolInfo    *tool_info,
                                                       gint             index,
                                                       GimpToolPalette *palette);
static void     gimp_tool_palette_tool_button_toggled (GtkWidget       *widget,
                                                       GimpToolPalette *palette);
static gboolean gimp_tool_palette_tool_button_press   (GtkWidget       *widget,
                                                       GdkEventButton  *bevent,
                                                       GimpToolPalette *palette);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolPalette, gimp_tool_palette,
                            GTK_TYPE_TOOL_PALETTE)

#define parent_class gimp_tool_palette_parent_class


static void
gimp_tool_palette_class_init (GimpToolPaletteClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->get_request_mode               = gimp_tool_palette_get_request_mode;
  widget_class->get_preferred_width            = gimp_tool_palette_get_preferred_width;
  widget_class->get_preferred_height           = gimp_tool_palette_get_preferred_height;
  widget_class->get_preferred_height_for_width = gimp_tool_palette_height_for_width;
  widget_class->style_updated                  = gimp_tool_palette_style_updated;
  widget_class->hierarchy_changed              = gimp_tool_palette_hierarchy_changed;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("tool-icon-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_TOOL_ICON_SIZE,
                                                              GIMP_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("button-relief",
                                                              NULL, NULL,
                                                              GTK_TYPE_RELIEF_STYLE,
                                                              DEFAULT_BUTTON_RELIEF,
                                                              GIMP_PARAM_READABLE));
}

static void
gimp_tool_palette_init (GimpToolPalette *palette)
{
  gtk_tool_palette_set_style (GTK_TOOL_PALETTE (palette), GTK_TOOLBAR_ICONS);
}

static GtkSizeRequestMode
gimp_tool_palette_get_request_mode (GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static gint
gimp_tool_palette_get_n_tools (GimpToolPalette *palette,
                               gint            *button_width,
                               gint            *button_height,
                               gint            *min_columns,
                               gint            *min_rows)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (palette);
  Gimp                   *gimp;
  GList                  *list;
  GdkMonitor             *monitor;
  GdkRectangle            workarea;
  gint                    n_tools;
  gint                    max_rows;
  gint                    max_columns;

  if (! gimp_tool_palette_get_button_size (palette,
                                           button_width, button_height))
    {
      /* arbitrary values, just to simplify our callers */
      *button_width  = 24;
      *button_height = 24;
    }

  gimp = gimp_toolbox_get_context (private->toolbox)->gimp;

  for (list = gimp_get_tool_info_iter (gimp), n_tools = 0;
       list;
       list = list->next)
    {
      GimpToolInfo *tool_info = list->data;

      if (tool_info->visible)
        n_tools++;
    }

  monitor = gimp_widget_get_monitor (GTK_WIDGET (palette));
  gdk_monitor_get_workarea (monitor, &workarea);

  max_columns = (workarea.width  * 0.9) / *button_width;
  max_rows    = (workarea.height * 0.7) / *button_height;

  *min_columns = MAX (2, n_tools / max_rows);
  *min_rows    = MAX (1, n_tools / max_columns);

  return n_tools;
}

static void
gimp_tool_palette_get_preferred_width (GtkWidget *widget,
                                       gint      *min_width,
                                       gint      *pref_width)
{
  gint button_width;
  gint button_height;
  gint min_columns;
  gint min_rows;

  gimp_tool_palette_get_n_tools (GIMP_TOOL_PALETTE (widget),
                                 &button_width, &button_height,
                                 &min_columns, &min_rows);

  *min_width  = min_columns * button_width;
  *pref_width = min_columns * button_width;
}

static void
gimp_tool_palette_get_preferred_height (GtkWidget *widget,
                                        gint      *min_height,
                                        gint      *pref_height)
{
  gint button_width;
  gint button_height;
  gint min_columns;
  gint min_rows;

  gimp_tool_palette_get_n_tools (GIMP_TOOL_PALETTE (widget),
                                 &button_width, &button_height,
                                 &min_columns, &min_rows);

  *min_height  = min_rows * button_height;
  *pref_height = min_rows * button_height;
}

static void
gimp_tool_palette_height_for_width (GtkWidget *widget,
                                    gint       width,
                                    gint      *min_height,
                                    gint      *pref_height)
{
  gint n_tools;
  gint button_width;
  gint button_height;
  gint min_columns;
  gint min_rows;
  gint tool_columns;
  gint tool_rows;

  n_tools = gimp_tool_palette_get_n_tools (GIMP_TOOL_PALETTE (widget),
                                           &button_width, &button_height,
                                           &min_columns, &min_rows);

  tool_columns = MAX (min_columns, width / button_width);
  tool_rows    = n_tools / tool_columns;

  if (n_tools % tool_columns)
    tool_rows++;

  *min_height = *pref_height = tool_rows * button_height;
}

static void
gimp_tool_palette_style_updated (GtkWidget *widget)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (widget);
  Gimp                   *gimp;
  GtkReliefStyle          relief;
  GList                  *list;
  GtkIconSize             tool_icon_size;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (! gimp_toolbox_get_context (private->toolbox))
    return;

  gimp = gimp_toolbox_get_context (private->toolbox)->gimp;

  gtk_widget_style_get (widget,
                        "button-relief",  &relief,
                        "tool-icon-size", &tool_icon_size,
                        NULL);

  gtk_tool_palette_set_icon_size (GTK_TOOL_PALETTE (widget),
                                  tool_icon_size);

  for (list = gimp_get_tool_info_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = list->data;
      GtkWidget    *tool_button;

      tool_button = g_object_get_data (G_OBJECT (tool_info),
                                       TOOL_BUTTON_DATA_KEY);

      if (tool_button)
        {
          GtkWidget *button = gtk_bin_get_child (GTK_BIN (tool_button));

          gtk_button_set_relief (GTK_BUTTON (button), relief);
        }
    }

  gimp_dock_invalidate_geometry (GIMP_DOCK (private->toolbox));
}

static void
gimp_tool_palette_hierarchy_changed (GtkWidget *widget,
                                     GtkWidget *previous_toplevel)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (widget);
  GimpUIManager          *ui_manager;

  ui_manager = gimp_dock_get_ui_manager (GIMP_DOCK (private->toolbox));

  if (ui_manager)
    {
      GimpContext *context = gimp_toolbox_get_context (private->toolbox);
      GList       *list;

      for (list = gimp_get_tool_info_iter (context->gimp);
           list;
           list = g_list_next (list))
        {
          GimpToolInfo  *tool_info = list->data;
          GtkToolItem   *item;
          GimpAction    *action;
          const gchar   *identifier;
          gchar         *tmp;
          gchar         *name;

          item = g_object_get_data (G_OBJECT (tool_info), TOOL_BUTTON_DATA_KEY);

          identifier = gimp_object_get_name (tool_info);

          tmp = g_strndup (identifier + strlen ("gimp-"),
                           strlen (identifier) - strlen ("gimp--tool"));
          name = g_strdup_printf ("tools-%s", tmp);
          g_free (tmp);

          action = gimp_ui_manager_find_action (ui_manager, "tools", name);
          g_free (name);

          if (action)
            gimp_widget_set_accel_help (GTK_WIDGET (item), action);
          else
            gimp_help_set_help_data (GTK_WIDGET (item),
                                     tool_info->tooltip, tool_info->help_id);

          /* Make sure the toolbox buttons won't grab focus, which has
           * nearly no practical use, and prevents various actions until
           * you click back in canvas.
           */
          gtk_widget_set_can_focus (gtk_bin_get_child (GTK_BIN (item)),
                                    FALSE);
        }
    }
}


/*  public functions  */

GtkWidget *
gimp_tool_palette_new (void)
{
  return g_object_new (GIMP_TYPE_TOOL_PALETTE, NULL);
}

void
gimp_tool_palette_set_toolbox (GimpToolPalette *palette,
                               GimpToolbox     *toolbox)
{
  GimpToolPalettePrivate *private;
  GimpContext            *context;
  GtkWidget              *group;
  GSList                 *item_group = NULL;
  GList                  *list;

  g_return_if_fail (GIMP_IS_TOOL_PALETTE (palette));
  g_return_if_fail (GIMP_IS_TOOLBOX (toolbox));

  private = GET_PRIVATE (palette);

  private->toolbox = toolbox;

  context = gimp_toolbox_get_context (toolbox);

  group = gtk_tool_item_group_new (_("Tools"));
  gtk_tool_item_group_set_label_widget (GTK_TOOL_ITEM_GROUP (group), NULL);
  gtk_container_add (GTK_CONTAINER (palette), group);
  gtk_widget_show (group);

  for (list = gimp_get_tool_info_iter (context->gimp);
       list;
       list = g_list_next (list))
    {
      GimpToolInfo  *tool_info = list->data;
      GtkToolItem   *item;
      const gchar   *icon_name;

      icon_name = gimp_viewable_get_icon_name (GIMP_VIEWABLE (tool_info));

      item = gtk_radio_tool_button_new (item_group);
      gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (item), icon_name);
      item_group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (item));
      gtk_tool_item_group_insert (GTK_TOOL_ITEM_GROUP (group), item, -1);
      gtk_widget_show (GTK_WIDGET (item));

      g_object_bind_property (tool_info, "visible",
                              item,      "visible-horizontal",
                              G_BINDING_SYNC_CREATE);
      g_object_bind_property (tool_info, "visible",
                              item,      "visible-vertical",
                              G_BINDING_SYNC_CREATE);

      g_object_set_data (G_OBJECT (tool_info), TOOL_BUTTON_DATA_KEY, item);
      g_object_set_data (G_OBJECT (item)  ,    TOOL_INFO_DATA_KEY,   tool_info);

      g_signal_connect (item, "toggled",
                        G_CALLBACK (gimp_tool_palette_tool_button_toggled),
                        palette);

      g_signal_connect (gtk_bin_get_child (GTK_BIN (item)), "button-press-event",
                        G_CALLBACK (gimp_tool_palette_tool_button_press),
                        palette);
    }

  g_signal_connect_object (context->gimp->tool_info_list, "reorder",
                           G_CALLBACK (gimp_tool_palette_tool_reorder),
                           palette, 0);

  g_signal_connect_object (context, "tool-changed",
                           G_CALLBACK (gimp_tool_palette_tool_changed),
                           palette,
                           0);
  gimp_tool_palette_tool_changed (context,
                                  gimp_context_get_tool (context),
                                  palette);
}

gboolean
gimp_tool_palette_get_button_size (GimpToolPalette *palette,
                                   gint            *width,
                                   gint            *height)
{
  GimpToolPalettePrivate *private;
  GimpToolInfo           *tool_info;
  GtkWidget              *tool_button;

  g_return_val_if_fail (GIMP_IS_TOOL_PALETTE (palette), FALSE);
  g_return_val_if_fail (width != NULL, FALSE);
  g_return_val_if_fail (height != NULL, FALSE);

  private = GET_PRIVATE (palette);

  tool_info   = gimp_get_tool_info (gimp_toolbox_get_context (private->toolbox)->gimp,
                                    "gimp-rect-select-tool");
  tool_button = g_object_get_data (G_OBJECT (tool_info), TOOL_BUTTON_DATA_KEY);

  if (tool_button)
    {
      GtkRequisition button_requisition;

      gtk_widget_get_preferred_size (tool_button, &button_requisition, NULL);

      *width  = button_requisition.width;
      *height = button_requisition.height;

      return TRUE;
    }

  return FALSE;
}


/*  private functions  */

static void
gimp_tool_palette_tool_changed (GimpContext      *context,
                                GimpToolInfo     *tool_info,
                                GimpToolPalette  *palette)
{
  if (tool_info)
    {
      GtkWidget *tool_button = g_object_get_data (G_OBJECT (tool_info),
                                                  TOOL_BUTTON_DATA_KEY);

      if (tool_button &&
          ! gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (tool_button)))
        {
          g_signal_handlers_block_by_func (tool_button,
                                           gimp_tool_palette_tool_button_toggled,
                                           palette);

          gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (tool_button),
                                             TRUE);

          g_signal_handlers_unblock_by_func (tool_button,
                                             gimp_tool_palette_tool_button_toggled,
                                             palette);
        }
    }
}

static void
gimp_tool_palette_tool_reorder (GimpContainer   *container,
                                GimpToolInfo    *tool_info,
                                gint             index,
                                GimpToolPalette *palette)
{
  if (tool_info)
    {
      GtkWidget *button = g_object_get_data (G_OBJECT (tool_info),
                                             TOOL_BUTTON_DATA_KEY);
      GtkWidget *group  = gtk_widget_get_parent (button);

      gtk_tool_item_group_set_item_position (GTK_TOOL_ITEM_GROUP (group),
                                             GTK_TOOL_ITEM (button), index);
    }
}

static void
gimp_tool_palette_tool_button_toggled (GtkWidget       *widget,
                                       GimpToolPalette *palette)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (palette);
  GimpToolInfo           *tool_info;

  tool_info = g_object_get_data (G_OBJECT (widget), TOOL_INFO_DATA_KEY);

  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
    gimp_context_set_tool (gimp_toolbox_get_context (private->toolbox), tool_info);
}

static gboolean
gimp_tool_palette_tool_button_press (GtkWidget       *widget,
                                     GdkEventButton  *event,
                                     GimpToolPalette *palette)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (palette);

  if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      GimpContext *context = gimp_toolbox_get_context (private->toolbox);
      GimpDock    *dock    = GIMP_DOCK (private->toolbox);

      gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (context->gimp)),
                                                 context->gimp,
                                                 gimp_dock_get_dialog_factory (dock),
                                                 gimp_widget_get_monitor (widget),
                                                 "gimp-tool-options");
    }

  return FALSE;
}
