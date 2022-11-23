/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"
#include "tinyscheme/scheme-private.h"
#include "script-fu-compat.h"

/*
 * Make some PDB procedure names deprecated in ScriptFu.
 * Until such time as we turn deprecation off and make them obsolete.
 *
 * This only makes them deprecated in ScriptFu.
 */


/* private */

static const struct
{
  const gchar *old_name;
  const gchar *new_name;
}
compat_procs[] =
{
  /*
   * deprecations since 2.99
   *
   * With respect to ScriptFu,
   * the old names are *obsolete in the PDB* (as of this writing.)
   * That is, they don't exist in the PDB with the same signature.
   * There is no "compatibility" procedure in the PDB.
   *
   * With respect to Python using GI, some old names are *NOT* obsolete.
   * (Where "some" means those dealing with ID.)
   * I.E. Ligma.Image.is_valid() exists but takes a GObject *, not an int ID.
   *
   * Original data was constructed more or less by hand, partially automated.
   */
   { "ligma-brightness-contrast"               , "ligma-drawable-brightness-contrast"      },
   { "ligma-brushes-get-brush"                 , "ligma-context-get-brush"                 },
   { "ligma-drawable-is-channel"               , "ligma-item-id-is-channel"                },
   { "ligma-drawable-is-layer"                 , "ligma-item-id-is-layer"                  },
   { "ligma-drawable-is-layer-mask"            , "ligma-item-id-is-layer-mask"             },
   { "ligma-drawable-is-text-layer"            , "ligma-item-id-is-text-layer"             },
   { "ligma-drawable-is-valid"                 , "ligma-item-id-is-valid"                  },
   { "ligma-drawable-transform-2d"             , "ligma-item-transform-2d"                 },
   { "ligma-drawable-transform-flip"           , "ligma-item-transform-flip"               },
   { "ligma-drawable-transform-flip-simple"    , "ligma-item-transform-flip-simple"        },
   { "ligma-drawable-transform-matrix"         , "ligma-item-transform-matrix"             },
   { "ligma-drawable-transform-perspective"    , "ligma-item-transform-perspective"        },
   { "ligma-drawable-transform-rotate"         , "ligma-item-transform-rotate"             },
   { "ligma-drawable-transform-rotate-simple"  , "ligma-item-transform-rotate-simple"      },
   { "ligma-drawable-transform-scale"          , "ligma-item-transform-scale"              },
   { "ligma-drawable-transform-shear"          , "ligma-item-transform-shear"              },
   { "ligma-display-is-valid"                  , "ligma-display-id-is-valid"               },
   { "ligma-image-is-valid"                    , "ligma-image-id-is-valid"                 },
   { "ligma-item-is-channel"                   , "ligma-item-id-is-channel"                },
   { "ligma-item-is-drawable"                  , "ligma-item-id-is-drawable"               },
   { "ligma-item-is-layer"                     , "ligma-item-id-is-layer"                  },
   { "ligma-item-is-layer-mask"                , "ligma-item-id-is-layer-mask"             },
   { "ligma-item-is-selection"                 , "ligma-item-id-is-selection"              },
   { "ligma-item-is-text-layer"                , "ligma-item-id-is-text-layer"             },
   { "ligma-item-is-valid"                     , "ligma-item-id-is-valid"                  },
   { "ligma-item-is-vectors"                   , "ligma-item-id-is-vectors"                },
   { "ligma-procedural-db-dump"                , "ligma-pdb-dump"                          },
   { "ligma-procedural-db-get-data"            , "ligma-pdb-get-data"                      },
   { "ligma-procedural-db-set-data"            , "ligma-pdb-set-data"                      },
   { "ligma-procedural-db-get-data-size"       , "ligma-pdb-get-data-size"                 },
   { "ligma-procedural-db-proc-arg"            , "ligma-pdb-get-proc-argument"             },
   { "ligma-procedural-db-proc-info"           , "ligma-pdb-get-proc-info"                 },
   { "ligma-procedural-db-proc-val"            , "ligma-pdb-get-proc-return-value"         },
   { "ligma-procedural-db-proc-exists"         , "ligma-pdb-proc-exists"                   },
   { "ligma-procedural-db-query"               , "ligma-pdb-query"                         },
   { "ligma-procedural-db-temp-name"           , "ligma-pdb-temp-name"                     },
   { "ligma-image-get-exported-uri"            , "ligma-image-get-exported-file"           },
   { "ligma-image-get-imported-uri"            , "ligma-image-get-imported-file"           },
   { "ligma-image-get-xcf-uri"                 , "ligma-image-get-xcf-file"                },
   { "ligma-image-get-filename"                , "ligma-image-get-file"                    },
   { "ligma-image-set-filename"                , "ligma-image-set-file"                    },
   { "ligma-plugin-menu-register"              , "ligma-pdb-add-proc-menu-path"            },
   { "ligma-plugin-get-pdb-error-handler"      , "ligma-plug-in-get-pdb-error-handler"     },
   { "ligma-plugin-help-register"              , "ligma-plug-in-help-register"             },
   { "ligma-plugin-menu-branch-register"       , "ligma-plug-in-menu-branch-register"      },
   { "ligma-plugin-set-pdb-error-handler"      , "ligma-plug-in-set-pdb-error-handler"     },
   { "ligma-plugins-query"                     , "ligma-plug-ins-query"                    },
   { "file-gtm-save"                          , "file-html-table-save"                   },
   { "python-fu-histogram-export"             , "histogram-export"                       },
   { "python-fu-gradient-save-as-css"         , "gradient-save-as-css"                   }
};

