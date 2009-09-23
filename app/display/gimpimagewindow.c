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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpuimanager.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpimagewindow.h"

#include "gimp-log.h"
#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_MENU_FACTORY
};


/*  local function prototypes  */

static GObject * gimp_image_window_constructor  (GType                type,
                                                 guint                n_params,
                                                 GObjectConstructParam *params);
static void      gimp_image_window_finalize     (GObject             *object);
static void      gimp_image_window_set_property (GObject             *object,
                                                 guint                property_id,
                                                 const GValue        *value,
                                                 GParamSpec          *pspec);
static void      gimp_image_window_get_property (GObject             *object,
                                                 guint                property_id,
                                                 GValue              *value,
                                                 GParamSpec          *pspec);

static void      gimp_image_window_destroy      (GtkObject           *object);

static gboolean  gimp_image_window_window_state (GtkWidget           *widget,
                                                 GdkEventWindowState *event);


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

  gtk_object_class->destroy        = gimp_image_window_destroy;

  widget_class->window_state_event = gimp_image_window_window_state;

  g_object_class_install_property (object_class, PROP_MENU_FACTORY,
                                   g_param_spec_object ("menu-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_MENU_FACTORY,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  gtk_rc_parse_string (image_window_rc_style);
}

static void
gimp_image_window_init (GimpImageWindow *window)
{
}

static GObject *
gimp_image_window_constructor (GType                  type,
                               guint                  n_params,
                               GObjectConstructParam *params)
{
  GObject         *object;
  GimpImageWindow *window;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  window = GIMP_IMAGE_WINDOW (object);

  g_assert (GIMP_IS_UI_MANAGER (window->menubar_manager));

  window->main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), window->main_vbox);
  gtk_widget_show (window->main_vbox);

  return object;
}

static void
gimp_image_window_finalize (GObject *object)
{
  GimpImageWindow *window = GIMP_IMAGE_WINDOW (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_image_window_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpImageWindow *window = GIMP_IMAGE_WINDOW (object);

  switch (property_id)
    {
    case PROP_MENU_FACTORY:
      {
        GimpMenuFactory *factory = g_value_get_object (value);

        window->menubar_manager = gimp_menu_factory_manager_new (factory,
                                                                 "<Image>",
                                                                 window,
                                                                 FALSE);
      }
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
  GimpImageWindow *window = GIMP_IMAGE_WINDOW (object);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_window_destroy (GtkObject *object)
{
  GimpImageWindow *window = GIMP_IMAGE_WINDOW (object);

  if (window->menubar_manager)
    {
      g_object_unref (window->menubar_manager);
      window->menubar_manager = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static gboolean
gimp_image_window_window_state (GtkWidget           *widget,
                                GdkEventWindowState *event)
{
  GimpImageWindow *window  = GIMP_IMAGE_WINDOW (widget);
  GimpDisplay     *display = gimp_image_window_get_active_display (window);

  window->window_state = event->new_window_state;

  if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
    {
      GimpActionGroup *group;
      gboolean         fullscreen;

      fullscreen = gimp_image_window_get_fullscreen (window);

      GIMP_LOG (WM, "Image window '%s' [%p] set fullscreen %s",
                gtk_window_get_title (GTK_WINDOW (widget)),
                widget,
                fullscreen ? "TURE" : "FALSE");

      group = gimp_ui_manager_get_action_group (window->menubar_manager, "view");
      gimp_action_group_set_action_active (group,
                                           "view-fullscreen", fullscreen);
    }

  if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED)
    {
      gboolean iconified = (event->new_window_state &
                            GDK_WINDOW_STATE_ICONIFIED) != 0;

      GIMP_LOG (WM, "Image window '%s' [%p] set %s",
                gtk_window_get_title (GTK_WINDOW (widget)),
                widget,
                iconified ? "iconified" : "uniconified");

      if (iconified)
        {
          if (gimp_displays_get_num_visible (display->gimp) == 0)
            {
              GIMP_LOG (WM, "No displays visible any longer");

              gimp_dialog_factories_hide_with_display ();
            }
        }
      else
        {
          gimp_dialog_factories_show_with_display ();
        }

    }

  return FALSE;
}


/*  public functions  */

GimpDisplay *
gimp_image_window_get_active_display (GimpImageWindow *window)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), NULL);

  return GIMP_DISPLAY_SHELL (window)->display;
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
  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW (window), FALSE);

  return (window->window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
}
