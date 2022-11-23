/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligma.h"
#include "ligmatoolinfo.h"
#include "ligmatooloptions.h"
#include "ligmatoolpreset.h"
#include "ligmatoolpreset-load.h"
#include "ligmatoolpreset-save.h"

#include "ligma-intl.h"


/*  The defaults are "everything except color", which is problematic
 *  with gradients, which is why we special case the gradient tool in
 *  ligma_tool_preset_set_options().
 */
#define DEFAULT_USE_FG_BG              FALSE
#define DEFAULT_USE_OPACITY_PAINT_MODE TRUE
#define DEFAULT_USE_BRUSH              TRUE
#define DEFAULT_USE_DYNAMICS           TRUE
#define DEFAULT_USE_MYBRUSH            TRUE
#define DEFAULT_USE_GRADIENT           FALSE
#define DEFAULT_USE_PATTERN            TRUE
#define DEFAULT_USE_PALETTE            FALSE
#define DEFAULT_USE_FONT               TRUE

enum
{
  PROP_0,
  PROP_NAME,
  PROP_LIGMA,
  PROP_TOOL_OPTIONS,
  PROP_USE_FG_BG,
  PROP_USE_OPACITY_PAINT_MODE,
  PROP_USE_BRUSH,
  PROP_USE_DYNAMICS,
  PROP_USE_MYBRUSH,
  PROP_USE_GRADIENT,
  PROP_USE_PATTERN,
  PROP_USE_PALETTE,
  PROP_USE_FONT
};


static void          ligma_tool_preset_config_iface_init    (LigmaConfigInterface *iface);

static void          ligma_tool_preset_constructed          (GObject          *object);
static void          ligma_tool_preset_finalize             (GObject          *object);
static void          ligma_tool_preset_set_property         (GObject          *object,
                                                            guint             property_id,
                                                            const GValue     *value,
                                                            GParamSpec       *pspec);
static void          ligma_tool_preset_get_property         (GObject          *object,
                                                            guint             property_id,
                                                            GValue           *value,
                                                            GParamSpec       *pspec);
static void
             ligma_tool_preset_dispatch_properties_changed  (GObject          *object,
                                                            guint             n_pspecs,
                                                            GParamSpec      **pspecs);

static const gchar * ligma_tool_preset_get_extension        (LigmaData         *data);

static gboolean      ligma_tool_preset_deserialize_property (LigmaConfig       *config,
                                                            guint             property_id,
                                                            GValue           *value,
                                                            GParamSpec       *pspec,
                                                            GScanner         *scanner,
                                                            GTokenType       *expected);

static void          ligma_tool_preset_set_options          (LigmaToolPreset   *preset,
                                                            LigmaToolOptions  *options);
static void          ligma_tool_preset_options_notify       (GObject          *tool_options,
                                                            const GParamSpec *pspec,
                                                            LigmaToolPreset   *preset);
static void     ligma_tool_preset_options_prop_name_changed (LigmaContext         *tool_options,
                                                            LigmaContextPropType  prop,
                                                            LigmaToolPreset      *preset);


G_DEFINE_TYPE_WITH_CODE (LigmaToolPreset, ligma_tool_preset, LIGMA_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_tool_preset_config_iface_init))

#define parent_class ligma_tool_preset_parent_class


