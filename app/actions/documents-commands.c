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

#include "actions-types.h"

#include "widgets/gimpdocumentview.h"

#include "documents-commands.h"


/*  public functions */

void
documents_open_document_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  GimpDocumentView *view = GIMP_DOCUMENT_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->open_button));
}

void
documents_raise_or_open_document_cmd_callback (GtkAction *action,
                                               gpointer   data)
{
  GimpDocumentView *view = GIMP_DOCUMENT_VIEW (data);

  gimp_button_extended_clicked (GIMP_BUTTON (view->open_button),
                                GDK_SHIFT_MASK);
}

void
documents_file_open_dialog_cmd_callback (GtkAction *action,
                                         gpointer   data)
{
  GimpDocumentView *view = GIMP_DOCUMENT_VIEW (data);

  gimp_button_extended_clicked (GIMP_BUTTON (view->open_button),
                                GDK_CONTROL_MASK);
}

void
documents_remove_document_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpDocumentView *view = GIMP_DOCUMENT_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->remove_button));
}


void
documents_recreate_preview_cmd_callback (GtkAction *action,
                                         gpointer   data)
{
  GimpDocumentView *view = GIMP_DOCUMENT_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->refresh_button));
}

void
documents_reload_previews_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpDocumentView *view = GIMP_DOCUMENT_VIEW (data);

  gimp_button_extended_clicked (GIMP_BUTTON (view->refresh_button),
                                GDK_SHIFT_MASK);
}

void
documents_delete_dangling_documents_cmd_callback (GtkAction *action,
                                                  gpointer   data)
{
  GimpDocumentView *view = GIMP_DOCUMENT_VIEW (data);

  gimp_button_extended_clicked (GIMP_BUTTON (view->refresh_button),
                                GDK_CONTROL_MASK);
}
