/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include "libgimpbase/gimplimits.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-types.h"
#include "config/gimpconfig-utils.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpstrokeoptions.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpstrokeeditor.h"

#include "stroke-dialog.h"

#include "gimp-intl.h"


#define STROKE_COLOR_SIZE 20


/*  local functions  */


static void reset_callback  (GtkWidget  *widget,
                             GtkWidget  *dialog);
static void cancel_callback (GtkWidget  *widget,
                             GtkWidget  *dialog);
static void ok_callback     (GtkWidget  *widget,
                             GtkWidget  *dialog);


/*  public function  */


GtkWidget *
stroke_dialog_new (GimpDrawable      *drawable,
                   GimpItem          *item,
                   GimpStrokeOptions *stroke_options)
{
  GtkWidget *dialog;
  GtkWidget *stroke_editor;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (!stroke_options)
    stroke_options = g_object_new (GIMP_TYPE_STROKE_OPTIONS,
                                   "gimp", GIMP_IMAGE (GIMP_ITEM (drawable)->gimage)->gimp,
                                   NULL);

  /* the dialog */
  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (GIMP_ITEM (drawable)->gimage),
                                     _("Stroke Options"), "configure_grid",
                                     GIMP_STOCK_GRID, _("Configure Stroke Appearance"),
                                     gimp_standard_help_func,
                                     GIMP_HELP_IMAGE_GRID,

                                     GIMP_STOCK_RESET, reset_callback,
                                     NULL, NULL, NULL, FALSE, FALSE,

                                     GTK_STOCK_CANCEL, cancel_callback,
                                     NULL, NULL, NULL, FALSE, TRUE,

                                     GTK_STOCK_OK, ok_callback,
                                     NULL, NULL, NULL, TRUE, FALSE,

                                     NULL);

  g_object_set_data (G_OBJECT (dialog), "drawable", drawable);
  g_object_set_data (G_OBJECT (dialog), "item", item);
  g_object_set_data (G_OBJECT (dialog), "stroke_options", stroke_options);

  stroke_editor = gimp_stroke_editor_new (stroke_options);

  gtk_widget_show (stroke_editor);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
                     stroke_editor);
  
  gtk_widget_show (stroke_editor);

  return dialog;
}


/*  local functions  */

static void
reset_callback (GtkWidget  *widget,
                GtkWidget  *dialog)
{
  /* something should reset the values here */
}


static void
cancel_callback (GtkWidget  *widget,
                 GtkWidget  *dialog)
{
  GimpStrokeOptions *stroke_options;

  stroke_options = g_object_get_data (G_OBJECT (dialog), "stroke_options");

  g_object_unref (stroke_options);
  gtk_widget_destroy (dialog);
}


static void
ok_callback (GtkWidget  *widget,
             GtkWidget  *dialog)
{
  GimpDrawable *drawable;
  GimpItem *item;
  GimpStrokeOptions *stroke_options;

  item = g_object_get_data (G_OBJECT (dialog), "item");
  drawable = g_object_get_data (G_OBJECT (dialog), "drawable");
  stroke_options = g_object_get_data (G_OBJECT (dialog), "stroke_options");

  gimp_item_stroke (item, drawable, GIMP_OBJECT (stroke_options));

  gimp_image_flush (GIMP_ITEM (drawable)->gimage);

  g_object_unref (stroke_options);
  gtk_widget_destroy (dialog);
}
