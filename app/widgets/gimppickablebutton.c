/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapickablebutton.c
 * Copyright (C) 2013 Michael Natterer <mitch@ligma.org>
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

#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmalayermask.h"
#include "core/ligmapickable.h"

#include "ligmadnd.h"
#include "ligmaview.h"
#include "ligmaviewrenderer.h"
#include "ligmapickablebutton.h"
#include "ligmapickablepopup.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_PICKABLE
};

struct _LigmaPickableButtonPrivate
{
  gint          view_size;
  gint          view_border_width;

  LigmaContext  *context;
  LigmaPickable *pickable;

  GtkWidget    *view;
};


static void     ligma_pickable_button_constructed   (GObject            *object);
static void     ligma_pickable_button_dispose       (GObject            *object);
static void     ligma_pickable_button_finalize      (GObject            *object);
static void     ligma_pickable_button_set_property  (GObject            *object,
                                                    guint               property_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void     ligma_pickable_button_get_property  (GObject            *object,
                                                    guint               property_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);

static void     ligma_pickable_button_clicked       (GtkButton          *button);

static void     ligma_pickable_button_popup_confirm (LigmaPickablePopup  *popup,
                                                    LigmaPickableButton *button);
static void     ligma_pickable_button_drop_pickable (GtkWidget          *widget,
                                                    gint                x,
                                                    gint                y,
                                                    LigmaViewable       *viewable,
                                                    gpointer            data);
static void     ligma_pickable_button_notify_buffer (LigmaPickable       *pickable,
                                                    const GParamSpec   *pspec,
                                                    LigmaPickableButton *button);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaPickableButton, ligma_pickable_button,
                            LIGMA_TYPE_BUTTON)

#define parent_class ligma_pickable_button_parent_class


