#include "Button2.h"
#include "SPIFFS.h"
#include "driver/rtc_io.h"
#include "esp_adc_cal.h"
#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

// bool sleeping = false;

//----- SCREEN
#pragma region SCREEN

const char *batteryImages[] = {
    "/battery_01.jpg",
    "/battery_02.jpg",
    "/battery_03.jpg",
    "/battery_04.jpg",
    "/battery_chrg_01.jpg",
    "/battery_chrg_02.jpg",
    "/battery_chrg_03.jpg",
    "/battery_chrg_04.jpg",
};
bool charging = false;

TaskHandle_t batteryTaskHandle = NULL;
TaskHandle_t screenTaskHandle = NULL;

#define ADC_EN 14 // ADC_EN is the ADC detection enable port

TFT_eSPI tft = TFT_eSPI();

bool screenOff = false;
int batteryLevel = 0;
int batteryIconIndex = 0;
float batteryVoltage = 0;
int sattelitesCount = 0;
int fixType = 0;
double latitude = 0;
double longitude = 0;
double altitude = 0;
int speed = 0;
int bearing = 0;
int padding = 0;
int smallFontOffset = 3;

#define ICON_WIDTH 70
#define ICON_HEIGHT 40
#define LINE_HEIGHT 22
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define ICON_POS_X (tft.width() - ICON_WIDTH)
#define ICON_POS_Y 0
#define VOLTAGE_POS_X 74
#define VOLTAGE_POS_Y 40

#define SATTELITES_POS_X 70
#define SATTELITES_POS_Y 75

#define FIXTYPE_POS_X 70
#define FIXTYPE_POS_Y (SATTELITES_POS_Y + LINE_HEIGHT)

#define LATITUDE_POS_X 45
#define LATITUDE_POS_Y (FIXTYPE_POS_Y + LINE_HEIGHT)

#define LONGITUDE_POS_X 45
#define LONGITUDE_POS_Y (LATITUDE_POS_Y + LINE_HEIGHT)

#define ALTITUDE_POS_X 45
#define ALTITUDE_POS_Y (LONGITUDE_POS_Y + LINE_HEIGHT)

#define BEARING_POS_X 70
#define BEARING_POS_Y (ALTITUDE_POS_Y + LINE_HEIGHT)

#define SPEED_POS_X 70
#define SPEED_POS_Y (BEARING_POS_Y + LINE_HEIGHT)

#define SCREENOFF_POS_X 67
#define SCREENOFF_POS_Y 224

void drawBatteryIcon()
{
    TJpgDec.drawFsJpg(ICON_POS_X, ICON_POS_Y, batteryImages[batteryIconIndex]);
}

void drawBatteryLevel()
{
    padding = tft.textWidth("100%", 4);
    tft.setTextPadding(padding);
    tft.drawString(charging ? "Chrg" : String(batteryLevel) + "%", 0, 8, 4);
}

void drawBatteryVoltage()
{
    padding = tft.textWidth("8.88v", 4);
    tft.setTextPadding(padding);
    tft.drawString(String(batteryVoltage) + "v", VOLTAGE_POS_X, VOLTAGE_POS_Y, 4);
}

void drawSattelites()
{
    padding = tft.textWidth("888", 4);
    tft.setTextPadding(padding);
    tft.drawString(String(sattelitesCount), SATTELITES_POS_X, SATTELITES_POS_Y, 4);
}

void drawFixType()
{
    padding = tft.textWidth("Dead Reck.", 2);
    tft.setTextPadding(padding);
    switch (fixType)
    {
    case 0:
        tft.fillRect(FIXTYPE_POS_X, FIXTYPE_POS_Y, padding, LINE_HEIGHT, TFT_BLACK);
        tft.drawString("No Fix", FIXTYPE_POS_X, FIXTYPE_POS_Y + smallFontOffset, 2);
        break;
    case 1:
        tft.fillRect(FIXTYPE_POS_X, FIXTYPE_POS_Y, padding, LINE_HEIGHT, TFT_BLACK);
        tft.drawString("Dead Reck.", FIXTYPE_POS_X, FIXTYPE_POS_Y + smallFontOffset, 2);
        break;
    case 2:
        tft.drawString("2D", FIXTYPE_POS_X, FIXTYPE_POS_Y, 4);
        break;
    case 3:
        tft.drawString("3D", FIXTYPE_POS_X, FIXTYPE_POS_Y, 4);
        break;
    case 4:
        tft.fillRect(FIXTYPE_POS_X, FIXTYPE_POS_Y, padding, LINE_HEIGHT, TFT_BLACK);
        tft.drawString("GNSS+DR", FIXTYPE_POS_X, FIXTYPE_POS_Y + smallFontOffset, 2);
        break;
    case 5:
        tft.fillRect(FIXTYPE_POS_X, FIXTYPE_POS_Y, padding, LINE_HEIGHT, TFT_BLACK);
        tft.drawString("Time Only", FIXTYPE_POS_X, FIXTYPE_POS_Y + smallFontOffset, 2);
        break;
    default:
        break;
    }
}

