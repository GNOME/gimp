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
#include "core/gimpcontainer.h"
#include "core/gimptoolitem.h"

#include "gimptoolbox.h"
#include "gimptoolbutton.h"
#include "gimptoolpalette.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"
#include "gimpwindowstrategy.h"

#include "gimp-intl.h"


#define DEFAULT_TOOL_ICON_SIZE GTK_ICON_SIZE_BUTTON
#define DEFAULT_BUTTON_RELIEF  GTK_RELIEF_NONE


typedef struct _GimpToolPalettePrivate GimpToolPalettePrivate;

struct _GimpToolPalettePrivate
{
  GimpToolbox *toolbox;

  GtkWidget   *group;
  GHashTable  *buttons;

  gint         tool_rows;
  gint         tool_columns;
};

#define GET_PRIVATE(p) ((GimpToolPalettePrivate *) gimp_tool_palette_get_instance_private ((GimpToolPalette *) (p)))


static void   gimp_tool_palette_finalize            (GObject        *object);

static void   gimp_tool_palette_size_allocate       (GtkWidget       *widget,
                                                     GtkAllocation   *allocation);
static void   gimp_tool_palette_style_set           (GtkWidget       *widget,
                                                     GtkStyle        *previous_style);

static void   gimp_tool_palette_tool_add            (GimpContainer   *container,
                                                     GimpToolItem    *tool_item,
                                                     GimpToolPalette *palette);
static void   gimp_tool_palette_tool_remove         (GimpContainer   *container,
                                                     GimpToolItem    *tool_item,
                                                     GimpToolPalette *palette);
static void   gimp_tool_palette_tool_reorder        (GimpContainer   *container,
                                                     GimpToolItem    *tool_item,
                                                     gint             index,
                                                     GimpToolPalette *palette);

static void   gimp_tool_palette_config_size_changed (GimpGuiConfig   *config,
                                                     GimpToolPalette *palette);

static void   gimp_tool_palette_add_button          (GimpToolPalette *palette,
                                                     GimpToolItem    *tool_item,
                                                     gint             index);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolPalette, gimp_tool_palette,
                            GTK_TYPE_TOOL_PALETTE)

#define parent_class gimp_tool_palette_parent_class


static void
gimp_tool_palette_class_init (GimpToolPaletteClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize      = gimp_tool_palette_finalize;

  widget_class->size_allocate = gimp_tool_palette_size_allocate;
  widget_class->style_set     = gimp_tool_palette_style_set;

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
  GimpToolPalettePrivate *private = GET_PRIVATE (palette);

  private->buttons = g_hash_table_new (g_direct_hash, g_direct_equal);

  gtk_tool_palette_set_style (GTK_TOOL_PALETTE (palette), GTK_TOOLBAR_ICONS);
}

static void
gimp_tool_palette_finalize (GObject *object)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->buttons, g_hash_table_unref);

  if (private->toolbox)
    {
      GimpContext *context = gimp_toolbox_get_context (private->toolbox);

      if (context)
        g_signal_handlers_disconnect_by_func (context->gimp->config,
                                              G_CALLBACK (gimp_tool_palette_config_size_changed),
                                              object);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tool_palette_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (widget);
  gint                    button_width;
  gint                    button_height;

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (gimp_tool_palette_get_button_size (GIMP_TOOL_PALETTE (widget),
                                         &button_width, &button_height))
    {
      GimpToolItem   *tool_item;
      GHashTableIter  iter;
      gint            n_tools;
      gint            tool_rows;
      gint            tool_columns;

      n_tools = 0;

      g_hash_table_iter_init (&iter, private->buttons);

      while (g_hash_table_iter_next (&iter, (gpointer *) &tool_item, NULL))
        {
          if (gimp_tool_item_get_visible (tool_item))
            n_tools++;
        }

      tool_columns = MAX (1, (allocation->width / button_width));
      tool_rows    = n_tools / tool_columns;

      if (n_tools % tool_columns)
        tool_rows++;

      if (private->tool_rows    != tool_rows  ||
          private->tool_columns != tool_columns)
        {
          private->tool_rows    = tool_rows;
          private->tool_columns = tool_columns;

          gtk_widget_set_size_request (widget, -1,
                                       tool_rows * button_height);
        }
    }
}

