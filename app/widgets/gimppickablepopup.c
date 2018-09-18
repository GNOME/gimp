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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimpviewable.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
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
  GimpPickable *pickable;
  GimpContext  *context;

  gint          view_size;
  gint          view_border_width;

  GtkWidget    *image_view;
  GtkWidget    *layer_view;
  GtkWidget    *channel_view;
  GtkWidget    *layer_label;
};


static void   gimp_pickable_popup_constructed    (GObject           *object);
static void   gimp_pickable_popup_finalize       (GObject           *object);
static void   gimp_pickable_popup_set_property   (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);
static void   gimp_pickable_popup_get_property   (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);

static void   gimp_pickable_popup_image_changed  (GimpContext       *context,
                                                  GimpImage         *image,
                                                  GimpPickablePopup *popup);
static void   gimp_pickable_popup_item_activate  (GimpContainerView *view,
                                                  GimpPickable      *pickable,
                                                  gpointer           unused,
                                                  GimpPickablePopup *popup);


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
                                                        GIMP_PARAM_READABLE));

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
  GtkWidget         *frame;
  GtkWidget         *hbox;
  GtkWidget         *vbox;
  GtkWidget         *label;
  GtkWidget         *notebook;
  GimpImage         *image;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_CONTEXT (popup->priv->context));

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (popup), frame);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Images"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  popup->priv->image_view =
    gimp_container_tree_view_new (popup->priv->context->gimp->images,
                                  popup->priv->context,
                                  popup->priv->view_size,
                                  popup->priv->view_border_width);
  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (popup->priv->image_view),
                                       4 * (popup->priv->view_size +
                                            2 * popup->priv->view_border_width),
                                       4 * (popup->priv->view_size +
                                            2 * popup->priv->view_border_width));
  gtk_box_pack_start (GTK_BOX (vbox), popup->priv->image_view, TRUE, TRUE, 0);
  gtk_widget_show (popup->priv->image_view);

  g_signal_connect_object (popup->priv->image_view, "activate-item",
                           G_CALLBACK (gimp_pickable_popup_item_activate),
                           G_OBJECT (popup), 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  popup->priv->layer_label = label =
    gtk_label_new (_("Select an image in the left pane"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show (notebook);

  popup->priv->layer_view =
    gimp_container_tree_view_new (NULL,
                                  popup->priv->context,
                                  popup->priv->view_size,
                                  popup->priv->view_border_width);
  gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (GIMP_CONTAINER_TREE_VIEW (popup->priv->layer_view)->view),
                                    TRUE);
  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (popup->priv->layer_view),
                                       4 * (popup->priv->view_size +
                                            2 * popup->priv->view_border_width),
                                       4 * (popup->priv->view_size +
                                            2 * popup->priv->view_border_width));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            popup->priv->layer_view,
                            gtk_label_new (_("Layers")));
  gtk_widget_show (popup->priv->layer_view);

  g_signal_connect_object (popup->priv->layer_view, "activate-item",
                           G_CALLBACK (gimp_pickable_popup_item_activate),
                           G_OBJECT (popup), 0);

  popup->priv->channel_view =
    gimp_container_tree_view_new (NULL,
                                  popup->priv->context,
                                  popup->priv->view_size,
                                  popup->priv->view_border_width);
  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (popup->priv->channel_view),
                                       4 * (popup->priv->view_size +
                                            2 * popup->priv->view_border_width),
                                       4 * (popup->priv->view_size +
                                            2 * popup->priv->view_border_width));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            popup->priv->channel_view,
                            gtk_label_new (_("Channels")));
  gtk_widget_show (popup->priv->channel_view);

  g_signal_connect_object (popup->priv->channel_view, "activate-item",
                           G_CALLBACK (gimp_pickable_popup_item_activate),
                           G_OBJECT (popup), 0);

  g_signal_connect_object (popup->priv->context, "image-changed",
                           G_CALLBACK (gimp_pickable_popup_image_changed),
                           G_OBJECT (popup), 0);

  image = gimp_context_get_image (popup->priv->context);
  gimp_pickable_popup_image_changed (popup->priv->context, image, popup);
}

static void
gimp_pickable_popup_finalize (GObject *object)
{
  GimpPickablePopup *popup = GIMP_PICKABLE_POPUP (object);

  g_clear_object (&popup->priv->pickable);
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

    case PROP_VIEW_SIZE:
      popup->priv->view_size = g_value_get_int (value);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      popup->priv->view_border_width = g_value_get_int (value);
      break;

    case PROP_PICKABLE:
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
      g_value_set_object (value, popup->priv->pickable);
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
  GtkWidget    *focus;
  GimpPickable *pickable = NULL;

  g_return_val_if_fail (GIMP_IS_PICKABLE_POPUP (popup), NULL);

  focus = gtk_window_get_focus (GTK_WINDOW (popup));

  if (focus && gtk_widget_is_ancestor (focus, popup->priv->image_view))
    {
      pickable = GIMP_PICKABLE (gimp_context_get_image (popup->priv->context));
    }
  else if (focus && gtk_widget_is_ancestor (focus, popup->priv->layer_view))
    {
      GList *selected;

      if (gimp_container_view_get_selected (GIMP_CONTAINER_VIEW (popup->priv->layer_view),
                                            &selected))
        {
          pickable = selected->data;
          g_list_free (selected);
        }
    }
  else if (focus && gtk_widget_is_ancestor (focus, popup->priv->channel_view))
    {
      GList *selected;

      if (gimp_container_view_get_selected (GIMP_CONTAINER_VIEW (popup->priv->channel_view),
                                            &selected))
        {
          pickable = selected->data;
          g_list_free (selected);
        }
    }

  return pickable;
}


/*  private functions  */

static void
gimp_pickable_popup_image_changed (GimpContext       *context,
                                   GimpImage         *image,
                                   GimpPickablePopup *popup)
{
  GimpContainer *layers   = NULL;
  GimpContainer *channels = NULL;

  if (image)
    {
      gchar *desc;

      layers   = gimp_image_get_layers (image);
      channels = gimp_image_get_channels (image);

      desc = gimp_viewable_get_description (GIMP_VIEWABLE (image), NULL);
      gtk_label_set_text (GTK_LABEL (popup->priv->layer_label), desc);
      g_free (desc);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (popup->priv->layer_label),
                          _("Select an image in the left pane"));
    }

  gimp_container_view_set_container (GIMP_CONTAINER_VIEW (popup->priv->layer_view),
                                     layers);
  gimp_container_view_set_container (GIMP_CONTAINER_VIEW (popup->priv->channel_view),
                                     channels);
}

static void
gimp_pickable_popup_item_activate (GimpContainerView *view,
                                   GimpPickable      *pickable,
                                   gpointer           unused,
                                   GimpPickablePopup *popup)
{
  g_signal_emit_by_name (popup, "confirm");
}
