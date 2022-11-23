/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"

#include "config/ligmacoreconfig.h"

#include "gegl/ligma-babl.h"

#include "core/ligmaimage.h"
#include "core/ligmaprojectable.h"

#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-actions.h"
#include "ligmadisplayshell-filter.h"
#include "ligmadisplayshell-profile.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_display_shell_profile_free        (LigmaDisplayShell *shell);

static void   ligma_display_shell_color_config_notify (LigmaColorConfig  *config,
                                                      const GParamSpec *pspec,
                                                      LigmaDisplayShell *shell);


/*  public functions  */

void
ligma_display_shell_profile_init (LigmaDisplayShell *shell)
{
  LigmaColorConfig *color_config;

  color_config = LIGMA_CORE_CONFIG (shell->display->config)->color_management;

  shell->color_config = ligma_config_duplicate (LIGMA_CONFIG (color_config));

  /* use after so we are called after the profile cache is invalidated
   * in ligma_widget_get_color_transform()
   */
  g_signal_connect_after (shell->color_config, "notify",
                          G_CALLBACK (ligma_display_shell_color_config_notify),
                          shell);
}

void
ligma_display_shell_profile_finalize (LigmaDisplayShell *shell)
{
  g_clear_object (&shell->color_config);

  ligma_display_shell_profile_free (shell);
}

void
ligma_display_shell_profile_update (LigmaDisplayShell *shell)
{
  LigmaImage               *image;
  LigmaColorProfile        *src_profile;
  const Babl              *src_format;
  LigmaColorProfile        *filter_profile;
  const Babl              *filter_format;
  const Babl              *dest_format;
  LigmaColorProfile        *proof_profile;
  LigmaColorRenderingIntent simulation_intent =
    LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  gboolean                 simulation_bpc    = FALSE;

  ligma_display_shell_profile_free (shell);

  image = ligma_display_get_image (shell->display);

  if (! image)
    return;

  src_profile = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (shell));

  if (! src_profile)
    return;

  proof_profile = ligma_color_managed_get_simulation_profile (LIGMA_COLOR_MANAGED (image));
  simulation_intent = ligma_color_managed_get_simulation_intent (LIGMA_COLOR_MANAGED (image));
  simulation_bpc = ligma_color_managed_get_simulation_bpc (LIGMA_COLOR_MANAGED (image));

  src_format = ligma_projectable_get_format (LIGMA_PROJECTABLE (image));

  if (ligma_display_shell_has_filter (shell))
    {
      filter_format  = shell->filter_format;
      filter_profile = shell->filter_profile;
    }
  else
    {
      filter_format  = src_format;
      filter_profile = src_profile;
    }

  if (! ligma_display_shell_profile_can_convert_to_u8 (shell))
    {
      dest_format = shell->filter_format;
    }
  else
    {
      dest_format = babl_format ("R'G'B'A u8");
    }

#if 0
  g_printerr ("src_profile:    %s\n"
              "src_format:     %s\n"
              "filter_profile: %s\n"
              "filter_format:  %s\n"
              "dest_format:    %s\n",
              ligma_color_profile_get_label (src_profile),
              babl_get_name (src_format),
              ligma_color_profile_get_label (filter_profile),
              babl_get_name (filter_format),
              babl_get_name (dest_format));
#endif

  if (! ligma_color_transform_can_gegl_copy (src_profile, filter_profile))
    {
      shell->filter_transform =
        ligma_color_transform_new (src_profile,
                                  src_format,
                                  filter_profile,
                                  filter_format,
                                  LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                  LIGMA_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION |
                                  LIGMA_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE);
    }

  shell->profile_transform =
    ligma_widget_get_color_transform (gtk_widget_get_toplevel (GTK_WIDGET (shell)),
                                     ligma_display_shell_get_color_config (shell),
                                     filter_profile,
                                     filter_format,
                                     dest_format,
                                     proof_profile,
                                     simulation_intent,
                                     simulation_bpc);

  if (shell->filter_transform || shell->profile_transform)
    {
      gint w = shell->render_buf_width;
      gint h = shell->render_buf_height;

      shell->profile_data =
        gegl_malloc (w * h * babl_format_get_bytes_per_pixel (src_format));

      shell->profile_stride =
        w * babl_format_get_bytes_per_pixel (src_format);

      shell->profile_buffer =
        gegl_buffer_linear_new_from_data (shell->profile_data,
                                          src_format,
                                          GEGL_RECTANGLE (0, 0, w, h),
                                          GEGL_AUTO_ROWSTRIDE,
                                          (GDestroyNotify) gegl_free,
                                          shell->profile_data);
    }
}

