/************************************************************/
/*    NAME:                                               */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: BlueRoboticsPing.h                                          */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#ifndef BlueRoboticsPing_HEADER
#define BlueRoboticsPing_HEADER

#include <cstdint>
#include <string>

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
// Old direct Ping1D serial includes, kept for easy manual rollback:
// #include "/home/student2680/ping-cpp/src/device/ping-device-ping1d.h"
// #include "ping-device-ping1d.h"
// #include "abstract-link/abstract-link.h"

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
   std::string m_state_url = "http://127.0.0.1:9324/state";
   uint32_t m_scan_start = 10;
   uint32_t m_scan_length = 1250;
   int m_auto = 1;

 private: // State variables
   //Ping1d m_device;
   bool FetchState();
   bool ParseStateBody(const std::string& body);

   double m_distance_mm = -1;
   double m_distance_meters = -1;
   double m_distance_feet = -1;
   double m_confidence = -1;
   double m_sample_age_seconds = -1;
   int m_connected = 0;
   //PingPort m_port = AbstractLink::openUrl("serial:/dev/ttyUSB0:115200");
};

#endif 
