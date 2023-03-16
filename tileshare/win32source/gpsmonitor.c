/* gpsmonitor.c - Win32 applet that monitors GPS device on com port.
   Upon request, it will start posting position messages to any applicant.
   Configuration will be obtained from a co-resident gpsmonitor.init file.
   (Programming note: this code was tested and found operational when
   compiled with Microsoft's C compiler, but failed when compiled with GCC.
   To be explored further?)
  */

#include <stdlib.h>
#include <windows.h>
#include <process.h>
#include "gpsmonitor.h"

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM) ;

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   PSTR szCmdLine,
                   int iCmdShow) {
   HWND       hWnd;
   MSG        msg;
   WNDCLASSEX wndclass;
   FILE      *traceFp;
   time_t     timeNow;
   struct gpsMonitorControl *gmc;
/* ------------------------------------------------------------------------ */
/* MessageBox(NULL, "Hello world...", APP_NAME, MB_OK);*/

/* Win32 will always set hPrevInstance to NULL. But we insist, as we don't  */
/* want multiple instances to run, so we just activate the running instance */

   hWnd = FindWindow(GPS_MONITOR_CLASS, NULL);
   if (hWnd) {         /* We found another version of ourself. Activate it: */
      if (IsIconic(hWnd)) {
         ShowWindow(hWnd, SW_RESTORE);
         }
      SetForegroundWindow(hWnd);
      return(0);                          /* terminate this dialog instance */
      }

   traceFp = NULL;                               /* assume it'll be dormant */
/* Comment out the following line, and tracing will be dormant... */
/* traceFp = fopen(TRACE_NAME, "at");*/

   debugTrace((char *)traceFp);           /* ...but never comment out this! */

   if (traceFp) {
      timeNow = time(NULL);
      debugTrace("###");
      debugTrace("### %s as-of: %s", APP_NAME, __DATE__ " " __TIME__);
      debugTrace("### start at: %s", ctime(&timeNow));
      }

   wndclass.cbSize        = sizeof(wndclass);
   wndclass.style         = CS_HREDRAW | CS_VREDRAW;
   wndclass.lpfnWndProc   = WndProc;
   wndclass.cbClsExtra    = 0;
   wndclass.cbWndExtra    = DLGWINDOWEXTRA;
   wndclass.hInstance     = hInstance;
   wndclass.hIcon         = LoadIcon(hInstance, APP_NAME);
   wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
   wndclass.lpszMenuName  = NULL;
   wndclass.lpszClassName = GPS_MONITOR_CLASS;
   wndclass.hIconSm       = LoadIcon(hInstance, APP_NAME);

   RegisterClassEx(&wndclass);

   hWnd = CreateDialog(hInstance, APP_NAME, 0, NULL);
   if (hWnd == NULL) FatalAppExit(0, "Unexpected error in dialog create.");

   gmc = (struct gpsMonitorControl *)GetWindowLong(hWnd, GWL_USERDATA);
   gmc->hInstance = hInstance;

/* This program is first, always-present self-recipient of location
   messages. Others will follow, once they "register" for service. */
   gmc->hWndSelf = hWnd;

   nmeaTime(NULL);
   if (initGpsMonitor(hWnd)) ExitProcess(1);
   ShowWindow (hWnd, iCmdShow);

   while (GetMessage (&msg, NULL, 0, 0)) {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
      }

   debugTrace("### real messages");
   debugTrace("### end at:   %s\n ", ctime(&timeNow));
   if (traceFp) fclose(traceFp);
   return(msg.wParam);
   }
/* ======================================================================== */
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

