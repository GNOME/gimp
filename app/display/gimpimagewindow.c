/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaprogress.h"
#include "core/ligmacontainer.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmadockbook.h"
#include "widgets/ligmadockcolumns.h"
#include "widgets/ligmadockcontainer.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamenufactory.h"
#include "widgets/ligmasessioninfo.h"
#include "widgets/ligmasessioninfo-aux.h"
#include "widgets/ligmasessionmanaged.h"
#include "widgets/ligmasessioninfo-dock.h"
#include "widgets/ligmatoolbox.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmaview.h"
#include "widgets/ligmaviewrenderer.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmadisplay.h"
#include "ligmadisplay-foreach.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-appearance.h"
#include "ligmadisplayshell-close.h"
#include "ligmadisplayshell-expose.h"
#include "ligmadisplayshell-render.h"
#include "ligmadisplayshell-scale.h"
#include "ligmadisplayshell-scroll.h"
#include "ligmadisplayshell-tool-events.h"
#include "ligmadisplayshell-transform.h"
#include "ligmaimagewindow.h"
#include "ligmastatusbar.h"

#include "ligma-log.h"
#include "ligma-priorities.h"

#include "ligma-intl.h"


#define LIGMA_EMPTY_IMAGE_WINDOW_ENTRY_ID   "ligma-empty-image-window"
#define LIGMA_SINGLE_IMAGE_WINDOW_ENTRY_ID  "ligma-single-image-window"

/* The width of the left and right dock areas */
#define LIGMA_IMAGE_WINDOW_LEFT_DOCKS_WIDTH  "left-docks-width"
#define LIGMA_IMAGE_WINDOW_RIGHT_DOCKS_WIDTH "right-docks-width"

/* deprecated property: GtkPaned position of the right docks area */
#define LIGMA_IMAGE_WINDOW_RIGHT_DOCKS_POS  "right-docks-position"

/* Whether the window's maximized or not */
#define LIGMA_IMAGE_WINDOW_MAXIMIZED        "maximized"


enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_DIALOG_FACTORY,
  PROP_INITIAL_MONITOR
};


typedef struct _LigmaImageWindowPrivate LigmaImageWindowPrivate;

struct _LigmaImageWindowPrivate
{
  Ligma              *ligma;
  LigmaUIManager     *menubar_manager;
  LigmaDialogFactory *dialog_factory;

  GList             *shells;
  LigmaDisplayShell  *active_shell;

  GtkWidget         *main_vbox;
  GtkWidget         *menubar;
  GtkWidget         *hbox;
  GtkWidget         *left_hpane;
  GtkWidget         *left_docks;
  GtkWidget         *right_hpane;
  GtkWidget         *notebook;
  GtkWidget         *right_docks;

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


#define LIGMA_IMAGE_WINDOW_GET_PRIVATE(window) \
        ((LigmaImageWindowPrivate *) ligma_image_window_get_instance_private ((LigmaImageWindow *) (window)))


/*  local function prototypes  */

static void      ligma_image_window_dock_container_iface_init
                                                       (LigmaDockContainerInterface
                                                                            *iface);
static void      ligma_image_window_session_managed_iface_init
                                                       (LigmaSessionManagedInterface
                                                                            *iface);
static void      ligma_image_window_constructed         (GObject             *object);
static void      ligma_image_window_dispose             (GObject             *object);
static void      ligma_image_window_finalize            (GObject             *object);
static void      ligma_image_window_set_property        (GObject             *object,
                                                        guint                property_id,
                                                        const GValue        *value,
                                                        GParamSpec          *pspec);
static void      ligma_image_window_get_property        (GObject             *object,
                                                        guint                property_id,
                                                        GValue              *value,
                                                        GParamSpec          *pspec);

static gboolean  ligma_image_window_delete_event        (GtkWidget           *widget,
                                                        GdkEventAny         *event);
static gboolean  ligma_image_window_configure_event     (GtkWidget           *widget,
                                                        GdkEventConfigure   *event);
static gboolean  ligma_image_window_window_state_event  (GtkWidget           *widget,
                                                        GdkEventWindowState *event);
static void      ligma_image_window_style_updated       (GtkWidget           *widget);

static void      ligma_image_window_monitor_changed     (LigmaWindow          *window,
                                                        GdkMonitor          *monitor);

static GList *   ligma_image_window_get_docks           (LigmaDockContainer   *dock_container);
static LigmaDialogFactory *
                 ligma_image_window_dock_container_get_dialog_factory
                                                       (LigmaDockContainer   *dock_container);
static LigmaUIManager *
                 ligma_image_window_dock_container_get_ui_manager
                                                       (LigmaDockContainer   *dock_container);
static void      ligma_image_window_add_dock            (LigmaDockContainer   *dock_container,
                                                        LigmaDock            *dock,
                                                        LigmaSessionInfoDock *dock_info);
static LigmaAlignmentType
                 ligma_image_window_get_dock_side       (LigmaDockContainer   *dock_container,
                                                        LigmaDock            *dock);
static GList   * ligma_image_window_get_aux_info        (LigmaSessionManaged  *session_managed);
static void      ligma_image_window_set_aux_info        (LigmaSessionManaged  *session_managed,
                                                        GList               *aux_info);

static void      ligma_image_window_config_notify       (LigmaImageWindow     *window,
                                                        GParamSpec          *pspec,
                                                        LigmaGuiConfig       *config);
static void      ligma_image_window_session_clear       (LigmaImageWindow     *window);
static void      ligma_image_window_session_apply       (LigmaImageWindow     *window,
                                                        const gchar         *entry_id,
                                                        GdkMonitor          *monitor);
static void      ligma_image_window_session_update      (LigmaImageWindow     *window,
                                                        LigmaDisplay         *new_display,
                                                        const gchar         *new_entry_id,
                                                        GdkMonitor          *monitor);
static const gchar *
                 ligma_image_window_config_to_entry_id  (LigmaGuiConfig       *config);
static void      ligma_image_window_show_tooltip        (LigmaUIManager       *manager,
                                                        const gchar         *tooltip,
                                                        LigmaImageWindow     *window);
static void      ligma_image_window_hide_tooltip        (LigmaUIManager       *manager,
                                                        LigmaImageWindow     *window);
static gboolean  ligma_image_window_update_ui_manager_idle
                                                       (LigmaImageWindow     *window);
static void      ligma_image_window_update_ui_manager   (LigmaImageWindow     *window);

static void      ligma_image_window_shell_size_allocate (LigmaDisplayShell    *shell,
                                                        GtkAllocation       *allocation,
                                                        PosCorrectionData   *data);
static gboolean  ligma_image_window_shell_events        (GtkWidget           *widget,
                                                        GdkEvent            *event,
                                                        LigmaImageWindow     *window);

static void      ligma_image_window_switch_page         (GtkNotebook         *notebook,
                                                        gpointer             page,
                                                        gint                 page_num,
                                                        LigmaImageWindow     *window);
static void      ligma_image_window_page_removed        (GtkNotebook         *notebook,
                                                        GtkWidget           *widget,
                                                        gint                 page_num,
                                                        LigmaImageWindow     *window);
static void      ligma_image_window_page_reordered      (GtkNotebook         *notebook,
                                                        GtkWidget           *widget,
                                                        gint                 page_num,
                                                        LigmaImageWindow     *window);
static void      ligma_image_window_disconnect_from_active_shell
                                                       (LigmaImageWindow *window);

static void      ligma_image_window_image_notify        (LigmaDisplay         *display,
                                                        const GParamSpec    *pspec,
                                                        LigmaImageWindow     *window);
static void      ligma_image_window_shell_scaled        (LigmaDisplayShell    *shell,
                                                        LigmaImageWindow     *window);
static void      ligma_image_window_shell_rotated       (LigmaDisplayShell    *shell,
                                                        LigmaImageWindow     *window);
static void      ligma_image_window_shell_title_notify  (LigmaDisplayShell    *shell,
                                                        const GParamSpec    *pspec,
                                                        LigmaImageWindow     *window);
static GtkWidget *
                 ligma_image_window_create_tab_label    (LigmaImageWindow     *window,
                                                        LigmaDisplayShell    *shell);
static void      ligma_image_window_update_tab_labels   (LigmaImageWindow     *window);


G_DEFINE_TYPE_WITH_CODE (LigmaImageWindow, ligma_image_window, LIGMA_TYPE_WINDOW,
                         G_ADD_PRIVATE (LigmaImageWindow)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCK_CONTAINER,
                                                ligma_image_window_dock_container_iface_init)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_SESSION_MANAGED,
                                                ligma_image_window_session_managed_iface_init))

