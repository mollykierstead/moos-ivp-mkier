/************************************************************/
/*    NAME: dd                                              */
/*    ORGN: MIT                                             */
/*    FILE: BHV_Garb.h                                      */
/*    DATE:                                                 */
/************************************************************/

#ifndef Garb_HEADER
#define Garb_HEADER

#include <string>
#include "IvPBehavior.h"

class BHV_Garb : public IvPBehavior {
public:
  BHV_Garb(IvPDomain);
  ~BHV_Garb() {};
  
  bool         setParam(std::string, std::string);
  void         onSetParamComplete();
  void         onCompleteState();
  void         onIdleState();
  void         onHelmStart();
  void         postConfigStatus();
  void         onRunToIdleState();
  void         onIdleToRunState();
  IvPFunction* onRunState();

protected: // Local Utility functions

protected: // Configuration parameters

protected: // State variables
};

#define IVP_EXPORT_FUNCTION

extern "C" {
  IVP_EXPORT_FUNCTION IvPBehavior * createBehavior(std::string name, IvPDomain domain) 
  {return new BHV_Garb(domain);}
}
#endif
