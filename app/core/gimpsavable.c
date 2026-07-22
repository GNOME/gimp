/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsavable.c
 * Copyright (C) 2026 Jehan
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gimp-utils.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpdrawable.h"
#include "gimppalette.h"
#include "gimpsavable.h"


static void    gimp_savable_print_incomplete_element (GimpSaveState *state,
                                                      const gchar   *element_name,
                                                      va_list        args);
static gchar * gimp_savable_printf                   (const gchar   *format,
                                                      va_list        args);


G_DEFINE_INTERFACE (GimpSavable, gimp_savable, G_TYPE_OBJECT)


static void
gimp_savable_default_init (GimpSavableInterface *iface)
{
}


/*  public functions  */

void
gimp_savable_save (GimpSavable   *savable,
                   GimpSaveState *state)
{
  GimpSavableInterface *savable_iface;

  g_return_if_fail (GIMP_IS_SAVABLE (savable));

  savable_iface = GIMP_SAVABLE_GET_IFACE (savable);

  if (savable_iface->save)
    {
      gint n_elements = g_queue_get_length (state->elements);
      savable_iface->save (savable, state);
      /* Sanity check. */
      g_return_if_fail (n_elements == g_queue_get_length (state->elements));
    }
}

void
gimp_savable_print_element_start (GimpSaveState *state,
                                  const gchar   *element_name,
                                  ...)
{
  va_list args;

  va_start (args, element_name);
  gimp_savable_print_incomplete_element (state, element_name, args);
  g_output_stream_printf (state->output, NULL, NULL, NULL, ">\n");
  va_end (args);

  /* Assuming we only use static strings and don't need to create dups. */
  g_queue_push_head (state->elements, (gpointer) element_name);
}

void
gimp_savable_print_element_end (GimpSaveState *state,
                                const gchar   *element_name)
{
  const gchar *current_element = (const gchar *) g_queue_pop_head (state->elements);
  gint         n_elements;

  g_return_if_fail (g_strcmp0 (current_element, element_name) == 0);

  n_elements = g_queue_get_length (state->elements);

  if (n_elements == 0)
    g_output_stream_printf (state->output, NULL, NULL, NULL, "</%s>\n", element_name);
  else
    g_output_stream_printf (state->output, NULL, NULL, NULL, "%*c</%s>\n",
                            n_elements * 2, ' ', element_name);
}

/**
 * gimp_savable_print_element:
 * @state:
 * @element_name:
 * @value_format:
 * @...: a main value (or %NULL) followed by (name, format, value)
 *       triplets for each attribute.
 *
 * This will create the element `<@element_name>`.
 *
 * If @value_format is not %NULL, then the next argument needs to be
 * non-%NULL either and of the type corresponding to @value_format.
 *
 * Then, the arguments will be in triplets, with the first being an
 * attribute name, the second a printf-style format (containing a single
 * conversion specifier), the third an attribute value of corresponding
 * type.
 * End the list of attributes with %NULL.
 *
 * Ex:
 * ```C
 * gimp_savable_print_element (state, "plop", "%f", 0.5,
 *                             "hello", "%s", "world",
 *                             "id", "%d", 10,
 *                             NULL);
 * ```
 *
 * will print: `<plop hello='world' id='10'>0.5</plop>`
 *
 * Note that additionally to the common supported specifiers, such as
 * "%d", "%u", "%f", etc. this function adds the following modifiers:
 *
 * - "%b": boolean values (will print "true" or "false");
 * - "%t": will print the %GType name of an object.
 * - "%T": will print the name of a %GType.
 */
void
gimp_savable_print_element (GimpSaveState *state,
                            const gchar   *element_name,
                            const gchar   *value_format,
                            ...)
{
  gchar   *strval = NULL;
  va_list  args;

  g_return_if_fail (element_name != NULL);

  va_start (args, value_format);

  if (value_format)
    {
      strval = gimp_savable_printf (value_format, args);
      g_return_if_fail (strval != NULL);
    }
    else
    {
      /* Ignore the next arg. */
      (void) va_arg (args, void *);
    }

  gimp_savable_print_incomplete_element (state, element_name, args);

  if (strval)
    {
      gchar *encoded;

      encoded = g_markup_escape_text (strval, -1);

      g_output_stream_printf (state->output, NULL, NULL, NULL, ">%s</%s>\n",
                              encoded, element_name);

      g_free (strval);
      g_free (encoded);
    }
  else
    {
      g_output_stream_printf (state->output, NULL, NULL, NULL, "/>\n");
    }

  va_end (args);
}