#define parent_class ligma_image_window_parent_class


static void
ligma_image_window_class_init (LigmaImageWindowClass *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass  *widget_class = GTK_WIDGET_CLASS (klass);
  LigmaWindowClass *window_class = LIGMA_WINDOW_CLASS (klass);

  object_class->constructed        = ligma_image_window_constructed;
  object_class->dispose            = ligma_image_window_dispose;
  object_class->finalize           = ligma_image_window_finalize;
  object_class->set_property       = ligma_image_window_set_property;
  object_class->get_property       = ligma_image_window_get_property;

  widget_class->delete_event       = ligma_image_window_delete_event;
  widget_class->configure_event    = ligma_image_window_configure_event;
  widget_class->window_state_event = ligma_image_window_window_state_event;
  widget_class->style_updated      = ligma_image_window_style_updated;

  window_class->monitor_changed    = ligma_image_window_monitor_changed;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_DIALOG_FACTORY,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_INITIAL_MONITOR,
                                   g_param_spec_object ("initial-monitor",
                                                        NULL, NULL,
                                                        GDK_TYPE_MONITOR,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_image_window_init (LigmaImageWindow *window)
{
  static gint  role_serial = 1;
  gchar       *role;

  role = g_strdup_printf ("ligma-image-window-%d", role_serial++);
  gtk_window_set_role (GTK_WINDOW (window), role);
  g_free (role);

  gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
}

static void
ligma_image_window_dock_container_iface_init (LigmaDockContainerInterface *iface)
{
  iface->get_docks          = ligma_image_window_get_docks;
  iface->get_dialog_factory = ligma_image_window_dock_container_get_dialog_factory;
  iface->get_ui_manager     = ligma_image_window_dock_container_get_ui_manager;
  iface->add_dock           = ligma_image_window_add_dock;
  iface->get_dock_side      = ligma_image_window_get_dock_side;
}

static void
ligma_image_window_session_managed_iface_init (LigmaSessionManagedInterface *iface)
{
  iface->get_aux_info = ligma_image_window_get_aux_info;
  iface->set_aux_info = ligma_image_window_set_aux_info;
}

static void
ligma_image_window_constructed (GObject *object)
{
  LigmaImageWindow        *window  = LIGMA_IMAGE_WINDOW (object);
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
  LigmaMenuFactory        *menu_factory;
  LigmaGuiConfig          *config;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LIGMA (private->ligma));
  ligma_assert (LIGMA_IS_DIALOG_FACTORY (private->dialog_factory));

  menu_factory = ligma_dialog_factory_get_menu_factory (private->dialog_factory);

  private->menubar_manager = ligma_menu_factory_manager_new (menu_factory,
                                                            "<Image>",
                                                            window);

  g_signal_connect_object (private->dialog_factory, "dock-window-added",
                           G_CALLBACK (ligma_image_window_update_ui_manager),
                           window, G_CONNECT_SWAPPED);
  g_signal_connect_object (private->dialog_factory, "dock-window-removed",
                           G_CALLBACK (ligma_image_window_update_ui_manager),
                           window, G_CONNECT_SWAPPED);

  gtk_window_add_accel_group (GTK_WINDOW (window),
                              ligma_ui_manager_get_accel_group (private->menubar_manager));

  g_signal_connect (private->menubar_manager, "show-tooltip",
                    G_CALLBACK (ligma_image_window_show_tooltip),
                    window);
  g_signal_connect (private->menubar_manager, "hide-tooltip",
                    G_CALLBACK (ligma_image_window_hide_tooltip),
                    window);

  config = LIGMA_GUI_CONFIG (private->ligma->config);

  /* Create the window toplevel container */
  private->main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (window), private->main_vbox);
  gtk_widget_show (private->main_vbox);

  /* Create the menubar */
#ifndef GDK_WINDOWING_QUARTZ
  private->menubar = ligma_ui_manager_get_widget (private->menubar_manager,
                                                 "/image-menubar");
#endif /* !GDK_WINDOWING_QUARTZ */
  if (private->menubar)
    {
      gtk_box_pack_start (GTK_BOX (private->main_vbox),
                          private->menubar, FALSE, FALSE, 0);

      /*  make sure we can activate accels even if the menubar is invisible
       *  (see https://bugzilla.gnome.org/show_bug.cgi?id=137151)
       */
      g_signal_connect (private->menubar, "can-activate-accel",
                        G_CALLBACK (gtk_true),
                        NULL);

      /*  active display callback  */
      g_signal_connect (private->menubar, "button-press-event",
                        G_CALLBACK (ligma_image_window_shell_events),
                        window);
      g_signal_connect (private->menubar, "button-release-event",
                        G_CALLBACK (ligma_image_window_shell_events),
                        window);
      g_signal_connect (private->menubar, "key-press-event",
                        G_CALLBACK (ligma_image_window_shell_events),
                        window);
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
    ligma_dock_columns_new (ligma_get_user_context (private->ligma),
                           private->dialog_factory,
                           private->menubar_manager);
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
                    G_CALLBACK (ligma_image_window_switch_page),
                    window);
  g_signal_connect (private->notebook, "page-removed",
                    G_CALLBACK (ligma_image_window_page_removed),
                    window);
  g_signal_connect (private->notebook, "page-reordered",
                    G_CALLBACK (ligma_image_window_page_reordered),
                    window);
  gtk_widget_show (private->notebook);

  /* Create the right dock columns widget */
  private->right_docks =
    ligma_dock_columns_new (ligma_get_user_context (private->ligma),
                           private->dialog_factory,
                           private->menubar_manager);
  gtk_paned_pack2 (GTK_PANED (private->right_hpane), private->right_docks,
                   FALSE, FALSE);
  gtk_widget_set_visible (private->right_docks, config->single_window_mode);

  g_signal_connect_object (config, "notify::single-window-mode",
                           G_CALLBACK (ligma_image_window_config_notify),
                           window, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::show-tabs",
                           G_CALLBACK (ligma_image_window_config_notify),
                           window, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::hide-docks",
                           G_CALLBACK (ligma_image_window_config_notify),
                           window, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::tabs-position",
                           G_CALLBACK (ligma_image_window_config_notify),
                           window, G_CONNECT_SWAPPED);

  ligma_image_window_session_update (window,
                                    NULL /*new_display*/,
                                    ligma_image_window_config_to_entry_id (config),
                                    private->initial_monitor);
}

