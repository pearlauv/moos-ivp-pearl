/****************************************************************/
/*   NAME: Charles Benjamin                                     */
/*   ORGN: MIT, Cambridge MA                                    */
/*   FILE: Rendezvous_Info.cpp                                  */
/*   DATE: August 2026                                          */
/****************************************************************/

#include <cstdlib>
#include <iostream>
#include "Rendezvous_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"

using namespace std;

void showSynopsis()
{
  blk("SYNOPSIS:                                                       ");
  blk("  pRendezvous coordinates a PEARL-selected meeting point with  ");
  blk("  explicit UAV acceptance and PEARL-issued landing clearance.  ");
  blk("  Landing is fail-closed behind fresh separation and visual-   ");
  blk("  target gates; target acquisition tracks PEARL horizontally. ");
  blk("  The same binary runs in either uav or pearl role and reuses   ");
  blk("  pMediator, pRouteBuffer, and existing waypoint behaviors.     ");
}

void showHelpAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("Usage: pRendezvous file.moos [OPTIONS]                          ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("Options:                                                        ");
  mag("  --alias", "=<ProcessName>                                     ");
  blk("      Launch with the given process name.                       ");
  mag("  --example, -e", "  Display an example configuration.          ");
  mag("  --help, -h", "     Display this help message.                ");
  mag("  --interface, -i", "Display subscriptions and publications.    ");
  mag("  --version, -v", "  Display release information.                ");
  blk("                                                                ");
  exit(0);
}

void showExampleConfigAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("pRendezvous Example MOOS Configuration                          ");
  blu("=============================================================== ");
  blk("                                                                ");
  blk("ProcessConfig = pRendezvous                                     ");
  blk("{                                                               ");
  blk("  AppTick   = 4                                                 ");
  blk("  CommsTick = 4                                                 ");
  blk("                                                                ");
  blk("  role                = uav                                     ");
  blk("  ownship             = uav                                     ");
  blk("  peer_node           = pearl                                   ");
  blk("  uav_speed           = 3                                       ");
  blk("  pearl_speed         = 0.5                                     ");
  blk("  min_battery         = 25                                      ");
  blk("  require_battery     = false                                   ");
  blk("  require_health      = true                                    ");
  blk("  require_flight_state = true                                   ");
  blk("  nav_stale_thresh    = 10                                      ");
  blk("  request_timeout     = 20                                      ");
  blk("  transit_timeout     = 180                                     ");
  blk("  arrival_radius      = 4                                       ");
  blk("  arrival_dwell       = 2                                       ");
  blk("  max_peer_separation = 3                                       ");
  blk("  require_landing_target = true                                 ");
  blk("  landing_target_max_age = 0.5                                  ");
  blk("  landing_target_lock_dwell = 2                                 ");
  blk("  landing_target_acquire_timeout = 30                           ");
  blk("  landing_target_max_offset = 1.5                               ");
  blk("  landing_target_max_angle = 0.20                               ");
  blk("  expected_target_num = 0                                      ");
  blk("  reacquire_update_interval = 2                                 ");
  blk("  reacquire_update_distance = 1                                 ");
  blk("}                                                               ");
  blk("                                                                ");
  exit(0);
}

void showInterfaceAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("pRendezvous INTERFACE                                           ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("COMMON SUBSCRIPTIONS:                                           ");
  blk("  NAV_X, NAV_Y, NODE_REPORT, RENDEZVOUS_ABORT                   ");
  blk("                                                                ");
  blk("UAV SUBSCRIPTIONS:                                              ");
  blk("  RENDEZVOUS_START, RENDEZVOUS_PROPOSAL, LANDING_CLEARANCE      ");
  blk("  UAV_IS_ARMED, UAV_LANDED_STATE, UAV_HEALTH_ALL_OK             ");
  blk("  UAV_BATTERY_PERCENT, ROUTE_BUFFER_VEHICLE_STATE               ");
  blk("  UAV_PREC_LAND_REQUEST (rejected; no manual bypass)            ");
  blk("  UAV_LANDING_TARGET_AVAILABLE/AGE/TARGET_NUM/POSITION_VALID    ");
  blk("  UAV_LANDING_TARGET_X/Y/ANGLE_X/ANGLE_Y                        ");
  blk("                                                                ");
  blk("PEARL SUBSCRIPTIONS:                                            ");
  blk("  RENDEZVOUS_REQUEST, RENDEZVOUS_RESPONSE                      ");
  blk("  PEARL_RENDEZVOUS_ARRIVED                                     ");
  blk("                                                                ");
  blk("PUBLICATIONS:                                                   ");
  blk("  NODE_MESSAGE_LOCAL, UAV_RENDEZVOUS_STATE or                   ");
  blk("  PEARL_RENDEZVOUS_STATE                                       ");
  blk("  UAV/PEARL_RENDEZVOUS_PHASE and _SESSION                      ");
  blk("  UAV role: ROUTE_BUFFER_COMMAND, UAV_PREC_LAND_COMMIT,         ");
  blk("            UAV_LANDING_GATE_*, UAV_PEER_DISTANCE              ");
  blk("  PEARL role: PEARL_RENDEZVOUS_UPDATE, PEARL_* controls,        ");
  blk("              PEARL_LANDING_GATE_*, RENDEZVOUS_POINT           ");
  blk("                                                                ");
  exit(0);
}

void showReleaseInfoAndExit()
{
  showReleaseInfo("pRendezvous", "gpl");
  exit(0);
}
