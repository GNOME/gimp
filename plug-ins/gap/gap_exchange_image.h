/*  gap_exchange_image.h
 *
 *
 */
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

/* revision history:
 * version 0.98.00; 1998/11/26  hof: 1.st release 
 *                             (substitute for the procedure "gimp_duplicate_into"
 *                              that was never part of the GIMP core)
 */

#ifndef _GAP_EXCHANGE_IMAGE_H
#define _GAP_EXCHANGE_IMAGE_H

#include "libgimp/gimp.h"

int p_exchange_image(gint32 dst_image_id, gint32 src_image_id);

#endif
