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

#include <string.h>

#include <glib-object.h>

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
#include "gimpimagefile.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimppalette.h"
#include "gimppattern.h"
#include "gimptoolinfo.h"

#include "libgimp/gimpintl.h" 


typedef void (* GimpContextCopyPropFunc) (GimpContext *src,
					  GimpContext *dest);


#define context_find_defined(context,prop_mask) \
        while (!(((context)->defined_props) & prop_mask) && (context)->parent) \
          (context) = (context)->parent


/*  local function prototypes  */

static void    gimp_context_class_init       (GimpContextClass *klass);
static void    gimp_context_init             (GimpContext      *context);

static void    gimp_context_dispose          (GObject          *object);
static void    gimp_context_finalize         (GObject          *object);
static void    gimp_context_set_property     (GObject          *object,
					      guint             property_id,
					      const GValue     *value,
					      GParamSpec       *pspec);
static void    gimp_context_get_property     (GObject          *object,
					      guint             property_id,
					      GValue           *value,
					      GParamSpec       *pspec);

static gsize   gimp_context_get_memsize      (GimpObject       *object);


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
					      gpointer          display);
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
					      GimpLayerModeEffects  
                                                               paint_mode);
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

/*  imagefile  */
static void gimp_context_imagefile_dirty     (GimpImagefile    *imagefile,
					      GimpContext      *context);
static void gimp_context_imagefile_removed   (GimpContainer    *container,
					      GimpImagefile    *imagefile,
					      GimpContext      *context);
static void gimp_context_imagefile_list_thaw (GimpContainer    *container,
					      GimpContext      *context);
static void gimp_context_real_set_imagefile  (GimpContext      *context,
					      GimpImagefile    *imagefile);
static void gimp_context_copy_imagefile      (GimpContext      *src,
					      GimpContext      *dest);


/*  properties & signals  */

enum
{
  PROP_0,
  PROP_GIMP,
  PROP_IMAGE,
  PROP_DISPLAY,
  PROP_TOOL,
  PROP_FOREGROUND,
  PROP_BACKGROUND,
  PROP_OPACITY,
  PROP_PAINT_MODE,
  PROP_BRUSH,
  PROP_PATTERN,
  PROP_GRADIENT,
  PROP_PALETTE,
  PROP_BUFFER,
  PROP_IMAGEFILE
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
  IMAGEFILE_CHANGED,
  LAST_SIGNAL
};

static gchar *gimp_context_prop_names[] =
{
  "image",
  "display",
  "tool",
  "foreground",
  "background",
  "opacity",
  "paint_mode",
  "brush",
  "pattern",
  "gradient",
  "palette",
  "buffer",
  "imagefile"
};

static GimpContextCopyPropFunc gimp_context_copy_prop_funcs[] =
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
  gimp_context_copy_buffer,
  gimp_context_copy_imagefile
};

