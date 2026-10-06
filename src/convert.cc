// Contains math functions for converting between energy units.

#include "convert.h"

// clang-format off
#include <logging.h>
// clang-format on

static const long double InvalidIn() {
  LOG(WARN) << L"Invalid conversion input, returning 0!";
  return 0.0L;
}

// Calculate wattage from volts + amps
long double ConvWatts(long double volts, long double amps) {
  if (amps < 0.0L || volts < 0.0L) {
    return InvalidIn();
  }
  return (volts * amps);
}

// Calculate wattage from imperial horsepower
long double ConvWattsImpHp(long double imperial_hp) {
  if (imperial_hp < 0.0L) {
    return InvalidIn();
  }
  return (imperial_hp * kImpHpConvFactor);
}

// Calculate wattage from metric horsepower
long double ConvWattsMetHp(long double metric_hp) {
  if (metric_hp < 0.0L) {
    return InvalidIn();
  }
  return (metric_hp * kMetricHpConvFactor);
}

// Calculate wattage from electric horsepower
long double ConvWattsElecHp(long double elec_hp) {
  if (elec_hp < 0.0L) {
    return InvalidIn();
  }
  return (elec_hp * kElecHpConvFactor);
}

// Calculate wattage from foot-pounds
long double ConvWattsFtLbs(long double foot_pounds) {
  if (foot_pounds < 0.0L) {
    return InvalidIn();
  }
  return (foot_pounds / kFtLbsConvFactor);
}

// Calculate wattage from calories
long double ConvWattsCalories(long double calories) {
  if (calories < 0.0L) {
    return InvalidIn();
  }
  return (calories / kCaloriesConvFactor);
}

// Convert watts to imperial horsepower
long double ConvImperialHorsepower(long double watts) {
  if (watts < 0.0L) {
    return InvalidIn();
  }
  return (watts / kImpHpConvFactor);
}

// Convert watts to metric horsepower
long double ConvMetricHorsepower(long double watts) {
  if (watts < 0.0L) {
    return InvalidIn();
  }
  return (watts / kMetricHpConvFactor);
}

// Convert watts to electric horsepower
long double ConvElectricHorsepower(long double watts) {
  if (watts < 0.0L) {
    return InvalidIn();
  }
  return (watts / kElecHpConvFactor);
}

// Convert watts to foot-pounds
long double ConvFootPounds(long double watts) {
  if (watts < 0.0L) {
    return InvalidIn();
  }
  return (watts * kFtLbsConvFactor);
}

// Convert watts to calories
long double ConvCalories(long double watts) {
  if (watts < 0.0L) {
    return InvalidIn();
  }
  return (watts * kCaloriesConvFactor);
}
