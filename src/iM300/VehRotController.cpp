/*****************************************************************/
/*    NAME: Tyler Paine                                          */
/*    ORGN: Dept of Mechanical Engineering, MIT, Cambridge MA    */
/*    FILE: RotateController.cpp                                 */
/*    DATE: Sept 9th  2021                                       */
/*                                                               */
/* This is unreleased BETA code. No permission is granted or     */
/* implied to use, copy, modify, and distribute this software    */
/* except by the author(s), or those designated by the author.   */
/*****************************************************************/

#include "VehRotController.h"
using namespace std;

//---------------------------------------------------------
// Constructor

VehRotController::VehRotController()
{
  m_rotate_in_place = false;
  m_heading_target = 0.0;
  m_cmd_tstamp = 0.0;
  m_nav_x_start_rot = 0.0;
  m_nav_y_start_rot = 0.0;
  m_rotate_radius = 5.0; // Meters

  m_max_time_lag = 15.0;  // Seconds
  m_dead_zone = 5.0; // degrees (plus or minus)
}


//--------------------------------------------------------
// Procedure:  handlePoint(string sval)
//             Processes an incoming string which contains
//             the x,y coordinates to point the vessel towards.

bool VehRotController::handlePoint(std::string sval)
{
  string points = tolower(sval);
  string param_name = biteStringX(points, '=');
  if (param_name != "point")
    return(false);
  
  string x = biteStringX(points, ',');
  string y = points;

  double dlb_x, dlb_y;
  bool ok1 = setDoubleOnString(dlb_x, x);
  bool ok2 = setDoubleOnString(dlb_y, y);
  double calc_heading = atan2( (dlb_x - m_nav_x_start_rot) , (dlb_y - m_nav_y_start_rot) );
  m_heading_target = calc_heading * 180.0 / M_PI;

  if (m_heading_target < 0.0)
    m_heading_target = m_heading_target + 360.0;
  

  return( (ok1 && ok2) );
}



//--------------------------------------------------------
// Procedure:  checkClearToRotate(double nav_x, double nav_y, double curr_time )
//             Checks time and position of vehicle

bool VehRotController::checkClearToRotate(double nav_x, double nav_y, double curr_time)
{

  // First check the bool flag
  if(!m_rotate_in_place)
    return(false);
  
  // Second, check time
  double lag_rotate = curr_time - m_cmd_tstamp;
  bool stale_rotate = (lag_rotate > m_max_time_lag);

  if(stale_rotate)
    return(false);
  
  // Thirdly, check that we have not wandered outside of our radius
  double dx = nav_x - m_nav_x_start_rot;
  double dy = nav_y - m_nav_y_start_rot;
  double dist = sqrt( dx * dx + dy * dy );

  if (dist >= m_rotate_radius)
    return(false);

  // Otherwise, we are good to rotate
  return(true);

}


//--------------------------------------------------------------
//   Procedure:  calControl
//               Calculates control for rotation using ownship heading,
//               nav_x, and nav_y, and outputs a desired thrust and rudder
//               command (by reference).  A simple controller is implemented
//               for now, but the framework is here for more sophisticated
//               controllers in the future.
//               The thrust and rudder outputs are scaled from [-1.0, 1.0]
//               (inclusive) and are intended to be scaled to the appropriate
//               min and max values that correspond to a specific vehicle.
//               This scaling happens elsewhere - not in this class. 

bool VehRotController::calControl(double nav_heading, double nav_x, double nav_y, double &thrust, double &rudder)
{
   
  int heading_diff = calcHeadingDiff(nav_heading, m_heading_target);
  int dead_zone = (int) round(m_dead_zone);
  
  // Simple bang-bang controller with deadband
  // Could also easily implement a proportional controller
  // with .setRudder( ( (double) heading_diff )/180.0) or something similar.

  if (abs(heading_diff) < dead_zone) {
    // do nothing
    rudder = 0.0;
    thrust = 0.0;
  
  } else if  (heading_diff > 0 ) {
    // turn right (clockwise)
    rudder = -1.0;
    thrust = 0.0;
  } else  {
    // turn left (ccw)
    rudder = 1.0;
    thrust = 0.0;
  }
  return(true);
}


//-----------------------------------------------------------
// Procedure: checkRotateFinished(double nav_heading)
//            checks whether heading in within the dead_zone
//            maybe include some other things. 


bool VehRotController::checkRotateFinished(double nav_heading)
{
  
  int heading_diff = calcHeadingDiff(nav_heading, m_heading_target);
  int dead_zone = (int) round(m_dead_zone);
  
  // Might want to check other things in the future. 
  if (abs(heading_diff) < dead_zone)
    return(true);
  else 
    return(false);

}




//----------------------------------------------------------
//  Procedure:   calcHeadingDiff(double curr_heading, double target_heading)
//               returns the difference in two angles along the compass rose
//               (clockwise = positive, counter-clockwise = negative)
//               returns an int

int VehRotController::calcHeadingDiff(double curr_heading, double target_heading)
{

  int curr_heading_int = (int) round(curr_heading);
  int target_heading_int = (int) round(target_heading);
  int heading_diff = ( ( ( curr_heading_int - target_heading_int) + 540) % 360 ) - 180;

  return(heading_diff);
}

