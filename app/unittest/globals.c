/* GLOBAL data */
int no_interface;
int no_data;
int no_splash;
int no_splash_image;
int be_verbose;
int use_shm;
int use_debug_handler;
int console_messages;
int restore_session;
GimpSet* image_context;

MessageHandlerType message_handler;

char *prog_name;		/* The path name we are invoked with */
char *alternate_gimprc;
char *alternate_system_gimprc;
char **batch_cmds;

