/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontext.c
 * Copyright (C) 1999-2001 Michael Natterer
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

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/base-config.h"
#include "base/temp-buf.h"

#include "gimp.h"
#include "gimpbrush.h"
#include "gimpbuffer.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpcoreconfig.h"
#include "gimpdatafactory.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimppalette.h"
#include "gimppattern.h"
#include "gimptoolinfo.h"

#include "gdisplay.h"


typedef void (* GimpContextCopyArgFunc) (GimpContext *src,
					 GimpContext *dest);


#define context_find_defined(context,arg_mask) \
        while (!(((context)->defined_args) & arg_mask) && (context)->parent) \
          (context) = (context)->parent


/*  local function prototypes  */

static void gimp_context_class_init          (GimpContextClass *klass);
static void gimp_context_init                (GimpContext      *context);
static void gimp_context_destroy             (GtkObject        *object);
static void gimp_context_set_arg             (GtkObject        *object,
					      GtkArg           *arg,
					      guint             arg_id);
static void gimp_context_get_arg             (GtkObject        *object,
					      GtkArg           *arg,
					      guint             arg_id);

/*  image  */
static void gimp_context_image_removed       (GimpContainer    *container,
					      GimpImage        *image,
					      GimpContext      *context);
static void gimp_context_real_set_image      (GimpContext      *context,
					      GimpImage        *image);
static void gimp_context_copy_image          (GimpContext      *src,
					      GimpContext      *dest);

/*  display  */
static void gimp_context_real_set_display    (GimpContext      *context,
					      GDisplay         *display);
static void gimp_context_copy_display        (GimpContext      *src,
					      GimpContext      *dest);

/*  tool  */
static void gimp_context_tool_dirty          (GimpToolInfo     *tool_info,
					      GimpContext      *context);
static void gimp_context_tool_removed        (GimpContainer    *container,
					      GimpToolInfo     *tool_info,
					      GimpContext      *context);
static void gimp_context_tool_list_thaw      (GimpContainer    *container,
					      GimpContext      *context);
static void gimp_context_real_set_tool       (GimpContext      *context,
					      GimpToolInfo     *tool_info);
static void gimp_context_copy_tool           (GimpContext      *src,
					      GimpContext      *dest);

/*  foreground  */
static void gimp_context_real_set_foreground (GimpContext      *context,
					      const GimpRGB    *color);
static void gimp_context_copy_foreground     (GimpContext      *src,
					      GimpContext      *dest);

/*  background  */
static void gimp_context_real_set_background (GimpContext      *context,
					      const GimpRGB    *color);
static void gimp_context_copy_background     (GimpContext      *src,
					      GimpContext      *dest);

/*  opacity  */
static void gimp_context_real_set_opacity    (GimpContext      *context,
					      gdouble           opacity);
static void gimp_context_copy_opacity        (GimpContext      *src,
					      GimpContext      *dest);

/*  paint mode  */
static void gimp_context_real_set_paint_mode (GimpContext      *context,
					      LayerModeEffects  paint_mode);
static void gimp_context_copy_paint_mode     (GimpContext      *src,
					      GimpContext      *dest);

/*  brush  */
static void gimp_context_brush_dirty         (GimpBrush        *brush,
					      GimpContext      *context);
static void gimp_context_brush_removed       (GimpContainer    *brush_list,
					      GimpBrush        *brush,
					      GimpContext      *context);
static void gimp_context_brush_list_thaw     (GimpContainer    *container,
					      GimpContext      *context);
static void gimp_context_real_set_brush      (GimpContext      *context,
					      GimpBrush        *brush);
static void gimp_context_copy_brush          (GimpContext      *src,
					      GimpContext      *dest);

/*  pattern  */
static void gimp_context_pattern_dirty       (GimpPattern      *pattern,
					      GimpContext      *context);
static void gimp_context_pattern_removed     (GimpContainer    *container,
					      GimpPattern      *pattern,
					      GimpContext      *context);
static void gimp_context_pattern_list_thaw   (GimpContainer    *container,
					      GimpContext      *context);
static void gimp_context_real_set_pattern    (GimpContext      *context,
					      GimpPattern      *pattern);
static void gimp_context_copy_pattern        (GimpContext      *src,
					      GimpContext      *dest);

/*  gradient  */
static void gimp_context_gradient_dirty      (GimpGradient     *gradient,
					      GimpContext      *context);
static void gimp_context_gradient_removed    (GimpContainer    *container,
					      GimpGradient     *gradient,
					      GimpContext      *context);
static void gimp_context_gradient_list_thaw  (GimpContainer    *container,
					      GimpContext      *context);
static void gimp_context_real_set_gradient   (GimpContext      *context,
					      GimpGradient     *gradient);
static void gimp_context_copy_gradient       (GimpContext      *src,
					      GimpContext      *dest);

/*  palette  */
static void gimp_context_palette_dirty       (GimpPalette      *palette,
					      GimpContext      *context);
static void gimp_context_palette_removed     (GimpContainer    *container,
					      GimpPalette      *palatte,
					      GimpContext      *context);
static void gimp_context_palette_list_thaw   (GimpContainer    *container,
					      GimpContext      *context);
static void gimp_context_real_set_palette    (GimpContext      *context,
					      GimpPalette      *palatte);
static void gimp_context_copy_palette        (GimpContext      *src,
					      GimpContext      *dest);

/*  buffer  */
static void gimp_context_buffer_dirty        (GimpBuffer       *buffer,
					      GimpContext      *context);
static void gimp_context_buffer_removed      (GimpContainer    *container,
					      GimpBuffer       *buffer,
					      GimpContext      *context);
static void gimp_context_buffer_list_thaw    (GimpContainer    *container,
					      GimpContext      *context);
static void gimp_context_real_set_buffer     (GimpContext      *context,
					      GimpBuffer       *buffer);
static void gimp_context_copy_buffer         (GimpContext      *src,
					      GimpContext      *dest);


/*  arguments & signals  */

enum
{
  ARG_0,
  ARG_IMAGE,
  ARG_DISPLAY,
  ARG_TOOL,
  ARG_FOREGROUND,
  ARG_BACKGROUND,
  ARG_OPACITY,
  ARG_PAINT_MODE,
  ARG_BRUSH,
  ARG_PATTERN,
  ARG_GRADIENT,
  ARG_PALETTE,
  ARG_BUFFER
};

enum
{
  IMAGE_CHANGED,
  DISPLAY_CHANGED,
  TOOL_CHANGED,
  FOREGROUND_CHANGED,
  BACKGROUND_CHANGED,
  OPACITY_CHANGED,
  PAINT_MODE_CHANGED,
  BRUSH_CHANGED,
  PATTERN_CHANGED,
  GRADIENT_CHANGED,
  PALETTE_CHANGED,
  BUFFER_CHANGED,
  LAST_SIGNAL
};

static gchar *gimp_context_arg_names[] =
{
  "GimpContext::image",
  "GimpContext::display",
  "GimpContext::tool",
  "GimpContext::foreground",
  "GimpContext::background",
  "GimpContext::opacity",
  "GimpContext::paint_mode",
  "GimpContext::brush",
  "GimpContext::pattern",
  "GimpContext::gradient",
  "GimpContext::palette",
  "GimpContext::buffer"
};

