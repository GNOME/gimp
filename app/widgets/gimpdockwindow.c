/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockwindow.c
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
 * Copyright (C)      2009 Martin Nordholts <martinn@src.gnome.org>
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

#include <gegl.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "dialogs/dialogs.h" /* FIXME, we are in the widget layer */

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontainer.h"
#include "core/gimplist.h"
#include "core/gimpimage.h"

#include "gimpcontainercombobox.h"
#include "gimpcontainerview.h"
#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpdockbook.h"
#include "gimpdockcolumns.h"
#include "gimpdockcontainer.h"
#include "gimpdockwindow.h"
#include "gimphelp-ids.h"
#include "gimpmenufactory.h"
#include "gimpsessioninfo-aux.h"
#include "gimpsessioninfo.h"
#include "gimpsessionmanaged.h"
#include "gimptoolbox.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"
#include "gimpwindow.h"

#include "gimp-intl.h"


#define DEFAULT_DOCK_HEIGHT          300
#define DEFAULT_MENU_VIEW_SIZE       GTK_ICON_SIZE_SMALL_TOOLBAR
#define AUX_INFO_SHOW_IMAGE_MENU     "show-image-menu"
#define AUX_INFO_FOLLOW_ACTIVE_IMAGE "follow-active-image"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_DIALOG_FACTORY,
  PROP_UI_MANAGER_NAME,
  PROP_IMAGE_CONTAINER,
  PROP_DISPLAY_CONTAINER,
  PROP_ALLOW_DOCKBOOK_ABSENCE
};


struct _GimpDockWindowPrivate
{
  GimpContext       *context;

  GimpDialogFactory *dialog_factory;

  gchar             *ui_manager_name;
  GimpUIManager     *ui_manager;
  GQuark             image_flush_handler_id;

  GimpDockColumns   *dock_columns;

  gboolean           allow_dockbook_absence;

  guint              update_title_idle_id;

  gint               ID;

  GimpContainer     *image_container;
  GimpContainer     *display_container;

  gboolean           show_image_menu;
  gboolean           auto_follow_active;

  GtkWidget         *image_combo;
  GtkWidget         *auto_button;
};


static void            gimp_dock_window_dock_container_iface_init (GimpDockContainerInterface *iface);
static void            gimp_dock_window_session_managed_iface_init(GimpSessionManagedInterface*iface);
static void            gimp_dock_window_constructed               (GObject                    *object);
static void            gimp_dock_window_dispose                   (GObject                    *object);
static void            gimp_dock_window_finalize                  (GObject                    *object);
static void            gimp_dock_window_set_property              (GObject                    *object,
                                                                   guint                       property_id,
                                                                   const GValue               *value,
                                                                   GParamSpec                 *pspec);
static void            gimp_dock_window_get_property              (GObject                    *object,
                                                                   guint                       property_id,
                                                                   GValue                     *value,
                                                                   GParamSpec                 *pspec);
static void            gimp_dock_window_style_updated             (GtkWidget                  *widget);
static gboolean        gimp_dock_window_delete_event              (GtkWidget                  *widget,
                                                                   GdkEventAny                *event);
static GList         * gimp_dock_window_get_docks                 (GimpDockContainer          *dock_container);
static GimpDialogFactory * gimp_dock_window_get_dialog_factory    (GimpDockContainer          *dock_container);
static GimpUIManager * gimp_dock_window_get_ui_manager            (GimpDockContainer          *dock_container);
static void            gimp_dock_window_add_dock_from_session     (GimpDockContainer          *dock_container,
                                                                   GimpDock                   *dock,
                                                                   GimpSessionInfoDock        *dock_info);
static GList         * gimp_dock_window_get_aux_info              (GimpSessionManaged         *session_managed);
static void            gimp_dock_window_set_aux_info              (GimpSessionManaged         *session_managed,
                                                                   GList                      *aux_info);
static GimpAlignmentType
                       gimp_dock_window_get_dock_side             (GimpDockContainer          *dock_container,
                                                                   GimpDock                   *dock);
static gboolean        gimp_dock_window_should_add_to_recent      (GimpDockWindow             *dock_window);
static void            gimp_dock_window_display_changed           (GimpDockWindow             *dock_window,
                                                                   GimpObject                 *display,
                                                                   GimpContext                *context);
static void            gimp_dock_window_image_changed             (GimpDockWindow             *dock_window,
                                                                   GimpImage                  *image,
                                                                   GimpContext                *context);
static void            gimp_dock_window_image_flush               (GimpImage                  *image,
                                                                   gboolean                    invalidate_preview,
                                                                   GimpDockWindow             *dock_window);
static void            gimp_dock_window_update_title              (GimpDockWindow             *dock_window);
static gboolean        gimp_dock_window_update_title_idle         (GimpDockWindow             *dock_window);
static gchar         * gimp_dock_window_get_description           (GimpDockWindow             *dock_window,
                                                                   gboolean                    complete);
