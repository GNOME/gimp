/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include <stdio.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-parasites.h"
#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpitem.h"
#include "gimplayer.h"
#include "gimplist.h"
#include "gimpmarshal.h"
#include "gimpparasitelist.h"

#include "vectors/gimpvectors.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


enum
{
  REMOVED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void    gimp_item_class_init   (GimpItemClass *klass);
static void    gimp_item_init         (GimpItem      *item);

static void    gimp_item_finalize     (GObject       *object);

static void    gimp_item_name_changed (GimpObject    *object);
static gsize   gimp_item_get_memsize  (GimpObject    *object);


/*  private variables  */

static guint gimp_item_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;


GType
gimp_item_get_type (void)
{
  static GType item_type = 0;

  if (! item_type)
    {
      static const GTypeInfo item_info =
      {
        sizeof (GimpItemClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_item_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpItem),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_item_init,
      };

      item_type = g_type_register_static (GIMP_TYPE_VIEWABLE,
                                          "GimpItem", 
                                          &item_info, 0);
    }

  return item_type;
}

static void
gimp_item_class_init (GimpItemClass *klass)
{
  GObjectClass    *object_class;
  GimpObjectClass *gimp_object_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_item_signals[REMOVED] =
    g_signal_new ("removed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpItemClass, removed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize          = gimp_item_finalize;

  gimp_object_class->name_changed = gimp_item_name_changed;
  gimp_object_class->get_memsize  = gimp_item_get_memsize;

  klass->removed                  = NULL;
}

static void
gimp_item_init (GimpItem *item)
{
  item->ID        = 0;
  item->tattoo    = 0;
  item->gimage    = NULL;
  item->parasites = gimp_parasite_list_new ();
}

static void
gimp_item_finalize (GObject *object)
{
  GimpItem *item;

  g_return_if_fail (GIMP_IS_ITEM (object));

  item = GIMP_ITEM (object);

  if (item->gimage && item->gimage->gimp)
    {
      g_hash_table_remove (item->gimage->gimp->item_table,
			   GINT_TO_POINTER (item->ID));
      item->gimage = NULL;
    }

  if (item->parasites)
    {
      g_object_unref (G_OBJECT (item->parasites));
      item->parasites = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_item_name_changed (GimpObject *object)
{
  GimpItem *item;
  GimpItem *item2;
  GList    *list, *list2, *base_list;
  gint      unique_ext = 0;
  gchar    *ext;
  gchar    *new_name = NULL;

  g_return_if_fail (GIMP_IS_ITEM (object));

  item = GIMP_ITEM (object);

  /*  if no other items to check name against  */
  if (item->gimage == NULL)
    return;

  if (GIMP_IS_LAYER (item))
    base_list = GIMP_LIST (item->gimage->layers)->list;
  else if (GIMP_IS_CHANNEL (item))
    base_list = GIMP_LIST (item->gimage->channels)->list;
  else if (GIMP_IS_VECTORS (item))
    base_list = GIMP_LIST (item->gimage->vectors)->list;
  else
    base_list = NULL;

  for (list = base_list; 
       list; 
       list = g_list_next (list))
    {
      item2 = GIMP_ITEM (list->data);

      if (item != item2 &&
	  strcmp (gimp_object_get_name (GIMP_OBJECT (item)),
		  gimp_object_get_name (GIMP_OBJECT (item2))) == 0)
	{
          ext = strrchr (GIMP_OBJECT (item)->name, '#');

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

	      new_name = g_strdup_printf ("%s#%d",
					  GIMP_OBJECT (item)->name,
					  unique_ext);

              for (list2 = base_list; list2; list2 = g_list_next (list2))
                {
		  item2 = GIMP_ITEM (list2->data);

		  if (item == item2)
		    continue;

                  if (! strcmp (GIMP_OBJECT (item2)->name, new_name))
                    {
                      break;
                    }
                }
            }
          while (list2);

          g_free (GIMP_OBJECT (item)->name);

          GIMP_OBJECT (item)->name = new_name;

          break;
        }
    }
}

static gsize
gimp_item_get_memsize (GimpObject *object)
{
  GimpItem *item;
  gsize     memsize = 0;

  item = GIMP_ITEM (object);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (item->parasites));

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
}

void
gimp_item_removed (GimpItem *item)
{
  g_return_if_fail (GIMP_IS_ITEM (item));

  g_signal_emit (G_OBJECT (item), gimp_item_signals[REMOVED], 0);
}

gint
gimp_item_get_ID (GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), -1);