LRESULT CALLBACK WndProc(HWND hWnd,
                         UINT iMsg,
                         WPARAM wParam,
                         LPARAM lParam) {
/* ------------------------------------------------------------------------ */
   switch (iMsg) {
      case WM_CREATE: {
         struct gpsMonitorControl *gmc;   /* Main monitor control structure */
         gmc = (struct gpsMonitorControl *)malloc(sizeof(struct gpsMonitorControl));
         if (gmc == NULL) FatalAppExit(0, "Unable to allocate any memory!?");
         memset(gmc, '\0', sizeof(struct gpsMonitorControl));
         gmc->smm = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE, 0, LOCATIONS_BUF * sizeof(struct tsGpsLocation), GPS_MONITOR_SMO);
         debugTrace("Mapping object handle: %d\n", (int)gmc->smm);
         if (gmc->smm == NULL) FatalAppExit(0, "Unable to create shared memory Win32 object.");
         gmc->gpsLoc = (struct tsGpsLocation *)MapViewOfFile(gmc->smm, FILE_MAP_WRITE, 0, 0, 0);
         if (gmc->gpsLoc == NULL) FatalAppExit(0, "Unable to access shared location memory.");
         SetWindowLong(hWnd, GWL_USERDATA, (DWORD)gmc);
         debugTrace("Monitor start %p\n", gmc);
         return(0);
         }
/* ------------------------------------------------------------------------ */
/*    Message from a client program, wparm 1: sign-in 0: sign-out.          */
      case UM_GPS_MONITOR_REQUEST: {
         int i, m;
         struct gpsMonitorControl *gmc = (struct gpsMonitorControl *)GetWindowLong(hWnd, GWL_USERDATA);
         debugTrace("Msg from applicant: %d, type: %d\n", (int)lParam, (int)wParam);
         if (wParam) {                                           /* sign in */
            for (i = 0; i < MAX_LOC_RECPTS; i++) {  /* double registration? */
               if (gmc->hWndLocRecpts[i] == (HWND)lParam) return(0);
               }
            for (i = 0; i < MAX_LOC_RECPTS; i++) {
               if (gmc->hWndLocRecpts[i] != NULL) continue;  /* empty slot? */
               m = SendMessage((HWND)lParam, UM_GPS_MONITOR_ACQ, (WPARAM)LOCATIONS_BUF, (LPARAM)hWnd);
               if (m) break;                            /* is invoker sick? */
               gmc->hWndLocRecpts[i] = (HWND)lParam;
               break;
               }
            if (i == MAX_LOC_RECPTS) {
               userError("maximum number of client processes reached?\n", MAX_LOC_RECPTS);
               break;
               }
            }
         else {                                                 /* sign out */
            for (i = 0; i < MAX_LOC_RECPTS; i++) {
               if (gmc->hWndLocRecpts[i] != (HWND)lParam) continue;
               gmc->hWndLocRecpts[i] = NULL;
               break;
               }
            }
         return(0);
         }
/* ------------------------------------------------------------------------ */
      case WM_CLOSE: {
         int i;
         struct gpsMonitorControl *gmc = (struct gpsMonitorControl *)GetWindowLong(hWnd, GWL_USERDATA);
         for (i = 0; i < MAX_LOC_RECPTS; i++) {
            if (gmc->hWndLocRecpts[i] != NULL) break;
            }
         debugTrace("Close request pending, %d, %d\n", i, MAX_LOC_RECPTS);
         if (i < MAX_LOC_RECPTS) {
            if (MessageBox(hWnd, "Active client program(s)! Close?", APP_NAME, MB_YESNO | MB_DEFBUTTON2 | MB_SYSTEMMODAL | MB_ICONQUESTION) == IDYES) DestroyWindow(hWnd);
            }
         else DestroyWindow(hWnd);
         }
      return(0);
/* ------------------------------------------------------------------------ */
      case WM_DESTROY: {
         struct gpsMonitorControl *gmc = (struct gpsMonitorControl *)GetWindowLong(hWnd, GWL_USERDATA);
         debugTrace("Monitor end %p\n", gmc);
         debugTrace("GPSmessages: %d, Errors: %d, ignored sentences %d\n", gmc->nOK, gmc->nErrors, gmc->nIgnored);
         if (gmc->logFp) fclose(gmc->logFp);  /* if log file is open, close */
         if (gmc->inCommHndl) {            /* closing COM port reading task */
            TerminateThread(gmc->comThreadHndl, 0);      /* unconditionally */
            CloseHandle(gmc->inCommHndl);
            CloseHandle(gmc->os.hEvent);
            }
         if (gmc->gpsLoc) UnmapViewOfFile(gmc->gpsLoc);
         if (gmc->smm) CloseHandle(gmc->smm);
         free(gmc);
         PostQuitMessage(0);
         return(0);
         }
/* ------------------------------------------------------------------------ */
      }
   return(DefWindowProc(hWnd, iMsg, wParam, lParam));
   }
