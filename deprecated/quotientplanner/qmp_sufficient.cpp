#include "qmp_sufficient.h"
#include "elements/plannerdata_vertex_annotated.h"

#include <ompl/datastructures/PDF.h>
#include <ompl/base/objectives/PathLengthOptimizationObjective.h>
#include <ompl/base/goals/GoalSampleableRegion.h>
#include <boost/foreach.hpp>
#include <boost/graph/graphviz.hpp>

#include <boost/random/linear_congruential.hpp>
#include <boost/random/variate_generator.hpp>

using namespace og;
using namespace ob;
#define foreach BOOST_FOREACH

QMPSufficient::QMPSufficient(const ob::SpaceInformationPtr &si, QuotientChart *parent_ ):
  BaseT(si, parent_)
{
  setName("QMPSufficient"+std::to_string(id));
  number_of_samples = 0;
  number_of_infeasible_samples = 0;
  number_of_sufficient_samples = 0;

  checker = dynamic_pointer_cast<OMPLValidityCheckerNecessarySufficient>(si->getStateValidityChecker());
  if(!checker){
    checkSufficiency = false;
    //OMPL_WARN("Sufficient checker not set!");
  }else{
    checkSufficiency = true;
  }

}
void QMPSufficient::setup()
{
  BaseT::setup();
  if(setup_){
    Vertex vs = startM_.at(0);
    Vertex vg = goalM_.at(0);
    if(checkSufficiency){
      G[vs].isSufficient = checker->IsSufficientFeasible(G[vs].state);
      G[vg].isSufficient = checker->IsSufficientFeasible(G[vg].state);
    }
    G[vs].isFeasible = true;
    G[vg].isFeasible = true;
  }
}


QMPSufficient::~QMPSufficient()
{
}

void QMPSufficient::Grow(double t){
  ob::State *state = xstates[0];
  iterations_++;
  bool isFeasible = Sample(state);

  number_of_samples++;
  // Three cases:
  // state is infeasible => state X P_k is infeasible
  // state is sufficient feasible => state X X_{k+1} is feasible
  // state is feasible (but not sufficient) => nothing can be said

  Vertex m = CreateNewVertex(state);
  if(isFeasible)
  {
    G[m].isFeasible = true;
    if(checkSufficiency){
      bool sufficient = checker->IsSufficientFeasible(state);
      if(sufficient){
        number_of_sufficient_samples++;
        double d = checker->SufficientDistance(state);
        G[m].isSufficient = true;
        G[m].outerApproximationDistance = d;
      }else{
        double d = checker->Distance(state);
        G[m].isSufficient = false;
        G[m].innerApproximationDistance = d;
      }
    }
  }else{
    number_of_infeasible_samples++;
  }
}

bool QMPSufficient::Sample(ob::State *q_random)
{
  Q1_sampler->sampleUniform(q_random);
  //totalNumberOfSamples++;
  //if(parent == nullptr){
  //  Q1_sampler->sampleUniform(q_random);
  //}else{
  //  //Adjusted sampling function: Sampling in G0 x X1
  //  ob::SpaceInformationPtr Q0 = parent->getSpaceInformation();
  //  base::State *s_X1 = X1->allocState();
  //  base::State *s_Q0 = Q0->allocState();

  //  X1_sampler->sampleUniform(s_X1);
  //  parent->SampleGraph(s_Q0);
  //  mergeStates(s_Q0, s_X1, q_random);

  //  X1->freeState(s_X1);
  //  Q0->freeState(s_Q0);
  //}
  return Q1->isValid(q_random);
}
using FeasibilityType = PlannerDataVertexAnnotated::FeasibilityType;
void QMPSufficient::getPlannerData(ob::PlannerData &data) const
{

  //###########################################################################
  //Get Path for this chart
  //###########################################################################
  std::vector<int> path;
  path.push_back(GetHorizontalIndex());
  og::QuotientChart *parent = static_cast<og::QuotientChart*>(GetParent());
  while(parent!=nullptr)
  {
    path.push_back(parent->GetHorizontalIndex());
    parent = static_cast<og::QuotientChart*>(parent->GetParent());
  }

  std::reverse(std::begin(path), std::end(path));
  //###########################################################################
  //Get Data from this chart
  //###########################################################################
  foreach (const Vertex v, boost::vertices(G))
  {
    PlannerDataVertexAnnotated p(G[v].state);

    p.SetLevel(level);
    p.SetPath(path);

    if(!G[v].isFeasible){
      p.SetFeasibility(FeasibilityType::INFEASIBLE);
    }else{
      if(G[v].isSufficient){
        p.SetFeasibility(FeasibilityType::SUFFICIENT_FEASIBLE);
        p.SetOpenNeighborhoodDistance(G[v].outerApproximationDistance);
      }else{
        p.SetFeasibility(FeasibilityType::FEASIBLE);
        p.SetOpenNeighborhoodDistance(G[v].innerApproximationDistance);
      }
    }

    if(G[v].start) data.addStartVertex(p);
    else if(G[v].goal) data.addGoalVertex(p);
    else data.addVertex(p);
  }
  std::cout << "Samples: " << number_of_samples << " (" 
    << 100.0*(double)number_of_infeasible_samples/(double)number_of_samples << "\% infeasible | "
    << 100.0*(double)(number_of_samples-number_of_infeasible_samples)/(double)number_of_samples << "\% feasible | "
    << 100.0*(double)number_of_sufficient_samples/(double)number_of_samples << "\% sufficient)"
    << std::endl;
}
