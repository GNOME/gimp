/*
 * "$Id$"
 *
 *   Printer maintenance utility for Epson Stylus printers
 *
 *   Copyright 2000 Robert Krawitz (rlk@alum.mit.edu)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#ifdef __GNU_LIBRARY__
#include <getopt.h>
#endif

char *banner = "\
Copyright 2000 Robert Krawitz (rlk@alum.mit.edu)\n\
\n\
This program is free software; you can redistribute it and/or modify it\n\
under the terms of the GNU General Public License as published by the Free\n\
Software Foundation; either version 2 of the License, or (at your option)\n\
any later version.\n\
\n\
This program is distributed in the hope that it will be useful, but\n\
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY\n\
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License\n\
for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program; if not, write to the Free Software\n\
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n";


#ifdef __GNU_LIBRARY__

struct option optlist[] =
{
  { "printer-name",	1,	NULL,	(int) 'P' },
  { "raw-device",	1,	NULL,	(int) 'r' },
  { "ink-level",	0,	NULL,	(int) 'i' },
  { "clean-head",	0,	NULL,	(int) 'c' },
  { "nozzle-check",	0,	NULL,	(int) 'n' },
  { "align-head",	0,	NULL,	(int) 'a' },
  { "usb",		0,	NULL,	(int) 'u' },
  { "help",		0,	NULL,	(int) 'h' },
  { "identify",		0,	NULL,	(int) 'd' },
  { "model",		1,	NULL,	(int) 'm' },
  { "quiet",		0,	NULL,	(int) 'q' },
  { NULL,		0,	NULL,	0 	  }
};

char *help_msg = "\
Usage: escputil [-P printer | -r device] [-m model] [-u]\n\
                [-c | -n | -a | -i] [-q]\n\
    -P|--printer-name  Specify the name of the printer to operate on.\n\
                       Default is the default system printer.\n\
    -r|--raw-device    Specify the name of the device to write to directly\n\
                       rather than going through a printer queue.\n\
    -c|--clean-head    Clean the print head.\n\
    -n|--nozzle-check  Print a nozzle test pattern.\n\
                       Dirty or clogged nozzles will show as gaps in the\n\
                       pattern.  If you see any gaps, you should run a\n\
                       head cleaning pass.\n\
    -a|--align-head    Align the print head.  CAUTION: Misuse of this\n\
                       utility may result in poor print quality and/or\n\
                       damage to the printer.\n\
    -i|--ink-level     Obtain the ink level from the printer.  This requires\n\
                       read/write access to the raw printer device.\n\
    -d|--identify      Query the printer for make and model information.\n\
                       This requires read/write access to the raw printer\n\
                       device.\n\
    -u|--usb           The printer is connected via USB.\n\
    -h|--help          Print this help message.\n\
    -q|--quiet         Suppress the banner.\n\
    -m|--model         Specify the precise printer model for head alignment.\n";
#else
char *help_msg = "\
Usage: escputil [-P printer | -r device] [-u] [-c | -n | -a | -i] [-q]\n\
    -P Specify the name of the printer to operate on.\n\
          Default is the default system printer.\n\
    -r Specify the name of the device to write to directly\n\
          rather than going through a printer queue.\n\
    -c Clean the print head.\n\
    -n Print a nozzle test pattern.\n\
          Dirty or clogged nozzles will show as gaps in the\n\
          pattern.  If you see any gaps, you should run a\n\
          head cleaning pass.\n\
    -a Align the print head.  CAUTION: Misuse of this\n\
          utility may result in poor print quality and/or\n\
          damage to the printer.\n\
    -i Obtain the ink level from the printer.  This requires\n\
          read/write access to the raw printer device.\n\
    -d Query the printer for make and model information.  This\n\
          requires read/write access to the raw printer device.\n\
    -u The printer is connected via USB.\n\
    -h Print this help message.\n\
    -q Suppress the banner.\n\
    -m Specify the precise printer model for head alignment.\n";
#endif

typedef struct
{
  char *short_name;
  char *long_name;
  int passes;
  int choices;
} printer_t;

printer_t printer_list[] =
{
  { "color",	"Stylus Color",		1,	7 },
  { "pro",	"Stylus Color Pro",	1,	7 },
  { "pro-xl",	"Stylus Color Pro XL",	1,	7 },
  { "400",	"Stylus Color 400",	1,	7 },
  { "440",	"Stylus Color 440",	1,	15 },
  { "460",	"Stylus Color 460",	1,	15 },
  { "480",	"Stylus Color 480",	1,	15 },
  { "500",	"Stylus Color 500",	1,	7 },
  { "600",	"Stylus Color 600",	1,	7 },
  { "640",	"Stylus Color 640",	1,	15 },
  { "660",	"Stylus Color 660",	1,	15 },
  { "670",	"Stylus Color 670",	3,	15 },
  { "740",	"Stylus Color 740",	3,	15 },
  { "760",	"Stylus Color 760",	3,	15 },
  { "800",	"Stylus Color 800",	1,	7 },
  { "850",	"Stylus Color 850",	1,	7 },
  { "860",	"Stylus Color 860",	3,	15 },
  { "880",	"Stylus Color 880",	3,	15 },
  { "900",	"Stylus Color 900",	3,	15 },
  { "980",	"Stylus Color 980",	3,	15 },
  { "1160",	"Stylus Color 1160",	3,	15 },
  { "1500",	"Stylus Color 1500",	1,	7 },
  { "1520",	"Stylus Color 1520",	1,	7 },
  { "3000",	"Stylus Color 3000",	1,	7 },
  { "photo",	"Stylus Photo",		1,	7 },
  { "700",	"Stylus Photo 700",	1,	7 },
  { "ex",	"Stylus Photo EX",	1,	7 },
  { "720",	"Stylus Photo 720",	3,	15 },
  { "750",	"Stylus Photo 750",	3,	15 },
  { "870",	"Stylus Photo 870",	3,	15 },
  { "1200",	"Stylus Photo 1200",	3,	15 },
  { "1270",	"Stylus Photo 1270",	3,	15 },
  { "2000",	"Stylus Photo 2000P",	2,	15 },
  { NULL,	NULL,			0,	0 }
};

void initialize_print_cmd(void);
void do_head_clean(void);
void do_nozzle_check(void);
void do_align(void);
void do_ink_level(void);
void do_identify(void);

char *printer = NULL;
char *raw_device = NULL;
char *printer_model = NULL;
char printer_cmd[1025];
int bufpos = 0;
int isUSB = 0;

void
do_help(int code)
{
  printer_t *printer = &printer_list[0];
  printf("%s", help_msg);
  printf("Available models are:\n");
  while (printer->short_name)
    {
      printf("%10s      %s\n", printer->short_name, printer->long_name);
      printer++;
    }
  exit(code);
}

int
main(int argc, char **argv)
{
  int quiet = 0;
  int operation = 0;
  int c;
  while (1)
    {
#ifdef __GNU_LIBRARY__
      int option_index = 0;
      c = getopt_long(argc, argv, "P:r:icnaduqm:", optlist, &option_index);
#else
      c = getopt(argc, argv, "P:r:icnaduqm:");
#endif
      if (c == -1)
	break;
      switch (c)
	{
	case 'q':
	  quiet = 1;
	  break;
	case 'c':
	case 'i':
	case 'n':
	case 'a':
	case 'd':
	  if (operation)
	    do_help(1);
	  operation = c;
	  break;
	case 'P':
	  if (printer || raw_device)
	    {
	      printf("You may only specify one printer or raw device.\n");
	      do_help(1);
	    }
	  printer = malloc(strlen(optarg) + 1);
	  strcpy(printer, optarg);
	  break;
	case 'r':
	  if (printer || raw_device)
	    {
	      printf("You may only specify one printer or raw device.\n");
	      do_help(1);
	    }
	  raw_device = malloc(strlen(optarg) + 1);
	  strcpy(raw_device, optarg);
	  break;
	case 'm':
	  if (printer_model)
	    {
	      printf("You may only specify one printer model.\n");
	      do_help(1);
	    }
	  printer_model = malloc(strlen(optarg) + 1);
	  strcpy(printer_model, optarg);
	  break;
	case 'u':
	  isUSB = 1;
	  break;
	case 'h':
	  do_help(0);
	  break;
	default:
	  printf("%s\n", banner);
	  fprintf(stderr, "Unknown option %c\n", c);
	  do_help(1);
	}
    }
  if (!quiet)
    printf("%s\n", banner);
  if (operation == 0)
    do_help(1);
  initialize_print_cmd();
  switch(operation)
    {
    case 'c':
      do_head_clean();
      break;
    case 'n':
      do_nozzle_check();
      break;
    case 'i':
      do_ink_level();
      break;
    case 'a':
      do_align();
      break;
    case 'd':
      do_identify();
      break;
    default:
      do_help(1);
    }
  exit(0);
}

int
do_print_cmd(void)
{
  FILE *pfile;
  int bytes = 0;
  int retries = 0;
  char command[1024];
  memcpy(printer_cmd + bufpos, "\f\033\000\033\000", 5);
  bufpos += 5;
  if (raw_device)
    {
      pfile = fopen(raw_device, "wb");
      if (!pfile)
	{
	  fprintf(stderr, "Cannot open device %s: %s\n", raw_device,
		  strerror(errno));
	  return 1;
	}
    }
  else
    {
      if (!access("/bin/lpr", X_OK) ||
          !access("/usr/bin/lpr", X_OK) ||
          !access("/usr/bsd/lpr", X_OK))
        {
        if (printer == NULL)
          strcpy(command, "lpr -l");
	else
          sprintf(command, "lpr -P%s -l", printer);
        }
      else if (printer == NULL)
	strcpy(command, "lp -s -oraw");
      else
	sprintf(command, "lp -s -oraw -d%s", printer);

      if ((pfile = popen(command, "w")) == NULL)
	{
	  fprintf(stderr, "Cannot print to printer %s with %s\n", printer,
		  command);
	  return 1;
	}
    }
  while (bytes < bufpos)
    {
      int status = fwrite(printer_cmd + bytes, 1, bufpos - bytes, pfile);
      if (status == 0)
	{
	  retries++;
	  if (retries > 2)
	    {
	      fprintf(stderr, "Unable to send command to printer\n");
	      if (raw_device)
		fclose(pfile);
	      else
		pclose(pfile);
	      return 1;
	    }
	}
      else if (status == -1)
	{
	  fprintf(stderr, "Unable to send command to printer\n");
	  if (raw_device)
	    fclose(pfile);
	  else
	    pclose(pfile);
	  return 1;
	}
      else
	{
	  bytes += status;
	  retries = 0;
	}
    }
  if (raw_device)
    fclose(pfile);
  else
    pclose(pfile);
  return 0;
}

void
initialize_print_cmd(void)
{
  bufpos = 0;
  if (isUSB)
    {
      static char hdr[] = "\000\000\000\033\001@EJL 1284.4\n@EJL     \n\033@";
      memcpy(printer_cmd, hdr, sizeof(hdr) - 1); /* Do NOT include the null! */
      bufpos = sizeof(hdr) - 1;
    }
}

