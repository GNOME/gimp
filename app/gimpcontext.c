/* The GIMP -- an image manipulation program
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
#include "gimpcontext.h"
#include "gimpsignal.h"

#define CONTEXT_CHECK_CURRENT(context) \
        ((context) = (context) ? (context) : current_context)

enum {
  OPACITY_CHANGED,
  PAINT_MODE_CHANGED,
  LAST_SIGNAL
};

static guint gimp_context_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass* parent_class = NULL;

/*  the currently active context  */
static GimpContext * current_context = NULL;

/*  for later use
static void
gimp_context_destroy (GtkObject *object)
{
  GimpContext *context;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_CONTEXT (object));

  context = GIMP_CONTEXT (object);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}
*/

static void
gimp_context_class_init (GimpContextClass *klass)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS (klass);

  parent_class = gtk_type_class (gimp_object_get_type ());

  gimp_context_signals[OPACITY_CHANGED] =
    gimp_signal_new ("opacity_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					opacity_changed),
		     gimp_sigtype_void);

  gimp_context_signals[PAINT_MODE_CHANGED] =
    gimp_signal_new ("paint_mode_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					paint_mode_changed),
		     gimp_sigtype_void);

  gtk_object_class_add_signals (object_class, gimp_context_signals,
				LAST_SIGNAL);

  klass->opacity_changed = NULL;
  klass->paint_mode_changed = NULL;

  /* object_class->destroy = gimp_context_destroy; */
}

static void
gimp_context_init (GimpContext *context)
{
  context->opacity    = 1.0;
  context->paint_mode = 0;
}

GtkType
gimp_context_get_type (void)
{
  static GtkType type = 0;

  if(!type)
    {
      GtkTypeInfo info =
      {
	"GimpContext",
	sizeof(GimpContext),
	sizeof(GimpContextClass),
	(GtkClassInitFunc) gimp_context_class_init,
	(GtkObjectInitFunc) gimp_context_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL
      };

      type = gtk_type_unique (gimp_object_get_type (), &info);
    }

  return type;
}

GimpContext *
gimp_context_new (void)
{
  GimpContext *context;

  context = gtk_type_new (gimp_context_get_type ());

  return context;
}

GimpContext *
gimp_context_get_current (void)
{
  return current_context;
}

void
gimp_context_set_current (GimpContext *context)
{
  current_context = context;
}

gdouble
gimp_context_get_opacity (GimpContext *context)
{
  CONTEXT_CHECK_CURRENT (context);
  g_return_val_if_fail (context != NULL, 1.0);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), 1.0);

  return context->opacity;
}

void
gimp_context_set_opacity (GimpContext *context,
			  gdouble      opacity)
{
  CONTEXT_CHECK_CURRENT (context);
  g_return_if_fail (context != NULL);
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  context->opacity = opacity;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[OPACITY_CHANGED]);
}

gint
gimp_context_get_paint_mode (GimpContext *context)
{
  CONTEXT_CHECK_CURRENT (context);
  g_return_val_if_fail (context != NULL, 0);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), 0);

  return context->paint_mode;
}

void
gimp_context_set_paint_mode (GimpContext *context,
			     gint         paint_mode)
{
  CONTEXT_CHECK_CURRENT (context);
  g_return_if_fail (context != NULL);
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  context->paint_mode = paint_mode;
  gtk_signal_emit (GTK_OBJECT(context),
		   gimp_context_signals[PAINT_MODE_CHANGED]);
}