void drawLatttitued()
{
    padding = tft.textWidth("-88.88888888", 2);
    tft.setTextPadding(padding);
    tft.drawString(String(latitude, 8), LATITUDE_POS_X, LATITUDE_POS_Y + smallFontOffset, 2);
}

void drawLongitude()
{
    padding = tft.textWidth("-88.88888888", 2);
    tft.setTextPadding(padding);
    tft.drawString(String(longitude, 8), LONGITUDE_POS_X, LONGITUDE_POS_Y + smallFontOffset, 2);
}

void drawAltitude()
{
    padding = tft.textWidth("888888", 2);
    tft.setTextPadding(padding);
    tft.drawString(String(altitude, 0), ALTITUDE_POS_X, ALTITUDE_POS_Y + smallFontOffset, 2);
}

void drawBearing()
{
    String direction = "";
    if (bearing >= 349 || bearing <= 11)
    {
        direction = "N";
    }
    else if (bearing >= 12 && bearing <= 33)
    {
        direction = "NNE";
    }
    else if (bearing >= 34 && bearing <= 56)
    {
        direction = "NE";
    }
    else if (bearing >= 57 && bearing <= 78)
    {
        direction = "ENE";
    }
    else if (bearing >= 79 && bearing <= 101)
    {
        direction = "E";
    }
    else if (bearing >= 102 && bearing <= 123)
    {
        direction = "ESE";
    }
    else if (bearing >= 124 && bearing <= 146)
    {
        direction = "SE";
    }
    else if (bearing >= 147 && bearing <= 168)
    {
        direction = "SSE";
    }
    else if (bearing >= 169 && bearing <= 191)
    {
        direction = "S";
    }
    else if (bearing >= 192 && bearing <= 213)
    {
        direction = "SSW";
    }
    else if (bearing >= 214 && bearing <= 236)
    {
        direction = "SW";
    }
    else if (bearing >= 237 && bearing <= 258)
    {
        direction = "WSW";
    }
    else if (bearing >= 259 && bearing <= 281)
    {
        direction = "W";
    }
    else if (bearing >= 282 && bearing <= 303)
    {
        direction = "WNW";
    }
    else if (bearing >= 304 && bearing <= 326)
    {
        direction = "NW";
    }
    else if (bearing >= 327)
    {
        direction = "NNW";
    }

    padding = tft.textWidth("WWW", 4);
    tft.setTextPadding(padding);
    tft.drawString(direction, BEARING_POS_X, BEARING_POS_Y, 4);
}

void drawLabels()
{
    tft.drawString("Volts:", 0, VOLTAGE_POS_Y, 4);
    tft.drawString("Sattelites :", 0, SATTELITES_POS_Y + smallFontOffset, 2);
    tft.drawString("Fix Type :", 0, FIXTYPE_POS_Y + smallFontOffset, 2);
    tft.drawString("Lat :", 0, LATITUDE_POS_Y + smallFontOffset, 2);
    tft.drawString("Long :", 0, LONGITUDE_POS_Y + smallFontOffset, 2);
    tft.drawString("Alt :", 0, ALTITUDE_POS_Y + smallFontOffset, 2);
    tft.drawString("meters", tft.width() - 40, ALTITUDE_POS_Y + smallFontOffset, 2);
    tft.drawString("Bearing :", 0, BEARING_POS_Y + smallFontOffset, 2);
    tft.drawString("Screen Off", SCREENOFF_POS_X, SCREENOFF_POS_Y, 2);
}

void drawSpeed()
{
    // tft.fillRect(0, SPEED_POS_Y, tft.width(), LINE_HEIGHT, TFT_BLACK);
    tft.drawString("Speed :", 0, SPEED_POS_Y + smallFontOffset, 2);
    tft.drawString(String(speed / 10), SPEED_POS_X, SPEED_POS_Y, 4);
    tft.drawString("kph", tft.width() - 20, SPEED_POS_Y + smallFontOffset, 2);
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h,
                uint16_t *bitmap)
{
    if (y >= tft.height())
        return 0;
    tft.pushImage(x, y, w, h, bitmap);
    return 1;
}

