/************************************************************/
/*    NAME: Blake Cole                                      */
/*    ORGN: MIT                                             */
/*    FILE: M300.cpp                                        */
/*    DATE: 01 APRIL 2020                                   */
/************************************************************/

#include <iterator>
#include "ACTable.h"
#include "M300.h"

using namespace std;

//---------------------------------------------------------
// Constructor

M300::M300()
{
  // Configuration variables  (overwritten by .moos params)
  m_IP           = "localhost"; // default IP_ADDRESS
  m_port         = 29500;       // default PORT_NUMBER
  m_max_rudder   = 30.0;        // default MAX_RUDDER (+/-)
  m_max_thrust   = 100.0;       // default MAX_THRUST (+/-)
  m_drive_mode   = "normal";    // default DRIVE_MODE ("normal"|"aggro")
  
  // IP:TCP Connection
  m_socketfd              = 0;     // file descriptor

  // GPS Variable Names (published to MOOSDB)
  m_name_x             = "NAV_X";
  m_name_y             = "NAV_Y";
  m_name_lat           = "NAV_LAT";
  m_name_lon           = "NAV_LON";
  m_name_heading       = "NAV_HEADING";
  m_name_speed         = "NAV_SPEED";

  // Rudder & Thrust Variables (transmitted to frontseat)
  m_desired_thrustL           = 0.0;
  m_desired_thrustR           = 0.0;
  m_desired_thrust            = 0.0;
  m_desired_rudder            = 0.0;
  m_ivp_allstop               = true;

  // Stale Message Detection
  m_stale_check_enabled       = false;
  m_stale_mode                = false;
  m_stale_threshold           = 1.5;
  m_count_stale               = 0;
  m_timestamp_desired_rudder  = MOOSTime();
  m_timestamp_desired_thrust  = MOOSTime();

  // Message Counters -- might not have to initialize maps
  /*
  m_msgs_from_moosdb["IVPHELM_ALLSTOP"]  = 0;
  m_msgs_from_moosdb["DESIRED_THRUST"]   = 0;
  m_msgs_from_moosdb["DESIRED_RUDDER"]   = 0;
  m_msgs_to_moosdb[m_name_x]             = 0;
  m_msgs_to_moosdb[m_name_y]             = 0;
  m_msgs_to_moosdb[m_name_lat]           = 0;
  m_msgs_to_moosdb[m_name_lon]           = 0;
  m_msgs_to_moosdb[m_name_heading]       = 0;
  m_msgs_to_moosdb[m_name_speed]         = 0;
  m_msgs_from_front["GPGGA"]             = 0;
  m_msgs_from_front["GPRMC"]             = 0;
  m_msgs_from_front["GPGSA"]             = 0;
  m_msgs_from_front["CPNVG"]             = 0;
  m_msgs_from_front["CPNVR"]             = 0;
  m_msgs_from_front["CPRCM"]             = 0;
  m_msgs_from_front["CPRBS"]             = 0;
  m_msgs_from_front["CPIMU"]             = 0;
  m_msgs_to_front["PYDIR"]               = 0;
  */

  m_count_badSentence                    = 0;
  m_count_badChecksum                    = 0;
  m_count_undefinedType                  = 0;
  m_count_undefinedKey                   = 0;
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool M300::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  //------------------------------------------------------
  // HANDLE PARAMETERS IN .MOOS FILE ---------------------
  //------------------------------------------------------
  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for (p = sParams.begin(); p != sParams.end(); p++) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    // ------------ Frontseat IP Address ------------
    bool handled = false;
    if(param == "stale_thresh")
      handled = setPosDoubleOnString(m_stale_threshold, value);

    else if(param == "ip_address") {
      handled = isValidIPAddress(value);
      m_IP = value;
    }

    // -------------- TCP Port Number ---------------
    else if(param == "port_number") {
      handled = setIntOnString(m_port, value);
      if(m_port < 0 || m_port > 65335) {
        m_port = -1;
        reportConfigWarning("PORT_NUMBER out of range [0,65335]");
      }
    }

    // ----------- Maximum Rudder Angle -------------
    else if(param == "max_rudder") {
      handled = setDoubleOnString(m_max_rudder, value);
      if (m_max_rudder <= 0.0 || m_max_rudder > 180.0)
        reportConfigWarning("MAX_RUDDER out of range [0,180]");
    }

    // --------- Maximum Thrust Percentage ----------
    else if(param == "max_thrust" ) {
      handled = setDoubleOnString(m_max_thrust, value);
      if(m_max_thrust <= 0.0 || m_max_thrust > 100.0)
        reportConfigWarning("MAX_THRUST out of range [0,100]");
    }

    // ----------------- Drive Mode -----------------
    if(param == "drive_mode") {
      if(value == "normal" || value == "aggro") {
        m_drive_mode = value;
        handled = true;
      }
      else
        reportConfigWarning("Invalid DRIVE_MODE ['normal' or 'aggro'].");
    }
    
    if(!handled)
      reportUnhandledConfigWarning(orig);
  }
  
  registerVariables();

  //------------------------------------------------------
  // INITIALISE GEODESY, OPEN CONNECTION TO FRONTSEAT
  //------------------------------------------------------
  if (GeodesySetup())
    if (ValidateTCP(m_IP, m_port))
      if (OpenSocket())
        m_socket_open = Connect();
  
  return(true);
}