static void
ligma_pickable_button_class_init (LigmaPickableButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  object_class->constructed  = ligma_pickable_button_constructed;
  object_class->dispose      = ligma_pickable_button_dispose;
  object_class->finalize     = ligma_pickable_button_finalize;
  object_class->get_property = ligma_pickable_button_get_property;
  object_class->set_property = ligma_pickable_button_set_property;

  button_class->clicked      = ligma_pickable_button_clicked;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTEXT,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PICKABLE,
                                   g_param_spec_object ("pickable",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_PICKABLE,
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_pickable_button_init (LigmaPickableButton *button)
{
  button->private = ligma_pickable_button_get_instance_private (button);

  button->private->view_size         = LIGMA_VIEW_SIZE_LARGE;
  button->private->view_border_width = 1;

  ligma_dnd_viewable_dest_add  (GTK_WIDGET (button), LIGMA_TYPE_LAYER,
                               ligma_pickable_button_drop_pickable,
                               NULL);
  ligma_dnd_viewable_dest_add  (GTK_WIDGET (button), LIGMA_TYPE_LAYER_MASK,
                               ligma_pickable_button_drop_pickable,
                               NULL);
  ligma_dnd_viewable_dest_add  (GTK_WIDGET (button), LIGMA_TYPE_CHANNEL,
                               ligma_pickable_button_drop_pickable,
                               NULL);
  ligma_dnd_viewable_dest_add  (GTK_WIDGET (button), LIGMA_TYPE_IMAGE,
                               ligma_pickable_button_drop_pickable,
                               NULL);
}

static void
ligma_pickable_button_constructed (GObject *object)
{
  LigmaPickableButton *button = LIGMA_PICKABLE_BUTTON (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_CONTEXT (button->private->context));

  button->private->view =
    ligma_view_new_by_types (button->private->context,
                            LIGMA_TYPE_VIEW,
                            LIGMA_TYPE_VIEWABLE,
                            button->private->view_size,
                            button->private->view_border_width,
                            FALSE);
  gtk_container_add (GTK_CONTAINER (button), button->private->view);
  gtk_widget_show (button->private->view);
}

static void
ligma_pickable_button_dispose (GObject *object)
{
  LigmaPickableButton *button = LIGMA_PICKABLE_BUTTON (object);

  ligma_pickable_button_set_pickable (button, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_pickable_button_finalize (GObject *object)
{
  LigmaPickableButton *button = LIGMA_PICKABLE_BUTTON (object);

  g_clear_object (&button->private->context);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_pickable_button_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaPickableButton *button = LIGMA_PICKABLE_BUTTON (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      if (button->private->context)
        g_object_unref (button->private->context);
      button->private->context = g_value_dup_object (value);
      break;
    case PROP_PICKABLE:
      ligma_pickable_button_set_pickable (button, g_value_get_object (value));
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_pickable_button_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaPickableButton *button = LIGMA_PICKABLE_BUTTON (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, button->private->context);
      break;
    case PROP_PICKABLE:
      g_value_set_object (value, button->private->pickable);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_pickable_button_clicked (GtkButton *button)
{
  LigmaPickableButton *pickable_button = LIGMA_PICKABLE_BUTTON (button);
  GtkWidget          *popup;

  popup = ligma_pickable_popup_new (pickable_button->private->context,
                                   pickable_button->private->view_size,
                                   pickable_button->private->view_border_width);

  g_signal_connect (popup, "confirm",
                    G_CALLBACK (ligma_pickable_button_popup_confirm),
                    button);

  ligma_popup_show (LIGMA_POPUP (popup), GTK_WIDGET (button));
}

static void
ligma_pickable_button_popup_confirm (LigmaPickablePopup  *popup,
                                    LigmaPickableButton *button)
{
  LigmaPickable *pickable = ligma_pickable_popup_get_pickable (popup);

  if (pickable)
    ligma_pickable_button_set_pickable (button, pickable);
}

static void
ligma_pickable_button_drop_pickable (GtkWidget    *widget,
                                    gint          x,
                                    gint          y,
                                    LigmaViewable *viewable,
                                    gpointer      data)
{
  ligma_pickable_button_set_pickable (LIGMA_PICKABLE_BUTTON (widget),
                                     LIGMA_PICKABLE (viewable));
}

static void
ligma_pickable_button_notify_buffer (LigmaPickable       *pickable,
                                    const GParamSpec   *pspec,
                                    LigmaPickableButton *button)
{
  GeglBuffer *buffer = ligma_pickable_get_buffer (pickable);

  if (buffer)
    g_object_notify (G_OBJECT (button), "pickable");
  else
    ligma_pickable_button_set_pickable (button, NULL);
}


/*  public functions  */

GtkWidget *
ligma_pickable_button_new (LigmaContext *context,
                          gint         view_size,
                          gint         view_border_width)
{
  LigmaPickableButton *button;

  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_BUTTON_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  button = g_object_new (LIGMA_TYPE_PICKABLE_BUTTON,
                         "context", context,
                         NULL);

  button->private->view_size         = view_size;
  button->private->view_border_width = view_border_width;

  return GTK_WIDGET (button);
}

LigmaPickable *
ligma_pickable_button_get_pickable (LigmaPickableButton *button)
{
  g_return_val_if_fail (LIGMA_IS_PICKABLE_BUTTON (button), NULL);

  return button->private->pickable;
}

void
ligma_pickable_button_set_pickable (LigmaPickableButton *button,
                                   LigmaPickable       *pickable)
{
  g_return_if_fail (LIGMA_IS_PICKABLE_BUTTON (button));

  if (pickable != button->private->pickable)
    {
      if (button->private->pickable)
        g_signal_handlers_disconnect_by_func (button->private->pickable,
                                              ligma_pickable_button_notify_buffer,
                                              button);

      g_set_object (&button->private->pickable, pickable);

      if (button->private->pickable)
        g_signal_connect (button->private->pickable, "notify::buffer",
                          G_CALLBACK (ligma_pickable_button_notify_buffer),
                          button);

      ligma_view_set_viewable (LIGMA_VIEW (button->private->view),
                              LIGMA_VIEWABLE (pickable));

      g_object_notify (G_OBJECT (button), "pickable");
    }
}