void
gimp_savable_config_save (GimpConfig    *config,
                          const gchar   *element_name,
                          GimpSaveState *state)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs = 0;
  guint          i;

  g_return_if_fail (G_IS_OBJECT (config));
  g_return_if_fail (element_name != NULL);

  gimp_savable_print_element_start (state, element_name,
                                    "type", "%t", config,
                                    NULL);

  klass = G_OBJECT_GET_CLASS (config);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];
      GValue      value     = G_VALUE_INIT;

      if (! (prop_spec->flags & GIMP_CONFIG_PARAM_SERIALIZE))
        continue;

      g_value_init (&value, prop_spec->value_type);
      g_object_get_property (G_OBJECT (config), prop_spec->name, &value);
      gimp_savable_value_save (&value, prop_spec, state);
      g_value_unset (&value);
    }

  g_free (property_specs);

  gimp_savable_print_element_end (state, element_name);
}

void
gimp_savable_format_save (const Babl    *format,
                          GimpSaveState *state)
{
  const Babl  *space;
  const gchar *encoding;

  encoding = babl_format_get_encoding (format);
  gimp_savable_print_element_start (state, "format", "encoding", "%s", encoding, NULL);

  space = babl_format_get_space (format);
  gimp_savable_space_save (space, state, 0);

  gimp_savable_print_element_end (state, "format");
}

void
gimp_savable_space_save (const Babl    *space,
                         GimpSaveState *state,
                         gint           new_space_id)
{
  g_return_if_fail (state->icc_references != NULL || new_space_id >= 0);

  if (space == babl_space ("sRGB") || space == NULL)
    {
      /* XXX Maybe I should actually also verify the format. A NULL
       * space on a CMYK or other models won't necessarily be sRGB.
       * Or maybe set name='default'?
       */
      gimp_savable_print_element (state, "space", NULL, NULL, "name", "%s", "sRGB", NULL);
    }
  else if (state->icc_references == NULL)
    {
      /* A NULL references is a special case where we are creating the
       * hash table of all used space in initialization step. This is
       * the only case where new_space_id is used.
       */
      int         icc_length = 0;
      const char *icc = babl_space_get_icc (space, &icc_length);
      gchar      *icc_b64;

      icc_b64 = g_base64_encode ((const guchar*) icc, icc_length);
      gimp_savable_print_element_start (state, "space", "id", "space-%d", new_space_id, NULL);
      gimp_savable_print_element (state, "icc", "%s", icc_b64, NULL);
      gimp_savable_print_element_end (state, "space");

      g_free (icc_b64);
    }
  else
    {
      gpointer key;
      gpointer space_id;

      if (g_hash_table_lookup_extended (state->icc_references, babl_get_name (space), &key, &space_id))
        {
          gimp_savable_print_element (state, "space", NULL, NULL,
                                      "idref", "space-%d", GPOINTER_TO_UINT (space_id),
                                      NULL);
        }
      else
        {
          /* We stored the easy target spaces at the start of the XCF
           * file, such as spaces of the image itself, layers and from
           * the colormap of indexed images.
           * We don't go as far as looking for colors used as filter
           * arguments, or inside gradient arguments.
           * XXX Should we update gimp_savable_save_all_spaces() to also
           * store these?
           * For now let's create an explicit space, but we could
           * imagine it generates duplicates.
           * g_critical ("%s: the space was not previously stored: %s", G_STRFUNC, babl_get_name (space));
           */
          int         icc_length = 0;
          const char *icc = babl_space_get_icc (space, &icc_length);
          gchar      *icc_b64;

          icc_b64 = g_base64_encode ((const guchar*) icc, icc_length);
          gimp_savable_print_element_start (state, "space", NULL);
          gimp_savable_print_element (state, "icc", "%s", icc_b64, NULL);
          gimp_savable_print_element_end (state, "space");

          g_free (icc_b64);
        }
    }
}

