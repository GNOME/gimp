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


#define GIMP_TYPE_FILE_PROCEDURE (gimp_file_procedure_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpFileProcedure, gimp_file_procedure, GIMP, FILE_PROCEDURE, GimpProcedure)

struct _GimpFileProcedureClass
{
  GimpProcedureClass parent_class;

  /* Padding for future expansion */
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
  void (*_gimp_reserved5) (void);
  void (*_gimp_reserved6) (void);
  void (*_gimp_reserved7) (void);
  void (*_gimp_reserved8) (void);
  void (*_gimp_reserved9) (void);
};


void            gimp_file_procedure_set_format_name (GimpFileProcedure *procedure,
                                                     const gchar       *format_name);
const gchar   * gimp_file_procedure_get_format_name (GimpFileProcedure *procedure);

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
