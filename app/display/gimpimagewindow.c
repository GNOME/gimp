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

#include <math.h>

#include <gegl.h>
#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <windef.h>
#include <winbase.h>
#include <windows.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"
#include "core/gimpcontainer.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdeviceinfo.h"
#include "widgets/gimpdevicemanager.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdockcolumns.h"
#include "widgets/gimpdockcontainer.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenubar.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimpsessioninfo-aux.h"
#include "widgets/gimpsessionmanaged.h"
#include "widgets/gimpsessioninfo-dock.h"
#include "widgets/gimptoolbox.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpview.h"
#include "widgets/gimpviewrenderer.h"
#include "widgets/gimpwidgets-utils.h"

#include "menus/menus.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-close.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-tool-events.h"
#include "gimpdisplayshell-transform.h"
#include "gimpimagewindow.h"
#include "gimpstatusbar.h"

#include "gimp-log.h"
#include "gimp-priorities.h"

#include "gimp-intl.h"


#define GIMP_EMPTY_IMAGE_WINDOW_ENTRY_ID   "gimp-empty-image-window"
#define GIMP_SINGLE_IMAGE_WINDOW_ENTRY_ID  "gimp-single-image-window"

/* The width of the left and right dock areas */
#define GIMP_IMAGE_WINDOW_LEFT_DOCKS_WIDTH  "left-docks-width"
#define GIMP_IMAGE_WINDOW_RIGHT_DOCKS_WIDTH "right-docks-width"

/* deprecated property: GtkPaned position of the right docks area */
#define GIMP_IMAGE_WINDOW_RIGHT_DOCKS_POS  "right-docks-position"

/* Whether the window's maximized or not */
#define GIMP_IMAGE_WINDOW_MAXIMIZED        "maximized"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_DIALOG_FACTORY,
  PROP_INITIAL_MONITOR
};


typedef struct _GimpImageWindowPrivate GimpImageWindowPrivate;

struct _GimpImageWindowPrivate
{
  Gimp              *gimp;
  GimpDialogFactory *dialog_factory;

  GList             *shells;
  GimpDisplayShell  *active_shell;

  GimpMenuModel     *menubar_model;

  GtkWidget         *main_vbox;
  GtkWidget         *menubar;
  GtkWidget         *hbox;
  GtkWidget         *left_hpane;
  GtkWidget         *left_docks;
  GtkWidget         *right_hpane;
  GtkWidget         *notebook;
  GtkWidget         *right_docks;

  GHashTable        *pad_controllers;

  GdkWindowState     window_state;

  const gchar       *entry_id;

  GdkMonitor        *initial_monitor;

  gint               scale_factor;

  gint               suspend_keep_pos;

  gint               update_ui_manager_idle_id;
};

typedef struct
{
  gint canvas_x;
  gint canvas_y;
  gint window_x;
  gint window_y;
} PosCorrectionData;


#define GIMP_IMAGE_WINDOW_GET_PRIVATE(window) \
        ((GimpImageWindowPrivate *) gimp_image_window_get_instance_private ((GimpImageWindow *) (window)))


/*  local function prototypes  */

static void      gimp_image_window_dock_container_iface_init
                                                       (GimpDockContainerInterface
                                                                            *iface);
static void      gimp_image_window_session_managed_iface_init
                                                       (GimpSessionManagedInterface
                                                                            *iface);
static void      gimp_image_window_constructed         (GObject             *object);
static void      gimp_image_window_dispose             (GObject             *object);
static void      gimp_image_window_finalize            (GObject             *object);
static void      gimp_image_window_set_property        (GObject             *object,
                                                        guint                property_id,
                                                        const GValue        *value,
                                                        GParamSpec          *pspec);
static void      gimp_image_window_get_property        (GObject             *object,
                                                        guint                property_id,
                                                        GValue              *value,
                                                        GParamSpec          *pspec);

static gboolean  gimp_image_window_delete_event        (GtkWidget           *widget,
                                                        GdkEventAny         *event);
static gboolean  gimp_image_window_configure_event     (GtkWidget           *widget,
                                                        GdkEventConfigure   *event);
static gboolean  gimp_image_window_window_state_event  (GtkWidget           *widget,
                                                        GdkEventWindowState *event);
static void      gimp_image_window_style_updated       (GtkWidget           *widget);

static void      gimp_image_window_monitor_changed     (GimpWindow          *window,
                                                        GdkMonitor          *monitor);

static GList *   gimp_image_window_get_docks           (GimpDockContainer   *dock_container);
static GimpDialogFactory *
                 gimp_image_window_dock_container_get_dialog_factory
                                                       (GimpDockContainer   *dock_container);
static GimpUIManager *
                 gimp_image_window_dock_container_get_ui_manager
                                                       (GimpDockContainer   *dock_container);
static void      gimp_image_window_add_dock            (GimpDockContainer   *dock_container,
                                                        GimpDock            *dock,
                                                        GimpSessionInfoDock *dock_info);
static GimpAlignmentType
                 gimp_image_window_get_dock_side       (GimpDockContainer   *dock_container,
                                                        GimpDock            *dock);
static GList   * gimp_image_window_get_aux_info        (GimpSessionManaged  *session_managed);
static void      gimp_image_window_set_aux_info        (GimpSessionManaged  *session_managed,
                                                        GList               *aux_info);

static void      gimp_image_window_config_notify       (GimpImageWindow     *window,
                                                        GParamSpec          *pspec,
                                                        GimpGuiConfig       *config);
static void      gimp_image_window_session_clear       (GimpImageWindow     *window);
static void      gimp_image_window_session_apply       (GimpImageWindow     *window,
                                                        const gchar         *entry_id,
                                                        GdkMonitor          *monitor);
static void      gimp_image_window_session_update      (GimpImageWindow     *window,
                                                        GimpDisplay         *new_display,
                                                        const gchar         *new_entry_id,
                                                        GdkMonitor          *monitor);
static const gchar *
                 gimp_image_window_config_to_entry_id  (GimpGuiConfig       *config);
static void      gimp_image_window_show_tooltip        (GimpUIManager       *manager,
                                                        const gchar         *tooltip,
                                                        GimpImageWindow     *window);
static void      gimp_image_window_hide_tooltip        (GimpUIManager       *manager,
                                                        GimpImageWindow     *window);
static gboolean  gimp_image_window_update_ui_manager_idle
                                                       (GimpImageWindow     *window);
static void      gimp_image_window_update_ui_manager   (GimpImageWindow     *window);

static void      gimp_image_window_shell_size_allocate (GimpDisplayShell    *shell,
                                                        GtkAllocation       *allocation,
                                                        PosCorrectionData   *data);
static gboolean  gimp_image_window_shell_events        (GtkWidget           *widget,
                                                        GdkEvent            *event,
                                                        GimpImageWindow     *window);

static void      gimp_image_window_switch_page         (GtkNotebook         *notebook,
                                                        gpointer             page,
                                                        gint                 page_num,
                                                        GimpImageWindow     *window);
static void      gimp_image_window_page_removed        (GtkNotebook         *notebook,
                                                        GtkWidget           *widget,
                                                        gint                 page_num,
                                                        GimpImageWindow     *window);
static void      gimp_image_window_page_reordered      (GtkNotebook         *notebook,
                                                        GtkWidget           *widget,
                                                        gint                 page_num,
                                                        GimpImageWindow     *window);
static void      gimp_image_window_disconnect_from_active_shell
                                                       (GimpImageWindow *window);

static void      gimp_image_window_image_notify        (GimpDisplay         *display,
                                                        const GParamSpec    *pspec,
                                                        GimpImageWindow     *window);
static void      gimp_image_window_shell_scaled        (GimpDisplayShell    *shell,
                                                        GimpImageWindow     *window);
static void      gimp_image_window_shell_rotated       (GimpDisplayShell    *shell,
                                                        GimpImageWindow     *window);
static void      gimp_image_window_shell_title_notify  (GimpDisplayShell    *shell,
                                                        const GParamSpec    *pspec,
                                                        GimpImageWindow     *window);
static GtkWidget *
                 gimp_image_window_create_tab_label    (GimpImageWindow     *window,
                                                        GimpDisplayShell    *shell);
static void      gimp_image_window_update_tab_labels   (GimpImageWindow     *window);

static void      gimp_image_window_configure_pad       (GimpImageWindow     *window,
                                                        GimpDeviceInfo      *info);
static void      gimp_image_window_init_pad_foreach    (GimpDeviceInfo      *info,
                                                        GimpImageWindow     *window);


G_DEFINE_TYPE_WITH_CODE (GimpImageWindow, gimp_image_window, GIMP_TYPE_WINDOW,
                         G_ADD_PRIVATE (GimpImageWindow)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCK_CONTAINER,
                                                gimp_image_window_dock_container_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_SESSION_MANAGED,
                                                gimp_image_window_session_managed_iface_init))

#define parent_class gimp_image_window_parent_class


