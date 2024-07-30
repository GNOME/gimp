/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprogressbar.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PROGRESS_BAR_H__
#define __GIMP_PROGRESS_BAR_H__

G_BEGIN_DECLS


#define GIMP_TYPE_PROGRESS_BAR (gimp_progress_bar_get_type ())
G_DECLARE_FINAL_TYPE (GimpProgressBar, gimp_progress_bar, GIMP, PROGRESS_BAR, GtkProgressBar)


GtkWidget * gimp_progress_bar_new  (void);


G_END_DECLS

#endif /* __GIMP_PROGRESS_BAR_H__ */
