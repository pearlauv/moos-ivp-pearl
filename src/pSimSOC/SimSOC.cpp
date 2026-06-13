/************************************************************/
/*    NAME: Charles Benjamin                               */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: SimSOC.cpp                                      */
/*    DATE: June 2026                                       */
/************************************************************/

#include <iterator>
#include <cmath>
#include <cstdlib>
#include "MBUtils.h"
#include "ACTable.h"
#include "SimSOC.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

SimSOC::SimSOC()
{
  m_publish_var = "BATT_SOC";
  m_start_soc = 70.0;
  m_min_soc = 0.0;
  m_max_soc = 100.0;
  m_battery_capacity_wh = 400.0;
  m_charge_w_base = 0.0;
  m_charge_w_gain = 90.0;
  m_idle_load_w = 5.01;
  m_speed_load_coeff = 40.5;
  m_speed_load_exponent = 2.97;

  m_soc = m_start_soc;
  m_solar_input_factor = 0.0;
  m_nav_speed = 0.0;
  m_last_iter_time = 0.0;
  m_charge_w = 0.0;
  m_load_w = 0.0;
  m_net_w = 0.0;
  m_updates = 0;
}

//---------------------------------------------------------
// Destructor

SimSOC::~SimSOC()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool SimSOC::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();

#if 0 // Keep these around just for template
    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString();
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif

    bool handled = false;
    if(key == "SOLAR_INPUT_FACTOR")
      handled = handleMailSolarInputFactor(msg);
    else if(key == "NAV_SPEED")
      handled = handleMailNavSpeed(msg);
    else if(key == "SOC_COMMAND")
      handled = handleMailSocCommand(msg);
    else if(key == "APPCAST_REQ")
      handled = true;

    if(!handled)
      reportRunWarning("Unhandled Mail: " + key);
   }

   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool SimSOC::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool SimSOC::Iterate()
{
  AppCastingMOOSApp::Iterate();
  double now = MOOSTime();
  if(m_last_iter_time <= 0.0)
    m_last_iter_time = now;

  double dt_hours = (now - m_last_iter_time) / 3600.0;
  if(dt_hours < 0.0)
    dt_hours = 0.0;
  m_last_iter_time = now;

  m_charge_w = m_charge_w_base + m_solar_input_factor * m_charge_w_gain;
  m_load_w = m_idle_load_w + m_speed_load_coeff *
             pow(std::max(0.0, m_nav_speed), m_speed_load_exponent);
  m_net_w = m_charge_w - m_load_w;

  if(m_battery_capacity_wh > 0.0)
    m_soc += 100.0 * m_net_w * dt_hours / m_battery_capacity_wh;
  m_soc = clampSoc(m_soc);

  Notify(m_publish_var, m_soc);
  Notify("SIMSOC_CHARGE_W", m_charge_w);
  Notify("SIMSOC_LOAD_W", m_load_w);
  Notify("SIMSOC_NET_W", m_net_w);
  Notify("SIMSOC_SOLAR_INPUT_FACTOR", m_solar_input_factor);
  Notify("SIMSOC_NAV_SPEED", m_nav_speed);
  m_updates++;

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool SimSOC::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p=sParams.begin(); p!=sParams.end(); p++) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled =
      setConfigDouble(param, value, "start_soc", m_start_soc) ||
      setConfigDouble(param, value, "min_soc", m_min_soc) ||
      setConfigDouble(param, value, "max_soc", m_max_soc) ||
      setConfigDouble(param, value, "battery_capacity_wh", m_battery_capacity_wh) ||
      setConfigDouble(param, value, "charge_w_base", m_charge_w_base) ||
      setConfigDouble(param, value, "charge_w_gain", m_charge_w_gain) ||
      setConfigDouble(param, value, "idle_load_w", m_idle_load_w) ||
      setConfigDouble(param, value, "speed_load_coeff", m_speed_load_coeff) ||
      setConfigDouble(param, value, "speed_load_exponent", m_speed_load_exponent);

    if(param == "publish_var") {
      m_publish_var = stripBlankEnds(value);
      handled = true;
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);

  }

  if(m_min_soc > m_max_soc) {
    reportConfigWarning("min_soc greater than max_soc; using 0..100");
    m_min_soc = 0.0;
    m_max_soc = 100.0;
  }
  if(m_battery_capacity_wh <= 0.0) {
    reportConfigWarning("battery_capacity_wh must be positive; using 400");
    m_battery_capacity_wh = 400.0;
  }
  if(m_publish_var.empty()) {
    reportConfigWarning("publish_var must not be empty; using BATT_SOC");
    m_publish_var = "BATT_SOC";
  }
  if(m_speed_load_exponent < 0.0) {
    reportConfigWarning("speed_load_exponent below zero; using 3");
    m_speed_load_exponent = 3.0;
  }
  m_soc = clampSoc(m_start_soc);
  m_last_iter_time = MOOSTime();
  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void SimSOC::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("SOLAR_INPUT_FACTOR", 0);
  Register("NAV_SPEED", 0);
  Register("SOC_COMMAND", 0);
}

