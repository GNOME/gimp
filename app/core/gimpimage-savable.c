/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpImage-save
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
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"
#ifdef G_OS_WIN32
#include <libgimpbase/gimpwin32-io.h>
#endif
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-babl.h"

#include "path/gimppath.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpgrid.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-grid.h"
#include "gimpimage-guides.h"
#include "gimpimage-metadata.h"
#include "gimpimage-private.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-savable.h"
#include "gimpimage-symmetry.h"
#include "gimpimage-undo.h"
#include "gimpparasitelist.h"
#include "gimpsavable.h"
#include "gimpsavable-load.h"
#include "gimpsymmetry.h"
#include "gimpsymmetry-mandala.h"
#include "gimpsymmetry-mirror.h"
#include "gimpsymmetry-tiling.h"
#include "gimptemplate.h"

#include "gimp-intl.h"


static gboolean   gimp_image_enter_xcf         (GimpLoadState        *state,
                                                const gchar         **attribute_names,
                                                const gchar         **attribute_values,
                                                gpointer              user_data,
                                                GError               **error);
static gboolean   gimp_image_enter_spaces      (GimpLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError                **error);
static gboolean   gimp_image_exit_spaces       (GimpLoadState         *state,
                                                const gchar           *text,
                                                gsize                  len,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   gimp_image_enter_project     (GimpLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError                **error);
static gboolean   gimp_image_exit_project      (GimpLoadState         *state,
                                                const gchar           *text,
                                                gsize                  len,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   gimp_image_exit_format       (GimpLoadState         *state,
                                                const gchar           *text,
                                                gsize                  len,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   gimp_image_enter_guides      (GimpLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   gimp_image_enter_symmetries  (GimpLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   gimp_image_exit_symmetry     (GimpLoadState         *state,
                                                const gchar           *text,
                                                gsize                  len,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   gimp_image_enter_parasites   (GimpLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   gimp_image_enter_paths       (GimpLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   gimp_image_enter_channels    (GimpLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);


/* Public Functions */

void
gimp_image_save_to_cache (GimpImage *image,
                          GFile     *xcf_file)
{
  const gchar   *folder;
  GFile         *file;
  GOutputStream *output;
  GError        *error = NULL;
  GimpSaveState  state = { 0 };

  g_return_if_fail (GIMP_IS_IMAGE (image));

  folder = gimp_image_get_cache_folder (image);
  if (g_mkdir_with_parents (folder,
                            S_IRUSR | S_IWUSR | S_IXUSR) == -1)
    {
      g_critical ("%s: failed to create the image cache folder `%s`: %s\n",
                  G_STRFUNC, folder, g_strerror (errno));
      return;
    }
  file   = gimp_image_get_cache_xml_file (image);
  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &error));
  if (output == NULL)
    {
      gimp_message (image->gimp, NULL, GIMP_MESSAGE_ERROR,
                    _("Error creating '%s': %s"),
                    gimp_file_get_utf8_name (file),
                    error->message);
      g_clear_error (&error);
      return;
    }

  state.output   = output;
  state.image    = image;
  state.xcf_file = xcf_file;
  state.spaces   = NULL;
  state.elements = g_queue_new ();

  g_output_stream_printf (output, NULL, NULL, NULL, "<?xml version='1.0' encoding='UTF-8'?>\n");
  gimp_savable_print_element_start (&state, "xcf", "version", "%d", WLBR_VERSION, NULL);
  gimp_savable_save (GIMP_SAVABLE (image), &state);
  gimp_savable_print_element_end (&state, "xcf");

  /* Sanity check: we should be back at root. */
  g_return_if_fail (g_queue_get_length (state.elements) == 0);

  if (! g_output_stream_close (output, NULL, &error))
    {
      gimp_message (image->gimp, NULL, GIMP_MESSAGE_ERROR,
                    _("Error closing '%s': %s"),
                    gimp_file_get_utf8_name (file),
                    error->message);
      g_clear_error (&error);
    }

  g_object_unref (output);
  g_queue_free (state.elements);
}

GimpImage *
gimp_image_load_from_cache (Gimp  *gimp,
                            GFile *backup_dir)
{
  GError        *error = NULL;
  GimpLoadState  state = { 0 };

  if (! gimp_savable_load_parse (&state, gimp, backup_dir, &error))
    {
      g_printerr ("Error loading '%s': %s\n",
                  gimp_file_get_utf8_name (backup_dir),
                  error->message);
      g_clear_error (&error);
    }

  if (state.image)
    {
      gimp_image_undo_enable (state.image);
      gimp_image_flush (state.image);
      gimp_create_display (gimp, state.image, gimp_unit_pixel (), 1.0, NULL);
    }

  gimp_savable_load_free_state (&state);

  return state.image;
}


/* Protected Functions */

void
gimp_image_savable_save (GimpSavable   *savable,
                         GimpSaveState *state)
{
  GimpImage        *image   = GIMP_IMAGE (savable);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
  GList            *iter;

  /* Saving all ICC profiles stored in this XCF. */
  gimp_savable_save_all_spaces (image, state);

  /* Saving the project itself */
  gimp_savable_print_element_start (state, "project", NULL);
  /* To avoid having dozens of <project> attributes, I break the various
   * properties down into sub-elements. This will also make these easier
   * to update in further versions, e.g. if we add concept of infinite
   * canvas, or add multi dimension concept (e.g. multi-page documents
   * whose pages may be different dimensions), if we reorganize how we
   * store some data, such as the print dimensions/pixel density
   * arguments, etc.
   */
  gimp_savable_print_element (state, "dimensions", NULL, NULL,
                              "width",  "%d", private->width,
                              "height", "%d", private->height,
                              NULL);
  gimp_savable_format_save (gimp_image_get_layer_format (image, TRUE), state);

  /* Image Properties */
  if (gimp_image_get_colormap_palette (image))
    {
      GimpPalette *palette = gimp_image_get_colormap_palette (image);
      gimp_savable_save (GIMP_SAVABLE (palette), state);
    }
  if (gimp_image_get_guides (image))
    {
      gimp_savable_print_element_start (state, "guides", NULL);
      iter = gimp_image_get_guides (image);
      for (; iter; iter = iter->next)
        gimp_savable_save (GIMP_SAVABLE (iter->data), state);
      gimp_savable_print_element_end (state, "guides");
    }
  if (gimp_image_get_sample_points (image))
    {
      gimp_savable_print_element_start (state, "sample-points", NULL);
      iter = gimp_image_get_sample_points (image);
      for (; iter; iter = iter->next)
        gimp_savable_save (GIMP_SAVABLE (iter->data), state);
      gimp_savable_print_element_end (state, "sample-points");
    }
  /* TODO: should we make resolution optional? E.g. for images "made for
   * random screen" instead of "made for printing"?
   */
  gimp_savable_print_element (state, "print-dimensions", NULL, NULL,
                              "xres", "%f", private->xresolution,
                              "yres", "%f", private->yresolution,
                              NULL);
  gimp_savable_print_element (state, "tattoo",
                              "%u", (guint) gimp_image_get_tattoo_state (image),
                              NULL);
  /* XXX: maybe we should merge the image unit  with the print
   * dimensions into some element about physical dimensions?
   */
  gimp_savable_unit_save (gimp_image_get_unit (image), state);

  if (gimp_image_get_grid (image))
    gimp_savable_save (GIMP_SAVABLE (gimp_image_get_grid (image)), state);

  if (gimp_image_get_metadata (image))
    gimp_savable_metadata_save (gimp_image_get_metadata (image), state);

  if (g_list_length (gimp_image_symmetry_get (image)))
    {
      gimp_savable_print_element_start (state, "symmetries", NULL);
      for (iter = gimp_image_symmetry_get (image); iter; iter = iter->next)
        {
          GimpSymmetry *symmetry;

          symmetry = GIMP_SYMMETRY (iter->data);
          if (G_TYPE_FROM_INSTANCE (symmetry) == GIMP_TYPE_SYMMETRY)
            /* Do not save the identity symmetry. */
            continue;

          gimp_savable_config_save (GIMP_CONFIG (symmetry), "symmetry", state);
        }
      gimp_savable_print_element_end (state, "symmetries");
    }

  if (gimp_parasite_list_length (private->parasites) > 0 &&
      gimp_parasite_list_persistent_length (private->parasites) > 0)
    gimp_savable_save (GIMP_SAVABLE (private->parasites), state);

  if (private->stored_layer_sets)
    {
      gimp_savable_print_element_start (state, "layer-sets", NULL);
      for (iter = private->stored_layer_sets; iter; iter = iter->next)
        gimp_savable_save (GIMP_SAVABLE (iter->data), state);
      gimp_savable_print_element_end (state, "layer-sets");
    }
  if (private->stored_channel_sets)
    {
      gimp_savable_print_element_start (state, "channel-sets", NULL);
      for (iter = private->stored_channel_sets; iter; iter = iter->next)
        gimp_savable_save (GIMP_SAVABLE (iter->data), state);
      gimp_savable_print_element_end (state, "channel-sets");
    }
  if (private->stored_path_sets)
    {
      gimp_savable_print_element_start (state, "path-sets", NULL);
      for (iter = private->stored_path_sets; iter; iter = iter->next)
        gimp_savable_save (GIMP_SAVABLE (iter->data), state);
      gimp_savable_print_element_end (state, "path-sets");
    }

  gimp_savable_print_element_start (state, "paths", NULL);
  iter = gimp_image_get_path_iter (image);
  for (; iter; iter = iter->next)
    gimp_savable_save (GIMP_SAVABLE (iter->data), state);
  gimp_savable_print_element_end (state, "paths");

  gimp_savable_print_element_start (state, "channels", NULL);
  iter = gimp_image_get_channel_iter (image);
  for (; iter; iter = iter->next)
    gimp_savable_save (GIMP_SAVABLE (iter->data), state);
  gimp_savable_print_element_end (state, "channels");

  gimp_savable_print_element_start (state, "layers", NULL);
  iter = gimp_image_get_layer_iter (image);
  for (; iter; iter = iter->next)
    gimp_savable_save (GIMP_SAVABLE (iter->data), state);
  gimp_savable_print_element_end (state, "layers");

  gimp_savable_print_element_end (state, "project");

  g_clear_pointer (&state->spaces, g_hash_table_unref);
}

void
gimp_image_savable_load (GimpLoadState *state)
{
  gimp_savable_load_add_handlers (state, "xcf",
                                  gimp_image_enter_xcf,
                                  NULL, NULL, NULL);
}

/* Private Functions */

static gboolean
gimp_image_enter_xcf (GimpLoadState  *state,
                      const gchar   **attribute_names,
                      const gchar   **attribute_values,
                      gpointer        user_data,
                      GError         **error)
{
  GHashTable *spaces;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "version") == 0)
        {
          gint version = -1;

          gimp_savable_load_store_from_string (state,
                                               "version", "%d", *attribute_values,
                                               NULL);
          gimp_savable_load_get_values (state, "version", &version, NULL);

          if (version != 1)
            {
              g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                           "%s: unsupported WLBR version %d.",
                           G_STRFUNC, version);
              return FALSE;
            }
        }
      else
        {
          g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                       "%s: unexpected attribute: '%s'",
                       G_STRFUNC, *attribute_names);
        }

      attribute_names++;
      attribute_values++;
    }

  spaces = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  gimp_savable_load_add_handlers (state, "spaces",
                                  gimp_image_enter_spaces,
                                  gimp_image_exit_spaces,
                                  spaces, NULL);
  gimp_savable_load_add_handlers (state, "project",
                                  gimp_image_enter_project,
                                  gimp_image_exit_project,
                                  NULL, NULL);

  return TRUE;
}

static gboolean
gimp_image_enter_spaces (GimpLoadState  *state,
                         const gchar   **attribute_names,
                         const gchar   **attribute_values,
                         gpointer        user_data,
                         GError         **error)
{
  /* <formats> has no attributes. */
  while (*attribute_names)
    {
      g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                   "%s: unexpected attribute: '%s'",
                   G_STRFUNC, *attribute_names);
      attribute_names++;
      attribute_values++;
    }

  gimp_savable_load_add_handlers (state, "space",
                                  gimp_savable_enter_space,
                                  gimp_savable_exit_space,
                                  user_data, NULL);
  return TRUE;
}

