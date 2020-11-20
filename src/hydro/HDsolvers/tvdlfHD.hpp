#include "../idefix.hpp"

// Compute Riemann fluxes from states using TVDLF solver
template<const int DIR, const int Xn, const int Xt, const int Xb>
void TvdlfHD(DataBlock & data, real gamma, real C2Iso) {
    int ioffset,joffset,koffset;

    idfx::pushRegion("TVDLF_Solver");
    ioffset=joffset=koffset=0;
    // Determine the offset along which we do the extrapolation
    if(DIR==IDIR) ioffset=1;
    if(DIR==JDIR) joffset=1;
    if(DIR==KDIR) koffset=1;

    IdefixArray4D<real> PrimL = data.PrimL;
    IdefixArray4D<real> PrimR = data.PrimR;
    IdefixArray4D<real> Flux = data.FluxRiemann;
    IdefixArray3D<real> cMax = data.cMax;

    real gamma_m1=gamma-ONE_F;

    idefix_for("TVDLF_Kernel",data.beg[KDIR],data.end[KDIR]+koffset,data.beg[JDIR],data.end[JDIR]+joffset,data.beg[IDIR],data.end[IDIR]+ioffset,
                        KOKKOS_LAMBDA (int k, int j, int i) 
            {

                // Primitive variables
                real vL[NVAR];
                real vR[NVAR];
                real vRL[NVAR];

                // Conservative variables
                real uL[NVAR];
                real uR[NVAR];

                // Flux (left and right)
                real fluxL[NVAR];
                real fluxR[NVAR];

                // Signal speeds
                real cRL, cmax;

                // 1-- Read primitive variables
                #pragma unroll
                for(int nv = 0 ; nv < NVAR; nv++) {
                    vL[nv] = PrimL(nv,k,j,i);
                    vR[nv] = PrimR(nv,k,j,i);
                    vRL[nv] = HALF_F*(vL[nv]+vR[nv]);
                }

                // 2-- Compute the conservative variables
                K_PrimToCons(uL, vL, gamma_m1);
                K_PrimToCons(uR, vR, gamma_m1);

                // 3-- Compute the left and right fluxes
                K_Flux(fluxL, vL, uL, C2Iso, Xn);
                K_Flux(fluxR, vR, uR, C2Iso, Xn);

                // 4-- Get the wave speed
#if HAVE_ENERGY
                cRL = SQRT((gamma_m1+ONE_F)*(vRL[PRS]/vRL[RHO]));
#else
                cRL = SQRT(C2Iso);
#endif
                cmax = FMAX(FABS(vRL[Xn]+cRL),FABS(vRL[Xn]-cRL));

                // 5-- Compute the flux from the left and right states
                #pragma unroll
                for(int nv = 0 ; nv < NVAR; nv++) {
                    Flux(nv,k,j,i) = HALF_F*(fluxL[nv]+fluxR[nv] - cmax*(uR[nv]-uL[nv]));
                }
                
                //6-- Compute maximum wave speed for this sweep
                cMax(k,j,i) = cmax;

            });

    idfx::popRegion();

}

