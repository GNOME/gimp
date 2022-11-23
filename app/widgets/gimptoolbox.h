/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_TOOLBOX_H__
#define __LIGMA_TOOLBOX_H__


#include "ligmadock.h"


#define LIGMA_TYPE_TOOLBOX            (ligma_toolbox_get_type ())
#define LIGMA_TOOLBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOLBOX, LigmaToolbox))
#define LIGMA_TOOLBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOLBOX, LigmaToolboxClass))
#define LIGMA_IS_TOOLBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOLBOX))
#define LIGMA_IS_TOOLBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOLBOX))
#define LIGMA_TOOLBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOLBOX, LigmaToolboxClass))


typedef struct _LigmaToolboxClass   LigmaToolboxClass;
typedef struct _LigmaToolboxPrivate LigmaToolboxPrivate;

struct _LigmaToolbox
{
  LigmaDock parent_instance;

  LigmaToolboxPrivate *p;
};

struct _LigmaToolboxClass
{
  LigmaDockClass parent_class;
};


GType               ligma_toolbox_get_type           (void) G_GNUC_CONST;
GtkWidget         * ligma_toolbox_new                (LigmaDialogFactory *factory,
                                                     LigmaContext       *context,
                                                     LigmaUIManager     *ui_manager);
LigmaContext       * ligma_toolbox_get_context        (LigmaToolbox       *toolbox);
void                ligma_toolbox_set_drag_handler   (LigmaToolbox       *toolbox,
                                                     LigmaPanedBox      *drag_handler);



#endif /* __LIGMA_TOOLBOX_H__ */