static gchar *empty_string = "";


static void
define_deprecated_scheme_func (const char   *old_name,
                               const char   *new_name,
                               const scheme *sc)
{
  gchar *buff;

  /* Creates a definition in Scheme of a function that calls a PDB procedure.
   *
   * The magic below that makes it deprecated:
   * - the "--ligma-proc-db-call"
   * - defining under the old_name but calling the new_name

   * See scheme-wrapper.c, where this was copied from.
   * But here creates scheme definition of old_name
   * that calls a PDB procedure of a different name, new_name.
   *
   * As functional programming is: eval(define(apply f)).
   * load_string is more typically called eval().
   */
  buff = g_strdup_printf (" (define (%s . args)"
                          " (apply --ligma-proc-db-call \"%s\" args))",
                          old_name, new_name);

  sc->vptr->load_string ((scheme *) sc, buff);

  g_free (buff);
}


/*  public functions  */

/* Define Scheme functions whose name is old name
 * that call compatible PDB procedures whose name is new name.
 * Define into the lisp machine.

 * Compatible means: signature same, semantics same.
 * The new names are not "compatibility" procedures, they are the new procedures.
 *
 * This can overwrite existing definitions in the lisp machine.
 * If the PDB has the old name already
 * (if a compatibility procedure is defined in the PDB
 * or the old name exists with a different signature)
 * and ScriptFu already defined functions for procedures of the PDB,
 * this will overwrite the ScriptFu definition,
 * but produce the same overall effect.
 * The definition here will not call the old name PDB procedure,
 * but from ScriptFu call the new name PDB procedure.
 */
void
define_compat_procs (scheme *sc)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (compat_procs); i++)
    {
      define_deprecated_scheme_func (compat_procs[i].old_name,
                                     compat_procs[i].new_name,
                                     sc);
    }
}

/* Return empty string or old_name */
/* Used for a warning message */
const gchar *
deprecated_name_for (const char *new_name)
{
  gint i;
  const gchar * result = empty_string;

  /* search values of dictionary/map. */
  for (i = 0; i < G_N_ELEMENTS (compat_procs); i++)
    {
      if (strcmp (compat_procs[i].new_name, new_name) == 0)
        {
          result = compat_procs[i].old_name;
          break;
        }
    }
  return result;

}

/* Not used.
 * Keep for future implementation: catch "undefined symbol" from lisp machine.
 */
gboolean
is_deprecated (const char *old_name)
{
  gint i;
  gboolean result = FALSE;

  /* search keys of dictionary/map. */
  for (i = 0; i < G_N_ELEMENTS (compat_procs); i++)
  {
    if (strcmp (compat_procs[i].old_name, old_name) == 0)
      {
        result = TRUE;
        break;
      }
  }
  return result;
}
