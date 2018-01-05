#include <ESP8266WiFi.h>
#include <EEPROM.h>

//////////////////////
// WiFi Definitions //
//////////////////////
const char WiFiAPPSK[] = "sparkfun";

/////////////////////
// Pin Definitions //
/////////////////////
const int LED_PIN = 5; // Thing's onboard, green LED
const int ANALOG_PIN = A0; // The only analog pin on the Thing
const int DIGITAL_PIN = 12; // Digital pin to be read

WiFiServer server(80);

boolean light_status = false; // used to know if the light is already on or off, analog light sensor will tell us.

int time_hour = 16;
int time_minute = 57;
int time_sec = 0;

int alarm_hour = 16;
int alarm_minute = 46;
int alarm_sec = 0;

long unsigned a_millis = 1000;
long unsigned b_millis = 1000;

boolean alarm_flag = true; // to know if the alarm has gone off

//////////// RTC stuff begin

#include <SparkFunDS1307RTC.h>
#include <Wire.h>

// Comment out the line below if you want month printed before date.
// E.g. October 31, 2016: 10/31/16 vs. 31/10/16
#define PRINT_USA_DATE

#define SQW_INPUT_PIN 2   // Input pin to read SQW
#define SQW_OUTPUT_PIN 13 // LED to indicate SQW's state

////////////  RTC stuff end


void setup() 
{
  initHardware();
  setupWiFi();
  server.begin();
  rtc_init();
}

void loop() 
{
  check_alarm();

  /// RTC stuff begin

    static int8_t lastSecond = -1;
  
  // Call rtc.update() to update all rtc.seconds(), rtc.minutes(),
  // etc. return functions.
  rtc.update();

  if (rtc.second() != lastSecond) // If the second has changed
  {
    printTime(); // Print the new time
    update_time();
    
    lastSecond = rtc.second(); // Update lastSecond value
  }

  /// RTC stuff end


  
  //Serial.println(readLight()); // looks like it sits at 6 with no lights on, 
                               // and then is 15-160 when on. Maybe 10 is a good threshold?
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Match the request
  boolean alarm_change = false;
  int val = -1; // We'll use 'val' to keep track of both the
                // request type (read/set) and value if set.
  if (req.indexOf("/led/0") != -1)
    val = 1; // Will write LED high
  else if (req.indexOf("/led/1") != -1)
    val = 0; // Will write LED low
  else if (req.indexOf("/alarm_hour/") != -1)
  {
    alarm_change = true;
    alarm_flag = true;  
    for(int i = 1 ; i <= 23 ; i++)
    {
      String temp = "/alarm_hour/";
      temp += String(i);
      if (req.indexOf(temp) != -1) alarm_hour = i;
    }
  }
  else if (req.indexOf("/alarm_minute/") != -1)
  {
    alarm_change = true;  
    for(int i = 0 ; i <= 55 ; i += 5)
    {
      String temp = "/alarm_minute/";
      temp += String(i);
      if (req.indexOf(temp) != -1) alarm_minute = i;
    }
  }  
           
  // Otherwise request will be invalid. We'll say as much in HTML


  if(readLight() > 7) light_status = true;
  else light_status = false;

  if(((light_status == true) && (val == 1)) || ((light_status == false) && (val == 0)))
  {
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);
    delay(250);
    pinMode(4, INPUT);
  }
  
  // Set GPIO5 according to the request
  //digitalWrite(LED_PIN, val);


  client.flush();

  // Prepare the response. Start with the common header:
  String s = "HTTP/1.1 200 OK\r\n";
  s += "Content-Type: text/html\r\n\r\n";
  s += "<!DOCTYPE HTML>\r\n<html>\r\n";
  s += "<div align=\"center\">";
  s += "<font size=\"20\">";
  s += "<br>";
  s += "<br>";
  // If we're setting the LED, print out a message saying we did
  if (val >= 0)
  {
    s += "LED is now ";
    s += (val)?"off":"on";
  }
  else if (alarm_change)
  {
    s += "New alarm time set";
  }  
  else
  {
    s += "Invalid Request.<br> Try /led/1, /led/0, or /read.";
  }
  
  s += "<p></p><a href=\"http://192.168.4.1/led/0\">OFF</a>";
  s += "<br>";
  s += "<br>";
  s += "<p></p><a href=\"http://192.168.4.1/led/1\">ON</a>";
  s += "<br>"; // Go to the next line.
  s += "<br>";
  s += "<br>";
  s += "Light Sensor = ";
  s += String(readLight());
  s += "<br>"; // Go to the next line.
  s += "Time = ";
  s += String(time_hour);
  s += ":";
  s += String(time_minute);
  s += ":";
  s += String(time_sec);
  s += "<br>"; // Go to the next line.
  s += "Alarm = ";
  s += String(alarm_hour);
  s += ":";
  s += String(alarm_minute);
  s += ":";
  s += String(alarm_sec);

  s += "<br><br>"; // Go to the next line.
  s += "SET ALARM HOUR";
  s += "<br><br>"; // Go to the next line.
  for(int i = 1 ; i <= 12 ; i++ )
  {
    s += "<a href=\"http://192.168.4.1/alarm_hour/";
    s += String(i);
    s += "\">";
    s += String(i);
    s += "</a>";
    s += "  ";
  }

  s += "<br><br>"; // Go to the next line.
  for(int i = 13 ; i <= 23 ; i++ )
  {
    s += "<a href=\"http://192.168.4.1/alarm_hour/";
    s += String(i);
    s += "\">";
    s += String(i);
    s += "</a>";
    s += "  ";
  }  

  s += "<br><br>"; // Go to the next line.
  s += "SET ALARM MINUTE";
  s += "<br><br>"; // Go to the next line.
  for(int i = 0 ; i <= 55 ; i += 5 )
  {
    s += "<a href=\"http://192.168.4.1/alarm_minute/";
    s += String(i);
    s += "\">";
    s += String(i);
    s += "</a>";
    s += " ";
  }  
  
  s += "</font>";
  s += "</div>";
  s += "</html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}

