/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagechooser.c
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
#include "core/gimpviewable.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpimagechooser.h"
#include "gimpviewrenderer.h"

#include "gimp-intl.h"


enum
{
  ACTIVATE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_IMAGE,
  PROP_VIEW_SIZE,
  PROP_VIEW_BORDER_WIDTH
};

struct _GimpImageChooserPrivate
{
  GimpImage    *image;
  GimpContext  *context;

  gint          view_size;
  gint          view_border_width;

  GtkWidget    *image_view;
};


static void   gimp_image_chooser_constructed    (GObject             *object);
static void   gimp_image_chooser_finalize       (GObject             *object);
static void   gimp_image_chooser_set_property   (GObject             *object,
                                                 guint                property_id,
                                                 const GValue        *value,
                                                 GParamSpec          *pspec);
static void   gimp_image_chooser_get_property   (GObject             *object,
                                                 guint                property_id,
                                                 GValue              *value,
                                                 GParamSpec          *pspec);

static void   gimp_image_chooser_item_activated (GimpContainerView   *view,
                                                 GimpImage           *image,
                                                 GimpImageChooser    *chooser);
static void   gimp_image_chooser_items_selected (GimpContainerView   *view,
                                                 GimpImageChooser    *chooser);


G_DEFINE_TYPE_WITH_PRIVATE (GimpImageChooser, gimp_image_chooser, GTK_TYPE_FRAME)

#define parent_class gimp_image_chooser_parent_class

static guint signals[LAST_SIGNAL];

static void
gimp_image_chooser_class_init (GimpImageChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_image_chooser_constructed;
  object_class->finalize     = gimp_image_chooser_finalize;
  object_class->get_property = gimp_image_chooser_get_property;
  object_class->set_property = gimp_image_chooser_set_property;

  /**
   * GimpImageChooser::activate:
   * @chooser:
   *
   * Emitted when an image is activated, which is mostly forwarding when
   * "item-activated" signal is emitted from the image view.
   */
  signals[ACTIVATE] =
    g_signal_new ("activate",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageChooserClass, activate),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_OBJECT);

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_EXPLICIT_NOTIFY));

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
gimp_image_chooser_init (GimpImageChooser *chooser)
{
  chooser->priv = gimp_image_chooser_get_instance_private (chooser);

  chooser->priv->view_size         = GIMP_VIEW_SIZE_SMALL;
  chooser->priv->view_border_width = 1;
}

static void
gimp_image_chooser_constructed (GObject *object)
{
  GimpImageChooser *chooser = GIMP_IMAGE_CHOOSER (object);
  GtkWidget        *hbox;
  GtkWidget        *vbox;
  GtkWidget        *label;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_CONTEXT (chooser->priv->context));

  gtk_frame_set_shadow_type (GTK_FRAME (chooser), GTK_SHADOW_OUT);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (chooser), hbox);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Images"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  chooser->priv->image_view =
    gimp_container_tree_view_new (chooser->priv->context->gimp->images,
                                  chooser->priv->context,
                                  chooser->priv->view_size,
                                  chooser->priv->view_border_width);
  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (chooser->priv->image_view),
                                       4 * (chooser->priv->view_size +
                                            2 * chooser->priv->view_border_width),
                                       4 * (chooser->priv->view_size +
                                            2 * chooser->priv->view_border_width));
  gtk_box_pack_start (GTK_BOX (vbox), chooser->priv->image_view, TRUE, TRUE, 0);
  gtk_widget_set_visible (chooser->priv->image_view, TRUE);

  g_signal_connect_object (chooser->priv->image_view, "item-activated",
                           G_CALLBACK (gimp_image_chooser_item_activated),
                           G_OBJECT (chooser), 0);
  g_signal_connect_object (chooser->priv->image_view, "selection-changed",
                           G_CALLBACK (gimp_image_chooser_items_selected),
                           G_OBJECT (chooser), 0);
}

static void
gimp_image_chooser_finalize (GObject *object)
{
  GimpImageChooser *chooser = GIMP_IMAGE_CHOOSER (object);

  g_clear_object (&chooser->priv->image);
  g_clear_object (&chooser->priv->context);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_image_chooser_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpImageChooser *chooser = GIMP_IMAGE_CHOOSER (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      chooser->priv->context = g_value_dup_object (value);
      break;

    case PROP_VIEW_SIZE:
      chooser->priv->view_size = g_value_get_int (value);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      chooser->priv->view_border_width = g_value_get_int (value);
      break;

    case PROP_IMAGE:
      gimp_image_chooser_set_image (chooser, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_chooser_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpImageChooser *chooser = GIMP_IMAGE_CHOOSER (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, chooser->priv->context);
      break;

    case PROP_IMAGE:
      g_value_set_object (value, chooser->priv->image);
      break;

    case PROP_VIEW_SIZE:
      g_value_set_int (value, chooser->priv->view_size);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      g_value_set_int (value, chooser->priv->view_border_width);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_image_chooser_new (GimpContext *context,
                        gint         view_size,
                        gint         view_border_width)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= GIMP_VIEWABLE_MAX_POPUP_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  return g_object_new (GIMP_TYPE_IMAGE_CHOOSER,
                       "context",           context,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       NULL);
}

GimpImage *
gimp_image_chooser_get_image (GimpImageChooser *chooser)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_CHOOSER (chooser), NULL);

  return chooser->priv->image;
}

void
gimp_image_chooser_set_image (GimpImageChooser *chooser,
                              GimpImage        *image)
{
  if (! gtk_widget_in_destruction (GTK_WIDGET (chooser)))
    {
      g_signal_handlers_disconnect_by_func (chooser->priv->image_view,
                                            G_CALLBACK (gimp_image_chooser_items_selected),
                                            chooser);

      if (GIMP_IS_IMAGE (image))
        {
          gimp_context_set_image (chooser->priv->context, image);
          gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (chooser->priv->image_view),
                                              GIMP_VIEWABLE (image));
        }

      g_signal_connect_object (chooser->priv->image_view, "selection-changed",
                               G_CALLBACK (gimp_image_chooser_items_selected),
                               G_OBJECT (chooser), 0);
    }

  if (image != chooser->priv->image)
    {
      g_clear_object (&chooser->priv->image);
      chooser->priv->image = (image != NULL ? g_object_ref (image) : NULL);
      g_object_notify (G_OBJECT (chooser), "image");
    }
}


/*  private functions  */

static void
gimp_image_chooser_item_activated (GimpContainerView *view,
                                   GimpImage         *image,
                                   GimpImageChooser  *chooser)
{
  g_signal_emit (chooser, signals[ACTIVATE], 0, image);
}

static void
gimp_image_chooser_items_selected (GimpContainerView *view,
                                   GimpImageChooser  *chooser)
{
  GimpImage *image = NULL;
  gint       n_items;
  GList     *items;

  n_items = gimp_container_view_get_selected (view, &items);

  g_return_if_fail (n_items <= 1);

  if (items)
    image = items->data;

  gimp_image_chooser_set_image (chooser, image);
}
