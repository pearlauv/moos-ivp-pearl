/************************************************************/
/*    NAME: Charles Benjamin                               */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: PearlSunPlan.cpp                                    */
/*    DATE: June 2026                                      */
/************************************************************/

#include <iterator>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "MBUtils.h"
#include "ACTable.h"
#include "PearlSunPlan.h"

using namespace std;

PearlSunPlanTask::PearlSunPlanTask()
{
  start_h = 0.0;
  cost_wh = 0.0;
  duration_h = 0.0;
  speed = 0.5;
  points = "";
  handled = false;
  dispatched = false;
}

//---------------------------------------------------------
// Constructor()

PearlSunPlan::PearlSunPlan()
{
  m_soc_var = "BATT_SOC";
  m_task_complete_var = "SUNPLAN_TASK_COMPLETE";
  m_task_active_var = "SUNPLAN_SURVEY_ACTIVE";
  m_wpt_update_var = "WPT_UPDATE";
  m_forecast_update_var = "SUN_FORECAST_24H";
  m_forecast_mode = "fixture";
  m_forecast_file = "forecast_example.csv";
  m_giveup_action = "return";
  m_battery_capacity_wh = 400.0;
  m_reserve_soc = 25.0;
  m_soc_stale_sec = 30.0;
  m_forecast_horizon_hours = 24.0;
  m_forecast_start_hour = 0.0;
  m_max_irradiance_w_m2 = 1000.0;
  m_charge_w_base = 0.0;
  m_charge_w_gain = 90.0;
  m_task_power_idle_w = 5.01;
  m_task_power_speed_coeff = 40.5;
  m_task_power_speed_exponent = 2.97;
  m_auto_deploy_on_dispatch = true;
  m_use_future_charge = true;

  m_mission_start_time = 0.0;
  m_latest_soc = 0.0;
  m_latest_soc_time = 0.0;
  m_last_irradiance = 0.0;
  m_last_solar_factor = 0.0;
  m_last_available_wh = 0.0;
  m_last_required_wh = 0.0;
  m_last_future_charge_wh = 0.0;
  m_active_dispatch_h = 0.0;
  m_active_duration_h = 0.0;
  m_have_soc = false;
  m_task_active = false;
  m_gave_up = false;
  m_last_decision = "startup";
  m_last_reason = "none";
  m_dispatch_count = 0;
  m_skip_count = 0;
}

//---------------------------------------------------------
// Destructor

