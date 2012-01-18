#include "copyright.h"
/*============================================================================*/
/*! \file bvals_mhd.c
 *  \brief Sets boundary conditions (quantities in ghost zones) 
 *   for the fluxes on each edge of a Grid.
 *
 * PURPOSE: Sets boundary conditions (quantities in ghost zones) 
 *   for the fluxes on each edge of a Grid.  See comments at start of
 *   bvals_mhd.c for more details.  MPI is NOT implemented here.
 * The only BC functions implemented here are for
 * Outflow boundary conditions
 *
 * CONTAINS PUBLIC FUNCTIONS: 
 * - bvals_ionrad() - calls appropriate functions to set ghost cells
 * - bvals_ionrad_init() - sets function pointers used by bvals_mhd()
/*============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "athena.h"
#include "globals.h"
#include "prototypes.h"

#ifdef ION_RADIATION

static VGFun_t ix1_radBCFun = NULL, ox1_radBCFun = NULL;
static VGFun_t ix2_radBCFun = NULL, ox2_radBCFun = NULL;
static VGFun_t ix3_radBCFun = NULL, ox3_radBCFun = NULL;

/*==============================================================================
 * PRIVATE FUNCTION PROTOTYPES:
 *   outflow_flux_???()  - outflow BCs at boundary ???
 *============================================================================*/

static void outflow_flux_ix1(GridS *pG);
static void outflow_flux_ox1(GridS *pG);
static void outflow_flux_ix2(GridS *pG);
static void outflow_flux_ox2(GridS *pG);
static void outflow_flux_ix3(GridS *pG);
static void outflow_flux_ox3(GridS *pG);

static void ProlongateLater(GridS *pG);


/*=========================== PUBLIC FUNCTIONS ===============================*/

/*----------------------------------------------------------------------------*/
/*! \fn void bvals_mhd(DomainS *pD)
 *  \brief Calls appropriate functions to set ghost zones.  
 *
 *   The function
 *   pointers (*(pD->???_BCFun)) are set by bvals_init() to be either a
 *   user-defined function, or one of the functions corresponding to reflecting,
 *   periodic, or outflow.  If the left- or right-Grid ID numbers are >= 1
 *   (neighboring grids exist), then MPI calls are used.
 *
 * Order for updating boundary conditions must always be x1-x2-x3 in order to
 * fill the corner cells properly
 */

void bvals_ionrad(DomainS *pD)
{
  GridS *pGrid = (pD->Grid);

/*--- Step 1. ------------------------------------------------------------------
 * Boundary Conditions in x1-direction */

  if (pGrid->Nx[0] > 1){

/* Physical boundaries on both left and right */
    if (pGrid->rx1_id < 0 && pGrid->lx1_id < 0) {
      (*(ix1_radBCFun))(pGrid);
      (*(ox1_radBCFun))(pGrid);
    } 

  }

/*--- Step 2. ------------------------------------------------------------------
 * Boundary Conditions in x2-direction */

  if (pGrid->Nx[1] > 1){

/* Physical boundaries on both left and right */
    if (pGrid->rx2_id < 0 && pGrid->lx2_id < 0) {
      (*(ix2_radBCFun))(pGrid);
      (*(ox2_radBCFun))(pGrid);
    } 


  }

/*--- Step 3. ------------------------------------------------------------------
 * Boundary Conditions in x3-direction */

  if (pGrid->Nx[2] > 1){


/* Physical boundaries on both left and right */
    if (pGrid->rx3_id < 0 && pGrid->lx3_id < 0) {
      (*(ix3_radBCFun))(pGrid);
      (*(ox3_radBCFun))(pGrid);
    } 

  }

  return;
}

/*----------------------------------------------------------------------------*/
/*! \fn void bvals_mhd_init(MeshS *pM)
 *  \brief Sets function pointers for physical boundaries during
 *   initialization, allocates memory for send/receive buffers with MPI.
 */

