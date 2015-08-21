/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-dialog.c
 * Copyright (C) 2015-2016 Jehan <jehan@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ANIMATION_DIALOG_H__
#define __ANIMATION_DIALOG_H__

#define ANIMATION_TYPE_DIALOG            (animation_dialog_get_type ())
#define ANIMATION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_DIALOG, AnimationDialog))
#define ANIMATION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_DIALOG, AnimationDialogClass))
#define ANIMATION_IS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_DIALOG))
#define ANIMATION_IS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_DIALOG))
#define ANIMATION_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_DIALOG, AnimationDialogClass))

typedef struct _AnimationDialog      AnimationDialog;
typedef struct _AnimationDialogClass AnimationDialogClass;

struct _AnimationDialog
{
  GtkWindow       parent_instance;
};

struct _AnimationDialogClass
{
  GtkWindowClass  parent_class;
};

GType       animation_dialog_get_type             (void) G_GNUC_CONST;

GtkWidget * animation_dialog_new                  (gint32 image_id);

#endif  /*  __ANIMATION_DIALOG_H__  */
