/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#include "core/gimp.h"

#include "display/gimpdisplay-foreach.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "quit-dialog.h"

#include "gimp-intl.h"


static void  quit_callback (GtkWidget *button,
                            gboolean   quit,
                            gpointer   data);


GtkWidget *
quit_dialog_new (Gimp *gimp)
{
  GtkWidget *dialog;
  GList     *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  for (list = gimp_action_groups_from_name ("file");
       list;
       list = g_list_next (list))
    {
      gimp_action_group_set_action_sensitive (list->data, "file-quit", FALSE);
    }

  dialog = gimp_query_boolean_box (_("Quit The GIMP?"),
                                   NULL,
                                   gimp_standard_help_func,
                                   GIMP_HELP_FILE_QUIT_CONFIRM,
                                   GIMP_STOCK_WILBER_EEK,
                                   _("Some images have unsaved changes.\n\n"
                                     "Really quit The GIMP?"),
                                   GTK_STOCK_QUIT, GTK_STOCK_CANCEL,
                                   NULL, NULL,
                                   quit_callback,
                                   gimp);

  return dialog;
}

static void
quit_callback (GtkWidget *button,
               gboolean   quit,
               gpointer   data)
{
  Gimp *gimp = GIMP (data);

  if (quit)
    {
      gimp_exit (gimp, TRUE);
    }
  else
    {
      GList *list;

      for (list = gimp_action_groups_from_name ("file");
           list;
           list = g_list_next (list))
        {
          gimp_action_group_set_action_sensitive (list->data, "file-quit",
                                                  TRUE);
        }
    }
}