void
gimp_savable_color_save  (GeglColor     *color,
                          const gchar   *name,
                          const Babl    *restricted_format,
                          GimpSaveState *state)
{
  const Babl *format;
  gchar      *pixel_b64;
  guint8      pixel[40];
  gint        bpp;

  g_return_if_fail (state->icc_references != NULL);

  if (name != NULL)
    gimp_savable_print_element_start (state, "color", "name", "%s", name, NULL);
  else
    gimp_savable_print_element_start (state, "color", NULL);

  /* XXX Do we want to make it possible to have a restricted format yet
   * store the colors in their source format (which may be higher
   * bit-depth)? If we were able to drop the restriction, we would not
   * lose precision data.
   */
  if (restricted_format)
    {
      format = restricted_format;
    }
  else
    {
      format = gegl_color_get_format (color);
      gimp_savable_format_save (format, state);
    }

  gegl_color_get_pixel (color, format, pixel);
  bpp = babl_format_get_bytes_per_pixel (format);
  pixel_b64 = g_base64_encode ((const guchar*) pixel, bpp);
  gimp_savable_print_element (state, "pixel", "%s", pixel_b64, NULL);

  gimp_savable_print_element_end (state, "color");

  g_free (pixel_b64);
}

void
gimp_savable_save_all_spaces (GimpImage     *image,
                              GimpSaveState *state)
{
  GHashTable *icc_refs;
  GList      *iter;
  const Babl *space;
  gint        icc_id = 0;

  g_return_if_fail (state->icc_references == NULL);

  icc_refs = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);

  /* Save the space of the image itself. */
  gimp_savable_print_element_start (state, "formats", NULL);
  space = gimp_image_get_layer_space (image);
  if (space != NULL && space != babl_space ("sRGB"))
    {
      gimp_savable_space_save (space, state, icc_id);
      g_hash_table_insert (icc_refs, (gpointer) babl_get_name (space), GINT_TO_POINTER (icc_id++));
    }

  /* Save the space of the layers (preparing a future where layers may
   * have their own model and space.
   */
  iter = gimp_image_get_layer_list (image);
  for (; iter; iter = iter->next)
    {
      GimpDrawable *drawable = iter->data;

      space = gimp_drawable_get_space (drawable);
      if (space != babl_space ("sRGB") &&
          ! g_hash_table_lookup_extended (icc_refs, babl_get_name (space), NULL, NULL))
        {
          gimp_savable_space_save (space, state, icc_id);
          g_hash_table_insert (icc_refs, (gpointer) babl_get_name (space), GUINT_TO_POINTER (icc_id++));
        }
    }
  g_list_free (iter);

  /* Save the space of the palette colors (preparing a future where they
   * may have their own model and space.
   */
  if (gimp_image_get_colormap_palette (image))
    {
      GimpPalette *palette = gimp_image_get_colormap_palette (image);
      const Babl  *format;

      format = gimp_palette_get_restriction (palette);

      if (format == NULL)
        {
          iter = gimp_palette_get_colors (palette);
          for (; iter; iter = iter->next)
            {
              GimpPaletteEntry *entry = iter->data;

              format = gegl_color_get_format (entry->color);
              space = babl_format_get_space (format);
              if (space != babl_space ("sRGB") &&
                  ! g_hash_table_lookup_extended (icc_refs, babl_get_name (space), NULL, NULL))
                {
                  gimp_savable_space_save (space, state, icc_id);
                  g_hash_table_insert (icc_refs, (gpointer) babl_get_name (space), GUINT_TO_POINTER (icc_id++));
                }
            }
        }
      else
        {
          space = babl_format_get_space (format);
          if (space != babl_space ("sRGB") &&
              ! g_hash_table_lookup_extended (icc_refs, babl_get_name (space), NULL, NULL))
            {
              gimp_savable_space_save (space, state, icc_id);
              g_hash_table_insert (icc_refs, (gpointer) babl_get_name (space), GUINT_TO_POINTER (icc_id++));
            }
        }
    }
  gimp_savable_print_element_end (state, "formats");

  state->icc_references = icc_refs;
}

void
gimp_savable_unit_save (GimpUnit      *unit,
                        GimpSaveState *state)
{
  if (gimp_unit_is_built_in (unit))
    {
      guint32 uid = gimp_unit_get_id (unit);

      /* Making sure the GType exists. */
      (void) GIMP_TYPE_UNIT_ID;
      gimp_savable_print_element (state, "unit", NULL, NULL,
                                  "built-in", "%[GimpUnitID]", uid, NULL);
    }
  else
    {
      gimp_savable_print_element_start (state, "unit", NULL);
      gimp_savable_print_element (state, "factor",
                                  "%f", gimp_unit_get_factor (unit),
                                  NULL);
      gimp_savable_print_element (state, "digits",
                                  "%d", gimp_unit_get_digits (unit),
                                  NULL);
      gimp_savable_print_element (state, "name",
                                  "%s", gimp_unit_get_name (unit),
                                  NULL);
      gimp_savable_print_element (state, "symbol",
                                  "%s", gimp_unit_get_symbol (unit),
                                  NULL);
      gimp_savable_print_element (state, "abbreviation",
                                  "%s", gimp_unit_get_abbreviation (unit),
                                  NULL);
      gimp_savable_print_element_end (state, "unit");
    }
}