void turnScreenOff()
{
    digitalWrite(4, LOW);             // Set backlight off
    tft.writecommand(ST7789_DISPOFF); // Switch off the display
    tft.writecommand(ST7789_SLPIN);   // Sleep the display driver
    digitalWrite(ADC_EN, LOW);        // Toggle voltage reading
    if (screenTaskHandle != NULL)
    {
        vTaskSuspend(screenTaskHandle);
    }
    if (batteryTaskHandle != NULL)
    {
        vTaskSuspend(batteryTaskHandle);
    }

    screenOff = true;
}

void turnScreenOn()
{
    digitalWrite(4, HIGH);           // Set backlight on
    tft.writecommand(ST7789_DISPON); // Switch on the display
    tft.writecommand(ST7789_SLPOUT); // Wake the display driver
    digitalWrite(ADC_EN, HIGH);      // Toggle voltage reading
    if (screenTaskHandle != NULL)
    {
        vTaskResume(screenTaskHandle);
    }
    if (batteryTaskHandle != NULL)
    {
        vTaskResume(batteryTaskHandle);
    }
    screenOff = false;
}

void toggleScreen()
{
    if (screenOff)
    {
        turnScreenOn();
    }
    else
    {
        turnScreenOff();
    }
}

void screen_loop(void *arg)
{
    tft.fillScreen(TFT_BLACK);
    drawLabels();

    while (true)
    {
        drawBatteryIcon();
        drawBatteryLevel();
        drawBatteryVoltage();
        drawSattelites();
        drawFixType();
        drawLatttitued();
        drawLongitude();
        drawAltitude();
        // drawSpeed();
        drawBearing();

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        // Serial.print("screen 4096/ ");
        // Serial.println(uxTaskGetStackHighWaterMark(NULL));
    }
}

void displayInit()
{
    pinMode(4, OUTPUT);
    tft.begin();
    tft.setRotation(4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK, true);
    tft.fillScreen(TFT_BLACK);
    tft.setSwapBytes(true);
    tft.setTextFont(4);
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    TJpgDec.drawFsJpg(0, 0, "/start.jpg");
    Serial.println("Display initialized");
}
#pragma endregion SCREEN
//-----

//----- BATTERY
#pragma region BATTERY

// Array with voltage - charge definitions
float voltageTable[] = {
    3.200, // 0
    3.250, // 1
    3.300, // 2
    3.350, // 3
    3.400, // 4
    3.450, // 5
    3.500, // 6
    3.550, // 7
    3.600, // 8
    3.650, // 9
    3.700, // 10
    3.703, // 11
    3.706, // 12
    3.710, // 13
    3.713, // 14
    3.716, // 15
    3.719, // 16
    3.723, // 17
    3.726, // 18
    3.729, // 19
    3.732, // 20
    3.735, // 21
    3.739, // 22
    3.742, // 23
    3.745, // 24
    3.748, // 25
    3.752, // 26
    3.755, // 27
    3.758, // 28
    3.761, // 29
    3.765, // 30
    3.768, // 31
    3.771, // 32
    3.774, // 33
    3.777, // 34
    3.781, // 35
    3.784, // 36
    3.787, // 37
    3.790, // 38
    3.794, // 39
    3.797, // 40
    3.800, // 41
    3.805, // 42
    3.811, // 43
    3.816, // 44
    3.821, // 45
    3.826, // 46
    3.832, // 47
    3.837, // 48
    3.842, // 49
    3.847, // 50
    3.853, // 51
    3.858, // 52
    3.863, // 53
    3.868, // 54
    3.874, // 55
    3.879, // 56
    3.884, // 57
    3.889, // 58
    3.895, // 59
    3.900, // 60
    3.906, // 61
    3.911, // 62
    3.917, // 63
    3.922, // 64
    3.928, // 65
    3.933, // 66
    3.939, // 67
    3.944, // 68
    3.950, // 69
    3.956, // 70
    3.961, // 71
    3.967, // 72
    3.972, // 73
    3.978, // 74
    3.983, // 75
    3.989, // 76
    3.994, // 77
    4.000, // 78
    4.008, // 79
    4.015, // 80
    4.023, // 81
    4.031, // 82
    4.038, // 83
    4.046, // 84
    4.054, // 85
    4.062, // 86
    4.069, // 87
    4.077, // 88
    4.085, // 89
    4.092, // 90
    4.100, // 91
    4.111, // 92
    4.122, // 93
    4.133, // 94
    4.144, // 95
    4.156, // 96
    4.167, // 97
    4.178, // 98
    4.189, // 99
    4.200, // 100
};

