/************************************************************/
/*    NAME: Molly Kierstead                                              */
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

private:
  std::vector<double> m_points_x;
  std::vector<double> m_points_y;

  bool m_build_path;

  std::string m_update_var;
  std::vector<std::string> m_visit_points;
  std::string v_name1;
  std::string v_name2;

  double m_nav_x;
  double m_nav_y;
  double m_visit_radius;
  std::vector<bool> m_point_visited;

private: // State variables
};

#endif
