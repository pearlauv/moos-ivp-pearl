/************************************************************/
/*    NAME: Charles Benjamin                                */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: SherlockTelemetry.cpp                           */
/*    DATE: August 17th, 2026                               */
/************************************************************/

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <sstream>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "ACTable.h"
#include "MBUtils.h"
#include "MOOS/libMOOS/Utils/MOOSUtilityFunctions.h"
#include "SherlockTelemetry.h"

using namespace std;

namespace {

const size_t MAX_RESPONSE_BYTES = 2 * 1024 * 1024;
const double PUBLISH_INTERVAL = 1.0;
const string FETCH_WARNING = "Sherlock metrics unavailable";
const string BATTERY_WARNING = "Sherlock battery metrics incomplete";
const string WIND_WARNING = "Sherlock Airmar wind metrics incomplete";

bool connectWithTimeout(int fd, const struct sockaddr* addr,
                        socklen_t addr_len, double timeout)
{
  int flags = fcntl(fd, F_GETFL, 0);
  if(flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    return false;

  int result = connect(fd, addr, addr_len);
  if(result < 0 && errno != EINPROGRESS)
    return false;

  if(result < 0) {
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(fd, &write_fds);

    struct timeval tv;
    tv.tv_sec = static_cast<int>(timeout);
    tv.tv_usec = static_cast<int>((timeout - tv.tv_sec) * 1000000.0);
    result = select(fd + 1, NULL, &write_fds, NULL, &tv);
    if(result <= 0)
      return false;

    int socket_error = 0;
    socklen_t error_len = sizeof(socket_error);
    if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &error_len) < 0 ||
       socket_error != 0) {
      errno = socket_error;
      return false;
    }
  }

  return fcntl(fd, F_SETFL, flags) == 0;
}

bool sendAll(int fd, const string& request)
{
  size_t sent = 0;
  while(sent < request.size()) {
    int flags = 0;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif
    ssize_t count = send(fd, request.data() + sent, request.size() - sent,
                         flags);
    if(count <= 0)
      return false;
    sent += static_cast<size_t>(count);
  }
  return true;
}

bool setPositiveDouble(double& target, const string& value)
{
  double parsed = 0;
  if(!setDoubleOnString(parsed, value) || parsed <= 0)
    return false;
  target = parsed;
  return true;
}

bool setPort(string& target, const string& value)
{
  unsigned int parsed = 0;
  if(!setUIntOnString(parsed, value) || parsed == 0 || parsed > 65535)
    return false;
  target = uintToString(parsed);
  return true;
}

bool extractMetric(const string& body, const string& name, double& value)
{
  istringstream input(body);
  string line;
  while(getline(input, line)) {
    if(!strBegins(line, name) || line.size() <= name.size())
      continue;

    char boundary = line[name.size()];
    if(boundary != '{' && boundary != ' ' && boundary != '\t')
      continue;

    size_t value_start = line.find_first_of(" \t", name.size());
    if(value_start == string::npos)
      continue;
    value_start = line.find_first_not_of(" \t", value_start);
    if(value_start == string::npos)
      continue;

    char* end = NULL;
    double parsed = strtod(line.c_str() + value_start, &end);
    if(end == line.c_str() + value_start || !isfinite(parsed))
      continue;

    value = parsed;
    return true;
  }
  return false;
}

} // namespace

//---------------------------------------------------------
SherlockTelemetry::SherlockTelemetry()
  : m_metrics_host("127.0.0.1"),
    m_metrics_port("9273"),
    m_metrics_path("/metrics"),
    m_poll_interval(2.0),
    m_http_timeout(1.0),
    m_battery_max_age(30.0),
    m_airmar_max_age(5.0),
    m_last_poll_time(-1),
    m_last_publish_time(-1),
    m_last_fetch_success_time(-1),
    m_battery_received_time(-1),
    m_wind_received_time(-1),
    m_battery_soc(-1),
    m_battery_charging(0),
    m_battery_source_age(-1),
    m_wind_speed(-1),
    m_airmar_source_age(-1),
    m_battery_connected(false),
    m_airmar_up(false),
    m_wind_measurement_valid(false),
    m_fetch_warning_active(false),
    m_battery_warning_active(false),
    m_wind_warning_active(false),
    m_fetch_count(0),
    m_fetch_success_count(0),
    m_battery_update_count(0),
    m_wind_update_count(0)
{
}

