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
#ifndef  __PATTERN_SELECT_H__
#define  __PATTERN_SELECT_H__

typedef struct _PatternSelect _PatternSelect, *PatternSelectP;

struct _PatternSelect {
  GtkWidget         *shell;
  GtkWidget         *frame;
  GtkWidget         *preview;
  GtkWidget         *pattern_name;
  GtkWidget         *pattern_size;
  GtkWidget         *options_box;
  GdkGC             *gc;
  GtkAdjustment     *sbar_data;
  int                width, height;
  int                cell_width, cell_height;
  int                scroll_offset;
  /*  Pattern popup  */
  GtkWidget *pattern_popup;
  GtkWidget *pattern_preview;
};

PatternSelectP  pattern_select_new     (void);
void            pattern_select_select  (PatternSelectP, int);
void            pattern_select_free    (PatternSelectP);


#endif  /*  __PATTERN_SELECT_H__  */
