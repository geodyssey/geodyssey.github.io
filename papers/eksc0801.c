/* ========================================================================= */
#include <math.h>

struct sDircos {       /* point on sphere or spheroid, direction cosine form */
   double dcx, dcy, dcz;           /* direction cosines for 3 cartesian axes */
   };
struct sLatLong {    /* point on sphere or spheroid, latitude/longitude form */
   double phi, lambda;                                /* latitude, longitude */
   };
#define GP_FUZZ_SQUARE  2.458e-14

struct cdc64 {           /* CDC (concise direction cosines), 4-byte integers */
   int u, v;                                         /* u: i or j; v: j or k */
   };
#define CDC64_UNDEF    -2147483647
/* ========================================================================= */
/* Stub, expected to be replaced with a compiler- and processor-specific     */
/* implementation that, presumably via assembler code, makes use of the      */
/* simultaneous derivation of sine and cosine offered on most FPU-s.         */
/* For instance, for GNU C compiler (gcc) and Intel 386+ FPU, use:           */
/* void gp_SineCosine(double a, double *s, double *c) {                      */ 
/*    asm("fsincos" : "=t" (*c), "=u" (*s) : "0" (a));                       */
/*    }                                                                      */
void gp_SineCosine(double angle,                   /* given angle in radians */
                   double *pSin,                            /* returned sine */
                   double *pCos) {                        /* returned cosine */
   *pSin = sin(angle);
   *pCos = cos(angle); 
   };
/* ========================================================================= */
/* Given latitude and longitude, return vector form of the normal. note that */
/* input value is assumed to be well-formed, no data checking is performed.  */
void gp_LatLongToDircos(const struct sLatLong *pltln,      /* given lat/long */
                        struct sDircos *pdcos) {          /* returned vector */
   double cosPhi, cosLmbd, sinLmbd;
/* ------------------------------------------------------------------------- */
   gp_SineCosine(pltln->phi, &(pdcos->dcz), &cosPhi);
   gp_SineCosine(pltln->lambda, &sinLmbd, &cosLmbd);
   pdcos->dcx = cosPhi * cosLmbd;
   pdcos->dcy = cosPhi * sinLmbd;
   return;
   }
/* ========================================================================= */  
/* Given vector form of the normal, return latitude and longitude. note that */
/* input value is assumed to be well-formed, no data checking is performed.  */
void gp_DircosToLatLong(const struct sDircos *pdcos,         /* given vector */
                        struct sLatLong *pltln) {       /* returned lat/long */
   double aux;
/* ------------------------------------------------------------------------- */
   aux = pdcos->dcx * pdcos->dcx + pdcos->dcy * pdcos->dcy;
   pltln->phi = atan2(pdcos->dcz, sqrt(aux));
   if (aux < GP_FUZZ_SQUARE) pltln->lambda = 0.0;
   else pltln->lambda = atan2(pdcos->dcy, pdcos->dcx);
   return;
   }
/* ========================================================================= */  #define CDC64_SCALE 2147483640.0

#define M_BIT_1 0x80000000                                 /* high-order bit */
#define N_BIT_1 0x40000000                                 /* one next to it */
#define BITS_OA 0x3fffffff     /* all other: to force vacated high bits to 0 */

#define TRUNC_INT(a) (BITS_OA & (((int)(a * CDC64_SCALE))>>2)) /* int is I4! */
#define RSTR_DBL(i)  ((double)(((i)<<2) | 2) / CDC64_SCALE)

/* Given three-component vector form of spherical or spheroidal coordinates, */
/* return "concise" form, by dropping the component with the greatest        */
/* numerical value, scaling the magnitude of the other two so that they      */
/* will fit into a 32-bit integer each. Use the most-significant bits to     */
/* encode the order (i,j or k) and the sign of the dropped third component.  */  
void gp_DircosToCdc64(const struct sDircos *vct, /* given vector coordinates */
                      struct cdc64 *cdc) {        /* returned "concise" form */
   double adi, adj, adk;
/* ------------------------------------------------------------------------- */
   adi = fabs(vct->dcx);
   adj = fabs(vct->dcy);
   adk = fabs(vct->dcz);

   if ((adi > adj) && (adi > adk)) {                               /* drop i */
      if (vct->dcx > 0.0) {
         cdc->u = TRUNC_INT(vct->dcy);
         cdc->v = TRUNC_INT(vct->dcz);
         }
      else {                                          /* swap to keep xOy RH */
         cdc->u = TRUNC_INT(-vct->dcy) | N_BIT_1;
         cdc->v = TRUNC_INT(vct->dcz);
         }
      }
   else if (adj > adk) {                                           /* drop j */
      if (vct->dcy > 0.0) {
         cdc->u = TRUNC_INT(-vct->dcx);
         cdc->v = TRUNC_INT(vct->dcz) | N_BIT_1;
         }
      else {                                          /* swap to keep xOy RH */
         cdc->u = TRUNC_INT(vct->dcx) | N_BIT_1;
         cdc->v = TRUNC_INT(vct->dcz) | N_BIT_1;
         }
      }
   else {                                                          /* drop k */
      if (vct->dcz > 0.0) {
         cdc->u = TRUNC_INT(vct->dcy);
         cdc->v = TRUNC_INT(-vct->dcx) | M_BIT_1;
         }
      else {                                          /* swap to keep xOy RH */
         cdc->u = TRUNC_INT(vct->dcy) | N_BIT_1;
         cdc->v = TRUNC_INT(vct->dcx) | M_BIT_1;
         }
      }
   return;
   }
/* ------------------------------------------------------------------------- */
/* Given concise form of spherical or spheroidal coordinates, return their   */
/* full vector form, suitable for computations. This funcitin is the inverse */
/* of gp_DircosToCdc64().                                                    */
void gp_Cdc64ToDircos(const struct cdc64 *cdc,         /* given concise form */ 
                      struct sDircos *vct) {  /* returned vector coordinates */ 
   double du, dv, dw;
/* ------------------------------------------------------------------------- */
   du = RSTR_DBL(cdc->u);
   dv = RSTR_DBL(cdc->v);
   dw = 1.0 - du * du - dv * dv;
   if (dw < GP_FUZZ_SQUARE) dw = 0.0;
   else dw = sqrt(dw);
   if (cdc->v & M_BIT_1) {                                    /* restoring k */
      if (cdc->u & N_BIT_1) {                 /* flip kept, reverse computed */
         vct->dcx = dv;
         vct->dcy = du;
         vct->dcz = -dw;
         }
      else {
         vct->dcx = -dv;
         vct->dcy = du;
         vct->dcz = dw;
         }
      }
   else if (cdc->v & N_BIT_1) {                               /* restoring j */
      if (cdc->u & N_BIT_1) {                 /* flip kept, reverse computed */
         vct->dcx = du;
         vct->dcy = -dw;
         vct->dcz = dv;
         }
      else {
         vct->dcx = -du;
         vct->dcy = dw;
         vct->dcz = dv;
         }
      }
   else {                                                     /* restoring i */
      if (cdc->u & N_BIT_1) {                 /* flip kept, reverse computed */
         vct->dcx = -dw;
         vct->dcy = -du;
         vct->dcz = dv;
         }
      else {
         vct->dcx = dw;
         vct->dcy = du;
         vct->dcz = dv;
         }
      }
   return;
   }
/* ========================================================================= */  
