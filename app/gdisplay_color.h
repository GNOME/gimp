/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Manish Singh
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
#ifndef __GDISPLAY_COLOR_H__
#define __GDISPLAY_COLOR_H__

#include "libgimp/color_display.h"
#include "gdisplayF.h"

typedef struct _ColorDisplayNode ColorDisplayNode;

struct _ColorDisplayNode {
  char                   *cd_name;
  gpointer                cd_ID; 
  GimpColorDisplayConvert cd_convert;
};

void gdisplay_color_init       (void);
void gdisplay_color_attach     (GDisplay   *gdisp,
				const char *name);
void gdisplay_color_detach     (GDisplay   *gdisp,
				const char *name);
void gdisplay_color_detach_all (GDisplay   *gdisp);

#endif /* __GDISPLAY_COLOR_H__ */
