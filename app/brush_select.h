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
#ifndef  __BRUSH_SELECT_H__
#define  __BRUSH_SELECT_H__

#include <gtk/gtk.h>

#include "gimpbrush.h"

typedef struct _BrushSelect BrushSelect;

struct _BrushSelect
{
  GtkWidget *shell;

  /*  Place holders which enable global<->per-tool paint options switching  */
  GtkWidget *left_box;
  GtkWidget *right_box;
  GtkWidget *brush_selection_box;
  GtkWidget *paint_options_box;

  /*  The preview and it's vscale data  */
  GtkWidget     *preview;
  GtkAdjustment *sbar_data;

  GtkWidget *options_box;
  GtkWidget *brush_name;
  GtkWidget *brush_size;

  GtkAdjustment *opacity_data;
  GtkAdjustment *spacing_data;
  GtkWidget     *edit_button;
  GtkWidget     *delete_button;
  GtkWidget     *option_menu;

  /*  Brush popup  */
  GtkWidget *brush_popup;
  GtkWidget *brush_preview;
  guint      popup_timeout_tag;
  guint      popup_anim_timeout_tag;
  guint      popup_pipe_index;

  /*  Callback function name  */
  gchar       *callback_name;

  /*  Context to store the current brush & paint options  */
  GimpContext *context;
  gint         spacing_value;

  /*  Some variables to keep the GUI consistent  */
  gint      cell_width;
  gint      cell_height;
  gint      scroll_offset;
  gint      old_row;
  gint      old_col;
  gint      NUM_BRUSH_COLUMNS;
  gint      NUM_BRUSH_ROWS;

  gboolean  redraw;
  gboolean  freeze; /*  so we don't waste so much time during refresh  */
};

/*  list of active dialogs  */
extern GSList *brush_active_dialogs;

/*  the main brush dialog */
extern BrushSelect *brush_select_dialog;

BrushSelect * brush_select_new        (gchar       *title,
				       /*  These are the required initial vals
					*  If init_name == NULL then use
					*  current brush
					*/
				       gchar       *init_name,
				       gdouble      init_opacity, 
				       gint         init_spacing,
				       gint         init_mode);

void          brush_select_free       (BrushSelect *bsp);

void          brush_select_freeze_all (void);
void          brush_select_thaw_all   (void);

void          brushes_check_dialogs   (void);

/*  show/hide paint options (main brush dialog if bsp == NULL)  */
void          brush_select_show_paint_options (BrushSelect *bsp,
					       gboolean     show);

/*  the main brush selection  */
void          brush_dialog_free   (void);
void          brush_dialog_create (void);

#endif  /*  __BRUSH_SELECT_H__  */
