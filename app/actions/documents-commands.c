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
#include "core/gimpimagefile.h"

#include "widgets/gimpcontainerlistview.h"
#include "widgets/gimpdocumentview.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "documents-commands.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void   documents_menu_set_sensitivity (GimpContainerEditor *editor);


/*  public functionss */

void
documents_open_document_cmd_callback (GtkWidget *widget,
                                      gpointer   data)
{
  GimpDocumentView *view;

  view = (GimpDocumentView *) gimp_widget_get_callback_context (widget);

  if (! view)
    return;

  gtk_button_clicked (GTK_BUTTON (view->open_button));
}

void
documents_raise_or_open_document_cmd_callback (GtkWidget *widget,
                                               gpointer   data)
{
  GimpDocumentView *view;

  view = (GimpDocumentView *) gimp_widget_get_callback_context (widget);

  if (! view)
    return;

  gimp_button_extended_clicked (GIMP_BUTTON (view->open_button),
                                GDK_SHIFT_MASK);
}

void
documents_file_open_dialog_cmd_callback (GtkWidget *widget,
                                         gpointer   data)
{
  GimpDocumentView *view;

  view = (GimpDocumentView *) gimp_widget_get_callback_context (widget);

  if (! view)
    return;

  gimp_button_extended_clicked (GIMP_BUTTON (view->open_button),
                                GDK_CONTROL_MASK);
}

void
documents_delete_document_cmd_callback (GtkWidget *widget,
                                        gpointer   data)
{
  GimpDocumentView *view;

  view = (GimpDocumentView *) gimp_widget_get_callback_context (widget);

  if (! view)
    return;

  gtk_button_clicked (GTK_BUTTON (view->delete_button));
}


void
documents_refresh_documents_cmd_callback (GtkWidget *widget,
                                          gpointer   data)
{
  GimpDocumentView *view;

  view = (GimpDocumentView *) gimp_widget_get_callback_context (widget);

  if (! view)
    return;

  gtk_button_clicked (GTK_BUTTON (view->refresh_button));
}

void
documents_show_context_menu (GimpContainerEditor *editor)
{
  GtkItemFactory *item_factory;

  documents_menu_set_sensitivity (editor);

  item_factory = gtk_item_factory_from_path ("<Documents>");

  gimp_item_factory_popup_with_data (item_factory, editor);
}


/*  private functions  */

static void
documents_menu_set_sensitivity (GimpContainerEditor *editor)
{
  GimpImagefile *imagefile;

  imagefile = gimp_context_get_imagefile (editor->view->context);

#define SET_SENSITIVE(menu,condition) \
        gimp_menu_item_set_sensitive ("<Documents>/" menu, (condition) != 0)

  SET_SENSITIVE ("Open Image",          imagefile);
  SET_SENSITIVE ("Raise or Open Image", imagefile);
  SET_SENSITIVE ("File Open Dialog...", TRUE);
  SET_SENSITIVE ("Remove Entry",        imagefile);
  SET_SENSITIVE ("Refresh History",     TRUE);

#undef SET_SENSITIVE
}