#define ADC_PIN 34
// #define MIN_USB_PWR_VOL 5.0
#define MIN_USB_CHRG_VOL 4.4

int vref = 1100;
int voltageReadCounts = 100;
float voltageCorrectionFactor = 1.88;

float getVolatge()
{
    int totalValue = 0;
    float averageValue = 0;

    for (int i = 0; i < voltageReadCounts; i++)
    {
        totalValue += analogRead(ADC_PIN);
    }
    averageValue = totalValue / voltageReadCounts;

    // return (averageValue / 4095.0) * 2.0 * 3.3 * (vref / 1000.0)
    return (averageValue * voltageCorrectionFactor) / 1000.0;
}

int getChargeLevel(float volts)
{
    int idx = 50;
    int prev = 0;
    if (volts >= 4.2)
    {
        return 100;
    }
    if (volts <= 3.2)
    {
        return 0;
    }
    while (true)
    {
        int half = abs(idx - prev) / 2;
        prev = idx;
        if (volts >= voltageTable[idx])
        {
            idx = idx + half;
        }
        else
        {
            idx = idx - half;
        }
        if (prev == idx)
        {
            break;
        }
    }
    return idx;
}

void voltageReadInit()
{
    pinMode(ADC_EN, OUTPUT);
    digitalWrite(ADC_EN, HIGH);
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
        ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
        vref = adc_chars.vref;
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n",
                      adc_chars.coeff_a, adc_chars.coeff_b);
    }
    else
    {
        Serial.println("Default Vref: 1100mV");
    }
}

