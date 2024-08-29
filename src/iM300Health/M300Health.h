/************************************************************/
/*    NAME: Tyler                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: M300Health.h                                          */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#ifndef M300Health_HEADER
#define M300Health_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class M300Health : public AppCastingMOOSApp
{
 public:
   M300Health();
   ~M300Health();

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
   double m_stale_time;
   double m_max_heading_delta;
   double m_low_batt_thresh;
   double m_full_batt_thresh;

   double m_time_average_length;
   double m_speed_est_deviation_thresh; 


 private: // State variables
   double m_nav_heading;
   double m_nav_heading_last_msg_time;
   std::string m_nav_heading_aux;
   bool   m_stale_nav;
   
   double m_gps_heading;
   double m_gps_heading_last_msg_time;
   std::string m_gps_heading_aux;
   bool   m_stale_gps;
   
   bool   m_imu_gps_heading_agreement;

   double m_batt_voltage;
   double m_batt_voltage_last_msg_time;
   bool   m_stale_batt_voltage;

   std::vector<double> m_nav_speed_vals;
   std::vector<double> m_nav_speed_times;
   double m_nav_speed_running_ave;

   std::vector<double> m_ave_nav_speed_vals;
   std::vector<double> m_ave_nav_speed_times;
   double m_ave_nav_speed_running_ave;

   double m_deviation_running_ave;
   int m_number_of_faults_detected; 

};

#endif 