static gboolean
gimp_image_exit_spaces (GimpLoadState  *state,
                        const gchar    *text,
                        gsize           len,
                        gpointer        user_data,
                        GError        **error)
{
  GHashTable *spaces = (GHashTable *) user_data;

  if (! state->spaces)
    {
      /* Spaces are referenced all throughout a project so we don't just
       * bubble this up and store it in the load state.
       */
      state->spaces = spaces;
    }
  else
    {
      /* Though not standard, this would allow concatenating several
       * <spaces/> lists.
       */
      for (GList *iter = g_hash_table_get_keys (spaces); iter; iter = iter->next)
        {
          const Babl *space = g_hash_table_lookup (spaces, iter->data);

          g_hash_table_insert (state->spaces, g_strdup (iter->data), (gpointer) space);
        }
    }

  return TRUE;
}

static gboolean
gimp_image_enter_project (GimpLoadState  *state,
                          const gchar   **attribute_names,
                          const gchar   **attribute_values,
                          gpointer        user_data,
                          GError         **error)
{
  gimp_savable_load_add_simple_handler (state, "dimensions", NULL,
                                        NULL, NULL, NULL,
                                        TRUE, TRUE,
                                        "width",  "%d",
                                        "height", "%d",
                                        NULL);
  gimp_savable_load_add_handlers (state, "format",
                                  gimp_savable_enter_format,
                                  gimp_image_exit_format,
                                  NULL, NULL);
  gimp_savable_load_add_handlers (state, "guides",
                                  gimp_image_enter_guides,
                                  NULL, NULL, NULL);

  gimp_savable_load_add_simple_handler (state, "print-dimensions", NULL,
                                        NULL, NULL, NULL,
                                        TRUE, FALSE,
                                        "xres", "%f",
                                        "yres", "%f",
                                        NULL);
  gimp_savable_load_add_simple_handler (state, "tattoo", "%u",
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);
  gimp_savable_load_add_handlers (state, "unit",
                                  gimp_savable_enter_unit,
                                  gimp_savable_exit_unit,
                                  NULL, NULL);

  gimp_savable_load (GIMP_TYPE_GRID, state);

  gimp_savable_load_add_simple_handler (state, "metadata", "%s",
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);
  gimp_savable_load_add_handlers (state, "symmetries",
                                  gimp_image_enter_symmetries,
                                  NULL, NULL, NULL);
  gimp_savable_load_add_handlers (state, "parasites",
                                  gimp_image_enter_parasites,
                                  NULL, NULL, NULL);
  gimp_savable_load_add_handlers (state, "paths",
                                  gimp_image_enter_paths,
                                  NULL, NULL, NULL);
  gimp_savable_load_add_handlers (state, "channels",
                                  gimp_image_enter_channels,
                                  NULL, NULL, NULL);
  return TRUE;
}