static void            gimp_dock_window_dock_removed              (GimpDockWindow             *dock_window,
                                                                   GimpDock                   *dock,
                                                                   GimpDockColumns            *dock_columns);
static void            gimp_dock_window_factory_display_changed   (GimpContext                *context,
                                                                   GimpObject                 *display,
                                                                   GimpDock                   *dock);
static void            gimp_dock_window_factory_image_changed     (GimpContext                *context,
                                                                   GimpImage                  *image,
                                                                   GimpDock                   *dock);
static void            gimp_dock_window_auto_clicked              (GtkWidget                  *widget,
                                                                   GimpDock                   *dock);


G_DEFINE_TYPE_WITH_CODE (GimpDockWindow, gimp_dock_window, GIMP_TYPE_WINDOW,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCK_CONTAINER,
                                                gimp_dock_window_dock_container_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_SESSION_MANAGED,
                                                gimp_dock_window_session_managed_iface_init))

#define parent_class gimp_dock_window_parent_class


static void
gimp_dock_window_class_init (GimpDockWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = gimp_dock_window_constructed;
  object_class->dispose       = gimp_dock_window_dispose;
  object_class->finalize      = gimp_dock_window_finalize;
  object_class->set_property  = gimp_dock_window_set_property;
  object_class->get_property  = gimp_dock_window_get_property;

  widget_class->style_updated = gimp_dock_window_style_updated;
  widget_class->delete_event  = gimp_dock_window_delete_event;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DIALOG_FACTORY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_MANAGER_NAME,
                                   g_param_spec_string ("ui-manager-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_IMAGE_CONTAINER,
                                   g_param_spec_object ("image-container",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTAINER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DISPLAY_CONTAINER,
                                   g_param_spec_object ("display-container",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTAINER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ALLOW_DOCKBOOK_ABSENCE,
                                   g_param_spec_boolean ("allow-dockbook-absence",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));


  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("default-height",
                                                             NULL, NULL,
                                                             -1, G_MAXINT,
                                                             DEFAULT_DOCK_HEIGHT,
                                                             GIMP_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("menu-preview-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_MENU_VIEW_SIZE,
                                                              GIMP_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (GimpDockWindowPrivate));
}

static void
gimp_dock_window_init (GimpDockWindow *dock_window)
{
  static gint  dock_window_ID = 1;
  gchar       *name           = NULL;

  dock_window->p = G_TYPE_INSTANCE_GET_PRIVATE (dock_window,
                                                GIMP_TYPE_DOCK_WINDOW,
                                                GimpDockWindowPrivate);
  dock_window->p->ID                 = dock_window_ID++;
  dock_window->p->auto_follow_active = TRUE;

  name = g_strdup_printf ("gimp-dock-%d", dock_window->p->ID);
  gtk_widget_set_name (GTK_WIDGET (dock_window), name);
  g_free (name);

  gtk_window_set_resizable (GTK_WINDOW (dock_window), TRUE);
  gtk_window_set_focus_on_map (GTK_WINDOW (dock_window), FALSE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dock_window), FALSE);
}

static void
gimp_dock_window_dock_container_iface_init (GimpDockContainerInterface *iface)
{
  iface->get_docks          = gimp_dock_window_get_docks;
  iface->get_dialog_factory = gimp_dock_window_get_dialog_factory;
  iface->get_ui_manager     = gimp_dock_window_get_ui_manager;
  iface->add_dock           = gimp_dock_window_add_dock_from_session;
  iface->get_dock_side      = gimp_dock_window_get_dock_side;
}

static void
gimp_dock_window_session_managed_iface_init (GimpSessionManagedInterface *iface)
{
  iface->get_aux_info = gimp_dock_window_get_aux_info;
  iface->set_aux_info = gimp_dock_window_set_aux_info;
}

static void
gimp_dock_window_constructed (GObject *object)
{
  GimpDockWindow  *dock_window = GIMP_DOCK_WINDOW (object);
  GimpGuiConfig   *config;
  GimpContext     *factory_context;
  GimpMenuFactory *menu_factory;
  GtkAccelGroup   *accel_group;
  Gimp            *gimp;
  GtkSettings     *settings;
  gint             menu_view_width  = -1;
  gint             menu_view_height = -1;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp   = GIMP (dock_window->p->context->gimp);
  config = GIMP_GUI_CONFIG (gimp->config);

  /* Create a separate context per dock so that docks can be bound to
   * a specific image and does not necessarily have to follow the
   * active image in the user context
   */
  g_object_unref (dock_window->p->context);
  dock_window->p->context           = gimp_context_new (gimp, "Dock Context", NULL);
  dock_window->p->image_container   = gimp->images;
  dock_window->p->display_container = gimp->displays;

  factory_context =
    gimp_dialog_factory_get_context (dock_window->p->dialog_factory);

  /* Setup hints */
  gimp_window_set_hint (GTK_WINDOW (dock_window), config->dock_window_hint);

  menu_factory =
    gimp_dialog_factory_get_menu_factory (dock_window->p->dialog_factory);

  /* Make image window related keyboard shortcuts work also when a
   * dock window is the focused window
   */
  dock_window->p->ui_manager =
    gimp_menu_factory_manager_new (menu_factory,
                                   dock_window->p->ui_manager_name,
                                   dock_window,
                                   config->tearoff_menus);
  accel_group =
    gtk_ui_manager_get_accel_group (GTK_UI_MANAGER (dock_window->p->ui_manager));
  gtk_window_add_accel_group (GTK_WINDOW (dock_window), accel_group);

  g_signal_connect_object (dock_window->p->context, "display-changed",
                           G_CALLBACK (gimp_dock_window_display_changed),
                           dock_window,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (dock_window->p->context, "image-changed",
                           G_CALLBACK (gimp_dock_window_image_changed),
                           dock_window,
                           G_CONNECT_SWAPPED);

  dock_window->p->image_flush_handler_id =
    gimp_container_add_handler (gimp->images, "flush",
                                G_CALLBACK (gimp_dock_window_image_flush),
                                dock_window);

  gimp_context_define_properties (dock_window->p->context,
                                  GIMP_CONTEXT_PROP_MASK_ALL &
                                  ~(GIMP_CONTEXT_PROP_MASK_IMAGE |
                                    GIMP_CONTEXT_PROP_MASK_DISPLAY),
                                  FALSE);
  gimp_context_set_parent (dock_window->p->context,
                           factory_context);

  /* Setup widget hierarchy */
  {
    GtkWidget *vbox = NULL;

    /* Top-level GtkVBox */
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (dock_window), vbox);
    gtk_widget_show (vbox);

    /* Image selection menu */
    {
      GtkWidget *hbox = NULL;

      /* GtkHBox */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      if (dock_window->p->show_image_menu)
        gtk_widget_show (hbox);

      /* Image combo */
      dock_window->p->image_combo = gimp_container_combo_box_new (NULL, NULL, 16, 1);
      gtk_box_pack_start (GTK_BOX (hbox), dock_window->p->image_combo, TRUE, TRUE, 0);
      g_signal_connect (dock_window->p->image_combo, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dock_window->p->image_combo);
      gimp_help_set_help_data (dock_window->p->image_combo,
                               NULL, GIMP_HELP_DOCK_IMAGE_MENU);
      gtk_widget_show (dock_window->p->image_combo);

      /* Auto button */
      dock_window->p->auto_button = gtk_toggle_button_new_with_label (_("Auto"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dock_window->p->auto_button),
                                    dock_window->p->auto_follow_active);
      gtk_box_pack_start (GTK_BOX (hbox), dock_window->p->auto_button, FALSE, FALSE, 0);
      gtk_widget_show (dock_window->p->auto_button);

      g_signal_connect (dock_window->p->auto_button, "clicked",
                        G_CALLBACK (gimp_dock_window_auto_clicked),
                        dock_window);

      gimp_help_set_help_data (dock_window->p->auto_button,
                               _("When enabled, the dialog automatically "
                                 "follows the image you are working on."),
                               GIMP_HELP_DOCK_AUTO_BUTTON);
    }

    /* GimpDockColumns */
    /* Let the GimpDockColumns mirror the context so that a GimpDock can
     * get it when inside a dock window. We do the same thing in the
     * GimpImageWindow so docks can get the GimpContext there as well
     */
    dock_window->p->dock_columns =
      GIMP_DOCK_COLUMNS (gimp_dock_columns_new (dock_window->p->context,
                                                dock_window->p->dialog_factory,
                                                dock_window->p->ui_manager));
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (dock_window->p->dock_columns),
                        TRUE, TRUE, 0);
    gtk_widget_show (GTK_WIDGET (dock_window->p->dock_columns));
    g_signal_connect_object (dock_window->p->dock_columns, "dock-removed",
                             G_CALLBACK (gimp_dock_window_dock_removed),
                             dock_window,
                             G_CONNECT_SWAPPED);

    g_signal_connect_object (dock_window->p->dock_columns, "dock-added",
                             G_CALLBACK (gimp_dock_window_update_title),
                             dock_window,
                             G_CONNECT_SWAPPED);
    g_signal_connect_object (dock_window->p->dock_columns, "dock-removed",
                             G_CALLBACK (gimp_dock_window_update_title),
                             dock_window,
                             G_CONNECT_SWAPPED);
  }

  if (dock_window->p->auto_follow_active)
    {
      if (gimp_context_get_display (factory_context))
        gimp_context_copy_property (factory_context,
                                    dock_window->p->context,
                                    GIMP_CONTEXT_PROP_DISPLAY);
      else
        gimp_context_copy_property (factory_context,
                                    dock_window->p->context,
                                    GIMP_CONTEXT_PROP_IMAGE);
    }

  g_signal_connect_object (factory_context, "display-changed",
                           G_CALLBACK (gimp_dock_window_factory_display_changed),
                           dock_window,
                           0);
  g_signal_connect_object (factory_context, "image-changed",
                           G_CALLBACK (gimp_dock_window_factory_image_changed),
                           dock_window,
                           0);

  settings = gtk_widget_get_settings (GTK_WIDGET (dock_window));
  gtk_icon_size_lookup_for_settings (settings,
                                     DEFAULT_MENU_VIEW_SIZE,
                                     &menu_view_width,
                                     &menu_view_height);

  g_object_set (dock_window->p->image_combo,
                "container", dock_window->p->image_container,
                "context",   dock_window->p->context,
                NULL);

  gimp_help_connect (GTK_WIDGET (dock_window), gimp_standard_help_func,
                     GIMP_HELP_DOCK, NULL);

  if (dock_window->p->auto_follow_active)
    {
      if (gimp_context_get_display (factory_context))
        gimp_context_copy_property (factory_context,
                                    dock_window->p->context,
                                    GIMP_CONTEXT_PROP_DISPLAY);
      else
        gimp_context_copy_property (factory_context,
                                    dock_window->p->context,
                                    GIMP_CONTEXT_PROP_IMAGE);
    }
}

static void
gimp_dock_window_dispose (GObject *object)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (object);

  if (dock_window->p->update_title_idle_id)
    {
      g_source_remove (dock_window->p->update_title_idle_id);
      dock_window->p->update_title_idle_id = 0;
    }

  if (dock_window->p->image_flush_handler_id)
    {
      gimp_container_remove_handler (dock_window->p->context->gimp->images,
                                     dock_window->p->image_flush_handler_id);
      dock_window->p->image_flush_handler_id = 0;
    }

  if (dock_window->p->ui_manager)
    {
      g_object_unref (dock_window->p->ui_manager);
      dock_window->p->ui_manager = NULL;
    }

  if (dock_window->p->dialog_factory)
    {
      g_object_unref (dock_window->p->dialog_factory);
      dock_window->p->dialog_factory = NULL;
    }

  if (dock_window->p->context)
    {
      g_object_unref (dock_window->p->context);
      dock_window->p->context = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_dock_window_finalize (GObject *object)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (object);

  if (dock_window->p->ui_manager_name)
    {
      g_free (dock_window->p->ui_manager_name);
      dock_window->p->ui_manager_name = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dock_window_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      dock_window->p->context = g_value_dup_object (value);
      break;

    case PROP_DIALOG_FACTORY:
      dock_window->p->dialog_factory = g_value_dup_object (value);
      break;

    case PROP_UI_MANAGER_NAME:
      g_free (dock_window->p->ui_manager_name);
      dock_window->p->ui_manager_name = g_value_dup_string (value);
      break;

    case PROP_IMAGE_CONTAINER:
      dock_window->p->image_container = g_value_dup_object (value);
      break;

    case PROP_DISPLAY_CONTAINER:
      dock_window->p->display_container = g_value_dup_object (value);
      break;

    case PROP_ALLOW_DOCKBOOK_ABSENCE:
      dock_window->p->allow_dockbook_absence = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dock_window_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, dock_window->p->context);
      break;

    case PROP_DIALOG_FACTORY:
      g_value_set_object (value, dock_window->p->dialog_factory);
      break;

    case PROP_UI_MANAGER_NAME:
      g_value_set_string (value, dock_window->p->ui_manager_name);
      break;

    case PROP_IMAGE_CONTAINER:
      g_value_set_object (value, dock_window->p->image_container);
      break;

    case PROP_DISPLAY_CONTAINER:
      g_value_set_object (value, dock_window->p->display_container);
      break;

    case PROP_ALLOW_DOCKBOOK_ABSENCE:
      g_value_set_boolean (value, dock_window->p->allow_dockbook_absence);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dock_window_style_updated (GtkWidget *widget)
{
  GimpDockWindow  *dock_window      = GIMP_DOCK_WINDOW (widget);
  GtkStyleContext *button_style;
  GtkIconSize      menu_view_size;
  gint             default_height = DEFAULT_DOCK_HEIGHT;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (widget,
                        "default-height", &default_height,
                        "menu-preview-size", &menu_view_size,
                        NULL);

  gtk_window_set_default_size (GTK_WINDOW (widget), -1, default_height);

  if (dock_window->p->image_combo)
    {
      GtkSettings *settings;
      GtkBorder    border;
      gint         menu_view_width  = 18;
      gint         menu_view_height = 18;
      gint         focus_line_width;
      gint         focus_padding;

      settings = gtk_widget_get_settings (dock_window->p->image_combo);
      gtk_icon_size_lookup_for_settings (settings,
                                         menu_view_size,
                                         &menu_view_width,
                                         &menu_view_height);

      gtk_widget_style_get (dock_window->p->auto_button,
                            "focus-line-width", &focus_line_width,
                            "focus-padding",    &focus_padding,
                            NULL);

      button_style = gtk_widget_get_style_context (widget);
      gtk_style_context_get_border (button_style,
                                    gtk_widget_get_state_flags (widget),
                                    &border);

      gimp_container_view_set_view_size (GIMP_CONTAINER_VIEW (dock_window->p->image_combo),
                                         menu_view_height, 1);

      gtk_widget_set_size_request (dock_window->p->auto_button, -1,
                                   menu_view_height +
                                   2 * (1 /* CHILD_SPACING */ +
                                        border.top            +
                                        border.bottom         +
                                        focus_padding         +
                                        focus_line_width));
    }
}

/**
 * gimp_dock_window_delete_event:
 * @widget:
 * @event:
 *
 * Makes sure that when dock windows are closed they are added to the
 * list of recently closed docks so that they are easy to bring back.
 **/
static gboolean
gimp_dock_window_delete_event (GtkWidget   *widget,
                               GdkEventAny *event)
{
  GimpDockWindow         *dock_window = GIMP_DOCK_WINDOW (widget);
  GimpSessionInfo        *info        = NULL;
  const gchar            *entry_name  = NULL;
  GimpDialogFactoryEntry *entry       = NULL;
  gchar                  *name        = NULL;

  /* Don't add docks with just a singe dockable to the list of
   * recently closed dock since those can be brought back through the
   * normal Windows->Dockable Dialogs menu
   */
  if (! gimp_dock_window_should_add_to_recent (dock_window))
    return FALSE;

  info = gimp_session_info_new ();

  name = gimp_dock_window_get_description (dock_window, TRUE /*complete*/);
  gimp_object_set_name (GIMP_OBJECT (info), name);
  g_free (name);

  gimp_session_info_get_info_with_widget (info, GTK_WIDGET (dock_window));

  entry_name = (gimp_dock_window_has_toolbox (dock_window) ?
                "gimp-toolbox-window" :
                "gimp-dock-window");
  entry = gimp_dialog_factory_find_entry (dock_window->p->dialog_factory,
                                          entry_name);
  gimp_session_info_set_factory_entry (info, entry);

  gimp_container_add (global_recent_docks, GIMP_OBJECT (info));
  g_object_unref (info);

  return FALSE;
}

static GList *
gimp_dock_window_get_docks (GimpDockContainer *dock_container)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (dock_container);

  return g_list_copy (gimp_dock_columns_get_docks (dock_window->p->dock_columns));
}

static GimpDialogFactory *
gimp_dock_window_get_dialog_factory (GimpDockContainer *dock_container)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (dock_container);

  return dock_window->p->dialog_factory;
}