static void
gimp_image_window_class_init (GimpImageWindowClass *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass  *widget_class = GTK_WIDGET_CLASS (klass);
  GimpWindowClass *window_class = GIMP_WINDOW_CLASS (klass);

  object_class->constructed        = gimp_image_window_constructed;
  object_class->dispose            = gimp_image_window_dispose;
  object_class->finalize           = gimp_image_window_finalize;
  object_class->set_property       = gimp_image_window_set_property;
  object_class->get_property       = gimp_image_window_get_property;

  widget_class->delete_event       = gimp_image_window_delete_event;
  widget_class->configure_event    = gimp_image_window_configure_event;
  widget_class->window_state_event = gimp_image_window_window_state_event;
  widget_class->style_updated      = gimp_image_window_style_updated;

  window_class->monitor_changed    = gimp_image_window_monitor_changed;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DIALOG_FACTORY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_INITIAL_MONITOR,
                                   g_param_spec_object ("initial-monitor",
                                                        NULL, NULL,
                                                        GDK_TYPE_MONITOR,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_image_window_init (GimpImageWindow *window)
{
  static gint  role_serial = 1;
  gchar       *role;

  role = g_strdup_printf ("gimp-image-window-%d", role_serial++);
  gtk_window_set_role (GTK_WINDOW (window), role);
  g_free (role);

  gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
}

static void
gimp_image_window_dock_container_iface_init (GimpDockContainerInterface *iface)
{
  iface->get_docks          = gimp_image_window_get_docks;
  iface->get_dialog_factory = gimp_image_window_dock_container_get_dialog_factory;
  iface->get_ui_manager     = gimp_image_window_dock_container_get_ui_manager;
  iface->add_dock           = gimp_image_window_add_dock;
  iface->get_dock_side      = gimp_image_window_get_dock_side;
}

static void
gimp_image_window_session_managed_iface_init (GimpSessionManagedInterface *iface)
{
  iface->get_aux_info = gimp_image_window_get_aux_info;
  iface->set_aux_info = gimp_image_window_set_aux_info;
}

static void
gimp_image_window_constructed (GObject *object)
{
  GimpImageWindow        *window  = GIMP_IMAGE_WINDOW (object);
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GimpUIManager          *menubar_manager;
  GimpGuiConfig          *config;
  GimpDeviceManager      *device_manager;
  gboolean                use_gtk_menubar = TRUE;
  gboolean                use_app_menu    = TRUE;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (private->gimp));
  gimp_assert (GIMP_IS_DIALOG_FACTORY (private->dialog_factory));

  g_signal_connect_object (private->dialog_factory, "dock-window-added",
                           G_CALLBACK (gimp_image_window_update_ui_manager),
                           window, G_CONNECT_SWAPPED);
  g_signal_connect_object (private->dialog_factory, "dock-window-removed",
                           G_CALLBACK (gimp_image_window_update_ui_manager),
                           window, G_CONNECT_SWAPPED);

  menubar_manager = menus_get_image_manager_singleton (private->gimp);
  g_signal_connect (menubar_manager, "show-tooltip",
                    G_CALLBACK (gimp_image_window_show_tooltip),
                    window);
  g_signal_connect (menubar_manager, "hide-tooltip",
                    G_CALLBACK (gimp_image_window_hide_tooltip),
                    window);

  config = GIMP_GUI_CONFIG (private->gimp->config);

  /* Create the window toplevel container */
  private->main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (window), private->main_vbox);
  gtk_widget_show (private->main_vbox);

  /* Create the menubar */
  private->menubar_model = gimp_ui_manager_get_model (menubar_manager,
                                                      "/image-menubar");

#ifndef GDK_WINDOWING_QUARTZ
  /* macOS has its native menubar system, which is implemented by
   * gtk_application_set_menubar(). Unfortunately the GTK menubar feature
   * doesn't support tooltips (though for macOS, having "native" application
   * menus was deemed worth the feature loss).
   * As a special-case, we allow testing the GTK menubar through setting the
   * environment variable GIMP_GTK_MENUBAR on non-macOS platforms.
   */
  use_gtk_menubar = (g_getenv ("GIMP_GTK_MENUBAR") != NULL);
#endif /* !GDK_WINDOWING_QUARTZ */

  if (use_gtk_menubar)
    {
      gtk_application_set_menubar (GTK_APPLICATION (private->gimp->app),
                                   G_MENU_MODEL (private->menubar_model));
    }
  else
    {
      private->menubar = gimp_menu_bar_new (private->menubar_model, menubar_manager);

      /*  make sure we can activate accels even if the menubar is invisible
       *  (see https://bugzilla.gnome.org/show_bug.cgi?id=137151)
       */
      g_signal_connect (private->menubar, "can-activate-accel",
                        G_CALLBACK (gtk_true),
                        NULL);

      /*  active display callback  */
      g_signal_connect (private->menubar, "button-press-event",
                        G_CALLBACK (gimp_image_window_shell_events),
                        window);
      g_signal_connect (private->menubar, "button-release-event",
                        G_CALLBACK (gimp_image_window_shell_events),
                        window);
      g_signal_connect (private->menubar, "key-press-event",
                        G_CALLBACK (gimp_image_window_shell_events),
                        window);

      if (config->custom_title_bar)
        {
          GtkWidget *headerbar;

          headerbar = gtk_header_bar_new ();
          gtk_window_set_titlebar (GTK_WINDOW (window), headerbar);

          /* This is important to turn off space reservation for the subtitle
           * (resulting in too high header bar).
           */
          gtk_header_bar_set_has_subtitle (GTK_HEADER_BAR (headerbar), FALSE);

          gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (headerbar), TRUE);
          gtk_header_bar_pack_start (GTK_HEADER_BAR (headerbar), private->menubar);
          gtk_widget_show (headerbar);

          /* XXX There are competing propositions on how the title should be
           * aligned. GTK is trying to center it (relatively to the window) as
           * much as it can. But some people seem to thing it should be centered
           * relatively to the remaining empty space after the menu.
           * I am personally unsure so leaving GTK's defaults for now.
           */
        }
      else
        {
          gtk_box_pack_start (GTK_BOX (private->main_vbox),
                              private->menubar, FALSE, FALSE, 0);
        }
    }

#ifndef GDK_WINDOWING_QUARTZ
  /* Docs says that macOS always returns FALSE but we actually want to create
   * our custom macOS menu.
   */
  use_app_menu = gtk_application_prefers_app_menu (GTK_APPLICATION (private->gimp->app));
#endif /* !GDK_WINDOWING_QUARTZ */

  if (use_app_menu)
    {
      GimpUIManager *app_menu_manager;
      GimpMenuModel *app_menu_model;

      app_menu_manager = gimp_menu_factory_get_manager (menus_get_global_menu_factory (private->gimp),
                                                        "<AppMenu>", private->gimp);
      app_menu_model   = gimp_ui_manager_get_model (app_menu_manager, "/app-menu");
      gtk_application_set_app_menu (GTK_APPLICATION (private->gimp->app),
                                    G_MENU_MODEL (app_menu_model));
    }

  /* Create the hbox that contains docks and images */
  private->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (private->main_vbox), private->hbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (private->hbox);

  /* Create the left pane */
  private->left_hpane = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_wide_handle (GTK_PANED (private->left_hpane), TRUE);
  gtk_box_pack_start (GTK_BOX (private->hbox), private->left_hpane,
                      TRUE, TRUE, 0);
  gtk_widget_show (private->left_hpane);

  /* Create the left dock columns widget */
  private->left_docks =
    gimp_dock_columns_new (gimp_get_user_context (private->gimp),
                           private->dialog_factory,
                           menubar_manager);
  gtk_paned_pack1 (GTK_PANED (private->left_hpane), private->left_docks,
                   FALSE, FALSE);
  gtk_widget_set_visible (private->left_docks, config->single_window_mode);

  /* Create the right pane */
  private->right_hpane = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_wide_handle (GTK_PANED (private->right_hpane), TRUE);
  gtk_paned_pack2 (GTK_PANED (private->left_hpane), private->right_hpane,
                   TRUE, FALSE);
  gtk_widget_show (private->right_hpane);

  /* Create notebook that contains images */
  private->notebook = gtk_notebook_new ();
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (private->notebook), TRUE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (private->notebook), FALSE);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (private->notebook), FALSE);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (private->notebook), GTK_POS_TOP);

  gtk_paned_pack1 (GTK_PANED (private->right_hpane), private->notebook,
                   TRUE, TRUE);
  g_signal_connect (private->notebook, "switch-page",
                    G_CALLBACK (gimp_image_window_switch_page),
                    window);
  g_signal_connect (private->notebook, "page-removed",
                    G_CALLBACK (gimp_image_window_page_removed),
                    window);
  g_signal_connect (private->notebook, "page-reordered",
                    G_CALLBACK (gimp_image_window_page_reordered),
                    window);
  gtk_widget_show (private->notebook);

  /* Create the right dock columns widget */
  private->right_docks =
    gimp_dock_columns_new (gimp_get_user_context (private->gimp),
                           private->dialog_factory,
                           menubar_manager);
  gtk_paned_pack2 (GTK_PANED (private->right_hpane), private->right_docks,
                   FALSE, FALSE);
  gtk_widget_set_visible (private->right_docks, config->single_window_mode);

  g_signal_connect_object (config, "notify::single-window-mode",
                           G_CALLBACK (gimp_image_window_config_notify),
                           window, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::show-tabs",
                           G_CALLBACK (gimp_image_window_config_notify),
                           window, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::hide-docks",
                           G_CALLBACK (gimp_image_window_config_notify),
                           window, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::tabs-position",
                           G_CALLBACK (gimp_image_window_config_notify),
                           window, G_CONNECT_SWAPPED);

  gimp_image_window_session_update (window,
                                    NULL /*new_display*/,
                                    gimp_image_window_config_to_entry_id (config),
                                    private->initial_monitor);

  private->pad_controllers =
    g_hash_table_new_full (NULL, NULL, NULL, g_object_unref);

  device_manager = gimp_devices_get_manager (private->gimp);
  g_signal_connect_object (device_manager, "configure-pad",
                           G_CALLBACK (gimp_image_window_configure_pad),
                           window, G_CONNECT_SWAPPED);
  gimp_container_foreach (GIMP_CONTAINER (device_manager),
                          (GFunc) gimp_image_window_init_pad_foreach,
                          window);
}

