/************************************************************/
/*    NAME: Tyler                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: M300Health.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "M300Health.h"

using namespace std;

//---------------------------------------------------------
// Constructor

M300Health::M300Health()
{
  m_stale_time = 3.0;
  m_max_heading_delta = 5.0;
  m_low_batt_thresh = 0.0;
  m_full_batt_thresh = 0.0;
  
  m_nav_heading = 0.0;
  m_nav_heading_last_msg_time = 0.0;
  m_nav_heading_aux = "";
  
  m_gps_heading = 0.0;
  m_gps_heading_last_msg_time = 0.0;
  m_gps_heading_aux = "";
  
  m_imu_gps_heading_agreement = false;
  m_stale_nav = true;
  m_stale_gps = true;

  m_batt_voltage = 0.0;
  m_batt_voltage_last_msg_time = 0.0;
  m_stale_batt_voltage = true;

  m_time_average_length = 2; //seconds.
  m_speed_est_deviation_thresh = 0.3;
  m_deviation_running_ave = 0.0;
  m_number_of_faults_detected = 0;
  

}

//---------------------------------------------------------
// Destructor

M300Health::~M300Health()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool M300Health::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();
    double mtime = msg.GetTime();
    double dval  = msg.GetDouble();
    string aux   = msg.GetSourceAux();

#if 0 // Keep these around just for template
    string comm  = msg.GetCommunity();
    string sval  = msg.GetString(); 
    string msrc  = msg.GetSource();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif

    if(key == "NAV_HEADING") {
      m_nav_heading = dval;
      m_nav_heading_last_msg_time = mtime;
      m_nav_heading_aux = aux;
       
    } else if(key == "GPS_HEADING") {
      m_gps_heading = dval;
      m_gps_heading_last_msg_time = mtime;
      m_gps_heading_aux = aux;

    } else if(key == "M300_BATT_VOLTAGE") {
      m_batt_voltage = dval;
      m_batt_voltage_last_msg_time = mtime;

    } else if(key == "NAV_SPEED") {
      m_nav_speed_vals.push_back(dval);
      m_nav_speed_times.push_back( mtime);

    } else if(key == "NAV_SPEED_EST_AVE") {
      m_ave_nav_speed_vals.push_back(dval);
      m_ave_nav_speed_times.push_back( mtime);
      
    } else if(key != "APPCAST_REQ") // handled by AppCastingMOOSApp
       reportRunWarning("Unhandled Mail: " + key);
   }
	
   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool M300Health::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool M300Health::Iterate()
{
  AppCastingMOOSApp::Iterate();

  // Check that messages are not stale
  m_stale_nav = ( (MOOSTime() - m_nav_heading_last_msg_time) >= m_stale_time );
  m_stale_gps = ( (MOOSTime() - m_gps_heading_last_msg_time) >= m_stale_time );
  m_stale_batt_voltage = ( (MOOSTime() - m_stale_batt_voltage) >= m_stale_time );
  
  if (!m_stale_nav and !m_stale_gps) {
    m_imu_gps_heading_agreement = false;
    
    if ( abs(m_nav_heading - m_gps_heading) <= m_max_heading_delta )
      m_imu_gps_heading_agreement = true;
  }

  // Calculate the running averages
  // do this every time, since there is no guarantee when the next reading will
  // come.
  m_nav_speed_running_ave = 0.0;
  double count1 = 0.0;
  for (int i=0; i<m_nav_speed_vals.size();)  {
    if ( (MOOSTime() - m_nav_speed_times[i]) < m_time_average_length)  {
      m_nav_speed_running_ave = (( m_nav_speed_vals[i] +  m_nav_speed_running_ave * count1) / (count1 + 1.0) );
      count1 = count1 + 1.0;
      i++;
    } else {
      // delete this index in both vectors
      m_nav_speed_vals.erase(m_nav_speed_vals.begin() + i);
      m_nav_speed_times.erase(m_nav_speed_times.begin() + i);
	
    }
  }

  m_ave_nav_speed_running_ave = 0.0;
  double count2 = 0.0;
  for (int i=0; i<m_ave_nav_speed_vals.size();)  {
    if ( (MOOSTime() - m_ave_nav_speed_times[i]) < m_time_average_length)  {
      m_ave_nav_speed_running_ave = (( m_ave_nav_speed_vals[i] +  m_ave_nav_speed_running_ave * count2) / (count2 + 1.0) );
      count2 = count2 + 1.0;
      i++;
    } else {
      // delete this index in both vectors
      m_ave_nav_speed_vals.erase(m_ave_nav_speed_vals.begin() + i);
      m_ave_nav_speed_times.erase(m_ave_nav_speed_times.begin() + i);
	
    }
  }


  // Compare the running averages to see if something is wrong;
  if ( (count1 > 0.0) and (count2 > 0.0) ) {
    
    Notify("NAV_SPEED_EST_RUNNING_AVE", m_ave_nav_speed_running_ave);
    
    m_deviation_running_ave = m_nav_speed_running_ave - m_ave_nav_speed_running_ave;
    if ( std::fabs(m_deviation_running_ave) > m_speed_est_deviation_thresh) {
      Notify("FAULT_DETECTED", "True");
      m_number_of_faults_detected++;
    }
  }

  
  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool M300Health::OnStartUp()
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
    if (param == "staletime") {
      handled = setPosDoubleOnString(m_stale_time, value);
   
    } else if (param == "maxheadingdelta") {
      handled = setPosDoubleOnString(m_max_heading_delta, value);
      
    } else if (param == "lowbattthresh")  {
      handled = setPosDoubleOnString(m_low_batt_thresh, value);
      
    } else if (param == "fullbattthresh") {
      handled = setPosDoubleOnString(m_full_batt_thresh, value);

    } else if (param == "timeaveragelength") {
      handled = setPosDoubleOnString(m_time_average_length, value);

    } else if (param == "speedfaultthreshold") {
      handled = setPosDoubleOnString(m_speed_est_deviation_thresh, value);
    }

    
    if(!handled)
      reportUnhandledConfigWarning(orig);

  }
  
  registerVariables();	
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables

void M300Health::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("NAV_HEADING", 0);
  Register("GPS_HEADING", 0);
  Register("M300_BATT_VOLTAGE", 0);
  Register("NAV_SPEED", 0);
  Register("NAV_SPEED_EST_AVE",0);

}


//------------------------------------------------------------
// Procedure: buildReport()

bool M300Health::buildReport() 
{
  m_msgs << "============================================" << endl;
  m_msgs << "File: iM300Health.cpp                       " << endl;
  m_msgs << "============================================" << endl;
  m_msgs << "                                            " << endl;

  if (m_imu_gps_heading_agreement and !m_stale_nav and !m_stale_gps)
    m_msgs << "IMU and GPS heading within "  << m_max_heading_delta  << " degrees."<< endl;
  else if (!m_stale_nav and !m_stale_gps)
    m_msgs << "Warning: IMU and GPS differ by " << abs( m_nav_heading - m_gps_heading ) << " degrees."<< endl;
  else if (m_stale_nav or m_stale_gps)
    m_msgs << "Warning: IMU or GPS messages is stale, see below." << endl;

  m_msgs << "--------------------------------------------" << endl;
  
  ACTable actab(5);
  actab << "Source | Value (deg) | Source Msg | Time Since Last Msg | Status ";
  actab.addHeaderLines();
  if (!m_stale_nav)
    actab << "IMU" << m_nav_heading << m_nav_heading_aux << (MOOSTime() - m_nav_heading_last_msg_time) << "       ";
  else
    actab << "IMU" << m_nav_heading << m_nav_heading_aux << (MOOSTime() - m_nav_heading_last_msg_time) << " STALE ";
  
  if (!m_stale_gps)
    actab << "GPS" << m_gps_heading << m_gps_heading_aux << (MOOSTime() - m_gps_heading_last_msg_time) << "       ";
  else
    actab << "GPS" << m_gps_heading << m_gps_heading_aux << (MOOSTime() - m_gps_heading_last_msg_time) << " STALE ";
  
  m_msgs << actab.getFormattedString();
  m_msgs << endl;
  
  m_msgs << "                                                      " << endl;

  if (!m_stale_batt_voltage)
    m_msgs << "Battery Status: Warning! Stale battery msg.         " << endl;
  if ( m_batt_voltage < m_low_batt_thresh)
    m_msgs << "Battery Status: Warning! Critically low level!      " << endl;
  else
    m_msgs << "Battery Status: Battery ok.                         " << endl;              

  m_msgs << "------------------------------------------------------" << endl;

  ACTable actab2(3);
  actab2 << " Low Threshold (V) | Current Reading (V) | Full Threshold (V) ";
  actab2.addHeaderLines();
  actab2 << m_low_batt_thresh << m_batt_voltage << m_full_batt_thresh;
  m_msgs << actab2.getFormattedString();
  m_msgs << endl;

  m_msgs << "                                                      " << endl;
  m_msgs << "------------------------------------------------------" << endl;
  m_msgs << "  Faults Detected: " << m_number_of_faults_detected << "             " << endl;
  m_msgs << "  Deviation of Estimate From Truth = " << m_deviation_running_ave << " m/s               " << endl;
  m_msgs << "                       (threshold is " << m_speed_est_deviation_thresh << "m/s)          " << endl;
  
  
  return(true);
}




