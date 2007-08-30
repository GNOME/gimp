/* GIMP - The GNU Image Manipulation Program
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "gimpdevices.h"
#include "gimpdialogfactory.h"
#include "gimpdockseparator.h"
#include "gimphelp-ids.h"
#include "gimptoolbox.h"
#include "gimptoolbox-color-area.h"
#include "gimptoolbox-dnd.h"
#include "gimptoolbox-image-area.h"
#include "gimptoolbox-indicator-area.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"
#include "gtkhwrapbox.h"

#include "about.h"

#include "gimp-intl.h"


#define DEFAULT_TOOL_ICON_SIZE GTK_ICON_SIZE_BUTTON
#define DEFAULT_BUTTON_RELIEF  GTK_RELIEF_NONE

#define TOOL_BUTTON_DATA_KEY   "gimp-tool-button"
#define TOOL_INFO_DATA_KEY     "gimp-tool-info"


/*  local function prototypes  */

static GObject   * gimp_toolbox_constructor      (GType           type,
                                                  guint           n_params,
                                                  GObjectConstructParam *params);

static gboolean    gimp_toolbox_delete_event     (GtkWidget      *widget,
                                                  GdkEventAny    *event);
static void        gimp_toolbox_size_allocate    (GtkWidget      *widget,
                                                  GtkAllocation  *allocation);
static void        gimp_toolbox_style_set        (GtkWidget      *widget,
                                                  GtkStyle       *previous_style);
static void        gimp_toolbox_book_added       (GimpDock       *dock,
                                                  GimpDockbook   *dockbook);
static void        gimp_toolbox_book_removed     (GimpDock       *dock,
                                                  GimpDockbook   *dockbook);
static void        gimp_toolbox_set_geometry     (GimpToolbox    *toolbox);

static void        toolbox_separator_expand      (GimpToolbox    *toolbox);
static void        toolbox_separator_collapse    (GimpToolbox    *toolbox);
static void        toolbox_create_tools          (GimpToolbox    *toolbox,
                                                  GimpContext    *context);
static GtkWidget * toolbox_create_color_area     (GimpToolbox    *toolbox,
                                                  GimpContext    *context);
static GtkWidget * toolbox_create_foo_area       (GimpToolbox    *toolbox,
                                                  GimpContext    *context);
static GtkWidget * toolbox_create_image_area     (GimpToolbox    *toolbox,
                                                  GimpContext    *context);

static void        toolbox_area_notify           (GimpGuiConfig  *config,
                                                  GParamSpec     *pspec,
                                                  GtkWidget      *area);
static void        toolbox_tool_changed          (GimpContext    *context,
                                                  GimpToolInfo   *tool_info,
                                                  gpointer        data);

static void        toolbox_tool_reorder          (GimpContainer  *container,
                                                  GimpToolInfo   *tool_info,
                                                  gint            index,
                                                  GtkWidget      *wrap_box);
static void        toolbox_tool_visible_notify   (GimpToolInfo   *tool_info,
                                                  GParamSpec     *pspec,
                                                  GtkWidget      *button);

static void        toolbox_tool_button_toggled   (GtkWidget      *widget,
                                                  GimpToolInfo   *tool_info);
static gboolean    toolbox_tool_button_press     (GtkWidget      *widget,
                                                  GdkEventButton *bevent,
                                                  GimpToolbox    *toolbox);

static gboolean    toolbox_check_device          (GtkWidget      *widget,
                                                  GdkEvent       *event,
                                                  Gimp           *gimp);

static void        toolbox_paste_received        (GtkClipboard   *clipboard,
                                                  const gchar    *text,
                                                  gpointer        data);


G_DEFINE_TYPE (GimpToolbox, gimp_toolbox, GIMP_TYPE_IMAGE_DOCK)

#define parent_class gimp_toolbox_parent_class


