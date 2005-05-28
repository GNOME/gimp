/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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
 *
 */

#include "config.h"

#include <gtk/gtk.h>

#include "imap_about.h"

#include "libgimp/stdplugins-intl.h"

void
do_about_dialog(void)
{
   static GtkWidget *dialog;
   if (!dialog)
     {
       const gchar* authors[] = {"Maurits Rijk (m.rijk@chello.nl)", NULL};

       dialog = gtk_about_dialog_new ();
       gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (dialog), "Imagemap plug-in");
       gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (dialog), "2.3");
       gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (dialog),
                                       _("Copyright(c) 1999-2005 by Maurits Rijk"));
       gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (dialog), authors);
       gtk_about_dialog_set_license (GTK_ABOUT_DIALOG (dialog), 
                                     _("Released under the GNU General Public License"));

       g_signal_connect (dialog, "destroy",
                         G_CALLBACK (gtk_widget_destroyed),
                         &dialog);

     }
   gtk_widget_show (dialog);
}
