/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewablebutton.c
 * Copyright (C) 2003-2005 Michael Natterer <mitch@ligma.org>
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

#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaviewable.h"

#include "ligmacontainerpopup.h"
#include "ligmadialogfactory.h"
#include "ligmapropwidgets.h"
#include "ligmaviewrenderer.h"
#include "ligmaviewablebutton.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_POPUP_VIEW_TYPE,
  PROP_POPUP_VIEW_SIZE
};


static void     ligma_viewable_button_finalize     (GObject            *object);
static void     ligma_viewable_button_set_property (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);
static void     ligma_viewable_button_get_property (GObject            *object,
                                                   guint               property_id,
                                                   GValue             *value,
                                                   GParamSpec         *pspec);
static gboolean ligma_viewable_button_scroll_event (GtkWidget          *widget,
                                                   GdkEventScroll     *sevent);
static void     ligma_viewable_button_clicked      (GtkButton          *button);

static void     ligma_viewable_button_popup_closed (LigmaContainerPopup *popup,
                                                   LigmaViewableButton *button);


G_DEFINE_TYPE (LigmaViewableButton, ligma_viewable_button, LIGMA_TYPE_BUTTON)

#define parent_class ligma_viewable_button_parent_class


static void
ligma_viewable_button_class_init (LigmaViewableButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  object_class->finalize     = ligma_viewable_button_finalize;
  object_class->get_property = ligma_viewable_button_get_property;
  object_class->set_property = ligma_viewable_button_set_property;

  widget_class->scroll_event = ligma_viewable_button_scroll_event;

  button_class->clicked      = ligma_viewable_button_clicked;

  g_object_class_install_property (object_class, PROP_POPUP_VIEW_TYPE,
                                   g_param_spec_enum ("popup-view-type",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_VIEW_TYPE,
                                                      LIGMA_VIEW_TYPE_LIST,
                                                      LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_POPUP_VIEW_SIZE,
                                   g_param_spec_int ("popup-view-size",
                                                     NULL, NULL,
                                                     LIGMA_VIEW_SIZE_TINY,
                                                     LIGMA_VIEW_SIZE_GIGANTIC,
                                                     LIGMA_VIEW_SIZE_SMALL,
                                                     LIGMA_PARAM_READWRITE));
}

static void
ligma_viewable_button_init (LigmaViewableButton *button)
{
  button->popup_view_type   = LIGMA_VIEW_TYPE_LIST;
  button->popup_view_size   = LIGMA_VIEW_SIZE_SMALL;

  button->button_view_size  = LIGMA_VIEW_SIZE_SMALL;
  button->view_border_width = 1;

  gtk_widget_add_events (GTK_WIDGET (button), GDK_SCROLL_MASK);
}

static void
ligma_viewable_button_finalize (GObject *object)
{
  LigmaViewableButton *button = LIGMA_VIEWABLE_BUTTON (object);

  g_clear_pointer (&button->dialog_identifier, g_free);
  g_clear_pointer (&button->dialog_icon_name,  g_free);
  g_clear_pointer (&button->dialog_tooltip,    g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_viewable_button_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaViewableButton *button = LIGMA_VIEWABLE_BUTTON (object);

  switch (property_id)
    {
    case PROP_POPUP_VIEW_TYPE:
      ligma_viewable_button_set_view_type (button, g_value_get_enum (value));
      break;
    case PROP_POPUP_VIEW_SIZE:
      ligma_viewable_button_set_view_size (button, g_value_get_int (value));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_viewable_button_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaViewableButton *button = LIGMA_VIEWABLE_BUTTON (object);

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
ligma_viewable_button_scroll_event (GtkWidget      *widget,
                                   GdkEventScroll *sevent)
{
  LigmaViewableButton *button = LIGMA_VIEWABLE_BUTTON (widget);
  LigmaObject         *object;
  gint                index;

  object = ligma_context_get_by_type (button->context,
                                     ligma_container_get_children_type (button->container));

  index = ligma_container_get_child_index (button->container, object);

  if (index != -1)
    {
      gint n_children;
      gint new_index = index;

      n_children = ligma_container_get_n_children (button->container);

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
          object = ligma_container_get_child_by_index (button->container,
                                                      new_index);

          if (object)
            ligma_context_set_by_type (button->context,
                                      ligma_container_get_children_type (button->container),
                                      object);
        }
    }

  return TRUE;
}

static void
ligma_viewable_button_clicked (GtkButton *button)
{
  LigmaViewableButton *viewable_button = LIGMA_VIEWABLE_BUTTON (button);
  GtkWidget          *popup;

  popup = ligma_container_popup_new (viewable_button->container,
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
                    G_CALLBACK (ligma_viewable_button_popup_closed),
                    button);
  g_signal_connect (popup, "confirm",
                    G_CALLBACK (ligma_viewable_button_popup_closed),
                    button);

  ligma_popup_show (LIGMA_POPUP (popup), GTK_WIDGET (button));
}

static void
ligma_viewable_button_popup_closed (LigmaContainerPopup *popup,
                                   LigmaViewableButton *button)
{
  ligma_viewable_button_set_view_type (button,
                                      ligma_container_popup_get_view_type (popup));
  ligma_viewable_button_set_view_size (button,
                                      ligma_container_popup_get_view_size (popup));
}


/*  public functions  */

GtkWidget *
ligma_viewable_button_new (LigmaContainer     *container,
                          LigmaContext       *context,
                          LigmaViewType       view_type,
                          gint               button_view_size,
                          gint               view_size,
                          gint               view_border_width,
                          LigmaDialogFactory *dialog_factory,
                          const gchar       *dialog_identifier,
                          const gchar       *dialog_icon_name,
                          const gchar       *dialog_tooltip)
{
  LigmaViewableButton *button;
  const gchar        *prop_name;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_BUTTON_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (dialog_factory == NULL ||
                        LIGMA_IS_DIALOG_FACTORY (dialog_factory), NULL);
  if (dialog_factory)
    {
      g_return_val_if_fail (dialog_identifier != NULL, NULL);
      g_return_val_if_fail (dialog_icon_name != NULL, NULL);
      g_return_val_if_fail (dialog_tooltip != NULL, NULL);
    }

  button = g_object_new (LIGMA_TYPE_VIEWABLE_BUTTON,
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

  prop_name = ligma_context_type_to_prop_name (ligma_container_get_children_type (container));

  button->view = ligma_prop_view_new (G_OBJECT (context), prop_name,
                                     context, button->button_view_size);
  gtk_container_add (GTK_CONTAINER (button), button->view);

  return GTK_WIDGET (button);
}

LigmaViewType
ligma_viewable_button_get_view_type (LigmaViewableButton *button)
{
  g_return_val_if_fail (LIGMA_IS_VIEWABLE_BUTTON (button), LIGMA_VIEW_TYPE_LIST);

  return button->popup_view_type;
}

void
ligma_viewable_button_set_view_type (LigmaViewableButton *button,
                                    LigmaViewType        view_type)
{
  g_return_if_fail (LIGMA_IS_VIEWABLE_BUTTON (button));

  if (view_type != button->popup_view_type)
    {
      button->popup_view_type = view_type;

      g_object_notify (G_OBJECT (button), "popup-view-type");
    }
}

gint
ligma_viewable_button_get_view_size (LigmaViewableButton *button)
{
  g_return_val_if_fail (LIGMA_IS_VIEWABLE_BUTTON (button), LIGMA_VIEW_SIZE_SMALL);

  return button->popup_view_size;
}

void
ligma_viewable_button_set_view_size (LigmaViewableButton *button,
                                    gint                view_size)
{
  g_return_if_fail (LIGMA_IS_VIEWABLE_BUTTON (button));

  if (view_size != button->popup_view_size)
    {
      button->popup_view_size = view_size;

      g_object_notify (G_OBJECT (button), "popup-view-size");
    }
}
