/****************************************************************/
/*   NAME: Charles Benjamin                             */
/*   ORGN: MIT, Cambridge MA                                    */
/*   FILE: SherlockTelemetry_Info.cpp                               */
/*   DATE: August 17th, 2026                                    */
/****************************************************************/

#include <cstdlib>
#include <iostream>
#include "SherlockTelemetry_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"

using namespace std;

//----------------------------------------------------------------
// Procedure: showSynopsis

void showSynopsis()
{
  blk("SYNOPSIS:                                                       ");
  blk("------------------------------------                            ");
  blk("  iSherlockTelemetry polls Sherlock's Prometheus endpoint and   ");
  blk("  publishes PEARL battery state and Airmar wind measurements.   ");
  blk("  Source-provided ages and validity are preserved so consumers  ");
  blk("  can distinguish fresh telemetry from retained values.         ");
  blk("                                                                ");
}

//----------------------------------------------------------------
// Procedure: showHelpAndExit

void showHelpAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("Usage: iSherlockTelemetry file.moos [OPTIONS]                   ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("Options:                                                        ");
  mag("  --alias","=<ProcessName>                                      ");
  blk("      Launch iSherlockTelemetry with the given process name         ");
  blk("      rather than iSherlockTelemetry.                           ");
  mag("  --example, -e                                                 ");
  blk("      Display example MOOS configuration block.                 ");
  mag("  --help, -h                                                    ");
  blk("      Display this help message.                                ");
  mag("  --interface, -i                                               ");
  blk("      Display MOOS publications and subscriptions.              ");
  mag("  --version,-v                                                  ");
  blk("      Display the release version of iSherlockTelemetry.        ");
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
  blu("iSherlockTelemetry Example MOOS Configuration                   ");
  blu("=============================================================== ");
  blk("                                                                ");
  blk("ProcessConfig = iSherlockTelemetry                              ");
  blk("{                                                               ");
  blk("  AppTick   = 4                                                 ");
  blk("  CommsTick = 4                                                 ");
  blk("                                                                ");
  blk("  metrics_host    = 192.168.88.252                              ");
  blk("  metrics_port    = 9273                                        ");
  blk("  metrics_path    = /metrics                                    ");
  blk("  poll_interval   = 2                                          ");
  blk("  http_timeout    = 5                                          ");
  blk("  battery_max_age = 30                                         ");
  blk("  wind_max_age    = 5                                          ");
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
  blu("iSherlockTelemetry INTERFACE                                    ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("SUBSCRIPTIONS:                                                  ");
  blk("------------------------------------                            ");
  blk("  APPCAST_REQ (handled by AppCastingMOOSApp)                    ");
  blk("PUBLICATIONS:                                                   ");
  blk("------------------------------------                            ");
  blk("  PEARL_BATTERY_SOC         Battery state of charge, percent    ");
  blk("  PEARL_BATTERY_CHARGING    1 when the CMP reports charging     ");
  blk("  PEARL_BATTERY_DATA_VALID  1 when connected and sufficiently  ");
  blk("                            fresh                               ");
  blk("  PEARL_BATTERY_DATA_AGE    Source sample age, seconds          ");
  blk("  PEARL_WIND_SPEED          Apparent wind speed, m/s            ");
  blk("  PEARL_WIND_SPEED_APPARENT Apparent wind speed, m/s            ");
  blk("  PEARL_WIND_SPEED_TRUE     True wind speed, m/s, when present  ");
  blk("  PEARL_WIND_DATA_VALID     1 when Airmar data is valid/fresh   ");
  blk("  PEARL_WIND_DATA_AGE       Source sentence age, seconds        ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showReleaseInfoAndExit

void showReleaseInfoAndExit()
{
  showReleaseInfo("iSherlockTelemetry", "gpl");
  exit(0);
}
