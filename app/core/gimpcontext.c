/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontext.c: Copyright (C) 1999 Michael Natterer <mitch@gimp.org>
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

#include "appenv.h"
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

#define context_find_defined(context,arg_mask) \
        while (!(((context)->defined_args) & arg_mask) && (context)->parent) \
          (context) = (context)->parent

typedef void (* GimpContextCopyArgFunc) (GimpContext *src, GimpContext *dest);

/*  local function prototypes  */
static void gimp_context_real_set_image      (GimpContext      *context,
					      GimpImage        *image);
static void gimp_context_real_set_display    (GimpContext      *context,
					      GDisplay         *display);
static void gimp_context_real_set_tool       (GimpContext      *context,
					      ToolType          tool);
static void gimp_context_real_set_foreground (GimpContext      *context,
					      gint              r,
					      gint              g,
					      gint              b);
static void gimp_context_real_set_background (GimpContext      *context,
					      gint              r,
					      gint              g,
					      gint              b);
static void gimp_context_real_set_opacity    (GimpContext      *context,
					      gdouble           opacity);
static void gimp_context_real_set_paint_mode (GimpContext      *context,
					      LayerModeEffects  paint_mode);
static void gimp_context_real_set_brush      (GimpContext      *context,
					      GimpBrush        *brush);
static void gimp_context_real_set_pattern    (GimpContext      *context,
					      GPattern         *pattern);
static void gimp_context_real_set_gradient   (GimpContext      *context,
					      gradient_t       *gradient);

static void gimp_context_copy_image          (GimpContext      *src,
					      GimpContext      *dest);
static void gimp_context_copy_display        (GimpContext      *src,
					      GimpContext      *dest);
static void gimp_context_copy_tool           (GimpContext      *src,
					      GimpContext      *dest);
static void gimp_context_copy_foreground     (GimpContext      *src,
					      GimpContext      *dest);
static void gimp_context_copy_background     (GimpContext      *src,
					      GimpContext      *dest);
static void gimp_context_copy_opacity        (GimpContext      *src,
					      GimpContext      *dest);
static void gimp_context_copy_paint_mode     (GimpContext      *src,
					      GimpContext      *dest);
static void gimp_context_copy_brush          (GimpContext      *src,
					      GimpContext      *dest);
static void gimp_context_copy_pattern        (GimpContext      *src,
					      GimpContext      *dest);
static void gimp_context_copy_gradient       (GimpContext      *src,
					      GimpContext      *dest);

static void gimp_context_image_changed       (GimpContext      *parent,
					      GimpImage        *image,
					      GimpContext      *child);
static void gimp_context_display_changed     (GimpContext      *parent,
					      GDisplay         *display,
					      GimpContext      *child);
static void gimp_context_tool_changed        (GimpContext      *parent,
					      ToolType          tool,
					      GimpContext      *child);
static void gimp_context_foreground_changed  (GimpContext      *parent,
					      gint              r,
					      gint              g,
					      gint              b,
					      GimpContext      *child);
static void gimp_context_background_changed  (GimpContext      *parent,
					      gint              r,
					      gint              g,
					      gint              b,
					      GimpContext      *child);
static void gimp_context_opacity_changed     (GimpContext      *parent,
					      gdouble           opacity,
					      GimpContext      *child);
static void gimp_context_paint_mode_changed  (GimpContext      *parent,
					      LayerModeEffects  paint_mode,
					      GimpContext      *child);
static void gimp_context_brush_changed       (GimpContext      *parent,
					      GimpBrush        *brush,
					      GimpContext      *child);
static void gimp_context_pattern_changed     (GimpContext      *parent,
					      GPattern         *pattern,
					      GimpContext      *child);
static void gimp_context_gradient_changed    (GimpContext      *parent,
					      gradient_t       *gradient,
					      GimpContext      *child);

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

static GimpContextCopyArgFunc gimp_context_copy_arg_funcs[LAST_SIGNAL] =
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
  gimp_context_copy_gradient
};

static gchar *gimp_context_arg_names[LAST_SIGNAL] =
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
  "GimpContext::gradient"
};

static guint gimp_context_signals[LAST_SIGNAL] = { 0 };

static gchar *gimp_context_signal_names[LAST_SIGNAL] =
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
  "gradient_changed"
};

