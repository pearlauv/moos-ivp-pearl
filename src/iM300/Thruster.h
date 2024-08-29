/*****************************************************************/
/*    NAME: Michael Benjamin                                     */
/*    ORGN: Dept of Mechanical Engineering, MIT, Cambridge MA    */
/*    FILE: thruster.h                                           */
/*    DATE: 11 APRIL 2020                                        */
/*                                                               */
/* This is unreleased BETA code. No permission is granted or     */
/* implied to use, copy, modify, and distribute this software    */
/* except by the author(s), or those designated by the author.   */
/*****************************************************************/

#ifndef THRUSTER_HEADER
#define THRUSTER_HEADER

#include <string>
#include <list>
#include <math.h>   // for fabs

class Thruster
{
public:
  Thruster();
  ~Thruster();

  // SET functions:
  bool  setDriveMode(std::string drive_mode);
  bool  setMaxRudder(std::string max_rud);
  bool  setMaxRudder(double max_rud);
  bool  setMaxThrust(std::string max_thr);
  bool  setMaxThrust(double max_thr);
  bool  setRudder(std::string des_rud);
  bool  setRudder(double des_rud);
  bool  setThrust(std::string des_thr);
  bool  setThrust(double des_thr);
  bool  setThrusterSpeed(std::string des_thrX, std::string thr_side);
  bool  setThrusterSpeed(double des_thrX, std::string thr_side);
    
  // GET functions:
  double  getRudder()       {return m_des_rud;};
  double  getThrust()       {return m_des_thr;};
  double  getThrustLeft()   {return m_des_thrL;};
  double  getThrustRight()  {return m_des_thrR;};
  std::string getDriveMode() {return m_drive_mode;}; 

  std::list<std::string>  getWarnings()  {return m_warnings;};

  bool  calcDiffThrust();  
protected:
  void  addWarning(std::string warning);


private:
  double       m_max_rud;
  double       m_max_thr;
  double       m_des_thrL;
  double       m_des_thrR;
  double       m_des_thr;
  double       m_des_rud;
  std::string  m_drive_mode;

  unsigned int            m_max_list_size;
  std::list<std::string>  m_warnings;
};

#endif


