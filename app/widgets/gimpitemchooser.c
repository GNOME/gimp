/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitemchooser.c
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
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimplayer.h"
#include "core/gimpviewable.h"

#include "path/gimppath.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpitemchooser.h"
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
  PROP_ITEM_TYPE,
  PROP_ITEM,
  PROP_VIEW_SIZE,
  PROP_VIEW_BORDER_WIDTH
};

struct _GimpItemChooserPrivate
{
  GType        item_type;
  GimpItem    *item;
  GimpContext *context;

  gint         view_size;
  gint         view_border_width;

  GtkWidget   *image_view;
  GtkWidget   *layer_view;
  GtkWidget   *channel_view;
  GtkWidget   *path_view;
  GtkWidget   *layer_label;
};


static void   gimp_item_chooser_constructed    (GObject           *object);
static void   gimp_item_chooser_finalize       (GObject           *object);
static void   gimp_item_chooser_set_property   (GObject           *object,
                                                guint              property_id,
                                                const GValue      *value,
                                                GParamSpec        *pspec);
static void   gimp_item_chooser_get_property   (GObject           *object,
                                                guint              property_id,
                                                GValue            *value,
                                                GParamSpec        *pspec);

static void   gimp_item_chooser_image_changed  (GimpContext       *context,
                                                GimpImage         *image,
                                                GimpItemChooser   *chooser);
static void   gimp_item_chooser_item_activated (GimpContainerView *view,
                                                GimpItem          *item,
                                                GimpItemChooser   *chooser);
static void   gimp_item_chooser_items_selected (GimpContainerView *view,
                                                GimpItemChooser   *chooser);


G_DEFINE_TYPE_WITH_PRIVATE (GimpItemChooser, gimp_item_chooser, GTK_TYPE_FRAME)

#define parent_class gimp_item_chooser_parent_class

static guint signals[LAST_SIGNAL];

