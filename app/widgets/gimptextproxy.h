/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextProxy
 * Copyright (C) 2009-2010  Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


#define GIMP_TYPE_TEXT_PROXY            (gimp_text_proxy_get_type ())
#define GIMP_TEXT_PROXY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEXT_PROXY, GimpTextProxy))
#define GIMP_IS_TEXT_PROXY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEXT_PROXY))
#define GIMP_TEXT_PROXY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TEXT_PROXY, GimpTextProxyClass))
#define GIMP_IS_TEXT_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TEXT_PROXY))


typedef struct _GimpTextProxy       GimpTextProxy;
typedef struct _GimpTextProxyClass  GimpTextProxyClass;

struct _GimpTextProxy
{
  GtkTextView  parent_instance;
};

struct _GimpTextProxyClass
{
  GtkTextViewClass  parent_class;

  void (* change_size)     (GimpTextProxy *proxy,
                            gdouble        amount);
  void (* change_baseline) (GimpTextProxy *proxy,
                            gdouble        amount);
  void (* change_kerning)  (GimpTextProxy *proxy,
                            gdouble        amount);
};


GType       gimp_text_proxy_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_text_proxy_new      (void);