static GimpContextCopyArgFunc gimp_context_copy_arg_funcs[] =
{
  gimp_context_copy_image,
  gimp_context_copy_display,
  gimp_context_copy_tool,
  gimp_context_copy_foreground,
  gimp_context_copy_background,
  gimp_context_copy_opacity,
  gimp_context_copy_paint_mode,
  gimp_context_copy_brush,
  gimp_context_copy_pattern,
  gimp_context_copy_gradient,
  gimp_context_copy_palette,
  gimp_context_copy_buffer
};

static GType gimp_context_arg_types[] =
{
  0,
  G_TYPE_NONE,
  0,
  G_TYPE_NONE,
  G_TYPE_NONE,
  G_TYPE_NONE,
  G_TYPE_NONE,
  0,
  0,
  0,
  0,
  0
};

static gchar *gimp_context_signal_names[] =
{
  "image_changed",
  "display_changed",
  "tool_changed",
  "foreground_changed",
  "background_changed",
  "opacity_changed",
  "paint_mode_changed",
  "brush_changed",
  "pattern_changed",
  "gradient_changed",
  "palette_changed",
  "buffer_changed"
};

static GCallback gimp_context_signal_handlers[] =
{
  G_CALLBACK (gimp_context_real_set_image),
  G_CALLBACK (gimp_context_real_set_display),
  G_CALLBACK (gimp_context_real_set_tool),
  G_CALLBACK (gimp_context_real_set_foreground),
  G_CALLBACK (gimp_context_real_set_background),
  G_CALLBACK (gimp_context_real_set_opacity),
  G_CALLBACK (gimp_context_real_set_paint_mode),
  G_CALLBACK (gimp_context_real_set_brush),
  G_CALLBACK (gimp_context_real_set_pattern),
  G_CALLBACK (gimp_context_real_set_gradient),
  G_CALLBACK (gimp_context_real_set_palette),
  G_CALLBACK (gimp_context_real_set_buffer)
};


static guint gimp_context_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass * parent_class = NULL;


GType
gimp_context_get_type (void)
{
  static GType context_type = 0;

  if (! context_type)
    {
      static const GTypeInfo context_info =
      {
        sizeof (GimpContextClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_context_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpContext),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_context_init,
      };

      context_type = g_type_register_static (GIMP_TYPE_OBJECT,
					     "GimpContext", 
					     &context_info, 0);
    }

  return context_type;
}

