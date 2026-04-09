/*****************************************************************/
/*    NAME: M.Benjamin, H.Schmidt, J. Leonard                    */
/*    ORGN: Dept. of Mechanical Eng / CSAIL, MIT Cambridge MA    */
/*    FILE: BHV_Pulse.h                                          */
/*    DATE: July 1st 2008                                        */
/*****************************************************************/

#ifndef BHV_PULSE_HEADER
#define BHV_PULSE_HEADER

#include <string>
#include "IvPBehavior.h"

class BHV_Pulse : public IvPBehavior {
public:
  BHV_Pulse(IvPDomain);
  ~BHV_Pulse() {};

  bool         setParam(std::string, std::string);
  void         onIdleState();
  IvPFunction* onRunState();

protected: // Configuration parameters
  double       m_pulse_range;
  double       m_pulse_duration;

protected: // State variables
  double       m_osx;
  double       m_osy;
  double       m_curr_time;

  unsigned int m_prev_wpt_index;
  double       m_wpt_change_time;

  bool         m_first_wpt_index;
  bool         m_pulse_pending;
};

#define IVP_EXPORT_FUNCTION

extern "C" {
  IVP_EXPORT_FUNCTION IvPBehavior * createBehavior(std::string name, IvPDomain domain)
  {return new BHV_Pulse(domain);}
}

#endif