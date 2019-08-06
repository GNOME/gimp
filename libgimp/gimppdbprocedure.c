/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppdbprocedure.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#include "gimp.h"
#include "gimppdb-private.h"
#include "gimppdbprocedure.h"


struct _GimpPDBProcedurePrivate
{
  GimpPDB *pdb;
};


static GimpValueArray * gimp_pdb_procedure_run (GimpProcedure        *procedure,
                                                const GimpValueArray *args);


G_DEFINE_TYPE_WITH_PRIVATE (GimpPDBProcedure, _gimp_pdb_procedure,
                            GIMP_TYPE_PROCEDURE)

#define parent_class gimp_pdb_procedure_parent_class


static void
_gimp_pdb_procedure_class_init (GimpPDBProcedureClass *klass)
{
  GimpProcedureClass *procedure_class = GIMP_PROCEDURE_CLASS (klass);

  procedure_class->run = gimp_pdb_procedure_run;
}

static void
_gimp_pdb_procedure_init (GimpPDBProcedure *procedure)
{
  procedure->priv = _gimp_pdb_procedure_get_instance_private (procedure);
}

static GimpValueArray *
gimp_pdb_procedure_run (GimpProcedure        *procedure,
                        const GimpValueArray *args)
{
  return gimp_run_procedure_with_array (gimp_procedure_get_name (procedure),
                                        (GimpValueArray *) args);
}


/*  public functions  */

GimpProcedure  *
_gimp_pdb_procedure_new (GimpPDB     *pdb,
                         const gchar *name)
{
  GimpProcedure   *procedure;
  gchar           *blurb;
  gchar           *help;
  gchar           *authors;
  gchar           *copyright;
  gchar           *date;
  GimpPDBProcType  type;
  gint             n_params;
  gint             n_return_vals;
  gint             i;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  _gimp_pdb_proc_info (name,
                       &blurb,
                       &help,
                       &authors,
                       &copyright,
                       &date,
                       &type,
                       &n_params,
                       &n_return_vals);

  procedure = g_object_new (GIMP_TYPE_PDB_PROCEDURE,
                            "plug-in",        _gimp_pdb_get_plug_in (pdb),
                            "name",           name,
                            "procedure-type", type,
                            NULL);

  GIMP_PDB_PROCEDURE (procedure)->priv->pdb = pdb;

  gimp_procedure_set_documentation (procedure, blurb, help, name);
  gimp_procedure_set_attribution (procedure, authors, copyright, date);

  g_free (blurb);
  g_free (help);
  g_free (authors);
  g_free (copyright);
  g_free (date);

  for (i = 0; i < n_params; i++)
    {
      GParamSpec *pspec = gimp_pdb_proc_argument (name, i);

      gimp_procedure_add_argument (procedure, pspec);
      g_param_spec_unref (pspec);
    }

  for (i = 0; i < n_return_vals; i++)
    {
      GParamSpec *pspec = gimp_pdb_proc_return_value (name, i);

      gimp_procedure_add_return_value (procedure, pspec);
      g_param_spec_unref (pspec);
    }

  return procedure;
}
