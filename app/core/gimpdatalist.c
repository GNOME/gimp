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

#include <glib-object.h>

#include "core-types.h"

#include "gimpdata.h"
#include "gimpdatalist.h"


static void   gimp_data_list_class_init        (GimpDataListClass *klass);
static void   gimp_data_list_init              (GimpDataList      *list);

static void   gimp_data_list_add               (GimpContainer     *container,
                                                GimpObject        *object);
static void   gimp_data_list_remove            (GimpContainer     *container,
                                                GimpObject        *object);

static void   gimp_data_list_object_renamed    (GimpObject        *object,
                                                GimpDataList      *data_list);
static gint   gimp_data_list_data_compare_func (gconstpointer      first,
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
  GimpContainerClass *container_class = GIMP_CONTAINER_CLASS (klass);

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
  GimpList *list = GIMP_LIST (container);

  gimp_list_uniquefy_name (GIMP_LIST (container), object, TRUE);

  g_signal_connect (object, "name_changed",
		    G_CALLBACK (gimp_data_list_object_renamed),
		    container);

  list->list = g_list_insert_sorted (list->list, object,
				     gimp_data_list_data_compare_func);
}

static void
gimp_data_list_remove (GimpContainer *container,
		       GimpObject    *object)
{
  GimpList *list = GIMP_LIST (container);

  g_signal_handlers_disconnect_by_func (object,
					gimp_data_list_object_renamed,
					container);

  list->list = g_list_remove (list->list, object);
}

GimpContainer *
gimp_data_list_new (GType children_type)
{
  GimpDataList *list;

  g_return_val_if_fail (g_type_is_a (children_type, GIMP_TYPE_DATA), NULL);

  list = g_object_new (GIMP_TYPE_DATA_LIST,
                       "children_type", children_type,
                       "policy",        GIMP_CONTAINER_POLICY_STRONG,
                       NULL);

  return GIMP_CONTAINER (list);
}

static void
gimp_data_list_object_renamed (GimpObject   *object,
                               GimpDataList *data_list)
{
  GimpList *gimp_list = GIMP_LIST (data_list);
  GList    *list;
  gint      old_index;
  gint      new_index = 0;

  g_signal_handlers_block_by_func (object,
                                   gimp_data_list_object_renamed,
                                   data_list);

  gimp_list_uniquefy_name (gimp_list, object, TRUE);

  g_signal_handlers_unblock_by_func (object,
                                     gimp_data_list_object_renamed,
                                     data_list);

  old_index = g_list_index (gimp_list->list, object);

  for (list = gimp_list->list; list; list = g_list_next (list))
    {
      GimpObject *object2 = GIMP_OBJECT (list->data);

      if (object == object2)
        continue;

      if (gimp_data_list_data_compare_func (object, object2) > 0)
        new_index++;
      else
        break;
    }

  if (new_index != old_index)
    gimp_container_reorder (GIMP_CONTAINER (data_list), object, new_index);
}

static gint
gimp_data_list_data_compare_func (gconstpointer first,
				  gconstpointer second)
{
  GimpData *first_data  = (GimpData *) first;
  GimpData *second_data = (GimpData *) second;

  /*  move the internal objects (like the FG -> BG) gradient) to the top  */
  if (first_data->internal != second_data->internal)
    return first_data->internal ? -1 : 1;

  return gimp_object_name_collate ((GimpObject *) first,
                                   (GimpObject *) second);
}
