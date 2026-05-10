/*****************************************************************/
/*    NAME: M. Kierstead                                         */
/*    ORGN: Dept of Mechanical Eng / CSAIL, MIT Cambridge MA     */
/*    FILE: BHV_Scout.cpp                                        */
/*    DATE: April 28th 2026                                   */
/*****************************************************************/

#include <cstdlib>
#include <math.h>
#include "BHV_Scout.h"
#include "MBUtils.h"
#include "AngleUtils.h"
#include "BuildUtils.h"
#include "GeomUtils.h"
#include "ZAIC_PEAK.h"
#include "OF_Coupler.h"
#include "XYFormatUtilsPoly.h"

using namespace std;

//-----------------------------------------------------------
// Constructor()

BHV_Scout::BHV_Scout(IvPDomain gdomain) : 
  IvPBehavior(gdomain)
{
  IvPBehavior::setParam("name", "scout");
 
  // Default values for behavior state variables
  m_osx = 0;
  m_osy = 0;

  m_desired_speed = 1;
  m_capture_radius = 10;

  m_pt_set = false;
  m_lawn_ix = 0;
  m_lawnmower_built = false;
  m_lane_spacing = 15;  
  
  addInfoVars("NAV_X, NAV_Y");
  addInfoVars("RESCUE_REGION");
  addInfoVars("SCOUTED_SWIMMER");
}

//---------------------------------------------------------------
// Procedure: setParam() - handle behavior configuration parameters

bool BHV_Scout::setParam(string param, string val) 
{
  // Convert the parameter to lower case for more general matching
  param = tolower(param);
  
  bool handled = true;
  if(param == "capture_radius")
    handled = setPosDoubleOnString(m_capture_radius, val);
  else if(param == "desired_speed")
    handled = setPosDoubleOnString(m_desired_speed, val);
  else if(param == "tmate")
    handled = setNonWhiteVarOnString(m_tmate, val);
  else if(param == "lane_spacing")
  handled = setPosDoubleOnString(m_lane_spacing, val);
  else
    handled = false;

  srand(time(NULL));
  
  return(handled);
}

//-----------------------------------------------------------
// Procedure: onEveryState()

void BHV_Scout::onEveryState(string str) 
{
  if(!getBufferVarUpdated("SCOUTED_SWIMMER"))
    return;

  string report = getBufferStringVal("SCOUTED_SWIMMER");
  if(report == "")
    return;

  if(m_tmate == "") {
    postWMessage("Mandatory Teammate name is null");
    return;
  }
  postOffboardMessage(m_tmate, "SWIMMER_ALERT", report);
}

//-----------------------------------------------------------
// Procedure: onIdleState()

void BHV_Scout::onIdleState() 
{
  m_curr_time = getBufferCurrTime();
}

//-----------------------------------------------------------
// Procedure: onRunState()

IvPFunction *BHV_Scout::onRunState() 
{
  bool ok1, ok2;
  m_osx = getBufferDoubleVal("NAV_X", ok1);
  m_osy = getBufferDoubleVal("NAV_Y", ok2);

  if(!ok1 || !ok2) {
    postWMessage("No ownship X/Y info in info_buffer.");
    return(0);
  }

  updateScoutPoint();

  if(!m_pt_set)
    return(0);

  double dist = hypot((m_ptx - m_osx), (m_pty - m_osy));

  if(dist <= m_capture_radius) {
    m_lawn_ix++;

    if(m_lawn_ix >= m_lawn_x.size())
      m_lawn_ix = 0;

    m_pt_set = false;
    postViewPoint(false);
    updateScoutPoint();
  }

  postViewPoint(true);

  IvPFunction *ipf = buildFunction();
  if(ipf == 0)
    postWMessage("Problem Creating the IvP Function");

  return(ipf);
}

//-----------------------------------------------------------
// Procedure: buildLawnmowerPattern()

