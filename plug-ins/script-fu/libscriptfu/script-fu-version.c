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

#include <libgimp/gimp.h>

#include "script-fu-version.h"

/* Flag indicating version 3 binding to the PDB.
 * while marshalling return values from PDB:
 *   - not wrap solitary values in list
 *   - bind c boolean to scheme truth values
 *
 * Set by a script calling (script-fu-use-v3).
 * Cleared by a script calling (script-fu-use-v2).
 *
 * Initial, default state of all ScriptFu tools is interpret v2 dialect.
 *
 * New-style scripts call (script-fu-use-v3) at top level.
 * Ends with process termination of the interpreter.
 *
 * Old-style scripts call in run function, but this is discouraged.
 * Ends after extension-script-fu finishes interpretation of current command.
 *
 * Affects interpretation for the duration of the current interpreter process.
 * !!! All script interpreted subsequently in the current process,
 * especially in called PDB procedures that are themselves scripts,
 * must use v3 binding to PDB.
 * Note most old script plugins in the PDB use v2 that is affected by this flag.
 * Note a very few routines in script-fu-util.scm call the PDB using v2 binding.
 * So a script should not call them while this flag is set.
 */
static gboolean language_version_is_3 = FALSE;

void
begin_interpret_v3_dialect (void)
{
  language_version_is_3 = TRUE;
}

void
begin_interpret_v2_dialect (void)
{
  language_version_is_3 = FALSE;
}

/* the default dialect is v2 */
void
begin_interpret_default_dialect (void)
{
  /* the default dialect is v2 */
  begin_interpret_v2_dialect ();
}

gboolean
is_interpret_v3_dialect (void)
{
  return language_version_is_3;
}
