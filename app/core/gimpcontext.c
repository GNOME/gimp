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
#include "gimpbrush.h"
#include "gimpbrushlist.h"
#include "gimpcontext.h"
#include "gimprc.h"
#include "gimpsignal.h"
#include "gradient_header.h"
#include "patterns.h"

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

/*  image  */
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
static void gimp_context_real_set_tool       (GimpContext      *context,
					      ToolType          tool);
static void gimp_context_copy_tool           (GimpContext      *src,
					      GimpContext      *dest);

/*  foreground  */
static void gimp_context_real_set_foreground (GimpContext      *context,
					      gint              r,
					      gint              g,
					      gint              b);
static void gimp_context_copy_foreground     (GimpContext      *src,
					      GimpContext      *dest);

/*  background  */
static void gimp_context_real_set_background (GimpContext      *context,
					      gint              r,
					      gint              g,
					      gint              b);
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
static void gimp_context_real_set_brush      (GimpContext      *context,
					      GimpBrush        *brush);
static void gimp_context_copy_brush          (GimpContext      *src,
					      GimpContext      *dest);

/*  pattern  */
static void gimp_context_real_set_pattern    (GimpContext      *context,
					      GPattern         *pattern);
static void gimp_context_copy_pattern        (GimpContext      *src,
					      GimpContext      *dest);

/*  gradient  */
static void gimp_context_real_set_gradient   (GimpContext      *context,
					      gradient_t       *gradient);
static void gimp_context_copy_gradient       (GimpContext      *src,
					      GimpContext      *dest);

/*  arguments  */

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
  ARG_GRADIENT
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
  "GimpContext::gradient"
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
  gimp_context_copy_gradient
};

/*  signals  */

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
  LAST_SIGNAL
};

static guint gimp_context_signals[LAST_SIGNAL] = { 0 };

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
  "gradient_changed"
};

static GtkSignalFunc gimp_context_signal_handlers[] =
{
  gimp_context_real_set_image,
  gimp_context_real_set_display,
  gimp_context_real_set_tool,
  gimp_context_real_set_foreground,
  gimp_context_real_set_background,
  gimp_context_real_set_opacity,
  gimp_context_real_set_paint_mode,
  gimp_context_real_set_brush,
  gimp_context_real_set_pattern,
  gimp_context_real_set_gradient
};

static GimpObjectClass * parent_class = NULL;

/*  the currently active context  */
static GimpContext *current_context  = NULL;

/*  the context user by the interface  */
static GimpContext *user_context     = NULL;

/*  the default context which is initialized from gimprc  */
static GimpContext *default_context  = NULL;

/*  the hardcoded standard context  */
static GimpContext *standard_context = NULL;

/*  the list of all contexts  */
static GSList      *context_list     = NULL;

/*****************************************************************************/
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

  if (context->parent)
    gimp_context_unset_parent (context);

  if (context->name)
    {
      g_free (context->name);
      context->name = NULL;
    }

  if (context->image)
    gtk_signal_disconnect_by_data (GTK_OBJECT (image_context), context);

  if (context->display)
    gtk_signal_disconnect_by_data (GTK_OBJECT (context->display->shell),
				   context);

  if (context->brush)
    {
      gtk_signal_disconnect_by_data (GTK_OBJECT (context->brush), context);
      gtk_signal_disconnect_by_data (GTK_OBJECT (brush_list), context);
    }

  if (context->brush_name)
    {
      g_free (context->brush_name);
      context->brush_name = NULL;
    }

  if (context->pattern_name)
    {
      g_free (context->pattern_name);
      context->pattern_name = NULL;
    }

  if (context->gradient_name)
    {
      g_free (context->gradient_name);
      context->gradient_name = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);

  context_list = g_slist_remove (context_list, context);
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
  gtk_object_add_arg_type (gimp_context_arg_names[GRADIENT_CHANGED],
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

  context->foreground[0] = 0;
  context->foreground[1] = 0;
  context->foreground[2] = 0;

  context->background[0] = 255;
  context->background[1] = 255;
  context->background[2] = 255;

  context->opacity    = 1.0;
  context->paint_mode = NORMAL_MODE;

  context->brush      = NULL;
  context->brush_name = NULL;

  context->pattern      = NULL;
  context->pattern_name = NULL;

  context->gradient      = NULL;
  context->gradient_name = NULL;

  context_list = g_slist_prepend (context_list, context);
}

/*****************************************************************************/
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

      gimp_context_copy_args (template, context, GIMP_CONTEXT_ALL_ARGS_MASK);
    }

  return context;
}

/*****************************************************************************/
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

