/*****************************************************************/
/*    NAME: Michael Benjamin                                     */
/*    ORGN: Dept of Mechanical Engineering, MIT, Cambridge MA    */
/*    FILE: M300_Info.cpp                                        */
/*    DATE: 01 APRIL 2020                                        */
/*                                                               */
/* This is unreleased BETA code. No permission is granted or     */
/* implied to use, copy, modify, and distribute this software    */
/* except by the author(s), or those designated by the author.   */
/*****************************************************************/

#include <cstdlib>
#include <iostream>
#include "M300_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"

using namespace std;

//----------------------------------------------------------------
// Procedure: showSynopsis

void showSynopsis()
{
  blk("SYNOPSIS:                                                       ");
  blk("------------------------------------                            ");
  blk("  The iM300 application serves as an intermediary between       ");
  blk("  a MOOS community (backseat) and a vehicle (frontseat).        ");
  blk("                                                                ");
  blk("  iM300 performs two essential tasks:                           ");
  blk("                                                                ");
  blk("    1) CONNECT: [FRONTSEAT] <-(IP:TCP)-> [BACKSEAT]             ");
  blk("                                                                ");
  blk("    2) TRANSLATE: [NMEA MESSAGES] <-(iM300)-> [MOOS MESSAGES]   ");
  blk("                                                                ");
}

//----------------------------------------------------------------
// Procedure: showHelpAndExit

void showHelpAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("Usage: iM300 file.moos [OPTIONS]                   ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("Options:                                                        ");
  mag("  --alias","=<ProcessName>                                      ");
  blk("      Launch iM300 with the given process name                  ");
  blk("      rather than iM300.                                        ");
  mag("  --example, -e                                                 ");
  blk("      Display example MOOS configuration block.                 ");
  mag("  --help, -h                                                    ");
  blk("      Display this help message.                                ");
  mag("  --interface, -i                                               ");
  blk("      Display MOOS publications and subscriptions.              ");
  mag("  --version,-v                                                  ");
  blk("      Display the release version of iM300.                     ");
  blk("                                                                ");
  blk("Note: If argv[2] does not otherwise match a known option,       ");
  blk("      then it will be interpreted as a run alias. This is       ");
  blk("      to support pAntler launching conventions.                 ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showExampleConfigAndExit

void showExampleConfigAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("iM300 Example MOOS Configuration                                ");
  blu("=============================================================== ");
  blk("                                                                ");
  blk("ProcessConfig = iM300                                           ");
  blk("{                                                               ");
  blk("  AppTick   = 10                                                ");
  blk("  CommsTick = 10                                                ");
  blk("                                                                ");
  blk("  ip_address     = 192.168.10.1 // frontseat IP address         ");
  blk("  port_number    = 29500        // port number at IP address    ");
  blk("  gps_prefix     = NAV_         // prepended to MOOS variables  ");
  blk("  nav_prefix     = NAV                                          ");
  blk("  compass_prefix = COMPASS                                      ");
  blk("                                                                ");
  blk("  max_rudder     = 50.0         // Max Rudder Angle  [+/- deg]  ");
  blk("  max_thrust     = 100.0        // Max Thrust        [+/- %]    ");
  blk("                                                                ");
  blk("  drive_mode     = normal       // [normal], aggro, rotate,     ");
  blk("                                // or direct                    ");
  blk("  comms_type     = client       // [server], client             ");
  blk("  stale_thresh   = 15           // [1.5] seconds                ");
  blk("                                                                ");
  blk("  ivp_allstop    = false        // Default is true              ");
  blk("                                                                ");
  blk("  ignore_msg = $GPGLL, $GPZDA                                   ");
  blk("  ignore_msg = $GPGSV, $GPVTG                                   ");
  blk("  ignore_checksum_errors = true // [false], true                ");
  blk("                                                                ");
  blk("  max_appcast_events = 8                                        ");
  blk("  max_appcast_run_warnings = 10                                 ");
  blk("                                                                ");
  blk("  warn_bad_nmea_len  = false                                    ");
  blk("  warn_bad_nmea_nend = false                                    ");
  blk("  warn_bad_nmea_rend = false                                    ");
  blk("  warn_bad_nmea_form = false                                    ");
  blk("  warn_bad_nmea_chks = false                                    ");
  blk("  warn_bad_nmea_key  = false                                    ");
  blk("                                                                ");
  blk("  // heading_source options are gps, imu, or auto where auto    ");
  blk("  // uses imu when available and not timed out as defined in    ");
  blk("  // nav_thresh param.                                          ");
  blk("  heading_source = auto                                         ");
  blk("                                                                ");
  blk("  // threshold in seconds, default is 1.5                       ");
  blk("  stale_nvg_msg_thresh = 2                                      ");
  blk("                                                                ");
  blk("  // Publish the body-relative velocities                       ");
  blk("  // for adaptive controller                                    ");
  blk("  publish_body_vel = true                                       ");
  blk("                                                                ");
  blk("  stale_compass_thresh = 1    // threshold for compass          ");
  blk("                              // measurement used in calc of    ");
  blk("                              // body-relative vel              ");
  blk("                                                                ");
  blk("  mag_declination_deg = -14.2 // for Boston                     ");
  blk("                                                                ");
  blk("  // Simulated Faults                                           ");
  blk("  add_thruster_fault_factor = true                              ");
  blk("  fault_factor_thruster_L   = 1.0                               ");
  blk("  fault_factor_thruster_R   = 1.0                               ");
  blk("                                                                ");
  blk("}                                                               ");
  blk("                                                                ");
  exit(0);
}


//----------------------------------------------------------------
// Procedure: showInterfaceAndExit

void showInterfaceAndExit()
{
  blu("=============================================================== ");
  blu("iM300 INTERFACE                                    ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("SUBSCRIPTIONS:                                                  ");
  blk("------------------------------------                            ");
  blk("  DESIRED_THRUST        (double) Desired thrust            [%]  ");
  blk("  DESIRED_RUDDER        (double) Desired rudder angle      [deg]");
  blk("                                                                ");
  blk("  ------------ Only possible in direct drive mode ------------  ");
  blk("  REQ_THRUSTER_L        (double) Requested left thrust spd [%]  ");
  blk("  REQ_THRUSTER_R        (double) Requested right thrust spd[%]  "); 
  blk("                                                                ");
  blk("PUBLICATIONS:                                                   ");
  blk("------------------------------------                            ");
  blk("  [gps_prefix]_X        (double)  X-position on local grid [m]  ");
  blk("  [gps_prefix]_Y        (double)  Y-position on local grid [m]  ");
  blk("  [gps_prefix]_LAT      (double)  Latitude                 [deg]");
  blk("  [gps_prefix]_LON      (double)  Longitude                [deg]");
  blk("  [gps_prefix]_SPEED    (double)  Speed                    [m/s]");
  blk("  [gps_prefix]_HEADING  (double)  Heading from true North  [deg]");
  blk("                                                                ");
  blk("  IMU_ROLL              (double)  Roll                     [deg]");
  blk("  IMU_PITCH             (double)  Pitch                    [deg]");
  blk("  IMU_YAW               (double)  Yaw (Heading)            [deg]");
  blk("                                                                ");
  blk("  M300_BATT_VOLTAGE     (double)  Vehicle battery voltage  [V]  ");
  blk("  M300_RAW_NMEA         (string)  Raw NMEA sentences       []   ");
  blk("  M300_THRUST_L         (double)  Motor thrust (left)      [?]  ");
  blk("  M300_THRUST_R         (double)  Motor thrust (right)     [?]  ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showReleaseInfoAndExit

void showReleaseInfoAndExit()
{
  showReleaseInfo("iM300", "gpl");
  exit(0);
}