/* ======================================================================== */
int initGpsMonitor(HWND hWnd) {
   static struct tm zeroTimeTm = {0, 0, 0, 1, 0, 100, 0, 0, 0};
   struct gpsMonitorControl *gmc;
   char comStr[6];
   char *pa;
   int i, istat, comPort, baudRate, dataBits, parity, stopBits;
   DCB dcb;                                          /* com port parameters */
   DWORD dwError;
   COMMTIMEOUTS commTimeOuts;
   int threadID;
   char initFileName[FILENAME_MAX + 4];
   char inLine[LINE_LENGTH + 2];
   FILE *tmpFptr;
/* ------------------------------------------------------------------------ */
   gmc = (struct gpsMonitorControl *)GetWindowLong(hWnd, GWL_USERDATA);
   debugTrace("Initializing %p\n", gmc);

   gmc->ztt = mktime(&zeroTimeTm);
   debugTrace("zero time: %s", ctime(&gmc->ztt));

   for (i = 0; i < LOCATIONS_BUF; i++) {      /* initialize location buffer */
      gmc->gpsLoc[i].lat = TS_I4A_UNDEF;
      }
   gmc->gpsLocNext = 0;

/* load defaults - used if no init file given */
   comPort = 1;
   baudRate = CBR_4800;
   dataBits = 8;
   parity = NOPARITY;
   stopBits = 1;
   gmc->logFileName[0] = '\0';
   gmc->logFp = NULL;                                         /* ...pointer */

/* if .init file is coresident, use values in it instead of compile defaults */
   GetModuleFileName(gmc->hInstance, initFileName, FILENAME_MAX);
   pa = strrchr(initFileName, '.');
   if (pa == NULL) FatalAppExit(0, "Unexpected error in search for initialization file.");
   strcpy(pa, ".init");
   tmpFptr = fopen(initFileName, "rt");
   if (tmpFptr) {
      debugTrace("Initialization file %s\n", initFileName);
      while ((pa = fgets(inLine, LINE_LENGTH, tmpFptr))) {
         if (*inLine == ';') continue;
         pa = strchr(inLine, '=');
         if (pa == NULL) continue;
         while (isspace(*pa)) pa++;
         if (*pa == '\0') continue;
         if (memcmp(inLine, "com.port", 8) == 0)  comPort = atoi(pa+1);
         if (memcmp(inLine, "baud.rate", 9) == 0) baudRate = atoi(pa+1);
         if (memcmp(inLine, "data.bits", 9) == 0) dataBits = atoi(pa+1);
         if (memcmp(inLine, "parity", 6) == 0)    parity = atoi(pa+1);
         if (memcmp(inLine, "stop.bits", 9) == 0) stopBits = atoi(pa+1);
         if (memcmp(inLine, "log.file", 8) == 0) {
            strncpy(gmc->logFileName, pa + 1, FILENAME_MAX);
            pa = strrchr(gmc->logFileName, '\n');
            if (pa) *pa = '\0';
            }
         }
      fclose(tmpFptr);
      }

   sprintf(comStr, "COM%d", comPort);
   debugTrace("Opening com port %s\n", comStr);

   gmc->inCommHndl = CreateFile(comStr,
                              GENERIC_READ,
                              0,                        /* exclusive access */
                              NULL,                    /* no security attrs */
                              OPEN_EXISTING,
                              FILE_FLAG_OVERLAPPED,
                              NULL);

   if (gmc->inCommHndl == INVALID_HANDLE_VALUE) {   /* quit, signal failure */
      dwError = GetLastError();
      userError("ComPort (%s) open error:%u", comStr, dwError);
      return(1);
      }
   istat = GetCommState(gmc->inCommHndl, &dcb);                 /* likewise */
   if (istat == 0) {
      userError("can't get port dcb? %s", comStr);
      return(1);
      }

   dcb.BaudRate = baudRate;
   dcb.ByteSize = dataBits;
   dcb.Parity = parity;
   if (stopBits == 3) dcb.StopBits = ONE5STOPBITS;
   else if (stopBits == 2) dcb.StopBits = TWOSTOPBITS;
   else dcb.StopBits = ONESTOPBIT;
   dcb.EvtChar = '\n';

   istat = SetCommState(gmc->inCommHndl, &dcb);
   if (istat == 0) {
      userError("can't put port dcb? %s", comStr);
      return(1);
      }

   SetCommMask(gmc->inCommHndl, EV_ERR | EV_RXFLAG);
   SetupComm(gmc->inCommHndl, COMM_READ_BUF, 0);
   PurgeComm(gmc->inCommHndl, PURGE_RXABORT | PURGE_RXCLEAR);

   commTimeOuts.ReadIntervalTimeout = 0;                       /* MAXDWORD? */
   commTimeOuts.ReadTotalTimeoutMultiplier = 0;
   commTimeOuts.ReadTotalTimeoutConstant = 0;
   commTimeOuts.WriteTotalTimeoutMultiplier = 0;
   commTimeOuts.WriteTotalTimeoutConstant = 0;
   SetCommTimeouts(gmc->inCommHndl, &commTimeOuts);

   gmc->os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

   if (gmc->os.hEvent == NULL) {
      userError("Event create error.\n");
      return(1);
      }

   istat = SetCommMask(gmc->inCommHndl, EV_ERR | EV_RXFLAG);
   if (istat == 0) {
      userError("Mask set error\n", stderr);
      return(1);
      }
   gmc->nOK = gmc->nErrors = gmc->nIgnored = 0;

   gmc->comThreadHndl = CreateThread(NULL, 2048, (LPTHREAD_START_ROUTINE)readGPScomPort, gmc, 0, (LPDWORD)&threadID);
   if (gmc->comThreadHndl == 0) {
      userError("Com port read thread create error.\n");
      return(1);
      }
/* SetThreadPriority(gmc->comThreadHndl, THREAD_PRIORITY_HIGHEST);*/

   if (gmc->logFileName[0]) {        /* unable to write: only a soft error? */
      debugTrace("Opening log file %s\n", gmc->logFileName);
      pa = strchr(gmc->logFileName, ' ');
      if (pa == NULL) gmc->logFp = fopen(gmc->logFileName, "ab+");
      if (gmc->logFp == NULL) userError("Unable to open log file: [%s]\n", gmc->logFileName);
      else SendDlgItemMessage(gmc->hWndSelf, IDC_LOGFILE, WM_SETTEXT, 0, (LPARAM)comprName(gmc->logFileName));
      }
   else SendDlgItemMessage(gmc->hWndSelf, IDC_LOGFILE, WM_SETTEXT, 0, (LPARAM)"(No log file)");

   debugTrace("Initialization complete.\n");
   return(0);
   }
