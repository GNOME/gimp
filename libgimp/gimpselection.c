/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmaselection.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ligma.h"


struct _LigmaSelection
{
  LigmaChannel parent_instance;
};


G_DEFINE_TYPE (LigmaSelection, ligma_selection, LIGMA_TYPE_CHANNEL)

#define parent_class ligma_selection_parent_class


static void
ligma_selection_class_init (LigmaSelectionClass *klass)
{
}

static void
ligma_selection_init (LigmaSelection *selection)
{
}

/**
 * ligma_selection_get_by_id:
 * @selection_id: The selection id.
 *
 * Returns a #LigmaSelection representing @selection_id. This function
 * calls ligma_item_get_by_id() and returns the item if it is selection
 * or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #LigmaSelection for
 *          @selection_id or %NULL if @selection_id does not represent
 *          a valid selection. The object belongs to libligma and you
 *          must not modify or unref it.
 *
 * Since: 3.0
 **/
LigmaSelection *
ligma_selection_get_by_id (gint32 selection_id)
{
  LigmaItem *item = ligma_item_get_by_id (selection_id);

  if (LIGMA_IS_SELECTION (item))
    return (LigmaSelection *) item;

  return NULL;
}

/**
 * ligma_selection_float:
 * @image:       ignored
 * @n_drawables: Size of @drawables.
 * @drawables:   (array length=n_drawables): The drawables from which to
 *               float selection.
 * @offx:        x offset for translation.
 * @offy:        y offset for translation.
 *
 * Float the selection from the specified drawable with initial offsets
 * as specified.
 *
 * This procedure determines the region of the specified drawable that
 * lies beneath the current selection. The region is then cut from the
 * drawable and the resulting data is made into a new layer which is
 * instantiated as a floating selection. The offsets allow initial
 * positioning of the new floating selection.
 *
 * Returns: (transfer none): The floated layer.
 */
LigmaLayer *
ligma_selection_float (LigmaImage     *image,
                      gint           n_drawables,
                      LigmaDrawable **drawables,
                      gint           offx,
                      gint           offy)
{
  return _ligma_selection_float (n_drawables,
                                (const LigmaItem **) drawables,
                                offx,
                                offy);
}
