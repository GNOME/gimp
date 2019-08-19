/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfileprocedure.h
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

#ifndef __GIMP_FILE_PROCEDURE_H__
#define __GIMP_FILE_PROCEDURE_H__

#include <libgimp/gimpprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_FILE_PROCEDURE            (gimp_file_procedure_get_type ())
#define GIMP_FILE_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FILE_PROCEDURE, GimpFileProcedure))
#define GIMP_FILE_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILE_PROCEDURE, GimpFileProcedureClass))
#define GIMP_IS_FILE_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FILE_PROCEDURE))
#define GIMP_IS_FILE_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILE_PROCEDURE))
#define GIMP_FILE_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FILE_PROCEDURE, GimpFileProcedureClass))


typedef struct _GimpFileProcedure        GimpFileProcedure;
typedef struct _GimpFileProcedureClass   GimpFileProcedureClass;
typedef struct _GimpFileProcedurePrivate GimpFileProcedurePrivate;

struct _GimpFileProcedure
{
  GimpProcedure             parent_instance;

  GimpFileProcedurePrivate *priv;
};

struct _GimpFileProcedureClass
{
  GimpProcedureClass parent_class;
};


GType           gimp_file_procedure_get_type        (void) G_GNUC_CONST;

void            gimp_file_procedure_set_mime_types  (GimpFileProcedure *procedure,
                                                     const gchar       *mime_types);
const gchar   * gimp_file_procedure_get_mime_types  (GimpFileProcedure *procedure);

void            gimp_file_procedure_set_extensions  (GimpFileProcedure *procedure,
                                                     const gchar       *extensions);
const gchar   * gimp_file_procedure_get_extensions  (GimpFileProcedure *procedure);

void            gimp_file_procedure_set_prefixes    (GimpFileProcedure *procedure,
                                                     const gchar       *prefixes);
const gchar   * gimp_file_procedure_get_prefixes    (GimpFileProcedure *procedure);

void            gimp_file_procedure_set_magics      (GimpFileProcedure *procedure,
                                                     const gchar       *magics);
const gchar   * gimp_file_procedure_get_magics      (GimpFileProcedure *procedure);

void            gimp_file_procedure_set_priority    (GimpFileProcedure *procedure,
                                                     gint               priority);
gint            gimp_file_procedure_get_priority    (GimpFileProcedure *procedure);

void            gimp_file_procedure_set_handles_remote
                                                    (GimpFileProcedure *procedure,
                                                     gboolean           handles_remote);
gboolean        gimp_file_procedure_get_handles_remote
                                                    (GimpFileProcedure *procedure);


G_END_DECLS

#endif  /*  __GIMP_FILE_PROCEDURE_H__  */