static void
ligma_image_window_dispose (GObject *object)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (object);

  if (private->dialog_factory)
    {
      g_signal_handlers_disconnect_by_func (private->dialog_factory,
                                            ligma_image_window_update_ui_manager,
                                            object);
      private->dialog_factory = NULL;
    }

  g_clear_object (&private->menubar_manager);

  if (private->update_ui_manager_idle_id)
    {
      g_source_remove (private->update_ui_manager_idle_id);
      private->update_ui_manager_idle_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_image_window_finalize (GObject *object)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (object);

  if (private->shells)
    {
      g_list_free (private->shells);
      private->shells = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_image_window_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaImageWindow        *window  = LIGMA_IMAGE_WINDOW (object);
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  switch (property_id)
    {
    case PROP_LIGMA:
      private->ligma = g_value_get_object (value);
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
ligma_image_window_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaImageWindow        *window  = LIGMA_IMAGE_WINDOW (object);
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, private->ligma);
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
ligma_image_window_delete_event (GtkWidget   *widget,
                                GdkEventAny *event)
{
  LigmaImageWindow        *window  = LIGMA_IMAGE_WINDOW (widget);
  LigmaDisplayShell       *shell   = ligma_image_window_get_active_shell (window);
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
  LigmaGuiConfig          *config  = LIGMA_GUI_CONFIG (private->ligma->config);

  if (config->single_window_mode)
    ligma_ui_manager_activate_action (ligma_image_window_get_ui_manager (window),
                                     "file", "file-quit");
  else if (shell)
    ligma_display_shell_close (shell, FALSE);

  return TRUE;
}

static gboolean
ligma_image_window_configure_event (GtkWidget         *widget,
                                   GdkEventConfigure *event)
{
  LigmaImageWindow        *window  = LIGMA_IMAGE_WINDOW (widget);
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
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
      LigmaDisplayShell *shell = ligma_image_window_get_active_shell (window);

      if (shell && ligma_display_get_image (shell->display))
        shell->size_allocate_from_configure_event = TRUE;
    }

  scale_factor = gdk_window_get_scale_factor (gtk_widget_get_window (widget));

  if (scale_factor != private->scale_factor)
    {
      GList *list;

      private->scale_factor = scale_factor;

      for (list = private->shells; list; list = g_list_next (list))
        {
          LigmaDisplayShell *shell = list->data;

          ligma_display_shell_render_set_scale (shell, scale_factor);
          ligma_display_shell_expose_full (shell);
        }
    }

  return TRUE;
}

static void
ligma_image_window_update_csd_on_fullscreen (GtkWidget *child,
                                            gpointer   user_data)
{
  gboolean fullscreen = GPOINTER_TO_INT (user_data);

  if (GTK_IS_HEADER_BAR (child))
    gtk_widget_set_visible (child, !fullscreen);
}

static gboolean
ligma_image_window_window_state_event (GtkWidget           *widget,
                                      GdkEventWindowState *event)
{
  LigmaImageWindow        *window  = LIGMA_IMAGE_WINDOW (widget);
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
  LigmaDisplayShell       *shell   = ligma_image_window_get_active_shell (window);

  /* Run the parent implementation */
  if (GTK_WIDGET_CLASS (parent_class)->window_state_event)
    GTK_WIDGET_CLASS (parent_class)->window_state_event (widget, event);

  if (! shell)
    return FALSE;

  private->window_state = event->new_window_state;

  if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
    {
      gboolean fullscreen = ligma_image_window_get_fullscreen (window);

      LIGMA_LOG (WM, "Image window '%s' [%p] set fullscreen %s",
                gtk_window_get_title (GTK_WINDOW (widget)),
                widget,
                fullscreen ? "TRUE" : "FALSE");

      ligma_image_window_suspend_keep_pos (window);
      ligma_display_shell_appearance_update (shell);
      ligma_image_window_resume_keep_pos (window);

      /* When using CSD (for example in Wayland), our title bar stays visible
       * when going fullscreen by default. There is no getter for it and it's
       * an internal child, so we use this workaround instead */
      gtk_container_forall (GTK_CONTAINER (window),
                            ligma_image_window_update_csd_on_fullscreen,
                            GINT_TO_POINTER (fullscreen));
    }

  if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED)
    {
      LigmaStatusbar *statusbar = ligma_display_shell_get_statusbar (shell);
      gboolean       iconified = ligma_image_window_is_iconified (window);

      LIGMA_LOG (WM, "Image window '%s' [%p] set %s",
                gtk_window_get_title (GTK_WINDOW (widget)),
                widget,
                iconified ? "iconified" : "uniconified");

      if (iconified)
        {
          if (ligma_displays_get_num_visible (private->ligma) == 0)
            {
              LIGMA_LOG (WM, "No displays visible any longer");

              ligma_dialog_factory_hide_with_display (private->dialog_factory);
            }
        }
      else
        {
          ligma_dialog_factory_show_with_display (private->dialog_factory);
        }

      if (ligma_progress_is_active (LIGMA_PROGRESS (statusbar)))
        {
          if (iconified)
            ligma_statusbar_override_window_title (statusbar);
          else
            gtk_window_set_title (GTK_WINDOW (window), shell->title);
        }
    }

  return FALSE;
}

static void
ligma_image_window_style_updated (GtkWidget *widget)
{
  LigmaImageWindow        *window        = LIGMA_IMAGE_WINDOW (widget);
  LigmaImageWindowPrivate *private       = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
  LigmaDisplayShell       *shell         = ligma_image_window_get_active_shell (window);
  LigmaStatusbar          *statusbar     = NULL;
  GtkRequisition          requisition   = { 0, };
  GdkGeometry             geometry      = { 0, };
  GdkWindowHints          geometry_mask = 0;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (! shell)
    return;

  statusbar = ligma_display_shell_get_statusbar (shell);

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
   *  set by ligma. All other displays should be placed by the window
   *  manager. See https://bugzilla.gnome.org/show_bug.cgi?id=559580
   */
  if (! ligma_display_get_image (shell->display))
    geometry_mask |= GDK_HINT_USER_POS;

  gtk_window_set_geometry_hints (GTK_WINDOW (widget), NULL,
                                 &geometry, geometry_mask);

  ligma_dialog_factory_set_has_min_size (GTK_WINDOW (widget), TRUE);
}

