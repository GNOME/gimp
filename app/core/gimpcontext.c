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
#include <gtk/gtk.h>

#include "gimpcontext.h"
#include "gimpsignal.h"

#define context_return_if_fail(context) \
        g_return_if_fail ((context) != NULL); \
        g_return_if_fail (GIMP_IS_CONTEXT (context))

#define context_return_val_if_fail(context,val) \
        g_return_val_if_fail ((context) != NULL, (val)); \
        g_return_val_if_fail (GIMP_IS_CONTEXT (context), (val))

#define context_check_current(context) \
        ((context) = (context) ? (context) : current_context)

#define context_find_defined(context,field_defined) \
        while (!((context)->field_defined) && (context)->parent) \
          (context) = (context)->parent

enum {
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
  ARG_GRADIENT
};

enum {
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
  LAST_SIGNAL
};

static guint gimp_context_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass * parent_class = NULL;

/*  the currently active context  */
static GimpContext * current_context = NULL;

/*  the context user by the interface  */
static GimpContext * user_context = NULL;

/*  the default context which is initialized from gimprc  */
static GimpContext * default_context = NULL;

/*  the hardcoded standard context  */
static GimpContext * standard_context = NULL;


/*  private functions  *******************************************************/

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
      gimp_context_set_tool (context, GTK_VALUE_INT (*arg));
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
      GTK_VALUE_INT (*arg) = gimp_context_get_tool (context);
      break;
    case ARG_FOREGROUND:
      {
	guchar *dest = GTK_VALUE_POINTER (*arg);
	guchar  src[3];
	gimp_context_get_foreground (context, src);
	dest[0] = src[0];
	dest[1] = src[1];
	dest[2] = src[2];
      }
      break;
    case ARG_BACKGROUND:
      {
	guchar *dest = GTK_VALUE_POINTER (*arg);
	guchar  src[3];
	gimp_context_get_background (context, src);
	dest[0] = src[0];
	dest[1] = src[1];
	dest[2] = src[2];
      }
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
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

static void
gimp_context_destroy (GtkObject *object)
{
  GimpContext *context;

  context_return_if_fail (object);

  context = GIMP_CONTEXT (object);

  if (context->name)
    g_free (context->name);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gimp_context_class_init (GimpContextClass *klass)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS (klass);

  gtk_object_add_arg_type ("GimpContext::image",
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_IMAGE);
  gtk_object_add_arg_type ("GimpContext::display",
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_DISPLAY);
  gtk_object_add_arg_type ("GimpContext::tool",
			   GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_TOOL);
  gtk_object_add_arg_type ("GimpContext::foreground",
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_FOREGROUND);
  gtk_object_add_arg_type ("GimpContext::background",
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_BACKGROUND);
  gtk_object_add_arg_type ("GimpContext::opacity",
			   GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_OPACITY);
  gtk_object_add_arg_type ("GimpContext::paint_mode",
			   GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_PAINT_MODE);
  gtk_object_add_arg_type ("GimpContext::brush",
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_BRUSH);
  gtk_object_add_arg_type ("GimpContext::pattern",
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_PATTERN);
  gtk_object_add_arg_type ("GimpContext::gradient",
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_GRADIENT);

  parent_class = gtk_type_class (gimp_object_get_type ());

  gimp_context_signals[IMAGE_CHANGED] =
    gimp_signal_new ("image_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					image_changed),
		     gimp_sigtype_pointer);

  gimp_context_signals[DISPLAY_CHANGED] =
    gimp_signal_new ("display_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					display_changed),
		     gimp_sigtype_pointer);

  gimp_context_signals[TOOL_CHANGED] =
    gimp_signal_new ("tool_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					tool_changed),
		     gimp_sigtype_int);

  gimp_context_signals[FOREGROUND_CHANGED] =
    gimp_signal_new ("foreground_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					foreground_changed),
		     gimp_sigtype_pointer);

  gimp_context_signals[BACKGROUND_CHANGED] =
    gimp_signal_new ("background_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					background_changed),
		     gimp_sigtype_pointer);

  gimp_context_signals[OPACITY_CHANGED] =
    gimp_signal_new ("opacity_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					opacity_changed),
		     gimp_sigtype_double);

  gimp_context_signals[PAINT_MODE_CHANGED] =
    gimp_signal_new ("paint_mode_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					paint_mode_changed),
		     gimp_sigtype_int);

  gimp_context_signals[BRUSH_CHANGED] =
    gimp_signal_new ("brush_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					brush_changed),
		     gimp_sigtype_pointer);

  gimp_context_signals[PATTERN_CHANGED] =
    gimp_signal_new ("pattern_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					pattern_changed),
		     gimp_sigtype_pointer);

  gimp_context_signals[GRADIENT_CHANGED] =
    gimp_signal_new ("gradient_changed",
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					gradient_changed),
		     gimp_sigtype_pointer);

  gtk_object_class_add_signals (object_class, gimp_context_signals,
				LAST_SIGNAL);

  object_class->set_arg = gimp_context_set_arg;
  object_class->get_arg = gimp_context_get_arg;
  object_class->destroy = gimp_context_destroy;

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
}

