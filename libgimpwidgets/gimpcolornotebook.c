/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpcolornotebook.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on color_notebook module
 * Copyright (C) 1998 Austin Donnelly <austin@greenend.org.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcolornotebook.h"
#include "gimpcolorscales.h"
#include "gimpwidgetsmarshal.h"


static void   gimp_color_notebook_class_init (GimpColorNotebookClass *klass);
static void   gimp_color_notebook_init       (GimpColorNotebook      *notebook);

static void   gimp_color_notebook_finalize        (GObject           *object);

static void   gimp_color_notebook_set_show_alpha  (GimpColorSelector *selector,
                                                   gboolean           show_alpha);
static void   gimp_color_notebook_set_color       (GimpColorSelector *selector,
                                                   const GimpRGB     *rgb,
                                                   const GimpHSV     *hsv);
static void   gimp_color_notebook_set_channel     (GimpColorSelector *selector,
                                                   GimpColorSelectorChannel channel);

static void   gimp_color_notebook_switch_page     (GtkNotebook       *gtk_notebook,
                                                   GtkNotebookPage   *page,
                                                   guint              page_num,
                                                   GimpColorNotebook *notebook);

static void   gimp_color_notebook_color_changed   (GimpColorSelector *page,
                                                   const GimpRGB     *rgb,
                                                   const GimpHSV     *hsv,
                                                   GimpColorNotebook *notebook);
static void   gimp_color_notebook_channel_changed (GimpColorSelector *page,
                                                   GimpColorSelectorChannel channel,
                                                   GimpColorNotebook *notebook);


static GimpColorSelectorClass *parent_class = NULL;


GType
gimp_color_notebook_get_type (void)
{
  static GType notebook_type = 0;

  if (! notebook_type)
    {
      static const GTypeInfo notebook_info =
      {
        sizeof (GimpColorNotebookClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_notebook_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorNotebook),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_notebook_init,
      };

      notebook_type = g_type_register_static (GIMP_TYPE_COLOR_SELECTOR,
                                              "GimpColorNotebook", 
                                              &notebook_info, 0);
    }

  return notebook_type;
}

static void
gimp_color_notebook_class_init (GimpColorNotebookClass *klass)
{
  GObjectClass           *object_class;
  GimpColorSelectorClass *selector_class;

  object_class   = G_OBJECT_CLASS (klass);
  selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize         = gimp_color_notebook_finalize;

  selector_class->name           = "Notebook";
  selector_class->help_page      = "notebook.html";
  selector_class->set_show_alpha = gimp_color_notebook_set_show_alpha;
  selector_class->set_color      = gimp_color_notebook_set_color;
  selector_class->set_channel    = gimp_color_notebook_set_channel;
}

static void
gimp_color_notebook_init (GimpColorNotebook *notebook)
{
  GimpColorSelector *selector;
  GtkWidget         *page;
  GtkWidget         *label;
  GType             *selector_types;
  gint               n_selector_types;
  gint               i;

  selector = GIMP_COLOR_SELECTOR (notebook);

  notebook->notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (notebook), notebook->notebook, TRUE, TRUE, 0);
  gtk_widget_show (notebook->notebook);

  g_signal_connect (G_OBJECT (notebook->notebook), "switch_page",
                    G_CALLBACK (gimp_color_notebook_switch_page),
                    notebook);

  selector_types = g_type_children (GIMP_TYPE_COLOR_SELECTOR, &n_selector_types);

  if (n_selector_types == 2)
    {
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook->notebook), FALSE);
      gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook->notebook), FALSE);
    }

  for (i = 0; i < n_selector_types; i++)
    {
      /*  skip ourselves  */
      if (g_type_is_a (selector_types[i], GIMP_TYPE_COLOR_NOTEBOOK))
        continue;

      /*  skip the "Scales" color selector  */
      if (g_type_is_a (selector_types[i], GIMP_TYPE_COLOR_SCALES))
        continue;

      page = gimp_color_selector_new (selector_types[i],
                                      &selector->rgb,
                                      &selector->hsv,
                                      selector->channel);

      if (! page)
        continue;

      gimp_color_selector_set_show_alpha (GIMP_COLOR_SELECTOR (page), FALSE);

      label = gtk_label_new (GIMP_COLOR_SELECTOR_GET_CLASS (page)->name);

      gtk_notebook_append_page (GTK_NOTEBOOK (notebook->notebook), page, label);

      if (! notebook->cur_page)
        notebook->cur_page = GIMP_COLOR_SELECTOR (page);

      notebook->selectors = g_list_append (notebook->selectors, page);

      gtk_widget_show (page);

      g_signal_connect (G_OBJECT (page), "color_changed",
                        G_CALLBACK (gimp_color_notebook_color_changed),
                        notebook);
      g_signal_connect (G_OBJECT (page), "channel_changed",
                        G_CALLBACK (gimp_color_notebook_channel_changed),
                        notebook);
    }

  g_free (selector_types);
}