static GimpUIManager *
gimp_dock_window_get_ui_manager (GimpDockContainer *dock_container)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (dock_container);

  return dock_window->p->ui_manager;
}

static void
gimp_dock_window_add_dock_from_session (GimpDockContainer   *dock_container,
                                        GimpDock            *dock,
                                        GimpSessionInfoDock *dock_info)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (dock_container);

  gimp_dock_window_add_dock (dock_window,
                             dock,
                             -1 /*index*/);
}

static GList *
gimp_dock_window_get_aux_info (GimpSessionManaged *session_managed)
{
  GimpDockWindow     *dock_window = GIMP_DOCK_WINDOW (session_managed);
  GList              *aux_info    = NULL;
  GimpSessionInfoAux *aux;

  if (dock_window->p->allow_dockbook_absence)
    {
      /* Assume it is the toolbox; it does not have aux info */
      return NULL;
    }

  g_return_val_if_fail (GIMP_IS_DOCK_WINDOW (dock_window), NULL);

  aux = gimp_session_info_aux_new (AUX_INFO_SHOW_IMAGE_MENU,
                                   dock_window->p->show_image_menu ?
                                   "true" : "false");
  aux_info = g_list_append (aux_info, aux);

  aux = gimp_session_info_aux_new (AUX_INFO_FOLLOW_ACTIVE_IMAGE,
                                   dock_window->p->auto_follow_active ?
                                   "true" : "false");
  aux_info = g_list_append (aux_info, aux);

  return aux_info;
}