PearlSunPlan::~PearlSunPlan()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool PearlSunPlan::OnNewMail(MOOSMSG_LIST &NewMail)
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
    if(key == m_soc_var)
      handled = handleMailSoc(msg);
    else if(key == m_task_complete_var)
      handled = handleMailTaskComplete(msg);
    else if(key == m_forecast_update_var)
      handled = handleMailForecast(msg);
    else if(key == "APPCAST_REQ")
      handled = true;

    if(!handled)
      reportRunWarning("Unhandled Mail: " + key);
   }

   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool PearlSunPlan::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool PearlSunPlan::Iterate()
{
  AppCastingMOOSApp::Iterate();
  double now = MOOSTime();
  if(m_mission_start_time <= 0.0)
    m_mission_start_time = now;

  double hours = missionHours();
  publishSolarInputs(hours);

  if(m_tasks.empty()) {
    publishDecision("idle", "no_tasks_configured");
    AppCastingMOOSApp::PostReport();
    return(true);
  }

  if(m_task_active) {
    if(m_active_duration_h > 0.0 &&
       hours >= (m_active_dispatch_h + m_active_duration_h)) {
      completeActiveTask("planned_duration_complete");
      AppCastingMOOSApp::PostReport();
      return(true);
    }
    publishDecision("active", "task_in_progress");
    AppCastingMOOSApp::PostReport();
    return(true);
  }

  PearlSunPlanTask* task = nextUnhandledTask();
  if(task == 0) {
    publishDecision("done", "all_tasks_handled");
    AppCastingMOOSApp::PostReport();
    return(true);
  }

  if(hours + 0.001 < task->start_h) {
    publishDecision("wait", "next_task_not_due");
    AppCastingMOOSApp::PostReport();
    return(true);
  }

  if(!socIsFresh(now)) {
    publishDecision("wait", "stale_or_missing_soc");
    publishEnergyDiagnostics(0.0, task->cost_wh, 0.0);
    AppCastingMOOSApp::PostReport();
    return(true);
  }

  double reserve_wh = (m_reserve_soc / 100.0) * m_battery_capacity_wh;
  double available_wh = (m_latest_soc / 100.0) * m_battery_capacity_wh - reserve_wh;
  double future_charge_wh = 0.0;
  if(m_use_future_charge)
    future_charge_wh = estimateFutureChargeWh(hours, m_forecast_horizon_hours);
  publishEnergyDiagnostics(available_wh, task->cost_wh, future_charge_wh);

  if(available_wh >= task->cost_wh)
    dispatchTask(*task, "sufficient_soc");
  else if(m_use_future_charge && (available_wh + future_charge_wh) >= task->cost_wh)
    dispatchTask(*task, "future_charge_supports_task");
  else
    giveUpTask(*task, "insufficient_soc_and_sun");

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool PearlSunPlan::OnStartUp()
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
      setConfigDouble(param, value, "battery_capacity_wh", m_battery_capacity_wh) ||
      setConfigDouble(param, value, "reserve_soc", m_reserve_soc) ||
      setConfigDouble(param, value, "soc_stale_sec", m_soc_stale_sec) ||
      setConfigDouble(param, value, "forecast_horizon_hours", m_forecast_horizon_hours) ||
      setConfigDouble(param, value, "forecast_start_hour", m_forecast_start_hour) ||
      setConfigDouble(param, value, "max_irradiance_w_m2", m_max_irradiance_w_m2) ||
      setConfigDouble(param, value, "charge_w_base", m_charge_w_base) ||
      setConfigDouble(param, value, "charge_w_gain", m_charge_w_gain) ||
      setConfigDouble(param, value, "task_power_idle_w", m_task_power_idle_w) ||
      setConfigDouble(param, value, "task_power_speed_coeff", m_task_power_speed_coeff) ||
      setConfigDouble(param, value, "task_power_speed_exponent", m_task_power_speed_exponent) ||
      setConfigBool(param, value, "auto_deploy_on_dispatch", m_auto_deploy_on_dispatch) ||
      setConfigBool(param, value, "use_future_charge", m_use_future_charge);

    if(param == "soc_var") {
      m_soc_var = stripBlankEnds(value);
      handled = true;
    }
    else if(param == "task_complete_var") {
      m_task_complete_var = stripBlankEnds(value);
      handled = true;
    }
    else if(param == "task_active_var") {
      m_task_active_var = stripBlankEnds(value);
      handled = true;
    }
    else if(param == "wpt_update_var") {
      m_wpt_update_var = stripBlankEnds(value);
      handled = true;
    }
    else if(param == "forecast_update_var") {
      m_forecast_update_var = stripBlankEnds(value);
      handled = true;
    }
    else if(param == "forecast_mode") {
      m_forecast_mode = tolower(stripBlankEnds(value));
      handled = true;
    }
    else if(param == "forecast_file") {
      m_forecast_file = stripBlankEnds(value);
      handled = true;
    }
    else if(param == "giveup_action") {
      m_giveup_action = tolower(stripBlankEnds(value));
      handled = true;
    }
    else if(param == "irradiance_24h") {
      handled = handleConfigForecastValues(value);
    }
    else if(param == "task") {
      handled = handleConfigTask(value);
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);

  }

  if(m_soc_var.empty()) {
    reportConfigWarning("soc_var must not be empty; using BATT_SOC");
    m_soc_var = "BATT_SOC";
  }
  if(m_battery_capacity_wh <= 0.0) {
    reportConfigWarning("battery_capacity_wh must be positive; using 400");
    m_battery_capacity_wh = 400.0;
  }
  if(m_forecast_horizon_hours <= 0.0 || m_forecast_horizon_hours > 24.0) {
    reportConfigWarning("forecast_horizon_hours must be in (0,24]; using 24");
    m_forecast_horizon_hours = 24.0;
  }
  if(m_forecast_start_hour < 0.0 || m_forecast_start_hour >= 24.0) {
    reportConfigWarning("forecast_start_hour must be in [0,24); using 0");
    m_forecast_start_hour = 0.0;
  }
  if(m_max_irradiance_w_m2 <= 0.0) {
    reportConfigWarning("max_irradiance_w_m2 must be positive; using 1000");
    m_max_irradiance_w_m2 = 1000.0;
  }
  if(m_task_power_idle_w < 0.0) {
    reportConfigWarning("task_power_idle_w must be non-negative; using 5.01");
    m_task_power_idle_w = 5.01;
  }
  if(m_task_power_speed_coeff < 0.0) {
    reportConfigWarning("task_power_speed_coeff must be non-negative; using 40.5");
    m_task_power_speed_coeff = 40.5;
  }
  if(m_task_power_speed_exponent < 0.0) {
    reportConfigWarning("task_power_speed_exponent must be non-negative; using 2.97");
    m_task_power_speed_exponent = 2.97;
  }
  if(m_giveup_action != "return" && m_giveup_action != "station") {
    reportConfigWarning("giveup_action must be return or station; using return");
    m_giveup_action = "return";
  }
  if(m_forecast_mode == "file" && !loadForecastFile(m_forecast_file))
    reportConfigWarning("Unable to load forecast_file: " + m_forecast_file);
  if(m_forecast_w_m2.empty())
    reportConfigWarning("No forecast values configured; SOLAR_INPUT_FACTOR will remain zero");

  sort(m_tasks.begin(), m_tasks.end(),
       [](const PearlSunPlanTask& a, const PearlSunPlanTask& b) {
         return(a.start_h < b.start_h);
       });

  m_mission_start_time = MOOSTime();
  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void PearlSunPlan::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register(m_soc_var, 0);
  Register(m_task_complete_var, 0);
  Register(m_forecast_update_var, 0);
}