static void
ligma_image_window_monitor_changed (LigmaWindow *window,
                                   GdkMonitor *monitor)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
  GList                  *list;

  for (list = private->shells; list; list = g_list_next (list))
    {
      /*  hack, this should live here, and screen_changed call
       *  monitor_changed
       */
      g_signal_emit_by_name (list->data, "screen-changed",
                             gtk_widget_get_screen (list->data));

      /*  make it fetch the new monitor's resolution  */
      ligma_display_shell_scale_update (LIGMA_DISPLAY_SHELL (list->data));

      /*  make it fetch the right monitor profile  */
      ligma_color_managed_profile_changed (LIGMA_COLOR_MANAGED (list->data));
    }
}

static GList *
ligma_image_window_get_docks (LigmaDockContainer *dock_container)
{
  LigmaImageWindowPrivate *private;
  GList                  *iter;
  GList                  *all_docks = NULL;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (dock_container), FALSE);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (dock_container);

  for (iter = ligma_dock_columns_get_docks (LIGMA_DOCK_COLUMNS (private->left_docks));
       iter;
       iter = g_list_next (iter))
    {
      all_docks = g_list_append (all_docks, LIGMA_DOCK (iter->data));
    }

  for (iter = ligma_dock_columns_get_docks (LIGMA_DOCK_COLUMNS (private->right_docks));
       iter;
       iter = g_list_next (iter))
    {
      all_docks = g_list_append (all_docks, LIGMA_DOCK (iter->data));
    }

  return all_docks;
}

static LigmaDialogFactory *
ligma_image_window_dock_container_get_dialog_factory (LigmaDockContainer *dock_container)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (dock_container);

  return private->dialog_factory;
}

static LigmaUIManager *
ligma_image_window_dock_container_get_ui_manager (LigmaDockContainer *dock_container)
{
  LigmaImageWindow *window = LIGMA_IMAGE_WINDOW (dock_container);

  return ligma_image_window_get_ui_manager (window);
}

void
ligma_image_window_add_dock (LigmaDockContainer   *dock_container,
                            LigmaDock            *dock,
                            LigmaSessionInfoDock *dock_info)
{
  LigmaImageWindow        *window;
  LigmaDisplayShell       *active_shell;
  LigmaImageWindowPrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (dock_container));

  window  = LIGMA_IMAGE_WINDOW (dock_container);
  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  if (dock_info->side == LIGMA_ALIGN_LEFT)
    {
      ligma_dock_columns_add_dock (LIGMA_DOCK_COLUMNS (private->left_docks),
                                  dock,
                                  -1 /*index*/);
    }
  else
    {
      ligma_dock_columns_add_dock (LIGMA_DOCK_COLUMNS (private->right_docks),
                                  dock,
                                  -1 /*index*/);
    }

  active_shell = ligma_image_window_get_active_shell (window);
  if (active_shell)
    ligma_display_shell_appearance_update (active_shell);
}

static LigmaAlignmentType
ligma_image_window_get_dock_side (LigmaDockContainer *dock_container,
                                 LigmaDock          *dock)
{
  LigmaAlignmentType       side = -1;
  LigmaImageWindowPrivate *private;
  GList                  *iter;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (dock_container), FALSE);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (dock_container);

  for (iter = ligma_dock_columns_get_docks (LIGMA_DOCK_COLUMNS (private->left_docks));
       iter && side == -1;
       iter = g_list_next (iter))
    {
      LigmaDock *dock_iter = LIGMA_DOCK (iter->data);

      if (dock_iter == dock)
        side = LIGMA_ALIGN_LEFT;
    }

  for (iter = ligma_dock_columns_get_docks (LIGMA_DOCK_COLUMNS (private->right_docks));
       iter && side == -1;
       iter = g_list_next (iter))
    {
      LigmaDock *dock_iter = LIGMA_DOCK (iter->data);

      if (dock_iter == dock)
        side = LIGMA_ALIGN_RIGHT;
    }

  return side;
}

static GList *
ligma_image_window_get_aux_info (LigmaSessionManaged *session_managed)
{
  GList                  *aux_info = NULL;
  LigmaImageWindowPrivate *private;
  LigmaGuiConfig          *config;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (session_managed), NULL);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (session_managed);
  config  = LIGMA_GUI_CONFIG (private->ligma->config);

  if (config->single_window_mode)
    {
      LigmaSessionInfoAux *aux;
      GtkAllocation       allocation;
      gchar               widthbuf[128];

      g_snprintf (widthbuf, sizeof (widthbuf), "%d",
                  gtk_paned_get_position (GTK_PANED (private->left_hpane)));
      aux = ligma_session_info_aux_new (LIGMA_IMAGE_WINDOW_LEFT_DOCKS_WIDTH,
                                       widthbuf);
      aux_info = g_list_append (aux_info, aux);

      gtk_widget_get_allocation (private->right_hpane, &allocation);

      g_snprintf (widthbuf, sizeof (widthbuf), "%d",
                  allocation.width -
                  gtk_paned_get_position (GTK_PANED (private->right_hpane)));
      aux = ligma_session_info_aux_new (LIGMA_IMAGE_WINDOW_RIGHT_DOCKS_WIDTH,
                                       widthbuf);
      aux_info = g_list_append (aux_info, aux);

      aux = ligma_session_info_aux_new (LIGMA_IMAGE_WINDOW_MAXIMIZED,
                                       ligma_image_window_is_maximized (LIGMA_IMAGE_WINDOW (session_managed)) ?
                                       "yes" : "no");
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static void
ligma_image_window_set_right_docks_width (GtkPaned      *paned,
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
                                        ligma_image_window_set_right_docks_width,
                                        data);
}