static void
gimp_context_init (GimpContext *context)
{
  context->name = NULL;
  context->parent = NULL;

  /*  Values to be taken from the parent context by default  */

  context->image_defined = FALSE;
  context->image = NULL;

  context->display_defined = FALSE;
  context->display = NULL;

  context->tool_defined = FALSE;
  context->tool = RECT_SELECT;

  /*  Values defined by default  */

  context->foreground_defined = TRUE;
  context->foreground[0] = 255;
  context->foreground[1] = 255;
  context->foreground[2] = 255;

  context->background_defined = TRUE;
  context->background[0] = 0;
  context->background[1] = 0;
  context->background[2] = 0;

  context->opacity_defined = TRUE;
  context->opacity = 1.0;

  context->paint_mode_defined = TRUE;
  context->paint_mode = 0;

  /*  Values to be taken from the parent context by default  */

  context->brush_defined = FALSE;
  context->brush = NULL;

  context->pattern_defined = FALSE;
  context->pattern = NULL;

  context->gradient_defined = FALSE;
  context->gradient = NULL;
}

/*  public functions  ********************************************************/

GtkType
gimp_context_get_type (void)
{
  static GtkType context_type = 0;

  if(! context_type)
    {
      GtkTypeInfo context_info =
      {
	"GimpContext",
	sizeof (GimpContext),
	sizeof (GimpContextClass),
	(GtkClassInitFunc) gimp_context_class_init,
	(GtkObjectInitFunc) gimp_context_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL
      };

      context_type = gtk_type_unique (gimp_object_get_type (), &context_info);
    }

  return context_type;
}

GimpContext *
gimp_context_new (gchar       *name,
		  GimpContext *template,
		  GimpContext *parent)
{
  GimpContext *context;

  g_return_val_if_fail (!template || GIMP_IS_CONTEXT (template), NULL);
  g_return_val_if_fail (!parent || GIMP_IS_CONTEXT (parent), NULL);

  context = gtk_type_new (gimp_context_get_type ());

  /*  FIXME: need unique (translated??) names here
   */
  context->name = g_strdup (name ? name : "Unnamed");
  context->parent = parent;

  if (template)
    {
      guchar col[3];

      context->image         = gimp_context_get_image (template);
      context->display       = gimp_context_get_display (template);
      context->tool          = gimp_context_get_tool (template);
      gimp_context_get_foreground (template, col);
      context->foreground[0] = col[0];
      context->foreground[1] = col[1];
      context->foreground[2] = col[2];
      gimp_context_get_background (template, col);
      context->background[0] = col[0];
      context->background[1] = col[1];
      context->background[2] = col[2];
      context->opacity       = gimp_context_get_opacity (template);
      context->paint_mode    = gimp_context_get_paint_mode (template);
      context->brush         = gimp_context_get_brush (template);
      context->pattern       = gimp_context_get_pattern (template);
      context->gradient      = gimp_context_get_gradient (template);

      context->image_defined      = template->image_defined;
      context->display_defined    = template->display_defined;
      context->tool_defined       = template->tool_defined;
      context->foreground_defined = template->foreground_defined;
      context->background_defined = template->background_defined;
      context->opacity_defined    = template->opacity_defined;
      context->paint_mode_defined = template->paint_mode_defined;
      context->brush_defined      = template->brush_defined;
      context->pattern_defined    = template->pattern_defined;
      context->gradient_defined   = template->gradient_defined;
    }

  return context;
}

/*  getting/setting the special contexts  ************************************/

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

GimpContext *
gimp_context_get_user (void)
{
  return user_context;
}

