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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "dialogs/dialogs.h" /* FIXME, we are in the widget layer */

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontainer.h"

#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpdockbook.h"
#include "gimpdockwindow.h"
#include "gimpmenufactory.h"
#include "gimpsessioninfo.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"
#include "gimpwindow.h"

#include "gimp-intl.h"


#define DEFAULT_DOCK_HEIGHT     300
#define DEFAULT_DOCK_FONT_SCALE PANGO_SCALE_SMALL


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_DIALOG_FACTORY,
  PROP_UI_MANAGER_NAME,
};


struct _GimpDockWindowPrivate
{
  GimpContext       *context;

  GimpDialogFactory *dialog_factory;

  gchar             *ui_manager_name;
  GimpUIManager     *ui_manager;
  GQuark             image_flush_handler_id;

  guint              update_title_idle_id;

  gint               ID;
};

static GObject *  gimp_dock_window_constructor       (GType                  type,
                                                      guint                  n_params,
                                                      GObjectConstructParam *params);
static void       gimp_dock_window_dispose           (GObject               *object);
static void       gimp_dock_window_set_property      (GObject               *object,
                                                      guint                  property_id,
                                                      const GValue          *value,
                                                      GParamSpec            *pspec);
static void       gimp_dock_window_get_property      (GObject               *object,
                                                      guint                  property_id,
                                                      GValue                *value,
                                                      GParamSpec            *pspec);
static void       gimp_dock_window_style_set         (GtkWidget             *widget,
                                                      GtkStyle              *prev_style);
static gboolean   gimp_dock_window_delete_event      (GtkWidget             *widget,
                                                      GdkEventAny           *event);
static GimpDock * gimp_dock_window_get_dock          (GimpDockWindow        *dock_window);
static void       gimp_dock_window_display_changed   (GimpDockWindow        *dock_window,
                                                      GimpObject            *display,
                                                      GimpContext           *context);
static void       gimp_dock_window_image_changed     (GimpDockWindow        *dock_window,
                                                      GimpImage             *image,
                                                      GimpContext           *context);
static void       gimp_dock_window_image_flush       (GimpImage             *image,
                                                      gboolean               invalidate_preview,
                                                      GimpDockWindow        *dock_window);
static void       gimp_dock_window_update_title      (GimpDockWindow        *dock_window);
static gboolean   gimp_dock_window_update_title_idle (GimpDockWindow        *dock_window);


G_DEFINE_TYPE (GimpDockWindow, gimp_dock_window, GIMP_TYPE_WINDOW)

#define parent_class gimp_dock_window_parent_class

