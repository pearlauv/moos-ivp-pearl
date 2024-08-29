/************************************************************/
/*    NAME: Blake Cole                                      */
/*    ORGN: MIT                                             */
/*    FILE: NMEA.cpp                                        */
/*    DATE: 01 APRIL 2020                                   */
/************************************************************/

#include "NMEA.h"

using namespace std;

NMEA::NMEA(string nmea_sentence)
{
  m_raw_sentence = nmea_sentence;
}

NMEA::~NMEA() {};


vector<nmeaDatum> NMEA::parseNMEA()
{
  // Trim ends off sentence
  m_rx_contents = biteString(m_raw_sentence, '*'); // remove checksum
  m_rx_contents = m_rx_contents.substr(1);         // remove '$'

  // Initialize nmeaDatum, parse
  m_fields = parseString(m_rx_contents, ',');
  m_source = m_fields[0];       // source (e.g. 'GPGGA')

  // (1) Direct to appropriate NMEA source format
  // (2) Create nmeaDatum object for each datum in NMEA sentence
  // (3) Push onto m_data vector and return m_data

  if      (MOOSStrCmp(m_source, "GPGGA")){
    return(formatGPGGA());
  }
  else if (MOOSStrCmp(m_source, "GPRMC")){
    return(formatGPRMC());
  }
  else if (MOOSStrCmp(m_source, "GPGSA")){
    return(formatGPGSA());
  }
  else if (MOOSStrCmp(m_source, "CPNVG")){
    return(formatCPNVG());
  }
  else if (MOOSStrCmp(m_source, "CPNVR")){
    return(formatCPNVR());
  }
  else if (MOOSStrCmp(m_source, "CPRCM")){
    return(formatCPRCM());
  }
  else if (MOOSStrCmp(m_source, "CPRBS")){
    return(formatCPRBS());
  }
  else if (MOOSStrCmp(m_source, "CPIMU")){
    return(formatCPIMU());
  }
  else if (MOOSStrCmp(m_source, "PGRME")){
    return(formatPGRME());
  }
  else{
    return(m_data);
  }  
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// GPGGA - Common NMEA message for position
//
// $GPGGA,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>,<13>,<14>*hh<CR><LF>
//     <1>  UTC time [hhmmss.ss]
//     <2>  Latitude [llll.ll]
//     <3>  Hemisphere [N/S]
//     <4>  Longitude [yyyyy.yy]
//     <5>  Hemisphere, [E/W]
//     <6>  GPS Quality
//            0=No fix
//            1=GPS Fix
//            2=Differential GPS fix (values above 2 are 2.3 features)
//            3=PPS fix
//            4=Real Time Kinematic
//            5=Float RTK
//            6 = estimated (dead reckoning)
//            7 = Manual input mode
//            8 = Simulation mode
//     <7>  Number of Satellites [00 - 12]
//     <8>  HDOP [0.5 - 99.9]
//     <9>  Antenna altitude above MSL [-9999.9 - 999999.9]
//     <10> Antenna altitude units [M=meters]
//     <11> Geoid separation [-999.9 - 9999.9]
//     <12> Geoid separation units [M=meters]
//     <13> Time in seconds since last DGPS update [s.s], null w/o DGPS
//     <14> Differential station ID, 0000 w/o DGPS
//     *hh  Checksum

vector<nmeaDatum> NMEA::formatGPGGA()
{
  string sval;
  double dval;
  string time;
  
  for (int i=1; i<m_fields.size(); i++){
    nmeaDatum field(m_source, i);

    switch(i){
    case 1:
      field.setKey("UTC_TIME");
      field.setDatatype("STRING");
      sval = m_fields[i];
      time = sval;
      field.setValue(sval);
      field.setTime(time);
      break;

    case 2:
      field.setKey("LATITUDE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 3:
      field.setKey("LAT_HEMISPHERE");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 4:
      field.setKey("LONGITUDE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 5:
      field.setKey("LON_HEMISPHERE");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 6:
      field.setKey("GPS_QUALITY");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 7:
      field.setKey("NUM_SATS");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 8:
      field.setKey("HDOP");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 9:
      field.setKey("ALTITUDE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 10:
      field.setKey("ALTITUDE_UNITS");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 11:
      field.setKey("GEOID_SEP");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 12:
      field.setKey("GEOID_SEP_UNITS");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 13:
      field.setKey("DGPS_INTERVAL");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 14:
      field.setKey("DGPS_STN_ID");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;
    }
    m_data.push_back(field);
  }
  return(m_data);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// GPRMC - Recommended Minimum Navigation Information
//
// $GPRMC,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>*hh<CR><LF>
//     <1>  UTC time [hhmmss.ss]
//     <2>  Status
//            A=Valid
//            V=Warning
//     <3>  Latitude [llll.ll]
//     <4>  Hemisphere [N/S]
//     <5>  Longitude [yyyyy.yy]
//     <6>  Hemisphere, [E/W]
//     <7>  Speed over ground in KNOTS, [000.00]
//     <8>  Course over ground in deg from true North [ddd.d]
//     <9>  UTC date [ddmmyy]
//     <10> Magnetic variation true North [ddd.d]
//     <11> Magnetic variation direction  [E/W]
//     <12> OPTIONAL FAA Mode indicator
//            A=Autonomous
//            D=Differential
//            E=Estimated
//            N=bad
//     *hh  Checksum

vector<nmeaDatum> NMEA::formatGPRMC()
{
  string sval;
  double dval;
  string time;
  
  for (int i=1; i<m_fields.size(); i++){
    nmeaDatum field(m_source, i);

    switch(i){
    case 1:
      field.setKey("UTC_TIME");
      field.setDatatype("STRING");
      sval = m_fields[i];
      time = sval;
      field.setValue(sval);
      field.setTime(time);
      break;

    case 2:
      field.setKey("STATUS");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 3:
      field.setKey("LATITUDE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 4:
      field.setKey("LAT_HEMISPHERE");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 5:
      field.setKey("LONGITUDE");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 6:
      field.setKey("LON_HEMISPHERE");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 7:
      field.setKey("SPEED");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 8:
      field.setKey("COURSE");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 9:
      field.setKey("UTC_DATE");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 10:
      field.setKey("MAG_VARIATION");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 11:
      field.setKey("MAG_VARIATION_DIR");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 12:
      field.setKey("FAA_MODE");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;
    }
    m_data.push_back(field);
  }
  return(m_data);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// GPGSA - GPSA DOP and Active Satellites
//
// $GPGSA,<1>,<2>,<3>...<14>,<15>,<16>,<17>*hh<CR><LF>
//     <1>    Selection Mode
//              M=Manual, forced to operate in 2D or 3D
//              A=Automatic, 3D/2D
//     <2>    Mode
//              1=No fix
//              2=2D fix
//              3=3D fix
//     <3-14> IDs of satelites 1-12 used in position fix (unused m_fields = null)
//     <15>   PDOP
//     <16>   HDOP
//     <17>   VDOP
//     *hh    Checksum

vector<nmeaDatum> NMEA::formatGPGSA()
{
  string sval;
  double dval;

  for (int i=1; i<m_fields.size(); i++){
    nmeaDatum field(m_source, i);

    switch(i){
    case 1:
      field.setKey("SAT_SELECT_MODE");
      field.setDatatype("STRING");
      break;

    case 2:
      field.setKey("SAT_FIX");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 3:
      field.setKey("SAT1_ID");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 4:
      field.setKey("SAT2_ID");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 5:
      field.setKey("SAT3_ID");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 6:
      field.setKey("SAT4_ID");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 7:
      field.setKey("SAT5_ID");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 8:
      field.setKey("SAT6_ID");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 9:
      field.setKey("SAT7_ID");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 10:
      field.setKey("SAT8_ID");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 11:
      field.setKey("SAT9_ID");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 12:
      field.setKey("SAT10_ID");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 13:
      field.setKey("SAT11_ID");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 14:
      field.setKey("SAT12_ID");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 15:
      field.setKey("PDOP");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 16:
      field.setKey("HDOP");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 17:
      field.setKey("VDOP");
      field.setDatatype("DOUBLE");
      sval = m_fields[i];
      field.setValue(sval);
      break;
    }
    m_data.push_back(field);
  }
  return(m_data);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CPNVG - Clearpath position / pose state estimation
//
// $CPNVG,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>*hh<CR><LF>
//     <1>  Timestamp
//     <2>  Latitude
//     <3>  Hemisphere [N/S]
//     <4>  Longitude
//     <5>  Hemisphere [E/W]
//     <6>  Quality of position estimate
//            0=no GPS
//            1=GPS
//     <7>  Altitude in meters from bottom, blank for ASVs
//     <8>  Depth in meters from top, blank for ASVs
//     <9>  Direction of travel in degrees clockwise from true North
//     <10> Roll [degrees]
//     <11> Pitch [degrees]
//     <12> Timestamp of pose/position calculation. If blank, use <1>.
//     *hh  Checksum

vector<nmeaDatum> NMEA::formatCPNVG()
{
  string sval;
  double dval;
  string time;

  for (int i=1; i<m_fields.size(); i++){
    nmeaDatum field(m_source, i);

    switch(i){
    case 1:
      field.setKey("TIMESTAMP");
      field.setDatatype("STRING");
      sval = m_fields[i];
      time = sval;
      field.setValue(sval);
      field.setTime(time);
      break;

    case 2:
      field.setKey("LATITUDE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 3:
      field.setKey("LAT_HEMISPHERE");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 4:
      field.setKey("LONGITUDE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 5:
      field.setKey("LON_HEMISPHERE");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;

    case 6:
      field.setKey("QUALITY");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 7:
      field.setKey("SUB_ALTITUDE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 8:
      field.setKey("SUB_DEPTH");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 9:
      field.setKey("COURSE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 10:
      field.setKey("ROLL");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 11:
      field.setKey("PITCH");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 12:
      field.setKey("AUX_TIMESTAMP");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;
    }
    m_data.push_back(field);
  }
  return(m_data);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CPNVR - Clearpath velocity / rotation rate state estimation
//
// $CPNVR,<1>,<2>,<3>,<4>,<5>,<6>,<7>*hh<CR><LF>
//     <1>  Timestamp
//     <2>  Velocity (EAST)
//     <3>  Velocity (NORTH)
//     <4>  Velocity (DOWN)
//     <5>  Pitch rate [deg/s]
//     <6>  Roll rate [deg/s]
//     <7>  Yaw rate [deg/s]

vector<nmeaDatum> NMEA::formatCPNVR()
{
  string sval;
  double dval;
  string time;

  for (int i=1; i<m_fields.size(); i++){
    nmeaDatum field(m_source, i);

    switch(i){
    case 1:
      field.setKey("TIMESTAMP");
      field.setDatatype("STRING");
      sval = m_fields[i];
      time = sval;
      field.setValue(sval);
      field.setTime(time);
      break;

    case 2:
      field.setKey("VEL_EAST");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 3:
      field.setKey("VEL_NORTH");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 4:
      field.setKey("VEL_DOWN");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 5:
      field.setKey("PITCH_RATE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 6:
      field.setKey("ROLL_RATE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 7:
      field.setKey("YAW_RATE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;
    }
    m_data.push_back(field);
  }
  return(m_data);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CPRCM - Clearpath raw compass data
//
// $CPRCM,<1>,<2>,<3>,<4>,<5>,<6>*hh<CR><LF>
//     <1>  Timestamp
//     <2>  Compass ID
//     <3>  HEADING in degrees clockwise from true North
//     <4>  PITCH [degrees]
//     <5>  ROLL [degrees]
//     <6>  Timestamp of compass reading. If blank, use <1>.

vector<nmeaDatum> NMEA::formatCPRCM()
{
  string sval;
  double dval;
  string time;

  for (int i=1; i<m_fields.size(); i++){
    nmeaDatum field(m_source, i);

    switch(i){
    case 1:
      field.setKey("TIMESTAMP");
      field.setDatatype("STRING");
      sval = m_fields[i];
      time = sval;
      field.setValue(sval);
      field.setTime(time);
      break;

    case 2:
      field.setKey("COMPASS_ID");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 3:
      field.setKey("HEADING");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 4:
      field.setKey("PITCH");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 5:
      field.setKey("ROLL");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 6:
      field.setKey("AUX_TIMESTEP");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;
    }
    m_data.push_back(field);
  }
  return(m_data);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CPRBS - Clearpath battery status
//
// $CPRBS,<1>,<2>,<3>,<4>,<5>,<6>*hh<CR><LF>
//     <1>  Timestamp
//     < >  Battery ID
//     <2>  Battery bank voltage [V]
//     <3>  Lowest voltage cell [V]
//     <4>  Highest voltage cell [V]
//     <5>  Battery pack temperature [C]
//          NOTE:
//          As of Aug 2013, this message is being published with no [ID_BATTERY]
//          and a 0 value published for [TEMPERATUREC]
//          example: $CPRBS,172909.322,15.121597,15.121597,15.121597,0*76

vector<nmeaDatum> NMEA::formatCPRBS()
{
  string sval;
  double dval;
  string time;

  for (int i=1; i<m_fields.size(); i++){
    nmeaDatum field(m_source, i);

    switch(i){
    case 1:
      field.setKey("TIMESTAMP");
      field.setDatatype("STRING");
      sval = m_fields[i];
      time = sval;
      field.setValue(sval);
      field.setTime(time);
      break;
    
    /*
    case 2:
      field.setKey("BATTERY_ID");
      field.setDatatype("DOUBLE");
      break;
    */
case 2:
      field.setKey("VOLTAGE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 3:
      field.setKey("MIN_CELL_VOLTAGE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 4:
      field.setKey("MAX_CELL_VOLTAGE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 5:
      field.setKey("BATT_TEMP");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;
    }
    m_data.push_back(field);
  }
  return(m_data);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CPIMU - Clearpath raw IMU
//
// $CPIMU,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>*hh<CR><LF>
//     <1>  Timestamp
//     <2>  Roll Rate [deg/s]
//     <3>  Pitch Rate [deg/s]
//     <4>  Yaw Rate [deg/s]
//     <5>  Surge [m/s^2]
//     <6>  Sway [m/s^2]
//     <7>  Heave [m/s^2]
//     <8>  Timestamp of IMU reading. If blank, use <1>.

vector<nmeaDatum> NMEA::formatCPIMU()
{
  string sval;
  double dval;
  string time;

  for (int i=1; i<m_fields.size(); i++){
    nmeaDatum field(m_source, i);

    switch(i){
    case 1:
      field.setKey("TIMESTAMP");
      field.setDatatype("STRING");
      sval = m_fields[i];
      time = sval;
      field.setValue(sval);
      field.setTime(time);
      break;

    case 2:
      field.setKey("ROLL_RATE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 3:
      field.setKey("PITCH_RATE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 4:
      field.setKey("YAW_RATE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 5:
      field.setKey("SURGE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 6:
      field.setKey("SWAY");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 7:
      field.setKey("HEAVE");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      field.setTime(time);
      break;

    case 8:
      field.setKey("AUX_TIMESTEP");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      field.setTime(time);
      break;
    }
    m_data.push_back(field);
  }
  return(m_data);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// PGRME - Garmin Estimated Position Error
//
// $GPRME,<1>,<2>,<3>*hh<CR><LF>
//     <1>  Estimated horizontal position error [0.0 - 999.99]
//     <2>  Horizontal error units [M=meters]
//     <3>  Estimated vertical position error [0.0 - 999.99]
//     <4>  Vertical error units [M=meters]
//     <5>  Estimated position error [0.0 - 999.99]
//     <6>  Position error units [M=meters]

vector<nmeaDatum> NMEA::formatPGRME()
{
  string sval;
  double dval;

  for (int i=1; i<m_fields.size(); i++){
    nmeaDatum field(m_source, i);

    switch(i){
    case 1:
      field.setKey("HORIZONTAL_ERROR");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      break;

    case 2:
      field.setKey("HORIZONTAL_ERROR_UNITS");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 3:
      field.setKey("VERTICAL_ERROR");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      break;

    case 4:
      field.setKey("VERTICAL_ERROR_UNITS");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      break;

    case 5:
      field.setKey("POSITION_ERROR");
      field.setDatatype("DOUBLE");
      setDoubleOnString(dval, m_fields[i]);
      field.setValue(dval);
      break;

    case 6:
      field.setKey("POSITION_ERROR_UNITS");
      field.setDatatype("STRING");
      sval = m_fields[i];
      field.setValue(sval);
      break;
    }
    m_data.push_back(field);
  }
  return(m_data);
}
// ----------------------------------------------------------------------------

