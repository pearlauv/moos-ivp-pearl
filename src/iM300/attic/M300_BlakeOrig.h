/************************************************************/
/*    NAME: Blake Cole                                      */
/*    ORGN: MIT                                             */
/*    FILE: M300.h                                          */
/*    DATE: 01 APRIL 2020                                   */
/************************************************************/

#ifndef M300_HEADER
#define M300_HEADER

#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <queue>
#include <map>
#include "MBUtils.h"
#include "nmeaDatum.h"
#include "NMEA.h"
#include "MOOS/libMOOSGeodesy/MOOSGeodesy.h"
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

#define MAX_NMEA_BYTES  1000

class M300 : public AppCastingMOOSApp
{
// -------------------------------------------------
// STANDARD MOOS-APP FUNCTIONS ---------------------
// -------------------------------------------------
public:
  M300();
  ~M300() {};

protected: // Standard MOOSApp functions to overload
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();

protected: // Standard AppCastingMOOSApp function to overload 
  bool buildReport();

protected:
  void registerVariables();

// -------------------------------------------------
// APP-SPECIFIC FUNCTIONS --------------------------
// -------------------------------------------------
protected:
  bool  GeodesySetup();
  bool  ValidateTCP(std::string addr, int port);
  bool  OpenSocket();
  bool  Connect();
  bool  Send();
  bool  Receive();
  bool  Publish(nmeaDatum datumOut);

  bool  checkStale();
  bool  diffThrust(const double &desired_rudder,
                   const double &desired_thrust,
                   double &desired_thrustL,
                   double &desired_thrustR);

// -------------------------------------------------
// .MOOS CONFIGURATION VARIABLES -------------------
// -------------------------------------------------
private:
  std::string       m_IP;               // IP_ADDRESS
  int               m_port;             // PORT_NUMBER 
  double            m_max_rudder;       // MAX_RUDDER
  double            m_max_thrust;       // MAX_THRUST
  std::string       m_drive_mode;       // DRIVE_MODE
  
// -------------------------------------------------
// STATE VARIABLES ---------------------------------
// -------------------------------------------------
private:
  CMOOSGeodesy      m_geodesy;
  
   // IP:TCP Connection
  struct hostent*     server;
  struct sockaddr_in  server_addr;
  int                 m_socketfd;       // socket file descriptor (-1 = error)
  bool                m_socket_open;

  // NMEA Variables (published to MOOSDB)
  std::string       m_name_x;
  std::string       m_name_y;
  std::string       m_name_lat;
  std::string       m_name_lon;
  std::string       m_name_heading;
  std::string       m_name_speed;

  // NMEA Parsing
  std::string             m_nmea_buffer;  // incoming data buffer
  std::queue<std::string> m_sentenceQ;    // NMEA sentence queue
  std::queue<nmeaDatum>   m_datumQ;              // NMEA data queue
  std::map<std::string, nmeaDatum> m_lastOut;  // Messages "on deck"

  bool  validSentence(std::string nmea_sentence);
  bool  parseLineNMEA(std::string nmea_sentence);
  
  // Rudder & Thrust (transmitted to frontseat)
  double            m_desired_thrustL;
  double            m_desired_thrustR;
  double            m_desired_thrust;
  double            m_desired_rudder;
  bool              m_ivp_allstop;

  // Stale Message Detection
  bool              m_stale_check_enabled;
  bool              m_stale_mode;
  double            m_stale_threshold;
  unsigned int      m_count_stale;
  double            m_timestamp_desired_rudder;
  double            m_timestamp_desired_thrust;


  // Message Counters
  std::map<std::string, unsigned int>   m_msgs_from_moosdb;
  std::map<std::string, unsigned int>   m_msgs_to_moosdb;
  std::map<std::string, unsigned int>   m_msgs_from_front;
  std::map<std::string, unsigned int>   m_msgs_to_front;

  unsigned int  m_count_badSentence;
  unsigned int  m_count_badChecksum;
  unsigned int  m_count_undefinedType;
  unsigned int  m_count_undefinedKey;

  std::string   m_errorMsg;
};

#endif 
