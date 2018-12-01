/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-scratch.h
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

#ifndef __GIMP_SCRATCH_H__
#define __GIMP_SCRATCH_H__


#define GIMP_SCRATCH_ALIGNMENT      16
#define GIMP_SCRATCH_MAX_BLOCK_SIZE (1 << 20)


/*  private types  */

typedef struct
{
  gsize  size;
  guint8 offset;
  guint8 padding[GIMP_SCRATCH_ALIGNMENT - (sizeof (gsize) + 1)];
  guint8 data[];
} GimpScratchBlock;

typedef struct
{
  GimpScratchBlock **blocks;
  gint               n_blocks;
  gint               n_available_blocks;
} GimpScratchContext;


/*  private variables  */

extern GPrivate gimp_scratch_context;


/*  private functions  */

GimpScratchBlock   * gimp_scratch_block_new    (gsize               size);
void                 gimp_scratch_block_free   (GimpScratchBlock   *block);

GimpScratchContext * gimp_scratch_context_new  (void);
void                 gimp_scratch_context_free (GimpScratchContext *context);


/*  public functions  */


inline gpointer
gimp_scratch_alloc (gsize size)
{
  GimpScratchContext *context;
  GimpScratchBlock   *block;

  if (! size)
    return NULL;

  if (size > GIMP_SCRATCH_MAX_BLOCK_SIZE)
    {
      block       = gimp_scratch_block_new (size);
      block->size = 0;

      return block->data;
    }

  context = g_private_get (&gimp_scratch_context);

  if (! context)
    {
      context = gimp_scratch_context_new ();

      g_private_set (&gimp_scratch_context, context);
    }

  if (context->n_available_blocks)
    {
      block = context->blocks[--context->n_available_blocks];

      if (block->size < size)
        {
          gimp_scratch_block_free (block);

          block = gimp_scratch_block_new (size);

          context->blocks[context->n_available_blocks] = block;
        }
    }
  else
    {
      block = gimp_scratch_block_new (size);
    }

  return block->data;
}

inline gpointer
gimp_scratch_alloc0 (gsize size)
{
  gpointer ptr;

  if (! size)
    return NULL;

  ptr = gimp_scratch_alloc (size);

  memset (ptr, 0, size);

  return ptr;
}

inline void
gimp_scratch_free (gpointer ptr)
{
  GimpScratchContext *context;
  GimpScratchBlock   *block;

  if (! ptr)
    return;

  block = (GimpScratchBlock *) ((guint8 *) ptr - GIMP_SCRATCH_ALIGNMENT);

  if (! block->size)
    {
      gimp_scratch_block_free (block);

      return;
    }

  context = g_private_get (&gimp_scratch_context);

  if (context->n_available_blocks == context->n_blocks)
    {
      context->n_blocks = MAX (2 * context->n_blocks, 1);
      context->blocks   = g_renew (GimpScratchBlock *, context->blocks,
                                   context->n_blocks);
    }

  context->blocks[context->n_available_blocks++] = block;
}


#define gimp_scratch_new(type, n) \
  ((type *) (gimp_scratch_alloc (sizeof (type) * (n))))

#define gimp_scratch_new0(type, n) \
  ((type *) (gimp_scratch_alloc0 (sizeof (type) * (n))))


/*  stats  */

guint64    gimp_scratch_get_total (void);


#endif /* __GIMP_SCRATCH_H__ */
