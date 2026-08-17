/************************************************************/
/*    NAME: Charles Benjamin                                */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: TakeoffGate.h                                   */
/*    DATE: August 17th, 2026                               */
/************************************************************/

#ifndef TAKEOFF_GATE_HEADER
#define TAKEOFF_GATE_HEADER

#include <string>
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class TakeoffGate : public AppCastingMOOSApp
{
public:
  TakeoffGate();

protected:
  bool OnNewMail(MOOSMSG_LIST &NewMail) override;
  bool Iterate() override;
  bool OnConnectToServer() override;
  bool OnStartUp() override;
  bool buildReport() override;

private:
  struct TimedValue {
    double value = 0.0;
    double time = -1.0;
  };

  void registerVariables();
  bool readDouble(const CMOOSMsg &msg, TimedValue &target);
  bool readBool(const CMOOSMsg &msg, bool &value) const;
  bool readBoolValue(const CMOOSMsg &msg, TimedValue &target);
  bool setPercentConfig(const std::string &value, double &target);
  bool isFresh(const TimedValue &value) const;
  std::string gateReason() const;
  std::string ageString(const TimedValue &value) const;
  std::string boolString(const TimedValue &value) const;

private:
  double m_min_uav_soc;
  double m_min_pearl_soc;
  double m_max_wind_speed;
  double m_input_max_age;

  TimedValue m_uav_soc;
  TimedValue m_uav_battery_valid;
  TimedValue m_pearl_soc;
  TimedValue m_pearl_battery_valid;
  TimedValue m_wind_speed;
  TimedValue m_wind_valid;

  bool m_request_pending;
  double m_last_status_post;
  bool m_last_ready;
  std::string m_last_reason;
  unsigned int m_requests_approved;
  unsigned int m_requests_rejected;
};

#endif