static void
ligma_image_window_set_aux_info (LigmaSessionManaged *session_managed,
                                GList              *aux_info)
{
  LigmaImageWindowPrivate *private;
  GList                  *iter;
  gint                    left_docks_width      = G_MININT;
  gint                    right_docks_width     = G_MININT;
  gboolean                wait_with_right_docks = FALSE;
  gboolean                maximized             = FALSE;
#ifdef G_OS_WIN32
  STARTUPINFO             StartupInfo;

  GetStartupInfo (&StartupInfo);
#endif

  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (session_managed));

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (session_managed);

  for (iter = aux_info; iter; iter = g_list_next (iter))
    {
      LigmaSessionInfoAux *aux   = iter->data;
      gint               *width = NULL;

      if (! strcmp (aux->name, LIGMA_IMAGE_WINDOW_LEFT_DOCKS_WIDTH))
        width = &left_docks_width;
      else if (! strcmp (aux->name, LIGMA_IMAGE_WINDOW_RIGHT_DOCKS_WIDTH))
        width = &right_docks_width;
      else if (! strcmp (aux->name, LIGMA_IMAGE_WINDOW_RIGHT_DOCKS_POS))
        width = &right_docks_width;
      else if (! strcmp (aux->name, LIGMA_IMAGE_WINDOW_MAXIMIZED))
        if (! g_ascii_strcasecmp (aux->value, "yes"))
          maximized = TRUE;

      if (width)
        sscanf (aux->value, "%d", width);

      /* compat handling for right docks */
      if (! strcmp (aux->name, LIGMA_IMAGE_WINDOW_RIGHT_DOCKS_POS))
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
                                 G_CALLBACK (ligma_image_window_set_right_docks_width),
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
   * line: `start /max ligma-2.9.exe` or with the shortcut's "run"
   * property.
   * When such a hint is given, we should follow it and bypass the
   * session's information.
   */
  if (StartupInfo.wShowWindow == SW_SHOWMAXIMIZED)
    gtk_window_maximize (GTK_WINDOW (session_managed));
  else if (StartupInfo.wShowWindow == SW_SHOWMINIMIZED   ||
           StartupInfo.wShowWindow == SW_SHOWMINNOACTIVE ||
           StartupInfo.wShowWindow == SW_MINIMIZE)
    /* XXX Iconification does not seem to work. I see the
     * window being iconified and immediately re-raised.
     * I leave this piece of code for later improvement. */
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

LigmaImageWindow *
ligma_image_window_new (Ligma              *ligma,
                       LigmaImage         *image,
                       LigmaDialogFactory *dialog_factory,
                       GdkMonitor        *monitor)
{
  LigmaImageWindow        *window;
  LigmaImageWindowPrivate *private;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (image == NULL || LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  window = g_object_new (LIGMA_TYPE_IMAGE_WINDOW,
                         "ligma",            ligma,
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

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  ligma->image_windows = g_list_append (ligma->image_windows, window);

  if (! LIGMA_GUI_CONFIG (private->ligma->config)->single_window_mode)
    {
      GdkMonitor *pointer_monitor = ligma_get_monitor_at_pointer ();

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
ligma_image_window_destroy (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (window));

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  private->ligma->image_windows = g_list_remove (private->ligma->image_windows,
                                                window);

  gtk_widget_destroy (GTK_WIDGET (window));
}

LigmaUIManager *
ligma_image_window_get_ui_manager (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (window), FALSE);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  return private->menubar_manager;
}

LigmaDockColumns  *
ligma_image_window_get_left_docks (LigmaImageWindow  *window)
{
  LigmaImageWindowPrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (window), FALSE);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  return LIGMA_DOCK_COLUMNS (private->left_docks);
}

LigmaDockColumns  *
ligma_image_window_get_right_docks (LigmaImageWindow  *window)
{
  LigmaImageWindowPrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (window), FALSE);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  return LIGMA_DOCK_COLUMNS (private->right_docks);
}

void
ligma_image_window_add_shell (LigmaImageWindow  *window,
                             LigmaDisplayShell *shell)
{
  LigmaImageWindowPrivate *private;
  GtkWidget              *tab_label;

  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (window));
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  g_return_if_fail (g_list_find (private->shells, shell) == NULL);

  private->shells = g_list_append (private->shells, shell);

  tab_label = ligma_image_window_create_tab_label (window, shell);

  gtk_notebook_append_page (GTK_NOTEBOOK (private->notebook),
                            GTK_WIDGET (shell), tab_label);
  gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (private->notebook),
                                    GTK_WIDGET (shell), TRUE);

  gtk_widget_show (GTK_WIDGET (shell));

  /*  make it fetch the right monitor profile  */
  ligma_color_managed_profile_changed (LIGMA_COLOR_MANAGED (shell));
}

LigmaDisplayShell *
ligma_image_window_get_shell (LigmaImageWindow *window,
                             gint             index)
{
  LigmaImageWindowPrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (window), NULL);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  return g_list_nth_data (private->shells, index);
}

void
ligma_image_window_remove_shell (LigmaImageWindow  *window,
                                LigmaDisplayShell *shell)
{
  LigmaImageWindowPrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (window));
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  g_return_if_fail (g_list_find (private->shells, shell) != NULL);

  private->shells = g_list_remove (private->shells, shell);

  gtk_container_remove (GTK_CONTAINER (private->notebook),
                        GTK_WIDGET (shell));
}

gint
ligma_image_window_get_n_shells (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (window), 0);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  return g_list_length (private->shells);
}

void
ligma_image_window_set_active_shell (LigmaImageWindow  *window,
                                    LigmaDisplayShell *shell)
{
  LigmaImageWindowPrivate *private;
  gint                    page_num;

  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (window));
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  g_return_if_fail (g_list_find (private->shells, shell));

  page_num = gtk_notebook_page_num (GTK_NOTEBOOK (private->notebook),
                                    GTK_WIDGET (shell));

  gtk_notebook_set_current_page (GTK_NOTEBOOK (private->notebook), page_num);
}

LigmaDisplayShell *
ligma_image_window_get_active_shell (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (window), NULL);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  return private->active_shell;
}

void
ligma_image_window_set_fullscreen (LigmaImageWindow *window,
                                  gboolean         fullscreen)
{
  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (window));

  if (fullscreen != ligma_image_window_get_fullscreen (window))
    {
      if (fullscreen)
        gtk_window_fullscreen (GTK_WINDOW (window));
      else
        gtk_window_unfullscreen (GTK_WINDOW (window));
    }
}

gboolean
ligma_image_window_get_fullscreen (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (window), FALSE);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  return (private->window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
}

void
ligma_image_window_set_show_menubar (LigmaImageWindow *window,
                                    gboolean         show)
{
  LigmaImageWindowPrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (window));

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  if (private->menubar)
    gtk_widget_set_visible (private->menubar, show);
}

gboolean
ligma_image_window_get_show_menubar (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (window), FALSE);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  if (private->menubar)
    return gtk_widget_get_visible (private->menubar);

  return FALSE;
}

gboolean
ligma_image_window_is_iconified (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (window), FALSE);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  return (private->window_state & GDK_WINDOW_STATE_ICONIFIED) != 0;
}

gboolean
ligma_image_window_is_maximized (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (window), FALSE);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  return (private->window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
}

/**
 * ligma_image_window_has_toolbox:
 * @window:
 *
 * Returns: %TRUE if the image window contains a LigmaToolbox.
 **/
gboolean
ligma_image_window_has_toolbox (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private;
  GList                  *iter = NULL;

  g_return_val_if_fail (LIGMA_IS_IMAGE_WINDOW (window), FALSE);

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  for (iter = ligma_dock_columns_get_docks (LIGMA_DOCK_COLUMNS (private->left_docks));
       iter;
       iter = g_list_next (iter))
    {
      if (LIGMA_IS_TOOLBOX (iter->data))
        return TRUE;
    }

  for (iter = ligma_dock_columns_get_docks (LIGMA_DOCK_COLUMNS (private->right_docks));
       iter;
       iter = g_list_next (iter))
    {
      if (LIGMA_IS_TOOLBOX (iter->data))
        return TRUE;
    }

  return FALSE;
}

