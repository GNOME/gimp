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
#ifndef __INFO_WINDOW_H__
#define __INFO_WINDOW_H__

#include "gdisplayF.h"
#include "info_dialog.h"

InfoDialog * info_window_create      (GDisplay   *gdisp);
void         info_window_free        (InfoDialog *info_win);

void         info_window_update      (InfoDialog *info_win);
void         info_window_update_RGB  (InfoDialog *info_win,
				      gdouble     tx,
				      gdouble     ty);

#endif /*  __INFO_WINDOW_H__  */
