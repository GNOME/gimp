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
#ifndef  __LC_DIALOG_H__
#define  __LC_DIALOG_H__

#include "gimpimageF.h"

void   lc_dialog_create            (GimpImage *gimage);
void   lc_dialog_free              (void);

void   lc_dialog_rebuild           (int);  /*  implies free & create  */

void   lc_dialog_flush             (void);

void   lc_dialog_update_image_list (void);

#endif  /*  __LC_DIALOG_H__  */