void
ligma_image_window_shrink_wrap (LigmaImageWindow *window,
                               gboolean         grow_only)
{
  LigmaDisplayShell *active_shell;
  GtkWidget        *widget;
  GtkAllocation     allocation;
  GdkMonitor       *monitor;
  GdkRectangle      rect;
  gint              disp_width, disp_height;
  gint              width, height;
  gint              max_auto_width, max_auto_height;
  gint              border_width, border_height;
  gboolean          resize = FALSE;

  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (window));

  if (! gtk_widget_get_realized (GTK_WIDGET (window)))
    return;

  /* FIXME this so needs cleanup and shell/window separation */

  active_shell = ligma_image_window_get_active_shell (window);

  if (!active_shell)
    return;

  widget  = GTK_WIDGET (window);
  monitor = ligma_widget_get_monitor (widget);

  gtk_widget_get_allocation (widget, &allocation);

  gdk_monitor_get_workarea (monitor, &rect);

  if (! ligma_display_shell_get_infinite_canvas (active_shell))
    {
      ligma_display_shell_scale_get_image_size (active_shell,
                                               &width, &height);
    }
  else
    {
      ligma_display_shell_scale_get_image_bounding_box (active_shell,
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
      LigmaStatusbar *statusbar = ligma_display_shell_get_statusbar (active_shell);
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
   * LigmaDisplayShell::configure_event().
   */
  /* FIXME multiple shells */
  ligma_display_shell_scroll_center_content (active_shell, TRUE, TRUE);
}

static GtkWidget *
ligma_image_window_get_first_dockbook (LigmaDockColumns *columns)
{
  GList *dock_iter;

  for (dock_iter = ligma_dock_columns_get_docks (columns);
       dock_iter;
       dock_iter = g_list_next (dock_iter))
    {
      LigmaDock *dock      = LIGMA_DOCK (dock_iter->data);
      GList    *dockbooks = ligma_dock_get_dockbooks (dock);

      if (dockbooks)
        return GTK_WIDGET (dockbooks->data);
    }

  return NULL;
}

/**
 * ligma_image_window_get_default_dockbook:
 * @window:
 *
 * Gets the default dockbook, which is the dockbook in which new
 * dockables should be put in single-window mode.
 *
 * Returns: (nullable): The default dockbook for new dockables, or %NULL if no
 *          dockbook were available.
 **/
GtkWidget *
ligma_image_window_get_default_dockbook (LigmaImageWindow  *window)
{
  LigmaImageWindowPrivate *private;
  LigmaDockColumns        *dock_columns;
  GtkWidget              *dockbook = NULL;

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  /* First try the first dockbook in the right docks */
  dock_columns = LIGMA_DOCK_COLUMNS (private->right_docks);
  dockbook     = ligma_image_window_get_first_dockbook (dock_columns);

  /* Then the left docks */
  if (! dockbook)
    {
      dock_columns = LIGMA_DOCK_COLUMNS (private->left_docks);
      dockbook     = ligma_image_window_get_first_dockbook (dock_columns);
    }

  return dockbook;
}

/**
 * ligma_image_window_keep_canvas_pos:
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
ligma_image_window_keep_canvas_pos (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private;
  LigmaDisplayShell       *shell;
  gint                    canvas_x;
  gint                    canvas_y;
  gint                    window_x;
  gint                    window_y;

  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (window));

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  if (private->suspend_keep_pos > 0)
    return;

  shell = ligma_image_window_get_active_shell (window);

  ligma_display_shell_transform_xy (shell, 0.0, 0.0, &canvas_x, &canvas_y);

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
                             G_CALLBACK (ligma_image_window_shell_size_allocate),
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
ligma_image_window_suspend_keep_pos (LigmaImageWindow  *window)
{
  LigmaImageWindowPrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (window));

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  private->suspend_keep_pos++;
}

void
ligma_image_window_resume_keep_pos (LigmaImageWindow  *window)
{
  LigmaImageWindowPrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (window));

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  g_return_if_fail (private->suspend_keep_pos > 0);

  private->suspend_keep_pos--;
}

/**
 * ligma_image_window_update_tabs:
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
ligma_image_window_update_tabs (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private;
  LigmaGuiConfig          *config;
  GtkPositionType         position;

  g_return_if_fail (LIGMA_IS_IMAGE_WINDOW (window));

  private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
  config = LIGMA_GUI_CONFIG (private->ligma->config);

  /* Tab visibility. */
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (private->notebook),
                              config->single_window_mode &&
                              config->show_tabs          &&
                              ! config->hide_docks       &&
                              ((private->active_shell          &&
                                private->active_shell->display &&
                                ligma_display_get_image (private->active_shell->display)) ||
                               g_list_length (private->shells) > 1));

  /* Tab position. */
  switch (config->tabs_position)
    {
    case LIGMA_POSITION_TOP:
      position = GTK_POS_TOP;
      break;
    case LIGMA_POSITION_BOTTOM:
      position = GTK_POS_BOTTOM;
      break;
    case LIGMA_POSITION_LEFT:
      position = GTK_POS_LEFT;
      break;
    case LIGMA_POSITION_RIGHT:
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
ligma_image_window_show_tooltip (LigmaUIManager   *manager,
                                const gchar     *tooltip,
                                LigmaImageWindow *window)
{
  LigmaDisplayShell *shell     = ligma_image_window_get_active_shell (window);
  LigmaStatusbar    *statusbar = NULL;

  if (! shell)
    return;

  statusbar = ligma_display_shell_get_statusbar (shell);

  ligma_statusbar_push (statusbar, "menu-tooltip",
                       NULL, "%s", tooltip);
}

static void
ligma_image_window_config_notify (LigmaImageWindow *window,
                                 GParamSpec      *pspec,
                                 LigmaGuiConfig   *config)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

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

          ligma_image_window_keep_canvas_pos (window);
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
              LigmaDisplayShell *shell;
              LigmaContext      *user_context;

              shell        = ligma_image_window_get_active_shell (window);
              user_context = ligma_get_user_context (private->ligma);

              if (ligma_context_get_display (user_context) == shell->display)
                {
                  GdkWindow *w = gtk_widget_get_window (GTK_WIDGET (window));

                  if (w)
                    gdk_window_focus (w, gtk_get_current_event_time ());
                }
            }
        }

      ligma_image_window_update_tabs (window);
    }

  /* Session management */
  if (strcmp (pspec->name, "single-window-mode") == 0)
    {
      ligma_image_window_session_update (window,
                                        NULL /*new_display*/,
                                        ligma_image_window_config_to_entry_id (config),
                                        ligma_widget_get_monitor (GTK_WIDGET (window)));
    }
}