//---------------------------------------------------------
bool SherlockTelemetry::OnNewMail(MOOSMSG_LIST& NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  for(MOOSMSG_LIST::iterator p = NewMail.begin(); p != NewMail.end(); ++p) {
    string key = p->GetKey();
    if(key != "APPCAST_REQ")
      reportRunWarning("Unhandled Mail: " + key);
  }
  return true;
}

//---------------------------------------------------------
bool SherlockTelemetry::OnConnectToServer()
{
  registerVariables();
  return true;
}

//---------------------------------------------------------
bool SherlockTelemetry::Iterate()
{
  AppCastingMOOSApp::Iterate();

  double now = MOOSTime();
  if(m_last_poll_time < 0 || now - m_last_poll_time >= m_poll_interval) {
    ++m_fetch_count;

    string body;
    string error;
    if(fetchMetrics(body, error)) {
      double completed = MOOSTime();
      m_last_fetch_success_time = completed;
      ++m_fetch_success_count;
      setWarning(FETCH_WARNING, false, m_fetch_warning_active);

      string battery_error;
      string wind_error;
      bool battery_ok = parseBatteryMetrics(body, battery_error);
      bool wind_ok = parseWindMetrics(body, wind_error);
      setWarning(BATTERY_WARNING, !battery_ok, m_battery_warning_active);
      setWarning(WIND_WARNING, !wind_ok, m_wind_warning_active);

      m_last_error.clear();
      if(!battery_ok)
        m_last_error = battery_error;
      if(!wind_ok) {
        if(!m_last_error.empty())
          m_last_error += "; ";
        m_last_error += wind_error;
      }
    } else {
      m_last_error = error;
      setWarning(FETCH_WARNING, true, m_fetch_warning_active);
    }
    m_last_poll_time = MOOSTime();
  }

  now = MOOSTime();
  if(m_last_publish_time < 0 || now - m_last_publish_time >= PUBLISH_INTERVAL) {
    publishTelemetry();
    m_last_publish_time = now;
  }
  AppCastingMOOSApp::PostReport();
  return true;
}

//---------------------------------------------------------
bool SherlockTelemetry::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST params;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), params))
    reportConfigWarning("No config block found for " + GetAppName());

  for(STRING_LIST::iterator p = params.begin(); p != params.end(); ++p) {
    string orig = *p;
    string line = *p;
    string param = tolower(biteStringX(line, '='));
    string value = stripBlankEnds(line);
    bool handled = false;

    if(param == "metrics_host") {
      handled = !value.empty();
      if(handled)
        m_metrics_host = value;
    } else if(param == "metrics_port") {
      handled = setPort(m_metrics_port, value);
    } else if(param == "metrics_path") {
      handled = strBegins(value, "/");
      if(handled)
        m_metrics_path = value;
    } else if(param == "poll_interval") {
      handled = setPositiveDouble(m_poll_interval, value);
    } else if(param == "http_timeout") {
      handled = setPositiveDouble(m_http_timeout, value);
    } else if(param == "battery_max_age") {
      handled = setPositiveDouble(m_battery_max_age, value);
    } else if(param == "airmar_max_age") {
      handled = setPositiveDouble(m_airmar_max_age, value);
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();
  return true;
}

//---------------------------------------------------------
void SherlockTelemetry::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
}

//---------------------------------------------------------
bool SherlockTelemetry::fetchMetrics(string& body, string& error) const
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo* addresses = NULL;
  int lookup = getaddrinfo(m_metrics_host.c_str(), m_metrics_port.c_str(),
                           &hints, &addresses);
  if(lookup != 0) {
    error = gai_strerror(lookup);
    return false;
  }

  int fd = -1;
  for(struct addrinfo* addr = addresses; addr != NULL; addr = addr->ai_next) {
    fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if(fd < 0)
      continue;
#ifdef SO_NOSIGPIPE
    int no_sigpipe = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe,
               sizeof(no_sigpipe));
