/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-call.h
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

#pragma once

#ifndef __YES_I_NEED_GIMP_PLUG_IN_MANAGER_CALL__
#error Do not use gimp_plug_in_manager_call_run*(), use gimp_procedure_execute*() instead.
#endif


/*  Call the plug-in's query() function
 */
void             gimp_plug_in_manager_call_query    (GimpPlugInManager      *manager,
                                                     GimpContext            *context,
                                                     GimpPlugInDef          *plug_in_def);

/*  Call the plug-in's init() function
 */
void             gimp_plug_in_manager_call_init     (GimpPlugInManager      *manager,
                                                     GimpContext            *context,
                                                     GimpPlugInDef          *plug_in_def);

/*  Run a plug-in as if it were a procedure database procedure
 */
GimpValueArray * gimp_plug_in_manager_call_run      (GimpPlugInManager      *manager,
                                                     GimpContext            *context,
                                                     GimpProgress           *progress,
                                                     GimpPlugInProcedure    *procedure,
                                                     GimpValueArray         *args,
                                                     gboolean                synchronous,
                                                     GimpDisplay            *display);

/*  Run a temp plug-in proc as if it were a procedure database procedure
 */
GimpValueArray * gimp_plug_in_manager_call_run_temp (GimpPlugInManager      *manager,
                                                     GimpContext            *context,
                                                     GimpProgress           *progress,
                                                     GimpTemporaryProcedure *procedure,
                                                     GimpValueArray         *args);
