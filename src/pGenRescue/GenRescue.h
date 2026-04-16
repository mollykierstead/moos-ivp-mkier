/************************************************************/
/*    NAME: Molly Kierstead                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: GenRescue.h                                          */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#ifndef GenRescue_HEADER
#define GenRescue_HEADER

#include <vector>
#include <string>
#include <set>

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"
#include "XYSegList.h"

class GenRescue : public AppCastingMOOSApp
{
public:
  GenRescue();
  ~GenRescue();

protected:
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();

protected:
  bool buildReport();
  void registerVariables();

private:
  bool handleSwimmerAlert(std::string alert);
  bool handleFoundSwimmer(std::string msg);
  void buildPath();
  void postPath();

private:
  std::vector<std::string> m_swimmer_ids;
  std::vector<double>      m_swimmer_x;
  std::vector<double>      m_swimmer_y;
  std::vector<bool>        m_swimmer_found;

  std::set<std::string>    m_known_ids;

  bool   m_build_path;
  double m_nav_x;
  double m_nav_y;
  double m_visit_radius;

  std::string m_update_var;
};

#endif