/* ======================================================================== */
#define STR_DEL   " ,\n"                                /* strtok delimiter */
#define NMEA_ITEM_LENGTH  64

int PASCAL readGPScomPort(void *pp) {
   static char rotor[] = "|/-\\";
   static int rotorIx;
   struct gpsMonitorControl *gmc;
   int i, l, m, istat;
   DWORD dwEvtMask;
   DWORD dwErrorFlags;
   COMSTAT comStat;
   char  nmeaSentence[COMM_READ_BUF];
   char  nmeaSec[NMEA_ITEM_LENGTH + 2];
   char  strAng[NMEA_ITEM_LENGTH + 2];
   char  strWaste[NMEA_ITEM_LENGTH + 2];
   char  strEllElev[NMEA_ITEM_LENGTH + 2];
   char  strGeoElev[NMEA_ITEM_LENGTH + 2];
   char  dsplTxt[64];
   char  sign;
   char *pa;
   struct tsGpsLocation tsg;
/* ------------------------------------------------------------------------ */
   gmc = (struct gpsMonitorControl *)pp;
   for (;;) {
      dwEvtMask = 0;
      WaitCommEvent(gmc->inCommHndl, &dwEvtMask, NULL);
      if ((dwEvtMask & EV_RXFLAG) == 0) continue;

      ClearCommError(gmc->inCommHndl, &dwErrorFlags, &comStat);
      m = min((DWORD)COMM_READ_BUF, comStat.cbInQue);
      istat = ReadFile(gmc->inCommHndl, nmeaSentence, m, (LPDWORD)&l, &gmc->os);
      if (istat == FALSE) {
         gmc->nErrors++;
         continue;
         }
      nmeaSentence[l] = '\0';
      pa = strchr(nmeaSentence, '$');    /* just in case there is some junk */
      if (pa == NULL) {                    /* following '\n' in nmea record */
/*       debugTrace("bad?:[%.48s]\n", nmeaSentence);*/
         gmc->nErrors++;
         continue;
         }
      if (memcmp(pa, "$GPGGA,", 7) == 0) {      /* new location & elevation */
/*       debugTrace("GPGGA:[%.72s]\n", nmeaSentence + 7);*/
         pa = strtok(nmeaSentence + 7, STR_DEL);
         strncpy(nmeaSec, pa, NMEA_ITEM_LENGTH);
         tsg.time = nmeaTime(nmeaSec);                   /* convert time */
         pa = strtok(NULL, STR_DEL);                    /* extract latitude */
         strncpy(strAng, pa, NMEA_ITEM_LENGTH);
         pa = strtok(NULL, STR_DEL);                       /* sign latitude */
         sign = *pa;
         tsg.lat = nmeaAngle(strAng, sign);
         pa = strtok(NULL, STR_DEL);                   /* extract longitude */
         strncpy(strAng, pa, NMEA_ITEM_LENGTH);
         pa = strtok(NULL, STR_DEL);                      /* sign longitude */
         sign = *pa;
         tsg.lng = nmeaAngle(strAng, sign);
         pa = strtok(NULL, STR_DEL);                                /* type */
         strncpy(strWaste, pa, NMEA_ITEM_LENGTH);
         pa = strtok(NULL, STR_DEL);                          /* count sats */
         strncpy(strWaste, pa, NMEA_ITEM_LENGTH);
         pa = strtok(NULL, STR_DEL);                                /* hdop */
         strncpy(strWaste, pa, NMEA_ITEM_LENGTH);
         pa = strtok(NULL, STR_DEL);                     /* ellipsoid elev. */
         strncpy(strEllElev, pa, NMEA_ITEM_LENGTH);
         pa = strtok(NULL, STR_DEL);                               /* units */
         strncpy(strWaste, pa, NMEA_ITEM_LENGTH);
         pa = strtok(NULL, STR_DEL);                         /* geoid elev. */
         strncpy(strGeoElev, pa, NMEA_ITEM_LENGTH);
         tsg.elevation = (int)(100.0 * (atof(strEllElev) + atof(strGeoElev)));
         gmc->nOK++;
         }
      else if (memcmp(pa, "$GPGLL,", 7) == 0) {             /* new location */
/*       debugTrace("GPGLL:[%.72s]\n", nmeaSentence + 7);*/
         pa = strtok(nmeaSentence + 7, STR_DEL);
         strncpy(strAng, pa, NMEA_ITEM_LENGTH);
         pa = strtok(NULL, STR_DEL);                      /* sign longitude */
         sign = *pa;
         tsg.lat = nmeaAngle(strAng, sign);
         pa = strtok(NULL, STR_DEL);                   /* extract longitude */
         strncpy(strAng, pa, NMEA_ITEM_LENGTH);
         pa = strtok(NULL, STR_DEL);                      /* sign longitude */
         sign = *pa;
         tsg.lng = nmeaAngle(strAng, sign);
         strncpy(nmeaSec, pa, NMEA_ITEM_LENGTH);
         tsg.time = nmeaTime(nmeaSec);                      /* convert time */
         tsg.elevation = 0;
         gmc->nOK++;
         }
      else {
         l = 0;
         debugTrace("? %.7s\n", nmeaSentence);
         gmc->nIgnored++;
         }
      if (l) {
         debugTrace("Fix: %3d %10.6f %10.6f %u %d\n", gmc->gpsLocNext, TS_RAD2DEG * TS_AngOfI4(tsg.lat), TS_RAD2DEG * TS_AngOfI4(tsg.lng), tsg.time, tsg.elevation);

         dsplTxt[0] = (char)(rotor[(rotorIx++)%4]);
         dsplTxt[1] = '\0';
         SendDlgItemMessage(gmc->hWndSelf, IDC_ROTOR, WM_SETTEXT, 0, (LPARAM)dsplTxt);

         gmc->gpsLoc[gmc->gpsLocNext].lat = tsg.lat;
         gmc->gpsLoc[gmc->gpsLocNext].lng = tsg.lng;
         for (i = 0; i < MAX_LOC_RECPTS; i++) {
            if (gmc->hWndLocRecpts[i] == NULL) continue;
            debugTrace("Post %d, client: %d at %3d\n", UM_GPS_POSITION_MESSAGE, i, gmc->gpsLocNext);
            PostMessage(gmc->hWndLocRecpts[i], UM_GPS_POSITION_MESSAGE, (WPARAM)(gmc->gpsLocNext), 0);
            }
         gmc->gpsLocNext = (gmc->gpsLocNext + 1)%LOCATIONS_BUF;
         gmc->gpsLoc[gmc->gpsLocNext].lat = TS_I4A_UNDEF;  /* to brake circulus viciosus */

         if ((tsg.lat == TS_I4A_UNDEF) || (tsg.lng == TS_I4A_UNDEF)) {
            SendDlgItemMessage(gmc->hWndSelf, IDC_LOCATION, WM_SETTEXT, 0, (LPARAM)"Undefined");
            SendDlgItemMessage(gmc->hWndSelf, IDC_ELEVATION, WM_SETTEXT, 0, (LPARAM)"Undefined");
            }
         else {
            sprintf(dsplTxt, " %c%d:%07.4f,  %c%d:%07.4f", TS_SIGN_LAT(tsg.lat), TS_INTDEG_LL(tsg.lat), TS_DECMIN_LL(tsg.lat), TS_SIGN_LNG(tsg.lng), TS_INTDEG_LL(tsg.lng), TS_DECMIN_LL(tsg.lng));
            SendDlgItemMessage(gmc->hWndSelf, IDC_LOCATION, WM_SETTEXT, 0, (LPARAM)dsplTxt);
            sprintf(dsplTxt, "%d", tsg.elevation);
            SendDlgItemMessage(gmc->hWndSelf, IDC_ELEVATION, WM_SETTEXT, 0, (LPARAM)dsplTxt);
            }
         sprintf(dsplTxt, "%u", tsg.time);
         SendDlgItemMessage(gmc->hWndSelf, IDC_TIMESTAMP, WM_SETTEXT, 0, (LPARAM)dsplTxt);

         if (gmc->logFp) {
            if (fwrite(&tsg, sizeof(struct tsGpsLocation), 1, gmc->logFp) < 1) {
/*             userError("Unable to write to log file: [%s]\nNo futher log records will be written!\n", gmc->logFileName);*/
               SendDlgItemMessage(gmc->hWndSelf, IDC_LOGFILE, WM_SETTEXT, 0, (LPARAM)"Log write error!");
               fclose(gmc->logFp);
               gmc->logFp = NULL;
               }
            if (gmc->gpsLocNext%16 == 0) fflush(gmc->logFp);
            }
         }
      }

   return(0);
   }
