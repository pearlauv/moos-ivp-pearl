/************************************************************/
/*    NAME: jmhamel                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: AUVdock.cpp                                        */
/*    DATE: August 2, 2023                           */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "AUVdock.h"
#include "XYPoint.h"
#include "XYFormatUtilsPoint.h"
#include <map>
#include <set>

using namespace std;

//---------------------------------------------------------
// Constructor()

AUVdock::AUVdock()
{
  m_xval = 0;
  m_yval = 0;
  m_cval = 0;
  m_xmidpoint = 87.5;
  m_assignment = true;
  // m_count = 0;
  //m_auv_x = 0;
  //m_auv_y = 0;
  m_visit_radius = 3;
  m_regen = false;
  // r_count = 0;
  m_current_list = 0;
  m_previous_list = 0;
  m_first_reading_pearl_x = false;
  m_first_reading_pearl_y = false;
  m_docked_count = 0;
  m_auv_count = 1;     //hardcode in 1 auv
}

//---------------------------------------------------------
// Destructor

AUVdock::~AUVdock()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool AUVdock::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for (p = NewMail.begin(); p != NewMail.end(); p++)
  {
    CMOOSMsg &msg = *p;
    string key = msg.GetKey();

#if 0 // Keep these around just for template
    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString(); 
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif

//current node report handeling doesn't ID which type of vessel. Will try to dock any vessel/will get confused if multiple vessels exist. Need to make more robust for future revs. -jmh

    if (key == "NODE_REPORT")     //Subs for node reports
    {
      //vessel_type = tokStringParse(msg.GetAsString(), "TYPE", ',', '=');
      //if (vessel_type == "uuv")
      //{
        auvx = tokStringParse(msg.GetAsString(), "X", ',', '=');
        m_auv_x = stod(auvx);
        auvy = tokStringParse(msg.GetAsString(), "Y", ',', '=');
        m_auv_y = stod(auvy);
        cout << "NODE_REPORT received" << endl;
        cout << "nav_x_auv: " << m_auv_x << endl;
        cout << "nav_y_auv: " << m_auv_y << endl;
      //}
    }

    else if (key == "NAV_X")     
    {
      if (m_first_reading_pearl_x == false)
      {
        m_first_reading_pearl_x = true;
      }
      m_asv_x = msg.GetDouble();
      cout << "nav_x_pearl: " << m_asv_x << endl;
    }

    else if (key == "NAV_Y")
    {
      if (m_first_reading_pearl_y == false)
      {
        m_first_reading_pearl_y = true;
      }
      m_asv_y = msg.GetDouble();
      cout << "nav_y_pearl: " << m_asv_y << endl;
    }

    else if (key != "APPCAST_REQ") // handled by AppCastingMOOSApp
      reportRunWarning("Unhandled Mail: " + key);
  }

  return (true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool AUVdock::OnConnectToServer()
{
  registerVariables();
  return (true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool AUVdock::Iterate()
{
  AppCastingMOOSApp::Iterate();
  
   
  if (m_auv_count != m_docked_count)
  {
    XYSegList seglist_to_visit;
    seglist_to_visit.insert_vertex(m_auv_x, m_auv_y);
    
    string update_str = "points = ";
    update_str += seglist_to_visit.get_spec();
    Notify("DOCK_RNDV", update_str); // UPDATES_VAR depends on your config
    r++;
    cout << update_str << endl;
  } 

  //update count if docked (same location = docked)
  if(m_asv_x == m_auv_x && m_asv_y == m_auv_y)
  {
    m_docked_count = 1;
  }

  AppCastingMOOSApp::PostReport();
  return (true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool AUVdock::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if (!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for (p = sParams.begin(); p != sParams.end(); p++)
  {
    string orig = *p;
    string line = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;
    if (param == "foo")
    {
      handled = true;
    }
    else if (param == "bar")
    {
      handled = true;
    }

    if (!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();
  return (true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void AUVdock::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  // Register("FOOBAR", 0);
  Register("NODE_REPORT", 0); // specific call for Node Reports
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
  //Register("FOUND_SWIMMER", 0);
}

//------------------------------------------------------------
// Procedure: buildReport()

bool AUVdock::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "File:  AUVdock.cpp                         " << endl;
  m_msgs << "============================================" << endl;
  m_msgs << endl;

  ACTable actab(5);
  actab << "m_docked_count | m_auv_count | m_auv_x | m_auv_y | runcount";
  actab.addHeaderLines();
  actab << m_docked_count << to_string(m_auv_count) << to_string(m_auv_x) << to_string(m_auv_x) << r;
  m_msgs << actab.getFormattedString();
  m_msgs << endl;
  m_msgs << endl;

  m_msgs << "============================================" << endl;
  m_msgs << "File:  AUVdock.cpp                         " << endl;
  m_msgs << "============================================" << endl;
  m_msgs << "List size: " << to_string(m_points_to_visit.size()) << endl;
  m_msgs << "visited size: " << to_string(m_points_visited.size()) << endl;
  return (true);
}
