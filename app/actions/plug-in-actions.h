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

#ifndef __PLUG_IN_ACTIONS_H__
#define __PLUG_IN_ACTIONS_H__


void   plug_in_actions_setup       (GimpActionGroup *group);
void   plug_in_actions_update      (GimpActionGroup *group,
                                    gpointer         data);

void   plug_in_actions_add_proc    (GimpActionGroup *group,
                                    PlugInProcDef   *proc_def);
void   plug_in_actions_add_path    (GimpActionGroup *group,
                                    PlugInProcDef   *proc_def,
                                    const gchar     *menu_path);
void   plug_in_actions_remove_proc (GimpActionGroup *group,
                                    PlugInProcDef   *proc_def);

void   plug_in_actions_add_branch  (GimpActionGroup *group,
                                    const gchar     *progname,
                                    const gchar     *menu_path,
                                    const gchar     *menu_label);


#endif /* __PLUG_IN_ACTIONS_H__ */
