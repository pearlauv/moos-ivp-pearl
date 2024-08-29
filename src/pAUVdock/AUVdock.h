/************************************************************/
/*    NAME: jmhamel                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: AUVdock.h                                          */
/*    DATE: August 2, 2023                         */
/************************************************************/

#ifndef AUVdock_HEADER
#define AUVdock_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include "XYSegList.h"
#include <map>
#include <unordered_map>
#include <set>
class AUVdock : public AppCastingMOOSApp
{
 public:
   AUVdock();
   ~AUVdock();

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
    
    std::string vessel_type;
    // std::string y;
    // std::string c;
    std::vector<std::string> vnames;
    //double 
    //std::set<std::string> m_set_seen;
    std::unordered_map<std::string,XYPoint> m_points_to_visit;
    std::set<std::string>m_points_visited;
    std::vector<std::string> m_points;
    std::vector<std::string> m_alert;
    std::vector<XYPoint> m_received;
    std::vector<std::string> m_found;
    std::vector<XYPoint> m_search;
    unsigned int m_auv_count;
    unsigned int m_docked_count;
    unsigned int ho_count;
    bool m_first_reading_abe;
    bool m_first_reading_pearl_x;
    bool m_first_reading_pearl_y;
    double m_auv_x;
    double m_auv_y;
    double m_asv_x;
    double m_asv_y;
    double r;
    std::string auvx;
    std::string auvy;
    double m_visit_radius;
    bool m_regen;
    // double r_count;
    double m_current_list;
    double m_previous_list;
    XYSegList ordered_seglist;
    XYSegList visited_seglist;
    XYSegList repeat_seglist;
    XYSegList reordered_seglist;

 private: // State variables
};

#endif 
