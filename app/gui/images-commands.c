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

#include "gui-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpcontainerlistview.h"
#include "widgets/gimpimageview.h"
#include "widgets/gimpitemfactory.h"

#include "images-commands.h"

#include "libgimp/gimpintl.h"


/*  public functionss */

void
images_raise_displays_cmd_callback (GtkWidget *widget,
                                    gpointer   data)
{
  GimpImageView *view;

  view = GIMP_IMAGE_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->raise_button));
}

void
images_new_display_cmd_callback (GtkWidget *widget,
                                 gpointer   data)
{
  GimpImageView *view;

  view = GIMP_IMAGE_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->new_button));
}

void
images_delete_image_cmd_callback (GtkWidget *widget,
                                  gpointer   data)
{
  GimpImageView *view;

  view = GIMP_IMAGE_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->delete_button));
}

void
images_menu_update (GtkItemFactory *factory,
                    gpointer        data)
{
  GimpContainerEditor *editor;
  GimpImage           *image;

  editor = GIMP_CONTAINER_EDITOR (data);

  image = gimp_context_get_image (editor->view->context);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Raise Displays", image);
  SET_SENSITIVE ("/New Display",    image);
  SET_SENSITIVE ("/Delete Image",   image && image->disp_count == 0);

#undef SET_SENSITIVE
}
