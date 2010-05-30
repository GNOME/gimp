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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#undef GSEAL_ENABLE

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "gimpcairo-wilber.h"
#include "gimpdevices.h"
#include "gimpdialogfactory.h"
#include "gimpdockwindow.h"
#include "gimphelp-ids.h"
#include "gimppanedbox.h"
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

#define TOOL_BUTTON_DATA_KEY   "gimp-tool-item"
#define TOOL_INFO_DATA_KEY     "gimp-tool-info"

enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_DIALOG_FACTORY,
  PROP_UI_MANAGER
};


struct _GimpToolboxPrivate
{
  GimpContext       *context;
  GimpDialogFactory *dialog_factory;
  GimpUIManager     *ui_manager;

  GtkWidget         *vbox;

  GtkWidget         *header;
  GtkWidget         *tool_palette;

  GtkWidget         *area_wbox;
  GtkWidget         *color_area;
  GtkWidget         *foo_area;
  GtkWidget         *image_area;

  gint               tool_rows;
  gint               tool_columns;
  gint               area_rows;
  gint               area_columns;

  GimpPanedBox      *drag_handler;

  gboolean           in_destruction;
};


static GObject   * gimp_toolbox_constructor             (GType                  type,
                                                         guint                  n_params,
                                                         GObjectConstructParam *params);
static void        gimp_toolbox_dispose                 (GObject               *object);
static void        gimp_toolbox_set_property            (GObject               *object,
                                                         guint                  property_id,
                                                         const GValue          *value,
                                                         GParamSpec            *pspec);
static void        gimp_toolbox_get_property            (GObject               *object,
                                                         guint                  property_id,
                                                         GValue                *value,
                                                         GParamSpec            *pspec);
static void        gimp_toolbox_size_allocate           (GtkWidget             *widget,
                                                         GtkAllocation         *allocation);
static void        gimp_toolbox_style_set               (GtkWidget             *widget,
                                                         GtkStyle              *previous_style);
static gboolean    gimp_toolbox_button_press_event      (GtkWidget             *widget,
                                                         GdkEventButton        *event);
static gboolean    gimp_toolbox_expose_event            (GtkWidget             *widget,
                                                         GdkEventExpose        *event);
static void        gimp_toolbox_drag_leave              (GtkWidget             *widget,
                                                         GdkDragContext        *context,
                                                         guint                  time,
                                                         GimpToolbox           *toolbox);
static gboolean    gimp_toolbox_drag_motion             (GtkWidget             *widget,
                                                         GdkDragContext        *context,
                                                         gint                   x,
                                                         gint                   y,
                                                         guint                  time,
                                                         GimpToolbox           *toolbox);
static gboolean    gimp_toolbox_drag_drop               (GtkWidget             *widget,
                                                         GdkDragContext        *context,
                                                         gint                   x,
                                                         gint                   y,
                                                         guint                  time,
                                                         GimpToolbox           *toolbox);
static gchar     * gimp_toolbox_get_description         (GimpDock              *dock,
                                                         gboolean               complete);
static void        gimp_toolbox_set_host_geometry_hints (GimpDock              *dock,
                                                         GtkWindow             *window);
static void        gimp_toolbox_book_added              (GimpDock              *dock,
                                                         GimpDockbook          *dockbook);
static void        gimp_toolbox_book_removed            (GimpDock              *dock,
                                                         GimpDockbook          *dockbook);
static void        toolbox_create_tools                 (GimpToolbox           *toolbox,
                                                         GimpContext           *context);
static GtkWidget * toolbox_create_color_area            (GimpToolbox           *toolbox,
                                                         GimpContext           *context);
static GtkWidget * toolbox_create_foo_area              (GimpToolbox           *toolbox,
                                                         GimpContext           *context);
static GtkWidget * toolbox_create_image_area            (GimpToolbox           *toolbox,
                                                         GimpContext           *context);
static void        toolbox_area_notify                  (GimpGuiConfig         *config,
                                                         GParamSpec            *pspec,
                                                         GtkWidget             *area);
static void        toolbox_wilber_notify                (GimpGuiConfig         *config,
                                                         GParamSpec            *pspec,
                                                         GtkWidget             *wilber);