  return item->ID;
}

GimpItem *
gimp_item_get_by_ID (Gimp *gimp,
                     gint  item_id)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp->item_table == NULL)
    return NULL;

  return (GimpItem *) g_hash_table_lookup (gimp->item_table, 
                                           GINT_TO_POINTER (item_id));
}

GimpTattoo
gimp_item_get_tattoo (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), 0); 

  return item->tattoo;
}

void
gimp_item_set_tattoo (GimpItem   *item,
                      GimpTattoo  tattoo)
{
  g_return_if_fail (GIMP_IS_ITEM (item));

  item->tattoo = tattoo;
}

GimpImage *
gimp_item_get_image (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  return item->gimage;
}

void
gimp_item_set_image (GimpItem  *item,
                     GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));;

  if (gimage == NULL)
    {
      item->tattoo = 0;
    }
  else if (item->tattoo == 0 || item->gimage != gimage)
    {
      item->tattoo = gimp_image_get_new_tattoo (gimage);
    }

  item->gimage = gimage;
}

void
gimp_item_parasite_attach (GimpItem     *item,
                           GimpParasite *parasite)
{
  g_return_if_fail (GIMP_IS_ITEM (item));

  /*  only set the dirty bit manually if we can be saved and the new
   *  parasite differs from the current one and we arn't undoable
   */
  if (gimp_parasite_is_undoable (parasite))
    {
      /* do a group in case we have attach_parent set */
      undo_push_group_start (item->gimage, PARASITE_ATTACH_UNDO_GROUP);

      undo_push_item_parasite (item->gimage, item, parasite);
    }
  else if (gimp_parasite_is_persistent (parasite) &&
	   ! gimp_parasite_compare (parasite,
				    gimp_item_parasite_find
				    (item, gimp_parasite_name (parasite))))
    {
      undo_push_cantundo (item->gimage, _("parasite attached to item"));
    }

  gimp_parasite_list_add (item->parasites, parasite);

  if (gimp_parasite_has_flag (parasite, GIMP_PARASITE_ATTACH_PARENT))
    {
      gimp_parasite_shift_parent (parasite);
      gimp_image_parasite_attach (item->gimage, parasite);
    }
  else if (gimp_parasite_has_flag (parasite, GIMP_PARASITE_ATTACH_GRANDPARENT))
    {
      gimp_parasite_shift_parent (parasite);
      gimp_parasite_shift_parent (parasite);
      gimp_parasite_attach (item->gimage->gimp, parasite);
    }

  if (gimp_parasite_is_undoable (parasite))
    {
      undo_push_group_end (item->gimage);
    }
}

void
gimp_item_parasite_detach (GimpItem    *item,
                           const gchar *name)
{
  GimpParasite *parasite;

  g_return_if_fail (GIMP_IS_ITEM (item));

  parasite = gimp_parasite_list_find (item->parasites, name);

  if (! parasite)
    return;

  if (gimp_parasite_is_undoable (parasite))
    {
      undo_push_item_parasite_remove (item->gimage, item,
                                      gimp_parasite_name (parasite));
    }
  else if (gimp_parasite_is_persistent (parasite))
    {
      undo_push_cantundo (item->gimage, _("parasite detached from item"));
    }

  gimp_parasite_list_remove (item->parasites, name);
}

GimpParasite *
gimp_item_parasite_find (const GimpItem *item,
                         const gchar    *name)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  return gimp_parasite_list_find (item->parasites, name);
}

static void
gimp_item_parasite_list_foreach_func (gchar          *name,
                                      GimpParasite   *parasite,
                                      gchar        ***cur)
{
  *(*cur)++ = (gchar *) g_strdup (name);
}

gchar **
gimp_item_parasite_list (const GimpItem *item,
                         gint           *count)
{
  gchar **list;
  gchar **cur;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (count != NULL, NULL);

  *count = gimp_parasite_list_length (item->parasites);

  cur = list = g_new (gchar *, *count);

  gimp_parasite_list_foreach (item->parasites,
			      (GHFunc) gimp_item_parasite_list_foreach_func,
			      &cur);

  return list;
}
