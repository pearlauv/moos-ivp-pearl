/************************************************************/
/*    NAME: Charles Benjamin                               */
/*    ORGN: MIT, Cambridge MA                              */
/*    FILE: Rendezvous.cpp                                 */
/*    DATE: August 2026                                    */
/************************************************************/

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "NodeMessage.h"
#include "Rendezvous.h"

using namespace std;

//---------------------------------------------------------
// Constructor

Rendezvous::Rendezvous()
{
  m_role                = "";
  m_ownship             = "";
  m_peer_node           = "";
  m_uav_speed           = 3.0;
  m_pearl_speed         = 0.5;
  m_min_battery         = 25.0;
  m_nav_stale_thresh    = 10.0;
  m_request_timeout     = 20.0;
  m_route_timeout       = 5.0;
  m_transit_timeout     = 180.0;
  m_arrival_radius      = 4.0;
  m_arrival_dwell       = 2.0;
  m_state_post_interval = 1.0;
  m_require_health      = true;
  m_require_battery     = false;
  m_require_flight_state = true;

  m_state                = "IDLE";
  m_reason               = "startup";
  m_state_enter_time     = 0.0;
  m_last_state_post_time = 0.0;
  m_route_command_time   = 0.0;
  m_route_state_time     = 0.0;
  m_nav_x                = 0.0;
  m_nav_y                = 0.0;
  m_nav_x_time           = 0.0;
  m_nav_y_time           = 0.0;
  m_battery              = 0.0;
  m_battery_time         = 0.0;
  m_target_x             = 0.0;
  m_target_y             = 0.0;
  m_uav_report_x         = 0.0;
  m_uav_report_y         = 0.0;
  m_uav_report_time      = 0.0;
  m_pearl_activation_time = 0.0;
  m_arrival_start_time   = 0.0;
  m_nav_x_set            = false;
  m_nav_y_set            = false;
  m_battery_set          = false;
  m_health_ok            = false;
  m_health_set           = false;
  m_uav_armed            = false;
  m_uav_armed_set        = false;
  m_uav_in_air           = false;
  m_uav_landed_set       = false;
  m_target_set           = false;
  m_pearl_activation_pending = false;
  m_pearl_arrived        = false;
  m_uav_arrived          = false;
  m_clearance_received   = false;
  m_completion_sent      = false;
  m_config_valid         = true;
  m_requests_sent        = 0;
  m_proposals_sent       = 0;
  m_acceptances_sent     = 0;
  m_clearances_sent      = 0;
  m_aborts               = 0;
}

Rendezvous::~Rendezvous()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool Rendezvous::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  for(MOOSMSG_LIST::iterator p=NewMail.begin(); p!=NewMail.end(); ++p) {
    CMOOSMsg &msg = *p;
    string key = msg.GetKey();
    bool handled = false;

    if((m_role == "uav") && (key == "RENDEZVOUS_START"))
      handled = handleMailStart(msg);
    else if(key == "RENDEZVOUS_ABORT")
      handled = handleMailAbort(msg);
    else if((m_role == "pearl") && (key == "RENDEZVOUS_REQUEST"))
      handled = handleMailRequest(msg);
    else if((m_role == "uav") && (key == "RENDEZVOUS_PROPOSAL"))
      handled = handleMailProposal(msg);
    else if(key == "RENDEZVOUS_RESPONSE")
      handled = handleMailResponse(msg);
    else if((m_role == "uav") && (key == "LANDING_CLEARANCE"))
      handled = handleMailClearance(msg);
    else if((m_role == "pearl") && (key == "NODE_REPORT"))
      handled = handleMailNodeReport(msg);
    else if(key == "NAV_X")
      handled = handleMailDouble(msg, m_nav_x, m_nav_x_time) &&
                (m_nav_x_set = true);
    else if(key == "NAV_Y")
      handled = handleMailDouble(msg, m_nav_y, m_nav_y_time) &&
                (m_nav_y_set = true);
    else if((m_role == "uav") && (key == "UAV_BATTERY_PERCENT"))
      handled = handleMailDouble(msg, m_battery, m_battery_time) &&
                (m_battery_set = true);
    else if((m_role == "uav") && (key == "UAV_HEALTH_ALL_OK"))
      handled = handleMailBool(msg, m_health_ok, m_health_set);
    else if((m_role == "uav") && (key == "UAV_IS_ARMED"))
      handled = handleMailBool(msg, m_uav_armed, m_uav_armed_set);
    else if((m_role == "uav") && (key == "UAV_LANDED_STATE")) {
      if(msg.IsString()) {
        string value = toupper(stripBlankEnds(msg.GetString()));
        m_uav_in_air = ((value == "IN_AIR") || (value == "TAKING_OFF") ||
                        (value == "LANDING"));
        m_uav_landed_set = true;
        handled = true;
      }
    }
    else if((m_role == "uav") && (key == "ROUTE_BUFFER_VEHICLE_STATE")) {
      if(msg.IsString()) {
        m_route_state = toupper(stripBlankEnds(msg.GetString()));
        m_route_state_time = m_curr_time;
        handled = true;
      }
    }
    else if((m_role == "pearl") && (key == "PEARL_RENDEZVOUS_ARRIVED")) {
      if(mailIsTrue(msg))
        m_pearl_arrived = true;
      handled = true;
    }
    else if(key == "APPCAST_REQ")
      handled = true;

    if(!handled && (key != "APPCAST_REQ"))
      reportRunWarning("Unhandled Mail: " + key);
  }

  return(true);
}