static void
gimp_toolbox_class_init (GimpToolboxClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GtkWidgetClass     *widget_class     = GTK_WIDGET_CLASS (klass);
  GimpDockClass      *dock_class       = GIMP_DOCK_CLASS (klass);
  GimpImageDockClass *image_dock_class = GIMP_IMAGE_DOCK_CLASS (klass);

  object_class->constructor         = gimp_toolbox_constructor;

  widget_class->delete_event        = gimp_toolbox_delete_event;
  widget_class->size_allocate       = gimp_toolbox_size_allocate;
  widget_class->style_set           = gimp_toolbox_style_set;

  dock_class->book_added            = gimp_toolbox_book_added;
  dock_class->book_removed          = gimp_toolbox_book_removed;

  image_dock_class->ui_manager_name = "<Toolbox>";

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
gimp_toolbox_init (GimpToolbox *toolbox)
{
  gtk_window_set_role (GTK_WINDOW (toolbox), "gimp-toolbox");

  gimp_help_connect (GTK_WIDGET (toolbox), gimp_standard_help_func,
                     GIMP_HELP_TOOLBOX, NULL);
}

static GObject *
gimp_toolbox_constructor (GType                  type,
                          guint                  n_params,
                          GObjectConstructParam *params)
{
  GObject       *object;
  GimpToolbox   *toolbox;
  GimpContext   *context;
  GimpGuiConfig *config;
  GtkUIManager  *manager;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GdkDisplay    *display;
  GList         *list;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  toolbox = GIMP_TOOLBOX (object);

  context = GIMP_DOCK (toolbox)->context;
  config  = GIMP_GUI_CONFIG (context->gimp->config);

  gimp_window_set_hint (GTK_WINDOW (toolbox), config->toolbox_window_hint);

  main_vbox = GIMP_DOCK (toolbox)->main_vbox;

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_vbox), vbox, 0);
  gtk_widget_show (vbox);

  manager = GTK_UI_MANAGER (GIMP_IMAGE_DOCK (toolbox)->ui_manager);

#ifndef HAVE_CARBON
  toolbox->menu_bar = gtk_ui_manager_get_widget (manager, "/toolbox-menubar");
