/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color_notebook module (C) 1998 Austin Donnelly <austin@greenend.org.uk>
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
#define __COLOR_NOTEBOOK_C__ 1

#include "config.h"

#include <gmodule.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "appenv.h"
#include "color_notebook.h"
#include "gimpui.h"

#include "libgimp/color_selector.h"
#include "libgimp/gimpintl.h"

static void color_notebook_ok_callback     (GtkWidget *, gpointer);
static void color_notebook_cancel_callback (GtkWidget *, gpointer);
static void color_notebook_update_callback (void *, int, int, int);
static void color_notebook_page_switch     (GtkWidget *, GtkNotebookPage *,
					    guint);
static void color_notebook_help_func       (gpointer data);

/* information we keep on each registered colour selector */
typedef struct _ColorSelectorInfo {
  char                          *name;    /* label used in notebook tab */
  char                          *help_page;
  GimpColorSelectorMethods       m;
  int                            refs;    /* number of instances around */
  gboolean                       active;
  void                         (*death_callback) (void *data);
  void                          *death_data;
  struct _ColorSelectorInfo     *next;
} ColorSelectorInfo;

typedef struct _ColorSelectorInstance {
  _ColorNotebook                *color_notebook;
  ColorSelectorInfo             *info;
  GtkWidget                     *frame;   /* main widget */
  gpointer                       selector_data;
  struct _ColorSelectorInstance *next;
} ColorSelectorInstance;

static void selector_death (ColorSelectorInfo *info);


/* master list of all registered colour selectors */
static ColorSelectorInfo *selector_info = NULL;


#define RED   0
#define GREEN 1
#define BLUE  2
#define NUM_COLORS 3


ColorNotebookP
color_notebook_new (int                    r,
		    int                    g,
		    int                    b,
		    ColorNotebookCallback  callback,
		    void                  *client_data,
		    int                    wants_updates)
{
  ColorNotebookP cnp;
  GtkWidget *label;
  ColorSelectorInfo *info;
  ColorSelectorInstance *csel;

  g_return_val_if_fail (selector_info != NULL, NULL);

  cnp = g_new (_ColorNotebook, 1);

  cnp->callback      = callback;
  cnp->client_data   = client_data;
  cnp->wants_updates = wants_updates;
  cnp->selectors     = NULL;
  cnp->cur_page      = NULL;

  cnp->values[RED]   = cnp->orig_values[RED]   = r & 0xff;
  cnp->values[GREEN] = cnp->orig_values[GREEN] = g & 0xff;
  cnp->values[BLUE]  = cnp->orig_values[BLUE]  = b & 0xff;

  cnp->shell =
    gimp_dialog_new (_("Color Selection"), "color_selection",
		     color_notebook_help_func, cnp,
		     GTK_WIN_POS_NONE,
		     FALSE, FALSE, FALSE,

		     wants_updates ? _("Close") : _("OK"),
		     color_notebook_ok_callback,
		     cnp, NULL, TRUE, FALSE,
		     wants_updates ? _("Revert to Old Color") : _("Cancel"),
		     color_notebook_cancel_callback,
		     cnp, NULL, FALSE, TRUE,

		     NULL);

  /* do we actually need a notebook? */
  if (selector_info->next)
    {
      cnp->notebook = gtk_notebook_new ();
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cnp->shell)->vbox),
			  cnp->notebook, TRUE, TRUE, 1);
      gtk_widget_show (cnp->notebook);
    }
  else /* only one selector */
    {
      cnp->notebook = NULL;
    }

  /* create each registered colour selector */
  info = selector_info;
  while (info)
    {
      if (info->active)
	{

	  csel = g_new (ColorSelectorInstance, 1);
	  csel->color_notebook = cnp;
	  csel->info = info;
	  info->refs++;
	  csel->frame = info->m.new (r, g, b,
				     color_notebook_update_callback, csel,
				     &csel->selector_data);
	  gtk_object_set_data (GTK_OBJECT (csel->frame), "gimp_color_notebook",
			       csel);

	  if (cnp->notebook)
	    {
	      label = gtk_label_new (info->name);
	      gtk_widget_show (label);
	      /* hide the frame, so it doesn't get selected by mistake */
	      gtk_widget_hide (csel->frame);
	      gtk_notebook_append_page (GTK_NOTEBOOK (cnp->notebook),
					csel->frame, label);
	    }
	  else
	    {
	      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cnp->shell)->vbox),
				  csel->frame, TRUE, TRUE, 1);
	    }

	  gtk_widget_show (csel->frame);

	  if (!cnp->cur_page)
	    cnp->cur_page = csel;

	  /* link into list of all selectors hanging off the new notebook */
	  csel->next = cnp->selectors;
	  cnp->selectors = csel;
	}

      info = info->next;
    }

  gtk_widget_show (cnp->shell);

  /* this must come after showing the widget, otherwise we get a
   * switch_page signal for a non-visible color selector, which is bad
   * news. */
  if (cnp->notebook)
    {
      gtk_object_set_user_data (GTK_OBJECT (cnp->notebook), cnp);
      gtk_signal_connect (GTK_OBJECT (cnp->notebook), "switch_page",
			  (GtkSignalFunc)color_notebook_page_switch, NULL);
    }

  return cnp;
}

void
color_notebook_show (ColorNotebookP cnp)
{
  g_return_if_fail (cnp != NULL);
  gtk_widget_show (cnp->shell);
}

void
color_notebook_hide (ColorNotebookP cnp)
{
  g_return_if_fail (cnp != NULL);
  gtk_widget_hide (cnp->shell);
}