//---------------------------------------------------------
// Procedure: OnConnectToServer

bool M300::OnConnectToServer()
{
   registerVariables();
   return(true);
}


//---------------------------------------------------------
// Procedure: registerVariables

void M300::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("IVPHELM_ALLSTOP", 0);
  Register("DESIRED_THRUST",  0);
  Register("DESIRED_RUDDER",  0);
}


//---------------------------------------------------------
// Procedure: OnNewMail

bool M300::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    double mtime  = msg.GetTime();
    string key    = msg.GetKey();
    double dval   = msg.GetDouble();
    string sval   = msg.GetString();

#if 0 // Keep these around just for template
    string comm  = msg.GetCommunity(); 
    string msrc  = msg.GetSource();  
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif

    // ------------- IvP Help ALLSTOP ----------------
    if (key == "IVPHELM_ALLSTOP"){
      m_ivp_allstop = (toupper(sval) != "CLEAR");
      m_msgs_from_moosdb[key] += 1;
    }

    // -------------- DESIRED THRUST -----------------
    else if (key == "DESIRED_THRUST"){
      m_stale_check_enabled = true;
      m_timestamp_desired_thrust = mtime;
      m_desired_thrust = dval;
      m_msgs_from_moosdb[key] += 1;
    }

    // --------------- DESIRED RUDDER ----------------
    else if (key == "DESIRED_RUDDER"){
      m_stale_check_enabled = true;
      m_timestamp_desired_rudder = mtime;
      m_desired_rudder = dval;
      m_msgs_from_moosdb[key] += 1;
    }

    else if (key != "APPCAST_REQ") // handled by AppCastingMOOSApp
       reportRunWarning("Unhandled Mail: " + key);
   }

  return(true);
}


//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool M300::Iterate()
{
  AppCastingMOOSApp::Iterate();

  // (1) Check for staleness (only afer first command issued)
  if (m_stale_check_enabled){
    bool stale_input = checkStale();

    // Transition INTO stale mode
    if (stale_input && !m_stale_mode){
      m_stale_mode = true;
      reportRunWarning("Stale Command Detected: Stopping Vehicle");
    }

    // Transition OUT OF stale mode
    if (!stale_input){
      m_stale_mode = false;
      retractRunWarning("Stale Command Detected: Stopping Vehicle");
    }
  }

  // (2) Check for IVPHELM_ALLSTOP flag
  if (m_ivp_allstop){
    m_desired_thrust  = 0;
    m_desired_rudder  = 0;
  }

  bool thrustOK = diffThrust(m_desired_rudder, m_desired_thrust,
                             m_desired_thrustL, m_desired_thrustR);

  // (3) SEND command to frontseat
  if (m_socket_open && thrustOK){
    bool sendOK = Send();
    if (!sendOK)
      reportRunWarning("Failure sending command to vehicle.");
  }

  // (4) RECEIVE NMEA messages from frontseat
  bool recdOK = Receive();

  // if messages available, collect, iterate, publish
  //while (!m_datumQ.empty()){
  //  nmeaDatum datumOut = m_datumQ.front();
  //  m_datumQ.pop();
  //  bool pubOK = Publish(datumOut);
  //}
  
  AppCastingMOOSApp::PostReport();
  return(true);
}


