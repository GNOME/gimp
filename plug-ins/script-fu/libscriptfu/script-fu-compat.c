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


/* About deprecation mechanisms.
 *
 * The mechanism here is only for ScriptFu.
 * This prints a warning to the console, but redirects the call
 * to the new name of a PDB procedure.
 *
 * Another mechanism is to define a compatibility procedure in the PDB.
 *
 * Another mechanism is in Gimp core PDB, which also aliases.
 * When a procedure is aliased there, it does not need to be aliased here.
 * The mechanism here lets ScriptFu alias PDB procedures that core PDB does not
 * (when script maintenance lags core development.)
 *
 * Another mechanism defines an alias in pure Scheme rather than in C as here.
 * See for example PDB-compat-v2.scm.
 * That script must be explicitly loaded by a script.
 *
 * In this mechanism, aliases are always loaded by the interpreter, not a script.
 * This mechanism also gives a warning that the name is deprecated.
 * This mechanism also is more independent of definition time.
 * The aliases here could be done in a Scheme script,
 * but only if loaded after PDB procedures were defined into the interpreter.
 */

compat_procs[] =
{
  /*  deprecations since 3.0  */
  /* Intended to be empty, for major release. */

  /* Template:
  { "gimp-brightness-contrast"               , "gimp-drawable-brightness-contrast"      },
  */
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
