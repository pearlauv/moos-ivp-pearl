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
    m_initially_attached(false),
    m_attachment_requested(false),
    m_attached(false),
    m_detaching(false),
    m_detach_x(0.0),
    m_detach_y(0.0),
    m_detach_heading(0.0),
    m_last_state(""),
    m_last_state_post(-1.0),
    m_last_enable_post(-1.0),
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

    if(key == "SIM_ATTACHMENT_REQUEST")
      handled = handleMailAttachment(msg);
    else if(key == "ATTACHMENT_NAV_X")
      handled = handleMailDouble(msg, m_source_x);
    else if(key == "ATTACHMENT_NAV_Y")
      handled = handleMailDouble(msg, m_source_y);
    else if(key == "ATTACHMENT_NAV_HEADING")
      handled = handleMailDouble(msg, m_source_heading);
    else if(key == "ATTACHMENT_NAV_SPEED")
      handled = handleMailDouble(msg, m_source_speed);
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

  // Reassert detached ownership so an app restart cannot leave the local
  // simulator disabled by an earlier attachment session.
  if(!m_attachment_requested && !m_attached && !m_detaching &&
     ((m_last_enable_post < 0.0) ||
      ((MOOSTime() - m_last_enable_post) >= 1.0))) {
    Notify("USM_ENABLED", "true");
    m_last_enable_post = MOOSTime();
  }

  if(m_attachment_requested && !m_attached && !m_detaching && poseFresh()) {
    Notify("USM_ENABLED", "false");
    m_attached = true;
    ++m_attach_count;
    reportEvent("Local simulator attached to external pose");
  }

  if(m_attached && !m_detaching)
    postAttachedPose();

  // Capture one handoff pose and reset the dormant simulator before returning
  // navigation ownership to it on the following iteration.
  if(!m_attachment_requested && m_attached && !m_detaching) {
    m_detach_x = m_source_x.value;
    m_detach_y = m_source_y.value;
    m_detach_heading = m_source_heading.value;
    Notify("USM_RESET", resetSpec(m_detach_x, m_detach_y,
                                  m_detach_heading));
    m_detaching = true;
  }
  else if(m_detaching) {
    postPose(m_detach_x, m_detach_y, m_detach_heading, 0.0);
    Notify("USM_ENABLED", "true");
    m_last_enable_post = MOOSTime();
    m_attached = false;
    m_detaching = false;
    ++m_detach_count;
    reportEvent("Local simulator detached from external pose");
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
    else if(param == "initially_attached")
      handled = setBooleanOnString(m_initially_attached, value);

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  m_attachment_requested = m_initially_attached;

  registerVariables();
  return true;
}

void SimAttachment::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("SIM_ATTACHMENT_REQUEST", 0);
  Register("ATTACHMENT_NAV_X", 0);
  Register("ATTACHMENT_NAV_Y", 0);
  Register("ATTACHMENT_NAV_HEADING", 0);
  Register("ATTACHMENT_NAV_SPEED", 0);
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
  if(msg.IsDouble()) {
    if(!std::isfinite(msg.GetDouble()))
      return false;
    requested = (msg.GetDouble() != 0.0);
  }
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
  return isFresh(m_source_x) && isFresh(m_source_y) &&
         isFresh(m_source_heading);
}

void SimAttachment::postAttachedPose()
{
  const double speed = isFresh(m_source_speed) ?
    max(0.0, m_source_speed.value) : 0.0;
  postPose(m_source_x.value, m_source_y.value,
           m_source_heading.value, speed);
}

void SimAttachment::postPose(double x, double y, double heading, double speed)
{
  Notify("NAV_X", x);
  Notify("NAV_Y", y);
  Notify("NAV_HEADING", heading);
  Notify("NAV_SPEED", speed);
  Notify("NAV_ALTITUDE", 0.0);
}

void SimAttachment::postState()
{
  const string state = stateString();
  if(state != m_last_state || (MOOSTime() - m_last_state_post) >= 1.0) {
    Notify("SIM_ATTACHMENT_STATE", state);
    Notify("SIM_ATTACHMENT_ATTACHED", m_attached ? 1.0 : 0.0);
    m_last_state = state;
    m_last_state_post = MOOSTime();
  }
}

string SimAttachment::resetSpec(double x, double y, double heading) const
{
  return "x=" + doubleToStringX(x, 12) +
         ",y=" + doubleToStringX(y, 12) +
         ",heading=" + doubleToStringX(heading, 10) +
         ",speed=0";
}

string SimAttachment::stateString() const
{
  if(m_detaching)
    return "DETACHING";
  if(m_attached)
    return poseFresh() ? "ATTACHED" : "ATTACHED_POSE_STALE";
  if(m_attachment_requested)
    return "WAITING_FOR_ATTACHMENT_POSE";
  return "DETACHED";
}

bool SimAttachment::buildReport()
{
  m_msgs << "Configuration" << endl;
  m_msgs << "  Input max age: " << doubleToStringX(m_input_max_age, 1)
         << " s" << endl;
  m_msgs << "  Initially attached: " << boolToString(m_initially_attached)
         << endl;
  m_msgs << "Attachment requested: " << boolToString(m_attachment_requested)
         << endl;
  m_msgs << "State: " << stateString() << endl;
  m_msgs << "Attach/detach count: " << m_attach_count << "/"
         << m_detach_count << endl << endl;

  ACTable table(3);
  table << "Attachment input | Value | Age (s)";
  table.addHeaderLines();
  table << "X" << doubleToStringX(m_source_x.value, 2)
        << (m_source_x.time < 0 ? "n/a" : doubleToStringX(MOOSTime() - m_source_x.time, 2));
  table << "Y" << doubleToStringX(m_source_y.value, 2)
        << (m_source_y.time < 0 ? "n/a" : doubleToStringX(MOOSTime() - m_source_y.time, 2));
  table << "Heading" << doubleToStringX(m_source_heading.value, 1)
        << (m_source_heading.time < 0 ? "n/a" : doubleToStringX(MOOSTime() - m_source_heading.time, 2));
  table << "Speed" << doubleToStringX(m_source_speed.value, 2)
        << (m_source_speed.time < 0 ? "n/a" : doubleToStringX(MOOSTime() - m_source_speed.time, 2));
  m_msgs << table.getFormattedString();
  return true;
}