static void
gimp_item_chooser_class_init (GimpItemChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_item_chooser_constructed;
  object_class->finalize     = gimp_item_chooser_finalize;
  object_class->get_property = gimp_item_chooser_get_property;
  object_class->set_property = gimp_item_chooser_set_property;

  /**
   * GimpItemChooser::activate:
   * @chooser:
   *
   * Emitted when a item is activated, which is mostly forwarding when
   * "item-activated" signal is emitted from any of either the image, layer or
   * channel view. E.g. this happens when one double-click on one of the
   * items.
   */
  signals[ACTIVATE] =
    g_signal_new ("activate",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpItemChooserClass, activate),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_OBJECT);

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ITEM_TYPE,
                                   g_param_spec_gtype ("item-type",
                                                       NULL, NULL,
                                                       GIMP_TYPE_ITEM,
                                                       GIMP_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ITEM,
                                   g_param_spec_object ("item",
                                                        NULL, NULL,
                                                        GIMP_TYPE_ITEM,
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
gimp_item_chooser_init (GimpItemChooser *chooser)
{
  chooser->priv = gimp_item_chooser_get_instance_private (chooser);

  chooser->priv->view_size         = GIMP_VIEW_SIZE_SMALL;
  chooser->priv->view_border_width = 1;

  chooser->priv->layer_view        = NULL;
  chooser->priv->channel_view      = NULL;
}

static void
gimp_item_chooser_constructed (GObject *object)
{
  GimpItemChooser *chooser = GIMP_ITEM_CHOOSER (object);
  GtkWidget       *hbox;
  GtkWidget       *vbox;
  GtkWidget       *label;
  GtkWidget       *notebook;
  GimpImage       *image;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_CONTEXT (chooser->priv->context));

  gtk_frame_set_shadow_type (GTK_FRAME (chooser), GTK_SHADOW_OUT);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (chooser), hbox);
  gtk_widget_set_visible (hbox, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  label = gtk_label_new (_("Images"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

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
                           G_CALLBACK (gimp_item_chooser_item_activated),
                           G_OBJECT (chooser), 0);
  g_signal_connect_object (chooser->priv->image_view, "selection-changed",
                           G_CALLBACK (gimp_item_chooser_items_selected),
                           G_OBJECT (chooser), 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  chooser->priv->layer_label = label =
    gtk_label_new (_("Select an image in the left pane"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_set_visible (notebook, TRUE);

  if (g_type_is_a (GIMP_TYPE_LAYER, chooser->priv->item_type))
    {
      chooser->priv->layer_view =
        gimp_container_tree_view_new (NULL,
                                      chooser->priv->context,
                                      chooser->priv->view_size,
                                      chooser->priv->view_border_width);
      gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (GIMP_CONTAINER_TREE_VIEW (chooser->priv->layer_view)->view),
                                        TRUE);
      gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (chooser->priv->layer_view),
                                           4 * (chooser->priv->view_size +
                                                2 * chooser->priv->view_border_width),
                                           4 * (chooser->priv->view_size +
                                                2 * chooser->priv->view_border_width));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                chooser->priv->layer_view,
                                gtk_label_new (_("Layers")));
      gtk_widget_set_visible (chooser->priv->layer_view, TRUE);

      g_signal_connect_object (chooser->priv->layer_view, "item-activated",
                               G_CALLBACK (gimp_item_chooser_item_activated),
                               G_OBJECT (chooser), 0);
      g_signal_connect_object (chooser->priv->layer_view, "selection-changed",
                               G_CALLBACK (gimp_item_chooser_items_selected),
                               G_OBJECT (chooser), 0);
    }

  if (g_type_is_a (GIMP_TYPE_CHANNEL, chooser->priv->item_type))
    {
      chooser->priv->channel_view =
        gimp_container_tree_view_new (NULL,
                                      chooser->priv->context,
                                      chooser->priv->view_size,
                                      chooser->priv->view_border_width);
      gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (chooser->priv->channel_view),
                                           4 * (chooser->priv->view_size +
                                                2 * chooser->priv->view_border_width),
                                           4 * (chooser->priv->view_size +
                                                2 * chooser->priv->view_border_width));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                chooser->priv->channel_view,
                                gtk_label_new (_("Channels")));
      gtk_widget_set_visible (chooser->priv->channel_view, TRUE);

      g_signal_connect_object (chooser->priv->channel_view, "item-activated",
                               G_CALLBACK (gimp_item_chooser_item_activated),
                               G_OBJECT (chooser), 0);
      g_signal_connect_object (chooser->priv->channel_view, "selection-changed",
                               G_CALLBACK (gimp_item_chooser_items_selected),
                               G_OBJECT (chooser), 0);
    }

  if (g_type_is_a (GIMP_TYPE_PATH, chooser->priv->item_type))
    {
      chooser->priv->path_view =
        gimp_container_tree_view_new (NULL,
                                      chooser->priv->context,
                                      chooser->priv->view_size,
                                      chooser->priv->view_border_width);
      gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (chooser->priv->path_view),
                                           4 * (chooser->priv->view_size +
                                                2 * chooser->priv->view_border_width),
                                           4 * (chooser->priv->view_size +
                                                2 * chooser->priv->view_border_width));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                chooser->priv->path_view,
                                gtk_label_new (_("Paths")));
      gtk_widget_set_visible (chooser->priv->path_view, TRUE);

      g_signal_connect_object (chooser->priv->path_view, "item-activated",
                               G_CALLBACK (gimp_item_chooser_item_activated),
                               G_OBJECT (chooser), 0);
      g_signal_connect_object (chooser->priv->path_view, "selection-changed",
                               G_CALLBACK (gimp_item_chooser_items_selected),
                               G_OBJECT (chooser), 0);
    }

  g_signal_connect_object (chooser->priv->context, "image-changed",
                           G_CALLBACK (gimp_item_chooser_image_changed),
                           G_OBJECT (chooser), 0);

  image = gimp_context_get_image (chooser->priv->context);
  gimp_item_chooser_image_changed (chooser->priv->context, image, chooser);
}

