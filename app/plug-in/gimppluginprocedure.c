/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginprocedure.c
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"

#include "plug-in-types.h"

#include "gegl/gimp-babl-compat.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpdrawable.h"
#include "core/gimpmarshal.h"
#include "core/gimpparamspecs.h"

#define __YES_I_NEED_GIMP_PLUG_IN_MANAGER_CALL__
#include "gimppluginmanager-call.h"

#include "gimppluginerror.h"
#include "gimppluginprocedure.h"
#include "plug-in-menu-path.h"

#include "gimp-intl.h"


enum
{
  MENU_PATH_ADDED,
  LAST_SIGNAL
};


static void             gimp_plug_in_procedure_finalize    (GObject        *object);

static gint64           gimp_plug_in_procedure_get_memsize (GimpObject     *object,
                                                            gint64         *gui_size);

static GimpValueArray * gimp_plug_in_procedure_execute     (GimpProcedure  *procedure,
                                                            Gimp           *gimp,
                                                            GimpContext    *context,
                                                            GimpProgress   *progress,
                                                            GimpValueArray *args,
                                                            GError        **error);
static void          gimp_plug_in_procedure_execute_async  (GimpProcedure  *procedure,
                                                            Gimp           *gimp,
                                                            GimpContext    *context,
                                                            GimpProgress   *progress,
                                                            GimpValueArray *args,
                                                            GimpObject     *display);

const gchar     * gimp_plug_in_procedure_real_get_progname (const GimpPlugInProcedure *procedure);


G_DEFINE_TYPE (GimpPlugInProcedure, gimp_plug_in_procedure,
               GIMP_TYPE_PROCEDURE)

#define parent_class gimp_plug_in_procedure_parent_class

static guint gimp_plug_in_procedure_signals[LAST_SIGNAL] = { 0 };


