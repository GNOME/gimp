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
#ifndef  __LAYERS_DIALOGP_H__
#define  __LAYERS_DIALOGP_H__

struct _Canvas;

#include "buildmenu.h"

GtkWidget *  layers_dialog_create    (void);
GtkWidget *  channels_dialog_create  (void);

GtkWidget *  create_image_menu       (int *, int *, MenuItemCallback);

void         layers_dialog_update    (int);
void         channels_dialog_update  (int);

void         layers_dialog_clear     (void);
void         channels_dialog_clear   (void);

void         layers_dialog_free      (void);
void         channels_dialog_free    (void);

void         render_fs_preview       (GtkWidget *, GdkPixmap *);
void         render_preview          (struct _Canvas *, GtkWidget *, int, int, int);

/*  Main dialog widget  */
extern GtkWidget *lc_shell;

#endif  /*  __LAYERS_DIALOGP_H__  */
