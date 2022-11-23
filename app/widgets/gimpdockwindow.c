/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadockwindow.c
 * Copyright (C) 2001-2005 Michael Natterer <mitch@ligma.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "dialogs/dialogs.h" /* FIXME, we are in the widget layer */

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontainer.h"
#include "core/ligmalist.h"
#include "core/ligmaimage.h"

#include "ligmacontainercombobox.h"
#include "ligmacontainerview.h"
#include "ligmadialogfactory.h"
#include "ligmadock.h"
#include "ligmadockbook.h"
#include "ligmadockcolumns.h"
#include "ligmadockcontainer.h"
#include "ligmadockwindow.h"
#include "ligmahelp-ids.h"
#include "ligmamenufactory.h"
#include "ligmasessioninfo-aux.h"
#include "ligmasessioninfo.h"
#include "ligmasessionmanaged.h"
#include "ligmatoolbox.h"
#include "ligmauimanager.h"
#include "ligmawidgets-utils.h"
#include "ligmawindow.h"

#include "ligma-intl.h"


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


struct _LigmaDockWindowPrivate
{
  LigmaContext       *context;

  LigmaDialogFactory *dialog_factory;

  gchar             *ui_manager_name;
  LigmaUIManager     *ui_manager;
  GQuark             image_flush_handler_id;

  LigmaDockColumns   *dock_columns;

  gboolean           allow_dockbook_absence;

  guint              update_title_idle_id;

  gint               ID;

  LigmaContainer     *image_container;
  LigmaContainer     *display_container;

  gboolean           show_image_menu;
  gboolean           auto_follow_active;

  GtkWidget         *image_combo;
  GtkWidget         *auto_button;
};


static void            ligma_dock_window_dock_container_iface_init (LigmaDockContainerInterface *iface);
static void            ligma_dock_window_session_managed_iface_init(LigmaSessionManagedInterface*iface);
static void            ligma_dock_window_constructed               (GObject                    *object);
static void            ligma_dock_window_dispose                   (GObject                    *object);
static void            ligma_dock_window_finalize                  (GObject                    *object);
static void            ligma_dock_window_set_property              (GObject                    *object,
                                                                   guint                       property_id,
                                                                   const GValue               *value,
                                                                   GParamSpec                 *pspec);
static void            ligma_dock_window_get_property              (GObject                    *object,
                                                                   guint                       property_id,
                                                                   GValue                     *value,
                                                                   GParamSpec                 *pspec);
static void            ligma_dock_window_style_updated             (GtkWidget                  *widget);
static gboolean        ligma_dock_window_delete_event              (GtkWidget                  *widget,
                                                                   GdkEventAny                *event);
static GList         * ligma_dock_window_get_docks                 (LigmaDockContainer          *dock_container);
static LigmaDialogFactory * ligma_dock_window_get_dialog_factory    (LigmaDockContainer          *dock_container);
static LigmaUIManager * ligma_dock_window_get_ui_manager            (LigmaDockContainer          *dock_container);
static void            ligma_dock_window_add_dock_from_session     (LigmaDockContainer          *dock_container,
                                                                   LigmaDock                   *dock,
                                                                   LigmaSessionInfoDock        *dock_info);
static GList         * ligma_dock_window_get_aux_info              (LigmaSessionManaged         *session_managed);
static void            ligma_dock_window_set_aux_info              (LigmaSessionManaged         *session_managed,
                                                                   GList                      *aux_info);
static LigmaAlignmentType
                       ligma_dock_window_get_dock_side             (LigmaDockContainer          *dock_container,
                                                                   LigmaDock                   *dock);
static gboolean        ligma_dock_window_should_add_to_recent      (LigmaDockWindow             *dock_window);
static void            ligma_dock_window_display_changed           (LigmaDockWindow             *dock_window,
                                                                   LigmaDisplay                *display,
                                                                   LigmaContext                *context);
static void            ligma_dock_window_image_changed             (LigmaDockWindow             *dock_window,
                                                                   LigmaImage                  *image,
                                                                   LigmaContext                *context);
static void            ligma_dock_window_image_flush               (LigmaImage                  *image,
                                                                   gboolean                    invalidate_preview,
                                                                   LigmaDockWindow             *dock_window);
