/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolpalette.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmacontainer.h"
#include "core/ligmatoolitem.h"

#include "ligmatoolbox.h"
#include "ligmatoolbutton.h"
#include "ligmatoolpalette.h"
#include "ligmauimanager.h"
#include "ligmawidgets-utils.h"
#include "ligmawindowstrategy.h"

#include "ligma-intl.h"


#define DEFAULT_TOOL_ICON_SIZE GTK_ICON_SIZE_BUTTON
#define DEFAULT_BUTTON_RELIEF  GTK_RELIEF_NONE


typedef struct _LigmaToolPalettePrivate LigmaToolPalettePrivate;

struct _LigmaToolPalettePrivate
{
  LigmaToolbox *toolbox;

  GtkWidget   *group;
  GHashTable  *buttons;
};

#define GET_PRIVATE(p) ((LigmaToolPalettePrivate *) ligma_tool_palette_get_instance_private ((LigmaToolPalette *) (p)))


static void     ligma_tool_palette_finalize            (GObject         *object);

static GtkSizeRequestMode
                ligma_tool_palette_get_request_mode    (GtkWidget       *widget);
static void     ligma_tool_palette_get_preferred_width (GtkWidget       *widget,
                                                       gint            *min_width,
                                                       gint            *pref_width);
static void     ligma_tool_palette_get_preferred_height(GtkWidget       *widget,
                                                       gint            *min_width,
                                                       gint            *pref_width);
static void     ligma_tool_palette_height_for_width    (GtkWidget       *widget,
                                                       gint             width,
                                                       gint            *min_height,
                                                       gint            *pref_height);
static void     ligma_tool_palette_style_updated       (GtkWidget       *widget);

static void     ligma_tool_palette_tool_add            (LigmaContainer   *container,
                                                       LigmaToolItem    *tool_item,
                                                       LigmaToolPalette *palette);
static void     ligma_tool_palette_tool_remove         (LigmaContainer   *container,
                                                       LigmaToolItem    *tool_item,
                                                       LigmaToolPalette *palette);
static void     ligma_tool_palette_tool_reorder        (LigmaContainer   *container,
                                                       LigmaToolItem    *tool_item,
                                                       gint             index,
                                                       LigmaToolPalette *palette);

static void     ligma_tool_palette_add_button          (LigmaToolPalette *palette,
                                                       LigmaToolItem    *tool_item,
                                                       gint             index);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolPalette, ligma_tool_palette,
                            GTK_TYPE_TOOL_PALETTE)

#define parent_class ligma_tool_palette_parent_class


static void
ligma_tool_palette_class_init (LigmaToolPaletteClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize                       = ligma_tool_palette_finalize;

  widget_class->get_request_mode               = ligma_tool_palette_get_request_mode;
  widget_class->get_preferred_width            = ligma_tool_palette_get_preferred_width;
  widget_class->get_preferred_height           = ligma_tool_palette_get_preferred_height;
  widget_class->get_preferred_height_for_width = ligma_tool_palette_height_for_width;
  widget_class->style_updated                  = ligma_tool_palette_style_updated;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("tool-icon-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_TOOL_ICON_SIZE,
                                                              LIGMA_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("button-relief",
                                                              NULL, NULL,
                                                              GTK_TYPE_RELIEF_STYLE,
                                                              DEFAULT_BUTTON_RELIEF,
                                                              LIGMA_PARAM_READABLE));
}

static void
ligma_tool_palette_init (LigmaToolPalette *palette)
{
  LigmaToolPalettePrivate *private = GET_PRIVATE (palette);

  private->buttons = g_hash_table_new (g_direct_hash, g_direct_equal);

  gtk_tool_palette_set_style (GTK_TOOL_PALETTE (palette), GTK_TOOLBAR_ICONS);
}

