/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaimageview.h
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_IMAGE_VIEW_H__
#define __LIGMA_IMAGE_VIEW_H__


#include "ligmacontainereditor.h"


#define LIGMA_TYPE_IMAGE_VIEW            (ligma_image_view_get_type ())
#define LIGMA_IMAGE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_IMAGE_VIEW, LigmaImageView))
#define LIGMA_IMAGE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_IMAGE_VIEW, LigmaImageViewClass))
#define LIGMA_IS_IMAGE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_IMAGE_VIEW))
#define LIGMA_IS_IMAGE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_IMAGE_VIEW))
#define LIGMA_IMAGE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_IMAGE_VIEW, LigmaImageViewClass))


typedef struct _LigmaImageViewClass LigmaImageViewClass;

struct _LigmaImageView
{
  LigmaContainerEditor  parent_instance;

  GtkWidget           *raise_button;
  GtkWidget           *new_button;
  GtkWidget           *delete_button;
};

struct _LigmaImageViewClass
{
  LigmaContainerEditorClass  parent_class;
};


GType       ligma_image_view_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_image_view_new      (LigmaViewType     view_type,
                                      LigmaContainer   *container,
                                      LigmaContext     *context,
                                      gint             view_size,
                                      gint             view_border_width,
                                      LigmaMenuFactory *menu_factory);


#endif  /*  __LIGMA_IMAGE_VIEW_H__  */