static void
gimp_context_class_init (GimpContextClass *klass)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_context_arg_types[GIMP_CONTEXT_ARG_IMAGE]    = GIMP_TYPE_IMAGE;
  gimp_context_arg_types[GIMP_CONTEXT_ARG_TOOL]     = GIMP_TYPE_TOOL_INFO;
  gimp_context_arg_types[GIMP_CONTEXT_ARG_BRUSH]    = GIMP_TYPE_BRUSH;
  gimp_context_arg_types[GIMP_CONTEXT_ARG_PATTERN]  = GIMP_TYPE_PATTERN;
  gimp_context_arg_types[GIMP_CONTEXT_ARG_GRADIENT] = GIMP_TYPE_GRADIENT;
  gimp_context_arg_types[GIMP_CONTEXT_ARG_PALETTE]  = GIMP_TYPE_PALETTE;
  gimp_context_arg_types[GIMP_CONTEXT_ARG_BUFFER]   = GIMP_TYPE_BUFFER;

  gtk_object_add_arg_type (gimp_context_arg_names[IMAGE_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE,
			   ARG_IMAGE);
  gtk_object_add_arg_type (gimp_context_arg_names[DISPLAY_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE,
			   ARG_DISPLAY);
  gtk_object_add_arg_type (gimp_context_arg_names[TOOL_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE,
			   ARG_TOOL);
  gtk_object_add_arg_type (gimp_context_arg_names[FOREGROUND_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE,
			   ARG_FOREGROUND);
  gtk_object_add_arg_type (gimp_context_arg_names[BACKGROUND_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE,
			   ARG_BACKGROUND);
  gtk_object_add_arg_type (gimp_context_arg_names[OPACITY_CHANGED],
			   GTK_TYPE_DOUBLE, GTK_ARG_READWRITE,
			   ARG_OPACITY);
  gtk_object_add_arg_type (gimp_context_arg_names[PAINT_MODE_CHANGED],
			   GTK_TYPE_INT, GTK_ARG_READWRITE,
			   ARG_PAINT_MODE);
  gtk_object_add_arg_type (gimp_context_arg_names[BRUSH_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE,
			   ARG_BRUSH);
  gtk_object_add_arg_type (gimp_context_arg_names[PATTERN_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE,
			   ARG_PATTERN);
  gtk_object_add_arg_type (gimp_context_arg_names[GRADIENT_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE,
			   ARG_GRADIENT);
  gtk_object_add_arg_type (gimp_context_arg_names[PALETTE_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE,
			   ARG_PALETTE);
  gtk_object_add_arg_type (gimp_context_arg_names[BUFFER_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE,
			   ARG_BUFFER);

  gimp_context_signals[IMAGE_CHANGED] =
    g_signal_new (gimp_context_signal_names[IMAGE_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, image_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_context_signals[DISPLAY_CHANGED] =
    g_signal_new (gimp_context_signal_names[DISPLAY_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, display_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_context_signals[TOOL_CHANGED] =
    g_signal_new (gimp_context_signal_names[TOOL_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, tool_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_context_signals[FOREGROUND_CHANGED] =
    g_signal_new (gimp_context_signal_names[FOREGROUND_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, foreground_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_context_signals[BACKGROUND_CHANGED] =
    g_signal_new (gimp_context_signal_names[BACKGROUND_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, background_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_context_signals[OPACITY_CHANGED] =
    g_signal_new (gimp_context_signal_names[OPACITY_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, opacity_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__DOUBLE,
		  G_TYPE_NONE, 1,
		  G_TYPE_DOUBLE);

  gimp_context_signals[PAINT_MODE_CHANGED] =
    g_signal_new (gimp_context_signal_names[PAINT_MODE_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, paint_mode_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__INT,
		  G_TYPE_NONE, 1,
		  G_TYPE_INT);

  gimp_context_signals[BRUSH_CHANGED] =
    g_signal_new (gimp_context_signal_names[BRUSH_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, brush_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_context_signals[PATTERN_CHANGED] =
    g_signal_new (gimp_context_signal_names[PATTERN_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, pattern_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_context_signals[GRADIENT_CHANGED] =
    g_signal_new (gimp_context_signal_names[GRADIENT_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, gradient_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_context_signals[PALETTE_CHANGED] =
    g_signal_new (gimp_context_signal_names[PALETTE_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, palette_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_context_signals[BUFFER_CHANGED] =
    g_signal_new (gimp_context_signal_names[BUFFER_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, buffer_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  object_class->set_arg     = gimp_context_set_arg;
  object_class->get_arg     = gimp_context_get_arg;
  object_class->destroy     = gimp_context_destroy;

  klass->image_changed      = NULL;
  klass->display_changed    = NULL;
  klass->tool_changed       = NULL;
  klass->foreground_changed = NULL;
  klass->background_changed = NULL;
  klass->opacity_changed    = NULL;
  klass->paint_mode_changed = NULL;
  klass->brush_changed      = NULL;
  klass->pattern_changed    = NULL;
  klass->gradient_changed   = NULL;
  klass->palette_changed    = NULL;
  klass->buffer_changed     = NULL;
}

static void
gimp_context_init (GimpContext *context)
{
  context->gimp          = NULL;

  context->parent        = NULL;

  context->defined_args  = GIMP_CONTEXT_ALL_ARGS_MASK;

  context->image         = NULL;
  context->display       = NULL;

  context->tool_info     = NULL;
  context->tool_name     = NULL;

  gimp_rgba_set (&context->foreground, 0.0, 0.0, 0.0, 1.0);
  gimp_rgba_set (&context->background, 1.0, 1.0, 1.0, 1.0);

  context->opacity       = 1.0;
  context->paint_mode    = NORMAL_MODE;

  context->brush         = NULL;
  context->brush_name    = NULL;

  context->pattern       = NULL;
  context->pattern_name  = NULL;

  context->gradient      = NULL;
  context->gradient_name = NULL;

  context->palette       = NULL;
  context->palette_name  = NULL;

  context->buffer        = NULL;
}

static void
gimp_context_destroy (GtkObject *object)
{
  GimpContext *context;

  context = GIMP_CONTEXT (object);

  if (context->parent)
    gimp_context_unset_parent (context);

  context->image   = NULL;
  context->display = NULL;

  if (context->tool_info)
    {
      g_object_unref (G_OBJECT (context->tool_info));
      context->tool_info = NULL;
    }

  if (context->tool_name)
    {
      g_free (context->tool_name);
      context->tool_name = NULL;
    }

  if (context->brush)
    {
      g_object_unref (G_OBJECT (context->brush));
      context->brush = NULL;
    }

  if (context->brush_name)
    {
      g_free (context->brush_name);
      context->brush_name = NULL;
    }

  if (context->pattern)
    {
      g_object_unref (G_OBJECT (context->pattern));
      context->pattern = NULL;
    }

  if (context->pattern_name)
    {
      g_free (context->pattern_name);
      context->pattern_name = NULL;
    }

  if (context->gradient)
    {
      g_object_unref (G_OBJECT (context->gradient));
      context->gradient = NULL;
    }

  if (context->gradient_name)
    {
      g_free (context->gradient_name);
      context->gradient_name = NULL;
    }

  if (context->palette)
    {
      g_object_unref (G_OBJECT (context->palette));
      context->palette = NULL;
    }

  if (context->palette_name)
    {
      g_free (context->palette_name);
      context->palette_name = NULL;
    }

  if (context->buffer)
    {
      g_object_unref (G_OBJECT (context->buffer));
      context->buffer = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_context_set_arg (GtkObject *object,
		      GtkArg    *arg,
		      guint      arg_id)
{
  GimpContext *context;

  context = GIMP_CONTEXT (object);

  switch (arg_id)
    {
    case ARG_IMAGE:
      gimp_context_set_image (context, GTK_VALUE_POINTER (*arg));
      break;
    case ARG_DISPLAY:
      gimp_context_set_display (context, GTK_VALUE_POINTER (*arg));
      break;
    case ARG_TOOL:
      gimp_context_set_tool (context, GTK_VALUE_POINTER (*arg));
      break;
    case ARG_FOREGROUND:
      gimp_context_set_foreground (context, GTK_VALUE_POINTER (*arg));
      break;
    case ARG_BACKGROUND:
      gimp_context_set_background (context, GTK_VALUE_POINTER (*arg));
      break;
    case ARG_OPACITY:
      gimp_context_set_opacity (context, GTK_VALUE_DOUBLE (*arg));
      break;
    case ARG_PAINT_MODE:
      gimp_context_set_paint_mode (context, GTK_VALUE_INT (*arg));
      break;
    case ARG_BRUSH:
      gimp_context_set_brush (context, GTK_VALUE_POINTER (*arg));
      break;
    case ARG_PATTERN:
      gimp_context_set_pattern (context, GTK_VALUE_POINTER (*arg));
      break;
    case ARG_GRADIENT:
      gimp_context_set_gradient (context, GTK_VALUE_POINTER (*arg));
      break;
    case ARG_PALETTE:
      gimp_context_set_palette (context, GTK_VALUE_POINTER (*arg));
      break;
    case ARG_BUFFER:
      gimp_context_set_buffer (context, GTK_VALUE_POINTER (*arg));
      break;
    default:
      break;
    }
}

static void
gimp_context_get_arg (GtkObject *object,
		      GtkArg    *arg,
		      guint      arg_id)
{
  GimpContext *context;

  context = GIMP_CONTEXT (object);

  switch (arg_id)
    {
    case ARG_IMAGE:
      GTK_VALUE_POINTER (*arg) = gimp_context_get_image (context);
      break;
    case ARG_DISPLAY:
      GTK_VALUE_POINTER (*arg) = gimp_context_get_display (context);
      break;
    case ARG_TOOL:
      GTK_VALUE_POINTER (*arg) = gimp_context_get_tool (context);
      break;
    case ARG_FOREGROUND:
      gimp_context_get_foreground (context, GTK_VALUE_POINTER (*arg));
      break;
    case ARG_BACKGROUND:
      gimp_context_get_background (context, GTK_VALUE_POINTER (*arg));
      break;
    case ARG_OPACITY:
      GTK_VALUE_DOUBLE (*arg) = gimp_context_get_opacity (context);
      break;
    case ARG_PAINT_MODE:
      GTK_VALUE_INT (*arg) = gimp_context_get_paint_mode (context);
      break;
    case ARG_BRUSH:
      GTK_VALUE_POINTER (*arg) = gimp_context_get_brush (context);
      break;
    case ARG_PATTERN:
      GTK_VALUE_POINTER (*arg) = gimp_context_get_pattern (context);
      break;
    case ARG_GRADIENT:
      GTK_VALUE_POINTER (*arg) = gimp_context_get_gradient (context);
      break;
    case ARG_PALETTE:
      GTK_VALUE_POINTER (*arg) = gimp_context_get_palette (context);
      break;
    case ARG_BUFFER:
      GTK_VALUE_POINTER (*arg) = gimp_context_get_buffer (context);
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

/*****************************************************************************/
/*  public functions  ********************************************************/

GimpContext *
gimp_context_new (Gimp        *gimp,
		  const gchar *name,
		  GimpContext *template)
{
  GimpContext *context;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (! template || GIMP_IS_CONTEXT (template), NULL);

  context = g_object_new (GIMP_TYPE_CONTEXT, NULL);

  context->gimp = gimp;

  gimp_object_set_name (GIMP_OBJECT (context), name);

  g_signal_connect_object (G_OBJECT (gimp->images), "remove",
			   G_CALLBACK (gimp_context_image_removed),
			   G_OBJECT (context),
			   0);

  g_signal_connect_object (G_OBJECT (gimp->tool_info_list), "remove",
			   G_CALLBACK (gimp_context_tool_removed),
			   G_OBJECT (context),
			   0);
  g_signal_connect_object (G_OBJECT (gimp->tool_info_list), "thaw",
			   G_CALLBACK (gimp_context_tool_list_thaw),
			   G_OBJECT (context),
			   0);

  g_signal_connect_object (G_OBJECT (gimp->brush_factory->container), "remove",
			   G_CALLBACK (gimp_context_brush_removed),
			   G_OBJECT (context),
			   0);
  g_signal_connect_object (G_OBJECT (gimp->brush_factory->container), "thaw",
			   G_CALLBACK (gimp_context_brush_list_thaw),
			   G_OBJECT (context),
			   0);

  g_signal_connect_object (G_OBJECT (gimp->pattern_factory->container), "remove",
			   G_CALLBACK (gimp_context_pattern_removed),
			   G_OBJECT (context),
			   0);
  g_signal_connect_object (G_OBJECT (gimp->pattern_factory->container), "thaw",
			   G_CALLBACK (gimp_context_pattern_list_thaw),
			   G_OBJECT (context),
			   0);

  g_signal_connect_object (G_OBJECT (gimp->gradient_factory->container), "remove",
			   G_CALLBACK (gimp_context_gradient_removed),
			   G_OBJECT (context),
			   0);
  g_signal_connect_object (G_OBJECT (gimp->gradient_factory->container), "thaw",
			   G_CALLBACK (gimp_context_gradient_list_thaw),
			   G_OBJECT (context),
			   0);

  g_signal_connect_object (G_OBJECT (gimp->palette_factory->container), "remove",
			   G_CALLBACK (gimp_context_palette_removed),
			   G_OBJECT (context),
			   0);
  g_signal_connect_object (G_OBJECT (gimp->palette_factory->container), "thaw",
			   G_CALLBACK (gimp_context_palette_list_thaw),
			   G_OBJECT (context),
			   0);

  g_signal_connect_object (G_OBJECT (gimp->named_buffers), "remove",
			   G_CALLBACK (gimp_context_buffer_removed),
			   G_OBJECT (context),
			   0);
  g_signal_connect_object (G_OBJECT (gimp->named_buffers), "thaw",
			   G_CALLBACK (gimp_context_buffer_list_thaw),
			   G_OBJECT (context),
			   0);

  if (template)
    {
      context->defined_args = template->defined_args;

      gimp_context_copy_args (template, context, GIMP_CONTEXT_ALL_ARGS_MASK);
    }

  return context;
}

const gchar *
gimp_context_get_name (const GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return gimp_object_get_name (GIMP_OBJECT (context));
}

void
gimp_context_set_name (GimpContext *context,
		       const gchar *name)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  if (! name)
    name = "Unnamed";

  gimp_object_set_name (GIMP_OBJECT (context), name);
}

GimpContext *
gimp_context_get_parent (const GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->parent;
}

void
gimp_context_set_parent (GimpContext *context,
			 GimpContext *parent)
{
  GimpContextArgType arg;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (!parent || GIMP_IS_CONTEXT (parent));

  if (context == parent || context->parent == parent)
    return;

  for (arg = 0; arg < GIMP_CONTEXT_NUM_ARGS; arg++)
    if (! ((1 << arg) & context->defined_args))
      {
	gimp_context_copy_arg (parent, context, arg);
	g_signal_connect_object (G_OBJECT (parent),
				 gimp_context_signal_names[arg],
				 gimp_context_signal_handlers[arg],
				 G_OBJECT (context),
				 G_CONNECT_SWAPPED);
      }

  context->parent = parent;
}

void
gimp_context_unset_parent (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  if (context->parent)
    {
      if (context->defined_args != GIMP_CONTEXT_ALL_ARGS_MASK)
	gtk_signal_disconnect_by_data (GTK_OBJECT (context->parent), context);

      context->parent = NULL;
    }
}

/*  define / undefinine context arguments  */

void
gimp_context_define_arg (GimpContext        *context,
			 GimpContextArgType  arg,
			 gboolean            defined)
{
  GimpContextArgMask mask;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail ((arg >= 0) && (arg < GIMP_CONTEXT_NUM_ARGS));

  mask = (1 << arg);

  if (defined)
    {
      if (! (context->defined_args & mask))
	{
	  context->defined_args |= mask;
	  if (context->parent)
	    g_signal_handlers_disconnect_by_func (G_OBJECT (context->parent),
						  gimp_context_signal_handlers[arg],
						  context);
	}
    }
  else
    {
      if (context->defined_args & mask)
	{
	  context->defined_args &= ~mask;
	  if (context->parent)
	    {
	      gimp_context_copy_arg (context->parent, context, arg);
	      g_signal_connect_object (G_OBJECT (context->parent),
				       gimp_context_signal_names[arg],
				       gimp_context_signal_handlers[arg],
				       G_OBJECT (context),
				       G_CONNECT_SWAPPED);
	    }
	}
    }
}

gboolean
gimp_context_arg_defined (GimpContext        *context,
			  GimpContextArgType  arg)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  return (context->defined_args & (1 << arg)) ? TRUE : FALSE;
}

void
gimp_context_define_args (GimpContext        *context,
			  GimpContextArgMask  args_mask,
			  gboolean            defined)
{
  GimpContextArgType arg;

  g_return_if_fail (GIMP_IS_CONTEXT (context));

  for (arg = 0; arg < GIMP_CONTEXT_NUM_ARGS; arg++)
    if ((1 << arg) & args_mask)
      gimp_context_define_arg (context, arg, defined);
}

/*  copying context arguments  */

void
gimp_context_copy_arg (GimpContext        *src,
		       GimpContext        *dest,
		       GimpContextArgType  arg)
{
  g_return_if_fail (GIMP_IS_CONTEXT (src));
  g_return_if_fail (GIMP_IS_CONTEXT (dest));
  g_return_if_fail ((arg >= 0) && (arg < GIMP_CONTEXT_NUM_ARGS));

  gimp_context_copy_arg_funcs[arg] (src, dest);
}

void
gimp_context_copy_args (GimpContext        *src,
			GimpContext        *dest,
			GimpContextArgMask  args_mask)
{
  GimpContextArgType arg;

  g_return_if_fail (GIMP_IS_CONTEXT (src));
  g_return_if_fail (GIMP_IS_CONTEXT (dest));

  for (arg = 0; arg < GIMP_CONTEXT_NUM_ARGS; arg++)
    if ((1 << arg) & args_mask)
      {
	gimp_context_copy_arg (src, dest, arg);
      }
}

/*  attribute access functions  */

/*****************************************************************************/
/*  manipulate by GType  *****************************************************/

GimpContextArgType
gimp_context_type_to_arg (GType type)
{
  gint i;

  for (i = 0; i < GIMP_CONTEXT_NUM_ARGS; i++)
    {
      if (g_type_is_a (type, gimp_context_arg_types[i]))
	return i;
    }

  return -1;
}

const gchar *
gimp_context_type_to_signal_name (GType type)
{
  gint i;

  for (i = 0; i < GIMP_CONTEXT_NUM_ARGS; i++)
    {
      if (g_type_is_a (type, gimp_context_arg_types[i]))
	return gimp_context_signal_names[i];
    }

  return NULL;
}

GimpObject *
gimp_context_get_by_type (GimpContext *context,
			  GType        type)
{
  GimpContextArgType  arg;
  GimpObject         *object = NULL;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail ((arg = gimp_context_type_to_arg (type)) != -1, NULL);

  gtk_object_get (GTK_OBJECT (context),
		  gimp_context_arg_names[arg], &object,
		  NULL);

  return object;
}

void
gimp_context_set_by_type (GimpContext *context,
			  GtkType      type,
			  GimpObject  *object)
{
  GimpContextArgType arg;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail ((arg = gimp_context_type_to_arg (type)) != -1);

  gtk_object_set (GTK_OBJECT (context),
		  gimp_context_arg_names[arg], object,
		  NULL);
}

void
gimp_context_changed_by_type (GimpContext *context,
			      GType        type)
{
  GimpContextArgType  arg;
  GimpObject         *object;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail ((arg = gimp_context_type_to_arg (type)) != -1);

  object = gimp_context_get_by_type (context, type);

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[arg], 0,
		 object);
}

/*****************************************************************************/
/*  image  *******************************************************************/

GimpImage *
gimp_context_get_image (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->image;
}

void
gimp_context_set_image (GimpContext *context,
			GimpImage   *image)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_IMAGE_MASK);

  gimp_context_real_set_image (context, image);
}

void
gimp_context_image_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[IMAGE_CHANGED], 0,
		 context->image);
}

/*  handle disappearing images  */
static void
gimp_context_image_removed (GimpContainer *container,
			    GimpImage     *image,
			    GimpContext   *context)
{
  if (context->image == image)
    gimp_context_real_set_image (context, NULL);
}

static void
gimp_context_real_set_image (GimpContext *context,
			     GimpImage   *image)
{
  if (context->image == image)
    return;

  context->image = image;

  gimp_context_image_changed (context);
}

static void
gimp_context_copy_image (GimpContext *src,
			 GimpContext *dest)
{
  gimp_context_real_set_image (dest, src->image);
}

/*****************************************************************************/
/*  display  *****************************************************************/

GDisplay *
gimp_context_get_display (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->display;
}

void
gimp_context_set_display (GimpContext *context,
			  GDisplay    *display)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_DISPLAY_MASK);

  gimp_context_real_set_display (context, display);
}

void
gimp_context_display_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[DISPLAY_CHANGED], 0,
		 context->display);
}

/*  handle dissapearing displays  */
static void
gimp_context_display_destroy (GtkWidget   *disp_shell,
			      GimpContext *context)
{
  context->display = NULL;
  gimp_context_display_changed (context);
}

static void
gimp_context_real_set_display (GimpContext *context,
			       GDisplay    *display)
{
  if (context->display == display)
    return;

  if (context->display && GTK_IS_OBJECT (context->display->shell))
    gtk_signal_disconnect_by_data (GTK_OBJECT (context->display->shell),
				   context);

  if (display)
    g_signal_connect_object (G_OBJECT (display->shell), "destroy",
			     G_CALLBACK (gimp_context_display_destroy),
			     G_OBJECT (context),
			     0);

  context->display = display;

  /*  set the image _before_ emitting the display_changed signal  */
  if (display)
    gimp_context_real_set_image (context, display->gimage);

  gimp_context_display_changed (context);
}

static void
gimp_context_copy_display (GimpContext *src,
			   GimpContext *dest)
{
  gimp_context_real_set_display (dest, src->display);
}

/*****************************************************************************/
/*  tool  ********************************************************************/

static GimpToolInfo *standard_tool_info = NULL;

GimpToolInfo *
gimp_context_get_tool (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->tool_info;
}

void
gimp_context_set_tool (GimpContext  *context,
		       GimpToolInfo *tool_info)
{
  g_return_if_fail (! context || GIMP_IS_CONTEXT (context));
  g_return_if_fail (! tool_info || GIMP_IS_TOOL_INFO (tool_info));

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_TOOL_MASK);

  gimp_context_real_set_tool (context, tool_info);
}

void
gimp_context_tool_changed (GimpContext *context)
{
  g_return_if_fail (! context || GIMP_IS_CONTEXT (context));

  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[TOOL_CHANGED], 0,
		 context->tool_info);
}

/*  the active tool was modified  */
static void
gimp_context_tool_dirty (GimpToolInfo *tool_info,
			 GimpContext  *context)
{
  g_free (context->tool_name);
  context->tool_name = g_strdup (GIMP_OBJECT (tool_info)->name);

  gimp_context_tool_changed (context);
}

/*  the global tool list is there again after refresh  */
static void
gimp_context_tool_list_thaw (GimpContainer *container,
			     GimpContext   *context)
{
  GimpToolInfo *tool_info;

  if (! context->tool_name)
    context->tool_name = g_strdup ("Color Picker");

  if ((tool_info = (GimpToolInfo *)
       gimp_container_get_child_by_name (container,
					 context->tool_name)))
    {
      gimp_context_real_set_tool (context, tool_info);
      return;
    }

  if (gimp_container_num_children (container))
    gimp_context_real_set_tool
      (context,
       GIMP_TOOL_INFO (gimp_container_get_child_by_index (container, 0)));
  else
    gimp_context_real_set_tool (context, gimp_tool_info_get_standard ());
}

/*  the active tool disappeared  */
static void
gimp_context_tool_removed (GimpContainer *container,
			   GimpToolInfo  *tool_info,
			   GimpContext   *context)
{
  if (tool_info == context->tool_info)
    {
      context->tool_info = NULL;

      g_signal_handlers_disconnect_by_func (G_OBJECT (tool_info),
					    gimp_context_tool_dirty,
					    context);
      g_object_unref (G_OBJECT (tool_info));

      if (! gimp_container_frozen (container))
	gimp_context_tool_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_tool (GimpContext  *context,
			    GimpToolInfo *tool_info)
{
  if (! standard_tool_info)
    standard_tool_info = gimp_tool_info_get_standard ();

  if (context->tool_info == tool_info)
    return;

  if (context->tool_name && tool_info != standard_tool_info)
    {
      g_free (context->tool_name);
      context->tool_name = NULL;
    }

  /*  disconnect from the old tool's signals  */
  if (context->tool_info)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (context->tool_info),
					    gimp_context_tool_dirty,
					    context);
      g_object_unref (G_OBJECT (context->tool_info));
    }

  context->tool_info = tool_info;

  if (tool_info)
    {
      g_object_ref (G_OBJECT (tool_info));

      g_signal_connect_object (G_OBJECT (tool_info), "invalidate_preview",
			       G_CALLBACK (gimp_context_tool_dirty),
			       G_OBJECT (context),
			       0);
      g_signal_connect_object (G_OBJECT (tool_info), "name_changed",
			       G_CALLBACK (gimp_context_tool_dirty),
			       G_OBJECT (context),
			       0);

      if (tool_info != standard_tool_info)
	context->tool_name = g_strdup (GIMP_OBJECT (tool_info)->name);
    }

  gimp_context_tool_changed (context);
}

static void
gimp_context_copy_tool (GimpContext *src,
			GimpContext *dest)
{
  gimp_context_real_set_tool (dest, src->tool_info);

  if ((!src->tool_info || src->tool_info == standard_tool_info) &&
      src->tool_name)
    {
      g_free (dest->tool_name);
      dest->tool_name = g_strdup (src->tool_name);
    }
}


/*****************************************************************************/
/*  foreground color  ********************************************************/

void
gimp_context_get_foreground (GimpContext *context,
			     GimpRGB     *color)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_return_if_fail (color != NULL);

  *color = context->foreground;
}

void
gimp_context_set_foreground (GimpContext   *context,
			     const GimpRGB *color)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_FOREGROUND_MASK);

  g_return_if_fail (color != NULL);

  gimp_context_real_set_foreground (context, color);
}

void
gimp_context_foreground_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[FOREGROUND_CHANGED], 0,
		 &context->foreground);
}

static void
gimp_context_real_set_foreground (GimpContext   *context,
				  const GimpRGB *color)
{
  if (gimp_rgba_distance (&context->foreground, color) < 0.0001)
    return;

  context->foreground = *color;

  gimp_context_foreground_changed (context);
}

static void
gimp_context_copy_foreground (GimpContext *src,
			      GimpContext *dest)
{
  gimp_context_real_set_foreground (dest, &src->foreground);
}

/*****************************************************************************/
/*  background color  ********************************************************/

void
gimp_context_get_background (GimpContext *context,
			     GimpRGB     *color)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_return_if_fail (color != NULL);

  *color = context->background;
}

void
gimp_context_set_background (GimpContext   *context,
			     const GimpRGB *color)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_BACKGROUND_MASK);

  g_return_if_fail (color != NULL);

  gimp_context_real_set_background (context, color);
}

void
gimp_context_background_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[BACKGROUND_CHANGED], 0,
		 &context->background);
}

static void
gimp_context_real_set_background (GimpContext   *context,
				  const GimpRGB *color)
{
  if (gimp_rgba_distance (&context->background, color) < 0.0001)
    return;

  context->background = *color;

  gimp_context_background_changed (context);
}

static void
gimp_context_copy_background (GimpContext *src,
			      GimpContext *dest)
{
  gimp_context_real_set_background (dest, &src->background);
}

/*****************************************************************************/
/*  color utility functions  *************************************************/

void
gimp_context_set_default_colors (GimpContext *context)
{
  GimpContext *bg_context;
  GimpRGB      fg;
  GimpRGB      bg;

  bg_context = context;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_FOREGROUND_MASK);
  context_find_defined (bg_context, GIMP_CONTEXT_BACKGROUND_MASK);

  gimp_rgba_set (&fg, 0.0, 0.0, 0.0, 1.0);
  gimp_rgba_set (&bg, 1.0, 1.0, 1.0, 1.0);

  gimp_context_real_set_foreground (context, &fg);
  gimp_context_real_set_background (bg_context, &bg);
}

void
gimp_context_swap_colors (GimpContext *context)
{
  GimpContext *bg_context;
  GimpRGB      fg;
  GimpRGB      bg;

  bg_context = context;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_FOREGROUND_MASK);
  context_find_defined (bg_context, GIMP_CONTEXT_BACKGROUND_MASK);

  gimp_context_get_foreground (context, &fg);
  gimp_context_get_background (bg_context, &bg);

  gimp_context_real_set_foreground (context, &bg);
  gimp_context_real_set_background (bg_context, &fg);
}

/*****************************************************************************/
/*  opacity  *****************************************************************/

gdouble
gimp_context_get_opacity (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), 1.0);

  return context->opacity;
}

void
gimp_context_set_opacity (GimpContext *context,
			  gdouble      opacity)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_OPACITY_MASK);

  gimp_context_real_set_opacity (context, opacity);
}

void
gimp_context_opacity_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[OPACITY_CHANGED], 0,
		 context->opacity);
}

static void
gimp_context_real_set_opacity (GimpContext *context,
			       gdouble      opacity)
{
  if (context->opacity == opacity)
    return;

  context->opacity = opacity;
  gimp_context_opacity_changed (context);
}

static void
gimp_context_copy_opacity (GimpContext *src,
			   GimpContext *dest)
{
  gimp_context_real_set_opacity (dest, src->opacity);
}

/*****************************************************************************/
/*  paint mode  **************************************************************/

LayerModeEffects
gimp_context_get_paint_mode (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NORMAL_MODE);

  return context->paint_mode;
}

void
gimp_context_set_paint_mode (GimpContext     *context,
			     LayerModeEffects paint_mode)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_PAINT_MODE_MASK);

  gimp_context_real_set_paint_mode (context, paint_mode);
}

void
gimp_context_paint_mode_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[PAINT_MODE_CHANGED], 0,
		 context->paint_mode);
}