void
gimp_savable_metadata_save (GimpMetadata  *metadata,
                            GimpSaveState *state)
{
  gchar *meta_string;

  meta_string = gimp_metadata_serialize (metadata);
  if (meta_string)
    gimp_savable_print_element (state, "metadata", "%s", meta_string, NULL);

  g_free (meta_string);
}

void
gimp_savable_parasite_save (GimpParasite  *parasite,
                            GimpSaveState *state)
{
  if (gimp_parasite_is_persistent (parasite))
    {
      const gchar   *name;
      gulong         flags;
      gconstpointer  data;
      guint32        data_length;
      gchar         *data_b64;

      name  = gimp_parasite_get_name (parasite);
      flags = gimp_parasite_get_flags (parasite);

      gimp_savable_print_element_start (state, "parasite",
                                        "name",  "%s",  name,
                                        "flags", "%lu", flags,
                                        NULL);

      data     = gimp_parasite_get_data (parasite, &data_length);
      data_b64 = g_base64_encode ((const guchar*) data, data_length);

      gimp_savable_print_element_end (state, "parasite");

      g_free (data_b64);
    }
}

void
gimp_savable_matrix2_save (GimpMatrix2   *matrix,
                           GimpSaveState *state)
{
  gimp_savable_print_element_start (state, "matrix",
                                    "dim0", "%d", 2,
                                    "dim1", "%d", 2,
                                    NULL);
  gimp_savable_print_element (state, "coeff", "%f", matrix->coeff[0][0], NULL);
  gimp_savable_print_element (state, "coeff", "%f", matrix->coeff[0][1], NULL);
  gimp_savable_print_element (state, "coeff", "%f", matrix->coeff[1][0], NULL);
  gimp_savable_print_element (state, "coeff", "%f", matrix->coeff[1][1], NULL);
  gimp_savable_print_element_end (state, "matrix");
}

