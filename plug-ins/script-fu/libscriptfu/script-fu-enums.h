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

#ifndef __SCRIPT_FU_ENUMS_H__
#define __SCRIPT_FU_ENUMS_H__

/* Note these are C names with underbar.
 * The Scheme names are usually the same with hyphen substituted for underbar.
 */

/*  script-fu argument types  */
typedef enum
{
  SF_IMAGE = 0,
  SF_DRAWABLE,
  SF_LAYER,
  SF_CHANNEL,
  SF_VECTORS,
  SF_COLOR,
  SF_TOGGLE,
  SF_STRING,
  SF_ADJUSTMENT,
  SF_FONT,
  SF_PATTERN,
  SF_BRUSH,
  SF_GRADIENT,
  SF_FILENAME,
  SF_DIRNAME,
  SF_OPTION,
  SF_PALETTE,
  SF_TEXT,
  SF_ENUM,
  SF_DISPLAY
} SFArgType;

typedef enum
{
  SF_SLIDER = 0,
  SF_SPINNER
} SFAdjustmentType;

/* This enum is local to ScriptFu
 * but the notion is general to other plugins.
 *
 * A GimpImageProcedure has drawable  arity > 1.
 * A GimpProcedure often does not take any drawables, i.e. arity zero.
 * Some GimpProcedure may take drawables i.e. arity > 0,
 * but the procedure's menu item is always sensitive,
 * and the drawable can be chosen in the plugin's dialog.
 *
 * Script author does not use SF-NO-DRAWABLE, for now.
 *
 * Scripts of class GimpProcedure are declared by script-fu-register.
 * Their GUI is handled by ScriptFu, script-fu-interface.c
 * An author does not declare drawable_arity.
 *
 * Scripts of class GimpImageProcedure are declared by script-fu-register-filter.
 * Their GUI is handled by libgimpui, GimpProcedureDialog.
 * Their drawable_arity is declared by the author of the script.
 *
 * For backward compatibility, GIMP deprecates but allows PDB procedures
 * to take a single drawable, and sets their sensitivity automatically.
 * Their drawable_arity is inferred by ScriptFu.
 * FUTURE insist that an author use script-fu-register-filter (not script-fu-register)
 * for GimpImageProcedure taking image and one or more drawables.
 */
typedef enum
{
  SF_NO_DRAWABLE = 0,       /* GimpProcedure. */
  SF_ONE_DRAWABLE,          /* GimpImageProcedure, but only process one layer */
  SF_ONE_OR_MORE_DRAWABLE,  /* GimpImageProcedure, multilayer capable */
  SF_TWO_OR_MORE_DRAWABLE,  /* GimpImageProcedure, requires at least two drawables. */
} SFDrawableArity;

#endif /*  __SCRIPT_FU_ENUMS__  */