//---------------------------------------------------------
// Procedure: handleMailSoc()

bool PearlSunPlan::handleMailSoc(CMOOSMsg& msg)
{
  if(!msg.IsDouble())
    return(false);
  m_latest_soc = std::max(0.0, std::min(100.0, msg.GetDouble()));
  m_latest_soc_time = MOOSTime();
  m_have_soc = true;
  return(true);
}

//---------------------------------------------------------
// Procedure: handleMailTaskComplete()

bool PearlSunPlan::handleMailTaskComplete(CMOOSMsg& msg)
{
  bool complete = false;
  if(msg.IsDouble())
    complete = (msg.GetDouble() != 0.0);
  else
    complete = (tolower(msg.GetAsString()) == "true");

  if(complete) {
    completeActiveTask("task_complete");
  }
  return(true);
}

//---------------------------------------------------------
// Procedure: handleMailForecast()

bool PearlSunPlan::handleMailForecast(CMOOSMsg& msg)
{
  return(handleConfigForecastValues(msg.GetAsString()));
}

//---------------------------------------------------------
// Procedure: handleConfigTask()

bool PearlSunPlan::handleConfigTask(const string& value)
{
  PearlSunPlanTask task;
  vector<string> fields = parseString(value, ';');
  bool cost_configured = false;
  bool duration_configured = false;

  for(unsigned int i=0; i<fields.size(); ++i) {
    string field = stripBlankEnds(fields[i]);
    string key = tolower(stripBlankEnds(biteStringX(field, '=')));
    string val = stripBlankEnds(field);

    if(key == "start_h" && isNumber(val))
      task.start_h = atof(val.c_str());
    else if(key == "cost_wh" && isNumber(val)) {
      task.cost_wh = atof(val.c_str());
      cost_configured = true;
    }
    else if(key == "duration_h" && isNumber(val)) {
      task.duration_h = atof(val.c_str());
      duration_configured = true;
    }
    else if(key == "speed" && isNumber(val))
      task.speed = atof(val.c_str());
    else if(key == "points")
      task.points = val;
    else
      return(false);
  }

  if(task.start_h < 0.0 || task.speed <= 0.0 || task.points.empty())
    return(false);

  double path_m = estimateTaskPathMeters(task.points);
  if(path_m <= 0.0)
    return(false);

  if(!duration_configured)
    task.duration_h = path_m / task.speed / 3600.0;
  if(!cost_configured)
    task.cost_wh = estimateTaskPowerWatts(task.speed) * task.duration_h;

  if(task.cost_wh < 0.0 || task.duration_h <= 0.0)
    return(false);

  m_tasks.push_back(task);
  return(true);
}

