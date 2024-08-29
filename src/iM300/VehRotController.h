/*****************************************************************/
/*    NAME: Tyler Paine                                          */
/*    ORGN: Dept of Mechanical Engineering, MIT, Cambridge MA    */
/*    FILE: RotateController.h                                   */
/*    DATE: Sept 9th  2021                                       */
/*                                                               */
/* This is unreleased BETA code. No permission is granted or     */
/* implied to use, copy, modify, and distribute this software    */
/* except by the author(s), or those designated by the author.   */
/*****************************************************************/

#ifndef VEH_ROTATE_CONTROLLER_HEADER
#define VEH_ROTATE_CONTROLLER_HEADER

#include <string>
#include "MBUtils.h"
#include <math.h>
#include <stdlib.h>


class VehRotController
{
public:
  VehRotController();
  ~VehRotController() {};

  // SET functions
  bool setRotateInPlace(bool val) {m_rotate_in_place = val; return(true); }
  bool setHeadingTarget(double val) {m_heading_target = val; return(true); }
  bool setCmdTimeStamp(double val) {m_cmd_tstamp = val; return(true); }
  bool setStartRotX(double val) {m_nav_x_start_rot = val; return(true); }
  bool setStartRotY(double val) {m_nav_y_start_rot = val; return(true); }
  bool setRotateRadius(double val) {m_rotate_radius = val; return(true); }
  bool setMaxTimeLag(double val) {m_max_time_lag = val; return(true); }
  bool setDeadZone(double val) {m_dead_zone = val; return(true); }

  // GET functions
  double getHeadingTarget() {return m_heading_target;}
  bool   getRotateInPlace() {return m_rotate_in_place;}
							 
  
  bool handlePoint(std::string sval);

  bool checkClearToRotate(double nav_x, double nav_y, double curr_time);
  bool calControl(double nav_heading, double nav_x, double nav_y, double &thrust, double &rudder);
  bool checkRotateFinished(double nav_heading);
  
  
 protected:
  int calcHeadingDiff(double curr_heading, double target_heading);
  
  bool         m_rotate_in_place;
  double       m_heading_target;
  double       m_cmd_tstamp;
  double       m_nav_x_start_rot;
  double       m_nav_y_start_rot;
  double       m_rotate_radius;

  double       m_max_time_lag;
  double       m_dead_zone;

  
  
};

#endif 