#endif
    if(connectWithTimeout(fd, addr->ai_addr, addr->ai_addrlen, m_http_timeout))
      break;
    close(fd);
    fd = -1;
  }
  freeaddrinfo(addresses);

  if(fd < 0) {
    error = "connection failed";
    return false;
  }

  struct timeval timeout;
  timeout.tv_sec = static_cast<int>(m_http_timeout);
  timeout.tv_usec = static_cast<int>((m_http_timeout - timeout.tv_sec) * 1000000.0);
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

  string request = "GET " + m_metrics_path + " HTTP/1.0\r\nHost: " +
                   m_metrics_host;
  request += "\r\nConnection: close\r\n\r\n";
  if(!sendAll(fd, request)) {
    error = "request send failed";
    close(fd);
    return false;
  }

  string response;
  char buffer[8192];
  while(response.size() < MAX_RESPONSE_BYTES) {
    ssize_t count = recv(fd, buffer, sizeof(buffer), 0);
    if(count == 0)
      break;
    if(count < 0) {
      error = (errno == EAGAIN || errno == EWOULDBLOCK) ?
              "response timed out" : strerror(errno);
      close(fd);
      return false;
    }
    response.append(buffer, static_cast<size_t>(count));
  }
  close(fd);

  if(response.size() >= MAX_RESPONSE_BYTES) {
    error = "response exceeded 2 MiB";
    return false;
  }

  size_t status_end = response.find("\r\n");
  size_t header_end = response.find("\r\n\r\n");
  if(status_end == string::npos || header_end == string::npos) {
    error = "malformed HTTP response";
    return false;
  }

  string status = response.substr(0, status_end);
  if(status.find(" 200 ") == string::npos) {
    error = "HTTP request did not return 200";
    return false;
  }

  body = response.substr(header_end + 4);
  return true;
}

//---------------------------------------------------------
bool SherlockTelemetry::parseBatteryMetrics(const string& body, string& error)
{
  double soc = 0;
  double charging = 0;
  double battery_connected = 0;
  double source_age = 0;

  bool complete = true;
  complete &= extractMetric(body, "pearl_cmp_ble_soc_percent_gauge", soc);
  complete &= extractMetric(body, "pearl_cmp_ble_charging_gauge", charging);
  complete &= extractMetric(body, "pearl_cmp_ble_connected_gauge", battery_connected);
  complete &= extractMetric(body, "pearl_cmp_ble_last_sample_age_seconds_gauge",
                            source_age);

  if(!complete) {
    error = "response is missing required battery metrics";
    return false;
  }

  m_battery_soc = soc;
  m_battery_charging = charging;
  m_battery_source_age = source_age;
  m_battery_connected = battery_connected >= 0.5;
  m_battery_received_time = MOOSTime();
  ++m_battery_update_count;
  return true;
}

//---------------------------------------------------------
bool SherlockTelemetry::parseWindMetrics(const string& body, string& error)
{
  double wind_speed = 0;
  double wind_valid = 0;
  double airmar_up = 0;
  double source_age = 0;

  bool complete = true;
  complete &= extractMetric(body,
                            "pearl_airmar_wind_speed_meters_per_second_gauge",
                            wind_speed);
  complete &= extractMetric(body, "pearl_airmar_wind_valid_gauge", wind_valid);
  complete &= extractMetric(body, "pearl_airmar_up_gauge", airmar_up);
  complete &= extractMetric(body, "pearl_airmar_last_sentence_age_seconds_gauge",
                            source_age);

  if(!complete) {
    error = "response is missing required Airmar wind metrics";
    return false;
  }

  m_wind_speed = wind_speed;
  m_wind_measurement_valid = wind_valid >= 0.5;
  m_airmar_up = airmar_up >= 0.5;
  m_airmar_source_age = source_age;
  m_wind_received_time = MOOSTime();
  ++m_wind_update_count;
  return true;
}

//---------------------------------------------------------
double SherlockTelemetry::batteryAge(double now) const
{
  if(m_battery_received_time < 0 || m_battery_source_age < 0)
    return -1;
  return m_battery_source_age + (now - m_battery_received_time);
}

