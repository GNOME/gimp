/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprowimagecomponent.c
 * Copyright (C) 2026 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpimage.h"

#include "gimpcomponenteditor.h"
#include "gimpdnd.h"
#include "gimpeditor.h"
#include "gimprowimagecomponent.h"
#include "gimpview.h"
#include "gimpviewrendererimage.h"


enum
{
  PROP_0,
  PROP_CHANNEL,
  N_PROPS
};

static GParamSpec *obj_props[N_PROPS] = { NULL, };


typedef struct _GimpRowImageComponentPrivate GimpRowImageComponentPrivate;

struct _GimpRowImageComponentPrivate
{
  GimpChannelType  channel;

  GtkWidget       *visible_toggle;
  GtkWidget       *visible_icon;
};

#define GET_PRIVATE(obj) \
  ((GimpRowImageComponentPrivate *) \
   gimp_row_image_component_get_instance_private ((GimpRowImageComponent *) obj))


static void   gimp_row_image_component_constructed  (GObject         *object);
static void   gimp_row_image_component_set_property (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void   gimp_row_image_component_get_property (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);

static gboolean gimp_row_image_component_button_press_event
                                                    (GtkWidget       *widget,
                                                     GdkEventButton  *bevent);

static void   gimp_row_image_component_set_viewable (GimpRow         *row,
                                                     GimpViewable    *viewable);
static void   gimp_row_image_component_name_changed (GimpRow         *row);

static void   gimp_row_component_visibility_changed (GimpImage       *image,
                                                     GimpChannelType  channel,
                                                     GimpRow         *row);
static void   gimp_row_component_visibility_toggled (GtkToggleButton *button,
                                                     GimpRow         *row);
static GimpImage * gimp_row_image_component_drag_component
                                                    (GtkWidget       *widget,
                                                     GimpContext    **context,
                                                     GimpChannelType *channel,
                                                     gpointer         data);


G_DEFINE_TYPE_WITH_PRIVATE (GimpRowImageComponent,
                            gimp_row_image_component,
                            GIMP_TYPE_ROW_IMAGE)

#define parent_class gimp_row_image_component_parent_class


static void
gimp_row_image_component_class_init (GimpRowImageComponentClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GimpRowClass   *row_class    = GIMP_ROW_CLASS (klass);

  object_class->constructed        = gimp_row_image_component_constructed;
  object_class->set_property       = gimp_row_image_component_set_property;
  object_class->get_property       = gimp_row_image_component_get_property;

  widget_class->button_press_event = gimp_row_image_component_button_press_event;

  row_class->set_viewable          = gimp_row_image_component_set_viewable;
  row_class->name_changed          = gimp_row_image_component_name_changed;

  obj_props[PROP_CHANNEL] =
    g_param_spec_enum ("channel",
                       NULL, NULL,
                       GIMP_TYPE_CHANNEL_TYPE,
                       GIMP_CHANNEL_RED,
                       GIMP_PARAM_READWRITE |
                       G_PARAM_CONSTRUCT);

  g_object_class_install_properties (object_class, N_PROPS, obj_props);
}

static void
gimp_row_image_component_init (GimpRowImageComponent *row)
{
  GimpRowImageComponentPrivate *priv = GET_PRIVATE (row);

  priv->channel = -1; /* so the initial set_channel() always sets the label */

  priv->visible_toggle = _gimp_row_add_toggle (GIMP_ROW (row),
                                               GIMP_ICON_VISIBLE,
                                               &priv->visible_icon);

  g_signal_connect (priv->visible_toggle, "toggled",
                    G_CALLBACK (gimp_row_component_visibility_toggled),
                    row);
}

static void
gimp_row_image_component_constructed (GObject *object)
{
  GimpRowImageComponentPrivate *priv = GET_PRIVATE (object);
  GtkWidget                    *view;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  view = _gimp_row_get_view (GIMP_ROW (object));
  if (view)
    {
      GIMP_VIEW_RENDERER_IMAGE (GIMP_VIEW (view)->renderer)->channel =
        priv->channel;
    }
}

static void
gimp_row_image_component_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpRowImageComponent *row = GIMP_ROW_IMAGE_COMPONENT (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      gimp_row_image_component_set_channel (row, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_row_image_component_get_property (GObject      *object,
                                       guint         property_id,
                                       GValue       *value,
                                       GParamSpec   *pspec)
{
  GimpRowImageComponent *row = GIMP_ROW_IMAGE_COMPONENT (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      g_value_set_enum (value, gimp_row_image_component_get_channel (row));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_row_image_component_button_press_event (GtkWidget      *widget,
                                             GdkEventButton *bevent)
{
  GimpRowImageComponentPrivate *priv   = GET_PRIVATE (widget);
  GdkEvent                     *event  = (GdkEvent *) bevent;
  GtkWidget                    *parent;
  GtkWidget                    *editor;

  parent = gtk_widget_get_parent (widget);
  editor = gtk_widget_get_ancestor (parent, GIMP_TYPE_COMPONENT_EDITOR);
  if (! editor)
    return FALSE;

  GIMP_COMPONENT_EDITOR (editor)->clicked_component = priv->channel;

  if (gdk_event_triggers_context_menu (event))
    {
      return gimp_editor_popup_menu_at_pointer (GIMP_EDITOR (editor),
                                                event);
    }
  else if (bevent->button == 1)
    {
      if (gtk_list_box_row_is_selected (GTK_LIST_BOX_ROW (widget)))
        {
          gtk_list_box_unselect_row (GTK_LIST_BOX (parent),
                                     GTK_LIST_BOX_ROW (widget));
        }
      else
        {
          gtk_list_box_select_row (GTK_LIST_BOX (parent),
                                   GTK_LIST_BOX_ROW (widget));
        }
    }

  return TRUE;
}


static void
gimp_row_image_component_set_viewable (GimpRow      *row,
                                       GimpViewable *viewable)
{
  GimpRowImageComponentPrivate *priv         = GET_PRIVATE (row);
  GimpViewable                 *old_viewable = gimp_row_get_viewable (row);

  if (old_viewable)
    {
      g_signal_handlers_disconnect_by_func (old_viewable,
                                            gimp_row_component_visibility_changed,
                                            row);

      gimp_dnd_component_source_remove (GTK_WIDGET (row));
    }


  GIMP_ROW_CLASS (parent_class)->set_viewable (row, viewable);

  if (viewable)
    {
      /*  remove the generic sources added by GimpRow  */
      if (gimp_dnd_viewable_source_remove (GTK_WIDGET (row), GIMP_TYPE_IMAGE))
        {
          gimp_dnd_pixbuf_source_remove (GTK_WIDGET (row));
          gtk_drag_source_unset (GTK_WIDGET (row));
        }

      gimp_dnd_component_source_add (GTK_WIDGET (row),
                                     gimp_row_image_component_drag_component,
                                     NULL);

      g_signal_connect (viewable, "component-visibility-changed",
                        G_CALLBACK (gimp_row_component_visibility_changed),
                        row);

      gimp_row_component_visibility_changed (GIMP_IMAGE (viewable),
                                             priv->channel,
                                             row);
    }
}

static void
gimp_row_image_component_name_changed (GimpRow *row)
{
  /*  do nothing and don't chain up, we set our own label  */
}


/*  public functions  */

void
gimp_row_image_component_set_channel (GimpRowImageComponent *row,
                                      GimpChannelType        channel)
{
  GimpRowImageComponentPrivate *priv = GET_PRIVATE (row);

  g_return_if_fail (GIMP_IS_ROW_IMAGE_COMPONENT (row));

  if (channel != priv->channel)
    {
      GtkWidget   *view;
      GtkWidget   *label;
      const gchar *desc;

      priv->channel = channel;

      view = _gimp_row_get_view (GIMP_ROW (row));
      if (view)
        {
          GIMP_VIEW_RENDERER_IMAGE (GIMP_VIEW (view)->renderer)->channel =
            priv->channel;
        }

      gimp_enum_get_value (GIMP_TYPE_CHANNEL_TYPE, channel,
                           NULL, NULL, &desc, NULL);
      label = _gimp_row_get_label (GIMP_ROW (row));
      gtk_label_set_text (GTK_LABEL (label), desc);

      g_object_notify_by_pspec (G_OBJECT (row), obj_props[PROP_CHANNEL]);

      gimp_row_component_visibility_changed (GIMP_IMAGE (gimp_row_get_viewable (GIMP_ROW (row))),
                                             priv->channel,
                                             GIMP_ROW (row));
    }
}

GimpChannelType
gimp_row_image_component_get_channel (GimpRowImageComponent *row)
{
  GimpRowImageComponentPrivate *priv = GET_PRIVATE (row);

  g_return_val_if_fail (GIMP_IS_ROW_IMAGE_COMPONENT (row), GIMP_CHANNEL_RED);

  return priv->channel;
}


/*  private functions  */

static void
gimp_row_component_visibility_changed (GimpImage       *image,
                                       GimpChannelType  channel,
                                       GimpRow         *row)
{
  GimpRowImageComponentPrivate *priv = GET_PRIVATE (row);

  if (channel == priv->channel)
    {
      gboolean visible;

      g_signal_handlers_block_by_func (priv->visible_toggle,
                                       gimp_row_component_visibility_toggled,
                                       row);

      visible = gimp_image_get_component_visible (image, channel);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->visible_toggle),
                                    visible);
      gtk_widget_set_opacity (priv->visible_icon, visible ? 1.0 : 0.0);

      g_signal_handlers_unblock_by_func (priv->visible_toggle,
                                         gimp_row_component_visibility_toggled,
                                         row);
    }
}

static void
gimp_row_component_visibility_toggled (GtkToggleButton *button,
                                       GimpRow         *row)
{
  GimpRowImageComponentPrivate *priv = GET_PRIVATE (row);
  GimpImage                    *image;
  gboolean                      active;

  image  = GIMP_IMAGE (gimp_row_get_viewable (row));
  active = gtk_toggle_button_get_active (button);

  gimp_image_set_component_visible (image, priv->channel, active);
  gimp_image_flush (image);
}

static GimpImage *
gimp_row_image_component_drag_component (GtkWidget        *widget,
                                         GimpContext     **context,
                                         GimpChannelType  *channel,
                                         gpointer          data)
{
  GimpRowImageComponentPrivate *priv = GET_PRIVATE (widget);
  GimpViewable                 *viewable;

  viewable = gimp_row_get_viewable (GIMP_ROW (widget));

  if (viewable)
    {
      if (channel)
        *channel = priv->channel;

      if (context)
        *context = gimp_row_get_context (GIMP_ROW (widget));

      return GIMP_IMAGE (viewable);
    }

  return NULL;
}
