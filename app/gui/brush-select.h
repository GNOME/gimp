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

  GtkAdjustment *opacity_data;
  GtkWidget     *option_menu;

  /*  Callback function name  */
  gchar         *callback_name;

  /*  Context to store the current brush & paint options  */
  GimpContext   *context;
  gint           spacing_value;
};


BrushSelect * brush_select_new                (Gimp        *gimp,
                                               const gchar *title,
                                               /*  These are the required
                                                *   initial vals
                                                *  If init_name == NULL then use
                                                *  current brush
                                                */
                                               const gchar *init_name,
                                               gdouble      init_opacity, 
                                               gint         init_spacing,
                                               gint         init_mode,
                                               const gchar *callback_name);
void          brush_select_free               (BrushSelect *bsp);

BrushSelect * brush_select_get_by_callback    (const gchar *callback_name);
void          brush_select_dialogs_check      (void);


/*  show/hide paint options (main brush dialog if bsp == NULL)  */
void          brush_select_show_paint_options (BrushSelect *bsp,
					       gboolean     show);


/*  the main brush selection  */
GtkWidget   * brush_dialog_create             (Gimp        *gimp);
void          brush_dialog_free               (void);


#endif  /*  __BRUSH_SELECT_H__  */
