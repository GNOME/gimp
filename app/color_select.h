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
#ifndef __COLOR_SELECT_H__
#define __COLOR_SELECT_H__

struct _PixelRow;

typedef enum {
  COLOR_SELECT_OK,
  COLOR_SELECT_CANCEL,
  COLOR_SELECT_UPDATE
} ColorSelectState;

typedef struct _ColorSelect _ColorSelect, *ColorSelectP;
typedef void (*ColorSelectCallback) (struct _PixelRow *, ColorSelectState, void *);

struct _ColorSelect {
  GtkWidget *shell;
  GtkWidget *xy_color;
  GtkWidget *z_color;
  GtkWidget *new_color;
  GtkWidget *orig_color;
  GtkWidget *toggles[6];
  GtkWidget *entries[6];
  GtkAdjustment *slider_data[6];
  gfloat pos[3];
  gfloat values[6];
  int z_color_fill;
  int xy_color_fill;
  gfloat orig_values[3];
  ColorSelectCallback callback;
  void *client_data;
  int wants_updates;
  GdkGC *gc;
};

ColorSelectP color_select_new (struct _PixelRow *, ColorSelectCallback, void *, int);
void color_select_show (ColorSelectP);
void color_select_hide (ColorSelectP);
void color_select_free (ColorSelectP);
void color_select_set_color (ColorSelectP, struct _PixelRow *, int);

#endif /* __COLOR_SELECT_H__ */
