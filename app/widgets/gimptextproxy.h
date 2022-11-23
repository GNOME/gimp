/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextProxy
 * Copyright (C) 2009-2010  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TEXT_PROXY_H__
#define __LIGMA_TEXT_PROXY_H__


#define LIGMA_TYPE_TEXT_PROXY            (ligma_text_proxy_get_type ())
#define LIGMA_TEXT_PROXY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TEXT_PROXY, LigmaTextProxy))
#define LIGMA_IS_TEXT_PROXY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TEXT_PROXY))
#define LIGMA_TEXT_PROXY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TEXT_PROXY, LigmaTextProxyClass))
#define LIGMA_IS_TEXT_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TEXT_PROXY))


typedef struct _LigmaTextProxy       LigmaTextProxy;
typedef struct _LigmaTextProxyClass  LigmaTextProxyClass;

struct _LigmaTextProxy
{
  GtkTextView  parent_instance;
};

struct _LigmaTextProxyClass
{
  GtkTextViewClass  parent_class;

  void (* change_size)     (LigmaTextProxy *proxy,
                            gdouble        amount);
  void (* change_baseline) (LigmaTextProxy *proxy,
                            gdouble        amount);
  void (* change_kerning)  (LigmaTextProxy *proxy,
                            gdouble        amount);
};


GType       ligma_text_proxy_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_text_proxy_new      (void);


#endif /* __LIGMA_TEXT_PROXY_H__ */