static void
gimp_image_window_dispose (GObject *object)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (object);

  if (private->dialog_factory)
    {
      g_signal_handlers_disconnect_by_func (private->dialog_factory,
                                            gimp_image_window_update_ui_manager,
                                            object);
      private->dialog_factory = NULL;
    }

  if (private->update_ui_manager_idle_id)
    {
      g_source_remove (private->update_ui_manager_idle_id);
      private->update_ui_manager_idle_id = 0;
    }

  g_clear_pointer (&private->pad_controllers, g_hash_table_unref);
  g_clear_object (&private->menubar_model);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_image_window_finalize (GObject *object)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (object);

  if (private->shells)
    {
      g_list_free (private->shells);
      private->shells = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_image_window_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpImageWindow        *window  = GIMP_IMAGE_WINDOW (object);
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  switch (property_id)
    {
    case PROP_GIMP:
      private->gimp = g_value_get_object (value);
      break;
    case PROP_DIALOG_FACTORY:
      private->dialog_factory = g_value_get_object (value);
      break;
    case PROP_INITIAL_MONITOR:
      private->initial_monitor = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_window_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpImageWindow        *window  = GIMP_IMAGE_WINDOW (object);
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, private->gimp);
      break;
    case PROP_DIALOG_FACTORY:
      g_value_set_object (value, private->dialog_factory);
      break;
    case PROP_INITIAL_MONITOR:
      g_value_set_object (value, private->initial_monitor);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_image_window_delete_event (GtkWidget   *widget,
                                GdkEventAny *event)
{
  GimpImageWindow        *window  = GIMP_IMAGE_WINDOW (widget);
  GimpDisplayShell       *shell   = gimp_image_window_get_active_shell (window);
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GimpGuiConfig          *config  = GIMP_GUI_CONFIG (private->gimp->config);

  if (config->single_window_mode)
    gimp_ui_manager_activate_action (menus_get_image_manager_singleton (private->gimp),
                                     "file", "file-quit");
  else if (shell)
    gimp_display_shell_close (shell, FALSE);

  return TRUE;
}

static gboolean
gimp_image_window_configure_event (GtkWidget         *widget,
                                   GdkEventConfigure *event)
{
  GimpImageWindow        *window  = GIMP_IMAGE_WINDOW (widget);
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GtkAllocation           allocation;
  gint                    current_width;
  gint                    current_height;
  gint                    scale_factor;

  gtk_widget_get_allocation (widget, &allocation);

  /* Grab the size before we run the parent implementation */
  current_width  = allocation.width;
  current_height = allocation.height;

  /* Run the parent implementation */
  if (GTK_WIDGET_CLASS (parent_class)->configure_event)
    GTK_WIDGET_CLASS (parent_class)->configure_event (widget, event);

  /* If the window size has changed, make sure additoinal logic is run
   * in the display shell's size-allocate
   */
  if (event->width  != current_width ||
      event->height != current_height)
    {
      /* FIXME multiple shells */
      GimpDisplayShell *shell = gimp_image_window_get_active_shell (window);

      if (shell && gimp_display_get_image (shell->display))
        shell->size_allocate_from_configure_event = TRUE;
    }

  scale_factor = gdk_window_get_scale_factor (gtk_widget_get_window (widget));

  if (scale_factor != private->scale_factor)
    {
      GList *list;

      private->scale_factor = scale_factor;

      for (list = private->shells; list; list = g_list_next (list))
        {
          GimpDisplayShell *shell = list->data;

          gimp_display_shell_render_set_scale (shell, scale_factor);
          gimp_display_shell_expose_full (shell);
        }
    }

  return TRUE;
}

static void
gimp_image_window_update_csd_on_fullscreen (GtkWidget *child,
                                            gpointer   user_data)
{
  GimpImageWindow        *window  = GIMP_IMAGE_WINDOW (user_data);
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  gboolean                fullscreen;

  fullscreen = gimp_image_window_get_fullscreen (window);

  if (GTK_IS_HEADER_BAR (child))
    {
      gtk_widget_set_visible (child, !fullscreen);

      /* When we manage the menubar ourselves (i.e. not when using the GTK
       * menubar, e.g. on macOS), if the menubar was set to stay visible in
       * fullscreen mode, we need to move it out of the header bar.
       * Note that here we only move the menubar from one parent to another.
       * This is not where we handle whether we make it visible or not.
       */
      if (private->menubar)
        {
          GtkWidget *parent = gtk_widget_get_parent (private->menubar);

          g_object_ref (private->menubar);
          gtk_container_remove (GTK_CONTAINER (parent), private->menubar);
          if (fullscreen)
            {
              gtk_box_pack_start (GTK_BOX (private->main_vbox),
                                  private->menubar, FALSE, FALSE, 0);
              gtk_box_reorder_child (GTK_BOX (private->main_vbox),
                                     private->menubar, 0);
            }
          else
            {
              gtk_header_bar_pack_start (GTK_HEADER_BAR (child), private->menubar);
            }
          g_object_unref (private->menubar);
        }
    }
}

static gboolean
gimp_image_window_window_state_event (GtkWidget           *widget,
                                      GdkEventWindowState *event)
{
  GimpImageWindow        *window  = GIMP_IMAGE_WINDOW (widget);
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GimpDisplayShell       *shell   = gimp_image_window_get_active_shell (window);

  /* Run the parent implementation */
  if (GTK_WIDGET_CLASS (parent_class)->window_state_event)
    GTK_WIDGET_CLASS (parent_class)->window_state_event (widget, event);

  if (! shell)
    return FALSE;

  private->window_state = event->new_window_state;

  if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
    {
      gboolean fullscreen = gimp_image_window_get_fullscreen (window);

      GIMP_LOG (WM, "Image window '%s' [%p] set fullscreen %s",
                gtk_window_get_title (GTK_WINDOW (widget)),
                widget,
                fullscreen ? "TRUE" : "FALSE");

      gimp_image_window_suspend_keep_pos (window);
      gimp_display_shell_appearance_update (shell);
      gimp_image_window_resume_keep_pos (window);

      /* When using CSD (for example in Wayland), our title bar stays visible
       * when going fullscreen by default. There is no getter for it and it's
       * an internal child, so we use this workaround instead */
      gtk_container_forall (GTK_CONTAINER (window),
                            gimp_image_window_update_csd_on_fullscreen,
                            window);
    }

  if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED)
    {
      GimpStatusbar *statusbar = gimp_display_shell_get_statusbar (shell);
      gboolean       iconified = gimp_image_window_is_iconified (window);

      GIMP_LOG (WM, "Image window '%s' [%p] set %s",
                gtk_window_get_title (GTK_WINDOW (widget)),
                widget,
                iconified ? "iconified" : "uniconified");

      if (iconified)
        {
          if (gimp_displays_get_num_visible (private->gimp) == 0)
            {
              GIMP_LOG (WM, "No displays visible any longer");

              gimp_dialog_factory_hide_with_display (private->dialog_factory);
            }
        }
      else
        {
          gimp_dialog_factory_show_with_display (private->dialog_factory);
        }

      if (gimp_progress_is_active (GIMP_PROGRESS (statusbar)))
        {
          if (iconified)
            gimp_statusbar_override_window_title (statusbar);
          else
            gtk_window_set_title (GTK_WINDOW (window), shell->title);
        }
    }

  return FALSE;
}

