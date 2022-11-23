/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontrollerlist.h
 * Copyright (C) 2005 Michael Natterer <mitch@ligma.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_CONTROLLER_LIST_H__
#define __LIGMA_CONTROLLER_LIST_H__


#define LIGMA_TYPE_CONTROLLER_LIST            (ligma_controller_list_get_type ())
#define LIGMA_CONTROLLER_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTROLLER_LIST, LigmaControllerList))
#define LIGMA_CONTROLLER_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTROLLER_LIST, LigmaControllerListClass))
#define LIGMA_IS_CONTROLLER_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTROLLER_LIST))
#define LIGMA_IS_CONTROLLER_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTROLLER_LIST))
#define LIGMA_CONTROLLER_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTROLLER_LIST, LigmaControllerListClass))


typedef struct _LigmaControllerListClass LigmaControllerListClass;

struct _LigmaControllerList
{
  GtkBox              parent_instance;

  Ligma               *ligma;

  GtkWidget          *hbox;

  GtkListStore       *src;
  GtkTreeSelection   *src_sel;
  GType               src_gtype;

  GtkWidget          *dest;
  LigmaControllerInfo *dest_info;

  GtkWidget          *add_button;
  GtkWidget          *remove_button;
  GtkWidget          *edit_button;
  GtkWidget          *up_button;
  GtkWidget          *down_button;
};

struct _LigmaControllerListClass
{
  GtkBoxClass   parent_class;
};


GType       ligma_controller_list_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_controller_list_new      (Ligma *ligma);


#endif  /*  __LIGMA_CONTROLLER_LIST_H__  */