static GtkSignalFunc gimp_context_signal_handlers[LAST_SIGNAL] =
{
  gimp_context_image_changed,
  gimp_context_display_changed,
  gimp_context_tool_changed,
  gimp_context_foreground_changed,
  gimp_context_background_changed,
  gimp_context_opacity_changed,
  gimp_context_paint_mode_changed,
  gimp_context_brush_changed,
  gimp_context_pattern_changed,
  gimp_context_gradient_changed
};

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
      {
	guchar *col = GTK_VALUE_POINTER (*arg);
	gimp_context_set_foreground (context, col[0], col[1], col[2]);
      }
      break;
    case ARG_BACKGROUND:
      {
	guchar *col = GTK_VALUE_POINTER (*arg);
	gimp_context_set_background (context, col[0], col[1], col[2]);
      }
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
	guchar *col = GTK_VALUE_POINTER (*arg);
	gimp_context_get_foreground (context, &col[0], &col[1], &col[2]);
      }
      break;
    case ARG_BACKGROUND:
      {
	guchar *col = GTK_VALUE_POINTER (*arg);
	gimp_context_get_background (context, &col[0], &col[1], &col[2]);
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

  context = GIMP_CONTEXT (object);

  if (context->parent &&
      context->defined_args != GIMP_CONTEXT_ALL_ARGS_MASK)
    gtk_signal_disconnect_by_data (GTK_OBJECT (context->parent), context);

  if (context->name)
    g_free (context->name);

  if (context->image)
    gtk_signal_disconnect_by_data (GTK_OBJECT (image_context), context);

  if (context->display)
    gtk_signal_disconnect_by_data (GTK_OBJECT (context->display->shell),
				   context);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gimp_context_class_init (GimpContextClass *klass)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS (klass);

  gtk_object_add_arg_type (gimp_context_arg_names[IMAGE_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_IMAGE);
  gtk_object_add_arg_type (gimp_context_arg_names[DISPLAY_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_DISPLAY);
  gtk_object_add_arg_type (gimp_context_arg_names[TOOL_CHANGED],
			   GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_TOOL);
  gtk_object_add_arg_type (gimp_context_arg_names[FOREGROUND_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_FOREGROUND);
  gtk_object_add_arg_type (gimp_context_arg_names[BACKGROUND_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_BACKGROUND);
  gtk_object_add_arg_type (gimp_context_arg_names[OPACITY_CHANGED],
			   GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_OPACITY);
  gtk_object_add_arg_type (gimp_context_arg_names[PAINT_MODE_CHANGED],
			   GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_PAINT_MODE);
  gtk_object_add_arg_type (gimp_context_arg_names[BRUSH_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_BRUSH);
  gtk_object_add_arg_type (gimp_context_arg_names[PATTERN_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_PATTERN);
  gtk_object_add_arg_type (gimp_context_arg_names[BRUSH_CHANGED],
			   GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_GRADIENT);

  parent_class = gtk_type_class (gimp_object_get_type ());

  gimp_context_signals[IMAGE_CHANGED] =
    gimp_signal_new (gimp_context_signal_names[IMAGE_CHANGED],
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					image_changed),
		     gimp_sigtype_pointer);

  gimp_context_signals[DISPLAY_CHANGED] =
    gimp_signal_new (gimp_context_signal_names[DISPLAY_CHANGED],
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					display_changed),
		     gimp_sigtype_pointer);

  gimp_context_signals[TOOL_CHANGED] =
    gimp_signal_new (gimp_context_signal_names[TOOL_CHANGED],
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					tool_changed),
		     gimp_sigtype_int);

  gimp_context_signals[FOREGROUND_CHANGED] =
    gimp_signal_new (gimp_context_signal_names[FOREGROUND_CHANGED],
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					foreground_changed),
		     gimp_sigtype_int_int_int);

  gimp_context_signals[BACKGROUND_CHANGED] =
    gimp_signal_new (gimp_context_signal_names[BACKGROUND_CHANGED],
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					background_changed),
		     gimp_sigtype_int_int_int);

  gimp_context_signals[OPACITY_CHANGED] =
    gimp_signal_new (gimp_context_signal_names[OPACITY_CHANGED],
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					opacity_changed),
		     gimp_sigtype_double);

  gimp_context_signals[PAINT_MODE_CHANGED] =
    gimp_signal_new (gimp_context_signal_names[PAINT_MODE_CHANGED],
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					paint_mode_changed),
		     gimp_sigtype_int);

  gimp_context_signals[BRUSH_CHANGED] =
    gimp_signal_new (gimp_context_signal_names[BRUSH_CHANGED],
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					brush_changed),
		     gimp_sigtype_pointer);

  gimp_context_signals[PATTERN_CHANGED] =
    gimp_signal_new (gimp_context_signal_names[PATTERN_CHANGED],
		     GTK_RUN_FIRST,
		     object_class->type,
		     GTK_SIGNAL_OFFSET (GimpContextClass,
					pattern_changed),
		     gimp_sigtype_pointer);

  gimp_context_signals[GRADIENT_CHANGED] =
    gimp_signal_new (gimp_context_signal_names[GRADIENT_CHANGED],
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
  context->name   = NULL;
  context->parent = NULL;

  context->defined_args = GIMP_CONTEXT_ALL_ARGS_MASK;

  context->image   = NULL;
  context->display = NULL;

  context->tool = RECT_SELECT;

  context->foreground[0] = 255;
  context->foreground[1] = 255;
  context->foreground[2] = 255;

  context->background[0] = 0;
  context->background[1] = 0;
  context->background[2] = 0;

  context->opacity    = 1.0;
  context->paint_mode = NORMAL_MODE;

  context->brush    = NULL;
  context->pattern  = NULL;
  context->gradient = NULL;
}

