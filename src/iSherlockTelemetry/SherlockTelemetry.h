/************************************************************/
/*    NAME: Charles Benjamin                                */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: SherlockTelemetry.h                             */
/*    DATE: August 17th, 2026                               */
/************************************************************/

#ifndef SHERLOCK_TELEMETRY_HEADER
#define SHERLOCK_TELEMETRY_HEADER

#include <string>

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class SherlockTelemetry : public AppCastingMOOSApp
{
 public:
  SherlockTelemetry();
  ~SherlockTelemetry() {}

 protected:
  bool OnNewMail(MOOSMSG_LIST& NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();
  bool buildReport();
  void registerVariables();

 private:
  bool fetchMetrics(std::string& body, std::string& error) const;
  bool parseMetrics(const std::string& body, std::string& error);
  void publishTelemetry();
  void setFetchWarning(bool active);

 private: // Configuration
  std::string m_metrics_host;
  std::string m_metrics_port;
  std::string m_metrics_path;
  double m_poll_interval;
  double m_http_timeout;
  double m_battery_max_age;
  double m_wind_max_age;

 private: // State
  double m_last_poll_time;
  double m_last_success_time;
  double m_battery_soc;
  double m_battery_charging;
  double m_battery_source_age;
  double m_wind_speed;
  double m_true_wind_speed;
  double m_wind_source_age;
  bool m_battery_connected;
  bool m_wind_source_valid;
  bool m_true_wind_set;
  bool m_fetch_warning_active;
  std::string m_last_error;
  unsigned int m_fetch_count;
  unsigned int m_success_count;
};

#endif