static void
gimp_color_notebook_finalize (GObject *object)
{
  GimpColorNotebook *notebook;

  notebook = GIMP_COLOR_NOTEBOOK (object);

  if (notebook->selectors)
    {
      g_list_free (notebook->selectors);
      notebook->selectors = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_notebook_set_show_alpha (GimpColorSelector *selector,
                                    gboolean           show_alpha)
{
  GimpColorNotebook *notebook;
  GimpColorSelector *child;
  GList             *list;

  notebook = GIMP_COLOR_NOTEBOOK (selector);

  for (list = notebook->selectors; list; list = g_list_next (list))
    {
      child = (GimpColorSelector *) list->data;

      gimp_color_selector_set_show_alpha (child, show_alpha);
    }
}

static void
gimp_color_notebook_set_color (GimpColorSelector *selector,
                               const GimpRGB     *rgb,
                               const GimpHSV     *hsv)
{
  GimpColorNotebook *notebook;

  notebook = GIMP_COLOR_NOTEBOOK (selector);

  g_signal_handlers_block_by_func (G_OBJECT (notebook->cur_page),
                                   gimp_color_notebook_color_changed,
                                   notebook);

  gimp_color_selector_set_color (notebook->cur_page, rgb, hsv);

  g_signal_handlers_unblock_by_func (G_OBJECT (notebook->cur_page),
                                     gimp_color_notebook_color_changed,
                                     notebook);
}

static void
gimp_color_notebook_set_channel (GimpColorSelector        *selector,
                                 GimpColorSelectorChannel  channel)
{
  GimpColorNotebook *notebook;

  notebook = GIMP_COLOR_NOTEBOOK (selector);

  g_signal_handlers_block_by_func (G_OBJECT (notebook->cur_page),
                                   gimp_color_notebook_channel_changed,
                                   notebook);

  gimp_color_selector_set_channel (notebook->cur_page, channel);

  g_signal_handlers_unblock_by_func (G_OBJECT (notebook->cur_page),
                                     gimp_color_notebook_channel_changed,
                                     notebook);
}

static void
gimp_color_notebook_switch_page (GtkNotebook       *gtk_notebook,
                                 GtkNotebookPage   *page,
                                 guint              page_num,
                                 GimpColorNotebook *notebook)
{
  GimpColorSelector *selector;
  GtkWidget         *page_widget;

  selector = GIMP_COLOR_SELECTOR (notebook);

  page_widget = gtk_notebook_get_nth_page (gtk_notebook, page_num);

  notebook->cur_page = GIMP_COLOR_SELECTOR (page_widget);

  g_signal_handlers_block_by_func (G_OBJECT (notebook->cur_page),
                                   gimp_color_notebook_color_changed,
                                   notebook);
  g_signal_handlers_block_by_func (G_OBJECT (notebook->cur_page),
                                   gimp_color_notebook_channel_changed,
                                   notebook);

  gimp_color_selector_set_color (notebook->cur_page,
                                 &selector->rgb,
                                 &selector->hsv);
  gimp_color_selector_set_channel (notebook->cur_page,
                                   selector->channel);

  g_signal_handlers_unblock_by_func (G_OBJECT (notebook->cur_page),
                                     gimp_color_notebook_color_changed,
                                     notebook);
  g_signal_handlers_unblock_by_func (G_OBJECT (notebook->cur_page),
                                     gimp_color_notebook_channel_changed,
                                     notebook);
}

static void
gimp_color_notebook_color_changed (GimpColorSelector *page,
                                   const GimpRGB     *rgb,
                                   const GimpHSV     *hsv,
                                   GimpColorNotebook *notebook)
{
  GimpColorSelector *selector;

  selector = GIMP_COLOR_SELECTOR (notebook);

  selector->rgb = *rgb;
  selector->hsv = *hsv;

  gimp_color_selector_color_changed (selector);
}

static void
gimp_color_notebook_channel_changed (GimpColorSelector        *page,
                                     GimpColorSelectorChannel  channel,
                                     GimpColorNotebook        *notebook)
{
  GimpColorSelector *selector;

  selector = GIMP_COLOR_SELECTOR (notebook);

  selector->channel = channel;

  gimp_color_selector_channel_changed (selector);
}