//---------------------------------------------------------
// Procedure: handleMailSolarInputFactor()

bool SimSOC::handleMailSolarInputFactor(CMOOSMsg& msg)
{
  if(!msg.IsDouble())
    return(false);
  m_solar_input_factor = std::max(0.0, std::min(1.0, msg.GetDouble()));
  return(true);
}

//---------------------------------------------------------
// Procedure: handleMailNavSpeed()

bool SimSOC::handleMailNavSpeed(CMOOSMsg& msg)
{
  if(!msg.IsDouble())
    return(false);
  m_nav_speed = std::max(0.0, msg.GetDouble());
  return(true);
}

//---------------------------------------------------------
// Procedure: handleMailSocCommand()

bool SimSOC::handleMailSocCommand(CMOOSMsg& msg)
{
  string command = stripBlankEnds(msg.GetAsString());
  string upper = toupper(command);

  if(strBegins(upper, "SET=")) {
    string value = command.substr(4);
    if(!isNumber(value))
      return(false);
    m_soc = clampSoc(atof(value.c_str()));
    return(true);
  }
  if(strBegins(upper, "ADD=")) {
    string value = command.substr(4);
    if(!isNumber(value))
      return(false);
    m_soc = clampSoc(m_soc + atof(value.c_str()));
    return(true);
  }
  if(strBegins(upper, "SUB=")) {
    string value = command.substr(4);
    if(!isNumber(value))
      return(false);
    m_soc = clampSoc(m_soc - atof(value.c_str()));
    return(true);
  }

  return(false);
}

//---------------------------------------------------------
// Procedure: setConfigDouble()

bool SimSOC::setConfigDouble(const string& param, const string& value,
                             const string& expected_param, double& target)
{
  if(param != expected_param)
    return(false);
  if(!isNumber(value)) {
    reportConfigWarning("Bad numeric value for " + expected_param + ": " + value);
    return(true);
  }
  target = atof(value.c_str());
  return(true);
}

//---------------------------------------------------------
// Procedure: clampSoc()

double SimSOC::clampSoc(double value) const
{
  return(std::max(m_min_soc, std::min(m_max_soc, value)));
}

//------------------------------------------------------------
// Procedure: buildReport()

bool SimSOC::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "pSimSOC                                    " << endl;
  m_msgs << "============================================" << endl;

  ACTable actab(2);
  actab << "Metric | Value";
  actab.addHeaderLines();
  actab << "Publish var" << m_publish_var;
  actab << "SOC" << doubleToString(m_soc, 2) + "%";
  actab << "Solar input factor" << doubleToString(m_solar_input_factor, 3);
  actab << "NAV_SPEED" << doubleToString(m_nav_speed, 2);
  actab << "Charge W" << doubleToString(m_charge_w, 1);
  actab << "Load W" << doubleToString(m_load_w, 1);
  actab << "Net W" << doubleToString(m_net_w, 1);
  actab << "Capacity Wh" << doubleToString(m_battery_capacity_wh, 1);
  actab << "Updates" << uintToString(m_updates);
  m_msgs << actab.getFormattedString();

  return(true);
}