void
color_notebook_free (ColorNotebookP cnp)
{
  ColorSelectorInstance *csel, *next;

  g_return_if_fail (cnp != NULL);

  gtk_widget_destroy (cnp->shell);

  /* call the free functions for all the colour selectors */
  csel = cnp->selectors;
  while (csel)
    {
      next = csel->next;

      csel->info->m.free (csel->selector_data);

      csel->info->refs--;
      if (csel->info->refs == 0 && !csel->info->active)
	selector_death (csel->info);

      g_free (csel);
      csel = next;
    }

  g_free (cnp);
}

void
color_notebook_set_color (ColorNotebookP cnp,
			  int            r,
			  int            g,
			  int            b,
			  int            set_current)
{
  ColorSelectorInstance *csel;
  g_return_if_fail (cnp != NULL);

  cnp->orig_values[RED] = r;
  cnp->orig_values[GREEN] = g;
  cnp->orig_values[BLUE] = b;

  if (set_current)
    {
      cnp->values[RED] = r;
      cnp->values[GREEN] = g;
      cnp->values[BLUE] = b;
    }

  csel = cnp->cur_page;
  csel->info->m.setcolor (csel->selector_data, r, g, b, set_current);
}

/* Called by a colour selector on user selection of a colour */
static void
color_notebook_update_callback (void *data,
				int   r,
				int   g,
				int   b)
{
  ColorSelectorInstance *csel;
  ColorNotebookP cnp;

  g_return_if_fail (data != NULL);

  csel = (ColorSelectorInstance *) data;
  cnp = csel->color_notebook;

  cnp->values[RED] = r;
  cnp->values[GREEN] = g;
  cnp->values[BLUE] = b;

  if (cnp->wants_updates && cnp->callback)
    {
      (* cnp->callback) (cnp->values[RED],
			 cnp->values[GREEN],
			 cnp->values[BLUE],
			 COLOR_NOTEBOOK_UPDATE,
			 cnp->client_data);
    }
}

static void
color_notebook_ok_callback (GtkWidget *widget,
			    gpointer   data)
{
  ColorNotebookP cnp;

  cnp = (ColorNotebookP) data;

  if (cnp->callback)
    (* cnp->callback) (cnp->values[RED],
		       cnp->values[GREEN],
		       cnp->values[BLUE],
		       COLOR_NOTEBOOK_OK,
		       cnp->client_data);
}

static void
color_notebook_cancel_callback (GtkWidget *widget,
				gpointer   data)
{
  ColorNotebookP cnp;

  cnp = (ColorNotebookP) data;

  if (cnp->callback)
    (* cnp->callback) (cnp->orig_values[RED],
		       cnp->orig_values[GREEN],
		       cnp->orig_values[BLUE],
		       COLOR_NOTEBOOK_CANCEL,
		       cnp->client_data);
}

static void
color_notebook_page_switch (GtkWidget       *widget,
			    GtkNotebookPage *page,
			    guint            page_num)
{
  ColorNotebookP cnp;
  ColorSelectorInstance *csel;

  cnp = gtk_object_get_user_data (GTK_OBJECT (widget));
  csel = gtk_object_get_data (GTK_OBJECT(page->child), "gimp_color_notebook");

  g_return_if_fail (cnp != NULL && csel != NULL);

  cnp->cur_page = csel;
  csel->info->m.setcolor (csel->selector_data,
			  cnp->values[RED], 
			  cnp->values[GREEN],
			  cnp->values[BLUE],
			  TRUE);
}

static void
color_notebook_help_func (gpointer data)
{
  ColorNotebookP cnp;
  gchar *help_path;

  cnp = (ColorNotebookP) data;

  help_path = g_strconcat ("dialogs/color_selectors/",
			   cnp->cur_page->info->help_page,
			   NULL);
  gimp_help (help_path);
  g_free (help_path);
}

/**************************************************************/
/* Registration functions */

G_MODULE_EXPORT
GimpColorSelectorID
gimp_color_selector_register (const char *name,
			      const char *help_page,
			      GimpColorSelectorMethods *methods)
{
  ColorSelectorInfo *info;

  /* check the name is unique */
  info = selector_info;
  while (info)
    {
      if (!strcmp (info->name, name))
	return NULL;
      info = info->next;
    }

  info = g_new (ColorSelectorInfo, 1);

  info->name = g_strdup (name);
  info->help_page = g_strdup (help_page);
  info->m = *methods;
  info->refs = 0;
  info->active = TRUE;

  info->next = selector_info;
  selector_info = info;
  
  return info;
}


G_MODULE_EXPORT
gboolean
gimp_color_selector_unregister (GimpColorSelectorID id,
				void (*callback)(void *data),
				void *data)
{
  ColorSelectorInfo *info;

  info = selector_info;
  while (info)
    {
      if (info == id)
	{
	  info->active = FALSE;
	  info->death_callback = callback;
	  info->death_data = data;
	  if (info->refs == 0)
	    selector_death (info);
	  return TRUE;
	}
      info = info->next;
    }

  g_warning ("unknown color selector id %p", id);
  return FALSE;
}

static void
selector_death (ColorSelectorInfo *info)
{
  ColorSelectorInfo *here, *prev;

  here = selector_info;
  prev = NULL;
  while (here)
    {
      if (here == info)
	{	  
	  if (prev)
	    prev->next = info->next;
	  else
	    selector_info = info->next;

	  if (info->death_callback)
	    (*info->death_callback) (info->death_data);

	  g_free (info->name);
	  g_free (info);

	  return;
	}
      prev = here;
      here = here->next;
    }

  g_warning ("color selector %p not found, can't happen!", info);
}

/* End of color_notebook.c */
