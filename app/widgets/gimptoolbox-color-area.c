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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#ifdef __GNUC__
#warning FIXME #include "gui/gui-types.h"
#endif
#include "gui/gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "gimpdialogfactory.h"
#include "gimpfgbgeditor.h"
#include "gimptoolbox.h"
#include "gimptoolbox-color-area.h"

#include "gui/color-notebook.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void     color_area_color_clicked   (GimpFgBgEditor     *editor,
                                            GimpActiveColor     active_color,
                                            GimpContext        *context);
static void     color_area_select_callback (ColorNotebook      *color_notebook,
                                            const GimpRGB      *color,
                                            ColorNotebookState  state,
                                            gpointer            data);


/*  local variables  */

static GtkWidget       *color_area            = NULL;
static ColorNotebook   *color_notebook        = NULL;
static gboolean         color_notebook_active = FALSE;
static GimpActiveColor  edit_color;
static GimpRGB          revert_fg;
static GimpRGB          revert_bg;


/*  public functions  */

GtkWidget *
gimp_toolbox_color_area_create (GimpToolbox *toolbox,
                                gint         width,
                                gint         height)
{
  GimpContext *context;

  g_return_val_if_fail (GIMP_IS_TOOLBOX (toolbox), NULL);

  context = GIMP_DOCK (toolbox)->context;

  color_area = gimp_fg_bg_editor_new (context);
  gtk_widget_set_size_request (color_area, width, height);
  gtk_widget_add_events (color_area,
                         GDK_ENTER_NOTIFY_MASK |
			 GDK_LEAVE_NOTIFY_MASK);

  g_signal_connect (color_area, "color-clicked",
		    G_CALLBACK (color_area_color_clicked),
		    context);

  return color_area;
}


/*  private functions  */

static void
color_area_select_callback (ColorNotebook      *color_notebook,
			    const GimpRGB      *color,
			    ColorNotebookState  state,
			    gpointer            data)
{
  GimpContext *context = GIMP_CONTEXT (data);

  if (color_notebook)
    {
      switch (state)
	{
	case COLOR_NOTEBOOK_OK:
	  color_notebook_hide (color_notebook);
	  color_notebook_active = FALSE;
	  /* Fallthrough */

	case COLOR_NOTEBOOK_UPDATE:
	  if (edit_color == GIMP_ACTIVE_COLOR_FOREGROUND)
	    gimp_context_set_foreground (context, color);
	  else
	    gimp_context_set_background (context, color);
	  break;

	case COLOR_NOTEBOOK_CANCEL:
	  color_notebook_hide (color_notebook);
	  color_notebook_active = FALSE;
	  gimp_context_set_foreground (context, &revert_fg);
	  gimp_context_set_background (context, &revert_bg);
          break;
	}
    }
}

static void
color_area_color_clicked (GimpFgBgEditor  *editor,
                          GimpActiveColor  active_color,
                          GimpContext     *context)
{
  GimpRGB      color;
  const gchar *title;

  if (! color_notebook_active)
    {
      gimp_context_get_foreground (context, &revert_fg);
      gimp_context_get_background (context, &revert_bg);
    }

  if (active_color == GIMP_ACTIVE_COLOR_FOREGROUND)
    {
      gimp_context_get_foreground (context, &color);
      title = _("Change Foreground Color");
    }
  else
    {
      gimp_context_get_background (context, &color);
      title = _("Change Background Color");
    }

  edit_color = active_color;

  if (! color_notebook)
    {
      GimpDialogFactory *toplevel_factory;

      toplevel_factory = gimp_dialog_factory_from_name ("toplevel");

      color_notebook = color_notebook_new (NULL, title, NULL, NULL,
                                           GTK_WIDGET (editor),
                                           toplevel_factory,
                                           "gimp-toolbox-color-dialog",
					   (const GimpRGB *) &color,
					   color_area_select_callback,
					   context, TRUE, FALSE);
      color_notebook_active = TRUE;
    }
  else
    {
      color_notebook_set_title (color_notebook, title);
      color_notebook_set_color (color_notebook, &color);

      color_notebook_show (color_notebook);
      color_notebook_active = TRUE;
    }
}