static void
gimp_tool_palette_style_set (GtkWidget *widget,
                             GtkStyle  *previous_style)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (widget);
  Gimp                   *gimp;
  GtkWidget              *tool_button;
  GHashTableIter          iter;
  GtkReliefStyle          relief;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);

  if (! gimp_toolbox_get_context (private->toolbox))
    return;

  gimp = gimp_toolbox_get_context (private->toolbox)->gimp;

  gtk_widget_style_get (widget,
                        "button-relief", &relief,
                        NULL);

  gimp_tool_palette_config_size_changed (GIMP_GUI_CONFIG (gimp->config),
                                         GIMP_TOOL_PALETTE (widget));

  g_hash_table_iter_init (&iter, private->buttons);

  while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &tool_button))
    {
      GtkWidget *button = gtk_bin_get_child (GTK_BIN (tool_button));

      gtk_button_set_relief (GTK_BUTTON (button), relief);
    }

  gimp_dock_invalidate_geometry (GIMP_DOCK (private->toolbox));
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
  GList                  *list;

  g_return_if_fail (GIMP_IS_TOOL_PALETTE (palette));
  g_return_if_fail (GIMP_IS_TOOLBOX (toolbox));

  private = GET_PRIVATE (palette);

  if (private->toolbox)
    {
      context = gimp_toolbox_get_context (private->toolbox);
      g_signal_handlers_disconnect_by_func (GIMP_GUI_CONFIG (context->gimp->config),
                                            G_CALLBACK (gimp_tool_palette_config_size_changed),
                                            palette);
    }
  private->toolbox = toolbox;

  context = gimp_toolbox_get_context (toolbox);

  private->group = gtk_tool_item_group_new (_("Tools"));
  gtk_tool_item_group_set_label_widget (GTK_TOOL_ITEM_GROUP (private->group),
                                        NULL);
  gtk_container_add (GTK_CONTAINER (palette), private->group);
  gtk_widget_show (private->group);

  for (list = gimp_get_tool_item_ui_iter (context->gimp);
       list;
       list = g_list_next (list))
    {
      GimpToolItem *tool_item = list->data;

      gimp_tool_palette_add_button (palette, tool_item, -1);
    }

  g_signal_connect_object (context->gimp->tool_item_ui_list, "add",
                           G_CALLBACK (gimp_tool_palette_tool_add),
                           palette, 0);
  g_signal_connect_object (context->gimp->tool_item_ui_list, "remove",
                           G_CALLBACK (gimp_tool_palette_tool_remove),
                           palette, 0);
  g_signal_connect_object (context->gimp->tool_item_ui_list, "reorder",
                           G_CALLBACK (gimp_tool_palette_tool_reorder),
                           palette, 0);

  /* Update the toolbox icon size on config change. */
  g_signal_connect (GIMP_GUI_CONFIG (context->gimp->config),
                    "size-changed",
                    G_CALLBACK (gimp_tool_palette_config_size_changed),
                    palette);
  gimp_tool_palette_config_size_changed (GIMP_GUI_CONFIG (context->gimp->config),
                                         palette);
}

gboolean
gimp_tool_palette_get_button_size (GimpToolPalette *palette,
                                   gint            *width,
                                   gint            *height)
{
  GimpToolPalettePrivate *private;
  GHashTableIter          iter;
  GtkWidget              *tool_button;

  g_return_val_if_fail (GIMP_IS_TOOL_PALETTE (palette), FALSE);
  g_return_val_if_fail (width != NULL, FALSE);
  g_return_val_if_fail (height != NULL, FALSE);

  private = GET_PRIVATE (palette);

  g_hash_table_iter_init (&iter, private->buttons);

  if (g_hash_table_iter_next (&iter, NULL, (gpointer *) &tool_button))
    {
      GtkRequisition button_requisition;

      gtk_widget_size_request (tool_button, &button_requisition);

      *width  = button_requisition.width;
      *height = button_requisition.height;

      return TRUE;
    }

  return FALSE;
}


/*  private functions  */

static void
gimp_tool_palette_tool_add (GimpContainer   *container,
                            GimpToolItem    *tool_item,
                            GimpToolPalette *palette)
{
  gimp_tool_palette_add_button (
    palette,
    tool_item,
    gimp_container_get_child_index (container, GIMP_OBJECT (tool_item)));
}

static void
gimp_tool_palette_tool_remove (GimpContainer   *container,
                               GimpToolItem    *tool_item,
                               GimpToolPalette *palette)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (palette);
  GtkWidget              *tool_button;

  tool_button = g_hash_table_lookup (private->buttons, tool_item);

  if (tool_button)
    {
      g_hash_table_remove (private->buttons, tool_item);

      gtk_container_remove (GTK_CONTAINER (private->group), tool_button);
    }
}

static void
gimp_tool_palette_tool_reorder (GimpContainer   *container,
                                GimpToolItem    *tool_item,
                                gint             index,
                                GimpToolPalette *palette)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (palette);
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
gimp_tool_palette_config_size_changed (GimpGuiConfig   *config,
                                       GimpToolPalette *palette)
{
  GimpIconSize size;
  GtkIconSize  tool_icon_size;

  size = gimp_gui_config_detect_icon_size (config);
  /* Match GimpIconSize with GtkIconSize for the toolbox icons. */
  switch (size)
    {
    case GIMP_ICON_SIZE_SMALL:
      tool_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
      break;
    case GIMP_ICON_SIZE_MEDIUM:
      tool_icon_size = GTK_ICON_SIZE_LARGE_TOOLBAR;
      break;
    case GIMP_ICON_SIZE_LARGE:
      tool_icon_size = GTK_ICON_SIZE_DND;
      break;
    case GIMP_ICON_SIZE_HUGE:
      tool_icon_size = GTK_ICON_SIZE_DIALOG;
      break;
    default:
      /* GIMP_ICON_SIZE_DEFAULT:
       * let's use the size set by the theme. */
      gtk_widget_style_get (GTK_WIDGET (palette),
                            "tool-icon-size", &tool_icon_size,
                            NULL);
      break;
    }

  gtk_tool_palette_set_icon_size (GTK_TOOL_PALETTE (palette), tool_icon_size);
}

static void
gimp_tool_palette_add_button (GimpToolPalette *palette,
                              GimpToolItem    *tool_item,
                              gint             index)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (palette);
  GtkToolItem            *tool_button;
  GtkWidget              *button;
  GtkReliefStyle          relief;

  tool_button = gimp_tool_button_new (private->toolbox, tool_item);
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
