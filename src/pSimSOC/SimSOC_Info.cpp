/****************************************************************/
/*   NAME: Charles Benjamin                                     */
/*   ORGN: MIT, Cambridge MA                                    */
/*   FILE: SimSOC_Info.cpp                                      */
/*   DATE: June 2026                                            */
/****************************************************************/

#include <cstdlib>
#include <iostream>
#include "SimSOC_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"

using namespace std;

//----------------------------------------------------------------
// Procedure: showSynopsis

void showSynopsis()
{
  blk("SYNOPSIS:                                                       ");
  blk("------------------------------------                            ");
  blk("  pSimSOC publishes a simulated battery state-of-charge value   ");
  blk("  from a simple solar charge and speed-based load model. It is  ");
  blk("  intended for planning missions that should not depend on live ");
  blk("  charge-controller hardware during simulation.                 ");
}

//----------------------------------------------------------------
// Procedure: showHelpAndExit

void showHelpAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("Usage: pSimSOC file.moos [OPTIONS]                   ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("Options:                                                        ");
  mag("  --alias","=<ProcessName>                                      ");
  blk("      Launch pSimSOC with the given process name         ");
  blk("      rather than pSimSOC.                           ");
  mag("  --example, -e                                                 ");
  blk("      Display example MOOS configuration block.                 ");
  mag("  --help, -h                                                    ");
  blk("      Display this help message.                                ");
  mag("  --interface, -i                                               ");
  blk("      Display MOOS publications and subscriptions.              ");
  mag("  --version,-v                                                  ");
  blk("      Display the release version of pSimSOC.        ");
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
  blu("pSimSOC Example MOOS Configuration                   ");
  blu("=============================================================== ");
  blk("                                                                ");
  blk("ProcessConfig = pSimSOC                              ");
  blk("{                                                               ");
  blk("  AppTick   = 2                                                 ");
  blk("  CommsTick = 2                                                 ");
  blk("                                                                ");
  blk("  publish_var         = BATT_SOC                                ");
  blk("  start_soc           = 72                                      ");
  blk("  battery_capacity_wh = 400                                     ");
  blk("  charge_w_base       = 0                                       ");
  blk("  charge_w_gain       = 90                                      ");
  blk("  idle_load_w         = 5.01                                    ");
  blk("  speed_load_coeff    = 40.5                                    ");
  blk("  speed_load_exponent = 2.97                                    ");
  blk("}                                                               ");
  blk("                                                                ");
  exit(0);
}


//----------------------------------------------------------------
// Procedure: showInterfaceAndExit

void showInterfaceAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("pSimSOC INTERFACE                                    ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("SUBSCRIPTIONS:                                                  ");
  blk("------------------------------------                            ");
  blk("  SOLAR_INPUT_FACTOR  double  Normalized available solar input  ");
  blk("  NAV_SPEED           double  Vehicle speed in meters/second    ");
  blk("  SOC_COMMAND         string  SET=nn, ADD=nn, or SUB=nn         ");
  blk("                                                                ");
  blk("PUBLICATIONS:                                                   ");
  blk("------------------------------------                            ");
  blk("  BATT_SOC            double  Simulated state of charge percent ");
  blk("  SIMSOC_CHARGE_W     double  Modeled charge power              ");
  blk("  SIMSOC_LOAD_W       double  Modeled load power                ");
  blk("  SIMSOC_NET_W        double  Charge minus load                 ");
  blk("  SIMSOC_SOLAR_INPUT_FACTOR double                              ");
  blk("  SIMSOC_NAV_SPEED    double                                    ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showReleaseInfoAndExit

void showReleaseInfoAndExit()
{
  showReleaseInfo("pSimSOC", "gpl");
  exit(0);
}
