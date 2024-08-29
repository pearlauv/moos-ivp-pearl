/************************************************************/
/*    NAME: Jane Doe                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: Odometry.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "Odometry.h"
#include <cmath>

using namespace std;

//---------------------------------------------------------
// Constructor()

Odometry::Odometry()
{

  m_first_reading = true;
  m_first_reading_x = true;
  m_first_reading_y = true;
  m_current_x =0;
  m_current_y =0;
  m_previous_x =0;
  m_previous_y =0;
  m_total_distance =0;
 
}

//---------------------------------------------------------
// Destructor

Odometry::~Odometry()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool Odometry::OnNewMail(MOOSMSG_LIST &NewMail)
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

     if(key == "FOO") 
       cout << "great!";
       
     else if(key == "NAV_X") 
     {
     	
     	m_previous_x = m_current_x;
     	m_current_x = msg.GetDouble();
     	m_first_reading_x = false;
     	cout << "NAV_X detected" << endl;
     	//cout << "first reading:" << m_first_reading_x << endl;
     	//cout << "first reading:" << m_first_reading_x << endl;
     	//cout << "first reading:" << m_first_reading_x << endl;
     }
     
     else if(key == "NAV_Y") 
     {
     	cout << "NAV_Y detected" << endl;
     	m_previous_y = m_current_y;
     	m_current_y = msg.GetDouble();
     	m_first_reading_y = false;
     }
       

     else if(key != "APPCAST_REQ") // handled by AppCastingMOOSApp
       reportRunWarning("Unhandled Mail: " + key);
   }
	
   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool Odometry::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool Odometry::Iterate()
{
  AppCastingMOOSApp::Iterate();
  if (!(m_first_reading_x) && !(m_first_reading_y))
  {
  	m_first_reading = false;
  	cout << "not first reading for both variables" << endl;
  }
  
  if (!m_first_reading)
  {
  	cout << "total distance="  << m_total_distance << endl;
  	double m_distance_step = sqrt(pow((m_current_x - m_previous_x),2) + pow((m_current_y - m_previous_y),2));
  	cout << "step="  << m_distance_step << endl;
  	m_total_distance = m_total_distance + m_distance_step;
    // m_previous_x = m_current_x;
    // m_previous_y = m_current_y;
  	cout << "total distance updated="  << m_total_distance << endl;
  }
  
  Notify("ODOMETRY_DIST", m_total_distance);
  cout << "odometry="  << m_total_distance << endl;
  
  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool Odometry::OnStartUp()
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

    bool handled = false;
    if(param == "foo") {
      handled = true;
    }
    else if(param == "bar") {
      handled = true;
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);

  }
  
  registerVariables();	
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void Odometry::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("FOOBAR", 0);
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
}


//------------------------------------------------------------
// Procedure: buildReport()

bool Odometry::buildReport() 
{
  m_msgs << "============================================" << endl;
  m_msgs << "File:                                       " << endl;
  m_msgs << "============================================" << endl;

  //ACTable actab(4);
  //actab << "Alpha | Bravo | Charlie | Delta";
  //actab.addHeaderLines();
  // actab << "one" << "two" << "three" << "four";
  //m_msgs << actab.getFormattedString();
  m_msgs << "Total distance travelled: " << m_total_distance << endl;

  return(true);
}




