
//AHT20 digital humidity/temperature sensor 

//Use a Grove cable to connect the AHT20 sensor board to the I2C Grove connector on the Mayfly

#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht;

void setup() {
  Serial.begin(57600);
  Serial.println("AHT20 Digital Humidity/Temperature sensor demo");

  if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }
  Serial.println("AHT20 found");
}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.print("% rH;  ");
  Serial.print("Temp: "); Serial.print(temp.temperature); Serial.print(" degrees C; ");

  // now convert from Celcius to Fahrenheit
  
  float temperatureF = (temp.temperature * 9.0 / 5.0) + 32.0;
  Serial.print(temperatureF); Serial.print(" degrees F");
  
  Serial.println();
  delay(1000);  // wait 1 second before next sample
}
