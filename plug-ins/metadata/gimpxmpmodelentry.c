/* gimpxmpmodelentry.c - custom entry widget linked to the xmp model
 *
 * Copyright (C) 2009, Róman Joost <romanofski@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "xmp-schemas.h"
#include "xmp-model.h"

#include "gimpxmpmodelwidget.h"
#include "gimpxmpmodelentry.h"


static void   gimp_xmp_model_entry_iface_init  (GimpXmpModelWidgetInterface *iface);

static void   gimp_xmp_model_entry_constructed (GObject            *object);

static void   gimp_xmp_model_entry_set_text    (GimpXmpModelWidget *widget,
                                                const gchar        *tree_value);

static void   gimp_xmp_model_entry_changed     (GimpXmpModelEntry  *entry);


G_DEFINE_TYPE_WITH_CODE (GimpXmpModelEntry, gimp_xmp_model_entry,
                         GTK_TYPE_ENTRY,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_XMP_MODEL_WIDGET,
                                                gimp_xmp_model_entry_iface_init))


#define parent_class gimp_xmp_model_entry_parent_class


static void
gimp_xmp_model_entry_class_init (GimpXmpModelEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_xmp_model_entry_constructed;
  object_class->set_property = gimp_xmp_model_widget_set_property;
  object_class->get_property = gimp_xmp_model_widget_get_property;

  gimp_xmp_model_widget_install_properties (object_class);
}

static void
gimp_xmp_model_entry_iface_init (GimpXmpModelWidgetInterface *iface)
{
  iface->widget_set_text = gimp_xmp_model_entry_set_text;
}

static void
gimp_xmp_model_entry_init (GimpXmpModelEntry *entry)
{
  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_xmp_model_entry_changed),
                    NULL);
}

static void
gimp_xmp_model_entry_constructed (GObject *object)
{
  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_xmp_model_widget_constructor (object);
}

static void
gimp_xmp_model_entry_set_text (GimpXmpModelWidget *widget,
                               const gchar        *tree_value)
{
  gtk_entry_set_text (GTK_ENTRY (widget), tree_value);
}

static void
gimp_xmp_model_entry_changed (GimpXmpModelEntry *entry)
{
  const gchar *value = gtk_entry_get_text (GTK_ENTRY (entry));

  gimp_xmp_model_widget_changed (GIMP_XMP_MODEL_WIDGET (entry), value);
}