static gboolean
gimp_image_exit_project (GimpLoadState  *state,
                         const gchar    *text,
                         gsize           len,
                         gpointer        user_data,
                         GError        **error)
{
  GimpUnit    *unit        = NULL;
  GimpGrid    *grid        = NULL;
  const gchar *meta_string = NULL;
  gdouble      xres        = -1.0;
  gdouble      yres        = -1.0;
  guint        tattoo      = 0;

  gimp_savable_load_get_values (state,
                                "unit", &unit,
                                "print-dimensions:xres", &xres,
                                "print-dimensions:yres", &yres,
                                "tattoo",                &tattoo,
                                "grid",                  &grid,
                                "metadata",              &meta_string,
                                NULL);
  if (unit)
    gimp_image_set_unit (state->image, unit);

  if (xres > 0.0 && yres > 0.0)
    {
      if (xres < GIMP_MIN_RESOLUTION || xres > GIMP_MAX_RESOLUTION ||
          yres < GIMP_MIN_RESOLUTION || yres > GIMP_MAX_RESOLUTION)
        {
          GimpTemplate *template = state->gimp->config->default_image;

          g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_DATA,
                       "Warning, resolution out of range in XCF file");
          xres = gimp_template_get_resolution_x (template);
          yres = gimp_template_get_resolution_y (template);
        }

      gimp_image_set_resolution (state->image, xres, yres);
    }

  if (tattoo > 0)
    gimp_image_set_tattoo_state (state->image, tattoo);

  if (grid)
    gimp_image_set_grid (state->image, grid, FALSE);

  if (meta_string)
    {
      GimpMetadata *metadata;

      metadata = gimp_metadata_deserialize (meta_string);
      if (metadata)
        {
          gimp_image_set_metadata (state->image, metadata, FALSE);
          g_object_unref (metadata);
        }
    }

  return TRUE;
}