static void
gimp_context_real_set_paint_mode (GimpContext     *context,
				  LayerModeEffects paint_mode)
{
  if (context->paint_mode == paint_mode)
    return;

  context->paint_mode = paint_mode;
  gimp_context_paint_mode_changed (context);
}

static void
gimp_context_copy_paint_mode (GimpContext *src,
			      GimpContext *dest)
{
  gimp_context_real_set_paint_mode (dest, src->paint_mode);
}

/*****************************************************************************/
/*  brush  *******************************************************************/

static GimpBrush *standard_brush = NULL;

GimpBrush *
gimp_context_get_brush (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->brush;
}

void
gimp_context_set_brush (GimpContext *context,
			GimpBrush   *brush)
{
  g_return_if_fail (! context || GIMP_IS_CONTEXT (context));
  g_return_if_fail (! brush || GIMP_IS_BRUSH (brush));

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_BRUSH_MASK);

  gimp_context_real_set_brush (context, brush);
}

void
gimp_context_brush_changed (GimpContext *context)
{
  g_return_if_fail (! context || GIMP_IS_CONTEXT (context));

  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[BRUSH_CHANGED], 0,
		 context->brush);
}

/*  the active brush was modified  */
static void
gimp_context_brush_dirty (GimpBrush   *brush,
			  GimpContext *context)
{
  g_free (context->brush_name);
  context->brush_name = g_strdup (GIMP_OBJECT (brush)->name);

  gimp_context_brush_changed (context);
}

