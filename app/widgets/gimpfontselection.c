/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfontselection.c
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * Partially based on code from GtkFontSelection.
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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimpfontselection.h"
#include "gimpfontselection-dialog.h"

#include "libgimp/gimpintl.h"


#define  FONT_LIST_HEIGHT       136
#define  FONT_LIST_WIDTH        190
#define  FONT_STYLE_LIST_WIDTH  170


static void   gimp_font_selection_class_init   (GimpFontSelectionClass *klass);
static void   gimp_font_selection_init         (GimpFontSelection      *fontsel);
static void   gimp_font_selection_finalize     (GObject        *object);

static void   gimp_font_selection_browse_callback   (GtkWidget *widget,
                                                     gpointer   data);
static void   gimp_font_selection_entry_callback    (GtkWidget *widget,
                                                     gpointer   data);

enum
{
  FONT_CHANGED,
  LAST_SIGNAL
};

static guint gimp_font_selection_signals[LAST_SIGNAL] = { 0 };

static GtkHBoxClass   *parent_class = NULL;


GType
gimp_font_selection_get_type (void)
{
  static GType fontsel_type = 0;

  if (!fontsel_type)
    {
      static const GTypeInfo fontsel_info =
      {
        sizeof (GimpFontSelectionClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_font_selection_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpFontSelection),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_font_selection_init,
      };

      fontsel_type = g_type_register_static (GTK_TYPE_HBOX, 
                                             "GimpFontSelection", 
                                             &fontsel_info, 0);
    }
  
  return fontsel_type;
}

