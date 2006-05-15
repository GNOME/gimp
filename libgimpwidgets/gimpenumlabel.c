/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenumlabel.c
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpenumlabel.h"


static void   gimp_enum_label_finalize (GObject *object);


G_DEFINE_TYPE (GimpEnumLabel, gimp_enum_label, GTK_TYPE_LABEL)

#define parent_class gimp_enum_label_parent_class


static void
gimp_enum_label_class_init (GimpEnumLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_enum_label_finalize;
}

static void
gimp_enum_label_init (GimpEnumLabel *enum_label)
{
}

static void
gimp_enum_label_finalize (GObject *object)
{
  GimpEnumLabel *enum_label = GIMP_ENUM_LABEL (object);

  if (enum_label->enum_class)
    g_type_class_unref (enum_label->enum_class);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gimp_enum_label_new:
 * @enum_type: the #GType of an enum.
 * @value:
 *
 * Return value: a new #GimpEnumLabel.
 *
 * Since: GIMP 2.4
 **/
GtkWidget *
gimp_enum_label_new (GType enum_type,
                     gint  value)
{
  GimpEnumLabel *label;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  label = g_object_new (GIMP_TYPE_ENUM_LABEL, NULL);

  label->enum_class = g_type_class_ref (enum_type);

  gimp_enum_label_set_value (label, value);

  return GTK_WIDGET (label);
}

/**
 * gimp_enum_label_set_value
 * @label: a #GimpEnumLabel
 * @value:
 *
 * Since: GIMP 2.4
 **/
void
gimp_enum_label_set_value (GimpEnumLabel *label,
                           gint           value)
{
  const gchar *desc;

  g_return_if_fail (GIMP_IS_ENUM_LABEL (label));

  if (! gimp_enum_get_value (G_TYPE_FROM_CLASS (label->enum_class), value,
                             NULL, NULL, &desc, NULL))
    {
      g_warning ("%s: %d is not valid for enum of type '%s'",
                 G_STRLOC, value,
                 g_type_name (G_TYPE_FROM_CLASS (label->enum_class)));
      return;
    }

  gtk_label_set_text (GTK_LABEL (label), desc);
}
