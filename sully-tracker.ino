#include "config.h"
#include "certificates.h"
#include <PromLokiTransport.h>
#include <GrafanaLoki.h>
#include <Arduino_MKRGPS.h>
#include <avr/dtostrf.h>


// Create a client object for sending our data.
PromLokiTransport transport;
LokiClient client(transport);



// lat=xx.xxxxx lon=xx.xxxxx spd=xx.xx
LokiStream loc(5, 50, "{job=\"sully\",type=\"loc\"}");  // 50*5+28 = 278
LokiStream logger(5, 100, "{job=\"sully\",type=\"log\"}"); // 100*3+28 =328
// bat=x.xx
LokiStream battery(1, 20, "{job=\"sully\",type=\"batt\"}"); // 20*1+29 = 50
LokiStreams streams(3, 1024);

uint16_t ticker = 0;
uint8_t loopCounter = 0;
float batt[5];
float battAvg;
uint8_t battCounter = 0;

float latitude;
float longitude;
float speed;


void setup() {
    Serial.begin(115200);
    //Serial.begin(9600);

    // Wait 5s for serial connection or continue without it
    // some boards like the esp32 will run whether or not the 
    // serial port is connected, others like the MKR boards will wait
    // for ever if you don't break the loop.
    uint8_t serialTimeout;
    while (!Serial && serialTimeout < 50) {
        delay(100);
        serialTimeout++;
    }


    Serial.println("Running Setup");

    transport.setUseTls(true);
    transport.setCerts(grafanaCert, strlen(grafanaCert));
    transport.setApn(APN);
    transport.setApnLogin(APN_LOGIN);
    transport.setApnPass(APN_PASS);
    transport.setDebug(Serial);
    if (!transport.begin()) {
        Serial.println(transport.errmsg);
        while (true) {};
    }

    // Configure the client
    client.setUrl(GC_URL);
    client.setPath(GC_PATH);
    client.setPort(GC_PORT);
    client.setUser(GC_USER);
    client.setPass(GC_PASS);

    client.setDebug(Serial); // Remove this line to disable debug logging of the client.
    if (!client.begin()) {
        Serial.println(client.errmsg);
        while (true) {};
    }

    // Add our stream objects to the streams object
    streams.addStream(loc);
    streams.addStream(logger);
    streams.addStream(battery);
    streams.setDebug(Serial);  // Remove this line to disable debug logging of the write request serialization and compression.

    // If you are using the MKR GPS as shield, change the next line to pass
  // the GPS_MODE_SHIELD parameter to the GPS.begin(...)
    if (!GPS.begin(GPS_MODE_SHIELD)) {
        Serial.println("Failed to initialize GPS!");
        while (1);
    }

}

void loop() {
    // Read GPS every 2s
    if (ticker % 2000 == 0) {
        uint64_t time;
        time = client.getTimeNanos();

        char lat[10];
        char lon[10];
        char spd[10];
        dtostrf(latitude, 8, 5, lat);
        dtostrf(longitude, 8, 5, lon);
        dtostrf(speed, 5, 2, spd);
        char str1[50];
        snprintf(str1, 50, "lat=%s lon=%s spd=%s", lat, lon, spd);
        Serial.println(str1);
        if (!loc.addEntry(time, str1, strlen(str1))) {
            Serial.println(loc.errmsg);
        }

        loopCounter++;

        if (loopCounter >= 5) {

            //Read battery voltage
            int sensorValue = analogRead(ADC_BATTERY);
            // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 4.3V):
            float voltage = sensorValue * (4.3 / 1023.0);
            Serial.print("Batt Average");
            battAvg = battAverage(voltage);
            Serial.println(battAvg);
            // The send runs ~every 12s (excluding how long it takes to send data)
            // Send batt voltage about every 5 mins
            if (battCounter >= 25) {
                char bat[10];
                // 3.23
                dtostrf(battAvg, 4, 2, bat);
                char str1[20];
                snprintf(str1, 20, "batt=%s", bat);
                if (!battery.addEntry(time, str1, strlen(str1))) {
                    Serial.println(battery.errmsg);
                }
                battCounter = 0;
            }
            else {
                battCounter++;
            }

            char str1[100];
            snprintf(str1, 100, "tCons=%d cCons=%d", transport.getConnectCount(), client.getConnectCount());
            if (!logger.addEntry(time, str1, strlen(str1))) {
                Serial.println(logger.errmsg);
            }


            LokiClient::SendResult res = client.send(streams);
            if (res != LokiClient::SendResult::SUCCESS) {
                //Failed to send
                if (res == LokiClient::SendResult::FAILED_DONT_RETRY) {
                    // If we get a non retryable error clear everything out and start from scratch.
                    Serial.println("Failed to send with a 400 error that can't be retried");
                    loc.resetEntries();
                    battery.resetEntries();
                    logger.resetEntries();
                    loopCounter = 0;
                }
                else {
                    char str1[100];
                    if (transport.errmsg) {
                        snprintf(str1, 100, "Send failed: %s", transport.errmsg);
                        if (!logger.addEntry(time, str1, strlen(str1))) {
                            Serial.println(logger.errmsg);
                        }
                    }
                    else if (client.errmsg) {
                        snprintf(str1, 100, "Send failed: %s", client.errmsg);
                        if (!logger.addEntry(time, str1, strlen(str1))) {
                            Serial.println(logger.errmsg);
                        }
                    }
                    else {
                        snprintf(str1, 100, "Send failed: unknown error");
                        if (!logger.addEntry(time, str1, strlen(str1))) {
                            Serial.println(logger.errmsg);
                        }
                    }
                }
            }
            else {
                // Sucessful send, reset streams and loopCounter.
                loc.resetEntries();
                battery.resetEntries();
                logger.resetEntries();
                loopCounter = 0;
            }
        }
    }

    // Change our delay to 1ms because we have to call GPS.avaialble() in a fairly tight loop;
    delay(1);
    if (GPS.available()) {
        latitude = GPS.latitude();
        longitude = GPS.longitude();
        speed = GPS.speed();
    }
    ticker++;
}

float battAverage(float newValue) {
    uint8_t arrSize = sizeof(batt) / sizeof(batt[0]);
    for (uint8_t i = 0; i < arrSize - 1; i++) {
        batt[i] = batt[i + 1];
    }
    batt[arrSize - 1] = newValue;
    float total = 0;
    for (uint8_t i = 0; i < arrSize; i++) {
        total = total + batt[i];
    }
    Serial.println();
    return total / arrSize;
}
