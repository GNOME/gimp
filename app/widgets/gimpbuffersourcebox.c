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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimpcontext.h"
#include "core/gimppickable.h"

#include "gimpbuffersourcebox.h"
#include "gimppickablebutton.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_SOURCE_NODE,
  PROP_NAME,
  PROP_PICKABLE,
  PROP_ENABLED
};

struct _GimpBufferSourceBoxPrivate
{
  GimpContext  *context;
  GeglNode     *source_node;
  gchar        *name;
  GimpPickable *pickable;
  gboolean      enabled;

  GtkWidget    *toggle;
  GtkWidget    *button;
  GtkWidget    *label;
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


G_DEFINE_TYPE_WITH_PRIVATE (GimpBufferSourceBox, gimp_buffer_source_box,
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

  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name", NULL, NULL,
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
}

static void
gimp_buffer_source_box_init (GimpBufferSourceBox *box)
{
  box->priv = gimp_buffer_source_box_get_instance_private (box);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (box), 2);
}

static void
gimp_buffer_source_box_constructed (GObject *object)
{
  GimpBufferSourceBox *box = GIMP_BUFFER_SOURCE_BOX (object);

  box->priv->toggle = gtk_check_button_new_with_mnemonic (box->priv->name);
  gtk_widget_set_valign (box->priv->toggle, GTK_ALIGN_CENTER);
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

  box->priv->label = gtk_label_new (_("(none)"));
  gtk_label_set_xalign (GTK_LABEL (box->priv->label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (box->priv->label), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (box), box->priv->label, TRUE, TRUE, 0);
  gtk_widget_show (box->priv->label);

  g_signal_connect_object (box->priv->button, "notify::pickable",
                           G_CALLBACK (gimp_buffer_source_box_notify_pickable),
                           box, 0);

  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
gimp_buffer_source_box_finalize (GObject *object)
{
  GimpBufferSourceBox *box = GIMP_BUFFER_SOURCE_BOX (object);

  g_clear_object (&box->priv->context);
  g_clear_object (&box->priv->source_node);
  g_clear_pointer (&box->priv->name, g_free);

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

    case PROP_NAME:
      box->priv->name = g_value_dup_string (value);
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

    case PROP_NAME:
      g_value_set_string (value, box->priv->name);
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

  if (box->priv->pickable)
    {
      gchar *desc;

      if (box->priv->enabled)
        {
          gimp_pickable_flush (box->priv->pickable);

          /* dup the buffer, since the original may be modified while applying
           * the operation.  see issue #1283.
           */
          buffer = gimp_gegl_buffer_dup (
            gimp_pickable_get_buffer (box->priv->pickable));
        }

      desc = gimp_viewable_get_description (GIMP_VIEWABLE (box->priv->pickable),
                                            NULL);
      gtk_label_set_text (GTK_LABEL (box->priv->label), desc);
      g_free (desc);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (box->priv->label), _("(none)"));
    }

  gegl_node_set (box->priv->source_node,
                 "buffer", buffer,
                 NULL);

  g_clear_object (&buffer);
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
                            const gchar *name)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (source_node), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (GIMP_TYPE_BUFFER_SOURCE_BOX,
                       "context",     context,
                       "source-node", source_node,
                       "name",        name,
                       NULL);
}

GtkWidget *
gimp_buffer_source_box_get_toggle (GimpBufferSourceBox *box)
{
  g_return_val_if_fail (GIMP_IS_BUFFER_SOURCE_BOX (box), NULL);

  return box->priv->toggle;
}
