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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "appenv.h"
#include "cursorutil.h"
#include "errors.h"
#include "general.h"

#include "dialog_handler.h"


static GSList * active_dialogs = NULL; /* List of dialogs that have 
					  been created and are on 
					  screen (may be hidden already).
				       */
static gint doing_update = FALSE;  /* Prevent multiple keypresses 
				      from unsetting me.
				   */

extern GtkWidget * fileload;   /* It's in fileops.c       */

/* State of individual dialogs */

typedef struct _dialog_state DIALOGSTATE,*DIALOGSTATEP;

typedef enum {
  WAS_HIDDEN,
  WAS_SHOWING,
  UNKNOWN,
} dialogstate;

struct _dialog_state {
  GtkWidget *d;
  dialogstate state;
};

/* This keeps track of the state the dialogs are in */
/* ie how many times we have pressed the tab key */

typedef enum{
  SHOW_ALL,
  HIDE_ALL,
  SHOW_TOOLBOX,
  LAST_SHOW_STATE,
} ShowState;

static ShowState dialogs_showing = SHOW_ALL; /* Start off with all 
						dialogs showing 
					     */

static DIALOGSTATEP toolbox_shell = NULL; /* Copy of the shell for the tool 
					      box - this has special behaviour 
					      so is not on the normal list.
					   */
/* Private */

/* Hide all currently registered dialogs */

static void
dialog_hide_all()
{
  GSList *list = active_dialogs;
  DIALOGSTATEP dstate;

  while (list)
    {
      dstate = (DIALOGSTATEP) list->data;
      list = g_slist_next (list);
  
      if(GTK_WIDGET_VISIBLE (dstate->d))
	{
	  dstate->state = WAS_SHOWING;
	  gtk_widget_hide(dstate->d);
	}
      else
	{
	  dstate->state = WAS_HIDDEN;
	}
    }
}

/* Show all currently registered dialogs */

static void
dialog_show_all()
{
  GSList * list = active_dialogs;
  DIALOGSTATEP dstate;

  while (list)
    {
      dstate = (DIALOGSTATEP) list->data;
      list = g_slist_next (list);

      if(dstate->state == WAS_SHOWING && !GTK_WIDGET_VISIBLE (dstate->d))
	gtk_widget_show(dstate->d);
    }
}

/* Handle the tool box in a special way */


static void
dialog_hide_toolbox()
{
  if(toolbox_shell && GTK_WIDGET_VISIBLE (toolbox_shell->d))
    {
      gtk_widget_hide(toolbox_shell->d);
      toolbox_shell->state = WAS_SHOWING;
    }
}

/* public */

void
dialog_show_toolbox()
{
  if(toolbox_shell && 
     toolbox_shell->state == WAS_SHOWING && 
     !GTK_WIDGET_VISIBLE (toolbox_shell->d))
    {
      gtk_widget_show(toolbox_shell->d);
    }
}


/* Set hourglass cursor on all currently registered dialogs */

void
dialog_idle_all()
{
  GSList *list = active_dialogs;
  DIALOGSTATEP dstate;

  while (list)
    {
      dstate = (DIALOGSTATEP) list->data;
      list = g_slist_next (list);
  
      if(GTK_WIDGET_VISIBLE (dstate->d))
	{
	  change_win_cursor (dstate->d->window, GDK_WATCH);
	}
    }

  if (toolbox_shell && GTK_WIDGET_VISIBLE(toolbox_shell->d))
    {
      change_win_cursor (toolbox_shell->d->window, GDK_WATCH);
    }

  if (fileload && GTK_WIDGET_VISIBLE (fileload))
    {
      change_win_cursor (fileload->window, GDK_WATCH);
    }
}

/* And remove the hourglass again. */

void
dialog_unidle_all()
{
  GSList *list = active_dialogs;
  DIALOGSTATEP dstate;

  while (list)
    {
      dstate = (DIALOGSTATEP) list->data;
      list = g_slist_next (list);
  
      if(GTK_WIDGET_VISIBLE (dstate->d))
	{
	  unset_win_cursor (dstate->d->window);
	}
    }

  if (toolbox_shell && GTK_WIDGET_VISIBLE(toolbox_shell->d))
    {
      unset_win_cursor (toolbox_shell->d->window);
    }

  if (fileload && GTK_WIDGET_VISIBLE (fileload))
    {
      unset_win_cursor (fileload->window);
    }
}

/* Register a dialog that we can handle */

void
dialog_register(GtkWidget *dialog)
{
  DIALOGSTATEP dstatep = g_new(DIALOGSTATE,1);
  dstatep->d = dialog;
  dstatep->state = UNKNOWN;
  active_dialogs = g_slist_append (active_dialogs,dstatep);
}

void
dialog_register_toolbox(GtkWidget *dialog)
{
  DIALOGSTATEP dstatep = g_new(DIALOGSTATE,1);
  dstatep->d = dialog;
  dstatep->state = UNKNOWN;
  toolbox_shell = dstatep;
}

/* unregister dialog */

void
dialog_unregister(GtkWidget *dialog)
{
  GSList * list = active_dialogs;
  DIALOGSTATEP dstate = NULL;

  while (list)
    {
      dstate = (DIALOGSTATEP) list->data;
      list = g_slist_next (list);
      
      if(dstate->d == dialog)
	break;
    }
  
  if(dstate != NULL)
    active_dialogs = g_slist_remove (active_dialogs,dstate);
}

/* Toggle showing of dialogs */
/* States:-
 * SHOW_ALL -> HIDE_ALL -> SHOW_TOOLBOX -> SHOW_ALL ....
 */

void 
dialog_toggle(void)
{
  if(doing_update == FALSE)
    doing_update = TRUE;
  else
    return;

  switch(dialogs_showing)
    {
    case SHOW_ALL:
      dialogs_showing = HIDE_ALL;
      dialog_hide_all();
      dialog_hide_toolbox();
      break;
    case HIDE_ALL:
      dialogs_showing = SHOW_TOOLBOX;
      dialog_show_toolbox();
      break;
    case SHOW_TOOLBOX:
      dialogs_showing = SHOW_ALL;
      dialog_show_all();
    default:
      break;
    }

  gdk_flush();
  while (gtk_events_pending())
    {
      gtk_main_iteration();
      gdk_flush();
    }

  doing_update = FALSE;
}

