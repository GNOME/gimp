/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Manish Singh
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

#include <gmodule.h>

#include "gdisplay_color.h"
#include "gdisplay.h"
#include "gimpimageP.h"
#include "gimpui.h"

#include "libgimp/parasite.h"

#include <gtk/gtk.h>

typedef struct _ColorDisplayInfo ColorDisplayInfo;

struct _ColorDisplayInfo {
  char                    *name;
  GimpColorDisplayMethods  methods;
  GSList                  *refs;
};

static GHashTable *color_display_table = NULL;

static void       color_display_foreach_real (gpointer          key,
					      gpointer          value,
					      gpointer          user_data);
static void       gdisplay_color_detach_real (GDisplay         *gdisp,
					      ColorDisplayNode *node,
					      gboolean          unref);
static gint       node_name_compare          (ColorDisplayNode *node,
					      const char       *name);

void
color_display_init (void)
{
}

G_MODULE_EXPORT gboolean
gimp_color_display_register (const char              *name,
    			     GimpColorDisplayMethods *methods)
{
  ColorDisplayInfo *info;
  
  info = g_new (ColorDisplayInfo, 1);
  
  info->name = g_strdup (name);
  info->methods = *methods;
  info->refs = NULL;
  
  if (!color_display_table)
    color_display_table = g_hash_table_new (g_str_hash, g_str_equal);
  
  if (g_hash_table_lookup (color_display_table, name))
    return FALSE;

  if (!methods->convert)
    return FALSE;

  g_hash_table_insert (color_display_table, info->name, info);
  
  return TRUE;
}

G_MODULE_EXPORT gboolean
gimp_color_display_unregister (const char *name)
{
  ColorDisplayInfo *info;
  GDisplay *gdisp;
  GList *node;

  if ((info = g_hash_table_lookup (color_display_table, name)))
    {
      GSList *refs = info->refs;

      while (refs)
	{
	  gdisp = (GDisplay *) refs->data;

	  node = g_list_find_custom (gdisp->cd_list, (gpointer) name,
	      			     (GCompareFunc) node_name_compare);
	  gdisp->cd_list = g_list_remove_link (gdisp->cd_list, node);

	  gdisplay_color_detach_real (gdisp, node->data, FALSE);

	  g_list_free_1 (node);

	  refs = refs->next;
	}

      g_slist_free (info->refs);

      g_hash_table_remove (color_display_table, name);

      g_free (info->name);
      g_free (info);
    }
  
  return TRUE;
}

typedef struct _DisplayForeachData DisplayForeachData;

struct _DisplayForeachData
{
  GimpCDFunc func;
  gpointer   user_data;
};

void
color_display_foreach (GimpCDFunc func,
		       gpointer   user_data)
{
  DisplayForeachData data;

  data.func = func;
  data.user_data = user_data;

  g_hash_table_foreach (color_display_table, color_display_foreach_real, &data);
}

static void
color_display_foreach_real (gpointer key,
		            gpointer value,
		            gpointer user_data)
{
  DisplayForeachData *data = (DisplayForeachData *) user_data;
  data->func (key, data->user_data);
}

ColorDisplayNode *
gdisplay_color_attach (GDisplay   *gdisp,
		       const char *name)
{
  ColorDisplayInfo *info;
  ColorDisplayNode *node;

  g_return_val_if_fail (gdisp != NULL, NULL);

  if ((info = g_hash_table_lookup (color_display_table, name)))
    {
      node = g_new (ColorDisplayNode, 1);

      node->cd_name = g_strdup (name);
      node->cd_ID = NULL;

      if (!info->refs && info->methods.init)
	info->methods.init ();

      info->refs = g_slist_append (info->refs, gdisp);

      if (info->methods.new)
	node->cd_ID = info->methods.new (gdisp->gimage->base_type);

      node->cd_convert = info->methods.convert;

      gdisp->cd_list = g_list_append (gdisp->cd_list, node);

      return node;
    }
  else
    g_warning ("Tried to attach a nonexistant color display");

  return NULL;
}

