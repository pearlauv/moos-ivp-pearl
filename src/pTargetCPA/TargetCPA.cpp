/************************************************************/
/*    NAME: Theo Schafer                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: TargetCPA.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "TargetCPA.h"
#include <cmath>

using namespace std;

//---------------------------------------------------------
// Constructor()

TargetCPA::TargetCPA()
{
  m_current_x =0;
  m_current_y =0;
  m_target_temp_x = 0;
  m_target_temp_y = 0;
  m_target_x =0;
  m_target_y =0;
  m_cpa = 0; 
  m_TS_name ;
}

//---------------------------------------------------------
// Destructor

TargetCPA::~TargetCPA()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool TargetCPA::OnNewMail(MOOSMSG_LIST &NewMail)
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
     	
     	
     	m_current_x = msg.GetDouble();
     	
     	cout << "NAV_X detected" << endl;
     	
     }
     
     else if(key == "NAV_Y") 
     {
     	cout << "NAV_Y detected" << endl;
     	
     	m_current_y = msg.GetDouble();
     	
     }
     
     else if(key == "NODE_REPORT") 
     {
     	cout << "NODE_REPORT detected" << endl;
     	
     	handleMailNodeReport(msg.GetString());
     	
     }
     

     else if(key != "APPCAST_REQ") // handled by AppCastingMOOSApp
       reportRunWarning("Unhandled Mail: " + key);
   }
	
   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool TargetCPA::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool TargetCPA::Iterate()
{
  AppCastingMOOSApp::Iterate();
  // Do your thing here!
  
  if (m_TS_name == "abe")
  {
  m_cpa = sqrt(pow((m_current_x-m_target_temp_x),2)+pow((m_current_y-m_target_temp_y),2));
  Notify("TARGET_CPA", m_cpa);
  }
  
  
  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool TargetCPA::OnStartUp()
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

void TargetCPA::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  // Register("FOOBAR", 0);
  Register("NODE_REPORT",0);
   Register("NAV_X", 0);
  Register("NAV_Y", 0);
}


//------------------------------------------------------------
// Procedure: buildReport()

bool TargetCPA::buildReport() 
{
  m_msgs << "============================================" << endl;
  m_msgs << "File:                                       " << endl;
  m_msgs << "============================================" << endl;

  m_msgs << "CPA to Abe: " << m_cpa << endl;

  return(true);
}


bool TargetCPA::handleMailNodeReport(string report)
  {
    vector<string> str_vector = parseString(report, ',');
    for(unsigned int i=0; i<str_vector.size(); i++) {
      string param = biteStringX(str_vector[i], '=');
      string value = str_vector[i];

      if(param == "NAME")
      {
      m_TS_name = value;
      }
        
      else if(param == "X")
        m_target_temp_x = stod(value); 
        
      else if(param == "Y")
      
        m_target_temp_y = stod(value); 
        
}
     return(true);
   
  }

