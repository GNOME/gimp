#ifndef __GIMP_WIRE_H__
#define __GIMP_WIRE_H__


#include <glib.h>


typedef struct _WireMessage  WireMessage;

typedef void (* WireReadFunc)    (int fd, WireMessage *msg);
typedef void (* WireWriteFunc)   (int fd, WireMessage *msg);
typedef void (* WireDestroyFunc) (WireMessage *msg);
typedef int  (* WireIOFunc)      (int fd, guint8 *buf, gulong count);
typedef int  (* WireFlushFunc)   (int fd);


struct _WireMessage
{
  guint32 type;
  gpointer data;
};


void wire_register     (guint32          type,
		        WireReadFunc     read_func,
		        WireWriteFunc    write_func,
		        WireDestroyFunc  destroy_func);
void wire_set_reader   (WireIOFunc       read_func);
void wire_set_writer   (WireIOFunc       write_func);
void wire_set_flusher  (WireFlushFunc    flush_func);
int  wire_read         (int              fd,
			guint8          *buf,
			gulong           count);
int  wire_write        (int              fd,
			guint8          *buf,
			gulong           count);
int  wire_flush        (int              fd);
int  wire_error        (void);
void wire_clear_error  (void);
int  wire_read_msg     (int              fd,
		        WireMessage     *msg);
int  wire_write_msg    (int              fd,
		        WireMessage     *msg);
void wire_destroy      (WireMessage     *msg);
int  wire_read_int32   (int              fd,
		        guint32         *data,
		        gint             count);
int  wire_read_int16   (int              fd,
		        guint16         *data,
		        gint             count);
int  wire_read_int8    (int              fd,
		        guint8          *data,
		        gint             count);
int  wire_read_double  (int              fd,
		        gdouble         *data,
		        gint             count);
int  wire_read_string  (int              fd,
			gchar          **data,
			gint             count);
int  wire_write_int32  (int              fd,
		        guint32         *data,
		        gint             count);
int  wire_write_int16  (int              fd,
		        guint16         *data,
		        gint             count);
int  wire_write_int8   (int              fd,
		        guint8          *data,
		        gint             count);
int  wire_write_double (int              fd,
			gdouble         *data,
			gint             count);
int  wire_write_string (int              fd,
			gchar          **data,
			gint             count);


#endif /* __GIMP_WIRE_H__ */