static void
gimp_font_selection_class_init (GimpFontSelectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class = (GObjectClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gimp_font_selection_signals[FONT_CHANGED] = 
    g_signal_new ("font_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpFontSelectionClass,
				   font_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize = gimp_font_selection_finalize;

  klass->font_changed = NULL;
}

static void
gimp_font_selection_init (GimpFontSelection *fontsel)
{
  GtkWidget *button;
  GtkWidget *image;

  fontsel->context   = NULL;

  button = gtk_button_new ();
  image = gtk_image_new_from_stock (GTK_STOCK_SELECT_FONT, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);
  gtk_box_pack_end (GTK_BOX (fontsel), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (gimp_font_selection_browse_callback),
                    fontsel);
  gtk_widget_show (button);

  fontsel->entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (fontsel), fontsel->entry, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (fontsel->entry), "activate",
                    G_CALLBACK (gimp_font_selection_entry_callback),
                    fontsel);
  gtk_widget_show (fontsel->entry);
}

static void
gimp_font_selection_finalize (GObject *object)
{
  GimpFontSelection *fontsel = GIMP_FONT_SELECTION (object);

  if (fontsel->dialog)
    gtk_widget_destroy (GTK_WIDGET (fontsel->dialog));

  if (fontsel->context)
    g_object_unref (G_OBJECT (fontsel->context));

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gimp_font_selection_new:
 * @context: the #PangoContext to select a font from.
 *
 * Creates a new #GimpFontSelection widget.
 *
 * Returns: A pointer to the new #GimpFontSelection widget.
 **/
GtkWidget *
gimp_font_selection_new (PangoContext *context)
{
  GimpFontSelection *fontsel;

  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  fontsel = g_object_new (GIMP_TYPE_FONT_SELECTION, NULL);

  fontsel->context = context;
  g_object_ref (G_OBJECT (fontsel->context));
  
  return GTK_WIDGET (fontsel);
}

void
gimp_font_selection_font_changed (GimpFontSelection *fontsel)
{
  gchar *name;

  g_return_if_fail (GIMP_IS_FONT_SELECTION (fontsel));

  if (fontsel->entry)
    {
      if (fontsel->font_desc)
        {
          name = pango_font_description_to_string (fontsel->font_desc);
          gtk_entry_set_text (GTK_ENTRY (fontsel->entry), name);
          g_free (name);
        }
      else
        {
          gtk_entry_set_text (GTK_ENTRY (fontsel->entry), NULL);
        }

      if (fontsel->dialog)
        gimp_font_selection_dialog_set_font_desc (GIMP_FONT_SELECTION_DIALOG (fontsel->dialog), 
                                                  fontsel->font_desc);
    }

  g_signal_emit (G_OBJECT (fontsel), 
                 gimp_font_selection_signals[FONT_CHANGED], 0);
}

gboolean
gimp_font_selection_set_fontname (GimpFontSelection *fontsel,
                                  const gchar       *fontname)
{
  PangoFontDescription  *new_desc;
  PangoFontDescription **descs;
  gint      n_descs, i;
  gboolean  found = FALSE;

  g_return_val_if_fail (GIMP_IS_FONT_SELECTION (fontsel), FALSE);
  g_return_val_if_fail (fontname != NULL, FALSE);

  new_desc = pango_font_description_from_string (fontname);

  /* Check to make sure that this is in the list of allowed fonts */

  pango_context_list_fonts (fontsel->context,
			    new_desc->family_name, &descs, &n_descs);
  
  for (i=0; i<n_descs; i++)
    {
      if (descs[i]->weight  == new_desc->weight  &&
	  descs[i]->style   == new_desc->style   &&
	  descs[i]->stretch == new_desc->stretch &&
	  descs[i]->variant == new_desc->variant)
	{
	  found = TRUE;
	  break;
	}
    }

  pango_font_descriptions_free (descs, n_descs);

  if (found)
    {
      if (fontsel->font_desc)
        pango_font_description_free (fontsel->font_desc);
      fontsel->font_desc = new_desc;
      gimp_font_selection_font_changed (fontsel);

      return TRUE;
    }
  else
    {
      pango_font_description_free (new_desc);
      return FALSE;
    }
}

const gchar *
gimp_font_selection_get_fontname (GimpFontSelection *fontsel)
{
  if (fontsel->font_desc)
    return pango_font_description_to_string (fontsel->font_desc);
  else
    return NULL;
}

void
gimp_font_selection_set_font_desc (GimpFontSelection          *fontsel,
                                   const PangoFontDescription *new_desc)
{
  PangoFontDescription *font_desc;
  gboolean              changed = FALSE;

  g_return_if_fail (GIMP_IS_FONT_SELECTION (fontsel));
  g_return_if_fail (new_desc != NULL);
  
  if (!fontsel->font_desc)
    {
      fontsel->font_desc = pango_font_description_copy (new_desc);
      gimp_font_selection_font_changed (fontsel);
      return;
    }

  font_desc = fontsel->font_desc;
  
  if (strcmp (font_desc->family_name, new_desc->family_name) != 0)
    {
      g_free (font_desc->family_name);
      font_desc->family_name = g_strdup (new_desc->family_name);
      
      changed = TRUE;
    }
  
  if (font_desc->style   != new_desc->style   ||
      font_desc->variant != new_desc->variant ||
      font_desc->weight  != new_desc->weight  ||
      font_desc->stretch != new_desc->stretch)
    {
      font_desc->style   = new_desc->style;
      font_desc->variant = new_desc->variant;
      font_desc->weight  = new_desc->weight;
      font_desc->stretch = new_desc->stretch;
      
      changed = TRUE;
    }

  if (changed)
    gimp_font_selection_font_changed (fontsel);
}

PangoFontDescription *
gimp_font_selection_get_font_desc (GimpFontSelection *fontsel)
{
  g_return_val_if_fail (GIMP_IS_FONT_SELECTION (fontsel), NULL);

  if (fontsel->font_desc)
    return pango_font_description_copy (fontsel->font_desc);
  else
    return NULL;
}

static void
gimp_font_selection_browse_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpFontSelection *fontsel = GIMP_FONT_SELECTION (data);

  if (!fontsel->dialog)
    {
      fontsel->dialog = gimp_font_selection_dialog_new (fontsel);
      g_object_add_weak_pointer (G_OBJECT (fontsel->dialog), 
                                 (gpointer *) &fontsel->dialog);
      g_signal_connect_object (G_OBJECT (widget), "unmap",
                               G_CALLBACK (gtk_widget_hide),
                               G_OBJECT (fontsel->dialog),
                               G_CONNECT_SWAPPED);
    }

  gtk_widget_show (fontsel->dialog);
}

static void
gimp_font_selection_entry_callback (GtkWidget *widget,
                                    gpointer   data)
{
  GimpFontSelection *fontsel = GIMP_FONT_SELECTION (data);
  const gchar       *name;

  name = gtk_entry_get_text (GTK_ENTRY (widget));
  if (name && *name)
    gimp_font_selection_set_fontname (fontsel, name);
}
