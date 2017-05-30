#include <Control/Controller.h>

class ContactStabilityController: public RobotController
{
public:
  ContactStabilityController(Robot& robot) : RobotController(robot) {}
  virtual ~ContactStabilityController() {}
  virtual const char* Type() const { return "ContactStabilityController"; }
  virtual void Reset() { 
    //put any initialization code here
    RobotController::Reset(); 
  } 
  virtual void Update(Real dt) {
    //We'll put our code here: read from this->sensors, and write to this->command.
    //See Sensor.h and Command.h for details on these structures

    Vector qcmd,vcmd;
    Vector qactual,vactual;
    GetCommandedConfig(qcmd);  //convenience function in RobotController
    GetCommandedVelocity(vcmd);  //convenience function in RobotController
    GetSensedConfig(qactual);  //convenience function in RobotController
    GetSensedVelocity(vactual);  //convenience function in RobotController
    int link = 15;  //the movement link
    if(time >= 1.0 && time < 2.0)
    {
      Real speed = -1.5;
      vcmd[link] = speed;
    }else
    {
      vcmd[link] = 0;
    }
    SetPIDCommand(qcmd,vcmd); //convenience function in RobotController

    RobotController::Update(dt);
  }
};

