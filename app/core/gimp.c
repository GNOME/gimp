/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "core-types.h"

#include "base/tile-manager.h"

#include "pdb/procedural_db.h"

#include "gimp.h"
#include "gimpbrush.h"
#include "gimpbrushgenerated.h"
#include "gimpbrushpipe.h"
#include "gimpbuffer.h"
#include "gimpdatafactory.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimplist.h"
#include "gimppalette.h"
#include "gimppattern.h"
#include "gimptoolinfo.h"

#include "appenv.h"
#include "app_procs.h"
#include "gimpparasite.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


static void   gimp_class_init (GimpClass *klass);
static void   gimp_init       (Gimp      *gimp);

static void   gimp_destroy    (GtkObject *object);


static GimpObjectClass *parent_class = NULL;


GtkType 
gimp_get_type (void)
{
  static GtkType object_type = 0;

  if (! object_type)
    {
      GtkTypeInfo object_info =
      {
        "Gimp",
        sizeof (Gimp),
        sizeof (GimpClass),
        (GtkClassInitFunc) gimp_class_init,
        (GtkObjectInitFunc) gimp_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      object_type = gtk_type_unique (GIMP_TYPE_OBJECT, &object_info);
    }

  return object_type;
}

static void
gimp_class_init (GimpClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);

  object_class->destroy = gimp_destroy;
}

static void
gimp_init (Gimp *gimp)
{
  static const GimpDataFactoryLoaderEntry brush_loader_entries[] =
  {
    { gimp_brush_load,           GIMP_BRUSH_FILE_EXTENSION           },
    { gimp_brush_load,           GIMP_BRUSH_PIXMAP_FILE_EXTENSION    },
    { gimp_brush_generated_load, GIMP_BRUSH_GENERATED_FILE_EXTENSION },
    { gimp_brush_pipe_load,      GIMP_BRUSH_PIPE_FILE_EXTENSION      }
  };
  static gint n_brush_loader_entries = (sizeof (brush_loader_entries) /
					sizeof (brush_loader_entries[0]));

  static const GimpDataFactoryLoaderEntry pattern_loader_entries[] =
  {
    { gimp_pattern_load, GIMP_PATTERN_FILE_EXTENSION }
  };
  static gint n_pattern_loader_entries = (sizeof (pattern_loader_entries) /
					  sizeof (pattern_loader_entries[0]));

  static const GimpDataFactoryLoaderEntry gradient_loader_entries[] =
  {
    { gimp_gradient_load, GIMP_GRADIENT_FILE_EXTENSION },
    { gimp_gradient_load, NULL /* legacy loader */     }
  };
  static gint n_gradient_loader_entries = (sizeof (gradient_loader_entries) /
					   sizeof (gradient_loader_entries[0]));

  static const GimpDataFactoryLoaderEntry palette_loader_entries[] =
  {
    { gimp_palette_load, GIMP_PALETTE_FILE_EXTENSION },
    { gimp_palette_load, NULL /* legacy loader */    }
  };
  static gint n_palette_loader_entries = (sizeof (palette_loader_entries) /
					  sizeof (palette_loader_entries[0]));

  gimp->images = gimp_list_new (GIMP_TYPE_IMAGE,
				GIMP_CONTAINER_POLICY_WEAK);

  gtk_object_ref (GTK_OBJECT (gimp->images));
  gtk_object_sink (GTK_OBJECT (gimp->images));

  gimp->global_buffer = NULL;
  gimp->named_buffers = gimp_list_new (GIMP_TYPE_BUFFER,
				       GIMP_CONTAINER_POLICY_STRONG);

  gtk_object_ref (GTK_OBJECT (gimp->named_buffers));
  gtk_object_sink (GTK_OBJECT (gimp->named_buffers));

  gimp_parasites_init (gimp);

  gimp->brush_factory =
    gimp_data_factory_new (GIMP_TYPE_BRUSH,
			   (const gchar **) &gimprc.brush_path,
			   brush_loader_entries,
			   n_brush_loader_entries,
			   gimp_brush_new,
			   gimp_brush_get_standard);

  gtk_object_ref (GTK_OBJECT (gimp->brush_factory));
  gtk_object_sink (GTK_OBJECT (gimp->brush_factory));

  gimp->pattern_factory =
    gimp_data_factory_new (GIMP_TYPE_PATTERN,
			   (const gchar **) &gimprc.pattern_path,
			   pattern_loader_entries,
			   n_pattern_loader_entries,
			   gimp_pattern_new,
			   gimp_pattern_get_standard);

  gtk_object_ref (GTK_OBJECT (gimp->pattern_factory));
  gtk_object_sink (GTK_OBJECT (gimp->pattern_factory));

  gimp->gradient_factory =
    gimp_data_factory_new (GIMP_TYPE_GRADIENT,
			   (const gchar **) &gimprc.gradient_path,
			   gradient_loader_entries,
			   n_gradient_loader_entries,
			   gimp_gradient_new,
			   gimp_gradient_get_standard);

  gtk_object_ref (GTK_OBJECT (gimp->gradient_factory));
  gtk_object_sink (GTK_OBJECT (gimp->gradient_factory));

  gimp->palette_factory =
    gimp_data_factory_new (GIMP_TYPE_PALETTE,
			   (const gchar **) &gimprc.palette_path,
			   palette_loader_entries,
			   n_palette_loader_entries,
			   gimp_palette_new,
			   gimp_palette_get_standard);

  gtk_object_ref (GTK_OBJECT (gimp->palette_factory));
  gtk_object_sink (GTK_OBJECT (gimp->palette_factory));

  procedural_db_init (gimp);

  gimp->tool_info_list = gimp_list_new (GIMP_TYPE_TOOL_INFO,
					GIMP_CONTAINER_POLICY_STRONG);

  gtk_object_ref (GTK_OBJECT (gimp->tool_info_list));
  gtk_object_sink (GTK_OBJECT (gimp->tool_info_list));
}