#endif /* !HAVE_CARBON */

  if (toolbox->menu_bar)
    {
      gtk_box_pack_start (GTK_BOX (vbox), toolbox->menu_bar, FALSE, FALSE, 0);
      gtk_widget_show (toolbox->menu_bar);
    }

  toolbox->tool_wbox = gtk_hwrap_box_new (FALSE);
  gtk_wrap_box_set_justify (GTK_WRAP_BOX (toolbox->tool_wbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify (GTK_WRAP_BOX (toolbox->tool_wbox),
                                 GTK_JUSTIFY_LEFT);
  gtk_wrap_box_set_aspect_ratio (GTK_WRAP_BOX (toolbox->tool_wbox), 5.0 / 6.0);

  gtk_box_pack_start (GTK_BOX (vbox), toolbox->tool_wbox, FALSE, FALSE, 0);
  gtk_widget_show (toolbox->tool_wbox);

  toolbox->area_wbox = gtk_hwrap_box_new (FALSE);
  gtk_wrap_box_set_justify (GTK_WRAP_BOX (toolbox->area_wbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify (GTK_WRAP_BOX (toolbox->area_wbox),
                                 GTK_JUSTIFY_LEFT);
  gtk_wrap_box_set_aspect_ratio (GTK_WRAP_BOX (toolbox->area_wbox), 5.0 / 6.0);

  gtk_box_pack_start (GTK_BOX (vbox), toolbox->area_wbox, FALSE, FALSE, 0);
  gtk_widget_show (toolbox->area_wbox);

  /* We need to know when the current device changes, so we can update
   * the correct tool - to do this we connect to motion events.
   * We can't just use EXTENSION_EVENTS_CURSOR though, since that
   * would get us extension events for the mouse pointer, and our
   * device would change to that and not change back. So we check
   * manually that all devices have a cursor, before establishing the check.
   */
  display = gtk_widget_get_display (GTK_WIDGET (toolbox));
  for (list = gdk_display_list_devices (display); list; list = list->next)
    if (! ((GdkDevice *) (list->data))->has_cursor)
      break;

  if (! list)  /* all devices have cursor */
    {
      g_signal_connect (toolbox, "motion-notify-event",
                        G_CALLBACK (toolbox_check_device),
                        context->gimp);

      gtk_widget_add_events (GTK_WIDGET (toolbox), GDK_POINTER_MOTION_MASK);
      gtk_widget_set_extension_events (GTK_WIDGET (toolbox),
                                       GDK_EXTENSION_EVENTS_CURSOR);
    }

  toolbox_create_tools (toolbox, context);

  toolbox->color_area = toolbox_create_color_area (toolbox, context);
  gtk_wrap_box_pack_wrapped (GTK_WRAP_BOX (toolbox->area_wbox),
                             toolbox->color_area,
                             TRUE, TRUE, FALSE, TRUE, TRUE);
  if (config->toolbox_color_area)
    gtk_widget_show (toolbox->color_area);

  g_signal_connect_object (config, "notify::toolbox-color-area",
                           G_CALLBACK (toolbox_area_notify),
                           toolbox->color_area, 0);

  toolbox->foo_area = toolbox_create_foo_area (toolbox, context);
  gtk_wrap_box_pack (GTK_WRAP_BOX (toolbox->area_wbox), toolbox->foo_area,
                     TRUE, TRUE, FALSE, TRUE);
  if (config->toolbox_foo_area)
    gtk_widget_show (toolbox->foo_area);

  g_signal_connect_object (config, "notify::toolbox-foo-area",
                           G_CALLBACK (toolbox_area_notify),
                           toolbox->foo_area, 0);

  toolbox->image_area = toolbox_create_image_area (toolbox, context);
  gtk_wrap_box_pack (GTK_WRAP_BOX (toolbox->area_wbox), toolbox->image_area,
                     TRUE, TRUE, FALSE, TRUE);
  if (config->toolbox_image_area)
    gtk_widget_show (toolbox->image_area);

  g_signal_connect_object (config, "notify::toolbox-image-area",
                           G_CALLBACK (toolbox_area_notify),
                           toolbox->image_area, 0);

  g_signal_connect_object (context, "tool-changed",
                           G_CALLBACK (toolbox_tool_changed),
                           toolbox->tool_wbox,
                           0);

  gimp_toolbox_dnd_init (GIMP_TOOLBOX (toolbox));

  gimp_toolbox_style_set (GTK_WIDGET (toolbox), GTK_WIDGET (toolbox)->style);

  toolbox_separator_expand (toolbox);

  return object;
}

static gboolean
gimp_toolbox_delete_event (GtkWidget   *widget,
                           GdkEventAny *event)
{
  /* Activate the action instead of simply calling gimp_exit(),
   * so that the quit action's sensitivity is taken into account.
   */
  gimp_ui_manager_activate_action (GIMP_IMAGE_DOCK (widget)->ui_manager,
                                   "file", "file-quit");

  return TRUE;
}

static void
gimp_toolbox_size_allocate (GtkWidget     *widget,
                            GtkAllocation *allocation)
{
  GimpToolbox   *toolbox = GIMP_TOOLBOX (widget);
  Gimp          *gimp;
  GimpGuiConfig *config;
  GimpToolInfo  *tool_info;
  GtkWidget     *tool_button;

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (! GIMP_DOCK (widget)->context)
    return;

  gimp = GIMP_DOCK (widget)->context->gimp;

  config = GIMP_GUI_CONFIG (gimp->config);

  tool_info = gimp_get_tool_info (gimp, "gimp-rect-select-tool");
  tool_button = g_object_get_data (G_OBJECT (tool_info), TOOL_BUTTON_DATA_KEY);

  if (tool_button)
    {
      GtkRequisition  button_requisition;
      GList          *list;
      gint            n_tools;
      gint            tool_rows;
      gint            tool_columns;

      gtk_widget_size_request (tool_button, &button_requisition);

      for (list = GIMP_LIST (gimp->tool_info_list)->list, n_tools = 0;
           list;
           list = list->next)
        {
          tool_info = list->data;

          if (tool_info->visible)
            n_tools++;
        }

      tool_columns = MAX (1, (allocation->width / button_requisition.width));
      tool_rows    = n_tools / tool_columns;

      if (n_tools % tool_columns)
        tool_rows++;

      if (toolbox->tool_rows    != tool_rows  ||
          toolbox->tool_columns != tool_columns)
        {
          toolbox->tool_rows    = tool_rows;
          toolbox->tool_columns = tool_columns;

          gtk_widget_set_size_request (toolbox->tool_wbox, -1,
                                       tool_rows * button_requisition.height);
        }
    }

  {
    GtkRequisition  color_requisition;
    GtkRequisition  foo_requisition;
    GtkRequisition  image_requisition;
    gint            width;
    gint            height;
    gint            n_areas;
    gint            area_rows;
    gint            area_columns;

    gtk_widget_size_request (toolbox->color_area, &color_requisition);
    gtk_widget_size_request (toolbox->foo_area,   &foo_requisition);
    gtk_widget_size_request (toolbox->image_area, &image_requisition);

    width  = MAX (color_requisition.width,
                  MAX (foo_requisition.width,
                       image_requisition.width));
    height = MAX (color_requisition.height,
                  MAX (foo_requisition.height,
                       image_requisition.height));

    n_areas = (config->toolbox_color_area +
               config->toolbox_foo_area   +
               config->toolbox_image_area);

    area_columns = MAX (1, (allocation->width / width));
    area_rows    = n_areas / area_columns;

    if (n_areas % area_columns)
      area_rows++;

    if (toolbox->area_rows    != area_rows  ||
        toolbox->area_columns != area_columns)
      {
        toolbox->area_rows    = area_rows;
        toolbox->area_columns = area_columns;

        gtk_widget_set_size_request (toolbox->area_wbox, -1,
                                     area_rows * height);
      }
  }
}

static void
gimp_toolbox_style_set (GtkWidget *widget,
                        GtkStyle  *previous_style)
{
  Gimp           *gimp;
  GtkIconSize     tool_icon_size;
  GtkReliefStyle  relief;
  GList          *list;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);

  if (! GIMP_DOCK (widget)->context)
    return;

  gimp = GIMP_DOCK (widget)->context->gimp;

  gtk_widget_style_get (widget,
                        "tool-icon-size", &tool_icon_size,
                        "button-relief",  &relief,
                        NULL);

  for (list = GIMP_LIST (gimp->tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = list->data;
      GtkWidget    *tool_button;

      tool_button = g_object_get_data (G_OBJECT (tool_info),
                                       TOOL_BUTTON_DATA_KEY);

      if (tool_button)
        {
          GtkImage *image;
          gchar    *stock_id;

          image = GTK_IMAGE (GTK_BIN (tool_button)->child);

          gtk_image_get_stock (image, &stock_id, NULL);
          gtk_image_set_from_stock (image, stock_id, tool_icon_size);

          gtk_button_set_relief (GTK_BUTTON (tool_button), relief);
        }
    }

  gimp_toolbox_set_geometry (GIMP_TOOLBOX (widget));
}

static void
gimp_toolbox_book_added (GimpDock     *dock,
                         GimpDockbook *dockbook)
{
  if (g_list_length (dock->dockbooks) == 1)
    {
      gimp_toolbox_set_geometry (GIMP_TOOLBOX (dock));
      toolbox_separator_collapse (GIMP_TOOLBOX (dock));
    }
}

static void
gimp_toolbox_book_removed (GimpDock     *dock,
                           GimpDockbook *dockbook)
{
  if (g_list_length (dock->dockbooks) == 0 &&
      ! (GTK_OBJECT_FLAGS (dock) & GTK_IN_DESTRUCTION))
    {
      gimp_toolbox_set_geometry (GIMP_TOOLBOX (dock));
      toolbox_separator_expand (GIMP_TOOLBOX (dock));
    }
}

static void
gimp_toolbox_set_geometry (GimpToolbox *toolbox)
{
  Gimp         *gimp;
  GimpToolInfo *tool_info;
  GtkWidget    *tool_button;

  gimp = GIMP_DOCK (toolbox)->context->gimp;

  tool_info = gimp_get_tool_info (gimp, "gimp-rect-select-tool");
  tool_button = g_object_get_data (G_OBJECT (tool_info), TOOL_BUTTON_DATA_KEY);

  if (tool_button)
    {
      GtkWidget      *main_vbox;
      GtkRequisition  menubar_requisition = { 0, 0 };
      GtkRequisition  button_requisition;
      gint            border_width;
      GdkGeometry     geometry;

      main_vbox = GIMP_DOCK (toolbox)->main_vbox;

      if (toolbox->menu_bar)
        gtk_widget_size_request (toolbox->menu_bar, &menubar_requisition);

      gtk_widget_size_request (tool_button, &button_requisition);

      border_width = gtk_container_get_border_width (GTK_CONTAINER (main_vbox));

      geometry.min_width  = (2 * border_width +
                             2 * button_requisition.width);
      geometry.min_height = -1;
      geometry.width_inc  = button_requisition.width;
      geometry.height_inc = (GIMP_DOCK (toolbox)->dockbooks ?
                             1 : button_requisition.height);

      gtk_window_set_geometry_hints (GTK_WINDOW (toolbox),
                                     NULL,
                                     &geometry,
                                     GDK_HINT_MIN_SIZE   |
                                     GDK_HINT_RESIZE_INC |
                                     GDK_HINT_USER_POS);
    }
}

GtkWidget *
gimp_toolbox_new (GimpDialogFactory *dialog_factory,
                  GimpContext       *context)
{
  GimpToolbox *toolbox;

  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  toolbox = g_object_new (GIMP_TYPE_TOOLBOX,
                          "title",          GIMP_ACRONYM,
                          "context",        context,
                          "dialog-factory", dialog_factory,
                          NULL);

  return GTK_WIDGET (toolbox);
}


/*  private functions  */

static void
toolbox_separator_expand (GimpToolbox *toolbox)
{
  GimpDock  *dock = GIMP_DOCK (toolbox);
  GList     *children;
  GtkWidget *separator;

  children = gtk_container_get_children (GTK_CONTAINER (dock->vbox));
  separator = children->data;
  g_list_free (children);

  gtk_box_set_child_packing (GTK_BOX (dock->vbox), separator,
                             TRUE, TRUE, 0, GTK_PACK_START);
  gimp_dock_separator_set_show_label (GIMP_DOCK_SEPARATOR (separator), TRUE);
}

static void
toolbox_separator_collapse (GimpToolbox *toolbox)
{
  GimpDock  *dock = GIMP_DOCK (toolbox);
  GList     *children;
  GtkWidget *separator;

  children = gtk_container_get_children (GTK_CONTAINER (dock->vbox));
  separator = children->data;
  g_list_free (children);

  gtk_box_set_child_packing (GTK_BOX (dock->vbox), separator,
                             FALSE, FALSE, 0, GTK_PACK_START);
  gimp_dock_separator_set_show_label (GIMP_DOCK_SEPARATOR (separator), FALSE);
}

static void
toolbox_create_tools (GimpToolbox *toolbox,
                      GimpContext *context)
{
  GimpToolInfo *active_tool;
  GList        *list;
  GSList       *group = NULL;

  active_tool = gimp_context_get_tool (context);

  for (list = GIMP_LIST (context->gimp->tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = list->data;
      GtkWidget    *button;
      GtkWidget    *image;
      const gchar  *stock_id;

      button = gtk_radio_button_new (group);
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

      gtk_wrap_box_pack (GTK_WRAP_BOX (toolbox->tool_wbox), button,
                         FALSE, FALSE, FALSE, FALSE);

      if (tool_info->visible)
        gtk_widget_show (button);

      g_signal_connect_object (tool_info, "notify::visible",
                               G_CALLBACK (toolbox_tool_visible_notify),
                               button, 0);

      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));
      image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      g_object_set_data (G_OBJECT (tool_info), TOOL_BUTTON_DATA_KEY, button);
      g_object_set_data (G_OBJECT (button),    TOOL_INFO_DATA_KEY,   tool_info);

      if (tool_info == active_tool)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      g_signal_connect (button, "toggled",
                        G_CALLBACK (toolbox_tool_button_toggled),
                        tool_info);

      g_signal_connect (button, "button-press-event",
                        G_CALLBACK (toolbox_tool_button_press),
                        toolbox);

      if (GIMP_IMAGE_DOCK (toolbox)->ui_manager)
        {
          GtkAction   *action;
          const gchar *identifier;
          gchar       *tmp;
          gchar       *name;

          identifier = gimp_object_get_name (GIMP_OBJECT (tool_info));

          tmp = g_strndup (identifier + strlen ("gimp-"),
                           strlen (identifier) - strlen ("gimp--tool"));
          name = g_strdup_printf ("tools-%s", tmp);
          g_free (tmp);

          action = gimp_ui_manager_find_action (GIMP_IMAGE_DOCK (toolbox)->ui_manager,
                                                "tools", name);
          g_free (name);

          if (action)
            gimp_widget_set_accel_help (button, action);
          else
            gimp_help_set_help_data (button,
                                     tool_info->help, tool_info->help_id);
        }
    }

  g_signal_connect_object (context->gimp->tool_info_list, "reorder",
                           G_CALLBACK (toolbox_tool_reorder),
                           toolbox->tool_wbox, 0);
}

static GtkWidget *
toolbox_create_color_area (GimpToolbox *toolbox,
                           GimpContext *context)
{
  GtkWidget *alignment;
  GtkWidget *col_area;

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 2);

  gimp_help_set_help_data (alignment, NULL, GIMP_HELP_TOOLBOX_COLOR_AREA);

  col_area = gimp_toolbox_color_area_create (toolbox, 54, 42);
  gtk_container_add (GTK_CONTAINER (alignment), col_area);
  gtk_widget_show (col_area);

  gimp_help_set_help_data
    (col_area, _("Foreground & background colors.  The black and white "
                 "squares reset colors.  The arrows swap colors. Click "
                 "to open the color selection dialog."), NULL);

  return alignment;
}

static GtkWidget *
toolbox_create_foo_area (GimpToolbox *toolbox,
                         GimpContext *context)
{
  GtkWidget *alignment;
  GtkWidget *foo_area;

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 2);

  gimp_help_set_help_data (alignment, NULL, GIMP_HELP_TOOLBOX_INDICATOR_AREA);

  foo_area = gimp_toolbox_indicator_area_create (toolbox);
  gtk_container_add (GTK_CONTAINER (alignment), foo_area);
  gtk_widget_show (foo_area);

  return alignment;
}

