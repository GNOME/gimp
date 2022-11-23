/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapdb.h
 * Copyright (C) 2019 Michael Natterer <mitch@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIGMA_PDB_H__
#define __LIGMA_PDB_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_PDB            (ligma_pdb_get_type ())
#define LIGMA_PDB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PDB, LigmaPDB))
#define LIGMA_PDB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PDB, LigmaPDBClass))
#define LIGMA_IS_PDB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PDB))
#define LIGMA_IS_PDB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PDB))
#define LIGMA_PDB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PDB, LigmaPDBClass))


typedef struct _LigmaPDBClass   LigmaPDBClass;
typedef struct _LigmaPDBPrivate LigmaPDBPrivate;

struct _LigmaPDB
{
  GObject         parent_instance;

  LigmaPDBPrivate *priv;
};

struct _LigmaPDBClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ligma_reserved1) (void);
  void (*_ligma_reserved2) (void);
  void (*_ligma_reserved3) (void);
  void (*_ligma_reserved4) (void);
  void (*_ligma_reserved5) (void);
  void (*_ligma_reserved6) (void);
  void (*_ligma_reserved7) (void);
  void (*_ligma_reserved8) (void);
};


GType            ligma_pdb_get_type             (void) G_GNUC_CONST;

gboolean         ligma_pdb_procedure_exists     (LigmaPDB              *pdb,
                                                const gchar          *procedure_name);

LigmaProcedure  * ligma_pdb_lookup_procedure     (LigmaPDB              *pdb,
                                                const gchar          *procedure_name);

LigmaValueArray * ligma_pdb_run_procedure        (LigmaPDB              *pdb,
                                                const gchar          *procedure_name,
                                                GType                 first_type,
                                                ...);
LigmaValueArray * ligma_pdb_run_procedure_valist (LigmaPDB              *pdb,
                                                const gchar          *procedure_name,
                                                GType                 first_type,
                                                va_list               args);
LigmaValueArray * ligma_pdb_run_procedure_argv   (LigmaPDB              *pdb,
                                                const gchar          *procedure_name,
                                                const GValue         *arguments,
                                                gint                  n_arguments);
LigmaValueArray * ligma_pdb_run_procedure_array  (LigmaPDB              *pdb,
                                                const gchar          *procedure_name,
                                                const LigmaValueArray *arguments);
LigmaValueArray * ligma_pdb_run_procedure_config (LigmaPDB              *pdb,
                                                const gchar          *procedure_name,
                                                LigmaProcedureConfig  *config);

gchar          * ligma_pdb_temp_procedure_name  (LigmaPDB              *pdb);

gboolean         ligma_pdb_dump_to_file         (LigmaPDB              *pdb,
                                                GFile                *file);
gchar         ** ligma_pdb_query_procedures     (LigmaPDB              *pdb,
                                                const gchar          *name,
                                                const gchar          *blurb,
                                                const gchar          *help,
                                                const gchar          *help_id,
                                                const gchar          *authors,
                                                const gchar          *copyright,
                                                const gchar          *date,
                                                const gchar          *proc_type);

const gchar       * ligma_pdb_get_last_error    (LigmaPDB              *pdb);
LigmaPDBStatusType   ligma_pdb_get_last_status   (LigmaPDB              *pdb);


/*  Cruft API  */

gboolean   ligma_pdb_get_data      (const gchar      *identifier,
                                   gpointer          data);
gint       ligma_pdb_get_data_size (const gchar      *identifier);
gboolean   ligma_pdb_set_data      (const gchar      *identifier,
                                   gconstpointer     data,
                                   guint32           bytes);


G_END_DECLS

#endif  /*  __LIGMA_PDB_H__  */
