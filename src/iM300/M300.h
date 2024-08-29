/*****************************************************************/
/*    NAME: Michael Benjamin                                     */
/*    ORGN: Dept of Mechanical Engineering, MIT, Cambridge MA    */
/*    FILE: M300.h                                               */
/*    DATE: 01 APRIL 2020                                        */
/*                                                               */
/* This is unreleased BETA code. No permission is granted or     */
/* implied to use, copy, modify, and distribute this software    */
/* except by the author(s), or those designated by the author.   */
/*****************************************************************/

#ifndef M300_HEADER
#define M300_HEADER

#include <string>
#include "SockNinja.h"
#include "Thruster.h"
#include "VehRotController.h"
#include "MOOS/libMOOSGeodesy/MOOSGeodesy.h"
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include "XYRangePulse.h"  // for send pulse

class M300 : public AppCastingMOOSApp
{
public:
  M300();
  ~M300();

protected: // Standard public MOOSApp functions
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();

protected: // Standard protected MOOS App functions
  bool buildReport();
  void registerVariables();

protected: // App Specific functions
  void reportWarningsEvents();
  void sendMessagesToSocket();
  void readMessagesFromSocket();

  bool handleConfigIgnoreMsg(std::string);
  bool handleMsgGPRMC(std::string);
  bool handleMsgGNRMC(std::string);
  bool handleMsgGPGGA(std::string);
  bool handleMsgGNGGA(std::string);
  bool handleMsgCPNVG(std::string);
  bool handleMsgCPNVG_heading(std::string);
  bool handleMsgCPRBS(std::string);
  bool handleMsgCPRCM(std::string);
  bool handleMsgCPNVR(std::string);

  bool reportBadMessage(std::string msg, std::string reason="");
  bool GeodesySetup();
  void checkForStalenessOrAllStop();
  void sendPulse();

  

private: // Config variables
  double       m_max_rudder;       // MAX_RUDDER
  double       m_max_thrust;       // MAX_THRUST
  std::string  m_drive_mode;       // DRIVE_MODE
  std::string  m_vname;            // Vehicle name
  
  // heading source variables
  std::string  m_heading_source;   // gps, imu, or auto;
  double       m_stale_gps_msg_thresh;
  double       m_last_gps_msg_time;
  bool         m_ignore_checksum;

  std::set<std::string> m_ignore_msgs;

  // Requred mods for HydroMAN integration 
  std::string  m_nav_prefix;
  std::string  m_gps_prefix;
  std::string  m_compass_prefix;
  
private: // State variables
  CMOOSGeodesy m_geodesy;
  SockNinja    m_ninja;
  Thruster     m_thrust;
  VehRotController m_rot_ctrl;

  bool         m_ivp_allstop;
  bool         m_moos_manual_override;

  // Stale Message Detection
  bool         m_stale_check_enabled;
  bool         m_stale_mode;
  double       m_stale_threshold;
  unsigned int m_count_stale;
  double       m_tstamp_des_rudder;
  double       m_tstamp_des_thrust;
  double       m_tstamp_compass_msg;

  unsigned int m_num_satellites;
  double       m_batt_voltage;
  double       m_nav_x;
  double       m_nav_y;
  double       m_nav_hdg;
  double       m_nav_spd;
  double       m_compass_hdg;

  unsigned int m_bad_nmea_semantic;

  // GPS denied navigation related
  bool         m_gps_blocked;

  // Adaptive control related
  bool         m_publish_body_vel;
  bool         m_use_nvg_msg_for_nav_x_nav_y;
  double       m_stale_compass_thresh;
  double       m_declination;  // used for processing
                               // raw compass
  bool         m_add_thruster_fault; 
  double       m_fault_factor_thr_L;
  double       m_fault_factor_thr_R;
};

#endif 


