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

#include <gtk/gtk.h>

#include "apptypes.h"

#include "gimpobject.h"


enum
{
  NAME_CHANGED,
  LAST_SIGNAL
};

enum
{
  ARG_0,
  ARG_NAME
};


static void   gimp_object_class_init (GimpObjectClass *klass);
static void   gimp_object_init       (GimpObject      *object);
static void   gimp_object_set_arg    (GtkObject       *object,
				      GtkArg          *arg,
				      guint            arg_id);
static void   gimp_object_get_arg    (GtkObject       *object,
				      GtkArg          *arg,
				      guint            arg_id);


static guint           object_signals[LAST_SIGNAL] = { 0 };

static GtkObjectClass *parent_class = NULL;


GtkType 
gimp_object_get_type (void)
{
  static GtkType object_type = 0;

  if (! object_type)
    {
      GtkTypeInfo object_info =
      {
        "GimpObject",
        sizeof (GimpObject),
        sizeof (GimpObjectClass),
        (GtkClassInitFunc) gimp_object_class_init,
        (GtkObjectInitFunc) gimp_object_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      object_type = gtk_type_unique (GTK_TYPE_OBJECT, &object_info);
    }

  return object_type;
}

static void
gimp_object_class_init (GimpObjectClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GTK_TYPE_OBJECT);

  gtk_object_add_arg_type ("GimpObject::name", GTK_TYPE_STRING,
			   GTK_ARG_READWRITE, ARG_NAME);

  object_signals[NAME_CHANGED] =
    gtk_signal_new ("name_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpObjectClass,
				       name_changed),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, object_signals, LAST_SIGNAL);

  object_class->set_arg = gimp_object_set_arg;
  object_class->get_arg = gimp_object_get_arg;

  klass->name_changed = NULL;
}

static void
gimp_object_init (GimpObject *object)
{
  object->name = NULL;
}

static void
gimp_object_set_arg (GtkObject *object,
		     GtkArg    *arg,
		     guint      arg_id)
{
  GimpObject *gimp_object;

  gimp_object = GIMP_OBJECT (object);

  switch (arg_id)
    {
    case ARG_NAME:
      gimp_object_set_name (gimp_object, GTK_VALUE_STRING (*arg));
      break;
    default:
      break;
    }
}

static void
gimp_object_get_arg (GtkObject *object,
		     GtkArg    *arg,
		     guint      arg_id)
{
  GimpObject *gimp_object;

  gimp_object = GIMP_OBJECT (object);

  switch (arg_id)
    {
    case ARG_NAME:
      GTK_VALUE_STRING (*arg) = g_strdup (gimp_object->name);
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

void
gimp_object_set_name (GimpObject  *object,
		      const gchar *name)
{
  g_return_if_fail (object);
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
  g_return_val_if_fail (object, NULL);
  g_return_val_if_fail (GIMP_IS_OBJECT (object), NULL);

  return object->name;
}

void
gimp_object_name_changed (GimpObject  *object)
{
  g_return_if_fail (object);
  g_return_if_fail (GIMP_IS_OBJECT (object));

  gtk_signal_emit (GTK_OBJECT (object), object_signals[NAME_CHANGED]);
}
