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
#ifndef  __LC_DIALOGP_H__
#define  __LC_DIALOGP_H__

typedef struct _LCDialog LCDialog;

struct _LCDialog
{
  GtkWidget *shell;
  GtkWidget *subshell;

  GtkWidget *image_menu;
  GtkWidget *image_option_menu;

  GimpImage *gimage;
  gboolean   auto_follow_active;

  GtkWidget *notebook;
};

GtkWidget *  layers_dialog_create    (void);
GtkWidget *  channels_dialog_create  (void);
GtkWidget *  paths_dialog_create     (void);

void         layers_dialog_free      (void);
void         channels_dialog_free    (void);
void         paths_dialog_free       (void);

void         layers_dialog_update    (GimpImage *);
void         channels_dialog_update  (GimpImage *);
void         paths_dialog_update     (GimpImage *);

void         layers_dialog_flush     (void);
void         channels_dialog_flush   (void);
void         paths_dialog_flush      (void);

void         layers_dialog_clear     (void);
void         channels_dialog_clear   (void);

void         lc_dialog_menu_preview_dirty (GtkObject *,gpointer);

/*  Main dialog structure  */
extern LCDialog *lc_dialog;

#endif  /*  __LC_DIALOGP_H__  */