void
do_remote_cmd(char *cmd, int nargs, int a0, int a1, int a2, int a3)
{
  static char remote_hdr[] = "\033@\033(R\010\000\000REMOTE1";
  static char remote_trailer[] = "\033\000\000\000\033\000";
  memcpy(printer_cmd + bufpos, remote_hdr, sizeof(remote_hdr) - 1);
  bufpos += sizeof(remote_hdr) - 1;
  memcpy(printer_cmd + bufpos, cmd, 2);
  bufpos += 2;
  printer_cmd[bufpos] = nargs % 256;
  printer_cmd[bufpos + 1] = (nargs >> 8) % 256;
  if (nargs > 0)
    printer_cmd[bufpos + 2] = a0;
  if (nargs > 1)
    printer_cmd[bufpos + 3] = a1;
  if (nargs > 2)
    printer_cmd[bufpos + 4] = a2;
  if (nargs > 3)
    printer_cmd[bufpos + 5] = a3;
  bufpos += 2 + nargs;
  memcpy(printer_cmd + bufpos, remote_trailer, sizeof(remote_trailer) - 1);
  bufpos += sizeof(remote_trailer) - 1;
}

void
add_newlines(int count)
{
  int i;
  for (i = 0; i < count; i++)
    {
      printer_cmd[bufpos++] = '\r';
      printer_cmd[bufpos++] = '\n';
    }
}

