/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_PDB_H__
#define __GIMP_PDB_H__


void            gimp_pdb_init       (Gimp          *gimp);
void            gimp_pdb_exit       (Gimp          *gimp);

void            gimp_pdb_init_procs (Gimp          *gimp);

void            gimp_pdb_register   (Gimp          *gimp,
                                     GimpProcedure *procedure);
void            gimp_pdb_unregister (Gimp          *gimp,
                                     const gchar   *procedure_name);

GimpProcedure * gimp_pdb_lookup     (Gimp          *gimp,
                                     const gchar   *procedure_name);

GValueArray   * gimp_pdb_execute    (Gimp          *gimp,
                                     GimpContext   *context,
                                     GimpProgress  *progress,
                                     const gchar   *procedure_name,
                                     GValueArray   *args);
GValueArray   * gimp_pdb_run_proc   (Gimp          *gimp,
                                     GimpContext   *context,
                                     GimpProgress  *progress,
                                     const gchar   *procedure_name,
                                     ...);


#endif  /*  __GIMP_PDB_H__  */
