/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Andy Thomas (alt@picnic.demon.co.uk)
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
#include <gtk/gtk.h>

#include "cursorutil.h"
#include "dialog_handler.h"

/*  State of individual dialogs  */

typedef enum
{
  INVISIBLE,
  VISIBLE,
  UNKNOWN,
} VisibilityState;

typedef struct _DialogState DialogState;

struct _DialogState
{
  GtkWidget       *dialog;
  VisibilityState  saved_state;
};

/*  This keeps track of the state the dialogs are in  */
/*  ie how many times we have pressed the tab key     */

typedef enum
{
  SHOW_ALL,
  HIDE_ALL,
  SHOW_TOOLBOX,
  LAST_SHOW_STATE,
} ShowState;

/*  Start off with all dialogs showing  */
static ShowState dialogs_showing = SHOW_ALL;

/*  Prevent multiple keypresses from unsetting me.  */
static gboolean doing_update = FALSE;

/*  List of dialogs that have been created and are on screen
 *  (may be hidden already).
 */
static GSList * active_dialogs = NULL;

/*  Those have a special behaviour  */
static DialogState * toolbox_shell  = NULL;
static DialogState * fileload_shell = NULL;

/* Private */

/*  Hide all currently registered dialogs  */

static void
dialog_hide_all (void)
{
  DialogState *dstate;
  GSList *list;

  for (list = active_dialogs; list; list = g_slist_next (list))
    {
      dstate = (DialogState *) list->data;
  
      if (GTK_WIDGET_VISIBLE (dstate->dialog))
	{
	  dstate->saved_state = VISIBLE;
	  gtk_widget_hide (dstate->dialog);
	}
      else
	{
	  dstate->saved_state = INVISIBLE;
	}
    }
}

/*  Show all currently registered dialogs  */

static void
dialog_show_all (void)
{
  DialogState *dstate;
  GSList *list;

  for (list = active_dialogs; list; list = g_slist_next (list))
    {
      dstate = (DialogState *) list->data;

      if (dstate->saved_state == VISIBLE && !GTK_WIDGET_VISIBLE (dstate->dialog))
	gtk_widget_show(dstate->dialog);
    }
}

/*  Handle the tool box in a special way  */


static void
dialog_hide_toolbox (void)
{
  if (toolbox_shell && GTK_WIDGET_VISIBLE (toolbox_shell->dialog))
    {
      gtk_widget_hide (toolbox_shell->dialog);
      toolbox_shell->saved_state = VISIBLE;
    }
}

/*  public  */

void
dialog_show_toolbox (void)
{
  if (toolbox_shell && 
      toolbox_shell->saved_state == VISIBLE && 
      !GTK_WIDGET_VISIBLE (toolbox_shell->dialog))
    {
      gtk_widget_show (toolbox_shell->dialog);
    }
}


/*  Set hourglass cursor on all currently registered dialogs  */

void
dialog_idle_all (void)
{
  DialogState *dstate;
  GSList *list;

  for (list = active_dialogs; list; list = g_slist_next (list))
    {
      dstate = (DialogState *) list->data;
  
      if(GTK_WIDGET_VISIBLE (dstate->dialog))
	{
	  change_win_cursor (dstate->dialog->window, GDK_WATCH);
	}
    }

  if (toolbox_shell && GTK_WIDGET_VISIBLE (toolbox_shell->dialog))
    {
      change_win_cursor (toolbox_shell->dialog->window, GDK_WATCH);
    }

  if (fileload_shell && GTK_WIDGET_VISIBLE (fileload_shell->dialog))
    {
      change_win_cursor (fileload_shell->dialog->window, GDK_WATCH);
    }
}

/*  And remove the hourglass again.  */

void
dialog_unidle_all (void)
{
  DialogState *dstate;
  GSList *list;

  for (list = active_dialogs; list; list = g_slist_next (list))
    {
      dstate = (DialogState *) list->data;

      if (GTK_WIDGET_VISIBLE (dstate->dialog))
	{
	  unset_win_cursor (dstate->dialog->window);
	}
    }

  if (toolbox_shell && GTK_WIDGET_VISIBLE (toolbox_shell->dialog))
    {
      unset_win_cursor (toolbox_shell->dialog->window);
    }

  if (fileload_shell && GTK_WIDGET_VISIBLE (fileload_shell->dialog))
    {
      unset_win_cursor (fileload_shell->dialog->window);
    }
}

/*  Register a dialog that we can handle  */

void
dialog_register (GtkWidget *dialog)
{
  DialogState *dstate;

  dstate = g_new (DialogState, 1);

  dstate->dialog      = dialog;
  dstate->saved_state = UNKNOWN;

  active_dialogs = g_slist_append (active_dialogs, dstate);
}

void
dialog_register_toolbox (GtkWidget *dialog)
{
  toolbox_shell = g_new (DialogState, 1);

  toolbox_shell->dialog      = dialog;
  toolbox_shell->saved_state = UNKNOWN;
}

void
dialog_register_fileload (GtkWidget *dialog)
{
  fileload_shell = g_new (DialogState, 1);

  fileload_shell->dialog      = dialog;
  fileload_shell->saved_state = UNKNOWN;
}

/*  unregister dialog  */

void
dialog_unregister (GtkWidget *dialog)
{
  DialogState *dstate = NULL;
  GSList *list;

  for (list = active_dialogs; list; list = g_slist_next (list))
    {
      dstate = (DialogState *) list->data;

      if (dstate->dialog == dialog)
	break;
    }

  if (dstate != NULL)
    {
      active_dialogs = g_slist_remove (active_dialogs, dstate);
      g_free (dstate);
    }
}

/*  Toggle showing of dialogs
 *
 *  States:-
 *  SHOW_ALL -> HIDE_ALL -> SHOW_TOOLBOX -> SHOW_ALL ....
 */

void 
dialog_toggle (void)
{
  if(doing_update == FALSE)
    doing_update = TRUE;
  else
    return;

  switch (dialogs_showing)
    {
    case SHOW_ALL:
      dialogs_showing = HIDE_ALL;
      dialog_hide_all ();
      dialog_hide_toolbox ();
      break;
    case HIDE_ALL:
      dialogs_showing = SHOW_TOOLBOX;
      dialog_show_toolbox ();
      break;
    case SHOW_TOOLBOX:
      dialogs_showing = SHOW_ALL;
      dialog_show_all ();
    default:
      break;
    }

  gdk_flush ();
  while (gtk_events_pending ())
    {
      gtk_main_iteration ();
      gdk_flush ();
    }

  doing_update = FALSE;
}

