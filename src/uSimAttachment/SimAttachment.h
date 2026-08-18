/************************************************************/
/*    NAME: Charles Benjamin                                */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: SimAttachment.h                                 */
/*    DATE: August 18th, 2026                               */
/************************************************************/

#ifndef SIM_ATTACHMENT_HEADER
#define SIM_ATTACHMENT_HEADER

#include <string>
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class SimAttachment : public AppCastingMOOSApp
{
public:
  SimAttachment();

protected:
  bool OnNewMail(MOOSMSG_LIST &NewMail) override;
  bool Iterate() override;
  bool OnConnectToServer() override;
  bool OnStartUp() override;
  bool buildReport() override;

private:
  struct TimedValue {
    double value = 0.0;
    double time = -1.0;
  };

  void registerVariables();
  bool handleMailDouble(const CMOOSMsg &msg, TimedValue &target);
  bool handleMailAttachment(const CMOOSMsg &msg);
  bool isFresh(const TimedValue &value) const;
  bool poseFresh() const;
  void postPose(double x, double y, double heading, double speed);
  void postAttachedPose();
  void postState();
  std::string resetSpec(double x, double y, double heading) const;
  std::string stateString() const;

private:
  double m_input_max_age;
  bool m_initially_attached;
  bool m_attachment_requested;
  bool m_attached;
  bool m_detaching;

  double m_detach_x;
  double m_detach_y;
  double m_detach_heading;

  TimedValue m_source_x;
  TimedValue m_source_y;
  TimedValue m_source_heading;
  TimedValue m_source_speed;

  std::string m_last_state;
  double m_last_state_post;
  unsigned int m_attach_count;
  unsigned int m_detach_count;
};

#endif
