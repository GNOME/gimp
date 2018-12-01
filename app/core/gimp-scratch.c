/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-scratch.c
 * Copyright (C) 2018 Ell
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimp-scratch.h"


/*  local variables  */

GPrivate                 gimp_scratch_context =
  G_PRIVATE_INIT ((GDestroyNotify) gimp_scratch_context_free);
static volatile guintptr gimp_scratch_total;


/*  private functions  */


GimpScratchBlock *
gimp_scratch_block_new (gsize size)
{
  GimpScratchBlock *block;
  gint              offset;

  g_atomic_pointer_add (&gimp_scratch_total, +size);

  block = g_malloc ((GIMP_SCRATCH_ALIGNMENT - 1) +
                    sizeof (GimpScratchBlock)    +
                    size);

  offset  = GIMP_SCRATCH_ALIGNMENT -
            ((guintptr) block) % GIMP_SCRATCH_ALIGNMENT;
  offset %= GIMP_SCRATCH_ALIGNMENT;

  block = (GimpScratchBlock *) ((guint8 *) block + offset);

  block->size   = size;
  block->offset = offset;

  return block;
}

void
gimp_scratch_block_free (GimpScratchBlock *block)
{
  g_atomic_pointer_add (&gimp_scratch_total, -block->size);

  g_free ((guint8 *) block - block->offset);
}

GimpScratchContext *
gimp_scratch_context_new (void)
{
  return g_slice_new0 (GimpScratchContext);
}

void
gimp_scratch_context_free (GimpScratchContext *context)
{
  gint i;

  for (i = 0; i < context->n_available_blocks; i++)
    gimp_scratch_block_free (context->blocks[i]);

  g_free (context->blocks);

  g_slice_free (GimpScratchContext, context);
}
