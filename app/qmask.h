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
#ifndef __QMASK_H__
#define __QMASK_H__
#include "pixmaps.h"
#ifdef icky
GtkWidget *
qmask_area_create (GDisplay *display);
#endif /* icky */
void qmask_activate(GtkWidget *w, GDisplay *gdisp);
void qmask_deactivate(GtkWidget *w, GDisplay *gdisp);
void qmask_buttons_update (GDisplay *gdisp);

#endif  /*  __QMASK_H__ */