static GtkWidget *
toolbox_create_image_area (GimpToolbox *toolbox,
                           GimpContext *context)
{
  GtkWidget *alignment;
  GtkWidget *image_area;

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 2);

  gimp_help_set_help_data (alignment, NULL, GIMP_HELP_TOOLBOX_IMAGE_AREA);

  image_area = gimp_toolbox_image_area_create (toolbox, 52, 42);
  gtk_container_add (GTK_CONTAINER (alignment), image_area);
  gtk_widget_show (image_area);

  return alignment;
}

static void
toolbox_area_notify (GimpGuiConfig *config,
                     GParamSpec    *pspec,
                     GtkWidget     *area)
{
  gboolean visible;

  if (config->toolbox_color_area ||
      config->toolbox_foo_area   ||
      config->toolbox_image_area)
    {
      GtkRequisition req;

      gtk_widget_show (area->parent);

#ifdef __GNUC__
#warning FIXME: fix GtkWrapBox child requisition/allocation instead of hacking badly (bug #162500).
#endif
      gtk_widget_size_request (area, &req);
      gtk_widget_set_size_request (area->parent, req.width, req.height);
    }
  else
    {
      gtk_widget_hide (area->parent);
      gtk_widget_set_size_request (area->parent, -1, -1);
    }

  g_object_get (config, pspec->name, &visible, NULL);
  g_object_set (area, "visible", visible, NULL);
}

