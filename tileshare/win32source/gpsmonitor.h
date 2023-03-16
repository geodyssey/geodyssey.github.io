/* GPS monitor defines */

#ifndef GPSMONITOR_H
#define GPSMONITOR_H 1

#include <stdio.h>
#include <time.h>
#include "../tileset.h"

#define APP_NAME            "gpsmonitor"
#define EXE_NAME            "gpsmonitor.exe"
#define TRACE_NAME          "gpsmonitor.trace"

#define GPS_MONITOR_CLASS   "tsGpsMonitorClass"
#define GPS_MONITOR_SMO     "tsGpsMonitorSharedMemory"

#define IDC_LOCATION            995
#define IDC_TIMESTAMP           996
#define IDC_ROTOR               997
#define IDC_ELEVATION           998
#define IDC_LOGFILE             999

#ifndef FILENAME_MAX
#define FILENAME_MAX            256
#endif

#define LINE_LENGTH             509                   /* this is C standard */
#define MAX_LOC_RECPTS           16         /* set it to something sensible */

#define FLUSH_COUNT              16       /* frequency of output file flush */
#define COMM_READ_BUF           256         /* maximum NMEA sentence length */

#define LOCATIONS_BUF          1024             /* previous fixes (max 32K) */

#define UM_GPS_MONITOR_REQUEST      (WM_APP + 21)      /* request locations */
#define UM_GPS_MONITOR_ACQ          (WM_APP + 22)    /* monitor acknowleges */
#define UM_GPS_POSITION_MESSAGE     (WM_APP + 23) /* monitor gives location */

struct gpsMonitorControl {
   HINSTANCE hInstance;
   HWND hWndSelf;                                  /* monitor window handle */
   HWND hWndLocRecpts[MAX_LOC_RECPTS];       /* location message recipients */
   char logFileName[FILENAME_MAX + 4];                     /* log file name */
   FILE *logFp;                                               /* ...pointer */
   HANDLE  inCommHndl;                      /* ...GPS input COM port handle */
   OVERLAPPED os;                     /* com port line-at-a-time read event */
   HANDLE comThreadHndl;                  /* ...com port read thread handle */
   int nErrors;                            /* count of com port read errors */
   int nIgnored;                         /* count of ingored NMEA sentences */
   int nOK;                            /* count of processed NMEA sentences */
   int gpsLocNext;                            /* in the following, where... */
   struct tsGpsLocation *gpsLoc;              /* ...locations are stored... */
   HANDLE smm;                                     /* shared memory mapping */
   time_t ztt;                                                 /* zero time */
   };

int initGpsMonitor(HWND);
int PASCAL readGPScomPort(void *);
void userInfo(char *, ...);
void userError(char *, ...);
void debugTrace(char *, ...);
int  nmeaAngle(char *, char);
int  nmeaTime(char *);
char *comprName(char *);

#endif