//---------------------------------------------------------
// Procedure: Iterate

bool Rendezvous::Iterate()
{
  AppCastingMOOSApp::Iterate();

  if(m_state_enter_time <= 0)
    m_state_enter_time = m_curr_time;

  if(m_config_valid) {
    if(m_role == "uav")
      iterateUAV();
    else if(m_role == "pearl")
      iteratePearl();
    publishState(false);
  }

  AppCastingMOOSApp::PostReport();
  return(true);
}

bool Rendezvous::OnConnectToServer()
{
  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp

bool Rendezvous::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  for(STRING_LIST::iterator p=sParams.begin(); p!=sParams.end(); ++p) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;
    bool handled = false;

    if(param == "role") {
      value = tolower(stripBlankEnds(value));
      handled = ((value == "uav") || (value == "pearl"));
      if(handled)
        m_role = value;
    }
    else if(param == "ownship")
      handled = setNonWhiteVarOnString(m_ownship, value);
    else if(param == "peer_node")
      handled = setNonWhiteVarOnString(m_peer_node, value);
    else if(param == "uav_speed")
      handled = setPosDoubleOnString(m_uav_speed, value);
    else if(param == "pearl_speed")
      handled = setPosDoubleOnString(m_pearl_speed, value);
    else if(param == "min_battery")
      handled = setNonNegDoubleOnString(m_min_battery, value) &&
                (m_min_battery <= 100);
    else if(param == "nav_stale_thresh")
      handled = setPosDoubleOnString(m_nav_stale_thresh, value);
    else if(param == "request_timeout")
      handled = setPosDoubleOnString(m_request_timeout, value);
    else if(param == "route_timeout")
      handled = setPosDoubleOnString(m_route_timeout, value);
    else if(param == "transit_timeout")
      handled = setPosDoubleOnString(m_transit_timeout, value);
    else if(param == "arrival_radius")
      handled = setPosDoubleOnString(m_arrival_radius, value);
    else if(param == "arrival_dwell")
      handled = setNonNegDoubleOnString(m_arrival_dwell, value);
    else if(param == "state_post_interval")
      handled = setPosDoubleOnString(m_state_post_interval, value);
    else if(param == "require_health")
      handled = setBooleanOnString(m_require_health, value);
    else if(param == "require_battery")
      handled = setBooleanOnString(m_require_battery, value);
    else if(param == "require_flight_state")
      handled = setBooleanOnString(m_require_flight_state, value);

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  if(m_ownship == "")
    m_ownship = m_host_community;

  if((m_role != "uav") && (m_role != "pearl")) {
    reportConfigWarning("role must be uav or pearl");
    m_config_valid = false;
  }
  if(m_peer_node == "") {
    reportConfigWarning("peer_node is required");
    m_config_valid = false;
  }

  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables

void Rendezvous::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("RENDEZVOUS_ABORT", 0);
  Register("NAV_X", 0);
  Register("NAV_Y", 0);

  if(m_role == "uav") {
    Register("RENDEZVOUS_START", 0);
    Register("RENDEZVOUS_PROPOSAL", 0);
    Register("LANDING_CLEARANCE", 0);
    Register("ROUTE_BUFFER_VEHICLE_STATE", 0);
    Register("UAV_BATTERY_PERCENT", 0);
    Register("UAV_HEALTH_ALL_OK", 0);
    Register("UAV_IS_ARMED", 0);
    Register("UAV_LANDED_STATE", 0);
    Register("RENDEZVOUS_RESPONSE", 0);
  }
  else if(m_role == "pearl") {
    Register("RENDEZVOUS_REQUEST", 0);
    Register("RENDEZVOUS_RESPONSE", 0);
    Register("PEARL_RENDEZVOUS_ARRIVED", 0);
    Register("NODE_REPORT", 0);
  }
}