void BHV_Scout::buildLawnmowerPattern()
{
  m_lawn_x.clear();
  m_lawn_y.clear();

  double min_x = m_rescue_region.get_min_x();
  double max_x = m_rescue_region.get_max_x();
  double min_y = m_rescue_region.get_min_y();
  double max_y = m_rescue_region.get_max_y();

  bool left_to_right = true;

  for(double y = min_y; y <= max_y; y += m_lane_spacing) {
    std::vector<double> x_candidates;

    for(double x = min_x; x <= max_x; x += 2.0) {
      if(m_rescue_region.contains(x, y))
        x_candidates.push_back(x);
    }

    if(x_candidates.size() == 0)
      continue;

    if(left_to_right) {
      m_lawn_x.push_back(x_candidates.front());
      m_lawn_y.push_back(y);
      m_lawn_x.push_back(x_candidates.back());
      m_lawn_y.push_back(y);
    }
    else {
      m_lawn_x.push_back(x_candidates.back());
      m_lawn_y.push_back(y);
      m_lawn_x.push_back(x_candidates.front());
      m_lawn_y.push_back(y);
    }

    left_to_right = !left_to_right;
  }

  postEventMessage("Built lawnmower pattern with " + uintToString(m_lawn_x.size()) + " points");
}

//-----------------------------------------------------------
// Procedure: updateScoutPoint()

void BHV_Scout::updateScoutPoint()
{
  if(m_pt_set)
    return;

  string region_str = getBufferStringVal("RESCUE_REGION");
  if(region_str == "") {
    postWMessage("Unknown RESCUE_REGION");
    return;
  }
  else
    postRetractWMessage("Unknown RESCUE_REGION");

  XYPolygon region = string2Poly(region_str);
  if(!region.is_convex()) {
    postWMessage("Badly formed RESCUE_REGION");
    return;
  }

  m_rescue_region = region;

  if(!m_lawnmower_built) {
    buildLawnmowerPattern();
    m_lawnmower_built = true;
  }

  if(m_lawn_x.size() == 0) {
    postWMessage("No lawnmower points generated");
    return;
  }

  if(m_lawn_ix >= m_lawn_x.size())
    m_lawn_ix = 0;

  m_ptx = m_lawn_x[m_lawn_ix];
  m_pty = m_lawn_y[m_lawn_ix];
  m_pt_set = true;

  string msg = "New lawnmower pt: " + doubleToStringX(m_ptx) + "," + doubleToStringX(m_pty);
  postEventMessage(msg);
}
//-----------------------------------------------------------
// Procedure: postViewPoint()

void BHV_Scout::postViewPoint(bool viewable) 
{

  XYPoint pt(m_ptx, m_pty);
  pt.set_vertex_size(5);
  pt.set_vertex_color("orange");
  pt.set_label(m_us_name + "'s next waypoint");
  
  string point_spec;
  if(viewable)
    point_spec = pt.get_spec("active=true");
  else
    point_spec = pt.get_spec("active=false");
  postMessage("VIEW_POINT", point_spec);
}


//-----------------------------------------------------------
// Procedure: buildFunction()

IvPFunction *BHV_Scout::buildFunction() 
{
  if(!m_pt_set)
    return(0);
  
  ZAIC_PEAK spd_zaic(m_domain, "speed");
  spd_zaic.setSummit(m_desired_speed);
  spd_zaic.setPeakWidth(0.5);
  spd_zaic.setBaseWidth(1.0);
  spd_zaic.setSummitDelta(0.8);  
  if(spd_zaic.stateOK() == false) {
    string warnings = "Speed ZAIC problems " + spd_zaic.getWarnings();
    postWMessage(warnings);
    return(0);
  }
  
  double rel_ang_to_wpt = relAng(m_osx, m_osy, m_ptx, m_pty);
  ZAIC_PEAK crs_zaic(m_domain, "course");
  crs_zaic.setSummit(rel_ang_to_wpt);
  crs_zaic.setPeakWidth(0);
  crs_zaic.setBaseWidth(180.0);
  crs_zaic.setSummitDelta(0);  
  crs_zaic.setValueWrap(true);
  if(crs_zaic.stateOK() == false) {
    string warnings = "Course ZAIC problems " + crs_zaic.getWarnings();
    postWMessage(warnings);
    return(0);
  }

  IvPFunction *spd_ipf = spd_zaic.extractIvPFunction();
  IvPFunction *crs_ipf = crs_zaic.extractIvPFunction();

  OF_Coupler coupler;
  IvPFunction *ivp_function = coupler.couple(crs_ipf, spd_ipf, 50, 50);

  return(ivp_function);
}
