/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "paint-types.h"

#include "core/gimpimage-undo.h"
#include "core/gimpimage.h"
#include "core/gimpundo.h"

#include "gimpink.h"
#include "gimpink-blob.h"
#include "gimpink-undo.h"


/**************/
/*  Ink Undo  */
/**************/

typedef struct _InkUndo InkUndo;

struct _InkUndo
{
  GimpInk *ink;

  Blob    *last_blob;

  gdouble  dt_buffer[DIST_SMOOTHER_BUFFER];
  gint     dt_index;

  guint32  ts_buffer[TIME_SMOOTHER_BUFFER];
  gint     ts_index;

  gdouble  last_time;

  gboolean init_velocity;
};


static gboolean  undo_pop_ink  (GimpUndo            *undo,
                                GimpUndoMode         undo_mode,
                                GimpUndoAccumulator *accum);
static void      undo_free_ink (GimpUndo            *undo,
                                GimpUndoMode         undo_mode);


gboolean
gimp_ink_push_undo (GimpPaintCore *core,
                    GimpImage     *image,
                    const gchar   *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_INK (core), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  if (! GIMP_PAINT_CORE_CLASS (g_type_class_peek_parent (GIMP_INK_GET_CLASS (core)))->push_undo (core, image, undo_desc))
    return FALSE;

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   sizeof (InkUndo),
                                   sizeof (InkUndo),
                                   GIMP_UNDO_INK, undo_desc,
                                   FALSE,
                                   undo_pop_ink,
                                   undo_free_ink,
                                   NULL)))
    {
      GimpInk *ink      = GIMP_INK (core);
      InkUndo *ink_undo = new->data;

      ink_undo->ink = ink;

      if (ink->start_blob)
        ink_undo->last_blob = blob_duplicate (ink->start_blob);

      memcpy (ink_undo->dt_buffer, ink->dt_buffer,
              sizeof (ink_undo->dt_buffer));

      ink_undo->dt_index = ink->dt_index;

      memcpy (ink_undo->ts_buffer, ink->ts_buffer,
              sizeof (ink_undo->ts_buffer));

      ink_undo->ts_index = ink->ts_index;

      ink_undo->last_time = ink->last_time;

      ink_undo->init_velocity = ink->init_velocity;

      g_object_add_weak_pointer (G_OBJECT (ink), (gpointer) &ink_undo->ink);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_ink (GimpUndo            *undo,
              GimpUndoMode         undo_mode,
              GimpUndoAccumulator *accum)
{
  InkUndo *ink_undo = undo->data;

  /*  only pop if the core still exists  */
  if (ink_undo->ink)
    {
      Blob    *tmp_blob;
      gint     tmp_int;
      gdouble  tmp_double;
      guint32  tmp_int_buf[DIST_SMOOTHER_BUFFER];
      gdouble  tmp_double_buf[DIST_SMOOTHER_BUFFER];

      tmp_blob = ink_undo->ink->last_blob;
      ink_undo->ink->last_blob = ink_undo->last_blob;
      ink_undo->last_blob = tmp_blob;

      memcpy (tmp_double_buf, ink_undo->ink->dt_buffer,
              sizeof (tmp_double_buf));
      memcpy (ink_undo->ink->dt_buffer, ink_undo->dt_buffer,
              sizeof (tmp_double_buf));
      memcpy (ink_undo->dt_buffer, tmp_double_buf,
              sizeof (tmp_double_buf));

      tmp_int = ink_undo->ink->dt_index;
      ink_undo->ink->dt_index = ink_undo->dt_index;
      ink_undo->dt_index = tmp_int;

      memcpy (tmp_int_buf, ink_undo->ink->ts_buffer,
              sizeof (tmp_int_buf));
      memcpy (ink_undo->ink->ts_buffer, ink_undo->ts_buffer,
              sizeof (tmp_int_buf));
      memcpy (ink_undo->ts_buffer, tmp_int_buf,
              sizeof (tmp_int_buf));

      tmp_int = ink_undo->ink->ts_index;
      ink_undo->ink->ts_index = ink_undo->ts_index;
      ink_undo->ts_index = tmp_int;

      tmp_double = ink_undo->ink->last_time;
      ink_undo->ink->last_time = ink_undo->last_time;
      ink_undo->last_time = tmp_double;

      tmp_int = ink_undo->ink->init_velocity;
      ink_undo->ink->init_velocity = ink_undo->init_velocity;
      ink_undo->init_velocity = tmp_int;
    }

  return TRUE;
}

static void
undo_free_ink (GimpUndo     *undo,
               GimpUndoMode  undo_mode)
{
  InkUndo *ink_undo = undo->data;

  if (ink_undo->ink)
    g_object_remove_weak_pointer (G_OBJECT (ink_undo->ink),
                                  (gpointer) &ink_undo->ink);

  if (ink_undo->last_blob)
    g_free (ink_undo->last_blob);

  g_free (ink_undo);
}