void bvals_ionrad_init(MeshS *pM)
{
  GridS *pG;
  DomainS *pD;
  int i,nl,nd,irefine;

/* Cycle through all the Domains that have active Grids on this proc */

  for (nl=0; nl<(pM->NLevels); nl++){
  for (nd=0; nd<(pM->DomainsPerLevel[nl]); nd++){
  if (pM->Domain[nl][nd].Grid != NULL) {
    pD = (DomainS*)&(pM->Domain[nl][nd]);  /* ptr to Domain */
    pG = pM->Domain[nl][nd].Grid;          /* ptr to Grid */
    irefine = 1;
    for (i=1;i<=nl;i++) irefine *= 2;   /* C pow fn only takes doubles !! */

/* Set function pointers for physical boundaries in x1-direction -------------*/

    if(pG->Nx[0] > 1) {

/*---- ix1 boundary ----------------------------------------------------------*/

      if(ix1_radBCFun == NULL){    /* BCFun ptr was not set in prob gen */

	/* Domain boundary is in interior of root */
        if(pD->Disp[0] != 0) {      
          ix1_radBCFun = ProlongateLater;
	  
	  /* Domain is at L-edge of root Domain */
	} else {                    
	  ix1_radBCFun = outflow_flux_ix1;
	}
      }
      
/*---- ox1 boundary ----------------------------------------------------------*/

      if(ox1_radBCFun == NULL){    /* BCFun ptr was not set in prob gen */
	
	/* Domain boundary is in interior of root */
        if((pD->Disp[0] + pD->Nx[0])/irefine != pM->Nx[0]) {
          ox1_radBCFun = ProlongateLater;
	  
	  /* Domain is at R-edge of root Domain */
	} else {
	  ox1_radBCFun = outflow_flux_ox1;
	}
      }
    }

/* Set function pointers for physical boundaries in x2-direction -------------*/

    if(pG->Nx[1] > 1) {

/*---- ix2 boundary ----------------------------------------------------------*/

      if(ix2_radBCFun == NULL){    /* BCFun ptr was not set in prob gen */

/* Domain boundary is in interior of root */
        if(pD->Disp[1] != 0) {
          ix2_radBCFun = ProlongateLater;
	  
	  /* Domain is at L-edge of root Domain */
	} else {
	  ix2_radBCFun = outflow_flux_ix2;

	}
      }

/*---- ox2 boundary ----------------------------------------------------------*/

      if(ox2_radBCFun == NULL){    /* BCFun ptr was not set in prob gen */

	/* Domain boundary is in interior of root */
        if((pD->Disp[1] + pD->Nx[1])/irefine != pM->Nx[1]) {
          ox2_radBCFun = ProlongateLater;

	  /* Domain is at R-edge of root Domain */
	} else {
	  ox2_radBCFun = outflow_flux_ox2;
	}
      }
    }

/* Set function pointers for physical boundaries in x3-direction -------------*/

    if(pG->Nx[2] > 1) {

/*---- ix3 boundary ----------------------------------------------------------*/
      
      if(ix3_radBCFun == NULL){    /* BCFun ptr was not set in prob gen */
	
/* Domain boundary is in interior of root */
        if(pD->Disp[2] != 0) {
          ix3_radBCFun = ProlongateLater;
	  
	  /* Domain is at L-edge of root Domain */
	} else {
	  ix3_radBCFun = outflow_flux_ix3;
	}
      }
/*---- ox3 boundary ----------------------------------------------------------*/

      if(ox3_radBCFun == NULL){    /* BCFun ptr was not set in prob gen */

	/* Domain boundary is in interior of root */
        if((pD->Disp[2] + pD->Nx[2])/irefine != pM->Nx[2]) {
          ox3_radBCFun = ProlongateLater;

	  /* Domain is at R-edge of root Domain */
	} else {
	  ox3_radBCFun = outflow_flux_ox3;
	}
      }
    }

  }}}  /* End loop over all Domains with active Grids -----------------------*/


  return;
}


/*=========================== PRIVATE FUNCTIONS ==============================*/
/* Following are the functions:
 *   outflow_??? where ???=[ix1,ox1,ix2,ox2,ix3,ox3]
 *   periodic_???
 *   conduct_???
 *   pack_???
 *   unpack_???
 */


/*----------------------------------------------------------------------------*/
/*! \fn static void outflow_flux_ix1(GridS *pGrid)
 *  \brief OUTFLOW boundary condition, Inner x1 boundary (bc_ix1=2) */

static void outflow_flux_ix1(GridS *pGrid)
{
  int is = pGrid->is;
  int js = pGrid->js, je = pGrid->je;
  int ks = pGrid->ks, ke = pGrid->ke;
  int i,j,k;
#ifdef MHD
  int ju, ku; /* j-upper, k-upper */
#endif

  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=1; i<=nghost; i++) {
        pGrid->U[k][j][is-i] = pGrid->U[k][j][is];
      }
    }
  }

#ifdef MHD
/* B1i is not set at i=is-nghost */
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=1; i<=nghost-1; i++) {
        pGrid->B1i[k][j][is-i] = pGrid->B1i[k][j][is];
      }
    }
  }

  if (pGrid->Nx[1] > 1) ju=je+1; else ju=je;
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=ju; j++) {
      for (i=1; i<=nghost; i++) {
        pGrid->B2i[k][j][is-i] = pGrid->B2i[k][j][is];
      }
    }
  }

  if (pGrid->Nx[2] > 1) ku=ke+1; else ku=ke;
  for (k=ks; k<=ku; k++) {
    for (j=js; j<=je; j++) {
      for (i=1; i<=nghost; i++) {
        pGrid->B3i[k][j][is-i] = pGrid->B3i[k][j][is];
      }
    }
  }
