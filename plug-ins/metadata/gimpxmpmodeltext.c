/* gimpxmpmodeltext.c - custom text widget linked to the xmp model
 *
 * Copyright (C) 2010, Róman Joost <romanofski@gimp.org>
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
#include "gimpxmpmodeltext.h"


static void   gimp_xmp_model_text_iface_init    (GimpXmpModelWidgetInterface *iface);

static void   gimp_xmp_model_text_changed       (GtkTextBuffer               *text_buffer,
                                                 gpointer                    *user_data);

void          gimp_xmp_model_text_set_text      (GimpXmpModelWidget          *widget,
                                                 const gchar                 *tree_value);


G_DEFINE_TYPE_WITH_CODE (GimpXmpModelText, gimp_xmp_model_text,
                         GTK_TYPE_TEXT_VIEW,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_XMP_MODEL_WIDGET,
                                                gimp_xmp_model_text_iface_init))

#define parent_class gimp_xmp_model_text_parent_class


static GObject *
gimp_xmp_model_text_constructor (GType                  type,
                                 guint                  n_params,
                                 GObjectConstructParam *params)
{
  GObject                  *obj;

  obj = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp_xmp_model_widget_constructor (obj);

  return obj;
}

static void
gimp_xmp_model_text_class_init (GimpXmpModelTextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_xmp_model_text_constructor;
  object_class->set_property = gimp_xmp_model_widget_set_property;
  object_class->get_property = gimp_xmp_model_widget_get_property;

  gimp_xmp_model_widget_install_properties (object_class);
}

static void
gimp_xmp_model_text_init (GimpXmpModelText *text)
{
  GtkTextBuffer *text_buffer;

  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
  g_signal_connect (text_buffer, "end-user-action",
                    G_CALLBACK (gimp_xmp_model_text_changed),
                    text);
}

static void
gimp_xmp_model_text_iface_init (GimpXmpModelWidgetInterface *iface)
{
  iface->widget_set_text = gimp_xmp_model_text_set_text;
}

static void
gimp_xmp_model_text_changed (GtkTextBuffer *text_buffer,
                             gpointer      *user_data)
{
  GimpXmpModelText *text = GIMP_XMP_MODEL_TEXT (user_data);
  GtkTextIter    start;
  GtkTextIter    end;
  const gchar   *value;

  gtk_text_buffer_get_bounds (text_buffer, &start, &end);
  value = gtk_text_buffer_get_text (text_buffer, &start, &end, FALSE);
  gimp_xmp_model_widget_changed (GIMP_XMP_MODEL_WIDGET (text),
                                 value);
}

void
gimp_xmp_model_text_set_text (GimpXmpModelWidget *widget,
                              const gchar        *tree_value)
{
  GtkTextBuffer *text_buffer;

  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  gtk_text_buffer_set_text (text_buffer, tree_value, -1);
}
