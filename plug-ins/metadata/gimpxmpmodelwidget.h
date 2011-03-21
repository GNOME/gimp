/* gimpxmpmodelwidget.h - interface definition for xmpmodel gtkwidgets
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

#ifndef __GIMP_XMP_MODEL_WIDGET_H__
#define __GIMP_XMP_MODEL_WIDGET_H__

G_BEGIN_DECLS


typedef enum
{
  GIMP_XMP_MODEL_WIDGET_PROP_0,
  GIMP_XMP_MODEL_WIDGET_PROP_SCHEMA_URI,
  GIMP_XMP_MODEL_WIDGET_PROP_PROPERTY_NAME,
  GIMP_XMP_MODEL_WIDGET_PROP_XMPMODEL
} GimpXmpModelWidgetProp;



#define GIMP_TYPE_XMP_MODEL_WIDGET                  (gimp_xmp_model_widget_interface_get_type ())
#define GIMP_IS_XMP_MODEL_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_XMP_MODEL_WIDGET))
#define GIMP_XMP_MODEL_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_XMP_MODEL_WIDGET, GimpXmpModelWidget))
#define GIMP_XMP_MODEL_WIDGET_GET_INTERFACE(obj)    (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_XMP_MODEL_WIDGET, GimpXmpModelWidgetInterface))


typedef struct _GimpXmpModelWidget              GimpXmpModelWidget;
typedef struct _GimpXmpModelWidgetInterface     GimpXmpModelWidgetInterface;


struct _GimpXmpModelWidgetInterface
{
  GTypeInterface base_iface;

  void          (*widget_set_text) (GimpXmpModelWidget  *widget,
                                    const gchar         *value);
};


GType           gimp_xmp_model_widget_interface_get_type    (void) G_GNUC_CONST;

void            gimp_xmp_model_widget_install_properties    (GObjectClass *klass);

void            gimp_xmp_model_widget_constructor           (GObject *object);

void            gimp_xmp_model_widget_set_text              (GimpXmpModelWidget *widget,
                                                             const gchar        *value);

void            gimp_xmp_model_widget_set_property          (GObject            *object,
                                                             guint               property_id,
                                                             const GValue       *value,
                                                             GParamSpec         *pspec);

void            gimp_xmp_model_widget_get_property          (GObject            *object,
                                                             guint               property_id,
                                                             GValue             *value,
                                                             GParamSpec         *pspec);

void            gimp_xmp_model_widget_changed               (GimpXmpModelWidget *widget,
                                                             const gchar        *value);


G_END_DECLS

#endif /* __GIMP_XMP_MODEL_WIDGET_H__ */
