/************************************************************/
/*    NAME: Charles Benjamin                                */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: TakeoffGate.cpp                                 */
/*    DATE: August 17th, 2026                               */
/************************************************************/

#include <algorithm>
#include <cmath>
#include "ACTable.h"
#include "MBUtils.h"
#include "TakeoffGate.h"

using namespace std;

TakeoffGate::TakeoffGate()
  : m_min_uav_soc(30.0),
    m_min_pearl_soc(15.0),
    m_max_wind_speed(4.0),
    m_input_max_age(3.0),
    m_request_pending(false),
    m_last_status_post(-1.0),
    m_last_ready(false),
    m_last_reason("INPUTS_UNAVAILABLE"),
    m_requests_approved(0),
    m_requests_rejected(0)
{
}

bool TakeoffGate::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  for(auto &msg : NewMail) {
    const string key = msg.GetKey();
    bool handled = true;

    if(key == "UAV_TAKEOFF_REQUEST") {
      bool requested = false;
      handled = readBool(msg, requested);
      if(handled && requested)
        m_request_pending = true;
    }
    else if(key == "UAV_BATTERY_SOC")
      handled = readDouble(msg, m_uav_soc);
    else if(key == "UAV_BATTERY_DATA_VALID")
      handled = readBoolValue(msg, m_uav_battery_valid);
    else if(key == "PEARL_BATTERY_SOC")
      handled = readDouble(msg, m_pearl_soc);
    else if(key == "PEARL_BATTERY_DATA_VALID")
      handled = readBoolValue(msg, m_pearl_battery_valid);
    else if(key == "PEARL_WIND_SPEED")
      handled = readDouble(msg, m_wind_speed);
    else if(key == "PEARL_WIND_DATA_VALID")
      handled = readBoolValue(msg, m_wind_valid);
    else if(key != "APPCAST_REQ")
      handled = false;

    if(!handled && key != "APPCAST_REQ")
      reportRunWarning("Unhandled or invalid mail: " + key);
  }

  return true;
}

bool TakeoffGate::OnConnectToServer()
{
  registerVariables();
  return true;
}

bool TakeoffGate::Iterate()
{
  AppCastingMOOSApp::Iterate();

  const string reason = gateReason();
  const bool ready = (reason == "READY");

  if(m_request_pending) {
    m_request_pending = false;
    Notify("UAV_TAKEOFF_REQUEST", 0.0);
    Notify("UAV_TAKEOFF_APPROVED", ready ? 1.0 : 0.0);

    if(ready) {
      ++m_requests_approved;
      Notify("UAV_TAKEOFF_RESULT", "status=APPROVED,reason=READY");
      reportEvent("Takeoff approved");
    }
    else {
      ++m_requests_rejected;
      Notify("UAV_TAKEOFF_RESULT", "status=REJECTED,reason=" + reason);
      reportEvent("Takeoff rejected: " + reason);
    }
  }

  if((MOOSTime() - m_last_status_post) >= 1.0 ||
     ready != m_last_ready || reason != m_last_reason) {
    Notify("UAV_TAKEOFF_GATE_READY", ready ? 1.0 : 0.0);
    Notify("UAV_TAKEOFF_GATE_REASON", reason);
    m_last_status_post = MOOSTime();
    m_last_ready = ready;
    m_last_reason = reason;
  }

  AppCastingMOOSApp::PostReport();
  return true;
}

bool TakeoffGate::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  for(const string &orig : sParams) {
    string line = orig;
    const string param = tolower(biteStringX(line, '='));
    const string value = line;
    bool handled = false;

    if(param == "min_uav_soc")
      handled = setPercentConfig(value, m_min_uav_soc);
    else if(param == "min_pearl_soc")
      handled = setPercentConfig(value, m_min_pearl_soc);
    else if(param == "max_wind_speed")
      handled = setNonNegDoubleOnString(m_max_wind_speed, value);
    else if(param == "input_max_age")
      handled = setPosDoubleOnString(m_input_max_age, value);

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();
  return true;
}

void TakeoffGate::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("UAV_TAKEOFF_REQUEST", 0);
  Register("UAV_BATTERY_SOC", 0);
  Register("UAV_BATTERY_DATA_VALID", 0);
  Register("PEARL_BATTERY_SOC", 0);
  Register("PEARL_BATTERY_DATA_VALID", 0);
  Register("PEARL_WIND_SPEED", 0);
  Register("PEARL_WIND_DATA_VALID", 0);
}

