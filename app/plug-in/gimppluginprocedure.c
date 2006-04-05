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

#include "pdb/gimpprocedure.h"

#include "plug-in.h"
#include "plug-ins.h"
#include "plug-in-proc-def.h"
#include "plug-in-run.h"

#include "gimp-intl.h"


static void          gimp_plug_in_procedure_finalize (GObject       *object);

static GValueArray * gimp_plug_in_procedure_execute  (GimpProcedure *procedure,
                                                      Gimp          *gimp,
                                                      GimpContext   *context,
                                                      GimpProgress  *progress,
                                                      GValueArray   *args);


G_DEFINE_TYPE (GimpPlugInProcedure, gimp_plug_in_procedure,
               GIMP_TYPE_PROCEDURE);

#define parent_class gimp_plug_in_procedure_parent_class


static void
gimp_plug_in_procedure_class_init (GimpPlugInProcedureClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GimpProcedureClass *proc_class   = GIMP_PROCEDURE_CLASS (klass);

  object_class->finalize = gimp_plug_in_procedure_finalize;

  proc_class->execute    = gimp_plug_in_procedure_execute;
}

static void
gimp_plug_in_procedure_init (GimpPlugInProcedure *proc)
{
  proc->icon_data_length = -1;
}

static void
gimp_plug_in_procedure_finalize (GObject *object)
{
  GimpPlugInProcedure *proc = GIMP_PLUG_IN_PROCEDURE (object);

  g_free (proc->prog);
  g_free (proc->menu_label);

  g_list_foreach (proc->menu_paths, (GFunc) g_free, NULL);
  g_list_free (proc->menu_paths);

  g_free (proc->icon_data);
  g_free (proc->image_types);

  g_free (proc->extensions);
  g_free (proc->prefixes);
  g_free (proc->magics);
  g_free (proc->mime_type);

  g_slist_foreach (proc->extensions_list, (GFunc) g_free, NULL);
  g_slist_free (proc->extensions_list);

  g_slist_foreach (proc->prefixes_list, (GFunc) g_free, NULL);
  g_slist_free (proc->prefixes_list);

  g_slist_foreach (proc->magics_list, (GFunc) g_free, NULL);
  g_slist_free (proc->magics_list);

  g_free (proc->thumb_loader);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GValueArray *
gimp_plug_in_procedure_execute  (GimpProcedure *procedure,
                                 Gimp          *gimp,
                                 GimpContext   *context,
                                 GimpProgress  *progress,
                                 GValueArray   *args)
{
  if (procedure->proc_type == GIMP_INTERNAL)
    return GIMP_PROCEDURE_CLASS (parent_class)->execute (procedure, gimp,
                                                         context, progress,
                                                         args);

  return plug_in_run (gimp, context, progress, procedure,
                      args, TRUE, FALSE, -1);
}


/*  public functions  */

GimpPlugInProcedure *
gimp_plug_in_procedure_new (void)
{
  GimpPlugInProcedure *proc = g_object_new (GIMP_TYPE_PLUG_IN_PROCEDURE, NULL);

  return proc;
}

GimpPlugInProcedure *
gimp_plug_in_procedure_find (GSList      *list,
                             const gchar *proc_name)
{
  GSList *l;

  for (l = list; l; l = g_slist_next (l))
    {
      GimpProcedure *proc = l->data;

      if (! strcmp (proc_name, proc->name))
        return GIMP_PLUG_IN_PROCEDURE (proc);
    }

  return NULL;
}

const gchar *
gimp_plug_in_procedure_get_progname (const GimpPlugInProcedure *proc)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), NULL);

  switch (GIMP_PROCEDURE (proc)->proc_type)
    {
    case GIMP_PLUGIN:
    case GIMP_EXTENSION:
      return proc->prog;

    case GIMP_TEMPORARY:
      return ((PlugIn *) GIMP_PROCEDURE (proc)->exec_method.temporary.plug_in)->prog;

    default:
      break;
    }

  return NULL;
}