//---------------------------------------------------------
// Procedure: GeodesySetup
//   Purpose: Initialize geodesy object with lat/lon origin.
//            Used for LatLon2LocalUTM conversion.

bool M300::GeodesySetup()
{
  double LatOrigin = 0.0;
  double LonOrigin = 0.0;

  // Get Latitude Origin from .MOOS Mission File
  bool latOK = m_MissionReader.GetValue("LatOrigin", LatOrigin);
  if (!latOK){
    reportConfigWarning("Latitude origin missing in MOOS file.");
    return(false);
  }

  // Get Longitude Origin from .MOOS Mission File
  bool lonOK = m_MissionReader.GetValue("LongOrigin", LonOrigin);
  if (!lonOK){
    reportConfigWarning("Longitude origin missing in MOOS file.");
    return(false);
  }

  // Initialise CMOOSGeodesy object
  bool geoOK = m_geodesy.Initialise(LatOrigin, LonOrigin);
  if (!geoOK){
    reportConfigWarning("CMOOSGeodesy::Initialise() failed. Invalid origin.");
    return(false);
  }
  return(true);
}


//---------------------------------------------------------
// Procedure: ValidateTCP
//   Purpose: Configure IP:TCP link

bool M300::ValidateTCP(string addr, int port)
{
  // Clearpath vehicle IP aliases stored in /etc/hosts
  set<string> aliases;
  aliases.insert("evan");
  aliases.insert("felix");
  aliases.insert("gus");
  aliases.insert("hal");
  aliases.insert("ida");
  aliases.insert("jing");
  aliases.insert("pabloE");
  aliases.insert("pabloF");
  aliases.insert("pabloG");

  // Check for valid IP address and port number
  if (!isValidIPAddress(addr) && (aliases.find(addr)==aliases.end())){
    reportConfigWarning("Invalid IP address in .moos mission file.");
    return(false);
  }
  else if (port != (int)port){
    reportConfigWarning("Invalid port number in .moos mission file.");
    return(false);
  }

  // Fill the linked list of host_info struct with the connection info
  server = gethostbyname(addr.c_str());
  if (server == NULL){
    string sPort = intToString(m_port);
    string warning = "Error with gethostbyname(): ";
    warning += hstrerror(h_errno);
    warning += addr + "] Port [" + sPort + "] .";
    reportConfigWarning(warning);
    return(false);
  }
  else {
    bzero((char*) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char*) server->h_addr,
          &server_addr.sin_addr.s_addr,
          server->h_length);
    server_addr.sin_port = htons(m_port);
    return(true);
  }
}


//---------------------------------------------------------
// Procedure: OpenSocket
//   Purpose: Open socket (IP:TCP)

bool M300::OpenSocket()
{
  m_socketfd = socket(AF_INET, SOCK_STREAM, 0);

  //------------------------------------------------------
  // CHECK FOR SOCKET ERRORS -----------------------------
  //------------------------------------------------------
  // errno (/usr/include/sys/errno.h)
  //    - declared as a result of including sys/socket.h
  //    - is an integer that refers to various OS errors.
  //    - if socket() returns an error (-1), then errno is set.
  //    - Convert int value to human readable with strerror().
  int tmp = errno;
  if (m_socketfd == -1){
    // Socket error
    string warning = "TCP socket open error: ";
    warning += strerror(tmp);
    reportConfigWarning(warning);
    return(false);
  }
  else {
    // Socket valid
    string sock_str = intToString(m_socketfd);
    reportEvent("Socket open. (" + sock_str + ")");
    return(true);
  }
}


//---------------------------------------------------------
// Procedure: Connect
//   Purpose: Connect socket (IP:TCP)

bool M300::Connect()
{
  int status = connect(m_socketfd,
                       (struct sockaddr*) &server_addr,
                       sizeof(server_addr));

  // See OpenSocket() for an explanation of errno
  int tmp = errno;
  if (status == -1){
    string warning = "TCP socket connect error: ";
    warning += strerror(tmp);
    reportConfigWarning(warning);
    return(false);
  }
  else {
    string status_str = intToString(status);
    reportEvent("Connection open. (" + status_str + ")");
    return(true);
  }
}


