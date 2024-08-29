/************************************************************/
/*    NAME: Blake Cole                                      */
/*    ORGN: MIT                                             */
/*    FILE: NMEA.h                                          */
/*    DATE: 01 APRIL 2020                                   */
/************************************************************/

#ifndef NMEA_HEADER
#define NMEA_HEADER

#include <string>
#include <vector>
#include <queue>
#include "MBUtils.h"
#include "MOOS/libMOOS/Utils/MOOSUtilityFunctions.h"
#include "nmeaDatum.h"

class NMEA
{
public:
  NMEA(std::string nmea_sentence);
  ~NMEA();

  std::vector<nmeaDatum> parseNMEA();
  
  std::string  getSource()  {return m_source;};

protected:
  std::vector<nmeaDatum> formatGPGGA();
  std::vector<nmeaDatum> formatGPRMC();
  std::vector<nmeaDatum> formatGPGSA();
  std::vector<nmeaDatum> formatCPNVG();
  std::vector<nmeaDatum> formatCPNVR();
  std::vector<nmeaDatum> formatCPRCM();
  std::vector<nmeaDatum> formatCPRBS();
  std::vector<nmeaDatum> formatCPIMU();
  std::vector<nmeaDatum> formatPGRME();

  

private:
  std::string               m_raw_sentence;
  std::string               m_rx_contents;
  std::vector<std::string>  m_fields;
  std::string               m_source;
  std::vector<nmeaDatum>    m_data;
};


#endif