static void
gimp_item_chooser_finalize (GObject *object)
{
  GimpItemChooser *chooser = GIMP_ITEM_CHOOSER (object);

  g_clear_object (&chooser->priv->item);
  g_clear_object (&chooser->priv->context);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_item_chooser_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpItemChooser *chooser = GIMP_ITEM_CHOOSER (object);

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

    case PROP_ITEM_TYPE:
      g_return_if_fail (g_value_get_gtype (value) == GIMP_TYPE_LAYER    ||
                        g_value_get_gtype (value) == GIMP_TYPE_CHANNEL  ||
                        g_value_get_gtype (value) == GIMP_TYPE_DRAWABLE ||
                        g_value_get_gtype (value) == GIMP_TYPE_PATH     ||
                        g_value_get_gtype (value) == GIMP_TYPE_IMAGE    ||
                        g_value_get_gtype (value) == GIMP_TYPE_ITEM);
      chooser->priv->item_type = g_value_get_gtype (value);
      break;

    case PROP_ITEM:
      g_return_if_fail (g_value_get_object (value) == NULL ||
                        g_type_is_a (G_TYPE_FROM_INSTANCE (g_value_get_object (value)),
                                     chooser->priv->item_type));
      gimp_item_chooser_set_item (chooser, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_item_chooser_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpItemChooser *chooser = GIMP_ITEM_CHOOSER (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, chooser->priv->context);
      break;

    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, chooser->priv->item_type);
      break;

    case PROP_ITEM:
      g_value_set_object (value, chooser->priv->item);
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
gimp_item_chooser_new (GimpContext *context,
                       GType        item_type,
                       gint         view_size,
                       gint         view_border_width)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= GIMP_VIEWABLE_MAX_POPUP_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  return g_object_new (GIMP_TYPE_ITEM_CHOOSER,
                       "context",           context,
                       "item-type",         item_type,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       NULL);
}

GimpItem *
gimp_item_chooser_get_item (GimpItemChooser *chooser)
{
  g_return_val_if_fail (GIMP_IS_ITEM_CHOOSER (chooser), NULL);

  return chooser->priv->item;
}

void
gimp_item_chooser_set_item (GimpItemChooser *chooser,
                            GimpItem        *item)
{
  if (! gtk_widget_in_destruction (GTK_WIDGET (chooser)))
    {
      g_signal_handlers_disconnect_by_func (chooser->priv->image_view,
                                            G_CALLBACK (gimp_item_chooser_items_selected),
                                            chooser);
      if (chooser->priv->layer_view != NULL)
        g_signal_handlers_disconnect_by_func (chooser->priv->layer_view,
                                              G_CALLBACK (gimp_item_chooser_items_selected),
                                              chooser);
      else
        g_return_if_fail (! GIMP_IS_LAYER (item));

      if (chooser->priv->channel_view != NULL)
        g_signal_handlers_disconnect_by_func (chooser->priv->channel_view,
                                              G_CALLBACK (gimp_item_chooser_items_selected),
                                              chooser);
      else
        g_return_if_fail (! GIMP_IS_CHANNEL (item));

      if (chooser->priv->path_view != NULL)
        g_signal_handlers_disconnect_by_func (chooser->priv->path_view,
                                              G_CALLBACK (gimp_item_chooser_items_selected),
                                              chooser);
      else
        g_return_if_fail (! GIMP_IS_PATH (item));

      if (GIMP_IS_IMAGE (item))
        {
          gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (chooser->priv->image_view),
                                              GIMP_VIEWABLE (item));
          gimp_context_set_image (chooser->priv->context, GIMP_IMAGE (item));
        }
      else if (GIMP_IS_LAYER (item))
        {
          gimp_context_set_image (chooser->priv->context, gimp_item_get_image (GIMP_ITEM (item)));
          gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (chooser->priv->layer_view),
                                              GIMP_VIEWABLE (item));
        }
      else if (GIMP_IS_CHANNEL (item))
        {
          gimp_context_set_image (chooser->priv->context, gimp_item_get_image (GIMP_ITEM (item)));
          gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (chooser->priv->channel_view),
                                              GIMP_VIEWABLE (item));
        }
      else if (GIMP_IS_PATH (item))
        {
          gimp_context_set_image (chooser->priv->context, gimp_item_get_image (GIMP_ITEM (item)));
          gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (chooser->priv->path_view),
                                              GIMP_VIEWABLE (item));
        }
      else
        {
          g_return_if_fail (item == NULL);
          gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (chooser->priv->image_view), NULL);
          gimp_context_set_image (chooser->priv->context, NULL);
        }
      g_signal_connect_object (chooser->priv->image_view, "selection-changed",
                               G_CALLBACK (gimp_item_chooser_items_selected),
                               G_OBJECT (chooser), 0);
      if (chooser->priv->layer_view != NULL)
        g_signal_connect_object (chooser->priv->layer_view, "selection-changed",
                                 G_CALLBACK (gimp_item_chooser_items_selected),
                                 G_OBJECT (chooser), 0);
      if (chooser->priv->channel_view != NULL)
        g_signal_connect_object (chooser->priv->channel_view, "selection-changed",
                                 G_CALLBACK (gimp_item_chooser_items_selected),
                                 G_OBJECT (chooser), 0);
      if (chooser->priv->path_view != NULL)
        g_signal_connect_object (chooser->priv->path_view, "selection-changed",
                                 G_CALLBACK (gimp_item_chooser_items_selected),
                                 G_OBJECT (chooser), 0);
    }

  if (item != chooser->priv->item)
    {
      g_clear_object (&chooser->priv->item);
      chooser->priv->item = (item != NULL ? g_object_ref (item) : NULL);
      g_object_notify (G_OBJECT (chooser), "item");
    }
}


