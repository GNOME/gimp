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
  gpointer                cd_ID; 
  char                   *cd_name;
  GimpColorDisplayConvert cd_convert;
};

typedef void (*GimpCDFunc) (const char *name,
			    gpointer    user_data);

void color_display_init    (void);
void color_display_foreach (GimpCDFunc func,
			    gpointer   user_data);

ColorDisplayNode * gdisplay_color_attach         (GDisplay         *gdisp,
						  const char       *name);
ColorDisplayNode * gdisplay_color_attach_clone   (GDisplay         *gdisp,
						  ColorDisplayNode *node);
void               gdisplay_color_detach         (GDisplay         *gdisp,
						  ColorDisplayNode *node);
void               gdisplay_color_detach_destroy (GDisplay         *gdisp,
						  ColorDisplayNode *node);
void               gdisplay_color_detach_all     (GDisplay         *gdisp);
void               gdisplay_color_reorder_up     (GDisplay         *gdisp,
						  ColorDisplayNode *node);
void               gdisplay_color_reorder_down   (GDisplay         *gdisp,
						  ColorDisplayNode *node);

void gdisplay_color_configure        (ColorDisplayNode *node,
				      GFunc             ok_func,
				      gpointer          ok_data,
				      GFunc             cancel_func,
				      gpointer          cancel_data);
void gdisplay_color_configure_cancel (ColorDisplayNode *node);

#endif /* __GDISPLAY_COLOR_H__ */