void battery_loop(void *arg)
{

    while (true)
    {
        if (!screenOff)
        {
            batteryVoltage = getVolatge();
            charging = batteryVoltage >= MIN_USB_CHRG_VOL;

            if (charging)
            {
                if (batteryIconIndex < 4 || batteryIconIndex >= 7)
                    batteryIconIndex = 4;
                else
                    batteryIconIndex += 1;
            }
            else
            {
                batteryLevel = getChargeLevel(batteryVoltage);
                if (batteryLevel >= 75)
                {
                    batteryIconIndex = 3;
                }
                else if (batteryLevel >= 50)
                {
                    batteryIconIndex = 2;
                }
                else if (batteryLevel >= 25)
                {
                    batteryIconIndex = 1;
                }
                else
                {
                    batteryIconIndex = 0;
                }
            }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Serial.print("batt 4096/ ");
        // Serial.println(uxTaskGetStackHighWaterMark(NULL));
    }
}

#pragma endregion BATTERY
//-----

//----- GPS
#pragma region GPS

#define UBX_ID_NAV_DOP 0x04
#define UBX_ID_NAV_PVT 0x07

int gpsPreviousDateAndHour = 0;
int dateAndHour;
int timeSinceHourStart;
int gps_bearing;
int gps_altitude;
int gps_speed;
uint8_t _byte;
uint8_t _parserState;
uint8_t _tempPacket[255];
uint8_t rc_data[20];
uint8_t gpsSyncBits = 0;
int16_t msg_length;

HardwareSerial GPSSerial2(2);
#define RX2 27
#define TX2 26

struct ublox
{
    uint8_t message_class;
    uint8_t message_id;
    uint16_t payload_length;
};

struct ublox_NAV_DOP : ublox
{
    uint32_t iTOW;
    uint16_t gDOP;
    uint16_t pDOP;
    uint16_t tDOP;
    uint16_t vDOP;
    uint16_t hDOP;
    uint16_t nDOP;
    uint16_t eDOP;
};

struct ublox_NAV_PVT : ublox
{
    uint32_t iTOW;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t valid;
    uint32_t tAcc;
    int32_t nano;
    uint8_t fixType;
    uint8_t flags;
    uint8_t flags2;
    uint8_t numSV;
    int32_t lon;
    int32_t lat;
    int32_t height;
    int32_t hMSL;
    uint32_t hAcc;
    uint32_t vAcc;
    int32_t velN;
    int32_t velE;
    int32_t velD;
    int32_t gSpeed;
    int32_t headMot;
    uint32_t sAcc;
    uint32_t headAcc;
    uint16_t pDOP;
    uint16_t reserved2;
    uint32_t reserved3;
    int32_t headVeh;
    int16_t magDec;
    uint16_t magAcc;
};

union
{
    ublox_NAV_DOP dop;
    ublox_NAV_PVT pvt;
} _validPacket;

// Checksum calculation for UBLOX module
void _calcChecksum(uint8_t *CK, uint8_t *payload, uint16_t length)
{
    CK[0] = 0;
    CK[1] = 0;

    for (uint8_t i = 0; i < length; i++)
    {
        CK[0] += payload[i];
        CK[1] += CK[0];
    }
}

// Function to send bytearray to ublox receiver ovcer serial
void ublox_sendPacket(byte *packet, byte len)
{
    for (byte i = 0; i < len; i++)
    {
        GPSSerial2.write(packet[i]);
    }
    GPSSerial2.flush();
}

// U-blox receiver disable NMEA messages
void ublox_noNMEA()
{
    byte messages[][2] = {
        {0xF0, 0x0A},
        {0xF0, 0x09},
        {0xF0, 0x00},
        {0xF0, 0x01},
        {0xF0, 0x0D},
        {0xF0, 0x06},
        {0xF0, 0x02},
        {0xF0, 0x07},
        {0xF0, 0x03},
        {0xF0, 0x04},
        {0xF0, 0x0E},
        {0xF0, 0x0F},
        {0xF0, 0x05},
        {0xF0, 0x08},
        {0xF1, 0x00},
        {0xF1, 0x01},
        {0xF1, 0x03},
        {0xF1, 0x04},
        {0xF1, 0x05},
        {0xF1, 0x06},
    };

    byte packet[] = {
        0xB5, // sync 1
        0x62, // sync  2
        0x06, // class
        0x01, // id
        0x08, // length
        0x00, // length
        0x00, // payload (first byte from messages array element)
        0x00, // payload (second byte from messages array element)
        0x00, // payload (not changed in the case)
        0x00, // payload (not changed in the case)
        0x00, // payload (not changed in the case)
        0x00, // payload (not changed in the case)
        0x00, // payload (not changed in the case)
        0x00, // payload (not changed in the case)
        0x00, // CK_A
        0x00, // CK_B
    };

    byte packetSize = sizeof(packet);

    // Offset to the place where payload starts.
    byte payloadOffset = 6;

    // Iterate over the messages array.
    for (byte i = 0; i < sizeof(messages) / sizeof(*messages); i++)
    {
        // Copy two bytes of payload to the packet buffer.
        for (byte j = 0; j < sizeof(*messages); j++)
        {
            packet[payloadOffset + j] = messages[i][j];
        }

        // Set checksum bytes to the null.
        packet[packetSize - 2] = 0x00;
        packet[packetSize - 1] = 0x00;
        _calcChecksum(&packet[packetSize - 2], &packet[2], (packetSize - 2));

        ublox_sendPacket(packet, packetSize);
    }
}

// U-blox receiver change baudrate to 115200
void ublox_setBaudrate()
{
    byte packet[] = {
        0xB5, // sync 1
        0x62, // sync 2
        0x06, // class
        0x00, // id
        0x14, // length
        0x00, // length
        0x01, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xD0, // payload
        0x08, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xC2, // payload
        0x01, // payload
        0x00, // payload
        0x07, // payload
        0x00, // payload
        0x03, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xC0, // CK_A
        0x7E, // CK_B
    };
    ublox_sendPacket(packet, sizeof(packet));
}

// U-blox receiver change frequency to 16Hz
void ublox_changeFrequency()
{
    byte packet[] = {
        0xB5,
        0x62,
        0x06,
        0x08,
        0x06,
        0x00,
        0x3D,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x53,
        0x28,
    };
    ublox_sendPacket(packet, sizeof(packet));
}

// U-blox turn off receiver
void ublox_turnOff()
{

    byte packet[] = {
        0xB5,
        0x62,
        0x06,
        0x57,
        0x08,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x50,
        0x4F,
        0x54,
        0x53,
        0xAC,
        0x85,
    };
    ublox_sendPacket(packet, sizeof(packet));
}

// U-blox turn on receiver
void ublox_turnOn()
{

    byte packet[] = {
        0xB5,
        0x62,
        0x06,
        0x57,
        0x08,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x20,
        0x4E,
        0x55,
        0x52,
        0x7B,
        0xC3,
    };
    ublox_sendPacket(packet, sizeof(packet));
}

// U-blox receiver enable NAV-PVT messages
void ublox_enableNavPvt()
{
    // CFG-MSG packet.
    byte packet[] = {
        0xB5, // sync 1
        0x62, // sync 2
        0x06, // class
        0x01, // id
        0x08, // length
        0x00, // length
        0x01, // payload
        0x07, // payload
        0x01, // payload
        0x01, // payload
        0x00, // payload
        0x01, // payload
        0x01, // payload
        0x00, // payload
        0x1B, // CK_A
        0xEC, // CK_B
    };
    ublox_sendPacket(packet, sizeof(packet));
}

// U-blox receiver enable NAV-DOP messages
void ublox_enableNavDop()
{
    // CFG-MSG packet.
    byte packet[] = {
        0xB5, // sync 1
        0x62, // sync 2
        0x06, // class
        0x01, // id
        0x08, // length
        0x00, // length
        0x01, // payload
        0x04, // payload
        0x01, // payload
        0x01, // payload
        0x00, // payload
        0x01, // payload
        0x01, // payload
        0x00, // payload
        0x18, // CK_A
        0xD7, // CK_B
    };
    ublox_sendPacket(packet, sizeof(packet));
}

// U-blox receiver configuration
void configGPS()
{
    GPSSerial2.begin(9600, SERIAL_8N1, RX2, TX2);
    Serial.println("[I] U-BLOX configuration starting");
    Serial.println("[I] Switching baudrate to 115200");
    ublox_setBaudrate();
    GPSSerial2.flush();
    delay(100);
    GPSSerial2.end();
    delay(100);
    GPSSerial2.begin(115200, SERIAL_8N1, RX2, TX2);
    Serial.println("[I] Turn on receiver");
    ublox_turnOn();
    Serial.println("[I] Disabling NMEA messages");
    ublox_noNMEA();
    Serial.println("[I] Changing frequency to 16Hz");
    ublox_changeFrequency();
    Serial.println("[I] Enabling NAV-PVT / NAV-DOP messages");
    ublox_enableNavPvt();
    ublox_enableNavDop();
    Serial.println("[I] U-BLOX configuration finished");
}

// U-blox read incoming messages
bool read_ublox()
{
    uint8_t _checksum[2];
    const uint8_t _ubxHeader[2] = {0xB5, 0x62};
    while (GPSSerial2.available())
    {
        _byte = GPSSerial2.read();

        if (_parserState < 2)
        {
            if (_byte == _ubxHeader[_parserState])
            {
                _parserState++;
            }
            else
            {
                _parserState = 0;
            }
        }
        else
        {
            if (_parserState == 2)
            {
                if (_byte == 1)
                { // NAV
                    msg_length = 2;
                }
            }
            if (_parserState == 3)
            {
                if (_byte == UBX_ID_NAV_DOP)
                {
                    msg_length = 22; // 18+4
                }
                else if (_byte == UBX_ID_NAV_PVT)
                {
                    msg_length = 96; // 92+4
                }
                else
                {
                    msg_length = 0;
                }
            }
            if ((_parserState - 2) < msg_length)
            {
                *((uint8_t *)&_tempPacket + _parserState - 2) = _byte;
            }
            _parserState++;
            // compute checksum
            if ((_parserState - 2) == msg_length)
            {
                _calcChecksum(_checksum, ((uint8_t *)&_tempPacket), msg_length);
            }
            else if ((_parserState - 2) == (msg_length + 1))
            {
                if (_byte != _checksum[0])
                {
                    _parserState = 0;
                }
            }
            else if ((_parserState - 2) == (msg_length + 2))
            {
                _parserState = 0;
                if (_byte == _checksum[1])
                {
                    memcpy(&_validPacket, &_tempPacket, sizeof(_validPacket));
                    return true;
                }
            }
            else if (_parserState > (msg_length + 4))
            {
                _parserState = 0;
            }
        }
    }
    return false;
}

#pragma endregion GPS
//-----

//----- BLE
#pragma region BLE

#define RACECHRONO_UUID \
    "00001ff8-0000-1000-8000-00805f9b34fb" // RaceChrono service UUID
String device_name = "RC_DYI_GPS_AP";
BLEServer *BLE_server = NULL;
BLEService *BLE_service = NULL;
BLECharacteristic *BLE_GPS_Main_Characteristic =
    NULL; // RaceChrono GPS Main characteristic UUID 0x03
BLECharacteristic *BLE_GPS_Time_Characteristic =
    NULL; // RaceChrono GPS Time characteristic UUID 0x04

bool deviceConnected = false;
bool oldDeviceConnected = false;

// Bluetooth Low Energy BLEServerCallbacks
class ServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *BLE_server)
    {
        deviceConnected = true;
        Serial.println("[I] Bluetooth client connected!");
    };

    void onDisconnect(BLEServer *BLE_server)
    {
        deviceConnected = false;
        Serial.println("[I] Bluetooth client disconnected!");
    }
};