gboolean
ligma_display_shell_profile_can_convert_to_u8 (LigmaDisplayShell *shell)
{
  LigmaImage *image = ligma_display_get_image (shell->display);

  if (image)
    {
      LigmaComponentType component_type;

      if (! ligma_display_shell_has_filter (shell))
        component_type = ligma_image_get_component_type (image);
      else
        component_type = ligma_babl_format_get_component_type (shell->filter_format);

      switch (component_type)
        {
        case LIGMA_COMPONENT_TYPE_U8:
#if 0
          /* would like to convert directly for these too, but it
           * produces inferior results, see bug 750874
           */
        case LIGMA_COMPONENT_TYPE_U16:
        case LIGMA_COMPONENT_TYPE_U32:
#endif
          return TRUE;

        default:
          break;
        }
    }

  return FALSE;
}


/*  private functions  */

static void
ligma_display_shell_profile_free (LigmaDisplayShell *shell)
{
  g_clear_object (&shell->profile_transform);
  g_clear_object (&shell->filter_transform);
  g_clear_object (&shell->profile_buffer);
  shell->profile_data   = NULL;
  shell->profile_stride = 0;
}

static void
ligma_display_shell_color_config_notify (LigmaColorConfig  *config,
                                        const GParamSpec *pspec,
                                        LigmaDisplayShell *shell)
{
  if (! strcmp (pspec->name, "mode")                                    ||
      ! strcmp (pspec->name, "display-rendering-intent")                ||
      ! strcmp (pspec->name, "display-use-black-point-compensation")    ||
      ! strcmp (pspec->name, "simulation-gamut-check"))
    {
      gboolean     managed   = FALSE;
      gboolean     softproof = FALSE;
      const gchar *action    = NULL;

#define SET_SENSITIVE(action, sensitive) \
      ligma_display_shell_set_action_sensitive (shell, action, sensitive);

#define SET_ACTIVE(action, active) \
      ligma_display_shell_set_action_active (shell, action, active);

      switch (ligma_color_config_get_mode (config))
        {
        case LIGMA_COLOR_MANAGEMENT_OFF:
          break;

        case LIGMA_COLOR_MANAGEMENT_DISPLAY:
          managed = TRUE;
          break;

        case LIGMA_COLOR_MANAGEMENT_SOFTPROOF:
          managed   = TRUE;
          softproof = TRUE;
          break;
        }

      SET_ACTIVE ("view-color-management-enable", managed);
      SET_ACTIVE ("view-color-management-softproof", softproof);

      switch (ligma_color_config_get_display_intent (config))
        {
        case LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL:
          action = "view-display-intent-perceptual";
          break;

        case LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC:
          action = "view-display-intent-relative-colorimetric";
          break;

        case LIGMA_COLOR_RENDERING_INTENT_SATURATION:
          action = "view-display-intent-saturation";
          break;

        case LIGMA_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC:
          action = "view-display-intent-absolute-colorimetric";
          break;
        }

      SET_SENSITIVE ("view-display-intent-perceptual",            managed);
      SET_SENSITIVE ("view-display-intent-relative-colorimetric", managed);
      SET_SENSITIVE ("view-display-intent-saturation",            managed);
      SET_SENSITIVE ("view-display-intent-absolute-colorimetric", managed);

      SET_ACTIVE (action, TRUE);

      SET_SENSITIVE ("view-display-black-point-compensation", managed);
      SET_ACTIVE    ("view-display-black-point-compensation",
                     ligma_color_config_get_display_bpc (config));
      SET_SENSITIVE ("view-softproof-gamut-check", softproof);
      SET_ACTIVE    ("view-softproof-gamut-check",
                     ligma_color_config_get_simulation_gamut_check (config));
    }

  ligma_color_managed_profile_changed (LIGMA_COLOR_MANAGED (shell));
}