//---------------------------------------------------------
// Procedure: checkStale
//   Purpose: Determine if command sent to fronseat is stale

bool M300::checkStale()
{
  double current_time = MOOSTime();

  double lagRudder = current_time - m_timestamp_desired_rudder;
  double lagThrust = current_time - m_timestamp_desired_thrust;

  bool stale_rudder = (lagRudder > m_stale_threshold);
  bool stale_thrust = (lagThrust > m_stale_threshold);

  if(stale_rudder)
    m_count_stale++;
  if(stale_thrust)
    m_count_stale++;

  if(stale_rudder || stale_thrust) {
    m_desired_thrustL       = 0.0;
    m_desired_thrustR       = 0.0;
    m_desired_rudder        = 0.0;
    m_desired_thrust        = 0.0;
  }
  
  return(stale_rudder || stale_thrust);
}


//---------------------------------------------------------
// Procedure: Send
//   Purpose: Send command to frontseat (Clearpath Heron)

bool M300::Send()
{

  // Publish command to MOOSDB
  Notify("PYDIR_THRUST_L", m_desired_thrustL);
  Notify("PYDIR_THRUST_R", m_desired_thrustR);

  string message = "$";
  message += "PYDIR,";
  message += doubleToString(m_desired_thrustL);
  message += ",";
  message += doubleToString(m_desired_thrustR);
  message += "*\r\n";

  ssize_t bytes_sent;
  int len = strlen(message.c_str());

  // !! SEND MESSAGE TO FRONTSEAT !!
  bytes_sent = send(m_socketfd, message.c_str(), len, 0);

  // See OpenSocket() for an explanation of errno
  int tmp = errno;
  if(bytes_sent == -1) {
    string warning = "TCP send error: ";
    warning += strerror(tmp);
    reportRunWarning(warning);
    return(false);
  }
  else {
    m_msgs_to_front["PYDIR"] += 1;
    return(true);
  }
}


//---------------------------------------------------------
// Procedure: Receive
//   Purpose: Receive data from frontseat (Clearpath Heron)

bool M300::Receive()
{
  // ---- INITIALIZE CONTAINER FOR INCOMING MESSAGE ---- //
  ssize_t numBytes;
  char nmea_incoming[MAX_NMEA_BYTES];
  

  // --------- READ CHAR BLOCK FROM TCP CLIENT --------- //
  numBytes = recv(this->m_socketfd, nmea_incoming, MAX_NMEA_BYTES, 0);

  // See OpenSocket() for an explanation of errno
  int tmp = errno;
  if(numBytes == -1) {
    string warning = "TCP Receive error: ";
    warning += strerror(tmp);
    reportRunWarning(warning);
    return(false);
  }

  if (numBytes == 0)
    return(false);

  // ------- CONVERT CHAR BLOCK TO STRING BLOCK ------- //
  string nmea_str(nmea_incoming, numBytes);
  if (nmea_str.empty())
    return(false);

  // ------- PARSE RAW NMEA BLOCK INTO SENTENCES ------ //
  vector<string> lines;

  // Append new string to leftovers in buffer
  m_nmea_buffer += nmea_incoming;

  // Search for the position of the first "$"
  size_t startNMEA =  m_nmea_buffer.find('$');

  // If "$" not found, no NMEA sentence to read.
  if (startNMEA == std::string::npos){
    m_nmea_buffer = "";
    return(false);
  }

  // If "$" found, delete everything before the first "$",
  // and replace all <CR><NL> instances of "\r\n" with "\n"
  m_nmea_buffer = m_nmea_buffer.substr(startNMEA);
  m_nmea_buffer = findReplace(m_nmea_buffer, "\r\n", "\n");

  // Parse raw NMEA into vector of single NMEA setence strings
  lines = parseString(m_nmea_buffer, "\n");
  int numLines = lines.size();

  // If last line is not a full sentence, save in buffer for next time
  string last = lines[numLines - 1];
  unsigned int lastSize = last.size();
  if ((lastSize >= 3) && (last.at(lastSize - 3) != '*'))
    m_nmea_buffer = last;
  else
    m_nmea_buffer = "";

  // Add lines to queue (ensure saved across iterations)
  for (int i=0; i<numLines; i++){
    std::string line = lines[i];
    if (validSentence(line))
      m_sentenceQ.push(line);
    else
      m_count_badSentence++;
  }

  //- PARSE SINGLE SENTENCE INTO DATA, RETURN AS VECTOR -//
  if (!m_sentenceQ.empty()){
    string nmea_sentence = m_sentenceQ.front();
    m_sentenceQ.pop();

    // Initialze NMEA object & parse sentence
    NMEA rxNMEA(nmea_sentence);
    vector<nmeaDatum> data = rxNMEA.parseNMEA();
    m_msgs_from_front[rxNMEA.getSource()] += 1;

    // Add data to queue (ensure saved across iterations)
    vector<nmeaDatum>::reverse_iterator rit;
    for(rit=data.rbegin(); rit!=data.rend(); ++rit) {
      m_datumQ.push(*rit);
    }
  }

  // TO DO: Individual field validation
  // if (!m_datumQ.empty()){
  //   validate data point
  //   if valid, udpate m_status=STATUS_VALID

  if (!m_datumQ.empty()){
    Publish(m_datumQ.front());
    m_datumQ.pop();
  }
  
  return(true);
}


