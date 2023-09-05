/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppickablepopup.c
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimpviewable.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimppickablechooser.h"
#include "gimppickablepopup.h"
#include "gimpviewrenderer.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_PICKABLE,
  PROP_VIEW_SIZE,
  PROP_VIEW_BORDER_WIDTH
};

struct _GimpPickablePopupPrivate
{
  GimpContext  *context;

  gint          view_size;
  gint          view_border_width;

  GtkWidget    *chooser;

  GtkWidget    *image_view;
  GtkWidget    *layer_view;
  GtkWidget    *channel_view;
  GtkWidget    *layer_label;
};


static void   gimp_pickable_popup_constructed     (GObject             *object);
static void   gimp_pickable_popup_finalize        (GObject             *object);
static void   gimp_pickable_popup_set_property    (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void   gimp_pickable_popup_get_property    (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static void   gimp_pickable_popup_activate        (GimpPickablePopup   *popup);
static void   gimp_pickable_popup_notify_pickable (GimpPickablePopup   *popup);


G_DEFINE_TYPE_WITH_PRIVATE (GimpPickablePopup, gimp_pickable_popup,
                            GIMP_TYPE_POPUP)

#define parent_class gimp_pickable_popup_parent_class


static void
gimp_pickable_popup_class_init (GimpPickablePopupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_pickable_popup_constructed;
  object_class->finalize     = gimp_pickable_popup_finalize;
  object_class->get_property = gimp_pickable_popup_get_property;
  object_class->set_property = gimp_pickable_popup_set_property;

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

  g_object_class_install_property (object_class, PROP_VIEW_SIZE,
                                   g_param_spec_int ("view-size",
                                                     NULL, NULL,
                                                     1, GIMP_VIEWABLE_MAX_PREVIEW_SIZE,
                                                     GIMP_VIEW_SIZE_MEDIUM,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_VIEW_BORDER_WIDTH,
                                   g_param_spec_int ("view-border-width",
                                                     NULL, NULL,
                                                     0,
                                                     GIMP_VIEW_MAX_BORDER_WIDTH,
                                                     1,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
gimp_pickable_popup_init (GimpPickablePopup *popup)
{
  popup->priv = gimp_pickable_popup_get_instance_private (popup);

  popup->priv->view_size         = GIMP_VIEW_SIZE_SMALL;
  popup->priv->view_border_width = 1;

  gtk_window_set_resizable (GTK_WINDOW (popup), FALSE);
}

static void
gimp_pickable_popup_constructed (GObject *object)
{
  GimpPickablePopup *popup = GIMP_PICKABLE_POPUP (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_CONTEXT (popup->priv->context));

  popup->priv->chooser = gimp_pickable_chooser_new (popup->priv->context, GIMP_TYPE_PICKABLE,
                                                    popup->priv->view_size,
                                                    popup->priv->view_border_width);
  gtk_container_add (GTK_CONTAINER (popup), popup->priv->chooser);
  g_signal_connect_swapped (popup->priv->chooser, "notify::pickable",
                            G_CALLBACK (gimp_pickable_popup_notify_pickable),
                            popup);
  g_signal_connect_swapped (popup->priv->chooser, "activate",
                            G_CALLBACK (gimp_pickable_popup_activate),
                            popup);
  gtk_widget_show (popup->priv->chooser);
}

static void
gimp_pickable_popup_finalize (GObject *object)
{
  GimpPickablePopup *popup = GIMP_PICKABLE_POPUP (object);

  g_clear_object (&popup->priv->context);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_pickable_popup_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpPickablePopup *popup = GIMP_PICKABLE_POPUP (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      if (popup->priv->context)
        g_object_unref (popup->priv->context);
      popup->priv->context = g_value_dup_object (value);
      break;

    case PROP_PICKABLE:
      gimp_pickable_chooser_set_pickable (GIMP_PICKABLE_CHOOSER (popup->priv->chooser),
                                          g_value_get_object (value));
      break;

    case PROP_VIEW_SIZE:
      popup->priv->view_size = g_value_get_int (value);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      popup->priv->view_border_width = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_pickable_popup_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpPickablePopup *popup = GIMP_PICKABLE_POPUP (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, popup->priv->context);
      break;

    case PROP_PICKABLE:
      g_value_set_object (value, gimp_pickable_popup_get_pickable (popup));
      break;

    case PROP_VIEW_SIZE:
      g_value_set_int (value, popup->priv->view_size);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      g_value_set_int (value, popup->priv->view_border_width);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public functions */

GtkWidget *
gimp_pickable_popup_new (GimpContext *context,
                         gint         view_size,
                         gint         view_border_width)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= GIMP_VIEWABLE_MAX_POPUP_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  return g_object_new (GIMP_TYPE_PICKABLE_POPUP,
                       "type",              GTK_WINDOW_POPUP,
                       "context",           context,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       NULL);
}

GimpPickable *
gimp_pickable_popup_get_pickable (GimpPickablePopup *popup)
{
  g_return_val_if_fail (GIMP_IS_PICKABLE_POPUP (popup), NULL);

  return gimp_pickable_chooser_get_pickable (GIMP_PICKABLE_CHOOSER (popup->priv->chooser));
}


/* Private functions */

static void
gimp_pickable_popup_activate (GimpPickablePopup *popup)
{
  g_signal_emit_by_name (popup, "confirm");
}

static void
gimp_pickable_popup_notify_pickable (GimpPickablePopup *popup)
{
  g_object_notify (G_OBJECT (popup), "pickable");
}
