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
#include "gimpcolorselector.h"
#include "gimpwidgetsmarshal.h"


enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};


static void   gimp_color_notebook_class_init (GimpColorNotebookClass *klass);
static void   gimp_color_notebook_init       (GimpColorNotebook      *notebook);

static void   gimp_color_notebook_finalize        (GObject           *object);
static void   gimp_color_notebook_switch_page     (GtkNotebook       *gtk_notebook,
                                                   GtkNotebookPage   *page,
                                                   guint              page_num);

static void   gimp_color_notebook_update_callback (GimpColorSelector *selector,
                                                   const GimpRGB     *rgb,
                                                   const GimpHSV     *hsv,
                                                   GimpColorNotebook *notebook);


static GtkNotebookClass *parent_class = NULL;

static guint  notebook_signals[LAST_SIGNAL] = { 0 };


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

      notebook_type = g_type_register_static (GTK_TYPE_NOTEBOOK,
                                              "GimpColorNotebook", 
                                              &notebook_info, 0);
    }

  return notebook_type;
}

static void
gimp_color_notebook_class_init (GimpColorNotebookClass *klass)
{
  GObjectClass     *object_class;
  GtkNotebookClass *notebook_class;

  object_class   = G_OBJECT_CLASS (klass);
  notebook_class = GTK_NOTEBOOK_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  notebook_signals[COLOR_CHANGED] =
    g_signal_new ("color_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorNotebookClass, color_changed),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__POINTER_POINTER,
                  G_TYPE_NONE, 2,
                  G_TYPE_POINTER,
                  G_TYPE_POINTER);

  object_class->finalize      = gimp_color_notebook_finalize;

  notebook_class->switch_page = gimp_color_notebook_switch_page;

  klass->color_changed        = NULL;
}

static void
gimp_color_notebook_init (GimpColorNotebook *notebook)
{
  GtkWidget *selector;
  GtkWidget *label;
  GType     *selector_types;
  gint       n_selector_types;
  gint       i;

  gimp_rgba_set (&notebook->rgb, 0.0, 0.0, 0.0, 1.0);
  gimp_rgb_to_hsv (&notebook->rgb, &notebook->hsv);

  selector_types = g_type_children (GIMP_TYPE_COLOR_SELECTOR, &n_selector_types);

  if (n_selector_types == 1)
    {
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
      gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
    }

  for (i = 0; i < n_selector_types; i++)
    {
      selector = gimp_color_selector_new (selector_types[i],
                                          &notebook->rgb,
                                          &notebook->hsv);

      if (! selector)
        continue;

      label = gtk_label_new (GIMP_COLOR_SELECTOR_GET_CLASS (selector)->name);

      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), selector, label);

      if (! notebook->cur_page)
        notebook->cur_page = GIMP_COLOR_SELECTOR (selector);

      notebook->selectors = g_list_append (notebook->selectors, selector);

      gtk_widget_show (selector);

      g_signal_connect (G_OBJECT (selector), "color_changed",
                        G_CALLBACK (gimp_color_notebook_update_callback),
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
gimp_color_notebook_switch_page (GtkNotebook     *gtk_notebook,
                                 GtkNotebookPage *page,
                                 guint            page_num)
{
  GimpColorNotebook *notebook;
  GtkWidget         *page_widget;

  notebook = GIMP_COLOR_NOTEBOOK (gtk_notebook);

  GTK_NOTEBOOK_CLASS (parent_class)->switch_page (gtk_notebook, page, page_num);

  page_widget = gtk_notebook_get_nth_page (gtk_notebook, page_num);

  notebook->cur_page = GIMP_COLOR_SELECTOR (page_widget);

  g_signal_handlers_block_by_func (G_OBJECT (notebook->cur_page),
                                   gimp_color_notebook_update_callback,
                                   notebook);

  gimp_color_selector_set_channel (notebook->cur_page,
                                   notebook->channel);
  gimp_color_selector_set_color (notebook->cur_page,
                                 &notebook->rgb,
                                 &notebook->hsv);

  g_signal_handlers_unblock_by_func (G_OBJECT (notebook->cur_page),
                                     gimp_color_notebook_update_callback,
                                     notebook);
}


/*  public functions  */

GtkWidget *
gimp_color_notebook_new (void)
{
  GimpColorNotebook *notebook;

  notebook = g_object_new (GIMP_TYPE_COLOR_NOTEBOOK, NULL);

  return GTK_WIDGET (notebook);
}

void
gimp_color_notebook_set_color (GimpColorNotebook *notebook,
                               const GimpRGB     *rgb,
                               const GimpHSV     *hsv)
{
  g_return_if_fail (GIMP_IS_COLOR_NOTEBOOK (notebook));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  notebook->rgb = *rgb;
  notebook->hsv = *hsv;

  g_signal_handlers_block_by_func (G_OBJECT (notebook->cur_page),
                                   gimp_color_notebook_update_callback,
                                   notebook);

  gimp_color_selector_set_color (notebook->cur_page,
                                 &notebook->rgb,
                                 &notebook->hsv);

  g_signal_handlers_unblock_by_func (G_OBJECT (notebook->cur_page),
                                     gimp_color_notebook_update_callback,
                                     notebook);
}

void
gimp_color_notebook_get_color (GimpColorNotebook *notebook,
                               GimpRGB           *rgb,
                               GimpHSV           *hsv)
{
  g_return_if_fail (GIMP_IS_COLOR_NOTEBOOK (notebook));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  *rgb = notebook->rgb;
  *hsv = notebook->hsv;
}

void
gimp_color_notebook_color_changed (GimpColorNotebook *notebook)
{
  g_return_if_fail (GIMP_IS_COLOR_NOTEBOOK (notebook));

  g_signal_emit (G_OBJECT (notebook), notebook_signals[COLOR_CHANGED], 0,
                 &notebook->rgb, &notebook->hsv);
}

void
gimp_color_notebook_set_channel (GimpColorNotebook        *notebook,
                                 GimpColorSelectorChannel  channel)
{
  g_return_if_fail (GIMP_IS_COLOR_NOTEBOOK (notebook));

  if (channel != notebook->channel)
    {
      notebook->channel = channel;

      gimp_color_selector_set_channel (notebook->cur_page,
                                       notebook->channel);
    }
}


/*  private functions  */

static void
gimp_color_notebook_update_callback (GimpColorSelector *selector,
                                     const GimpRGB     *rgb,
                                     const GimpHSV     *hsv,
                                     GimpColorNotebook *notebook)
{
  notebook->rgb = *rgb;
  notebook->hsv = *hsv;

  gimp_color_notebook_color_changed (notebook);
}
