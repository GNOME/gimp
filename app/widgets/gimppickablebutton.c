/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppickablebutton.c
 * Copyright (C) 2013 Michael Natterer <mitch@gimp.org>
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

#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimppickable.h"

#include "gimpdnd.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"
#include "gimppickablebutton.h"
#include "gimppickablepopup.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_PICKABLE
};

struct _GimpPickableButtonPrivate
{
  gint          view_size;
  gint          view_border_width;

  GimpContext  *context;
  GimpPickable *pickable;

  GtkWidget    *view;
};


static void     gimp_pickable_button_constructed   (GObject            *object);
static void     gimp_pickable_button_dispose       (GObject            *object);
static void     gimp_pickable_button_finalize      (GObject            *object);
static void     gimp_pickable_button_set_property  (GObject            *object,
                                                    guint               property_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void     gimp_pickable_button_get_property  (GObject            *object,
                                                    guint               property_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);

static void     gimp_pickable_button_clicked       (GtkButton          *button);

static void     gimp_pickable_button_popup_confirm (GimpPickablePopup  *popup,
                                                    GimpPickableButton *button);
static void     gimp_pickable_button_drop_pickable (GtkWidget          *widget,
                                                    gint                x,
                                                    gint                y,
                                                    GimpViewable       *viewable,
                                                    gpointer            data);
static void     gimp_pickable_button_notify_buffer (GimpPickable       *pickable,
                                                    const GParamSpec   *pspec,
                                                    GimpPickableButton *button);


G_DEFINE_TYPE (GimpPickableButton, gimp_pickable_button, GIMP_TYPE_BUTTON)

#define parent_class gimp_pickable_button_parent_class


static void
gimp_pickable_button_class_init (GimpPickableButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  object_class->constructed  = gimp_pickable_button_constructed;
  object_class->dispose      = gimp_pickable_button_dispose;
  object_class->finalize     = gimp_pickable_button_finalize;
  object_class->get_property = gimp_pickable_button_get_property;
  object_class->set_property = gimp_pickable_button_set_property;

  button_class->clicked      = gimp_pickable_button_clicked;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PICKABLE,
                                   g_param_spec_object ("pickable",
                                                        NULL, NULL,
                                                        GIMP_TYPE_PICKABLE,
                                                        GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpPickableButtonPrivate));
}

static void
gimp_pickable_button_init (GimpPickableButton *button)
{
  button->private = G_TYPE_INSTANCE_GET_PRIVATE (button,
                                                 GIMP_TYPE_PICKABLE_BUTTON,
                                                 GimpPickableButtonPrivate);

  button->private->view_size         = GIMP_VIEW_SIZE_LARGE;
  button->private->view_border_width = 1;

  gimp_dnd_viewable_dest_add  (GTK_WIDGET (button), GIMP_TYPE_LAYER,
                               gimp_pickable_button_drop_pickable,
                               NULL);
  gimp_dnd_viewable_dest_add  (GTK_WIDGET (button), GIMP_TYPE_LAYER_MASK,
                               gimp_pickable_button_drop_pickable,
                               NULL);
  gimp_dnd_viewable_dest_add  (GTK_WIDGET (button), GIMP_TYPE_CHANNEL,
                               gimp_pickable_button_drop_pickable,
                               NULL);
  gimp_dnd_viewable_dest_add  (GTK_WIDGET (button), GIMP_TYPE_IMAGE,
                               gimp_pickable_button_drop_pickable,
                               NULL);
}

static void
gimp_pickable_button_constructed (GObject *object)
{
  GimpPickableButton *button = GIMP_PICKABLE_BUTTON (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_CONTEXT (button->private->context));

  button->private->view =
    gimp_view_new_by_types (button->private->context,
                            GIMP_TYPE_VIEW,
                            GIMP_TYPE_VIEWABLE,
                            button->private->view_size,
                            button->private->view_border_width,
                            FALSE);
  gtk_container_add (GTK_CONTAINER (button), button->private->view);
  gtk_widget_show (button->private->view);
}

