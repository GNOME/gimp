import sys
if 'gdb' in sys.modules:
  import gdb
elif 'lldb' in sys.modules:
  import lldb
  import os

def my_signal_handler(event=None):
  if 'gdb' in sys.modules:
    if (isinstance(event, gdb.SignalEvent)):
      gdb.write("Eeeeeeeeeeeek: in-build GIMP crashed!\n")
      gdb.execute('info threads')
      gdb.execute("thread apply all backtrace full")

  elif 'lldb' in sys.modules:
    target = lldb.debugger.GetSelectedTarget()
    process = target.GetProcess()
    state = process.GetState()
    if state == lldb.eStateStopped:
      thread = process.GetSelectedThread()
      if thread.GetStopReason() == lldb.eStopReasonSignal:
        print("Eeeeeeeeeeeek: in-build GIMP crashed!")
        lldb.debugger.HandleCommand("thread backtrace all")
        os._exit(1)

if 'gdb' in sys.modules:
  gdb.events.stop.connect(my_signal_handler)
  gdb.execute("run")
elif 'lldb' in sys.modules:
  lldb.debugger.HandleCommand("target stop-hook add -o 'script debug_in_build_gimp.my_signal_handler()'")
  lldb.debugger.HandleCommand("process launch")
  lldb.debugger.HandleCommand("quit")