//---------------------------------------------------------
double SherlockTelemetry::airmarAge(double now) const
{
  if(m_wind_received_time < 0 || m_airmar_source_age < 0)
    return -1;
  return m_airmar_source_age + (now - m_wind_received_time);
}

//---------------------------------------------------------
bool SherlockTelemetry::batteryValid(double now) const
{
  double age = batteryAge(now);
  return m_battery_received_time >= 0 && m_battery_connected &&
         m_battery_soc >= 0 && m_battery_soc <= 100 && age >= 0 &&
         age <= m_battery_max_age;
}

//---------------------------------------------------------
bool SherlockTelemetry::windValid(double now) const
{
  double age = airmarAge(now);
  return m_wind_received_time >= 0 && m_airmar_up &&
         m_wind_measurement_valid && m_wind_speed >= 0 && age >= 0 &&
         age <= m_airmar_max_age;
}

//---------------------------------------------------------
void SherlockTelemetry::publishTelemetry()
{
  double now = MOOSTime();
  double battery_age = batteryAge(now);
  double airmar_age = airmarAge(now);

  if(m_battery_received_time >= 0) {
    Notify("PEARL_BATTERY_SOC", m_battery_soc);
    Notify("PEARL_BATTERY_CHARGING", m_battery_charging >= 0.5 ? 1.0 : 0.0);
  }
  if(m_wind_received_time >= 0)
    Notify("PEARL_WIND_SPEED", m_wind_speed);

  Notify("PEARL_BATTERY_DATA_AGE", battery_age);
  Notify("PEARL_BATTERY_DATA_VALID", batteryValid(now) ? 1.0 : 0.0);
  Notify("PEARL_AIRMAR_DATA_AGE", airmar_age);
  Notify("PEARL_WIND_DATA_VALID", windValid(now) ? 1.0 : 0.0);
}

//---------------------------------------------------------
void SherlockTelemetry::setWarning(const string& warning, bool active,
                                    bool& warning_active)
{
  if(active && !warning_active) {
    reportRunWarning(warning);
    warning_active = true;
  } else if(!active && warning_active) {
    retractRunWarning(warning);
    warning_active = false;
  }
}

//------------------------------------------------------------
bool SherlockTelemetry::buildReport()
{
  double now = MOOSTime();
  double battery_age = batteryAge(now);
  double airmar_age = airmarAge(now);

  m_msgs << "Configuration" << endl;
  m_msgs << "  metrics_host:    " << m_metrics_host << endl;
  m_msgs << "  metrics_port:    " << m_metrics_port << endl;
  m_msgs << "  metrics_path:    " << m_metrics_path << endl;
  m_msgs << "  poll_interval:   " << m_poll_interval << " s" << endl;
  m_msgs << "  http_timeout:    " << m_http_timeout << " s" << endl;
  m_msgs << "  battery_max_age: " << m_battery_max_age << " s" << endl;
  m_msgs << "  airmar_max_age:  " << m_airmar_max_age << " s" << endl;
  m_msgs << endl;

  ACTable table(4);
  table << "Source | Value | Age (s) | Valid";
  table.addHeaderLines();
  table << "Battery SOC" << doubleToStringX(m_battery_soc, 1)
        << doubleToStringX(battery_age, 2) << boolToString(batteryValid(now));
  table << "Apparent wind" << doubleToStringX(m_wind_speed, 2)
        << doubleToStringX(airmar_age, 2) << boolToString(windValid(now));
  m_msgs << table.getFormattedString() << endl;
  m_msgs << "Charging: " << boolToString(m_battery_charging >= 0.5) << endl;
  m_msgs << "Fetches: " << m_fetch_count
         << ", HTTP successes: " << m_fetch_success_count << endl;
  double fetch_age = (m_last_fetch_success_time < 0) ?
                     -1 : now - m_last_fetch_success_time;
  m_msgs << "Last HTTP success age: " << doubleToStringX(fetch_age, 2)
         << " s" << endl;
  m_msgs << "Battery updates: " << m_battery_update_count
         << ", wind updates: " << m_wind_update_count << endl;
  if(!m_last_error.empty())
    m_msgs << "Last error: " << m_last_error << endl;

  return true;
}
