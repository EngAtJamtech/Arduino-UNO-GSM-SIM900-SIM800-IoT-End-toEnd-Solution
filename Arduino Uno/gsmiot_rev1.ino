/* Arduino UNO IoT Device
 * ---------------------
 *
 * Internet of things (IoT) demonstration using Arduino Uno with SIM900 or SIM800
 * shield.
 *
 * Reads a potentionmeter and sends the value to proprietary server. Refer
 * to www.jamtechiot.co.uk/iot-resource
 *
 *
 * 03-10-2018 Rev 1.0
 * 1. Initial release.
 *
 * ==========================================================================
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a copy of the GNU General Public License see:
 * https://www.gnu.org/licenses/.
 * ==========================================================================
 */
#include <SoftwareSerial.h>

//------------//
// CONFIGURE
//------------//
// Set your server URL and API here.
#define      _SRV_URL       "your_host_url"
#define      _SRV_API       "/api.php"

// Set your SIM provider Access point name here.
#define      _APN_NAME      "giffgaff.com"
#define      _APN_USER      "giffgaff"
#define      _APN_PASSWORD  ""

// Device details:
#define      _DEV_ID        "123456"


///////////////////////////////////////////////////////////////////////////////
// TIMER METHODS                                                             //
///////////////////////////////////////////////////////////////////////////////

// Interrupt driver timer counter. Incremented once per millisecond.
static signed long _msTimerCounter;

// Defines the timer variable type.
#define TTimer long

// Define macro to turn on or off timer1 interrupts.
#define _TM1_INTERRUTS_ON TIMSK1 |= (1 << TOIE1)
#define _TM1_INTERRUTS_OFF TIMSK1 &= ~(1 << TOIE1)


/**
 * Initialise timer. Uses hardware timer1.
 *
 */
void timerInit()
{
  // Initialize timer1
  noInterrupts();           // Disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;

  TCNT1 = 65475;            // Preload timer 65536-(16MHz/256/1000Hz).
  TCCR1B |= (1 << CS12);    // 256 prescaler.
  TIMSK1 |= (1 << TOIE1);   // Enable timer overflow interrupt.
  interrupts();             // Enable all interrupts.
}


/**
 * Timer1 overflow interrupt.
 */
ISR(TIMER1_OVF_vect)
{
  TCNT1 = 65475;            // Preload timer to time out after ~1ms.
  _msTimerCounter++;        // Increment the 1ms timer counter.
}

/**
 * Get safe value of 1ms timer.
 *
 */
long timerGetTimerCounter()
{
  long t1, t2;

  do  {
    t1 = _msTimerCounter;
    t2 = _msTimerCounter;
  } while (t1 != t2);

  // Return value of timer.
  return t1;
}

/**
 * Sets up timer to time out after a number of milliseconds.
 * Use with timedOut()
 *
 * @param timer Pointer to timer variable.
 * @param value Value in milliseconds to time out.
 */
void timeOutAfter(long *timer, long value)
{
  *timer = timerGetTimerCounter() + value;
}


/**
 * Test timer for time out.
 *
 * @return 0:Not timed out, 1:timed out.
 */