static void
gimp_plug_in_procedure_class_init (GimpPlugInProcedureClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass    *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpProcedureClass *proc_class        = GIMP_PROCEDURE_CLASS (klass);

  gimp_plug_in_procedure_signals[MENU_PATH_ADDED] =
    g_signal_new ("menu-path-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPlugInProcedureClass, menu_path_added),
                  NULL, NULL,
                  gimp_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  object_class->finalize         = gimp_plug_in_procedure_finalize;

  gimp_object_class->get_memsize = gimp_plug_in_procedure_get_memsize;

  proc_class->execute            = gimp_plug_in_procedure_execute;
  proc_class->execute_async      = gimp_plug_in_procedure_execute_async;

  klass->get_progname            = gimp_plug_in_procedure_real_get_progname;
  klass->menu_path_added         = NULL;
}

static void
gimp_plug_in_procedure_init (GimpPlugInProcedure *proc)
{
  GIMP_PROCEDURE (proc)->proc_type = GIMP_PLUGIN;

  proc->label            = NULL;
  proc->icon_data_length = -1;
}

static void
gimp_plug_in_procedure_finalize (GObject *object)
{
  GimpPlugInProcedure *proc = GIMP_PLUG_IN_PROCEDURE (object);

  g_free (proc->prog);
  g_free (proc->menu_label);

  g_list_free_full (proc->menu_paths, (GDestroyNotify) g_free);

  g_free (proc->label);

  g_free (proc->icon_data);
  g_free (proc->image_types);

  g_free (proc->extensions);
  g_free (proc->prefixes);
  g_free (proc->magics);
  g_free (proc->mime_type);

  g_slist_free_full (proc->extensions_list, (GDestroyNotify) g_free);
  g_slist_free_full (proc->prefixes_list, (GDestroyNotify) g_free);
  g_slist_free_full (proc->magics_list, (GDestroyNotify) g_free);

  g_free (proc->thumb_loader);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_plug_in_procedure_get_memsize (GimpObject *object,
                                    gint64     *gui_size)
{
  GimpPlugInProcedure *proc    = GIMP_PLUG_IN_PROCEDURE (object);
  gint64               memsize = 0;
  GList               *list;
  GSList              *slist;

  memsize += gimp_string_get_memsize (proc->prog);
  memsize += gimp_string_get_memsize (proc->menu_label);

  for (list = proc->menu_paths; list; list = g_list_next (list))
    memsize += sizeof (GList) + gimp_string_get_memsize (list->data);

  switch (proc->icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      memsize += gimp_string_get_memsize ((const gchar *) proc->icon_data);
      break;

    case GIMP_ICON_TYPE_INLINE_PIXBUF:
      memsize += proc->icon_data_length;
      break;
    }

  memsize += gimp_string_get_memsize (proc->extensions);
  memsize += gimp_string_get_memsize (proc->prefixes);
  memsize += gimp_string_get_memsize (proc->magics);
  memsize += gimp_string_get_memsize (proc->mime_type);
  memsize += gimp_string_get_memsize (proc->thumb_loader);

  for (slist = proc->extensions_list; slist; slist = g_slist_next (slist))
    memsize += sizeof (GSList) + gimp_string_get_memsize (slist->data);

  for (slist = proc->prefixes_list; slist; slist = g_slist_next (slist))
    memsize += sizeof (GSList) + gimp_string_get_memsize (slist->data);

  for (slist = proc->magics_list; slist; slist = g_slist_next (slist))
    memsize += sizeof (GSList) + gimp_string_get_memsize (slist->data);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static GimpValueArray *
gimp_plug_in_procedure_execute (GimpProcedure  *procedure,
                                Gimp           *gimp,
                                GimpContext    *context,
                                GimpProgress   *progress,
                                GimpValueArray *args,
                                GError        **error)
{
  if (procedure->proc_type == GIMP_INTERNAL)
    return GIMP_PROCEDURE_CLASS (parent_class)->execute (procedure, gimp,
                                                         context, progress,
                                                         args, error);

  return gimp_plug_in_manager_call_run (gimp->plug_in_manager,
                                        context, progress,
                                        GIMP_PLUG_IN_PROCEDURE (procedure),
                                        args, TRUE, NULL);
}

static void
gimp_plug_in_procedure_execute_async (GimpProcedure  *procedure,
                                      Gimp           *gimp,
                                      GimpContext    *context,
                                      GimpProgress   *progress,
                                      GimpValueArray *args,
                                      GimpObject     *display)
{
  GimpPlugInProcedure *plug_in_procedure = GIMP_PLUG_IN_PROCEDURE (procedure);
  GimpValueArray      *return_vals;

  return_vals = gimp_plug_in_manager_call_run (gimp->plug_in_manager,
                                               context, progress,
                                               plug_in_procedure,
                                               args, FALSE, display);

  if (return_vals)
    {
      gimp_plug_in_procedure_handle_return_values (plug_in_procedure,
                                                   gimp, progress,
                                                   return_vals);
      gimp_value_array_unref (return_vals);
    }
}

const gchar *
gimp_plug_in_procedure_real_get_progname (const GimpPlugInProcedure *procedure)
{
  return procedure->prog;
}


/*  public functions  */

GimpProcedure *
gimp_plug_in_procedure_new (GimpPDBProcType  proc_type,
                            const gchar     *prog)
{
  GimpPlugInProcedure *proc;

  g_return_val_if_fail (proc_type == GIMP_PLUGIN ||
                        proc_type == GIMP_EXTENSION, NULL);
  g_return_val_if_fail (prog != NULL, NULL);

  proc = g_object_new (GIMP_TYPE_PLUG_IN_PROCEDURE, NULL);

  proc->prog = g_strdup (prog);

  GIMP_PROCEDURE (proc)->proc_type = proc_type;

  return GIMP_PROCEDURE (proc);
}

GimpPlugInProcedure *
gimp_plug_in_procedure_find (GSList      *list,
                             const gchar *proc_name)
{
  GSList *l;

  for (l = list; l; l = g_slist_next (l))
    {
      GimpObject *object = l->data;

      if (! strcmp (proc_name, gimp_object_get_name (object)))
        return GIMP_PLUG_IN_PROCEDURE (object);
    }

  return NULL;
}

const gchar *
gimp_plug_in_procedure_get_progname (const GimpPlugInProcedure *proc)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), NULL);

  return GIMP_PLUG_IN_PROCEDURE_GET_CLASS (proc)->get_progname (proc);
}

