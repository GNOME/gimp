/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-run.h
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

#ifndef __PLUG_IN_RUN_H__
#define __PLUG_IN_RUN_H__

#ifndef __YES_I_NEED_PLUG_IN_RUN__
#error Don't use plug_in_run*(), use gimp_procedure_execute*() instead.
#endif


/*  Run a plug-in as if it were a procedure database procedure
 */
GValueArray * plug_in_run      (Gimp                   *gimp,
                                GimpContext            *context,
                                GimpProgress           *progress,
                                GimpPlugInProcedure    *procedure,
                                GValueArray            *args,
                                gboolean                synchronous,
                                gboolean                destroy_return_vals,
                                gint                    display_ID);

/*  Run a temp plug-in proc as if it were a procedure database procedure
 */
GValueArray * plug_in_run_temp (Gimp                   *gimp,
                                GimpContext            *context,
                                GimpProgress           *progress,
                                GimpTemporaryProcedure *procedure,
                                GValueArray            *args);


#endif /* __PLUG_IN_RUN_H__ */
