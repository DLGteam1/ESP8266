#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "SerialCommand.h"
#include "EEPROMAnything.h"

SerialCommand sCmd(Serial);
unsigned long currentTime, prevTime, delta;
int voltage;

uint8_t sbus_data[25] = {0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
int16_t channel[18] = {1092, 1092, 1092, 1092, 1092, 1092, 1092, 1092, 1092, 1092, 1092, 1092, 1092, 1092, 1092, 1092, 0, 0};

// TARANIS 100%
// 172  =  988 µs
// 992  = 1500 µs
// 2012 = 2012 µs

/* Set these to your desired credentials. */
const char *ssid = "DLG1 Vleugel";
const char *password = "wachtwoord123";  /* empty string or at least 8 characters */
const int wifiChannel = 11;
const IPAddress Ip(192, 168, 1, 1);
const IPAddress Mask(255, 255, 255, 0);
WiFiUDP Udp;
unsigned int localPort = 12345;
byte packetBuffer[512];

struct param_t {
  float vDiv;
} params;

void setup() {
  EEPROM_readAnything(0, params);

  Serial.begin(115200);
  sCmd.addCommand("set", onSet);
  sCmd.addCommand("get", onGet);
  sCmd.setDefaultHandler(onUnknownCommand);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(Ip, Ip, Mask);
  if (!WiFi.softAP(ssid, password, wifiChannel)) {
    Serial.println("WiFi.softAP failed. (Password to short?)");
    return;
  }
  Udp.begin(localPort);

  Serial1.begin(100000, SERIAL_8E2, SERIAL_TX_ONLY, 15);  //transmit on pin D4 = GPIO2 = buildin led

  currentTime = micros();
  prevTime = currentTime;
}

void loop() {
  // put your main code here, to run repeatedly:
  sCmd.readSerial();

  int noBytes = Udp.parsePacket();
  if (noBytes > 0) {
    //Serial.print("Bytes received: ");
    //Serial.println(noBytes);
    Udp.read(packetBuffer, noBytes);
    if ((packetBuffer[0] == 0xAA) && (packetBuffer[9] == 0xAA)) {
      channel[0] = packetBuffer[2];
      channel[0] *= 256;
      channel[0] += packetBuffer[1];

      channel[1] = packetBuffer[4];
      channel[1] *= 256;
      channel[1] += packetBuffer[3];

      channel[2] = packetBuffer[6];
      channel[2] *= 256;
      channel[2] += packetBuffer[5];

      channel[3] = packetBuffer[8];
      channel[3] *= 256;
      channel[3] += packetBuffer[7];

      channel[4] = channel[0];
      channel[5] = channel[1];
      channel[6] = channel[2];
      channel[7] = channel[3];
    }
  }

  currentTime = micros();
  delta = currentTime - prevTime;

  if (delta > 7000) { // send sbus every 7 ms (3 ms send frame + 4 ms pause)
    sbus_data[ 1] =   channel[ 0] & 0x00FF;                                               // 8
    sbus_data[ 2] = ((channel[ 0] & 0x0700) >>  8) | ((channel[ 1] & 0x001F) <<  3);      // 3 + 5
    sbus_data[ 3] = ((channel[ 1] & 0x07E0) >>  5) | ((channel[ 2] & 0x0003) <<  6);      // 6 + 2
    sbus_data[ 4] = ((channel[ 2] & 0x03FC) >>  2);                                       // 8
    sbus_data[ 5] = ((channel[ 2] & 0x0400) >> 10) | ((channel[ 3] & 0x007F) <<  1);      // 1 + 7
    sbus_data[ 6] = ((channel[ 3] & 0x0780) >>  7) | ((channel[ 4] & 0x000F) <<  4);      // 4 + 4
    sbus_data[ 7] = ((channel[ 4] & 0x07F0) >>  4) | ((channel[ 5] & 0x0001) <<  7);      // 7 + 1
    sbus_data[ 8] = ((channel[ 5] & 0x01FE) >>  1);                                       // 8
    sbus_data[ 9] = ((channel[ 5] & 0x0600) >>  9) | ((channel[ 6] & 0x003F) <<  2);      // 2 + 6
    sbus_data[10] = ((channel[ 6] & 0x07C0) >>  6) | ((channel[ 7] & 0x0007) <<  5);      // 5 + 3
    sbus_data[11] = ((channel[ 7] & 0x07F8) >>  3);                                       // 8

    sbus_data[12] =   channel[ 8] & 0x00FF;                                               // 8
    sbus_data[13] = ((channel[ 8] & 0x0700) >>  8) | ((channel[ 9] & 0x001F) <<  3);      // 3 + 5
    sbus_data[14] = ((channel[ 9] & 0x07E0) >>  5) | ((channel[10] & 0x0003) <<  6);      // 6 + 2
    sbus_data[15] = ((channel[10] & 0x03FC) >>  2);                                       // 8
    sbus_data[16] = ((channel[10] & 0x0400) >> 10) | ((channel[11] & 0x007F) <<  1);      // 1 + 7
    sbus_data[17] = ((channel[11] & 0x0780) >>  7) | ((channel[12] & 0x000F) <<  4);      // 4 + 4
    sbus_data[18] = ((channel[12] & 0x07F0) >>  4) | ((channel[13] & 0x0001) <<  7);      // 7 + 1
    sbus_data[19] = ((channel[13] & 0x01FE) >>  1);                                       // 8
    sbus_data[20] = ((channel[13] & 0x0600) >>  9) | ((channel[14] & 0x003F) <<  2);      // 2 + 6
    sbus_data[21] = ((channel[14] & 0x07C0) >>  6) | ((channel[15] & 0x0007) <<  5);      // 5 + 3
    sbus_data[22] = ((channel[15] & 0x07F8) >>  3);                                       // 8

    if (channel[16] == 0) sbus_data[23] &= ~0x01; else sbus_data[23] |= 0x01;
    if (channel[17] == 0) sbus_data[23] &= ~0x02; else sbus_data[23] |= 0x02;

    Serial1.write(sbus_data, 25);

    //read adc
    voltage = analogRead(A0);

    prevTime = currentTime;
  }
}

void onSet() {
  char* param = sCmd.next();
  if (param == NULL) {
    Serial.println("SET: parameter required");
    return;
  }

  if (strcmp(param, "channel") == 0) {
    char* ch = sCmd.next();
    char* value = sCmd.next();
    channel[atoi(ch)] = atoi(value);
  } else if (strcmp(param, "devider") == 0) {
    char* value = sCmd.next();
    params.vDiv = atof(value);
    EEPROM_writeAnything(0, params);
  } else {
    Serial.print("SET: unknown parameter \"");
    Serial.print(param);
    Serial.println("\"");
  }
}

void onGet() {
  Serial.print("Voltage: ");
  Serial.println(voltage / params.vDiv);
  Serial.print("Channel 0: ");
  Serial.println(channel[0]);
  Serial.print("Channel 1: ");
  Serial.println(channel[1]);
  Serial.print("Channel 2: ");
  Serial.println(channel[2]);
  Serial.print("Channel 3: ");
  Serial.println(channel[3]);
}

void onUnknownCommand(char* command) {
  Serial.print(command);
  Serial.println(": unknown command");
}


