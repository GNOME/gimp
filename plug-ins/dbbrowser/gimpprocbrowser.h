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

/*
 * dbbrowser_utils.h
 * 0.08  26th sept 97  by Thomas NOEL <thomas@minet.net>
 */

#ifndef __GIMP_PROC_BROWSER_H__
#define __GIMP_PROC_BROWSER_H__


typedef void (* GimpProcBrowserApplyCallback) (const gchar        *proc_name,
                                               const gchar        *proc_blurb,
                                               const gchar        *proc_help,
                                               const gchar        *proc_author,
                                               const gchar        *proc_copyright,
                                               const gchar        *proc_date,
                                               GimpPDBProcType     proc_type,
                                               gint                n_params,
                                               gint                n_return_vals,
                                               const GimpParamDef *params,
                                               const GimpParamDef *return_vals,
                                               gpointer            user_data);


GtkWidget * gimp_proc_browser_dialog_new (gboolean                     scheme_names,
                                          GimpProcBrowserApplyCallback apply_callback,
                                          gpointer                     user_data);


#endif  /* __GIMP_PROC_BROWSER_H__ */
