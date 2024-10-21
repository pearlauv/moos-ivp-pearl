/************************************************************/
/*    NAME:                                               */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: BlueRoboticsPing.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "BlueRoboticsPing.h"
#include "abstract-link/abstract-link.h"

using namespace std;

//static Ping1d m_device;

auto m_port = AbstractLink::openUrl("serial:/dev/ttyUSB0:115200");
Ping1d m_device = Ping1d(*m_port.get());


//---------------------------------------------------------
// Constructor

BlueRoboticsPing::BlueRoboticsPing()
{
  cout << "constructor" << endl;
  Notify("DEBUG", "constructor");
  // connect to device
}

//---------------------------------------------------------
// Destructor

//BlueRoboticsPing::~BlueRoboticsPing()
//{
//}

//---------------------------------------------------------
// Procedure: OnNewMail

bool BlueRoboticsPing::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();

    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString();
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();

    if(key == "SPEED_OF_SOUND")
      m_device.set_speed_of_sound(dval);
    else if(key != "APPCAST_REQ") // handled by AppCastingMOOSApp
      reportRunWarning("Unhandled Mail: " + key);
   }

   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool BlueRoboticsPing::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool BlueRoboticsPing::Iterate()
{
  AppCastingMOOSApp::Iterate();


  if (m_profile) {
    m_device.request(1300);
    m_distance = m_device.profile_data.distance;
    m_confidence = m_device.profile_data.confidence;

    int profile_length = m_device.profile_data.profile_data_length;
    uint8_t *ptr = m_device.profile_data.profile_data;

    m_profile_str = "";

    for (int i=0; i < profile_length; i++) {
      m_profile_str += to_string(static_cast<unsigned>(*ptr)) + " ";
      ptr++;
    }
    
    Notify("PING_DISTANCE", m_distance);
    Notify("PING_CONFIDENCE", m_confidence);
    Notify("PING_PROFILE", m_profile_str);
  } else {
    m_device.request(1212);
    m_distance = m_device.distance_data.distance;
    m_confidence = m_device.distance_data.confidence;
    Notify("PING_DISTANCE", m_distance);
    Notify("PING_CONFIDENCE", m_confidence);
  }
    
  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool BlueRoboticsPing::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  //auto m_port = AbstractLink::openUrl("serial:/dev/ttyUSB0:115200");
  //m_device = Ping1d(*m_port.get());
  

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

    bool handled = false;
    if(param == "speed_of_sound") {
      m_speed_of_sound = stoi(value);
      cout << "speed config" << endl;
      handled = true;
    }
    else if(param == "ping_interval") {
      m_ping_interval = stoi(value);
      handled = true;
    }
    else if(param == "profile") {
      if (value == "1")
	m_profile = true;
      handled = true;
    }
    else if(param == "scan_start") {
      m_scan_start = stoi(value);
      handled = true;
    }
    else if(param == "scan_length") {
      m_scan_length = stoi(value);
      handled = true;
    }
    else if(param == "manual") {
      if (value == "1")
	m_auto = 0;
      handled = true;
    }
    
    if(!handled)
      reportUnhandledConfigWarning(orig);

  }

  m_device.initialize(m_ping_interval);
  m_device.set_mode_auto(m_auto);
  m_device.set_ping_enable(1);
  m_device.set_speed_of_sound(m_speed_of_sound);
  m_device.set_range(m_scan_start, m_scan_length);

  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables

void BlueRoboticsPing::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("SPEED_OF_SOUND", 0);
}


//------------------------------------------------------------
// Procedure: buildReport()

bool BlueRoboticsPing::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "File:                                       " << endl;
  m_msgs << "============================================" << endl;

  m_msgs << "DISTANCE: " << m_distance << endl;
  m_msgs << "CONFIDENCE: " << m_confidence << endl;
  m_msgs << "PROFILE: " << m_profile_str << endl;

  return(true);
}




