/************************************************************/
/*    NAME: Charles Benjamin                               */
/*    ORGN: MIT, Cambridge MA                              */
/*    FILE: Rendezvous.h                                   */
/*    DATE: August 2026                                    */
/************************************************************/

#ifndef Rendezvous_HEADER
#define Rendezvous_HEADER

#include <string>
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class Rendezvous : public AppCastingMOOSApp
{
 public:
  Rendezvous();
  ~Rendezvous();

 protected: // Standard MOOSApp functions to overload
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();

 protected: // Standard AppCastingMOOSApp function to overload
  bool buildReport();

 protected:
  void registerVariables();

  bool handleMailStart(const CMOOSMsg& msg);
  bool handleMailAbort(const CMOOSMsg& msg);
  bool handleMailRequest(const CMOOSMsg& msg);
  bool handleMailProposal(const CMOOSMsg& msg);
  bool handleMailResponse(const CMOOSMsg& msg);
  bool handleMailClearance(const CMOOSMsg& msg);
  bool handleMailNodeReport(const CMOOSMsg& msg);
  bool handleMailDouble(const CMOOSMsg& msg, double& value,
                        double& value_time);
  bool handleMailBool(const CMOOSMsg& msg, bool& value,
                      bool& value_set);

  void iterateUAV();
  void iteratePearl();
  bool beginRequest();
  bool proposeRendezvous(const std::string& request);
  bool acceptProposal(const std::string& proposal);
  void activatePearlTransit();
  void checkPearlArrival();
  void checkUAVArrival();
  void grantLandingClearance();
  void beginPrecisionLanding();
  void abortMission(const std::string& reason, bool notify_peer=true);
  void transitionTo(const std::string& state, const std::string& reason);
  void publishState(bool force=false);
  void publishPoint(bool active);

  bool sendMessage(const std::string& destination,
                   const std::string& variable,
                   const std::string& value);
  bool navIsFresh() const;
  bool uavIsReady(std::string& reason) const;
  bool mailIsTrue(const CMOOSMsg& msg) const;
  std::string makeSessionID() const;
  std::string pointString(double x, double y) const;

 private: // Configuration variables
  std::string m_role;
  std::string m_ownship;
  std::string m_peer_node;
  double m_uav_speed;
  double m_pearl_speed;
  double m_min_battery;
  double m_nav_stale_thresh;
  double m_request_timeout;
  double m_route_timeout;
  double m_transit_timeout;
  double m_arrival_radius;
  double m_arrival_dwell;
  double m_state_post_interval;
  bool m_require_health;
  bool m_require_battery;
  bool m_require_flight_state;

 private: // State variables
  std::string m_state;
  std::string m_reason;
  std::string m_session_id;
  std::string m_route_state;
  double m_state_enter_time;
  double m_last_state_post_time;
  double m_route_command_time;
  double m_route_state_time;
  double m_nav_x;
  double m_nav_y;
  double m_nav_x_time;
  double m_nav_y_time;
  double m_battery;
  double m_battery_time;
  double m_target_x;
  double m_target_y;
  double m_uav_report_x;
  double m_uav_report_y;
  double m_uav_report_time;
  double m_pearl_activation_time;
  double m_arrival_start_time;
  bool m_nav_x_set;
  bool m_nav_y_set;
  bool m_battery_set;
  bool m_health_ok;
  bool m_health_set;
  bool m_uav_armed;
  bool m_uav_armed_set;
  bool m_uav_in_air;
  bool m_uav_landed_set;
  bool m_target_set;
  bool m_pearl_activation_pending;
  bool m_pearl_arrived;
  bool m_uav_arrived;
  bool m_clearance_received;
  bool m_completion_sent;
  bool m_config_valid;
  unsigned int m_requests_sent;
  unsigned int m_proposals_sent;
  unsigned int m_acceptances_sent;
  unsigned int m_clearances_sent;
  unsigned int m_aborts;
};

#endif
