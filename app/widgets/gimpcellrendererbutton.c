/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcellrendererbutton.c
 * Copyright (C) 2016 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpmarshal.h"

#include "gimpcellrendererbutton.h"


enum
{
  CLICKED,
  LAST_SIGNAL
};


static gboolean gimp_cell_renderer_button_activate (GtkCellRenderer     *cell,
                                                    GdkEvent            *event,
                                                    GtkWidget           *widget,
                                                    const gchar         *path,
                                                    const GdkRectangle  *background_area,
                                                    const GdkRectangle  *cell_area,
                                                    GtkCellRendererState flags);


G_DEFINE_TYPE (GimpCellRendererButton, gimp_cell_renderer_button,
               GTK_TYPE_CELL_RENDERER_PIXBUF)

#define parent_class gimp_cell_renderer_button_parent_class

static guint button_cell_signals[LAST_SIGNAL] = { 0 };


static void
gimp_cell_renderer_button_class_init (GimpCellRendererButtonClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  /**
   * GimpCellRendererButton::clicked:
   * @cell:
   * @path:
   * @state:
   *
   * Called on a button cell when it is clicked.
   **/
  button_cell_signals[CLICKED] =
    g_signal_new ("clicked",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpCellRendererButtonClass, clicked),
                  NULL, NULL,
                  gimp_marshal_VOID__STRING_FLAGS,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  GDK_TYPE_MODIFIER_TYPE);

  cell_class->activate = gimp_cell_renderer_button_activate;

  klass->clicked       = NULL;
}

static void
gimp_cell_renderer_button_init (GimpCellRendererButton *cell_button)
{
  g_object_set (cell_button,
                "mode",       GTK_CELL_RENDERER_MODE_ACTIVATABLE,
                "xpad",       2,
                "ypad",       2,
                "stock-size", GTK_ICON_SIZE_BUTTON,
                NULL);
}

static gboolean
gimp_cell_renderer_button_activate (GtkCellRenderer      *cell,
                                    GdkEvent             *event,
                                    GtkWidget            *widget,
                                    const gchar          *path,
                                    const GdkRectangle   *background_area,
                                    const GdkRectangle   *cell_area,
                                    GtkCellRendererState  flags)
{
  GimpCellRendererButton *cell_button = GIMP_CELL_RENDERER_BUTTON (cell);
  GdkModifierType         state       = 0;

  if (event && ((GdkEventAny *) event)->type == GDK_BUTTON_PRESS)
    state = ((GdkEventButton *) event)->state;

  if (! event ||
      (((GdkEventAny *) event)->type == GDK_BUTTON_PRESS &&
       ((GdkEventButton *) event)->button == 1))
    {
      gimp_cell_renderer_button_clicked (cell_button, path, state);

      return TRUE;
    }

  return FALSE;
}


/*  public functions  */

GtkCellRenderer *
gimp_cell_renderer_button_new (void)
{
  return g_object_new (GIMP_TYPE_CELL_RENDERER_BUTTON, NULL);
}

void
gimp_cell_renderer_button_clicked (GimpCellRendererButton *cell,
                                   const gchar            *path,
                                   GdkModifierType         state)
{
  g_return_if_fail (GIMP_IS_CELL_RENDERER_BUTTON (cell));
  g_return_if_fail (path != NULL);

  g_signal_emit (cell, button_cell_signals[CLICKED], 0, path, state);
}