bool TakeoffGate::readDouble(const CMOOSMsg &msg, TimedValue &target)
{
  if(!msg.IsDouble() || !std::isfinite(msg.GetDouble()))
    return false;

  target.value = msg.GetDouble();
  target.time = MOOSTime();
  return true;
}

bool TakeoffGate::readBool(const CMOOSMsg &msg, bool &value) const
{
  if(msg.IsDouble()) {
    value = (msg.GetDouble() != 0.0);
    return true;
  }

  if(!msg.IsString())
    return false;

  return setBooleanOnString(value, msg.GetString());
}

bool TakeoffGate::readBoolValue(const CMOOSMsg &msg, TimedValue &target)
{
  bool value = false;
  if(!readBool(msg, value))
    return false;

  target.value = value ? 1.0 : 0.0;
  target.time = MOOSTime();
  return true;
}

bool TakeoffGate::setPercentConfig(const string &value, double &target)
{
  double parsed = 0.0;
  if(!setNonNegDoubleOnString(parsed, value) || parsed > 100.0)
    return false;

  target = parsed;
  return true;
}

bool TakeoffGate::isFresh(const TimedValue &value) const
{
  return value.time >= 0.0 && (MOOSTime() - value.time) <= m_input_max_age;
}

string TakeoffGate::gateReason() const
{
  if(!isFresh(m_uav_soc) || !isFresh(m_uav_battery_valid))
    return "UAV_BATTERY_STALE";
  if(m_uav_battery_valid.value < 0.5)
    return "UAV_BATTERY_INVALID";
  if(m_uav_soc.value < m_min_uav_soc)
    return "UAV_BATTERY_LOW";

  if(!isFresh(m_pearl_soc) || !isFresh(m_pearl_battery_valid))
    return "PEARL_BATTERY_STALE";
  if(m_pearl_battery_valid.value < 0.5)
    return "PEARL_BATTERY_INVALID";
  if(m_pearl_soc.value < m_min_pearl_soc)
    return "PEARL_BATTERY_LOW";

  if(!isFresh(m_wind_speed) || !isFresh(m_wind_valid))
    return "WIND_STALE";
  if(m_wind_valid.value < 0.5)
    return "WIND_INVALID";
  if(m_wind_speed.value < 0.0 || m_wind_speed.value > m_max_wind_speed)
    return "WIND_HIGH";

  return "READY";
}

bool TakeoffGate::buildReport()
{
  m_msgs << "Thresholds" << endl;
  m_msgs << "  UAV SOC:   >= " << doubleToStringX(m_min_uav_soc, 1) << "%" << endl;
  m_msgs << "  PEARL SOC: >= " << doubleToStringX(m_min_pearl_soc, 1) << "%" << endl;
  m_msgs << "  Wind:      <= " << doubleToStringX(m_max_wind_speed, 1) << " m/s" << endl;
  m_msgs << "  Input age: <= " << doubleToStringX(m_input_max_age, 1) << " s" << endl;
  m_msgs << "Gate: " << gateReason() << endl;
  m_msgs << "Requests approved/rejected: " << m_requests_approved
         << "/" << m_requests_rejected << endl << endl;

  ACTable table(4);
  table << "Input | Value | Age | Valid";
  table.addHeaderLines();
  table << "UAV battery" << doubleToStringX(m_uav_soc.value, 1)
        << ageString(m_uav_soc) << boolString(m_uav_battery_valid);
  table << "PEARL battery" << doubleToStringX(m_pearl_soc.value, 1)
        << ageString(m_pearl_soc) << boolString(m_pearl_battery_valid);
  table << "Apparent wind" << doubleToStringX(m_wind_speed.value, 1)
        << ageString(m_wind_speed) << boolString(m_wind_valid);
  m_msgs << table.getFormattedString();
  return true;
}

string TakeoffGate::ageString(const TimedValue &value) const
{
  if(value.time < 0.0)
    return "n/a";
  return doubleToStringX(max(0.0, MOOSTime() - value.time), 1);
}

string TakeoffGate::boolString(const TimedValue &value) const
{
  if(value.time < 0.0)
    return "unknown";
  return value.value >= 0.5 ? "true" : "false";
}