//---------------------------------------------------------
bool M300::validSentence(string nmea_sentence)
{
  if(nmea_sentence.empty()){
    m_count_badSentence++;
    return(false);
  }
  else if(nmea_sentence.at(0) != '$'){
    m_count_badSentence++;
    return(false);
  }
  else if(nmea_sentence.at(nmea_sentence.length() - 3) != '*'){
    m_count_badSentence++;
    return(false);
  }

  // -------------- VALIDATE CHECKSUM -------------- //
  else{
    string rx_checksum = rbiteString(nmea_sentence, '*'); // save checksum
    string rx_contents = nmea_sentence.substr(1);         // remove '$'
    string calc_checksum;

    if (rx_checksum.length() != 2)
    return(false);

    // Checksum Calculation:
    //  1. Convert all characters between '$' and '*' to ASCII
    //  2. Initialize checksum = 0
    //  3. Iterate through each char in nmea_sentence, and apply
    //     XOR bitwise to each char w.r.t. the prior checksum value
    //  4. Convert final checksum value to hexidecimal
    //
    //  e.g.
    //  ---(1st iteration)----------------------------------
    //     rx_contents[0] = G = 71 (ASCII) --> 1 0 0 0 1 1 1
    //     checksum           = 0  (ASCII) --> 0 0 0 0 0 0 0
    //                                        ---------------XOR
    //     new_checksum       = 71 (ASCII) <-- 1 0 0 0 1 1 1
    //
    //  ---(2nd iteration)----------------------------------
    //     rx_contents[1] = P = 80 (ASCII) --> 1 0 1 0 0 0 0
    //     checksum           = 71 (ASCII) --> 1 0 0 0 1 1 1
    //                                        ---------------XOR
    //     new_checksum       = 23 (ASCII) <-- 0 0 1 0 1 1 1

    unsigned char checksum = 0;
    std::string::iterator it;
    for (it = rx_contents.begin(); it != rx_contents.end(); it++)
      checksum ^= *it;

    // Convert checksum: char -> ASCII(double) -> hex (string)
    calc_checksum = doubleToHex((double)checksum);

    // If hex value is single digit, prepend with "0"
    if (calc_checksum.length() < 2)
      calc_checksum = "0" + calc_checksum;

    bool checksumOK = MOOSStrCmp(rx_checksum, calc_checksum);
    return(checksumOK);
  }
}