static void        toolbox_tool_changed                 (GimpContext           *context,
                                                         GimpToolInfo          *tool_info,
                                                         GimpToolbox           *toolbox);
static void        toolbox_tool_reorder                 (GimpContainer         *container,
                                                         GimpToolInfo          *tool_info,
                                                         gint                   index,
                                                         GimpToolbox           *toolbox);
static void        toolbox_tool_visible_notify          (GimpToolInfo          *tool_info,
                                                         GParamSpec            *pspec,
                                                         GtkToolItem           *item);
static void        toolbox_tool_button_toggled          (GtkWidget             *widget,
                                                         GimpToolbox           *toolbox);
static gboolean    toolbox_tool_button_press            (GtkWidget             *widget,
                                                         GdkEventButton        *bevent,
                                                         GimpToolbox           *toolbox);
static gboolean    toolbox_check_device                 (GtkWidget             *widget,
                                                         GdkEvent              *event,
                                                         Gimp                  *gimp);
static void        toolbox_paste_received               (GtkClipboard          *clipboard,
                                                         const gchar           *text,
                                                         gpointer               data);


G_DEFINE_TYPE (GimpToolbox, gimp_toolbox, GIMP_TYPE_DOCK)

#define parent_class gimp_toolbox_parent_class


static void
gimp_toolbox_class_init (GimpToolboxClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GimpDockClass  *dock_class   = GIMP_DOCK_CLASS (klass);

  object_class->constructor           = gimp_toolbox_constructor;
  object_class->dispose               = gimp_toolbox_dispose;
  object_class->set_property          = gimp_toolbox_set_property;
  object_class->get_property          = gimp_toolbox_get_property;

  widget_class->size_allocate         = gimp_toolbox_size_allocate;
  widget_class->style_set             = gimp_toolbox_style_set;
  widget_class->button_press_event    = gimp_toolbox_button_press_event;
  widget_class->expose_event          = gimp_toolbox_expose_event;

  dock_class->get_description         = gimp_toolbox_get_description;
  dock_class->set_host_geometry_hints = gimp_toolbox_set_host_geometry_hints;
  dock_class->book_added              = gimp_toolbox_book_added;
  dock_class->book_removed            = gimp_toolbox_book_removed;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DIALOG_FACTORY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_MANAGER,
                                   g_param_spec_object ("ui-manager",
                                                        NULL, NULL,
                                                        GIMP_TYPE_UI_MANAGER,
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

  g_type_class_add_private (klass, sizeof (GimpToolboxPrivate));
}

static void
gimp_toolbox_init (GimpToolbox *toolbox)
{
  toolbox->p = G_TYPE_INSTANCE_GET_PRIVATE (toolbox,
                                            GIMP_TYPE_TOOLBOX,
                                            GimpToolboxPrivate);

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
  GimpGuiConfig *config;
  GtkWidget     *main_vbox;
  GdkDisplay    *display;
  GList         *list;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  toolbox = GIMP_TOOLBOX (object);

  config  = GIMP_GUI_CONFIG (toolbox->p->context->gimp->config);

  main_vbox = gimp_dock_get_main_vbox (GIMP_DOCK (toolbox));

  toolbox->p->vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), toolbox->p->vbox, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_vbox), toolbox->p->vbox, 0);
  gtk_widget_show (toolbox->p->vbox);

  /* Use g_signal_connect() also for the toolbox itself so we can pass
   * data and reuse the same function for the vbox
   */
  g_signal_connect (toolbox, "drag-leave",
                    G_CALLBACK (gimp_toolbox_drag_leave),
                    toolbox);
  g_signal_connect (toolbox, "drag-motion",
                    G_CALLBACK (gimp_toolbox_drag_motion),
                    toolbox);
  g_signal_connect (toolbox, "drag-drop",
                    G_CALLBACK (gimp_toolbox_drag_drop),
                    toolbox);
  g_signal_connect (toolbox->p->vbox, "drag-leave",
                    G_CALLBACK (gimp_toolbox_drag_leave),
                    toolbox);
  g_signal_connect (toolbox->p->vbox, "drag-motion",
                    G_CALLBACK (gimp_toolbox_drag_motion),
                    toolbox);
  g_signal_connect (toolbox->p->vbox, "drag-drop",
                    G_CALLBACK (gimp_toolbox_drag_drop),
                    toolbox);

  toolbox->p->header = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (toolbox->p->header), GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (toolbox->p->vbox), toolbox->p->header,
                      FALSE, FALSE, 0);

  if (config->toolbox_wilber)
    gtk_widget_show (toolbox->p->header);

  gimp_help_set_help_data (toolbox->p->header,
                           _("Drop image files here to open them"), NULL);

  g_signal_connect_object (config, "notify::toolbox-wilber",
                           G_CALLBACK (toolbox_wilber_notify),
                           toolbox->p->header, 0);

  toolbox->p->tool_palette = gtk_tool_palette_new ();
  gtk_box_pack_start (GTK_BOX (toolbox->p->vbox), toolbox->p->tool_palette,
                      TRUE, TRUE, 0);
  gtk_widget_show (toolbox->p->tool_palette);

  toolbox->p->area_wbox = gtk_hwrap_box_new (FALSE);
  gtk_wrap_box_set_justify (GTK_WRAP_BOX (toolbox->p->area_wbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify (GTK_WRAP_BOX (toolbox->p->area_wbox),
                                 GTK_JUSTIFY_LEFT);
  gtk_wrap_box_set_aspect_ratio (GTK_WRAP_BOX (toolbox->p->area_wbox),
                                 2.0 / 15.0);

  gtk_box_pack_start (GTK_BOX (toolbox->p->vbox), toolbox->p->area_wbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (toolbox->p->area_wbox);

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
                        toolbox->p->context->gimp);

      gtk_widget_add_events (GTK_WIDGET (toolbox), GDK_POINTER_MOTION_MASK);
      gtk_widget_set_extension_events (GTK_WIDGET (toolbox),
                                       GDK_EXTENSION_EVENTS_CURSOR);
    }

  toolbox_create_tools (toolbox, toolbox->p->context);

  toolbox->p->color_area = toolbox_create_color_area (toolbox, toolbox->p->context);
  gtk_wrap_box_pack_wrapped (GTK_WRAP_BOX (toolbox->p->area_wbox),
                             toolbox->p->color_area,
                             TRUE, TRUE, FALSE, TRUE, TRUE);
  if (config->toolbox_color_area)
    gtk_widget_show (toolbox->p->color_area);

  g_signal_connect_object (config, "notify::toolbox-color-area",
                           G_CALLBACK (toolbox_area_notify),
                           toolbox->p->color_area, 0);

  toolbox->p->foo_area = toolbox_create_foo_area (toolbox, toolbox->p->context);
  gtk_wrap_box_pack (GTK_WRAP_BOX (toolbox->p->area_wbox), toolbox->p->foo_area,
                     TRUE, TRUE, FALSE, TRUE);
  if (config->toolbox_foo_area)
    gtk_widget_show (toolbox->p->foo_area);

  g_signal_connect_object (config, "notify::toolbox-foo-area",
                           G_CALLBACK (toolbox_area_notify),
                           toolbox->p->foo_area, 0);

  toolbox->p->image_area = toolbox_create_image_area (toolbox, toolbox->p->context);
  gtk_wrap_box_pack (GTK_WRAP_BOX (toolbox->p->area_wbox), toolbox->p->image_area,
                     TRUE, TRUE, FALSE, TRUE);
  if (config->toolbox_image_area)
    gtk_widget_show (toolbox->p->image_area);

  g_signal_connect_object (config, "notify::toolbox-image-area",
                           G_CALLBACK (toolbox_area_notify),
                           toolbox->p->image_area, 0);

  g_signal_connect_object (toolbox->p->context, "tool-changed",
                           G_CALLBACK (toolbox_tool_changed),
                           toolbox,
                           0);

  gimp_toolbox_dnd_init (GIMP_TOOLBOX (toolbox));

  gimp_toolbox_style_set (GTK_WIDGET (toolbox),
                          gtk_widget_get_style (GTK_WIDGET (toolbox)));

  return object;
}