void setupWiFi()
{
  WiFi.mode(WIFI_AP);

  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "ThingDev-":
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String AP_NameString = "ThingDev-" + macID;

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  WiFi.softAP(AP_NameChar, WiFiAPPSK);
}

void initHardware()
{
  Serial.begin(115200);
  pinMode(DIGITAL_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  // Don't need to set ANALOG_PIN as input, 
  // that's all it can be.
}

int readLight()
{
  int reading = 0;
  for(int i = 0 ; i < 10 ; i++ )
  {
    reading += analogRead(ANALOG_PIN);
    delay(10);
  }
  reading /= 10;
  return reading;
}


void check_alarm()
{
  if((alarm_hour == time_hour) && (alarm_minute == time_minute) && (alarm_flag == true))
  {
    Serial.println("ALARM");
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);
    delay(250);
    pinMode(4, INPUT);
    alarm_flag = false;
  }
}

void rtc_init()
{
    // Use the serial monitor to view time/date output
  //Serial.begin(9600);
  pinMode(SQW_INPUT_PIN, INPUT_PULLUP);
  pinMode(SQW_OUTPUT_PIN, OUTPUT);
  digitalWrite(SQW_OUTPUT_PIN, digitalRead(SQW_INPUT_PIN));
  
  rtc.begin(); // Call rtc.begin() to initialize the library
  // (Optional) Sets the SQW output to a 1Hz square wave.
  // (Pull-up resistor is required to use the SQW pin.)
  rtc.writeSQW(SQW_SQUARE_1);

  // Now set the time...
  // You can use the autoTime() function to set the RTC's clock and
  // date to the compiliers predefined time. (It'll be a few seconds
  // behind, but close!)
  // rtc.autoTime();
  // Or you can use the rtc.setTime(s, m, h, day, date, month, year)
  // function to explicitly set the time:
  // e.g. 7:32:16 | Monday October 31, 2016:
  
  //rtc.setTime(1, 41, 16, 3, 4, 1, 18);  // Uncomment to manually set time
  
  // rtc.set12Hour(); // Use rtc.set12Hour to set to 12-hour mode
}

void printTime()
{
  Serial.print(String(rtc.hour()) + ":"); // Print hour
  if (rtc.minute() < 10)
    Serial.print('0'); // Print leading '0' for minute
  Serial.print(String(rtc.minute()) + ":"); // Print minute
  if (rtc.second() < 10)
    Serial.print('0'); // Print leading '0' for second
  Serial.print(String(rtc.second())); // Print second

  if (rtc.is12Hour()) // If we're in 12-hour mode
  {
    // Use rtc.pm() to read the AM/PM state of the hour
    if (rtc.pm()) Serial.print(" PM"); // Returns true if PM
    else Serial.print(" AM");
  }
  
  Serial.print(" | ");

  // Few options for printing the day, pick one:
  Serial.print(rtc.dayStr()); // Print day string
  //Serial.print(rtc.dayC()); // Print day character
  //Serial.print(rtc.day()); // Print day integer (1-7, Sun-Sat)
  Serial.print(" - ");
#ifdef PRINT_USA_DATE
  Serial.print(String(rtc.month()) + "/" +   // Print month
                 String(rtc.date()) + "/");  // Print date
#else
  Serial.print(String(rtc.date()) + "/" +    // (or) print date
                 String(rtc.month()) + "/"); // Print month
#endif
  Serial.println(String(rtc.year()));        // Print year
}

void update_time()
{
  time_hour = rtc.hour();
  time_minute = rtc.minute();
  time_sec = rtc.second();
}