//---------------------------------------------------------
// Mail handlers

bool Rendezvous::handleMailStart(const CMOOSMsg& msg)
{
  if(!mailIsTrue(msg))
    return(true);

  if((m_state == "REQUESTING") || (m_state == "RENDEZVOUS") ||
     (m_state == "LANDING")) {
    reportRunWarning("Rendezvous start rejected: mission already active");
    return(true);
  }

  return(beginRequest());
}

bool Rendezvous::handleMailAbort(const CMOOSMsg& msg)
{
  if(!mailIsTrue(msg))
    return(true);

  if(m_state == "LANDING") {
    reportRunWarning("Rendezvous abort ignored: precision landing committed");
    return(true);
  }

  abortMission("operator_abort");
  return(true);
}

bool Rendezvous::handleMailRequest(const CMOOSMsg& msg)
{
  if(!msg.IsString())
    return(false);

  string request = msg.GetString();
  string id = tokStringParse(request, "id", '#', '=');
  if(id == "") {
    reportRunWarning("Rejected rendezvous request without an ID");
    return(true);
  }

  if((m_state == "REQUESTING") || (m_state == "RENDEZVOUS") ||
     (m_state == "LANDING")) {
    string response = "id=" + id + "#status=rejected#reason=busy";
    sendMessage(m_peer_node, "RENDEZVOUS_RESPONSE", response);
    return(true);
  }

  return(proposeRendezvous(request));
}

bool Rendezvous::handleMailProposal(const CMOOSMsg& msg)
{
  if(!msg.IsString())
    return(false);

  if(m_state != "REQUESTING") {
    reportRunWarning("Ignored rendezvous proposal while not requesting");
    return(true);
  }

  return(acceptProposal(msg.GetString()));
}

bool Rendezvous::handleMailResponse(const CMOOSMsg& msg)
{
  if(!msg.IsString())
    return(false);

  string response = msg.GetString();
  string id = tokStringParse(response, "id", '#', '=');
  string status = tolower(tokStringParse(response, "status", '#', '='));
  string reason = tokStringParse(response, "reason", '#', '=');
  if((id == "") || (id != m_session_id)) {
    reportRunWarning("Ignored response for a stale rendezvous session");
    return(true);
  }

  if(m_role == "uav") {
    if((status == "rejected") || (status == "abort"))
      abortMission((reason == "") ? status : reason, false);
    else
      reportRunWarning("Ignored unexpected response status: " + status);
  }
  else if((status == "accepted") && (m_state == "REQUESTING")) {
    activatePearlTransit();
    transitionTo("RENDEZVOUS", "proposal_accepted");
  }
  else if(status == "complete") {
    transitionTo("COMPLETE", "uav_landed");
    publishPoint(false);
  }
  else if((status == "rejected") || (status == "abort"))
    abortMission((reason == "") ? status : reason, false);

  return(true);
}