static void
gimp_toolbox_dispose (GObject *object)
{
  GimpToolbox *toolbox = GIMP_TOOLBOX (object);

  toolbox->p->in_destruction = TRUE;

  G_OBJECT_CLASS (parent_class)->dispose (object);

  toolbox->p->in_destruction = FALSE;
}

static void
gimp_toolbox_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpToolbox *toolbox = GIMP_TOOLBOX (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      toolbox->p->context = g_value_dup_object (value);
      break;

    case PROP_DIALOG_FACTORY:
      toolbox->p->dialog_factory = g_value_dup_object (value);
      break;

    case PROP_UI_MANAGER:
      toolbox->p->ui_manager = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_toolbox_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpToolbox *toolbox = GIMP_TOOLBOX (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, toolbox->p->context);
      break;

    case PROP_DIALOG_FACTORY:
      g_value_set_object (value, toolbox->p->dialog_factory);
      break;

    case PROP_UI_MANAGER:
      g_value_set_object (value, toolbox->p->ui_manager);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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

  if (! gimp_toolbox_get_context (toolbox))
    return;

  gimp = gimp_toolbox_get_context (toolbox)->gimp;

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

      for (list = gimp_get_tool_info_iter (gimp), n_tools = 0;
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

      if (toolbox->p->tool_rows    != tool_rows  ||
          toolbox->p->tool_columns != tool_columns)
        {
          toolbox->p->tool_rows    = tool_rows;
          toolbox->p->tool_columns = tool_columns;

          gtk_widget_set_size_request (toolbox->p->tool_palette, -1,
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

    gtk_widget_size_request (toolbox->p->color_area, &color_requisition);
    gtk_widget_size_request (toolbox->p->foo_area,   &foo_requisition);
    gtk_widget_size_request (toolbox->p->image_area, &image_requisition);

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

    if (toolbox->p->area_rows    != area_rows  ||
        toolbox->p->area_columns != area_columns)
      {
        toolbox->p->area_rows    = area_rows;
        toolbox->p->area_columns = area_columns;

        gtk_widget_set_size_request (toolbox->p->area_wbox, -1,
                                     area_rows * height);
      }
  }
}

static void
gimp_toolbox_style_set (GtkWidget *widget,
                        GtkStyle  *previous_style)
{
  GimpToolbox    *toolbox = GIMP_TOOLBOX (widget);
  Gimp           *gimp;
  GtkIconSize     tool_icon_size;
  GtkReliefStyle  relief;
  GList          *list;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);

  if (! gimp_toolbox_get_context (toolbox))
    return;

  gimp = gimp_toolbox_get_context (toolbox)->gimp;

  gtk_widget_style_get (widget,
                        "tool-icon-size", &tool_icon_size,
                        "button-relief",  &relief,
                        NULL);

  gtk_tool_palette_set_icon_size (GTK_TOOL_PALETTE (toolbox->p->tool_palette),
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

  gimp_dock_invalidate_geometry (GIMP_DOCK (widget));
}

static gboolean
gimp_toolbox_button_press_event (GtkWidget      *widget,
                                 GdkEventButton *event)
{
  if (event->type == GDK_BUTTON_PRESS && event->button == 2)
    {
      GtkClipboard *clipboard;

      clipboard = gtk_widget_get_clipboard (widget, GDK_SELECTION_PRIMARY);
      gtk_clipboard_request_text (clipboard,
                                  toolbox_paste_received,
                                  g_object_ref (gimp_toolbox_get_context (GIMP_TOOLBOX (widget))));

      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_toolbox_expose_event (GtkWidget      *widget,
                           GdkEventExpose *event)
{
  GimpToolbox  *toolbox = GIMP_TOOLBOX (widget);
  GtkAllocation header_allocation;
  GdkRectangle  clip_rect;

  GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

  gtk_widget_get_allocation (toolbox->p->header, &header_allocation);

  if (gtk_widget_get_visible (toolbox->p->header) &&
      gdk_rectangle_intersect (&event->area,
                               &header_allocation,
                               &clip_rect))
    {
      GtkStyle     *style = gtk_widget_get_style (widget);
      GtkStateType  state = gtk_widget_get_state (widget);
      cairo_t      *cr;
      gint          header_height;
      gint          header_width;
      gdouble       wilber_width;
      gdouble       wilber_height;
      gdouble       factor;

      cr = gdk_cairo_create (gtk_widget_get_window (widget));
      gdk_cairo_rectangle (cr, &clip_rect);
      cairo_clip (cr);

      header_width  = header_allocation.width;
      header_height = header_allocation.height;

      gimp_cairo_wilber_get_size (cr, &wilber_width, &wilber_height);

      factor = header_width / wilber_width * 0.9;

      cairo_scale (cr, factor, factor);

      gimp_cairo_wilber (cr,
                         (header_width  / factor - wilber_width)  / 2.0,
                         (header_height / factor - wilber_height) / 2.0);

      cairo_set_source_rgba (cr,
                             style->fg[state].red   / 65535.0,
                             style->fg[state].green / 65535.0,
                             style->fg[state].blue  / 65535.0,
                             0.10);
      cairo_fill (cr);

      cairo_destroy (cr);
    }

  return FALSE;
}

static void
gimp_toolbox_drag_leave (GtkWidget      *widget,
                         GdkDragContext *context,
                         guint           time,
                         GimpToolbox    *toolbox)
{
  gimp_highlight_widget (widget, FALSE);
}

static gboolean
gimp_toolbox_drag_motion (GtkWidget      *widget,
                          GdkDragContext *context,
                          gint            x,
                          gint            y,
                          guint           time,
                          GimpToolbox    *toolbox)
{
  gboolean other_will_handle = FALSE;
  gboolean we_will_handle    = FALSE;
  gboolean handled           = FALSE;

  other_will_handle = gimp_paned_box_will_handle_drag (toolbox->p->drag_handler,
                                                       widget,
                                                       context,
                                                       x, y,
                                                       time);
  we_will_handle = (gtk_drag_dest_find_target (widget, context, NULL) !=
                    GDK_NONE);

  handled = ! other_will_handle && we_will_handle;
  gdk_drag_status (context, handled ? GDK_ACTION_MOVE : 0, time);
  gimp_highlight_widget (widget, handled);
  return handled;
}

static gboolean
gimp_toolbox_drag_drop (GtkWidget      *widget,
                        GdkDragContext *context,
                        gint            x,
                        gint            y,
                        guint           time,
                        GimpToolbox    *toolbox)
{
  gboolean handled = FALSE;

  if (gimp_paned_box_will_handle_drag (toolbox->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      /* Make event fall through to the drag handler */
      handled = FALSE;
    }
  else
    {
      GdkAtom target = gtk_drag_dest_find_target (widget, context, NULL);

      if (target != GDK_NONE)
        {
          /* The URI handlers etc will handle this */
          gtk_drag_get_data (widget, context, target, time);
          handled = TRUE;
        }
    }

  if (handled)	  
    gtk_drag_finish (context, 
                     TRUE /*success*/,
                     (context->action == GDK_ACTION_MOVE) /*del*/,
                     time);

  return handled;
}

static gchar *
gimp_toolbox_get_description (GimpDock *dock,
                              gboolean  complete)
{
  GString *desc      = g_string_new (_("Toolbox"));
  gchar   *dock_desc = GIMP_DOCK_CLASS (parent_class)->get_description (dock,
                                                                        complete);

  if (dock_desc && strlen (dock_desc) > 0)
    {
      g_string_append (desc, GIMP_DOCK_BOOK_SEPARATOR);
      g_string_append (desc, dock_desc);
    }

  g_free (dock_desc);

  return g_string_free (desc, FALSE /*free_segment*/);
}

static void
gimp_toolbox_book_added (GimpDock     *dock,
                         GimpDockbook *dockbook)
{
  if (GIMP_DOCK_CLASS (gimp_toolbox_parent_class)->book_added)
    GIMP_DOCK_CLASS (gimp_toolbox_parent_class)->book_added (dock, dockbook);

  if (g_list_length (gimp_dock_get_dockbooks (dock)) == 1)
    {
      gimp_dock_invalidate_geometry (dock);
    }
}

static void
gimp_toolbox_book_removed (GimpDock     *dock,
                           GimpDockbook *dockbook)
{
  GimpToolbox *toolbox = GIMP_TOOLBOX (dock);

  if (GIMP_DOCK_CLASS (gimp_toolbox_parent_class)->book_removed)
    GIMP_DOCK_CLASS (gimp_toolbox_parent_class)->book_removed (dock, dockbook);

  if (g_list_length (gimp_dock_get_dockbooks (dock)) == 0 &&
      ! toolbox->p->in_destruction)
    {
      gimp_dock_invalidate_geometry (dock);
    }
}

static void
gimp_toolbox_set_host_geometry_hints (GimpDock  *dock,
                                      GtkWindow *window)
{
  GimpToolbox  *toolbox = GIMP_TOOLBOX (dock);
  Gimp         *gimp;
  GimpToolInfo *tool_info;
  GtkWidget    *tool_button;

  gimp = gimp_toolbox_get_context (toolbox)->gimp;

  tool_info = gimp_get_tool_info (gimp, "gimp-rect-select-tool");
  tool_button = g_object_get_data (G_OBJECT (tool_info), TOOL_BUTTON_DATA_KEY);

  if (tool_button)
    {
      GtkWidget      *main_vbox = gimp_dock_get_main_vbox (GIMP_DOCK (toolbox));
      GtkRequisition  button_requisition;
      gint            border_width;
      GdkGeometry     geometry;

      gtk_widget_size_request (tool_button, &button_requisition);

      gtk_widget_set_size_request (toolbox->p->header,
                                   -1,
                                   button_requisition.height *
                                   PANGO_SCALE_SMALL);

      border_width = gtk_container_get_border_width (GTK_CONTAINER (main_vbox));

      geometry.min_width  = (2 * border_width +
                             2 * button_requisition.width);
      geometry.min_height = -1;
      geometry.width_inc  = button_requisition.width;
      geometry.height_inc = (gimp_dock_get_dockbooks (GIMP_DOCK (toolbox)) ?
                             1 : button_requisition.height);

      gtk_window_set_geometry_hints (window,
                                     NULL,
                                     &geometry,
                                     GDK_HINT_MIN_SIZE   |
                                     GDK_HINT_RESIZE_INC |
                                     GDK_HINT_USER_POS);

      gimp_dialog_factory_set_has_min_size (window, TRUE);
    }
}

GtkWidget *
gimp_toolbox_new (GimpDialogFactory *factory,
                  GimpContext       *context,
                  GimpUIManager     *ui_manager)
{
  return g_object_new (GIMP_TYPE_TOOLBOX,
                       "dialog-factory", factory,
                       "context",        context,
                       "ui-manager",     ui_manager,
                       NULL);
}

GimpContext *
gimp_toolbox_get_context (GimpToolbox *toolbox)
{
  g_return_val_if_fail (GIMP_IS_TOOLBOX (toolbox), NULL);

  return toolbox->p->context;
}

GimpDialogFactory *
gimp_toolbox_get_dialog_factory (GimpToolbox *toolbox)
{
  g_return_val_if_fail (GIMP_IS_TOOLBOX (toolbox), NULL);

  return toolbox->p->dialog_factory;
}

GimpUIManager *
gimp_toolbox_get_ui_manager (GimpToolbox *toolbox)
{
  g_return_val_if_fail (GIMP_IS_TOOLBOX (toolbox), NULL);

  return toolbox->p->ui_manager;
}

GtkWidget *
gimp_toolbox_get_vbox (GimpToolbox *toolbox)
{
  g_return_val_if_fail (GIMP_IS_TOOLBOX (toolbox), NULL);

  return toolbox->p->vbox;
}

void
gimp_toolbox_set_drag_handler (GimpToolbox  *toolbox,
                               GimpPanedBox *drag_handler)
{
  g_return_if_fail (GIMP_IS_TOOLBOX (toolbox));

  toolbox->p->drag_handler = drag_handler;
}


/*  private functions  */

static void
toolbox_create_tools (GimpToolbox *toolbox,
                      GimpContext *context)
{
  GtkWidget    *group;
  GimpToolInfo *active_tool;
  GList        *list;
  GSList       *item_group = NULL;

  group = gtk_tool_item_group_new (_("Tools"));
  gtk_tool_item_group_set_label_widget (GTK_TOOL_ITEM_GROUP (group), NULL);
  gtk_container_add (GTK_CONTAINER (toolbox->p->tool_palette), group);
  gtk_widget_show (group);

  active_tool = gimp_context_get_tool (context);

  for (list = gimp_get_tool_info_iter (context->gimp);
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
                               G_CALLBACK (toolbox_tool_visible_notify),
                               item, 0);

      g_object_set_data (G_OBJECT (tool_info), TOOL_BUTTON_DATA_KEY, item);
      g_object_set_data (G_OBJECT (item)  ,    TOOL_INFO_DATA_KEY,   tool_info);

      if (tool_info == active_tool)
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (item), TRUE);

      g_signal_connect (item, "toggled",
                        G_CALLBACK (toolbox_tool_button_toggled),
                        toolbox);

      g_signal_connect (gtk_bin_get_child (GTK_BIN (item)), "button-press-event",
                        G_CALLBACK (toolbox_tool_button_press),
                        toolbox);


      if (toolbox->p->ui_manager)
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

          action = gimp_ui_manager_find_action (toolbox->p->ui_manager, "tools", name);
          g_free (name);

          if (action)
            gimp_widget_set_accel_help (GTK_WIDGET (item), action);
          else
            gimp_help_set_help_data (GTK_WIDGET (item),
                                     tool_info->help, tool_info->help_id);
        }
    }

  g_signal_connect_object (context->gimp->tool_info_list, "reorder",
                           G_CALLBACK (toolbox_tool_reorder),
                           toolbox, 0);
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
  GtkWidget *parent = gtk_widget_get_parent (area);
  gboolean   visible;

  if (config->toolbox_color_area ||
      config->toolbox_foo_area   ||
      config->toolbox_image_area)
    {
      GtkRequisition req;

      gtk_widget_show (parent);

      /* FIXME: fix GtkWrapBox child requisition/allocation instead of hacking badly (bug #162500). */
      gtk_widget_size_request (area, &req);
      gtk_widget_set_size_request (parent, req.width, req.height);
    }
  else
    {
      gtk_widget_hide (parent);
      gtk_widget_set_size_request (parent, -1, -1);
    }

  g_object_get (config, pspec->name, &visible, NULL);
  g_object_set (area, "visible", visible, NULL);
}

static void
toolbox_wilber_notify (GimpGuiConfig *config,
                       GParamSpec    *pspec,
                       GtkWidget     *wilber)
{
  gboolean   visible;

  g_object_get (config, pspec->name, &visible, NULL);
  g_object_set (wilber, "visible", visible, NULL);
}

static void
toolbox_tool_changed (GimpContext  *context,
                      GimpToolInfo *tool_info,
                      GimpToolbox  *toolbox)
{
  if (tool_info)
    {
      GtkWidget *tool_button = g_object_get_data (G_OBJECT (tool_info),
                                                  TOOL_BUTTON_DATA_KEY);

      if (tool_button &&
          ! gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (tool_button)))
        {
          g_signal_handlers_block_by_func (tool_button,
                                           toolbox_tool_button_toggled,
                                           toolbox);

          gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (tool_button),
                                             TRUE);

          g_signal_handlers_unblock_by_func (tool_button,
                                             toolbox_tool_button_toggled,
                                             toolbox);
        }
    }
}

static void
toolbox_tool_reorder (GimpContainer *container,
                      GimpToolInfo  *tool_info,
                      gint           index,
                      GimpToolbox   *toolbox)
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
toolbox_tool_visible_notify (GimpToolInfo *tool_info,
                             GParamSpec   *pspec,
                             GtkToolItem  *item)
{
  gtk_tool_item_set_visible_horizontal (item, tool_info->visible);
  gtk_tool_item_set_visible_vertical   (item, tool_info->visible);
}

static void
toolbox_tool_button_toggled (GtkWidget   *widget,
                             GimpToolbox *toolbox)
{
  GimpToolInfo *tool_info = g_object_get_data (G_OBJECT (widget),
                                               TOOL_INFO_DATA_KEY);

  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
    gimp_context_set_tool (gimp_toolbox_get_context (toolbox), tool_info);
}

static gboolean
toolbox_tool_button_press (GtkWidget      *widget,
                           GdkEventButton *event,
                           GimpToolbox    *toolbox)
{
  if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      gimp_dialog_factory_dialog_raise (toolbox->p->dialog_factory,
                                        gtk_widget_get_screen (widget),
                                        "gimp-tool-options",
                                        -1);
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