static void
gimp_dock_window_set_aux_info (GimpSessionManaged *session_managed,
                               GList              *aux_info)
{
  GimpDockWindow *dock_window;
  GList          *list;
  gboolean        menu_shown;
  gboolean        auto_follow;

  g_return_if_fail (GIMP_IS_DOCK_WINDOW (session_managed));

  dock_window = GIMP_DOCK_WINDOW (session_managed);
  menu_shown  = dock_window->p->show_image_menu;
  auto_follow = dock_window->p->auto_follow_active;

  for (list = aux_info; list; list = g_list_next (list))
    {
      GimpSessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_SHOW_IMAGE_MENU))
        {
          menu_shown = ! g_ascii_strcasecmp (aux->value, "true");
        }
      else if (! strcmp (aux->name, AUX_INFO_FOLLOW_ACTIVE_IMAGE))
        {
          auto_follow = ! g_ascii_strcasecmp (aux->value, "true");
        }
    }

  if (menu_shown != dock_window->p->show_image_menu)
    gimp_dock_window_set_show_image_menu (dock_window, menu_shown);

  if (auto_follow != dock_window->p->auto_follow_active)
    gimp_dock_window_set_auto_follow_active (dock_window, auto_follow);
}

static GimpAlignmentType
gimp_dock_window_get_dock_side (GimpDockContainer *dock_container,
                                GimpDock          *dock)
{
  g_return_val_if_fail (GIMP_IS_DOCK_WINDOW (dock_container), -1);
  g_return_val_if_fail (GIMP_IS_DOCK (dock), -1);

  /* A GimpDockWindow don't have docks on different sides, it's just
   * one set of columns
   */
  return -1;
}