/*  the global brush list is there again after refresh  */
static void
gimp_context_brush_list_thaw (GimpContainer *container,
			      GimpContext   *context)
{
  GimpBrush *brush;

  if (! context->brush_name)
    context->brush_name = g_strdup (context->gimp->config->default_brush);

  if ((brush = (GimpBrush *)
       gimp_container_get_child_by_name (container,
					 context->brush_name)))
    {
      gimp_context_real_set_brush (context, brush);
      return;
    }

  if (gimp_container_num_children (container))
    gimp_context_real_set_brush 
      (context, GIMP_BRUSH (gimp_container_get_child_by_index (container, 0)));
  else
    gimp_context_real_set_brush (context,
				 GIMP_BRUSH (gimp_brush_get_standard ()));
}

/*  the active brush disappeared  */
static void
gimp_context_brush_removed (GimpContainer *container,
			    GimpBrush     *brush,
			    GimpContext   *context)
{
  if (brush == context->brush)
    {
      context->brush = NULL;

      g_signal_handlers_disconnect_by_func (G_OBJECT (brush),
					    gimp_context_brush_dirty,
					    context);
      g_object_unref (G_OBJECT (brush));

      if (! gimp_container_frozen (container))
	gimp_context_brush_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_brush (GimpContext *context,
			     GimpBrush   *brush)
{
  if (! standard_brush)
    standard_brush = GIMP_BRUSH (gimp_brush_get_standard ());

  if (context->brush == brush)
    return;

  if (context->brush_name && brush != standard_brush)
    {
      g_free (context->brush_name);
      context->brush_name = NULL;
    }

  /*  make sure the active brush is swapped before we get a new one...  */
  if (base_config->stingy_memory_use         &&
      context->brush && context->brush->mask &&
      G_OBJECT (context->brush)->ref_count == 2)
    {
      temp_buf_swap (brush->mask);
    }

  /*  disconnect from the old brush's signals  */
  if (context->brush)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (context->brush),
					    gimp_context_brush_dirty,
					    context);
      g_object_unref (G_OBJECT (context->brush));
    }

  context->brush = brush;

  if (brush)
    {
      g_object_ref (G_OBJECT (brush));

      g_signal_connect_object (G_OBJECT (brush), "invalidate_preview",
			       G_CALLBACK (gimp_context_brush_dirty),
			       G_OBJECT (context),
			       0);
      g_signal_connect_object (G_OBJECT (brush), "name_changed",
			       G_CALLBACK (gimp_context_brush_dirty),
			       G_OBJECT (context),
			       0);

      /*  Make sure the active brush is unswapped... */
      if (base_config->stingy_memory_use &&
	  brush->mask                    &&
	  G_OBJECT (brush)->ref_count < 2)
	{
	  temp_buf_unswap (brush->mask);
	}

      if (brush != standard_brush)
	context->brush_name = g_strdup (GIMP_OBJECT (brush)->name);
    }

  gimp_context_brush_changed (context);
}

