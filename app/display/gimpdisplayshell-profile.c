/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-babl.h"

#include "core/gimpimage.h"
#include "core/gimpprojectable.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-actions.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-profile.h"
#include "gimpdisplayxfer.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_display_shell_profile_free        (GimpDisplayShell *shell);

static void   gimp_display_shell_color_config_notify (GimpColorConfig  *config,
                                                      const GParamSpec *pspec,
                                                      GimpDisplayShell *shell);


/*  public functions  */

void
gimp_display_shell_profile_init (GimpDisplayShell *shell)
{
  GimpColorConfig *color_config;

  color_config = GIMP_CORE_CONFIG (shell->display->config)->color_management;

  shell->color_config = gimp_config_duplicate (GIMP_CONFIG (color_config));

  /* use after so we are called after the profile cache is invalidated
   * in gimp_widget_get_color_transform()
   */
  g_signal_connect_after (shell->color_config, "notify",
                          G_CALLBACK (gimp_display_shell_color_config_notify),
                          shell);
}

void
gimp_display_shell_profile_finalize (GimpDisplayShell *shell)
{
  g_clear_object (&shell->color_config);

  gimp_display_shell_profile_free (shell);
}

void
gimp_display_shell_profile_update (GimpDisplayShell *shell)
{
  GimpImage        *image;
  GimpColorProfile *src_profile;
  const Babl       *src_format;
  GimpColorProfile *filter_profile;
  const Babl       *filter_format;
  const Babl       *dest_format;

  gimp_display_shell_profile_free (shell);

  image = gimp_display_get_image (shell->display);

  if (! image)
    return;

  src_profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (shell));

  if (! src_profile)
    return;

  src_format = gimp_projectable_get_format (GIMP_PROJECTABLE (image));

  if (gimp_display_shell_has_filter (shell))
    {
      filter_format  = shell->filter_format;
      filter_profile = gimp_babl_format_get_color_profile (filter_format);
    }
  else
    {
      filter_format  = src_format;
      filter_profile = src_profile;
    }

  if (! gimp_display_shell_profile_can_convert_to_u8 (shell))
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
              gimp_color_profile_get_label (src_profile),
              babl_get_name (src_format),
              gimp_color_profile_get_label (filter_profile),
              babl_get_name (filter_format),
              babl_get_name (dest_format));
#endif

  if (! gimp_color_transform_can_gegl_copy (src_profile, filter_profile))
    {
      shell->filter_transform =
        gimp_color_transform_new (src_profile,
                                  src_format,
                                  filter_profile,
                                  filter_format,
                                  GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                  GIMP_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION |
                                  GIMP_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE);
    }

  shell->profile_transform =
    gimp_widget_get_color_transform (gtk_widget_get_toplevel (GTK_WIDGET (shell)),
                                     gimp_display_shell_get_color_config (shell),
                                     filter_profile,
                                     filter_format,
                                     dest_format);

  if (shell->filter_transform || shell->profile_transform)
    {
      gint w = GIMP_DISPLAY_RENDER_BUF_WIDTH;
      gint h = GIMP_DISPLAY_RENDER_BUF_HEIGHT;

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
gimp_display_shell_profile_can_convert_to_u8 (GimpDisplayShell *shell)
{
  GimpImage *image = gimp_display_get_image (shell->display);

  if (image)
    {
      GimpComponentType component_type;

      if (! gimp_display_shell_has_filter (shell))
        component_type = gimp_image_get_component_type (image);
      else
        component_type = gimp_babl_format_get_component_type (shell->filter_format);

      switch (component_type)
        {
        case GIMP_COMPONENT_TYPE_U8:
#if 0
          /* would like to convert directly for these too, but it
           * produces inferior results, see bug 750874
           */
        case GIMP_COMPONENT_TYPE_U16:
        case GIMP_COMPONENT_TYPE_U32:
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
gimp_display_shell_profile_free (GimpDisplayShell *shell)
{
  g_clear_object (&shell->profile_transform);
  g_clear_object (&shell->filter_transform);
  g_clear_object (&shell->profile_buffer);
  shell->profile_data   = NULL;
  shell->profile_stride = 0;
}

static void
gimp_display_shell_color_config_notify (GimpColorConfig  *config,
                                        const GParamSpec *pspec,
                                        GimpDisplayShell *shell)
{
  if (! strcmp (pspec->name, "mode")                                    ||
      ! strcmp (pspec->name, "display-rendering-intent")                ||
      ! strcmp (pspec->name, "display-use-black-point-compensation")    ||
      ! strcmp (pspec->name, "simulation-rendering-intent")             ||
      ! strcmp (pspec->name, "simulation-use-black-point-compensation") ||
      ! strcmp (pspec->name, "simulation-gamut-check"))
    {
      gboolean     managed   = FALSE;
      gboolean     softproof = FALSE;
      const gchar *action    = NULL;

#define SET_SENSITIVE(action, sensitive) \
      gimp_display_shell_set_action_sensitive (shell, action, sensitive);

#define SET_ACTIVE(action, active) \
      gimp_display_shell_set_action_active (shell, action, active);

      switch (gimp_color_config_get_mode (config))
        {
        case GIMP_COLOR_MANAGEMENT_OFF:
          break;

        case GIMP_COLOR_MANAGEMENT_DISPLAY:
          managed = TRUE;
          break;

        case GIMP_COLOR_MANAGEMENT_SOFTPROOF:
          managed   = TRUE;
          softproof = TRUE;
          break;
        }

      SET_ACTIVE ("view-color-management-enable",    managed);
      SET_ACTIVE ("view-color-management-softproof", softproof);

      switch (gimp_color_config_get_display_intent (config))
        {
        case GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL:
          action = "view-display-intent-perceptual";
          break;

        case GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC:
          action = "view-display-intent-relative-colorimetric";
          break;

        case GIMP_COLOR_RENDERING_INTENT_SATURATION:
          action = "view-display-intent-saturation";
          break;

        case GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC:
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
                     gimp_color_config_get_display_bpc (config));

      SET_SENSITIVE ("view-softproof-profile", softproof);

      switch (gimp_color_config_get_simulation_intent (config))
        {
        case GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL:
          action = "view-softproof-intent-perceptual";
          break;

        case GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC:
          action = "view-softproof-intent-relative-colorimetric";
          break;

        case GIMP_COLOR_RENDERING_INTENT_SATURATION:
          action = "view-softproof-intent-saturation";
          break;

        case GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC:
          action = "view-softproof-intent-absolute-colorimetric";
          break;
        }

      SET_SENSITIVE ("view-softproof-intent-perceptual",            softproof);
      SET_SENSITIVE ("view-softproof-intent-relative-colorimetric", softproof);
      SET_SENSITIVE ("view-softproof-intent-saturation",            softproof);
      SET_SENSITIVE ("view-softproof-intent-absolute-colorimetric", softproof);

      SET_ACTIVE (action, TRUE);

      SET_SENSITIVE ("view-softproof-black-point-compensation", softproof);
      SET_ACTIVE    ("view-softproof-black-point-compensation",
                     gimp_color_config_get_simulation_bpc (config));

      SET_SENSITIVE ("view-softproof-gamut-check", softproof);
      SET_ACTIVE    ("view-softproof-gamut-check",
                     gimp_color_config_get_simulation_gamut_check (config));
    }

  gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (shell));
}