static void
gimp_destroy (GtkObject *object)
{
  Gimp *gimp;

  gimp = GIMP (object);

  if (gimp->tool_info_list)
    {
      gtk_object_unref (GTK_OBJECT (gimp->tool_info_list));
      gimp->tool_info_list = NULL;
    }

  procedural_db_free (gimp);

  if (gimp->brush_factory)
    {
      gimp_data_factory_data_free (gimp->brush_factory);
      gtk_object_unref (GTK_OBJECT (gimp->brush_factory));
      gimp->brush_factory = NULL;
    }

  if (gimp->pattern_factory)
    {
      gimp_data_factory_data_free (gimp->pattern_factory);
      gtk_object_unref (GTK_OBJECT (gimp->pattern_factory));
      gimp->pattern_factory = NULL;
    }

  if (gimp->gradient_factory)
    {
      gimp_data_factory_data_free (gimp->gradient_factory);
      gtk_object_unref (GTK_OBJECT (gimp->gradient_factory));
      gimp->gradient_factory = NULL;
    }

  if (gimp->palette_factory)
    {
      gimp_data_factory_data_free (gimp->palette_factory);
      gtk_object_unref (GTK_OBJECT (gimp->palette_factory));
      gimp->palette_factory = NULL;
    }

  gimp_parasites_exit (gimp);

  if (gimp->named_buffers)
    {
      gtk_object_unref (GTK_OBJECT (gimp->named_buffers));
      gimp->named_buffers = NULL;
    }

  if (gimp->global_buffer)
    {
      tile_manager_destroy (gimp->global_buffer);
      gimp->global_buffer = NULL;
    }

  if (gimp->images)
    {
      gtk_object_unref (GTK_OBJECT (gimp->images));
      gimp->images = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

Gimp *
gimp_new (void)
{
  Gimp *gimp;

  gimp = gtk_type_new (GIMP_TYPE_GIMP);

  return gimp;
}

void
gimp_restore (Gimp *gimp)
{
  g_return_if_fail (gimp != NULL);
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /*  initialize  the global parasite table  */
  app_init_update_status (_("Looking for data files"), _("Parasites"), 0.00);
  gimp_parasiterc_load (gimp);

  /*  initialize the list of gimp brushes    */
  app_init_update_status (NULL, _("Brushes"), 0.20);
  gimp_data_factory_data_init (gimp->brush_factory, no_data); 

  /*  initialize the list of gimp patterns   */
  app_init_update_status (NULL, _("Patterns"), 0.40);
  gimp_data_factory_data_init (gimp->pattern_factory, no_data); 

  /*  initialize the list of gimp palettes   */
  app_init_update_status (NULL, _("Palettes"), 0.60);
  gimp_data_factory_data_init (gimp->palette_factory, no_data); 

  /*  initialize the list of gimp gradients  */
  app_init_update_status (NULL, _("Gradients"), 0.80);
  gimp_data_factory_data_init (gimp->gradient_factory, no_data); 

  app_init_update_status (NULL, NULL, 1.00);
}

void
gimp_shutdown (Gimp *gimp)
{
  gimp_data_factory_data_save (gimp->brush_factory);
  gimp_data_factory_data_save (gimp->pattern_factory);
  gimp_data_factory_data_save (gimp->gradient_factory);
  gimp_data_factory_data_save (gimp->palette_factory);
  gimp_parasiterc_save (gimp);
}