static void
gimp_context_copy_brush (GimpContext *src,
			 GimpContext *dest)
{
  gimp_context_real_set_brush (dest, src->brush);

  if ((!src->brush || src->brush == standard_brush) && src->brush_name)
    {
      g_free (dest->brush_name);
      dest->brush_name = g_strdup (src->brush_name);
    }
}


/*****************************************************************************/
/*  pattern  *****************************************************************/

static GimpPattern *standard_pattern = NULL;

GimpPattern *
gimp_context_get_pattern (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->pattern;
}

void
gimp_context_set_pattern (GimpContext *context,
			  GimpPattern *pattern)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_PATTERN_MASK);

  gimp_context_real_set_pattern (context, pattern);
}

void
gimp_context_pattern_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[PATTERN_CHANGED], 0,
		 context->pattern);
}

/*  the active pattern was modified  */
static void
gimp_context_pattern_dirty (GimpPattern *pattern,
			    GimpContext *context)
{
  g_free (context->pattern_name);
  context->pattern_name = g_strdup (GIMP_OBJECT (pattern)->name);

  gimp_context_pattern_changed (context);
}

/*  the global pattern list is there again after refresh  */
static void
gimp_context_pattern_list_thaw (GimpContainer *container,
				GimpContext   *context)
{
  GimpPattern *pattern;

  if (! context->pattern_name)
    context->pattern_name = g_strdup (context->gimp->config->default_pattern);

  if ((pattern = (GimpPattern *)
       gimp_container_get_child_by_name (container,
					 context->pattern_name)))
    {
      gimp_context_real_set_pattern (context, pattern);
      return;
    }

  if (gimp_container_num_children (container))
    gimp_context_real_set_pattern
      (context,
       GIMP_PATTERN (gimp_container_get_child_by_index (container, 0)));
  else
    gimp_context_real_set_pattern (context,
				   GIMP_PATTERN (gimp_pattern_get_standard ()));
}

