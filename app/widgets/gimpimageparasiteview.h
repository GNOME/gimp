/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaImageParasiteView
 * Copyright (C) 2006  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_IMAGE_PARASITE_VIEW_H__
#define __LIGMA_IMAGE_PARASITE_VIEW_H__


#define LIGMA_TYPE_IMAGE_PARASITE_VIEW            (ligma_image_parasite_view_get_type ())
#define LIGMA_IMAGE_PARASITE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_IMAGE_PARASITE_VIEW, LigmaImageParasiteView))
#define LIGMA_IMAGE_PARASITE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_IMAGE_PARASITE_VIEW, LigmaImageParasiteViewClass))
#define LIGMA_IS_IMAGE_PARASITE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_IMAGE_PARASITE_VIEW))
#define LIGMA_IS_IMAGE_PARASITE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_IMAGE_PARASITE_VIEW))
#define LIGMA_IMAGE_PARASITE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_IMAGE_PARASITE_VIEW, LigmaImageParasiteViewClass))


typedef struct _LigmaImageParasiteViewClass LigmaImageParasiteViewClass;

struct _LigmaImageParasiteView
{
  GtkBox     parent_instance;

  LigmaImage *image;
  gchar     *parasite;
};

struct _LigmaImageParasiteViewClass
{
  GtkBoxClass  parent_class;

  /*  signals  */
  void (* update) (LigmaImageParasiteView *view);
};


GType                ligma_image_parasite_view_get_type     (void) G_GNUC_CONST;

GtkWidget          * ligma_image_parasite_view_new          (LigmaImage   *image,
                                                            const gchar *parasite);
LigmaImage          * ligma_image_parasite_view_get_image    (LigmaImageParasiteView *view);
const LigmaParasite * ligma_image_parasite_view_get_parasite (LigmaImageParasiteView *view);


#endif /*  __LIGMA_IMAGE_PARASITE_VIEW_H__  */
