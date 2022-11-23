/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaextensiondetails.h
 * Copyright (C) 2018 Jehan <jehan@ligma.org>
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

#ifndef __LIGMA_EXTENSION_DETAILS_H__
#define __LIGMA_EXTENSION_DETAILS_H__


#define LIGMA_TYPE_EXTENSION_DETAILS            (ligma_extension_details_get_type ())
#define LIGMA_EXTENSION_DETAILS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_EXTENSION_DETAILS, LigmaExtensionDetails))
#define LIGMA_EXTENSION_DETAILS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_EXTENSION_DETAILS, LigmaExtensionDetailsClass))
#define LIGMA_IS_EXTENSION_DETAILS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_EXTENSION_DETAILS))
#define LIGMA_IS_EXTENSION_DETAILS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_EXTENSION_DETAILS))
#define LIGMA_EXTENSION_DETAILS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_EXTENSION_DETAILS, LigmaExtensionDetailsClass))


typedef struct _LigmaExtensionDetailsClass    LigmaExtensionDetailsClass;
typedef struct _LigmaExtensionDetailsPrivate  LigmaExtensionDetailsPrivate;

struct _LigmaExtensionDetails
{
  GtkFrame                     parent_instance;

  LigmaExtensionDetailsPrivate *p;
};

struct _LigmaExtensionDetailsClass
{
  GtkFrameClass                parent_class;
};


GType        ligma_extension_details_get_type     (void) G_GNUC_CONST;

GtkWidget  * ligma_extension_details_new          (void);

void         ligma_extension_details_set          (LigmaExtensionDetails *details,
                                                  LigmaExtension        *extension);

#endif /* __LIGMA_EXTENSION_DETAILS_H__ */