static void
ligma_image_window_hide_tooltip (LigmaUIManager   *manager,
                                LigmaImageWindow *window)
{
  LigmaDisplayShell *shell     = ligma_image_window_get_active_shell (window);
  LigmaStatusbar    *statusbar = NULL;

  if (! shell)
    return;

  statusbar = ligma_display_shell_get_statusbar (shell);

  ligma_statusbar_pop (statusbar, "menu-tooltip");
}

static gboolean
ligma_image_window_update_ui_manager_idle (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  ligma_assert (private->active_shell != NULL);

  ligma_ui_manager_update (private->menubar_manager,
                          private->active_shell->display);

  private->update_ui_manager_idle_id = 0;

  return G_SOURCE_REMOVE;
}

static void
ligma_image_window_update_ui_manager (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  if (! private->update_ui_manager_idle_id)
    {
      private->update_ui_manager_idle_id =
        g_idle_add_full (LIGMA_PRIORITY_IMAGE_WINDOW_UPDATE_UI_MANAGER_IDLE,
                         (GSourceFunc) ligma_image_window_update_ui_manager_idle,
                         window,
                         NULL);
    }
}

static void
ligma_image_window_shell_size_allocate (LigmaDisplayShell  *shell,
                                       GtkAllocation     *allocation,
                                       PosCorrectionData *data)
{
  LigmaImageWindow *window = ligma_display_shell_get_window (shell);
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
        ligma_display_shell_scroll (shell, off_x, off_y);
    }

  g_signal_handlers_disconnect_by_func (shell,
                                        ligma_image_window_shell_size_allocate,
                                        data);
}

static gboolean
ligma_image_window_shell_events (GtkWidget       *widget,
                                GdkEvent        *event,
                                LigmaImageWindow *window)
{
  LigmaDisplayShell *shell = ligma_image_window_get_active_shell (window);

  if (! shell)
    return FALSE;

  return ligma_display_shell_events (widget, event, shell);
}

static void
ligma_image_window_switch_page (GtkNotebook     *notebook,
                               gpointer         page,
                               gint             page_num,
                               LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
  LigmaDisplayShell       *shell;
  LigmaDisplay            *active_display;

  shell = LIGMA_DISPLAY_SHELL (gtk_notebook_get_nth_page (notebook, page_num));

  if (shell == private->active_shell)
    return;

  ligma_image_window_disconnect_from_active_shell (window);

  LIGMA_LOG (WM, "LigmaImageWindow %p, private->active_shell = %p; \n",
            window, shell);
  private->active_shell = shell;

  ligma_window_set_primary_focus_widget (LIGMA_WINDOW (window),
                                        shell->canvas);

  active_display = private->active_shell->display;

  g_signal_connect (active_display, "notify::image",
                    G_CALLBACK (ligma_image_window_image_notify),
                    window);

  g_signal_connect (private->active_shell, "scaled",
                    G_CALLBACK (ligma_image_window_shell_scaled),
                    window);
  g_signal_connect (private->active_shell, "rotated",
                    G_CALLBACK (ligma_image_window_shell_rotated),
                    window);
  g_signal_connect (private->active_shell, "notify::title",
                    G_CALLBACK (ligma_image_window_shell_title_notify),
                    window);

  gtk_window_set_title (GTK_WINDOW (window), shell->title);

  ligma_display_shell_appearance_update (private->active_shell);

  if (gtk_widget_get_window (GTK_WIDGET (window)))
    {
      /*  we are fully initialized, use the window's current monitor
       */
      ligma_image_window_session_update (window,
                                        active_display,
                                        NULL /*new_entry_id*/,
                                        ligma_widget_get_monitor (GTK_WIDGET (window)));
    }
  else
    {
      /*  we are in construction, use the initial monitor; calling
       *  ligma_widget_get_monitor() would get us the monitor where the
       *  pointer is
       */
      ligma_image_window_session_update (window,
                                        active_display,
                                        NULL /*new_entry_id*/,
                                        private->initial_monitor);
    }

  ligma_context_set_display (ligma_get_user_context (private->ligma),
                            active_display);

  ligma_image_window_update_ui_manager (window);

  ligma_image_window_update_tab_labels (window);
}

static void
ligma_image_window_page_removed (GtkNotebook     *notebook,
                                GtkWidget       *widget,
                                gint             page_num,
                                LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

  if (GTK_WIDGET (private->active_shell) == widget)
    {
      LIGMA_LOG (WM, "LigmaImageWindow %p, private->active_shell = %p; \n",
                window, NULL);
      ligma_image_window_disconnect_from_active_shell (window);
      private->active_shell = NULL;
    }
}

static void
ligma_image_window_page_reordered (GtkNotebook     *notebook,
                                  GtkWidget       *widget,
                                  gint             page_num,
                                  LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private  = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
  LigmaContainer          *displays = private->ligma->displays;
  gint                    index    = g_list_index (private->shells, widget);

  if (index != page_num)
    {
      private->shells = g_list_remove (private->shells, widget);
      private->shells = g_list_insert (private->shells, widget, page_num);
    }

  /* We need to reorder the displays as well in order to update the
   * numbered accelerators (alt-1, alt-2, etc.).
   */
  ligma_container_reorder (displays,
                          LIGMA_OBJECT (LIGMA_DISPLAY_SHELL (widget)->display),
                          page_num);

  gtk_notebook_reorder_child (notebook, widget, page_num);
}

static void
ligma_image_window_disconnect_from_active_shell (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private        = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
  LigmaDisplay            *active_display = NULL;

  if (! private->active_shell)
    return;

  active_display = private->active_shell->display;

  if (active_display)
    g_signal_handlers_disconnect_by_func (active_display,
                                          ligma_image_window_image_notify,
                                          window);

  g_signal_handlers_disconnect_by_func (private->active_shell,
                                        ligma_image_window_shell_scaled,
                                        window);
  g_signal_handlers_disconnect_by_func (private->active_shell,
                                        ligma_image_window_shell_rotated,
                                        window);
  g_signal_handlers_disconnect_by_func (private->active_shell,
                                        ligma_image_window_shell_title_notify,
                                        window);

  if (private->menubar_manager)
    ligma_image_window_hide_tooltip (private->menubar_manager, window);

  if (private->update_ui_manager_idle_id)
    {
      g_source_remove (private->update_ui_manager_idle_id);
      private->update_ui_manager_idle_id = 0;
    }
}

static void
ligma_image_window_image_notify (LigmaDisplay      *display,
                                const GParamSpec *pspec,
                                LigmaImageWindow  *window)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
  GtkWidget              *tab_label;
  GList                  *children;
  GtkWidget              *view;

  ligma_image_window_session_update (window,
                                    display,
                                    NULL /*new_entry_id*/,
                                    ligma_widget_get_monitor (GTK_WIDGET (window)));

  tab_label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (private->notebook),
                                          GTK_WIDGET (ligma_display_get_shell (display)));
  children  = gtk_container_get_children (GTK_CONTAINER (tab_label));
  view      = GTK_WIDGET (children->data);
  g_list_free (children);

  ligma_view_set_viewable (LIGMA_VIEW (view),
                          LIGMA_VIEWABLE (ligma_display_get_image (display)));

  ligma_image_window_update_ui_manager (window);
}

