/************************************************************/
/*    NAME:                                               */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: BlueRoboticsPing.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "MBUtils.h"
#include "ACTable.h"
#include "BlueRoboticsPing.h"
// Old direct Ping1D serial include, kept for easy manual rollback:
// #include "abstract-link/abstract-link.h"

using namespace std;

//static Ping1d m_device;

// Old direct Ping1D serial ownership, kept for easy manual rollback:
// auto m_port = AbstractLink::openUrl("serial:/dev/ttyUSB1:115200");
// Ping1d m_device = Ping1d(*m_port.get());

static bool ParseHttpUrl(const string& url, string& host, string& port, string& path)
{
  string rest = url;
  if (strBegins(rest, "http://"))
    rest = rest.substr(7);
  else
    return false;

  size_t slash = rest.find('/');
  string authority = (slash == string::npos) ? rest : rest.substr(0, slash);
  path = (slash == string::npos) ? "/" : rest.substr(slash);

  size_t colon = authority.rfind(':');
  if (colon == string::npos) {
    host = authority;
    port = "80";
  } else {
    host = authority.substr(0, colon);
    port = authority.substr(colon + 1);
  }

  return !host.empty() && !port.empty();
}

static bool FetchHttpBody(const string& url, string& body, string& err)
{
  string host;
  string port;
  string path;
  if (!ParseHttpUrl(url, host, port, path)) {
    err = "Only http://host[:port]/path URLs are supported.";
    return false;
  }

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo* result = NULL;
  int gai = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
  if (gai != 0) {
    err = gai_strerror(gai);
    return false;
  }

  int fd = -1;
  for (struct addrinfo* rp = result; rp != NULL; rp = rp->ai_next) {
    fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (fd == -1)
      continue;
    if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;
    close(fd);
    fd = -1;
  }
  freeaddrinfo(result);

  if (fd == -1) {
    err = strerror(errno);
    return false;
  }

  struct timeval receive_timeout;
  receive_timeout.tv_sec = 2;
  receive_timeout.tv_usec = 0;
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &receive_timeout, sizeof(receive_timeout));

  string request = "GET " + path + " HTTP/1.0\r\nHost: " + host;
  request += "\r\nConnection: close\r\n\r\n";
  if (write(fd, request.c_str(), request.size()) < 0) {
    err = strerror(errno);
    close(fd);
    return false;
  }

  char buffer[1024];
  string response;
  ssize_t n = 0;
  while ((n = read(fd, buffer, sizeof(buffer))) > 0)
    response.append(buffer, n);
  close(fd);

  size_t split = response.find("\r\n\r\n");
  if (split == string::npos) {
    err = "HTTP response did not contain headers.";
    return false;
  }

  body = response.substr(split + 4);
  return true;
}

static bool ExtractJsonNumber(const string& body, const string& key, double& value)
{
  string needle = "\"" + key + "\":";
  size_t start = body.find(needle);
  if (start == string::npos)
    return false;
  start += needle.size();
  while (start < body.size() && body[start] == ' ')
    start++;
  size_t end = start;
  while (end < body.size() &&
         (isdigit(body[end]) || body[end] == '-' || body[end] == '.'))
    end++;
  if (end == start)
    return false;
  value = strtod(body.substr(start, end - start).c_str(), NULL);
  return true;
}


//---------------------------------------------------------
// Constructor

BlueRoboticsPing::BlueRoboticsPing()
{
  cout << "constructor" << endl;
  Notify("DEBUG", "constructor");
  // connect to device
}

//---------------------------------------------------------
// Destructor

//BlueRoboticsPing::~BlueRoboticsPing()
//{
//}

//---------------------------------------------------------
// Procedure: OnNewMail

