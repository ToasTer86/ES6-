#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>


/* V1.0
 * CMOS clock manipulation - Charles Hedrick, hedrick@cs.rutgers.edu, Apr 1992
 * 
 * clock [-u] -r  - read cmos clock
 * clock [-u] -w  - write cmos clock from system time
 * clock [-u] -s  - set system time from cmos clock
 * clock [-u] -a  - set system time from cmos clock, adjust the time to
 *                  correct for systematic error, and put it back to the cmos.
 *  -u indicates cmos clock is kept in universal time
 *
 * The program is designed to run setuid, since we need to be able to
 * write the CMOS port.
 *
 * I don't know what the CMOS clock will do in 2000, so this program
 * probably won't work past the century boundary.
 *
 *********************
 * V1.1
 * Modified for clock adjustments - Rob Hooft, hooft@chem.ruu.nl, Nov 1992
 * Also moved error messages to stderr. The program now uses getopt.
 * Changed some exit codes. Made 'gcc 2.3 -Wall' happy.
 *
 * I think a small explanation of the adjustment routine should be given
 * here. The problem with my machine is that its CMOS clock is 10 seconds 
 * per day slow. With this version of clock.c, and my '/etc/rc.local' 
 * reading '/etc/clock -au' instead of '/etc/clock -u -s', this error 
 * is automatically corrected at every boot. 
 *
 * To do this job, the program reads and writes the file '/etc/adjtime' 
 * to determine the correction, and to save its data. In this file are 
 * three numbers: 
 *
 * 1) the correction in seconds per day (So if your clock runs 5 
 *    seconds per day fast, the first number should read -5.0)
 * 2) the number of seconds since 1/1/1970 the last time the program was
 *    used.
 * 3) the remaining part of a second which was leftover after the last 
 *    adjustment
 *
 * Installation and use of this program:
 *
 * a) create a file '/etc/adjtime' containing as the first and only line:
 *    '0.0 0 0.0'
 * b) run 'clock -au' or 'clock -a', depending on whether your cmos is in
 *    universal or local time. This updates the second number.
 * c) set your system time using the 'date' command.
 * d) update your cmos time using 'clock -wu' or 'clock -w'
 * e) replace the first number in /etc/adjtime by your correction.
 * f) put the command 'clock -au' or 'clock -a' in your '/etc/rc.local'
 *
 * If the adjustment doesn't work for you, try contacting me by E-mail.
 *
 ******
 * V1.2
 *
 * Applied patches by Harald Koenig (koenig@nova.tat.physik.uni-tuebingen.de)
 * Patched and indented by Rob Hooft (hooft@EMBL-Heidelberg.DE)
 * 
 * A free quote from a MAIL-message (with spelling corrections):
 *
 * "I found the explanation and solution for the CMOS reading 0xff problem
 *  in the 0.99pl13c (ALPHA) kernel: the RTC goes offline for a small amount
 *  of time for updating. Solution is included in the kernel source 
 *  (linux/kernel/time.c)."
 *
 * "I modified clock.c to fix this problem and added an option (now default,
 *  look for USE_INLINE_ASM_IO) that I/O instructions are used as inline
 *  code and not via /dev/port (still possible via #undef ...)."
 *
 * With the new code, which is partially taken from the kernel sources, 
 * the CMOS clock handling looks much more "official".
 * Thanks Harald (and Torsten for the kernel code)!
 *
 ******
 * V1.3
 * Canges from alan@spri.levels.unisa.edu.au (Alan Modra):
 * a) Fix a few typos in comments and remove reference to making
 *    clock -u a cron job.  The kernel adjusts cmos time every 11
 *    minutes - see kernel/sched.c and kernel/time.c set_rtc_mmss().
 *    This means we should really have a cron job updating
 *    /etc/adjtime every 11 mins (set last_time to the current time
 *    and not_adjusted to ???).
 * b) Swapped arguments of outb() to agree with asm/io.h macro of the
 *    same name.  Use outb() from asm/io.h as it's slightly better.
 * c) Changed CMOS_READ and CMOS_WRITE to inline functions.  Inserted
 *    cli()..sti() pairs in appropriate places to prevent possible
 *    errors, and changed ioperm() call to iopl() to allow cli.
 * d) Moved some variables around to localise them a bit.
 * e) Fixed bug with clock -ua or clock -us that cleared environment
 *    variable TZ.  This fix also cured the annoying display of bogus
 *    day of week on a number of machines. (Use mktime(), ctime()
 *    rather than asctime() )
 * f) Use settimeofday() rather than stime().  This one is important
 *    as it sets the kernel's timezone offset, which is returned by
 *    gettimeofday(), and used for display of MSDOS and OS2 file
 *    times.
 * g) faith@cs.unc.edu added -D flag for debugging
 *
 * V1.4: alan@SPRI.Levels.UniSA.Edu.Au (Alan Modra)
 *       Wed Feb  8 12:29:08 1995, fix for years > 2000.
 *       faith@cs.unc.edu added -v option to print version.
 *
 * August 1996 Tom Dyas (tdyas@eden.rutgers.edu)
 *       Converted to be compatible with the SPARC /dev/rtc driver.
 *
 */

