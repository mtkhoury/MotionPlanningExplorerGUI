#pragma once
#include "planner/integrator/principalfibrebundle.h"
#include "planner/integrator/tangentbundle.h"
#include "planner/cspace/cspace_input.h"
#include "elements/path_pwl_euclid.h"

#include <ompl/base/spaces/SE3StateSpace.h>
#include <ompl/base/ScopedState.h>
#include <ompl/base/StateSpace.h>
#include <ompl/base/PlannerData.h>
#include <ompl/base/PlannerDataGraph.h>
#include <ompl/base/objectives/PathLengthOptimizationObjective.h>
#include <ompl/tools/benchmark/Benchmark.h>

#include <ompl/geometric/SimpleSetup.h>
#include <ompl/geometric/PathGeometric.h>

#include <ompl/control/SimpleSetup.h>
#include <ompl/control/SpaceInformation.h>
#include <ompl/control/spaces/RealVectorControlSpace.h>
#include <Planning/RobotCSpace.h>

namespace ob = ompl::base;
namespace oc = ompl::control;
namespace og = ompl::geometric;

namespace ompl{
  namespace control{
    typedef std::shared_ptr<RealVectorControlSpace> RealVectorControlSpacePtr;
    typedef std::shared_ptr<StatePropagator> StatePropagatorPtr;
  }
  namespace base{
    typedef std::shared_ptr<StateValidityChecker> StateValidityCheckerPtr;
  }
}

class CSpaceOMPL
{
  public:

    CSpaceOMPL(Robot *robot_, CSpace *kspace_);

    virtual const oc::StatePropagatorPtr StatePropagatorPtr(oc::SpaceInformationPtr si) = 0;
    virtual const ob::StateValidityCheckerPtr StateValidityCheckerPtr(oc::SpaceInformationPtr si) = 0;
    virtual const ob::StateValidityCheckerPtr StateValidityCheckerPtr(ob::SpaceInformationPtr si) = 0;

    virtual ob::ScopedState<> ConfigToOMPLState(const Config &q) = 0;
    virtual Config OMPLStateToConfig(const ob::State *qompl) = 0;

    virtual Config OMPLStateToConfig(const ob::ScopedState<> &qompl){
      const ob::State* s = qompl.get();
      return OMPLStateToConfig(s);
    }

    virtual void initSpace() = 0;
    virtual void initControlSpace() = 0;
    virtual void print() = 0;

    virtual const ob::StateSpacePtr SpacePtr(){
      return space;
    }
    virtual const oc::RealVectorControlSpacePtr ControlSpacePtr(){
      return control_space;
    }
    virtual uint GetDimensionality(){
      return space->getDimension();
    }
    virtual uint GetControlDimensionality(){
      return control_space->getDimension();
    }
    virtual void SetCSpaceInput(const CSpaceInput &input_){
      input = input_;
    }
    virtual Robot* GetRobotPtr(){
      return robot;
    }
    virtual CSpace* GetCSpacePtr(){
      return kspace;
    }

  protected:
    CSpaceInput input;

    uint Nklampt;
    uint Nompl;

    //klampt:
    //
    // SE(3) x R^Nklampt
    //
    //ompl:
    //
    // SE(3) x R^Nompl
    //
    // ompl_to_klampt: maps a dimension in R^Nompl to SE(3)xR^Nklampt
    // klampt_to_ompl: maps a dimension in SE(3)xR^Nklampt to R^Nompl
    std::vector<int> ompl_to_klampt;
    std::vector<int> klampt_to_ompl;

    ob::StateSpacePtr space;
    oc::RealVectorControlSpacePtr control_space;

    Robot *robot;
    CSpace *kspace;
};
//GeometricCSpaceOMPL: Space = Configuration manifold; control space = tangent
//space of configuration manifold, i.e. control happens in velocity space
//
class GeometricCSpaceOMPL: public CSpaceOMPL
{
  public:
    GeometricCSpaceOMPL(Robot *robot_, CSpace *kspace_);

    virtual const oc::StatePropagatorPtr StatePropagatorPtr(oc::SpaceInformationPtr si);
    virtual const ob::StateValidityCheckerPtr StateValidityCheckerPtr(oc::SpaceInformationPtr si);
    virtual const ob::StateValidityCheckerPtr StateValidityCheckerPtr(ob::SpaceInformationPtr si);

    virtual void initSpace();
    virtual void initControlSpace();
    virtual ob::ScopedState<> ConfigToOMPLState(const Config &q);

    virtual Config OMPLStateToConfig(const ob::State *qompl);
    virtual Config OMPLStateToConfig(const ob::SE3StateSpace::StateType *qomplSE3, const ob::RealVectorStateSpace::StateType *qomplRnState);
    virtual void print();

  protected:
    bool hasRealVectorSpace;
};

//KinodynamicCSpaceOMPL: Space = tangent bundle of configuration manifold;
//control space = tangent space of tangent bundle, i.e. control happens in
//acceleration space, i.e. we can control torques of revolute joints, forces
//of prismatic joints, and any additional booster/thruster which act directly
//on the se(3) component

class KinodynamicCSpaceOMPL: public CSpaceOMPL
{
  public:
    KinodynamicCSpaceOMPL(Robot *robot_, CSpace *kspace_);

    virtual const oc::StatePropagatorPtr StatePropagatorPtr(oc::SpaceInformationPtr si);
    virtual const ob::StateValidityCheckerPtr StateValidityCheckerPtr(oc::SpaceInformationPtr si);
    virtual const ob::StateValidityCheckerPtr StateValidityCheckerPtr(ob::SpaceInformationPtr si);

    virtual void initSpace();
    virtual void initControlSpace();

    virtual ob::ScopedState<> ConfigToOMPLState(const Config &q);

    virtual Config OMPLStateToConfig(const ob::State *qompl);

    Config OMPLStateToConfig(const ob::SE3StateSpace::StateType *qomplSE3, const ob::RealVectorStateSpace::StateType *qomplRnState, const ob::RealVectorStateSpace::StateType *qomplTMState);

    virtual void print();
};

class GeometricCSpaceOMPLRotationalInvariance: public GeometricCSpaceOMPL
{
  public:
    GeometricCSpaceOMPLRotationalInvariance(Robot *robot_, CSpace *space_);
    virtual void initSpace();
    virtual void print();
    virtual ob::ScopedState<> ConfigToOMPLState(const Config &q);
    virtual Config OMPLStateToConfig(const ob::State *qompl);
};

class GeometricCSpaceOMPLPathConstraintRollInvariance: public GeometricCSpaceOMPL
{
  public:
    GeometricCSpaceOMPLPathConstraintRollInvariance(Robot *robot_, CSpace *space_, std::vector<Config> path_);
    virtual void initSpace();
    virtual void print();
    virtual ob::ScopedState<> ConfigToOMPLState(const Config &q);
    virtual Config OMPLStateToConfig(const ob::State *qompl);
  private:
    PathPiecewiseLinearEuclidean* path_constraint;
    double t;

};