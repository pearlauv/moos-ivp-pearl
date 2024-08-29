/************************************************************/
/*    NAME: jmhamel                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenPath.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "GenPath.h"
#include "XYPoint.h"


using namespace std;

//---------------------------------------------------------
// Constructor()

GenPath::GenPath()
{
  m_xval = 0;
  m_yval = 0;
  m_cval = 0;
  m_xmidpoint = 87.5;
  m_assignment = true;
  m_count = 0;
  m_start_x = 0;
  m_start_y = 0;
  m_visit_radius = 3;
  m_regen = false;
  r_count = 0;
}

//---------------------------------------------------------
// Destructor

GenPath::~GenPath()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool GenPath::OnNewMail(MOOSMSG_LIST &NewMail)
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

    if (key == "VISIT_POINT")
    {
      m_points.push_back(msg.GetAsString());
      if (msg.GetAsString() == "lastpoint")
      {
        end_of_list = true;
      }
    }

    else if (key == "NAV_X")
    {
      if (m_first_reading_x == false)
      {
        m_first_reading_x = true;
      }
      m_start_x = msg.GetDouble();
      cout << "nav_x: " << m_start_x << endl;
    }

    else if (key == "NAV_Y")
    {
      if (m_first_reading_y == false)
      {
        m_first_reading_y = true;
      }
      m_start_y = msg.GetDouble();
      cout << "nav_y: " << m_start_x << endl;
    }
    
    else if (key == "GENPATH_REGENERATE")
    {
      if(msg.GetAsString() == "true")
      {
        m_regen = true;
      }
    }

    else if (key != "APPCAST_REQ") // handled by AppCastingMOOSApp
      reportRunWarning("Unhandled Mail: " + key);
  }

  return (true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool GenPath::OnConnectToServer()
{
  registerVariables();
  return (true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool GenPath::Iterate()
{
  AppCastingMOOSApp::Iterate();
  // Do your thing here!
  Notify("READY_TO_RECEIVE", "true");

  XYSegList my_seglist; // pulled from psuedo code

  //XYSegList visited_seglist;
  //XYSegList repeat_seglist;
  //XYSegList reordered_seglist;
  XYPoint current_point(m_start_x, m_start_y);

  if (end_of_list == true)
  {
    for (int i = 1; i < m_points.size()-1; i++)
    {
      // dist to x & y values
      string point = m_points[i];
      string xval = tokStringParse(point, "x", ',', '=');
      string yval = tokStringParse(point, "y", ',', '=');
      // string cval = tokStringParse(point, "unique_id", ',', '=');

      double x = stod(xval);
      double y = stod(yval);
      // double c = stod(cval);

      my_seglist.add_vertex(x, y);
    }
    // greedy for closest point
    int og_size = my_seglist.size();

    for (int k = 0; k < og_size; k++) 
    {
      int close_vertex_index = my_seglist.closest_vertex(current_point.x(), current_point.y());
      current_point = my_seglist.get_point(close_vertex_index);
      ordered_seglist.add_vertex(current_point.x(), current_point.y());
      //my_seglist.delete_vertex(close_vertex_index); // removes
      my_seglist.delete_vertex(current_point.x(),current_point.y()); // removes
      m_count++;
    }

    string update_str = "points = ";
    update_str += ordered_seglist.get_spec();
    Notify("UPDATES_VAR", update_str); // UPDATES_VAR depends on your config
    Notify("SURVEYING", "true");
    Notify("LOITER", "false");

    end_of_list = false;
  }
  
  if (end_of_list == false && m_regen == false) 
  {
    vector<XYPoint> deletePoints;
    for(int j=0; j < ordered_seglist.size(); j++)   //check to see if radius is met
    {
      double distx = m_start_x - ordered_seglist.get_vx(j);
      double disty = m_start_y - ordered_seglist.get_vy(j);
      double dist = sqrt((distx*distx)+(disty*disty));
      //repeat_seglist.add_vertex(ordered_seglist.get_vx(j), ordered_seglist.get_vy(j));
 
      if(dist < m_visit_radius) 
      {
       //visited_seglist.add_vertex(ordered_seglist.get_vx(j), ordered_seglist.get_vy(j));
        //repeat_seglist.delete_vertex(ordered_seglist.get_vx(j), ordered_seglist.get_vy(j));
        //ordered_seglist.delete_vertex(j);
        deletePoints.push_back(XYPoint(ordered_seglist.get_vx(j),ordered_seglist.get_vy(j)));
      }
    } 
  for(int i=0;i<deletePoints.size();i++)
    {
    ordered_seglist.delete_vertex(deletePoints[i].get_vx(),deletePoints[i].get_vy());
    }
  if(ordered_seglist.size() == 0)
    {
      Notify("SURVEYING", "false");
      //Notify("DONE", "true");
    }
  }

  
 

  if(m_regen == true)  //loop to reorder "repeat" list for points needed to be revisited
  {
    // for (int i = 0; i < repeat_seglist.size(); i++) 
    // {
    //   int close_vertex_index = repeat_seglist.closest_vertex(current_point.x(), current_point.y());
    //   current_point = repeat_seglist.get_point(close_vertex_index);
    //   reordered_seglist.add_vertex(current_point.x(), current_point.y());
    //   repeat_seglist.delete_vertex(close_vertex_index); // removes
    //   r_count++;
    // }
 //update string of missed points
  string update_str = "points = ";
  update_str += ordered_seglist.get_spec();
  Notify("UPDATES_VAR", update_str); // UPDATES_VAR depends on your config
  Notify("SURVEYING", "true");
  Notify("STATION_KEEP", "false");

  m_regen = false;

  }

  AppCastingMOOSApp::PostReport();
  return (true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool GenPath::OnStartUp()
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

void GenPath::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  // Register("FOOBAR", 0);
  Register("VISIT_POINT", 0); // specific for each vessel
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
  Register("GENPATH_REGENERATE",0);
}

//------------------------------------------------------------
// Procedure: buildReport()

bool GenPath::buildReport()
{
    m_msgs << "============================================" << endl;
  m_msgs << "File:  GenPath.cpp                         " << endl;
  m_msgs << "============================================" << endl;
  m_msgs << endl;

  ACTable actab(5);
  actab << "end_of_list | ordered_seglist | r_count | m_regen | xx";
  actab.addHeaderLines();
  actab << end_of_list << to_string(m_count) << to_string(r_count) << m_regen << "";
  m_msgs << actab.getFormattedString();
  m_msgs << endl;
  m_msgs << endl;

  m_msgs << "============================================" << endl;
  m_msgs << "File:  GenPath.cpp                        " << endl;
  m_msgs << "============================================" << endl;
  m_msgs << "List size: " << to_string(m_points.size());
  return (true);
}
