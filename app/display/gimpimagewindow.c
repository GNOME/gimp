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

#undef GSEAL_ENABLE

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockcolumns.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpview.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-close.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpimagewindow.h"
#include "gimpstatusbar.h"

#include "gimp-log.h"
#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_MENU_FACTORY,
  PROP_DIALOG_FACTORY,
};


typedef struct _GimpImageWindowPrivate GimpImageWindowPrivate;

struct _GimpImageWindowPrivate
{
  Gimp              *gimp;
  GimpUIManager     *menubar_manager;
  GimpDialogFactory *dialog_factory;

  GList             *shells;
  GimpDisplayShell  *active_shell;

  GtkWidget         *main_vbox;
  GtkWidget         *menubar;
  GtkWidget         *hbox;
  GtkWidget         *left_hpane;
  GtkWidget         *left_docks;
  GtkWidget         *right_hpane;
  GtkWidget         *notebook;
  GtkWidget         *right_docks;

  GdkWindowState     window_state;
  gboolean           is_empty;
};

#define GIMP_IMAGE_WINDOW_GET_PRIVATE(window) \
        G_TYPE_INSTANCE_GET_PRIVATE (window, \
                                     GIMP_TYPE_IMAGE_WINDOW, \
                                     GimpImageWindowPrivate)


/*  local function prototypes  */

static GObject * gimp_image_window_constructor         (GType                type,
                                                        guint                n_params,
                                                        GObjectConstructParam *params);
static void      gimp_image_window_finalize            (GObject             *object);
static void      gimp_image_window_set_property        (GObject             *object,
                                                        guint                property_id,
                                                        const GValue        *value,
                                                        GParamSpec          *pspec);
static void      gimp_image_window_get_property        (GObject             *object,
                                                        guint                property_id,
                                                        GValue              *value,
                                                        GParamSpec          *pspec);

static void      gimp_image_window_real_destroy        (GtkObject           *object);

static gboolean  gimp_image_window_delete_event        (GtkWidget           *widget,
                                                        GdkEventAny         *event);
static gboolean  gimp_image_window_configure_event     (GtkWidget           *widget,
                                                        GdkEventConfigure   *event);
static gboolean  gimp_image_window_window_state_event  (GtkWidget           *widget,
                                                        GdkEventWindowState *event);
static void      gimp_image_window_style_set           (GtkWidget           *widget,
                                                        GtkStyle            *prev_style);

static void      gimp_image_window_config_notify       (GimpImageWindow     *window,
                                                        GParamSpec          *pspec,
                                                        GimpGuiConfig       *config);
static void      gimp_image_window_show_tooltip        (GimpUIManager       *manager,
                                                        const gchar         *tooltip,
                                                        GimpImageWindow     *window);
static void      gimp_image_window_hide_tooltip        (GimpUIManager       *manager,
                                                        GimpImageWindow     *window);

static gboolean  gimp_image_window_shell_events        (GtkWidget           *widget,
                                                        GdkEvent            *event,
                                                        GimpImageWindow     *window);

static void      gimp_image_window_switch_page         (GtkNotebook         *notebook,
                                                        GtkNotebookPage     *page,
                                                        gint                 page_num,
                                                        GimpImageWindow     *window);