// BLE configuration
void configBLE()
{
    BLEDevice::init(device_name.c_str());

    BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_DEFAULT);
    BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_ADV);
    BLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_SCAN);

    BLE_server = BLEDevice::createServer();
    BLE_server->setCallbacks(new ServerCallbacks());
    BLE_service = BLE_server->createService(RACECHRONO_UUID);

    // GPS main characteristic definition
    BLE_GPS_Main_Characteristic = BLE_service->createCharacteristic(
        BLEUUID((uint16_t)0x3), BLECharacteristic::PROPERTY_READ |
                                    BLECharacteristic::PROPERTY_NOTIFY |
                                    BLECharacteristic::PROPERTY_INDICATE);
    BLE_GPS_Main_Characteristic->addDescriptor(new BLE2902());

    // GPS time characteristic definition
    BLE_GPS_Time_Characteristic = BLE_service->createCharacteristic(
        BLEUUID((uint16_t)0x4), BLECharacteristic::PROPERTY_READ |
                                    BLECharacteristic::PROPERTY_NOTIFY |
                                    BLECharacteristic::PROPERTY_INDICATE);
    BLE_GPS_Time_Characteristic->addDescriptor(new BLE2902());

    BLE_service->start();

    BLEAdvertising *BLE_advertising = BLEDevice::getAdvertising();
    BLE_advertising->addServiceUUID(RACECHRONO_UUID);
    BLE_advertising->setScanResponse(false);
    BLE_advertising->setMinInterval(50);
    BLE_advertising->setMaxInterval(100);
    BLEDevice::startAdvertising();
}
#pragma endregion BLE
//-----