ColorDisplayNode *
gdisplay_color_attach_clone (GDisplay         *gdisp,
			     ColorDisplayNode *node)
{
  ColorDisplayInfo *info;
  ColorDisplayNode *clone;

  g_return_val_if_fail (gdisp != NULL, NULL);
  g_return_val_if_fail (node != NULL, NULL);

  if ((info = g_hash_table_lookup (color_display_table, node->cd_name)))
    {
      clone = g_new (ColorDisplayNode, 1);

      clone->cd_name = g_strdup (node->cd_name);
      clone->cd_ID = NULL;

      info->refs = g_slist_append (info->refs, gdisp);

      if (info->methods.clone)
	node->cd_ID = info->methods.clone (node->cd_ID);

      node->cd_convert = info->methods.convert;

      gdisp->cd_list = g_list_append (gdisp->cd_list, node);

      return node;
    }
  else
    g_warning ("Tried to clone a nonexistant color display");

  return NULL;
}

void
gdisplay_color_detach (GDisplay         *gdisp,
		       ColorDisplayNode *node)
{
  g_return_if_fail (gdisp != NULL);

  gdisp->cd_list = g_list_remove (gdisp->cd_list, node);
}

void
gdisplay_color_detach_destroy (GDisplay         *gdisp,
			       ColorDisplayNode *node)
{
  g_return_if_fail (gdisp != NULL);

  gdisplay_color_detach_real (gdisp, node, TRUE);
  gdisp->cd_list = g_list_remove (gdisp->cd_list, node);
}

void
gdisplay_color_detach_all (GDisplay *gdisp)
{
  GList *list;

  g_return_if_fail (gdisp != NULL);

  list = gdisp->cd_list;

  while (list)
    {
      gdisplay_color_detach_real (gdisp, list->data, TRUE);
      list = list->next;
    }

  g_list_free (gdisp->cd_list);
  gdisp->cd_list = NULL;
}

static void
gdisplay_color_detach_real (GDisplay         *gdisp,
			    ColorDisplayNode *node,
			    gboolean          unref)
{
  ColorDisplayInfo *info;

  g_return_if_fail (gdisp != NULL);

  if ((info = g_hash_table_lookup (color_display_table, node->cd_name)))
    {
      if (info->methods.destroy)
	info->methods.destroy (node->cd_ID);

      if (unref)
        info->refs = g_slist_remove (info->refs, gdisp);
      
      if (!info->refs && info->methods.finalize)
	info->methods.finalize ();
    }

  g_free (node->cd_name);
  g_free (node);
}  

void
gdisplay_color_reorder_up (GDisplay         *gdisp,
			   ColorDisplayNode *node)
{
  GList *node_list;

  node_list = g_list_find (gdisp->cd_list, node);

  if (node_list->prev)
    {
      node_list->data = node_list->prev->data;
      node_list->prev->data = node;
    }
}

void
gdisplay_color_reorder_down (GDisplay         *gdisp,
			     ColorDisplayNode *node)
{
  GList *node_list;

  g_return_if_fail (gdisp != NULL);

  node_list = g_list_find (gdisp->cd_list, node);

  if (node_list->next)
    {
      node_list->data = node_list->next->data;
      node_list->next->data = node;
    }
}

static gint
node_name_compare (ColorDisplayNode *node,
		   const char       *name)
{
  return strcmp (node->cd_name, name);
}

void
gdisplay_color_configure (ColorDisplayNode *node,
			  GFunc             ok_func,
			  gpointer          ok_data,
			  GFunc             cancel_func,
			  gpointer          cancel_data)
{
  ColorDisplayInfo *info;

  g_return_if_fail (node != NULL);

  if ((info = g_hash_table_lookup (color_display_table, node->cd_name)))
    {
      if (info->methods.configure)
	info->methods.configure (node->cd_ID,
	    			 ok_func, ok_data,
				 cancel_func, cancel_data);
    }
  else
    g_warning ("Tried to configure a nonexistant color display");

  return;
}

void
gdisplay_color_configure_cancel (ColorDisplayNode *node)
{
  ColorDisplayInfo *info;

  g_return_if_fail (node != NULL);

  if ((info = g_hash_table_lookup (color_display_table, node->cd_name)))
    {
      if (info->methods.cancel)
	info->methods.cancel (node->cd_ID);
    }
  else
    g_warning ("Tried to configure cancel a nonexistant color display");

  return;
}
