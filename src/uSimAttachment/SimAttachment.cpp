/************************************************************/
/*    NAME: Charles Benjamin                                */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: SimAttachment.cpp                               */
/*    DATE: August 18th, 2026                               */
/************************************************************/

#include <algorithm>
#include <cmath>
#include "ACTable.h"
#include "MBUtils.h"
#include "SimAttachment.h"

using namespace std;

SimAttachment::SimAttachment()
  : m_input_max_age(2.0),
    m_attachment_requested(false),
    m_attached(false),
    m_detaching(false),
    m_last_state(""),
    m_last_state_post(-1.0),
    m_attach_count(0),
    m_detach_count(0)
{
}

bool SimAttachment::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  for(auto &msg : NewMail) {
    const string key = msg.GetKey();
    bool handled = true;

    if(key == "UAV_SIM_ATTACHMENT")
      handled = handleMailAttachment(msg);
    else if(key == "PEARL_NAV_X")
      handled = handleMailDouble(msg, m_pearl_x);
    else if(key == "PEARL_NAV_Y")
      handled = handleMailDouble(msg, m_pearl_y);
    else if(key == "PEARL_NAV_HEADING")
      handled = handleMailDouble(msg, m_pearl_heading);
    else if(key == "PEARL_NAV_SPEED")
      handled = handleMailDouble(msg, m_pearl_speed);
    else if(key != "APPCAST_REQ")
      handled = false;

    if(!handled && key != "APPCAST_REQ")
      reportRunWarning("Unhandled or invalid mail: " + key);
  }

  return true;
}

bool SimAttachment::OnConnectToServer()
{
  registerVariables();
  return true;
}

bool SimAttachment::Iterate()
{
  AppCastingMOOSApp::Iterate();

  if(m_attachment_requested && !m_attached && !m_detaching && poseFresh()) {
    Notify("USM_ENABLED", "false");
    m_attached = true;
    ++m_attach_count;
    reportEvent("UAV attached to PEARL simulation pose");
  }

  if(m_attached)
    postAttachedPose();

  // Reset the dormant simulator at the platform pose before returning
  // navigation ownership to it on the following iteration.
  if(!m_attachment_requested && m_attached && !m_detaching) {
    Notify("USM_RESET", resetSpec());
    m_detaching = true;
  }
  else if(m_detaching) {
    Notify("USM_ENABLED", "true");
    m_attached = false;
    m_detaching = false;
    ++m_detach_count;
    reportEvent("UAV detached from PEARL simulation pose");
  }

  postState();
  AppCastingMOOSApp::PostReport();
  return true;
}

bool SimAttachment::OnStartUp()
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

    if(param == "input_max_age")
      handled = setPosDoubleOnString(m_input_max_age, value);

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();
  return true;
}

void SimAttachment::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("UAV_SIM_ATTACHMENT", 0);
  Register("PEARL_NAV_X", 0);
  Register("PEARL_NAV_Y", 0);
  Register("PEARL_NAV_HEADING", 0);
  Register("PEARL_NAV_SPEED", 0);
}

bool SimAttachment::handleMailDouble(const CMOOSMsg &msg, TimedValue &target)
{
  if(!msg.IsDouble() || !std::isfinite(msg.GetDouble()))
    return false;

  target.value = msg.GetDouble();
  target.time = MOOSTime();
  return true;
}

bool SimAttachment::handleMailAttachment(const CMOOSMsg &msg)
{
  bool requested = false;
  if(msg.IsDouble())
    requested = (msg.GetDouble() != 0.0);
  else if(msg.IsString()) {
    if(!setBooleanOnString(requested, msg.GetString()))
      return false;
  }
  else
    return false;

  m_attachment_requested = requested;
  return true;
}

bool SimAttachment::isFresh(const TimedValue &value) const
{
  return value.time >= 0.0 && (MOOSTime() - value.time) <= m_input_max_age;
}

bool SimAttachment::poseFresh() const
{
  return isFresh(m_pearl_x) && isFresh(m_pearl_y) &&
         isFresh(m_pearl_heading);
}

void SimAttachment::postAttachedPose()
{
  Notify("NAV_X", m_pearl_x.value);
  Notify("NAV_Y", m_pearl_y.value);
  Notify("NAV_HEADING", m_pearl_heading.value);
  Notify("NAV_SPEED", isFresh(m_pearl_speed) ? max(0.0, m_pearl_speed.value) : 0.0);
  Notify("NAV_ALTITUDE", 0.0);
}

void SimAttachment::postState()
{
  const string state = stateString();
  if(state != m_last_state || (MOOSTime() - m_last_state_post) >= 1.0) {
    Notify("UAV_SIM_ATTACHMENT_STATE", state);
    Notify("UAV_SIM_ATTACHED", m_attached ? 1.0 : 0.0);
    m_last_state = state;
    m_last_state_post = MOOSTime();
  }
}

string SimAttachment::resetSpec() const
{
  return "x=" + doubleToStringX(m_pearl_x.value, 3) +
         ",y=" + doubleToStringX(m_pearl_y.value, 3) +
         ",heading=" + doubleToStringX(m_pearl_heading.value, 2) +
         ",speed=0";
}

string SimAttachment::stateString() const
{
  if(m_detaching)
    return "DETACHING";
  if(m_attached)
    return poseFresh() ? "ATTACHED" : "ATTACHED_POSE_STALE";
  if(m_attachment_requested)
    return "WAITING_FOR_PEARL_POSE";
  return "DETACHED";
}

bool SimAttachment::buildReport()
{
  m_msgs << "Configuration" << endl;
  m_msgs << "  Input max age: " << doubleToStringX(m_input_max_age, 1)
         << " s" << endl;
  m_msgs << "State: " << stateString() << endl;
  m_msgs << "Attach/detach count: " << m_attach_count << "/"
         << m_detach_count << endl << endl;

  ACTable table(3);
  table << "PEARL input | Value | Age (s)";
  table.addHeaderLines();
  table << "X" << doubleToStringX(m_pearl_x.value, 2)
        << (m_pearl_x.time < 0 ? "n/a" : doubleToStringX(MOOSTime() - m_pearl_x.time, 2));
  table << "Y" << doubleToStringX(m_pearl_y.value, 2)
        << (m_pearl_y.time < 0 ? "n/a" : doubleToStringX(MOOSTime() - m_pearl_y.time, 2));
  table << "Heading" << doubleToStringX(m_pearl_heading.value, 1)
        << (m_pearl_heading.time < 0 ? "n/a" : doubleToStringX(MOOSTime() - m_pearl_heading.time, 2));
  table << "Speed" << doubleToStringX(m_pearl_speed.value, 2)
        << (m_pearl_speed.time < 0 ? "n/a" : doubleToStringX(MOOSTime() - m_pearl_speed.time, 2));
  m_msgs << table.getFormattedString();
  return true;
}