#define VERSION "1.4"

/* Here the information for time adjustments is kept. */
#define ADJPATH "/etc/adjtime"


/* used for debugging the code. */
/*#define KEEP_OFF */

/* Globals */
int readit = 0;
int adjustit = 0;
int writeit = 0;
int setit = 0;
int universal = 0;
int debug = 0;

volatile void 
usage ()
{
  fprintf (stderr, 
    "clock [-u] -r|w|s|a|v\n"
    "  r: read and print CMOS clock\n"
    "  w: write CMOS clock from system time\n"
    "  s: set system time from CMOS clock\n"
    "  a: get system time and adjust CMOS clock\n"
    "  u: CMOS clock is in universal time\n"
    "  v: print version (" VERSION ") and exit\n"
  );
  exit (1);
}

int rtc_fd;

void
rtc_init ()
{
  rtc_fd = open ("/dev/rtc", 2);
  if (rtc_fd < 0)
    {
      perror ("unable to open /dev/rtc read/write : ");
      exit (1);
    }
}

int 
main (int argc, char **argv, char **envp)
{
  struct tm tm;
  time_t systime;
  time_t last_time;
  char arg;
  double factor;
  double not_adjusted;
  int adjustment = 0;
  unsigned char save_control, save_freq_select;
  struct rtc_time rtc_tm;

  while ((arg = getopt (argc, argv, "rwsuaDv")) != -1)
    {
      switch (arg)
	{
	case 'r':
	  readit = 1;
	  break;
	case 'w':
	  writeit = 1;
	  break;
	case 's':
	  setit = 1;
	  break;
	case 'u':
	  universal = 1;
	  break;
	case 'a':
	  adjustit = 1;
	  break;
        case 'D':
	  debug = 1;
	  break;
	case 'v':
	  fprintf( stderr, "clock " VERSION "\n" );
	  exit(0);
	default:
	  usage ();
	}
    }

  if (readit + writeit + setit + adjustit > 1)
    usage ();			/* only allow one of these */

  if (!(readit | writeit | setit | adjustit))	/* default to read */
    readit = 1;

  rtc_init ();

  if (adjustit)
    {				/* Read adjustment parameters first */
      FILE *adj;
      if ((adj = fopen (ADJPATH, "r")) == NULL)
	{
	  perror (ADJPATH);
	  exit (2);
	}
      if (fscanf (adj, "%lf %d %lf", &factor, &last_time, &not_adjusted) < 0)
	{
	  perror (ADJPATH);
	  exit (2);
	}
      fclose (adj);
      if (debug) printf ("Last adjustment done at %d seconds after 1/1/1970\n", last_time);
    }

  if (readit || setit || adjustit)
    {
      int i;

/* The purpose of the "do" loop is called "low-risk programming" */
/* In theory it should never run more than once */
      if (ioctl(rtc_fd, RTCGET, &rtc_tm) < 0)
	{
	  perror("ioctl(RTCGET)");
	  exit (2);
	}
      
      tm.tm_sec = rtc_tm.sec;
      tm.tm_min = rtc_tm.min;
      tm.tm_hour = rtc_tm.hour;
      tm.tm_wday = rtc_tm.dow - 1;
      tm.tm_mday = rtc_tm.dom ;
      tm.tm_mon = rtc_tm.month - 1;
      tm.tm_year = rtc_tm.year - 1900;
      tm.tm_isdst = -1;		/* don't know whether it's daylight */
    }

  if (readit || setit || adjustit)
    {
/*
 * mktime() assumes we're giving it local time.  If the CMOS clock
 * is in GMT, we have to set up TZ so mktime knows it.  tzset() gets
 * called implicitly by the time code, but only the first time.  When
 * changing the environment variable, better call tzset() explicitly.
 */
      if (universal)
	{
	  char *zone;
	  zone = (char *) getenv ("TZ");	/* save original time zone */
	  (void) putenv ("TZ=");
	  tzset ();
	  systime = mktime (&tm);
	  /* now put back the original zone */
	  if (zone)
	    {

             char *zonebuf;
             zonebuf = malloc (strlen (zone) + 4);
             strcpy (zonebuf, "TZ=");
             strcpy (zonebuf+3, zone);
             putenv (zonebuf);
             free (zonebuf);
	    }
	  else
	    {			/* wasn't one, so clear it */
	      putenv ("TZ");
	    }
	  tzset ();
	}
      else
	{
	  systime = mktime (&tm);
	}
      if (debug) printf ("Number of seconds since 1/1/1970 is %d\n", systime);
    }

  if (readit)
    {
      printf ("%s", ctime (&systime ));
    }

  if (setit || adjustit)
    {
      struct timeval tv;
      struct timezone tz;

/* program is designed to run setuid, be secure! */

      if (getuid () != 0)
	{			
	  fprintf (stderr, "Sorry, must be root to set or adjust time\n");
	  exit (2);
	}

      if (adjustit)
	{			/* the actual adjustment */
	  double exact_adjustment;

	  exact_adjustment = ((double) (systime - last_time))
	    * factor / (24 * 60 * 60)
	    + not_adjusted;
	  if (exact_adjustment > 0)
	    adjustment = (int) (exact_adjustment + 0.5);
	  else
	    adjustment = (int) (exact_adjustment - 0.5);
	  not_adjusted = exact_adjustment - (double) adjustment;
	  systime += adjustment;
	  if (debug) {
	     printf ("Time since last adjustment is %d seconds\n",
		     (int) (systime - last_time));
	     printf ("Adjusting time by %d seconds\n",
		     adjustment);
	     printf ("remaining adjustment is %.3f seconds\n",
		     not_adjusted);
	  }
	}
#ifndef KEEP_OFF
      tv.tv_sec = systime;
      tv.tv_usec = 0;
      tz.tz_minuteswest = timezone / 60;
      tz.tz_dsttime = daylight;

      if (settimeofday (&tv, &tz) != 0)
        {
	  fprintf (stderr,
		   "Unable to set time -- probably you are not root\n");
	  exit (1);
	}
      
      if (debug) {
	 printf( "Called settimeofday:\n" );
	 printf( "\ttv.tv_sec = %ld, tv.tv_usec = %ld\n",
		 tv.tv_sec, tv.tv_usec );
	 printf( "\ttz.tz_minuteswest = %d, tz.tz_dsttime = %d\n",
		 tz.tz_minuteswest, tz.tz_dsttime );
      }
#endif
    }
  
  if (writeit || (adjustit && adjustment != 0))
    {
      struct tm *tmp;
      systime = time (NULL);
      if (universal)
	tmp = gmtime (&systime);
      else
	tmp = localtime (&systime);

      rtc_tm.sec = tmp->tm_sec;
      rtc_tm.min = tmp->tm_min;
      rtc_tm.hour = tmp->tm_hour;
      rtc_tm.dow = tmp->tm_wday + 1;
      rtc_tm.dom = tmp->tm_mday;
      rtc_tm.month = tmp->tm_mon + 1;
      rtc_tm.year = tmp->tm_year + 1900;

      if (ioctl(rtc_fd, RTCSET, &rtc_tm) < 0)
	{
	  perror("ioctl(RTCGET)");
	  exit (2);
	}
      
      if (debug) printf ("Set to : %d:%d:%d\n", tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
    }
  else
    if (debug) printf ("CMOS clock unchanged.\n");
  /* Save data for next 'adjustit' call */
  if (adjustit)
    {
      FILE *adj;
      if ((adj = fopen (ADJPATH, "w")) == NULL)
	{
	  perror (ADJPATH);
	  exit (2);
	}
      fprintf (adj, "%f %d %f\n", factor, systime, not_adjusted);
      fclose (adj);
    }
  exit (0);
}