static void
ligma_tool_palette_finalize (GObject *object)
{
  LigmaToolPalettePrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->buttons, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GtkSizeRequestMode
ligma_tool_palette_get_request_mode (GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static gint
ligma_tool_palette_get_n_tools (LigmaToolPalette *palette,
                               gint            *button_width,
                               gint            *button_height,
                               gint            *min_columns,
                               gint            *min_rows)
{
  LigmaToolPalettePrivate *private = GET_PRIVATE (palette);
  LigmaToolItem           *tool_item;
  GHashTableIter          iter;
  GdkMonitor             *monitor;
  GdkRectangle            workarea;
  gint                    n_tools;
  gint                    max_rows;
  gint                    max_columns;

  if (! ligma_tool_palette_get_button_size (palette,
                                           button_width, button_height))
    {
      /* arbitrary values, just to simplify our callers */
      *button_width  = 24;
      *button_height = 24;
    }

  n_tools = 0;

  g_hash_table_iter_init (&iter, private->buttons);

  while (g_hash_table_iter_next (&iter, (gpointer *) &tool_item, NULL))
    {
      if (ligma_tool_item_get_visible (tool_item))
        n_tools++;
    }

  monitor = ligma_widget_get_monitor (GTK_WIDGET (palette));
  gdk_monitor_get_workarea (monitor, &workarea);

  max_columns = (workarea.width  * 0.9) / *button_width;
  max_rows    = (workarea.height * 0.7) / *button_height;

  *min_columns = MAX (1, n_tools / max_rows);
  *min_rows    = MAX (1, n_tools / max_columns);

  return n_tools;
}

static void
ligma_tool_palette_get_preferred_width (GtkWidget *widget,
                                       gint      *min_width,
                                       gint      *pref_width)
{
  gint button_width;
  gint button_height;
  gint min_columns;
  gint min_rows;

  ligma_tool_palette_get_n_tools (LIGMA_TOOL_PALETTE (widget),
                                 &button_width, &button_height,
                                 &min_columns, &min_rows);

  *min_width  = min_columns * button_width;
  *pref_width = min_columns * button_width;
}

static void
ligma_tool_palette_get_preferred_height (GtkWidget *widget,
                                        gint      *min_height,
                                        gint      *pref_height)
{
  gint button_width;
  gint button_height;
  gint min_columns;
  gint min_rows;

  ligma_tool_palette_get_n_tools (LIGMA_TOOL_PALETTE (widget),
                                 &button_width, &button_height,
                                 &min_columns, &min_rows);

  *min_height  = min_rows * button_height;
  *pref_height = min_rows * button_height;
}

static void
ligma_tool_palette_height_for_width (GtkWidget *widget,
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

  n_tools = ligma_tool_palette_get_n_tools (LIGMA_TOOL_PALETTE (widget),
                                           &button_width, &button_height,
                                           &min_columns, &min_rows);

  tool_columns = MAX (min_columns, width / button_width);
  tool_rows    = n_tools / tool_columns;

  if (n_tools % tool_columns)
    tool_rows++;

  *min_height = *pref_height = tool_rows * button_height;
}

static void
ligma_tool_palette_style_updated (GtkWidget *widget)
{
  LigmaToolPalettePrivate *private = GET_PRIVATE (widget);
  GtkWidget              *tool_button;
  GHashTableIter          iter;
  GtkReliefStyle          relief;
  GtkIconSize             tool_icon_size;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (! ligma_toolbox_get_context (private->toolbox))
    return;

  gtk_widget_style_get (widget,
                        "button-relief",  &relief,
                        "tool-icon-size", &tool_icon_size,
                        NULL);

  gtk_tool_palette_set_icon_size (GTK_TOOL_PALETTE (widget),
                                  tool_icon_size);

  g_hash_table_iter_init (&iter, private->buttons);

  while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &tool_button))
    {
      GtkWidget *button = gtk_bin_get_child (GTK_BIN (tool_button));

      gtk_button_set_relief (GTK_BUTTON (button), relief);
    }

  ligma_dock_invalidate_geometry (LIGMA_DOCK (private->toolbox));
}


/*  public functions  */

GtkWidget *
ligma_tool_palette_new (void)
{
  return g_object_new (LIGMA_TYPE_TOOL_PALETTE, NULL);
}

