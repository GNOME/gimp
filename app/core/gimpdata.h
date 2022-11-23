/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadata.h
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DATA_H__
#define __LIGMA_DATA_H__


#include "ligmaviewable.h"


typedef enum
{
  LIGMA_DATA_ERROR_OPEN,   /*  opening data file failed   */
  LIGMA_DATA_ERROR_READ,   /*  reading data file failed   */
  LIGMA_DATA_ERROR_WRITE,  /*  writing data file failed   */
  LIGMA_DATA_ERROR_DELETE  /*  deleting data file failed  */
} LigmaDataError;


#define LIGMA_TYPE_DATA            (ligma_data_get_type ())
#define LIGMA_DATA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DATA, LigmaData))
#define LIGMA_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DATA, LigmaDataClass))
#define LIGMA_IS_DATA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DATA))
#define LIGMA_IS_DATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DATA))
#define LIGMA_DATA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DATA, LigmaDataClass))


typedef struct _LigmaDataPrivate LigmaDataPrivate;
typedef struct _LigmaDataClass   LigmaDataClass;

struct _LigmaData
{
  LigmaViewable     parent_instance;

  LigmaDataPrivate *priv;
};

struct _LigmaDataClass
{
  LigmaViewableClass  parent_class;

  /*  signals  */
  void          (* dirty)         (LigmaData  *data);

  /*  virtual functions  */
  gboolean      (* save)          (LigmaData       *data,
                                   GOutputStream  *output,
                                   GError        **error);
  const gchar * (* get_extension) (LigmaData       *data);
  void          (* copy)          (LigmaData       *data,
                                   LigmaData       *src_data);
  LigmaData    * (* duplicate)     (LigmaData       *data);
  gint          (* compare)       (LigmaData       *data1,
                                   LigmaData       *data2);
};


GType         ligma_data_get_type         (void) G_GNUC_CONST;

gboolean      ligma_data_save             (LigmaData     *data,
                                          GError      **error);

void          ligma_data_dirty            (LigmaData     *data);
void          ligma_data_clean            (LigmaData     *data);
gboolean      ligma_data_is_dirty         (LigmaData     *data);

void          ligma_data_freeze           (LigmaData     *data);
void          ligma_data_thaw             (LigmaData     *data);
gboolean      ligma_data_is_frozen        (LigmaData     *data);

gboolean      ligma_data_delete_from_disk (LigmaData     *data,
                                          GError      **error);

const gchar * ligma_data_get_extension    (LigmaData     *data);

void          ligma_data_set_file         (LigmaData     *data,
                                          GFile        *file,
                                          gboolean      writable,
                                          gboolean      deletable);
GFile       * ligma_data_get_file         (LigmaData     *data);

void          ligma_data_create_filename  (LigmaData     *data,
                                          GFile        *dest_dir);

void          ligma_data_set_folder_tags  (LigmaData     *data,
                                          GFile        *top_directory);

const gchar * ligma_data_get_mime_type    (LigmaData     *data);

gboolean      ligma_data_is_writable      (LigmaData     *data);
gboolean      ligma_data_is_deletable     (LigmaData     *data);

void          ligma_data_set_mtime        (LigmaData     *data,
                                          gint64        mtime);
gint64        ligma_data_get_mtime        (LigmaData     *data);

gboolean      ligma_data_is_copyable      (LigmaData     *data);
void          ligma_data_copy             (LigmaData     *data,
                                          LigmaData     *src_data);

gboolean      ligma_data_is_duplicatable  (LigmaData     *data);
LigmaData    * ligma_data_duplicate        (LigmaData     *data);

void          ligma_data_make_internal    (LigmaData     *data,
                                          const gchar  *identifier);
gboolean      ligma_data_is_internal      (LigmaData     *data);

gint          ligma_data_compare          (LigmaData     *data1,
                                          LigmaData     *data2);

#define LIGMA_DATA_ERROR (ligma_data_error_quark ())

GQuark        ligma_data_error_quark      (void) G_GNUC_CONST;


#endif /* __LIGMA_DATA_H__ */