//----- BUTTONS
#pragma region BUTTONS

// #define BUTTON1PIN 0
#define BUTTON2PIN 35
// Button2 btn1(BUTTON1PIN);
Button2 btn2(BUTTON2PIN);

void button_init()
{
    // btn1.setClickHandler([](Button2 &b) { toggleScreen(); });
    // btn1.setLongClickHandler([](Button2 &b) { toggleScreen(); });

    btn2.setClickHandler([](Button2 &b)
                         { toggleScreen(); });
    btn2.setLongClickHandler([](Button2 &b)
                             { toggleScreen(); });
}

void button_loop()
{
    // btn1.loop();
    btn2.loop();
}

#pragma endregion BUTTONS
//----- BUTTONS

// Setup UART/BLE/GPS/AXP
void setup()
{
    // ESP32 UART - 115200
    Serial.begin(115200);

    if (!SPIFFS.begin())
    {
        Serial.println("SPIFFS initialisation failed!");
        while (1)
            yield(); // Stay here twiddling thumbs waiting
    }
    Serial.println("\r\nSPIFFS available!");

    displayInit();

    configBLE();

    configGPS();

    button_init();

    voltageReadInit();
    xTaskCreatePinnedToCore(battery_loop, "battery_info", 4096, NULL, 2, &batteryTaskHandle, 0);

    xTaskCreatePinnedToCore(screen_loop, "screen_loop", 4096, NULL, 1, &screenTaskHandle, 0);

    Serial.println("[I] Setup complete");
}

