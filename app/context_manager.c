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

#include "config.h"

#include <gtk/gtk.h>

#include "core/core-types.h"

#include "core/gimpbrush.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpbrushpipe.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"
#include "core/gimplist.h"
#include "core/gimpimage.h"
#include "core/gimppalette.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "tools/gimptool.h"
#include "tools/paint_options.h"
#include "tools/tool_manager.h"
#include "tools/tools.h"

#include "gui/brush-select.h"

#include "appenv.h"
#include "context_manager.h"
#include "gdisplay.h"
#include "gimprc.h"


#define PAINT_OPTIONS_MASK GIMP_CONTEXT_OPACITY_MASK | \
                           GIMP_CONTEXT_PAINT_MODE_MASK


/*
 *  the list of all images
 */
GimpContainer *image_context = NULL;

/*
 *  the global data lists
 */
GimpDataFactory *global_brush_factory    = NULL;
GimpDataFactory *global_pattern_factory  = NULL;
GimpDataFactory *global_gradient_factory = NULL;
GimpDataFactory *global_palette_factory  = NULL;

/*
 *  the global tool context
 */
GimpContext *global_tool_context = NULL;



static void
context_manager_display_changed (GimpContext *context,
				 GDisplay    *display,
				 gpointer     data)
{
  gdisplay_set_menu_sensitivity (display);
}

static void
context_manager_tool_changed (GimpContext  *user_context,
			      GimpToolInfo *tool_info,
			      gpointer      data)
{
  if (! tool_info)
    return;

  /* FIXME: gimp_busy HACK */
  if (gimp_busy)
    {
      /*  there may be contexts waiting for the user_context's "tool_changed"
       *  signal, so stop emitting it.
       */
      gtk_signal_emit_stop_by_name (GTK_OBJECT (user_context), "tool_changed");

      if (GTK_OBJECT (active_tool)->klass->type != tool_info->tool_type)
	{
	  gtk_signal_handler_block_by_func (GTK_OBJECT (user_context),
					    context_manager_tool_changed,
					    NULL);

	  /*  explicitly set the current tool  */
	  gimp_context_set_tool (user_context,
				 tool_manager_get_info_by_tool (active_tool));

	  gtk_signal_handler_unblock_by_func (GTK_OBJECT (user_context),
					      context_manager_tool_changed,
					      NULL);
	}
    }
  else
    {
      GimpTool    *new_tool     = NULL;
      GimpContext *tool_context = NULL;

      if (tool_info->tool_type != GTK_TYPE_NONE)
	{
	  new_tool = gtk_type_new (tool_info->tool_type);
	}
      else
	{
	  g_warning ("%s(): tool_info contains no valid GtkType",
		     G_GNUC_FUNCTION);
	  return;
	}

      if (! gimprc.global_paint_options)
	{
	  if (active_tool &&
	      (tool_context = tool_manager_get_info_by_tool (active_tool)->context))
	    {
	      gimp_context_unset_parent (tool_context);
	    }

	  if ((tool_context = tool_info->context))
	    {
	      gimp_context_copy_args (tool_context, user_context,
				      PAINT_OPTIONS_MASK);
	      gimp_context_set_parent (tool_context, user_context);
	    }
	}

      tool_manager_select_tool (new_tool);
    }
}

