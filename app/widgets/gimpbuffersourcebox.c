/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbuffersourcebox.c
 * Copyright (C) 2015 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimppickable.h"

#include "gimpbuffersourcebox.h"
#include "gimppickablebutton.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_SOURCE_NODE,
  PROP_LABEL,
  PROP_PICKABLE,
  PROP_ENABLED
};

struct _GimpBufferSourceBoxPrivate
{
  GimpContext  *context;
  GeglNode     *source_node;
  gchar        *label;
  GimpPickable *pickable;
  gboolean      enabled;

  GtkWidget    *button;
  GtkWidget    *toggle;
};


static void   gimp_buffer_source_box_constructed     (GObject             *object);
static void   gimp_buffer_source_box_finalize        (GObject             *object);
static void   gimp_buffer_source_box_set_property    (GObject             *object,
                                                      guint                property_id,
                                                      const GValue        *value,
                                                      GParamSpec          *pspec);
static void   gimp_buffer_source_box_get_property    (GObject             *object,
                                                      guint                property_id,
                                                      GValue              *value,
                                                      GParamSpec          *pspec);

static void   gimp_buffer_source_box_notify_pickable (GimpPickableButton  *button,
                                                      const GParamSpec    *pspec,
                                                      GimpBufferSourceBox *box);
static void   gimp_buffer_source_box_enable_toggled  (GtkToggleButton     *button,
                                                      GimpBufferSourceBox *box);


G_DEFINE_TYPE (GimpBufferSourceBox, gimp_buffer_source_box,
               GTK_TYPE_BOX)

#define parent_class gimp_buffer_source_box_parent_class


static void
gimp_buffer_source_box_class_init (GimpBufferSourceBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_buffer_source_box_constructed;
  object_class->finalize     = gimp_buffer_source_box_finalize;
  object_class->set_property = gimp_buffer_source_box_set_property;
  object_class->get_property = gimp_buffer_source_box_get_property;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SOURCE_NODE,
                                   g_param_spec_object ("source-node", NULL, NULL,
                                                        GEGL_TYPE_NODE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_LABEL,
                                   g_param_spec_string ("label", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PICKABLE,
                                   g_param_spec_object ("pickable", NULL, NULL,
                                                        GIMP_TYPE_PICKABLE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ENABLED,
                                   g_param_spec_boolean ("enabled", NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_type_class_add_private (klass, sizeof (GimpBufferSourceBoxPrivate));
}

static void
gimp_buffer_source_box_init (GimpBufferSourceBox *box)
{
  box->priv = G_TYPE_INSTANCE_GET_PRIVATE (box,
                                           GIMP_TYPE_BUFFER_SOURCE_BOX,
                                           GimpBufferSourceBoxPrivate);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_HORIZONTAL);
}

static void
gimp_buffer_source_box_constructed (GObject *object)
{
  GimpBufferSourceBox *box = GIMP_BUFFER_SOURCE_BOX (object);

  box->priv->toggle = gtk_check_button_new_with_mnemonic (box->priv->label);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (box->priv->toggle),
                                box->priv->enabled);
  gtk_box_pack_start (GTK_BOX (box), box->priv->toggle, FALSE, FALSE, 0);
  gtk_widget_show (box->priv->toggle);

  g_signal_connect_object (box->priv->toggle, "toggled",
                           G_CALLBACK (gimp_buffer_source_box_enable_toggled),
                           box, 0);

  box->priv->button = gimp_pickable_button_new (box->priv->context,
                                                GIMP_VIEW_SIZE_LARGE, 1);
  gimp_pickable_button_set_pickable (GIMP_PICKABLE_BUTTON (box->priv->button),
                                     box->priv->pickable);
  gtk_box_pack_start (GTK_BOX (box), box->priv->button, FALSE, FALSE, 0);
  gtk_widget_show (box->priv->button);

  g_signal_connect_object (box->priv->button, "notify::pickable",
                           G_CALLBACK (gimp_buffer_source_box_notify_pickable),
                           box, 0);

  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
gimp_buffer_source_box_finalize (GObject *object)
{
  GimpBufferSourceBox *box = GIMP_BUFFER_SOURCE_BOX (object);

  if (box->priv->context)
    {
      g_object_unref (box->priv->context);
      box->priv->context = NULL;
    }

  if (box->priv->source_node)
    {
      g_object_unref (box->priv->source_node);
      box->priv->source_node = NULL;
    }

  if (box->priv->label)
    {
      g_free (box->priv->label);
      box->priv->label = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_buffer_source_box_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpBufferSourceBox *box = GIMP_BUFFER_SOURCE_BOX (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      box->priv->context = g_value_dup_object (value);
      break;

    case PROP_SOURCE_NODE:
      box->priv->source_node = g_value_dup_object (value);
      break;

    case PROP_LABEL:
      box->priv->label = g_value_dup_string (value);
      break;

    case PROP_PICKABLE:
      box->priv->pickable = g_value_get_object (value);
      if (box->priv->button)
        gimp_pickable_button_set_pickable (GIMP_PICKABLE_BUTTON (box->priv->button),
                                           box->priv->pickable);
      break;

    case PROP_ENABLED:
      box->priv->enabled = g_value_get_boolean (value);
      if (box->priv->toggle)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (box->priv->toggle),
                                      box->priv->enabled);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_buffer_source_box_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpBufferSourceBox *box = GIMP_BUFFER_SOURCE_BOX (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, box->priv->context);
      break;

    case PROP_SOURCE_NODE:
      g_value_set_object (value, box->priv->source_node);
      break;

    case PROP_LABEL:
      g_value_set_string (value, box->priv->label);
      break;

    case PROP_PICKABLE:
      g_value_set_object (value, box->priv->pickable);
      break;

    case PROP_ENABLED:
      g_value_set_boolean (value, box->priv->enabled);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_buffer_source_box_update_node (GimpBufferSourceBox *box)
{
  GeglBuffer *buffer = NULL;

  if (box->priv->pickable && box->priv->enabled)
    {
      buffer = gimp_pickable_get_buffer (box->priv->pickable);
    }

  gegl_node_set (box->priv->source_node,
                 "buffer", buffer,
                 NULL);
}

static void
gimp_buffer_source_box_notify_pickable (GimpPickableButton  *button,
                                        const GParamSpec    *pspec,
                                        GimpBufferSourceBox *box)
{
  box->priv->pickable = gimp_pickable_button_get_pickable (button);

  gimp_buffer_source_box_update_node (box);

  g_object_notify (G_OBJECT (box), "pickable");
}

static void
gimp_buffer_source_box_enable_toggled (GtkToggleButton     *button,
                                       GimpBufferSourceBox *box)
{
  box->priv->enabled = gtk_toggle_button_get_active (button);

  gimp_buffer_source_box_update_node (box);

  g_object_notify (G_OBJECT (box), "enabled");
}


/*  public functions  */

GtkWidget *
gimp_buffer_source_box_new (GimpContext *context,
                            GeglNode    *source_node,
                            const gchar *label)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (source_node), NULL);
  g_return_val_if_fail (label != NULL, NULL);

  return g_object_new (GIMP_TYPE_BUFFER_SOURCE_BOX,
                       "context",     context,
                       "source-node", source_node,
                       "label",       label,
                       NULL);
}
