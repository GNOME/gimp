/* LIGMA - The GNU Image Manipulation Program
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "paint-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-new.h"
#include "core/ligmaitem.h"
#include "core/ligmapickable.h"

#include "ligmasourceoptions.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_SRC_DRAWABLES,
  PROP_SRC_X,
  PROP_SRC_Y,
  PROP_ALIGN_MODE,
  PROP_SAMPLE_MERGED
};


static void   ligma_source_options_finalize     (GObject           *object);
static void   ligma_source_options_set_property (GObject           *object,
                                                guint              property_id,
                                                const GValue      *value,
                                                GParamSpec        *pspec);
static void   ligma_source_options_get_property (GObject           *object,
                                                guint              property_id,
                                                GValue            *value,
                                                GParamSpec        *pspec);

static void
         ligma_source_options_set_src_drawables (LigmaSourceOptions *options,
                                                GList             *drawables);
static void
      ligma_source_options_src_drawable_removed (LigmaDrawable      *drawable,
                                                LigmaSourceOptions *options);
static void  ligma_source_options_make_pickable (LigmaSourceOptions *options);


G_DEFINE_TYPE (LigmaSourceOptions, ligma_source_options, LIGMA_TYPE_PAINT_OPTIONS)

#define parent_class ligma_source_options_parent_class


static void
ligma_source_options_class_init (LigmaSourceOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = ligma_source_options_finalize;
  object_class->set_property = ligma_source_options_set_property;
  object_class->get_property = ligma_source_options_get_property;

  g_object_class_install_property (object_class, PROP_SRC_DRAWABLES,
                                   g_param_spec_pointer ("src-drawables",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SRC_X,
                                   g_param_spec_int ("src-x",
                                                     NULL, NULL,
                                                     G_MININT, G_MAXINT, 0,
                                                     LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SRC_Y,
                                   g_param_spec_int ("src-y",
                                                     NULL, NULL,
                                                     G_MININT, G_MAXINT, 0,
                                                     LIGMA_PARAM_READWRITE));

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_ALIGN_MODE,
                         "align-mode",
                         _("Alignment"),
                         NULL,
                         LIGMA_TYPE_SOURCE_ALIGN_MODE,
                         LIGMA_SOURCE_ALIGN_NO,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                            "sample-merged",
                            _("Sample merged"),
                            NULL,
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_source_options_init (LigmaSourceOptions *options)
{
  options->src_drawables = NULL;
}

static void
ligma_source_options_finalize (GObject *object)
{
  LigmaSourceOptions *options = LIGMA_SOURCE_OPTIONS (object);

  ligma_source_options_set_src_drawables (options, NULL);
  g_clear_object (&options->src_image);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_source_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LigmaSourceOptions *options = LIGMA_SOURCE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SRC_DRAWABLES:
      ligma_source_options_set_src_drawables (options,
                                             g_value_get_pointer (value));
      break;
    case PROP_SRC_X:
      options->src_x = g_value_get_int (value);
      break;
    case PROP_SRC_Y:
      options->src_y = g_value_get_int (value);
      break;
    case PROP_ALIGN_MODE:
      options->align_mode = g_value_get_enum (value);
      break;
    case PROP_SAMPLE_MERGED:
      options->sample_merged = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_source_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  LigmaSourceOptions *options = LIGMA_SOURCE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SRC_DRAWABLES:
      g_value_set_pointer (value, options->src_drawables);
      break;
    case PROP_SRC_X:
      g_value_set_int (value, options->src_x);
      break;
    case PROP_SRC_Y:
      g_value_set_int (value, options->src_y);
      break;
    case PROP_ALIGN_MODE:
      g_value_set_enum (value, options->align_mode);
      break;
    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, options->sample_merged);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_source_options_set_src_drawables (LigmaSourceOptions *options,
                                       GList             *drawables)
{
  LigmaImage *image = NULL;
  GList     *iter;

  if (g_list_length (options->src_drawables) == g_list_length (drawables))
    {
      GList *iter2;

      for (iter = options->src_drawables, iter2 = drawables;
           iter; iter = iter->next, iter2 = iter2->next)
        {
          if (iter->data != iter2->data)
            break;
        }
      if (iter == NULL)
        return;
    }
  for (GList *iter = drawables; iter; iter = iter->next)
    {
      /* Make sure all drawables are from the same image. */
      if (image == NULL)
        image = ligma_item_get_image (LIGMA_ITEM (iter->data));
      else
        g_return_if_fail (image == ligma_item_get_image (LIGMA_ITEM (iter->data)));
    }

  if (options->src_drawables)
    {
      for (GList *iter = options->src_drawables; iter; iter = iter->next)
        g_signal_handlers_disconnect_by_func (iter->data,
                                              ligma_source_options_src_drawable_removed,
                                              options);

      g_list_free (options->src_drawables);
    }

  options->src_drawables = g_list_copy (drawables);

  if (options->src_drawables)
    {
      for (GList *iter = options->src_drawables; iter; iter = iter->next)
        g_signal_connect (iter->data, "removed",
                          G_CALLBACK (ligma_source_options_src_drawable_removed),
                          options);
    }

  ligma_source_options_make_pickable (options);
  g_object_notify (G_OBJECT (options), "src-drawables");
}

static void
ligma_source_options_src_drawable_removed (LigmaDrawable      *drawable,
                                          LigmaSourceOptions *options)
{
  options->src_drawables = g_list_remove (options->src_drawables, drawable);

  g_signal_handlers_disconnect_by_func (drawable,
                                        ligma_source_options_src_drawable_removed,
                                        options);

  ligma_source_options_make_pickable (options);
  g_object_notify (G_OBJECT (options), "src-drawables");
}

static void
ligma_source_options_make_pickable (LigmaSourceOptions *options)
{
  g_clear_object (&options->src_image);
  options->src_pickable = NULL;

  if (options->src_drawables)
    {
      LigmaImage *image;

      image = ligma_item_get_image (LIGMA_ITEM (options->src_drawables->data));

      if (g_list_length (options->src_drawables) > 1)
        {
          /* A composited projection of src_drawables as if they were on
           * their own in the image. Some kind of sample_merged limited
           * to these drawables.
           */
          options->src_image = ligma_image_new_from_drawables (image->ligma, options->src_drawables,
                                                              FALSE, FALSE);
          ligma_container_remove (image->ligma->images, LIGMA_OBJECT (options->src_image));

          options->src_pickable = LIGMA_PICKABLE (options->src_image);
          ligma_pickable_flush (options->src_pickable);
        }
      else
        {
          options->src_pickable = LIGMA_PICKABLE (options->src_drawables->data);
        }
    }
}