void
gimp_savable_value_save (GValue        *value,
                         GParamSpec    *pspec,
                         GimpSaveState *state)
{
  const gchar *tag_name = pspec ? pspec->name : "value";

  if (G_VALUE_HOLDS_BOOLEAN (value))
    {
      gimp_savable_print_element (state, tag_name, "%b",
                                  g_value_get_boolean (value),
                                  "type", "%s", "boolean",
                                  NULL);
    }
  else if (G_VALUE_HOLDS_INT (value))
    {
      gimp_savable_print_element (state, tag_name,
                                  "%d", g_value_get_int (value),
                                  "type", "%s", "int",
                                  NULL);
    }
  else if (G_VALUE_HOLDS_ENUM (value))
    {
      gimp_savable_print_element (state, tag_name, "%s",
                                  gimp_get_enum_value_nick (G_VALUE_TYPE (value),
                                                            g_value_get_enum (value)),
                                  "type", "%T", G_VALUE_TYPE (value),
                                  NULL);
    }
  else if (G_VALUE_HOLDS_STRING (value))
    {
      const gchar *str = g_value_get_string (value);
      gimp_savable_print_element (state, tag_name,
                                  "%s", str ? str : "",
                                  "type", "%s", "string",
                                  NULL);
    }
  else if (G_VALUE_HOLDS_DOUBLE (value))
    {
      gimp_savable_print_element (state, tag_name,
                                  "%f", g_value_get_double (value),
                                  "type", "%s", "double",
                                  NULL);
    }
  else if (G_VALUE_HOLDS_FLOAT (value))
    {
      gimp_savable_print_element (state, tag_name,
                                  "%f", g_value_get_float (value),
                                  "type", "%s", "float",
                                  NULL);
    }
  else if (G_VALUE_HOLDS_OBJECT (value) &&
           g_type_is_a (G_VALUE_TYPE (value), GIMP_TYPE_UNIT))
    {
      GimpUnit *unit = g_value_get_object (value);

      gimp_savable_print_element_start (state, tag_name,
                                        "type", "%s", "GimpUnit",
                                        NULL);
      if (unit)
        gimp_savable_unit_save (unit, state);
      gimp_savable_print_element_end (state, tag_name);
    }
  else if (G_VALUE_HOLDS_OBJECT (value) &&
           g_type_is_a (G_VALUE_TYPE (value), GEGL_TYPE_COLOR))
    {
      GeglColor *color = g_value_get_object (value);

      gimp_savable_print_element_start (state, tag_name,
                                        "type", "%s", "GeglColor",
                                        NULL);
      if (color)
        gimp_savable_color_save (color, NULL, NULL, state);
      gimp_savable_print_element_end (state, tag_name);
    }
  else if (G_VALUE_HOLDS_OBJECT (value) &&
           g_type_is_a (G_VALUE_TYPE (value), GIMP_TYPE_SAVABLE))
    {
      GObject *object = g_value_get_object (value);

      gimp_savable_print_element_start (state, tag_name,
                                        "type", "%T", G_VALUE_TYPE (value),
                                        NULL);
      if (object)
        gimp_savable_save (GIMP_SAVABLE (object), state);
      gimp_savable_print_element_end (state, tag_name);
    }
  else if (GIMP_VALUE_HOLDS_MATRIX2 (value))
    {
      GimpMatrix2 *matrix = g_value_get_boxed (value);

      gimp_savable_print_element_start (state, tag_name,
                                        "type", "%s", "GimpMatrix2",
                                        NULL);
      if (matrix)
        gimp_savable_matrix2_save (matrix, state);
      gimp_savable_print_element_end (state, tag_name);
    }
  else if (GIMP_VALUE_HOLDS_VALUE_ARRAY (value))
    {
      GimpValueArray *values = g_value_get_boxed (value);

      gimp_savable_print_element_start (state, tag_name,
                                        "type", "%s", "GimpValueArray",
                                        NULL);
      if (values)
        gimp_savable_value_array_save (values, state);
      gimp_savable_print_element_end (state, tag_name);
    }
  else
    {
      if (pspec)
        g_critical ("%s: value for property '%s' of type '%s' is not supported.",
                    G_STRFUNC, pspec->name, g_type_name (G_VALUE_TYPE (value)));
      else
        g_critical ("%s: value of type '%s' is not supported.",
                    G_STRFUNC, g_type_name (G_VALUE_TYPE (value)));
    }
}

void
gimp_savable_value_array_save (GimpValueArray *values,
                               GimpSaveState  *state)
{
  gint length = gimp_value_array_length (values);

  gimp_savable_print_element_start (state, "value-array", NULL);

  for (gint i = 0; i < length; i++)
    {
      GValue *value = gimp_value_array_index (values, i);

      gimp_savable_value_save (value, NULL, state);
    }

  gimp_savable_print_element_end (state, "value-array");
}
void
gimp_savable_composite_mode_save (GimpLayerCompositeMode  composite_mode,
                                  GimpLayerMode           mode,
                                  GimpSaveState          *state)
{
  gboolean auto_set = FALSE;

  if (composite_mode == GIMP_LAYER_COMPOSITE_AUTO)
    {
      /* if composite_mode is AUTO, save the actual value AUTO maps to
       * for the given mode, so that we can correctly load it even if
       * the mapping changes in the future.
       */
      composite_mode = gimp_layer_mode_get_composite_mode (mode);
      auto_set       = TRUE;
    }

  gimp_savable_print_element (state, "composite-mode",
                              "%[GimpLayerCompositeMode]", composite_mode,
                              auto_set ? "auto" : NULL, "%s", "true",
                              NULL);
}

void
gimp_savable_composite_space_save (GimpLayerColorSpace  composite_space,
                                   GimpLayerMode        mode,
                                   GimpSaveState       *state)
{
  gboolean auto_set = FALSE;

  if (composite_space == GIMP_LAYER_COLOR_SPACE_AUTO)
    {
      /* if composite_space is AUTO, save the actual value AUTO maps to
       * for the given mode, so that we can correctly load it even if
       * the mapping changes in the future.
       */
      composite_space = gimp_layer_mode_get_composite_space (mode);
      auto_set        = TRUE;
    }

  gimp_savable_print_element (state, "composite-space",
                              "%[GimpLayerColorSpace]", composite_space,
                              auto_set ? "auto" : NULL, "%s", "true",
                              NULL);
}