/*  private functions  */

static void
gimp_item_chooser_image_changed (GimpContext     *context,
                                 GimpImage       *image,
                                 GimpItemChooser *chooser)
{
  GimpContainer *layers   = NULL;
  GimpContainer *channels = NULL;
  GimpContainer *paths    = NULL;

  if (image)
    {
      gchar *desc;

      layers   = gimp_image_get_layers (image);
      channels = gimp_image_get_channels (image);
      paths    = gimp_image_get_paths (image);

      desc = gimp_viewable_get_description (GIMP_VIEWABLE (image), NULL);
      gtk_label_set_text (GTK_LABEL (chooser->priv->layer_label), desc);
      g_free (desc);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (chooser->priv->layer_label),
                          _("Select an image in the left pane"));
    }

  g_signal_handlers_disconnect_by_func (chooser->priv->image_view,
                                        G_CALLBACK (gimp_item_chooser_items_selected),
                                        chooser);
  if (chooser->priv->layer_view != NULL)
    {
      g_signal_handlers_disconnect_by_func (chooser->priv->layer_view,
                                            G_CALLBACK (gimp_item_chooser_items_selected),
                                            chooser);
      gimp_container_view_set_container (GIMP_CONTAINER_VIEW (chooser->priv->layer_view),
                                         layers);
      g_signal_connect_object (chooser->priv->layer_view, "selection-changed",
                               G_CALLBACK (gimp_item_chooser_items_selected),
                               G_OBJECT (chooser), 0);
    }
  if (chooser->priv->channel_view != NULL)
    {
      g_signal_handlers_disconnect_by_func (chooser->priv->channel_view,
                                            G_CALLBACK (gimp_item_chooser_items_selected),
                                            chooser);
      gimp_container_view_set_container (GIMP_CONTAINER_VIEW (chooser->priv->channel_view),
                                         channels);
      g_signal_connect_object (chooser->priv->channel_view, "selection-changed",
                               G_CALLBACK (gimp_item_chooser_items_selected),
                               G_OBJECT (chooser), 0);
    }
  if (chooser->priv->path_view != NULL)
    {
      g_signal_handlers_disconnect_by_func (chooser->priv->path_view,
                                            G_CALLBACK (gimp_item_chooser_items_selected),
                                            chooser);
      gimp_container_view_set_container (GIMP_CONTAINER_VIEW (chooser->priv->path_view),
                                         paths);
      g_signal_connect_object (chooser->priv->path_view, "selection-changed",
                               G_CALLBACK (gimp_item_chooser_items_selected),
                               G_OBJECT (chooser), 0);
    }
  g_signal_connect_object (chooser->priv->image_view, "selection-changed",
                           G_CALLBACK (gimp_item_chooser_items_selected),
                           G_OBJECT (chooser), 0);
}

static void
gimp_item_chooser_item_activated (GimpContainerView *view,
                                  GimpItem          *item,
                                  GimpItemChooser   *chooser)
{
  g_signal_emit (chooser, signals[ACTIVATE], 0, item);
}

static void
gimp_item_chooser_items_selected (GimpContainerView *view,
                                  GimpItemChooser   *chooser)
{
  GimpItem *item = NULL;
  gint      n_items;
  GList    *items;

  n_items = gimp_container_view_get_selected (view, &items);

  g_return_if_fail (n_items <= 1);

  if (items)
    item = items->data;

  gimp_item_chooser_set_item (chooser, item);
}
