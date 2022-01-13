/*
   RadioLib SX127x Transmit with Frequency Hopping Example

   This example transmits packets using SX1278 LoRa radio module.
   Each packet contains up to 256 bytes of data, in the form of:
    - Arduino String
    - null-terminated char array (C-string)
    - arbitrary binary data (byte array)

   Other modules from SX127x/RFM9x family can also be used.

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx127xrfm9x---lora-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/

   The SX1276 / 7 / 8 / 9 supports FHSS or Frequency Hopping Spread Spectrum.
   Once a hopping period is set and a transmission is started the radio
   will begin triggering interrupts every hop period where the radio frequency
   is changed to the next channel. This allows a simple mechanism to abide by
   the FCC 400ms max dwell time rule.
   https://www.govinfo.gov/content/pkg/CFR-2019-title47-vol1/pdf/CFR-2019-title47-vol1-sec15-247.pdf
*/

#include <RadioLib.h> //Click here to get the library: http://librarymanager/All#RadioLib

//Pins for RFM97 100mW Shield to SparkFun ESP32 Thing Plus C
int pin_cs = 15;
int pin_dio0 = 26;
int pin_dio1 = 25;
int pin_rst = 32;
SX1276 radio = new Module(pin_cs, pin_dio0, pin_rst, pin_dio1);

int counter = 0;

volatile bool rxComplete = false;
volatile bool fhssChange = false;

//The channel frequencies can be generated randomly or hard coded
float channels[] = {908.0, 906.0, 907.0, 905.0, 903.0, 910.0, 909.0};
int numberOfChannels = sizeof(channels) / sizeof(float);

int hopsCompleted = 0;

void setup()
{
  Serial.begin(115200);

  //Begin radio on home channel
  Serial.print(F("[SX127x] Initializing ... "));
  int state = radio.begin(channels[0]);
  if (state != RADIOLIB_ERR_NONE)
  {
    Serial.print(F("Failed with code: "));
    Serial.println(state);
  }
  else
    Serial.println(F("Success!"));

  // Set hop period to enable FHSS
  // We set an artifically short period to show lots of hops
  // HoppingPeriod = Tsym * FreqHoppingPeriod
  // Given defaults of spreadfactor = 9, bandwidth = 125, it follows Tsym = 4.10ms
  // HoppingPeriod = 4.10 * 9 = 36.9ms. Can be as high as 400ms to be within regulatory limits
  radio.setFHSSHoppingPeriod(9);

  Serial.print(F("Hopping period: "));
  Serial.println(radio.getFHSSHoppingPeriod());

  radio.setDio0Action(dio0ISR); //Called when transmission is finished
  radio.setDio1Action(dio1ISR); //Called after a transmission has started, so we can move to next freq

  radio.startReceive();

  Serial.println(F("Waiting for new packet"));
}

void loop()
{
  if (rxComplete == true)
  {
    uint8_t incomingBuffer[255];
    radio.readData(incomingBuffer, 255);
    uint8_t receivedBytes = radio.getPacketLength();
    Serial.write(incomingBuffer, receivedBytes);
    Serial.println();

    Serial.print(F("Hops completed: "));
    Serial.println(hopsCompleted);
    hopsCompleted = 0;

    radio.startReceive();

    rxComplete = false;
  }

  if (fhssChange == true)
  {
    radio.setFrequency(channels[radio.getFHSSChannel() % numberOfChannels]);
    //Serial.print(F("Radio on channel: "));
    //Serial.println(radio.getFHSSChannel());

    hopsCompleted++;
    radio.clearFHSSInt();
    fhssChange = false;
  }
}

//ISR when DIO0 goes low
//Called when transmission is complete or when RX is received
void dio0ISR(void)
{
  rxComplete = true;
}

//ISR when DIO1 goes low
//Called when FhssChangeChannel interrupt occurs (at the beginning of each transmission)
void dio1ISR(void)
{
  fhssChange = true;
}
