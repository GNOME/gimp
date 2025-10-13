/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprocedure-params.c
 * Copyright (C) 2019  Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include "gimp.h"

#include "libgimpbase/gimpversion-private.h"

#include "gimpprocedure-params.h"

#include "libgimp-intl.h"


/* It looks like for some generated widgets, we consider the nick or
 * blurb as Pangom markup, and in others, not. We should be consistent
 * and properly document our choice in function docs. The question is
 * basically: do we want to allow text styling for generated widgets?
 *
 * - If yes, it is up to plug-in developers to make sure they don't
 *   break markup, and for us to filter out any markup when needed.
 * - If no, we will have to use markup variants for setting labels and
 *   tooltips, and run g_markup_escape_text() when we need to integrate
 *   the text into larger markup contents.
 *
 * For GIMP 3, let's stick to the statement that nick and blurb is
 * normal (non-markup) text and review in GIMP 4.
 */
GIMP_WARNING_API_BREAK("Should nick and/or blurb be in Pango markup?")

/**
 * gimp_procedure_add_boolean_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new boolean argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_boolean_argument (GimpProcedure *procedure,
                                     const gchar   *name,
                                     const gchar   *nick,
                                     const gchar   *blurb,
                                     gboolean       value,
                                     GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                g_param_spec_boolean (name, nick, blurb,
                                                      value, flags));
}

/**
 * gimp_procedure_add_boolean_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new boolean auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_boolean_aux_argument (GimpProcedure *procedure,
                                         const gchar   *name,
                                         const gchar   *nick,
                                         const gchar   *blurb,
                                         gboolean       value,
                                         GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    g_param_spec_boolean (name, nick, blurb,
                                                          value, flags));
}

/**
 * gimp_procedure_add_boolean_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new boolean return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_boolean_return_value (GimpProcedure *procedure,
                                         const gchar   *name,
                                         const gchar   *nick,
                                         const gchar   *blurb,
                                         gboolean       value,
                                         GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    g_param_spec_boolean (name, nick, blurb,
                                                          value, flags));
}

/**
 * gimp_procedure_add_int_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @min:         the minimum value for this argument
 * @max:         the maximum value for this argument
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new integer argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_int_argument (GimpProcedure *procedure,
                                 const gchar   *name,
                                 const gchar   *nick,
                                 const gchar   *blurb,
                                 gint           min,
                                 gint           max,
                                 gint           value,
                                 GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                g_param_spec_int (name, nick, blurb,
                                                  min, max, value,
                                                  flags));
}

/**
 * gimp_procedure_add_int_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @min:         the minimum value for this argument
 * @max:         the maximum value for this argument
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new integer auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_int_aux_argument (GimpProcedure *procedure,
                                     const gchar   *name,
                                     const gchar   *nick,
                                     const gchar   *blurb,
                                     gint           min,
                                     gint           max,
                                     gint           value,
                                     GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    g_param_spec_int (name, nick, blurb,
                                                      min, max, value,
                                                      flags));
}

/**
 * gimp_procedure_add_int_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @min:         the minimum value for this argument
 * @max:         the maximum value for this argument
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new integer return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_int_return_value (GimpProcedure *procedure,
                                     const gchar   *name,
                                     const gchar   *nick,
                                     const gchar   *blurb,
                                     gint           min,
                                     gint           max,
                                     gint           value,
                                     GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    g_param_spec_int (name, nick, blurb,
                                                      min, max, value,
                                                      flags));
}

/**
 * gimp_procedure_add_uint_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @min:         the minimum value for this argument
 * @max:         the maximum value for this argument
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new unsigned integer argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_uint_argument (GimpProcedure *procedure,
                                  const gchar   *name,
                                  const gchar   *nick,
                                  const gchar   *blurb,
                                  guint          min,
                                  guint          max,
                                  guint          value,
                                  GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                g_param_spec_uint (name, nick, blurb,
                                                   min, max, value,
                                                   flags));
}

/**
 * gimp_procedure_add_uint_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @min:         the minimum value for this argument
 * @max:         the maximum value for this argument
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new unsigned integer auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_uint_aux_argument (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      guint          min,
                                      guint          max,
                                      guint          value,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    g_param_spec_uint (name, nick, blurb,
                                                       min, max, value,
                                                       flags));
}

/**
 * gimp_procedure_add_uint_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @min:         the minimum value for this argument
 * @max:         the maximum value for this argument
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new unsigned integer return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_uint_return_value (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      guint          min,
                                      guint          max,
                                      guint          value,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    g_param_spec_uint (name, nick, blurb,
                                                       min, max, value,
                                                       flags));
}

/**
 * gimp_procedure_add_unit_argument:
 * @procedure:    the #GimpProcedure.
 * @name:         the name of the argument to be created.
 * @nick:         the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @show_pixels:  whether to allow pixels as a valid option
 * @show_percent: whether to allow percent as a valid option
 * @value:        the default value.
 * @flags:        argument flags.
 *
 * Add a new #GimpUnit argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_unit_argument (GimpProcedure *procedure,
                                  const gchar   *name,
                                  const gchar   *nick,
                                  const gchar   *blurb,
                                  gboolean       show_pixels,
                                  gboolean       show_percent,
                                  GimpUnit      *value,
                                  GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_unit (name, nick, blurb,
                                                      show_pixels, show_percent,
                                                      value, flags));
}

/**
 * gimp_procedure_add_unit_aux_argument:
 * @procedure:    the #GimpProcedure.
 * @name:         the name of the argument to be created.
 * @nick:         the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @show_pixels:  whether to allow pixels as a valid option
 * @show_percent: whether to allow percent as a valid option
 * @value:        the default value.
 * @flags:        argument flags.
 *
 * Add a new #GimpUnit auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_unit_aux_argument (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      gboolean       show_pixels,
                                      gboolean       show_percent,
                                      GimpUnit      *value,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_unit (name, nick, blurb,
                                                          show_pixels, show_percent,
                                                          value, flags));
}

/**
 * gimp_procedure_add_unit_return_value:
 * @procedure:    the #GimpProcedure.
 * @name:         the name of the argument to be created.
 * @nick:         the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @show_pixels:  whether to allow pixels as a valid option
 * @show_percent: whether to allow percent as a valid option
 * @value:        the default value.
 * @flags:        argument flags.
 *
 * Add a new #GimpUnit return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_unit_return_value (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      gboolean       show_pixels,
                                      gboolean       show_percent,
                                      GimpUnit      *value,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_unit (name, nick, blurb,
                                                          show_pixels, show_percent,
                                                          value, flags));
}

/**
 * gimp_procedure_add_double_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @min:         the minimum value for this argument
 * @max:         the maximum value for this argument
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new floating-point in double precision argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_double_argument (GimpProcedure *procedure,
                                    const gchar   *name,
                                    const gchar   *nick,
                                    const gchar   *blurb,
                                    gdouble        min,
                                    gdouble        max,
                                    gdouble        value,
                                    GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                g_param_spec_double (name, nick, blurb,
                                                     min, max, value,
                                                     flags));
}

/**
 * gimp_procedure_add_double_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @min:         the minimum value for this argument
 * @max:         the maximum value for this argument
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new floating-point in double precision auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_double_aux_argument (GimpProcedure *procedure,
                                        const gchar   *name,
                                        const gchar   *nick,
                                        const gchar   *blurb,
                                        gdouble        min,
                                        gdouble        max,
                                        gdouble        value,
                                        GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    g_param_spec_double (name, nick, blurb,
                                                         min, max, value,
                                                         flags));
}

/**
 * gimp_procedure_add_double_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @min:         the minimum value for this argument
 * @max:         the maximum value for this argument
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new floating-point in double precision return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_double_return_value (GimpProcedure *procedure,
                                        const gchar   *name,
                                        const gchar   *nick,
                                        const gchar   *blurb,
                                        gdouble        min,
                                        gdouble        max,
                                        gdouble        value,
                                        GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    g_param_spec_double (name, nick, blurb,
                                                         min, max, value,
                                                         flags));
}

/**
 * gimp_procedure_add_enum_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @enum_type:   the #GType for this argument
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new enum argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_enum_argument (GimpProcedure *procedure,
                                  const gchar   *name,
                                  const gchar   *nick,
                                  const gchar   *blurb,
                                  GType          enum_type,
                                  gint           value,
                                  GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                g_param_spec_enum (name, nick, blurb,
                                                   enum_type, value,
                                                   flags));
}

/**
 * gimp_procedure_add_enum_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @enum_type:   the #GType for this argument
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new enum auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_enum_aux_argument (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      GType          enum_type,
                                      gint           value,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    g_param_spec_enum (name, nick, blurb,
                                                       enum_type, value,
                                                       flags));
}

/**
 * gimp_procedure_add_enum_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @enum_type:   the #GType for this argument
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new enum return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_enum_return_value (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      GType          enum_type,
                                      gint           value,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    g_param_spec_enum (name, nick, blurb,
                                                       enum_type, value,
                                                       flags));
}

/**
 * gimp_procedure_add_choice_argument:
 * @procedure:               the #GimpProcedure.
 * @name:                    the name of the argument to be created.
 * @nick:                    the label used in #GimpProcedureDialog.
 * @blurb: (nullable):       a more detailed help description.
 * @choice: (transfer full): the #GimpChoice
 * @value:                   the default value for #GimpChoice.
 * @flags:                   argument flags.
 *
 * Add a new #GimpChoice argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_choice_argument (GimpProcedure *procedure,
                                    const gchar   *name,
                                    const gchar   *nick,
                                    const gchar   *blurb,
                                    GimpChoice    *choice,
                                    const gchar   *value,
                                    GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_choice (name, nick, blurb,
                                                        choice, value, flags));
}

/**
 * gimp_procedure_add_choice_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @choice:      the #GimpChoice
 * @value:       the default value for #GimpChoice.
 * @flags:       argument flags.
 *
 * Add a new #GimpChoice auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_choice_aux_argument (GimpProcedure *procedure,
                                        const gchar    *name,
                                        const gchar    *nick,
                                        const gchar    *blurb,
                                        GimpChoice     *choice,
                                        const gchar    *value,
                                        GParamFlags     flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_choice (name, nick, blurb,
                                                            choice, value, flags));
}

/**
 * gimp_procedure_add_choice_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @choice:      the #GimpChoice
 * @value:       the default value for #GimpChoice.
 * @flags:       argument flags.
 *
 * Add a new #GimpChoice return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_choice_return_value (GimpProcedure *procedure,
                                        const gchar    *name,
                                        const gchar    *nick,
                                        const gchar    *blurb,
                                        GimpChoice     *choice,
                                        const gchar    *value,
                                        GParamFlags     flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_choice (name, nick, blurb,
                                                            choice, value, flags));
}

/**
 * gimp_procedure_add_string_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new string argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_string_argument (GimpProcedure *procedure,
                                    const gchar   *name,
                                    const gchar   *nick,
                                    const gchar   *blurb,
                                    const gchar   *value,
                                    GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                g_param_spec_string (name, nick, blurb,
                                                     value, flags));
}

/**
 * gimp_procedure_add_string_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new string auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_string_aux_argument (GimpProcedure *procedure,
                                        const gchar   *name,
                                        const gchar   *nick,
                                        const gchar   *blurb,
                                        const gchar   *value,
                                        GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    g_param_spec_string (name, nick, blurb,
                                                         value, flags));
}

/**
 * gimp_procedure_add_string_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @value:       the default value.
 * @flags:       argument flags.
 *
 * Add a new string return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_string_return_value (GimpProcedure *procedure,
                                        const gchar   *name,
                                        const gchar   *nick,
                                        const gchar   *blurb,
                                        const gchar   *value,
                                        GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    g_param_spec_string (name, nick, blurb,
                                                         value, flags));
}

/**
 * gimp_procedure_add_color_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @has_alpha:   whether the argument has transparency.
 * @value:       the default #GeglColor value.
 * @flags:       argument flags.
 *
 * Add a new #GeglColor argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_color_argument (GimpProcedure *procedure,
                                   const gchar   *name,
                                   const gchar   *nick,
                                   const gchar   *blurb,
                                   gboolean       has_alpha,
                                   GeglColor     *value,
                                   GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_color (name, nick, blurb,
                                                       has_alpha, value, flags));
}

/**
 * gimp_procedure_add_color_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @has_alpha:   whether the argument has transparency.
 * @value:       the default #GeglColor value.
 * @flags:       argument flags.
 *
 * Add a new #GeglColor auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_color_aux_argument (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       gboolean       has_alpha,
                                       GeglColor     *value,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_color (name, nick, blurb,
                                                           has_alpha, value, flags));
}

/**
 * gimp_procedure_add_color_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @has_alpha:   whether the argument has transparency.
 * @value:       the default #GeglColor value.
 * @flags:       argument flags.
 *
 * Add a new #GeglColor return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_color_return_value (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       gboolean       has_alpha,
                                       GeglColor     *value,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_color (name, nick, blurb,
                                                           has_alpha, value, flags));
}

/**
 * gimp_procedure_add_color_from_string_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @has_alpha:   whether the argument has transparency.
 * @value:       the default #GeglColor value.
 * @flags:       argument flags.
 *
 * Add a new #GeglColor argument to @procedure from a string representation.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_color_from_string_argument (GimpProcedure *procedure,
                                               const gchar   *name,
                                               const gchar   *nick,
                                               const gchar   *blurb,
                                               gboolean       has_alpha,
                                               const gchar   *value,
                                               GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_color_from_string (name, nick, blurb,
                                                                   has_alpha, value,
                                                                   flags));
}

/**
 * gimp_procedure_add_color_from_string_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @has_alpha:   whether the argument has transparency.
 * @value:       the default #GeglColor value.
 * @flags:       argument flags.
 *
 * Add a new #GeglColor auxiliary argument to @procedure from a string representation.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_color_from_string_aux_argument (GimpProcedure *procedure,
                                                   const gchar   *name,
                                                   const gchar   *nick,
                                                   const gchar   *blurb,
                                                   gboolean       has_alpha,
                                                   const gchar   *value,
                                                   GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_color_from_string (name, nick, blurb,
                                                                       has_alpha, value,
                                                                       flags));
}

/**
 * gimp_procedure_add_color_from_string_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @has_alpha:   whether the argument has transparency.
 * @value:       the default #GeglColor value.
 * @flags:       argument flags.
 *
 * Add a new #GeglColor return value to @procedure from a string representation.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_color_from_string_return_value (GimpProcedure *procedure,
                                                   const gchar   *name,
                                                   const gchar   *nick,
                                                   const gchar   *blurb,
                                                   gboolean       has_alpha,
                                                   const gchar   *value,
                                                   GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_color_from_string (name, nick, blurb,
                                                                       has_alpha, value,
                                                                       flags));
}

/**
 * gimp_procedure_add_parasite_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GimpParasite argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_parasite_argument (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_parasite (name, nick, blurb,
                                                          flags));
}

/**
 * gimp_procedure_add_parasite_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GimpParasite auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_parasite_aux_argument (GimpProcedure *procedure,
                                          const gchar   *name,
                                          const gchar   *nick,
                                          const gchar   *blurb,
                                          GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_parasite (name, nick, blurb,
                                                              flags));
}

/**
 * gimp_procedure_add_parasite_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GimpParasite return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_parasite_return_value (GimpProcedure *procedure,
                                          const gchar   *name,
                                          const gchar   *nick,
                                          const gchar   *blurb,
                                          GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_parasite (name, nick, blurb,
                                                              flags));
}

/**
 * gimp_procedure_add_param_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @param_type:  the #GPParamType for this argument
 * @flags:       argument flags.
 *
 * Add a new param argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_param_argument (GimpProcedure *procedure,
                                   const gchar   *name,
                                   const gchar   *nick,
                                   const gchar   *blurb,
                                   GType          param_type,
                                   GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                g_param_spec_param (name, nick, blurb,
                                                    param_type, flags));
}

/**
 * gimp_procedure_add_param_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @param_type:  the #GPParamType for this argument
 * @flags:       argument flags.
 *
 * Add a new param auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_param_aux_argument (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       GType          param_type,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    g_param_spec_param (name, nick, blurb,
                                                        param_type, flags));
}

/**
 * gimp_procedure_add_param_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @param_type:  the #GPParamType for this argument
 * @flags:       argument flags.
 *
 * Add a new param return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_param_return_value (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       GType          param_type,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    g_param_spec_param (name, nick, blurb,
                                                        param_type, flags));
}

/**
 * gimp_procedure_add_bytes_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GBytes argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_bytes_argument (GimpProcedure *procedure,
                                   const gchar   *name,
                                   const gchar   *nick,
                                   const gchar   *blurb,
                                   GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                g_param_spec_boxed (name, nick, blurb,
                                                    G_TYPE_BYTES, flags));
}

/**
 * gimp_procedure_add_bytes_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GBytes auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_bytes_aux_argument (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    g_param_spec_boxed (name, nick, blurb,
                                                        G_TYPE_BYTES, flags));
}

/**
 * gimp_procedure_add_bytes_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GBytes return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_bytes_return_value (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    g_param_spec_boxed (name, nick, blurb,
                                                        G_TYPE_BYTES, flags));
}

/**
 * gimp_procedure_add_int32_array_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new integer array argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_int32_array_argument (GimpProcedure *procedure,
                                         const gchar   *name,
                                         const gchar   *nick,
                                         const gchar   *blurb,
                                         GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_int32_array (name, nick, blurb,
                                                             flags));
}

/**
 * gimp_procedure_add_int32_array_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new integer array auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_int32_array_aux_argument (GimpProcedure *procedure,
                                             const gchar   *name,
                                             const gchar   *nick,
                                             const gchar   *blurb,
                                             GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_int32_array (name, nick, blurb,
                                                                 flags));
}

/**
 * gimp_procedure_add_int32_array_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new integer array return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_int32_array_return_value (GimpProcedure *procedure,
                                             const gchar   *name,
                                             const gchar   *nick,
                                             const gchar   *blurb,
                                             GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_int32_array (name, nick, blurb,
                                                                 flags));
}

/**
 * gimp_procedure_add_double_array_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new double array argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_double_array_argument (GimpProcedure *procedure,
                                          const gchar   *name,
                                          const gchar   *nick,
                                          const gchar   *blurb,
                                          GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_double_array (name, nick, blurb,
                                                              flags));
}

/**
 * gimp_procedure_add_double_array_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new double array auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_double_array_aux_argument (GimpProcedure *procedure,
                                              const gchar   *name,
                                              const gchar   *nick,
                                              const gchar   *blurb,
                                              GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_double_array (name, nick, blurb,
                                                                  flags));
}

/**
 * gimp_procedure_add_double_array_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new double array return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_double_array_return_value (GimpProcedure *procedure,
                                              const gchar   *name,
                                              const gchar   *nick,
                                              const gchar   *blurb,
                                              GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_double_array (name, nick, blurb,
                                                                  flags));
}

/**
 * gimp_procedure_add_string_array_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new string array argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_string_array_argument (GimpProcedure *procedure,
                                         const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         GParamFlags     flags)
{
  _gimp_procedure_add_argument (procedure,
                                g_param_spec_boxed (name, nick, blurb,
                                                    G_TYPE_STRV, flags));
}

/**
 * gimp_procedure_add_string_array_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new string array auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_string_array_aux_argument (GimpProcedure *procedure,
                                              const gchar   *name,
                                              const gchar   *nick,
                                              const gchar   *blurb,
                                              GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    g_param_spec_boxed (name, nick, blurb,
                                                        G_TYPE_STRV, flags));
}

/**
 * gimp_procedure_add_string_array_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new string array return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_string_array_return_value (GimpProcedure *procedure,
                                              const gchar   *name,
                                              const gchar   *nick,
                                              const gchar   *blurb,
                                              GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    g_param_spec_boxed (name, nick, blurb,
                                                        G_TYPE_STRV, flags));
}

/**
 * gimp_procedure_add_core_object_array_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @object_type  the type of object stored in the array
 * @flags:       argument flags.
 *
 * Add a new object array argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_core_object_array_argument (GimpProcedure *procedure,
                                               const gchar   *name,
                                               const gchar   *nick,
                                               const gchar   *blurb,
                                               GType          object_type,
                                               GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_core_object_array (name, nick, blurb,
                                                                   object_type, flags));
}

/**
 * gimp_procedure_add_core_object_array_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @object_type  the type of object stored in the array
 * @flags:       argument flags.
 *
 * Add a new object array auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_core_object_array_aux_argument (GimpProcedure *procedure,
                                                   const gchar   *name,
                                                   const gchar   *nick,
                                                   const gchar   *blurb,
                                                   GType          object_type,
                                                   GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_core_object_array (name, nick, blurb,
                                                                       object_type, flags));
}

/**
 * gimp_procedure_add_core_object_array_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @object_type  the type of object stored in the array
 * @flags:       argument flags.
 *
 * Add a new object array return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_core_object_array_return_value (GimpProcedure *procedure,
                                                   const gchar   *name,
                                                   const gchar   *nick,
                                                   const gchar   *blurb,
                                                   GType          object_type,
                                                   GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_core_object_array (name, nick, blurb,
                                                                       object_type, flags));
}

/**
 * gimp_procedure_add_display_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpDisplay argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_display_argument (GimpProcedure *procedure,
                                     const gchar   *name,
                                     const gchar   *nick,
                                     const gchar   *blurb,
                                     gboolean       none_ok,
                                     GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_display (name, nick, blurb,
                                                         none_ok, flags));
}

/**
 * gimp_procedure_add_display_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpDisplay auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_display_aux_argument (GimpProcedure *procedure,
                                         const gchar   *name,
                                         const gchar   *nick,
                                         const gchar   *blurb,
                                         gboolean       none_ok,
                                         GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_display (name, nick, blurb,
                                                             none_ok, flags));
}

/**
 * gimp_procedure_add_display_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpDisplay return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_display_return_value (GimpProcedure *procedure,
                                         const gchar   *name,
                                         const gchar   *nick,
                                         const gchar   *blurb,
                                         gboolean       none_ok,
                                         GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_display (name, nick, blurb,
                                                             none_ok, flags));
}

/**
 * gimp_procedure_add_image_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpImage argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_image_argument (GimpProcedure *procedure,
                                   const gchar   *name,
                                   const gchar   *nick,
                                   const gchar   *blurb,
                                   gboolean       none_ok,
                                   GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_image (name, nick, blurb,
                                                       none_ok, flags));
}

/**
 * gimp_procedure_add_image_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpImage auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_image_aux_argument (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       gboolean       none_ok,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_image (name, nick, blurb,
                                                           none_ok, flags));
}

/**
 * gimp_procedure_add_image_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpImage return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_image_return_value (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       gboolean       none_ok,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_image (name, nick, blurb,
                                                           none_ok, flags));
}

/**
 * gimp_procedure_add_item_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpItem argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_item_argument (GimpProcedure *procedure,
                                  const gchar   *name,
                                  const gchar   *nick,
                                  const gchar   *blurb,
                                  gboolean       none_ok,
                                  GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_item (name, nick, blurb,
                                                      none_ok, flags));
}

/**
 * gimp_procedure_add_item_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpItem auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_item_aux_argument (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      gboolean       none_ok,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_item (name, nick, blurb,
                                                          none_ok, flags));
}

/**
 * gimp_procedure_add_item_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpItem return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_item_return_value (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      gboolean       none_ok,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_item (name, nick, blurb,
                                                          none_ok, flags));
}

/**
 * gimp_procedure_add_drawable_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpDrawable argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_drawable_argument (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      gboolean       none_ok,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_drawable (name, nick, blurb,
                                                          none_ok, flags));
}

/**
 * gimp_procedure_add_drawable_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpDrawable auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_drawable_aux_argument (GimpProcedure *procedure,
                                          const gchar   *name,
                                          const gchar   *nick,
                                          const gchar   *blurb,
                                          gboolean       none_ok,
                                          GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_drawable (name, nick, blurb,
                                                              none_ok, flags));
}

/**
 * gimp_procedure_add_drawable_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpDrawable return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_drawable_return_value (GimpProcedure *procedure,
                                          const gchar   *name,
                                          const gchar   *nick,
                                          const gchar   *blurb,
                                          gboolean       none_ok,
                                          GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_drawable (name, nick, blurb,
                                                              none_ok, flags));
}

/**
 * gimp_procedure_add_layer_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpLayer argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_layer_argument (GimpProcedure *procedure,
                                   const gchar   *name,
                                   const gchar   *nick,
                                   const gchar   *blurb,
                                   gboolean       none_ok,
                                   GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_layer (name, nick, blurb,
                                                       none_ok, flags));
}

/**
 * gimp_procedure_add_layer_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpLayer auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_layer_aux_argument (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       gboolean       none_ok,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_layer (name, nick, blurb,
                                                           none_ok, flags));
}

/**
 * gimp_procedure_add_layer_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpLayer return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_layer_return_value (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       gboolean       none_ok,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_layer (name, nick, blurb,
                                                           none_ok, flags));
}

/**
 * gimp_procedure_add_text_layer_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpTextLayer argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_text_layer_argument (GimpProcedure *procedure,
                                        const gchar   *name,
                                        const gchar   *nick,
                                        const gchar   *blurb,
                                        gboolean       none_ok,
                                        GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_text_layer (name, nick, blurb,
                                                            none_ok, flags));
}

/**
 * gimp_procedure_add_text_layer_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpTextLayer auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_text_layer_aux_argument (GimpProcedure *procedure,
                                            const gchar   *name,
                                            const gchar   *nick,
                                            const gchar   *blurb,
                                            gboolean       none_ok,
                                            GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_text_layer (name, nick, blurb,
                                                                none_ok, flags));
}

/**
 * gimp_procedure_add_text_layer_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpTextLayer return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_text_layer_return_value (GimpProcedure *procedure,
                                            const gchar   *name,
                                            const gchar   *nick,
                                            const gchar   *blurb,
                                            gboolean       none_ok,
                                            GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_text_layer (name, nick, blurb,
                                                                none_ok, flags));
}

/**
 * gimp_procedure_add_vector_layer_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpVectorLayer argument to @procedure.
 *
 * Since: 3.2
 **/
