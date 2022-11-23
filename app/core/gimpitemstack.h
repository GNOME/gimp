/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmaitemstack.h
 * Copyright (C) 2008 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_ITEM_STACK_H__
#define __LIGMA_ITEM_STACK_H__

#include "ligmafilterstack.h"


#define LIGMA_TYPE_ITEM_STACK            (ligma_item_stack_get_type ())
#define LIGMA_ITEM_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ITEM_STACK, LigmaItemStack))
#define LIGMA_ITEM_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ITEM_STACK, LigmaItemStackClass))
#define LIGMA_IS_ITEM_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ITEM_STACK))
#define LIGMA_IS_ITEM_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ITEM_STACK))


typedef struct _LigmaItemStackClass LigmaItemStackClass;

struct _LigmaItemStack
{
  LigmaFilterStack  parent_instance;
};

struct _LigmaItemStackClass
{
  LigmaFilterStackClass  parent_class;
};


GType           ligma_item_stack_get_type            (void) G_GNUC_CONST;
LigmaContainer * ligma_item_stack_new                 (GType          item_type);

gint            ligma_item_stack_get_n_items         (LigmaItemStack *stack);
gboolean        ligma_item_stack_is_flat             (LigmaItemStack *stack);
GList         * ligma_item_stack_get_item_iter       (LigmaItemStack *stack);
GList         * ligma_item_stack_get_item_list       (LigmaItemStack *stack);
LigmaItem      * ligma_item_stack_get_item_by_tattoo  (LigmaItemStack *stack,
                                                     LigmaTattoo     tattoo);
LigmaItem      * ligma_item_stack_get_item_by_path    (LigmaItemStack *stack,
                                                     GList         *path);
LigmaItem      * ligma_item_stack_get_parent_by_path  (LigmaItemStack *stack,
                                                     GList         *path,
                                                     gint          *index);

void            ligma_item_stack_invalidate_previews (LigmaItemStack *stack);
void            ligma_item_stack_profile_changed     (LigmaItemStack *stack);


#endif  /*  __LIGMA_ITEM_STACK_H__  */
