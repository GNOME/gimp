/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapdbprocedure.c
 * Copyright (C) 2019 Michael Natterer <mitch@ligma.org>
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

#include "ligma.h"

#include "ligmapdb-private.h"
#include "ligmapdb_pdb.h"
#include "ligmapdbprocedure.h"


enum
{
  PROP_0,
  PROP_PDB,
  N_PROPS
};


struct _LigmaPDBProcedurePrivate
{
  LigmaPDB *pdb;
};


static void       ligma_pdb_procedure_constructed   (GObject              *object);
static void       ligma_pdb_procedure_finalize      (GObject              *object);
static void       ligma_pdb_procedure_set_property  (GObject              *object,
                                                    guint                 property_id,
                                                    const GValue         *value,
                                                    GParamSpec           *pspec);
static void       ligma_pdb_procedure_get_property  (GObject              *object,
                                                    guint                 property_id,
                                                    GValue               *value,
                                                    GParamSpec           *pspec);

static void       ligma_pdb_procedure_install       (LigmaProcedure        *procedure);
static void       ligma_pdb_procedure_uninstall     (LigmaProcedure        *procedure);
static LigmaValueArray *
                  ligma_pdb_procedure_run           (LigmaProcedure        *procedure,
                                                    const LigmaValueArray *args);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaPDBProcedure, _ligma_pdb_procedure,
                            LIGMA_TYPE_PROCEDURE)

#define parent_class _ligma_pdb_procedure_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
_ligma_pdb_procedure_class_init (LigmaPDBProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  LigmaProcedureClass *procedure_class = LIGMA_PROCEDURE_CLASS (klass);

  object_class->constructed  = ligma_pdb_procedure_constructed;
  object_class->finalize     = ligma_pdb_procedure_finalize;
  object_class->set_property = ligma_pdb_procedure_set_property;
  object_class->get_property = ligma_pdb_procedure_get_property;

  procedure_class->install   = ligma_pdb_procedure_install;
  procedure_class->uninstall = ligma_pdb_procedure_uninstall;
  procedure_class->run       = ligma_pdb_procedure_run;

  props[PROP_PDB] =
    g_param_spec_object ("pdb",
                         "PDB",
                         "The LigmaPDB of this plug-in process",
                         LIGMA_TYPE_PDB,
                         LIGMA_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
_ligma_pdb_procedure_init (LigmaPDBProcedure *procedure)
{
  procedure->priv = _ligma_pdb_procedure_get_instance_private (procedure);
}

static void
ligma_pdb_procedure_constructed (GObject *object)
{
  LigmaPDBProcedure *procedure = LIGMA_PDB_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (LIGMA_IS_PDB (procedure->priv->pdb));
}

static void
ligma_pdb_procedure_finalize (GObject *object)
{
  LigmaPDBProcedure *procedure = LIGMA_PDB_PROCEDURE (object);

  g_clear_object (&procedure->priv->pdb);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_pdb_procedure_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaPDBProcedure *procedure = LIGMA_PDB_PROCEDURE (object);

  switch (property_id)
    {
    case PROP_PDB:
      g_set_object (&procedure->priv->pdb, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_pdb_procedure_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaPDBProcedure *procedure = LIGMA_PDB_PROCEDURE (object);

  switch (property_id)
    {
    case PROP_PDB:
      g_value_set_object (value, procedure->priv->pdb);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_pdb_procedure_install (LigmaProcedure *procedure)
{
  g_warning ("Cannot install a LigmaPDBProcedure");
}

static void
ligma_pdb_procedure_uninstall (LigmaProcedure *procedure)
{
  g_warning ("Cannot uninstall a LigmaPDBProcedure");
}

static LigmaValueArray *
ligma_pdb_procedure_run (LigmaProcedure        *procedure,
                        const LigmaValueArray *args)
{
  LigmaPDBProcedure *pdb_procedure = LIGMA_PDB_PROCEDURE (procedure);

  return ligma_pdb_run_procedure_array (pdb_procedure->priv->pdb,
                                       ligma_procedure_get_name (procedure),
                                       (LigmaValueArray *) args);
}


/*  public functions  */

LigmaProcedure  *
_ligma_pdb_procedure_new (LigmaPDB     *pdb,
                         const gchar *name)
{
  LigmaProcedure   *procedure;
  gchar           *blurb;
  gchar           *help;
  gchar           *help_id;
  gchar           *authors;
  gchar           *copyright;
  gchar           *date;
  LigmaPDBProcType  type;
  gint             n_args;
  gint             n_return_vals;
  gint             i;

  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (name), NULL);

  _ligma_pdb_get_proc_info (name, &type, &n_args, &n_return_vals);

  procedure = g_object_new (LIGMA_TYPE_PDB_PROCEDURE,
                            "plug-in",        _ligma_pdb_get_plug_in (pdb),
                            "name",           name,
                            "procedure-type", type,
                            "pdb",            pdb,
                            NULL);

  _ligma_pdb_get_proc_documentation (name,      &blurb, &help, &help_id);
  ligma_procedure_set_documentation (procedure,  blurb,  help,  help_id);
  g_free (blurb);
  g_free (help);
  g_free (help_id);

  _ligma_pdb_get_proc_attribution (name,      &authors, &copyright, &date);
  ligma_procedure_set_attribution (procedure,  authors,  copyright,  date);
  g_free (authors);
  g_free (copyright);
  g_free (date);

  for (i = 0; i < n_args; i++)
    {
      GParamSpec *pspec = _ligma_pdb_get_proc_argument (name, i);

      ligma_procedure_add_argument (procedure, pspec);
    }

  for (i = 0; i < n_return_vals; i++)
    {
      GParamSpec *pspec = _ligma_pdb_get_proc_return_value (name, i);

      ligma_procedure_add_return_value (procedure, pspec);
    }

  if (type != LIGMA_PDB_PROC_TYPE_INTERNAL)
    {
      gchar  *string;
      gchar **menu_paths;
      gchar **path;

      string = _ligma_pdb_get_proc_image_types (name);
      if (string)
        {
          ligma_procedure_set_image_types (procedure, string);
          g_free (string);
        }

      string = _ligma_pdb_get_proc_menu_label (name);
      if (string)
        {
          ligma_procedure_set_menu_label (procedure, string);
          g_free (string);
        }

      menu_paths = _ligma_pdb_get_proc_menu_paths (name);
      for (path = menu_paths; path && *path; path++)
        ligma_procedure_add_menu_path (procedure, *path);
      g_strfreev (menu_paths);
    }

  return procedure;
}
