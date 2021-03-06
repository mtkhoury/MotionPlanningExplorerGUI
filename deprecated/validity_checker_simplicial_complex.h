#pragma once
#include "validity_checker_ompl.h"
#include "planner/cspace/cover/open_set_convex.h"

class ValidityCheckerSimplicialComplex: public OMPLValidityChecker
{
  public:
    ValidityCheckerSimplicialComplex(const ob::SpaceInformationPtr &si, CSpaceOMPL *cspace_);
    cover::OpenSetConvex* ComputeNeighborhood(const ob::State* state) const;
    std::vector<cover::OpenSetConvex*> GetConvexWorkspaceCover(const ob::State* start, const ob::State* goal) const;
};