/*****************************************************************************/
/*  functions manipulating a single context  *********************************/

gchar *
gimp_context_get_name (GimpContext *context)
{
  context_check_current (context);
  context_return_val_if_fail (context, NULL);

  return context->name;
}

void
gimp_context_set_name (GimpContext *context,
		       gchar       *name)
{
  context_check_current (context);
  context_return_if_fail (context);

  if (context->name)
    g_free (context->name);

  context->name = g_strdup (name ? name : "Unnamed");
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

  if (context == parent)
    return;

  for (arg = 0; arg < GIMP_CONTEXT_NUM_ARGS; arg++)
    if (! ((1 << arg) & context->defined_args))
      {
	gimp_context_copy_arg (parent, context, arg);
	gtk_signal_connect_object (GTK_OBJECT (parent),
				   gimp_context_signal_names[arg],
				   gimp_context_signal_handlers[arg],
				   GTK_OBJECT (context));
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
	      gtk_signal_connect_object (GTK_OBJECT (context->parent),
					 gimp_context_signal_names[arg],
					 gimp_context_signal_handlers[arg],
					 GTK_OBJECT (context));
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
gimp_context_define_args (GimpContext        *context,
			  GimpContextArgMask  args_mask,
			  gboolean            defined)
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

/*****************************************************************************/
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

void
gimp_context_image_changed (GimpContext *context)
{
  context_check_current (context);
  context_return_if_fail (context);

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[IMAGE_CHANGED],
		   context->image);
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
  if (context->image == image)
    return;

  if (image == NULL)
    gtk_signal_disconnect_by_data (GTK_OBJECT (image_context), context);

  if (context->image == NULL)
    gtk_signal_connect (GTK_OBJECT (image_context), "remove",
			GTK_SIGNAL_FUNC (gimp_context_image_removed),
			context);

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

void
gimp_context_display_changed (GimpContext *context)
{
  context_check_current (context);
  context_return_if_fail (context);

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[DISPLAY_CHANGED],
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
    gtk_signal_connect (GTK_OBJECT (display->shell), "destroy",
			GTK_SIGNAL_FUNC (gimp_context_display_destroy),
			context);

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

void
gimp_context_tool_changed (GimpContext *context)
{
  context_check_current (context);
  context_return_if_fail (context);

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[TOOL_CHANGED],
		   context->tool);
}

static void
gimp_context_real_set_tool (GimpContext *context,
			    ToolType     tool)
{
  if (context->tool == tool)
    return;

  context->tool = tool;
  gimp_context_tool_changed (context);
}

static void
gimp_context_copy_tool (GimpContext *src,
			GimpContext *dest)
{
  gimp_context_real_set_tool (dest, src->tool);
}

/*****************************************************************************/
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

void
gimp_context_foreground_changed (GimpContext *context)
{
  context_check_current (context);
  context_return_if_fail (context);

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[FOREGROUND_CHANGED],
		   context->foreground[0],
		   context->foreground[1],
		   context->foreground[2]);
}

static void
gimp_context_real_set_foreground (GimpContext *context,
				  gint         r,
				  gint         g,
				  gint         b)
{
  if (context->foreground[0] == r &&
      context->foreground[1] == g &&
      context->foreground[2] == b)
    return;

  context->foreground[0] = r;
  context->foreground[1] = g;
  context->foreground[2] = b;

  gimp_context_foreground_changed (context);
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

/*****************************************************************************/
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

void
gimp_context_background_changed (GimpContext *context)
{
  context_check_current (context);
  context_return_if_fail (context);

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[BACKGROUND_CHANGED],
		   context->background[0],
		   context->background[1],
		   context->background[2]);
}

static void
gimp_context_real_set_background (GimpContext *context,
				  gint         r,
				  gint         g,
				  gint         b)
{
  if (context->background[0] == r &&
      context->background[1] == g &&
      context->background[2] == b)
    return;

  context->background[0] = r;
  context->background[1] = g;
  context->background[2] = b;

  gimp_context_background_changed (context);
}

static void
gimp_context_copy_background (GimpContext *src,
			      GimpContext *dest)
{
  gimp_context_real_set_background (dest,
				    src->background[0],
				    src->background[1],
				    src->background[2]);
}

/*****************************************************************************/
/*  color utility functions  *************************************************/

void
gimp_context_set_default_colors (GimpContext *context)
{
  GimpContext *bg_context;

  bg_context = context;

  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, GIMP_CONTEXT_FOREGROUND_MASK);
  context_find_defined (bg_context, GIMP_CONTEXT_BACKGROUND_MASK);

  gimp_context_real_set_foreground (context, 0, 0, 0);
  gimp_context_real_set_background (bg_context, 255, 255, 255);
}

