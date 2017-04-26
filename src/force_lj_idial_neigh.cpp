#include<force_lj_idial_neigh.h>

ForceLJIDialNeigh::ForceLJIDialNeigh(char** args, System* system, bool half_neigh_):Force(args,system,half_neigh_) {
  lj1 = t_fparams("ForceLJIDialNeigh::lj1",system->ntypes,system->ntypes);
  lj2 = t_fparams("ForceLJIDialNeigh::lj2",system->ntypes,system->ntypes);
  cutsq = t_fparams("ForceLJIDialNeigh::cutsq",system->ntypes,system->ntypes);
  num_nrepeat = t_fparams("ForceLJIDialNeigh::num_nrepeat",system->ntypes,system->ntypes);
}

void ForceLJIDialNeigh::init_coeff(int nargs, char** args) {
  int one_based_type = 1;
  int t1 = atoi(args[1])-one_based_type;
  int t2 = atoi(args[2])-one_based_type;
  double eps = atof(args[3]);
  double sigma = atof(args[4]);
  double cut = atof(args[5]);
  int max_nrepeat = atoi(args[6]);

  t_fparams::HostMirror h_lj1 = Kokkos::create_mirror_view(lj1);
  t_fparams::HostMirror h_lj2 = Kokkos::create_mirror_view(lj2);
  t_fparams::HostMirror h_cutsq = Kokkos::create_mirror_view(cutsq);
  t_fparams::HostMirror num_nrepeat = Kokkos::create_mirror_view(num_nrepeat);
  Kokkos::deep_copy(h_lj1,lj1);
  Kokkos::deep_copy(h_lj2,lj2);
  Kokkos::deep_copy(h_cutsq,cutsq);
  Kokkos::deep_copy(h_num_nrepeat,num_nrepeat);

  h_lj1(t1,t2) = 48.0 * eps * pow(sigma,12.0);
  h_lj2(t1,t2) = 24.0 * eps * pow(sigma,6.0);
  h_lj1(t2,t1) = h_lj1(t1,t2);
  h_lj2(t2,t1) = h_lj2(t1,t2);
  h_cutsq(t1,t2) = cut*cut;
  h_cutsq(t2,t1) = cut*cut;
  h_num_nrepeat = max_nrepeat;

  Kokkos::deep_copy(lj1,h_lj1);
  Kokkos::deep_copy(lj2,h_lj2);
  Kokkos::deep_copy(cutsq,h_cutsq);
  Kokkos::deep_copy(num_nrepeat,h_num_nrepeat);

  rnd_lj1 = lj1;
  rnd_lj2 = lj2;
  rnd_cutsq = cutsq;
  rnd_num_nrepeat = num_nrepeat;
  step = 0;
};

void ForceLJIDialNeigh::compute(System* system, Binning* binning, Neighbor* neighbor_ ) {
  // Set internal data handles
  if(neighbor_->neigh_type == NEIGH_CSR) {
    NeighborCSR<t_neigh_mem_space>* neighbor = (NeighborCSR<t_neigh_mem_space>*) neighbor_;
    neigh_list = neighbor->get_neigh_list();
  } else if(neighbor_->neigh_type == NEIGH_CSR_MAPCONSTR) {
    NeighborCSRMapConstr<t_neigh_mem_space>* neighbor = (NeighborCSRMapConstr<t_neigh_mem_space>*) neighbor_;
    neigh_list = neighbor->get_neigh_list();
  }

  N_local = system->N_local;
  x = system->x;
  f = system->f;
  f_a = system->f;
  type = system->type;
  if(half_neigh)
    Kokkos::parallel_for("ForceLJIDialNeigh::computer", t_policy_half_neigh(0, system->N_local), *this);
  else
    Kokkos::parallel_for("ForceLJIDialNeigh::computer", t_policy_full_neigh(0, system->N_local), *this);
  Kokkos::fence();

  // Reset internal data handles so we don't keep a reference count
  /*x = t_x();
  type = t_type();
  f = t_f();
  neigh_list = NeighborCSR<t_neigh_mem_space>::t_neigh_list();*/
  step++;
}

const char* ForceLJIDialNeigh::name() { return half_neigh?"ForceLJNeighHalf":"ForceLJNeighFull"; }