//---------------------------------------------------------
// Procedure: estimateTaskPathMeters()

double PearlSunPlan::estimateTaskPathMeters(const string& points) const
{
  vector<string> vertices = parseString(points, ':');
  double last_x = 0.0;
  double last_y = 0.0;
  bool have_last = false;
  double total = 0.0;

  for(unsigned int i=0; i<vertices.size(); ++i) {
    string vertex = stripBlankEnds(vertices[i]);
    vector<string> xy = parseString(vertex, ',');
    if(xy.size() < 2)
      return(0.0);

    string sx = stripBlankEnds(xy[0]);
    string sy = stripBlankEnds(xy[1]);
    if(!isNumber(sx) || !isNumber(sy))
      return(0.0);

    double x = atof(sx.c_str());
    double y = atof(sy.c_str());
    if(have_last)
      total += hypot(x - last_x, y - last_y);

    last_x = x;
    last_y = y;
    have_last = true;
  }

  return(total);
}

//---------------------------------------------------------
// Procedure: estimateTaskPowerWatts()

double PearlSunPlan::estimateTaskPowerWatts(double speed) const
{
  return(m_task_power_speed_coeff * pow(std::max(0.0, speed),
                                        m_task_power_speed_exponent) +
         m_task_power_idle_w);
}

//---------------------------------------------------------
// Procedure: handleConfigForecastValues()

bool PearlSunPlan::handleConfigForecastValues(const string& value)
{
  vector<string> parts = parseString(value, ',');
  vector<double> values;
  for(unsigned int i=0; i<parts.size(); ++i) {
    string part = stripBlankEnds(parts[i]);
    if(part.empty())
      continue;
    if(!isNumber(part))
      return(false);
    values.push_back(atof(part.c_str()));
  }

  if(values.empty())
    return(false);

  m_forecast_w_m2 = values;
  if(m_forecast_w_m2.size() > 24)
    m_forecast_w_m2.resize(24);
  return(true);
}

//---------------------------------------------------------
// Procedure: loadForecastFile()

bool PearlSunPlan::loadForecastFile(const string& path)
{
  ifstream in(path.c_str());
  if(!in)
    return(false);

  vector<double> values;
  string line;
  while(getline(in, line)) {
    string clean = biteStringX(line, '#');
    clean = stripBlankEnds(clean);
    if(clean.empty())
      continue;

    vector<string> csv = parseString(clean, ',');
    if(csv.size() >= 2 && isNumber(stripBlankEnds(csv[1]))) {
      values.push_back(atof(stripBlankEnds(csv[1]).c_str()));
      continue;
    }

    for(unsigned int i=0; i<csv.size(); ++i) {
      string part = stripBlankEnds(csv[i]);
      if(isNumber(part))
        values.push_back(atof(part.c_str()));
    }
  }

  if(values.empty())
    return(false);

  m_forecast_w_m2 = values;
  if(m_forecast_w_m2.size() > 24)
    m_forecast_w_m2.resize(24);
  return(true);
}

//---------------------------------------------------------
// Procedure: setConfigDouble()

