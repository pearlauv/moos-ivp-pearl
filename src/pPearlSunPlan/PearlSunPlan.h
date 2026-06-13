/************************************************************/
/*    NAME: Charles Benjamin                               */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: PearlSunPlan.h                                      */
/*    DATE: June 2026                                      */
/************************************************************/

#ifndef PearlSunPlan_HEADER
#define PearlSunPlan_HEADER

#include <string>
#include <vector>
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

struct PearlSunPlanTask {
  double start_h;
  double cost_wh;
  double duration_h;
  double speed;
  std::string points;
  bool handled;
  bool dispatched;

  PearlSunPlanTask();
};

class PearlSunPlan : public AppCastingMOOSApp
{
 public:
   PearlSunPlan();
   ~PearlSunPlan();

 protected: // Standard MOOSApp functions to overload
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();

 protected: // Standard AppCastingMOOSApp function to overload
   bool buildReport();

 protected:
   void registerVariables();
   bool handleMailSoc(CMOOSMsg& msg);
   bool handleMailTaskComplete(CMOOSMsg& msg);
   bool handleMailForecast(CMOOSMsg& msg);
   bool handleConfigTask(const std::string& value);
   bool handleConfigForecastValues(const std::string& value);
   bool loadForecastFile(const std::string& path);
   double estimateTaskPathMeters(const std::string& points) const;
   double estimateTaskPowerWatts(double speed) const;
   bool setConfigDouble(const std::string& param, const std::string& value,
                        const std::string& expected_param, double& target);
   bool setConfigBool(const std::string& param, const std::string& value,
                      const std::string& expected_param, bool& target);
   double missionHours() const;
   double forecastHour(double hours) const;
   double currentIrradiance(double hours) const;
   double currentSolarFactor(double hours) const;
   double estimateFutureChargeWh(double from_h, double to_h) const;
   bool socIsFresh(double now) const;
   void publishSolarInputs(double hours);
   void publishDecision(const std::string& decision,
                        const std::string& reason);
	   void publishEnergyDiagnostics(double available_wh, double required_wh,
	                                 double future_charge_wh);
	   void completeActiveTask(const std::string& reason);
   void dispatchTask(PearlSunPlanTask& task, const std::string& reason);
   void giveUpTask(PearlSunPlanTask& task, const std::string& reason);
   PearlSunPlanTask* nextUnhandledTask();
   std::string taskUpdateString(const PearlSunPlanTask& task) const;

 private: // Configuration variables
   std::string m_soc_var;
   std::string m_task_complete_var;
   std::string m_task_active_var;
   std::string m_wpt_update_var;
   std::string m_forecast_update_var;
   std::string m_forecast_mode;
   std::string m_forecast_file;
   std::string m_giveup_action;
   double      m_battery_capacity_wh;
   double      m_reserve_soc;
   double      m_soc_stale_sec;
   double      m_forecast_horizon_hours;
   double      m_forecast_start_hour;
   double      m_max_irradiance_w_m2;
   double      m_charge_w_base;
   double      m_charge_w_gain;
   double      m_task_power_idle_w;
   double      m_task_power_speed_coeff;
   double      m_task_power_speed_exponent;
   bool        m_auto_deploy_on_dispatch;
   bool        m_use_future_charge;

 private: // State variables
   std::vector<double> m_forecast_w_m2;
   std::vector<PearlSunPlanTask> m_tasks;
   double      m_mission_start_time;
   double      m_latest_soc;
   double      m_latest_soc_time;
	   double      m_last_irradiance;
	   double      m_last_solar_factor;
	   double      m_last_available_wh;
	   double      m_last_required_wh;
	   double      m_last_future_charge_wh;
	   double      m_active_dispatch_h;
	   double      m_active_duration_h;
	   bool        m_have_soc;
   bool        m_task_active;
   bool        m_gave_up;
   std::string m_last_decision;
   std::string m_last_reason;
   unsigned int m_dispatch_count;
   unsigned int m_skip_count;
};

#endif