gchar *
gimp_plug_in_procedure_get_label (const GimpPlugInProcedure *proc,
                                  const gchar         *locale_domain)
{
  const gchar *path;
  gchar       *stripped;
  gchar       *ellipses;
  gchar       *label;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), NULL);

  if (proc->menu_label)
    path = dgettext (locale_domain, proc->menu_label);
  else if (proc->menu_paths)
    path = dgettext (locale_domain, proc->menu_paths->data);
  else
    return NULL;

  stripped = gimp_strip_uline (path);

  if (proc->menu_label)
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
gimp_plug_in_procedure_set_icon (GimpPlugInProcedure *proc,
                                 GimpIconType   icon_type,
                                 const guint8  *icon_data,
                                 gint           icon_data_length)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));
  g_return_if_fail (icon_type == -1 || icon_data != NULL);
  g_return_if_fail (icon_type == -1 || icon_data_length > 0);

  if (proc->icon_data)
    {
      g_free (proc->icon_data);
      proc->icon_data_length = -1;
      proc->icon_data        = NULL;
    }

  proc->icon_type = icon_type;

  switch (proc->icon_type)
    {
    case GIMP_ICON_TYPE_STOCK_ID:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      proc->icon_data_length = -1;
      proc->icon_data        = (guint8 *) g_strdup ((gchar *) icon_data);
      break;

    case GIMP_ICON_TYPE_INLINE_PIXBUF:
      proc->icon_data_length = icon_data_length;
      proc->icon_data        = g_memdup (icon_data, icon_data_length);
      break;
    }
}

const gchar *
gimp_plug_in_procedure_get_stock_id (const GimpPlugInProcedure *proc)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), NULL);

  switch (proc->icon_type)
    {
    case GIMP_ICON_TYPE_STOCK_ID:
      return (gchar *) proc->icon_data;

    default:
      return NULL;
    }
}

GdkPixbuf *
gimp_plug_in_procedure_get_pixbuf (const GimpPlugInProcedure *proc)
{
  GdkPixbuf *pixbuf = NULL;
  GError    *error  = NULL;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), NULL);

  switch (proc->icon_type)
    {
    case GIMP_ICON_TYPE_INLINE_PIXBUF:
      pixbuf = gdk_pixbuf_new_from_inline (proc->icon_data_length,
                                           proc->icon_data, TRUE, &error);
      break;

    case GIMP_ICON_TYPE_IMAGE_FILE:
      pixbuf = gdk_pixbuf_new_from_file ((gchar *) proc->icon_data,
                                         &error);
      break;

    default:
      break;
    }

  if (! pixbuf && error)
    {
      g_printerr (error->message);
      g_clear_error (&error);
    }

  return pixbuf;
}

gchar *
gimp_plug_in_procedure_get_help_id (const GimpPlugInProcedure *proc,
                                    const gchar               *help_domain)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), NULL);

  if (help_domain)
    return g_strconcat (help_domain, "?", GIMP_PROCEDURE (proc)->name, NULL);

  return g_strdup (GIMP_PROCEDURE (proc)->name);
}

gboolean
gimp_plug_in_procedure_get_sensitive (const GimpPlugInProcedure *proc,
                                      GimpImageType              image_type)
{
  gboolean sensitive;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), FALSE);

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
      sensitive = proc->image_types_val & PLUG_IN_RGB_IMAGE;
      break;
    case GIMP_RGBA_IMAGE:
      sensitive = proc->image_types_val & PLUG_IN_RGBA_IMAGE;
      break;
    case GIMP_GRAY_IMAGE:
      sensitive = proc->image_types_val & PLUG_IN_GRAY_IMAGE;
      break;
    case GIMP_GRAYA_IMAGE:
      sensitive = proc->image_types_val & PLUG_IN_GRAYA_IMAGE;
      break;
    case GIMP_INDEXED_IMAGE:
      sensitive = proc->image_types_val & PLUG_IN_INDEXED_IMAGE;
      break;
    case GIMP_INDEXEDA_IMAGE:
      sensitive = proc->image_types_val & PLUG_IN_INDEXEDA_IMAGE;
      break;
    default:
      sensitive = FALSE;
      break;
    }

  return sensitive ? TRUE : FALSE;
}