/*  public functions  ********************************************************/

GtkType
gimp_context_get_type (void)
{
  static GtkType context_type = 0;

  if (! context_type)
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
		  GimpContext *template)
{
  GimpContext *context;

  g_return_val_if_fail (!template || GIMP_IS_CONTEXT (template), NULL);

  context = gtk_type_new (gimp_context_get_type ());

  /*  FIXME: need unique names here  */
  context->name = g_strdup (name ? name : "Unnamed");

  if (template)
    {
      context->defined_args = template->defined_args;

      context->image        = gimp_context_get_image (template);
      context->display      = gimp_context_get_display (template);
      context->tool         = gimp_context_get_tool (template);
      gimp_context_get_foreground (template,
				   &context->foreground[0],
				   &context->foreground[1],
				   &context->foreground[2]);
      gimp_context_get_background (template,
				   &context->background[0],
				   &context->background[1],
				   &context->background[2]);
      context->opacity      = gimp_context_get_opacity (template);
      context->paint_mode   = gimp_context_get_paint_mode (template);
      context->brush        = gimp_context_get_brush (template);
      context->pattern      = gimp_context_get_pattern (template);
      context->gradient     = gimp_context_get_gradient (template);
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
      standard_context = gimp_context_new ("Standard", NULL);

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
  context_check_current (context);
  context_return_val_if_fail (context, NULL);

  return context->parent;
}

void
gimp_context_set_parent (GimpContext *context,
			 GimpContext *parent)
{
  GimpContextArgType arg;

  context_check_current (context);
  context_return_if_fail (context);
  g_return_if_fail (!parent || GIMP_IS_CONTEXT (parent));

  if (context == parent) return;

  for (arg = 0; arg < GIMP_CONTEXT_NUM_ARGS; arg++)
    if (! ((1 << arg) & context->defined_args))
      {
	gimp_context_copy_arg (parent, context, arg);
	gtk_signal_connect (GTK_OBJECT (parent),
			    gimp_context_signal_names[arg],
			    gimp_context_signal_handlers[arg],
			    context);
      }

  context->parent = parent;
}

void
gimp_context_unset_parent (GimpContext *context)
{
  context_check_current (context);
  context_return_if_fail (context);

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

  context_check_current (context);
  context_return_if_fail (context);
  g_return_if_fail ((arg >= 0) && (arg < GIMP_CONTEXT_NUM_ARGS));

  mask = (1 << arg);

  if (defined)
    {
      if (! (context->defined_args & mask))
	{
	  context->defined_args |= mask;
	  if (context->parent)
	    gtk_signal_disconnect_by_func (GTK_OBJECT (context->parent),
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
	      gtk_signal_connect (GTK_OBJECT (context->parent),
				  gimp_context_signal_names[arg],
				  gimp_context_signal_handlers[arg],
				  context);
	    }
	}
    }
}

gboolean
gimp_context_arg_defined (GimpContext        *context,
			  GimpContextArgType  arg)
{
  context_check_current (context);
  context_return_val_if_fail (context, FALSE);

  return (context->defined_args & (1 << arg)) ? TRUE : FALSE;
}

void
gimp_context_define_args (GimpContext *context,
			  guint32      args_mask,
			  gboolean     defined)
{
  GimpContextArgType arg;

  context_check_current (context);
  context_return_if_fail (context);

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
  context_check_current (src);
  context_return_if_fail (src);
  context_check_current (dest);
  context_return_if_fail (dest);
  g_return_if_fail ((arg >= 0) && (arg < GIMP_CONTEXT_NUM_ARGS));

  (* gimp_context_copy_arg_funcs[arg]) (src, dest);
}

void
gimp_context_copy_args (GimpContext        *src,
			GimpContext        *dest,
			GimpContextArgMask  args_mask)
{
  GimpContextArgType arg;

  context_check_current (src);
  context_return_if_fail (src);
  context_check_current (dest);
  context_return_if_fail (dest);

  for (arg = 0; arg < GIMP_CONTEXT_NUM_ARGS; arg++)
    if ((1 << arg) & args_mask)
      {
	gimp_context_copy_arg (src, dest, arg);
      }
}

/*  attribute access functions  */

/*  image  *******************************************************************/

GimpImage *
gimp_context_get_image (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);

  return context->image;
}