void
context_manager_init (void)
{
  GimpContext *standard_context;
  GimpContext *default_context;
  GimpContext *user_context;
  GimpContext *tool_context;

  static const GimpDataFactoryLoaderEntry brush_loader_entries[] =
  {
    { gimp_brush_load,           GIMP_BRUSH_FILE_EXTENSION           },
    { gimp_brush_load,           GIMP_BRUSH_PIXMAP_FILE_EXTENSION    },
    { gimp_brush_generated_load, GIMP_BRUSH_GENERATED_FILE_EXTENSION },
    { gimp_brush_pipe_load,      GIMP_BRUSH_PIPE_FILE_EXTENSION      }
  };
  static gint n_brush_loader_entries = (sizeof (brush_loader_entries) /
					sizeof (brush_loader_entries[0]));

  static const GimpDataFactoryLoaderEntry pattern_loader_entries[] =
  {
    { gimp_pattern_load, GIMP_PATTERN_FILE_EXTENSION }
  };
  static gint n_pattern_loader_entries = (sizeof (pattern_loader_entries) /
					  sizeof (pattern_loader_entries[0]));

  static const GimpDataFactoryLoaderEntry gradient_loader_entries[] =
  {
    { gimp_gradient_load, GIMP_GRADIENT_FILE_EXTENSION },
    { gimp_gradient_load, NULL /* legacy loader */     }
  };
  static gint n_gradient_loader_entries = (sizeof (gradient_loader_entries) /
					   sizeof (gradient_loader_entries[0]));

  static const GimpDataFactoryLoaderEntry palette_loader_entries[] =
  {
    { gimp_palette_load, GIMP_PALETTE_FILE_EXTENSION },
    { gimp_palette_load, NULL /* legacy loader */    }
  };
  static gint n_palette_loader_entries = (sizeof (palette_loader_entries) /
					  sizeof (palette_loader_entries[0]));

  /* Create the context of all existing images */
  image_context = gimp_list_new (GIMP_TYPE_IMAGE, GIMP_CONTAINER_POLICY_WEAK);

  /* Create the global data factories */
  global_brush_factory =
    gimp_data_factory_new (GIMP_TYPE_BRUSH,
			   (const gchar **) &gimprc.brush_path,
			   brush_loader_entries,
			   n_brush_loader_entries,
			   gimp_brush_new,
			   gimp_brush_get_standard);

  global_pattern_factory =
    gimp_data_factory_new (GIMP_TYPE_PATTERN,
			   (const gchar **) &gimprc.pattern_path,
			   pattern_loader_entries,
			   n_pattern_loader_entries,
			   gimp_pattern_new,
			   gimp_pattern_get_standard);

  global_gradient_factory =
    gimp_data_factory_new (GIMP_TYPE_GRADIENT,
			   (const gchar **) &gimprc.gradient_path,
			   gradient_loader_entries,
			   n_gradient_loader_entries,
			   gimp_gradient_new,
			   gimp_gradient_get_standard);

  global_palette_factory =
    gimp_data_factory_new (GIMP_TYPE_PALETTE,
			   (const gchar **) &gimprc.palette_path,
			   palette_loader_entries,
			   n_palette_loader_entries,
			   gimp_palette_new,
			   gimp_palette_get_standard);

  /*  Create the global tool info list  */
  tool_manager_init ();

  /*  Implicitly create the standard context  */
  standard_context = gimp_context_get_standard ();

  /*  TODO: load from disk  */
  default_context = gimp_context_new ("Default", NULL);

  gimp_context_set_default (default_context);

  /*  Initialize the user context with the default context's values  */
  user_context = gimp_context_new ("User", default_context);
  gimp_context_set_user (user_context);

  /*  Update the tear-off menus  */
  gtk_signal_connect (GTK_OBJECT (user_context), "display_changed",
		      GTK_SIGNAL_FUNC (context_manager_display_changed),
		      NULL);

  /*  Update the tool system  */
  gtk_signal_connect (GTK_OBJECT (user_context), "tool_changed",
		      GTK_SIGNAL_FUNC (context_manager_tool_changed),
		      NULL);

  /*  Make the user contect the currently active context  */
  gimp_context_set_current (user_context);

  /*  Create a context to store the paint options of the
   *  global paint options mode
   */
  global_tool_context = gimp_context_new ("Global Tool Context", user_context);

  /*  TODO: add foreground, background, brush, pattern, gradient  */
  gimp_context_define_args (global_tool_context, PAINT_OPTIONS_MASK, FALSE);

  /* register internal tools */
  tools_init ();

  if (! gimprc.global_paint_options && active_tool &&
      (tool_context = tool_manager_get_info_by_tool (active_tool)->context))
    {
      gimp_context_set_parent (tool_context, user_context);
    }
  else if (gimprc.global_paint_options)
    {
      gimp_context_set_parent (global_tool_context, user_context);
    }

  gimp_container_thaw (global_tool_info_list);
}

void
context_manager_free (void)
{
  gtk_object_unref (GTK_OBJECT (global_tool_context));
  global_tool_context = NULL;

  gtk_object_unref (GTK_OBJECT (gimp_context_get_user ()));
  gimp_context_set_user (NULL);
  gimp_context_set_current (NULL);

  /*  TODO: Save to disk before destroying  */
  gtk_object_unref (GTK_OBJECT (gimp_context_get_default ()));
  gimp_context_set_default (NULL);

  gimp_data_factory_data_free (global_brush_factory);
  gimp_data_factory_data_free (global_pattern_factory);
  gimp_data_factory_data_free (global_gradient_factory);
  gimp_data_factory_data_free (global_palette_factory);
}

void
context_manager_set_global_paint_options (gboolean global)
{
  GimpToolInfo *tool_info;
  GimpContext  *context;

  if (global == gimprc.global_paint_options)
    return;

  paint_options_set_global (global);

  /*  NULL is the main brush selection  */
  brush_select_show_paint_options (NULL, global);

  tool_info = gimp_context_get_tool (gimp_context_get_user ());

  if (global)
    {
      if (tool_info && (context = tool_info->context))
	{
	  gimp_context_unset_parent (context);
	}

      gimp_context_copy_args (global_tool_context, gimp_context_get_user (),
			      PAINT_OPTIONS_MASK);
      gimp_context_set_parent (global_tool_context, gimp_context_get_user ());
    }
  else
    {
      gimp_context_unset_parent (global_tool_context);

      if (tool_info && (context = tool_info->context))
	{
	  gimp_context_copy_args (context, gimp_context_get_user (),
				  GIMP_CONTEXT_PAINT_ARGS_MASK);
	  gimp_context_set_parent (context, gimp_context_get_user ());
	}
    }
}