void
add_resets(int count)
{
  int i;
  for (i = 0; i < count; i++)
    {
      printer_cmd[bufpos++] = '\033';
      printer_cmd[bufpos++] = '\000';
    }
}

char *colors[] = {
  "Black", "Cyan", "Magenta", "Yellow", "Light Cyan", "Light Magenta", 0
};

void
do_ink_level(void)
{
  int fd;
  int status;
  char buf[1024];
  char *ind;
  int i;
  if (!raw_device)
    {
      fprintf(stderr, "Obtaining ink levels requires using a raw device.\n");
      exit(1);
    }
  fd = open(raw_device, O_RDWR, 0666);
  if (fd == -1)
    {
      fprintf(stderr, "Cannot open %s read/write: %s\n", raw_device,
	      strerror(errno));
      exit(1);
    }
  do_remote_cmd("IQ", 1, 1, 0, 0, 0);
  add_resets(2);
  if (write(fd, printer_cmd, bufpos) < bufpos)
    {
      fprintf(stderr, "Cannot write to %s: %s\n", raw_device, strerror(errno));
      exit(1);
    }
  sleep(1);
  memset(buf, 0, 1024);
  status = read(fd, buf, 1023);
  if (status < 0)
    {
      fprintf(stderr, "Cannot read from %s: %s\n", raw_device,strerror(errno));
      exit(1);
    }
  ind = strchr(buf, 'I');
  if (!ind || ind[1] != 'Q' || ind[2] != ':')
    {
      fprintf(stderr, "Cannot parse output from printer\n");
      exit(1);
    }
  ind += 3;
  printf("%20s    %s\n", "Ink color", "Percent remaining");
  for (i = 0; i < 6; i++)
    {
      int val, j;
      if (!ind[0] || ind[0] == ';')
	exit(0);
      for (j = 0; j < 2; j++)
	{
	  if (ind[j] >= '0' && ind[j] <= '9')
	    ind[j] -= '0';
	  else if (ind[j] >= 'A' && ind[j] <= 'F')
	    ind[j] = ind[j] - 'A' + 10;
	  else if (ind[j] >= 'a' && ind[j] <= 'f')
	    ind[j] = ind[j] - 'a' + 10;
	  else
	    exit(1);
	}
      val = (ind[0] << 4) + ind[1];
      printf("%20s    %3d\n", colors[i], val);
      ind += 2;
    }
  (void) close(fd);
  exit(0);
}