void
gimp_context_set_user (GimpContext *context)
{
  user_context = context;
}

GimpContext *
gimp_context_get_default (void)
{
  return default_context;
}

void
gimp_context_set_default (GimpContext *context)
{
  default_context = context;
}

GimpContext *
gimp_context_get_standard (void)
{
  if (! standard_context)
    {
      standard_context = gimp_context_new ("Standard", NULL, NULL);

      gtk_quit_add_destroy (TRUE, GTK_OBJECT (standard_context));
    }

  return standard_context;
}

/*  functions manipulating a single context  *********************************/

gchar *
gimp_context_get_name (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);

  return context->name;
}

GimpContext *
gimp_context_get_parent (GimpContext *context)
{
  context_return_val_if_fail (context, NULL);

  return context->parent;
}

void
gimp_context_set_parent (GimpContext *context,
			 GimpContext *parent)
{
  context_return_if_fail (context);
  g_return_if_fail (!parent || GIMP_IS_CONTEXT (parent));

  context->parent = parent;
}


/*  attribute access functions  */

/* FIXME: - this is UGLY code duplication
 *        - gimp_context_*_defined and _define_* sounds very ugly, too
 * TODO:  - implement a generic way or alternatively
 *        - write some macros which will fold one of the following
 *          functions into a single macro call
 */

/*  image  *******************************************************************/

GimpImage *
gimp_context_get_image (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);
  context_find_defined (context, image_defined);

  return context->image;
}

void
gimp_context_set_image (GimpContext *context,
			GimpImage   *image)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, image_defined);

  if (context->image == image) return;

  context->image = image;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[IMAGE_CHANGED],
		   image);
}

gboolean
gimp_context_image_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->image_defined;
}

void
gimp_context_define_image (GimpContext *context,
			   gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    context->image = gimp_context_get_image (context);

  context->image_defined = defined;
}

/*  display  *****************************************************************/

GDisplay *
gimp_context_get_display (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);
  context_find_defined (context, display_defined);

  return context->display;
}

void
gimp_context_set_display (GimpContext *context,
			  GDisplay    *display)
{
  GimpContext *orig = context;

  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, display_defined);

  if (context->display == display) return;

  context->display = display;

  /*  set the image _before_ emitting the display_changed signal  */
  if (display)
    gimp_context_set_image (orig, display->gimage);

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[DISPLAY_CHANGED],
		   display);
}

gboolean
gimp_context_display_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->display_defined;
}

void
gimp_context_define_display (GimpContext *context,
			     gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    context->display = gimp_context_get_display (context);

  context->display_defined = defined;
}

/*  tool  ********************************************************************/

ToolType
gimp_context_get_tool (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, 0);
  context_find_defined (context, tool_defined);

  return context->tool;
}

void
gimp_context_set_tool (GimpContext *context,
		       ToolType     tool)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, tool_defined);

  if (context->tool == tool) return;

  context->tool = tool;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[TOOL_CHANGED],
		   tool);
}

gboolean
gimp_context_tool_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->tool_defined;
}

void
gimp_context_define_tool (GimpContext *context,
			  gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    context->tool = gimp_context_get_tool (context);

  context->tool_defined = defined;
}

/*  foreground color  ********************************************************/

void
gimp_context_get_foreground (GimpContext *context,
			     guchar       foreground[3])
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, foreground_defined);

  foreground[0] = context->foreground[0];
  foreground[1] = context->foreground[1];
  foreground[2] = context->foreground[2];
}

void
gimp_context_set_foreground (GimpContext *context,
			     guchar       foreground[3])
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, foreground_defined);

  if (context->foreground[0] == foreground[0] &&
      context->foreground[1] == foreground[1] &&
      context->foreground[2] == foreground[2]) return;

  context->foreground[0] = foreground[0];
  context->foreground[1] = foreground[1];
  context->foreground[2] = foreground[2];

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[FOREGROUND_CHANGED],
		   context->foreground);
}

gboolean
gimp_context_foreground_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->foreground_defined;
}

void
gimp_context_define_foreground (GimpContext *context,
				gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    {
      guchar col[3];

      gimp_context_get_foreground (context, col);
      context->foreground[0] = col[0];
      context->foreground[1] = col[1];
      context->foreground[2] = col[2];
    }

  context->foreground_defined = defined;
}

/*  background color  ********************************************************/

