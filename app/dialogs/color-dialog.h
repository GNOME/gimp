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

#include <gtk/gtk.h>

typedef enum
{
  COLOR_NOTEBOOK_OK,
  COLOR_NOTEBOOK_CANCEL,
  COLOR_NOTEBOOK_UPDATE
} ColorNotebookState;

typedef void (* ColorNotebookCallback) (gint               red,
					gint               green,
					gint               blue,
					ColorNotebookState state,
					gpointer           data);

typedef struct _ColorSelectorInstance ColorSelectorInstance;

typedef struct _ColorNotebook ColorNotebook;

struct _ColorNotebook
{
  GtkWidget             *shell;
  GtkWidget             *notebook;

  gint                   values[3];
  gint                   orig_values[3];

  ColorNotebookCallback  callback;
  gpointer               client_data;

  gint                   wants_updates;

  ColorSelectorInstance *selectors;
  ColorSelectorInstance *cur_page;
};

ColorNotebook * color_notebook_new       (gint                  red,
					  gint                  green,
					  gint                  blue,
					  ColorNotebookCallback callback,
					  gpointer              data,
				          gboolean              wants_update);

void            color_notebook_show      (ColorNotebook        *cnb);
void            color_notebook_hide      (ColorNotebook        *cnb);
void            color_notebook_free      (ColorNotebook        *cnb);

void            color_notebook_set_color (ColorNotebook        *cnb,
					  gint                  red,
					  gint                  green,
					  gint                  blue,
					  gboolean              set_current);

#endif /* __COLOR_NOTEBOOK_H__ */
