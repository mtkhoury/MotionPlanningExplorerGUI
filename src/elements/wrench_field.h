#pragma once
#include <KrisLibrary/math3d/primitives.h>
#include <KrisLibrary/utils/SmartPointer.h>
#include <vector>
#include <memory>
#include <iostream>
#include "elements/force_field.h"

class TiXmlElement;

//a wrench (force/torque) \in \R^6 acting at a point on \R^3
struct Wrench{
  Math3D::Vector3 torque;
  Math3D::Vector3 force;
  Math3D::Vector3 position;
};
struct Momentum{
  Math3D::Vector3 linear;
  Math3D::Vector3 angular;
};
struct COM{
  Wrench wrench;
  Momentum momentum;
};

class WrenchField{

  private:
    std::vector<SmartPointer<ForceField> > forcefields;

    //wrench induced by force field on each link of the main robot
    std::vector<Wrench> wrench_per_link;

    //Wrench wrench_at_com;

    COM com;

  public: 

    void init(uint Nlinks);
    WrenchField();

    bool LoadFromWorldFile( const char * file);
    bool Load( TiXmlElement *node );

    //force at position
    Math3D::Vector3 getForceFieldAtPosition(Math3D::Vector3 &position);
    //TODO: force at origin induced by force at position 
    Math3D::Vector3 getTorqueFieldAtPosition(Math3D::Vector3 &position, Math3D::Vector3 &origin);

    const std::vector<SmartPointer<ForceField> >& GetForceFields() const;

    uint size();

    void setForce( uint id, Math3D::Vector3 force);
    void setTorque( uint id, Math3D::Vector3 torque);
    void setPosition( uint id, Math3D::Vector3 position);
    Math3D::Vector3 getForce( uint id );
    Math3D::Vector3 getTorque( uint id );
    Math3D::Vector3 getPosition( uint id );

    void setCOMLinearMomentum( Math3D::Vector3 linearmomentum);
    void setCOMAngularMomentum( Math3D::Vector3 angularmomentum);
    void setCOMForce( Math3D::Vector3 force);
    void setCOMTorque( Math3D::Vector3 torque);
    void setCOMPosition( Math3D::Vector3 position);

    Math3D::Vector3 getCOMLinearMomentum();
    Math3D::Vector3 getCOMAngularMomentum();
    Math3D::Vector3 getCOMForce();
    Math3D::Vector3 getCOMTorque();
    Math3D::Vector3 getCOMPosition();

    void print();
};

