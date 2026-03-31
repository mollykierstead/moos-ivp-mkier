/************************************************************/
/*    NAME: Molly Kierstead                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenPath.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "GenPath.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

GenPath::GenPath()
{
  m_build_path = false;
  m_update_var = "WPT_UPDATE";
  m_visit_points.clear();
  v_name1 = "";
  v_name2 = "";
  m_nav_x = 0;
  m_nav_y = 0;
  m_visit_radius = 5.0;
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

    if (key == "VISIT_POINT")
    {
      string sval = msg.GetString();
      m_visit_points.push_back(sval);

      if (sval == "end")
        m_build_path = true;
    }
    else if (key == "NAV_X")
    {
      m_nav_x = msg.GetDouble();
    }
    else if (key == "NAV_Y")
    {
      m_nav_y = msg.GetDouble();
    }
    else if (key == "GENPATH_REGENERATE")
    {
      m_build_path = true;
    }
    else if (key != "APPCAST_REQ")
    {
      reportRunWarning("Unhandled Mail: " + key);
    }
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
  Notify("READY_TO_RECEIVE", "true");

  m_points_x.clear();
  m_points_y.clear();

  bool got_end = false;

  // Rebuild the full point list from stored VISIT_POINT messages
  for (unsigned int i = 0; i < m_visit_points.size(); i++)
  {
    string sval = stripBlankEnds(m_visit_points[i]);

    if (sval == "end")
    {
      got_end = true;
      continue;
    }

    if (sval == "firstpoint")
      continue;

    string sx = tokStringParse(sval, "x", ',', '=');
    string sy = tokStringParse(sval, "y", ',', '=');

    if ((sx != "") && (sy != ""))
    {
      m_points_x.push_back(atof(sx.c_str()));
      m_points_y.push_back(atof(sy.c_str()));
    }
  }

  // Keep visited flags aligned with full original point list
  if (m_point_visited.size() != m_points_x.size())
    m_point_visited.assign(m_points_x.size(), false);

  // Continuously mark points as visited based on current vehicle position
  for (unsigned int i = 0; i < m_points_x.size(); i++)
  {
    if (m_point_visited[i])
      continue;

    double dx = m_nav_x - m_points_x[i];
    double dy = m_nav_y - m_points_y[i];
    double dist = hypot(dx, dy);

    if (dist <= m_visit_radius)
      m_point_visited[i] = true;
  }

  // Only build a path once a full batch exists and build is requested
  if (m_build_path && got_end)
  {
    vector<double> unvisited_x;
    vector<double> unvisited_y;

    // Build a list of only missed points
    for (unsigned int i = 0; i < m_points_x.size(); i++)
    {
      if (!m_point_visited[i])
      {
        unvisited_x.push_back(m_points_x[i]);
        unvisited_y.push_back(m_points_y[i]);
      }
    }

    if (!unvisited_x.empty())
    {
      XYSegList seglist;
      vector<bool> used(unvisited_x.size(), false);

      double curr_x = unvisited_x[0];
      double curr_y = unvisited_y[0];

      seglist.add_vertex(curr_x, curr_y);
      used[0] = true;

      for (unsigned int k = 1; k < unvisited_x.size(); k++)
      {
        int best_ix = -1;
        double best_dist = -1;

        for (unsigned int i = 0; i < unvisited_x.size(); i++)
        {
          if (used[i])
            continue;

          double dx = unvisited_x[i] - curr_x;
          double dy = unvisited_y[i] - curr_y;
          double dist = sqrt(dx * dx + dy * dy);

          if ((best_ix == -1) || (dist < best_dist))
          {
            best_ix = i;
            best_dist = dist;
          }
        }

        if (best_ix != -1)
        {
          seglist.add_vertex(unvisited_x[best_ix], unvisited_y[best_ix]);
          used[best_ix] = true;
          curr_x = unvisited_x[best_ix];
          curr_y = unvisited_y[best_ix];
        }
      }

      string point_str = "";
      for (unsigned int i = 0; i < seglist.size(); i++)
      {
        point_str += doubleToStringX(seglist.get_vx(i), 2);
        point_str += ",";
        point_str += doubleToStringX(seglist.get_vy(i), 2);

        if (i < seglist.size() - 1)
          point_str += ":";
      }

      string update_str = "points=" + point_str;
      Notify("WPT_UPDATE", update_str);
      reportEvent("Posted regenerated waypoint update: " + update_str);
    }
    else
    {
      reportEvent("No missed points remaining to regenerate.");
      Notify("ALL_POINTS_VISITED", "true");
      Notify("RETURN", "true");
    }

    m_build_path = false;
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

    if (param == "v_name1")
    {
      v_name1 = value;
      handled = true;
    }
    else if (param == "v_name2")
    {
      v_name2 = value;
      handled = true;
    }
    else if (param == "visit_radius")
    {
      m_visit_radius = atof(value.c_str());
      handled = true;
    }
    else if (param == "bar")
    {
      handled = true;
    }

    if (!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();   // 
  return (true);         // 
}
//---------------------------------------------------------
// Procedure: registerVariables()

void GenPath::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  // Register("FOOBAR", 0);
  Register("VISIT_POINT", 0);
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
  Register("GENPATH_REGENERATE", 0);
}

//------------------------------------------------------------
// Procedure: buildReport()

bool GenPath::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "File:                                       " << endl;
  m_msgs << "============================================" << endl;
  m_msgs << "====================" << endl;
  m_msgs << "GenPath Report" << endl;
  m_msgs << "====================" << endl;

  m_msgs << "Visit Points:" << endl;

  for (unsigned int i = 0; i < m_visit_points.size(); i++)
  {
    m_msgs << "  [" << i << "] " << m_visit_points[i] << endl;
  }

  return (true);
}