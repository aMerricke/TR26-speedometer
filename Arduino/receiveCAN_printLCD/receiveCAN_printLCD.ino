/* Receive CAN message as float; Print float to LCD; TR Baja; Feb 2026  */

#include <mcp_can.h>
#include <SPI.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); /* common I2C address is 0x27 or 0x3F */

#define CAN0_INT 2        /* set INT to pin 2; INT is interrupt for SPI comms */
MCP_CAN CAN0(10);         /* set CS to pin 10; CS is chip select for SPI comms */

long unsigned int rxId;   /* id of message received? perhaps */
unsigned char len = 0;    /* length of message received in data bytes */
unsigned char rxBuf[8];   /* buffer for received data bytes */
char msgString[128];      /* array to store serial string */

void setup()
{
  // set up LCD screen
  lcd.init();
  lcd.backlight();
  Serial.begin(115200);
  
  // initialize MCP2515; run at 8MHz w/ baud rate of 500 kb/s; disable masks and filters
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK)
    Serial.println("MCP2515 Initialized Successfully!");
  else
    Serial.println("Error Initializing MCP2515...");
  CAN0.setMode(MCP_NORMAL); /* set operation mode to normal so the MCP2515 sends acks to received data */

  pinMode(CAN0_INT, INPUT);

  Serial.println("CAN Receive with MCP2515");
}

void loop()
{
  // only read receive buffer if CAN_0INT pin is low
  if (digitalRead(CAN0_INT))
    return;

  CAN0.readMsgBuf(&rxId, &len, rxBuf);  // read the data; len: data length; buf: data byte(s)

  // only print data to a float if the message is not a remote request 
  if ((rxId & 0x40000000) == 0x40000000) {
    sprintf(msgString, " REMOTE REQUEST FRAME");
    Serial.print(msgString);
    return;
  }

  // print raw bytes
  for (byte i = 0; i < len; i++) {
    sprintf(msgString, " 0x%.2X", rxBuf[i]);
    Serial.print(msgString);
  }

  // decode first 4 bytes into a float (little-endian)
  if (len >= 4) {
    uint32_t u =
      ((uint32_t)rxBuf[0]) |
      ((uint32_t)rxBuf[1] << 8) |
      ((uint32_t)rxBuf[2] << 16) |
      ((uint32_t)rxBuf[3] << 24);

    int32_t i32 = (int32_t)u;

    // if you sent Pitch*1000 on the RaceCapture side:
    float pitch = i32 / 10000.0f;
    Serial.print("  Pitch=");
    Serial.println(pitch, 3);
    lcd.setCursor(0, 1);
    lcd.print(pitch);
  } else {
    Serial.println("  (len < 4)");
  }
}

// Notes:
// Connect Arduino to TJA1050 CAN Module Board through SPI interface. Use digital pins 10-13 and digital pin 2.
// pin 10: SS/CS  (slave select / chip select)
// pin 11: MOSI   (master out slave in)
// pin 12: MISO   (master in slave out)
// pin 13: SCK    (serial clock)
// pin 2: INT     (interrupt)
// Also supply the CAN module board with +5V at VCC and GND at GND.

// Connect Arduino to LCD screen through I2C interface. Use Analog pins A4 and A5.
// pin A4: SDA    (serial data)
// pin A5: SCK    (serial clock)
// Also supply the LCD screen with +5V at VDD and GND at VSS.


// Extra snippet:
/// Drop the following in the loop function to debug the type of message ID we are using.
// // Determine if ID is standard (11 bits) or extended (29 bits)
// if((rxId & 0x80000000) == 0x80000000)    
//   sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
// else
//   sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);
// Serial.print(msgString);
