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

#ifndef  __LC_DIALOG_H__
#define  __LC_DIALOG_H__


#define PREVIEW_EVENT_MASK (GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | \
                            GDK_ENTER_NOTIFY_MASK)
#define BUTTON_EVENT_MASK  (GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK | \
                            GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | \
                            GDK_BUTTON_RELEASE_MASK)

#define LIST_WIDTH  200
#define LIST_HEIGHT 150

#define NORMAL      0
#define SELECTED    1
#define INSENSITIVE 2


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


/*  Main dialog structure  */
extern LCDialog *lc_dialog;


GtkWidget * lc_dialog_create            (GimpImage *gimage);
void        lc_dialog_free              (void);

/*  implies free & create
 */
void        lc_dialog_rebuild           (gint       new_preview_size);

void        lc_dialog_flush             (void);

void        lc_dialog_update_image_list (void);
void        lc_dialog_preview_update    (GimpImage *gimage);


void        lc_dialog_menu_preview_dirty (GtkObject *,gpointer);


#endif  /*  __LC_DIALOG_H__  */