bool Rendezvous::handleMailClearance(const CMOOSMsg& msg)
{
  if(!msg.IsString())
    return(false);

  string clearance = msg.GetString();
  string id = tokStringParse(clearance, "id", '#', '=');
  if((id != m_session_id) || (m_state != "RENDEZVOUS")) {
    reportRunWarning("Ignored stale or unexpected landing clearance");
    return(true);
  }

  m_clearance_received = true;
  if(m_uav_arrived)
    beginPrecisionLanding();
  return(true);
}

bool Rendezvous::handleMailNodeReport(const CMOOSMsg& msg)
{
  if(!msg.IsString())
    return(false);

  string report = msg.GetString();
  string name = tolower(tokStringParse(report, "NAME"));
  if(name == "")
    name = tolower(tokStringParse(report, "name"));
  if(name != tolower(m_peer_node))
    return(true);

  string sx = tokStringParse(report, "X");
  string sy = tokStringParse(report, "Y");
  if(sx == "")
    sx = tokStringParse(report, "x");
  if(sy == "")
    sy = tokStringParse(report, "y");
  if(!isNumber(sx) || !isNumber(sy))
    return(true);

  m_uav_report_x = atof(sx.c_str());
  m_uav_report_y = atof(sy.c_str());
  m_uav_report_time = m_curr_time;
  return(true);
}

bool Rendezvous::handleMailDouble(const CMOOSMsg& msg, double& value,
                                  double& value_time)
{
  if(!msg.IsDouble())
    return(false);
  value = msg.GetDouble();
  value_time = m_curr_time;
  return(std::isfinite(value));
}

bool Rendezvous::handleMailBool(const CMOOSMsg& msg, bool& value,
                                bool& value_set)
{
  value = mailIsTrue(msg);
  value_set = true;
  return(true);
}

//---------------------------------------------------------
// Role logic

void Rendezvous::iterateUAV()
{
  double elapsed = m_curr_time - m_state_enter_time;

  if(m_state == "REQUESTING") {
    if(m_route_command_time > 0) {
      bool recent_state = (m_route_state_time >= m_route_command_time);
      if(recent_state && (m_route_state == "DEPLOY_ACCEPTED")) {
        string response = "id=" + m_session_id + "#status=accepted";
        sendMessage(m_peer_node, "RENDEZVOUS_RESPONSE", response);
        m_acceptances_sent++;
        transitionTo("RENDEZVOUS", "route_accepted");
      }
      else if(recent_state && strContains(m_route_state, "REJECTED")) {
        string response = "id=" + m_session_id;
        response += "#status=rejected#reason=route_rejected";
        sendMessage(m_peer_node, "RENDEZVOUS_RESPONSE", response);
        abortMission("route_rejected", false);
      }
      else if((m_curr_time - m_route_command_time) > m_route_timeout) {
        string response = "id=" + m_session_id;
        response += "#status=rejected#reason=route_timeout";
        sendMessage(m_peer_node, "RENDEZVOUS_RESPONSE", response);
        abortMission("route_timeout", false);
      }
    }
    else if(elapsed > m_request_timeout)
      abortMission("proposal_timeout");
  }
  else if(m_state == "RENDEZVOUS") {
    string reason;
    if(!uavIsReady(reason)) {
      abortMission(reason);
      return;
    }
    if(elapsed > m_transit_timeout) {
      abortMission("transit_timeout");
      return;
    }
    checkUAVArrival();
    if(m_clearance_received && m_uav_arrived)
      beginPrecisionLanding();
  }
  else if((m_state == "LANDING") && m_uav_landed_set && !m_uav_in_air &&
          !m_completion_sent) {
    string response = "id=" + m_session_id + "#status=complete";
    sendMessage(m_peer_node, "RENDEZVOUS_RESPONSE", response);
    m_completion_sent = true;
    transitionTo("COMPLETE", "landed");
  }
}