void
do_identify(void)
{
  int fd;
  int status;
  char buf[1024];
  if (!raw_device)
    {
      fprintf(stderr, "Printer identification requires using a raw device.\n");
      exit(1);
    }
  fd = open(raw_device, O_RDWR, 0666);
  if (fd == -1)
    {
      fprintf(stderr, "Cannot open %s read/write: %s\n", raw_device,
	      strerror(errno));
      exit(1);
    }
  bufpos = 0;
  sprintf(printer_cmd, "\033\001@EJL ID\r\n");
  if (write(fd, printer_cmd, strlen(printer_cmd)) < strlen(printer_cmd))
    {
      fprintf(stderr, "Cannot write to %s: %s\n", raw_device, strerror(errno));
      exit(1);
    }
  sleep(1);
  memset(buf, 0, 1024);
  status = read(fd, buf, 1023);
  if (status < 0)
    {
      fprintf(stderr, "Cannot read from %s: %s\n", raw_device,strerror(errno));
      exit(1);
    }
  printf("%s\n", buf);
  (void) close(fd);
  exit(0);
}


void
do_head_clean(void)
{
  do_remote_cmd("CH", 2, 0, 0, 0, 0);
  printf("Cleaning heads...\n");
  exit(do_print_cmd());
}

void
do_nozzle_check(void)
{
  do_remote_cmd("VI", 2, 0, 0, 0, 0);
  do_remote_cmd("NC", 2, 0, 0, 0, 0);
  printf("Running nozzle check, please ensure paper is in the printer.\n");
  exit(do_print_cmd());
}