void
gimp_savable_blend_space_save (GimpLayerColorSpace  blend_space,
                               GimpLayerMode        mode,
                               GimpSaveState       *state)
{
  gboolean auto_set = FALSE;

  if (blend_space == GIMP_LAYER_COLOR_SPACE_AUTO)
    {
      /* if blend_space is AUTO, save the actual value AUTO maps to for
       * the given mode, so that we can correctly load it even if the
       * mapping changes in the future.
       */
      blend_space = gimp_layer_mode_get_blend_space (mode);
      auto_set    = TRUE;
    }

  gimp_savable_print_element (state, "blend-space",
                              "%[GimpLayerColorSpace]", blend_space,
                              auto_set ? "auto" : NULL, "%s", "true",
                              NULL);
}


/* Private Functions */

static void
gimp_savable_print_incomplete_element (GimpSaveState *state,
                                       const gchar   *element_name,
                                       va_list        args)
{
  const gchar *attr;
  const gchar *format;
  gint         n_elements;

  n_elements = g_queue_get_length (state->elements);
  if (n_elements == 0)
    g_output_stream_printf (state->output, NULL, NULL, NULL, "<%s", element_name);
  else
    g_output_stream_printf (state->output, NULL, NULL, NULL, "%*c<%s",
                            n_elements * 2, ' ', element_name);

  attr = va_arg (args, char *);
  while (attr)
    {
      gchar *strval;
      gchar *encoded;

      format = va_arg (args, char *);
      g_return_if_fail (format != NULL);

      strval = gimp_savable_printf (format, args);
      g_return_if_fail (strval != NULL);

      encoded = g_markup_escape_text (strval, -1);
      g_output_stream_printf (state->output, NULL, NULL, NULL, " %s='%s'", attr, encoded);

      attr = va_arg (args, char *);

      g_free (strval);
      g_free (encoded);
    }
}

static gchar *
gimp_savable_printf (const gchar *format,
                     va_list      args)
{
  gchar *strval;

  g_return_val_if_fail (format != NULL, NULL);

  if (strstr (format, "%s"))
    {
      const gchar *value = va_arg (args, gchar *);
      strval = g_strdup_printf (format, value);
    }
  else if (strstr (format, "%d"))
    {
      gint value = va_arg (args, gint);
      strval = g_strdup_printf (format, value);
    }
  else if (strstr (format, "%lu"))
    {
      gulong value = va_arg (args, gulong);
      strval = g_strdup_printf (format, value);
    }
  else if (strstr (format, "%ld"))
    {
      glong value = va_arg (args, glong);
      strval = g_strdup_printf (format, value);
    }
  else if (strstr (format, "%u"))
    {
      guint value = va_arg (args, guint);
      strval = g_strdup_printf (format, value);
    }
  else if (strstr (format, "%f"))
    {
      gdouble value = va_arg (args, gdouble);
      strval = g_strdup_printf (format, value);
    }
  else if (g_strcmp0 ("%b", format) == 0)
    {
      /* Custom format: boolean type. */
      gboolean value = va_arg (args, gboolean);
      strval = g_strdup (value ? "true" : "false");
    }
  else if (g_strcmp0 ("%t", format) == 0)
    {
      /* Custom format: type name of passed object. */
      GObject *gobject = va_arg (args, GObject *);
      strval = g_strdup (g_type_name (G_TYPE_FROM_INSTANCE (gobject)));
    }
  else if (g_strcmp0 ("%T", format) == 0)
    {
      /* Custom format: type name. */
      GType gtype = va_arg (args, GType);
      strval = g_strdup (g_type_name (gtype));
    }
  else
    {
      gsize flen = strlen (format);

      if (flen > 3 && format[0] == '%' && format[1] == '[' &&
          format[flen - 1] == ']')
        {
          /* Custom format: enum types. */
          gchar *type_name;
          GType  enum_type;
          gint   value = va_arg (args, gint);

          type_name = g_strdup (format + 2);
          type_name[strlen (type_name) - 1] = '\0';

          enum_type = g_type_from_name (type_name);
          g_free (type_name);
          g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
          strval = g_strdup (gimp_get_enum_value_nick (enum_type, value));
        }
      else
        {
          g_return_val_if_reached (NULL);
        }
    }

  return strval;
}
