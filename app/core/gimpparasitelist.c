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

#include <string.h>

#include "gimpobjectP.h"
#include "gimpobject.h"
#include "gimpsignal.h"
#include "parasitelistP.h"
#include "libgimp/parasite.h"
#include "libgimp/parasiteP.h"

enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};

static guint parasite_list_signals[LAST_SIGNAL];

static void parasite_list_destroy    (GtkObject* list);
static void parasite_list_init       (ParasiteList* list);
static void parasite_list_class_init (ParasiteListClass *klass);
static int  free_a_parasite          (void *key, void *parasite, void *unused);

static void
parasite_list_init (ParasiteList* list)
{
  list->table=NULL;
}

static void
parasite_list_class_init (ParasiteListClass *klass)
{
  GtkObjectClass *class = GTK_OBJECT_CLASS(klass);
  GtkType type = class->type;

  class->destroy = parasite_list_destroy;

  parasite_list_signals[ADD] =
    gimp_signal_new ("add",    GTK_RUN_FIRST, type, 0, gimp_sigtype_pointer);
  parasite_list_signals[REMOVE] = 
    gimp_signal_new ("remove", GTK_RUN_FIRST, type, 0, gimp_sigtype_pointer);
  gtk_object_class_add_signals (class, parasite_list_signals, LAST_SIGNAL);
}

GtkType
parasite_list_get_type (void)
{
  static GtkType type = 0;

  if (!type)
    {
      GtkTypeInfo info =
      {
	"ParasiteList",
	sizeof (ParasiteList),
	sizeof (ParasiteListClass),
	(GtkClassInitFunc) parasite_list_class_init,
	(GtkObjectInitFunc) parasite_list_init,
	NULL,
	NULL,
	(GtkClassInitFunc) NULL
      };
      type = gtk_type_unique (GIMP_TYPE_OBJECT, &info);
    }
  return type;
}

ParasiteList*
parasite_list_new (void)
{
  ParasiteList *list = gtk_type_new (GIMP_TYPE_PARASITE_LIST);
  list->table = NULL;
  return list;
}

static int
free_a_parasite (void *key, 
		 void *parasite, 
		 void *unused)
{
  parasite_free ((Parasite *)parasite);
  return TRUE;
}

static void
parasite_list_destroy (GtkObject *obj)
{
  ParasiteList *list;

  g_return_if_fail (obj != NULL);
  g_return_if_fail (GIMP_IS_PARASITE_LIST(obj));

  list = (ParasiteList *)obj;

  if (list->table)
    {
      g_hash_table_foreach_remove (list->table, free_a_parasite, NULL);
      g_hash_table_destroy (list->table);
    }
}

static void
parasite_copy_one (void *key, 
		   void *p, 
		   void *data)
{
  ParasiteList *list = (ParasiteList*)data;
  Parasite *parasite = (Parasite*)p;
  parasite_list_add (list, parasite);
}

ParasiteList*
parasite_list_copy (const ParasiteList *list)
{
  ParasiteList *newlist;

  newlist = parasite_list_new ();
  if (list->table)
    g_hash_table_foreach (list->table, parasite_copy_one, newlist);

  return newlist;
}

void
parasite_list_add (ParasiteList *list, 
		   Parasite     *p)
{
  g_return_if_fail (list != NULL);

  if (list->table == NULL)
    list->table = g_hash_table_new (g_str_hash, g_str_equal);

  g_return_if_fail (p != NULL);
  g_return_if_fail (p->name != NULL);

  parasite_list_remove (list, p->name);
  p = parasite_copy (p);
  g_hash_table_insert (list->table, p->name, p);
  gtk_signal_emit (GTK_OBJECT(list), parasite_list_signals[ADD], p);
}

void
parasite_list_remove (ParasiteList *list, 
		      const char   *name)
{
  Parasite *p;

  g_return_if_fail (list != NULL);

  if (list->table)
    {
      p = parasite_list_find(list, name);
      if (p)
	{
	  g_hash_table_remove (list->table, name);
	  gtk_signal_emit (GTK_OBJECT(list), parasite_list_signals[REMOVE], p);
	  parasite_free (p);
	}
    }
}

gint
parasite_list_length (ParasiteList *list)
{
  g_return_val_if_fail (list != NULL, 0);

  if (!list->table)
    return 0;
  return g_hash_table_size (list->table);
}

static void 
ppcount_func (char     *key, 
	      Parasite *p, 
	      int      *count)
{
  if (parasite_is_persistent (p))
    *count = *count + 1;
}

gint
parasite_list_persistent_length (ParasiteList *list)
{
  int ppcount = 0;

  g_return_val_if_fail (list != NULL, 0);

  if (!list->table)
    return 0;

  parasite_list_foreach (list, (GHFunc)ppcount_func, &ppcount);
  return ppcount;
}

void
parasite_list_foreach (ParasiteList *list, 
		       GHFunc        function, 
		       gpointer      user_data)
{
  g_return_if_fail (list != NULL);

  if (!list->table)
    return;

  g_hash_table_foreach (list->table, function, user_data);
}

Parasite *
parasite_list_find (ParasiteList *list, 
		    const char   *name)
{
  g_return_val_if_fail (list != NULL, NULL);

  if (list->table)
    return (Parasite *)g_hash_table_lookup (list->table, name);
  else
    return NULL;
}

void
parasite_shift_parent (Parasite *p)
{
  if (p == NULL)
    return;

  p->flags = (p->flags >> 8);
}