void
ligma_tool_palette_set_toolbox (LigmaToolPalette *palette,
                               LigmaToolbox     *toolbox)
{
  LigmaToolPalettePrivate *private;
  LigmaContext            *context;
  GList                  *list;

  g_return_if_fail (LIGMA_IS_TOOL_PALETTE (palette));
  g_return_if_fail (LIGMA_IS_TOOLBOX (toolbox));

  private = GET_PRIVATE (palette);

  private->toolbox = toolbox;

  context = ligma_toolbox_get_context (toolbox);

  private->group = gtk_tool_item_group_new (_("Tools"));
  gtk_tool_item_group_set_label_widget (GTK_TOOL_ITEM_GROUP (private->group),
                                        NULL);
  gtk_container_add (GTK_CONTAINER (palette), private->group);
  gtk_widget_show (private->group);

  for (list = ligma_get_tool_item_ui_iter (context->ligma);
       list;
       list = g_list_next (list))
    {
      LigmaToolItem *tool_item = list->data;

      ligma_tool_palette_add_button (palette, tool_item, -1);
    }

  g_signal_connect_object (context->ligma->tool_item_ui_list, "add",
                           G_CALLBACK (ligma_tool_palette_tool_add),
                           palette, 0);
  g_signal_connect_object (context->ligma->tool_item_ui_list, "remove",
                           G_CALLBACK (ligma_tool_palette_tool_remove),
                           palette, 0);
  g_signal_connect_object (context->ligma->tool_item_ui_list, "reorder",
                           G_CALLBACK (ligma_tool_palette_tool_reorder),
                           palette, 0);

  g_signal_connect_object (LIGMA_GUI_CONFIG (context->ligma->config),
                           "notify::theme",
                           G_CALLBACK (ligma_tool_palette_style_updated),
                           palette, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_object (LIGMA_GUI_CONFIG (context->ligma->config),
                           "notify::override-theme-icon-size",
                           G_CALLBACK (ligma_tool_palette_style_updated),
                           palette, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_object (LIGMA_GUI_CONFIG (context->ligma->config),
                           "notify::custom-icon-size",
                           G_CALLBACK (ligma_tool_palette_style_updated),
                           palette, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
}

gboolean
ligma_tool_palette_get_button_size (LigmaToolPalette *palette,
                                   gint            *width,
                                   gint            *height)
{
  LigmaToolPalettePrivate *private;
  GHashTableIter          iter;
  GtkWidget              *tool_button;

  g_return_val_if_fail (LIGMA_IS_TOOL_PALETTE (palette), FALSE);
  g_return_val_if_fail (width != NULL, FALSE);
  g_return_val_if_fail (height != NULL, FALSE);

  private = GET_PRIVATE (palette);

  g_hash_table_iter_init (&iter, private->buttons);

  if (g_hash_table_iter_next (&iter, NULL, (gpointer *) &tool_button))
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
ligma_tool_palette_tool_add (LigmaContainer   *container,
                            LigmaToolItem    *tool_item,
                            LigmaToolPalette *palette)
{
  ligma_tool_palette_add_button (
    palette,
    tool_item,
    ligma_container_get_child_index (container, LIGMA_OBJECT (tool_item)));
}

static void
ligma_tool_palette_tool_remove (LigmaContainer   *container,
                               LigmaToolItem    *tool_item,
                               LigmaToolPalette *palette)
{
  LigmaToolPalettePrivate *private = GET_PRIVATE (palette);
  GtkWidget              *tool_button;

  tool_button = g_hash_table_lookup (private->buttons, tool_item);

  if (tool_button)
    {
      g_hash_table_remove (private->buttons, tool_item);

      gtk_container_remove (GTK_CONTAINER (private->group), tool_button);
    }
}

static void
ligma_tool_palette_tool_reorder (LigmaContainer   *container,
                                LigmaToolItem    *tool_item,
                                gint             index,
                                LigmaToolPalette *palette)
{
  LigmaToolPalettePrivate *private = GET_PRIVATE (palette);
  GtkWidget              *tool_button;

  tool_button = g_hash_table_lookup (private->buttons, tool_item);

  if (tool_button)
    {
      gtk_tool_item_group_set_item_position (
        GTK_TOOL_ITEM_GROUP (private->group),
        GTK_TOOL_ITEM (tool_button), index);
    }
}

static void
ligma_tool_palette_add_button (LigmaToolPalette *palette,
                              LigmaToolItem    *tool_item,
                              gint             index)
{
  LigmaToolPalettePrivate *private = GET_PRIVATE (palette);
  GtkToolItem            *tool_button;
  GtkWidget              *button;
  GtkReliefStyle          relief;

  tool_button = ligma_tool_button_new (private->toolbox, tool_item);
  gtk_tool_item_group_insert (GTK_TOOL_ITEM_GROUP (private->group),
                              tool_button, index);
  gtk_widget_show (GTK_WIDGET (tool_button));

  g_object_bind_property (tool_item,   "shown",
                          tool_button, "visible-horizontal",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (tool_item,   "shown",
                          tool_button, "visible-vertical",
                          G_BINDING_SYNC_CREATE);

  button = gtk_bin_get_child (GTK_BIN (tool_button));

  gtk_widget_style_get (GTK_WIDGET (palette),
                        "button-relief", &relief,
                        NULL);

  gtk_button_set_relief (GTK_BUTTON (button), relief);

  g_hash_table_insert (private->buttons, tool_item, tool_button);
}