/**
 * gimp_dock_window_should_add_to_recent:
 * @dock_window:
 *
 * Returns: %FALSE if the dock window can be recreated with one
 *          Windows menu item such as Windows->Toolbox or
 *          Windows->Dockable Dialogs->Layers, %TRUE if not. It should
 *          then be added to the list of recently closed docks.
 **/
static gboolean
gimp_dock_window_should_add_to_recent (GimpDockWindow *dock_window)
{
  GList    *docks;
  gboolean  should_add = TRUE;

  docks = gimp_dock_container_get_docks (GIMP_DOCK_CONTAINER (dock_window));

  if (! docks)
    {
      should_add = FALSE;
    }
  else if (g_list_length (docks) == 1)
    {
      GimpDock *dock = GIMP_DOCK (g_list_nth_data (docks, 0));

      if (GIMP_IS_TOOLBOX (dock) &&
          gimp_dock_get_n_dockables (dock) == 0)
        {
          should_add = FALSE;
        }
      else if (! GIMP_IS_TOOLBOX (dock) &&
               gimp_dock_get_n_dockables (dock) == 1)
        {
          should_add = FALSE;
        }
    }

  g_list_free (docks);

  return should_add;
}

static void
gimp_dock_window_image_flush (GimpImage      *image,
                              gboolean        invalidate_preview,
                              GimpDockWindow *dock_window)
{
  if (image == gimp_context_get_image (dock_window->p->context))
    {
      GimpObject *display = gimp_context_get_display (dock_window->p->context);

      if (display)
        gimp_ui_manager_update (dock_window->p->ui_manager, display);
    }
}

