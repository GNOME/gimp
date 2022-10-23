/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewablebutton.c
 * Copyright (C) 2003-2005 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "gimpcontainerpopup.h"
#include "gimpdialogfactory.h"
#include "gimppropwidgets.h"
#include "gimpviewrenderer.h"
#include "gimpviewablebutton.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_POPUP_VIEW_TYPE,
  PROP_POPUP_VIEW_SIZE
};


static void     gimp_viewable_button_finalize     (GObject            *object);
static void     gimp_viewable_button_set_property (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);
static void     gimp_viewable_button_get_property (GObject            *object,
                                                   guint               property_id,
                                                   GValue             *value,
                                                   GParamSpec         *pspec);
static gboolean gimp_viewable_button_scroll_event (GtkWidget          *widget,
                                                   GdkEventScroll     *sevent);
static void     gimp_viewable_button_clicked      (GtkButton          *button);

static void     gimp_viewable_button_popup_closed (GimpContainerPopup *popup,
                                                   GimpViewableButton *button);


G_DEFINE_TYPE (GimpViewableButton, gimp_viewable_button, GIMP_TYPE_BUTTON)

#define parent_class gimp_viewable_button_parent_class


static void
gimp_viewable_button_class_init (GimpViewableButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  object_class->finalize     = gimp_viewable_button_finalize;
  object_class->get_property = gimp_viewable_button_get_property;
  object_class->set_property = gimp_viewable_button_set_property;

  widget_class->scroll_event = gimp_viewable_button_scroll_event;

  button_class->clicked      = gimp_viewable_button_clicked;

  g_object_class_install_property (object_class, PROP_POPUP_VIEW_TYPE,
                                   g_param_spec_enum ("popup-view-type",
                                                      NULL, NULL,
                                                      GIMP_TYPE_VIEW_TYPE,
                                                      GIMP_VIEW_TYPE_LIST,
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_POPUP_VIEW_SIZE,
                                   g_param_spec_int ("popup-view-size",
                                                     NULL, NULL,
                                                     GIMP_VIEW_SIZE_TINY,
                                                     GIMP_VIEW_SIZE_GIGANTIC,
                                                     GIMP_VIEW_SIZE_SMALL,
                                                     GIMP_PARAM_READWRITE));
}

static void
gimp_viewable_button_init (GimpViewableButton *button)
{
  button->popup_view_type   = GIMP_VIEW_TYPE_LIST;
  button->popup_view_size   = GIMP_VIEW_SIZE_SMALL;

  button->button_view_size  = GIMP_VIEW_SIZE_SMALL;
  button->view_border_width = 1;

  gtk_widget_add_events (GTK_WIDGET (button), GDK_SCROLL_MASK);
}

static void
gimp_viewable_button_finalize (GObject *object)
{
  GimpViewableButton *button = GIMP_VIEWABLE_BUTTON (object);

  g_clear_pointer (&button->dialog_identifier, g_free);
  g_clear_pointer (&button->dialog_icon_name,  g_free);
  g_clear_pointer (&button->dialog_tooltip,    g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_viewable_button_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpViewableButton *button = GIMP_VIEWABLE_BUTTON (object);

  switch (property_id)
    {
    case PROP_POPUP_VIEW_TYPE:
      gimp_viewable_button_set_view_type (button, g_value_get_enum (value));
      break;
    case PROP_POPUP_VIEW_SIZE:
      gimp_viewable_button_set_view_size (button, g_value_get_int (value));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_viewable_button_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpViewableButton *button = GIMP_VIEWABLE_BUTTON (object);

  switch (property_id)
    {
    case PROP_POPUP_VIEW_TYPE:
      g_value_set_enum (value, button->popup_view_type);
      break;
    case PROP_POPUP_VIEW_SIZE:
      g_value_set_int (value, button->popup_view_size);
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_viewable_button_scroll_event (GtkWidget      *widget,
                                   GdkEventScroll *sevent)
{
  GimpViewableButton *button = GIMP_VIEWABLE_BUTTON (widget);
  GimpObject         *object;
  gint                index;

  object = gimp_context_get_by_type (button->context,
                                     gimp_container_get_children_type (button->container));

  index = gimp_container_get_child_index (button->container, object);

  if (index != -1)
    {
      gint n_children;
      gint new_index = index;

      n_children = gimp_container_get_n_children (button->container);

      if (sevent->direction == GDK_SCROLL_UP)
        {
          if (index > 0)
            new_index--;
          else
            new_index = n_children - 1;
        }
      else if (sevent->direction == GDK_SCROLL_DOWN)
        {
          if (index == (n_children - 1))
            new_index = 0;
          else
            new_index++;
        }

      if (new_index != index)
        {
          object = gimp_container_get_child_by_index (button->container,
                                                      new_index);

          if (object)
            gimp_context_set_by_type (button->context,
                                      gimp_container_get_children_type (button->container),
                                      object);
        }
    }

  return TRUE;
}

static void
gimp_viewable_button_clicked (GtkButton *button)
{
  GimpViewableButton *viewable_button = GIMP_VIEWABLE_BUTTON (button);
  GtkWidget          *popup;

  popup = gimp_container_popup_new (viewable_button->container,
                                    viewable_button->context,
                                    viewable_button->popup_view_type,
                                    viewable_button->button_view_size,
                                    viewable_button->popup_view_size,
                                    viewable_button->view_border_width,
                                    viewable_button->dialog_factory,
                                    viewable_button->dialog_identifier,
                                    viewable_button->dialog_icon_name,
                                    viewable_button->dialog_tooltip);

  g_signal_connect (popup, "cancel",
                    G_CALLBACK (gimp_viewable_button_popup_closed),
                    button);
  g_signal_connect (popup, "confirm",
                    G_CALLBACK (gimp_viewable_button_popup_closed),
                    button);

  gimp_popup_show (GIMP_POPUP (popup), GTK_WIDGET (button));
}

static void
gimp_viewable_button_popup_closed (GimpContainerPopup *popup,
                                   GimpViewableButton *button)
{
  gimp_viewable_button_set_view_type (button,
                                      gimp_container_popup_get_view_type (popup));
  gimp_viewable_button_set_view_size (button,
                                      gimp_container_popup_get_view_size (popup));
}


/*  public functions  */

GtkWidget *
gimp_viewable_button_new (GimpContainer     *container,
                          GimpContext       *context,
                          GimpViewType       view_type,
                          gint               button_view_size,
                          gint               view_size,
                          gint               view_border_width,
                          GimpDialogFactory *dialog_factory,
                          const gchar       *dialog_identifier,
                          const gchar       *dialog_icon_name,
                          const gchar       *dialog_tooltip)
{
  GimpViewableButton *button;
  const gchar        *prop_name;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= GIMP_VIEWABLE_MAX_BUTTON_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (dialog_factory == NULL ||
                        GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  if (dialog_factory)
    {
      g_return_val_if_fail (dialog_identifier != NULL, NULL);
      g_return_val_if_fail (dialog_icon_name != NULL, NULL);
      g_return_val_if_fail (dialog_tooltip != NULL, NULL);
    }

  button = g_object_new (GIMP_TYPE_VIEWABLE_BUTTON,
                         "popup-view-type", view_type,
                         "popup-view-size", view_size,
                         NULL);

  button->container = container;
  button->context   = context;

  button->button_view_size  = button_view_size;
  button->view_border_width = view_border_width;

  if (dialog_factory)
    {
      button->dialog_factory    = dialog_factory;
      button->dialog_identifier = g_strdup (dialog_identifier);
      button->dialog_icon_name  = g_strdup (dialog_icon_name);
      button->dialog_tooltip    = g_strdup (dialog_tooltip);
    }

  prop_name = gimp_context_type_to_prop_name (gimp_container_get_children_type (container));

  button->view = gimp_prop_view_new (G_OBJECT (context), prop_name,
                                     context, button->button_view_size);
  gtk_container_add (GTK_CONTAINER (button), button->view);

  return GTK_WIDGET (button);
}

GimpViewType
gimp_viewable_button_get_view_type (GimpViewableButton *button)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE_BUTTON (button), GIMP_VIEW_TYPE_LIST);

  return button->popup_view_type;
}

void
gimp_viewable_button_set_view_type (GimpViewableButton *button,
                                    GimpViewType        view_type)
{
  g_return_if_fail (GIMP_IS_VIEWABLE_BUTTON (button));

  if (view_type != button->popup_view_type)
    {
      button->popup_view_type = view_type;

      g_object_notify (G_OBJECT (button), "popup-view-type");
    }
}

gint
gimp_viewable_button_get_view_size (GimpViewableButton *button)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE_BUTTON (button), GIMP_VIEW_SIZE_SMALL);

  return button->popup_view_size;
}

void
gimp_viewable_button_set_view_size (GimpViewableButton *button,
                                    gint                view_size)
{
  g_return_if_fail (GIMP_IS_VIEWABLE_BUTTON (button));

  if (view_size != button->popup_view_size)
    {
      button->popup_view_size = view_size;

      g_object_notify (G_OBJECT (button), "popup-view-size");
    }
}
