/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimphintbox.c
 * Copyright (C) 2006 Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "gimpwidgets.h"


/**
 * SECTION: gimphintbox
 * @title: GimpHintBox
 * @short_description: Displays a wilber icon and a text.
 *
 * Displays a wilber icon and a text.
 **/


typedef GtkBoxClass  GimpHintBoxClass;

typedef struct
{
  GtkBox    parent_instance;

  gchar    *icon_name;
  gchar    *hint;
} GimpHintBox;

#define GIMP_HINT_BOX(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HINT_BOX, GimpHintBox))


enum
{
  PROP_0,
  PROP_ICON_NAME,
  PROP_HINT
};


static void   gimp_hint_box_constructed  (GObject      *object);
static void   gimp_hint_box_finalize     (GObject      *object);
static void   gimp_hint_box_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void   gimp_hint_box_get_property (GObject      *object,
                                          guint         property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);


G_DEFINE_TYPE (GimpHintBox, gimp_hint_box, GTK_TYPE_BOX)

#define parent_class gimp_hint_box_parent_class


static void
gimp_hint_box_class_init (GimpHintBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed   = gimp_hint_box_constructed;
  object_class->finalize      = gimp_hint_box_finalize;
  object_class->set_property  = gimp_hint_box_set_property;
  object_class->get_property  = gimp_hint_box_get_property;

  g_object_class_install_property (object_class, PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name",
                                                        "Icon Name",
                                                        "The icon to show next to the hint",
                                                        GIMP_ICON_DIALOG_INFORMATION,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HINT,
                                   g_param_spec_string ("hint",
                                                        "Hint",
                                                        "The hint to display",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_hint_box_init (GimpHintBox *box)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_HORIZONTAL);
}

static void
gimp_hint_box_constructed (GObject *object)
{
  GimpHintBox *box   = GIMP_HINT_BOX (object);
  GtkWidget   *image = NULL;
  GtkWidget   *label;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_box_set_spacing (GTK_BOX (box), 12);

  if (box->icon_name)
    {
      image = gtk_image_new_from_icon_name (box->icon_name,
                                            GTK_ICON_SIZE_DIALOG);
    }

  if (image)
    {
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
}

static void
gimp_hint_box_finalize (GObject *object)
{
  GimpHintBox *box = GIMP_HINT_BOX (object);

  if (box->icon_name)
    {
      g_free (box->icon_name);
      box->icon_name = NULL;
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
    case PROP_ICON_NAME:
      box->icon_name = g_value_dup_string (value);
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
    case PROP_ICON_NAME:
      g_value_set_string (value, box->icon_name);
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
 * decorated with a GIMP_ICON_INFO wilber icon.
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