static void
gimp_dock_window_update_title (GimpDockWindow *dock_window)
{
  if (dock_window->p->update_title_idle_id)
    g_source_remove (dock_window->p->update_title_idle_id);

  dock_window->p->update_title_idle_id =
    g_idle_add ((GSourceFunc) gimp_dock_window_update_title_idle,
                dock_window);
}

static gboolean
gimp_dock_window_update_title_idle (GimpDockWindow *dock_window)
{
  gchar *desc = gimp_dock_window_get_description (dock_window,
                                                  FALSE /*complete*/);
  if (desc)
    {
      gtk_window_set_title (GTK_WINDOW (dock_window), desc);
      g_free (desc);
    }

  dock_window->p->update_title_idle_id = 0;

  return FALSE;
}

static gchar *
gimp_dock_window_get_description (GimpDockWindow *dock_window,
                                  gboolean        complete)
{
  GString *complete_desc = g_string_new (NULL);
  GList   *docks         = NULL;
  GList   *iter          = NULL;

  docks = gimp_dock_container_get_docks (GIMP_DOCK_CONTAINER (dock_window));

  for (iter = docks;
       iter;
       iter = g_list_next (iter))
    {
      gchar *desc = gimp_dock_get_description (GIMP_DOCK (iter->data), complete);
      g_string_append (complete_desc, desc);
      g_free (desc);

      if (g_list_next (iter))
        g_string_append (complete_desc, GIMP_DOCK_COLUMN_SEPARATOR);
    }

  g_list_free (docks);

  return g_string_free (complete_desc, FALSE /*free_segment*/);
}

