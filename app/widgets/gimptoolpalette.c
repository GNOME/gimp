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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "gimpdialogfactory.h"
#include "gimptoolpalette.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define DEFAULT_TOOL_ICON_SIZE GTK_ICON_SIZE_BUTTON
#define DEFAULT_BUTTON_RELIEF  GTK_RELIEF_NONE

#define TOOL_BUTTON_DATA_KEY   "gimp-tool-palette-item"
#define TOOL_INFO_DATA_KEY     "gimp-tool-info"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_UI_MANAGER,
  PROP_DIALOG_FACTORY
};


typedef struct _GimpToolPalettePrivate GimpToolPalettePrivate;

struct _GimpToolPalettePrivate
{
  GimpContext       *context;
  GimpUIManager     *ui_manager;
  GimpDialogFactory *dialog_factory;

  gint               tool_rows;
  gint               tool_columns;
};

#define GET_PRIVATE(p) G_TYPE_INSTANCE_GET_PRIVATE (p, \
                                                    GIMP_TYPE_TOOL_PALETTE, \
                                                    GimpToolPalettePrivate)


static void     gimp_tool_palette_constructed         (GObject         *object);
static void     gimp_tool_palette_dispose             (GObject         *object);
static void     gimp_tool_palette_set_property        (GObject         *object,
                                                       guint            property_id,
                                                       const GValue    *value,
                                                       GParamSpec      *pspec);
static void     gimp_tool_palette_get_property        (GObject         *object,
                                                       guint            property_id,
                                                       GValue          *value,
                                                       GParamSpec      *pspec);
static void     gimp_tool_palette_size_allocate       (GtkWidget       *widget,
                                                       GtkAllocation   *allocation);
static void     gimp_tool_palette_style_set           (GtkWidget       *widget,
                                                       GtkStyle        *previous_style);

static void     gimp_tool_palette_tool_changed        (GimpContext     *context,
                                                       GimpToolInfo    *tool_info,
                                                       GimpToolPalette *palette);
static void     gimp_tool_palette_tool_reorder        (GimpContainer   *container,
                                                       GimpToolInfo    *tool_info,
                                                       gint             index,
                                                       GimpToolPalette *palette);
static void     gimp_tool_palette_tool_visible_notify (GimpToolInfo    *tool_info,
                                                       GParamSpec      *pspec,
                                                       GtkToolItem     *item);
static void     gimp_tool_palette_tool_button_toggled (GtkWidget       *widget,
                                                       GimpToolPalette *palette);
static gboolean gimp_tool_palette_tool_button_press   (GtkWidget       *widget,
                                                       GdkEventButton  *bevent,
                                                       GimpToolPalette *palette);


G_DEFINE_TYPE (GimpToolPalette, gimp_tool_palette, GTK_TYPE_TOOL_PALETTE)

#define parent_class gimp_tool_palette_parent_class


static void
gimp_tool_palette_class_init (GimpToolPaletteClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed           = gimp_tool_palette_constructed;
  object_class->dispose               = gimp_tool_palette_dispose;
  object_class->set_property          = gimp_tool_palette_set_property;
  object_class->get_property          = gimp_tool_palette_get_property;

  widget_class->size_allocate         = gimp_tool_palette_size_allocate;
  widget_class->style_set             = gimp_tool_palette_style_set;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_UI_MANAGER,
                                   g_param_spec_object ("ui-manager",
                                                        NULL, NULL,
                                                        GIMP_TYPE_UI_MANAGER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DIALOG_FACTORY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

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

  g_type_class_add_private (klass, sizeof (GimpToolPalettePrivate));
}

static void
gimp_tool_palette_init (GimpToolPalette *palette)
{
}

static void
gimp_tool_palette_constructed (GObject *object)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (object);
  GtkWidget              *group;
  GimpToolInfo           *active_tool;
  GList                  *list;
  GSList                 *item_group = NULL;

  g_assert (GIMP_IS_CONTEXT (private->context));
  g_assert (GIMP_IS_UI_MANAGER (private->ui_manager));
  g_assert (GIMP_IS_DIALOG_FACTORY (private->dialog_factory));

  group = gtk_tool_item_group_new (_("Tools"));
  gtk_tool_item_group_set_label_widget (GTK_TOOL_ITEM_GROUP (group), NULL);
  gtk_container_add (GTK_CONTAINER (object), group);
  gtk_widget_show (group);

  active_tool = gimp_context_get_tool (private->context);

  for (list = gimp_get_tool_info_iter (private->context->gimp);
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = list->data;
      GtkToolItem  *item;
      const gchar  *stock_id;

      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));

      item = gtk_radio_tool_button_new_from_stock (item_group, stock_id);
      item_group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (item));
      gtk_tool_item_group_insert (GTK_TOOL_ITEM_GROUP (group), item, -1);
      gtk_widget_show (GTK_WIDGET (item));

      gtk_tool_item_set_visible_horizontal (item, tool_info->visible);
      gtk_tool_item_set_visible_vertical   (item, tool_info->visible);

      g_signal_connect_object (tool_info, "notify::visible",
                               G_CALLBACK (gimp_tool_palette_tool_visible_notify),
                               item, 0);

      g_object_set_data (G_OBJECT (tool_info), TOOL_BUTTON_DATA_KEY, item);
      g_object_set_data (G_OBJECT (item)  ,    TOOL_INFO_DATA_KEY,   tool_info);

      if (tool_info == active_tool)
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (item), TRUE);

      g_signal_connect (item, "toggled",
                        G_CALLBACK (gimp_tool_palette_tool_button_toggled),
                        object);

      g_signal_connect (gtk_bin_get_child (GTK_BIN (item)), "button-press-event",
                        G_CALLBACK (gimp_tool_palette_tool_button_press),
                        object);

      if (private->ui_manager)
        {
          GtkAction   *action     = NULL;
          const gchar *identifier = NULL;
          gchar       *tmp        = NULL;
          gchar       *name       = NULL;

          identifier = gimp_object_get_name (tool_info);

          tmp = g_strndup (identifier + strlen ("gimp-"),
                           strlen (identifier) - strlen ("gimp--tool"));
          name = g_strdup_printf ("tools-%s", tmp);
          g_free (tmp);

          action = gimp_ui_manager_find_action (private->ui_manager,
                                                "tools", name);
          g_free (name);

          if (action)
            gimp_widget_set_accel_help (GTK_WIDGET (item), action);
          else
            gimp_help_set_help_data (GTK_WIDGET (item),
                                     tool_info->help, tool_info->help_id);
        }
    }

  g_signal_connect_object (private->context->gimp->tool_info_list, "reorder",
                           G_CALLBACK (gimp_tool_palette_tool_reorder),
                           object, 0);

  g_signal_connect_object (private->context, "tool-changed",
                           G_CALLBACK (gimp_tool_palette_tool_changed),
                           object,
                           0);
}