#endif /* MHD */

  return;
}

/*----------------------------------------------------------------------------*/
/*! \fn static void outflow_flux_ox1(GridS *pGrid)
 *  \brief OUTFLOW boundary conditions, Outer x1 boundary (bc_ox1=2) */

static void outflow_flux_ox1(GridS *pGrid)
{
  int ie = pGrid->ie;
  int js = pGrid->js, je = pGrid->je;
  int ks = pGrid->ks, ke = pGrid->ke;
  int i,j,k;
#ifdef MHD
  int ju, ku; /* j-upper, k-upper */
#endif

  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=1; i<=nghost; i++) {
        pGrid->U[k][j][ie+i] = pGrid->U[k][j][ie];
      }
    }
  }

#ifdef MHD
/* i=ie+1 is not a boundary condition for the interface field B1i */
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=2; i<=nghost; i++) {
        pGrid->B1i[k][j][ie+i] = pGrid->B1i[k][j][ie];
      }
    }
  }

  if (pGrid->Nx[1] > 1) ju=je+1; else ju=je;
  for (k=ks; k<=ke; k++) {
    for (j=js; j<=ju; j++) {
      for (i=1; i<=nghost; i++) {
        pGrid->B2i[k][j][ie+i] = pGrid->B2i[k][j][ie];
      }
    }
  }

  if (pGrid->Nx[2] > 1) ku=ke+1; else ku=ke;
  for (k=ks; k<=ku; k++) {
    for (j=js; j<=je; j++) {
      for (i=1; i<=nghost; i++) {
        pGrid->B3i[k][j][ie+i] = pGrid->B3i[k][j][ie];
      }
    }
  }
#endif /* MHD */

  return;
}

/*----------------------------------------------------------------------------*/
/*! \fn static void outflow_flux_ix2(GridS *pGrid)
 *  \brief OUTFLOW boundary conditions, Inner x2 boundary (bc_ix2=2) */

static void outflow_flux_ix2(GridS *pGrid)
{
  int is = pGrid->is, ie = pGrid->ie;
  int js = pGrid->js;
  int ks = pGrid->ks, ke = pGrid->ke;
  int i,j,k;
#ifdef MHD
  int ku; /* k-upper */
#endif

  for (k=ks; k<=ke; k++) {
    for (j=1; j<=nghost; j++) {
      for (i=is-nghost; i<=ie+nghost; i++) {
        pGrid->U[k][js-j][i] = pGrid->U[k][js][i];
      }
    }
  }

#ifdef MHD
/* B1i is not set at i=is-nghost */
  for (k=ks; k<=ke; k++) {
    for (j=1; j<=nghost; j++) {
      for (i=is-(nghost-1); i<=ie+nghost; i++) {
        pGrid->B1i[k][js-j][i] = pGrid->B1i[k][js][i];
      }
    }
  }

/* B2i is not set at j=js-nghost */
  for (k=ks; k<=ke; k++) {
    for (j=1; j<=nghost-1; j++) {
      for (i=is-nghost; i<=ie+nghost; i++) {
        pGrid->B2i[k][js-j][i] = pGrid->B2i[k][js][i];
      }
    }
  }

  if (pGrid->Nx[2] > 1) ku=ke+1; else ku=ke;
  for (k=ks; k<=ku; k++) {
    for (j=1; j<=nghost; j++) {
      for (i=is-nghost; i<=ie+nghost; i++) {
        pGrid->B3i[k][js-j][i] = pGrid->B3i[k][js][i];
      }
    }
  }
#endif /* MHD */

  return;
}

/*----------------------------------------------------------------------------*/
/*! \fn static void outflow_flux_ox2(GridS *pGrid)
 *  \brief OUTFLOW boundary conditions, Outer x2 boundary (bc_ox2=2) */

static void outflow_flux_ox2(GridS *pGrid)
{
  int is = pGrid->is, ie = pGrid->ie;
  int je = pGrid->je;
  int ks = pGrid->ks, ke = pGrid->ke;
  int i,j,k;
#ifdef MHD
  int ku; /* k-upper */
#endif

  for (k=ks; k<=ke; k++) {
    for (j=1; j<=nghost; j++) {
      for (i=is-nghost; i<=ie+nghost; i++) {
        pGrid->U[k][je+j][i] = pGrid->U[k][je][i];
      }
    }
  }

#ifdef MHD
/* B1i is not set at i=is-nghost */
  for (k=ks; k<=ke; k++) {
    for (j=1; j<=nghost; j++) {
      for (i=is-(nghost-1); i<=ie+nghost; i++) {
        pGrid->B1i[k][je+j][i] = pGrid->B1i[k][je][i];
      }
    }
  }

/* j=je+1 is not a boundary condition for the interface field B2i */
  for (k=ks; k<=ke; k++) {
    for (j=2; j<=nghost; j++) {
      for (i=is-nghost; i<=ie+nghost; i++) {
        pGrid->B2i[k][je+j][i] = pGrid->B2i[k][je][i];
      }
    }
  }

  if (pGrid->Nx[2] > 1) ku=ke+1; else ku=ke;
  for (k=ks; k<=ku; k++) {
    for (j=1; j<=nghost; j++) {
      for (i=is-nghost; i<=ie+nghost; i++) {
        pGrid->B3i[k][je+j][i] = pGrid->B3i[k][je][i];
      }
    }
  }
#endif /* MHD */

  return;
}