bool PearlSunPlan::setConfigDouble(const string& param, const string& value,
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
// Procedure: setConfigBool()

bool PearlSunPlan::setConfigBool(const string& param, const string& value,
                            const string& expected_param, bool& target)
{
  if(param != expected_param)
    return(false);

  string lowered = tolower(stripBlankEnds(value));
  if(lowered == "true" || lowered == "yes" || lowered == "1") {
    target = true;
    return(true);
  }
  if(lowered == "false" || lowered == "no" || lowered == "0") {
    target = false;
    return(true);
  }
  reportConfigWarning("Bad boolean value for " + expected_param + ": " + value);
  return(true);
}

//---------------------------------------------------------
// Procedure: missionHours()

double PearlSunPlan::missionHours() const
{
  if(m_mission_start_time <= 0.0)
    return(0.0);
  return((MOOSTime() - m_mission_start_time) / 3600.0);
}

//---------------------------------------------------------
// Procedure: forecastHour()

double PearlSunPlan::forecastHour(double hours) const
{
  return(m_forecast_start_hour + hours);
}

//---------------------------------------------------------
// Procedure: currentIrradiance()

double PearlSunPlan::currentIrradiance(double hours) const
{
  if(m_forecast_w_m2.empty())
    return(0.0);
  int index = (int)floor(forecastHour(hours));
  if(index < 0)
    index = 0;
  if(index >= (int)m_forecast_w_m2.size())
    return(0.0);
  return(std::max(0.0, m_forecast_w_m2[index]));
}

//---------------------------------------------------------
// Procedure: currentSolarFactor()

double PearlSunPlan::currentSolarFactor(double hours) const
{
  double factor = currentIrradiance(hours) / m_max_irradiance_w_m2;
  return(std::max(0.0, std::min(1.0, factor)));
}

//---------------------------------------------------------
// Procedure: estimateFutureChargeWh()

double PearlSunPlan::estimateFutureChargeWh(double from_h, double to_h) const
{
  if(to_h <= from_h)
    return(0.0);

  double total_wh = 0.0;
  double t = from_h;
  while(t < to_h) {
    double next = std::min(to_h, floor(t) + 1.0);
    double factor = currentSolarFactor(t);
    double charge_w = m_charge_w_base + factor * m_charge_w_gain;
    total_wh += charge_w * (next - t);
    t = next;
  }
  return(total_wh);
}

//---------------------------------------------------------
// Procedure: socIsFresh()

bool PearlSunPlan::socIsFresh(double now) const
{
  return(m_have_soc && ((now - m_latest_soc_time) <= m_soc_stale_sec));
}

//---------------------------------------------------------
// Procedure: publishSolarInputs()

void PearlSunPlan::publishSolarInputs(double hours)
{
  m_last_irradiance = currentIrradiance(hours);
  m_last_solar_factor = currentSolarFactor(hours);
  Notify("SOLAR_IRRADIANCE", m_last_irradiance);
  Notify("SOLAR_INPUT_FACTOR", m_last_solar_factor);
  Notify("SUNPLAN_MISSION_HOURS", hours);
  Notify("SUNPLAN_FORECAST_HOUR", forecastHour(hours));
  if(m_have_soc)
    Notify("SUNPLAN_SOC", m_latest_soc);
}

//---------------------------------------------------------
// Procedure: publishDecision()

void PearlSunPlan::publishDecision(const string& decision, const string& reason)
{
  m_last_decision = decision;
  m_last_reason = reason;
  Notify("SUNPLAN_DECISION", decision);
  Notify("SUNPLAN_REASON", reason);
}

//---------------------------------------------------------
// Procedure: publishEnergyDiagnostics()

void PearlSunPlan::publishEnergyDiagnostics(double available_wh, double required_wh,
                                       double future_charge_wh)
{
  m_last_available_wh = available_wh;
  m_last_required_wh = required_wh;
  m_last_future_charge_wh = future_charge_wh;
  Notify("SUNPLAN_AVAILABLE_WH", available_wh);
  Notify("SUNPLAN_REQUIRED_WH", required_wh);
  Notify("SUNPLAN_EXPECTED_CHARGE_WH", future_charge_wh);
}

//---------------------------------------------------------
// Procedure: completeActiveTask()

void PearlSunPlan::completeActiveTask(const string& reason)
{
  m_task_active = false;
  m_active_duration_h = 0.0;
  Notify(m_task_active_var, "false");
  Notify(m_task_complete_var, "false");
  Notify("STATION_KEEP", "true");
  Notify("RETURN", "false");
  publishDecision("wait", reason);
}

//---------------------------------------------------------
// Procedure: dispatchTask()

void PearlSunPlan::dispatchTask(PearlSunPlanTask& task, const string& reason)
{
  Notify(m_wpt_update_var, taskUpdateString(task));
  Notify(m_task_active_var, "true");
  Notify(m_task_complete_var, "false");
  Notify("SUNPLAN_TASK_START_H", task.start_h);
  Notify("SUNPLAN_TASK_COST_WH", task.cost_wh);
  Notify("RETURN", "false");
  Notify("STATION_KEEP", "false");
  if(m_auto_deploy_on_dispatch) {
    Notify("DEPLOY", "true");
    Notify("MOOS_MANUAL_OVERRIDE", "false");
  }
  task.dispatched = true;
  task.handled = true;
  m_task_active = true;
  m_active_dispatch_h = missionHours();
  m_active_duration_h = task.duration_h;
  m_dispatch_count++;
  publishDecision("dispatch", reason);
}

//---------------------------------------------------------
// Procedure: giveUpTask()

void PearlSunPlan::giveUpTask(PearlSunPlanTask& task, const string& reason)
{
  task.handled = true;
  m_gave_up = true;
  m_skip_count++;
  Notify("SUNPLAN_TASK_SKIPPED", "true");
  Notify("SUNPLAN_TASK_START_H", task.start_h);
  Notify("SUNPLAN_TASK_COST_WH", task.cost_wh);
  Notify(m_task_active_var, "false");

  if(m_giveup_action == "station") {
    Notify("DEPLOY", "true");
    Notify("MOOS_MANUAL_OVERRIDE", "false");
    Notify("RETURN", "false");
    Notify("STATION_KEEP", "true");
  }
  else {
    Notify("DEPLOY", "true");
    Notify("MOOS_MANUAL_OVERRIDE", "false");
    Notify("STATION_KEEP", "false");
    Notify("RETURN", "true");
  }
  publishDecision("give_up", reason);
}

//---------------------------------------------------------
// Procedure: nextUnhandledTask()

PearlSunPlanTask* PearlSunPlan::nextUnhandledTask()
{
  for(unsigned int i=0; i<m_tasks.size(); ++i) {
    if(!m_tasks[i].handled)
      return(&m_tasks[i]);
  }
  return(0);
}

//---------------------------------------------------------
// Procedure: taskUpdateString()

string PearlSunPlan::taskUpdateString(const PearlSunPlanTask& task) const
{
  return("points=" + task.points + " # speed=" + doubleToString(task.speed, 2));
}

//------------------------------------------------------------
// Procedure: buildReport()

bool PearlSunPlan::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "pPearlSunPlan                                   " << endl;
  m_msgs << "============================================" << endl;

  ACTable actab(2);
  actab << "Metric | Value";
  actab.addHeaderLines();
  actab << "SOC var" << m_soc_var;
  actab << "SOC" << (m_have_soc ? doubleToString(m_latest_soc, 1) + "%" : "unset");
  actab << "Decision" << m_last_decision;
  actab << "Reason" << m_last_reason;
  actab << "Mission hours" << doubleToString(missionHours(), 3);
  actab << "Forecast hour" << doubleToString(forecastHour(missionHours()), 3);
  actab << "Irradiance" << doubleToString(m_last_irradiance, 1);
  actab << "Solar factor" << doubleToString(m_last_solar_factor, 3);
  actab << "Available Wh" << doubleToString(m_last_available_wh, 1);
  actab << "Required Wh" << doubleToString(m_last_required_wh, 1);
  actab << "Future charge Wh" << doubleToString(m_last_future_charge_wh, 1);
  actab << "Tasks" << uintToString(m_tasks.size());
  actab << "Dispatches" << uintToString(m_dispatch_count);
  actab << "Skips" << uintToString(m_skip_count);
  m_msgs << actab.getFormattedString();

  return(true);
}