static void
gimp_image_window_style_updated (GtkWidget *widget)
{
  GimpImageWindow        *window        = GIMP_IMAGE_WINDOW (widget);
  GimpImageWindowPrivate *private       = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GimpDisplayShell       *shell         = gimp_image_window_get_active_shell (window);
  GimpStatusbar          *statusbar     = NULL;
  GtkRequisition          requisition   = { 0, };
  GdkGeometry             geometry      = { 0, };
  GdkWindowHints          geometry_mask = 0;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (! shell)
    return;

  statusbar = gimp_display_shell_get_statusbar (shell);

  gtk_widget_get_preferred_size (GTK_WIDGET (statusbar), &requisition, NULL);

  geometry.min_height = 23;

  geometry.min_width   = requisition.width;
  geometry.min_height += requisition.height;

  if (private->menubar)
    {
      gtk_widget_get_preferred_size (private->menubar, &requisition, NULL);

      geometry.min_height += requisition.height;
    }

  geometry_mask = GDK_HINT_MIN_SIZE;

  /*  Only set user pos on the empty display because it gets a pos
   *  set by gimp. All other displays should be placed by the window
   *  manager. See https://bugzilla.gnome.org/show_bug.cgi?id=559580
   */
  if (! gimp_display_get_image (shell->display))
    geometry_mask |= GDK_HINT_USER_POS;

  gtk_window_set_geometry_hints (GTK_WINDOW (widget), NULL,
                                 &geometry, geometry_mask);

  gimp_dialog_factory_set_has_min_size (GTK_WINDOW (widget), TRUE);
}

static void
gimp_image_window_monitor_changed (GimpWindow *window,
                                   GdkMonitor *monitor)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GList                  *list;

  for (list = private->shells; list; list = g_list_next (list))
    {
      /*  hack, this should live here, and screen_changed call
       *  monitor_changed
       */
      g_signal_emit_by_name (list->data, "screen-changed",
                             gtk_widget_get_screen (list->data));

      /*  make it fetch the new monitor's resolution  */
      gimp_display_shell_scale_update (GIMP_DISPLAY_SHELL (list->data));

      /*  make it fetch the right monitor profile  */
      gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (list->data));
    }
}

static GList *
gimp_image_window_get_docks (GimpDockContainer *dock_container)
{
  GimpImageWindowPrivate *private;
  GList                  *iter;
  GList                  *all_docks = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (dock_container), FALSE);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (dock_container);

  for (iter = gimp_dock_columns_get_docks (GIMP_DOCK_COLUMNS (private->left_docks));
       iter;
       iter = g_list_next (iter))
    {
      all_docks = g_list_append (all_docks, GIMP_DOCK (iter->data));
    }

  for (iter = gimp_dock_columns_get_docks (GIMP_DOCK_COLUMNS (private->right_docks));
       iter;
       iter = g_list_next (iter))
    {
      all_docks = g_list_append (all_docks, GIMP_DOCK (iter->data));
    }

  return all_docks;
}

static GimpDialogFactory *
gimp_image_window_dock_container_get_dialog_factory (GimpDockContainer *dock_container)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (dock_container);

  return private->dialog_factory;
}

static GimpUIManager *
gimp_image_window_dock_container_get_ui_manager (GimpDockContainer *dock_container)
{
  GimpImageWindow *window = GIMP_IMAGE_WINDOW (dock_container);

  return menus_get_image_manager_singleton (GIMP_IMAGE_WINDOW_GET_PRIVATE (window)->gimp);
}

void
gimp_image_window_add_dock (GimpDockContainer   *dock_container,
                            GimpDock            *dock,
                            GimpSessionInfoDock *dock_info)
{
  GimpImageWindow        *window;
  GimpDisplayShell       *active_shell;
  GimpImageWindowPrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (dock_container));

  window  = GIMP_IMAGE_WINDOW (dock_container);
  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  if (dock_info->side == GIMP_ALIGN_LEFT)
    {
      gimp_dock_columns_add_dock (GIMP_DOCK_COLUMNS (private->left_docks),
                                  dock,
                                  -1 /*index*/);
    }
  else
    {
      gimp_dock_columns_add_dock (GIMP_DOCK_COLUMNS (private->right_docks),
                                  dock,
                                  -1 /*index*/);
    }

  active_shell = gimp_image_window_get_active_shell (window);
  if (active_shell)
    gimp_display_shell_appearance_update (active_shell);
}

static GimpAlignmentType
gimp_image_window_get_dock_side (GimpDockContainer *dock_container,
                                 GimpDock          *dock)
{
  GimpAlignmentType       side = -1;
  GimpImageWindowPrivate *private;
  GList                  *iter;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (dock_container), FALSE);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (dock_container);

  for (iter = gimp_dock_columns_get_docks (GIMP_DOCK_COLUMNS (private->left_docks));
       iter && side == -1;
       iter = g_list_next (iter))
    {
      GimpDock *dock_iter = GIMP_DOCK (iter->data);

      if (dock_iter == dock)
        side = GIMP_ALIGN_LEFT;
    }

  for (iter = gimp_dock_columns_get_docks (GIMP_DOCK_COLUMNS (private->right_docks));
       iter && side == -1;
       iter = g_list_next (iter))
    {
      GimpDock *dock_iter = GIMP_DOCK (iter->data);

      if (dock_iter == dock)
        side = GIMP_ALIGN_RIGHT;
    }

  return side;
}

static GList *
gimp_image_window_get_aux_info (GimpSessionManaged *session_managed)
{
  GList                  *aux_info = NULL;
  GimpImageWindowPrivate *private;
  GimpGuiConfig          *config;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (session_managed), NULL);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (session_managed);
  config  = GIMP_GUI_CONFIG (private->gimp->config);

  if (config->single_window_mode)
    {
      GimpSessionInfoAux *aux;
      GtkAllocation       allocation;
      gchar               widthbuf[128];

      g_snprintf (widthbuf, sizeof (widthbuf), "%d",
                  gtk_paned_get_position (GTK_PANED (private->left_hpane)));
      aux = gimp_session_info_aux_new (GIMP_IMAGE_WINDOW_LEFT_DOCKS_WIDTH,
                                       widthbuf);
      aux_info = g_list_append (aux_info, aux);

      gtk_widget_get_allocation (private->right_hpane, &allocation);

      g_snprintf (widthbuf, sizeof (widthbuf), "%d",
                  allocation.width -
                  gtk_paned_get_position (GTK_PANED (private->right_hpane)));
      aux = gimp_session_info_aux_new (GIMP_IMAGE_WINDOW_RIGHT_DOCKS_WIDTH,
                                       widthbuf);
      aux_info = g_list_append (aux_info, aux);

      aux = gimp_session_info_aux_new (GIMP_IMAGE_WINDOW_MAXIMIZED,
                                       gimp_image_window_is_maximized (GIMP_IMAGE_WINDOW (session_managed)) ?
                                       "yes" : "no");
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static void
gimp_image_window_set_right_docks_width (GtkPaned      *paned,
                                         GtkAllocation *allocation,
                                         void          *data)
{
  gint width = GPOINTER_TO_INT (data);

  g_return_if_fail (GTK_IS_PANED (paned));

  if (width > 0)
    gtk_paned_set_position (paned, allocation->width - width);
  else
    gtk_paned_set_position (paned, - width);

  g_signal_handlers_disconnect_by_func (paned,
                                        gimp_image_window_set_right_docks_width,
                                        data);
}

static void
gimp_image_window_set_aux_info (GimpSessionManaged *session_managed,
                                GList              *aux_info)
{
  GimpImageWindowPrivate *private;
  GList                  *iter;
  gint                    left_docks_width      = G_MININT;
  gint                    right_docks_width     = G_MININT;
  gboolean                wait_with_right_docks = FALSE;
  gboolean                maximized             = FALSE;
#ifdef G_OS_WIN32
  STARTUPINFOW            StartupInfo;

  GetStartupInfoW (&StartupInfo);
#endif

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (session_managed));

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (session_managed);

  for (iter = aux_info; iter; iter = g_list_next (iter))
    {
      GimpSessionInfoAux *aux   = iter->data;
      gint               *width = NULL;

      if (! strcmp (aux->name, GIMP_IMAGE_WINDOW_LEFT_DOCKS_WIDTH))
        width = &left_docks_width;
      else if (! strcmp (aux->name, GIMP_IMAGE_WINDOW_RIGHT_DOCKS_WIDTH))
        width = &right_docks_width;
      else if (! strcmp (aux->name, GIMP_IMAGE_WINDOW_RIGHT_DOCKS_POS))
        width = &right_docks_width;
      else if (! strcmp (aux->name, GIMP_IMAGE_WINDOW_MAXIMIZED))
        if (! g_ascii_strcasecmp (aux->value, "yes"))
          maximized = TRUE;

      if (width)
        sscanf (aux->value, "%d", width);

      /* compat handling for right docks */
      if (! strcmp (aux->name, GIMP_IMAGE_WINDOW_RIGHT_DOCKS_POS))
        {
          /* negate the value because negative docks pos means docks width,
           * also use the negativenes of a real docks pos as condition below.
           */
          *width = - *width;
        }
    }

  if (left_docks_width != G_MININT &&
      gtk_paned_get_position (GTK_PANED (private->left_hpane)) !=
      left_docks_width)
    {
      gtk_paned_set_position (GTK_PANED (private->left_hpane), left_docks_width);

      /* We can't set the position of the right docks, because it will
       * be undesirably adjusted when its get a new size
       * allocation. We must wait until after the size allocation.
       */
      wait_with_right_docks = TRUE;
    }

  if (right_docks_width != G_MININT &&
      gtk_paned_get_position (GTK_PANED (private->right_hpane)) !=
      right_docks_width)
    {
      if (wait_with_right_docks || right_docks_width > 0)
        {
          /* We must wait for a size allocation before we can set the
           * position
           */
          g_signal_connect_data (private->right_hpane, "size-allocate",
                                 G_CALLBACK (gimp_image_window_set_right_docks_width),
                                 GINT_TO_POINTER (right_docks_width), NULL,
                                 G_CONNECT_AFTER);
        }
      else
        {
          /* We can set the position directly, because we didn't
           * change the left hpane position, and we got the old compat
           * dock pos property.
           */
          gtk_paned_set_position (GTK_PANED (private->right_hpane),
                                  - right_docks_width);
        }
    }

#ifdef G_OS_WIN32
  /* On Windows, user can provide startup hints to have a program
   * maximized/minimized on startup. This can be done through command
   * line: `start /max gimp-2.9.exe` or with the shortcut's "run"
   * property.
   * When such a hint is given, we should follow it and bypass the
   * session's information.
   */
  if (StartupInfo.wShowWindow == SW_SHOWMAXIMIZED)
    gtk_window_maximize (GTK_WINDOW (session_managed));
  else if (StartupInfo.wShowWindow == SW_SHOWMINIMIZED   ||
           StartupInfo.wShowWindow == SW_SHOWMINNOACTIVE ||
           StartupInfo.wShowWindow == SW_MINIMIZE)
    gtk_window_iconify (GTK_WINDOW (session_managed));
  else
    /* Another show property not relevant to min/max.
     * Defaults is: SW_SHOWNORMAL
     */
#endif
    if (maximized)
      gtk_window_maximize (GTK_WINDOW (session_managed));
    else
      gtk_window_unmaximize (GTK_WINDOW (session_managed));
}


