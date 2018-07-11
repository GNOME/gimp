/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprocview.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_PROC_VIEW_H__
#define __GIMP_PROC_VIEW_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


GtkWidget * gimp_proc_view_new (const gchar        *name,
                                const gchar        *menu_path,
                                const gchar        *blurb,
                                const gchar        *help,
                                const gchar        *author,
                                const gchar        *copyright,
                                const gchar        *date,
                                GimpPDBProcType     type,
                                gint                n_params,
                                gint                n_return_vals,
                                const GimpParamDef *params,
                                const GimpParamDef *return_vals);


G_END_DECLS

#endif  /* __GIMP_PROC_VIEW_H__ */