void
gimp_context_swap_colors (GimpContext *context)
{
  GimpContext *bg_context;
  guchar f_r, f_g, f_b;
  guchar b_r, b_g, b_b;

  bg_context = context;

  context_check_current (context);
  context_return_if_fail (context);
  context_find_defined (context, GIMP_CONTEXT_FOREGROUND_MASK);
  context_find_defined (bg_context, GIMP_CONTEXT_BACKGROUND_MASK);

  gimp_context_get_foreground (context, &f_r, &f_g, &f_b);
  gimp_context_get_background (bg_context, &b_r, &b_g, &b_b);

  gimp_context_real_set_foreground (context, b_r, b_g, b_b);
  gimp_context_real_set_background (bg_context, f_r, f_g, f_b);
}

/*****************************************************************************/
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

void
gimp_context_opacity_changed (GimpContext *context)
{
  context_check_current (context);
  context_return_if_fail (context);

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[OPACITY_CHANGED],
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

void
gimp_context_paint_mode_changed (GimpContext *context)
{
  context_check_current (context);
  context_return_if_fail (context);

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[PAINT_MODE_CHANGED],
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

void
gimp_context_brush_changed (GimpContext *context)
{
  context_check_current (context);
  context_return_if_fail (context);

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[BRUSH_CHANGED],
		   context->brush);
}

/*  the active brush was modified  */
static void
gimp_context_brush_dirty (GimpBrush   *brush,
			  GimpContext *context)
{
  g_free (context->brush_name);
  context->brush_name = g_strdup (brush->name);

  gimp_context_brush_changed (context);
}

/*  the active brush disappeared  */
static void
gimp_context_brush_removed (GimpBrushList *brush_list,
			    GimpBrush     *brush,
			    GimpContext   *context)
{
  if (brush == context->brush)
    {
      context->brush = NULL;

      gtk_signal_disconnect_by_data (GTK_OBJECT (brush), context);
      gtk_signal_disconnect_by_data (GTK_OBJECT (brush_list), context);
      gtk_object_unref (GTK_OBJECT (brush));
    }
}

static void
gimp_context_real_set_brush (GimpContext *context,
			     GimpBrush   *brush)
{
  if (! standard_brush)
    standard_brush = brushes_get_standard_brush ();

  if (context->brush == brush)
    return;

  if (context->brush_name && brush != standard_brush)
    {
      g_free (context->brush_name);
      context->brush_name = NULL;
    }

  /*  make sure the active brush is swapped before we get a new one...  */
  if (stingy_memory_use &&
      context->brush && context->brush->mask &&
      GTK_OBJECT (context->brush)->ref_count == 2)
    {
      temp_buf_swap (brush->mask);
    }

  /*  disconnect from the old brush's signals  */
  if (context->brush)
    {
      gtk_signal_disconnect_by_data (GTK_OBJECT (context->brush), context);
      gtk_object_unref (GTK_OBJECT (context->brush));

      /*  if we don't get a new brush, also disconnect from the brush list  */
      if (! brush)
	{
	  gtk_signal_disconnect_by_data (GTK_OBJECT (brush_list), context);
	}
    }
  /*  if we get a new brush but didn't have one before...  */
  else if (brush)
    {
      /*  ...connect to the brush list  */
      gtk_signal_connect (GTK_OBJECT (brush_list), "remove",
			  GTK_SIGNAL_FUNC (gimp_context_brush_removed),
			  context);
    }

  context->brush = brush;

  if (brush)
    {
      gtk_object_ref (GTK_OBJECT (brush));
      gtk_signal_connect (GTK_OBJECT (brush), "dirty",
			  GTK_SIGNAL_FUNC (gimp_context_brush_dirty),
			  context);
      gtk_signal_connect (GTK_OBJECT (brush), "rename",
			  GTK_SIGNAL_FUNC (gimp_context_brush_dirty),
			  context);

      /*  Make sure the active brush is unswapped... */
      if (stingy_memory_use &&
	  brush->mask &&
	  GTK_OBJECT (brush)->ref_count < 2)
	{
	  temp_buf_unswap (brush->mask);
	}

      if (brush != standard_brush)
	context->brush_name = g_strdup (brush->name);
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

static void
gimp_context_refresh_brush (GimpContext *context,
			    gpointer     data)
{
  GimpBrush *brush;

  if (! context->brush_name)
    context->brush_name = g_strdup (default_brush);

  if ((brush = gimp_brush_list_get_brush (brush_list, context->brush_name)))
    {
      gimp_context_real_set_brush (context, brush);
      return;
    }

  if (gimp_brush_list_length (brush_list))
    gimp_context_real_set_brush 
      (context, gimp_brush_list_get_brush_by_index (brush_list, 0));
  else
    gimp_context_real_set_brush (context, brushes_get_standard_brush ());
}

void
gimp_context_refresh_brushes (void)
{
  g_slist_foreach (context_list, (GFunc) gimp_context_refresh_brush, NULL);
}

/*****************************************************************************/
/*  pattern  *****************************************************************/

static GPattern *standard_pattern = NULL;

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

void
gimp_context_pattern_changed (GimpContext *context)
{
  context_check_current (context);
  context_return_if_fail (context);

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[PATTERN_CHANGED],
		   context->pattern);
}

static void
gimp_context_real_set_pattern (GimpContext *context,
			       GPattern    *pattern)
{
  if (! standard_pattern)
    standard_pattern = patterns_get_standard_pattern ();

  if (context->pattern == pattern)
    return;

  if (context->pattern_name && pattern != standard_pattern)
    {
      g_free (context->pattern_name);
      context->pattern_name = NULL;
    }

  context->pattern = pattern;

  if (pattern && pattern != standard_pattern)
    context->pattern_name = g_strdup (pattern->name);

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

static void
gimp_context_refresh_pattern (GimpContext *context,
			      gpointer     data)
{
  GPattern *pattern;

  if (! context->pattern_name)
    context->pattern_name = g_strdup (default_pattern);

  if ((pattern = pattern_list_get_pattern (pattern_list,
					   context->pattern_name)))
    {
      gimp_context_real_set_pattern (context, pattern);
      return;
    }

  if ((pattern = pattern_list_get_pattern_by_index (pattern_list, 0)))
    gimp_context_real_set_pattern (context, pattern);
  else
    gimp_context_real_set_pattern (context, patterns_get_standard_pattern ());
}

void
gimp_context_refresh_patterns (void)
{
  g_slist_foreach (context_list, (GFunc) gimp_context_refresh_pattern, NULL);
}

static void
gimp_context_update_pattern (GimpContext *context,
			     GPattern    *pattern)
{
  if (context->pattern == pattern)
    {
      if (context->pattern_name)
	g_free (context->pattern_name);

      context->pattern_name = g_strdup (pattern->name);

      gimp_context_pattern_changed (context);
    }
}

void
gimp_context_update_patterns (GPattern *pattern)
{
  g_slist_foreach (context_list, (GFunc) gimp_context_update_pattern, pattern);
}

/*****************************************************************************/
/*  gradient  ****************************************************************/

static gradient_t *standard_gradient = NULL;

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

void
gimp_context_gradient_changed (GimpContext *context)
{
  context_check_current (context);
  context_return_if_fail (context);

  gtk_signal_emit (GTK_OBJECT (context),
		   gimp_context_signals[GRADIENT_CHANGED],
		   context->gradient);
}

static void
gimp_context_real_set_gradient (GimpContext *context,
				gradient_t  *gradient)
{
  if (! standard_gradient)
    standard_gradient = gradients_get_standard_gradient ();

  if (context->gradient == gradient)
    return;

  if (context->gradient_name && gradient != standard_gradient)
    {
      g_free (context->gradient_name);
      context->gradient_name = NULL;
    }

  context->gradient = gradient;

  if (gradient && gradient != standard_gradient)
    context->gradient_name = g_strdup (gradient->name);

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

static void
gimp_context_refresh_gradient (GimpContext *context,
			       gpointer     data)
{
  gradient_t *gradient;

  if (! context->gradient_name)
    context->gradient_name = g_strdup (default_gradient);

  if ((gradient = gradient_list_get_gradient (gradients_list,
					      context->gradient_name)))
    {
      gimp_context_real_set_gradient (context, gradient);
      return;
    }

  if (gradients_list)
    gimp_context_real_set_gradient (context,
				    (gradient_t *) gradients_list->data);
  else
    gimp_context_real_set_gradient (context, gradients_get_standard_gradient ());
}

void
gimp_context_refresh_gradients (void)
{
  g_slist_foreach (context_list, (GFunc) gimp_context_refresh_gradient, NULL);
}

static void
gimp_context_update_gradient (GimpContext *context,
			      gradient_t  *gradient)
{
  if (context->gradient == gradient)
    {
      if (context->gradient_name)
	g_free (context->gradient_name);

      context->gradient_name = g_strdup (gradient->name);

      gimp_context_gradient_changed (context);
    }
}

void
gimp_context_update_gradients (gradient_t *gradient)
{
  g_slist_foreach (context_list, (GFunc) gimp_context_update_gradient, gradient);
}
