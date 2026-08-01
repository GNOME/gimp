#GDB mode
try:
  import gdb

  #get stacktrace from process
  def my_signal_handler (event):
    if (isinstance(event, gdb.SignalEvent)):
      gdb.write("Eeeeeeeeeeeek: in-build GIMP crashed!\n")
      gdb.execute('info threads')
      gdb.execute("thread apply all backtrace full")

  #watch process
  gdb.events.stop.connect(my_signal_handler)

  #run process
  gdb.execute("run")


#LLDB mode
except ImportError:
  import lldb
  import os
  import sys
  import time

  def run_target_and_catch_crash(debugger):
    target = debugger.GetSelectedTarget()
    if target.IsValid():
      #watch process
      listener = lldb.SBListener("my_signal_handler")
      error = lldb.SBError()

      #run process
      process = target.Launch(listener, None, None, None, None, None, None, 0, False, error)

      #get stacktrace from process
      #FIXME: https://github.com/llvm/llvm-project/issues/125355
      for module in target.module_iter():
        module_spec = module.GetFileSpec()
        module_dir = module_spec.GetDirectory()
        module_name = module_spec.GetFilename()
        if module_dir and module_name:
          pdb_path = os.path.join(module_dir, f"{os.path.splitext(module_name)[0]}.pdb")
          if os.path.exists(pdb_path):
            debugger.GetCommandInterpreter().HandleCommand(f'target symbols add "{pdb_path}"', lldb.SBCommandReturnObject())
      if error.Success():
        event = lldb.SBEvent()
        while True:
          if listener.WaitForEvent(lldb.UINT32_MAX, event):
            state = lldb.SBProcess.GetStateFromEvent(event)
            if state == lldb.eStateStopped:
              for thread in process:
                if thread.GetStopReason() in (lldb.eStopReasonSignal, lldb.eStopReasonException):
                  sys.stdout.write("Eeeeeeeeeeeek: in-build GIMP crashed!\n")
                  #print manually to avoid 'command requires a current process' error
                  for t in process:
                    sys.stdout.write(f"\nThread {t.GetIndexID()} (Thread ID {t.GetThreadID()}):\n")
                    for frame in t:
                      sys.stdout.write(f"  {frame}\n")
                  #._exit wizardry is needed since lldb --batch does not return 1,
                  #.flush and .sleep fix incomplete output before returning 1
                  sys.stdout.flush()
                  sys.stderr.flush()
                  time.sleep(0.1)
                  os._exit(1)
            elif state == lldb.eStateCrashed:
              os._exit(1)
            elif state == lldb.eStateExited:
              os._exit(process.GetExitStatus())

  lldb.debugger.SetAsync(True)
  run_target_and_catch_crash(lldb.debugger)