/* ======================================================================== */
int nmeaAngle(char *strAng,
              char sign) {
   char *pa;
   double a;
/* ------------------------------------------------------------------------ */
   pa = strrchr(strAng, '.');
   if (pa == NULL) return(TS_I4A_UNDEF);
   pa -= 2;
   if (pa < strAng) return(TS_I4A_UNDEF);
   a = TS_MIN2RAD * atof(pa);
   *pa = '\0';
   a += TS_DEG2RAD * atof(strAng);
   if (a < 0) return(TS_I4OfAng(a));
   if ((sign == 'N') || (sign == 'S')) {
      if (a > (TS_PIHALF + TS_FUZZA)) return(TS_I4A_UNDEF);
      }
   else {
      if (a > (TS_PI + TS_FUZZA)) return(TS_I4A_UNDEF);
      }
   if ((sign == 'N') || (sign == 'E')) return(TS_I4OfAng(a));
   else if ((sign == 'S') || (sign == 'W')) return(TS_I4OfAng(-a));
   else return(TS_I4A_UNDEF);
   }
/* ======================================================================== */
/* this version is preliminary: it ignores MMEA info and returns timestamp
   (seconds from second 0 of Y2K) based only on system time. */

int nmeaTime(char *st) {                     /* utc string of NMEA sentence */
   int is, im, ih;
   int n;
   static struct tm zeroTimeTm = {0, 0, 0, 1, 0, 100, 0, 0, 0};
   static time_t ztt;
   time_t ntt;
   unsigned secs;
/* ------------------------------------------------------------------------ */
   if (st == NULL) {
      ztt = mktime(&zeroTimeTm);
      debugTrace("zero time: %s", ctime(&ztt));
      return(0);
      }
   time(&ntt);
/* debugTrace("now  time: %s\n", ctime(&ntt));*/

   secs = (unsigned)difftime(ntt, ztt);
/* debugTrace("seconds %u\n", secs);*/

   st[6] = '\0';
   is = atoi(st + 4);
   st[4] = '\0';
   im = atoi(st + 2);
   st[2] = '\0';
   ih = atoi(st);
   n = (int)(3600.0 * ih + 60.0 * im + is);
   return(secs);                                   /* incomplete, test only */
   }
