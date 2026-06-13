/************************************************************/
/*    NAME: Charles Benjamin                               */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: SimSOC.h                                        */
/*    DATE: June 2026                                       */
/************************************************************/

#ifndef SimSOC_HEADER
#define SimSOC_HEADER

#include <string>
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class SimSOC : public AppCastingMOOSApp
{
 public:
   SimSOC();
   ~SimSOC();

 protected: // Standard MOOSApp functions to overload
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();

 protected: // Standard AppCastingMOOSApp function to overload
   bool buildReport();

 protected:
   void registerVariables();
   bool handleMailSolarInputFactor(CMOOSMsg& msg);
   bool handleMailNavSpeed(CMOOSMsg& msg);
   bool handleMailSocCommand(CMOOSMsg& msg);
   bool setConfigDouble(const std::string& param, const std::string& value,
                        const std::string& expected_param, double& target);
   double clampSoc(double value) const;

 private: // Configuration variables
   std::string m_publish_var;
   double      m_start_soc;
   double      m_min_soc;
   double      m_max_soc;
   double      m_battery_capacity_wh;
   double      m_charge_w_base;
   double      m_charge_w_gain;
   double      m_idle_load_w;
   double      m_speed_load_coeff;
   double      m_speed_load_exponent;

 private: // State variables
   double      m_soc;
   double      m_solar_input_factor;
   double      m_nav_speed;
   double      m_last_iter_time;
   double      m_charge_w;
   double      m_load_w;
   double      m_net_w;
   unsigned int m_updates;
};

#endif
