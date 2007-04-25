/* GIMP - The GNU Image Manipulation Program
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

#include "core/gimp.h"
#include "core/gimpparamspecs.h"

#include "gimppdb.h"
#include "gimp-pdb-compat.h"


/*  public functions  */

GParamSpec *
gimp_pdb_compat_param_spec (Gimp           *gimp,
                            GimpPDBArgType  arg_type,
                            const gchar    *name,
                            const gchar    *desc)
{
  GParamSpec *pspec = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  switch (arg_type)
    {
    case GIMP_PDB_INT32:
      pspec = gimp_param_spec_int32 (name, name, desc,
                                     G_MININT32, G_MAXINT32, 0,
                                     G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT16:
      pspec = gimp_param_spec_int16 (name, name, desc,
                                     G_MININT16, G_MAXINT16, 0,
                                     G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT8:
      pspec = gimp_param_spec_int8 (name, name, desc,
                                    0, G_MAXUINT8, 0,
                                    G_PARAM_READWRITE);
      break;

    case GIMP_PDB_FLOAT:
      pspec = g_param_spec_double (name, name, desc,
                                   -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                   G_PARAM_READWRITE);
      break;

    case GIMP_PDB_STRING:
      pspec = gimp_param_spec_string (name, name, desc,
                                      TRUE, TRUE, FALSE,
                                      NULL,
                                      G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT32ARRAY:
      pspec = gimp_param_spec_int32_array (name, name, desc,
                                           G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT16ARRAY:
      pspec = gimp_param_spec_int16_array (name, name, desc,
                                           G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT8ARRAY:
      pspec = gimp_param_spec_int8_array (name, name, desc,
                                          G_PARAM_READWRITE);
      break;

    case GIMP_PDB_FLOATARRAY:
      pspec = gimp_param_spec_float_array (name, name, desc,
                                           G_PARAM_READWRITE);
      break;

    case GIMP_PDB_STRINGARRAY:
      pspec = gimp_param_spec_string_array (name, name, desc,
                                            G_PARAM_READWRITE);
      break;

    case GIMP_PDB_COLOR:
      pspec = gimp_param_spec_rgb (name, name, desc,
                                   TRUE, NULL,
                                   G_PARAM_READWRITE);
      break;

    case GIMP_PDB_REGION:
      break;

    case GIMP_PDB_DISPLAY:
      pspec = gimp_param_spec_display_id (name, name, desc,
                                          gimp, TRUE,
                                          G_PARAM_READWRITE);
      break;

    case GIMP_PDB_IMAGE:
      pspec = gimp_param_spec_image_id (name, name, desc,
                                        gimp, TRUE,
                                        G_PARAM_READWRITE);
      break;

    case GIMP_PDB_LAYER:
      pspec = gimp_param_spec_layer_id (name, name, desc,
                                        gimp, TRUE,
                                        G_PARAM_READWRITE);
      break;

    case GIMP_PDB_CHANNEL:
      pspec = gimp_param_spec_channel_id (name, name, desc,
                                          gimp, TRUE,
                                          G_PARAM_READWRITE);
      break;

    case GIMP_PDB_DRAWABLE:
      pspec = gimp_param_spec_drawable_id (name, name, desc,
                                           gimp, TRUE,
                                           G_PARAM_READWRITE);
      break;

    case GIMP_PDB_SELECTION:
      pspec = gimp_param_spec_selection_id (name, name, desc,
                                            gimp, TRUE,
                                            G_PARAM_READWRITE);
      break;

    case GIMP_PDB_BOUNDARY:
      break;

    case GIMP_PDB_VECTORS:
      pspec = gimp_param_spec_vectors_id (name, name, desc,
                                          gimp, TRUE,
                                          G_PARAM_READWRITE);
      break;

    case GIMP_PDB_PARASITE:
      pspec = gimp_param_spec_parasite (name, name, desc,
                                        G_PARAM_READWRITE);
      break;

    case GIMP_PDB_STATUS:
      pspec = g_param_spec_enum (name, name, desc,
                                 GIMP_TYPE_PDB_STATUS_TYPE,
                                 GIMP_PDB_EXECUTION_ERROR,
                                 G_PARAM_READWRITE);
      break;

    case GIMP_PDB_END:
      break;
    }

  if (! pspec)
    g_warning ("%s: returning NULL for %s (%s)",
               G_STRFUNC, name, gimp_pdb_compat_arg_type_to_string (arg_type));

  return pspec;
}

GType
gimp_pdb_compat_arg_type_to_gtype (GimpPDBArgType  type)
{

  switch (type)
    {
    case GIMP_PDB_INT32:
      return GIMP_TYPE_INT32;

    case GIMP_PDB_INT16:
      return GIMP_TYPE_INT16;

    case GIMP_PDB_INT8:
      return GIMP_TYPE_INT8;

    case GIMP_PDB_FLOAT:
      return G_TYPE_DOUBLE;

    case GIMP_PDB_STRING:
      return G_TYPE_STRING;

    case GIMP_PDB_INT32ARRAY:
      return GIMP_TYPE_INT32_ARRAY;

    case GIMP_PDB_INT16ARRAY:
      return GIMP_TYPE_INT16_ARRAY;

    case GIMP_PDB_INT8ARRAY:
      return GIMP_TYPE_INT8_ARRAY;

    case GIMP_PDB_FLOATARRAY:
      return GIMP_TYPE_FLOAT_ARRAY;

    case GIMP_PDB_STRINGARRAY:
      return GIMP_TYPE_STRING_ARRAY;

    case GIMP_PDB_COLOR:
      return GIMP_TYPE_RGB;

    case GIMP_PDB_REGION:
    case GIMP_PDB_BOUNDARY:
      break;

    case GIMP_PDB_DISPLAY:
      return GIMP_TYPE_DISPLAY_ID;

    case GIMP_PDB_IMAGE:
      return GIMP_TYPE_IMAGE_ID;

    case GIMP_PDB_LAYER:
      return GIMP_TYPE_LAYER_ID;

    case GIMP_PDB_CHANNEL:
      return GIMP_TYPE_CHANNEL_ID;

    case GIMP_PDB_DRAWABLE:
      return GIMP_TYPE_DRAWABLE_ID;

    case GIMP_PDB_SELECTION:
      return GIMP_TYPE_SELECTION_ID;

    case GIMP_PDB_VECTORS:
      return GIMP_TYPE_VECTORS_ID;

    case GIMP_PDB_PARASITE:
      return GIMP_TYPE_PARASITE;

    case GIMP_PDB_STATUS:
      return GIMP_TYPE_PDB_STATUS_TYPE;

    case GIMP_PDB_END:
      break;
    }

  g_warning ("%s: returning G_TYPE_NONE for %d (%s)",
             G_STRFUNC, type, gimp_pdb_compat_arg_type_to_string (type));

  return G_TYPE_NONE;
}

GimpPDBArgType
gimp_pdb_compat_arg_type_from_gtype (GType type)
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
gimp_pdb_compat_arg_type_to_string (GimpPDBArgType type)
{
  const gchar *name;

  if (! gimp_enum_get_value (GIMP_TYPE_PDB_ARG_TYPE, type,
                             &name, NULL, NULL, NULL))
    {
      return  g_strdup_printf ("(PDB type %d unknown)", type);
    }

  return g_strdup (name);
}

void
gimp_pdb_compat_procs_register (GimpPDB           *pdb,
                                GimpPDBCompatMode  compat_mode)
{
  static const struct
  {
    const gchar *old_name;
    const gchar *new_name;
  }
  compat_procs[] =
  {
    { "gimp-blend",                      "gimp-edit-blend"                 },
    { "gimp-brushes-list",               "gimp-brushes-get-list"           },
    { "gimp-bucket-fill",                "gimp-edit-bucket-fill"           },
    { "gimp-channel-delete",             "gimp-drawable-delete"            },
    { "gimp-channel-get-name",           "gimp-drawable-get-name"          },
    { "gimp-channel-get-tattoo",         "gimp-drawable-get-tattoo"        },
    { "gimp-channel-get-visible",        "gimp-drawable-get-visible"       },
    { "gimp-channel-set-name",           "gimp-drawable-set-name"          },
    { "gimp-channel-set-tattoo",         "gimp-drawable-set-tattoo"        },
    { "gimp-channel-set-visible",        "gimp-drawable-set-visible"       },
    { "gimp-color-picker",               "gimp-image-pick-color"           },
    { "gimp-convert-grayscale",          "gimp-image-convert-grayscale"    },
    { "gimp-convert-indexed",            "gimp-image-convert-indexed"      },
    { "gimp-convert-rgb",                "gimp-image-convert-rgb"          },
    { "gimp-crop",                       "gimp-image-crop"                 },
    { "gimp-drawable-bytes",             "gimp-drawable-bpp"               },
    { "gimp-drawable-image",             "gimp-drawable-get-image"         },
    { "gimp-image-active-drawable",      "gimp-image-get-active-drawable"  },
    { "gimp-image-floating-selection",   "gimp-image-get-floating-sel"     },
    { "gimp-layer-delete",               "gimp-drawable-delete"            },
    { "gimp-layer-get-linked",           "gimp-drawable-get-linked"        },
    { "gimp-layer-get-name",             "gimp-drawable-get-name"          },
    { "gimp-layer-get-tattoo",           "gimp-drawable-get-tattoo"        },
    { "gimp-layer-get-visible",          "gimp-drawable-get-visible"       },
    { "gimp-layer-mask",                 "gimp-layer-get-mask"             },
    { "gimp-layer-set-linked",           "gimp-drawable-set-linked"        },
    { "gimp-layer-set-name",             "gimp-drawable-set-name"          },
    { "gimp-layer-set-tattoo",           "gimp-drawable-set-tattoo"        },
    { "gimp-layer-set-visible",          "gimp-drawable-set-visible"       },
    { "gimp-palette-refresh",            "gimp-palettes-refresh"           },
    { "gimp-patterns-list",              "gimp-patterns-get-list"          },
    { "gimp-temp-PDB-name",              "gimp-procedural-db-temp-name"    },
    { "gimp-undo-push-group-end",        "gimp-image-undo-group-end"       },
    { "gimp-undo-push-group-start",      "gimp-image-undo-group-start"     },

    /*  deprecations since 2.0  */
    { "gimp-brushes-get-opacity",        "gimp-context-get-opacity"        },
    { "gimp-brushes-get-paint-mode",     "gimp-context-get-paint-mode"     },
    { "gimp-brushes-set-brush",          "gimp-context-set-brush"          },
    { "gimp-brushes-set-opacity",        "gimp-context-set-opacity"        },
    { "gimp-brushes-set-paint-mode",     "gimp-context-set-paint-mode"     },
    { "gimp-channel-ops-duplicate",      "gimp-image-duplicate"            },
    { "gimp-channel-ops-offset",         "gimp-drawable-offset"            },
    { "gimp-gradients-get-active",       "gimp-context-get-gradient"       },
    { "gimp-gradients-get-gradient",     "gimp-context-get-gradient"       },
    { "gimp-gradients-set-active",       "gimp-context-set-gradient"       },
    { "gimp-gradients-set-gradient",     "gimp-context-set-gradient"       },
    { "gimp-image-get-cmap",             "gimp-image-get-colormap"         },
    { "gimp-image-set-cmap",             "gimp-image-set-colormap"         },
    { "gimp-palette-get-background",     "gimp-context-get-background"     },
    { "gimp-palette-get-foreground",     "gimp-context-get-foreground"     },
    { "gimp-palette-set-background",     "gimp-context-set-background"     },
    { "gimp-palette-set-default-colors", "gimp-context-set-default-colors" },
    { "gimp-palette-set-foreground",     "gimp-context-set-foreground"     },
    { "gimp-palette-swap-colors",        "gimp-context-swap-colors"        },
    { "gimp-palettes-set-palette",       "gimp-context-set-palette"        },
    { "gimp-patterns-set-pattern",       "gimp-context-set-pattern"        },
    { "gimp-selection-clear",            "gimp-selection-none"             },

    /*  deprecations since 2.2  */
    { "gimp-layer-get-preserve-trans",   "gimp-layer-get-lock-alpha"       },
    { "gimp-layer-set-preserve-trans",   "gimp-layer-set-lock-alpha"       }
  };

  g_return_if_fail (GIMP_IS_PDB (pdb));

  if (compat_mode != GIMP_PDB_COMPAT_OFF)
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (compat_procs); i++)
        gimp_pdb_register_compat_proc_name (pdb,
                                            compat_procs[i].old_name,
                                            compat_procs[i].new_name);
    }
}