void Rendezvous::iteratePearl()
{
  double elapsed = m_curr_time - m_state_enter_time;

  if(m_pearl_activation_pending &&
     (m_curr_time >= m_pearl_activation_time)) {
    Notify("PEARL_RENDEZVOUS_ACTIVE", "true");
    m_pearl_activation_pending = false;
  }

  if((m_state == "REQUESTING") && (elapsed > m_request_timeout))
    abortMission("response_timeout");
  else if(m_state == "RENDEZVOUS") {
    if(!navIsFresh()) {
      abortMission("pearl_navigation_stale");
      return;
    }
    if((m_uav_report_time <= 0) ||
       ((m_curr_time - m_uav_report_time) > m_nav_stale_thresh)) {
      if(elapsed > m_nav_stale_thresh)
        abortMission("uav_report_stale");
      return;
    }
    if(elapsed > m_transit_timeout) {
      abortMission("transit_timeout");
      return;
    }

    checkPearlArrival();
    double uav_dist = hypot(m_uav_report_x - m_target_x,
                            m_uav_report_y - m_target_y);
    m_uav_arrived = (uav_dist <= m_arrival_radius);

    if(m_pearl_arrived && m_uav_arrived) {
      if(m_arrival_start_time <= 0)
        m_arrival_start_time = m_curr_time;
      if((m_curr_time - m_arrival_start_time) >= m_arrival_dwell)
        grantLandingClearance();
    }
    else
      m_arrival_start_time = 0;
  }
}

bool Rendezvous::beginRequest()
{
  string reason;
  if(!uavIsReady(reason)) {
    m_session_id = "";
    m_target_set = false;
    transitionTo("ABORT", reason);
    reportRunWarning("Rendezvous rejected: " + reason);
    return(true);
  }

  m_session_id = makeSessionID();
  m_target_set = false;
  m_route_command_time = 0;
  m_route_state_time = 0;
  m_arrival_start_time = 0;
  m_uav_arrived = false;
  m_clearance_received = false;
  m_completion_sent = false;

  string battery = m_battery_set ? doubleToStringX(m_battery, 1) : "unknown";
  string request = "id=" + m_session_id;
  request += "#x=" + doubleToStringX(m_nav_x, 2);
  request += "#y=" + doubleToStringX(m_nav_y, 2);
  request += "#speed=" + doubleToStringX(m_uav_speed, 2);
  request += "#battery=" + battery;
  request += "#health=ok";

  if(!sendMessage(m_peer_node, "RENDEZVOUS_REQUEST", request))
    return(false);

  m_requests_sent++;
  transitionTo("REQUESTING", "request_sent");
  return(true);
}

bool Rendezvous::proposeRendezvous(const string& request)
{
  string id = tokStringParse(request, "id", '#', '=');
  string sx = tokStringParse(request, "x", '#', '=');
  string sy = tokStringParse(request, "y", '#', '=');
  string sspeed = tokStringParse(request, "speed", '#', '=');
  string battery = tolower(tokStringParse(request, "battery", '#', '='));
  string health = tolower(tokStringParse(request, "health", '#', '='));

  string reject_reason;
  if(!navIsFresh())
    reject_reason = "pearl_navigation_stale";
  else if(!isNumber(sx) || !isNumber(sy) || !isNumber(sspeed))
    reject_reason = "invalid_request";
  else if(health != "ok")
    reject_reason = "uav_unhealthy";
  else if(m_require_battery && !isNumber(battery))
    reject_reason = "battery_unknown";
  else if(isNumber(battery) && (atof(battery.c_str()) < m_min_battery))
    reject_reason = "battery_below_reserve";

  if(reject_reason != "") {
    string response = "id=" + id + "#status=rejected#reason=" + reject_reason;
    sendMessage(m_peer_node, "RENDEZVOUS_RESPONSE", response);
    transitionTo("ABORT", reject_reason);
    return(true);
  }

  double uav_x = atof(sx.c_str());
  double uav_y = atof(sy.c_str());
  double request_speed = atof(sspeed.c_str());
  double speed_sum = m_pearl_speed + request_speed;
  if(speed_sum <= 0) {
    string response = "id=" + id + "#status=rejected#reason=invalid_speed";
    sendMessage(m_peer_node, "RENDEZVOUS_RESPONSE", response);
    transitionTo("ABORT", "invalid_speed");
    return(true);
  }

  // With negligible PEARL turn radius, this fraction equalizes nominal
  // travel time along the straight line between the two vehicles.
  double fraction = m_pearl_speed / speed_sum;
  m_target_x = m_nav_x + fraction * (uav_x - m_nav_x);
  m_target_y = m_nav_y + fraction * (uav_y - m_nav_y);
  m_target_set = true;
  m_session_id = id;
  m_pearl_arrived = false;
  m_uav_arrived = false;
  m_arrival_start_time = 0;

  string proposal = "id=" + id;
  proposal += "#x=" + doubleToStringX(m_target_x, 2);
  proposal += "#y=" + doubleToStringX(m_target_y, 2);
  proposal += "#valid_for=" + doubleToStringX(m_request_timeout, 1);
  if(!sendMessage(m_peer_node, "RENDEZVOUS_PROPOSAL", proposal))
    return(false);

  m_proposals_sent++;
  publishPoint(true);
  transitionTo("REQUESTING", "proposal_sent");
  return(true);
}

