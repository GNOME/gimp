/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __PAINT_OPTIONS_H__
#define __PAINT_OPTIONS_H__


#include "tool_options.h"


/*  paint tool options functions  */

GimpToolOptions * paint_options_new        (GimpToolInfo     *tool_info);

void              paint_options_reset      (GimpToolOptions  *tool_options);


/*  to be used by "derived" paint options only  */
void              paint_options_init       (GimpPaintOptions *options,
                                            GimpToolInfo     *tool_info);


/*  functions for the global paint options  */

/*  switch between global and per-tool paint options  */
void              paint_options_set_global (gboolean          global);


#endif  /*  __PAINT_OPTIONS_H__  */