static void            ligma_dock_window_update_title              (LigmaDockWindow             *dock_window);
static gboolean        ligma_dock_window_update_title_idle         (LigmaDockWindow             *dock_window);
static gchar         * ligma_dock_window_get_description           (LigmaDockWindow             *dock_window,
                                                                   gboolean                    complete);
static void            ligma_dock_window_dock_removed              (LigmaDockWindow             *dock_window,
                                                                   LigmaDock                   *dock,
                                                                   LigmaDockColumns            *dock_columns);
static void            ligma_dock_window_factory_display_changed   (LigmaContext                *context,
                                                                   LigmaDisplay                *display,
                                                                   LigmaDock                   *dock);
static void            ligma_dock_window_factory_image_changed     (LigmaContext                *context,
                                                                   LigmaImage                  *image,
                                                                   LigmaDock                   *dock);
static void            ligma_dock_window_auto_clicked              (GtkWidget                  *widget,
                                                                   LigmaDock                   *dock);


G_DEFINE_TYPE_WITH_CODE (LigmaDockWindow, ligma_dock_window, LIGMA_TYPE_WINDOW,
                         G_ADD_PRIVATE (LigmaDockWindow)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCK_CONTAINER,
                                                ligma_dock_window_dock_container_iface_init)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_SESSION_MANAGED,
                                                ligma_dock_window_session_managed_iface_init))

#define parent_class ligma_dock_window_parent_class