bool Rendezvous::acceptProposal(const string& proposal)
{
  string id = tokStringParse(proposal, "id", '#', '=');
  string sx = tokStringParse(proposal, "x", '#', '=');
  string sy = tokStringParse(proposal, "y", '#', '=');

  string reason;
  if((id == "") || (id != m_session_id))
    reason = "stale_session";
  else if(!isNumber(sx) || !isNumber(sy))
    reason = "invalid_proposal";
  else {
    string readiness_reason;
    if(!uavIsReady(readiness_reason))
      reason = readiness_reason;
  }

  if(reason != "") {
    string response = "id=" + ((id == "") ? m_session_id : id);
    response += "#status=rejected#reason=" + reason;
    sendMessage(m_peer_node, "RENDEZVOUS_RESPONSE", response);
    abortMission(reason, false);
    return(true);
  }

  m_target_x = atof(sx.c_str());
  m_target_y = atof(sy.c_str());
  m_target_set = true;
  m_uav_arrived = false;
  m_arrival_start_time = 0;

  string command = "action=deploy # points={";
  command += doubleToStringX(m_target_x, 2) + ",";
  command += doubleToStringX(m_target_y, 2) + "}";
  Notify("ROUTE_BUFFER_COMMAND", command);
  m_route_command_time = m_curr_time;
  m_reason = "staging_route";
  publishState(true);
  return(true);
}

void Rendezvous::activatePearlTransit()
{
  string update = "points=" + doubleToStringX(m_target_x, 2) + ",";
  update += doubleToStringX(m_target_y, 2);
  update += " # repeat=0 # order=normal";

  // Stage the waypoint before enabling the initially-empty behavior. This
  // prevents it from completing on activation before it sees its first point.
  Notify("PEARL_RENDEZVOUS_UPDATE", update);
  Notify("PEARL_RENDEZVOUS_ARRIVED", "false");
  Notify("PEARL_DEPLOY", "true");
  Notify("PEARL_RETURN", "false");
  Notify("PEARL_STATION_KEEP", "false");
  Notify("PEARL_MANUAL_OVERRIDE", "false");

  // Give the Helm one iteration to apply the update before activating an
  // initially-empty waypoint behavior.
  m_pearl_activation_pending = true;
  m_pearl_activation_time = m_curr_time + 0.25;

  m_pearl_arrived = false;
  m_arrival_start_time = 0;
}

void Rendezvous::checkPearlArrival()
{
  if(!m_target_set)
    return;
  if(hypot(m_nav_x - m_target_x, m_nav_y - m_target_y) <= m_arrival_radius)
    m_pearl_arrived = true;
}

void Rendezvous::checkUAVArrival()
{
  if(!m_target_set)
    return;

  bool inside = (hypot(m_nav_x - m_target_x, m_nav_y - m_target_y) <=
                 m_arrival_radius);
  if(inside) {
    if(m_arrival_start_time <= 0)
      m_arrival_start_time = m_curr_time;
    m_uav_arrived = ((m_curr_time - m_arrival_start_time) >= m_arrival_dwell);
  }
  else {
    m_arrival_start_time = 0;
    m_uav_arrived = false;
  }
}