static void
gimp_dock_window_class_init (GimpDockWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructor     = gimp_dock_window_constructor;
  object_class->dispose         = gimp_dock_window_dispose;
  object_class->set_property    = gimp_dock_window_set_property;
  object_class->get_property    = gimp_dock_window_get_property;

  widget_class->style_set       = gimp_dock_window_style_set;
  widget_class->delete_event    = gimp_dock_window_delete_event;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("gimp-context", NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("gimp-dialog-factory",
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

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("default-height",
                                                             NULL, NULL,
                                                             -1, G_MAXINT,
                                                             DEFAULT_DOCK_HEIGHT,
                                                             GIMP_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_double ("font-scale",
                                                                NULL, NULL,
                                                                0.0,
                                                                G_MAXDOUBLE,
                                                                DEFAULT_DOCK_FONT_SCALE,
                                                                GIMP_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (GimpDockWindowPrivate));
}

static void
gimp_dock_window_init (GimpDockWindow *dock_window)
{
  static gint  dock_ID = 1;
  gchar       *name;

  /* Initialize members */
  dock_window->p = G_TYPE_INSTANCE_GET_PRIVATE (dock_window,
                                                GIMP_TYPE_DOCK_WINDOW,
                                                GimpDockWindowPrivate);
  dock_window->p->context                = NULL;
  dock_window->p->dialog_factory         = NULL;
  dock_window->p->ui_manager_name        = NULL;
  dock_window->p->ui_manager             = NULL;
  dock_window->p->image_flush_handler_id = 0;
  dock_window->p->ID                     = dock_ID++;
  dock_window->p->update_title_idle_id   = 0;

  /* Some common initialization for all dock windows */
  gtk_window_set_resizable (GTK_WINDOW (dock_window), TRUE);
  gtk_window_set_focus_on_map (GTK_WINDOW (dock_window), FALSE);

  /* Initialize theming and style-setting stuff */
  name = g_strdup_printf ("gimp-dock-%d", dock_window->p->ID);
  gtk_widget_set_name (GTK_WIDGET (dock_window), name);
  g_free (name);
}

static GObject *
gimp_dock_window_constructor (GType                  type,
                              guint                  n_params,
                              GObjectConstructParam *params)
{
  GObject        *object;
  GimpDockWindow *dock_window;
  GimpGuiConfig  *config;
  GtkAccelGroup  *accel_group;
  GimpDock       *dock;

  /* Init */
  object      = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  dock_window = GIMP_DOCK_WINDOW (object);
  config      = GIMP_GUI_CONFIG (dock_window->p->context->gimp->config);

  /* Setup hints */
  gimp_window_set_hint (GTK_WINDOW (dock_window), config->dock_window_hint);

  /* Make image window related keyboard shortcuts work also when a
   * dock window is the focused window
   */
  dock_window->p->ui_manager =
    gimp_menu_factory_manager_new (dock_window->p->dialog_factory->menu_factory,
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
    gimp_container_add_handler (dock_window->p->context->gimp->images, "flush",
                                G_CALLBACK (gimp_dock_window_image_flush),
                                dock_window);

  /* Update window title now and when docks title is invalidated */
  gimp_dock_window_update_title (dock_window);
  dock = gimp_dock_window_get_dock (dock_window);
  g_signal_connect_object (dock, "title-invalidated",
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

  /* Done! */
  return object;
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dock_window_style_set (GtkWidget *widget,
                            GtkStyle  *prev_style)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (widget);
  gint            default_height;
  gdouble         font_scale;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "default-height", &default_height,
                        "font-scale",     &font_scale,
                        NULL);

  gtk_window_set_default_size (GTK_WINDOW (widget), -1, default_height);

  if (font_scale != 1.0)
    {
      PangoContext         *context;
      PangoFontDescription *font_desc;
      gint                  font_size;
      gchar                *font_str;
      gchar                *rc_string;

      context = gtk_widget_get_pango_context (widget);
      font_desc = pango_context_get_font_description (context);
      font_desc = pango_font_description_copy (font_desc);

      font_size = pango_font_description_get_size (font_desc);
      font_size = font_scale * font_size;
      pango_font_description_set_size (font_desc, font_size);

      font_str = pango_font_description_to_string (font_desc);
      pango_font_description_free (font_desc);

      rc_string =
        g_strdup_printf ("style \"gimp-dock-style\""
                         "{"
                         "  font_name = \"%s\""
                         "}"
                         "widget \"gimp-dock-%d.*\" style \"gimp-dock-style\"",
                         font_str,
                         dock_window->p->ID);
      g_free (font_str);

      gtk_rc_parse_string (rc_string);
      g_free (rc_string);

      if (gtk_bin_get_child (GTK_BIN (widget)))
        gtk_widget_reset_rc_styles (gtk_bin_get_child (GTK_BIN (widget)));
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
  GimpDockWindow  *dock_window = GIMP_DOCK_WINDOW (widget);
  GimpDock        *dock        = gimp_dock_window_get_dock (dock_window);
  GimpSessionInfo *info        = NULL;

  /* Don't add docks with just a singe dockable to the list of
   * recently closed dock since those can be brought back through the
   * normal Windows->Dockable Dialogs menu
   */ 
  if (gimp_dock_get_n_dockables (dock) < 2)
    return FALSE;

  info = gimp_session_info_new ();

  gimp_object_set_name (GIMP_OBJECT (info),
                        gtk_window_get_title (GTK_WINDOW (dock_window)));

  info->widget = GTK_WIDGET (dock);
  gimp_session_info_get_info (info);
  info->widget = NULL;

  gimp_container_add (global_recent_docks, GIMP_OBJECT (info));
  g_object_unref (info);

  return FALSE;
}

static GimpDock *
gimp_dock_window_get_dock (GimpDockWindow *dock_window)
{
  /* Change this to return the GimpDock *inside* the GimpDockWindow
   * once GimpDock is not a subclass of GimpDockWindow any longer
   */
  return GIMP_DOCK (dock_window);
}

static void
gimp_dock_window_display_changed (GimpDockWindow *dock_window,
                                  GimpObject     *display,
                                  GimpContext    *context)
{
  gimp_ui_manager_update (dock_window->p->ui_manager,
                          display);
}

static void
gimp_dock_window_image_changed (GimpDockWindow *dock_window,
                                GimpImage     *image,
                                GimpContext   *context)
{
  gimp_ui_manager_update (dock_window->p->ui_manager,
                          gimp_context_get_display (context));
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
  GimpDock *dock  = gimp_dock_window_get_dock (dock_window);
  gchar    *title = gimp_dock_get_title (dock);

  if (title)
    gtk_window_set_title (GTK_WINDOW (dock_window), title);

  g_free (title);

  dock_window->p->update_title_idle_id = 0;

  return FALSE;
}

gint
gimp_dock_window_get_id (GimpDockWindow *dock_window)
{
  g_return_val_if_fail (GIMP_IS_DOCK_WINDOW (dock_window), 0);

  return dock_window->p->ID;
}

GimpUIManager *
gimp_dock_window_get_ui_manager (GimpDockWindow *dock_window)
{
  g_return_val_if_fail (GIMP_IS_DOCK_WINDOW (dock_window), NULL);

  return dock_window->p->ui_manager;
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
