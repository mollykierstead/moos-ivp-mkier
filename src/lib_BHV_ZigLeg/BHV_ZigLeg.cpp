/*****************************************************************/
/*    NAME: M.Benjamin, H.Schmidt, J. Leonard                    */
/*    ORGN: Dept of Mechanical Eng / CSAIL, MIT Cambridge MA     */
/*    FILE: BHV_ZigLeg.cpp                               */
/*    DATE: July 1st 2008  (For purposes of simple illustration) */
/*                                                               */
/* This program is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU General Public License   */
/* as published by the Free Software Foundation; either version  */
/* 2 of the License, or (at your option) any later version.      */
/*                                                               */
/* This program is distributed in the hope that it will be       */
/* useful, but WITHOUT ANY WARRANTY; without even the implied    */
/* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/* PURPOSE. See the GNU General Public License for more details. */
/*                                                               */
/* You should have received a copy of the GNU General Public     */
/* License along with this program; if not, write to the Free    */
/* Software Foundation, Inc., 59 Temple Place - Suite 330,       */
/* Boston, MA 02111-1307, USA.                                   */
/*****************************************************************/

#include <cstdlib>
#include <math.h>
#include "BHV_ZigLeg.h"
#include "MBUtils.h"
#include "AngleUtils.h"
#include "BuildUtils.h"
#include "ZAIC_PEAK.h"
#include "OF_Coupler.h"
#include "OF_Reflector.h"

using namespace std;

//-----------------------------------------------------------
// Procedure: Constructor

BHV_ZigLeg::BHV_ZigLeg(IvPDomain gdomain) :
  IvPBehavior(gdomain)
{
  IvPBehavior::setParam("name", "zigleg");
  m_domain = subDomain(m_domain, "course");

  m_zig_duration = 10;
  m_zig_angle = 45;

  m_osx = 0;
  m_osy = 0;
  m_osheading = 0;
  m_curr_time = 0;

  m_prev_wpt_index = 0;
  m_wpt_change_time = 0;

  m_first_wpt_index = true;
  m_zig_pending = false;
  m_zig_active = false;

  m_zig_start_time = 0;
  m_zig_heading = 0;

  addInfoVars("NAV_X, NAV_Y, NAV_HEADING, WPT_INDEX");
}


//---------------------------------------------------------------
// Procedure: setParam - handle behavior configuration parameters

bool BHV_ZigLeg::setParam(std::string param, std::string val)
{
  param = tolower(param);
  double dval = atof(val.c_str());

  if((param == "zig_duration") && isNumber(val) && (dval > 0)) {
    m_zig_duration = dval;
    return(true);
  }
  else if((param == "zig_angle") && isNumber(val)) {
    m_zig_angle = dval;
    return(true);
  }

  return(IvPBehavior::setParam(param, val));
}

//-----------------------------------------------------------
// Procedure: onIdleState

void BHV_ZigLeg::onIdleState() 
{
  return;
}

//-----------------------------------------------------------

//-----------------------------------------------------------
// Procedure: onRunState

IvPFunction* BHV_ZigLeg::onRunState()
{
  bool ok_x, ok_y, ok_hdg, ok_wpt;

  m_osx = getBufferDoubleVal("NAV_X", ok_x);
  m_osy = getBufferDoubleVal("NAV_Y", ok_y);
  m_osheading = getBufferDoubleVal("NAV_HEADING", ok_hdg);
  double curr_wpt_index_dbl = getBufferDoubleVal("WPT_INDEX", ok_wpt);
  m_curr_time = getBufferCurrTime();

  if(!ok_x || !ok_y || !ok_hdg || !ok_wpt) {
    postWMessage("Missing NAV_X, NAV_Y, NAV_HEADING, or WPT_INDEX in info_buffer.");
    return(0);
  }

  unsigned int curr_wpt_index = (unsigned int)(curr_wpt_index_dbl);

  if(m_first_wpt_index) {
    m_prev_wpt_index = curr_wpt_index;
    m_first_wpt_index = false;
    return(0);
  }

  // detect waypoint change
  if(curr_wpt_index != m_prev_wpt_index) {
    m_prev_wpt_index = curr_wpt_index;
    m_wpt_change_time = m_curr_time;
    m_zig_pending = true;
    m_zig_active = false;
  }

  // start zig 5 sec after waypoint change
  if(m_zig_pending && ((m_curr_time - m_wpt_change_time) >= 5.0)) {
    m_zig_pending = false;
    m_zig_active = true;
    m_zig_start_time = m_curr_time;
    m_zig_heading = m_osheading;   // save heading at zig start
  }

  // stop zig after duration
  if(m_zig_active && ((m_curr_time - m_zig_start_time) > m_zig_duration)) {
    m_zig_active = false;
  }

  if(m_zig_active) {
    IvPFunction* ipf = buildFunctionWithZAIC();
    if(ipf)
      ipf->setPWT(m_priority_wt);
    return(ipf);
  }

  return(0);
}
//-----------------------------------------------------------
// Procedure: buildFunctionWithZAIC

#include "ZAIC_PEAK.h"
#include "AngleUtils.h"

IvPFunction* BHV_ZigLeg::buildFunctionWithZAIC()
{
  double desired_course = angle360(m_zig_heading + m_zig_angle);

  ZAIC_PEAK crs_zaic(m_domain, "course");
  crs_zaic.setSummit(desired_course);
  crs_zaic.setPeakWidth(0);
  crs_zaic.setBaseWidth(180.0);
  crs_zaic.setSummitDelta(0);
  crs_zaic.setValueWrap(true);

  if(!crs_zaic.stateOK()) {
    postWMessage("Course ZAIC problems: " + crs_zaic.getWarnings());
    return(0);
  }

  return(crs_zaic.extractIvPFunction());
}
