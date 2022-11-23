/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainerbox.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CONTAINER_BOX_H__
#define __LIGMA_CONTAINER_BOX_H__


#include "ligmaeditor.h"


#define LIGMA_TYPE_CONTAINER_BOX            (ligma_container_box_get_type ())
#define LIGMA_CONTAINER_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTAINER_BOX, LigmaContainerBox))
#define LIGMA_CONTAINER_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTAINER_BOX, LigmaContainerBoxClass))
#define LIGMA_IS_CONTAINER_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTAINER_BOX))
#define LIGMA_IS_CONTAINER_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTAINER_BOX))
#define LIGMA_CONTAINER_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTAINER_BOX, LigmaContainerBoxClass))


typedef struct _LigmaContainerBoxClass  LigmaContainerBoxClass;

struct _LigmaContainerBox
{
  LigmaEditor  parent_instance;

  GtkWidget  *scrolled_win;
};

struct _LigmaContainerBoxClass
{
  LigmaEditorClass  parent_class;
};


GType     ligma_container_box_get_type         (void) G_GNUC_CONST;

void      ligma_container_box_set_size_request (LigmaContainerBox *box,
                                               gint              width,
                                               gint              height);


#endif  /*  __LIGMA_CONTAINER_BOX_H__  */
