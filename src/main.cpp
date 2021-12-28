#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <FS.h>
#include "FtpClient.h"
#include <ezButton.h>

const char *ssid = "Matsuya MIC";
const char *password = "M@tsuyaR&D2020";

const byte buttonPin = 4;
ezButton button(buttonPin);

unsigned long pressedTime = 0;  // button
unsigned long releasedTime = 0; // button
unsigned long i = 0;
int bytesWritten;
boolean debug = false;

//Week Days
String weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
//Month names
String months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
//value global
String strs[20] = {};
//File Name
String file_name = "";
//Path Name
String path_name = "";
// Define FTP library
FtpClient ftpClient;

// Define time.h
#define MYTZ "<+07>-7"

// function setup
void printLocalTime();             // get datetime
void test_split_str(String input); // split string by spaces
int Check_Month(String month);
void checkAndDeleteFiles();
int comparetime(String time1, String time2); // void comparetime();

void setup()
{
  Serial.begin(115200);
  button.setDebounceTime(50); // set debounce time to 50 milliseconds

  // Wifi Connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print(" | IP address: ");
  Serial.println(WiFi.localIP());

  // init parameters for time.h
  configTime(MYTZ, "pool.ntp.org", "time.nist.gov");
  printLocalTime();

  if (!SPIFFS.begin())
  {
    if (debug)
      Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  delay(1000);

  // Serial.println("*************Create Files******************");
  // bool success = SPIFFS.begin();
  // File file1 = SPIFFS.open("/26_12_2021_12_32_00.csv", "w");
  // file1.print("TEST SPIFFS");
  // file1.close();
  if (debug)
    Serial.println("*************List file******************");
  ftpClient.listAllFiles();
  delay(1000);

  // Serial.println("*************Check and Delete******************");
  // checkAndDeleteFiles();
  // delay(1000);
  if (debug)
    Serial.println("***************Remove All Files****************");
  ftpClient.removeAllFiles();
  delay(1000);

  if (debug)
    Serial.println("*************List file after remove******************");
  ftpClient.listAllFiles();
  delay(1000);

  if (debug)
    Serial.println("*******************************");
}

void loop()
{
  button.loop(); // MUST call the loop() function first

  if (button.isPressed())
    pressedTime = millis();

  if (button.isReleased())
  {
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;

    if (pressDuration < 2000)
    {
      if (debug)
        Serial.println("A short press is detected");
      if (i == 0)
      {
        printLocalTime();
        file_name = String(strs[2]) + "_" + String(Check_Month(strs[1]) + 1) + "_" + String(strs[4]) + "_" + String(strs[3] + ".csv");
        file_name.replace(":", "_");
        file_name.replace(" ", "");
        file_name.replace("\n", "");
        path_name = "/" + file_name;
        path_name.replace(" ", "");
        Serial.println(file_name);

        ftpClient.SetFilename(file_name);
        ftpClient.SetPathName(path_name);

        File file = SPIFFS.open(path_name, "w");
        bytesWritten = file.print(String(i) + "," + String(strs[3]));
        file.close();
      }
      else
      {
        printLocalTime();
        File file = SPIFFS.open(path_name, "a");
        bytesWritten = file.print("\n" + String(i) + "," + String(strs[3]));
        file.close();
      }
      if (debug)
        Serial.println("Upload File!");
      ftpClient.Upload(true);
      i++;
    }

    else if (pressDuration > 2000)
    {
      // Serial.println("A long press is detected");

      if (debug)
        Serial.println("Reset!");
      i = 0;
    }
  }
}

int Check_Month(String month)
{
  int j;
  for (j = 0; j < 12; j++)
  {
    if (months[j] == month)
    {
      break;
    }
  }
  return j;
}

void printLocalTime()
{
  time_t now = time(0); // <----------- here!
  String _time = ctime(&now);
  if (debug)
    Serial.println(_time);
  test_split_str(_time);
}

void test_split_str(String input)
{
  String str = "";
  int StringCount = 0;
  str = input;
  while (str.length() > 0)
  {
    int index = str.indexOf(' ');
    if (index == -1) // No space found
    {
      strs[StringCount++] = str;
      break;
    }
    else
    {
      strs[StringCount++] = str.substring(0, index);
      str = str.substring(index + 1);
    }
  }
  if (debug)
    for (int i = 0; i < StringCount; i++)
    {
      Serial.print(i);
      Serial.print(": \"");
      Serial.print(strs[i]);
      Serial.println("\"");
    }
}

void checkAndDeleteFiles()
{
  printLocalTime();
  String _timenow1 = String(strs[2]) + "_" + String(Check_Month(strs[1]) + 1) + "_" + String(strs[4]);
  if (debug)
  {
    Serial.print("Time now : ");
    Serial.println(_timenow1);
  }

  if (!SPIFFS.begin())
  {
    if (debug)
      Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  Dir dir = SPIFFS.openDir("");
  int i = 0;
  while (dir.next())
  {
    if (debug)
    {
      Serial.println("File name : " + dir.fileName());
      Serial.println("File name cut : " + dir.fileName().substring(1, 11));
    }

    int flag_compare = comparetime(dir.fileName().substring(1, 11), _timenow1);
    i++;
    if (flag_compare == 1)
    {
      if (debug)
        Serial.print("Deleted file : ");
      Serial.println(dir.fileName());
      SPIFFS.remove(dir.fileName());
    }
  }
}

int comparetime(String time1, String time2)
{
  int day1 = (time1.substring(0, 2)).toInt();
  int month1 = (time1.substring(3, 5)).toInt();
  int year1 = (time1.substring(6)).toInt();

  int day2 = (time2.substring(0, 2)).toInt();
  int month2 = (time2.substring(3, 5)).toInt();
  int year2 = (time2.substring(6)).toInt();

  if (year1 > year2)
    return -1;
  else if (year1 < year2)
    return +1;

  if (month1 > month2)
    return -1;
  else if (month1 < month2)
    return +1;

  if (day1 > day2)
    return -1;
  else if (day1 < day2)
    return +1;

  return 0;
}