static void
gimp_dock_window_dock_removed (GimpDockWindow  *dock_window,
                               GimpDock        *dock,
                               GimpDockColumns *dock_columns)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));

  if (gimp_dock_columns_get_docks (dock_columns) == NULL &&
      ! dock_window->p->allow_dockbook_absence)
    gtk_widget_destroy (GTK_WIDGET (dock_window));
}

static void
gimp_dock_window_factory_display_changed (GimpContext *context,
                                          GimpObject  *display,
                                          GimpDock    *dock)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (dock);

  if (display && dock_window->p->auto_follow_active)
    gimp_context_set_display (dock_window->p->context, display);
}

static void
gimp_dock_window_factory_image_changed (GimpContext *context,
                                        GimpImage   *image,
                                        GimpDock    *dock)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (dock);

  /*  won't do anything if we already set the display above  */
  if (image && dock_window->p->auto_follow_active)
    gimp_context_set_image (dock_window->p->context, image);
}

static void
gimp_dock_window_display_changed (GimpDockWindow *dock_window,
                                  GimpObject     *display,
                                  GimpContext    *context)
{
  /*  make sure auto-follow-active works both ways  */
  if (display && dock_window->p->auto_follow_active)
    {
      GimpContext *factory_context =
        gimp_dialog_factory_get_context (dock_window->p->dialog_factory);

      gimp_context_set_display (factory_context, display);
    }

  gimp_ui_manager_update (dock_window->p->ui_manager,
                          display);
}

static void
gimp_dock_window_image_changed (GimpDockWindow *dock_window,
                                GimpImage      *image,
                                GimpContext    *context)
{
  GimpContainer *image_container   = dock_window->p->image_container;
  GimpContainer *display_container = dock_window->p->display_container;

  /*  make sure auto-follow-active works both ways  */
  if (image && dock_window->p->auto_follow_active)
    {
      GimpContext *factory_context =
        gimp_dialog_factory_get_context (dock_window->p->dialog_factory);

      gimp_context_set_image (factory_context, image);
    }

  if (image == NULL && ! gimp_container_is_empty (image_container))
    {
      image = GIMP_IMAGE (gimp_container_get_first_child (image_container));

      /*  this invokes this function recursively but we don't enter
       *  the if() branch the second time
       */
      gimp_context_set_image (context, image);

      /*  stop the emission of the original signal (the emission of
       *  the recursive signal is finished)
       */
      g_signal_stop_emission_by_name (context, "image-changed");
    }
  else if (image != NULL && ! gimp_container_is_empty (display_container))
    {
      GimpObject *display;
      GimpImage  *display_image;
      gboolean    find_display = TRUE;

      display = gimp_context_get_display (context);

      if (display)
        {
          g_object_get (display, "image", &display_image, NULL);

          if (display_image)
            {
              g_object_unref (display_image);

              if (display_image == image)
                find_display = FALSE;
            }
        }

      if (find_display)
        {
          GList *list;

          for (list = GIMP_LIST (display_container)->queue->head;
               list;
               list = g_list_next (list))
            {
              display = GIMP_OBJECT (list->data);

              g_object_get (display, "image", &display_image, NULL);

              if (display_image)
                {
                  g_object_unref (display_image);

                  if (display_image == image)
                    {
                      /*  this invokes this function recursively but we
                       *  don't enter the if(find_display) branch the
                       *  second time
                       */
                      gimp_context_set_display (context, display);

                      /*  don't stop signal emission here because the
                       *  context's image was not changed by the
                       *  recursive call
                       */
                      break;
                    }
                }
            }
        }
    }

  gimp_ui_manager_update (dock_window->p->ui_manager,
                          gimp_context_get_display (context));
}

static void
gimp_dock_window_auto_clicked (GtkWidget *widget,
                               GimpDock  *dock)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (dock);

  gimp_toggle_button_update (widget, &dock_window->p->auto_follow_active);

  if (dock_window->p->auto_follow_active)
    {
      GimpContext *context;

      context = gimp_dialog_factory_get_context (dock_window->p->dialog_factory);

      gimp_context_copy_properties (context,
                                    dock_window->p->context,
                                    GIMP_CONTEXT_PROP_MASK_DISPLAY |
                                    GIMP_CONTEXT_PROP_MASK_IMAGE);
    }
}


void
gimp_dock_window_add_dock (GimpDockWindow *dock_window,
                           GimpDock       *dock,
                           gint            index)
{
  g_return_if_fail (GIMP_IS_DOCK_WINDOW (dock_window));
  g_return_if_fail (GIMP_IS_DOCK (dock));

  gimp_dock_columns_add_dock (dock_window->p->dock_columns,
                              GIMP_DOCK (dock),
                              index);

  g_signal_connect_object (dock, "description-invalidated",
                           G_CALLBACK (gimp_dock_window_update_title),
                           dock_window,
                           G_CONNECT_SWAPPED);

  /* Some docks like the toolbox dock needs to maintain special hints
   * on its container GtkWindow, allow those to do so
   */
  gimp_dock_set_host_geometry_hints (dock, GTK_WINDOW (dock_window));
  g_signal_connect_object (dock, "geometry-invalidated",
                           G_CALLBACK (gimp_dock_set_host_geometry_hints),
                           dock_window, 0);
}