/*----------------------------------------------------------------------------*/
/*! \fn static void outflow_flux_ix3(GridS *pGrid)
 *  \brief OUTFLOW boundary conditions, Inner x3 boundary (bc_ix3=2) */

static void outflow_flux_ix3(GridS *pGrid)
{
  int is = pGrid->is, ie = pGrid->ie;
  int js = pGrid->js, je = pGrid->je;
  int ks = pGrid->ks;
  int i,j,k;

  for (k=1; k<=nghost; k++) {
    for (j=js-nghost; j<=je+nghost; j++) {
      for (i=is-nghost; i<=ie+nghost; i++) {
        pGrid->U[ks-k][j][i] = pGrid->U[ks][j][i];
      }
    }
  }

#ifdef MHD
/* B1i is not set at i=is-nghost */
  for (k=1; k<=nghost; k++) {
    for (j=js-nghost; j<=je+nghost; j++) {
      for (i=is-(nghost-1); i<=ie+nghost; i++) {
        pGrid->B1i[ks-k][j][i] = pGrid->B1i[ks][j][i];
      }
    }
  }

/* B2i is not set at j=js-nghost */
  for (k=1; k<=nghost; k++) {
    for (j=js-(nghost-1); j<=je+nghost; j++) {
      for (i=is-nghost; i<=ie+nghost; i++) {
        pGrid->B2i[ks-k][j][i] = pGrid->B2i[ks][j][i];
      }
    }
  }

/* B3i is not set at k=ks-nghost */
  for (k=1; k<=nghost-1; k++) {
    for (j=js-nghost; j<=je+nghost; j++) {
      for (i=is-nghost; i<=ie+nghost; i++) {
        pGrid->B3i[ks-k][j][i] = pGrid->B3i[ks][j][i];
      }
    }
  }
#endif /* MHD */

  return;
}

/*----------------------------------------------------------------------------*/
/*! \fn static void outflow_flux_ox3(GridS *pGrid)
 *  \brief OUTFLOW boundary conditions, Outer x3 boundary (bc_ox3=2) */

static void outflow_flux_ox3(GridS *pGrid)
{
  int is = pGrid->is, ie = pGrid->ie;
  int js = pGrid->js, je = pGrid->je;
  int ke = pGrid->ke;
  int i,j,k;

  for (k=1; k<=nghost; k++) {
    for (j=js-nghost; j<=je+nghost; j++) {
      for (i=is-nghost; i<=ie+nghost; i++) {
        pGrid->U[ke+k][j][i] = pGrid->U[ke][j][i];
      }
    }
  }

#ifdef MHD
/* B1i is not set at i=is-nghost */
  for (k=1; k<=nghost; k++) {
    for (j=js-nghost; j<=je+nghost; j++) {
      for (i=is-(nghost-1); i<=ie+nghost; i++) {
        pGrid->B1i[ke+k][j][i] = pGrid->B1i[ke][j][i];
      }
    }
  }

/* B2i is not set at j=js-nghost */
  for (k=1; k<=nghost; k++) {
    for (j=js-(nghost-1); j<=je+nghost; j++) {
      for (i=is-nghost; i<=ie+nghost; i++) {
        pGrid->B2i[ke+k][j][i] = pGrid->B2i[ke][j][i];
      }
    }
  }

/* k=ke+1 is not a boundary condition for the interface field B3i */
  for (k=2; k<=nghost; k++) {
    for (j=js-nghost; j<=je+nghost; j++) {
      for (i=is-nghost; i<=ie+nghost; i++) {
        pGrid->B3i[ke+k][j][i] = pGrid->B3i[ke][j][i];
      }
    }
  }
#endif /* MHD */

  return;
}


/*----------------------------------------------------------------------------*/
/*! \fn static void ProlongateLater(GridS *pGrid)
 *  \brief PROLONGATION boundary conditions.  
 *
 *  Nothing is actually done here, the
 * prolongation is actually handled in ProlongateGhostZones in main loop, so
 * this is just a NoOp Grid function.  */

static void ProlongateLater(GridS *pGrid)
{
  return;
}

#endif /* ION_RADIATION */