/* based on pausep by Daniel Turini
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>

static BOOL
resume_process (DWORD dwOwnerPID)
{
  HANDLE        hThreadSnap = NULL;
  BOOL          bRet        = FALSE;
  THREADENTRY32 te32        = { 0 };

  hThreadSnap = CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0);
  if (hThreadSnap == INVALID_HANDLE_VALUE)
    return FALSE;

  te32.dwSize = sizeof (THREADENTRY32);

  if (Thread32First (hThreadSnap, &te32))
    {
      do
        {
          if (te32.th32OwnerProcessID == dwOwnerPID)
            {
              HANDLE hThread = OpenThread (THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
              printf ("Resuming Thread: %u\n", (unsigned int) te32.th32ThreadID);
              ResumeThread (hThread);
              CloseHandle (hThread);
            }
        }
      while (Thread32Next (hThreadSnap, &te32));
      bRet = TRUE;
    }
  else
    bRet = FALSE;

  CloseHandle (hThreadSnap);

  return bRet;
}

static BOOL
process_list (void)
{
  HANDLE         hProcessSnap = NULL;
  BOOL           bRet      = FALSE;
  PROCESSENTRY32 pe32      = {0};

  hProcessSnap = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

  if (hProcessSnap == INVALID_HANDLE_VALUE)
    return FALSE;

  pe32.dwSize = sizeof (PROCESSENTRY32);

  if (Process32First (hProcessSnap, &pe32))
    {
      do
        {
          printf ("PID:\t%u\t%s\n",
                  (unsigned int) pe32.th32ProcessID,
                  pe32.szExeFile);
        }
      while (Process32Next (hProcessSnap, &pe32));
      bRet = TRUE;
    }
  else
    bRet = FALSE;

  CloseHandle (hProcessSnap);

  return bRet;
}

int
main (int   argc,
      char* argv[])
{
  DWORD pid;

  if (argc <= 1)
    {
      process_list ();
    }
  else if (argc == 2)
    {
      pid = atoi (argv[1]);
      if (pid == 0)
        {
          printf ("invalid: %lu\n", pid);
          return 1;
        }
      else
        {
          printf ("process: %lu\n", pid);
          resume_process (pid);
        }
    }
  else
    {
      printf ("Usage:\n"
              "resume    : show processlist\n"
              "resume PID: resuming thread\n");
    }

  return 0;
}
