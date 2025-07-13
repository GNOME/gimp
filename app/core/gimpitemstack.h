/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpitemstack.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "gimpfilterstack.h"


#define GIMP_TYPE_ITEM_STACK            (gimp_item_stack_get_type ())
#define GIMP_ITEM_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ITEM_STACK, GimpItemStack))
#define GIMP_ITEM_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ITEM_STACK, GimpItemStackClass))
#define GIMP_IS_ITEM_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ITEM_STACK))
#define GIMP_IS_ITEM_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ITEM_STACK))


typedef struct _GimpItemStackClass GimpItemStackClass;

struct _GimpItemStack
{
  GimpFilterStack  parent_instance;
};

struct _GimpItemStackClass
{
  GimpFilterStackClass  parent_class;
};


GType           gimp_item_stack_get_type            (void) G_GNUC_CONST;
GimpContainer * gimp_item_stack_new                 (GType          item_type);

gint            gimp_item_stack_get_n_items         (GimpItemStack *stack);
gboolean        gimp_item_stack_is_flat             (GimpItemStack *stack);
GList         * gimp_item_stack_get_item_iter       (GimpItemStack *stack);
GList         * gimp_item_stack_get_item_list       (GimpItemStack *stack);
GimpItem      * gimp_item_stack_get_item_by_tattoo  (GimpItemStack *stack,
                                                     GimpTattoo     tattoo);
GimpItem      * gimp_item_stack_get_item_by_path    (GimpItemStack *stack,
                                                     GList         *path);
GimpItem      * gimp_item_stack_get_parent_by_path  (GimpItemStack *stack,
                                                     GList         *path,
                                                     gint          *index);

void            gimp_item_stack_invalidate_previews (GimpItemStack *stack);
void            gimp_item_stack_profile_changed     (GimpItemStack *stack);