static void
gimp_tool_palette_dispose (GObject *object)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (object);

  if (private->context)
    {
      g_object_unref (private->context);
      private->context = NULL;
    }

  if (private->ui_manager)
    {
      g_object_unref (private->ui_manager);
      private->ui_manager = NULL;
    }

  if (private->dialog_factory)
    {
      g_object_unref (private->dialog_factory);
      private->dialog_factory = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tool_palette_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      private->context = g_value_dup_object (value);
      break;

    case PROP_UI_MANAGER:
      private->ui_manager = g_value_dup_object (value);
      break;

    case PROP_DIALOG_FACTORY:
      private->dialog_factory = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_palette_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, private->context);
      break;

    case PROP_UI_MANAGER:
      g_value_set_object (value, private->ui_manager);
      break;

    case PROP_DIALOG_FACTORY:
      g_value_set_object (value, private->ui_manager);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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
      Gimp  *gimp = private->context->gimp;
      GList *list;
      gint   n_tools;
      gint   tool_rows;
      gint   tool_columns;

      for (list = gimp_get_tool_info_iter (gimp), n_tools = 0;
           list;
           list = list->next)
        {
          GimpToolInfo *tool_info = list->data;

          if (tool_info->visible)
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
  GtkIconSize             tool_icon_size;
  GtkReliefStyle          relief;
  GList                  *list;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);

  if (! private->context)
    return;

  gimp = private->context->gimp;

  gtk_widget_style_get (widget,
                        "tool-icon-size", &tool_icon_size,
                        "button-relief",  &relief,
                        NULL);

  gtk_tool_palette_set_icon_size (GTK_TOOL_PALETTE (widget), tool_icon_size);

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
}

GtkWidget *
gimp_tool_palette_new (GimpContext       *context,
                       GimpUIManager     *ui_manager,
                       GimpDialogFactory *dialog_factory)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (ui_manager), NULL);
  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);

  return g_object_new (GIMP_TYPE_TOOL_PALETTE,
                       "context",        context,
                       "ui-manager",     ui_manager,
                       "dialog-factory", dialog_factory,
                       NULL);
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

  tool_info   = gimp_get_tool_info (private->context->gimp,
                                    "gimp-rect-select-tool");
  tool_button = g_object_get_data (G_OBJECT (tool_info), TOOL_BUTTON_DATA_KEY);

  if (tool_button)
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
gimp_tool_palette_tool_visible_notify (GimpToolInfo *tool_info,
                                       GParamSpec   *pspec,
                                       GtkToolItem  *item)
{
  gtk_tool_item_set_visible_horizontal (item, tool_info->visible);
  gtk_tool_item_set_visible_vertical   (item, tool_info->visible);
}

static void
gimp_tool_palette_tool_button_toggled (GtkWidget       *widget,
                                       GimpToolPalette *palette)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (palette);
  GimpToolInfo           *tool_info;

  tool_info = g_object_get_data (G_OBJECT (widget), TOOL_INFO_DATA_KEY);

  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
    gimp_context_set_tool (private->context, tool_info);
}

static gboolean
gimp_tool_palette_tool_button_press (GtkWidget       *widget,
                                     GdkEventButton  *event,
                                     GimpToolPalette *palette)
{
  GimpToolPalettePrivate *private = GET_PRIVATE (palette);

  if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      gimp_dialog_factory_dialog_raise (private->dialog_factory,
                                        gtk_widget_get_screen (widget),
                                        "gimp-tool-options",
                                        -1);
    }

  return FALSE;
}
