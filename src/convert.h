#ifndef TORQALC_CONVERT_H_
#define TORQALC_CONVERT_H_

#include "constants.h"

/* Physical conversion constants, we use long double for everything */
inline constexpr long double kImpHpConvFactor    = 745.7L;
inline constexpr long double kMetricHpConvFactor = 735.5L;
inline constexpr long double kElecHpConvFactor   = 746.0L;
inline constexpr long double kFtLbsConvFactor    = 0.7375621L;
inline constexpr long double kCaloriesConvFactor = 0.23884589662749595L;

/* Baseline unit is the watt, other units are calculated based on it. */

// Calculate wattage from volts + amps
long double ConvWatts(long double volts, long double amps);

// Calculate wattage from imperial horsepower
long double ConvWattsImpHp(long double imperial_hp);

// Calculate wattage from metric horsepower
long double ConvWattsMetHp(long double metric_hp);

// Calculate wattage from electric horsepower
long double ConvWattsElecHp(long double elec_hp);

// Calculate wattage from foot-pounds
long double ConvWattsFtLbs(long double foot_pounds);

// Calculate wattage from calories
long double ConvWattsCalories(long double calories);

// For filling in other values once we got watts from one of the above functions

// Convert watts to imperial horsepower
long double ConvImperialHorsepower(long double watts);

// Convert watts to metric horsepower
long double ConvMetricHorsepower(long double watts);

// Convert watts to electric horsepower
long double ConvElectricHorsepower(long double watts);

// Convert watts to foot-pounds
long double ConvFootPounds(long double watts);

// Convert watts to calories
long double ConvCalories(long double watts);

#endif // TORQALC_CONVERT_H_
