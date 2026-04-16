/************************************************************/
/*    NAME: Molly Kierstead                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenRescue.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

/************************************************************/
/*    NAME: Molly Kierstead                                 */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenRescue.cpp                                   */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include <cmath>
#include "MBUtils.h"
#include "ACTable.h"
#include "GenRescue.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

GenRescue::GenRescue()
{
  m_build_path = false;
  m_update_var = "SURVEY_UPDATE";

  m_swimmer_ids.clear();
  m_swimmer_x.clear();
  m_swimmer_y.clear();
  m_swimmer_found.clear();
  m_known_ids.clear();

  m_nav_x = 0;
  m_nav_y = 0;
  m_visit_radius = 5.0;
}

//---------------------------------------------------------
// Destructor

GenRescue::~GenRescue()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool GenRescue::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for (p = NewMail.begin(); p != NewMail.end(); p++)
  {
    CMOOSMsg &msg = *p;
    string key = msg.GetKey();

    if (key == "SWIMMER_ALERT")
    {
      string sval = msg.GetString();

      string sx = tokStringParse(sval, "x", ',', '=');
      string sy = tokStringParse(sval, "y", ',', '=');
      string sid = tokStringParse(sval, "id", ',', '=');

      if ((sx != "") && (sy != "") && (sid != ""))
      {
        if (m_known_ids.count(sid) == 0)
        {
          m_known_ids.insert(sid);
          m_swimmer_ids.push_back(sid);
          m_swimmer_x.push_back(atof(sx.c_str()));
          m_swimmer_y.push_back(atof(sy.c_str()));
          m_swimmer_found.push_back(false);

          m_build_path = true;
          reportEvent("New swimmer received: " + sid);
        }
      }
    }
    else if (key == "FOUND_SWIMMER")
    {
      string sval = msg.GetString();
      string sid = tokStringParse(sval, "id", ',', '=');

      for (unsigned int i = 0; i < m_swimmer_ids.size(); i++)
      {
        if (m_swimmer_ids[i] == sid)
        {
          m_swimmer_found[i] = true;
          m_build_path = true;
          reportEvent("Swimmer found: " + sid);
          break;
        }
      }
    }
    else if (key == "NAV_X")
    {
      m_nav_x = msg.GetDouble();
    }
    else if (key == "NAV_Y")
    {
      m_nav_y = msg.GetDouble();
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

bool GenRescue::OnConnectToServer()
{
  registerVariables();
  return (true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool GenRescue::Iterate()
{
  AppCastingMOOSApp::Iterate();

  m_build_path = true;

  if (m_build_path)
  {
    vector<bool> used(m_swimmer_x.size(), false);
    XYSegList seglist;

    double curr_x = m_nav_x;
    double curr_y = m_nav_y;

    reportEvent("Rebuilding path from NAV: " +
                doubleToStringX(m_nav_x,2) + "," +
                doubleToStringX(m_nav_y,2));

    for (unsigned int k = 0; k < m_swimmer_x.size(); k++)
    {
      int best_ix = -1;
      double best_dist = -1;

      for (unsigned int i = 0; i < m_swimmer_x.size(); i++)
      {
        if (used[i] || m_swimmer_found[i])
          continue;

        double dx = m_swimmer_x[i] - curr_x;
        double dy = m_swimmer_y[i] - curr_y;
        double dist = hypot(dx, dy);

        if ((best_ix == -1) || (dist < best_dist))
        {
          best_ix = i;
          best_dist = dist;
        }
      }

      if (best_ix != -1)
      {
        seglist.add_vertex(m_swimmer_x[best_ix], m_swimmer_y[best_ix]);
        used[best_ix] = true;
        curr_x = m_swimmer_x[best_ix];
        curr_y = m_swimmer_y[best_ix];
      }
    }

    if (seglist.size() > 0)
    {
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
      Notify(m_update_var, update_str);
      reportEvent("Path order: " + point_str);
      reportEvent("Posted SURVEY_UPDATE: " + update_str);
      XYSegList view_seg = seglist;
      view_seg.set_label("rescue_path");
      view_seg.set_color("edge", "yellow");
      view_seg.set_color("vertex", "red");
      view_seg.set_edge_size(2);
      view_seg.set_vertex_size(6);
      Notify("VIEW_SEGLIST", view_seg.get_spec());
          }
    else
    {
      reportEvent("No remaining swimmers to visit.");
    }

    m_build_path = false;
  }

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool GenRescue::OnStartUp()
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

    if (param == "visit_radius")
    {
      m_visit_radius = atof(value.c_str());
      handled = true;
    }
    else if (param == "update_var")
    {
      m_update_var = value;
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

void GenRescue::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
  Register("SWIMMER_ALERT", 0);
  Register("FOUND_SWIMMER", 0);
}

//------------------------------------------------------------
// Procedure: buildReport()

bool GenRescue::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "GenRescue Report                            " << endl;
  m_msgs << "============================================" << endl;
  m_msgs << "Nav X: " << m_nav_x << endl;
  m_msgs << "Nav Y: " << m_nav_y << endl;
  m_msgs << "Build Path Pending: " << boolToString(m_build_path) << endl;
  m_msgs << "Known Swimmers:" << endl;

  for (unsigned int i = 0; i < m_swimmer_ids.size(); i++)
  {
    m_msgs << "  id=" << m_swimmer_ids[i]
           << ", x=" << m_swimmer_x[i]
           << ", y=" << m_swimmer_y[i]
           << ", found=" << boolToString(m_swimmer_found[i])
           << endl;
  }

  return (true);
}