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

#ifndef __PLUG_IN_PARAMS_H__
#define __PLUG_IN_PARAMS_H__


Argument * plug_in_params_to_args   (GPParam     *params,
                                     gint         n_params,
                                     gboolean     full_copy);
GPParam  * plug_in_args_to_params   (Argument    *args,
                                     gint         n_args,
                                     gboolean     full_copy);

void       plug_in_params_destroy   (GPParam     *params,
                                     gint         n_params,
                                     gboolean     full_destroy);
void       plug_in_args_destroy     (Argument    *args,
                                     gint         n_args,
                                     gboolean     full_destroy);

gboolean   plug_in_param_defs_check (const gchar *plug_in_name,
                                     const gchar *plug_in_prog,
                                     const gchar *procedure_name,
                                     const gchar *menu_path,
                                     GPParamDef  *params,
                                     guint32      n_args,
                                     GPParamDef  *return_vals,
                                     guint32      n_return_vals,
                                     GError     **error);
gboolean   plug_in_proc_args_check  (const gchar *plug_in_name,
                                     const gchar *plug_in_prog,
                                     const gchar *procedure_name,
                                     const gchar *menu_path,
                                     ProcArg     *args,
                                     guint32      n_args,
                                     ProcArg     *return_vals,
                                     guint32      n_return_vals,
                                     GError     **error);


#endif /* __PLUG_IN_PARAMS_H__ */