static GType gimp_context_prop_types[] =
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
  "buffer_changed",
  "imagefile_changed"
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
  G_CALLBACK (gimp_context_real_set_buffer),
  G_CALLBACK (gimp_context_real_set_imagefile)
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
  GObjectClass    *object_class;
  GimpObjectClass *gimp_object_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_context_signals[IMAGE_CHANGED] =
    g_signal_new (gimp_context_signal_names[IMAGE_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, image_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_IMAGE);

  gimp_context_signals[DISPLAY_CHANGED] =
    g_signal_new (gimp_context_signal_names[DISPLAY_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, display_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_context_signals[TOOL_CHANGED] =
    g_signal_new (gimp_context_signal_names[TOOL_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, tool_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_TOOL_INFO);

  gimp_context_signals[FOREGROUND_CHANGED] =
    g_signal_new (gimp_context_signal_names[FOREGROUND_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, foreground_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_context_signals[BACKGROUND_CHANGED] =
    g_signal_new (gimp_context_signal_names[BACKGROUND_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, background_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  gimp_context_signals[OPACITY_CHANGED] =
    g_signal_new (gimp_context_signal_names[OPACITY_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, opacity_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__DOUBLE,
		  G_TYPE_NONE, 1,
		  G_TYPE_DOUBLE);

  gimp_context_signals[PAINT_MODE_CHANGED] =
    g_signal_new (gimp_context_signal_names[PAINT_MODE_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, paint_mode_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__INT,
		  G_TYPE_NONE, 1,
		  G_TYPE_INT);

  gimp_context_signals[BRUSH_CHANGED] =
    g_signal_new (gimp_context_signal_names[BRUSH_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, brush_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_BRUSH);

  gimp_context_signals[PATTERN_CHANGED] =
    g_signal_new (gimp_context_signal_names[PATTERN_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, pattern_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_PATTERN);

  gimp_context_signals[GRADIENT_CHANGED] =
    g_signal_new (gimp_context_signal_names[GRADIENT_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, gradient_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_GRADIENT);

  gimp_context_signals[PALETTE_CHANGED] =
    g_signal_new (gimp_context_signal_names[PALETTE_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, palette_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_PALETTE);

  gimp_context_signals[BUFFER_CHANGED] =
    g_signal_new (gimp_context_signal_names[BUFFER_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, buffer_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_BUFFER);

  gimp_context_signals[IMAGEFILE_CHANGED] =
    g_signal_new (gimp_context_signal_names[IMAGEFILE_CHANGED],
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpContextClass, imagefile_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GIMP_TYPE_IMAGEFILE);

  object_class->set_property     = gimp_context_set_property;
  object_class->get_property     = gimp_context_get_property;
  object_class->dispose          = gimp_context_dispose;
  object_class->finalize         = gimp_context_finalize;

  gimp_object_class->get_memsize = gimp_context_get_memsize;

  klass->image_changed           = NULL;
  klass->display_changed         = NULL;
  klass->tool_changed            = NULL;
  klass->foreground_changed      = NULL;
  klass->background_changed      = NULL;
  klass->opacity_changed         = NULL;
  klass->paint_mode_changed      = NULL;
  klass->brush_changed           = NULL;
  klass->pattern_changed         = NULL;
  klass->gradient_changed        = NULL;
  klass->palette_changed         = NULL;
  klass->buffer_changed          = NULL;
  klass->imagefile_changed       = NULL;

  gimp_context_prop_types[GIMP_CONTEXT_PROP_IMAGE]     = GIMP_TYPE_IMAGE;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_TOOL]      = GIMP_TYPE_TOOL_INFO;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_BRUSH]     = GIMP_TYPE_BRUSH;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_PATTERN]   = GIMP_TYPE_PATTERN;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_GRADIENT]  = GIMP_TYPE_GRADIENT;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_PALETTE]   = GIMP_TYPE_PALETTE;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_BUFFER]    = GIMP_TYPE_BUFFER;
  gimp_context_prop_types[GIMP_CONTEXT_PROP_IMAGEFILE] = GIMP_TYPE_IMAGEFILE;

  g_object_class_install_property (object_class,
                                   PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
				   PROP_IMAGE,
				   g_param_spec_object (gimp_context_prop_names[IMAGE_CHANGED],
							NULL, NULL,
							GIMP_TYPE_IMAGE,
							G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_DISPLAY,
				   g_param_spec_pointer (gimp_context_prop_names[DISPLAY_CHANGED],
							 NULL, NULL,
							 G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_TOOL,
				   g_param_spec_object (gimp_context_prop_names[TOOL_CHANGED],
							NULL, NULL,
							GIMP_TYPE_TOOL_INFO,
							G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_FOREGROUND,
				   g_param_spec_pointer (gimp_context_prop_names[FOREGROUND_CHANGED],
							 NULL, NULL,
							 G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_BACKGROUND,
				   g_param_spec_pointer (gimp_context_prop_names[BACKGROUND_CHANGED],
							 NULL, NULL,
							 G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_OPACITY,
				   g_param_spec_double (gimp_context_prop_names[OPACITY_CHANGED],
							NULL, NULL,
							0.0,
							1.0,
							1.0,
							G_PARAM_READWRITE));

  /* FIXME: convert to enum property */
  g_object_class_install_property (object_class,
				   PROP_PAINT_MODE,
				   g_param_spec_int (gimp_context_prop_names[PAINT_MODE_CHANGED],
						     NULL, NULL,
						     GIMP_NORMAL_MODE,
						     GIMP_ANTI_ERASE_MODE,
						     GIMP_NORMAL_MODE,
						     G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_BRUSH,
				   g_param_spec_object (gimp_context_prop_names[BRUSH_CHANGED],
							NULL, NULL,
							GIMP_TYPE_BRUSH,
							G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_PATTERN,
				   g_param_spec_object (gimp_context_prop_names[PATTERN_CHANGED],
							NULL, NULL,
							GIMP_TYPE_PATTERN,
							G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_GRADIENT,
				   g_param_spec_object (gimp_context_prop_names[GRADIENT_CHANGED],
							NULL, NULL,
							GIMP_TYPE_GRADIENT,
							G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_PALETTE,
				   g_param_spec_object (gimp_context_prop_names[PALETTE_CHANGED],
							NULL, NULL,
							GIMP_TYPE_PALETTE,
							G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_BUFFER,
				   g_param_spec_object (gimp_context_prop_names[BUFFER_CHANGED],
							NULL, NULL,
							GIMP_TYPE_BUFFER,
							G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_IMAGEFILE,
				   g_param_spec_object (gimp_context_prop_names[IMAGEFILE_CHANGED],
							NULL, NULL,
							GIMP_TYPE_IMAGEFILE,
							G_PARAM_READWRITE));
}

static void
gimp_context_init (GimpContext *context)
{
  context->gimp          = NULL;

  context->parent        = NULL;

  context->defined_props = GIMP_CONTEXT_ALL_PROPS_MASK;

  context->image         = NULL;
  context->display       = NULL;

  context->tool_info     = NULL;
  context->tool_name     = NULL;

  gimp_rgba_set (&context->foreground, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&context->background, 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);

  context->opacity       = GIMP_OPACITY_OPAQUE;
  context->paint_mode    = GIMP_NORMAL_MODE;

  context->brush         = NULL;
  context->brush_name    = NULL;

  context->pattern       = NULL;
  context->pattern_name  = NULL;

  context->gradient      = NULL;
  context->gradient_name = NULL;

  context->palette       = NULL;
  context->palette_name  = NULL;

  context->buffer        = NULL;
  context->imagefile     = NULL;
}

static void
gimp_context_dispose (GObject *object)
{
  GimpContext *context;

  context = GIMP_CONTEXT (object);

  if (context->gimp)
    {
      context->gimp->context_list = g_list_remove (context->gimp->context_list,
                                                   context);
      context->gimp = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_context_finalize (GObject *object)
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

  if (context->imagefile)
    {
      g_object_unref (G_OBJECT (context->imagefile));
      context->imagefile = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_context_set_property (GObject      *object,
			   guint         property_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  GimpContext *context;

  context = GIMP_CONTEXT (object);

  switch (property_id)
    {
    case PROP_GIMP:
      context->gimp = g_value_get_object (value);
      context->gimp->context_list = g_list_prepend (context->gimp->context_list,
                                                    context);
      break;
    case PROP_IMAGE:
      gimp_context_set_image (context, g_value_get_object (value));
      break;
    case PROP_DISPLAY:
      gimp_context_set_display (context, g_value_get_pointer (value));
      break;
    case PROP_TOOL:
      gimp_context_set_tool (context, g_value_get_object (value));
      break;
    case PROP_FOREGROUND:
      gimp_context_set_foreground (context, g_value_get_pointer (value));
      break;
    case PROP_BACKGROUND:
      gimp_context_set_background (context, g_value_get_pointer (value));
      break;
    case PROP_OPACITY:
      gimp_context_set_opacity (context, g_value_get_double (value));
      break;
    case PROP_PAINT_MODE:
      gimp_context_set_paint_mode (context, g_value_get_int (value));
      break;
    case PROP_BRUSH:
      gimp_context_set_brush (context, g_value_get_object (value));
      break;
    case PROP_PATTERN:
      gimp_context_set_pattern (context, g_value_get_object (value));
      break;
    case PROP_GRADIENT:
      gimp_context_set_gradient (context, g_value_get_object (value));
      break;
    case PROP_PALETTE:
      gimp_context_set_palette (context, g_value_get_object (value));
      break;
    case PROP_BUFFER:
      gimp_context_set_buffer (context, g_value_get_object (value));
      break;
    case PROP_IMAGEFILE:
      gimp_context_set_imagefile (context, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_context_get_property (GObject    *object,
			   guint       property_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  GimpContext *context;

  context = GIMP_CONTEXT (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, context->gimp);
      break;
    case PROP_IMAGE:
      g_value_set_object (value, gimp_context_get_image (context));
      break;
    case PROP_DISPLAY:
      g_value_set_pointer (value, gimp_context_get_display (context));
      break;
    case PROP_TOOL:
      g_value_set_object (value, gimp_context_get_tool (context));
      break;
    case PROP_FOREGROUND:
      gimp_context_get_foreground (context, g_value_get_pointer (value));
      break;
    case PROP_BACKGROUND:
      gimp_context_get_background (context, g_value_get_pointer (value));
      break;
    case PROP_OPACITY:
      g_value_set_double (value, gimp_context_get_opacity (context));
      break;
    case PROP_PAINT_MODE:
      g_value_set_int (value, gimp_context_get_paint_mode (context));
      break;
    case PROP_BRUSH:
      g_value_set_object (value, gimp_context_get_brush (context));
      break;
    case PROP_PATTERN:
      g_value_set_object (value, gimp_context_get_pattern (context));
      break;
    case PROP_GRADIENT:
      g_value_set_object (value, gimp_context_get_gradient (context));
      break;
    case PROP_PALETTE:
      g_value_set_object (value, gimp_context_get_palette (context));
      break;
    case PROP_BUFFER:
      g_value_set_object (value, gimp_context_get_buffer (context));
      break;
    case PROP_IMAGEFILE:
      g_value_set_object (value, gimp_context_get_imagefile (context));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gsize
gimp_context_get_memsize (GimpObject *object)
{
  GimpContext *context;
  gsize        memsize = 0;

  context = GIMP_CONTEXT (object);

  if (context->tool_name)
    memsize += strlen (context->tool_name) + 1;

  if (context->brush_name)
    memsize += strlen (context->brush_name) + 1;

  if (context->pattern_name)
    memsize += strlen (context->pattern_name) + 1;

  if (context->palette_name)
    memsize += strlen (context->palette_name) + 1;

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
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

  context = g_object_new (GIMP_TYPE_CONTEXT,
                          "name", name,
                          "gimp", gimp,
                          NULL);

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

  g_signal_connect_object (G_OBJECT (gimp->documents), "remove",
			   G_CALLBACK (gimp_context_imagefile_removed),
			   G_OBJECT (context),
			   0);
  g_signal_connect_object (G_OBJECT (gimp->documents), "thaw",
			   G_CALLBACK (gimp_context_imagefile_list_thaw),
			   G_OBJECT (context),
			   0);

  if (template)
    {
      context->defined_props = template->defined_props;

      gimp_context_copy_properties (template, context,
				    GIMP_CONTEXT_ALL_PROPS_MASK);
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
    name = _("Unnamed");

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
  GimpContextPropType prop;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (! parent || GIMP_IS_CONTEXT (parent));
  g_return_if_fail (context != parent);

  if (context == parent || context->parent == parent)
    return;

  for (prop = 0; prop < GIMP_CONTEXT_NUM_PROPS; prop++)
    if (! ((1 << prop) & context->defined_props))
      {
	gimp_context_copy_property (parent, context, prop);
	g_signal_connect_object (G_OBJECT (parent),
				 gimp_context_signal_names[prop],
				 gimp_context_signal_handlers[prop],
				 G_OBJECT (context),
				 G_CONNECT_SWAPPED);
      }

  context->parent = parent;
}

void
gimp_context_unset_parent (GimpContext *context)
{
  GimpContextPropType prop;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (GIMP_IS_CONTEXT (context->parent));

  for (prop = 0; prop < GIMP_CONTEXT_NUM_PROPS; prop++)
    if (! ((1 << prop) & context->defined_props))
      {
	g_signal_handlers_disconnect_by_func (G_OBJECT (context->parent),
					      gimp_context_signal_handlers[prop],
					      context);
      }

  context->parent = NULL;
}

/*  define / undefinine context properties  */

void
gimp_context_define_property (GimpContext         *context,
			      GimpContextPropType  prop,
			      gboolean             defined)
{
  GimpContextPropMask mask;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail ((prop >= 0) && (prop < GIMP_CONTEXT_NUM_PROPS));

  mask = (1 << prop);

  if (defined)
    {
      if (! (context->defined_props & mask))
	{
	  context->defined_props |= mask;
	  if (context->parent)
	    g_signal_handlers_disconnect_by_func (G_OBJECT (context->parent),
						  gimp_context_signal_handlers[prop],
						  context);
	}
    }
  else
    {
      if (context->defined_props & mask)
	{
	  context->defined_props &= ~mask;
	  if (context->parent)
	    {
	      gimp_context_copy_property (context->parent, context, prop);
	      g_signal_connect_object (G_OBJECT (context->parent),
				       gimp_context_signal_names[prop],
				       gimp_context_signal_handlers[prop],
				       G_OBJECT (context),
				       G_CONNECT_SWAPPED);
	    }
	}
    }
}

gboolean
gimp_context_property_defined (GimpContext         *context,
			       GimpContextPropType  prop)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  return (context->defined_props & (1 << prop)) ? TRUE : FALSE;
}

void
gimp_context_define_properties (GimpContext         *context,
				GimpContextPropMask  props_mask,
				gboolean             defined)
{
  GimpContextPropType prop;

  g_return_if_fail (GIMP_IS_CONTEXT (context));

  for (prop = 0; prop < GIMP_CONTEXT_NUM_PROPS; prop++)
    if ((1 << prop) & props_mask)
      gimp_context_define_property (context, prop, defined);
}

/*  copying context properties  */

void
gimp_context_copy_property (GimpContext         *src,
			    GimpContext         *dest,
			    GimpContextPropType  prop)
{
  g_return_if_fail (GIMP_IS_CONTEXT (src));
  g_return_if_fail (GIMP_IS_CONTEXT (dest));
  g_return_if_fail ((prop >= 0) && (prop < GIMP_CONTEXT_NUM_PROPS));

  gimp_context_copy_prop_funcs[prop] (src, dest);
}

void
gimp_context_copy_properties (GimpContext         *src,
			      GimpContext         *dest,
			      GimpContextPropMask  props_mask)
{
  GimpContextPropType prop;

  g_return_if_fail (GIMP_IS_CONTEXT (src));
  g_return_if_fail (GIMP_IS_CONTEXT (dest));

  for (prop = 0; prop < GIMP_CONTEXT_NUM_PROPS; prop++)
    if ((1 << prop) & props_mask)
      {
	gimp_context_copy_property (src, dest, prop);
      }
}

/*  attribute access functions  */

/*****************************************************************************/
/*  manipulate by GType  *****************************************************/

GimpContextPropType
gimp_context_type_to_property (GType type)
{
  gint i;

  for (i = 0; i < GIMP_CONTEXT_NUM_PROPS; i++)
    {
      if (g_type_is_a (type, gimp_context_prop_types[i]))
	return i;
    }

  return -1;
}

const gchar *
gimp_context_type_to_signal_name (GType type)
{
  gint i;

  for (i = 0; i < GIMP_CONTEXT_NUM_PROPS; i++)
    {
      if (g_type_is_a (type, gimp_context_prop_types[i]))
	return gimp_context_signal_names[i];
    }

  return NULL;
}

GimpObject *
gimp_context_get_by_type (GimpContext *context,
			  GType        type)
{
  GimpContextPropType  prop;
  GimpObject          *object = NULL;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail ((prop = gimp_context_type_to_property (type)) != -1,
			NULL);

  g_object_get (G_OBJECT (context),
		gimp_context_prop_names[prop], &object,
		NULL);

  return object;
}

void
gimp_context_set_by_type (GimpContext *context,
			  GType        type,
			  GimpObject  *object)
{
  GimpContextPropType prop;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail ((prop = gimp_context_type_to_property (type)) != -1);

  g_object_set (G_OBJECT (context),
		gimp_context_prop_names[prop], object,
		NULL);
}

void
gimp_context_changed_by_type (GimpContext *context,
			      GType        type)
{
  GimpContextPropType  prop;
  GimpObject          *object;

  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail ((prop = gimp_context_type_to_property (type)) != -1);

  object = gimp_context_get_by_type (context, type);

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[prop], 0,
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

gpointer
gimp_context_get_display (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->display;
}

void
gimp_context_set_display (GimpContext *context,
			  gpointer     display)
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
gimp_context_display_destroy (GObject     *disp_shell,
			      GimpContext *context)
{
  context->display = NULL;

  gimp_context_real_set_image (context, NULL);

  gimp_context_display_changed (context);
}

static void
gimp_context_real_set_display (GimpContext *context,
			       gpointer     display)
{
#ifdef __GNUC__
#warning FIXME: EEKWrapper
#endif
  typedef struct
  {
    GimpObject  foo;
    gint        bar;
    GimpImage  *gimage;
    gint        baz;
    GObject    *shell;
  } EEKWrapper;

  EEKWrapper *eek_wrapper;

  if (context->display == display)
    return;

  if (context->display)
    {
      eek_wrapper = (EEKWrapper *) context->display;

      if (G_IS_OBJECT (eek_wrapper->shell))
	g_signal_handlers_disconnect_by_func (eek_wrapper->shell,
					      gimp_context_display_destroy,
					      context);
    }

  context->display = display;

  if (context->display)
    {
      eek_wrapper = (EEKWrapper *) context->display;

      g_signal_connect_object (G_OBJECT (eek_wrapper->shell), "destroy",
			       G_CALLBACK (gimp_context_display_destroy),
			       G_OBJECT (context),
			       0);

      gimp_context_real_set_image (context, eek_wrapper->gimage);
    }

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
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (! tool_info || GIMP_IS_TOOL_INFO (tool_info));
  context_find_defined (context, GIMP_CONTEXT_TOOL_MASK);

  gimp_context_real_set_tool (context, tool_info);
}

void
gimp_context_tool_changed (GimpContext *context)
{
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
    context->tool_name = g_strdup ("gimp-rect-select-tool");

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
    gimp_context_real_set_tool (context, gimp_tool_info_get_standard (context->gimp));
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
    standard_tool_info = gimp_tool_info_get_standard (context->gimp);

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

  if ((! src->tool_info || src->tool_info == standard_tool_info) &&
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
  g_return_if_fail (color != NULL);
  context_find_defined (context, GIMP_CONTEXT_FOREGROUND_MASK);

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
  g_return_if_fail (color != NULL);
  context_find_defined (context, GIMP_CONTEXT_BACKGROUND_MASK);

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

  gimp_rgba_set (&fg, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&bg, 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);

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
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), GIMP_OPACITY_OPAQUE);

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

GimpLayerModeEffects
gimp_context_get_paint_mode (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), GIMP_NORMAL_MODE);

  return context->paint_mode;
}

void
gimp_context_set_paint_mode (GimpContext          *context,
			     GimpLayerModeEffects  paint_mode)
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
gimp_context_real_set_paint_mode (GimpContext          *context,
				  GimpLayerModeEffects  paint_mode)
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
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (! brush || GIMP_IS_BRUSH (brush));
  context_find_defined (context, GIMP_CONTEXT_BRUSH_MASK);

  gimp_context_real_set_brush (context, brush);
}

void
gimp_context_brush_changed (GimpContext *context)
{
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

  /*  disconnect from the old brush's signals  */
  if (context->brush)
    {
      /*  make sure the active brush is swapped before we get a new one...  */
      if (base_config->stingy_memory_use &&
	  context->brush->mask           &&
	  G_OBJECT (context->brush)->ref_count == 2)
	{
	  temp_buf_swap (context->brush->mask);
	}

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

  if ((! src->brush || src->brush == standard_brush) && src->brush_name)
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


/*****************************************************************************/
/*  imagefile  ***************************************************************/

/*
static GimpImagefile *standard_imagefile = NULL;
*/

GimpImagefile *
gimp_context_get_imagefile (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return context->imagefile;
}

void
gimp_context_set_imagefile (GimpContext   *context,
                            GimpImagefile *imagefile)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  context_find_defined (context, GIMP_CONTEXT_IMAGEFILE_MASK);

  gimp_context_real_set_imagefile (context, imagefile);
}

void
gimp_context_imagefile_changed (GimpContext *context)
{
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  g_signal_emit (G_OBJECT (context),
		 gimp_context_signals[IMAGEFILE_CHANGED], 0,
		 context->imagefile);
}

/*  the active imagefile was modified  */
static void
gimp_context_imagefile_dirty (GimpImagefile *imagefile,
                              GimpContext   *context)
{
  /*
  g_free (context->imagefile_name);
  context->imagefile_name = g_strdup (GIMP_OBJECT (imagefile)->name);
  */

  gimp_context_imagefile_changed (context);
}

/*  the global gradient list is there again after refresh  */
static void
gimp_context_imagefile_list_thaw (GimpContainer *container,
                                  GimpContext   *context)
{
  /*
  GimpBuffer *imagefile;

  if (! context->imagefile_name)
    context->imagefile_name = g_strdup (context->gimp->config->default_imagefile);

  if ((imagefile = (GimpImagefile *)
       gimp_container_get_child_by_name (container,
					 context->imagefile_name)))
    {
      gimp_context_real_set_imagefile (context, imagefile);
      return;
    }
  */

  if (gimp_container_num_children (container))
    gimp_context_real_set_imagefile
      (context,
       GIMP_IMAGEFILE (gimp_container_get_child_by_index (container, 0)));
  else
    gimp_context_imagefile_changed (context);

  /*
  else
    gimp_context_real_set_imagefile (context,
				     GIMP_IMAGEFILE (gimp_imagefile_get_standard ()));
  */
}

/*  the active imagefile disappeared  */
static void
gimp_context_imagefile_removed (GimpContainer *container,
                                GimpImagefile *imagefile,
                                GimpContext   *context)
{
  if (imagefile == context->imagefile)
    {
      context->imagefile = NULL;

      g_signal_handlers_disconnect_by_func (G_OBJECT (imagefile),
					    gimp_context_imagefile_dirty,
					    context);
      g_object_unref (G_OBJECT (imagefile));

      if (! gimp_container_frozen (container))
	gimp_context_imagefile_list_thaw (container, context);
    }
}

static void
gimp_context_real_set_imagefile (GimpContext   *context,
                                 GimpImagefile *imagefile)
{
  /*
  if (! standard_imagefile)
    standard_imagefile = GIMP_IMAGEFILE (gimp_imagefile_get_standard ());
  */

  if (context->imagefile == imagefile)
    return;

  /*
  if (context->imagefile_name && imagefile != standard_imagefile)
    {
      g_free (context->imagefile_name);
      context->imagefile_name = NULL;
    }
  */

  /*  disconnect from the old imagefile's signals  */
  if (context->imagefile)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (context->imagefile),
					    gimp_context_imagefile_dirty,
					    context);
      g_object_unref (G_OBJECT (context->imagefile));
    }

  context->imagefile = imagefile;

  if (imagefile)
    {
      g_object_ref (G_OBJECT (imagefile));

      g_signal_connect_object (G_OBJECT (imagefile), "name_changed",
			       G_CALLBACK (gimp_context_imagefile_dirty),
			       G_OBJECT (context),
			       0);

      /*
      if (imagefile != standard_imagefile)
	context->imagefile_name = g_strdup (GIMP_OBJECT (imagefile)->name);
      */
    }

  gimp_context_imagefile_changed (context);
}

static void
gimp_context_copy_imagefile (GimpContext *src,
                             GimpContext *dest)
{
  gimp_context_real_set_imagefile (dest, src->imagefile);

  /*
  if ((!src->imagefile || src->imagefile == standard_imagefile) && src->imagefile_name)
    {
      g_free (dest->imagefile_name);
      dest->imagefile_name = g_strdup (src->imagefile_name);
    }
  */
}