static void
ligma_image_window_session_clear (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
  GtkWidget              *widget  = GTK_WIDGET (window);

  if (ligma_dialog_factory_from_widget (widget, NULL))
    ligma_dialog_factory_remove_dialog (private->dialog_factory,
                                       widget);
}

static void
ligma_image_window_session_apply (LigmaImageWindow *window,
                                 const gchar     *entry_id,
                                 GdkMonitor      *monitor)
{
  LigmaImageWindowPrivate *private      = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
  LigmaSessionInfo        *session_info = NULL;
  gint                    width        = -1;
  gint                    height       = -1;

  gtk_window_unfullscreen (GTK_WINDOW (window));

  /*  get the NIW size before adding the display to the dialog
   *  factory so the window's current size doesn't affect the
   *  stored session info entry.
   */
  session_info =
    ligma_dialog_factory_find_session_info (private->dialog_factory, entry_id);

  if (session_info)
    {
      width  = ligma_session_info_get_width  (session_info);
      height = ligma_session_info_get_height (session_info);
    }
  else
    {
      GtkAllocation allocation;

      gtk_widget_get_allocation (GTK_WIDGET (window), &allocation);

      width  = allocation.width;
      height = allocation.height;
    }

  ligma_dialog_factory_add_foreign (private->dialog_factory,
                                   entry_id,
                                   GTK_WIDGET (window),
                                   monitor);

  gtk_window_unmaximize (GTK_WINDOW (window));
  gtk_window_resize (GTK_WINDOW (window), width, height);
}

static void
ligma_image_window_session_update (LigmaImageWindow *window,
                                  LigmaDisplay     *new_display,
                                  const gchar     *new_entry_id,
                                  GdkMonitor      *monitor)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);

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
          if (strcmp (new_entry_id, LIGMA_SINGLE_IMAGE_WINDOW_ENTRY_ID) == 0)
            {
              ligma_image_window_session_apply (window, new_entry_id, monitor);
            }
        }
      else if (strcmp (private->entry_id, new_entry_id) != 0)
        {
          /* The entry id changed, immediately and always stop session
           * managing the old entry
           */
          ligma_image_window_session_clear (window);

          if (strcmp (new_entry_id, LIGMA_EMPTY_IMAGE_WINDOW_ENTRY_ID) == 0)
            {
              /* If there is only one imageless display, we shall
               * become the empty image window
               */
              if (private->active_shell &&
                  private->active_shell->display &&
                  ! ligma_display_get_image (private->active_shell->display) &&
                  g_list_length (private->shells) <= 1)
                {
                  ligma_image_window_session_apply (window, new_entry_id,
                                                   monitor);
                }
            }
          else if (strcmp (new_entry_id, LIGMA_SINGLE_IMAGE_WINDOW_ENTRY_ID) == 0)
            {
              /* As soon as we become the single image window, we
               * shall session manage ourself until single-window mode
               * is exited
               */
              ligma_image_window_session_apply (window, new_entry_id,
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
      strcmp (private->entry_id, LIGMA_EMPTY_IMAGE_WINDOW_ENTRY_ID) == 0)
    {
      if (ligma_display_get_image (new_display))
        {
          /* As soon as we have an image we should not affect the size of the
           * empty image window
           */
          ligma_image_window_session_clear (window);
        }
      else if (! ligma_display_get_image (new_display) &&
               g_list_length (private->shells) <= 1)
        {
          /* As soon as we have no image (and no other shells that may
           * contain images) we should become the empty image window
           */
          ligma_image_window_session_apply (window, private->entry_id,
                                           monitor);
        }
    }
}

static const gchar *
ligma_image_window_config_to_entry_id (LigmaGuiConfig *config)
{
  return (config->single_window_mode ?
          LIGMA_SINGLE_IMAGE_WINDOW_ENTRY_ID :
          LIGMA_EMPTY_IMAGE_WINDOW_ENTRY_ID);
}

static void
ligma_image_window_shell_scaled (LigmaDisplayShell *shell,
                                LigmaImageWindow  *window)
{
  /* update the <Image>/View/Zoom menu */
  ligma_image_window_update_ui_manager (window);
}

static void
ligma_image_window_shell_rotated (LigmaDisplayShell *shell,
                                 LigmaImageWindow  *window)
{
  /* update the <Image>/View/Rotate menu */
  ligma_image_window_update_ui_manager (window);
}

static void
ligma_image_window_shell_title_notify (LigmaDisplayShell *shell,
                                      const GParamSpec *pspec,
                                      LigmaImageWindow  *window)
{
  gtk_window_set_title (GTK_WINDOW (window), shell->title);
}

static void
ligma_image_window_shell_close_button_callback (LigmaDisplayShell *shell)
{
  if (shell)
    ligma_display_shell_close (shell, FALSE);
}

static GtkWidget *
ligma_image_window_create_tab_label (LigmaImageWindow  *window,
                                    LigmaDisplayShell *shell)
{
  GtkWidget *hbox;
  GtkWidget *view;
  LigmaImage *image;
  GtkWidget *button;
  GtkWidget *gtk_image;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_show (hbox);

  view = ligma_view_new_by_types (ligma_get_user_context (shell->display->ligma),
                                 LIGMA_TYPE_VIEW, LIGMA_TYPE_IMAGE,
                                 LIGMA_VIEW_SIZE_LARGE, 0, FALSE);
  gtk_widget_set_size_request (view, LIGMA_VIEW_SIZE_LARGE, -1);
  ligma_view_renderer_set_color_config (LIGMA_VIEW (view)->renderer,
                                       ligma_display_shell_get_color_config (shell));
  gtk_box_pack_start (GTK_BOX (hbox), view, FALSE, FALSE, 0);
  gtk_widget_show (view);

  image = ligma_display_get_image (shell->display);
  if (image)
    ligma_view_set_viewable (LIGMA_VIEW (view), LIGMA_VIEWABLE (image));

  button = gtk_button_new ();
  gtk_widget_set_can_focus (button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_image = gtk_image_new_from_icon_name (LIGMA_ICON_CLOSE,
                                            GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), gtk_image);
  gtk_widget_show (gtk_image);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (ligma_image_window_shell_close_button_callback),
                            shell);

  g_object_set_data (G_OBJECT (hbox), "close-button", button);

  return hbox;
}

static void
ligma_image_window_update_tab_labels (LigmaImageWindow *window)
{
  LigmaImageWindowPrivate *private = LIGMA_IMAGE_WINDOW_GET_PRIVATE (window);
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

      if (ligma_context_get_display (ligma_get_user_context (private->ligma)) ==
          LIGMA_DISPLAY_SHELL (shell)->display)
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