static gboolean
gimp_image_exit_format (GimpLoadState  *state,
                        const gchar    *text,
                        gsize           len,
                        gpointer        user_data,
                        GError        **error)
{
  GimpImage         *image;
  const Babl        *format = NULL;
  gint               width  = 0;
  gint               height = 0;
  GimpImageBaseType  image_type;
  GimpPrecision      precision;

  /* Run the generic exit_format() handler, but additionally (in the
   * case of GimpImage load code), actually create the image!
   *
   * XXX Note that this current logic implies that the <dimensions/> and
   * <format/> elements were both happening first and in this order. I'm
   * not sure if we should just enforce element orders with xs:sequence
   * schema.
   */
  gimp_savable_exit_format (state, text, len, user_data, error);
  gimp_savable_load_get_parent_values (state,
                                       "format",            &format,
                                       "dimensions:width",  &width,
                                       "dimensions:height", &height,
                                       NULL);

  image_type = gimp_babl_format_get_base_type (format);
  precision  = gimp_babl_format_get_precision (format);
  image      = gimp_create_image (state->gimp, width, height,
                                  image_type, precision, FALSE);
  gimp_image_undo_disable (image);
  state->image = image;

  return TRUE;
}

static gboolean
gimp_image_enter_guides (GimpLoadState  *state,
                         const gchar   **attribute_names,
                         const gchar   **attribute_values,
                         gpointer        user_data,
                         GError        **error)
{
  if (state->image == NULL)
    {
      /* The image was not created. */
      g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                   "%s: basic dimensions and/or format information "
                   "to create the image were missing.",
                   G_STRFUNC);
      return FALSE;
    }

  gimp_savable_load (GIMP_TYPE_GUIDE, state);

  return TRUE;
}

