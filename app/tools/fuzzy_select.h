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
#ifndef __FUZZY_SELECT_H__
#define __FUZZY_SELECT_H__

#include "gimage.h"
#include "tools.h"
#include "procedural_db.h"


/*  fuzzy select functions  */
Tool *        tools_new_fuzzy_select      (void);
void          tools_free_fuzzy_select     (Tool *);


/*  functions  */
Channel *     find_contiguous_region      (GImage *, GimpDrawable *, int, gfloat, int, int, int);

/*  Procedure definition and marshalling function  */
extern ProcRecord fuzzy_select_proc;

#endif  /*  __FUZZY_SELECT_H__  */