void
gimp_context_set_image (GimpContext *context,
			GimpImage   *image)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, GIMP_CONTEXT_IMAGE_MASK);

  gimp_context_real_set_image (context, image);
}

/*  handle disappearing images  */
static void
gimp_context_image_removed (GimpSet     *set,
			    GimpImage   *image,
			    GimpContext *context)
{
  if (context->image == image)
    gimp_context_real_set_image (context, NULL);
}

static void
gimp_context_real_set_image (GimpContext *context,
			     GimpImage   *image)
{
  if (context->image == image) return;

  if (image == NULL)
    gtk_signal_disconnect_by_data (GTK_OBJECT (image_context), context);

  if (context->image == NULL)
    gtk_signal_connect (GTK_OBJECT (image_context), "remove",
			GTK_SIGNAL_FUNC (gimp_context_image_removed),
			context);

  context->image = image;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[IMAGE_CHANGED],
		   image);
}

static void
gimp_context_copy_image (GimpContext *src,
			 GimpContext *dest)
{
  gimp_context_real_set_image (dest, src->image);
}

static void
gimp_context_image_changed (GimpContext *parent,
			    GimpImage   *image,
			    GimpContext *child)
{
  gimp_context_real_set_image (child, image);
}

/*  display  *****************************************************************/

GDisplay *
gimp_context_get_display (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);

  return context->display;
}

void
gimp_context_set_display (GimpContext *context,
			  GDisplay    *display)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, GIMP_CONTEXT_DISPLAY_MASK);

  gimp_context_real_set_display (context, display);
}

/*  handle dissapearing displays  */
static void
gimp_context_display_destroy (GtkWidget   *disp_shell,
			      GimpContext *context)
{
  gimp_context_real_set_display (context, NULL);
}

static void
gimp_context_real_set_display (GimpContext *context,
			       GDisplay    *display)
{
  if (context->display == display) return;

  if (context->display && GTK_IS_OBJECT (context->display->shell))
    gtk_signal_disconnect_by_data (GTK_OBJECT (context->display->shell),
				   context);

  if (display)
    gtk_signal_connect (GTK_OBJECT (display->shell), "destroy",
			GTK_SIGNAL_FUNC (gimp_context_display_destroy),
			context);

  context->display = display;

  /*  set the image _before_ emitting the display_changed signal  */
  if (display)
    gimp_context_real_set_image (context, display->gimage);

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[DISPLAY_CHANGED],
		   display);
}

static void
gimp_context_copy_display (GimpContext *src,
			   GimpContext *dest)
{
  gimp_context_real_set_display (dest, src->display);
}

static void
gimp_context_display_changed (GimpContext *parent,
			      GDisplay    *display,
			      GimpContext *child)
{
  gimp_context_real_set_display (child, display);
}

