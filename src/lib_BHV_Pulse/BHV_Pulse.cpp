/*****************************************************************/
/*    NAME: M.Benjamin, H.Schmidt, J. Leonard                    */
/*    ORGN: Dept. of Mechanical Eng / CSAIL, MIT Cambridge MA    */
/*    FILE: BHV_Pulse.cpp                                        */
/*    DATE: July 1st 2008                                        */
/*****************************************************************/

#include <cstdlib>
#include "BHV_Pulse.h"
#include "MBUtils.h"
#include "XYRangePulse.h"

using namespace std;

//-----------------------------------------------------------
// Constructor

BHV_Pulse::BHV_Pulse(IvPDomain gdomain) :
  IvPBehavior(gdomain)
{
  IvPBehavior::setParam("name", "pulse");

  // Configuration parameters
  m_pulse_range = 40;
  m_pulse_duration = 4;

  // State variables
  m_osx = 0;
  m_osy = 0;
  m_curr_time = 0;

  m_prev_wpt_index = 0;
  m_wpt_change_time = 0;

  m_first_wpt_index = true;
  m_pulse_pending = false;

  addInfoVars("NAV_X, NAV_Y, WPT_INDEX");
}

//-----------------------------------------------------------
// Procedure: setParam

bool BHV_Pulse::setParam(string param, string val)
{
  param = tolower(param);
  double dval = atof(val.c_str());

  if((param == "pulse_range") && isNumber(val) && (dval > 0)) {
    m_pulse_range = dval;
    return(true);
  }
  else if((param == "pulse_duration") && isNumber(val) && (dval > 0)) {
    m_pulse_duration = dval;
    return(true);
  }

  return(IvPBehavior::setParam(param, val));
}

//-----------------------------------------------------------
// Procedure: onIdleState

void BHV_Pulse::onIdleState()
{
  return;
}

//-----------------------------------------------------------
// Procedure: onRunState

IvPFunction* BHV_Pulse::onRunState()
{
  bool ok_x, ok_y, ok_wpt;

  m_osx = getBufferDoubleVal("NAV_X", ok_x);
  m_osy = getBufferDoubleVal("NAV_Y", ok_y);
  double curr_wpt_index_dbl = getBufferDoubleVal("WPT_INDEX", ok_wpt);
  m_curr_time = getBufferCurrTime();

  if(!ok_x || !ok_y || !ok_wpt) {
    postWMessage("Missing NAV_X, NAV_Y, or WPT_INDEX in info_buffer.");
    return(0);
  }

  unsigned int curr_wpt_index = (unsigned int)(curr_wpt_index_dbl);

  if(m_first_wpt_index) {
    m_prev_wpt_index = curr_wpt_index;
    m_first_wpt_index = false;
    return(0);
  }

  if(curr_wpt_index != m_prev_wpt_index) {
    m_prev_wpt_index = curr_wpt_index;
    m_wpt_change_time = m_curr_time;
    m_pulse_pending = true;
  }

  if(m_pulse_pending && ((m_curr_time - m_wpt_change_time) >= 5.0)) {
    XYRangePulse pulse;
    pulse.set_x(m_osx);
    pulse.set_y(m_osy);
    pulse.set_label("pulse");
    pulse.set_rad(m_pulse_range);
    pulse.set_duration(m_pulse_duration);
    pulse.set_time(m_curr_time);
    pulse.set_color("edge", "yellow");
    pulse.set_color("fill", "yellow");

    string spec = pulse.get_spec();
    postMessage("VIEW_RANGE_PULSE", spec);

    m_pulse_pending = false;
  }

  return(0);
}