/* GimpXmpModelText.h - custom text widget linked to the xmp model
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

#ifndef __GIMP_XMP_MODEL_TEXT_H__
#define __GIMP_XMP_MODEL_TEXT_H__

G_BEGIN_DECLS

#define GIMP_TYPE_XMP_MODEL_TEXT               (gimp_xmp_model_text_get_type ())
#define GIMP_XMP_MODEL_TEXT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_XMP_MODEL_TEXT, GimpXmpModelText))
#define GIMP_XMP_MODEL_TEXT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_XMP_MODEL_TEXT, XMPModelClass))
#define GIMP_IS_XMP_MODEL_TEXT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_XMP_MODEL_TEXT))
#define GIMP_IS_XMP_MODEL_TEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_XMP_MODEL_TEXT))
#define GIMP_XMP_MODEL_TEXT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_XMP_MODEL_TEXT, XMPModelClass))


typedef struct _GimpXmpModelText       GimpXmpModelText;
typedef struct _GimpXmpModelTextClass  GimpXmpModelTextClass;


struct _GimpXmpModelTextClass
{
  GtkTextViewClass parent_class;

  void          (*gimp_xmp_model_set_text_text) (GimpXmpModelText *text,
                                                 const gchar      *value);

  const gchar * (*gimp_xmp_model_get_text_text) (GimpXmpModelText *text);
};

struct _GimpXmpModelText
{
  GtkTextView   parent_instance;
  gpointer      priv;
};


GType         gimp_xmp_model_text_get_type       (void) G_GNUC_CONST;

GtkWidget   * gimp_xmp_model_text_new            (const gchar       *schema_uri,
                                                  const gchar       *property,
                                                  XMPModel          *xmp_model);

void          gimp_xmp_model_set_text_text       (GimpXmpModelText  *text,
                                                  const gchar       *value);

const gchar * gimp_xmp_model_get_text_text       (GimpXmpModelText *text);

G_END_DECLS

#endif /* __GIMP_XMP_MODEL_TEXT_H__ */
