/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
   Contributing author: Inderaj Bains (NVIDIA), ibains@nvidia.com
------------------------------------------------------------------------- */

#include <iostream>
#include <cassert>
#include <math.h>

#include "lj_expand_gpu_memory.h"

using namespace std;

static LJE_GPU_Memory<PRECISION,ACC_PRECISION> LJEMF;

// ---------------------------------------------------------------------------
// Allocate memory on host and device and copy constants to device
// ---------------------------------------------------------------------------
bool lje_gpu_init(const int ntypes, double **cutsq, double **host_lj1,
                  double **host_lj2, double **host_lj3, double **host_lj4,
                  double **offset, double **shift, double *special_lj,
                  const int inum, const int nall, const int max_nbors, 
                  const int maxspecial, const double cell_size, int &gpu_mode,
                  FILE *screen) {
  LJEMF.clear();
  gpu_mode=LJEMF.device->gpu_mode();
  double gpu_split=LJEMF.device->particle_split();
  int first_gpu=LJEMF.device->first_device();
  int last_gpu=LJEMF.device->last_device();
  int world_me=LJEMF.device->world_me();
  int gpu_rank=LJEMF.device->gpu_rank();
  int procs_per_gpu=LJEMF.device->procs_per_gpu();

  LJEMF.device->init_message(screen,"lj/expand",first_gpu,last_gpu);

  bool message=false;
  if (LJEMF.device->replica_me()==0 && screen)
    message=true;

  if (message) {
    fprintf(screen,"Initializing GPU and compiling on process 0...");
    fflush(screen);
  }

  if (world_me==0) {
    bool init_ok=LJEMF.init(ntypes, cutsq, host_lj1, host_lj2, host_lj3,
                            host_lj4, offset, shift, special_lj, inum, nall, 300,
                            maxspecial, cell_size, gpu_split, screen);
    if (!init_ok)
      return false;
  }

  LJEMF.device->world_barrier();
  if (message)
    fprintf(screen,"Done.\n");

  for (int i=0; i<procs_per_gpu; i++) {
    if (message) {
      if (last_gpu-first_gpu==0)
        fprintf(screen,"Initializing GPU %d on core %d...",first_gpu,i);
      else
        fprintf(screen,"Initializing GPUs %d-%d on core %d...",first_gpu,
                last_gpu,i);
      fflush(screen);
    }
    if (gpu_rank==i && world_me!=0) {
      bool init_ok=LJEMF.init(ntypes, cutsq, host_lj1, host_lj2, host_lj3,
                              host_lj4, offset, shift, special_lj, inum, nall, 
                              300,maxspecial, cell_size, gpu_split,screen);
      if (!init_ok)
        return false;
    }
    LJEMF.device->world_barrier();
    if (message) 
      fprintf(screen,"Done.\n");
  }
  if (message)
    fprintf(screen,"\n");
  return true;
}

void lje_gpu_clear() {
  LJEMF.clear();
}

int * lje_gpu_compute_n(const int ago, const int inum_full,
                        const int nall, double **host_x, int *host_type,
                        double *boxlo, double *boxhi, int *tag, int **nspecial,
                        int **special, const bool eflag, const bool vflag,
                        const bool eatom, const bool vatom, int &host_start,
                        const double cpu_time, bool &success) {
  return LJEMF.compute(ago, inum_full, nall, host_x, host_type, boxlo,
                       boxhi, tag, nspecial, special, eflag, vflag, eatom,
                       vatom, host_start, cpu_time, success);
}  
			
void lje_gpu_compute(const int ago, const int inum_full, const int nall,
                     double **host_x, int *host_type, int *ilist, int *numj,
                     int **firstneigh, const bool eflag, const bool vflag,
                     const bool eatom, const bool vatom, int &host_start,
                     const double cpu_time, bool &success) {
  LJEMF.compute(ago,inum_full,nall,host_x,host_type,ilist,numj,
                firstneigh,eflag,vflag,eatom,vatom,host_start,cpu_time,success);
}

double lje_gpu_bytes() {
  return LJEMF.host_memory_usage();
}


