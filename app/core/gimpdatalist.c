/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatalist.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimpdata.h"
#include "gimpdatalist.h"

#include "libgimp/gimpintl.h"


static void   gimp_data_list_class_init         (GimpDataListClass *klass);
static void   gimp_data_list_init               (GimpDataList      *list);
static void   gimp_data_list_add                (GimpContainer     *container,
						 GimpObject        *object);
static void   gimp_data_list_remove             (GimpContainer     *container,
						 GimpObject        *object);

static void   gimp_data_list_uniquefy_data_name (GimpDataList      *data_list,
						 GimpObject        *object);
static void   gimp_data_list_object_renamed_callback (GimpObject   *object,
						      GimpDataList *data_list);
static gint   gimp_data_list_data_compare_func  (gconstpointer      first,
						 gconstpointer      second);


static GimpListClass *parent_class = NULL;


GType
gimp_data_list_get_type (void)
{
  static GType list_type = 0;

  if (! list_type)
    {
      static const GTypeInfo list_info =
      {
        sizeof (GimpDataListClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_data_list_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpDataList),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_data_list_init,
      };

      list_type = g_type_register_static (GIMP_TYPE_LIST,
					  "GimpDataList", 
					  &list_info, 0);
    }

  return list_type;
}

static void
gimp_data_list_class_init (GimpDataListClass *klass)
{
  GimpContainerClass *container_class;
  
  container_class = GIMP_CONTAINER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  container_class->add    = gimp_data_list_add;
  container_class->remove = gimp_data_list_remove;
}

static void
gimp_data_list_init (GimpDataList *list)
{
}

static void
gimp_data_list_add (GimpContainer *container,
		    GimpObject    *object)
{
  GimpList *list;

  list = GIMP_LIST (container);

  gimp_data_list_uniquefy_data_name (GIMP_DATA_LIST (container), object);

  list->list = g_list_insert_sorted (list->list, object,
				     gimp_data_list_data_compare_func);

  g_signal_connect (G_OBJECT (object), "name_changed",
		    G_CALLBACK (gimp_data_list_object_renamed_callback),
		    container);
}

static void
gimp_data_list_remove (GimpContainer *container,
		       GimpObject    *object)
{
  GimpList *list;

  list = GIMP_LIST (container);

  g_signal_handlers_disconnect_by_func (G_OBJECT (object),
					gimp_data_list_object_renamed_callback,
					container);

  list->list = g_list_remove (list->list, object);
}

GimpContainer *
gimp_data_list_new (GType children_type)

{
  GimpDataList *list;

  g_return_val_if_fail (g_type_is_a (children_type, GIMP_TYPE_DATA), NULL);

  list = GIMP_DATA_LIST (g_object_new (GIMP_TYPE_DATA_LIST, NULL));

  GIMP_CONTAINER (list)->children_type = children_type;
  GIMP_CONTAINER (list)->policy        = GIMP_CONTAINER_POLICY_STRONG;

  return GIMP_CONTAINER (list);
}

static void
gimp_data_list_uniquefy_data_name (GimpDataList *data_list,
				   GimpObject   *object)
{
  GList      *base_list;
  GList      *list;
  GList      *list2;
  GimpObject *object2;
  gint        unique_ext = 0;
  gchar      *new_name   = NULL;
  gchar      *ext;
  gboolean    have;

  g_return_if_fail (GIMP_IS_DATA_LIST (data_list));
  g_return_if_fail (GIMP_IS_OBJECT (object));

  base_list = GIMP_LIST (data_list)->list;

  have = gimp_container_have (GIMP_CONTAINER (data_list), object);

  for (list = base_list; list; list = g_list_next (list))
    {
      object2 = GIMP_OBJECT (list->data);

      if (object != object2 &&
	  strcmp (gimp_object_get_name (GIMP_OBJECT (object)),
		  gimp_object_get_name (GIMP_OBJECT (object2))) == 0)
	{
          ext = strrchr (object->name, '#');

          if (ext)
            {
              gchar *ext_str;

              unique_ext = atoi (ext + 1);

              ext_str = g_strdup_printf ("%d", unique_ext);

              /*  check if the extension really is of the form "#<n>"  */
              if (! strcmp (ext_str, ext + 1))
                {
                  *ext = '\0';
                }
              else
                {
                  unique_ext = 0;
                }

              g_free (ext_str);
            }
          else
            {
              unique_ext = 0;
            }

          do
            {
              unique_ext++;

              g_free (new_name);

              new_name = g_strdup_printf ("%s#%d", object->name, unique_ext);

              for (list2 = base_list; list2; list2 = g_list_next (list2))
                {
                  object2 = GIMP_OBJECT (list2->data);

                  if (object == object2)
                    continue;

                  if (! strcmp (object2->name, new_name))
		    break;
                }
            }
          while (list2);

	  if (have)
	    g_signal_handlers_block_by_func (G_OBJECT (object),
					     gimp_data_list_object_renamed_callback,
					     data_list);

	  gimp_object_set_name (object, new_name);

	  if (have)
	    g_signal_handlers_unblock_by_func (G_OBJECT (object),
					       gimp_data_list_object_renamed_callback,
					       data_list);

	  g_free (new_name);

	  break;
	}
    }

  if (have)
    {
      gint old_index;
      gint new_index = 0;

      old_index = g_list_index (base_list, object);

      for (list2 = base_list; list2; list2 = g_list_next (list2))
	{
	  object2 = GIMP_OBJECT (list2->data);

	  if (object == object2)
	    continue;

	  if (strcmp (object->name, object2->name) > 0)
	    new_index++;
	  else
	    break;
	}

      if (new_index != old_index)
	{
	  gimp_container_reorder (GIMP_CONTAINER (data_list),
				  object, new_index);
	}
    }
}

static void
gimp_data_list_object_renamed_callback (GimpObject   *object,
					GimpDataList *data_list)
{
  gimp_data_list_uniquefy_data_name (data_list, object);
}

static gint
gimp_data_list_data_compare_func (gconstpointer first,
				  gconstpointer second)
{
  return strcmp (((const GimpObject *) first)->name, 
		 ((const GimpObject *) second)->name);
}
