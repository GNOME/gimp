/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdataselect.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpdataselect.h"


enum
{
  PROP_0,
  PROP_EDIT_FUNC
};


static void   gimp_data_select_class_init   (GimpDataSelectClass *klass);

static void   gimp_data_select_set_property (GObject             *object,
                                             guint                property_id,
                                             const GValue        *value,
                                             GParamSpec          *pspec);


static GimpPdbDialogClass *parent_class = NULL;


GType
gimp_data_select_get_type (void)
{
  static GType dialog_type = 0;

  if (! dialog_type)
    {
      static const GTypeInfo dialog_info =
      {
        sizeof (GimpDataSelectClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_data_select_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpDataSelect),
        0,              /* n_preallocs    */
        NULL,           /* instance_init  */
      };

      dialog_type = g_type_register_static (GIMP_TYPE_PDB_DIALOG,
                                            "GimpDataSelect",
                                            &dialog_info,
                                            G_TYPE_FLAG_ABSTRACT);
    }

  return dialog_type;
}

static void
gimp_data_select_class_init (GimpDataSelectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_data_select_set_property;

  g_object_class_install_property (object_class, PROP_EDIT_FUNC,
                                   g_param_spec_pointer ("edit-func",
                                                         NULL, NULL,
                                                         G_PARAM_WRITABLE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_data_select_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpDataSelect *select = GIMP_DATA_SELECT (object);

  switch (property_id)
    {
    case PROP_EDIT_FUNC:
      select->edit_func = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
