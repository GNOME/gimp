def my_signal_handler (event):
  if (isinstance(event, gdb.SignalEvent)):
    gdb.write("Eeeeeeeeeeeek: in-build GIMP crashed!\n")
    gdb.execute('info threads')
    gdb.execute("thread apply all backtrace full")

gdb.events.stop.connect(my_signal_handler)
gdb.execute("run")
