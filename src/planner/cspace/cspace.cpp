#include "planner/cspace/cspace.h"
#include "planner/validitychecker/validity_checker_ompl.h"
#include <ompl/base/spaces/SO2StateSpace.h>

Vector3 CSpaceOMPL::getXYZ(const ob::State *s){
  double x = 0;
  double y = 0;
  double z = 0;

  ob::StateSpacePtr space_first_subspace;

  //extract first component subspace
  if(!space->isCompound()){
    space_first_subspace = space;
  }else{
    ob::CompoundStateSpace *M1_compound = space->as<ob::CompoundStateSpace>();
    const std::vector<ob::StateSpacePtr> decomposed = M1_compound->getSubspaces();
    space_first_subspace = decomposed.front();
  }

  if(space_first_subspace->getType() == ob::STATE_SPACE_SE3){
    const ob::SE3StateSpace::StateType *qomplSE3;
    if(space->isCompound()){
      qomplSE3 = s->as<ob::CompoundState>()->as<ob::SE3StateSpace::StateType>(0);
    }else{
      qomplSE3 = s->as<ob::SE3StateSpace::StateType>();
    }
    x = qomplSE3->getX();
    y = qomplSE3->getY();
    z = qomplSE3->getZ();
  }else if(space_first_subspace->getType() == ob::STATE_SPACE_REAL_VECTOR){
    //fixed base robot: visualize last link

    Config q = OMPLStateToConfig(s);
    robot->UpdateConfig(q);
    robot->UpdateGeometry();
    Vector3 qq;
    Vector3 zero; zero.setZero();
    int lastLink = robot->links.size()-1;
    robot->GetWorldPosition(zero, lastLink, qq);

    x = qq[0];
    y = qq[1];
    z = qq[2];

  }else{
    std::cout << "cannot deal with space type" << space_first_subspace->getType() << std::endl;
    std::cout << "please check ompl/base/StateSpaceTypes.h" << std::endl;
    exit(0);
  }
  Vector3 q(x,y,z);
  return q;
}

std::vector<double> CSpaceOMPL::EulerXYZFromOMPLSO3StateSpace( const ob::SO3StateSpace::StateType *q ){
  double qx = q->x;
  double qy = q->y;
  double qz = q->z;
  double qw = q->w;

  double d = sqrt(qx*qx + qy*qy + qz*qz + qw*qw);
  if(abs(d-1.0) > 1e-10){
    //scale them down
    //if(d<0){
    //  d*=-1;
    //}
    qx /= d;
    qy /= d;
    qz /= d;
    qw /= d;
  }

  Math3D::QuaternionRotation qr(qw, qx, qy, qz);
  Math3D::Matrix3 qrM;
  qr.getMatrix(qrM);
  Math3D::EulerAngleRotation R;
  bool Rvalid = R.setMatrixXYZ(qrM);
  if(!Rvalid){

    si->printSettings();

    std::cout << "quaternions: " << qr << std::endl;
    std::cout << qrM << std::endl;
    Real b=Asin(qrM(0,2));  //m(0,2)=sb
    Real cb = Cos(b);
    Real ca = qrM(2,2)/cb;   //m(2,2)=ca*cb
    std::cout << ca << std::endl;

    std::cout << "QuaternionRotation to EulerAngle not valid" << std::endl;
    exit(0);

  }

  double rx = R[2];
  double ry = R[1];
  double rz = R[0];

  if(rx<-M_PI) rx+=2*M_PI;
  if(rx>M_PI) rx-=2*M_PI;

  if(ry<-M_PI/2) ry+=M_PI;
  if(ry>M_PI/2) ry-=M_PI;

  if(rz<-M_PI) rz+=2*M_PI;
  if(rz>M_PI) rz-=2*M_PI;

  std::vector<double> out;
  out.push_back(rx);
  out.push_back(ry);
  out.push_back(rz);
  return out;
}
void CSpaceOMPL::OMPLSO3StateSpaceFromEulerXYZ( double x, double y, double z, ob::SO3StateSpace::StateType *q ){
  q->setIdentity();

  //q SE3: X Y Z yaw pitch roll
  //double yaw = q[3];
  //double pitch = q[4];
  //double roll = q[5];

  //Math3D::EulerAngleRotation Reuler(q(5),q(4),q(3));
  //Matrix3 R;
  //Reuler.getMatrixXYZ(R);
  Math3D::EulerAngleRotation Reuler(z,y,x);
  Matrix3 R;
  Reuler.getMatrixXYZ(R);

  QuaternionRotation qr;
  qr.setMatrix(R);

  double qx,qy,qz,qw;
  qr.get(qw,qx,qy,qz);

  q->x = qx;
  q->y = qy;
  q->z = qz;
  q->w = qw;
}


CSpaceOMPL::CSpaceOMPL(RobotWorld *world_, int robot_idx):
  si(nullptr), world(world_)
{
  robot = world->robots[robot_idx];
  worldsettings.InitializeDefault(*world);
  kspace = new SingleRobotCSpace(*world,robot_idx,&worldsettings);
}

CSpaceOMPL::CSpaceOMPL(Robot *robot_, CSpace *kspace_):
  robot(robot_), kspace(kspace_)
{
  if(!(robot->joints[0].type==RobotJoint::Floating))
  {
    std::cout << "[MotionPlanner] only supports robots with a configuration space equal to SE(3) x R^n" << std::endl;
    exit(0);
  }
}
Config CSpaceOMPL::OMPLStateToConfig(const ob::ScopedState<> &qompl){
  const ob::State* s = qompl.get();
  return OMPLStateToConfig(s);
}

const ob::StateSpacePtr CSpaceOMPL::SpacePtr(){
  return space;
}

ob::SpaceInformationPtr CSpaceOMPL::SpaceInformationPtr(){
  if(si==nullptr){
    si = std::make_shared<ob::SpaceInformation>(SpacePtr());
    const ob::StateValidityCheckerPtr checker = StateValidityCheckerPtr();
    si->setStateValidityChecker(checker);
  }
  return si;
}

const oc::RealVectorControlSpacePtr CSpaceOMPL::ControlSpacePtr(){
  return control_space;
}
uint CSpaceOMPL::GetDimensionality() const{
  return space->getDimension();
}
uint CSpaceOMPL::GetControlDimensionality() const{
  return control_space->getDimension();
}
void CSpaceOMPL::SetCSpaceInput(const CSpaceInput &input_){
  input = input_;
}
Robot* CSpaceOMPL::GetRobotPtr(){
  return robot;
}
CSpace* CSpaceOMPL::GetCSpacePtr(){
  return kspace;
}
const ob::StateValidityCheckerPtr CSpaceOMPL::StateValidityCheckerPtr()
{
  return StateValidityCheckerPtr(SpaceInformationPtr());
}

void CSpaceOMPL::print(std::ostream& out) const
{
  out << std::string(80, '-') << std::endl;
  out << "[ConfigurationSpace]" << std::endl;
  out << std::string(80, '-') << std::endl;
  std::cout << "Robot                    : " << robot->name << std::endl;
  std::cout << "Dimensionality (OMPL)    : " << GetDimensionality() << std::endl;
  std::cout << "Dimensionality (Klampt)  : " << robot->q.size() << std::endl;
  //space.space->printSettings(std::cout);
  std::cout << "OMPL Representation      : " << std::endl;
  space->diagram(std::cout << "   ");

  out << std::string(80, '-') << std::endl;
}
std::ostream& operator<< (std::ostream& out, const CSpaceOMPL& space) 
{
  space.print(out);
  return out;
}