void Rendezvous::grantLandingClearance()
{
  Notify("PEARL_RENDEZVOUS_ACTIVE", "false");
  Notify("PEARL_DEPLOY", "true");
  Notify("PEARL_RETURN", "false");
  Notify("PEARL_STATION_KEEP", "true");
  Notify("PEARL_MANUAL_OVERRIDE", "false");

  string clearance = "id=" + m_session_id + "#status=cleared";
  sendMessage(m_peer_node, "LANDING_CLEARANCE", clearance);
  m_clearances_sent++;
  transitionTo("LANDING", "clearance_sent");
}

void Rendezvous::beginPrecisionLanding()
{
  if(m_state != "RENDEZVOUS")
    return;
  Notify("UAV_PREC_LAND_REQUEST", "true");
  transitionTo("LANDING", "clearance_received");
}

void Rendezvous::abortMission(const string& reason, bool notify_peer)
{
  if(m_state == "ABORT")
    return;

  if(m_role == "uav") {
    if((m_state == "RENDEZVOUS") || (m_route_command_time > 0))
      Notify("ROUTE_BUFFER_COMMAND", "action=clear");
  }
  else if(m_role == "pearl") {
    m_pearl_activation_pending = false;
    Notify("PEARL_RENDEZVOUS_ACTIVE", "false");
    Notify("PEARL_DEPLOY", "true");
    Notify("PEARL_RETURN", "false");
    Notify("PEARL_STATION_KEEP", "true");
    Notify("PEARL_MANUAL_OVERRIDE", "false");
    publishPoint(false);
  }

  if(notify_peer && (m_session_id != "")) {
    string response = "id=" + m_session_id;
    response += "#status=abort#reason=" + reason;
    sendMessage(m_peer_node, "RENDEZVOUS_RESPONSE", response);
  }

  m_aborts++;
  transitionTo("ABORT", reason);
}

//---------------------------------------------------------
// Publications and utility methods

void Rendezvous::transitionTo(const string& state, const string& reason)
{
  m_state = state;
  m_reason = reason;
  m_state_enter_time = m_curr_time;
  publishState(true);
  reportEvent("Rendezvous state: " + state + " (" + reason + ")");
}

void Rendezvous::publishState(bool force)
{
  if(!force && ((m_curr_time - m_last_state_post_time) < m_state_post_interval))
    return;

  string state = "state=" + m_state + "#reason=" + m_reason;
  if(m_session_id != "")
    state += "#id=" + m_session_id;
  if(m_target_set) {
    state += "#x=" + doubleToStringX(m_target_x, 2);
    state += "#y=" + doubleToStringX(m_target_y, 2);
  }
  string state_var = (m_role == "uav") ? "UAV_RENDEZVOUS_STATE" :
                                          "PEARL_RENDEZVOUS_STATE";
  Notify(state_var, state);
  m_last_state_post_time = m_curr_time;
}

void Rendezvous::publishPoint(bool active)
{
  if((m_role != "pearl") || !m_target_set)
    return;

  string spec = "x=" + doubleToStringX(m_target_x, 2);
  spec += ",y=" + doubleToStringX(m_target_y, 2);
  spec += ",label=rendezvous_point";
  spec += ",vertex_color=magenta,label_color=magenta,vertex_size=10";
  spec += active ? ",active=true" : ",active=false";
  Notify("VIEW_POINT", spec);
  Notify("RENDEZVOUS_POINT", pointString(m_target_x, m_target_y));
}

bool Rendezvous::sendMessage(const string& destination,
                             const string& variable,
                             const string& value)
{
  NodeMessage message(m_ownship, destination, variable);
  message.setSourceApp(GetAppName());
  message.setStringVal(value);
  if(!message.valid()) {
    reportRunWarning("Unable to form rendezvous node message");
    return(false);
  }
  Notify("NODE_MESSAGE_LOCAL", message.getSpec());
  return(true);
}

bool Rendezvous::navIsFresh() const
{
  if(!m_nav_x_set || !m_nav_y_set)
    return(false);
  double latest = (m_nav_x_time < m_nav_y_time) ? m_nav_x_time : m_nav_y_time;
  return((m_curr_time - latest) <= m_nav_stale_thresh);
}