void
gimp_procedure_add_vector_layer_argument (GimpProcedure *procedure,
                                          const gchar   *name,
                                          const gchar   *nick,
                                          const gchar   *blurb,
                                          gboolean       none_ok,
                                          GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_vector_layer (name, nick, blurb,
                                                              none_ok, flags));
}

/**
 * gimp_procedure_add_vector_layer_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpVectorLayer auxiliary argument to @procedure.
 *
 * Since: 3.2
 **/
void
gimp_procedure_add_vector_layer_aux_argument (GimpProcedure *procedure,
                                              const gchar   *name,
                                              const gchar   *nick,
                                              const gchar   *blurb,
                                              gboolean       none_ok,
                                              GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_vector_layer (name, nick, blurb,
                                                                  none_ok, flags));
}

/**
 * gimp_procedure_add_vector_layer_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpVectorLayer return value to @procedure.
 *
 * Since: 3.2
 **/
void
gimp_procedure_add_vector_layer_return_value (GimpProcedure *procedure,
                                              const gchar   *name,
                                              const gchar   *nick,
                                              const gchar   *blurb,
                                              gboolean       none_ok,
                                              GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_vector_layer (name, nick, blurb,
                                                                  none_ok, flags));
}

/**
 * gimp_procedure_add_link_layer_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpLinkLayer argument to @procedure.
 *
 * Since: 3.2
 **/