/*  tool  ********************************************************************/

ToolType
gimp_context_get_tool (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, 0);

  return context->tool;
}

void
gimp_context_set_tool (GimpContext *context,
		       ToolType     tool)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, GIMP_CONTEXT_TOOL_MASK);

  gimp_context_real_set_tool (context, tool);
}

static void
gimp_context_real_set_tool (GimpContext *context,
			    ToolType     tool)
{
  if (context->tool == tool) return;

  context->tool = tool;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[TOOL_CHANGED],
		   tool);
}

static void
gimp_context_copy_tool (GimpContext *src,
			GimpContext *dest)
{
  gimp_context_real_set_tool (dest, src->tool);
}

static void
gimp_context_tool_changed (GimpContext *parent,
			   ToolType     tool,
			   GimpContext *child)
{
  gimp_context_real_set_tool (child, tool);
}

/*  foreground color  ********************************************************/

void
gimp_context_get_foreground (GimpContext *context,
			     guchar      *r,
			     guchar      *g,
			     guchar      *b)
{
  context_check_current (context);
  context_return_if_fail (context);

  *r = context->foreground[0];
  *g = context->foreground[1];
  *b = context->foreground[2];
}

void
gimp_context_set_foreground (GimpContext *context,
			     gint         r,
			     gint         g,
			     gint         b)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, GIMP_CONTEXT_FOREGROUND_MASK);

  gimp_context_real_set_foreground (context, r, g, b);
}

static void
gimp_context_real_set_foreground (GimpContext *context,
				  gint         r,
				  gint         g,
				  gint         b)
{
  if (context->foreground[0] == r &&
      context->foreground[1] == g &&
      context->foreground[2] == b) return;

  context->foreground[0] = r;
  context->foreground[1] = g;
  context->foreground[2] = b;

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[FOREGROUND_CHANGED],
		   r, g, b);
}

static void
gimp_context_copy_foreground (GimpContext *src,
			      GimpContext *dest)
{
  gimp_context_real_set_foreground (dest,
				    src->foreground[0],
				    src->foreground[1],
				    src->foreground[2]);
}

static void
gimp_context_foreground_changed (GimpContext *parent,
				 gint         r,
				 gint         g,
				 gint         b,
				 GimpContext *child)
{
  gimp_context_real_set_foreground (child, r, g, b);
}

/*  background color  ********************************************************/

void
gimp_context_get_background (GimpContext *context,
			     guchar      *r,
			     guchar      *g,
			     guchar      *b)
{
  context_check_current (context);
  context_return_if_fail (context);

  *r = context->background[0];
  *g = context->background[1];
  *b = context->background[2];
}

void
gimp_context_set_background (GimpContext *context,
			     gint         r,
			     gint         g,
			     gint         b)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, GIMP_CONTEXT_BACKGROUND_MASK);

  gimp_context_real_set_background (context, r, g, b);
}

static void
gimp_context_real_set_background (GimpContext *context,
				  gint         r,
				  gint         g,
				  gint         b)
{
  if (context->background[0] == r &&
      context->background[1] == g &&
      context->background[2] == b) return;

  context->background[0] = r;
  context->background[1] = g;
  context->background[2] = b;

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[BACKGROUND_CHANGED],
		   r, g, b);
}

static void
gimp_context_copy_background (GimpContext *src,
			      GimpContext *dest)
{
  gimp_context_real_set_foreground (dest,
				    src->background[0],
				    src->background[1],
				    src->background[2]);
}

static void
gimp_context_background_changed (GimpContext *parent,
				 gint         r,
				 gint         g,
				 gint         b,
				 GimpContext *child)
{
  gimp_context_real_set_background (child, r, g, b);
}

/*  opacity  *****************************************************************/

gdouble
gimp_context_get_opacity (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, 1.0);

  return context->opacity;
}

void
gimp_context_set_opacity (GimpContext *context,
			  gdouble      opacity)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, GIMP_CONTEXT_OPACITY_MASK);

  gimp_context_real_set_opacity (context, opacity);
}

static void
gimp_context_real_set_opacity (GimpContext *context,
			       gdouble      opacity)
{
  if (context->opacity == opacity) return;

  context->opacity = opacity;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[OPACITY_CHANGED],
		   opacity);
}

