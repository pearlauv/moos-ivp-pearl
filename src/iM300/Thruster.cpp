/*****************************************************************/
/*    NAME: Michael Benjamin                                     */
/*    ORGN: Dept of Mechanical Engineering, MIT, Cambridge MA    */
/*    FILE: thruster.cpp                                         */
/*    DATE: 11 APRIL 2020                                        */
/*                                                               */
/* This is unreleased BETA code. No permission is granted or     */
/* implied to use, copy, modify, and distribute this software    */
/* except by the author(s), or those designated by the author.   */
/*****************************************************************/

#include "Thruster.h"
#include "MBUtils.h"

using namespace std;

Thruster::Thruster()
{
  m_des_thrL    = 0.0;
  m_des_thrR    = 0.0;
  m_des_thr     = 0.0;
  m_des_rud     = 0.0;
  
  m_max_rud     = 30.0;      // defualt MAX_RUDDER (+/-)
  m_max_thr     = 100.0;     // default MAX_THRUST (+/-)
  m_drive_mode  = "normal";  // default DRIVE_MODE ("normal"|"aggro")

  m_max_list_size = 100;
}

Thruster::~Thruster() {};

//---------------------------------------------------------
// Procedure: setDriveMode()

bool Thruster::setDriveMode(string drive_mode)
{
  string msg;
  msg = tolower(drive_mode);
  
  if( msg == "normal" || msg == "aggro" || msg == "rotate" || msg == "direct"){
    m_drive_mode = msg;
    return(true);
  }
  else{
    msg = "DRIVE_MODE not recognized. ";
    msg += "Valid modes are 'NORMAL' or 'AGGRO' or 'ROTATE' or 'DIRECT'. Check .MOOS file.";
    addWarning(msg);
    return(false);
  }
}

//---------------------------------------------------------
// Procedure: setMaxRudder()
bool Thruster::setMaxRudder(string max_rud)
{
  string msg;
  double dmax_rud;
  if(setDoubleOnString(dmax_rud, max_rud)){
    return(setMaxRudder(dmax_rud));
  }
  else{
    msg = "MAX_RUDDER is not a number. Check .MOOS file.";
    addWarning(msg);
    return(false);
  }
}

bool Thruster::setMaxRudder(double max_rud)
{
  string msg;
  if(max_rud > 0 && max_rud <= 180){
    m_max_rud = max_rud;
    return(true);
  }
  else{
    msg = "MAX_RUDDER out of range (0,180].  Check .MOOS file.";
    addWarning(msg);
    return(false);
  }
}

//---------------------------------------------------------
// Procedure: setMaxThrust()

bool Thruster::setMaxThrust(string max_thr)
{
  string msg;
  double dmax_thr;
  if(setDoubleOnString(dmax_thr, max_thr)){
    return(setMaxThrust(dmax_thr));
  }
  else{
    msg = "MAX_THRUST is not a number. Check .MOOS file.";
    addWarning(msg);
    return(false);
  }
}

bool Thruster::setMaxThrust(double max_thr)
{
  string msg;
  if(max_thr >= 0 && max_thr <= 100){
    m_max_thr = max_thr;
    return(true);
  }
  else{
    msg = "MAX_THRUST out of range (0,100].  Check .MOOS file.";
    addWarning(msg);
    return(false);
  }
}

//---------------------------------------------------------
// Procedure: setRudder()

bool Thruster::setRudder(string des_rud)
{
  string msg;
  double ddes_rud;
  if(setDoubleOnString(ddes_rud, des_rud)){
    return(setRudder(ddes_rud));
  }
  else{
    msg = "DESIRED_RUDDER is not a number.";
    addWarning(msg);
    return(false);
  }
}

bool Thruster::setRudder(double des_rud)
{
  string msg;
  if(des_rud >= -m_max_rud && des_rud <= m_max_rud){
    m_des_rud = des_rud;
  }
  else{
    m_des_rud = vclip(des_rud, -m_max_rud, m_max_rud);
    //msg = "DESIRED_RUDDER out of range, clipped at +/- MAX_RUDDER.";
    //addWarning(msg);
  }

  calcDiffThrust();
  return(true);
}


//---------------------------------------------------------
// Procedure: setThrust()

bool Thruster::setThrust(string des_thr)
{
  string msg;
  double ddes_thr;
  if(setDoubleOnString(ddes_thr, des_thr)){
    return(setThrust(ddes_thr));
  }
  else{
    msg = "DESIRED_THRUST is not a number.";
    addWarning(msg);
    return(false);
  }
}

bool Thruster::setThrust(double des_thr)
{
  string msg;
  if(des_thr >= -m_max_thr && des_thr <= m_max_thr){
    m_des_thr = des_thr;
  }
  else{
    m_des_thr = vclip(des_thr, -m_max_thr, m_max_thr);
    //msg = "DESIRED_THRUST out of range, clipped at +/- MAX_THRUST.";
    //addWarning(msg);
  }

  calcDiffThrust();
  return(true);
}


//---------------------------------------------------------
// Procedure: addWarning()

void Thruster::addWarning(string warning)
{
  m_warnings.push_front(warning);
  if(m_warnings.size() > m_max_list_size)
    m_warnings.pop_back();
}