static void
ligma_dock_window_class_init (LigmaDockWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = ligma_dock_window_constructed;
  object_class->dispose       = ligma_dock_window_dispose;
  object_class->finalize      = ligma_dock_window_finalize;
  object_class->set_property  = ligma_dock_window_set_property;
  object_class->get_property  = ligma_dock_window_get_property;

  widget_class->style_updated = ligma_dock_window_style_updated;
  widget_class->delete_event  = ligma_dock_window_delete_event;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        LIGMA_TYPE_CONTEXT,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_DIALOG_FACTORY,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_MANAGER_NAME,
                                   g_param_spec_string ("ui-manager-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_IMAGE_CONTAINER,
                                   g_param_spec_object ("image-container",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTAINER,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DISPLAY_CONTAINER,
                                   g_param_spec_object ("display-container",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTAINER,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ALLOW_DOCKBOOK_ABSENCE,
                                   g_param_spec_boolean ("allow-dockbook-absence",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));


  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("default-height",
                                                             NULL, NULL,
                                                             -1, G_MAXINT,
                                                             DEFAULT_DOCK_HEIGHT,
                                                             LIGMA_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("menu-preview-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_MENU_VIEW_SIZE,
                                                              LIGMA_PARAM_READABLE));
}

static void
ligma_dock_window_init (LigmaDockWindow *dock_window)
{
  static gint  dock_window_ID = 1;
  gchar       *name           = NULL;

  dock_window->p = ligma_dock_window_get_instance_private (dock_window);
  dock_window->p->ID                 = dock_window_ID++;
  dock_window->p->auto_follow_active = TRUE;

  name = g_strdup_printf ("ligma-dock-%d", dock_window->p->ID);
  gtk_widget_set_name (GTK_WIDGET (dock_window), name);
  g_free (name);

  gtk_window_set_resizable (GTK_WINDOW (dock_window), TRUE);
  gtk_window_set_focus_on_map (GTK_WINDOW (dock_window), FALSE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dock_window), FALSE);
}

static void
ligma_dock_window_dock_container_iface_init (LigmaDockContainerInterface *iface)
{
  iface->get_docks          = ligma_dock_window_get_docks;
  iface->get_dialog_factory = ligma_dock_window_get_dialog_factory;
  iface->get_ui_manager     = ligma_dock_window_get_ui_manager;
  iface->add_dock           = ligma_dock_window_add_dock_from_session;
  iface->get_dock_side      = ligma_dock_window_get_dock_side;
}

static void
ligma_dock_window_session_managed_iface_init (LigmaSessionManagedInterface *iface)
{
  iface->get_aux_info = ligma_dock_window_get_aux_info;
  iface->set_aux_info = ligma_dock_window_set_aux_info;
}

static void
ligma_dock_window_constructed (GObject *object)
{
  LigmaDockWindow  *dock_window = LIGMA_DOCK_WINDOW (object);
  LigmaGuiConfig   *config;
  LigmaContext     *factory_context;
  LigmaMenuFactory *menu_factory;
  GtkAccelGroup   *accel_group;
  Ligma            *ligma;
  gint             menu_view_width  = -1;
  gint             menu_view_height = -1;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma   = LIGMA (dock_window->p->context->ligma);
  config = LIGMA_GUI_CONFIG (ligma->config);

  /* Create a separate context per dock so that docks can be bound to
   * a specific image and does not necessarily have to follow the
   * active image in the user context
   */
  g_object_unref (dock_window->p->context);
  dock_window->p->context           = ligma_context_new (ligma, "Dock Context", NULL);
  dock_window->p->image_container   = ligma->images;
  dock_window->p->display_container = ligma->displays;

  factory_context =
    ligma_dialog_factory_get_context (dock_window->p->dialog_factory);

  /* Setup hints */
  ligma_window_set_hint (GTK_WINDOW (dock_window), config->dock_window_hint);

  menu_factory =
    ligma_dialog_factory_get_menu_factory (dock_window->p->dialog_factory);

  /* Make image window related keyboard shortcuts work also when a
   * dock window is the focused window
   */
  dock_window->p->ui_manager =
    ligma_menu_factory_manager_new (menu_factory,
                                   dock_window->p->ui_manager_name,
                                   dock_window);
  accel_group = ligma_ui_manager_get_accel_group (dock_window->p->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (dock_window), accel_group);

  g_signal_connect_object (dock_window->p->context, "display-changed",
                           G_CALLBACK (ligma_dock_window_display_changed),
                           dock_window,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (dock_window->p->context, "image-changed",
                           G_CALLBACK (ligma_dock_window_image_changed),
                           dock_window,
                           G_CONNECT_SWAPPED);

  dock_window->p->image_flush_handler_id =
    ligma_container_add_handler (ligma->images, "flush",
                                G_CALLBACK (ligma_dock_window_image_flush),
                                dock_window);

  ligma_context_define_properties (dock_window->p->context,
                                  LIGMA_CONTEXT_PROP_MASK_ALL &
                                  ~(LIGMA_CONTEXT_PROP_MASK_IMAGE |
                                    LIGMA_CONTEXT_PROP_MASK_DISPLAY),
                                  FALSE);
  ligma_context_set_parent (dock_window->p->context,
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
      dock_window->p->image_combo = ligma_container_combo_box_new (NULL, NULL, 16, 1);
      gtk_box_pack_start (GTK_BOX (hbox), dock_window->p->image_combo, TRUE, TRUE, 0);
      g_signal_connect (dock_window->p->image_combo, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dock_window->p->image_combo);
      ligma_help_set_help_data (dock_window->p->image_combo,
                               NULL, LIGMA_HELP_DOCK_IMAGE_MENU);
      gtk_widget_show (dock_window->p->image_combo);

      /* Auto button */
      dock_window->p->auto_button = gtk_toggle_button_new_with_label (_("Auto"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dock_window->p->auto_button),
                                    dock_window->p->auto_follow_active);
      gtk_box_pack_start (GTK_BOX (hbox), dock_window->p->auto_button, FALSE, FALSE, 0);
      gtk_widget_show (dock_window->p->auto_button);

      g_signal_connect (dock_window->p->auto_button, "clicked",
                        G_CALLBACK (ligma_dock_window_auto_clicked),
                        dock_window);

      ligma_help_set_help_data (dock_window->p->auto_button,
                               _("When enabled, the dialog automatically "
                                 "follows the image you are working on."),
                               LIGMA_HELP_DOCK_AUTO_BUTTON);
    }

    /* LigmaDockColumns */
    /* Let the LigmaDockColumns mirror the context so that a LigmaDock can
     * get it when inside a dock window. We do the same thing in the
     * LigmaImageWindow so docks can get the LigmaContext there as well
     */
    dock_window->p->dock_columns =
      LIGMA_DOCK_COLUMNS (ligma_dock_columns_new (dock_window->p->context,
                                                dock_window->p->dialog_factory,
                                                dock_window->p->ui_manager));
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (dock_window->p->dock_columns),
                        TRUE, TRUE, 0);
    gtk_widget_show (GTK_WIDGET (dock_window->p->dock_columns));
    g_signal_connect_object (dock_window->p->dock_columns, "dock-removed",
                             G_CALLBACK (ligma_dock_window_dock_removed),
                             dock_window,
                             G_CONNECT_SWAPPED);

    g_signal_connect_object (dock_window->p->dock_columns, "dock-added",
                             G_CALLBACK (ligma_dock_window_update_title),
                             dock_window,
                             G_CONNECT_SWAPPED);
    g_signal_connect_object (dock_window->p->dock_columns, "dock-removed",
                             G_CALLBACK (ligma_dock_window_update_title),
                             dock_window,
                             G_CONNECT_SWAPPED);
  }

  if (dock_window->p->auto_follow_active)
    {
      if (ligma_context_get_display (factory_context))
        ligma_context_copy_property (factory_context,
                                    dock_window->p->context,
                                    LIGMA_CONTEXT_PROP_DISPLAY);
      else
        ligma_context_copy_property (factory_context,
                                    dock_window->p->context,
                                    LIGMA_CONTEXT_PROP_IMAGE);
    }

  g_signal_connect_object (factory_context, "display-changed",
                           G_CALLBACK (ligma_dock_window_factory_display_changed),
                           dock_window,
                           0);
  g_signal_connect_object (factory_context, "image-changed",
                           G_CALLBACK (ligma_dock_window_factory_image_changed),
                           dock_window,
                           0);

  gtk_icon_size_lookup (DEFAULT_MENU_VIEW_SIZE,
                        &menu_view_width,
                        &menu_view_height);

  g_object_set (dock_window->p->image_combo,
                "container", dock_window->p->image_container,
                "context",   dock_window->p->context,
                NULL);

  ligma_help_connect (GTK_WIDGET (dock_window), ligma_standard_help_func,
                     LIGMA_HELP_DOCK, NULL, NULL);

  if (dock_window->p->auto_follow_active)
    {
      if (ligma_context_get_display (factory_context))
        ligma_context_copy_property (factory_context,
                                    dock_window->p->context,
                                    LIGMA_CONTEXT_PROP_DISPLAY);
      else
        ligma_context_copy_property (factory_context,
                                    dock_window->p->context,
                                    LIGMA_CONTEXT_PROP_IMAGE);
    }
}