static gboolean
gimp_image_enter_symmetries (GimpLoadState  *state,
                             const gchar   **attribute_names,
                             const gchar   **attribute_values,
                             gpointer        user_data,
                             GError        **error)
{
  /* Making sure the types are registered and known. */
  (void) GIMP_TYPE_MANDALA;
  (void) GIMP_TYPE_MIRROR;
  (void) GIMP_TYPE_TILING;

  gimp_savable_config_load (GIMP_TYPE_SYMMETRY, "symmetry", state,
                            gimp_image_exit_symmetry,
                            "image", GIMP_TYPE_IMAGE, state->image,
                            NULL);

  return TRUE;
}

static gboolean
gimp_image_exit_symmetry (GimpLoadState  *state,
                          const gchar    *text,
                          gsize           len,
                          gpointer        user_data,
                          GError        **error)
{
  GimpSymmetry *symmetry = NULL;

  gimp_savable_load_get_values (state, "symmetry", &symmetry, NULL);

  if (symmetry)
    {
      gimp_image_symmetry_add (state->image, symmetry);

      g_signal_emit_by_name (symmetry, "active-changed", NULL);
      if (symmetry->active)
        gimp_image_set_active_symmetry (state->image,
                                        G_TYPE_FROM_INSTANCE (symmetry));
    }

  return TRUE;
}

static gboolean
gimp_image_enter_parasites (GimpLoadState  *state,
                            const gchar   **attribute_names,
                            const gchar   **attribute_values,
                            gpointer        user_data,
                            GError        **error)
{
  gimp_savable_parasite_load (state, G_OBJECT (state->image));

  return TRUE;
}

static gboolean
gimp_image_enter_paths (GimpLoadState  *state,
                        const gchar   **attribute_names,
                        const gchar   **attribute_values,
                        gpointer        user_data,
                        GError        **error)
{
  g_return_val_if_fail (gimp_savable_load_peek_active_object (state) == NULL, FALSE);
  gimp_savable_load (GIMP_TYPE_PATH, state);

  return TRUE;
}

static gboolean
gimp_image_enter_channels (GimpLoadState  *state,
                           const gchar   **attribute_names,
                           const gchar   **attribute_values,
                           gpointer        user_data,
                           GError        **error)
{
  g_return_val_if_fail (gimp_savable_load_peek_active_object (state) == NULL, FALSE);
  gimp_savable_load (GIMP_TYPE_CHANNEL, state);

  return TRUE;
}
