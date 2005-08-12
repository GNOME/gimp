/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-proc-def.c
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

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"

#include "plug-in-types.h"

#include "core/gimp.h"

#include "plug-in.h"
#include "plug-ins.h"
#include "plug-in-proc-def.h"

#include "gimp-intl.h"


PlugInProcDef *
plug_in_proc_def_new (void)
{
  PlugInProcDef *proc_def = g_new0 (PlugInProcDef, 1);

  proc_def->icon_data_length = -1;

  return proc_def;
}

void
plug_in_proc_def_free (PlugInProcDef *proc_def)
{
  gint i;

  g_return_if_fail (proc_def != NULL);

  g_free (proc_def->db_info.name);
  g_free (proc_def->db_info.original_name);
  g_free (proc_def->db_info.blurb);
  g_free (proc_def->db_info.help);
  g_free (proc_def->db_info.author);
  g_free (proc_def->db_info.copyright);
  g_free (proc_def->db_info.date);

  for (i = 0; i < proc_def->db_info.num_args; i++)
    {
      g_free (proc_def->db_info.args[i].name);
      g_free (proc_def->db_info.args[i].description);
    }

  for (i = 0; i < proc_def->db_info.num_values; i++)
    {
      g_free (proc_def->db_info.values[i].name);
      g_free (proc_def->db_info.values[i].description);
    }

  g_free (proc_def->db_info.args);
  g_free (proc_def->db_info.values);

  g_free (proc_def->prog);
  g_free (proc_def->menu_label);

  g_list_foreach (proc_def->menu_paths, (GFunc) g_free, NULL);
  g_list_free (proc_def->menu_paths);

  g_free (proc_def->icon_data);
  g_free (proc_def->image_types);

  g_free (proc_def->extensions);
  g_free (proc_def->prefixes);
  g_free (proc_def->magics);
  g_free (proc_def->mime_type);

  g_slist_foreach (proc_def->extensions_list, (GFunc) g_free, NULL);
  g_slist_free (proc_def->extensions_list);

  g_slist_foreach (proc_def->prefixes_list, (GFunc) g_free, NULL);
  g_slist_free (proc_def->prefixes_list);

  g_slist_foreach (proc_def->magics_list, (GFunc) g_free, NULL);
  g_slist_free (proc_def->magics_list);

  g_free (proc_def->thumb_loader);

  g_free (proc_def);
}

PlugInProcDef *
plug_in_proc_def_find (GSList      *list,
                       const gchar *proc_name)
{
  GSList *l;

  for (l = list; l; l = g_slist_next (l))
    {
      PlugInProcDef *proc_def = l->data;

      if (! strcmp (proc_name, proc_def->db_info.name))
        return proc_def;
    }

  return NULL;
}

const ProcRecord *
plug_in_proc_def_get_proc (const PlugInProcDef *proc_def)
{
  g_return_val_if_fail (proc_def != NULL, NULL);

  return &proc_def->db_info;
}

const gchar *
plug_in_proc_def_get_progname (const PlugInProcDef *proc_def)
{
  g_return_val_if_fail (proc_def != NULL, NULL);

  switch (proc_def->db_info.proc_type)
    {
    case GIMP_PLUGIN:
    case GIMP_EXTENSION:
      return proc_def->prog;

    case GIMP_TEMPORARY:
      return ((PlugIn *) proc_def->db_info.exec_method.temporary.plug_in)->prog;

    default:
      break;
    }

  return NULL;
}

gchar *
plug_in_proc_def_get_label (const PlugInProcDef *proc_def,
                            const gchar         *locale_domain)
{
  const gchar *path;
  gchar       *stripped;
  gchar       *ellipses;
  gchar       *label;

  g_return_val_if_fail (proc_def != NULL, NULL);

  if (proc_def->menu_label)
    path = dgettext (locale_domain, proc_def->menu_label);
  else if (proc_def->menu_paths)
    path = dgettext (locale_domain, proc_def->menu_paths->data);
  else
    return NULL;

  stripped = gimp_strip_uline (path);

  if (proc_def->menu_label)
    label = g_strdup (stripped);
  else
    label = g_path_get_basename (stripped);

  g_free (stripped);

  ellipses = strstr (label, "...");

  if (ellipses && ellipses == (label + strlen (label) - 3))
    *ellipses = '\0';

  return label;
}

