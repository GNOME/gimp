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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __COLOR_PANEL_H__
#define __COLOR_PANEL_H__

typedef struct _ColorPanel ColorPanel;

struct _ColorPanel
{
  /*  The calling procedure is respondible for showing this widget  */
  GtkWidget *   color_panel_widget;

  /*  The actual color  */
  unsigned char color [3];

  /*  Don't touch this :)  */
  void *        private_part;
};

ColorPanel * color_panel_new    (unsigned char * initial,
				 int             width,
				 int             height);
void         color_panel_free   (ColorPanel *    color_panel);

#endif  /*  __COLOR_PANEL_H__  */
