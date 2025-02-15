/**
 * @file AnalogSensorManager.cpp
 * @brief Implementation of the AnalogSensorManager class.
 *
 */

#include "AnalogSensorManager.h"

AnalogSensorManager::AnalogSensorManager()
    : adc(new ADC()), num_of_sensors(0), num_of_a0_sensor(0),
      num_of_a1_sensor(0) {}

bool AnalogSensorManager::isA0A1Sensor(AnalogSensor *sensor) const {
  if ((sensor->pin_num >= 14 && sensor->pin_num <= 23) ||
      sensor->pin_num == 40 || sensor->pin_num == 41) {
    return true;
  }
  return false;
}

bool AnalogSensorManager::isA0Sensor(AnalogSensor *sensor) const {
  if (isA0A1Sensor(sensor) || sensor->pin_num == 24 ||
      sensor->pin_num == 25) {
    return true;
  }
  return false;
}

bool AnalogSensorManager::isA1Sensor(AnalogSensor *sensor) const {
  if (isA0A1Sensor(sensor) || sensor->pin_num == 26 ||
      sensor->pin_num == 27 || sensor->pin_num == 38 ||
      sensor->pin_num == 39) {
    return true;
  }
  return false;
}

bool AnalogSensorManager::isInGroupA0(AnalogSensor *sensor) const {
  for (int i = 0; i < num_of_a0_sensor; i++) {
    if (sensors[group_a0[i]]->pin_num == sensor->pin_num) {
      return true;
    }
  }
  return false;
}

bool AnalogSensorManager::isInGroupA1(AnalogSensor *sensor) const {
  for (int i = 0; i < num_of_a1_sensor; i++) {
    if (sensors[group_a1[i]]->pin_num == sensor->pin_num) {
      return true;
    }
  }
  return false;
}

bool AnalogSensorManager::addSensor(AnalogSensor *sensor) {
  if (num_of_sensors >= MAX_NUM_OF_SENSORS) {
    return false;
  }
  sensors[num_of_sensors] = sensor;

  // Greedy assignment of A0 and A1 sensors.
  if (isA0A1Sensor(sensor)) {
    if (isInGroupA0(sensor)) {
      group_a0[num_of_a0_sensor++] = num_of_sensors;
    } else if (isInGroupA1(sensor)) {
      group_a1[num_of_a1_sensor++] = num_of_sensors;
    } else if (num_of_a0_sensor <= num_of_a1_sensor) {
      group_a0[num_of_a0_sensor++] = num_of_sensors;
    } else {
      group_a1[num_of_a1_sensor++] = num_of_sensors;
    }
  } else if (isA0Sensor(sensor)) {
    group_a0[num_of_a0_sensor++] = num_of_sensors;
  } else if (isA1Sensor(sensor)) {
    group_a1[num_of_a1_sensor++] = num_of_sensors;
  }

  // Update the total number of sensors.
  num_of_sensors++;
  return true;
}

void AnalogSensorManager::begin() {
  // Set the resolution and speed of the ADC.
  adc->adc0->setAveraging(8);
  adc->adc0->setResolution(DEFAULT_ADC_RESOLUTION);
  adc->adc1->setAveraging(8);
  adc->adc1->setResolution(DEFAULT_ADC_RESOLUTION);

  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_SPEED);
  adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
  adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_SPEED);
}

void AnalogSensorManager::setMuxIfNeeded(AnalogSensor *sensor) const {
  if (sensor->mux == nullptr) {
    return;
  }
  sensor->mux->setChannel(sensor->mux_pin_num);
}

void AnalogSensorManager::updateSensorData() {
  int pair_count = min(num_of_a0_sensor, num_of_a1_sensor);

  // Read sensor pairs synchronously.
  for (int i = 0; i < pair_count; i++) {
    AnalogSensor *sA0 = sensors[group_a0[i]];
    AnalogSensor *sA1 = sensors[group_a1[i]];

    setMuxIfNeeded(sA0);
    setMuxIfNeeded(sA1);

    ADC::Sync_result result =
        adc->analogSyncRead(sA0->pin_num, sA1->pin_num);
    sensor_data[group_a0[i]] = sA0->getSensorData(result.result_adc0);
    sensor_data[group_a1[i]] = sA1->getSensorData(result.result_adc1);
  }

  // Process leftover sensors from the channel with more sensors.
  if (num_of_a0_sensor > num_of_a1_sensor) {
    for (int i = pair_count; i < num_of_a0_sensor; i++) {
      AnalogSensor *s = sensors[group_a0[i]];
      setMuxIfNeeded(s);
      int raw_data = adc->analogRead(s->pin_num);
      sensor_data[group_a0[i]] = s->getSensorData(raw_data);
    }
  } else if (num_of_a1_sensor > num_of_a0_sensor) {
    for (int i = pair_count; i < num_of_a1_sensor; i++) {
      AnalogSensor *s = sensors[group_a1[i]];
      setMuxIfNeeded(s);
      int raw_data = adc->analogRead(s->pin_num);
      sensor_data[group_a1[i]] = s->getSensorData(raw_data);
    }
  }
}