/*  public functions  */

GimpImageWindow *
gimp_image_window_new (Gimp              *gimp,
                       GimpImage         *image,
                       GimpDialogFactory *dialog_factory,
                       GdkMonitor        *monitor)
{
  GimpImageWindow        *window;
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (image == NULL || GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  window = g_object_new (GIMP_TYPE_IMAGE_WINDOW,
                         "gimp",            gimp,
                         "dialog-factory",  dialog_factory,
                         "initial-monitor", monitor,
                         "application",     g_application_get_default (),
                         /* The window position will be overridden by the
                          * dialog factory, it is only really used on first
                          * startup.
                          */
                         image ? NULL : "window-position",
                         GTK_WIN_POS_CENTER,
                         NULL);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  gimp->image_windows = g_list_append (gimp->image_windows, window);

  if (! GIMP_GUI_CONFIG (private->gimp->config)->single_window_mode)
    {
      GdkMonitor *pointer_monitor = gimp_get_monitor_at_pointer ();

      /*  If we are supposed to go to a monitor other than where the
       *  pointer is, place the window on that monitor manually,
       *  otherwise simply let the window manager place the window on
       *  the poiner's monitor.
       */
      if (pointer_monitor != monitor)
        {
          GdkRectangle rect;

          gdk_monitor_get_workarea (monitor, &rect);

          gtk_window_move (GTK_WINDOW (window),
                           rect.x + 300, rect.y + 30);
          gtk_window_set_geometry_hints (GTK_WINDOW (window),
                                         NULL, NULL, GDK_HINT_USER_POS);
        }
    }

  return window;
}

void
gimp_image_window_destroy (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  private->gimp->image_windows = g_list_remove (private->gimp->image_windows,
                                                window);

  gtk_widget_destroy (GTK_WIDGET (window));
}

GimpDockColumns  *
gimp_image_window_get_left_docks (GimpImageWindow  *window)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), FALSE);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  return GIMP_DOCK_COLUMNS (private->left_docks);
}

GimpDockColumns  *
gimp_image_window_get_right_docks (GimpImageWindow  *window)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), FALSE);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  return GIMP_DOCK_COLUMNS (private->right_docks);
}

void
gimp_image_window_add_shell (GimpImageWindow  *window,
                             GimpDisplayShell *shell)
{
  GimpImageWindowPrivate *private;
  GtkWidget              *tab_label;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  g_return_if_fail (g_list_find (private->shells, shell) == NULL);

  private->shells = g_list_append (private->shells, shell);

  tab_label = gimp_image_window_create_tab_label (window, shell);

  gtk_notebook_append_page (GTK_NOTEBOOK (private->notebook),
                            GTK_WIDGET (shell), tab_label);
  gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (private->notebook),
                                    GTK_WIDGET (shell), TRUE);

  gtk_widget_show (GTK_WIDGET (shell));

  /*  make it fetch the right monitor profile  */
  gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (shell));
}

GimpDisplayShell *
gimp_image_window_get_shell (GimpImageWindow *window,
                             gint             index)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), NULL);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  return g_list_nth_data (private->shells, index);
}

void
gimp_image_window_remove_shell (GimpImageWindow  *window,
                                GimpDisplayShell *shell)
{
  GimpImageWindowPrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  g_return_if_fail (g_list_find (private->shells, shell) != NULL);

  private->shells = g_list_remove (private->shells, shell);

  gtk_container_remove (GTK_CONTAINER (private->notebook),
                        GTK_WIDGET (shell));
}

gint
gimp_image_window_get_n_shells (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), 0);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  return g_list_length (private->shells);
}

void
gimp_image_window_set_active_shell (GimpImageWindow  *window,
                                    GimpDisplayShell *shell)
{
  GimpImageWindowPrivate *private;
  gint                    page_num;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  g_return_if_fail (g_list_find (private->shells, shell));

  page_num = gtk_notebook_page_num (GTK_NOTEBOOK (private->notebook),
                                    GTK_WIDGET (shell));

  gtk_notebook_set_current_page (GTK_NOTEBOOK (private->notebook), page_num);
}

GimpDisplayShell *
gimp_image_window_get_active_shell (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), NULL);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  return private->active_shell;
}

GimpMenuModel *
gimp_image_window_get_menubar_model (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), NULL);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  return private->menubar_model;
}

void
gimp_image_window_set_fullscreen (GimpImageWindow *window,
                                  gboolean         fullscreen)
{
  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));

  if (fullscreen != gimp_image_window_get_fullscreen (window))
    {
      if (fullscreen)
        gtk_window_fullscreen (GTK_WINDOW (window));
      else
        gtk_window_unfullscreen (GTK_WINDOW (window));
    }
}

gboolean
gimp_image_window_get_fullscreen (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), FALSE);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  return (private->window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
}

void
gimp_image_window_set_show_menubar (GimpImageWindow *window,
                                    gboolean         show)
{
  GimpImageWindowPrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  if (private->menubar)
    gtk_widget_set_visible (private->menubar, show);
}

gboolean
gimp_image_window_get_show_menubar (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), FALSE);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  if (private->menubar)
    return gtk_widget_get_visible (private->menubar);

  return FALSE;
}

gboolean
gimp_image_window_is_iconified (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), FALSE);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  return (private->window_state & GDK_WINDOW_STATE_ICONIFIED) != 0;
}

gboolean
gimp_image_window_is_maximized (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), FALSE);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  return (private->window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
}

/**
 * gimp_image_window_has_toolbox:
 * @window:
 *
 * Returns: %TRUE if the image window contains a GimpToolbox.
 **/
gboolean
gimp_image_window_has_toolbox (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;
  GList                  *iter = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), FALSE);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  for (iter = gimp_dock_columns_get_docks (GIMP_DOCK_COLUMNS (private->left_docks));
       iter;
       iter = g_list_next (iter))
    {
      if (GIMP_IS_TOOLBOX (iter->data))
        return TRUE;
    }

  for (iter = gimp_dock_columns_get_docks (GIMP_DOCK_COLUMNS (private->right_docks));
       iter;
       iter = g_list_next (iter))
    {
      if (GIMP_IS_TOOLBOX (iter->data))
        return TRUE;
    }

  return FALSE;
}

