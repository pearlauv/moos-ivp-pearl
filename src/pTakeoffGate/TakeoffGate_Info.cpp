/****************************************************************/
/*   NAME: Charles Benjamin                                     */
/*   ORGN: MIT, Cambridge MA                                    */
/*   FILE: TakeoffGate_Info.cpp                                 */
/*   DATE: August 17th, 2026                                    */
/****************************************************************/

#include <cstdlib>
#include "TakeoffGate_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"

using namespace std;

void showSynopsis()
{
  blk("SYNOPSIS:                                                       ");
  blk("  pTakeoffGate consumes each UAV takeoff request and approves it ");
  blk("  only when fresh UAV battery, PEARL battery, and apparent-wind  ");
  blk("  inputs satisfy the configured thresholds.                      ");
}

void showHelpAndExit()
{
  blu("=============================================================== ");
  blu("Usage: pTakeoffGate file.moos [OPTIONS]                         ");
  blu("=============================================================== ");
  showSynopsis();
  blk("Options:                                                        ");
  mag("  --alias", "=<ProcessName>                                    ");
  mag("  --example, -e                                                 ");
  mag("  --help, -h                                                    ");
  mag("  --interface, -i                                               ");
  mag("  --version, -v                                                 ");
  exit(0);
}

void showExampleConfigAndExit()
{
  blu("=============================================================== ");
  blu("pTakeoffGate Example MOOS Configuration                         ");
  blu("=============================================================== ");
  blk("ProcessConfig = pTakeoffGate                                    ");
  blk("{                                                               ");
  blk("  AppTick   = 4                                                 ");
  blk("  CommsTick = 4                                                 ");
  blk("                                                                ");
  blk("  min_uav_soc      = 30                                        ");
  blk("  min_pearl_soc    = 15                                        ");
  blk("  max_wind_speed   = 4                                         ");
  blk("  input_max_age    = 3                                         ");
  blk("}                                                               ");
  exit(0);
}

void showInterfaceAndExit()
{
  blu("=============================================================== ");
  blu("pTakeoffGate INTERFACE                                          ");
  blu("=============================================================== ");
  showSynopsis();
  blk("SUBSCRIPTIONS:                                                  ");
  blk("  UAV_TAKEOFF_REQUEST                                           ");
  blk("  UAV_BATTERY_SOC, UAV_BATTERY_DATA_VALID                       ");
  blk("  PEARL_BATTERY_SOC, PEARL_BATTERY_DATA_VALID                   ");
  blk("  PEARL_WIND_SPEED, PEARL_WIND_DATA_VALID                       ");
  blk("PUBLICATIONS:                                                   ");
  blk("  UAV_TAKEOFF_REQUEST       Consumed by posting false           ");
  blk("  UAV_TAKEOFF_APPROVED      One-shot approval for local action  ");
  blk("  UAV_TAKEOFF_RESULT        APPROVED or REJECTED with reason    ");
  blk("  UAV_TAKEOFF_GATE_READY    Current readiness                   ");
  blk("  UAV_TAKEOFF_GATE_REASON   Current reason token                ");
  exit(0);
}

void showReleaseInfoAndExit()
{
  showReleaseInfo("pTakeoffGate", "gpl");
  exit(0);
}