static void      gimp_image_window_page_removed        (GtkNotebook         *notebook,
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
static void      gimp_image_window_shell_title_notify  (GimpDisplayShell    *shell,
                                                        const GParamSpec    *pspec,
                                                        GimpImageWindow     *window);
static void      gimp_image_window_shell_icon_notify   (GimpDisplayShell    *shell,
                                                        const GParamSpec    *pspec,
                                                        GimpImageWindow     *window);


G_DEFINE_TYPE (GimpImageWindow, gimp_image_window, GIMP_TYPE_WINDOW)

#define parent_class gimp_image_window_parent_class


static const gchar image_window_rc_style[] =
  "style \"fullscreen-menubar-style\"\n"
  "{\n"
  "  GtkMenuBar::shadow-type      = none\n"
  "  GtkMenuBar::internal-padding = 0\n"
  "}\n"
  "widget \"*.gimp-menubar-fullscreen\" style \"fullscreen-menubar-style\"\n";

static void
gimp_image_window_class_init (GimpImageWindowClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class     = GTK_WIDGET_CLASS (klass);

  object_class->constructor        = gimp_image_window_constructor;
  object_class->finalize           = gimp_image_window_finalize;
  object_class->set_property       = gimp_image_window_set_property;
  object_class->get_property       = gimp_image_window_get_property;

  gtk_object_class->destroy        = gimp_image_window_real_destroy;

  widget_class->delete_event       = gimp_image_window_delete_event;
  widget_class->configure_event    = gimp_image_window_configure_event;
  widget_class->window_state_event = gimp_image_window_window_state_event;
  widget_class->style_set          = gimp_image_window_style_set;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_MENU_FACTORY,
                                   g_param_spec_object ("menu-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_MENU_FACTORY,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DIALOG_FACTORY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (GimpImageWindowPrivate));

  gtk_rc_parse_string (image_window_rc_style);
}

static void
gimp_image_window_init (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  gtk_window_set_role (GTK_WINDOW (window), "gimp-image-window");
  gtk_window_set_resizable (GTK_WINDOW (window), TRUE);

  private->is_empty = TRUE;
}

static GObject *
gimp_image_window_constructor (GType                  type,
                               guint                  n_params,
                               GObjectConstructParam *params)
{
  GObject                *object;
  GimpGuiConfig          *config;
  GimpImageWindow        *window;
  GimpImageWindowPrivate *private;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  window  = GIMP_IMAGE_WINDOW (object);
  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  g_assert (GIMP_IS_UI_MANAGER (private->menubar_manager));

  gtk_window_add_accel_group (GTK_WINDOW (window),
                              gtk_ui_manager_get_accel_group (GTK_UI_MANAGER (private->menubar_manager)));

  g_signal_connect (private->menubar_manager, "show-tooltip",
                    G_CALLBACK (gimp_image_window_show_tooltip),
                    window);
  g_signal_connect (private->menubar_manager, "hide-tooltip",
                    G_CALLBACK (gimp_image_window_hide_tooltip),
                    window);

  config = GIMP_GUI_CONFIG (gimp_dialog_factory_get_context (private->dialog_factory)->gimp->config);

  /* Create the window toplevel container */
  private->main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), private->main_vbox);
  gtk_widget_show (private->main_vbox);

  /* Create the menubar */
#ifndef GDK_WINDOWING_QUARTZ
  private->menubar =
    gtk_ui_manager_get_widget (GTK_UI_MANAGER (private->menubar_manager),
                               "/image-menubar");
#endif /* !GDK_WINDOWING_QUARTZ */
  if (private->menubar)
    {
      gtk_box_pack_start (GTK_BOX (private->main_vbox),
                          private->menubar, FALSE, FALSE, 0);

      /*  make sure we can activate accels even if the menubar is invisible
       *  (see http://bugzilla.gnome.org/show_bug.cgi?id=137151)
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
    }

  /* Create the hbox that contains docks and images */
  private->hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (private->main_vbox), private->hbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (private->hbox);

  /* Create the left pane */
  private->left_hpane = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (private->hbox), private->left_hpane,
                      TRUE, TRUE, 0);
  gtk_widget_show (private->left_hpane);

  /* Create the left dock columns widget */
  private->left_docks =
    gimp_dock_columns_new (gimp_get_user_context (private->gimp),
                           private->dialog_factory,
                           private->menubar_manager);
  gtk_paned_pack1 (GTK_PANED (private->left_hpane), private->left_docks,
                   FALSE, FALSE);
  gtk_widget_set_visible (private->left_docks, config->single_window_mode);

  /* Create the right pane */
  private->right_hpane = gtk_hpaned_new ();
  gtk_paned_pack2 (GTK_PANED (private->left_hpane), private->right_hpane,
                   TRUE, FALSE);
  gtk_widget_show (private->right_hpane);

  /* Create notebook that contains images */
  private->notebook = gtk_notebook_new ();
  gtk_notebook_set_show_border (GTK_NOTEBOOK (private->notebook), FALSE);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (private->notebook), FALSE);
  gtk_paned_pack1 (GTK_PANED (private->right_hpane), private->notebook,
                   TRUE, TRUE);
  g_signal_connect (private->notebook, "switch-page",
                    G_CALLBACK (gimp_image_window_switch_page),
                    window);
  g_signal_connect (private->notebook, "page-removed",
                    G_CALLBACK (gimp_image_window_page_removed),
                    window);
  gtk_widget_show (private->notebook);

  /* Create the right dock columns widget */
  private->right_docks =
    gimp_dock_columns_new (gimp_get_user_context (private->gimp),
                           private->dialog_factory,
                           private->menubar_manager);
  gtk_paned_pack2 (GTK_PANED (private->right_hpane), private->right_docks,
                   FALSE, FALSE);
  gtk_widget_set_visible (private->right_docks, config->single_window_mode);

  g_signal_connect_object (config, "notify::single-window-mode",
                           G_CALLBACK (gimp_image_window_config_notify),
                           window, G_CONNECT_SWAPPED);
  return object;
}

