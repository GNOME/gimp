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
#ifndef __GRADIENT_SELECT_H__
#define __GRADIENT_SELECT_H__

#include <gtk/gtk.h>

#include "gimpcontext.h"

#define GRADIENT_SAMPLE_SIZE 40

typedef struct _GradientSelect GradientSelect;

struct _GradientSelect
{
  GtkWidget   *shell;
  GtkWidget   *frame;
  GtkWidget   *preview;
  GtkWidget   *clist;

  GimpContext *context;
  gchar       *callback_name;
  gint         sample_size;

  GdkColor     black;
  GdkGC       *gc;
};

/*  list of active dialogs    */
extern GSList *gradient_active_dialogs;

/*  the main gradient selection dialog  */
extern GradientSelect *gradient_select_dialog;

GradientSelect * gradient_select_new        (gchar          *title,
					     gchar          *initial_gradient);

void             gradient_select_free       (GradientSelect *gsp);

void             gradient_select_rename_all (gint            n,
					     gradient_t     *gradient);
void             gradient_select_insert_all (gint            pos,
					     gradient_t     *gradient);
void             gradient_select_delete_all (gint            n);
void             gradient_select_free_all   (void);
void             gradient_select_refill_all (void);
void             gradient_select_update_all (gint            row,
					     gradient_t     *gradient);

void             gradients_check_dialogs   (void);

/*  the main gradient selection  */
void             gradient_dialog_create    (void);
void             gradient_dialog_free      (void);

#endif  /*  __GRADIENT_SELECT_H__  */
