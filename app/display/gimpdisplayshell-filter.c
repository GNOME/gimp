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
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "display-types.h"

#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-filter.h"


typedef struct _ColorDisplayInfo ColorDisplayInfo;

struct _ColorDisplayInfo
 {
  gchar                   *name;
  GimpColorDisplayMethods  methods;
  GSList                  *refs;
};


static void   color_display_foreach_real            (gpointer          key,
                                                     gpointer          value,
                                                     gpointer          user_data);
static void   gimp_display_shell_filter_detach_real (GimpDisplayShell *shell,
                                                     ColorDisplayNode *node,
                                                     gboolean          unref);
static gint   node_name_compare                     (ColorDisplayNode *node,
                                                     const gchar      *name);


static GHashTable *color_display_table = NULL;


void
color_display_init (void)
{
}

G_MODULE_EXPORT gboolean
gimp_color_display_register (const gchar             *name,
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
gimp_color_display_unregister (const gchar *name)
{
  ColorDisplayInfo *info;
  GimpDisplayShell *shell;
  GList            *node;

  if ((info = g_hash_table_lookup (color_display_table, name)))
    {
      GSList *refs;

      for (refs = info->refs; refs; refs = g_slist_next (refs))
	{
	  shell = GIMP_DISPLAY_SHELL (refs->data);

	  node = g_list_find_custom (shell->cd_list, (gpointer) name,
	      			     (GCompareFunc) node_name_compare);
	  shell->cd_list = g_list_remove_link (shell->cd_list, node);

	  gimp_display_shell_filter_detach_real (shell, node->data, FALSE);

	  g_list_free_1 (node);
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
  DisplayForeachData *data;

  data = (DisplayForeachData *) user_data;

  data->func (key, data->user_data);
}

ColorDisplayNode *
gimp_display_shell_filter_attach (GimpDisplayShell *shell,
                                  const gchar      *name)
{
  ColorDisplayInfo *info;
  ColorDisplayNode *node;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  if ((info = g_hash_table_lookup (color_display_table, name)))
    {
      node = g_new (ColorDisplayNode, 1);

      node->cd_name = g_strdup (name);
      node->cd_ID = NULL;

      if (!info->refs && info->methods.init)
	info->methods.init ();

      info->refs = g_slist_append (info->refs, shell);

      if (info->methods.new)
	node->cd_ID = info->methods.new (shell->gdisp->gimage->base_type);

      node->cd_convert = info->methods.convert;

      shell->cd_list = g_list_append (shell->cd_list, node);

      return node;
    }
  else
    g_warning ("Tried to attach a nonexistant color display");

  return NULL;
}

ColorDisplayNode *
gimp_display_shell_filter_attach_clone (GimpDisplayShell *shell,
                                        ColorDisplayNode *node)
{
  ColorDisplayInfo *info;
  ColorDisplayNode *clone;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (node != NULL, NULL);

  if ((info = g_hash_table_lookup (color_display_table, node->cd_name)))
    {
      clone = g_new (ColorDisplayNode, 1);

      clone->cd_name = g_strdup (node->cd_name);
      clone->cd_ID   = NULL;

      info->refs = g_slist_append (info->refs, shell);

      if (info->methods.clone)
	node->cd_ID = info->methods.clone (node->cd_ID);

      node->cd_convert = info->methods.convert;

      shell->cd_list = g_list_append (shell->cd_list, node);

      return node;
    }
  else
    g_warning ("Tried to clone a nonexistant color display");

  return NULL;
}

void
gimp_display_shell_filter_detach (GimpDisplayShell *shell,
                                  ColorDisplayNode *node)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  shell->cd_list = g_list_remove (shell->cd_list, node);
}

void
gimp_display_shell_filter_detach_destroy (GimpDisplayShell *shell,
                                          ColorDisplayNode *node)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_filter_detach_real (shell, node, TRUE);

  shell->cd_list = g_list_remove (shell->cd_list, node);
}

void
gimp_display_shell_filter_detach_all (GimpDisplayShell *shell)
{
  GList *list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  for (list = shell->cd_list; list; list = g_list_next (list))
    {
      gimp_display_shell_filter_detach_real (shell, list->data, TRUE);
    }

  g_list_free (shell->cd_list);
  shell->cd_list = NULL;
}

static void
gimp_display_shell_filter_detach_real (GimpDisplayShell *shell,
                                       ColorDisplayNode *node,
                                       gboolean          unref)
{
  ColorDisplayInfo *info;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (node  != NULL);

  if ((info = g_hash_table_lookup (color_display_table, node->cd_name)))
    {
      if (info->methods.destroy)
	info->methods.destroy (node->cd_ID);

      if (unref)
        info->refs = g_slist_remove (info->refs, shell);

      if (!info->refs && info->methods.finalize)
	info->methods.finalize ();
    }

  g_free (node->cd_name);
  g_free (node);
}

void
gimp_display_shell_filter_reorder_up (GimpDisplayShell *shell,
                                      ColorDisplayNode *node)
{
  GList *node_list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (node  != NULL);

  node_list = g_list_find (shell->cd_list, node);

  if (node_list->prev)
    {
      node_list->data = node_list->prev->data;
      node_list->prev->data = node;
    }
}

void
gimp_display_shell_filter_reorder_down (GimpDisplayShell *shell,
                                        ColorDisplayNode *node)
{
  GList *node_list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (node  != NULL);

  node_list = g_list_find (shell->cd_list, node);

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
gimp_display_shell_filter_configure (ColorDisplayNode *node,
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
gimp_display_shell_filter_configure_cancel (ColorDisplayNode *node)
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