bool BlueRoboticsPing::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();

    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString();
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();

    if(key == "SPEED_OF_SOUND")
      reportRunWarning("SPEED_OF_SOUND is owned by the Ping1D telemetry daemon.");
    else if(key != "APPCAST_REQ") // handled by AppCastingMOOSApp
      reportRunWarning("Unhandled Mail: " + key);
   }

   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool BlueRoboticsPing::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool BlueRoboticsPing::Iterate()
{
  AppCastingMOOSApp::Iterate();


  if (FetchState()) {
    // Old direct Ping1D serial request path is intentionally replaced by the
    // daemon state endpoint so MOOS and Telegraf do not compete for sonar.
    Notify("PING_CONNECTED", m_connected);
    Notify("PING_DISTANCE", m_distance_mm);
    Notify("PING_DISTANCE_MM", m_distance_mm);
    Notify("PING_DISTANCE_METERS", m_distance_meters);
    Notify("PING_DISTANCE_FEET", m_distance_feet);
    Notify("PING_CONFIDENCE", m_confidence);
    Notify("PING_SAMPLE_AGE_SECONDS", m_sample_age_seconds);
  }
    
  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool BlueRoboticsPing::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  //auto m_port = AbstractLink::openUrl("serial:/dev/ttyUSB0:115200");
  //m_device = Ping1d(*m_port.get());
  

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p=sParams.begin(); p!=sParams.end(); p++) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;
    if(param == "speed_of_sound") {
      m_speed_of_sound = stoi(value);
      cout << "speed config" << endl;
      handled = true;
    }
    else if(param == "ping_interval") {
      m_ping_interval = stoi(value);
      handled = true;
    }
    else if(param == "profile") {
      if (value == "1")
	m_profile = true;
      handled = true;
    }
    else if(param == "state_url") {
      m_state_url = stripBlankEnds(value);
      handled = true;
    }
    else if(param == "scan_start") {
      m_scan_start = stoi(value);
      handled = true;
    }
    else if(param == "scan_length") {
      m_scan_length = stoi(value);
      handled = true;
    }
    else if(param == "manual") {
      if (value == "1")
	m_auto = 0;
      handled = true;
    }
    
    if(!handled)
      reportUnhandledConfigWarning(orig);

  }

  // Old direct Ping1D serial initialization, kept for easy manual rollback:
  // m_device.initialize(m_ping_interval);
  // m_device.set_mode_auto(m_auto);
  // m_device.set_ping_enable(1);
  // m_device.set_speed_of_sound(m_speed_of_sound);
  // m_device.set_range(m_scan_start, m_scan_length);

  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables

void BlueRoboticsPing::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("SPEED_OF_SOUND", 0);
}


//------------------------------------------------------------
// Procedure: buildReport()

bool BlueRoboticsPing::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "File:                                       " << endl;
  m_msgs << "============================================" << endl;

  m_msgs << "STATE_URL: " << m_state_url << endl;
  m_msgs << "CONNECTED: " << m_connected << endl;
  m_msgs << "DISTANCE_MM: " << m_distance_mm << endl;
  m_msgs << "DISTANCE_METERS: " << m_distance_meters << endl;
  m_msgs << "DISTANCE_FEET: " << m_distance_feet << endl;
  m_msgs << "CONFIDENCE: " << m_confidence << endl;
  m_msgs << "SAMPLE AGE: " << m_sample_age_seconds << endl;
  m_msgs << "PROFILE: " << m_profile_str << endl;

  return(true);
}

bool BlueRoboticsPing::FetchState()
{
  string body;
  string err;
  if (!FetchHttpBody(m_state_url, body, err)) {
    reportRunWarning("Unable to fetch Ping1D state: " + err);
    return false;
  }
  return ParseStateBody(body);
}

bool BlueRoboticsPing::ParseStateBody(const string& body)
{
  double connected = 0;
  double distance = -1;
  double confidence = -1;
  double age = -1;

  bool ok = true;
  ok &= ExtractJsonNumber(body, "connected", connected);
  ok &= ExtractJsonNumber(body, "distance_mm", distance);
  ok &= ExtractJsonNumber(body, "confidence_pct", confidence);
  ok &= ExtractJsonNumber(body, "last_sample_age_seconds", age);
  if (!ok) {
    reportRunWarning("Ping1D state response is missing expected fields.");
    return false;
  }

  m_connected = static_cast<int>(connected);
  m_distance_mm = distance;
  m_distance_meters = distance / 1000.0;
  m_distance_feet = m_distance_meters * 3.28084;
  m_confidence = confidence;
  m_sample_age_seconds = age;
  return true;
}

