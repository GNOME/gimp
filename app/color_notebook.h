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

#ifndef __COLOR_NOTEBOOK_H__
#define __COLOR_NOTEBOOK_H__

typedef enum {
  COLOR_NOTEBOOK_OK,
  COLOR_NOTEBOOK_CANCEL,
  COLOR_NOTEBOOK_UPDATE
} ColorNotebookState;

typedef struct _ColorNotebook _ColorNotebook, *ColorNotebookP;
typedef void (*ColorNotebookCallback) (int, int, int, ColorNotebookState,
				       void *);

struct _ColorSelectorInstance;

struct _ColorNotebook {
  GtkWidget *shell;
  GtkWidget *notebook;
  int values[3];
  int orig_values[3];
  ColorNotebookCallback callback;
  void *client_data;
  int wants_updates;
  struct _ColorSelectorInstance *selectors;
  struct _ColorSelectorInstance *cur_page;
};

ColorNotebookP color_notebook_new (int, int, int, ColorNotebookCallback, void *, int);
void color_notebook_show (ColorNotebookP);
void color_notebook_hide (ColorNotebookP);
void color_notebook_free (ColorNotebookP);
void color_notebook_set_color (ColorNotebookP, int, int, int, int);


#endif /* __COLOR_NOTEBOOK_H__ */
