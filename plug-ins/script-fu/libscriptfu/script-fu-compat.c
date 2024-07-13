/* GIMP - The GNU Image Manipulation Program
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
   * I.E. Gimp.Image.is_valid() exists but takes a GObject *, not an int ID.
   *
   * Original data was constructed more or less by hand, partially automated.
   */
   { "gimp-brightness-contrast"               , "gimp-drawable-brightness-contrast"      },
   { "gimp-brushes-get-brush"                 , "gimp-context-get-brush"                 },
   { "gimp-drawable-is-channel"               , "gimp-item-id-is-channel"                },
   { "gimp-drawable-is-layer"                 , "gimp-item-id-is-layer"                  },
   { "gimp-drawable-is-layer-mask"            , "gimp-item-id-is-layer-mask"             },
   { "gimp-drawable-is-text-layer"            , "gimp-item-id-is-text-layer"             },
   { "gimp-drawable-is-valid"                 , "gimp-item-id-is-valid"                  },
   { "gimp-drawable-transform-2d"             , "gimp-item-transform-2d"                 },
   { "gimp-drawable-transform-flip"           , "gimp-item-transform-flip"               },
   { "gimp-drawable-transform-flip-simple"    , "gimp-item-transform-flip-simple"        },
   { "gimp-drawable-transform-matrix"         , "gimp-item-transform-matrix"             },
   { "gimp-drawable-transform-perspective"    , "gimp-item-transform-perspective"        },
   { "gimp-drawable-transform-rotate"         , "gimp-item-transform-rotate"             },
   { "gimp-drawable-transform-rotate-simple"  , "gimp-item-transform-rotate-simple"      },
   { "gimp-drawable-transform-scale"          , "gimp-item-transform-scale"              },
   { "gimp-drawable-transform-shear"          , "gimp-item-transform-shear"              },
   { "gimp-display-is-valid"                  , "gimp-display-id-is-valid"               },
   { "gimp-image-is-valid"                    , "gimp-image-id-is-valid"                 },
   { "gimp-image-freeze-vectors"              , "gimp-image-freeze-paths"                },
   { "gimp-image-get-vectors"                 , "gimp-image-get-paths"                   },
   { "gimp-image-get-selected-vectors"        , "gimp-image-get-selected-paths"          },
   { "gimp-image-set-selected-vectors"        , "gimp-image-set-selected-paths"          },
   { "gimp-image-thaw-vectors"                , "gimp-image-thaw-paths"                  },
   { "gimp-item-is-channel"                   , "gimp-item-id-is-channel"                },
   { "gimp-item-is-drawable"                  , "gimp-item-id-is-drawable"               },
   { "gimp-item-is-layer"                     , "gimp-item-id-is-layer"                  },
   { "gimp-item-is-layer-mask"                , "gimp-item-id-is-layer-mask"             },
   { "gimp-item-is-selection"                 , "gimp-item-id-is-selection"              },
   { "gimp-item-is-text-layer"                , "gimp-item-id-is-text-layer"             },
   { "gimp-item-is-valid"                     , "gimp-item-id-is-valid"                  },
   { "gimp-item-is-vectors"                   , "gimp-item-id-is-path"                   },
   { "gimp-item-id-is-vectors"                , "gimp-item-id-is-path"                   },
   /* TODO more -vectors- => -path- or -paths- */
   /* TODO more layer-group => group-layer */
   { "gimp-layer-group-new"                   , "gimp-group-layer-new"                   },
   { "gimp-procedural-db-dump"                , "gimp-pdb-dump"                          },
   { "gimp-procedural-db-get-data"            , "gimp-pdb-get-data"                      },
   { "gimp-procedural-db-set-data"            , "gimp-pdb-set-data"                      },
   { "gimp-procedural-db-get-data-size"       , "gimp-pdb-get-data-size"                 },
   { "gimp-procedural-db-proc-arg"            , "gimp-pdb-get-proc-argument"             },
   { "gimp-procedural-db-proc-info"           , "gimp-pdb-get-proc-info"                 },
   { "gimp-procedural-db-proc-val"            , "gimp-pdb-get-proc-return-value"         },
   { "gimp-procedural-db-proc-exists"         , "gimp-pdb-proc-exists"                   },
   { "gimp-procedural-db-query"               , "gimp-pdb-query"                         },
   { "gimp-procedural-db-temp-name"           , "gimp-pdb-temp-name"                     },
   { "gimp-image-get-exported-uri"            , "gimp-image-get-exported-file"           },
   { "gimp-image-get-imported-uri"            , "gimp-image-get-imported-file"           },
   { "gimp-image-get-xcf-uri"                 , "gimp-image-get-xcf-file"                },
   { "gimp-image-get-filename"                , "gimp-image-get-file"                    },
   { "gimp-image-set-filename"                , "gimp-image-set-file"                    },
   { "gimp-plugin-menu-register"              , "gimp-pdb-add-proc-menu-path"            },
   { "gimp-plugin-get-pdb-error-handler"      , "gimp-plug-in-get-pdb-error-handler"     },
   { "gimp-plugin-help-register"              , "gimp-plug-in-help-register"             },
   { "gimp-plugin-menu-branch-register"       , "gimp-plug-in-menu-branch-register"      },
   { "gimp-plugin-set-pdb-error-handler"      , "gimp-plug-in-set-pdb-error-handler"     },
   { "gimp-plugins-query"                     , "gimp-plug-ins-query"                    },
   { "file-gtm-save"                          , "file-html-table-export"                 },
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
   * - the "--gimp-proc-db-call"
   * - defining under the old_name but calling the new_name

   * See scheme-wrapper.c, where this was copied from.
   * But here creates scheme definition of old_name
   * that calls a PDB procedure of a different name, new_name.
   *
   * As functional programming is: eval(define(apply f)).
   * load_string is more typically called eval().
   */
  buff = g_strdup_printf (" (define (%s . args)"
                          " (apply --gimp-proc-db-call \"%s\" args))",
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
