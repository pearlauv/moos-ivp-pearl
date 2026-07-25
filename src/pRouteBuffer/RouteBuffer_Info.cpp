/****************************************************************/
/*   NAME: Charles Benjamin                                     */
/*   ORGN: MIT, Cambridge MA                                    */
/*   FILE: RouteBuffer_Info.cpp                                 */
/*   DATE: July 25th, 2026                                      */
/****************************************************************/

#include <cstdlib>
#include <iostream>
#include "RouteBuffer_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"

using namespace std;

//----------------------------------------------------------------
// Procedure: showSynopsis

void showSynopsis()
{
  blk("SYNOPSIS:                                                       ");
  blk("------------------------------------                            ");
  blk("  pRouteBuffer collects operator-selected route points on the   ");
  blk("  shoreside and submits one complete route command through      ");
  blk("  pMediator. Its vehicle role expands the received command into ");
  blk("  local route update, deploy, or clear publications.            ");
}

//----------------------------------------------------------------
// Procedure: showHelpAndExit

void showHelpAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("Usage: pRouteBuffer file.moos [OPTIONS]                         ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("Options:                                                        ");
  mag("  --alias","=<ProcessName>                                      ");
  blk("      Launch pRouteBuffer with the given process name           ");
  blk("      rather than pRouteBuffer.                                 ");
  mag("  --example, -e                                                 ");
  blk("      Display example MOOS configuration block.                 ");
  mag("  --help, -h                                                    ");
  blk("      Display this help message.                                ");
  mag("  --interface, -i                                               ");
  blk("      Display MOOS publications and subscriptions.              ");
  mag("  --version,-v                                                  ");
  blk("      Display the release version of pRouteBuffer.              ");
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
  blu("pRouteBuffer Example MOOS Configuration                         ");
  blu("=============================================================== ");
  blk("                                                                ");
  blk("ProcessConfig = pRouteBuffer                                    ");
  blk("{                                                               ");
  blk("  AppTick   = 4                                                 ");
  blk("  CommsTick = 4                                                 ");
  blk("                                                                ");
  blk("  role               = shoreside                                ");
  blk("  destination_node   = briggs                                   ");
  blk("  point_var          = ROUTE_POINT                              ");
  blk("  deploy_request_var = ROUTE_BUFFER_DEPLOY                      ");
  blk("  clear_request_var  = ROUTE_BUFFER_CLEAR                       ");
  blk("  command_var        = ROUTE_BUFFER_COMMAND                     ");
  blk("  max_points         = 50                                       ");
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
  blu("pRouteBuffer INTERFACE                                          ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("SUBSCRIPTIONS:                                                  ");
  blk("------------------------------------                            ");
  blk("  Shoreside role:                                               ");
  blk("    ROUTE_POINT         = x=-100,y=-50                          ");
  blk("    ROUTE_BUFFER_DEPLOY = true                                  ");
  blk("    ROUTE_BUFFER_CLEAR  = true                                  ");
  blk("  Vehicle role:                                                 ");
  blk("    ROUTE_BUFFER_COMMAND = action=deploy # points={-100,-50}    ");
  blk("                                                                ");
  blk("PUBLICATIONS:                                                   ");
  blk("------------------------------------                            ");
  blk("  Shoreside role:                                               ");
  blk("    NODE_MESSAGE_LOCAL, VIEW_POINT, VIEW_SEGLIST                ");
  blk("    ROUTE_BUFFER_STATE, ROUTE_BUFFER_COUNT                      ");
  blk("  Vehicle role:                                                 ");
  blk("    ROUTE_UPDATE, ROUTE_DEPLOY (configurable), ROUTE_CLEAR      ");
  blk("    ROUTE_BUFFER_VEHICLE_STATE                                  ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showReleaseInfoAndExit

void showReleaseInfoAndExit()
{
  showReleaseInfo("pRouteBuffer", "gpl");
  exit(0);
}