void
gimp_image_window_shrink_wrap (GimpImageWindow *window,
                               gboolean         grow_only)
{
  GimpDisplayShell *active_shell;
  GtkWidget        *widget;
  GtkAllocation     allocation;
  GdkMonitor       *monitor;
  GdkRectangle      rect;
  gint              disp_width, disp_height;
  gint              width, height;
  gint              max_auto_width, max_auto_height;
  gint              border_width, border_height;
  gboolean          resize = FALSE;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));

  if (! gtk_widget_get_realized (GTK_WIDGET (window)))
    return;

  /* FIXME this so needs cleanup and shell/window separation */

  active_shell = gimp_image_window_get_active_shell (window);

  if (!active_shell)
    return;

  widget  = GTK_WIDGET (window);
  monitor = gimp_widget_get_monitor (widget);

  gtk_widget_get_allocation (widget, &allocation);

  gdk_monitor_get_workarea (monitor, &rect);

  if (! gimp_display_shell_get_infinite_canvas (active_shell))
    {
      gimp_display_shell_scale_get_image_size (active_shell,
                                               &width, &height);
    }
  else
    {
      gimp_display_shell_scale_get_image_bounding_box (active_shell,
                                                       NULL,   NULL,
                                                       &width, &height);
    }

  disp_width  = active_shell->disp_width;
  disp_height = active_shell->disp_height;


  /* As long as the disp_width/disp_height is larger than 1 we
   * can reliably depend on it to calculate the
   * border_width/border_height because that means there is enough
   * room in the top-level for the canvas as well as the rulers and
   * scrollbars. If it is 1 or smaller it is likely that the rulers
   * and scrollbars are overlapping each other and thus we cannot use
   * the normal approach to border size, so special case that.
   */
  if (disp_width > 1 || !active_shell->vsb)
    {
      border_width = allocation.width - disp_width;
    }
  else
    {
      GtkAllocation vsb_allocation;

      gtk_widget_get_allocation (active_shell->vsb, &vsb_allocation);

      border_width = allocation.width - disp_width + vsb_allocation.width;
    }

  if (disp_height > 1 || !active_shell->hsb)
    {
      border_height = allocation.height - disp_height;
    }
  else
    {
      GtkAllocation hsb_allocation;

      gtk_widget_get_allocation (active_shell->hsb, &hsb_allocation);

      border_height = allocation.height - disp_height + hsb_allocation.height;
    }


  max_auto_width  = (rect.width  - border_width)  * 0.75;
  max_auto_height = (rect.height - border_height) * 0.75;

  /* If one of the display dimensions has changed and one of the
   * dimensions fits inside the screen
   */
  if (((width  + border_width)  < rect.width ||
       (height + border_height) < rect.height) &&
      (width  != disp_width ||
       height != disp_height))
    {
      width  = ((width  + border_width)  < rect.width)  ? width  : max_auto_width;
      height = ((height + border_height) < rect.height) ? height : max_auto_height;

      resize = TRUE;
    }

  /*  If the projected dimension is greater than current, but less than
   *  3/4 of the screen size, expand automagically
   */
  else if ((width  > disp_width ||
            height > disp_height) &&
           (disp_width  < max_auto_width ||
            disp_height < max_auto_height))
    {
      width  = MIN (max_auto_width,  width);
      height = MIN (max_auto_height, height);

      resize = TRUE;
    }

  if (resize)
    {
      GimpStatusbar *statusbar = gimp_display_shell_get_statusbar (active_shell);
      gint           statusbar_width;

      gtk_widget_get_size_request (GTK_WIDGET (statusbar),
                                   &statusbar_width, NULL);

      if (width < statusbar_width)
        width = statusbar_width;

      width  = width  + border_width;
      height = height + border_height;

      if (grow_only)
        {
          if (width < allocation.width)
            width = allocation.width;

          if (height < allocation.height)
            height = allocation.height;
        }

      gtk_window_resize (GTK_WINDOW (window), width, height);
    }

  /* A wrap always means that we should center the image too. If the
   * window changes size another center will be done in
   * GimpDisplayShell::configure_event().
   */
  /* FIXME multiple shells */
  gimp_display_shell_scroll_center_content (active_shell, TRUE, TRUE);
}

static GtkWidget *
gimp_image_window_get_first_dockbook (GimpDockColumns *columns)
{
  GList *dock_iter;

  for (dock_iter = gimp_dock_columns_get_docks (columns);
       dock_iter;
       dock_iter = g_list_next (dock_iter))
    {
      GimpDock *dock      = GIMP_DOCK (dock_iter->data);
      GList    *dockbooks = gimp_dock_get_dockbooks (dock);

      if (dockbooks)
        return GTK_WIDGET (dockbooks->data);
    }

  return NULL;
}

/**
 * gimp_image_window_get_default_dockbook:
 * @window:
 *
 * Gets the default dockbook, which is the dockbook in which new
 * dockables should be put in single-window mode.
 *
 * Returns: (nullable): The default dockbook for new dockables, or %NULL if no
 *          dockbook were available.
 **/
GtkWidget *
gimp_image_window_get_default_dockbook (GimpImageWindow  *window)
{
  GimpImageWindowPrivate *private;
  GimpDockColumns        *dock_columns;
  GtkWidget              *dockbook = NULL;

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  /* First try the first dockbook in the right docks */
  dock_columns = GIMP_DOCK_COLUMNS (private->right_docks);
  dockbook     = gimp_image_window_get_first_dockbook (dock_columns);

  /* Then the left docks */
  if (! dockbook)
    {
      dock_columns = GIMP_DOCK_COLUMNS (private->left_docks);
      dockbook     = gimp_image_window_get_first_dockbook (dock_columns);
    }

  return dockbook;
}

/**
 * gimp_image_window_keep_canvas_pos:
 * @window:
 *
 * Stores the coordinates of the current image canvas origin relatively
 * its GtkWindow; and on the first size-allocate sets the offsets in
 * the shell so that the image origin remains the same (even on another
 * GtkWindow).
 *
 * Example use case: The user hides docks attached to the side of image
 * windows. You want the image to remain fixed on the screen though,
 * so you use this function to keep the image fixed after the docks
 * have been hidden.
 **/
void
gimp_image_window_keep_canvas_pos (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;
  GimpDisplayShell       *shell;
  gint                    canvas_x;
  gint                    canvas_y;
  gint                    window_x;
  gint                    window_y;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  if (private->suspend_keep_pos > 0)
    return;

  shell = gimp_image_window_get_active_shell (window);

  gimp_display_shell_transform_xy (shell, 0.0, 0.0, &canvas_x, &canvas_y);

  if (gtk_widget_translate_coordinates (GTK_WIDGET (shell->canvas),
                                        GTK_WIDGET (window),
                                        canvas_x, canvas_y,
                                        &window_x, &window_y))
    {
      PosCorrectionData *data = g_new0 (PosCorrectionData, 1);

      data->canvas_x = canvas_x;
      data->canvas_y = canvas_y;
      data->window_x = window_x;
      data->window_y = window_y;

      g_signal_connect_data (shell, "size-allocate",
                             G_CALLBACK (gimp_image_window_shell_size_allocate),
                             data, (GClosureNotify) g_free,
                             G_CONNECT_AFTER);
      /*
       * XXX Asking a resize should not be necessary and should happen
       * automatically with the translate coordinates. And it does, but
       * sometimes only after some further event (like clicking on shell
       * widgets). Until such event happens, the shell look broken, for
       * instance with rulers and canvas overlapping and misaligned
       * ruler values.
       * So I queue a resize explicitly to force the correct shell
       * visualization immediately.
       * See also #5813.
       */
      gtk_widget_queue_resize (GTK_WIDGET (shell));
    }
}

void
gimp_image_window_suspend_keep_pos (GimpImageWindow  *window)
{
  GimpImageWindowPrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  private->suspend_keep_pos++;
}

void
gimp_image_window_resume_keep_pos (GimpImageWindow  *window)
{
  GimpImageWindowPrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  g_return_if_fail (private->suspend_keep_pos > 0);

  private->suspend_keep_pos--;
}

/**
 * gimp_image_window_update_tabs:
 * @window: the Image Window to update.
 *
 * Holds the logics of whether shell tabs are to be shown or not in the
 * Image Window @window. This function should be called after every
 * change to @window where one might expect tab visibility to change.
 *
 * No direct call to gtk_notebook_set_show_tabs() should ever be made.
 * If we change the logics of tab hiding, we should only change this
 * procedure instead.
 **/
void
gimp_image_window_update_tabs (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;
  GimpGuiConfig          *config;
  GtkPositionType         position;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  config = GIMP_GUI_CONFIG (private->gimp->config);

  /* Tab visibility. */
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (private->notebook),
                              config->single_window_mode &&
                              config->show_tabs          &&
                              ! config->hide_docks       &&
                              ((private->active_shell          &&
                                private->active_shell->display &&
                                gimp_display_get_image (private->active_shell->display)) ||
                               g_list_length (private->shells) > 1));

  /* Tab position. */
  switch (config->tabs_position)
    {
    case GIMP_POSITION_TOP:
      position = GTK_POS_TOP;
      break;
    case GIMP_POSITION_BOTTOM:
      position = GTK_POS_BOTTOM;
      break;
    case GIMP_POSITION_LEFT:
      position = GTK_POS_LEFT;
      break;
    case GIMP_POSITION_RIGHT:
      position = GTK_POS_RIGHT;
      break;
    default:
      /* If we have any strange value, just reset to default. */
      position = GTK_POS_TOP;
      break;
    }

  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (private->notebook), position);
}


/*  private functions  */

static void
gimp_image_window_show_tooltip (GimpUIManager   *manager,
                                const gchar     *tooltip,
                                GimpImageWindow *window)
{
  GimpDisplayShell *shell     = gimp_image_window_get_active_shell (window);
  GimpStatusbar    *statusbar = NULL;

  if (! shell)
    return;

  statusbar = gimp_display_shell_get_statusbar (shell);

  gimp_statusbar_push (statusbar, "menu-tooltip",
                       NULL, "%s", tooltip);
}