static PlugInImageType
image_types_parse (const gchar *image_types)
{
  const gchar     *type_spec = image_types;
  PlugInImageType  types     = 0;

  /*  If the plug_in registers with image_type == NULL or "", return 0
   *  By doing so it won't be touched by plug_in_set_menu_sensitivity()
   */
  if (! image_types)
    return types;

  while (*image_types)
    {
      while (*image_types &&
	     ((*image_types == ' ') ||
	      (*image_types == '\t') ||
	      (*image_types == ',')))
	image_types++;

      if (*image_types)
	{
	  if (strncmp (image_types, "RGBA", 4) == 0)
	    {
	      types |= PLUG_IN_RGBA_IMAGE;
	      image_types += 4;
	    }
	  else if (strncmp (image_types, "RGB*", 4) == 0)
	    {
	      types |= PLUG_IN_RGB_IMAGE | PLUG_IN_RGBA_IMAGE;
	      image_types += 4;
	    }
	  else if (strncmp (image_types, "RGB", 3) == 0)
	    {
	      types |= PLUG_IN_RGB_IMAGE;
	      image_types += 3;
	    }
	  else if (strncmp (image_types, "GRAYA", 5) == 0)
	    {
	      types |= PLUG_IN_GRAYA_IMAGE;
	      image_types += 5;
	    }
	  else if (strncmp (image_types, "GRAY*", 5) == 0)
	    {
	      types |= PLUG_IN_GRAY_IMAGE | PLUG_IN_GRAYA_IMAGE;
	      image_types += 5;
	    }
	  else if (strncmp (image_types, "GRAY", 4) == 0)
	    {
	      types |= PLUG_IN_GRAY_IMAGE;
	      image_types += 4;
	    }
	  else if (strncmp (image_types, "INDEXEDA", 8) == 0)
	    {
	      types |= PLUG_IN_INDEXEDA_IMAGE;
	      image_types += 8;
	    }
	  else if (strncmp (image_types, "INDEXED*", 8) == 0)
	    {
	      types |= PLUG_IN_INDEXED_IMAGE | PLUG_IN_INDEXEDA_IMAGE;
	      image_types += 8;
	    }
	  else if (strncmp (image_types, "INDEXED", 7) == 0)
	    {
	      types |= PLUG_IN_INDEXED_IMAGE;
	      image_types += 7;
	    }
	  else if (strncmp (image_types, "*", 1) == 0)
	    {
	      types |= PLUG_IN_RGB_IMAGE | PLUG_IN_RGBA_IMAGE
	             | PLUG_IN_GRAY_IMAGE | PLUG_IN_GRAYA_IMAGE
	             | PLUG_IN_INDEXED_IMAGE | PLUG_IN_INDEXEDA_IMAGE;
	      image_types += 1;
	    }
	  else
	    {
              g_printerr ("image_type contains unrecognizable parts: '%s'\n",
                          type_spec);

	      while (*image_types &&
                     ((*image_types != ' ') ||
                      (*image_types != '\t') ||
                      (*image_types != ',')))
		image_types++;
	    }
	}
    }

  return types;
}

void
gimp_plug_in_procedure_set_image_types (GimpPlugInProcedure *proc,
                                        const gchar         *image_types)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  if (proc->image_types)
    g_free (proc->image_types);

  proc->image_types = g_strdup (image_types);

  proc->image_types_val = image_types_parse (proc->image_types);
}

static GSList *
extensions_parse (gchar *extensions)
{
  GSList *list = NULL;

  /* EXTENSIONS can be NULL.  Avoid calling strtok if it is.  */
  if (extensions)
    {
      gchar *extension;
      gchar *next_token;

      extensions = g_strdup (extensions);

      next_token = extensions;
      extension = strtok (next_token, " \t,");

      while (extension)
	{
	  list = g_slist_prepend (list, g_strdup (extension));
	  extension = strtok (NULL, " \t,");
	}

      g_free (extensions);
    }

  return g_slist_reverse (list);
}

void
gimp_plug_in_procedure_set_file_proc (GimpPlugInProcedure *proc,
                                      const gchar         *extensions,
                                      const gchar         *prefixes,
                                      const gchar         *magics)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  proc->file_proc = TRUE;

  if (proc->extensions != extensions)
    {
      if (proc->extensions)
        g_free (proc->extensions);
      proc->extensions = g_strdup (extensions);
    }

  proc->extensions_list = extensions_parse (proc->extensions);

  if (proc->prefixes != prefixes)
    {
      if (proc->prefixes)
        g_free (proc->prefixes);
      proc->prefixes = g_strdup (prefixes);
    }

  proc->prefixes_list = extensions_parse (proc->prefixes);

  if (proc->magics != magics)
    {
      if (proc->magics)
        g_free (proc->magics);
      proc->magics = g_strdup (magics);
    }

  proc->magics_list = extensions_parse (proc->magics);
}

void
gimp_plug_in_procedure_set_mime_type (GimpPlugInProcedure *proc,
                                      const gchar         *mime_type)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  if (proc->mime_type)
    g_free (proc->mime_type);
  proc->mime_type = g_strdup (mime_type);
}

void
gimp_plug_in_procedure_set_thumb_loader (GimpPlugInProcedure *proc,
                                         const gchar         *thumb_loader)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  if (proc->thumb_loader)
    g_free (proc->thumb_loader);
  proc->thumb_loader = g_strdup (thumb_loader);
}
