/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpexportproceduredialog.h
 * Copyright (C) 2024 Jehan
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

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_VECTOR_LOAD_PROCEDURE_DIALOG_H__
#define __GIMP_VECTOR_LOAD_PROCEDURE_DIALOG_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_VECTOR_LOAD_PROCEDURE_DIALOG (gimp_vector_load_procedure_dialog_get_type ())
G_DECLARE_FINAL_TYPE (GimpVectorLoadProcedureDialog, gimp_vector_load_procedure_dialog, GIMP, VECTOR_LOAD_PROCEDURE_DIALOG, GimpProcedureDialog)


GtkWidget * gimp_vector_load_procedure_dialog_new (GimpVectorLoadProcedure *procedure,
                                                   GimpProcedureConfig     *config,
                                                   GimpVectorLoadData      *extracted_data,
                                                   GFile                   *file);



G_END_DECLS

#endif /* __GIMP_VECTOR_LOAD_PROCEDURE_DIALOG_H__ */
