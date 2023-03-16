/* ccdata.c - columbus application-wide data initialized at
   program start-up time and used in read-only mode thereafter. 
   It is laid out as a columbusControl structure; the pointer
   to which is returned to the invoker by a call to ccDP().
   All of it is initialized at the program initialization time
   and not changed during the execution.
*/
#include "columbus.h"
static struct columbusControl ccd;            /* all nulls at program start */
/* ======================================================================== */
/* Initialize the data. Return 1 for error, 0 otherwise.
   The function is called only once at program start-up.
 */
int initColumbus(void) {
   SInt16      scrnHRes, scrnVRes;
   CFBundleRef cfb;
   CFURLRef    cfr;
   char        fileName[FILENAME_MAX + 2];
   char        ellName[32];
   OSStatus	   err;
/* ------------------------------------------------------------------------ */
   ScreenRes(&scrnHRes, &scrnVRes);
/* Come on now Apple, it's 2002: "inches"? - find pixel size in SI measure: */
   ccd.screen_unit_x = 0.0254 / (double)scrnHRes;
   ccd.screen_unit_y = 0.0254 / (double)scrnVRes;
/* fprintf(stderr, "Pixel size: %g x %g\n", ccd.screen_unit_x, ccd.screen_unit_y); */ 

/* Initialize ellipsoid parameters. */
   h4_SetEllipsoidParameters(WGS_84, &ccd.ellprm, ellName);
/* fprintf(stderr, "Ellipsoid name: %s.\n", ellName); */ 
   
   cfb = CFBundleGetMainBundle();
/* fprintf(stderr, "Bundle: %d\n", (int)cfb); */
   if (!cfb) return(101);

   cfr = CFBundleCopyResourceURL(cfb, CFSTR(COAST_SGM_FN), NULL, NULL);
   if (!cfr) return(102);
   err = CFURLGetFileSystemRepresentation(cfr, false, fileName, FILENAME_MAX);
   CFRelease(cfr);
   if (err == 0) return(103);
/* fprintf(stderr, "Segment file: [%s]\n", fileName); */ 
   ccd.cstsFp = fopen(fileName, "rb");
   if (ccd.cstsFp == NULL) return(104);

   cfr = CFBundleCopyResourceURL(cfb, CFSTR(COAST_VTX_FN), NULL, NULL);
   if (!cfr) return(105);
   err = CFURLGetFileSystemRepresentation(cfr, false, fileName, FILENAME_MAX);
   CFRelease(cfr);
   if (err == 0) return(106);
/* fprintf(stderr, "Vertex file: [%s]\n", fileName); */ 
   ccd.cstvFp = fopen(fileName, "rb");
   if (ccd.cstvFp == NULL) return(107);
   
   return(0);
   }
/* ======================================================================== */
void closeColumbus(void) { 
   if (ccd.cstsFp) fclose(ccd.cstsFp);
   if (ccd.cstvFp) fclose(ccd.cstvFp);
   return;
   }
/* ======================================================================== */
const struct columbusControl *ccDP() {    /* called whenever read-access... */ 
   return(&ccd);                       /* ...is required outside this book. */
   }
/* ======================================================================== */
