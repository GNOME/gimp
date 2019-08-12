/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppdb.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_PDB_H__
#define __GIMP_PDB_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_PDB            (gimp_pdb_get_type ())
#define GIMP_PDB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PDB, GimpPDB))
#define GIMP_PDB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PDB, GimpPDBClass))
#define GIMP_IS_PDB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PDB))
#define GIMP_IS_PDB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PDB))
#define GIMP_PDB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PDB, GimpPDBClass))


typedef struct _GimpPDBClass   GimpPDBClass;
typedef struct _GimpPDBPrivate GimpPDBPrivate;

struct _GimpPDB
{
  GObject         parent_instance;

  GimpPDBPrivate *priv;
};

struct _GimpPDBClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
  void (*_gimp_reserved5) (void);
  void (*_gimp_reserved6) (void);
  void (*_gimp_reserved7) (void);
  void (*_gimp_reserved8) (void);
};


GType            gimp_pdb_get_type             (void) G_GNUC_CONST;

gboolean         gimp_pdb_procedure_exists     (GimpPDB              *pdb,
                                                const gchar          *procedure_name);

GimpProcedure  * gimp_pdb_lookup_procedure     (GimpPDB              *pdb,
                                                const gchar          *procedure_name);

GimpValueArray * gimp_pdb_run_procedure        (GimpPDB              *pdb,
                                                const gchar          *procedure_name,
                                                GType                 first_type,
                                                ...);
GimpValueArray * gimp_pdb_run_procedure_valist (GimpPDB              *pdb,
                                                const gchar          *procedure_name,
                                                GType                 first_type,
                                                va_list               args);
GimpValueArray * gimp_pdb_run_procedure_array  (GimpPDB              *pdb,
                                                const gchar          *procedure_name,
                                                const GimpValueArray *arguments);

gchar          * gimp_pdb_temp_procedure_name  (GimpPDB              *pdb);

gboolean         gimp_pdb_dump_to_file         (GimpPDB              *pdb,
                                                GFile                *file);
gchar         ** gimp_pdb_query_procedures     (GimpPDB              *pdb,
                                                const gchar          *name,
                                                const gchar          *blurb,
                                                const gchar          *help,
                                                const gchar          *help_id,
                                                const gchar          *authors,
                                                const gchar          *copyright,
                                                const gchar          *date,
                                                const gchar          *proc_type,
                                                gint                 *num_matches);

const gchar       * gimp_pdb_get_last_error    (GimpPDB              *pdb);
GimpPDBStatusType   gimp_pdb_get_last_status   (GimpPDB              *pdb);


/*  Cruft API  */

gboolean   gimp_pdb_get_data      (const gchar      *identifier,
                                   gpointer          data);
gint       gimp_pdb_get_data_size (const gchar      *identifier);
gboolean   gimp_pdb_set_data      (const gchar      *identifier,
                                   gconstpointer     data,
                                   guint32           bytes);


G_END_DECLS

#endif  /*  __GIMP_PDB_H__  */