char new_align_help[] = "\
Please read these instructions very carefully before proceeding.\n\
\n\
This utility lets you align the print head of your Epson Stylus inkjet\n\
printer.  Misuse of this utility may cause your print quality to degrade\n\
and possibly damage your printer.  This utility has not been reviewed by\n\
Seiko Epson for correctness, and is offered with no warranty at all.  The\n\
entire risk of using this utility lies with you.\n\
\n\
This utility prints %d test patterns.  Each pattern looks very similar.\n\
The patterns consist of a series of pairs of vertical lines that overlap.\n\
Below each pair of lines is a number between %d and %d.\n\
\n\
When you inspect the pairs of lines, you should find the pair of lines that\n\
is best in alignment, that is, that best forms a single vertical line.\n\
Inspect the pairs very carefully to find the best match.  Using a loupe\n\
or magnifying glass is recommended for the most critical inspection.\n\
It is also suggested that you use a good quality paper for the test,\n\
so that the lines are well-formed and do not spread through the paper.\n\
After picking the number matching the best pair, place the paper back in\n\
the paper input tray before typing it in.\n\
\n\
Each pattern is similar, but later patterns use finer dots for more\n\
critical alignment.  You must run all of the passes to correctly align your\n\
printer.  After running all the alignment passes, the alignment\n\
patterns will be printed once more.  You should find that the middle-most\n\
pair (#%d out of the %d) is the best for all patterns.\n\
\n\
After the passes are printed once more, you will be offered the\n\
choices of (s)aving the result in the printer, (r)epeating the process,\n\
or (q)uitting without saving.  Quitting will not restore the previous\n\
settings, but powering the printer off and back on will.  If you quit,\n\
you must repeat the entire process if you wish to later save the results.\n\
It is essential that you not turn your printer off during this procedure.\n\n";

char old_align_help[] = "\
Please read these instructions very carefully before proceeding.\n\
\n\
This utility lets you align the print head of your Epson Stylus inkjet\n\
printer.  Misuse of this utility may cause your print quality to degrade\n\
and possibly damage your printer.  This utility has not been reviewed by\n\
Seiko Epson for correctness, and is offered with no warranty at all.  The\n\
entire risk of using this utility lies with you.\n\
\n\
This utility prints a test pattern that consist of a series of pairs of\n\
vertical lines that overlap.  Below each pair of lines is a number between\n\
%d and %d.\n\
\n\
When you inspect the pairs of lines, you should find the pair of lines that\n\
is best in alignment, that is, that best forms a single vertical align.\n\
Inspect the pairs very carefully to find the best match.  Using a loupe\n\
or magnifying glass is recommended for the most critical inspection.\n\
It is also suggested that you use a good quality paper for the test,\n\
so that the lines are well-formed and do not spread through the paper.\n\
After picking the number matching the best pair, place the paper back in\n\
the paper input tray before typing it in.\n\
\n\
After running the alignment pattern, it will be printed once more.  You\n\
should find that the middle-most pair (#%d out of the %d) is the best.\n\
You will then be offered the choices of (s)aving the result in the printer,\n\
(r)epeating the process, or (q)uitting without saving.  Quitting will not\n\
restore the previous settings, but powering the printer off and back on will.\n\
If you quit, you must repeat the entire process if you wish to later save\n\
the results.  It is essential that you not turn off your printer during\n\
this procedure.\n\n";

void
do_align_help(int passes, int choices)
{
  if (passes > 1)
    printf(new_align_help, passes, 1, choices, (choices + 1) / 2, choices);
  else
    printf(old_align_help, 1, choices, (choices + 1) / 2, choices);
  fflush(stdout);
}

void
align_error(void)
{
  printf("Unable to send command to the printer, exiting.\n");
  exit(1);
}