/* ======================================================================== */
void userError(char *fmt, ...) {
   static char msg[256];
   va_list      p_arg;
   va_start(p_arg, fmt);
/* ------------------------------------------------------------------------ */
   vsprintf(msg, fmt, p_arg);
   MessageBeep(MB_ICONHAND);
   MessageBox(GetFocus(), msg, APP_NAME, MB_ICONEXCLAMATION | MB_OK);
   va_end(p_arg);
   return;
   }
/* ======================================================================== */
void userInfo(char *fmt, ...) {
   static char msg[256];
   va_list      p_arg;
   va_start(p_arg, fmt);
/* ------------------------------------------------------------------------ */
   vsprintf(msg, fmt, p_arg);
   MessageBeep(MB_ICONHAND);
   MessageBox(GetFocus(), msg, APP_NAME, MB_ICONINFORMATION | MB_OK);
   va_end(p_arg);
   return;
   }
/* ======================================================================== */
/* Debugging tracing can be active or inactive. (cf. viewtgxm_util source).
 */
void debugTrace(char *fmt, ...) {
   static int initialized;
   static FILE *traceFp;
   static char  msg[LINE_LENGTH + 1];
   va_list      p_arg;
   int          i;
/* ------------------------------------------------------------------------ */
   if (initialized == 0) {                                    /* first call */
      initialized = 1;
      traceFp = (FILE *)fmt;                /* if NULL, we just go to sleep */
      return;
      }
   if (traceFp == NULL) return;                      /* no trace file open? */
   va_start(p_arg, fmt);
   if (fmt == NULL) traceFp = va_arg(p_arg, FILE *);          /* initialize */
   else {
      vsprintf(msg, fmt, p_arg);
      i = strlen(msg);
      if (i && traceFp) {
         fputs(msg, traceFp);
         if (msg[i - 1] != '\n') fputc('\n', traceFp);
         fflush(traceFp);
         }
      }
   va_end(p_arg);
   return;
   }
/* ======================================================================== */
#define MAX_NL 24
/* return pointer to a static buffer with compressed (if required) fileneme */
char *comprName(char *fileName) {
   static char cn[MAX_NL + 2];
   int i, l;
   char *pa;
/* ------------------------------------------------------------------------ */
   l = strlen(fileName);
   debugTrace("compressing log fileName: %s (%d)\n", fileName, l);
   if (l < MAX_NL) return(fileName);                 /* needs no shortening */
   cn[MAX_NL + 1] = '\0';
   pa = strrchr(fileName, '\\');
   if (pa == NULL) {
      for (i = MAX_NL; i > 2; i--) cn[i] = fileName[--l];
      cn[i--] = '.';
      cn[i--] = '.';
      cn[i--] = '.';
      }
   else {
      cn[MAX_NL + 1] = '\0';
      for (i = MAX_NL; i > 10; i--) cn[i] = fileName[--l];
      cn[i--] = '.';
      cn[i--] = '.';
      cn[i--] = '.';
      for (i = 0; i < 8; i++) cn[i] = fileName[i];
      }
   return(cn);
   }
/* ======================================================================== */