// Main loop
void loop()
{
    button_loop();

    if (!screenOff || deviceConnected)
    {
        if (read_ublox())
        {
            if (_validPacket.pvt.message_id == UBX_ID_NAV_PVT)
            {
                dateAndHour = (_validPacket.pvt.year - 2000) * 8928 +
                              (_validPacket.pvt.month - 1) * 744 +
                              (_validPacket.pvt.day - 1) * 24 + _validPacket.pvt.hour;

                /*
                 * UBLOX   -> scaling:N/A unit:min name:min  desc:Minute of hour,
                 * range 0..59 (UTC)
                 *   -> scaling:N/A unit:s   name:sec  desc:Seconds of minute, range
                 * 0..60 (UTC)
                 *   -> scaling:N/A unit:ns  name:nano desc:Fraction of second, range
                 * -1e9 .. 1e9 (UTC) RaceChrono -> time from hour start = (minute *
                 * 30000) + (seconds * 500) + (milliseconds / 2)
                 */
                timeSinceHourStart = _validPacket.pvt.min * 30000 +
                                     _validPacket.pvt.sec * 500 +
                                     (_validPacket.pvt.nano / 1000000.0) / 2;

                /*
                 * UBLOX   -> scaling:1e-5 unit:deg name:headMot desc:Heading of
                 * motion (2-D) RaceChrono -> scaling:1e+2 unit:deg
                 */
                gps_bearing =
                    max(0.0, round((double)_validPacket.pvt.headMot / 1000.0));

                /*
                 * UBLOX   -> scaling:N/A unit:mm name:hMSL desc:Height above mean sea
                 * level RaceChrono -> scaling:N/A unit:deg
                 */
                gps_altitude =
                    ((double)_validPacket.pvt.hMSL * 1e-3) > 6000.f
                        ? ((int)max(0.0, round(((double)_validPacket.pvt.hMSL * 1e-3) +
                                               500.f)) &
                           0x7FFF) |
                              0x8000
                        : (int)max(0.0, round((((double)_validPacket.pvt.hMSL * 1e-3) +
                                               500.f) *
                                              10.f)) &
                              0x7FFF;

                /*
                 * UBLOX   -> scaling:N/A unit:mm/s name:gSpeed desc:Ground Speed
                 * (2-D) RaceChrono -> scaling:N/A unit:km/h
                 */
                gps_speed =
                    ((double)_validPacket.pvt.gSpeed * 0.0036) > 600.f
                        ? ((int)(max(0.0,
                                     round(((double)_validPacket.pvt.gSpeed * 0.0036) *
                                           10.f))) &
                           0x7FFF) |
                              0x8000
                        : (int)(max(0.0,
                                    round(((double)_validPacket.pvt.gSpeed * 0.0036) *
                                          100.f))) &
                              0x7FFF;

                if (!screenOff)
                {
                    sattelitesCount = _validPacket.pvt.numSV;
                    fixType = _validPacket.pvt.fixType;
                    latitude = _validPacket.pvt.lat * 1e-7;
                    longitude = _validPacket.pvt.lon * 1e-7;
                    altitude = _validPacket.pvt.hMSL * 1e-3;
                    speed = gps_speed;
                    bearing = gps_bearing / 100;
                }
            }

            if (deviceConnected)
            {
                if (_validPacket.pvt.message_id == UBX_ID_NAV_PVT)
                {
                    rc_data[0] =
                        ((gpsSyncBits & 0x7) << 5) | ((timeSinceHourStart >> 16) & 0x1F);
                    rc_data[1] = timeSinceHourStart >> 8;
                    rc_data[2] = timeSinceHourStart;
                    rc_data[3] = ((min(0x03, (int)_validPacket.pvt.fixType) & 0x3) << 6) |
                                 ((min(0x3F, (int)_validPacket.pvt.numSV)) & 0x3F);
                    rc_data[4] = _validPacket.pvt.lat >> 24;
                    rc_data[5] = _validPacket.pvt.lat >> 16;
                    rc_data[6] = _validPacket.pvt.lat >> 8;
                    rc_data[7] = _validPacket.pvt.lat;
                    rc_data[8] = _validPacket.pvt.lon >> 24;
                    rc_data[9] = _validPacket.pvt.lon >> 16;
                    rc_data[10] = _validPacket.pvt.lon >> 8;
                    rc_data[11] = _validPacket.pvt.lon;
                    rc_data[12] = gps_altitude >> 8;
                    rc_data[13] = gps_altitude;
                    rc_data[14] = gps_speed >> 8;
                    rc_data[15] = gps_speed;
                    rc_data[16] = gps_bearing >> 8;
                    rc_data[17] = gps_bearing;
                }
                else if (_validPacket.dop.message_id == UBX_ID_NAV_DOP)
                {
                    /*
                     * UBLOX   -> scaling:1e-2 unit:N/A name:hDOP des:Horizontal DOP
                     *   -> scaling:1e-2 unit:N/A name:vDOP des:Vertical DOP
                     * RaceChrono -> scaling:1e1  unit:N/A
                     */
                    rc_data[18] = _validPacket.dop.hDOP / 10;
                    rc_data[19] = _validPacket.dop.vDOP / 10;
                }

                BLE_GPS_Main_Characteristic->setValue(rc_data, 20);
                BLE_GPS_Main_Characteristic->notify();

                if (gpsPreviousDateAndHour != dateAndHour)
                {
                    gpsPreviousDateAndHour = dateAndHour;
                    gpsSyncBits++;
                    rc_data[0] = ((gpsSyncBits & 0x7) << 5) | ((dateAndHour >> 16) & 0x1F);
                    rc_data[1] = dateAndHour >> 8;
                    rc_data[2] = dateAndHour;
                    BLE_GPS_Time_Characteristic->setValue(rc_data, 3);
                    BLE_GPS_Time_Characteristic->notify();
                }
            }
            if (!deviceConnected && oldDeviceConnected)
            {
                delay(500);
                BLE_server->startAdvertising();
                Serial.println("[I] Bluetooth device discoverable");
                oldDeviceConnected = deviceConnected;
            }
            if (deviceConnected && !oldDeviceConnected)
            {
                oldDeviceConnected = deviceConnected;
            }
        }
    }
}