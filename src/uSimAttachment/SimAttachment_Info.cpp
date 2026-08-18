/****************************************************************/
/*   NAME: Charles Benjamin                                     */
/*   ORGN: MIT, Cambridge MA                                    */
/*   FILE: SimAttachment_Info.cpp                               */
/*   DATE: August 18th, 2026                                    */
/****************************************************************/

#include <cstdlib>
#include "SimAttachment_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"

using namespace std;

void showSynopsis()
{
  blk("SYNOPSIS:                                                       ");
  blk("  uSimAttachment temporarily owns simulated UAV navigation and   ");
  blk("  follows PEARL's pose. On detach it returns uSimMarineV22 to    ");
  blk("  normal propagation at the final attached position.             ");
}

void showHelpAndExit()
{
  blu("=============================================================== ");
  blu("Usage: uSimAttachment file.moos [OPTIONS]                       ");
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
  blu("uSimAttachment Example MOOS Configuration                       ");
  blu("=============================================================== ");
  blk("ProcessConfig = uSimAttachment                                  ");
  blk("{                                                               ");
  blk("  AppTick   = 10                                                ");
  blk("  CommsTick = 10                                                ");
  blk("                                                                ");
  blk("  input_max_age = 2                                             ");
  blk("}                                                               ");
  exit(0);
}

void showInterfaceAndExit()
{
  blu("=============================================================== ");
  blu("uSimAttachment INTERFACE                                        ");
  blu("=============================================================== ");
  showSynopsis();
  blk("SUBSCRIPTIONS:                                                  ");
  blk("  UAV_SIM_ATTACHMENT       Desired attached state               ");
  blk("  PEARL_NAV_X/Y/HEADING/SPEED                                  ");
  blk("PUBLICATIONS:                                                   ");
  blk("  NAV_X/Y/HEADING/SPEED, NAV_ALTITUDE                          ");
  blk("  USM_ENABLED, USM_RESET                                       ");
  blk("  UAV_SIM_ATTACHED, UAV_SIM_ATTACHMENT_STATE                   ");
  exit(0);
}

void showReleaseInfoAndExit()
{
  showReleaseInfo("uSimAttachment", "gpl");
  exit(0);
}