static void
gimp_image_window_finalize (GObject *object)
{
  GimpImageWindow        *window  = GIMP_IMAGE_WINDOW (object);
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  if (private->menubar_manager)
    {
      g_object_unref (private->menubar_manager);
      private->menubar_manager = NULL;
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
    case PROP_MENU_FACTORY:
      {
        GimpMenuFactory *factory = g_value_get_object (value);

        private->menubar_manager = gimp_menu_factory_manager_new (factory,
                                                                  "<Image>",
                                                                  window,
                                                                  FALSE);
      }
      break;
    case PROP_DIALOG_FACTORY:
      private->dialog_factory = g_value_get_object (value);
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

    case PROP_MENU_FACTORY:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_window_real_destroy (GtkObject *object)
{
  GimpImageWindow        *window  = GIMP_IMAGE_WINDOW (object);
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  if (private->menubar_manager)
    {
      g_object_unref (private->menubar_manager);
      private->menubar_manager = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static gboolean
gimp_image_window_delete_event (GtkWidget   *widget,
                                GdkEventAny *event)
{
  GimpImageWindow  *window = GIMP_IMAGE_WINDOW (widget);
  GimpDisplayShell *shell  = gimp_image_window_get_active_shell (window);

  /* FIXME multiple shells */
  if (shell)
    gimp_display_shell_close (shell, FALSE);

  return TRUE;
}

static gboolean
gimp_image_window_configure_event (GtkWidget         *widget,
                                   GdkEventConfigure *event)
{
  GimpImageWindow *window = GIMP_IMAGE_WINDOW (widget);
  GtkAllocation    allocation;
  gint             current_width;
  gint             current_height;

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

  return TRUE;
}

static gboolean
gimp_image_window_window_state_event (GtkWidget           *widget,
                                      GdkEventWindowState *event)
{
  GimpImageWindow        *window  = GIMP_IMAGE_WINDOW (widget);
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GimpDisplayShell       *shell   = gimp_image_window_get_active_shell (window);

  if (! shell)
    return FALSE;

  private->window_state = event->new_window_state;

  if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
    {
      gboolean fullscreen = gimp_image_window_get_fullscreen (window);

      GIMP_LOG (WM, "Image window '%s' [%p] set fullscreen %s",
                gtk_window_get_title (GTK_WINDOW (widget)),
                widget,
                fullscreen ? "TURE" : "FALSE");

      if (private->menubar)
        gtk_widget_set_name (private->menubar,
                             fullscreen ? "gimp-menubar-fullscreen" : NULL);

      gimp_display_shell_appearance_update (shell);
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
          if (gimp_displays_get_num_visible (shell->display->gimp) == 0)
            {
              GIMP_LOG (WM, "No displays visible any longer");

              gimp_dialog_factories_hide_with_display ();
            }
        }
      else
        {
          gimp_dialog_factories_show_with_display ();
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
gimp_image_window_style_set (GtkWidget *widget,
                             GtkStyle  *prev_style)
{
  GimpImageWindow        *window        = GIMP_IMAGE_WINDOW (widget);
  GimpImageWindowPrivate *private       = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GimpDisplayShell       *shell         = gimp_image_window_get_active_shell (window);
  GimpStatusbar          *statusbar     = NULL;
  GtkRequisition          requisition   = { 0, };
  GdkGeometry             geometry      = { 0, };
  GdkWindowHints          geometry_mask = 0;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  if (! shell)
    return;

  statusbar = gimp_display_shell_get_statusbar (shell);

  gtk_widget_size_request (GTK_WIDGET (statusbar), &requisition);

  geometry.min_height = 23;

  geometry.min_width   = requisition.width;
  geometry.min_height += requisition.height;

  if (private->menubar)
    {
      gtk_widget_size_request (private->menubar, &requisition);

      geometry.min_height += requisition.height;
    }

  geometry_mask = GDK_HINT_MIN_SIZE;

  /*  Only set user pos on the empty display because it gets a pos
   *  set by gimp. All other displays should be placed by the window
   *  manager. See http://bugzilla.gnome.org/show_bug.cgi?id=559580
   */
  if (! gimp_display_get_image (shell->display))
    geometry_mask |= GDK_HINT_USER_POS;

  gtk_window_set_geometry_hints (GTK_WINDOW (widget), NULL,
                                 &geometry, geometry_mask);

  gimp_dialog_factory_set_has_min_size (GTK_WINDOW (widget), TRUE);
}


/*  public functions  */

GimpImageWindow *
gimp_image_window_new (Gimp              *gimp,
                       GimpImage         *image,
                       GimpMenuFactory   *menu_factory,
                       GimpDialogFactory *dialog_factory)
{
  GimpImageWindow *window;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (image) || image == NULL, NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);

  window = g_object_new (GIMP_TYPE_IMAGE_WINDOW,
                         "gimp",            gimp,
                         "menu-factory",    menu_factory,
                         "dialog-factory",  dialog_factory,
                         /* The window position will be overridden by the
                          * dialog factory, it is only really used on first
                          * startup.
                          */
                         image ? NULL : "window-position",
                         GTK_WIN_POS_CENTER,
                         NULL);

  gimp->image_windows = g_list_prepend (gimp->image_windows, window);

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

GimpUIManager *
gimp_image_window_get_ui_manager (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), FALSE);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  return private->menubar_manager;
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
  GtkWidget              *view;
  GimpImage              *image;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  g_return_if_fail (g_list_find (private->shells, shell) == NULL);

  private->shells = g_list_append (private->shells, shell);

  view = gimp_view_new_by_types (gimp_get_user_context (shell->display->gimp),
                                 GIMP_TYPE_VIEW, GIMP_TYPE_IMAGE,
                                 GIMP_VIEW_SIZE_LARGE, 0, FALSE);

  gtk_notebook_append_page (GTK_NOTEBOOK (private->notebook),
                            GTK_WIDGET (shell), view);

  image = gimp_display_get_image (shell->display);

  if (image)
    {
      gimp_view_set_viewable (GIMP_VIEW (view), GIMP_VIEWABLE (image));

      if (g_list_length (private->shells) == 1)
        private->is_empty = FALSE;
    }

  if (g_list_length (private->shells) > 1)
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (private->notebook), TRUE);

  gtk_widget_show (GTK_WIDGET (shell));
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

  if (g_list_length (private->shells) == 1)
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (private->notebook), FALSE);

  if (! private->shells)
    private->is_empty = TRUE;
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

  gtk_widget_set_visible (private->menubar, show);
}

gboolean
gimp_image_window_get_show_menubar (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), FALSE);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  return gtk_widget_get_visible (private->menubar);
}