static void
ligma_dock_window_dispose (GObject *object)
{
  LigmaDockWindow *dock_window = LIGMA_DOCK_WINDOW (object);

  if (dock_window->p->update_title_idle_id)
    {
      g_source_remove (dock_window->p->update_title_idle_id);
      dock_window->p->update_title_idle_id = 0;
    }

  if (dock_window->p->image_flush_handler_id)
    {
      ligma_container_remove_handler (dock_window->p->context->ligma->images,
                                     dock_window->p->image_flush_handler_id);
      dock_window->p->image_flush_handler_id = 0;
    }

  g_clear_object (&dock_window->p->ui_manager);
  g_clear_object (&dock_window->p->dialog_factory);
  g_clear_object (&dock_window->p->context);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_dock_window_finalize (GObject *object)
{
  LigmaDockWindow *dock_window = LIGMA_DOCK_WINDOW (object);

  g_clear_pointer (&dock_window->p->ui_manager_name, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_dock_window_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaDockWindow *dock_window = LIGMA_DOCK_WINDOW (object);

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
ligma_dock_window_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaDockWindow *dock_window = LIGMA_DOCK_WINDOW (object);

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
ligma_dock_window_style_updated (GtkWidget *widget)
{
  LigmaDockWindow  *dock_window      = LIGMA_DOCK_WINDOW (widget);
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
      GtkBorder border;
      gint      menu_view_width  = 18;
      gint      menu_view_height = 18;
      gint      focus_line_width;
      gint      focus_padding;

      gtk_icon_size_lookup (menu_view_size,
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

      ligma_container_view_set_view_size (LIGMA_CONTAINER_VIEW (dock_window->p->image_combo),
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
 * ligma_dock_window_delete_event:
 * @widget:
 * @event:
 *
 * Makes sure that when dock windows are closed they are added to the
 * list of recently closed docks so that they are easy to bring back.
 **/
static gboolean
ligma_dock_window_delete_event (GtkWidget   *widget,
                               GdkEventAny *event)
{
  LigmaDockWindow         *dock_window = LIGMA_DOCK_WINDOW (widget);
  LigmaSessionInfo        *info        = NULL;
  const gchar            *entry_name  = NULL;
  LigmaDialogFactoryEntry *entry       = NULL;
  gchar                  *name        = NULL;

  /* Don't add docks with just a single dockable to the list of
   * recently closed dock since those can be brought back through the
   * normal Windows->Dockable Dialogs menu
   */
  if (! ligma_dock_window_should_add_to_recent (dock_window))
    return FALSE;

  info = ligma_session_info_new ();

  name = ligma_dock_window_get_description (dock_window, TRUE /*complete*/);
  ligma_object_set_name (LIGMA_OBJECT (info), name);
  g_free (name);

  ligma_session_info_get_info_with_widget (info, GTK_WIDGET (dock_window));

  entry_name = (ligma_dock_window_has_toolbox (dock_window) ?
                "ligma-toolbox-window" :
                "ligma-dock-window");
  entry = ligma_dialog_factory_find_entry (dock_window->p->dialog_factory,
                                          entry_name);
  ligma_session_info_set_factory_entry (info, entry);

  ligma_container_add (global_recent_docks, LIGMA_OBJECT (info));
  g_object_unref (info);

  return FALSE;
}

static GList *
ligma_dock_window_get_docks (LigmaDockContainer *dock_container)
{
  LigmaDockWindow *dock_window = LIGMA_DOCK_WINDOW (dock_container);

  return g_list_copy (ligma_dock_columns_get_docks (dock_window->p->dock_columns));
}

static LigmaDialogFactory *
ligma_dock_window_get_dialog_factory (LigmaDockContainer *dock_container)
{
  LigmaDockWindow *dock_window = LIGMA_DOCK_WINDOW (dock_container);

  return dock_window->p->dialog_factory;
}

static LigmaUIManager *
ligma_dock_window_get_ui_manager (LigmaDockContainer *dock_container)
{
  LigmaDockWindow *dock_window = LIGMA_DOCK_WINDOW (dock_container);

  return dock_window->p->ui_manager;
}

static void
ligma_dock_window_add_dock_from_session (LigmaDockContainer   *dock_container,
                                        LigmaDock            *dock,
                                        LigmaSessionInfoDock *dock_info)
{
  LigmaDockWindow *dock_window = LIGMA_DOCK_WINDOW (dock_container);

  ligma_dock_window_add_dock (dock_window,
                             dock,
                             -1 /*index*/);
}

static GList *
ligma_dock_window_get_aux_info (LigmaSessionManaged *session_managed)
{
  LigmaDockWindow     *dock_window = LIGMA_DOCK_WINDOW (session_managed);
  GList              *aux_info    = NULL;
  LigmaSessionInfoAux *aux;

  if (dock_window->p->allow_dockbook_absence)
    {
      /* Assume it is the toolbox; it does not have aux info */
      return NULL;
    }

  g_return_val_if_fail (LIGMA_IS_DOCK_WINDOW (dock_window), NULL);

  aux = ligma_session_info_aux_new (AUX_INFO_SHOW_IMAGE_MENU,
                                   dock_window->p->show_image_menu ?
                                   "true" : "false");
  aux_info = g_list_append (aux_info, aux);

  aux = ligma_session_info_aux_new (AUX_INFO_FOLLOW_ACTIVE_IMAGE,
                                   dock_window->p->auto_follow_active ?
                                   "true" : "false");
  aux_info = g_list_append (aux_info, aux);

  return aux_info;
}

static void
ligma_dock_window_set_aux_info (LigmaSessionManaged *session_managed,
                               GList              *aux_info)
{
  LigmaDockWindow *dock_window;
  GList          *list;
  gboolean        menu_shown;
  gboolean        auto_follow;

  g_return_if_fail (LIGMA_IS_DOCK_WINDOW (session_managed));

  dock_window = LIGMA_DOCK_WINDOW (session_managed);
  menu_shown  = dock_window->p->show_image_menu;
  auto_follow = dock_window->p->auto_follow_active;

  for (list = aux_info; list; list = g_list_next (list))
    {
      LigmaSessionInfoAux *aux = list->data;

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
    ligma_dock_window_set_show_image_menu (dock_window, menu_shown);

  if (auto_follow != dock_window->p->auto_follow_active)
    ligma_dock_window_set_auto_follow_active (dock_window, auto_follow);
}

static LigmaAlignmentType
ligma_dock_window_get_dock_side (LigmaDockContainer *dock_container,
                                LigmaDock          *dock)
{
  g_return_val_if_fail (LIGMA_IS_DOCK_WINDOW (dock_container), -1);
  g_return_val_if_fail (LIGMA_IS_DOCK (dock), -1);

  /* A LigmaDockWindow don't have docks on different sides, it's just
   * one set of columns
   */
  return -1;
}

/**
 * ligma_dock_window_should_add_to_recent:
 * @dock_window:
 *
 * Returns: %FALSE if the dock window can be recreated with one
 *          Windows menu item such as Windows->Toolbox or
 *          Windows->Dockable Dialogs->Layers, %TRUE if not. It should
 *          then be added to the list of recently closed docks.
 **/
static gboolean
ligma_dock_window_should_add_to_recent (LigmaDockWindow *dock_window)
{
  GList    *docks;
  gboolean  should_add = TRUE;

  docks = ligma_dock_container_get_docks (LIGMA_DOCK_CONTAINER (dock_window));

  if (! docks)
    {
      should_add = FALSE;
    }
  else if (g_list_length (docks) == 1)
    {
      LigmaDock *dock = LIGMA_DOCK (g_list_nth_data (docks, 0));

      if (LIGMA_IS_TOOLBOX (dock) &&
          ligma_dock_get_n_dockables (dock) == 0)
        {
          should_add = FALSE;
        }
      else if (! LIGMA_IS_TOOLBOX (dock) &&
               ligma_dock_get_n_dockables (dock) == 1)
        {
          should_add = FALSE;
        }
    }

  g_list_free (docks);

  return should_add;
}

static void
ligma_dock_window_image_flush (LigmaImage      *image,
                              gboolean        invalidate_preview,
                              LigmaDockWindow *dock_window)
{
  if (image == ligma_context_get_image (dock_window->p->context))
    {
      LigmaDisplay *display = ligma_context_get_display (dock_window->p->context);

      if (display)
        ligma_ui_manager_update (dock_window->p->ui_manager, display);
    }
}

static void
ligma_dock_window_update_title (LigmaDockWindow *dock_window)
{
  if (dock_window->p->update_title_idle_id)
    g_source_remove (dock_window->p->update_title_idle_id);

  dock_window->p->update_title_idle_id =
    g_idle_add ((GSourceFunc) ligma_dock_window_update_title_idle,
                dock_window);
}

static gboolean
ligma_dock_window_update_title_idle (LigmaDockWindow *dock_window)
{
  gchar *desc = ligma_dock_window_get_description (dock_window,
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
ligma_dock_window_get_description (LigmaDockWindow *dock_window,
                                  gboolean        complete)
{
  GString *complete_desc = g_string_new (NULL);
  GList   *docks         = NULL;
  GList   *iter          = NULL;

  docks = ligma_dock_container_get_docks (LIGMA_DOCK_CONTAINER (dock_window));

  for (iter = docks;
       iter;
       iter = g_list_next (iter))
    {
      gchar *desc = ligma_dock_get_description (LIGMA_DOCK (iter->data), complete);
      g_string_append (complete_desc, desc);
      g_free (desc);

      if (g_list_next (iter))
        g_string_append (complete_desc, LIGMA_DOCK_COLUMN_SEPARATOR);
    }

  g_list_free (docks);

  return g_string_free (complete_desc, FALSE /*free_segment*/);
}

static void
ligma_dock_window_dock_removed (LigmaDockWindow  *dock_window,
                               LigmaDock        *dock,
                               LigmaDockColumns *dock_columns)
{
  g_return_if_fail (LIGMA_IS_DOCK (dock));

  if (ligma_dock_columns_get_docks (dock_columns) == NULL &&
      ! dock_window->p->allow_dockbook_absence)
    gtk_widget_destroy (GTK_WIDGET (dock_window));
}

static void
ligma_dock_window_factory_display_changed (LigmaContext *context,
                                          LigmaDisplay *display,
                                          LigmaDock    *dock)
{
  LigmaDockWindow *dock_window = LIGMA_DOCK_WINDOW (dock);

  if (display && dock_window->p->auto_follow_active)
    ligma_context_set_display (dock_window->p->context, display);
}

static void
ligma_dock_window_factory_image_changed (LigmaContext *context,
                                        LigmaImage   *image,
                                        LigmaDock    *dock)
{
  LigmaDockWindow *dock_window = LIGMA_DOCK_WINDOW (dock);

  /*  won't do anything if we already set the display above  */
  if (image && dock_window->p->auto_follow_active)
    ligma_context_set_image (dock_window->p->context, image);
}

static void
ligma_dock_window_display_changed (LigmaDockWindow *dock_window,
                                  LigmaDisplay    *display,
                                  LigmaContext    *context)
{
  /*  make sure auto-follow-active works both ways  */
  if (display && dock_window->p->auto_follow_active)
    {
      LigmaContext *factory_context =
        ligma_dialog_factory_get_context (dock_window->p->dialog_factory);

      ligma_context_set_display (factory_context, display);
    }

  ligma_ui_manager_update (dock_window->p->ui_manager,
                          display);
}

static void
ligma_dock_window_image_changed (LigmaDockWindow *dock_window,
                                LigmaImage      *image,
                                LigmaContext    *context)
{
  LigmaContainer *image_container   = dock_window->p->image_container;
  LigmaContainer *display_container = dock_window->p->display_container;

  /*  make sure auto-follow-active works both ways  */
  if (image && dock_window->p->auto_follow_active)
    {
      LigmaContext *factory_context =
        ligma_dialog_factory_get_context (dock_window->p->dialog_factory);

      ligma_context_set_image (factory_context, image);
    }

  if (image == NULL && ! ligma_container_is_empty (image_container))
    {
      image = LIGMA_IMAGE (ligma_container_get_first_child (image_container));

      /*  this invokes this function recursively but we don't enter
       *  the if() branch the second time
       */
      ligma_context_set_image (context, image);

      /*  stop the emission of the original signal (the emission of
       *  the recursive signal is finished)
       */
      g_signal_stop_emission_by_name (context, "image-changed");
    }
  else if (image != NULL && ! ligma_container_is_empty (display_container))
    {
      LigmaDisplay *display;
      LigmaImage   *display_image;
      gboolean     find_display = TRUE;

      display = ligma_context_get_display (context);

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

          for (list = LIGMA_LIST (display_container)->queue->head;
               list;
               list = g_list_next (list))
            {
              display = list->data;

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
                      ligma_context_set_display (context, display);

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

  ligma_ui_manager_update (dock_window->p->ui_manager,
                          ligma_context_get_display (context));
}

static void
ligma_dock_window_auto_clicked (GtkWidget *widget,
                               LigmaDock  *dock)
{
  LigmaDockWindow *dock_window = LIGMA_DOCK_WINDOW (dock);

  ligma_toggle_button_update (widget, &dock_window->p->auto_follow_active);

  if (dock_window->p->auto_follow_active)
    {
      LigmaContext *context;

      context = ligma_dialog_factory_get_context (dock_window->p->dialog_factory);

      ligma_context_copy_properties (context,
                                    dock_window->p->context,
                                    LIGMA_CONTEXT_PROP_MASK_DISPLAY |
                                    LIGMA_CONTEXT_PROP_MASK_IMAGE);
    }
}


void
ligma_dock_window_add_dock (LigmaDockWindow *dock_window,
                           LigmaDock       *dock,
                           gint            index)
{
  g_return_if_fail (LIGMA_IS_DOCK_WINDOW (dock_window));
  g_return_if_fail (LIGMA_IS_DOCK (dock));

  ligma_dock_columns_add_dock (dock_window->p->dock_columns,
                              LIGMA_DOCK (dock),
                              index);

  g_signal_connect_object (dock, "description-invalidated",
                           G_CALLBACK (ligma_dock_window_update_title),
                           dock_window,
                           G_CONNECT_SWAPPED);

  /* Some docks like the toolbox dock needs to maintain special hints
   * on its container GtkWindow, allow those to do so
   */
  ligma_dock_set_host_geometry_hints (dock, GTK_WINDOW (dock_window));
  g_signal_connect_object (dock, "geometry-invalidated",
                           G_CALLBACK (ligma_dock_set_host_geometry_hints),
                           dock_window, 0);
}

void
ligma_dock_window_remove_dock (LigmaDockWindow *dock_window,
                              LigmaDock       *dock)
{
  ligma_dock_columns_remove_dock (dock_window->p->dock_columns,
                                 LIGMA_DOCK (dock));

  g_signal_handlers_disconnect_by_func (dock,
                                        ligma_dock_window_update_title,
                                        dock_window);
  g_signal_handlers_disconnect_by_func (dock,
                                        ligma_dock_set_host_geometry_hints,
                                        dock_window);
}

GtkWidget *
ligma_dock_window_new (const gchar       *role,
                      const gchar       *ui_manager_name,
                      gboolean           allow_dockbook_absence,
                      LigmaDialogFactory *dialog_factory,
                      LigmaContext       *context)
{
  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return g_object_new (LIGMA_TYPE_DOCK_WINDOW,
                       "role",                   role,
                       "ui-manager-name",        ui_manager_name,
                       "allow-dockbook-absence", allow_dockbook_absence,
                       "dialog-factory",         dialog_factory,
                       "context",                context,
                       NULL);
}

gint
ligma_dock_window_get_id (LigmaDockWindow *dock_window)
{
  g_return_val_if_fail (LIGMA_IS_DOCK_WINDOW (dock_window), 0);

  return dock_window->p->ID;
}

LigmaContext *
ligma_dock_window_get_context (LigmaDockWindow *dock_window)
{
  g_return_val_if_fail (LIGMA_IS_DOCK_WINDOW (dock_window), NULL);

  return dock_window->p->context;
}

gboolean
ligma_dock_window_get_auto_follow_active (LigmaDockWindow *dock_window)
{
  g_return_val_if_fail (LIGMA_IS_DOCK_WINDOW (dock_window), FALSE);

  return dock_window->p->auto_follow_active;
}

void
ligma_dock_window_set_auto_follow_active (LigmaDockWindow *dock_window,
                                         gboolean        auto_follow_active)
{
  g_return_if_fail (LIGMA_IS_DOCK_WINDOW (dock_window));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dock_window->p->auto_button),
                                auto_follow_active ? TRUE : FALSE);
}

gboolean
ligma_dock_window_get_show_image_menu (LigmaDockWindow *dock_window)
{
  g_return_val_if_fail (LIGMA_IS_DOCK_WINDOW (dock_window), FALSE);

  return dock_window->p->show_image_menu;
}

void
ligma_dock_window_set_show_image_menu (LigmaDockWindow *dock_window,
                                      gboolean        show)
{
  GtkWidget *parent;

  g_return_if_fail (LIGMA_IS_DOCK_WINDOW (dock_window));

  parent = gtk_widget_get_parent (dock_window->p->image_combo);

  gtk_widget_set_visible (parent, show);

  dock_window->p->show_image_menu = show ? TRUE : FALSE;
}

void
ligma_dock_window_setup (LigmaDockWindow *dock_window,
                        LigmaDockWindow *template)
{
  ligma_dock_window_set_auto_follow_active (LIGMA_DOCK_WINDOW (dock_window),
                                           template->p->auto_follow_active);
  ligma_dock_window_set_show_image_menu (LIGMA_DOCK_WINDOW (dock_window),
                                        template->p->show_image_menu);
}

/**
 * ligma_dock_window_has_toolbox:
 * @dock_window:
 *
 * Returns: %TRUE if the dock window has a LigmaToolbox dock, %FALSE
 * otherwise.
 **/
gboolean
ligma_dock_window_has_toolbox (LigmaDockWindow *dock_window)
{
  GList *iter = NULL;

  g_return_val_if_fail (LIGMA_IS_DOCK_WINDOW (dock_window), FALSE);

  for (iter = ligma_dock_columns_get_docks (dock_window->p->dock_columns);
       iter;
       iter = g_list_next (iter))
    {
      if (LIGMA_IS_TOOLBOX (iter->data))
        return TRUE;
    }

  return FALSE;
}


/**
 * ligma_dock_window_from_dock:
 * @dock:
 *
 * For convenience.
 *
 * Returns: If the toplevel widget for the dock is a LigmaDockWindow,
 * return that. Otherwise return %NULL.
 **/
LigmaDockWindow *
ligma_dock_window_from_dock (LigmaDock *dock)
{
  GtkWidget *toplevel = NULL;

  g_return_val_if_fail (LIGMA_IS_DOCK (dock), NULL);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (dock));

  if (LIGMA_IS_DOCK_WINDOW (toplevel))
    return LIGMA_DOCK_WINDOW (toplevel);
  else
    return NULL;
}
