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

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "pdb/procedural_db.h"

#include "plug-in/plug-in-run.h"
#include "plug-in/plug-in-proc.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "actions.h"
#include "plug-in-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   plug_in_reset_all_callback (GtkWidget *widget,
                                          gboolean   reset_all,
                                          gpointer   data);


/*  public functions  */

void
plug_in_run_cmd_callback (GtkAction     *action,
                          PlugInProcDef *proc_def,
                          gpointer       data)
{
  Gimp          *gimp;
  ProcRecord    *proc_rec;
  Argument      *args;
  gint           gdisp_ID = -1;
  gint           i;
  gint           argc;
  GimpImageType  drawable_type = GIMP_RGB_IMAGE;

  gimp = action_data_get_gimp (data);
  if (! gimp)
    return;

  proc_rec = &proc_def->db_info;

  /* construct the procedures arguments */
  args = g_new0 (Argument, proc_rec->num_args);

  /* initialize the argument types */
  for (i = 0; i < proc_rec->num_args; i++)
    args[i].arg_type = proc_rec->args[i].arg_type;

  /* initialize the first argument  */
  args[0].value.pdb_int = GIMP_RUN_INTERACTIVE;
  argc = 1;

  switch (proc_rec->proc_type)
    {
    case GIMP_EXTENSION:
      break;

    case GIMP_PLUGIN:
    case GIMP_TEMPORARY:
      if (proc_rec->num_args >= 2 &&
          proc_rec->args[1].arg_type == GIMP_PDB_IMAGE)
        {
          GimpDisplay *gdisplay = action_data_get_display (data);

          if (gdisplay)
            {
              gdisp_ID = gimp_display_get_ID (gdisplay);

              args[1].value.pdb_int = gimp_image_get_ID (gdisplay->gimage);
              argc++;

              if (proc_rec->num_args >= 2 &&
                  proc_rec->args[2].arg_type == GIMP_PDB_DRAWABLE)
                {
                  GimpDrawable *drawable;

                  drawable = gimp_image_active_drawable (gdisplay->gimage);

                  if (drawable)
                    {
                      drawable_type = gimp_drawable_type (drawable);

                      args[2].value.pdb_int =
                        gimp_item_get_ID (GIMP_ITEM (drawable));
                      argc++;
                    }
                  else
                    {
                      g_warning ("Uh-oh, no active drawable for the plug-in!");
                      g_free (args);
                      return;
                    }
                }
            }
	}
      break;

    default:
      g_error ("Unknown procedure type.");
      g_free (args);
      return;
    }

  /* run the plug-in procedure */
  plug_in_run (gimp, gimp_get_user_context (gimp),
               proc_rec, args, argc, FALSE, TRUE, gdisp_ID);

  /* remember only "standard" plug-ins */
  if (proc_rec->proc_type == GIMP_PLUGIN           &&
      proc_rec->num_args >= 2                      &&
      proc_rec->args[1].arg_type == GIMP_PDB_IMAGE &&
      proc_rec->args[2].arg_type == GIMP_PDB_DRAWABLE)
    {
      gimp->last_plug_in = proc_rec;
#if 0
      FIXME
      plug_in_menus_update (GIMP_ITEM_FACTORY (item_factory), drawable_type);
#endif
    }

  g_free (args);
}

void
plug_in_repeat_cmd_callback (GtkAction *action,
                             gint       value,
                             gpointer   data)
{
  GimpDisplay  *gdisp;
  GimpDrawable *drawable;
  gboolean      interactive;

  gdisp = action_data_get_display (data);
  if (! gdisp)
    return;

  drawable = gimp_image_active_drawable (gdisp->gimage);
  if (! drawable)
    return;

  interactive = value ? TRUE : FALSE;

  plug_in_repeat (gdisp->gimage->gimp,
                  gimp_get_user_context (gdisp->gimage->gimp),
                  gimp_display_get_ID (gdisp),
                  gimp_image_get_ID (gdisp->gimage),
                  gimp_item_get_ID (GIMP_ITEM (drawable)),
                  interactive);
}

void
plug_in_reset_all_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  Gimp      *gimp;
  GtkWidget *qbox;

  gimp = action_data_get_gimp (data);
  if (! gimp)
    return;

  qbox = gimp_query_boolean_box (_("Reset all Filters"),
                                 NULL,
                                 gimp_standard_help_func,
                                 GIMP_HELP_FILTER_RESET_ALL,
                                 GTK_STOCK_DIALOG_QUESTION,
                                 _("Do you really want to reset all "
                                   "filters to default values?"),
                                 GIMP_STOCK_RESET, GTK_STOCK_CANCEL,
                                 NULL, NULL,
                                 plug_in_reset_all_callback,
                                 gimp);
  gtk_widget_show (qbox);
}


/*  private functions  */

static void
plug_in_reset_all_callback (GtkWidget *widget,
                            gboolean   reset_all,
                            gpointer   data)
{
  Gimp *gimp = GIMP (data);

  if (reset_all)
    procedural_db_free_data (gimp);
}
