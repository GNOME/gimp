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


typedef struct _PatternSelect PatternSelect;

struct _PatternSelect
{
  GtkWidget     *shell;

  GtkWidget     *view;

  /*  Callback function name  */
  gchar         *callback_name;

  /*  Context to store the current pattern  */
  GimpContext   *context;
};


/*  list of active dialogs  */
extern GSList *pattern_active_dialogs;

/*  the main pattern dialog  */
extern PatternSelect *pattern_select_dialog;


PatternSelect * pattern_select_new           (gchar         *title,
					      gchar         *initial_pattern);
void            pattern_select_free          (PatternSelect *psp);
void            pattern_select_dialogs_check (void);


/*  the main pattern selection  */
GtkWidget     * pattern_dialog_create        (void);
void            pattern_dialog_free          (void);


#endif  /*  __PATTERN_SELECT_H__  */
