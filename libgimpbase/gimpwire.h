/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_WIRE_H__
#define __GIMP_WIRE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


typedef struct _GimpWireMessage  GimpWireMessage;

typedef void     (* GimpWireReadFunc)    (GIOChannel      *channel,
                                          GimpWireMessage *msg,
                                          gpointer         user_data);
typedef void     (* GimpWireWriteFunc)   (GIOChannel      *channel,
                                          GimpWireMessage *msg,
                                          gpointer         user_data);
typedef void     (* GimpWireDestroyFunc) (GimpWireMessage *msg);
typedef gboolean (* GimpWireIOFunc)      (GIOChannel      *channel,
                                          const guint8    *buf,
                                          gulong           count,
                                          gpointer         user_data);
typedef gboolean (* GimpWireFlushFunc)   (GIOChannel      *channel,
                                          gpointer         user_data);


struct _GimpWireMessage
{
  guint32  type;
  gpointer data;
};


void      gimp_wire_register      (guint32              type,
                                   GimpWireReadFunc     read_func,
                                   GimpWireWriteFunc    write_func,
                                   GimpWireDestroyFunc  destroy_func);

void      gimp_wire_set_reader    (GimpWireIOFunc       read_func);
void      gimp_wire_set_writer    (GimpWireIOFunc       write_func);
void      gimp_wire_set_flusher   (GimpWireFlushFunc    flush_func);

gboolean  gimp_wire_read          (GIOChannel          *channel,
                                   guint8              *buf,
                                   gsize                count,
                                   gpointer             user_data);
gboolean  gimp_wire_write         (GIOChannel          *channel,
                                   const guint8        *buf,
                                   gsize                count,
                                   gpointer             user_data);
gboolean  gimp_wire_flush         (GIOChannel          *channel,
                                   gpointer             user_data);

gboolean  gimp_wire_error         (void);
void      gimp_wire_clear_error   (void);

gboolean  gimp_wire_read_msg      (GIOChannel          *channel,
                                   GimpWireMessage     *msg,
                                   gpointer             user_data);
gboolean  gimp_wire_write_msg     (GIOChannel          *channel,
                                   GimpWireMessage     *msg,
                                   gpointer             user_data);

void      gimp_wire_destroy       (GimpWireMessage     *msg);


/*  for internal use in libgimpbase  */

G_GNUC_INTERNAL gboolean  _gimp_wire_read_int64   (GIOChannel     *channel,
                                                   guint64        *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_read_int32   (GIOChannel     *channel,
                                                   guint32        *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_read_int16   (GIOChannel     *channel,
                                                   guint16        *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_read_int8    (GIOChannel     *channel,
                                                   guint8         *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_read_double  (GIOChannel     *channel,
                                                   gdouble        *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_read_string  (GIOChannel     *channel,
                                                   gchar         **data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_read_color   (GIOChannel     *channel,
                                                   GimpRGB        *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_write_int64  (GIOChannel     *channel,
                                                   const guint64  *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_write_int32  (GIOChannel     *channel,
                                                   const guint32  *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_write_int16  (GIOChannel     *channel,
                                                   const guint16  *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_write_int8   (GIOChannel     *channel,
                                                   const guint8   *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_write_double (GIOChannel     *channel,
                                                   const gdouble  *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_write_string (GIOChannel     *channel,
                                                   gchar         **data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _gimp_wire_write_color  (GIOChannel     *channel,
                                                   const GimpRGB  *data,
                                                   gint            count,
                                                   gpointer        user_data);


G_END_DECLS

#endif /* __GIMP_WIRE_H__ */