void
gimp_plug_in_procedure_set_locale_domain (GimpPlugInProcedure *proc,
                                          const gchar         *locale_domain)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  proc->locale_domain = locale_domain ? g_quark_from_string (locale_domain) : 0;
}

const gchar *
gimp_plug_in_procedure_get_locale_domain (const GimpPlugInProcedure *proc)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), NULL);

  return g_quark_to_string (proc->locale_domain);
}

void
gimp_plug_in_procedure_set_help_domain (GimpPlugInProcedure *proc,
                                        const gchar         *help_domain)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  proc->help_domain = help_domain ? g_quark_from_string (help_domain) : 0;
}

const gchar *
gimp_plug_in_procedure_get_help_domain (const GimpPlugInProcedure *proc)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), NULL);

  return g_quark_to_string (proc->help_domain);
}

gboolean
gimp_plug_in_procedure_add_menu_path (GimpPlugInProcedure  *proc,
                                      const gchar          *menu_path,
                                      GError              **error)
{
  GimpProcedure *procedure;
  gchar         *basename = NULL;
  const gchar   *required = NULL;
  gchar         *p;
  gchar         *mapped_path;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), FALSE);
  g_return_val_if_fail (menu_path != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  procedure = GIMP_PROCEDURE (proc);

  p = strchr (menu_path, '>');
  if (p == NULL || (*(++p) && *p != '/'))
    {
      basename = g_filename_display_basename (proc->prog);

      g_set_error (error, GIMP_PLUG_IN_ERROR, GIMP_PLUG_IN_FAILED,
                   "Plug-In \"%s\"\n(%s)\n"
                   "attempted to install procedure \"%s\"\n"
                   "in the invalid menu location \"%s\".\n"
                   "The menu path must look like either \"<Prefix>\" "
                   "or \"<Prefix>/path/to/item\".",
                   basename, gimp_filename_to_utf8 (proc->prog),
                   gimp_object_get_name (proc),
                   menu_path);
      goto failure;
    }

  if (g_str_has_prefix (menu_path, "<Toolbox>") ||
      g_str_has_prefix (menu_path, "<Image>"))
    {
      if ((procedure->num_args < 1) ||
          ! GIMP_IS_PARAM_SPEC_INT32 (procedure->args[0]))
        {
          required = "INT32";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Layers>"))
    {
      if ((procedure->num_args < 3)                             ||
          ! GIMP_IS_PARAM_SPEC_INT32       (procedure->args[0]) ||
          ! GIMP_IS_PARAM_SPEC_IMAGE_ID    (procedure->args[1]) ||
          ! (G_TYPE_FROM_INSTANCE (procedure->args[2])
                               == GIMP_TYPE_PARAM_LAYER_ID ||
             G_TYPE_FROM_INSTANCE (procedure->args[2])
                               == GIMP_TYPE_PARAM_DRAWABLE_ID))
        {
          required = "INT32, IMAGE, (LAYER | DRAWABLE)";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Channels>"))
    {
      if ((procedure->num_args < 3)                             ||
          ! GIMP_IS_PARAM_SPEC_INT32       (procedure->args[0]) ||
          ! GIMP_IS_PARAM_SPEC_IMAGE_ID    (procedure->args[1]) ||
          ! (G_TYPE_FROM_INSTANCE (procedure->args[2])
                               == GIMP_TYPE_PARAM_CHANNEL_ID ||
             G_TYPE_FROM_INSTANCE (procedure->args[2])
                               == GIMP_TYPE_PARAM_DRAWABLE_ID))
        {
          required = "INT32, IMAGE, (CHANNEL | DRAWABLE)";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Vectors>"))
    {
      if ((procedure->num_args < 3)                            ||
          ! GIMP_IS_PARAM_SPEC_INT32      (procedure->args[0]) ||
          ! GIMP_IS_PARAM_SPEC_IMAGE_ID   (procedure->args[1]) ||
          ! GIMP_IS_PARAM_SPEC_VECTORS_ID (procedure->args[2]))
        {
          required = "INT32, IMAGE, VECTORS";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Colormap>"))
    {
      if ((procedure->num_args < 2)                            ||
          ! GIMP_IS_PARAM_SPEC_INT32      (procedure->args[0]) ||
          ! GIMP_IS_PARAM_SPEC_IMAGE_ID   (procedure->args[1]))
        {
          required = "INT32, IMAGE";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Load>"))
    {
      if ((procedure->num_args < 3)                       ||
          ! GIMP_IS_PARAM_SPEC_INT32 (procedure->args[0]) ||
          ! G_IS_PARAM_SPEC_STRING   (procedure->args[1]) ||
          ! G_IS_PARAM_SPEC_STRING   (procedure->args[2]))
        {
          required = "INT32, STRING, STRING";
          goto failure;
        }

      if ((procedure->num_values < 1) ||
          ! GIMP_IS_PARAM_SPEC_IMAGE_ID (procedure->values[0]))
        {
          required = "IMAGE";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Save>"))
    {
      if ((procedure->num_args < 5)                             ||
          ! GIMP_IS_PARAM_SPEC_INT32       (procedure->args[0]) ||
          ! GIMP_IS_PARAM_SPEC_IMAGE_ID    (procedure->args[1]) ||
          ! GIMP_IS_PARAM_SPEC_DRAWABLE_ID (procedure->args[2]) ||
          ! G_IS_PARAM_SPEC_STRING         (procedure->args[3]) ||
          ! G_IS_PARAM_SPEC_STRING         (procedure->args[4]))
        {
          required = "INT32, IMAGE, DRAWABLE, STRING, STRING";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Brushes>")     ||
           g_str_has_prefix (menu_path, "<Dynamics>")    ||
           g_str_has_prefix (menu_path, "<Gradients>")   ||
           g_str_has_prefix (menu_path, "<Palettes>")    ||
           g_str_has_prefix (menu_path, "<Patterns>")    ||
           g_str_has_prefix (menu_path, "<ToolPresets>") ||
           g_str_has_prefix (menu_path, "<Fonts>")       ||
           g_str_has_prefix (menu_path, "<Buffers>"))
    {
      if ((procedure->num_args < 1) ||
          ! GIMP_IS_PARAM_SPEC_INT32 (procedure->args[0]))
        {
          required = "INT32";
          goto failure;
        }
    }
  else
    {
      basename = g_filename_display_basename (proc->prog);

      g_set_error (error, GIMP_PLUG_IN_ERROR, GIMP_PLUG_IN_FAILED,
                   "Plug-In \"%s\"\n(%s)\n"
                   "attempted to install procedure \"%s\" "
                   "in the invalid menu location \"%s\".\n"
                   "Use either \"<Image>\", "
                   "\"<Layers>\", \"<Channels>\", \"<Vectors>\", "
                   "\"<Colormap>\", \"<Brushes>\", \"<Dynamics>\", "
                   "\"<Gradients>\", \"<Palettes>\", \"<Patterns>\", "
                   "\"<ToolPresets>\", \"<Fonts>\" or \"<Buffers>\".",
                   basename, gimp_filename_to_utf8 (proc->prog),
                   gimp_object_get_name (proc),
                   menu_path);
      goto failure;
    }

  g_free (basename);

  mapped_path = plug_in_menu_path_map (menu_path, NULL);

  proc->menu_paths = g_list_append (proc->menu_paths, mapped_path);

  g_signal_emit (proc, gimp_plug_in_procedure_signals[MENU_PATH_ADDED], 0,
                 mapped_path);

  return TRUE;

 failure:
  if (required)
    {
      gchar *prefix = g_strdup (menu_path);

      p = strchr (prefix, '>') + 1;
      *p = '\0';

      basename = g_filename_display_basename (proc->prog);

      g_set_error (error, GIMP_PLUG_IN_ERROR, GIMP_PLUG_IN_FAILED,
                   "Plug-In \"%s\"\n(%s)\n\n"
                   "attempted to install %s procedure \"%s\" "
                   "which does not take the standard %s Plug-In "
                   "arguments: (%s).",
                   basename, gimp_filename_to_utf8 (proc->prog),
                   prefix, gimp_object_get_name (proc), prefix,
                   required);

      g_free (prefix);
    }

  g_free (basename);

  return FALSE;
}

const gchar *
gimp_plug_in_procedure_get_label (GimpPlugInProcedure *proc)
{
  const gchar *path;
  gchar       *stripped;
  gchar       *ellipsis;
  gchar       *label;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), NULL);

  if (proc->label)
    return proc->label;

  if (proc->menu_label)
    path = dgettext (gimp_plug_in_procedure_get_locale_domain (proc),
                     proc->menu_label);
  else if (proc->menu_paths)
    path = dgettext (gimp_plug_in_procedure_get_locale_domain (proc),
                     proc->menu_paths->data);
  else
    return NULL;

  stripped = gimp_strip_uline (path);

  if (proc->menu_label)
    label = g_strdup (stripped);
  else
    label = g_path_get_basename (stripped);

  g_free (stripped);

  ellipsis = strstr (label, "...");

  if (! ellipsis)
    ellipsis = strstr (label, "\342\200\246" /* U+2026 HORIZONTAL ELLIPSIS */);

  if (ellipsis && ellipsis == (label + strlen (label) - 3))
    *ellipsis = '\0';

  proc->label = label;

  return proc->label;
}

const gchar *
gimp_plug_in_procedure_get_blurb (const GimpPlugInProcedure *proc)
{
  GimpProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), NULL);

  procedure = GIMP_PROCEDURE (proc);

  /*  do not to pass the empty string to gettext()  */
  if (procedure->blurb && strlen (procedure->blurb))
    return dgettext (gimp_plug_in_procedure_get_locale_domain (proc),
                     procedure->blurb);

  return NULL;
}

void
gimp_plug_in_procedure_set_icon (GimpPlugInProcedure *proc,
                                 GimpIconType         icon_type,
                                 const guint8        *icon_data,
                                 gint                 icon_data_length)
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
    case GIMP_ICON_TYPE_ICON_NAME:
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
gimp_plug_in_procedure_get_icon_name (const GimpPlugInProcedure *proc)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), NULL);

  switch (proc->icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
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
      g_printerr ("%s\n", error->message);
      g_clear_error (&error);
    }

  return pixbuf;
}

gchar *
gimp_plug_in_procedure_get_help_id (const GimpPlugInProcedure *proc)
{
  const gchar *domain;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), NULL);

  domain = gimp_plug_in_procedure_get_help_domain (proc);

  if (domain)
    return g_strconcat (domain, "?", gimp_object_get_name (proc), NULL);

  return g_strdup (gimp_object_get_name (proc));
}

gboolean
gimp_plug_in_procedure_get_sensitive (const GimpPlugInProcedure *proc,
                                      GimpDrawable              *drawable)
{
  GimpImageType image_type = -1;
  gboolean      sensitive  = FALSE;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc), FALSE);
  g_return_val_if_fail (drawable == NULL || GIMP_IS_DRAWABLE (drawable), FALSE);

  if (drawable)
    {
      const Babl *format = gimp_drawable_get_format (drawable);

      image_type = gimp_babl_format_get_image_type (format);
    }

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
      sensitive = proc->image_types_val & GIMP_PLUG_IN_RGB_IMAGE;
      break;
    case GIMP_RGBA_IMAGE:
      sensitive = proc->image_types_val & GIMP_PLUG_IN_RGBA_IMAGE;
      break;
    case GIMP_GRAY_IMAGE:
      sensitive = proc->image_types_val & GIMP_PLUG_IN_GRAY_IMAGE;
      break;
    case GIMP_GRAYA_IMAGE:
      sensitive = proc->image_types_val & GIMP_PLUG_IN_GRAYA_IMAGE;
      break;
    case GIMP_INDEXED_IMAGE:
      sensitive = proc->image_types_val & GIMP_PLUG_IN_INDEXED_IMAGE;
      break;
    case GIMP_INDEXEDA_IMAGE:
      sensitive = proc->image_types_val & GIMP_PLUG_IN_INDEXEDA_IMAGE;
      break;
    default:
      break;
    }

  return sensitive ? TRUE : FALSE;
}

static GimpPlugInImageType
image_types_parse (const gchar *name,
                   const gchar *image_types)
{
  const gchar         *type_spec = image_types;
  GimpPlugInImageType  types     = 0;

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
          if (g_str_has_prefix (image_types, "RGBA"))
            {
              types |= GIMP_PLUG_IN_RGBA_IMAGE;
              image_types += strlen ("RGBA");
            }
          else if (g_str_has_prefix (image_types, "RGB*"))
            {
              types |= GIMP_PLUG_IN_RGB_IMAGE | GIMP_PLUG_IN_RGBA_IMAGE;
              image_types += strlen ("RGB*");
            }
          else if (g_str_has_prefix (image_types, "RGB"))
            {
              types |= GIMP_PLUG_IN_RGB_IMAGE;
              image_types += strlen ("RGB");
            }
          else if (g_str_has_prefix (image_types, "GRAYA"))
            {
              types |= GIMP_PLUG_IN_GRAYA_IMAGE;
              image_types += strlen ("GRAYA");
            }
          else if (g_str_has_prefix (image_types, "GRAY*"))
            {
              types |= GIMP_PLUG_IN_GRAY_IMAGE | GIMP_PLUG_IN_GRAYA_IMAGE;
              image_types += strlen ("GRAY*");
            }
          else if (g_str_has_prefix (image_types, "GRAY"))
            {
              types |= GIMP_PLUG_IN_GRAY_IMAGE;
              image_types += strlen ("GRAY");
            }
          else if (g_str_has_prefix (image_types, "INDEXEDA"))
            {
              types |= GIMP_PLUG_IN_INDEXEDA_IMAGE;
              image_types += strlen ("INDEXEDA");
            }
          else if (g_str_has_prefix (image_types, "INDEXED*"))
            {
              types |= GIMP_PLUG_IN_INDEXED_IMAGE | GIMP_PLUG_IN_INDEXEDA_IMAGE;
              image_types += strlen ("INDEXED*");
            }
          else if (g_str_has_prefix (image_types, "INDEXED"))
            {
              types |= GIMP_PLUG_IN_INDEXED_IMAGE;
              image_types += strlen ("INDEXED");
            }
          else if (g_str_has_prefix (image_types, "*"))
            {
              types |= (GIMP_PLUG_IN_RGB_IMAGE     | GIMP_PLUG_IN_RGBA_IMAGE  |
                        GIMP_PLUG_IN_GRAY_IMAGE    | GIMP_PLUG_IN_GRAYA_IMAGE |
                        GIMP_PLUG_IN_INDEXED_IMAGE | GIMP_PLUG_IN_INDEXEDA_IMAGE);
              image_types += strlen ("*");
            }
          else
            {
              g_printerr ("%s: image-type contains unrecognizable parts:"
                          "'%s'\n", name, type_spec);

              while (*image_types &&
                     ((*image_types != ' ') ||
                      (*image_types != '\t') ||
                      (*image_types != ',')))
                {
                  image_types++;
                }
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

  proc->image_types     = g_strdup (image_types);
  proc->image_types_val = image_types_parse (gimp_object_get_name (proc),
                                             proc->image_types);
}

static GSList *
extensions_parse (gchar *extensions)
{
  GSList *list = NULL;

  /*  extensions can be NULL.  Avoid calling strtok if it is.  */
  if (extensions)
    {
      gchar *extension;
      gchar *next_token;

      /*  work on a copy  */
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
  GSList *list;

  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  proc->file_proc = TRUE;

  /*  extensions  */

  if (proc->extensions != extensions)
    {
      if (proc->extensions)
        g_free (proc->extensions);

      proc->extensions = g_strdup (extensions);
    }

  if (proc->extensions_list)
    g_slist_free_full (proc->extensions_list, (GDestroyNotify) g_free);

  proc->extensions_list = extensions_parse (proc->extensions);

  /*  prefixes  */

  if (proc->prefixes != prefixes)
    {
      if (proc->prefixes)
        g_free (proc->prefixes);

      proc->prefixes = g_strdup (prefixes);
    }

  if (proc->prefixes_list)
    g_slist_free_full (proc->prefixes_list, (GDestroyNotify) g_free);

  proc->prefixes_list = extensions_parse (proc->prefixes);

  /* don't allow "file:" to be registered as prefix */
  for (list = proc->prefixes_list; list; list = g_slist_next (list))
    {
      const gchar *prefix = list->data;

      if (prefix && strcmp (prefix, "file:") == 0)
        {
          g_free (list->data);
          proc->prefixes_list = g_slist_delete_link (proc->prefixes_list, list);
          break;
        }
    }

  /*  magics  */

  if (proc->magics != magics)
    {
      if (proc->magics)
        g_free (proc->magics);

      proc->magics = g_strdup (magics);
    }

  if (proc->magics_list)
    g_slist_free_full (proc->magics_list, (GDestroyNotify) g_free);

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
gimp_plug_in_procedure_set_handles_uri (GimpPlugInProcedure *proc)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  proc->handles_uri = TRUE;
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

void
gimp_plug_in_procedure_handle_return_values (GimpPlugInProcedure *proc,
                                             Gimp                *gimp,
                                             GimpProgress        *progress,
                                             GimpValueArray      *return_vals)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));
  g_return_if_fail (return_vals != NULL);

  if (! gimp_value_array_length (return_vals) > 0 ||
      G_VALUE_TYPE (gimp_value_array_index (return_vals, 0)) !=
      GIMP_TYPE_PDB_STATUS_TYPE)
    {
      return;
    }

  switch (g_value_get_enum (gimp_value_array_index (return_vals, 0)))
    {
    case GIMP_PDB_SUCCESS:
      break;

    case GIMP_PDB_CALLING_ERROR:
      if (gimp_value_array_length (return_vals) > 1 &&
          G_VALUE_HOLDS_STRING (gimp_value_array_index (return_vals, 1)))
        {
          gimp_message (gimp, G_OBJECT (progress), GIMP_MESSAGE_ERROR,
                        _("Calling error for '%s':\n"
                          "%s"),
                        gimp_plug_in_procedure_get_label (proc),
                        g_value_get_string (gimp_value_array_index (return_vals, 1)));
        }
      break;

    case GIMP_PDB_EXECUTION_ERROR:
      if (gimp_value_array_length (return_vals) > 1 &&
          G_VALUE_HOLDS_STRING (gimp_value_array_index (return_vals, 1)))
        {
          gimp_message (gimp, G_OBJECT (progress), GIMP_MESSAGE_ERROR,
                        _("Execution error for '%s':\n"
                          "%s"),
                        gimp_plug_in_procedure_get_label (proc),
                        g_value_get_string (gimp_value_array_index (return_vals, 1)));
        }
      break;
    }
}
