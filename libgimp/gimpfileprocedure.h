/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafileprocedure.h
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

#ifndef __LIGMA_FILE_PROCEDURE_H__
#define __LIGMA_FILE_PROCEDURE_H__

#include <libligma/ligmaprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_FILE_PROCEDURE            (ligma_file_procedure_get_type ())
#define LIGMA_FILE_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FILE_PROCEDURE, LigmaFileProcedure))
#define LIGMA_FILE_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FILE_PROCEDURE, LigmaFileProcedureClass))
#define LIGMA_IS_FILE_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FILE_PROCEDURE))
#define LIGMA_IS_FILE_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FILE_PROCEDURE))
#define LIGMA_FILE_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FILE_PROCEDURE, LigmaFileProcedureClass))


typedef struct _LigmaFileProcedure        LigmaFileProcedure;
typedef struct _LigmaFileProcedureClass   LigmaFileProcedureClass;
typedef struct _LigmaFileProcedurePrivate LigmaFileProcedurePrivate;

struct _LigmaFileProcedure
{
  LigmaProcedure             parent_instance;

  LigmaFileProcedurePrivate *priv;
};

struct _LigmaFileProcedureClass
{
  LigmaProcedureClass parent_class;
};


GType           ligma_file_procedure_get_type        (void) G_GNUC_CONST;

void            ligma_file_procedure_set_format_name (LigmaFileProcedure *procedure,
                                                     const gchar       *format_name);
const gchar   * ligma_file_procedure_get_format_name (LigmaFileProcedure *procedure);

void            ligma_file_procedure_set_mime_types  (LigmaFileProcedure *procedure,
                                                     const gchar       *mime_types);
const gchar   * ligma_file_procedure_get_mime_types  (LigmaFileProcedure *procedure);

void            ligma_file_procedure_set_extensions  (LigmaFileProcedure *procedure,
                                                     const gchar       *extensions);
const gchar   * ligma_file_procedure_get_extensions  (LigmaFileProcedure *procedure);

void            ligma_file_procedure_set_prefixes    (LigmaFileProcedure *procedure,
                                                     const gchar       *prefixes);
const gchar   * ligma_file_procedure_get_prefixes    (LigmaFileProcedure *procedure);

void            ligma_file_procedure_set_magics      (LigmaFileProcedure *procedure,
                                                     const gchar       *magics);
const gchar   * ligma_file_procedure_get_magics      (LigmaFileProcedure *procedure);

void            ligma_file_procedure_set_priority    (LigmaFileProcedure *procedure,
                                                     gint               priority);
gint            ligma_file_procedure_get_priority    (LigmaFileProcedure *procedure);

void            ligma_file_procedure_set_handles_remote
                                                    (LigmaFileProcedure *procedure,
                                                     gboolean           handles_remote);
gboolean        ligma_file_procedure_get_handles_remote
                                                    (LigmaFileProcedure *procedure);


G_END_DECLS

#endif  /*  __LIGMA_FILE_PROCEDURE_H__  */
