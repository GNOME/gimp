/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "pdb-types.h"

#include "core/gimpparamspecs.h"

#include "gimpargument.h"


/*  public functions  */

void
gimp_argument_init_compat (GValue         *value,
                           GimpPDBArgType  type)
{
  g_return_if_fail (value != NULL);

  switch (type)
    {
    case GIMP_PDB_INT32:
      g_value_init (value, GIMP_TYPE_INT32);
      break;

    case GIMP_PDB_INT16:
      g_value_init (value, GIMP_TYPE_INT16);
      break;

    case GIMP_PDB_INT8:
      g_value_init (value, GIMP_TYPE_INT8);
      break;

    case GIMP_PDB_FLOAT:
      g_value_init (value, G_TYPE_DOUBLE);
      break;

    case GIMP_PDB_STRING:
      g_value_init (value, G_TYPE_STRING);
      break;

    case GIMP_PDB_INT32ARRAY:
      g_value_init (value, GIMP_TYPE_INT32_ARRAY);
      break;

    case GIMP_PDB_INT16ARRAY:
      g_value_init (value, GIMP_TYPE_INT16_ARRAY);
      break;

    case GIMP_PDB_INT8ARRAY:
      g_value_init (value, GIMP_TYPE_INT8_ARRAY);
      break;

    case GIMP_PDB_FLOATARRAY:
      g_value_init (value, GIMP_TYPE_FLOAT_ARRAY);
      break;

    case GIMP_PDB_STRINGARRAY:
      g_value_init (value, GIMP_TYPE_STRING_ARRAY);
      break;

    case GIMP_PDB_COLOR:
      g_value_init (value, GIMP_TYPE_RGB);
      break;

    case GIMP_PDB_REGION:
    case GIMP_PDB_BOUNDARY:
      break;

    case GIMP_PDB_DISPLAY:
      g_value_init (value, GIMP_TYPE_DISPLAY_ID);
      break;

    case GIMP_PDB_IMAGE:
      g_value_init (value, GIMP_TYPE_IMAGE_ID);
      break;

    case GIMP_PDB_LAYER:
      g_value_init (value, GIMP_TYPE_LAYER_ID);
      break;

    case GIMP_PDB_CHANNEL:
      g_value_init (value, GIMP_TYPE_CHANNEL_ID);
      break;

    case GIMP_PDB_DRAWABLE:
      g_value_init (value, GIMP_TYPE_DRAWABLE_ID);
      break;

    case GIMP_PDB_SELECTION:
      g_value_init (value, GIMP_TYPE_SELECTION_ID);
      break;

    case GIMP_PDB_VECTORS:
      g_value_init (value, GIMP_TYPE_VECTORS_ID);
      break;

    case GIMP_PDB_PARASITE:
      g_value_init (value, GIMP_TYPE_PARASITE);
      break;

    case GIMP_PDB_STATUS:
      g_value_init (value, GIMP_TYPE_PDB_STATUS_TYPE);
      break;

    case GIMP_PDB_END:
      break;
    }
}

GimpPDBArgType
gimp_argument_type_to_pdb_arg_type (GType type)
{
  static GQuark  pdb_type_quark = 0;
  GimpPDBArgType pdb_type;

  if (! pdb_type_quark)
    {
      struct
      {
        GType          g_type;
        GimpPDBArgType pdb_type;
      }
      type_mapping[] =
      {
        { GIMP_TYPE_INT32,           GIMP_PDB_INT32       },
        { G_TYPE_INT,                GIMP_PDB_INT32       },
        { G_TYPE_UINT,               GIMP_PDB_INT32       },
        { G_TYPE_ENUM,               GIMP_PDB_INT32       },
        { G_TYPE_BOOLEAN,            GIMP_PDB_INT32       },

        { GIMP_TYPE_INT16,           GIMP_PDB_INT16       },
        { GIMP_TYPE_INT8,            GIMP_PDB_INT8        },
        { G_TYPE_DOUBLE,             GIMP_PDB_FLOAT       },

        { G_TYPE_STRING,             GIMP_PDB_STRING      },

        { GIMP_TYPE_INT32_ARRAY,     GIMP_PDB_INT32ARRAY  },
        { GIMP_TYPE_INT16_ARRAY,     GIMP_PDB_INT16ARRAY  },
        { GIMP_TYPE_INT8_ARRAY,      GIMP_PDB_INT8ARRAY   },
        { GIMP_TYPE_FLOAT_ARRAY,     GIMP_PDB_FLOATARRAY  },
        { GIMP_TYPE_STRING_ARRAY,    GIMP_PDB_STRINGARRAY },

        { GIMP_TYPE_RGB,             GIMP_PDB_COLOR       },

        { GIMP_TYPE_DISPLAY_ID,      GIMP_PDB_DISPLAY     },
        { GIMP_TYPE_IMAGE_ID,        GIMP_PDB_IMAGE       },
        { GIMP_TYPE_LAYER_ID,        GIMP_PDB_LAYER       },
        { GIMP_TYPE_CHANNEL_ID,      GIMP_PDB_CHANNEL     },
        { GIMP_TYPE_DRAWABLE_ID,     GIMP_PDB_DRAWABLE    },
        { GIMP_TYPE_SELECTION_ID,    GIMP_PDB_SELECTION   },
        { GIMP_TYPE_LAYER_MASK_ID,   GIMP_PDB_CHANNEL     },
        { GIMP_TYPE_VECTORS_ID,      GIMP_PDB_VECTORS     },

        { GIMP_TYPE_PARASITE,        GIMP_PDB_PARASITE    },

        { GIMP_TYPE_PDB_STATUS_TYPE, GIMP_PDB_STATUS      }
      };

      gint i;

      pdb_type_quark = g_quark_from_static_string ("gimp-pdb-type");

      for (i = 0; i < G_N_ELEMENTS (type_mapping); i++)
        g_type_set_qdata (type_mapping[i].g_type, pdb_type_quark,
                          GINT_TO_POINTER (type_mapping[i].pdb_type));
    }

  pdb_type = GPOINTER_TO_INT (g_type_get_qdata (type, pdb_type_quark));

#if 0
  g_printerr ("%s: arg_type = %p (%s)  ->  %d (%s)\n",
              G_STRFUNC,
              (gpointer) type, g_type_name (type),
              pdb_type, gimp_pdb_arg_type_to_string (pdb_type));
#endif

  return pdb_type;
}

gchar *
gimp_pdb_arg_type_to_string (GimpPDBArgType type)
{
  const gchar *name;

  if (! gimp_enum_get_value (GIMP_TYPE_PDB_ARG_TYPE, type,
                             &name, NULL, NULL, NULL))
    {
      return  g_strdup_printf ("(PDB type %d unknown)", type);
    }

  return g_strdup (name);
}
