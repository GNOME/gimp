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

#include "context_manager.h"

#include "appenv.h"
#include "gdisplay.h"
#include "gimprc.h"
#include "paint_options.h"
#include "tools.h"

static GimpContext * global_user_context;


static void
context_manager_display_changed (GimpContext *context,
				 GDisplay    *display,
				 gpointer     data)
{
  gdisplay_set_menu_sensitivity (display);
}

/* FIXME: finally, install callbacks for all created contexts to prevent
 *        the image from appearing without notifying us
 */
static void
context_manager_image_removed (GimpSet     *set,
			       GimpImage   *gimage,
			       GimpContext *user_context)
{
  if (gimp_context_get_image (user_context) == gimage)
    gimp_context_set_image (user_context, NULL);
}

void
context_manager_init (void)
{
  GimpContext *context;
  gint i;

  /*  Implicitly create the standard context  */
  context = gimp_context_get_standard ();

  /*  TODO: load from disk  */
  context = gimp_context_new ("Default", NULL, NULL);
  gimp_context_set_default (context);

  /*  Finally the user context will be initialized with the default context's
   *  values.
   */
  context = gimp_context_new ("User", NULL, NULL);
  gimp_context_set_user (context);
  gimp_context_set_current (context);

  global_user_context = gimp_context_new ("Don't use :)", NULL, context);

  gtk_signal_connect (GTK_OBJECT (context), "display_changed",
		      GTK_SIGNAL_FUNC (context_manager_display_changed),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (image_context), "remove",
		      GTK_SIGNAL_FUNC (context_manager_image_removed),
		      context);

  /*  Initialize the tools' contexts  */
  for (i = 0; i < num_tools; i++)
    {
      switch (tool_info[i].tool_id)
	{
	case BUCKET_FILL:
	case BLEND:
	case PENCIL:
	case PAINTBRUSH:
	case ERASER:
	case AIRBRUSH:
	case CLONE:
	case CONVOLVE:
	case INK:
	case DODGEBURN:
	case SMUDGE:
	case PIXMAPBRUSH:
	  tool_info[i].tool_context =
	    gimp_context_new (tool_info[i].private_tip, NULL, context);
	  break;

	default:
	  tool_info[i].tool_context = NULL;
	  break;
	}
    }
}

void
context_manager_free (void)
{
  gint i;

  for (i = 0; i < num_tools; i++)
    {
      if (tool_info[i].tool_context != NULL)
	{
	  gtk_object_unref (GTK_OBJECT (tool_info[i].tool_context));
	  tool_info[i].tool_context = NULL;
	}
    }

  gtk_object_unref (GTK_OBJECT (gimp_context_get_user ()));
  gimp_context_set_user (NULL);
  gimp_context_set_current (NULL);

  /*  TODO: Save to disk before destroying  */
  gtk_object_unref (GTK_OBJECT (gimp_context_get_default ()));
  gimp_context_set_default (NULL);
}

void
context_manager_set_global_paint_options (gboolean global)
{
  GimpContext* context;

  paint_options_set_global (global);

  if (global)
    {
      if (active_tool &&
	  (context = tool_info[active_tool->type].tool_context))
	{
	  gimp_context_define_opacity (context, TRUE);
	  gimp_context_define_paint_mode (context, TRUE);
	}

      gimp_context_set_opacity (gimp_context_get_user (),
				gimp_context_get_opacity (global_user_context));
      gimp_context_set_paint_mode (gimp_context_get_user (),
				   gimp_context_get_paint_mode (global_user_context));

      gimp_context_define_opacity (global_user_context, FALSE);
      gimp_context_define_paint_mode (global_user_context, FALSE);
    }
  else
    {
      gimp_context_define_opacity (global_user_context, TRUE);
      gimp_context_define_paint_mode (global_user_context, TRUE);

      if (active_tool &&
	  (context = tool_info[active_tool->type].tool_context))
	{
	  gimp_context_set_opacity (gimp_context_get_user (),
				    gimp_context_get_opacity (context));
	  gimp_context_set_paint_mode (gimp_context_get_user (),
				       gimp_context_get_paint_mode (context));

	  gimp_context_define_opacity (context, FALSE);
	  gimp_context_define_paint_mode (context, FALSE);
	}
    }
}

void
context_manager_set_tool (ToolType tool_type)
{
  GimpContext* context;

  if (! global_paint_options)
    {
      if (active_tool &&
	  (context = tool_info[active_tool->type].tool_context))
	{
	  gimp_context_define_opacity (context, TRUE);
	  gimp_context_define_paint_mode (context, TRUE);
	}

      if ((context = tool_info[tool_type].tool_context))
	{
	  gimp_context_set_opacity (gimp_context_get_user (),
				    gimp_context_get_opacity (context));
	  gimp_context_set_paint_mode (gimp_context_get_user (),
				       gimp_context_get_paint_mode (context));

	  gimp_context_define_opacity (context, FALSE);
	  gimp_context_define_paint_mode (context, FALSE);
	}
    }
}