static void
ligma_tool_preset_class_init (LigmaToolPresetClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaDataClass *data_class   = LIGMA_DATA_CLASS (klass);

  object_class->constructed                 = ligma_tool_preset_constructed;
  object_class->finalize                    = ligma_tool_preset_finalize;
  object_class->set_property                = ligma_tool_preset_set_property;
  object_class->get_property                = ligma_tool_preset_get_property;
  object_class->dispatch_properties_changed = ligma_tool_preset_dispatch_properties_changed;

  data_class->save                          = ligma_tool_preset_save;
  data_class->get_extension                 = ligma_tool_preset_get_extension;

  LIGMA_CONFIG_PROP_STRING (object_class, PROP_NAME,
                           "name",
                           NULL, NULL,
                           "Unnamed",
                           LIGMA_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_TOOL_OPTIONS,
                           "tool-options",
                           NULL, NULL,
                           LIGMA_TYPE_TOOL_OPTIONS,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_FG_BG,
                            "use-fg-bg",
                            _("Apply stored FG/BG"),
                            NULL,
                            DEFAULT_USE_FG_BG,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_OPACITY_PAINT_MODE,
                            "use-opacity-paint-mode",
                            _("Apply stored opacity/paint mode"),
                            NULL,
                            DEFAULT_USE_OPACITY_PAINT_MODE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_BRUSH,
                            "use-brush",
                            _("Apply stored brush"),
                            NULL,
                            DEFAULT_USE_BRUSH,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_DYNAMICS,
                            "use-dynamics",
                            _("Apply stored dynamics"),
                            NULL,
                            DEFAULT_USE_DYNAMICS,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_MYBRUSH,
                            "use-mypaint-brush",
                            _("Apply stored MyPaint brush"),
                            NULL,
                            DEFAULT_USE_MYBRUSH,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_PATTERN,
                            "use-pattern",
                            _("Apply stored pattern"),
                            NULL,
                            DEFAULT_USE_PATTERN,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_PALETTE,
                            "use-palette",
                            _("Apply stored palette"),
                            NULL,
                            DEFAULT_USE_PALETTE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_GRADIENT,
                            "use-gradient",
                            _("Apply stored gradient"),
                            NULL,
                            DEFAULT_USE_GRADIENT,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_FONT,
                            "use-font",
                            _("Apply stored font"),
                            NULL,
                            DEFAULT_USE_FONT,
                            LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_tool_preset_config_iface_init (LigmaConfigInterface *iface)
{
  iface->deserialize_property = ligma_tool_preset_deserialize_property;
}

static void
ligma_tool_preset_init (LigmaToolPreset *tool_preset)
{
}

static void
ligma_tool_preset_constructed (GObject *object)
{
  LigmaToolPreset *preset = LIGMA_TOOL_PRESET (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_return_if_fail (LIGMA_IS_LIGMA (preset->ligma));
}

static void
ligma_tool_preset_finalize (GObject *object)
{
  LigmaToolPreset *tool_preset = LIGMA_TOOL_PRESET (object);

  ligma_tool_preset_set_options (tool_preset, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_tool_preset_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaToolPreset *tool_preset = LIGMA_TOOL_PRESET (object);

  switch (property_id)
    {
    case PROP_NAME:
      ligma_object_set_name (LIGMA_OBJECT (tool_preset),
                            g_value_get_string (value));
      break;

    case PROP_LIGMA:
      tool_preset->ligma = g_value_get_object (value); /* don't ref */
      break;

    case PROP_TOOL_OPTIONS:
      ligma_tool_preset_set_options (tool_preset,
                                    LIGMA_TOOL_OPTIONS (g_value_get_object (value)));
      break;

    case PROP_USE_FG_BG:
      tool_preset->use_fg_bg = g_value_get_boolean (value);
      break;
    case PROP_USE_OPACITY_PAINT_MODE:
      tool_preset->use_opacity_paint_mode = g_value_get_boolean (value);
      break;
    case PROP_USE_BRUSH:
      tool_preset->use_brush = g_value_get_boolean (value);
      break;
    case PROP_USE_DYNAMICS:
      tool_preset->use_dynamics = g_value_get_boolean (value);
      break;
    case PROP_USE_MYBRUSH:
      tool_preset->use_mybrush = g_value_get_boolean (value);
      break;
    case PROP_USE_PATTERN:
      tool_preset->use_pattern = g_value_get_boolean (value);
      break;
    case PROP_USE_PALETTE:
      tool_preset->use_palette = g_value_get_boolean (value);
      break;
    case PROP_USE_GRADIENT:
      tool_preset->use_gradient = g_value_get_boolean (value);
      break;
    case PROP_USE_FONT:
      tool_preset->use_font = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_tool_preset_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaToolPreset *tool_preset = LIGMA_TOOL_PRESET (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, ligma_object_get_name (tool_preset));
      break;

    case PROP_LIGMA:
      g_value_set_object (value, tool_preset->ligma);
      break;

    case PROP_TOOL_OPTIONS:
      g_value_set_object (value, tool_preset->tool_options);
      break;

    case PROP_USE_FG_BG:
      g_value_set_boolean (value, tool_preset->use_fg_bg);
      break;
    case PROP_USE_OPACITY_PAINT_MODE:
      g_value_set_boolean (value, tool_preset->use_opacity_paint_mode);
      break;
    case PROP_USE_BRUSH:
      g_value_set_boolean (value, tool_preset->use_brush);
      break;
    case PROP_USE_MYBRUSH:
      g_value_set_boolean (value, tool_preset->use_mybrush);
      break;
    case PROP_USE_DYNAMICS:
      g_value_set_boolean (value, tool_preset->use_dynamics);
      break;
    case PROP_USE_PATTERN:
      g_value_set_boolean (value, tool_preset->use_pattern);
      break;
    case PROP_USE_PALETTE:
      g_value_set_boolean (value, tool_preset->use_palette);
      break;
    case PROP_USE_GRADIENT:
      g_value_set_boolean (value, tool_preset->use_gradient);
      break;
    case PROP_USE_FONT:
      g_value_set_boolean (value, tool_preset->use_font);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_tool_preset_dispatch_properties_changed (GObject     *object,
                                              guint        n_pspecs,
                                              GParamSpec **pspecs)
{
  gint i;

  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs, pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      if (pspecs[i]->flags & LIGMA_CONFIG_PARAM_SERIALIZE)
        {
          ligma_data_dirty (LIGMA_DATA (object));
          break;
        }
    }
}

static const gchar *
ligma_tool_preset_get_extension (LigmaData *data)
{
  return LIGMA_TOOL_PRESET_FILE_EXTENSION;
}

static gboolean
ligma_tool_preset_deserialize_property (LigmaConfig *config,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec,
                                       GScanner   *scanner,
                                       GTokenType *expected)
{
  LigmaToolPreset *tool_preset = LIGMA_TOOL_PRESET (config);

  switch (property_id)
    {
    case PROP_TOOL_OPTIONS:
      {
        GObject             *options;
        gchar               *type_name;
        GType                type;
        LigmaContextPropMask  serialize_props;

        if (! ligma_scanner_parse_string (scanner, &type_name))
          {
            *expected = G_TOKEN_STRING;
            break;
          }

        if (! (type_name && *type_name))
          {
            g_scanner_error (scanner, "LigmaToolOptions type name is empty");
            *expected = G_TOKEN_NONE;
            g_free (type_name);
            break;
          }

        if (! strcmp (type_name, "LigmaTransformOptions"))
          {
            g_printerr ("Correcting tool options type LigmaTransformOptions "
                        "to LigmaTransformGridOptions\n");
            g_free (type_name);
            type_name = g_strdup ("LigmaTransformGridOptions");
          }

        type = g_type_from_name (type_name);

        if (! type)
          {
            g_scanner_error (scanner,
                             "unable to determine type of '%s'",
                             type_name);
            *expected = G_TOKEN_NONE;
            g_free (type_name);
            break;
          }

        if (! g_type_is_a (type, LIGMA_TYPE_TOOL_OPTIONS))
          {
            g_scanner_error (scanner,
                             "'%s' is not a subclass of LigmaToolOptions",
                             type_name);
            *expected = G_TOKEN_NONE;
            g_free (type_name);
            break;
          }

        g_free (type_name);

        options = g_object_new (type,
                                "ligma", tool_preset->ligma,
                                NULL);

        /*  Initialize all LigmaContext object properties that can be
         *  used by presets with some non-NULL object, so loading a
         *  broken preset won't leave us with NULL objects that have
         *  bad effects. See bug #742159.
         */
        ligma_context_copy_properties (ligma_get_user_context (tool_preset->ligma),
                                      LIGMA_CONTEXT (options),
                                      LIGMA_CONTEXT_PROP_MASK_BRUSH    |
                                      LIGMA_CONTEXT_PROP_MASK_DYNAMICS |
                                      LIGMA_CONTEXT_PROP_MASK_MYBRUSH  |
                                      LIGMA_CONTEXT_PROP_MASK_PATTERN  |
                                      LIGMA_CONTEXT_PROP_MASK_GRADIENT |
                                      LIGMA_CONTEXT_PROP_MASK_PALETTE  |
                                      LIGMA_CONTEXT_PROP_MASK_FONT);

        if (! LIGMA_CONFIG_GET_IFACE (options)->deserialize (LIGMA_CONFIG (options),
                                                            scanner, 1, NULL))
          {
            *expected = G_TOKEN_NONE;
            g_object_unref (options);
            break;
          }

        /* we need both tool and tool-info on the options */
        if (ligma_context_get_tool (LIGMA_CONTEXT (options)))
          {
            g_object_set (options,
                          "tool-info",
                          ligma_context_get_tool (LIGMA_CONTEXT (options)),
                          NULL);
          }
        else if (LIGMA_TOOL_OPTIONS (options)->tool_info)
          {
            g_object_set (options,
                          "tool", LIGMA_TOOL_OPTIONS (options)->tool_info,
                          NULL);
          }
        else
          {
            /* if we have none, the options set_property() logic will
             * replace the NULL with its best guess
             */
            g_object_set (options,
                          "tool",      NULL,
                          "tool-info", NULL,
                          NULL);
          }

        serialize_props =
          ligma_context_get_serialize_properties (LIGMA_CONTEXT (options));

        ligma_context_set_serialize_properties (LIGMA_CONTEXT (options),
                                               serialize_props |
                                               LIGMA_CONTEXT_PROP_MASK_TOOL);

        g_value_take_object (value, options);
      }
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static void
ligma_tool_preset_set_options (LigmaToolPreset  *preset,
                              LigmaToolOptions *options)
{
  if (preset->tool_options)
    {
      g_signal_handlers_disconnect_by_func (preset->tool_options,
                                            ligma_tool_preset_options_notify,
                                            preset);

      g_signal_handlers_disconnect_by_func (preset->tool_options,
                                            ligma_tool_preset_options_prop_name_changed,
                                            preset);

      g_clear_object (&preset->tool_options);
    }

  if (options)
    {
      LigmaContextPropMask serialize_props;

      preset->tool_options =
        LIGMA_TOOL_OPTIONS (ligma_config_duplicate (LIGMA_CONFIG (options)));

      serialize_props =
        ligma_context_get_serialize_properties (LIGMA_CONTEXT (preset->tool_options));

      ligma_context_set_serialize_properties (LIGMA_CONTEXT (preset->tool_options),
                                             serialize_props |
                                             LIGMA_CONTEXT_PROP_MASK_TOOL);

      if (! (serialize_props & LIGMA_CONTEXT_PROP_MASK_FOREGROUND) &&
          ! (serialize_props & LIGMA_CONTEXT_PROP_MASK_BACKGROUND))
        g_object_set (preset, "use-fg-bg", FALSE, NULL);

      if (! (serialize_props & LIGMA_CONTEXT_PROP_MASK_OPACITY) &&
          ! (serialize_props & LIGMA_CONTEXT_PROP_MASK_PAINT_MODE))
        g_object_set (preset, "use-opacity-paint-mode", FALSE, NULL);

      if (! (serialize_props & LIGMA_CONTEXT_PROP_MASK_BRUSH))
        g_object_set (preset, "use-brush", FALSE, NULL);

      if (! (serialize_props & LIGMA_CONTEXT_PROP_MASK_DYNAMICS))
        g_object_set (preset, "use-dynamics", FALSE, NULL);

      if (! (serialize_props & LIGMA_CONTEXT_PROP_MASK_MYBRUSH))
        g_object_set (preset, "use-mypaint-brush", FALSE, NULL);

      if (! (serialize_props & LIGMA_CONTEXT_PROP_MASK_GRADIENT))
        g_object_set (preset, "use-gradient", FALSE, NULL);

      if (! (serialize_props & LIGMA_CONTEXT_PROP_MASK_PATTERN))
        g_object_set (preset, "use-pattern", FALSE, NULL);

      if (! (serialize_props & LIGMA_CONTEXT_PROP_MASK_PALETTE))
        g_object_set (preset, "use-palette", FALSE, NULL);

      if (! (serialize_props & LIGMA_CONTEXT_PROP_MASK_FONT))
        g_object_set (preset, "use-font", FALSE, NULL);

      /*  see comment above the DEFAULT defines at the top of the file  */
      if (! g_strcmp0 ("ligma-gradient-tool",
                       ligma_object_get_name (preset->tool_options->tool_info)))
        g_object_set (preset, "use-gradient", TRUE, NULL);

      g_signal_connect (preset->tool_options, "notify",
                        G_CALLBACK (ligma_tool_preset_options_notify),
                        preset);

      g_signal_connect (preset->tool_options, "prop-name-changed",
                        G_CALLBACK (ligma_tool_preset_options_prop_name_changed),
                        preset);
    }

  g_object_notify (G_OBJECT (preset), "tool-options");
}

static void
ligma_tool_preset_options_notify (GObject          *tool_options,
                                 const GParamSpec *pspec,
                                 LigmaToolPreset   *preset)
{
  if (pspec->owner_type == LIGMA_TYPE_CONTEXT)
    {
      LigmaContextPropMask serialize_props;

      serialize_props =
        ligma_context_get_serialize_properties (LIGMA_CONTEXT (tool_options));

      if ((1 << pspec->param_id) & serialize_props)
        {
          g_object_notify (G_OBJECT (preset), "tool-options");
        }
    }
  else if (pspec->flags & LIGMA_CONFIG_PARAM_SERIALIZE)
    {
      g_object_notify (G_OBJECT (preset), "tool-options");
    }
}

static void
ligma_tool_preset_options_prop_name_changed (LigmaContext         *tool_options,
                                            LigmaContextPropType  prop,
                                            LigmaToolPreset      *preset)
{
  LigmaContextPropMask serialize_props;

  serialize_props =
    ligma_context_get_serialize_properties (LIGMA_CONTEXT (preset->tool_options));

  if ((1 << prop) & serialize_props)
    {
      g_object_notify (G_OBJECT (preset), "tool-options");
    }
}


/*  public functions  */

LigmaData *
ligma_tool_preset_new (LigmaContext *context,
                      const gchar *unused)
{
  LigmaToolInfo *tool_info;
  const gchar  *icon_name;

  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  tool_info = ligma_context_get_tool (context);

  g_return_val_if_fail (tool_info != NULL, NULL);

  icon_name = ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool_info));

  return g_object_new (LIGMA_TYPE_TOOL_PRESET,
                       "name",         tool_info->label,
                       "icon-name",    icon_name,
                       "ligma",         context->ligma,
                       "tool-options", tool_info->tool_options,
                       NULL);
}

LigmaContextPropMask
ligma_tool_preset_get_prop_mask (LigmaToolPreset *preset)
{
  LigmaContextPropMask serialize_props;
  LigmaContextPropMask use_props = 0;

  g_return_val_if_fail (LIGMA_IS_TOOL_PRESET (preset), 0);

  serialize_props =
    ligma_context_get_serialize_properties (LIGMA_CONTEXT (preset->tool_options));

  if (preset->use_fg_bg)
    {
      use_props |= (LIGMA_CONTEXT_PROP_MASK_FOREGROUND & serialize_props);
      use_props |= (LIGMA_CONTEXT_PROP_MASK_BACKGROUND & serialize_props);
    }

  if (preset->use_opacity_paint_mode)
    {
      use_props |= (LIGMA_CONTEXT_PROP_MASK_OPACITY    & serialize_props);
      use_props |= (LIGMA_CONTEXT_PROP_MASK_PAINT_MODE & serialize_props);
    }

  if (preset->use_brush)
    use_props |= (LIGMA_CONTEXT_PROP_MASK_BRUSH & serialize_props);

  if (preset->use_dynamics)
    use_props |= (LIGMA_CONTEXT_PROP_MASK_DYNAMICS & serialize_props);

  if (preset->use_mybrush)
    use_props |= (LIGMA_CONTEXT_PROP_MASK_MYBRUSH & serialize_props);

  if (preset->use_pattern)
    use_props |= (LIGMA_CONTEXT_PROP_MASK_PATTERN & serialize_props);

  if (preset->use_palette)
    use_props |= (LIGMA_CONTEXT_PROP_MASK_PALETTE & serialize_props);

  if (preset->use_gradient)
    use_props |= (LIGMA_CONTEXT_PROP_MASK_GRADIENT & serialize_props);

  if (preset->use_font)
    use_props |= (LIGMA_CONTEXT_PROP_MASK_FONT & serialize_props);

  return use_props;
}
