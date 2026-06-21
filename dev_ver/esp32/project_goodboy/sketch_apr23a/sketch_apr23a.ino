#include <SPI.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN   38
#define NUM_LEDS  1
#define PIN_CS     10
#define PIN_MOSI   11
#define PIN_SCK    12
#define PIN_MISO   13
#define PIN_READY  14

SPIClass spi(FSPI);

uint8_t txData = 0;
Adafruit_NeoPixel pixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    Serial.begin(115200);

    pinMode(PIN_CS, OUTPUT);
    digitalWrite(PIN_CS, HIGH);

    pinMode(PIN_READY, INPUT_PULLUP);

    spi.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);

    Serial.println("ESP32 SPI Master Test");
    pixel.begin();
    pixel.clear();
    pixel.show();
}

void loop() {

    // Wait for STM32 ready signal
    if (digitalRead(PIN_READY) == HIGH) {

        uint8_t rxData = 0;

        digitalWrite(PIN_CS, LOW);

        spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

        rxData = spi.transfer(txData);

        spi.endTransaction();

        digitalWrite(PIN_CS, HIGH);

        Serial.print("TX: ");
        Serial.print(txData);

        Serial.print("   RX: ");
        Serial.println(rxData);

        txData++;

        delay(500);
        pixel.setPixelColor(0, pixel.Color(255, 0, 0));
    pixel.show();
    delay(500);

    // Green
    pixel.setPixelColor(0, pixel.Color(0, 255, 0));
    pixel.show();
    delay(500);

    // Blue
    pixel.setPixelColor(0, pixel.Color(0, 0, 255));
    pixel.show();
    delay(500);

    // Off
    pixel.clear();
    pixel.show();
    delay(500);
    }
    else {
        Serial.println("STM32 not ready");
        delay(500);
    }
}