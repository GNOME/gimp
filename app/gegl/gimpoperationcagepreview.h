/* GIMP - The GNU Image Manipulation Program
 *
 * gimpoperationcagepreview.h
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_OPERATION_CAGE_PREVIEW_H__
#define __GIMP_OPERATION_CAGE_PREVIEW_H__

#include <gegl-plugin.h>
#include <operation/gegl-operation-composer.h>

enum
{
  GIMP_OPERATION_CAGE_PREVIEW_PROP_0,
  GIMP_OPERATION_CAGE_PREVIEW_PROP_CONFIG
};

#define GIMP_TYPE_OPERATION_CAGE_PREVIEW            (gimp_operation_cage_preview_get_type ())
#define GIMP_OPERATION_CAGE_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_CAGE_PREVIEW, GimpOperationCagePreview))
#define GIMP_OPERATION_CAGE_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_CAGE_PREVIEW, GimpOperationCagePreviewClass))
#define GIMP_IS_OPERATION_CAGE_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_CAGE_PREVIEW))
#define GIMP_IS_OPERATION_CAGE_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_CAGE_PREVIEW))
#define GIMP_OPERATION_CAGE_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_CAGE_PREVIEW, GimpOperationCagePreviewClass))


typedef struct _GimpOperationCagePreviewClass GimpOperationCagePreviewClass;

struct _GimpOperationCagePreview
{
  GeglOperationComposer  parent_instance;

  GimpCageConfig       *config;
};

struct _GimpOperationCagePreviewClass
{
  GeglOperationComposerClass  parent_class;
};


GType   gimp_operation_cage_preview_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_OPERATION_CAGE_PREVIEW_H__ */
