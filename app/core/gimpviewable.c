/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpviewable.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <glib-object.h>

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpmarshal.h"
#include "gimpviewable.h"


enum
{
  INVALIDATE_PREVIEW,
  SIZE_CHANGED,
  LAST_SIGNAL
};


static void   gimp_viewable_class_init         (GimpViewableClass *klass);
static void   gimp_viewable_init               (GimpViewable      *viewable);

static void   gimp_viewable_real_invalidate_preview (GimpViewable *viewable);


static guint  viewable_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass *parent_class = NULL;


GType 
gimp_viewable_get_type (void)
{
  static GType viewable_type = 0;

  if (! viewable_type)
    {
      static const GTypeInfo viewable_info =
      {
        sizeof (GimpViewableClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_viewable_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpViewable),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_viewable_init,
      };

      viewable_type = g_type_register_static (GIMP_TYPE_OBJECT,
					      "GimpViewable",
					      &viewable_info, 0);
    }

  return viewable_type;
}

static void
gimp_viewable_class_init (GimpViewableClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  viewable_signals[INVALIDATE_PREVIEW] =
    g_signal_new ("invalidate_preview",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpViewableClass, invalidate_preview),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  viewable_signals[SIZE_CHANGED] =
    g_signal_new ("size_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpViewableClass, size_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  klass->invalidate_preview = gimp_viewable_real_invalidate_preview;
  klass->size_changed       = NULL;

  klass->get_preview        = NULL;
  klass->get_new_preview    = NULL;
}

static void
gimp_viewable_init (GimpViewable *viewable)
{
}

void
gimp_viewable_invalidate_preview (GimpViewable *viewable)
{
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_signal_emit (G_OBJECT (viewable), viewable_signals[INVALIDATE_PREVIEW], 0);
}

void
gimp_viewable_size_changed (GimpViewable *viewable)
{
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_signal_emit (G_OBJECT (viewable), viewable_signals[SIZE_CHANGED], 0);
}

static void
gimp_viewable_real_invalidate_preview (GimpViewable *viewable)
{
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_object_set_data (G_OBJECT (viewable), "static-viewable-preview", NULL);
}

TempBuf *
gimp_viewable_get_preview (GimpViewable *viewable,
			   gint          width,
			   gint          height)
{
  GimpViewableClass *viewable_class;
  TempBuf           *temp_buf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_preview)
    temp_buf = viewable_class->get_preview (viewable, width, height);

  if (temp_buf)
    return temp_buf;

  temp_buf = g_object_get_data (G_OBJECT (viewable),
				"static-viewable-preview");

  if (temp_buf                   &&
      temp_buf->width  == width  &&
      temp_buf->height == height)
    return temp_buf;

  temp_buf = NULL;

  if (viewable_class->get_new_preview)
    temp_buf = viewable_class->get_new_preview (viewable, width, height);

  g_object_set_data_full (G_OBJECT (viewable), "static-viewable-preview",
			  temp_buf,
			  (GDestroyNotify) temp_buf_free);

  return temp_buf;
}

TempBuf *
gimp_viewable_get_new_preview (GimpViewable *viewable,
			       gint          width,
			       gint          height)
{
  GimpViewableClass *viewable_class;
  TempBuf           *temp_buf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_new_preview)
    temp_buf = viewable_class->get_new_preview (viewable, width, height);

  if (temp_buf)
    return temp_buf;

  if (viewable_class->get_preview)
    temp_buf = viewable_class->get_preview (viewable, width, height);

  if (temp_buf)
    return temp_buf_copy (temp_buf, NULL);

  return NULL;
}