//---------------------------------------------------------
// Procedure: calcDiffThrust()

bool Thruster::calcDiffThrust()
{
  double delta, upper_lim, lower_lim;

  // NORMAL MODE:
  if(m_drive_mode == "normal"){
    upper_lim = 100;
    lower_lim = 0;
    delta = (m_des_thr/m_max_rud)*m_des_rud;
    m_des_thrL = m_des_thr + delta;
    m_des_thrR = m_des_thr - delta;

    // Clip saturated values for each thruster:
    m_des_thrL = vclip(m_des_thrL, lower_lim, upper_lim);
    m_des_thrR = vclip(m_des_thrR, lower_lim, upper_lim);
  }

  // AGGRO MODE:
  else if(m_drive_mode == "aggro"){
    double max_revthrust, rev_factor, max_delta;
    upper_lim = 100;
    lower_lim = -100;
    rev_factor = 0.2;
    max_revthrust = -rev_factor*m_max_thr;
    max_delta = m_max_thr - max_revthrust;
    delta = (max_delta/m_max_rud)*m_des_rud;
    m_des_thrL = m_des_thr + (delta/2);
    m_des_thrR = m_des_thr - (delta/2);

    // Rebalance saturated values, preserving delta:
    if(m_des_thrL > upper_lim){
      double overL = m_des_thrL - upper_lim;
      m_des_thrL = upper_lim;
      m_des_thrR -= overL;
      if (m_des_thrR < 0)  // rescale negative thrust
        m_des_thrR *= -1/rev_factor;
    }
    else if(m_des_thrR > upper_lim){
      double overR = m_des_thrR - upper_lim;
      m_des_thrR = upper_lim;
      m_des_thrL -= overR;
      if (m_des_thrL < 0)  // rescale negative thrust
        m_des_thrL *= -1/rev_factor;
    }
    else if(m_des_thrL < max_revthrust){
      double underL = max_revthrust - m_des_thrL;
      m_des_thrL = lower_lim;
      m_des_thrR += underL;
    }
    else if(m_des_thrR < max_revthrust){
      double underR = max_revthrust - m_des_thrR;
      m_des_thrR = lower_lim;
      m_des_thrL += underR;
    }
  }
  
  // ROTATE MODE
  else if (m_drive_mode == "rotate"){
    // Experimental for now...
    double max_fwd = 0.3 * m_max_thr;
    double max_rev = -1.0 * m_max_thr;
    double scale = fabs( m_des_rud / m_max_rud );  // only mag

    if(m_des_rud > 0.0) {
      // turn right (clockwise)
      m_des_thrL = max_fwd * scale;
      m_des_thrR = max_rev * scale;

    } else {
      // turn left (ccw)
      m_des_thrL = max_rev * scale;
      m_des_thrR = max_fwd * scale;
    }
  }

  // DIRECT DRIVE MODE
  else if (m_drive_mode == "direct") {
    // Check lims (again)
    upper_lim = 100;
    lower_lim = -100;
    // Clip saturated values for each thruster:
    m_des_thrL = vclip(m_des_thrL, lower_lim, upper_lim);
    m_des_thrR = vclip(m_des_thrR, lower_lim, upper_lim);

  }
  
  return(true);
}




//---------------------------------------------------------
// Procedure: setThrusterSpeed()
//            This is used to set the specific thruster speed
//            directly without any thruster mapping.
//            Mode must be direct
//            Possible thruster sides are left and right

bool Thruster::setThrusterSpeed(std::string des_thrX, std::string thr_side)
{

  string msg;
  if (!(m_drive_mode == "direct")){
    msg = "Not in direct drive mode: Not allowed to set thruster speed directly";
    addWarning(msg);
    return(false);
  }

  if (!( (thr_side == "right") || (thr_side == "left") )) {
    msg = "Thruster side specified was not left or right";
    addWarning(msg);
    return(false);
  }
  
  double ddes_thrX;
  if(setDoubleOnString(ddes_thrX, des_thrX)){
    return(setThrusterSpeed(ddes_thrX, thr_side));
  }
  else{
    msg = "Thruster value specified in direct drive mode is not a number.";   
    addWarning(msg); 
    return(false);
  }
}

bool Thruster::setThrusterSpeed(double des_thrX, std::string thr_side)
{

  // Keep error checking here in case this public function is called
  // directly.
  string msg;
  if (!(m_drive_mode == "direct")){
    msg = "Not in direct drive mode: Not allowed to set thruster speed directly";
    addWarning(msg);
    return(false);
  }

  if (!( (thr_side == "right") || (thr_side == "left") )){
    msg = "Thruster side specified was not left or right";
    addWarning(msg);
    return(false);
  }
  
  // Set the thrust accordingly
  double checked_des_thrX = 0;
  if(des_thrX >= -m_max_thr && des_thrX <= m_max_thr){
    checked_des_thrX = des_thrX;
  }
  else{
    checked_des_thrX = vclip(des_thrX, -m_max_thr, m_max_thr);
  }

  if (thr_side == "right") {
    m_des_thrR = checked_des_thrX;
  } else { // will only be "left" if here
    m_des_thrL = checked_des_thrX;
  }

  calcDiffThrust();
  return(true);
}