static void
toolbox_tool_changed (GimpContext  *context,
                      GimpToolInfo *tool_info,
                      gpointer      data)
{
  if (tool_info)
    {
      GtkWidget *toolbox_button = g_object_get_data (G_OBJECT (tool_info),
                                                     TOOL_BUTTON_DATA_KEY);

      if (toolbox_button && ! GTK_TOGGLE_BUTTON (toolbox_button)->active)
        {
          g_signal_handlers_block_by_func (toolbox_button,
                                           toolbox_tool_button_toggled,
                                           tool_info);

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbox_button),
                                        TRUE);

          g_signal_handlers_unblock_by_func (toolbox_button,
                                             toolbox_tool_button_toggled,
                                             tool_info);
        }
    }
}

static void
toolbox_tool_reorder (GimpContainer *container,
                      GimpToolInfo  *tool_info,
                      gint           index,
                      GtkWidget     *wrap_box)
{
  if (tool_info)
    {
      GtkWidget *button = g_object_get_data (G_OBJECT (tool_info),
                                             TOOL_BUTTON_DATA_KEY);

      gtk_wrap_box_reorder_child (GTK_WRAP_BOX (wrap_box), button, index);
    }
}

static void
toolbox_tool_visible_notify (GimpToolInfo *tool_info,
                             GParamSpec   *pspec,
                             GtkWidget    *button)
{
  if (tool_info->visible)
    gtk_widget_show (button);
  else
    gtk_widget_hide (button);
}

