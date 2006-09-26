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

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "gimpcolordialog.h"
#include "gimpdialogfactory.h"
#include "gimpfgbgeditor.h"
#include "gimptoolbox.h"
#include "gimptoolbox-color-area.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   color_area_color_clicked (GimpFgBgEditor       *editor,
                                        GimpActiveColor       active_color,
                                        GimpContext          *context);
static void   color_area_dialog_update (GimpColorDialog      *dialog,
                                        const GimpRGB        *color,
                                        GimpColorDialogState  state,
                                        GimpContext          *context);


/*  local variables  */

static GtkWidget       *color_area          = NULL;
static GtkWidget       *color_dialog        = NULL;
static gboolean         color_dialog_active = FALSE;
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
color_area_dialog_update (GimpColorDialog      *dialog,
                          const GimpRGB        *color,
                          GimpColorDialogState  state,
                          GimpContext          *context)
{
  switch (state)
    {
    case GIMP_COLOR_DIALOG_OK:
      gtk_widget_hide (color_dialog);
      color_dialog_active = FALSE;
      /* Fallthrough */

    case GIMP_COLOR_DIALOG_UPDATE:
      if (edit_color == GIMP_ACTIVE_COLOR_FOREGROUND)
        gimp_context_set_foreground (context, color);
      else
        gimp_context_set_background (context, color);
      break;

    case GIMP_COLOR_DIALOG_CANCEL:
      gtk_widget_hide (color_dialog);
      color_dialog_active = FALSE;
      gimp_context_set_foreground (context, &revert_fg);
      gimp_context_set_background (context, &revert_bg);
      break;
    }
}

static void
color_area_color_clicked (GimpFgBgEditor  *editor,
                          GimpActiveColor  active_color,
                          GimpContext     *context)
{
  GimpRGB      color;
  const gchar *title;

  if (! color_dialog_active)
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

  if (! color_dialog)
    {
      GimpDialogFactory *toplevel_factory;

      toplevel_factory = gimp_dialog_factory_from_name ("toplevel");

      color_dialog = gimp_color_dialog_new (NULL, context,
                                            NULL, NULL, NULL,
                                            GTK_WIDGET (editor),
                                            toplevel_factory,
                                            "gimp-toolbox-color-dialog",
                                            &color,
                                            TRUE, FALSE);

      g_signal_connect (color_dialog, "update",
                        G_CALLBACK (color_area_dialog_update),
                        context);
    }

  gtk_window_set_title (GTK_WINDOW (color_dialog), title);
  gimp_color_dialog_set_color (GIMP_COLOR_DIALOG (color_dialog), &color);

  gtk_window_present (GTK_WINDOW (color_dialog));
  color_dialog_active = TRUE;
}