void
gimp_context_get_background (GimpContext *context,
			     guchar       background[3])
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, background_defined);

  background[0] = context->background[0];
  background[1] = context->background[1];
  background[2] = context->background[2];
}

void
gimp_context_set_background (GimpContext *context,
			     guchar       background[3])
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, background_defined);

  if (context->background[0] == background[0] &&
      context->background[1] == background[1] &&
      context->background[2] == background[2]) return;

  context->background[0] = background[0];
  context->background[1] = background[1];
  context->background[2] = background[2];

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[BACKGROUND_CHANGED],
		   context->background);
}

gboolean
gimp_context_background_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->background_defined;
}

void
gimp_context_define_background (GimpContext *context,
				gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    {
      guchar col[3];

      gimp_context_get_background (context, col);
      context->background[0] = col[0];
      context->background[1] = col[1];
      context->background[2] = col[2];
    }

  context->background_defined = defined;
}

/*  opacity  *****************************************************************/

gdouble
gimp_context_get_opacity (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, 1.0);
  context_find_defined (context, opacity_defined);

  return context->opacity;
}

void
gimp_context_set_opacity (GimpContext *context,
			  gdouble      opacity)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, opacity_defined);

  if (context->opacity == opacity) return;

  context->opacity = opacity;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[OPACITY_CHANGED],
		   opacity);
}

gboolean
gimp_context_opacity_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->opacity_defined;
}

void
gimp_context_define_opacity (GimpContext *context,
			     gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    context->opacity = gimp_context_get_opacity (context);

  context->opacity_defined = defined;
}

/*  paint mode  **************************************************************/

LayerModeEffects
gimp_context_get_paint_mode (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, 0);
  context_find_defined (context, paint_mode_defined);

  return context->paint_mode;
}

void
gimp_context_set_paint_mode (GimpContext     *context,
			     LayerModeEffects paint_mode)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, paint_mode_defined);

  if (context->paint_mode == paint_mode) return;

  context->paint_mode = paint_mode;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[PAINT_MODE_CHANGED],
		   paint_mode);
}

gboolean
gimp_context_paint_mode_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->paint_mode_defined;
}

void
gimp_context_define_paint_mode (GimpContext *context,
				gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    context->paint_mode = gimp_context_get_paint_mode (context);

  context->paint_mode_defined = defined;
}

/*  brush  *******************************************************************/

GimpBrush *
gimp_context_get_brush (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);
  context_find_defined (context, brush_defined);

  return context->brush;
}

void
gimp_context_set_brush (GimpContext *context,
			GimpBrush   *brush)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, brush_defined);

  if (context->brush == brush) return;

  context->brush = brush;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[BRUSH_CHANGED],
		   brush);
}

gboolean
gimp_context_brush_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->brush_defined;
}

void
gimp_context_define_brush (GimpContext *context,
			   gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    context->brush = gimp_context_get_brush (context);

  context->brush_defined = defined;
}

/*  pattern  *****************************************************************/

GPattern *
gimp_context_get_pattern (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);
  context_find_defined (context, pattern_defined);

  return context->pattern;
}

void
gimp_context_set_pattern (GimpContext *context,
			  GPattern    *pattern)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, pattern_defined);

  if (context->pattern == pattern) return;

  context->pattern = pattern;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[PATTERN_CHANGED],
		   pattern);
}

gboolean
gimp_context_pattern_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->pattern_defined;
}

void
gimp_context_define_pattern (GimpContext *context,
			     gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    context->pattern = gimp_context_get_pattern (context);

  context->pattern_defined = defined;
}

/*  gradient  ****************************************************************/

gradient_t *
gimp_context_get_gradient (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);
  context_find_defined (context, gradient_defined);

  return context->gradient;
}

void
gimp_context_set_gradient (GimpContext *context,
			   gradient_t  *gradient)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, gradient_defined);

  if (context->gradient == gradient) return;

  context->gradient = gradient;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[GRADIENT_CHANGED],
		   gradient);
}

gboolean
gimp_context_gradient_defined (GimpContext *context)
{
  context_return_val_if_fail (context, FALSE);

  return context->gradient_defined;
}

void
gimp_context_define_gradient (GimpContext *context,
			      gboolean     defined)
{
  context_return_if_fail (context);

  if (defined)
    context->gradient = gimp_context_get_gradient (context);

  context->gradient_defined = defined;
}