gboolean
gimp_image_window_is_iconified (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), FALSE);

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  return (private->window_state & GDK_WINDOW_STATE_ICONIFIED) != 0;
}

void
gimp_image_window_shrink_wrap (GimpImageWindow *window,
                               gboolean         grow_only)
{
  GimpImageWindowPrivate *private;
  GimpDisplayShell       *active_shell;
  GimpImage              *image;
  GtkWidget              *widget;
  GtkAllocation           allocation;
  GdkScreen              *screen;
  GdkRectangle            rect;
  gint                    monitor;
  gint                    disp_width, disp_height;
  gint                    width, height;
  gint                    max_auto_width, max_auto_height;
  gint                    border_width, border_height;
  gboolean                resize = FALSE;

  g_return_if_fail (GIMP_IS_IMAGE_WINDOW (window));

  private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  if (! GTK_WIDGET_REALIZED (window))
    return;

  /* FIXME this so needs cleanup and shell/window separation */

  active_shell = gimp_image_window_get_active_shell (window);

  if (!active_shell)
    return;

  image = gimp_display_get_image (active_shell->display);

  widget = GTK_WIDGET (window);
  screen = gtk_widget_get_screen (widget);

  gtk_widget_get_allocation (widget, &allocation);

  monitor = gdk_screen_get_monitor_at_window (screen,
                                              gtk_widget_get_window (widget));
  gdk_screen_get_monitor_geometry (screen, monitor, &rect);

  width  = SCALEX (active_shell, gimp_image_get_width  (image));
  height = SCALEY (active_shell, gimp_image_get_height (image));

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
  gimp_display_shell_scroll_center_image (active_shell, TRUE, TRUE);
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

  if (strcmp (pspec->name, "single-window-mode") == 0)
    {
      gtk_widget_set_visible (private->left_docks, config->single_window_mode);
      gtk_widget_set_visible (private->right_docks, config->single_window_mode);
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
                               GtkNotebookPage *page,
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

  active_display = private->active_shell->display;

  g_signal_connect (active_display, "notify::image",
                    G_CALLBACK (gimp_image_window_image_notify),
                    window);

  g_signal_connect (private->active_shell, "scaled",
                    G_CALLBACK (gimp_image_window_shell_scaled),
                    window);
  g_signal_connect (private->active_shell, "notify::title",
                    G_CALLBACK (gimp_image_window_shell_title_notify),
                    window);
  g_signal_connect (private->active_shell, "notify::icon",
                    G_CALLBACK (gimp_image_window_shell_icon_notify),
                    window);

  gtk_window_set_title (GTK_WINDOW (window), shell->title);
  gtk_window_set_icon (GTK_WINDOW (window), shell->icon);

  gimp_display_shell_appearance_update (private->active_shell);

  if (! gimp_display_get_image (active_display))
    {
      gimp_dialog_factory_add_foreign (private->dialog_factory,
                                       "gimp-empty-image-window",
                                       GTK_WIDGET (window));
    }

  gimp_ui_manager_update (private->menubar_manager, active_display);
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
gimp_image_window_disconnect_from_active_shell (GimpImageWindow *window)
{
  GimpImageWindowPrivate *private        = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GimpDisplay            *active_display = NULL;

  if (! private->active_shell)
    return;

  active_display = private->active_shell->display;

  g_signal_handlers_disconnect_by_func (active_display,
                                        gimp_image_window_image_notify,
                                        window);

  g_signal_handlers_disconnect_by_func (private->active_shell,
                                        gimp_image_window_shell_scaled,
                                        window);
  g_signal_handlers_disconnect_by_func (private->active_shell,
                                        gimp_image_window_shell_title_notify,
                                        window);
  g_signal_handlers_disconnect_by_func (private->active_shell,
                                        gimp_image_window_shell_icon_notify,
                                        window);

  gimp_image_window_hide_tooltip (private->menubar_manager, window);
}

static void
gimp_image_window_image_notify (GimpDisplay      *display,
                                const GParamSpec *pspec,
                                GimpImageWindow  *window)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GtkWidget              *view;

  if (gimp_display_get_image (display))
    {
      if (private->is_empty)
        {
          private->is_empty = FALSE;

          gimp_dialog_factory_remove_dialog (private->dialog_factory,
                                             GTK_WIDGET (window));
        }
    }
  else if (g_list_length (private->shells) == 1)
    {
      GimpSessionInfo *session_info;
      gint             width;
      gint             height;

      private->is_empty = TRUE;

      gtk_window_unfullscreen (GTK_WINDOW (window));

      /*  get the NIW size before adding the display to the dialog
       *  factory so the window's current size doesn't affect the
       *  stored session info entry.
       */
      session_info =
        gimp_dialog_factory_find_session_info (private->dialog_factory,
                                               "gimp-empty-image-window");

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
                                       "gimp-empty-image-window",
                                       GTK_WIDGET (window));

      gtk_window_unmaximize (GTK_WINDOW (window));
      gtk_window_resize (GTK_WINDOW (window), width, height);
    }

  view = gtk_notebook_get_tab_label (GTK_NOTEBOOK (private->notebook),
                                     GTK_WIDGET (gimp_display_get_shell (display)));

  gimp_view_set_viewable (GIMP_VIEW (view),
                          GIMP_VIEWABLE (gimp_display_get_image (display)));

  gimp_ui_manager_update (private->menubar_manager, display);
}

static void
gimp_image_window_shell_scaled (GimpDisplayShell *shell,
                                GimpImageWindow  *window)
{
  GimpImageWindowPrivate *private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);

  /* update the <Image>/View/Zoom menu */
  gimp_ui_manager_update (private->menubar_manager,
                          shell->display);
}

static void
gimp_image_window_shell_title_notify (GimpDisplayShell *shell,
                                      const GParamSpec *pspec,
                                      GimpImageWindow  *window)
{
  gtk_window_set_title (GTK_WINDOW (window), shell->title);
}

static void
gimp_image_window_shell_icon_notify (GimpDisplayShell *shell,
                                     const GParamSpec *pspec,
                                     GimpImageWindow  *window)
{
  gtk_window_set_icon (GTK_WINDOW (window), shell->icon);
}