static void
gimp_image_window_config_notify (GimpImageWindow *window,
                                 GParamSpec      *pspec,
                                 GimpGuiConfig   *config)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  /* Dock column visibility */
  if (strcmp (pspec->name, "single-window-mode") == 0 ||
      strcmp (pspec->name, "hide-docks")         == 0 ||
      strcmp (pspec->name, "show-tabs")          == 0 ||
      strcmp (pspec->name, "tabs-position")      == 0)
    {
      if (strcmp (pspec->name, "single-window-mode") == 0 ||
          strcmp (pspec->name, "hide-docks")         == 0)
        {
          gboolean show_docks = (config->single_window_mode &&
                                 ! config->hide_docks);

          gimp_image_window_keep_canvas_pos (window);
          gtk_widget_set_visible (private->left_docks, show_docks);
          gtk_widget_set_visible (private->right_docks, show_docks);

          /* If docks are being shown, and we are in multi-window-mode,
           * and this is the window of the active display, try to set
           * the keyboard focus to this window because it might have
           * been stolen by a dock. See bug #567333.
           */
          if (strcmp (pspec->name, "hide-docks") == 0 &&
              ! config->single_window_mode            &&
              ! config->hide_docks)
            {
              GimpDisplayShell *shell;
              GimpContext      *user_context;

              shell        = gimp_image_window_get_active_shell (window);
              user_context = gimp_get_user_context (private->gimp);

              if (gimp_context_get_display (user_context) == shell->display)
                {
                  GdkWindow *w = gtk_widget_get_window (GTK_WIDGET (window));

                  if (w)
                    gdk_window_focus (w, gtk_get_current_event_time ());
                }
            }
        }

      gimp_image_window_update_tabs (window);
    }

  /* Session management */
  if (strcmp (pspec->name, "single-window-mode") == 0)
    {
      gimp_image_window_session_update (window,
                                        NULL /*new_display*/,
                                        gimp_image_window_config_to_entry_id (config),
                                        gimp_widget_get_monitor (GTK_WIDGET (window)));
    }
}

static void
gimp_image_window_hide_tooltip (GimpUIManager   *manager,
                                GimpImageWindow *window)
{
  GimpDisplayShell *shell     = gimp_image_window_get_active_shell (window);
  GimpStatusbar    *statusbar = NULL;

  if (! shell)
    return;

  statusbar = gimp_display_shell_get_statusbar (shell);

  gimp_statusbar_pop (statusbar, "menu-tooltip");
}

static gboolean
gimp_image_window_update_ui_manager_idle (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private      = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GimpImage              *active_image = NULL;
  GimpContext            *context;

  gimp_assert (private->active_shell != NULL);

  context = gimp_get_user_context (private->active_shell->display->gimp);

  if (context)
    active_image = gimp_context_get_image (context);

  /* Since we are running idle, it is possible this runs after the
   * active display switched, and therefore we may call the wrong
   * actions for an image. See #10441.
   */
  if ((active_image &&
       (! private->active_shell->display || ! gimp_display_get_image (private->active_shell->display))) ||
      active_image != gimp_display_get_image (private->active_shell->display))
    {
      private->update_ui_manager_idle_id = 0;
      return G_SOURCE_REMOVE;
    }

  gimp_ui_manager_update (menus_get_image_manager_singleton (private->active_shell->display->gimp),
                          private->active_shell->display);

  private->update_ui_manager_idle_id = 0;

  return G_SOURCE_REMOVE;
}

static void
gimp_image_window_update_ui_manager (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  if (! private->update_ui_manager_idle_id)
    {
      private->update_ui_manager_idle_id =
        g_idle_add_full (GIMP_PRIORITY_IMAGE_WINDOW_UPDATE_UI_MANAGER_IDLE,
                         (GSourceFunc) gimp_image_window_update_ui_manager_idle,
                         window,
                         NULL);
    }
}

static void
gimp_image_window_shell_size_allocate (GimpDisplayShell  *shell,
                                       GtkAllocation     *allocation,
                                       PosCorrectionData *data)
{
  GimpImageWindow *window = gimp_display_shell_get_window (shell);
  gint             new_window_x;
  gint             new_window_y;

  if (gtk_widget_translate_coordinates (GTK_WIDGET (shell->canvas),
                                        GTK_WIDGET (window),
                                        data->canvas_x, data->canvas_y,
                                        &new_window_x, &new_window_y))
    {
      gint off_x = new_window_x - data->window_x;
      gint off_y = new_window_y - data->window_y;

      if (off_x || off_y)
        gimp_display_shell_scroll (shell, off_x, off_y);
    }

  g_signal_handlers_disconnect_by_func (shell,
                                        gimp_image_window_shell_size_allocate,
                                        data);
}

static gboolean
gimp_image_window_shell_events (GtkWidget       *widget,
                                GdkEvent        *event,
                                GimpImageWindow *window)
{
  GimpDisplayShell *shell = gimp_image_window_get_active_shell (window);

  if (! shell)
    return FALSE;

  return gimp_display_shell_events (widget, event, shell);
}

static void
gimp_image_window_switch_page (GtkNotebook     *notebook,
                               gpointer         page,
                               gint             page_num,
                               GimpImageWindow *window)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GimpDisplayShell       *shell;
  GimpDisplay            *active_display;

  shell = GIMP_DISPLAY_SHELL (gtk_notebook_get_nth_page (notebook, page_num));

  if (shell == private->active_shell)
    return;

  gimp_image_window_disconnect_from_active_shell (window);

  GIMP_LOG (WM, "GimpImageWindow %p, private->active_shell = %p; \n",
            window, shell);
  private->active_shell = shell;

  gimp_window_set_primary_focus_widget (GIMP_WINDOW (window),
                                        shell->canvas);

  active_display = private->active_shell->display;

  g_signal_connect (active_display, "notify::image",
                    G_CALLBACK (gimp_image_window_image_notify),
                    window);

  g_signal_connect (private->active_shell, "scaled",
                    G_CALLBACK (gimp_image_window_shell_scaled),
                    window);
  g_signal_connect (private->active_shell, "rotated",
                    G_CALLBACK (gimp_image_window_shell_rotated),
                    window);
  g_signal_connect (private->active_shell, "notify::title",
                    G_CALLBACK (gimp_image_window_shell_title_notify),
                    window);

  gtk_window_set_title (GTK_WINDOW (window), shell->title);

  if (gtk_widget_get_window (GTK_WIDGET (window)))
    {
      /*  we are fully initialized, use the window's current monitor
       */
      gimp_image_window_session_update (window,
                                        active_display,
                                        NULL /*new_entry_id*/,
                                        gimp_widget_get_monitor (GTK_WIDGET (window)));
    }
  else
    {
      /*  we are in construction, use the initial monitor; calling
       *  gimp_widget_get_monitor() would get us the monitor where the
       *  pointer is
       */
      gimp_image_window_session_update (window,
                                        active_display,
                                        NULL /*new_entry_id*/,
                                        private->initial_monitor);
    }

  gimp_context_set_display (gimp_get_user_context (private->gimp),
                            active_display);

  gimp_display_shell_appearance_update (private->active_shell);

  gimp_image_window_update_ui_manager (window);

  gimp_image_window_update_tab_labels (window);
}

static void
gimp_image_window_page_removed (GtkNotebook     *notebook,
                                GtkWidget       *widget,
                                gint             page_num,
                                GimpImageWindow *window)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  if (GTK_WIDGET (private->active_shell) == widget)
    {
      GIMP_LOG (WM, "GimpImageWindow %p, private->active_shell = %p; \n",
                window, NULL);
      gimp_image_window_disconnect_from_active_shell (window);
      private->active_shell = NULL;
    }
}

static void
gimp_image_window_page_reordered (GtkNotebook     *notebook,
                                  GtkWidget       *widget,
                                  gint             page_num,
                                  GimpImageWindow *window)
{
  GimpImageWindowPrivate *private  = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GimpContainer          *displays = private->gimp->displays;
  gint                    index    = g_list_index (private->shells, widget);

  if (index != page_num)
    {
      private->shells = g_list_remove (private->shells, widget);
      private->shells = g_list_insert (private->shells, widget, page_num);
    }

  /* We need to reorder the displays as well in order to update the
   * numbered accelerators (alt-1, alt-2, etc.).
   */
  gimp_container_reorder (displays,
                          GIMP_OBJECT (GIMP_DISPLAY_SHELL (widget)->display),
                          page_num);

  gtk_notebook_reorder_child (notebook, widget, page_num);
}

static void
gimp_image_window_disconnect_from_active_shell (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private        = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GimpDisplay            *active_display = NULL;

  if (! private->active_shell)
    return;

  active_display = private->active_shell->display;

  if (active_display)
    g_signal_handlers_disconnect_by_func (active_display,
                                          gimp_image_window_image_notify,
                                          window);

  g_signal_handlers_disconnect_by_func (private->active_shell,
                                        gimp_image_window_shell_scaled,
                                        window);
  g_signal_handlers_disconnect_by_func (private->active_shell,
                                        gimp_image_window_shell_rotated,
                                        window);
  g_signal_handlers_disconnect_by_func (private->active_shell,
                                        gimp_image_window_shell_title_notify,
                                        window);

  gimp_image_window_hide_tooltip (menus_get_image_manager_singleton (private->gimp), window);

  if (private->update_ui_manager_idle_id)
    {
      g_source_remove (private->update_ui_manager_idle_id);
      private->update_ui_manager_idle_id = 0;
    }
}

