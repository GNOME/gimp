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


static void   gimp_enum_label_class_init (GimpEnumLabelClass *klass);
static void   gimp_enum_label_finalize   (GObject            *object);


static GtkLabelClass *parent_class = NULL;


GType
gimp_enum_label_get_type (void)
{
  static GType enum_label_type = 0;

  if (!enum_label_type)
    {
      static const GTypeInfo enum_label_info =
      {
        sizeof (GimpEnumLabelClass),
        NULL,           /* base_init      */
        NULL,           /* base_finalize  */
        (GClassInitFunc) gimp_enum_label_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpEnumLabel),
        0,              /* n_preallocs    */
        NULL            /* instance_init  */
      };

      enum_label_type = g_type_register_static (GTK_TYPE_LABEL,
                                                "GimpEnumLabel",
                                                &enum_label_info, 0);
    }

  return enum_label_type;
}

static void
gimp_enum_label_class_init (GimpEnumLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_enum_label_finalize;
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