/*  the active pattern disappeared  */
static void
gimp_context_pattern_removed (GimpContainer *container,
			      GimpPattern   *pattern,
			      GimpContext   *context)
{
  if (pattern == context->pattern)
    {
      context->pattern = NULL;

      g_signal_handlers_disconnect_by_func (G_OBJECT (pattern),
					    gimp_context_pattern_dirty,
					    context);
      g_object_unref (G_OBJECT (pattern));

      if (! gimp_container_frozen (container))
	gimp_context_pattern_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_pattern (GimpContext *context,
			       GimpPattern *pattern)
{
  if (! standard_pattern)
    standard_pattern = GIMP_PATTERN (gimp_pattern_get_standard ());

  if (context->pattern == pattern)
    return;

  if (context->pattern_name && pattern != standard_pattern)
    {
      g_free (context->pattern_name);
      context->pattern_name = NULL;
    }

  /*  make sure the active pattern is swapped before we get a new one...  */
  if (base_config->stingy_memory_use             &&
      context->pattern && context->pattern->mask &&
      G_OBJECT (context->pattern)->ref_count == 2)
    {
      temp_buf_swap (pattern->mask);
    }

  /*  disconnect from the old pattern's signals  */
  if (context->pattern)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (context->pattern),
					    gimp_context_pattern_dirty,
					    context);
      g_object_unref (G_OBJECT (context->pattern));
    }

  context->pattern = pattern;

  if (pattern)
    {
      g_object_ref (G_OBJECT (pattern));

      g_signal_connect_object (G_OBJECT (pattern), "name_changed",
			       G_CALLBACK (gimp_context_pattern_dirty),
			       G_OBJECT (context),
			       0);

      /*  Make sure the active pattern is unswapped... */
      if (base_config->stingy_memory_use   &&
	  pattern->mask                    &&
	  G_OBJECT (pattern)->ref_count < 2)
	{
	  temp_buf_unswap (pattern->mask);
	}

      if (pattern != standard_pattern)
	context->pattern_name = g_strdup (GIMP_OBJECT (pattern)->name);
    }

  gimp_context_pattern_changed (context);
}

static void
gimp_context_copy_pattern (GimpContext *src,
			   GimpContext *dest)
{
  gimp_context_real_set_pattern (dest, src->pattern);

  if ((!src->pattern || src->pattern == standard_pattern) && src->pattern_name)
    {
      g_free (dest->pattern_name);
      dest->pattern_name = g_strdup (src->pattern_name);
    }
}


/*****************************************************************************/
/*  gradient  ****************************************************************/

static GimpGradient *standard_gradient = NULL;

GimpGradient *
gimp_context_get_gradient (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->gradient;
}

void
gimp_context_set_gradient (GimpContext  *context,
			   GimpGradient *gradient)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_GRADIENT_MASK);

  gimp_context_real_set_gradient (context, gradient);
}

void
gimp_context_gradient_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[GRADIENT_CHANGED], 0,
		 context->gradient);
}

/*  the active gradient was modified  */
static void
gimp_context_gradient_dirty (GimpGradient *gradient,
			     GimpContext  *context)
{
  g_free (context->gradient_name);
  context->gradient_name = g_strdup (GIMP_OBJECT (gradient)->name);

  gimp_context_gradient_changed (context);
}

/*  the global gradient list is there again after refresh  */
static void
gimp_context_gradient_list_thaw (GimpContainer *container,
				 GimpContext   *context)
{
  GimpGradient *gradient;

  if (! context->gradient_name)
    context->gradient_name = g_strdup (context->gimp->config->default_gradient);

  if ((gradient = (GimpGradient *)
       gimp_container_get_child_by_name (container,
					 context->gradient_name)))
    {
      gimp_context_real_set_gradient (context, gradient);
      return;
    }

  if (gimp_container_num_children (container))
    gimp_context_real_set_gradient
      (context,
       GIMP_GRADIENT (gimp_container_get_child_by_index (container, 0)));
  else
    gimp_context_real_set_gradient (context,
				    GIMP_GRADIENT (gimp_gradient_get_standard ()));
}

/*  the active gradient disappeared  */
static void
gimp_context_gradient_removed (GimpContainer *container,
			       GimpGradient  *gradient,
			       GimpContext   *context)
{
  if (gradient == context->gradient)
    {
      context->gradient = NULL;

      g_signal_handlers_disconnect_by_func (G_OBJECT (gradient),
					    gimp_context_gradient_dirty,
					    context);
      g_object_unref (G_OBJECT (gradient));

      if (! gimp_container_frozen (container))
	gimp_context_gradient_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_gradient (GimpContext  *context,
				GimpGradient *gradient)
{
  if (! standard_gradient)
    standard_gradient = GIMP_GRADIENT (gimp_gradient_get_standard ());

  if (context->gradient == gradient)
    return;

  if (context->gradient_name && gradient != standard_gradient)
    {
      g_free (context->gradient_name);
      context->gradient_name = NULL;
    }

  /*  disconnect from the old gradient's signals  */
  if (context->gradient)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (context->gradient),
					    gimp_context_gradient_dirty,
					    context);
      g_object_unref (G_OBJECT (context->gradient));
    }

  context->gradient = gradient;

  if (gradient)
    {
      g_object_ref (G_OBJECT (gradient));

      g_signal_connect_object (G_OBJECT (gradient), "name_changed",
			       G_CALLBACK (gimp_context_gradient_dirty),
			       G_OBJECT (context),
			       0);

      if (gradient != standard_gradient)
	context->gradient_name = g_strdup (GIMP_OBJECT (gradient)->name);
    }

  gimp_context_gradient_changed (context);
}