int timedOut(long timer)
{
  long diff;

  diff = timer - timerGetTimerCounter();

  if (diff < 0)
    // Timed out.
    return !0;
  else
    // Not timed out.
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
// GSM METHODS  (for SIM900 shield)                                          //
///////////////////////////////////////////////////////////////////////////////

// GSM uses software serial on io pin 7 & 8. Ensure jumpers
// are set correctly on GSM SIM900 or SIM800 shield.
const byte    GSM_RX_PIN = 7;
const byte    GSM_TX_PIN = 8;

// General purpose GSM line buffer. This is in global space
// to save stack space.
char         _gsmLine[80];

// Define software serial object for GSM.
SoftwareSerial _gsmSer (GSM_RX_PIN, GSM_TX_PIN);

/**
 * Send string to GSM.
 *
 * @param s String to send.
 */
void gsmSend(char *s)
{
  // Out put to monitor.
  Serial.print(F("[GSM TX]:"));
  Serial.println(s);

  // Send to GSM.
  _gsmSer.print(s);
}


/**
 * Send string to GSM and append line feed carriage return.
 *
 * @param s String to send.
 */
void gsmSendLn(char *s)
{
  // Output to monitor.
  Serial.print(F("[GSM TX]:"));
  Serial.print(s);
  Serial.println(F("\\r\\n"));

  // Send the string to GSM.
  _gsmSer.println(s);
}

/**
 * Sends command string to GSM and wait for OK response or timeout.
 *
 * @return 0:No, or wrong response. 1:Success got OK response.
 */
int gsmSendLnWaitOK(char *cmd)
{

  // Send command.
  gsmSendLn(cmd);

  // Get the reponse.
  gsmGetLnWait( _gsmLine, sizeof( _gsmLine), 1000);

  if (strstr(_gsmLine, "OK"))
    return 1;
  else
    return 0;

}

/**
 * Wait for response from GSM. Waits upto msTimeout milliseconds.
 *
 * @param line Line buffer where response will be stored.
 * @param n Size of line buffer.
 * @param msTimeout Max time to wait for response.
 * @return 0:timeout waiting for response. 1: success.
 */
int gsmGetLnWait(char *line, int n, long msTimeout)
{
  char ch;
  char *psz;
  char *pszEnd;
  TTimer msTimer;

  // Initialise.
  psz = line;
  *psz = 0;
  pszEnd = line + n-1;
  timeOutAfter(&msTimer, msTimeout);

  // Collect response line from GSM or timeout.
  while ( 1 ) {
    // Return error if timed out.
    if (timedOut(msTimer)) {
      *psz = 0; // Terminate with null.
      _TM1_INTERRUTS_OFF;
      Serial.print(F("[GSM RX]: "));
      Serial.println(line);
      _TM1_INTERRUTS_ON;
      // Uncomment to reveal time outs.
      //Serial.println("<timed out>");
      return 0;
    }

    // Collect one character.
    if (_gsmSer.available())
      ch = _gsmSer.read();
    else
      // Nothing to collect, go round again.
    continue;

    // Parse collected character.
    switch(ch) {

      // Line feed //
    case '\n':
      // Got a line feed. That means we've received a complete line.
      *psz = 0; // Terminate with null.

      // Ignore blank lines.
      if (line[0] == 0)
        // Try for another.
        continue;

      // Got a non-blank line. Output to monitor and return.
      _TM1_INTERRUTS_OFF;
      Serial.print(F("[GSM RX]:"));
      Serial.println(line );
      _TM1_INTERRUTS_ON;
      return 1; // Success got line.
      break;

      // Carriage return //
    case '\r':
      break;  // Ignore carriage return;

      // All other characters //
    default:
      if ( psz < pszEnd) {
        // Insert char into buffer.
        *psz = ch;
        psz++;
      }
      else {
        // Maxed out buffer.
        *psz = 0;
      }
      break;

    } // end switch
  } // end while ( gsmRxHit() )
}

/**
 * Get connection status.
 */
int gsmGetConnectionStatus(char *line, int n)
{
  // Request connection status.
  gsmSendLn("AT+CIPSTATUS");

  // Get 1st line response.
  gsmGetLnWait(line, n, 1000);     // OK.

  if (gsmGetLnWait(line, n, 1000))
    return 1;
  else
    return 0;
}


/**
 * Get the registration status. Refer to AT command
 * for registration number meanings.
 *
 * @return -1: Error, n:reg status.
 */
int gsmGetRegistrationStatus()
{
    //char line[16];
    int reg;

    // Send the registration command.
    gsmSendLn("AT+CREG?");

    // Get response.
    // Expect "+CREG: 0,1"
    //                  ^- Reg. status.
    if (!gsmGetLnWait(_gsmLine, sizeof(_gsmLine), 500)) {
      // Error, could not get response.
      return -1;
    }
    reg = _gsmLine[9] - '0';

    // Expect OK.
    gsmGetLnWait(_gsmLine, sizeof(_gsmLine), 500);

    // Return registration satus.
    return reg;
}

/**
 * Initiate start of sending data.
 *
 * @return 0:Error, 1:success.
 */
int gsmDataBegin()
{
  //char line[8];

  // Data begin.
  Serial.println(F("Begin data send:..."));
  gsmSendLn("AT+CIPSEND");
  gsmGetLnWait(_gsmLine, sizeof(_gsmLine), 2000);
  if (!strstr(_gsmLine, "> ")) {
    // Error.
    Serial.println(F("Begin data send:fail."));
    return 0;
  }

  // Success.
  Serial.println(F("Begin data send:sucess."));
  return 1;
}

/**
 * End data send.
 *
 * @return 0:Error, 1:Success.
 */
int gsmDataEnd()
{
  //char line[16];

  // Send a 0x1A to invoke sending data thats in the GSM transmit
  // buffer. This can take serveral seconds.
  Serial.println(F("End data send:..."));
  _gsmLine[0] = 0x1A;
  _gsmLine[1] = 0;
  gsmSend(_gsmLine);

  // Expect SEND OK response from GSM. Allow good half minute for this.
  gsmGetLnWait(_gsmLine, sizeof(_gsmLine), 30000);
  if (!strstr(_gsmLine, "SEND OK")) {
      // Error did not get expected response.
      Serial.println(F("End data send:fail"));
      return 0;
  }

  // Success.
  Serial.println(F("End data send:sucess."));
  return 1;
}


/**
 * Close socket.
 *
 */
int gsmSocketDisconnect()
{
  //char line[10];

  Serial.println(F("Disconnect:..."));
  gsmSendLn("AT+CIPSHUT");
  gsmGetLnWait(_gsmLine, sizeof(_gsmLine), 10000);
  if (!strstr(_gsmLine, "SHUT OK")) {
    // Failed to disconnect.
    Serial.println(F("Disconnect: fail."));
    return 0;
  }

  // Success.
  Serial.println(F("Disconnect success."));
  return 1;
}


/**
 * Connect to socket.
 *
 * @return  0: Fail. 1:success.
 */
int gsmSocketConnect()
{
  //char line[32];
  int i, reg;


  // Wait for registration.
  Serial.println(F("Registration:..."));
  i = 6;
  while(1) {

    reg = gsmGetRegistrationStatus();
    if (reg==1 || reg==5)
      // Great, registered with network. Break from loop.
      break;

    // Another attempt.
    if (i-- == 0) {
      // Too manu attempts. Return error.
      Serial.println(F("Registration: not registered."));
      return 0;
    }

    // Check in 5 seconds time.
    delay(5000);
  }
  Serial.println(F("Registration: registered."));


  // Shut CIP.
  Serial.println(F("Shut:..."));
  gsmSendLn("AT+CIPSHUT");
  if (!gsmGetLnWait(_gsmLine, sizeof(_gsmLine), 500)) {
      Serial.println(F("Shut: fail."));
      return 0;
  }

  if (!strstr(_gsmLine, "SHUT OK")) {
      Serial.println(F("Shut: fail."));
      return 0;
  }

  // Attach.
  Serial.println("Attach:...");
  if (!gsmSendLnWaitOK("AT+CGATT=1")) {
      Serial.println(F("Attach: fail."));
      return 0;
  }
  Serial.println(F("Attach: success."));
  gsmGetConnectionStatus(_gsmLine, sizeof(_gsmLine));


  // Define PDP context.
  Serial.println(F("Set PDP:..."));
  sprintf(_gsmLine, "AT+CGDCONT=1,\"IP\",\"%s\"", _APN_NAME );
    if (!gsmSendLnWaitOK(_gsmLine)) {
    Serial.println(F("Set PDP fail."));
    return 0;
  }
  Serial.println(F("Set PDP: success."));
  gsmGetConnectionStatus(_gsmLine, sizeof(_gsmLine));

  // Set APN.
  Serial.println(F("Set APN:..."));
  sprintf(_gsmLine, "AT+CSTT=\"%s\",\"%s\",\"%s\"", _APN_NAME, _APN_USER, _APN_PASSWORD);
  if (!gsmSendLnWaitOK(_gsmLine)) {
    Serial.println(F("Set APN fail."));
    return 0;
  }
  Serial.println(F("Set APN: success."));
  gsmGetConnectionStatus(_gsmLine, sizeof(_gsmLine));

  // Bring up wirless. This can take many seconds.
  Serial.println(F("Bring up wireless:..."));
  gsmSendLn("AT+CIICR");
  gsmGetLnWait(_gsmLine, sizeof(_gsmLine), 30000);
  if (!strstr(_gsmLine, "OK")) {
    Serial.println(F("Bring up wireless: fail."));
    return 0;
  }
  Serial.println(F("Bring up wireless: success."));

  // Get local IP address.
  gsmSendLn("AT+CIFSR");
  gsmGetLnWait(_gsmLine, sizeof(_gsmLine), 500);
  gsmGetConnectionStatus(_gsmLine, sizeof(_gsmLine));


  // Start TCP connection.
  Serial.println(F("Start TCP connect:..."));
  gsmSend("AT+CIPSTART=\"TCP\",\"");
  gsmSend(_SRV_URL);                          // _SRV_URL
  gsmSendLn("\",\"80\"");                // Port.

  // Expect OK.
  gsmGetLnWait(_gsmLine, sizeof(_gsmLine), 30000);
  if (!strstr(_gsmLine, "OK")) {
    Serial.println(F("Start TCP connect: fail."));
    return 0;
  }
  // Expect CONNECT OK.
  gsmGetLnWait(_gsmLine, sizeof(_gsmLine), 30000);
  if (!strstr(_gsmLine, "CONNECT OK")) {
    Serial.println(F("Start TCP connect: fail."));
    return 0;
  }
  Serial.println(F("Start TCP connect: success."));

  // Success.
  return 1;
}


/**
 * Initialise GSM.
 *
 * @return 0:Initialise error, 1:Success.
 */
int gsmInit()
{
  int tries = 0;

  // Open serial communications to GSM and initialise.
  _gsmSer.begin(9600);

  // Loop until got AT response.
  while (1) {
    if (tries++ > 5) {
      Serial.println(F("Is the GSM powered up?"));
      tries = 0;
    }

    // Send AT command.
    Serial.println(F("Initialise GSM:..."));
    _gsmSer.flush();
    gsmSendLn("AT");

    // Expect echoed AT or OK response.
    if (!gsmGetLnWait(_gsmLine, sizeof(_gsmLine), 1000) != 0)
      // Timed out waiting for response. Try again.
      continue;

    if (strstr(_gsmLine, "OK")) {
      // Great! Echo is turned off, all done.
      break;
    }

    if (strstr(_gsmLine, "AT")) {
      // Echo not off so lets turn it off.
      _gsmSer.flush();
      gsmSendLn("ATE0");
    }
  }

  // Text mode.
  gsmSendLnWaitOK("AT+CMGF=1");

  // Sucess.
  Serial.println(F("Initialise GSM: success"));
  return 1;
}


///////////////////////////////////////////////////////////////////////////////
// APPLICATION CODE                                                          //
///////////////////////////////////////////////////////////////////////////////

// Timer for LED blink one second blink.
TTimer        _ledTimer;

// Timer for server update rate.
TTimer        _serverUpdateTimer;

// Define pin for LED.
#define       _LED_PIN 13

// Define pin used for reading analogue potentionmeter.
#define       _POT_PIN 0

/**
 * Send sensor data to server.
 *
 * @param field1 Field 1 sensor value to send to server.
 * @return 0:failed to send, 1: successful send.
 */
int sendSensorDataToServer(int field1)
{
  char line[40];
  int len;
  char lenStr[4];


  //-------------------
  // Connect to socket.
  //--------------------
  _gsmSer.flush();
  if (!gsmSocketConnect())
    return 0;

  //-----------
  // Send data.
  //-----------

  // Begin data send.
  if (!gsmDataBegin()) {
    // Error occured.
    return 0;
  }

  // Assemble HTTP POST data string.
  sprintf(line, "device_id=%s&field1=%d", _DEV_ID, field1);
  len = strlen(line);
  sprintf(lenStr, "%d", len);

  // Send the HTTP POST header.
  gsmSend("POST "); gsmSend(_SRV_API); gsmSendLn(" HTTP/1.1");
  gsmSend("Host: "); gsmSendLn(_SRV_URL);
  gsmSendLn("Connection: close");
  gsmSendLn("Content-Type: application/x-www-form-urlencoded");
  gsmSend("Content-Length: "); gsmSendLn(lenStr);
  gsmSendLn("");

  // Send HTTP data.
  gsmSendLn(line);


  if (!gsmDataEnd()) {
    // Error ending data send.
    return 0;
  }

  //--------------------------------------
  // Collect response from server, if any.
  //--------------------------------------
  Serial.println(F("\r\nServer response:..."));
  while (1) {

    if (!gsmGetLnWait(line, sizeof(line), 1000)) {
      // Timed out. No response or no more lines sent by server.
      break;
    }
    // This is where to process the received line of data if needed.

  }
  Serial.println(F("Server response: end\r\n"));


  //--------------------
  // Disconnect socket.
  //--------------------
  if (!gsmSocketDisconnect())
    return 0;
  else
    return 1;
}

/**
 * Setup. Execution starts here.
 */
void setup()
{
  // Set the LED pin and timer.
  pinMode(_LED_PIN, OUTPUT);
  timeOutAfter(&_ledTimer, 5000);

  // Initialise the timer. General purpose timer.
  timerInit();

  // Initialise the monitor.
  Serial.begin(9600);
  while(!Serial);

  // Initialise GSM.
  gsmInit();

  // Set initial server update to take place in 5 seconds.
  timeOutAfter(&_serverUpdateTimer, 5000);
}

/**
 * Main program loop.
 */
void loop()
{
  int potVal;


  // Every second.
  if (timedOut(_ledTimer) ) {
    // Reload timer counter.
    timeOutAfter(&_ledTimer, 1000);

    // Toggle LED.
    digitalWrite(_LED_PIN, digitalRead(_LED_PIN) ^ 1);
  }


  // Send sensor data to server every minute.
  if (timedOut(_serverUpdateTimer) ) {
    // Set to time out in one minute.
    timeOutAfter(&_serverUpdateTimer, 60000);

    // Read the value of the potentiometer (sensor) and send it to the
    // server.
    potVal = analogRead(_POT_PIN);
    sendSensorDataToServer( potVal);
  }

}


