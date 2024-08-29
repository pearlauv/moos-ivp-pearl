/************************************************************/
/*    NAME: Theo Schafer                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: TargetCPA.h                                          */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#ifndef TargetCPA_HEADER
#define TargetCPA_HEADER

#include <cmath>
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include<iostream>
#include<string>

using namespace std;


class TargetCPA : public AppCastingMOOSApp
{
 public:
   TargetCPA();
   ~TargetCPA();

 protected: // Standard MOOSApp functions to overload  
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();

 protected: // Standard AppCastingMOOSApp function to overload 
   bool buildReport();

 protected:
   void registerVariables();
   bool handleMailNodeReport(string report);

 private: // Configuration variables

 private: // State variables
 
  double m_current_x =0;
 double m_current_y =0;
 double m_target_temp_x = 0;
 double m_target_temp_y = 0;
 double m_target_x =0;
 double m_target_y =0;
 double m_cpa = 0; 
 string m_TS_name ;
 
};

#endif 
