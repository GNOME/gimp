/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpplugin_pdb.h
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

#ifndef __GIMP_PLUG_IN_PDB_H__
#define __GIMP_PLUG_IN_PDB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


/* Initialize the progress bar with "message". If "message"
 *  is NULL, the message displayed in the progress window will
 *  be the name of the plug-in.
 */
void   gimp_progress_init          (gchar   *message);

/* Update the progress bar. If the progress bar has not been
 *  initialized then it will be automatically initialized as if
 *  "gimp_progress_init (NULL)" were called. "percentage" is a
 *  value between 0 and 1.
 */
void   gimp_progress_update        (gdouble  percentage);

void   gimp_plugin_domain_register (gchar   *domain_name,
				    gchar   *domain_path);

void   gimp_plugin_help_register   (gchar   *help_path);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_PLUG_IN_PDB_H__ */
