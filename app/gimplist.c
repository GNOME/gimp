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

#include <stdio.h>

#include <gtk/gtk.h>

#include "apptypes.h"

#include "gimpobject.h"
#include "gimplist.h"


/*  code mostly ripped from nether's gimpset class  */

enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};

static guint gimp_list_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass *parent_class = NULL;


static void gimp_list_add_func    (GimpList *list,
				   gpointer  object);
static void gimp_list_remove_func (GimpList *list,
				   gpointer  object);


static void
gimp_list_destroy (GtkObject *object)
{
  GimpList *list = GIMP_LIST (object);

  while (list->list) /* ought to put a sanity check in here... */
    {
      gimp_list_remove (list, list->list->data);
    }
  g_slist_free (list->list);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_list_init (GimpList *list)
{
  list->list = NULL;
  list->type = GTK_TYPE_OBJECT;
}

static void
gimp_list_class_init (GimpListClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);

  gimp_list_signals[ADD]=
    gtk_signal_new ("add",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpListClass,
				       add),
                    gtk_marshal_NONE__POINTER,
                    GTK_TYPE_NONE, 1,
		    GTK_TYPE_POINTER);

  gimp_list_signals[REMOVE]=
    gtk_signal_new ("remove",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpListClass,
				       remove),
                    gtk_marshal_NONE__POINTER,
                    GTK_TYPE_NONE, 1,
		    GTK_TYPE_POINTER);

  gtk_object_class_add_signals (object_class, gimp_list_signals, LAST_SIGNAL);

  object_class->destroy = gimp_list_destroy;

  klass->add    = gimp_list_add_func;
  klass->remove = gimp_list_remove_func;
}

GtkType
gimp_list_get_type (void)
{
  static GtkType list_type = 0;

  if (! list_type)
    {
      GtkTypeInfo list_info =
      {
	"GimpList",
	sizeof (GimpList),
	sizeof (GimpListClass),
	(GtkClassInitFunc) gimp_list_class_init,
	(GtkObjectInitFunc) gimp_list_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL
      };

      list_type = gtk_type_unique (GIMP_TYPE_OBJECT, &list_info);
    }

  return list_type;
}

GimpList *
gimp_list_new (GtkType  type,
	       gboolean weak)
{
  GimpList *list;

  list = gtk_type_new (GIMP_TYPE_LIST);

  list->type = type;
  list->weak = weak;

  return list;
}

static void
gimp_list_destroy_cb (GtkObject *object,
		      gpointer   data)
{
  GimpList *list;

  list = GIMP_LIST (data);

  gimp_list_remove (list, object);
}

static void
gimp_list_add_func (GimpList *list,
		    gpointer  object)
{
  list->list = g_slist_prepend (list->list, object);
}

static void
gimp_list_remove_func (GimpList *list,
		       gpointer  object)
{
  list->list = g_slist_remove (list->list, object);
}

gboolean
gimp_list_add (GimpList *list,
	       gpointer  object)
{
  g_return_val_if_fail (list, FALSE);
  g_return_val_if_fail (GTK_CHECK_TYPE (object, list->type), FALSE);
	
  if (g_slist_find (list->list, object))
    return FALSE;
	
  if (list->weak)
    {
      gtk_signal_connect (GTK_OBJECT (object), "destroy",
			  GTK_SIGNAL_FUNC (gimp_list_destroy_cb),
			  list);
    }
  else
    {
      gtk_object_ref (GTK_OBJECT (object));
      gtk_object_sink (GTK_OBJECT (object));
    }

  gtk_signal_emit (GTK_OBJECT (list), gimp_list_signals[ADD], object);

  return TRUE;
}

gboolean
gimp_list_remove (GimpList *list,
		  gpointer  object)
{
  g_return_val_if_fail (list, FALSE);

  if (!g_slist_find (list->list, object))
    {
      g_warning ("gimp_list_remove: can't find val");
      return FALSE;
    }

  gtk_signal_emit (GTK_OBJECT (list), gimp_list_signals[REMOVE], object);

  if (list->weak)
    {
      gtk_signal_disconnect_by_func (GTK_OBJECT (object),
				     GTK_SIGNAL_FUNC (gimp_list_destroy_cb),
				     list);
    }
  else
    {
      gtk_object_unref (GTK_OBJECT (object));
    }

  return TRUE;
}

gboolean
gimp_list_have (GimpList *list,
		gpointer  object)
{
  return g_slist_find (list->list, object) ? TRUE : FALSE;
}

void
gimp_list_foreach (GimpList *list,
		   GFunc     func,
		   gpointer  user_data)
{
  g_slist_foreach (list->list, func, user_data);
}

GtkType
gimp_list_type (GimpList *list)
{
  return list->type;
}
