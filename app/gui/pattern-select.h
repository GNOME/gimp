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

#include <gtk/gtk.h>

#include "patterns.h"

typedef struct _PatternSelect PatternSelect;

struct _PatternSelect
{
  GtkWidget     *shell;

  /*  The preview and it's vscale data  */
  GtkWidget     *preview;
  GdkGC         *gc;
  GtkAdjustment *sbar_data;

  GtkWidget *options_box;
  GtkWidget *pattern_name;
  GtkWidget *pattern_size;

  /*  Pattern popup  */
  GtkWidget *pattern_popup;
  GtkWidget *pattern_preview;
  guint      popup_timeout_tag;

  /*  Callback function name  */
  gchar       *callback_name;

  /*  Context to store the current pattern  */
  GimpContext *context;

  /*  Some variables to keep the GUI consistent  */
  gint  cell_width;
  gint  cell_height;
  gint  scroll_offset;
  gint  old_row;
  gint  old_col;
  gint  NUM_PATTERN_COLUMNS;
  gint  NUM_PATTERN_ROWS;
};

/*  list of active dialogs  */
extern GSList *pattern_active_dialogs;

/*  the main pattern dialog  */
extern PatternSelect *pattern_select_dialog;

PatternSelect * pattern_select_new       (gchar         *title,
					  gchar         *initial_pattern);

void            pattern_select_free      (PatternSelect *psp);

void            pattern_change_callbacks (PatternSelect *psp,
					  gint           closing);
void            patterns_check_dialogs   (void);

/*  the main pattern selection  */
void            pattern_dialog_create    (void);
void            pattern_dialog_free      (void);

#endif  /*  __PATTERN_SELECT_H__  */