static void
gimp_pickable_button_dispose (GObject *object)
{
  GimpPickableButton *button = GIMP_PICKABLE_BUTTON (object);

  gimp_pickable_button_set_pickable (button, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_pickable_button_finalize (GObject *object)
{
  GimpPickableButton *button = GIMP_PICKABLE_BUTTON (object);

  if (button->private->context)
    {
      g_object_unref (button->private->context);
      button->private->context = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_pickable_button_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpPickableButton *button = GIMP_PICKABLE_BUTTON (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      if (button->private->context)
        g_object_unref (button->private->context);
      button->private->context = g_value_dup_object (value);
      break;
    case PROP_PICKABLE:
      gimp_pickable_button_set_pickable (button, g_value_get_object (value));
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_pickable_button_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpPickableButton *button = GIMP_PICKABLE_BUTTON (object);

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
gimp_pickable_button_clicked (GtkButton *button)
{
  GimpPickableButton *pickable_button = GIMP_PICKABLE_BUTTON (button);
  GtkWidget          *popup;

  popup = gimp_pickable_popup_new (pickable_button->private->context,
                                   pickable_button->private->view_size,
                                   pickable_button->private->view_border_width);

  g_signal_connect (popup, "confirm",
                    G_CALLBACK (gimp_pickable_button_popup_confirm),
                    button);

  gimp_popup_show (GIMP_POPUP (popup), GTK_WIDGET (button));
}

static void
gimp_pickable_button_popup_confirm (GimpPickablePopup  *popup,
                                    GimpPickableButton *button)
{
  GimpPickable *pickable = gimp_pickable_popup_get_pickable (popup);

  if (pickable)
    gimp_pickable_button_set_pickable (button, pickable);
}

static void
gimp_pickable_button_drop_pickable (GtkWidget    *widget,
                                    gint          x,
                                    gint          y,
                                    GimpViewable *viewable,
                                    gpointer      data)
{
  gimp_pickable_button_set_pickable (GIMP_PICKABLE_BUTTON (widget),
                                     GIMP_PICKABLE (viewable));
}

static void
gimp_pickable_button_notify_buffer (GimpPickable       *pickable,
                                    const GParamSpec   *pspec,
                                    GimpPickableButton *button)
{
  GeglBuffer *buffer = gimp_pickable_get_buffer (pickable);

  if (buffer)
    g_object_notify (G_OBJECT (button), "pickable");
  else
    gimp_pickable_button_set_pickable (button, NULL);
}


/*  public functions  */

GtkWidget *
gimp_pickable_button_new (GimpContext *context,
                          gint         view_size,
                          gint         view_border_width)
{
  GimpPickableButton *button;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= GIMP_VIEWABLE_MAX_BUTTON_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  button = g_object_new (GIMP_TYPE_PICKABLE_BUTTON,
                         "context", context,
                         NULL);

  button->private->view_size         = view_size;
  button->private->view_border_width = view_border_width;

  return GTK_WIDGET (button);
}

GimpPickable *
gimp_pickable_button_get_pickable (GimpPickableButton *button)
{
  g_return_val_if_fail (GIMP_IS_PICKABLE_BUTTON (button), NULL);

  return button->private->pickable;
}

void
gimp_pickable_button_set_pickable (GimpPickableButton *button,
                                   GimpPickable       *pickable)
{
  g_return_if_fail (GIMP_IS_PICKABLE_BUTTON (button));

  if (pickable != button->private->pickable)
    {
      if (button->private->pickable)
        {
          g_signal_handlers_disconnect_by_func (button->private->pickable,
                                                gimp_pickable_button_notify_buffer,
                                                button);

          g_object_unref (button->private->pickable);
        }

      button->private->pickable = pickable;

      if (button->private->pickable)
        {
          g_object_ref (button->private->pickable);

          g_signal_connect (button->private->pickable, "notify::buffer",
                            G_CALLBACK (gimp_pickable_button_notify_buffer),
                            button);
        }

      gimp_view_set_viewable (GIMP_VIEW (button->private->view),
                              GIMP_VIEWABLE (pickable));

      g_object_notify (G_OBJECT (button), "pickable");
    }
}