//---------------------------------------------------------
bool M300::diffThrust(const double &desired_rudder,
                      const double &desired_thrust,
                      double &desired_thrustL,
                      double &desired_thrustR)
{
  double delta, upper_lim, lower_lim;

  // NORMAL MODE:
  if(MOOSStrCmp(m_drive_mode, "normal")){
    upper_lim = 100;
    lower_lim = 0;
    delta = (desired_thrust/m_max_rudder)*m_desired_rudder;
    desired_thrustL = desired_thrust + delta;
    desired_thrustR = desired_thrust - delta;

    // Clip saturated values:
    desired_thrustL = vclip(desired_thrustL, lower_lim, upper_lim);
    desired_thrustR = vclip(desired_thrustR, lower_lim, upper_lim);

    return(true);
  }

  // AGGRO MODE:
  else if(MOOSStrCmp(m_drive_mode, "aggro")){
    upper_lim = 100;
    lower_lim = -100;
    double max_revthrust = -0.2*m_max_thrust;
    double max_delta = m_max_thrust - max_revthrust;
    delta = (max_delta/m_max_rudder)*m_desired_rudder;
    desired_thrustL = desired_thrust + (delta/2);
    desired_thrustR = desired_thrust - (delta/2);

    // Rebalance saturated values, preserving delta:
    if(desired_thrustL > upper_lim){
      double overL = desired_thrustL - upper_lim;
      desired_thrustL = upper_lim;
      desired_thrustR -= overL;
      if (desired_thrustR < 0)  // rescale -20 -> -100
        desired_thrustR *= -(m_max_thrust/max_revthrust);
    }
    else if(desired_thrustR > upper_lim){
      double overR = desired_thrustR - upper_lim;
      desired_thrustR = upper_lim;
      desired_thrustL -= overR;
      if (desired_thrustL < 0)  // rescale -20 -> -100
        desired_thrustL *= -(m_max_thrust/max_revthrust);
    }
    else if(desired_thrustL < max_revthrust){
      double underL = max_revthrust - desired_thrustL;
      desired_thrustL = lower_lim;
      desired_thrustR += underL;
    }
    else if(desired_thrustR < max_revthrust){
      double underR = max_revthrust - desired_thrustR;
      desired_thrustR = lower_lim;
      desired_thrustL += underR;
    }
    return(true);
  }

  // MODE ERROR:
  else {
    string warning = "DRIVE_MODE not recognized. Check .MOOS file.";
    reportRunWarning(warning);
    return(false);
  }
}


//---------------------------------------------------------
// Procedure: Publish
//   Purpose:

bool M300::Publish(nmeaDatum datumOut)
{
  string source  = datumOut.getSource();
  string key     = datumOut.getKey();

  if (key == "UTC_TIME"){
    Notify(key, datumOut.getSValue());
    m_lastOut[key] = datumOut;
  }
  else if (key == "LATITUDE"){
    m_name_lat += "_" + toupper(source);
    Notify(m_name_lat, datumOut.getDValue());
    m_lastOut[m_name_lat] = datumOut;
    //use MOOSGeodesy to produce 'NAV_X' value
  }
  else if (key == "LONGITUDE"){
    m_name_lon += "_" + toupper(source);
    Notify(m_name_lon, datumOut.getDValue());
    m_lastOut[m_name_lon] = datumOut;
    //use MOOSGeodesy to produce 'NAV_Y' value
  }
  else if (key == "SPEED"){
    m_name_speed += "_" + toupper(source);
    Notify(m_name_speed, datumOut.getDValue());
    m_lastOut[m_name_speed] = datumOut;
  }
  else if (key == "COURSE"){
    m_name_heading += "_" + toupper(source);
    Notify(m_name_heading, datumOut.getDValue());
    m_lastOut[m_name_heading] = datumOut;
  }
  else if (key == "ROLL"){
    Notify("NAV_ROLL", datumOut.getDValue());
    m_lastOut["NAV_ROLL"] = datumOut;
  }
  else if (key == "PITCH"){
    Notify("NAV_PITCH", datumOut.getDValue());
    m_lastOut["NAV_PITCH"] = datumOut;
  }
  return(true);
}

  
//------------------------------------------------------------
// Procedure: buildReport()

bool M300::buildReport() 
{
  m_msgs << "============================================" << endl;
  m_msgs << "File:                                       " << endl;
  m_msgs << "============================================" << endl;

  ACTable actab(4);
  actab << "Alpha | Bravo | Charlie | Delta";
  actab.addHeaderLines();
  actab << "one" << "two" << "three" << "four";
  m_msgs << actab.getFormattedString();

  return(true);
}
