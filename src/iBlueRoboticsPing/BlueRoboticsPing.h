/************************************************************/
/*    NAME:                                               */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: BlueRoboticsPing.h                                          */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#ifndef BlueRoboticsPing_HEADER
#define BlueRoboticsPing_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
//#include "/home/student2680/ping-cpp/src/device/ping-device-ping1d.h"
#include "ping-device-ping1d.h"
#include "abstract-link/abstract-link.h"

class BlueRoboticsPing : public AppCastingMOOSApp
{
 public:
   BlueRoboticsPing();
   ~BlueRoboticsPing(){};

 protected: // Standard MOOSApp functions to overload  
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();

 protected: // Standard AppCastingMOOSApp function to overload 
   bool buildReport();

 protected:
   void registerVariables();

 private: // Configuration variables
  
   int m_ping_interval = 100; // ms
   int m_speed_of_sound = 1500000; // water
   bool m_profile = false;
   std::string m_profile_str = "";
   uint32_t m_scan_start = 10;
   uint32_t m_scan_length = 1250;
   int m_auto = 1;

 private: // State variables
   //Ping1d m_device;
   uint32_t m_distance = -1;
   uint16_t m_confidence = -1;
   //PingPort m_port = AbstractLink::openUrl("serial:/dev/ttyUSB0:115200");
};

#endif 