static void
toolbox_tool_button_toggled (GtkWidget    *widget,
                             GimpToolInfo *tool_info)
{
  GtkWidget *toolbox = gtk_widget_get_toplevel (widget);

  if (GTK_TOGGLE_BUTTON (widget)->active)
    gimp_context_set_tool (GIMP_DOCK (toolbox)->context, tool_info);
}

static gboolean
toolbox_tool_button_press (GtkWidget      *widget,
                           GdkEventButton *event,
                           GimpToolbox    *toolbox)
{
  if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      gimp_dialog_factory_dialog_raise (GIMP_DOCK (toolbox)->dialog_factory,
                                        gtk_widget_get_screen (widget),
                                        "gimp-tool-options",
                                        -1);
    }
  else if (event->type == GDK_BUTTON_PRESS && event->button == 2)
    {
      GimpContext  *context = GIMP_DOCK (toolbox)->context;
      GtkClipboard *clipboard;

      clipboard = gtk_widget_get_clipboard (widget, GDK_SELECTION_PRIMARY);
      gtk_clipboard_request_text (clipboard,
                                  toolbox_paste_received,
                                  g_object_ref (context));
    }

  return FALSE;
}

static gboolean
toolbox_check_device (GtkWidget *widget,
                      GdkEvent  *event,
                      Gimp      *gimp)
{
  gimp_devices_check_change (gimp, event);

  return FALSE;
}

static void
toolbox_paste_received (GtkClipboard *clipboard,
                        const gchar  *text,
                        gpointer      data)
{
  GimpContext *context = GIMP_CONTEXT (data);

  if (text)
    {
      const gchar *newline = strchr (text, '\n');
      gchar       *copy;

      if (newline)
        copy = g_strndup (text, newline - text);
      else
        copy = g_strdup (text);

      g_strstrip (copy);

      if (strlen (copy))
        {
          GimpImage         *image;
          GimpPDBStatusType  status;
          GError            *error = NULL;

          image = file_open_with_display (context->gimp, context, NULL,
                                          copy, FALSE, &status, &error);

          if (! image && status != GIMP_PDB_CANCEL)
            {
              gchar *filename = file_utils_uri_display_name (copy);

              gimp_message (context->gimp, NULL, GIMP_MESSAGE_ERROR,
                            _("Opening '%s' failed:\n\n%s"),
                            filename, error->message);

              g_clear_error (&error);
              g_free (filename);
            }
        }

      g_free (copy);
    }

  g_object_unref (context);
}
