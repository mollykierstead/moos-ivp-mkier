/************************************************************/
/*    NAME: Molly Kierstead                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: PointAssign.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "PointAssign.h"
#include "XYPoint.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

PointAssign::PointAssign()
{
  m_visit_point_string = "";
  m_point_count = 0;
  m_region_assign = false; // default: use old alternating method
  m_x_divider = 0;         // east/west split line
  m_ready_henry = false;
  m_ready_gilda = false; 
}

//---------------------------------------------------------
// Destructor

PointAssign::~PointAssign()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool PointAssign::OnNewMail(MOOSMSG_LIST &NewMail)
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

    if (key == "FOO")
      cout << "great!";
    else if (key == "VISIT_POINT")
    {
      string point = msg.GetString();

      Notify("DEBUG_DATA", "New point: " + msg.GetString());
      m_point_queue.push(point);
    }
    else if (key == "READY_TO_RECEIVE_HENRY")
    {
      string sval = msg.GetString();
      m_ready_henry = (tolower(sval) == "true");
    }

    else if (key == "READY_TO_RECEIVE_GILDA")
    {
      string sval = msg.GetString();
      m_ready_gilda = (tolower(sval) == "true");
    }
    else if (key != "APPCAST_REQ") // handled by AppCastingMOOSApp
    {
      reportRunWarning("Unhandled Mail: " + key);
    }
  }
  return (true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool PointAssign::OnConnectToServer()
{
  registerVariables();
  //Notify("POINT_ASSIGN_START", "false");
  return (true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool PointAssign::Iterate()
{
  AppCastingMOOSApp::Iterate();

  bool sent_henry = false;
  bool sent_gilda = false;

  if (m_point_queue.empty() or !m_ready_gilda or !m_ready_henry)
  {
    AppCastingMOOSApp::PostReport();
    return (true);
  }

  while (!m_point_queue.empty())
  {
    m_visit_point_string = m_point_queue.front();
    m_point_queue.pop();

    if ((m_visit_point_string == "firstpoint") || (m_visit_point_string == "end"))
      continue;

    string a = tokStringParse(m_visit_point_string, "x", ',', '=');
    string b = tokStringParse(m_visit_point_string, "y", ',', '=');
    string c = tokStringParse(m_visit_point_string, "id", ',', '=');

    double x = atof(a.c_str());
    double y = atof(b.c_str());

    string color = "yellow";

    if (m_region_assign)
    {
      if (x < m_x_divider)
      {
        if (!sent_gilda)
        {
          Notify("VISIT_POINT_GILDA", "firstpoint");
          sent_gilda = true;
        }
        Notify("VISIT_POINT_GILDA", m_visit_point_string);
        color = "red";
      }
      else
      {
        if (!sent_henry)
        {
          Notify("VISIT_POINT_HENRY", "firstpoint");
          sent_henry = true;
        }
        Notify("VISIT_POINT_HENRY", m_visit_point_string);
        color = "yellow";
      }
    }
    else
    {
      if (m_point_count % 2 == 0)
      {
        if (!sent_henry)
        {
          Notify("VISIT_POINT_HENRY", "firstpoint");
          sent_henry = true;
        }
        Notify("VISIT_POINT_HENRY", m_visit_point_string);
        color = "yellow";
      }
      else
      {
        if (!sent_gilda)
        {
          Notify("VISIT_POINT_GILDA", "firstpoint");
          sent_gilda = true;
        }
        Notify("VISIT_POINT_GILDA", m_visit_point_string);
        color = "red";
      }
      m_point_count++;
    }

    postViewPoint(x, y, c, color);
  }

  if (sent_henry)
    Notify("VISIT_POINT_HENRY", "end");
  if (sent_gilda)
    Notify("VISIT_POINT_GILDA", "end");

  AppCastingMOOSApp::PostReport();
  return (true);
}
  //---------------------------------------------------------
  // Procedure: OnStartUp()
  //            happens before connection is open

  bool PointAssign::OnStartUp()
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
      if (param == "region_assign")
      {
        m_region_assign = (tolower(value) == "true");
        handled = true;
      }

      else if (param == "x_divider")
      {
         m_x_divider = atof(value.c_str());
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

  void PointAssign::registerVariables()
  {
    AppCastingMOOSApp::RegisterVariables();

    Register("VISIT_POINT", 0);
    Register("READY_TO_RECEIVE_HENRY", 0);
    Register("READY_TO_RECEIVE_GILDA", 0);
  }
  //------------------------------------------------------------
  // Procedure: buildReport()

  bool PointAssign::buildReport()
  {
    m_msgs << "============================================" << endl;
    m_msgs << "File:                                       " << endl;
    m_msgs << "============================================" << endl;

    ACTable actab(4);
    actab << "Alpha | Bravo | Charlie | Delta";
    actab.addHeaderLines();
    actab << "one" << "two" << "three" << "four";
    m_msgs << actab.getFormattedString();

    return (true);
  }

  void PointAssign::postViewPoint(double x, double y, std::string label, std::string color)
  {
    XYPoint point(x, y);
    point.set_label(label);
    point.set_color("vertex", color); // yellow is handy on dark screen
    point.set_param("vertex_size", "4");
    string spec = point.get_spec();
    Notify("VIEW_POINT", spec);
    // gets the string representation of a point
  }
