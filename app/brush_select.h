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


typedef struct _BrushSelect BrushSelect;

struct _BrushSelect
{
  GtkWidget     *shell;

  GtkWidget     *view;
  GtkWidget     *paint_options_box;

  GtkWidget     *brush_name;
  GtkWidget     *brush_size;

  GtkAdjustment *spacing_data;

  GtkAdjustment *opacity_data;
  GtkWidget     *option_menu;

  /*  Callback function name  */
  gchar         *callback_name;

  /*  Context to store the current brush & paint options  */
  GimpContext   *context;
  gint           spacing_value;

  GQuark         name_changed_handler_id;
  GQuark         dirty_handler_id;
};

/*  list of active dialogs  */
extern GSList *brush_active_dialogs;

/*  the main brush dialog */
extern BrushSelect *brush_select_dialog;


BrushSelect * brush_select_new           (gchar       *title,
					  /*  These are the required initial vals
					   *  If init_name == NULL then use
					   *  current brush
					   */
					  gchar       *init_name,
					  gdouble      init_opacity, 
					  gint         init_spacing,
					  gint         init_mode);
void          brush_select_free          (BrushSelect *bsp);
void          brush_select_dialogs_check (void);


/*  show/hide paint options (main brush dialog if bsp == NULL)  */
void          brush_select_show_paint_options (BrushSelect *bsp,
					       gboolean     show);


/*  the main brush selection  */
void          brush_dialog_free   (void);
void          brush_dialog_create (void);


#endif  /*  __BRUSH_SELECT_H__  */
