/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "core-types.h"

#include "gimpobject.h"


enum
{
  NAME_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_NAME
};


static void   gimp_object_class_init   (GimpObjectClass *klass);
static void   gimp_object_init         (GimpObject      *object);

static void   gimp_object_finalize     (GObject         *object);
static void   gimp_object_set_property (GObject         *object,
					guint            property_id,
					const GValue    *value,
					GParamSpec      *pspec);
static void   gimp_object_get_property (GObject         *object,
					guint            property_id,
					GValue          *value,
					GParamSpec      *pspec);


static guint   object_signals[LAST_SIGNAL] = { 0 };

static GtkObjectClass *parent_class = NULL;


GType 
gimp_object_get_type (void)
{
  static GType object_type = 0;

  if (! object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (GimpObjectClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_object_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpObject),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_object_init,
      };

      object_type = g_type_register_static (GTK_TYPE_OBJECT,
					    "GimpObject", 
					    &object_info, 0);
    }

  return object_type;
}

static void
gimp_object_class_init (GimpObjectClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_signals[NAME_CHANGED] =
    g_signal_new ("name_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpObjectClass, name_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize     = gimp_object_finalize;
  object_class->set_property = gimp_object_set_property;
  object_class->get_property = gimp_object_get_property;

  klass->name_changed        = NULL;

  g_object_class_install_property (object_class,
				   PROP_NAME,
				   g_param_spec_string ("name",
							NULL, NULL,
							NULL,
							G_PARAM_READWRITE));
}

static void
gimp_object_init (GimpObject *object)
{
  object->name = NULL;
}

static void
gimp_object_finalize (GObject *object)
{
  GimpObject *gimp_object;

  gimp_object = GIMP_OBJECT (object);

  if (gimp_object->name)
    {
      g_free (gimp_object->name);
      gimp_object->name = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_object_set_property (GObject      *object,
			  guint         property_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  GimpObject *gimp_object;

  gimp_object = GIMP_OBJECT (object);

  switch (property_id)
    {
    case PROP_NAME:
      gimp_object_set_name (gimp_object, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_object_get_property (GObject    *object,
			  guint       property_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
  GimpObject *gimp_object;

  gimp_object = GIMP_OBJECT (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, gimp_object->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_object_set_name (GimpObject  *object,
		      const gchar *name)
{
  g_return_if_fail (GIMP_IS_OBJECT (object));

  if ((!object->name && !name) ||
      (object->name && name && !strcmp (object->name, name)))
    return;

  g_free (object->name);

  object->name = g_strdup (name);

  gimp_object_name_changed (object);
}

const gchar *
gimp_object_get_name (const GimpObject  *object)
{
  g_return_val_if_fail (GIMP_IS_OBJECT (object), NULL);

  return object->name;
}

void
gimp_object_name_changed (GimpObject  *object)
{
  g_return_if_fail (GIMP_IS_OBJECT (object));

  g_signal_emit (G_OBJECT (object), object_signals[NAME_CHANGED], 0);
}