bool Rendezvous::uavIsReady(string& reason) const
{
  if(!navIsFresh())
    reason = "navigation_stale";
  else if(m_require_health && (!m_health_set || !m_health_ok))
    reason = "health_not_ready";
  else if(m_require_flight_state && (!m_uav_armed_set || !m_uav_armed))
    reason = "not_armed";
  else if(m_require_flight_state && (!m_uav_landed_set || !m_uav_in_air))
    reason = "not_airborne";
  else if(m_require_battery && !m_battery_set)
    reason = "battery_unknown";
  else if(m_battery_set && (m_battery < m_min_battery))
    reason = "battery_below_reserve";
  else {
    reason = "ready";
    return(true);
  }
  return(false);
}

bool Rendezvous::mailIsTrue(const CMOOSMsg& msg) const
{
  if(msg.IsDouble())
    return(msg.GetDouble() != 0);
  string value = tolower(stripBlankEnds(msg.GetString()));
  return((value == "true") || (value == "1") || (value == "on"));
}

string Rendezvous::makeSessionID() const
{
  uint64_t usec = static_cast<uint64_t>(chrono::duration_cast<chrono::microseconds>
    (chrono::system_clock::now().time_since_epoch()).count());
  return(tolower(m_ownship) + "_" + to_string(usec));
}

string Rendezvous::pointString(double x, double y) const
{
  return("{" + doubleToStringX(x, 2) + "," +
         doubleToStringX(y, 2) + "}");
}

//------------------------------------------------------------
// Procedure: buildReport

bool Rendezvous::buildReport()
{
  m_msgs << "Role:               " << m_role << endl;
  m_msgs << "Ownship / peer:     " << m_ownship << " / " << m_peer_node << endl;
  m_msgs << "State:              " << m_state << endl;
  m_msgs << "Reason:             " << m_reason << endl;
  m_msgs << "Session:            " << m_session_id << endl;
  m_msgs << "Navigation fresh:   " << boolToString(navIsFresh()) << endl;
  m_msgs << "NAV X/Y:            " << doubleToStringX(m_nav_x, 2) << " / "
         << doubleToStringX(m_nav_y, 2) << endl;
  m_msgs << "Target set:         " << boolToString(m_target_set) << endl;
  if(m_target_set)
    m_msgs << "Target X/Y:         " << doubleToStringX(m_target_x, 2) << " / "
           << doubleToStringX(m_target_y, 2) << endl;

  if(m_role == "uav") {
    string ready_reason;
    bool ready = uavIsReady(ready_reason);
    m_msgs << "UAV ready:          " << boolToString(ready)
           << " (" << ready_reason << ")" << endl;
    m_msgs << "Battery:            ";
    if(m_battery_set)
      m_msgs << doubleToStringX(m_battery, 1) << "%" << endl;
    else
      m_msgs << "unknown" << endl;
    m_msgs << "Route state:        " << m_route_state << endl;
    m_msgs << "At rendezvous:      " << boolToString(m_uav_arrived) << endl;
    m_msgs << "Clearance received: " << boolToString(m_clearance_received) << endl;
  }
  else {
    m_msgs << "PEARL arrived:      " << boolToString(m_pearl_arrived) << endl;
    m_msgs << "UAV arrived:        " << boolToString(m_uav_arrived) << endl;
    m_msgs << "UAV report age:     ";
    if(m_uav_report_time > 0)
      m_msgs << doubleToStringX(m_curr_time - m_uav_report_time, 2) << endl;
    else
      m_msgs << "n/a" << endl;
  }

  ACTable table(2);
  table << "Event | Count";
  table.addHeaderLines();
  table << "Requests sent" << m_requests_sent;
  table << "Proposals sent" << m_proposals_sent;
  table << "Acceptances sent" << m_acceptances_sent;
  table << "Clearances sent" << m_clearances_sent;
  table << "Aborts" << m_aborts;
  m_msgs << endl << table.getFormattedString();
  return(true);
}