static void
gimp_image_window_image_notify (GimpDisplay      *display,
                                const GParamSpec *pspec,
                                GimpImageWindow  *window)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GtkWidget              *tab_label;
  GList                  *children;
  GtkWidget              *view;

  gimp_image_window_session_update (window,
                                    display,
                                    NULL /*new_entry_id*/,
                                    gimp_widget_get_monitor (GTK_WIDGET (window)));

  tab_label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (private->notebook),
                                          GTK_WIDGET (gimp_display_get_shell (display)));
  children  = gtk_container_get_children (GTK_CONTAINER (tab_label));
  view      = GTK_WIDGET (children->data);
  g_list_free (children);

  gimp_view_set_viewable (GIMP_VIEW (view),
                          GIMP_VIEWABLE (gimp_display_get_image (display)));

  gimp_image_window_update_ui_manager (window);
}

static void
gimp_image_window_session_clear (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GtkWidget              *widget  = GTK_WIDGET (window);

  if (gimp_dialog_factory_from_widget (widget, NULL))
    gimp_dialog_factory_remove_dialog (private->dialog_factory,
                                       widget);
}

static void
gimp_image_window_session_apply (GimpImageWindow *window,
                                 const gchar     *entry_id,
                                 GdkMonitor      *monitor)
{
  GimpImageWindowPrivate *private      = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GimpSessionInfo        *session_info = NULL;
  gint                    width        = -1;
  gint                    height       = -1;

  gtk_window_unfullscreen (GTK_WINDOW (window));

  /*  get the NIW size before adding the display to the dialog
   *  factory so the window's current size doesn't affect the
   *  stored session info entry.
   */
  session_info =
    gimp_dialog_factory_find_session_info (private->dialog_factory, entry_id);

  if (session_info)
    {
      width  = gimp_session_info_get_width  (session_info);
      height = gimp_session_info_get_height (session_info);
    }
  else
    {
      GtkAllocation allocation;

      gtk_widget_get_allocation (GTK_WIDGET (window), &allocation);

      width  = allocation.width;
      height = allocation.height;
    }

  gimp_dialog_factory_add_foreign (private->dialog_factory,
                                   entry_id,
                                   GTK_WIDGET (window),
                                   monitor);

  gtk_window_unmaximize (GTK_WINDOW (window));
  gtk_window_resize (GTK_WINDOW (window), width, height);
}

static void
gimp_image_window_session_update (GimpImageWindow *window,
                                  GimpDisplay     *new_display,
                                  const gchar     *new_entry_id,
                                  GdkMonitor      *monitor)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  /* Handle changes to the entry id */
  if (new_entry_id)
    {
      if (! private->entry_id)
        {
          /* We're initializing. If we're in single-window mode, this
           * will be the only window, so start to session manage
           * it. If we're in multi-window mode, we will find out if we
           * should session manage ourselves when we get a display
           */
          if (strcmp (new_entry_id, GIMP_SINGLE_IMAGE_WINDOW_ENTRY_ID) == 0)
            {
              gimp_image_window_session_apply (window, new_entry_id, monitor);
            }
        }
      else if (strcmp (private->entry_id, new_entry_id) != 0)
        {
          /* The entry id changed, immediately and always stop session
           * managing the old entry
           */
          gimp_image_window_session_clear (window);

          if (strcmp (new_entry_id, GIMP_EMPTY_IMAGE_WINDOW_ENTRY_ID) == 0)
            {
              /* If there is only one imageless display, we shall
               * become the empty image window
               */
              if (private->active_shell &&
                  private->active_shell->display &&
                  ! gimp_display_get_image (private->active_shell->display) &&
                  g_list_length (private->shells) <= 1)
                {
                  gimp_image_window_session_apply (window, new_entry_id,
                                                   monitor);
                }
            }
          else if (strcmp (new_entry_id, GIMP_SINGLE_IMAGE_WINDOW_ENTRY_ID) == 0)
            {
              /* As soon as we become the single image window, we
               * shall session manage ourself until single-window mode
               * is exited
               */
              gimp_image_window_session_apply (window, new_entry_id,
                                               monitor);
            }
        }

      private->entry_id = new_entry_id;
    }

  /* Handle changes to the displays. When in single-window mode, we
   * just keep session managing the single image window. We only need
   * to care about the multi-window mode case here
   */
  if (new_display &&
      strcmp (private->entry_id, GIMP_EMPTY_IMAGE_WINDOW_ENTRY_ID) == 0)
    {
      if (gimp_display_get_image (new_display))
        {
          /* As soon as we have an image we should not affect the size of the
           * empty image window
           */
          gimp_image_window_session_clear (window);
        }
      else if (! gimp_display_get_image (new_display) &&
               g_list_length (private->shells) <= 1)
        {
          /* As soon as we have no image (and no other shells that may
           * contain images) we should become the empty image window
           */
          gimp_image_window_session_apply (window, private->entry_id,
                                           monitor);
        }
    }
}

static const gchar *
gimp_image_window_config_to_entry_id (GimpGuiConfig *config)
{
  return (config->single_window_mode ?
          GIMP_SINGLE_IMAGE_WINDOW_ENTRY_ID :
          GIMP_EMPTY_IMAGE_WINDOW_ENTRY_ID);
}

static void
gimp_image_window_shell_scaled (GimpDisplayShell *shell,
                                GimpImageWindow  *window)
{
  /* update the <Image>/View/Zoom menu */
  gimp_image_window_update_ui_manager (window);
}

static void
gimp_image_window_shell_rotated (GimpDisplayShell *shell,
                                 GimpImageWindow  *window)
{
  /* update the <Image>/View/Rotate menu */
  gimp_image_window_update_ui_manager (window);
}

static void
gimp_image_window_shell_title_notify (GimpDisplayShell *shell,
                                      const GParamSpec *pspec,
                                      GimpImageWindow  *window)
{
  gtk_window_set_title (GTK_WINDOW (window), shell->title);
}

static void
gimp_image_window_shell_close_button_callback (GimpDisplayShell *shell)
{
  if (shell)
    gimp_display_shell_close (shell, FALSE);
}

static GtkWidget *
gimp_image_window_create_tab_label (GimpImageWindow  *window,
                                    GimpDisplayShell *shell)
{
  GtkWidget *hbox;
  GtkWidget *view;
  GimpImage *image;
  GtkWidget *button;
  GtkWidget *gtk_image;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_show (hbox);

  view = gimp_view_new_by_types (gimp_get_user_context (shell->display->gimp),
                                 GIMP_TYPE_VIEW, GIMP_TYPE_IMAGE,
                                 GIMP_VIEW_SIZE_LARGE, 0, FALSE);
  gtk_widget_set_size_request (view, GIMP_VIEW_SIZE_LARGE, -1);
  gimp_view_renderer_set_color_config (GIMP_VIEW (view)->renderer,
                                       gimp_display_shell_get_color_config (shell));
  gtk_box_pack_start (GTK_BOX (hbox), view, FALSE, FALSE, 0);
  gtk_widget_show (view);

  image = gimp_display_get_image (shell->display);
  if (image)
    gimp_view_set_viewable (GIMP_VIEW (view), GIMP_VIEWABLE (image));

  button = gtk_button_new ();
  gtk_widget_set_can_focus (button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_image = gtk_image_new_from_icon_name (GIMP_ICON_CLOSE,
                                            GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), gtk_image);
  gtk_widget_show (gtk_image);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_image_window_shell_close_button_callback),
                            shell);

  g_object_set_data (G_OBJECT (hbox), "close-button", button);

  return hbox;
}

static void
gimp_image_window_update_tab_labels (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GList                  *children;
  GList                  *list;

  children = gtk_container_get_children (GTK_CONTAINER (private->notebook));

  for (list = children; list; list = g_list_next (list))
    {
      GtkWidget *shell = list->data;
      GtkWidget *tab_widget;
      GtkWidget *close_button;

      tab_widget = gtk_notebook_get_tab_label (GTK_NOTEBOOK (private->notebook),
                                               shell);

      close_button = g_object_get_data (G_OBJECT (tab_widget), "close-button");

      if (gimp_context_get_display (gimp_get_user_context (private->gimp)) ==
          GIMP_DISPLAY_SHELL (shell)->display)
        {
          gtk_widget_show (close_button);
        }
      else
        {
          gtk_widget_hide (close_button);
        }
    }

  g_list_free (children);
}

static void
gimp_image_window_configure_pad (GimpImageWindow *window,
                                 GimpDeviceInfo  *info)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GtkPadController       *controller;

  g_hash_table_remove (private->pad_controllers, info);

  if (gimp_device_info_get_device (info, NULL) != NULL)
    {
      controller =
        gimp_device_info_create_pad_controller (info, GIMP_WINDOW (window));
      if (controller)
        g_hash_table_insert (private->pad_controllers, info, controller);
    }
}

static void
gimp_image_window_init_pad_foreach (GimpDeviceInfo  *info,
                                    GimpImageWindow *window)
{
  if (gimp_device_info_get_device (info, NULL) == NULL)
    return;
  if (gimp_device_info_get_source (info) != GDK_SOURCE_TABLET_PAD)
    return;

  gimp_image_window_configure_pad (window, info);
}