void
plug_in_proc_def_set_icon (PlugInProcDef *proc_def,
                           GimpIconType   icon_type,
                           const gchar   *icon_data,
                           gint           icon_data_length)
{
  g_return_if_fail (proc_def != NULL);
  g_return_if_fail (icon_type == -1 || icon_data != NULL);
  g_return_if_fail (icon_type == -1 || icon_data_length > 0);

  if (proc_def->icon_data)
    {
      g_free (proc_def->icon_data);
      proc_def->icon_data_length = -1;
      proc_def->icon_data        = NULL;
    }

  proc_def->icon_type = icon_type;

  switch (proc_def->icon_type)
    {
    case GIMP_ICON_TYPE_STOCK_ID:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      proc_def->icon_data_length = -1;
      proc_def->icon_data        = g_strdup (icon_data);
      break;

    case GIMP_ICON_TYPE_INLINE_PIXBUF:
      proc_def->icon_data_length = icon_data_length;
      proc_def->icon_data        = g_memdup (icon_data,
                                             icon_data_length);
      break;
    }
}

const gchar *
plug_in_proc_def_get_stock_id (const PlugInProcDef *proc_def)
{
  g_return_val_if_fail (proc_def != NULL, NULL);

  switch (proc_def->icon_type)
    {
    case GIMP_ICON_TYPE_STOCK_ID:
      return proc_def->icon_data;

    default:
      return NULL;
    }
}

GdkPixbuf *
plug_in_proc_def_get_pixbuf (const PlugInProcDef *proc_def)
{
  GdkPixbuf *pixbuf = NULL;
  GError    *error  = NULL;

  g_return_val_if_fail (proc_def != NULL, NULL);

  switch (proc_def->icon_type)
    {
    case GIMP_ICON_TYPE_INLINE_PIXBUF:
      pixbuf = gdk_pixbuf_new_from_inline (proc_def->icon_data_length,
                                           proc_def->icon_data, TRUE, &error);
      if (! pixbuf)
        {
          g_printerr (error->message);
          g_clear_error (&error);
        }
      break;

    case GIMP_ICON_TYPE_IMAGE_FILE:
      pixbuf = gdk_pixbuf_new_from_file (proc_def->icon_data, &error);
      if (! pixbuf)
        {
          g_printerr (error->message);
          g_clear_error (&error);
        }
      break;

    default:
      break;
    }

  return pixbuf;
}

gchar *
plug_in_proc_def_get_help_id (const PlugInProcDef *proc_def,
                              const gchar         *help_domain)
{
  g_return_val_if_fail (proc_def != NULL, NULL);

  if (help_domain)
    return g_strconcat (help_domain, "?", proc_def->db_info.name, NULL);

  return g_strdup (proc_def->db_info.name);
}

gboolean
plug_in_proc_def_get_sensitive (const PlugInProcDef *proc_def,
                                GimpImageType        image_type)
{
  gboolean sensitive;

  g_return_val_if_fail (proc_def != NULL, FALSE);

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
      sensitive = proc_def->image_types_val & PLUG_IN_RGB_IMAGE;
      break;
    case GIMP_RGBA_IMAGE:
      sensitive = proc_def->image_types_val & PLUG_IN_RGBA_IMAGE;
      break;
    case GIMP_GRAY_IMAGE:
      sensitive = proc_def->image_types_val & PLUG_IN_GRAY_IMAGE;
      break;
    case GIMP_GRAYA_IMAGE:
      sensitive = proc_def->image_types_val & PLUG_IN_GRAYA_IMAGE;
      break;
    case GIMP_INDEXED_IMAGE:
      sensitive = proc_def->image_types_val & PLUG_IN_INDEXED_IMAGE;
      break;
    case GIMP_INDEXEDA_IMAGE:
      sensitive = proc_def->image_types_val & PLUG_IN_INDEXEDA_IMAGE;
      break;
    default:
      sensitive = FALSE;
      break;
    }

  return sensitive ? TRUE : FALSE;
}
