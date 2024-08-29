/************************************************************/
/*    NAME: jmhamel                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenPath.h                                          */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#ifndef GenPath_HEADER
#define GenPath_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include "XYSegList.h"
class GenPath : public AppCastingMOOSApp
{
 public:
   GenPath();
   ~GenPath();

 protected: // Standard MOOSApp functions to overload  
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();

 protected: // Standard AppCastingMOOSApp function to overload 
   bool buildReport();

 protected:
   void registerVariables();

 private: // Configuration variables
    double m_xval;
    double m_yval;
    double m_lines;
    double m_cval;
    double m_xmidpoint;      //
    bool assign_by_region;
    bool end_of_list;
    bool m_assignment;
    // std::string x;
    // std::string y;
    // std::string c;
    std::vector<std::string> vnames;
    //double 
    std::vector<std::string> m_points;
    unsigned int m_count;
    unsigned int h_count;
    unsigned int ho_count;
    bool m_first_reading_x;
    bool m_first_reading_y;
    double m_start_x;
    double m_start_y;
    double m_visit_radius;
    bool m_regen;
    double r_count;
    XYSegList ordered_seglist;
    XYSegList visited_seglist;
    XYSegList repeat_seglist;
    XYSegList reordered_seglist;

 private: // State variables
};

#endif 