void
gimp_procedure_add_link_layer_argument (GimpProcedure *procedure,
                                        const gchar   *name,
                                        const gchar   *nick,
                                        const gchar   *blurb,
                                        gboolean       none_ok,
                                        GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_link_layer (name, nick, blurb,
                                                            none_ok, flags));
}

/**
 * gimp_procedure_add_link_layer_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpLinkLayer auxiliary argument to @procedure.
 *
 * Since: 3.2
 **/
void
    gimp_procedure_add_link_layer_aux_argument (GimpProcedure *procedure,
                                                const gchar   *name,
                                                const gchar   *nick,
                                                const gchar   *blurb,
                                                gboolean       none_ok,
                                                GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_link_layer (name, nick, blurb,
                                                                none_ok, flags));
}

/**
 * gimp_procedure_add_link_layer_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpLinkLayer return value to @procedure.
 *
 * Since: 3.2
 **/
void
gimp_procedure_add_link_layer_return_value (GimpProcedure *procedure,
                                            const gchar   *name,
                                            const gchar   *nick,
                                            const gchar   *blurb,
                                            gboolean       none_ok,
                                            GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_link_layer (name, nick, blurb,
                                                                none_ok, flags));
}

/**
 * gimp_procedure_add_group_layer_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new [class@GroupLayer] argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_group_layer_argument (GimpProcedure *procedure,
                                         const gchar   *name,
                                         const gchar   *nick,
                                         const gchar   *blurb,
                                         gboolean       none_ok,
                                         GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_group_layer (name, nick, blurb,
                                                             none_ok, flags));
}

/**
 * gimp_procedure_add_group_layer_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new [class@GroupLayer] auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_group_layer_aux_argument (GimpProcedure *procedure,
                                             const gchar   *name,
                                             const gchar   *nick,
                                             const gchar   *blurb,
                                             gboolean       none_ok,
                                             GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_group_layer (name, nick, blurb,
                                                                 none_ok, flags));
}

/**
 * gimp_procedure_add_group_layer_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new [class@GroupLayer] return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_group_layer_return_value (GimpProcedure *procedure,
                                             const gchar   *name,
                                             const gchar   *nick,
                                             const gchar   *blurb,
                                             gboolean       none_ok,
                                             GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_group_layer (name, nick, blurb,
                                                                 none_ok, flags));
}

/**
 * gimp_procedure_add_channel_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpChannel argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_channel_argument (GimpProcedure *procedure,
                                     const gchar   *name,
                                     const gchar   *nick,
                                     const gchar   *blurb,
                                     gboolean       none_ok,
                                     GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_channel (name, nick, blurb,
                                                         none_ok, flags));
}

/**
 * gimp_procedure_add_channel_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpChannel auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_channel_aux_argument (GimpProcedure *procedure,
                                         const gchar   *name,
                                         const gchar   *nick,
                                         const gchar   *blurb,
                                         gboolean       none_ok,
                                         GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_channel (name, nick, blurb,
                                                             none_ok, flags));
}

/**
 * gimp_procedure_add_channel_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpChannel return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_channel_return_value (GimpProcedure *procedure,
                                         const gchar   *name,
                                         const gchar   *nick,
                                         const gchar   *blurb,
                                         gboolean       none_ok,
                                         GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_channel (name, nick, blurb,
                                                             none_ok, flags));
}

/**
 * gimp_procedure_add_layer_mask_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpLayerMask argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_layer_mask_argument (GimpProcedure *procedure,
                                        const gchar   *name,
                                        const gchar   *nick,
                                        const gchar   *blurb,
                                        gboolean       none_ok,
                                        GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_layer_mask (name, nick, blurb,
                                                            none_ok, flags));
}

/**
 * gimp_procedure_add_layer_mask_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpLayerMask auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_layer_mask_aux_argument (GimpProcedure *procedure,
                                            const gchar   *name,
                                            const gchar   *nick,
                                            const gchar   *blurb,
                                            gboolean       none_ok,
                                            GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_layer_mask (name, nick, blurb,
                                                                none_ok, flags));
}

/**
 * gimp_procedure_add_layer_mask_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpLayerMask return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_layer_mask_return_value (GimpProcedure *procedure,
                                            const gchar   *name,
                                            const gchar   *nick,
                                            const gchar   *blurb,
                                            gboolean       none_ok,
                                            GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_layer_mask (name, nick, blurb,
                                                                none_ok, flags));
}

/**
 * gimp_procedure_add_selection_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpSelection argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_selection_argument (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       gboolean       none_ok,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_selection (name, nick, blurb,
                                                           none_ok, flags));
}

/**
 * gimp_procedure_add_selection_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpSelection auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_selection_aux_argument (GimpProcedure *procedure,
                                           const gchar   *name,
                                           const gchar   *nick,
                                           const gchar   *blurb,
                                           gboolean       none_ok,
                                           GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_selection (name, nick, blurb,
                                                               none_ok, flags));
}

/**
 * gimp_procedure_add_selection_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpSelection return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_selection_return_value (GimpProcedure *procedure,
                                           const gchar   *name,
                                           const gchar   *nick,
                                           const gchar   *blurb,
                                           gboolean       none_ok,
                                           GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_selection (name, nick, blurb,
                                                               none_ok, flags));
}

/**
 * gimp_procedure_add_path_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpPath argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_path_argument (GimpProcedure *procedure,
                                  const gchar   *name,
                                  const gchar   *nick,
                                  const gchar   *blurb,
                                  gboolean       none_ok,
                                  GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_path (name, nick, blurb,
                                                      none_ok, flags));
}

/**
 * gimp_procedure_add_path_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpPath auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_path_aux_argument (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      gboolean       none_ok,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_path (name, nick, blurb,
                                                          none_ok, flags));
}

/**
 * gimp_procedure_add_path_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     Whether no is a valid value.
 * @flags:       argument flags.
 *
 * Add a new #GimpPath return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_path_return_value (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      gboolean       none_ok,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_path (name, nick, blurb,
                                                          none_ok, flags));
}

/**
 * gimp_procedure_add_file_argument:
 * @procedure:     The #GimpProcedure.
 * @name:          The name of the argument to be created.
 * @nick:          The label used in #GimpProcedureDialog.
 * @blurb: (nullable): A more detailed help description.
 * @action:        The type of file to expect.
 * @none_ok:       Whether %NULL is allowed.
 * @default_file: (nullable): File to use if none is assigned.
 * @flags:         Argument flags.
 *
 * Add a new #GFile argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_file_argument (GimpProcedure         *procedure,
                                  const gchar           *name,
                                  const gchar           *nick,
                                  const gchar           *blurb,
                                  GimpFileChooserAction  action,
                                  gboolean               none_ok,
                                  GFile                 *default_file,
                                  GParamFlags            flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_file (name, nick, blurb,
                                                      action, none_ok,
                                                      default_file, flags));
}

/**
 * gimp_procedure_add_file_aux_argument:
 * @procedure:    The #GimpProcedure.
 * @name:         The name of the argument to be created.
 * @nick:         The label used in #GimpProcedureDialog.
 * @blurb: (nullable): A more detailed help description.
 * @action:       The type of file to expect.
 * @none_ok:      Whether %NULL is allowed.
 * @default_file: (nullable): File to use if none is assigned.
 * @flags:        Argument flags.
 *
 * Add a new #GFile auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_file_aux_argument (GimpProcedure         *procedure,
                                      const gchar           *name,
                                      const gchar           *nick,
                                      const gchar           *blurb,
                                      GimpFileChooserAction  action,
                                      gboolean               none_ok,
                                      GFile                 *default_file,
                                      GParamFlags            flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_file (name, nick, blurb,
                                                          action, none_ok,
                                                          default_file, flags));
}

/**
 * gimp_procedure_add_file_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GFile return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_file_return_value (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_file (name, nick, blurb,
                                                          GIMP_FILE_CHOOSER_ACTION_ANY,
                                                          TRUE, NULL, flags));
}

/**
 * gimp_procedure_add_resource_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     whether %NULL is a valid value.
 * @default_value: (nullable): default value.
 * @flags:       argument flags.
 *
 * Add a new #GimpResource argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_resource_argument (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      gboolean       none_ok,
                                      GimpResource  *default_value,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_resource (name, nick, blurb,
                                                          GIMP_TYPE_RESOURCE,
                                                          none_ok,
                                                          default_value,
                                                          FALSE,
                                                          flags));
}

/**
 * gimp_procedure_add_resource_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @default_value: (nullable): default value.
 * @flags:       argument flags.
 *
 * Add a new #GimpResource auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_resource_aux_argument (GimpProcedure *procedure,
                                          const gchar   *name,
                                          const gchar   *nick,
                                          const gchar   *blurb,
                                          GimpResource  *default_value,
                                          GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_resource (name, nick, blurb,
                                                              GIMP_TYPE_RESOURCE,
                                                              TRUE,
                                                              default_value,
                                                              FALSE,
                                                              flags));
}

/**
 * gimp_procedure_add_resource_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GimpResource return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_resource_return_value (GimpProcedure *procedure,
                                          const gchar   *name,
                                          const gchar   *nick,
                                          const gchar   *blurb,
                                          GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_resource (name, nick, blurb,
                                                              GIMP_TYPE_RESOURCE,
                                                              TRUE, NULL, FALSE, flags));
}

/**
 * gimp_procedure_add_brush_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     whether %NULL is a valid value.
 * @default_value: (nullable): default value
 * @default_to_context: Use the context's brush as default value.
 * @flags:       argument flags.
 *
 * Add a new #GimpBrush argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_brush_argument (GimpProcedure *procedure,
                                   const gchar   *name,
                                   const gchar   *nick,
                                   const gchar   *blurb,
                                   gboolean       none_ok,
                                   GimpBrush     *default_value,
                                   gboolean       default_to_context,
                                   GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_brush (name, nick, blurb,
                                                       none_ok,
                                                       default_value,
                                                       default_to_context,
                                                       flags));
}

/**
 * gimp_procedure_add_brush_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @default_value: (nullable): default value
 * @default_to_context: Use the context's brush as default value.
 * @flags:       argument flags.
 *
 * Add a new #GimpBrush auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_brush_aux_argument (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       GimpBrush     *default_value,
                                       gboolean       default_to_context,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_brush (name, nick, blurb,
                                                           TRUE,
                                                           default_value,
                                                           default_to_context,
                                                           flags));
}

/**
 * gimp_procedure_add_brush_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GimpBrush return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_brush_return_value (GimpProcedure *procedure,
                                       const gchar   *name,
                                       const gchar   *nick,
                                       const gchar   *blurb,
                                       GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_brush (name, nick, blurb,
                                                           TRUE, NULL, FALSE, flags));
}

/**
 * gimp_procedure_add_font_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     whether %NULL is a valid value.
 * @default_value: (nullable): default value
 * @default_to_context: Use the context's font as default value.
 * @flags:       argument flags.
 *
 * Add a new #GimpFont argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_font_argument (GimpProcedure *procedure,
                                  const gchar   *name,
                                  const gchar   *nick,
                                  const gchar   *blurb,
                                  gboolean       none_ok,
                                  GimpFont      *default_value,
                                  gboolean       default_to_context,
                                  GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_font (name, nick, blurb,
                                                      none_ok,
                                                      default_value,
                                                      default_to_context,
                                                      flags));
}

/**
 * gimp_procedure_add_font_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @default_value: (nullable): default value
 * @default_to_context: Use the context's font as default value.
 * @flags:       argument flags.
 *
 * Add a new #GimpFont auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_font_aux_argument (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      GimpFont      *default_value,
                                      gboolean       default_to_context,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_font (name, nick, blurb,
                                                          TRUE,
                                                          default_value,
                                                          default_to_context,
                                                          flags));
}

/**
 * gimp_procedure_add_font_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GimpFont return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_font_return_value (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_font (name, nick, blurb,
                                                          TRUE, NULL, FALSE, flags));
}

/**
 * gimp_procedure_add_gradient_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     whether %NULL is a valid value.
 * @default_value: (nullable): default value
 * @default_to_context: Use the context's gradient as default value.
 * @flags:       argument flags.
 *
 * Add a new #GimpGradient argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_gradient_argument (GimpProcedure *procedure,
                                      const gchar   *name,
                                      const gchar   *nick,
                                      const gchar   *blurb,
                                      gboolean       none_ok,
                                      GimpGradient  *default_value,
                                      gboolean       default_to_context,
                                      GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_gradient (name, nick, blurb,
                                                          none_ok,
                                                          default_value,
                                                          default_to_context,
                                                          flags));
}

/**
 * gimp_procedure_add_gradient_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @default_value: (nullable): default value
 * @default_to_context: Use the context's gradient as default value.
 * @flags:       argument flags.
 *
 * Add a new #GimpGradient auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_gradient_aux_argument (GimpProcedure *procedure,
                                          const gchar   *name,
                                          const gchar   *nick,
                                          const gchar   *blurb,
                                          GimpGradient  *default_value,
                                          gboolean       default_to_context,
                                          GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_gradient (name, nick, blurb,
                                                              TRUE,
                                                              default_value,
                                                              default_to_context,
                                                              flags));
}

/**
 * gimp_procedure_add_gradient_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GimpGradient return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_gradient_return_value (GimpProcedure *procedure,
                                          const gchar   *name,
                                          const gchar   *nick,
                                          const gchar   *blurb,
                                          GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_gradient (name, nick, blurb,
                                                              TRUE, NULL, FALSE, flags));
}

/**
 * gimp_procedure_add_palette_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     whether %NULL is a valid value.
 * @default_value: (nullable): default value
 * @default_to_context: Use the context's palette as default value.
 * @flags:       argument flags.
 *
 * Add a new #GimpPalette argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_palette_argument (GimpProcedure *procedure,
                                     const gchar   *name,
                                     const gchar   *nick,
                                     const gchar   *blurb,
                                     gboolean       none_ok,
                                     GimpPalette   *default_value,
                                     gboolean       default_to_context,
                                     GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_palette (name, nick, blurb,
                                                         none_ok,
                                                         default_value,
                                                         default_to_context,
                                                         flags));
}

/**
 * gimp_procedure_add_palette_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @default_value: (nullable): default value
 * @default_to_context: Use the context's palette as default value.
 * @flags:       argument flags.
 *
 * Add a new #GimpPalette auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_palette_aux_argument (GimpProcedure *procedure,
                                         const gchar   *name,
                                         const gchar   *nick,
                                         const gchar   *blurb,
                                         GimpPalette   *default_value,
                                         gboolean       default_to_context,
                                         GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_palette (name, nick, blurb,
                                                             TRUE,
                                                             default_value,
                                                             default_to_context,
                                                             flags));
}

/**
 * gimp_procedure_add_palette_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GimpPalette return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_palette_return_value (GimpProcedure *procedure,
                                         const gchar   *name,
                                         const gchar   *nick,
                                         const gchar   *blurb,
                                         GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_palette (name, nick, blurb,
                                                             TRUE, NULL, FALSE, flags));
}

/**
 * gimp_procedure_add_pattern_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @none_ok:     whether %NULL is a valid value.
 * @default_value: (nullable): default value
 * @default_to_context: Use the context's pattern as default value.
 * @flags:       argument flags.
 *
 * Add a new #GimpPattern argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_pattern_argument (GimpProcedure *procedure,
                                     const gchar   *name,
                                     const gchar   *nick,
                                     const gchar   *blurb,
                                     gboolean       none_ok,
                                     GimpPattern   *default_value,
                                     gboolean       default_to_context,
                                     GParamFlags    flags)
{
  _gimp_procedure_add_argument (procedure,
                                gimp_param_spec_pattern (name, nick, blurb,
                                                         none_ok,
                                                         default_value,
                                                         default_to_context,
                                                         flags));
}

/**
 * gimp_procedure_add_pattern_aux_argument:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @default_value: (nullable): default value
 * @default_to_context: Use the context's pattern as default value.
 * @flags:       argument flags.
 *
 * Add a new #GimpPattern auxiliary argument to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_pattern_aux_argument (GimpProcedure *procedure,
                                         const gchar   *name,
                                         const gchar   *nick,
                                         const gchar   *blurb,
                                         GimpPattern   *default_value,
                                         gboolean       default_to_context,
                                         GParamFlags    flags)
{
  _gimp_procedure_add_aux_argument (procedure,
                                    gimp_param_spec_pattern (name, nick, blurb,
                                                             TRUE,
                                                             default_value,
                                                             default_to_context,
                                                             flags));
}

/**
 * gimp_procedure_add_pattern_return_value:
 * @procedure:   the #GimpProcedure.
 * @name:        the name of the argument to be created.
 * @nick:        the label used in #GimpProcedureDialog.
 * @blurb: (nullable): a more detailed help description.
 * @flags:       argument flags.
 *
 * Add a new #GimpPattern return value to @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_pattern_return_value (GimpProcedure *procedure,
                                         const gchar   *name,
                                         const gchar   *nick,
                                         const gchar   *blurb,
                                         GParamFlags    flags)
{
  _gimp_procedure_add_return_value (procedure,
                                    gimp_param_spec_pattern (name, nick, blurb,
                                                             TRUE, NULL, FALSE, flags));
}
