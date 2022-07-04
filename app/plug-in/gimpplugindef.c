/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugindef.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "plug-in-types.h"

#include "core/gimp-memsize.h"

#include "gimpplugindef.h"
#include "gimppluginprocedure.h"


static void     gimp_plug_in_def_finalize    (GObject    *object);

static gint64   gimp_plug_in_def_get_memsize (GimpObject *object,
                                              gint64     *gui_size);


G_DEFINE_TYPE (GimpPlugInDef, gimp_plug_in_def, GIMP_TYPE_OBJECT)

#define parent_class gimp_plug_in_def_parent_class


static void
gimp_plug_in_def_class_init (GimpPlugInDefClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize         = gimp_plug_in_def_finalize;

  gimp_object_class->get_memsize = gimp_plug_in_def_get_memsize;
}

static void
gimp_plug_in_def_init (GimpPlugInDef *def)
{
}

static void
gimp_plug_in_def_finalize (GObject *object)
{
  GimpPlugInDef *plug_in_def = GIMP_PLUG_IN_DEF (object);

  g_object_unref (plug_in_def->file);
  g_free (plug_in_def->help_domain_name);
  g_free (plug_in_def->help_domain_uri);

  g_slist_free_full (plug_in_def->procedures, (GDestroyNotify) g_object_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_plug_in_def_get_memsize (GimpObject *object,
                              gint64     *gui_size)
{
  GimpPlugInDef *plug_in_def = GIMP_PLUG_IN_DEF (object);
  gint64         memsize     = 0;

  memsize += gimp_g_object_get_memsize (G_OBJECT (plug_in_def->file));
  memsize += gimp_string_get_memsize (plug_in_def->help_domain_name);
  memsize += gimp_string_get_memsize (plug_in_def->help_domain_uri);

  memsize += gimp_g_slist_get_memsize (plug_in_def->procedures, 0);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}


/*  public functions  */

GimpPlugInDef *
gimp_plug_in_def_new (GFile *file)
{
  GimpPlugInDef *plug_in_def;

  g_return_val_if_fail (G_IS_FILE (file), NULL);

  plug_in_def = g_object_new (GIMP_TYPE_PLUG_IN_DEF, NULL);

  plug_in_def->file = g_object_ref (file);

  return plug_in_def;
}

void
gimp_plug_in_def_add_procedure (GimpPlugInDef       *plug_in_def,
                                GimpPlugInProcedure *proc)
{
  GimpPlugInProcedure *overridden;

  g_return_if_fail (GIMP_IS_PLUG_IN_DEF (plug_in_def));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  overridden = gimp_plug_in_procedure_find (plug_in_def->procedures,
                                            gimp_object_get_name (proc));

  if (overridden)
    gimp_plug_in_def_remove_procedure (plug_in_def, overridden);

  proc->mtime = plug_in_def->mtime;

  gimp_plug_in_procedure_set_help_domain (proc,
                                          plug_in_def->help_domain_name);

  plug_in_def->procedures = g_slist_append (plug_in_def->procedures,
                                            g_object_ref (proc));
}

void
gimp_plug_in_def_remove_procedure (GimpPlugInDef       *plug_in_def,
                                   GimpPlugInProcedure *proc)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_DEF (plug_in_def));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  plug_in_def->procedures = g_slist_remove (plug_in_def->procedures, proc);
  g_object_unref (proc);
}

void
gimp_plug_in_def_set_help_domain (GimpPlugInDef *plug_in_def,
                                  const gchar   *domain_name,
                                  const gchar   *domain_uri)
{
  GSList *list;

  g_return_if_fail (GIMP_IS_PLUG_IN_DEF (plug_in_def));

  if (plug_in_def->help_domain_name)
    g_free (plug_in_def->help_domain_name);
  plug_in_def->help_domain_name = g_strdup (domain_name);

  if (plug_in_def->help_domain_uri)
    g_free (plug_in_def->help_domain_uri);
  plug_in_def->help_domain_uri = g_strdup (domain_uri);

  for (list = plug_in_def->procedures; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *procedure = list->data;

      gimp_plug_in_procedure_set_help_domain (procedure,
                                              plug_in_def->help_domain_name);
    }
}

void
gimp_plug_in_def_set_mtime (GimpPlugInDef *plug_in_def,
                            gint64         mtime)
{
  GSList *list;

  g_return_if_fail (GIMP_IS_PLUG_IN_DEF (plug_in_def));

  plug_in_def->mtime = mtime;

  for (list = plug_in_def->procedures; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *proc = list->data;

      proc->mtime = plug_in_def->mtime;
    }
}

void
gimp_plug_in_def_set_needs_query (GimpPlugInDef *plug_in_def,
                                  gboolean       needs_query)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_DEF (plug_in_def));

  plug_in_def->needs_query = needs_query ? TRUE : FALSE;
}

void
gimp_plug_in_def_set_has_init (GimpPlugInDef *plug_in_def,
                               gboolean       has_init)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_DEF (plug_in_def));

  plug_in_def->has_init = has_init ? TRUE : FALSE;
}
