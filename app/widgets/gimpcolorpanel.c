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

#include <stdio.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gui/color-notebook.h"

#include "gimpcolorpanel.h"


struct _GimpColorPanel
{
  GimpColorButton  parent;

  ColorNotebook   *color_notebook;
  gboolean         color_notebook_active;
};


/*  local function prototypes  */
static void   gimp_color_panel_class_init      (GimpColorPanelClass *klass);
static void   gimp_color_panel_init            (GimpColorPanel      *panel);
static void   gimp_color_panel_destroy         (GtkObject           *object);
static void   gimp_color_panel_color_changed   (GimpColorButton     *button);
static void   gimp_color_panel_clicked         (GtkButton           *button);

static void   gimp_color_panel_select_callback (ColorNotebook       *notebook,
						const GimpRGB       *color,
						ColorNotebookState   state,
						gpointer             data);


static GimpColorButtonClass *parent_class = NULL;


GtkType
gimp_color_panel_get_type (void)
{
  static guint panel_type = 0;

  if (!panel_type)
    {
      GtkTypeInfo panel_info =
      {
	"GimpColorPanel",
	sizeof (GimpColorPanel),
	sizeof (GimpColorPanelClass),
	(GtkClassInitFunc) gimp_color_panel_class_init,
	(GtkObjectInitFunc) gimp_color_panel_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      panel_type = gtk_type_unique (GIMP_TYPE_COLOR_BUTTON, &panel_info);
    }
  
  return panel_type;
}

static void
gimp_color_panel_class_init (GimpColorPanelClass *klass)
{
  GtkObjectClass       *object_class;
  GtkButtonClass       *button_class;
  GimpColorButtonClass *color_button_class;

  object_class       = (GtkObjectClass *) klass;
  button_class       = (GtkButtonClass *) klass;
  color_button_class = (GimpColorButtonClass *) klass;
  
  parent_class = gtk_type_class (GIMP_TYPE_COLOR_BUTTON);

  object_class->destroy             = gimp_color_panel_destroy;
  button_class->clicked             = gimp_color_panel_clicked;
  color_button_class->color_changed = gimp_color_panel_color_changed;
}

static void
gimp_color_panel_init (GimpColorPanel *panel)
{
  panel->color_notebook        = NULL;
  panel->color_notebook_active = FALSE;
}

static void
gimp_color_panel_destroy (GtkObject *object)
{
  GimpColorPanel *panel;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_COLOR_PANEL (object));
  
  panel = GIMP_COLOR_PANEL (object);

  if (panel->color_notebook)
    {
      color_notebook_hide (panel->color_notebook);
      color_notebook_free (panel->color_notebook);
      panel->color_notebook = NULL;
    }
  
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_color_panel_new (const gchar       *title,
		      const GimpRGB     *color,
		      GimpColorAreaType  type,
		      gint               width,
		      gint               height)
{
  GimpColorPanel *panel;

  g_return_val_if_fail (color != NULL, NULL);

  panel = gtk_type_new (GIMP_TYPE_COLOR_PANEL);

  GIMP_COLOR_BUTTON (panel)->title = g_strdup (title);

  gimp_color_button_set_type (GIMP_COLOR_BUTTON (panel), type);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (panel), color);
  gtk_widget_set_usize (GTK_WIDGET (panel), width, height);

  return GTK_WIDGET (panel);
}

static void
gimp_color_panel_color_changed (GimpColorButton *button)
{
  GimpColorPanel *panel;
  GimpRGB         color;

  panel = GIMP_COLOR_PANEL (button);

  if (panel->color_notebook_active)
    {
      gimp_color_button_get_color (GIMP_COLOR_BUTTON (button), &color);
      color_notebook_set_color (panel->color_notebook, &color);
    }
}

static void
gimp_color_panel_clicked (GtkButton *button)
{
  GimpColorPanel *panel;
  GimpRGB         color;

  panel = GIMP_COLOR_PANEL (button);

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (button), &color);

  if (! panel->color_notebook)
    {
      panel->color_notebook =
	color_notebook_new (GIMP_COLOR_BUTTON (button)->title,
			    (const GimpRGB *) &color,
			    gimp_color_panel_select_callback,
			    panel,
			    FALSE,
			    gimp_color_button_has_alpha (GIMP_COLOR_BUTTON (button)));
      panel->color_notebook_active = TRUE;
    }
  else
    {
      if (! panel->color_notebook_active)
	{
	  color_notebook_show (panel->color_notebook);
	  panel->color_notebook_active = TRUE;
	}
      color_notebook_set_color (panel->color_notebook, &color);
    }
}

static void
gimp_color_panel_select_callback (ColorNotebook      *notebook,
				  const GimpRGB      *color,
				  ColorNotebookState  state,
				  gpointer            data)
{
  GimpColorPanel *panel;

  panel = GIMP_COLOR_PANEL (data);

  if (panel->color_notebook)
    {
      switch (state)
	{
	case COLOR_NOTEBOOK_UPDATE:
	  break;
	case COLOR_NOTEBOOK_OK:
	  gimp_color_button_set_color (GIMP_COLOR_BUTTON (panel), color);
	  /* Fallthrough */
	case COLOR_NOTEBOOK_CANCEL:
	  color_notebook_hide (panel->color_notebook);
	  panel->color_notebook_active = FALSE;
	}
    }
}