static void
gimp_context_copy_gradient (GimpContext *src,
			    GimpContext *dest)
{
  gimp_context_real_set_gradient (dest, src->gradient);

  if ((!src->gradient || src->gradient == standard_gradient) &&
      src->gradient_name)
    {
      g_free (dest->gradient_name);
      dest->gradient_name = g_strdup (src->gradient_name);
    }
}


/*****************************************************************************/
/*  palette  *****************************************************************/

static GimpPalette *standard_palette = NULL;

GimpPalette *
gimp_context_get_palette (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->palette;
}

void
gimp_context_set_palette (GimpContext *context,
			  GimpPalette *palette)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_PALETTE_MASK);

  gimp_context_real_set_palette (context, palette);
}

void
gimp_context_palette_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[PALETTE_CHANGED], 0,
		 context->palette);
}

/*  the active palette was modified  */
static void
gimp_context_palette_dirty (GimpPalette *palette,
			    GimpContext *context)
{
  g_free (context->palette_name);
  context->palette_name = g_strdup (GIMP_OBJECT (palette)->name);

  gimp_context_palette_changed (context);
}

/*  the global gradient list is there again after refresh  */
static void
gimp_context_palette_list_thaw (GimpContainer *container,
				GimpContext   *context)
{
  GimpPalette *palette;

  if (! context->palette_name)
    context->palette_name = g_strdup (context->gimp->config->default_palette);

  if ((palette = (GimpPalette *)
       gimp_container_get_child_by_name (container,
					 context->palette_name)))
    {
      gimp_context_real_set_palette (context, palette);
      return;
    }

  if (gimp_container_num_children (container))
    gimp_context_real_set_palette
      (context,
       GIMP_PALETTE (gimp_container_get_child_by_index (container, 0)));
  else
    gimp_context_real_set_palette (context,
				   GIMP_PALETTE (gimp_palette_get_standard ()));
}

/*  the active palette disappeared  */
static void
gimp_context_palette_removed (GimpContainer *container,
			      GimpPalette   *palette,
			      GimpContext   *context)
{
  if (palette == context->palette)
    {
      context->palette = NULL;

      g_signal_handlers_disconnect_by_func (G_OBJECT (palette),
					    gimp_context_palette_dirty,
					    context);
      g_object_unref (G_OBJECT (palette));

      if (! gimp_container_frozen (container))
	gimp_context_palette_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_palette (GimpContext *context,
			       GimpPalette *palette)
{
  if (! standard_palette)
    standard_palette = GIMP_PALETTE (gimp_palette_get_standard ());

  if (context->palette == palette)
    return;

  if (context->palette_name && palette != standard_palette)
    {
      g_free (context->palette_name);
      context->palette_name = NULL;
    }

  /*  disconnect from the old palette's signals  */
  if (context->palette)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (context->palette),
					    gimp_context_palette_dirty,
					    context);
      g_object_unref (G_OBJECT (context->palette));
    }

  context->palette = palette;

  if (palette)
    {
      g_object_ref (G_OBJECT (palette));

      g_signal_connect_object (G_OBJECT (palette), "name_changed",
			       G_CALLBACK (gimp_context_palette_dirty),
			       G_OBJECT (context),
			       0);

      if (palette != standard_palette)
	context->palette_name = g_strdup (GIMP_OBJECT (palette)->name);
    }

  gimp_context_palette_changed (context);
}

static void
gimp_context_copy_palette (GimpContext *src,
			   GimpContext *dest)
{
  gimp_context_real_set_palette (dest, src->palette);

  if ((!src->palette || src->palette == standard_palette) && src->palette_name)
    {
      g_free (dest->palette_name);
      dest->palette_name = g_strdup (src->palette_name);
    }
}


/*****************************************************************************/
/*  buffer  ******************************************************************/

/*
static GimpBuffer *standard_buffer = NULL;
*/

GimpBuffer *
gimp_context_get_buffer (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->buffer;
}

void
gimp_context_set_buffer (GimpContext *context,
			 GimpBuffer *buffer)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_BUFFER_MASK);

  gimp_context_real_set_buffer (context, buffer);
}

void
gimp_context_buffer_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[BUFFER_CHANGED], 0,
		 context->buffer);
}

/*  the active buffer was modified  */
static void
gimp_context_buffer_dirty (GimpBuffer  *buffer,
			   GimpContext *context)
{
  /*
  g_free (context->buffer_name);
  context->buffer_name = g_strdup (GIMP_OBJECT (buffer)->name);
  */

  gimp_context_buffer_changed (context);
}

/*  the global gradient list is there again after refresh  */
static void
gimp_context_buffer_list_thaw (GimpContainer *container,
			       GimpContext   *context)
{
  /*
  GimpBuffer *buffer;

  if (! context->buffer_name)
    context->buffer_name = g_strdup (context->gimp->config->default_buffer);

  if ((buffer = (GimpBuffer *)
       gimp_container_get_child_by_name (container,
					 context->buffer_name)))
    {
      gimp_context_real_set_buffer (context, buffer);
      return;
    }
  */

  if (gimp_container_num_children (container))
    gimp_context_real_set_buffer
      (context,
       GIMP_BUFFER (gimp_container_get_child_by_index (container, 0)));
  else
    gimp_context_buffer_changed (context);

  /*
  else
    gimp_context_real_set_buffer (context,
				  GIMP_BUFFER (gimp_buffer_get_standard ()));
  */
}

/*  the active buffer disappeared  */
static void
gimp_context_buffer_removed (GimpContainer *container,
			     GimpBuffer    *buffer,
			     GimpContext   *context)
{
  if (buffer == context->buffer)
    {
      context->buffer = NULL;

      g_signal_handlers_disconnect_by_func (G_OBJECT (buffer),
					    gimp_context_buffer_dirty,
					    context);
      g_object_unref (G_OBJECT (buffer));

      if (! gimp_container_frozen (container))
	gimp_context_buffer_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_buffer (GimpContext *context,
			      GimpBuffer  *buffer)
{
  /*
  if (! standard_buffer)
    standard_buffer = GIMP_BUFFER (gimp_buffer_get_standard ());
  */

  if (context->buffer == buffer)
    return;

  /*
  if (context->buffer_name && buffer != standard_buffer)
    {
      g_free (context->buffer_name);
      context->buffer_name = NULL;
    }
  */

  /*  disconnect from the old buffer's signals  */
  if (context->buffer)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (context->buffer),
					    gimp_context_buffer_dirty,
					    context);
      g_object_unref (G_OBJECT (context->buffer));
    }

  context->buffer = buffer;

  if (buffer)
    {
      g_object_ref (G_OBJECT (buffer));

      g_signal_connect_object (G_OBJECT (buffer), "name_changed",
			       G_CALLBACK (gimp_context_buffer_dirty),
			       G_OBJECT (context),
			       0);

      /*
      if (buffer != standard_buffer)
	context->buffer_name = g_strdup (GIMP_OBJECT (buffer)->name);
      */
    }

  gimp_context_buffer_changed (context);
}

static void
gimp_context_copy_buffer (GimpContext *src,
			  GimpContext *dest)
{
  gimp_context_real_set_buffer (dest, src->buffer);

  /*
  if ((!src->buffer || src->buffer == standard_buffer) && src->buffer_name)
    {
      g_free (dest->buffer_name);
      dest->buffer_name = g_strdup (src->buffer_name);
    }
  */
}