void
gimp_dock_window_remove_dock (GimpDockWindow *dock_window,
                              GimpDock       *dock)
{
  gimp_dock_columns_remove_dock (dock_window->p->dock_columns,
                                 GIMP_DOCK (dock));

  g_signal_handlers_disconnect_by_func (dock,
                                        gimp_dock_window_update_title,
                                        dock_window);
  g_signal_handlers_disconnect_by_func (dock,
                                        gimp_dock_set_host_geometry_hints,
                                        dock_window);
}

GtkWidget *
gimp_dock_window_new (const gchar       *role,
                      const gchar       *ui_manager_name,
                      gboolean           allow_dockbook_absence,
                      GimpDialogFactory *dialog_factory,
                      GimpContext       *context)
{
  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return g_object_new (GIMP_TYPE_DOCK_WINDOW,
                       "role",                   role,
                       "ui-manager-name",        ui_manager_name,
                       "allow-dockbook-absence", allow_dockbook_absence,
                       "dialog-factory",         dialog_factory,
                       "context",                context,
                       NULL);
}

gint
gimp_dock_window_get_id (GimpDockWindow *dock_window)
{
  g_return_val_if_fail (GIMP_IS_DOCK_WINDOW (dock_window), 0);

  return dock_window->p->ID;
}

GimpContext *
gimp_dock_window_get_context (GimpDockWindow *dock_window)
{
  g_return_val_if_fail (GIMP_IS_DOCK_WINDOW (dock_window), NULL);

  return dock_window->p->context;
}

gboolean
gimp_dock_window_get_auto_follow_active (GimpDockWindow *dock_window)
{
  g_return_val_if_fail (GIMP_IS_DOCK_WINDOW (dock_window), FALSE);

  return dock_window->p->auto_follow_active;
}

void
gimp_dock_window_set_auto_follow_active (GimpDockWindow *dock_window,
                                         gboolean        auto_follow_active)
{
  g_return_if_fail (GIMP_IS_DOCK_WINDOW (dock_window));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dock_window->p->auto_button),
                                auto_follow_active ? TRUE : FALSE);
}

gboolean
gimp_dock_window_get_show_image_menu (GimpDockWindow *dock_window)
{
  g_return_val_if_fail (GIMP_IS_DOCK_WINDOW (dock_window), FALSE);

  return dock_window->p->show_image_menu;
}

void
gimp_dock_window_set_show_image_menu (GimpDockWindow *dock_window,
                                      gboolean        show)
{
  GtkWidget *parent;

  g_return_if_fail (GIMP_IS_DOCK_WINDOW (dock_window));

  parent = gtk_widget_get_parent (dock_window->p->image_combo);

  gtk_widget_set_visible (parent, show);

  dock_window->p->show_image_menu = show ? TRUE : FALSE;
}

void
gimp_dock_window_setup (GimpDockWindow *dock_window,
                        GimpDockWindow *template)
{
  gimp_dock_window_set_auto_follow_active (GIMP_DOCK_WINDOW (dock_window),
                                           template->p->auto_follow_active);
  gimp_dock_window_set_show_image_menu (GIMP_DOCK_WINDOW (dock_window),
                                        template->p->show_image_menu);
}

/**
 * gimp_dock_window_has_toolbox:
 * @dock_window:
 *
 * Returns: %TRUE if the dock window has a GimpToolbox dock, %FALSE
 * otherwise.
 **/
gboolean
gimp_dock_window_has_toolbox (GimpDockWindow *dock_window)
{
  GList *iter = NULL;

  g_return_val_if_fail (GIMP_IS_DOCK_WINDOW (dock_window), FALSE);

  for (iter = gimp_dock_columns_get_docks (dock_window->p->dock_columns);
       iter;
       iter = g_list_next (iter))
    {
      if (GIMP_IS_TOOLBOX (iter->data))
        return TRUE;
    }

  return FALSE;
}


/**
 * gimp_dock_window_from_dock:
 * @dock:
 *
 * For convenience.
 *
 * Returns: If the toplevel widget for the dock is a GimpDockWindow,
 * return that. Otherwise return %NULL.
 **/
GimpDockWindow *
gimp_dock_window_from_dock (GimpDock *dock)
{
  GtkWidget *toplevel = NULL;

  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (dock));

  if (GIMP_IS_DOCK_WINDOW (toplevel))
    return GIMP_DOCK_WINDOW (toplevel);
  else
    return NULL;
}
