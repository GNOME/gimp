/* parasitelist.c: Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpparasitelist.h"


enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};


static void   gimp_parasite_list_class_init (GimpParasiteListClass *klass);
static void   gimp_parasite_list_init       (GimpParasiteList      *list);

static void   gimp_parasite_list_finalize   (GObject               *object);

static gint   free_a_parasite               (gpointer               key,
					     gpointer               parasite,
					     gpointer               unused);


static guint parasite_list_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass *parent_class = NULL;


GType
gimp_parasite_list_get_type (void)
{
  static GType list_type = 0;

  if (! list_type)
    {
      static const GTypeInfo list_info =
      {
        sizeof (GimpParasiteListClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_parasite_list_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpParasiteList),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_parasite_list_init,
      };

      list_type = g_type_register_static (GIMP_TYPE_OBJECT,
					  "GimpParasiteList", 
					  &list_info, 0);
    }

  return list_type;
}

static void
gimp_parasite_list_class_init (GimpParasiteListClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  parasite_list_signals[ADD] =
    g_signal_new ("add",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpParasiteListClass, add),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  parasite_list_signals[REMOVE] = 
    g_signal_new ("remove",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpParasiteListClass, remove),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  GTK_TYPE_POINTER);

  object_class->finalize = gimp_parasite_list_finalize;

  klass->add             = NULL;
  klass->remove          = NULL;
}

static void
gimp_parasite_list_init (GimpParasiteList *list)
{
  list->table = NULL;
}

GimpParasiteList *
gimp_parasite_list_new (void)
{
  GimpParasiteList *list;

  list = g_object_new (GIMP_TYPE_PARASITE_LIST, NULL);

  return list;
}

static void
gimp_parasite_list_finalize (GObject *object)
{
  GimpParasiteList *list;

  g_return_if_fail (GIMP_IS_PARASITE_LIST (object));

  list = GIMP_PARASITE_LIST (object);

  if (list->table)
    {
      g_hash_table_foreach_remove (list->table, free_a_parasite, NULL);
      g_hash_table_destroy (list->table);
      list->table = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint
free_a_parasite (void *key, 
		 void *parasite, 
		 void *unused)
{
  gimp_parasite_free ((GimpParasite *) parasite);

  return TRUE;
}

static void
parasite_copy_one (gpointer key, 
		   gpointer p, 
		   gpointer data)
{
  GimpParasiteList *list     = (GimpParasiteList *) data;
  GimpParasite     *parasite = (GimpParasite *) p;

  gimp_parasite_list_add (list, parasite);
}

GimpParasiteList *
gimp_parasite_list_copy (const GimpParasiteList *list)
{
  GimpParasiteList *newlist;

  g_return_val_if_fail (GIMP_IS_PARASITE_LIST (list), NULL);

  newlist = gimp_parasite_list_new ();

  if (list->table)
    g_hash_table_foreach (list->table, parasite_copy_one, newlist);

  return newlist;
}

void
gimp_parasite_list_add (GimpParasiteList *list, 
			GimpParasite     *parasite)
{
  g_return_if_fail (GIMP_IS_PARASITE_LIST (list));
  g_return_if_fail (parasite != NULL);
  g_return_if_fail (parasite->name != NULL);

  if (list->table == NULL)
    list->table = g_hash_table_new (g_str_hash, g_str_equal);

  gimp_parasite_list_remove (list, parasite->name);
  parasite = gimp_parasite_copy (parasite);
  g_hash_table_insert (list->table, parasite->name, parasite);

  g_signal_emit (G_OBJECT (list), parasite_list_signals[ADD], 0,
		 parasite);
}

void
gimp_parasite_list_remove (GimpParasiteList *list, 
			   const gchar      *name)
{
  GimpParasite *parasite;

  g_return_if_fail (GIMP_IS_PARASITE_LIST (list));

  if (list->table)
    {
      parasite = gimp_parasite_list_find (list, name);

      if (parasite)
	{
	  g_hash_table_remove (list->table, name);

	  g_signal_emit (G_OBJECT (list), parasite_list_signals[REMOVE], 0,
			 parasite);

	  gimp_parasite_free (parasite);
	}
    }
}

gint
gimp_parasite_list_length (GimpParasiteList *list)
{
  g_return_val_if_fail (GIMP_IS_PARASITE_LIST (list), 0);

  if (! list->table)
    return 0;

  return g_hash_table_size (list->table);
}

static void 
ppcount_func (gchar        *key, 
	      GimpParasite *p, 
	      gint         *count)
{
  if (gimp_parasite_is_persistent (p))
    *count = *count + 1;
}

gint
gimp_parasite_list_persistent_length (GimpParasiteList *list)
{
  gint ppcount = 0;

  g_return_val_if_fail (GIMP_IS_PARASITE_LIST (list), 0);

  if (!list->table)
    return 0;

  gimp_parasite_list_foreach (list, (GHFunc) ppcount_func, &ppcount);

  return ppcount;
}

void
gimp_parasite_list_foreach (GimpParasiteList *list, 
			    GHFunc            function, 
			    gpointer          user_data)
{
  g_return_if_fail (GIMP_IS_PARASITE_LIST (list));

  if (!list->table)
    return;

  g_hash_table_foreach (list->table, function, user_data);
}

GimpParasite *
gimp_parasite_list_find (GimpParasiteList *list, 
			 const gchar      *name)
{
  g_return_val_if_fail (GIMP_IS_PARASITE_LIST (list), NULL);

  if (list->table)
    return (GimpParasite *) g_hash_table_lookup (list->table, name);
  else
    return NULL;
}

void
gimp_parasite_shift_parent (GimpParasite *parasite)
{
  if (parasite == NULL)
    return;

  parasite->flags = (parasite->flags >> 8);
}
