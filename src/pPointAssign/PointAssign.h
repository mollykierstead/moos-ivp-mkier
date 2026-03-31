/************************************************************/
/*    NAME: Chris Jones                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: PointAssign.h                                          */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#ifndef PointAssign_HEADER
#define PointAssign_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class PointAssign : public AppCastingMOOSApp
{
 public:
   PointAssign();
   ~PointAssign();

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

 std::string m_visit_point_string;
 int m_point_count;

 private: // State variables

 void postViewPoint(double x, double y, std::string label, std::string color); 

std::queue<std::string> m_point_queue;
bool m_region_assign;
double m_x_divider;
bool m_ready_henry;
bool m_ready_gilda;

};

#endif 