/*
 * This is the thorny one.
 */
void
do_align(void)
{
  char inbuf[64];
  long answer;
  char *endptr;
  int passes = 0;
  int choices = 0;
  int curpass;
  int notfound = 1;
  printer_t *printer = &printer_list[0];
  char *printer_name = NULL;
  if (!printer_model)
    {
      char buf[1024];
      int fd;
      int status;
      char *pos = NULL;
      char *spos = NULL;
      if (!raw_device)
	{
	  printf("Printer alignment must be done with a raw device or else\n");
	  printf("the -m option must be used to specify a printer.\n");
	  do_help(1);
	}
      printf("Attempting to detect printer model...");
      fflush(stdout);
      fd = open(raw_device, O_RDWR, 0666);
      if (fd == -1)
	{
	  fprintf(stderr, "\nCannot open %s read/write: %s\n", raw_device,
		  strerror(errno));
	  exit(1);
	}
      bufpos = 0;
      sprintf(printer_cmd, "\033\001@EJL ID\r\n");
      if (write(fd, printer_cmd, strlen(printer_cmd)) < strlen(printer_cmd))
	{
	  fprintf(stderr, "\nCannot write to %s: %s\n", raw_device,
		  strerror(errno));
	  exit(1);
	}
      sleep(1);
      memset(buf, 0, 1024);
      status = read(fd, buf, 1023);
      if (status < 0)
	{
	  fprintf(stderr, "\nCannot read from %s: %s\n", raw_device,
		  strerror(errno));
	  exit(1);
	}
      (void) close(fd);
      pos = strchr(buf, (int) ';');
      if (pos)
	pos = strchr(pos + 1, (int) ';');
      if (pos)
	pos = strchr(pos, (int) ':');
      if (pos)
	spos = strchr(pos, (int) ';');
      if (!pos)
	{
	  fprintf(stderr, "\nCannot detect printer type.  Please use -m to specify your printer model.\n");
	  do_help(1);
	}
      if (spos)
	*spos = '\000';
      printer_model = pos + 1;
      printf("%s\n\n", printer_model);
    }
  while (printer->short_name && notfound)
    {
      if (!strcmp(printer_model, printer->short_name) ||
	  !strcmp(printer_model, printer->long_name))
	{
	  passes = printer->passes;
	  choices = printer->choices;
	  printer_name = printer->long_name;
	  notfound = 0;
	}
      else
	printer++;
    }
  if (notfound)
    {
      printf("Printer model %s is not known.\n", printer_model);
      do_help(1);
    }

 start:
  do_align_help(passes, choices);
  printf("This procedure assumes that your printer is an Epson %s.\n",
	 printer_name);
  printf("If this is not your printer model, please type control-C now and\n");
  printf("choose your actual printer model.\n");
  printf("\n");
  printf("Please place a sheet of paper in your printer to begin the head\n");
  printf("alignment procedure.\n");
  memset(inbuf, 0, 64);
  fflush(stdin);
  fgets(inbuf, 63, stdin);
  putc('\n', stdout);
  fflush(stdout);
  initialize_print_cmd();
  for (curpass = 1; curpass <= passes; curpass ++)
    {
    top:
      add_newlines(7 * (curpass - 1));
      do_remote_cmd("DT", 3, 0, curpass - 1, 0, 0);
      if (do_print_cmd())
	align_error();
    reread:
      printf("Please inspect the print, and choose the best pair of lines\n");
      if (curpass == passes)
	printf("in pattern #%d, and then insert a fresh page in the input tray.\n",
	       curpass);
      else
	printf("in pattern #%d, and then reinsert the page in the input tray.\n",
	       curpass);
      printf("Type a pair number, '?' for help, or 'r' to retry this pattern. ==> ");
      fflush(stdout);
      memset(inbuf, 0, 64);
      fflush(stdin);
      fgets(inbuf, 63, stdin);
      putc('\n', stdout);
      switch (inbuf[0])
	{
	case 'r':
	case 'R':
	  printf("Please insert a fresh sheet of paper, and then type the enter key.\n");
	  initialize_print_cmd();
	  fflush(stdin);
	  fgets(inbuf, 15, stdin);
	  putc('\n', stdout);
	  fflush(stdout);
	  goto top;
	case 'h':
	case '?':
	  do_align_help(passes, choices);
	  fflush(stdout);
	case '\n':
	case '\000':
	  goto reread;
	default:
	  break;
	}
      answer = strtol(inbuf, &endptr, 10);
      if (endptr == inbuf)
	{
	  printf("I cannot understand what you typed!\n");
	  fflush(stdout);
	  goto reread;
	}
      if (answer < 1 || answer > choices)
	{
	  printf("The best pair of lines should be numbered between 1 and %d.\n",
		 choices);
	  fflush(stdout);
	  goto reread;
	}
      if (curpass == passes)
	{
	  printf("Aligning phase %d, and performing final test.\n", curpass);
	  printf("Please insert a fresh sheet of paper.\n");
	}
      else
	printf("Aligning phase %d, and starting phase %d.\n", curpass,
	       curpass + 1);
      fflush(stdout);
      initialize_print_cmd();
      do_remote_cmd("DA", 4, 0, curpass - 1, 0, answer);
    }
  for (curpass = 0; curpass < passes; curpass++)
    do_remote_cmd("DT", 3, 0, curpass, 0, 0);
  if (do_print_cmd())
    align_error();
 read_final:
  printf("Please inspect the final output very carefully to ensure that your\n");
  printf("printer is in proper alignment.  You may now (s)ave the results in\n");
  printf("the printer, (q)uit without saving the results, or (r)epeat the entire\n");
  printf("process from the beginning.  You will then be asked to confirm your choice\n");
  printf("What do you want to do (s, q, r)? ");
  fflush(stdout);
  memset(inbuf, 0, 64);
  fflush(stdin);
  fgets(inbuf, 15, stdin);
  putc('\n', stdout);
  switch (inbuf[0])
    {
    case 'q':
    case 'Q':
      printf("Please confirm by typing 'q' again that you wish to quit without saving: ");
      fflush(stdout);
      memset(inbuf, 0, 64);
      fflush(stdin);
      putc('\n', stdout);
      fgets(inbuf, 15, stdin);
      if (inbuf[0] == 'q' || inbuf[0] == 'Q')
	{
	  printf("OK, your printer is aligned, but the alignment has not been saved.\n");
	  printf("If you wish to save the alignment, you must repeat this process.\n");
	  exit(0);
	}
      break;
    case 'r':
    case 'R':
      printf("Please confirm by typing 'r' again that you wish to repeat the\n");
      printf("alignment process: ");
      fflush(stdout);
      memset(inbuf, 0, 64);
      fflush(stdin);
      putc('\n', stdout);
      fgets(inbuf, 15, stdin);
      if (inbuf[0] == 'r' || inbuf[0] == 'R')
	{
	  printf("Repeating the alignment process.\n");
	  goto start;
	}
      break;
    case 's':
    case 'S':
      printf("Please confirm by typing 's' again that you wish to save the settings\n");
      printf("to your printer.  This will permanently alter the configuration of\n");
      printf("your printer.  WARNING: this procedure has not been approved by\n");
      printf("Seiko Epson, and it may damage your printer.  Proceed? ");
	     
      fflush(stdout);
      memset(inbuf, 0, 64);
      fflush(stdin);
      putc('\n', stdout);
      fgets(inbuf, 15, stdin);
      if (inbuf[0] == 's' || inbuf[0] == 'S')
	{
	  printf("Please insert your alignment test page in the printer once more\n");
	  printf("for the final save of your alignment settings.  When the printer\n");
	  printf("feeds the page through, your settings have been saved.\n");
	  fflush(stdout);
	  initialize_print_cmd();
	  add_newlines(2);
	  do_remote_cmd("SV", 0, 0, 0, 0, 0);
	  add_newlines(2);
	  if (do_print_cmd())
	    align_error();
	  exit(0);
	}
      break;
    default:
      printf("Unrecognized command.\n");
      goto read_final;
    }
  printf("Final command was not confirmed.\n");
  goto read_final;
}
