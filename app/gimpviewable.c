/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "apptypes.h"

#include "gimpmarshal.h"
#include "gimpviewable.h"
#include "temp_buf.h"


enum
{
  INVALIDATE_PREVIEW,
  GET_PREVIEW,
  GET_NEW_PREVIEW,
  LAST_SIGNAL
};


static void   gimp_viewable_class_init (GimpViewableClass *klass);
static void   gimp_viewable_init       (GimpViewable      *viewable);


static guint  viewable_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass *parent_class = NULL;


GtkType 
gimp_viewable_get_type (void)
{
  static GtkType viewable_type = 0;

  if (! viewable_type)
    {
      GtkTypeInfo viewable_info =
      {
        "GimpViewable",
        sizeof (GimpViewable),
        sizeof (GimpViewableClass),
        (GtkClassInitFunc) gimp_viewable_class_init,
        (GtkObjectInitFunc) gimp_viewable_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      viewable_type = gtk_type_unique (GIMP_TYPE_OBJECT, &viewable_info);
    }

  return viewable_type;
}

static void
gimp_viewable_class_init (GimpViewableClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);

  viewable_signals[INVALIDATE_PREVIEW] =
    gtk_signal_new ("invalidate_preview",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpViewableClass,
				       invalidate_preview),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);

  viewable_signals[GET_PREVIEW] =
    gtk_signal_new ("get_preview",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpViewableClass,
				       get_preview),
		    gimp_marshal_POINTER__INT_INT,
		    GTK_TYPE_POINTER, 3,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_INT,
		    GTK_TYPE_INT);

  viewable_signals[GET_NEW_PREVIEW] =
    gtk_signal_new ("get_new_preview",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpViewableClass,
				       get_new_preview),
		    gimp_marshal_POINTER__INT_INT,
		    GTK_TYPE_POINTER, 3,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_INT,
		    GTK_TYPE_INT);

  gtk_object_class_add_signals (object_class, viewable_signals, LAST_SIGNAL);

  klass->invalidate_preview = NULL;
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
  g_return_if_fail (viewable);
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  gtk_signal_emit (GTK_OBJECT (viewable), viewable_signals[INVALIDATE_PREVIEW]);
}

TempBuf *
gimp_viewable_get_preview (GimpViewable *viewable,
			   gint          width,
			   gint          height)
{
  TempBuf *temp_buf = NULL;

  g_return_val_if_fail (viewable, NULL);
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);

  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  gtk_signal_emit (GTK_OBJECT (viewable), viewable_signals[GET_PREVIEW],
		   width, height, &temp_buf);

  if (temp_buf)
    return temp_buf;

  temp_buf = gtk_object_get_data (GTK_OBJECT (viewable),
				  "static_viewable_preview");

  if (temp_buf)
    {
      if (temp_buf->width  == width  &&
	  temp_buf->height == height)
	return temp_buf;

      temp_buf_free (temp_buf);
      temp_buf = NULL;
    }

  gtk_signal_emit (GTK_OBJECT (viewable), viewable_signals[GET_NEW_PREVIEW],
		   width, height, &temp_buf);

  gtk_object_set_data (GTK_OBJECT (viewable), "static_viewable_preview",
		       temp_buf);

  return temp_buf;
}

TempBuf *
gimp_viewable_get_new_preview (GimpViewable *viewable,
			       gint          width,
			       gint          height)
{
  TempBuf *temp_buf = NULL;

  g_return_val_if_fail (viewable, NULL);
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);

  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  gtk_signal_emit (GTK_OBJECT (viewable), viewable_signals[GET_NEW_PREVIEW],
		   width, height, &temp_buf);

  if (temp_buf)
    return temp_buf;

  gtk_signal_emit (GTK_OBJECT (viewable), viewable_signals[GET_PREVIEW],
		   width, height, &temp_buf);

  if (temp_buf)
    return temp_buf_copy (temp_buf, NULL);

  return NULL;
}
