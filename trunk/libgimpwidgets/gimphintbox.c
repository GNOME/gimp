/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimphintbox.c
 * Copyright (C) 2006 Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpwidgets.h"


typedef GtkHBoxClass  GimpHintBoxClass;

typedef struct
{
  GtkHBox    parent_instance;

  gchar     *stock_id;
  gchar     *hint;
} GimpHintBox;

#define GIMP_HINT_BOX(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HINT_BOX, GimpHintBox))


enum
{
  PROP_0,
  PROP_STOCK_ID,
  PROP_HINT
};

static GObject * gimp_hint_box_constructor  (GType                  type,
                                             guint                  n_params,
                                             GObjectConstructParam *params);
static void      gimp_hint_box_finalize     (GObject               *object);
static void      gimp_hint_box_set_property (GObject               *object,
                                             guint                  property_id,
                                             const GValue          *value,
                                             GParamSpec            *pspec);
static void      gimp_hint_box_get_property (GObject               *object,
                                             guint                  property_id,
                                             GValue                *value,
                                             GParamSpec            *pspec);

G_DEFINE_TYPE (GimpHintBox, gimp_hint_box, GTK_TYPE_HBOX)

#define parent_class gimp_hint_box_parent_class


static void
gimp_hint_box_class_init (GimpHintBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor   = gimp_hint_box_constructor;
  object_class->finalize      = gimp_hint_box_finalize;
  object_class->set_property  = gimp_hint_box_set_property;
  object_class->get_property  = gimp_hint_box_get_property;

  g_object_class_install_property (object_class, PROP_STOCK_ID,
                                   g_param_spec_string ("stock-id", NULL, NULL,
                                                        GIMP_STOCK_INFO,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_HINT,
                                   g_param_spec_string ("hint", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_hint_box_init (GimpHintBox *box)
{
  box->stock_id = NULL;
  box->hint     = NULL;
}

static GObject *
gimp_hint_box_constructor (GType                  type,
                           guint                  n_params,
                           GObjectConstructParam *params)
{
  GObject     *object;
  GimpHintBox *box;
  GtkWidget   *label;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  box = GIMP_HINT_BOX (object);

  gtk_box_set_spacing (GTK_BOX (box), 12);

  if (box->stock_id)
    {
      GtkWidget *image = gtk_image_new_from_stock (box->stock_id,
                                                   GTK_ICON_SIZE_DIALOG);

      gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
      gtk_widget_show (image);
    }

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   box->hint,
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.5,
                        NULL);

  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  return object;
}

static void
gimp_hint_box_finalize (GObject *object)
{
  GimpHintBox *box = GIMP_HINT_BOX (object);

  if (box->stock_id)
    {
      g_free (box->stock_id);
      box->stock_id = NULL;
    }

  if (box->hint)
    {
      g_free (box->hint);
      box->hint = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_hint_box_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpHintBox *box = GIMP_HINT_BOX (object);

  switch (property_id)
    {
    case PROP_STOCK_ID:
      box->stock_id = g_value_dup_string (value);
      break;

    case PROP_HINT:
      box->hint = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_hint_box_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpHintBox *box = GIMP_HINT_BOX (object);

  switch (property_id)
    {
    case PROP_STOCK_ID:
      g_value_set_string (value, box->stock_id);
      break;

    case PROP_HINT:
      g_value_set_string (value, box->hint);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_hint_box_new:
 * @hint: text to display as a user hint
 *
 * Creates a new widget that shows a text label showing @hint,
 * decorated with a GIMP_STOCK_INFO wilber icon.
 *
 * Return value: a new widget
 *
 * Since GIMP 2.4
 **/
GtkWidget *
gimp_hint_box_new (const gchar *hint)
{
  g_return_val_if_fail (hint != NULL, NULL);

  return g_object_new (GIMP_TYPE_HINT_BOX,
                       "hint", hint,
                       NULL);
}
