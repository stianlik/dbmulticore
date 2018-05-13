/****************************************************************

Name:      wtime

Purpose:   Returns the seconds since 00:00 GMT, January 1, 1970
           (see gettimeofday(2))

Arguments: None

Returns:   The wall clock time in seconds as a double is returned. 

Notes:     This function uses two structures defined in the UNIX
           system call, gettimeofday(2). The structure, "timeval" 
           is defined in the include file <sys/time.h> as:

                 struct timeval {
                     long tv_sec;
                     long tv_usec;
                 }

           where timeval.tv_sec is the seconds and timeval.tv_usec
           is the microseconds.  
 
History:   Written by Tim Mattson, Dec 1, 1988

****************************************************************/

#include <sys/time.h> 
#define  USEC_TO_SEC   1.0e-6    /* to convert microsecs to secs */

double wtime()

{
   double time_seconds;
   struct timezone tzp;       /* time zone data */
   struct timeval  time_data; /* seconds since 0 GMT */
   
   tzp.tz_minuteswest = 0;    /* time zone info.  I don't care */
   tzp.tz_dsttime     = 0;    /* this so I just zero them      */
   
   gettimeofday(&time_data,&tzp);
   
   time_seconds  = (double) time_data.tv_sec;
   time_seconds += (double) time_data.tv_usec * USEC_TO_SEC;

   return time_seconds;
}
