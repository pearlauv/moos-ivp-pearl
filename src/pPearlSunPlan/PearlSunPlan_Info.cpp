/****************************************************************/
/*   NAME: Charles Benjamin                                     */
/*   ORGN: MIT, Cambridge MA                                    */
/*   FILE: PearlSunPlan_Info.cpp                                     */
/*   DATE: June 2026                                            */
/****************************************************************/

#include <cstdlib>
#include <iostream>
#include "PearlSunPlan_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"

using namespace std;

//----------------------------------------------------------------
// Procedure: showSynopsis

void showSynopsis()
{
  blk("SYNOPSIS:                                                       ");
  blk("------------------------------------                            ");
  blk("  pPearlSunPlan dispatches scheduled waypoint tasks when reported    ");
  blk("  battery SOC is sufficient, waits when future forecast charge   ");
  blk("  can plausibly help, and gives up strategically when the        ");
  blk("  one-day solar horizon cannot support the task.                 ");
}

//----------------------------------------------------------------
// Procedure: showHelpAndExit

void showHelpAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("Usage: pPearlSunPlan file.moos [OPTIONS]                   ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("Options:                                                        ");
  mag("  --alias","=<ProcessName>                                      ");
  blk("      Launch pPearlSunPlan with the given process name         ");
  blk("      rather than pPearlSunPlan.                           ");
  mag("  --example, -e                                                 ");
  blk("      Display example MOOS configuration block.                 ");
  mag("  --help, -h                                                    ");
  blk("      Display this help message.                                ");
  mag("  --interface, -i                                               ");
  blk("      Display MOOS publications and subscriptions.              ");
  mag("  --version,-v                                                  ");
  blk("      Display the release version of pPearlSunPlan.        ");
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
  blu("pPearlSunPlan Example MOOS Configuration                   ");
  blu("=============================================================== ");
  blk("                                                                ");
  blk("ProcessConfig = pPearlSunPlan                              ");
  blk("{                                                               ");
  blk("  AppTick   = 4                                                 ");
  blk("  CommsTick = 4                                                 ");
  blk("                                                                ");
  blk("  soc_var             = BATT_SOC                                ");
  blk("  battery_capacity_wh = 400                                     ");
  blk("  reserve_soc         = 25                                      ");
  blk("  forecast_mode       = fixture                                 ");
  blk("  forecast_start_hour = 8                                       ");
  blk("  irradiance_24h      = 0,0,0,0,80,220,450,650,760,720,610,480, ");
  blk("                        260,90,10,0,0,0,0,0,0,0,0,0             ");
  blk("  charge_w_base       = 0                                       ");
  blk("  charge_w_gain       = 90                                      ");
  blk("  task_power_idle_w   = 5.01                                    ");
  blk("  task_power_speed_coeff = 40.5                                 ");
  blk("  task_power_speed_exponent = 2.97                              ");
  blk("  giveup_action       = return                                  ");
  blk("                                                                ");
  blk("  task = start_h=8.0; speed=1.5;                                ");
  blk("         points=22,-9 : 32,-35 : 14,-44                        ");
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
  blu("pPearlSunPlan INTERFACE                                    ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("SUBSCRIPTIONS:                                                  ");
  blk("------------------------------------                            ");
  blk("  BATT_SOC or configured soc_var  double  Reported SOC percent  ");
  blk("  SUNPLAN_TASK_COMPLETE         bool/string Task completion flag ");
  blk("  SUN_FORECAST_24H              string Runtime forecast update   ");
  blk("                                                                ");
  blk("PUBLICATIONS:                                                   ");
  blk("------------------------------------                            ");
  blk("  SOLAR_IRRADIANCE          double Current forecast W/m^2        ");
  blk("  SOLAR_INPUT_FACTOR        double Normalized irradiance factor  ");
  blk("  SUNPLAN_FORECAST_HOUR     double Local hour used in forecast   ");
  blk("  WPT_UPDATE                string Waypoint behavior update      ");
  blk("  SUNPLAN_SURVEY_ACTIVE     string true/false                    ");
  blk("  SUNPLAN_DECISION          string dispatch/wait/give_up/done    ");
  blk("  SUNPLAN_REASON            string Diagnostic reason             ");
  blk("  SUNPLAN_AVAILABLE_WH      double Energy above reserve          ");
  blk("  SUNPLAN_REQUIRED_WH       double Current task energy estimate  ");
  blk("  SUNPLAN_EXPECTED_CHARGE_WH double Remaining horizon charge     ");
  blk("  DEPLOY, RETURN, STATION_KEEP, MOOS_MANUAL_OVERRIDE             ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showReleaseInfoAndExit

void showReleaseInfoAndExit()
{
  showReleaseInfo("pPearlSunPlan", "gpl");
  exit(0);
}
