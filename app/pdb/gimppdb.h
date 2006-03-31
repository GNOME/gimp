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

#ifndef __PROCEDURAL_DB_H__
#define __PROCEDURAL_DB_H__


void            procedural_db_init             (Gimp             *gimp);
void            procedural_db_free             (Gimp             *gimp);

void            procedural_db_init_procs       (Gimp             *gimp);

void            procedural_db_register         (Gimp             *gimp,
                                                GimpProcedure    *procedure);
void            procedural_db_unregister       (Gimp             *gimp,
                                                const gchar      *name);
GimpProcedure * procedural_db_lookup           (Gimp             *gimp,
                                                const gchar      *name);

GimpArgument  * procedural_db_execute          (Gimp             *gimp,
                                                GimpContext      *context,
                                                GimpProgress     *progress,
                                                const gchar      *name,
                                                GimpArgument     *args,
                                                gint              n_args,
                                                gint             *n_return_vals);
GimpArgument  * procedural_db_run_proc         (Gimp             *gimp,
                                                GimpContext      *context,
                                                GimpProgress     *progress,
                                                const gchar      *name,
                                                gint             *n_return_vals,
                                                ...);

gchar         * procedural_db_type_name        (GimpPDBArgType    type);


#endif  /*  __PROCEDURAL_DB_H__  */
