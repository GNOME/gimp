/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapluginmanager-call.h
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

#ifndef __LIGMA_PLUG_IN_MANAGER_CALL_H__
#define __LIGMA_PLUG_IN_MANAGER_CALL_H__

#ifndef __YES_I_NEED_LIGMA_PLUG_IN_MANAGER_CALL__
#error Do not use ligma_plug_in_manager_call_run*(), use ligma_procedure_execute*() instead.
#endif


/*  Call the plug-in's query() function
 */
void             ligma_plug_in_manager_call_query    (LigmaPlugInManager      *manager,
                                                     LigmaContext            *context,
                                                     LigmaPlugInDef          *plug_in_def);

/*  Call the plug-in's init() function
 */
void             ligma_plug_in_manager_call_init     (LigmaPlugInManager      *manager,
                                                     LigmaContext            *context,
                                                     LigmaPlugInDef          *plug_in_def);

/*  Run a plug-in as if it were a procedure database procedure
 */
LigmaValueArray * ligma_plug_in_manager_call_run      (LigmaPlugInManager      *manager,
                                                     LigmaContext            *context,
                                                     LigmaProgress           *progress,
                                                     LigmaPlugInProcedure    *procedure,
                                                     LigmaValueArray         *args,
                                                     gboolean                synchronous,
                                                     LigmaDisplay            *display);

/*  Run a temp plug-in proc as if it were a procedure database procedure
 */
LigmaValueArray * ligma_plug_in_manager_call_run_temp (LigmaPlugInManager      *manager,
                                                     LigmaContext            *context,
                                                     LigmaProgress           *progress,
                                                     LigmaTemporaryProcedure *procedure,
                                                     LigmaValueArray         *args);


#endif /* __LIGMA_PLUG_IN_MANAGER_CALL_H__ */
