/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpproceduraldb_pdb.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */         

#ifndef __GIMP_PROCEDURAL_DB_PDB_H__
#define __GIMP_PROCEDURAL_DB_PDB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


/* Specify a range of data to be associated with 'id'.
 *  The data will exist for as long as the main gimp
 *  application is running.
 */
void      gimp_procedural_db_set_data      (gchar    *id,
					    gpointer  data,
					    guint32   length);
  
/* Retrieve the piece of data stored within the main
 *  gimp application specified by 'id'. The data is
 *  stored in the supplied buffer.  Make sure enough
 *  space is allocated.
 */
void      gimp_procedural_db_get_data      (gchar    *id,
					    gpointer  data);

/* Get the size in bytes of the data stored by a gimp_get_data
 * id. As size of zero may indicate that there is no such
 * identifier in the database.
 */
guint32   gimp_procedural_db_get_data_size (gchar    *id);

/* Query the gimp application's procedural database.
 *  The arguments are regular expressions which select
 *  which procedure names will be returned in 'proc_names'.
 */
void      gimp_procedural_db_query (gchar    *name_regexp,
				    gchar    *blurb_regexp,
				    gchar    *help_regexp,
				    gchar    *author_regexp,
				    gchar    *copyright_regexp,
				    gchar    *date_regexp,
				    gchar    *proc_type_regexp,
				    gint     *nprocs,
				    gchar  ***proc_names);

/* Query the gimp application's procedural database
 *  regarding a particular procedure.
 */
gboolean    gimp_procedural_db_query_proc (gchar          *proc_name,
					   gchar         **proc_blurb,
					   gchar         **proc_help,
					   gchar         **proc_author,
					   gchar         **proc_copyright,
					   gchar         **proc_date,
					   gint           *proc_type,
					   gint           *nparams,
					   gint           *nreturn_vals,
					   GimpParamDef  **params,
					   GimpParamDef  **return_vals);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_PROCEDURAL_DB_PDB_H__ */