static void
gimp_context_copy_opacity (GimpContext *src,
			   GimpContext *dest)
{
  gimp_context_real_set_opacity (dest, src->opacity);
}

static void
gimp_context_opacity_changed (GimpContext *parent,
			      gdouble      opacity,
			      GimpContext *child)
{
  gimp_context_real_set_opacity (child, opacity);
}

/*  paint mode  **************************************************************/

LayerModeEffects
gimp_context_get_paint_mode (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, 0);

  return context->paint_mode;
}

void
gimp_context_set_paint_mode (GimpContext     *context,
			     LayerModeEffects paint_mode)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, GIMP_CONTEXT_PAINT_MODE_MASK);

  gimp_context_real_set_paint_mode (context, paint_mode);
}

static void
gimp_context_real_set_paint_mode (GimpContext     *context,
				  LayerModeEffects paint_mode)
{
  if (context->paint_mode == paint_mode) return;

  context->paint_mode = paint_mode;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[PAINT_MODE_CHANGED],
		   paint_mode);
}

static void
gimp_context_copy_paint_mode (GimpContext *src,
			      GimpContext *dest)
{
  gimp_context_real_set_paint_mode (dest, src->paint_mode);
}

static void
gimp_context_paint_mode_changed (GimpContext      *parent,
				 LayerModeEffects  paint_mode,
				 GimpContext      *child)
{
  gimp_context_real_set_paint_mode (child, paint_mode);
}

/*  brush  *******************************************************************/

GimpBrush *
gimp_context_get_brush (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);

  return context->brush;
}

void
gimp_context_set_brush (GimpContext *context,
			GimpBrush   *brush)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, GIMP_CONTEXT_BRUSH_MASK);

  gimp_context_real_set_brush (context, brush);
}

static void
gimp_context_real_set_brush (GimpContext *context,
			     GimpBrush   *brush)
{
  if (context->brush == brush) return;

  context->brush = brush;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[BRUSH_CHANGED],
		   brush);
}

static void
gimp_context_copy_brush (GimpContext *src,
			 GimpContext *dest)
{
  gimp_context_real_set_brush (dest, src->brush);
}

static void
gimp_context_brush_changed (GimpContext *parent,
			    GimpBrush   *brush,
			    GimpContext *child)
{
  gimp_context_real_set_brush (child, brush);
}

/*  pattern  *****************************************************************/

GPattern *
gimp_context_get_pattern (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);

  return context->pattern;
}

void
gimp_context_set_pattern (GimpContext *context,
			  GPattern    *pattern)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, GIMP_CONTEXT_PATTERN_MASK);

  gimp_context_real_set_pattern (context, pattern);
}

static void
gimp_context_real_set_pattern (GimpContext *context,
			       GPattern    *pattern)
{
  if (context->pattern == pattern) return;

  context->pattern = pattern;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[PATTERN_CHANGED],
		   pattern);
}

static void
gimp_context_copy_pattern (GimpContext *src,
			   GimpContext *dest)
{
  gimp_context_real_set_pattern (dest, src->pattern);
}

static void
gimp_context_pattern_changed (GimpContext *parent,
			      GPattern    *pattern,
			      GimpContext *child)
{
  gimp_context_real_set_pattern (child, pattern);
}

/*  gradient  ****************************************************************/

gradient_t *
gimp_context_get_gradient (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);

  return context->gradient;
}

void
gimp_context_set_gradient (GimpContext *context,
			   gradient_t  *gradient)
{
  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, GIMP_CONTEXT_GRADIENT_MASK);

  gimp_context_real_set_gradient (context, gradient);
}

static void
gimp_context_real_set_gradient (GimpContext *context,
				gradient_t  *gradient)
{
  if (context->gradient == gradient) return;

  context->gradient = gradient;
  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[GRADIENT_CHANGED],
		   gradient);
}

static void
gimp_context_copy_gradient (GimpContext *src,
			    GimpContext *dest)
{
  gimp_context_real_set_gradient (dest, src->gradient);
}

static void
gimp_context_gradient_changed (GimpContext *parent,
			       gradient_t  *gradient,
			       GimpContext *child)
{
  gimp_context_real_set_gradient (child, gradient);
}
