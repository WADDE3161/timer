/*REVISION HISTORY
 * 
 * Ver 1.00
 * --------
 * First version for NodeMCU
 * 
 * 
 */

#include <FS.h>  
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>       
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

// Defines
#define      DefaultName       "IoT Smart Timer"  // Default device name
#define      NTPfastReq        10                 // NTP time request in seconds when  time not set
#define      NTPslowReq        3600               // NTP time request in seconds after time is  set
#define      Version           "1.00"             // Firmware version

// NTP Server details
//-------------------
IPAddress timeServer(129, 6, 15, 28);              // time.nist.gov NTP server
WiFiUDP Udp;
unsigned int localPort       = 3232;               // Local port to listen for UDP packets
WiFiServer server(80);

//define your default values here, if there are different values in config.json, they are overwritten.
//length should be max size + 1 
//-----------------------------
char static_ip[16] = "192.168.4.1";
char static_gw[16] = "192.168.4.1";
char static_sn[16] = "255.255.255.0";

//flag for saving configuration data
//----------------------------------
bool shouldSaveConfig = false;

// I/O config
//-----------
#define      Relay             5   // (D1) Output relay
#define      LED               4   // (D2) Satatus LED
#define      Button            0   // (D3) Button

// Variables
//----------
#define      ErrMsg            "<font color=\"red\"> < < Error !</font>"
#define      EEPROM_chk        123
String       DevName         = DefaultName;
float        TimeZone        = 0;
byte         TimeZoneH       = 0;
byte         TimeZoneM       = 0;
boolean      ResetWiFi       = false;
long         timeNow         = 0;
long         timeOld         = 300000;
boolean      TimeOk          = false;
boolean      NTPtimeOk       = false;
String       request         = "";
byte         Page            = 1;
int          IP_1            = 129;
int          IP_2            = 6;
int          IP_3            = 15;
int          IP_4            = 28;
boolean      TimeCheck       = false;
byte         Mode            = 0;
byte         OnMode          = 0;
byte         OffMode         = 0;
int          TimerMinute     = 0;
int          TimerHour       = 0;
int          NewHH           = 0;
int          NewMM           = 0;
int          Newdd           = 0;
int          Newmm           = 0;
int          Newyy           = 0;
int          PgmNr           = 0;
int          OnHour          = 0;
int          OnMinute        = 0;
int          OffHour         = 0;
int          OffMinute       = 0;
boolean      ManualOff       = false;
boolean      ManualOn        = false;
boolean      ManualTime      = false;
long         ManualSec       = 0;
int          LastHH          = 0;
int          LastMM          = 0;
int          Lastdd          = 0;
int          Lastmm          = 0;
int          Lastyy          = 0;
byte         old_sec         = 0;
long         old_ms          = 0;
boolean      WebButton       = false;
boolean      PgmPrev         = false;
boolean      PgmNext         = false;
boolean      PgmSave         = false;
boolean      Error1          = false;
boolean      Error2          = false;
boolean      Error3          = false;
boolean      Error4          = false;
boolean      Error5          = false;
boolean      Error6          = false;
boolean      Error7          = false;
boolean      Error8          = false;
boolean      D[8]            = {false, false, false, false, false, false, false, false};
int          On_Time[7];
int          Off_Time[7];
boolean      On_Days[7][8];


//###############################################################################################
// Setup
//
//###############################################################################################
void setup() {
  // For debug only - REMOVE
  //----------------------------------------------------------------------
  //Serial.begin(115200);

  // Setup I/O pins
  //----------------------------------------------------------------------
  pinMode(Button,INPUT_PULLUP);
  pinMode(Relay,OUTPUT);
  pinMode(LED,OUTPUT);
  digitalWrite(Relay,LOW);
  digitalWrite(LED,LOW);
  // If button pressed, reset WiFi connection
  //----------------------------------------------------------------------
  CheckReset();
  // Start WiFi and connect to AP
  //----------------------------------------------------------------------
  StartWiFi();
}


//###############################################################################################
// Main program loop
//
//###############################################################################################
void loop() {
  // Scan Button
  //----------------------------------------------------------------------
  ScanButton();
  // Update LED
  //----------------------------------------------------------------------
  UpdateLED();
  // Scan for NTP time changes
  //----------------------------------------------------------------------
  CheckNTPtime();
  // See if time has changed
  //----------------------------------------------------------------------
  DoTimeCheck();
  
  // Handle Web page
  //----------------------------------------------------------------------
   WiFiClient client = server.available();
  // Read requests from web page
  //----------------------------------------------------------------------
  request = client.readStringUntil('\r');
  client.flush();
  // See if data was received
  //----------------------------------------------------------------------
  if (request.indexOf("/") != -1)  {
    //Serial.println(request);
    if (request.indexOf("Link=1")     != -1) Page = 1;
    if (request.indexOf("Link=2")     != -1) Page = 2;
    if (request.indexOf("Link=3")     != -1) Page = 3;
    if (request.indexOf("Link=4")     != -1) Page = 4;
    if (request.indexOf("Link=5")     != -1) Page = 5;
    if (request.indexOf("GET / HTTP") != -1) Page = 1;
    Error1 = false;
    Error2 = false;
    Error3 = false;
    Error4 = false;
    Error5 = false;
    Error6 = false;
    Error7 = false;


    // Respond to Buttons
    //==================================

    // PAGE 1 - STATUS
    //----------------
    // See if Save Button was presed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtn1=") != -1) {
      Page = 1;
      WebButton = true;
      ScanButton();
    }
    // See if Refresh Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("RefreshBtn1=") != -1) {
      Page = 1;
    }

    // PAGE 2 - PROGRAMS
    //------------------
    // See if Previous Buttomn was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtnPrev=") != -1) {
      Page = 2;
      PgmPrev = true;
      PgmNext = false;
      PgmSave = true;
    }
    // See if Next Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtnNext=") != -1) {
      Page = 2;
      PgmPrev = false;
      PgmNext = true;
      PgmSave = true;
    }
    // See if Save Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtn2=") != -1) {
      Page = 2;
      PgmSave = true;
      PgmPrev = false;
      PgmNext = false;
    }
    // See if Clear Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("ClearBtn1=") != -1) {
      Page = 2;
      On_Time[PgmNr]  = 0;
      Off_Time[PgmNr] = 0;
      for (byte i = 0; i < 7; i++ ) {
        On_Days[PgmNr][i] = false;
      }
      PgmPrev = false;
      PgmNext = false;
      // Save program data
      SaveProgram();
    }
    // Get program data if any button was pressed
    //----------------------------------------------------------------------
    if (PgmSave == true) {
      PgmSave = false;
      // On Hour
      if (request.indexOf("OnH=") != -1)  {
        String Tmp = request;
        int t1 = Tmp.indexOf("OnH=");
        Tmp.remove(0,t1+4);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        OnHour = Tmp.toInt();
        if ( (OnHour < 0) or (OnHour > 23) ) Error1 = true;
      }
      // On Minute
      if (request.indexOf("OnM=") != -1)  {
        String Tmp = request;
        int t1 = Tmp.indexOf("OnM=");
        Tmp.remove(0,t1+4);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        OnMinute = Tmp.toInt();
        if ( (OnMinute < 0) or (OnMinute > 59) ) Error1 = true;
      }
      // Off Hour
      if (request.indexOf("OffH=") != -1)  {
        String Tmp = request;
        int t1 = Tmp.indexOf("OffH=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        OffHour = Tmp.toInt();
        if ( (OffHour < 0) or (OffHour > 23) ) Error2 = true;
      }
      // Off Minute
      if (request.indexOf("OffM=") != -1)  {
        String Tmp = request;
        int t1 = Tmp.indexOf("OffM=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        if (t1 == -1) {
          t1 = Tmp.indexOf(" ");
        }
        Tmp.remove(t1);
        OffMinute = Tmp.toInt();
        if ( (OffMinute < 0) or (OffMinute > 59) ) Error2 = true;
      }
      // Reset day flags
      D[0] = false;
      D[1] = false;
      D[2] = false;
      D[3] = false;
      D[4] = false;
      D[5] = false;
      D[6] = false;
      D[7] = false;
      // Day 1
      if (request.indexOf("D1=on") != -1) D[0] = true;
      // Day 2
      if (request.indexOf("D2=on") != -1) D[1] = true;
      // Day 3
      if (request.indexOf("D3=on") != -1) D[2] = true;
      // Day 4
      if (request.indexOf("D4=on") != -1) D[3] = true;
      // Day 5
      if (request.indexOf("D5=on") != -1) D[4] = true;
      // Day 6
      if (request.indexOf("D6=on") != -1) D[5] = true;
      // Day 7
      if (request.indexOf("D7=on") != -1) D[6] = true;
      // Update program if no errors
      if ( (Error1 == false) and (Error2 == false) ) {
        On_Time[PgmNr]  = (OnHour  * 100) + OnMinute;
        Off_Time[PgmNr] = (OffHour * 100) + OffMinute;        
        for (byte i = 0; i < 7; i++) {
          if (D[i] == true) On_Days[PgmNr][i] = true; else On_Days[PgmNr][i] = false;
        }  
        // Save program data
        ManualOff = false;
        ManualOn = false;
        ManualTime = false;
        SaveProgram();
        timeOld = 0;
      }
      else {
        PgmPrev = false;
        PgmNext = false;
      }
    }
    // Change to Prev/Next Program
    if (PgmPrev == true) {
      PgmPrev = false;
      PgmNr = PgmNr - 1;
      if (PgmNr <0) PgmNr = 6;
    }
    if (PgmNext == true) {
      PgmNext = false;
      PgmNr = PgmNr + 1;
      if (PgmNr > 6) PgmNr = 0;        
    }

    // PAGE 3 - CONFIG
    //----------------
    // See if Save Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtn3=") != -1) {
      Page = 3;
      // Device Name
      if (request.indexOf("Dev=") != -1)  {
        String Tmp = request;
        int t1 = Tmp.indexOf("Dev=");
        Tmp.remove(0,t1+4);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        Tmp.replace("+"," ");
        DevName = Tmp;
      }
      // Mode select
      if (request.indexOf("mode=Auto") != -1) Mode = 2;
      if (request.indexOf("mode=On") != -1) Mode = 1;
      if (request.indexOf("mode=Off") != -1) Mode = 0;
      //get button when on
      if (request.indexOf("BtnOff=0") != -1) OffMode = 0;
      if (request.indexOf("BtnOff=1") != -1) OffMode = 1;
      // get button when off
      if (request.indexOf("BtnOn=0") != -1) OnMode = 0;
      if (request.indexOf("BtnOn=1") != -1) OnMode = 1;
      if (request.indexOf("BtnOn=2") != -1) OnMode = 2;
      // get timer on hour
      if (request.indexOf("TonH=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("TonH=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        TimerHour = Tmp.toInt();
        if ( (TimerHour > 12) or (TimerHour < 0) ) {
          TimerHour = 0;
          Error3 = true;
        }
      }
      // get timer on min
      if (request.indexOf("TonM=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("TonM=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        TimerMinute = Tmp.toInt();
        if ( (TimerMinute > 59) or (TimerMinute < 0) ) {
          TimerMinute = 0;
          Error3 = true;
        }
      }
      if (Error3 == false) {
        SaveConfig();
        ManualOff = false;
        ManualOn = false;
        ManualTime = false;
        timeOld = 0;
      }
    }

    // PAGE 4 - NTP Setup
    //-------------------
    // See if Save Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtn4=") != -1) {
      Page = 4;
      // Time Zone
      if (request.indexOf("TZH=") != -1)  {
        String Tmp = request;
        int t1 = Tmp.indexOf("TZH=");
        Tmp.remove(0,t1+4);
        t1 = Tmp.indexOf("&");
        if (t1 == -1) {
          t1 = Tmp.indexOf(" ");
        }
        Tmp.remove(t1);
        TimeZone = Tmp.toFloat();
        if ( (TimeZone < -12) or (TimeZone > 12) ) {
          Error4 = true;
          TimeZone = 0;
        }
      }
      // Get NTP IP address
      //--------------------------------
      if (request.indexOf("IP_1=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("IP_1=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        IP_1 = Tmp.toInt();
        if ( (IP_1 < 0) or (IP_1 > 255) ) Error5 = true;
      }
      if (request.indexOf("IP_2=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("IP_2=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        IP_2 = Tmp.toInt();
        if ( (IP_2 < 0) or (IP_2 > 255) ) Error5 = true;
      }
      if (request.indexOf("IP_3=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("IP_3=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        IP_3 = Tmp.toInt();
        if ( (IP_3 < 0) or (IP_3 > 255) ) Error5 = true;
      }
      if (request.indexOf("IP_4=") != -1) {
        String Tmp = request;
        int t1 = Tmp.indexOf("IP_4=");
        Tmp.remove(0,t1+5);
        t1 = Tmp.indexOf("&");
        Tmp.remove(t1);
        IP_4 = Tmp.toInt();
        if ( (IP_4 < 0) or (IP_4 > 255) ) Error5 = true;
      }
      if ( (Error4 == false) and (Error5 == false) ) {
        // Set new NTP IP
        timeServer[0] = IP_1;
        timeServer[1] = IP_2;
        timeServer[2] = IP_3;
        timeServer[3] = IP_4;
        ManualOff = false;
        ManualOn = false;
        ManualTime = false;        
        //Save Time Server Settings
        SaveNTP();
        NTPtimeOk = false;
        setSyncInterval(NTPfastReq);
      }
    }
    // See if Save Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("SaveBtn5=") != -1) {
      Page = 4;
      //Get new hour
      String Tmp = request;
      int t1 = Tmp.indexOf("TimeHour=");
      Tmp.remove(0,t1+9);
      t1 = Tmp.indexOf("&");
      Tmp.remove(t1);
      NewHH = Tmp.toInt();
      if ( (NewHH < 0) or (NewHH > 23) ) Error6 = true;
      //Get new minute
      Tmp = request;
      t1 = Tmp.indexOf("TimeMinute=");
      Tmp.remove(0,t1+11);
      t1 = Tmp.indexOf("&");
      Tmp.remove(t1);
      NewMM = Tmp.toInt();
      if ( (NewMM < 0) or (NewMM > 59) ) Error6 = true;
      //Get new date
      Tmp = request;
      t1 = Tmp.indexOf("TimeDate=");
      Tmp.remove(0,t1+9);
      t1 = Tmp.indexOf("&");
      Tmp.remove(t1);
      Newdd = Tmp.toInt();
      if ( (Newdd < 1) or (Newdd > 31) ) Error7 = true;
      //Get new month
      Tmp = request;
      t1 = Tmp.indexOf("TimeMonth=");
      Tmp.remove(0,t1+10);
      t1 = Tmp.indexOf("&");
      Tmp.remove(t1);
      Newmm = Tmp.toInt();
      if ( (Newmm < 1) or (Newmm > 12) ) Error7 = true;
      //Get new year
      Tmp = request;
      t1 = Tmp.indexOf("TimeYear=");
      Tmp.remove(0,t1+9);
      t1 = Tmp.indexOf("&");
      if (t1 == -1) {
        t1 = Tmp.indexOf(" ");
      }
      Tmp.remove(t1);
      Newyy = Tmp.toInt();
      if ( (Newyy < 2000) or (Newyy > 2069) ) Error7 = true;
      // Update time
      //------------
      setTime(NewHH, NewMM, 0, Newdd, Newmm, Newyy);
      LastHH = NewHH;
      LastMM = NewMM;
      Lastdd = Newdd;
      Lastmm = Newmm;
      Lastyy = Newyy;
      TimeOk = true;
      ManualOff = false;
      ManualOn = false;
      ManualTime = false;
      timeOld = 0;
      setSyncInterval(NTPslowReq);
    }
    // See if Refresh Button was pressed
    //----------------------------------------------------------------------
    if (request.indexOf("RefreshBtn5=") != -1) {
      Page = 4;
    }

    // Check time before updating web page
    //----------------------------------------------------------------------
    DoTimeCheck();

    // Web Page HTML Code
    //==================================
    client.println("<!doctype html>");
    client.println("<html lang='en'>");
    client.println("<head>");
    // Refresh home page every 60 sec
    //client.println("<META HTTP-EQUIV=""refresh"" CONTENT=""60"">");
    client.print("<style> body {background-color: #C3FCF7;Color: #2B276E;}</style>");
    client.println("<title>");
    client.println(DevName);
    client.println("</title>");
    client.println("</head>");
    client.print("<body>");
    client.print("<font size = \"5\"><b>");
    client.println(DevName);
    client.print("</font></b><br>"); 
    // Show time
    //----------------------------------------------------------------------
    client.print("<p style=\"color:#180BF4;\";>");  
    client.print("<font size = \"5\"><b>");
    if (hour() < 10) client.print(" ");
    client.print(hour());
    client.print(":");
    if (minute() < 10) client.print("0");
    client.print(minute());
    client.print("</font></p>"); 
    // Day of the week
    //----------------------------------------------------------------------
    switch (weekday()) {
      case 1: client.print("Sunday, ");
              break;
      case 2: client.print("Monday, ");
              break;
      case 3: client.print("Tuesday, ");
              break;  
      case 4: client.print("Wednesday, ");
              break;  
      case 5: client.print("Thursday, ");
              break;  
      case 6: client.print("Friday, ");
              break;  
      case 7: client.print("Saturday, ");
              break;  
      default: client.print("");
              break;        
    }
    // Date
    //----------------------------------------------------------------------
    client.print(day());
    // Month
    //----------------------------------------------------------------------
    switch (month()) {
      case  1: client.print(" January ");
               break;
      case  2: client.print(" February ");
               break;
      case  3: client.print(" March ");
               break;
      case  4: client.print(" April ");
               break;
      case  5: client.print(" May ");
               break;
      case  6: client.print(" June ");
               break;
      case  7: client.print(" July ");
               break;
      case  8: client.print(" August ");
               break;
      case  9: client.print(" September ");
               break;
      case 10: client.print(" October ");
               break;
      case 11: client.print(" November ");
               break;
      case 12: client.print(" December ");
               break;
      default: client.print(" ");
               break;        
    }
    // Year
    //----------------------------------------------------------------------
    client.print(year());
    client.print("<br><br>");
    // Show system status
    //----------------------------------------------------------------------
    client.print("Output: </b>");
    if ( (TimeOk == false) and (Mode == 2) ) { 
      client.print("Blocked - Time not set!");
    }
    else {
      if ( (ManualOff == true) and (Mode == 2) ){
        client.print("Override OFF until next event");
      }
      if ( (ManualOn == true) and (Mode == 2) ){
        client.print("Override ON until next event");
      }
      if ( (ManualTime == true) and (Mode == 2) ){
        client.print("Override ON for ");
        if ( (ManualSec/3600) > 0) {
          client.print((ManualSec/3600));
          client.print(" hours, ");
        }
        client.print( ( (ManualSec%3600)/60) );
        client.print(" minutes");
      }
      if ( (ManualOff == false) and (ManualOn == false) and (ManualTime == false) ) {
        if (digitalRead(Relay) == 1) client.print("ON"); else client.print("OFF");
      }
    }
    client.println("<br><br>");
    //Menu
    //----------------------------------------------------------------------
    client.print("<form action= method=\"get\"><b>");
    client.print("<a href=\"Link=1\">Home</a>&emsp;");
    client.print("<a href=\"Link=2\">Programs</a>&emsp;");
    client.print("<a href=\"Link=3\">Config</a>&emsp;");
    client.print("<a href=\"Link=4\">Time</a><br>"); 
    // Draw line
    //----------------------------------------------------------------------
    client.print("</b><hr />");  
    
    // Status PAGE
    //============
    if (Page == 1) {
      client.print("<font size = \"4\"><b>Status</font></b><br><br>"); 
      client.print("<b>Mode : </b>");
      switch (Mode) {
        case 0 : client.print("Off");
                 break;
        case 1 : client.print("On");
                 break;
        case 2 : client.print("Auto");
                 break;
      }
      client.print("<br><br>");
      client.print("<b>Override : </b>");
      if ( (ManualOff  == true) or (ManualOn   == true) or (ManualTime == true) ) {
        client.print("Active");
      }
      else {
        client.print("Off");
      }
      client.print("<br>");
      //Button 1
      client.println("<br>");
      client.println("<input type=\"submit\" name =\"SaveBtn1\" value=\"Manual Override\">");
      //Button 2
      client.println("&emsp;");
      client.println("<input type=\"submit\" name =\"RefreshBtn1\" value=\"Refresh\">");
      client.println("<br>");
      // Draw line
      client.print("<hr />");        
      // Show last time synch
      client.print("<font size = \"2\">"); 
      client.print("</b>Version ");
      client.print(Version);
      client.print("<br>");
      client.print("Time was last updated on ");
      client.print(Lastdd);
      client.print("/");
      if (Lastmm < 10) client.print("0");
      client.print(Lastmm);
      client.print("/");
      client.print(Lastyy);
      client.print(" at ");
      if (LastHH < 10) client.print("0");
      client.print(LastHH);
      client.print(":");
      if (LastMM < 10) client.print("0");
      client.print(LastMM);
      client.print("</font>");
    }
  
    //Program  PAGE
    //============
    if (Page == 2) {
      // Program number
      client.print("<font size = \"4\"><b>"); 
      client.print("Program ");
      client.print(PgmNr + 1);
      client.print(" of 7");
      //Previous Button
      client.println("&emsp;<input type=\"submit\" name =\"SaveBtnPrev\" value=\" << \">");
      client.println("&emsp;");
      //Next Button
      client.println("<input type=\"submit\" name =\"SaveBtnNext\" value=\" >> \"></font></b><br><br>");
      //On time
      client.print("On  Time: <input type=\"text\"<input maxlength=\"2\" size=\"2\" name=\"OnH\"value =\"");
//      if ( (On_Time[PgmNr] / 100) < 10) client.print("0");
      client.print(On_Time[PgmNr]/100);    
      client.print("\"> : <input type=\"text\"<input maxlength=\"2\" size=\"2\" name=\"OnM\"value =\"");
      if ( (On_Time[PgmNr] % 100) < 10) client.print("0");
      client.print(On_Time[PgmNr]%100);    
      client.print("\">");
      if (Error1 == true) client.print(ErrMsg);
      client.print("<br><br>");
      //Off time
      client.print("Off Time: <input type=\"text\"<input maxlength=\"2\" size=\"2\" name=\"OffH\"value =\"");
//      if ( (Off_Time[PgmNr] / 100) < 10) client.print("0");
      client.print(Off_Time[PgmNr]/100);    
      client.print("\"> : <input type=\"text\"<input maxlength=\"2\" size=\"2\" name=\"OffM\"value =\"");
      if ( (Off_Time[PgmNr] % 100) < 10) client.print("0");
      client.print(Off_Time[PgmNr]%100);    
      client.print("\">");
      if (Error2 == true) client.print(ErrMsg);
      client.print("<br><br>");
      //Day 1
      client.print("<input type=\"Checkbox\" name=\"D1\"");
      if (On_Days[PgmNr][0]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Sun<br>");
      //Day 2
      client.print("<input type=\"Checkbox\" name=\"D2\"");
      if (On_Days[PgmNr][1]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Mon<br>");
      //Day 3
      client.print("<input type=\"Checkbox\" name=\"D3\"");
      if (On_Days[PgmNr][2]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Tue<br>");
      //Day 4
      client.print("<input type=\"Checkbox\" name=\"D4\"");
      if (On_Days[PgmNr][3]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Wed<br>");
      //Day 5
      client.print("<input type=\"Checkbox\" name=\"D5\"");
      if (On_Days[PgmNr][4]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Thu<br>");
      //Day 6
      client.print("<input type=\"Checkbox\" name=\"D6\"");
      if (On_Days[PgmNr][5]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Fri<br>");
      //Day 7
      client.print("<input type=\"Checkbox\" name=\"D7\"");
      if (On_Days[PgmNr][6]==true) client.print("checked"); else client.print("unchecked");
      client.print("> Sat<br>");
      //Button
      client.println("<br>");
      client.println("<input type=\"submit\" name =\"SaveBtn2\" value=\"Save\">");
      client.print("&emsp;");
      client.println("<input type=\"submit\" name =\"ClearBtn1\" value=\"Clear\">");
    }
  
    // Config PAGE
    //============
    if (Page == 3) {
      client.print("<font size = \"4\"><b>Configuration</font></b><br><br>"); 
      // Device Name
      client.print("<b>Device Name: </b>");
      client.print("<input type=\"text\"<input maxlength=\"30\" size=\"35\" name=\"Dev\" value =\"");
      client.print(DevName);
      client.println("\"><br><br>");
      //Mode Select
      client.println("<b>Mode: </b>");
      client.print("<input type=\"radio\" name=\"mode\" value=\"Auto\"");
      if (Mode == 2) client.print("checked");
      client.print("/> Auto ");
      client.print("<input type=\"radio\" name=\"mode\" value=\"On\"");
      if (Mode == 1) client.print("checked");
      client.print("/> On ");
      client.print("<input type=\"radio\" name=\"mode\" value=\"Off\"");
      if (Mode == 0) client.print("checked");
      client.print("/> Off <br><br>");
      // When output ON
      client.print("<b>When ");
      client.print(DevName);
      client.print(" is ON:</b><br>");
      // do nothing
      client.print("<input type=\"radio\" name=\"BtnOff\" value=\"0\"");
      if (OffMode == 0) client.print("checked");
      client.print("/> Do nothing<br>");
      // turn off
      client.print("<input type=\"radio\" name=\"BtnOff\" value=\"1\"");
      if (OffMode == 1) client.print("checked");
      client.print("/> Turn OFF until next event<br><br>");
      //When output is OFF
      //------------------
      client.print("<b>When ");
      client.print(DevName);
      client.print(" is OFF:<br></b>");
      // do nothing
      client.print("<input type=\"radio\" name=\"BtnOn\" value=\"0\"");
      if (OnMode == 0) client.print("checked");
      client.print("/> Do nothing<br>");
      // turn on til next event
      client.print("<input type=\"radio\" name=\"BtnOn\" value=\"1\"");
      if (OnMode == 1) client.print("checked");
      client.print("/> Turn ON until next event<br>");
      // turn on for specific time
      client.print("<input type=\"radio\" name=\"BtnOn\" value=\"2\"");
      if (OnMode == 2) client.print("checked");
      client.print("/> Turn ON for specific time<br>");
      // time input
      client.print("&emsp;&emsp;<input type=\"text\"<input maxlength=\"2\" size=\"3\" name=\"TonH\"value =\"");
      client.print(TimerHour);
      client.print("\">");
      client.print(" : <input type=\"text\"<input maxlength=\"2\" size=\"3\" name=\"TonM\"value =\"");
      if (TimerMinute < 10) client.print("0");
      client.print(TimerMinute);
      client.print("\">");
      if (Error3 == true) {
        client.print(ErrMsg);
      }
      else {
        client.print(" (hh:mm)");
      }
      //Button
      client.println("<br><br>");
      client.println("<input type=\"submit\" name =\"SaveBtn3\" value=\"Save\">");
    }
  
    // Time Server PAGE
    //=================
    if (Page == 4) {
      client.print("<font size = \"4\"><b><u>Time Setup</u></font></b><br><br>"); 
      
      //Time Zone
      client.print("<font size = \"3\"><b>NTP Network Setup</font></b><br>"); 
      client.print("Time Zone ");
      client.print("<input type=\"text\"<input maxlength=\"6\" size=\"7\" name=\"TZH\" value =\"");
      client.print(TimeZone,2);
      client.println("\">");
//      if (Error4 == true) client.print(ErrMsg); else client.print(" (hours)");
      client.print("<br><br>");
      //IP Addtess if time server
      client.print("Time Server IP : <i>(default 129.6.15.28)</i><br>");
      client.print(" <input type=\"text\"<input maxlength=\"3\" size=\"3\" name=\"IP_1\"value =\"");
      client.print(IP_1);    
      client.print("\">");
      client.print(" <input type=\"text\"<input maxlength=\"3\" size=\"3\" name=\"IP_2\"value =\"");
      client.print(IP_2);    
      client.print("\">");
      client.print(" <input type=\"text\"<input maxlength=\"3\" size=\"3\" name=\"IP_3\"value =\"");
      client.print(IP_3);    
      client.print("\">");
      client.print(" <input type=\"text\"<input maxlength=\"3\" size=\"3\" name=\"IP_4\"value =\"");
      client.print(IP_4);    
      client.print("\">");
//      if (Error5 == true) client.print(ErrMsg);
      //Button 1
      client.println("<br><br>");
      client.println("<input type=\"submit\" name =\"SaveBtn4\" value=\"Save\"><br>");
      // Draw line
      client.print("<hr />");        

      // Set Time Inputs
      client.print("<font size = \"3\"><b>Local Time Adjust</font></b><br>"); 
      client.print("<br>Time: <input type=\"text\"<input maxlength=\"2\" size=\"2\" name=\"TimeHour\"value =\"");
      client.print(hour());
      client.print("\">");
      client.print(" : <input type=\"text\"<input maxlength=\"3\" size=\"3\" name=\"TimeMinute\"value =\"");
      if (minute() < 10) client.print("0");
      client.print(minute());
      client.print("\">");
//      if (Error6 == true) client.print(ErrMsg); else client.print(" (hh:mm)");
      // Set Date Inputs
      client.print("<br><br>");
      client.print("Date: <input type=\"text\"<input maxlength=\"2\" size=\"2\" name=\"TimeDate\"value =\"");
      client.print(day());
      client.print("\">");
      client.print(" / <input type=\"text\"<input maxlength=\"3\" size=\"3\" name=\"TimeMonth\"value =\"");
      if (month() < 10) client.print("0");
      client.print(month());
      client.print("\">");
      client.print(" / <input type=\"text\"<input maxlength=\"4\" size=\"4\" name=\"TimeYear\"value =\"");
      client.print(year());
      client.print("\">");
//      if (Error7 == true) client.print(ErrMsg); else client.print(" (dd/mm/yyyy)");
      //Button 2
      client.println("<br><br>");
      client.println("<input type=\"submit\" name =\"SaveBtn5\" value=\"Update Time\">");
      //Button 3
      client.println("&emsp;");
      client.println("<input type=\"submit\" name =\"RefreshBtn\" value=\"Refresh\">");
      
      // Draw line
      client.print("<hr />");        
      // Show last time synch
      client.print("<font size = \"2\">"); 
      client.print("</b>Time last updated ");
      client.print(Lastdd);
      client.print("/");
      if (Lastmm < 10) client.print("0");
      client.print(Lastmm);
      client.print("/");
      client.print(Lastyy);
      client.print(" at ");
      if (LastHH < 10) client.print("0");
      client.print(LastHH);
      client.print(":");
      if (LastMM < 10) client.print("0");
      client.print(LastMM);
      client.print("</font>");
    }
    
    client.println("</body>");
    client.println("</html>");
    // End of Web Page
  }
}


//###############################################################################################
// Save Config Data
// 
//###############################################################################################
void SaveConfig() {
  EEPROM.write( 0,EEPROM_chk);        // EEPROM Check
  EEPROM.write( 1,Mode);              // Mode
  EEPROM.write( 2,OnMode);            // OnMode
  EEPROM.write( 3,OffMode);           // Off Mode
  EEPROM.write( 4,TimerHour);         // On Hour
  EEPROM.write( 5,TimerMinute);       // On minute
  SaveString(DevName,1);              // Device name (110-139)
  EEPROM.commit();
}


//###############################################################################################
// Save Program Data
// 
//###############################################################################################
void SaveProgram() {
  for (byte i = 0; i < 7; i++) {
    byte nr = i;
    byte t1 = 20 + (nr * 12);
    //On Time
    EEPROM.write(t1,On_Time[nr]/100);
    EEPROM.write(t1 +  1,On_Time[nr]%100);
    // Off time
    EEPROM.write(t1 +  2,Off_Time[nr]/100);
    EEPROM.write(t1 +  3,Off_Time[nr]%100);
    // On days
    EEPROM.write(t1 +  4,On_Days[nr][0]);  
    EEPROM.write(t1 +  5,On_Days[nr][1]);  
    EEPROM.write(t1 +  6,On_Days[nr][2]);  
    EEPROM.write(t1 +  7,On_Days[nr][3]);  
    EEPROM.write(t1 +  8,On_Days[nr][4]);  
    EEPROM.write(t1 +  9,On_Days[nr][5]);  
    EEPROM.write(t1 + 10,On_Days[nr][6]);  
    // Spare
    EEPROM.write(t1 + 11,On_Days[nr][7]);  
    EEPROM.commit();
  }
}


//###############################################################################################
// Save NTP Server Data
// 
//###############################################################################################
void SaveNTP() {
  int Tz = (TimeZone * 100);
  Tz = Tz + 1200;
  TimeZoneH = Tz/100;
  TimeZoneM = Tz%100;
  EEPROM.write( 6,TimeZoneH);
  EEPROM.write( 7,TimeZoneM); 
  EEPROM.write( 8,IP_1);
  EEPROM.write( 9,IP_2);
  EEPROM.write(10,IP_3);
  EEPROM.write(11,IP_4);
  EEPROM.commit();
}


//###############################################################################################
// SAVE string to EEPROM
// 
//###############################################################################################
void SaveString(String s1, unsigned int s2) {
  // Adjust string length to 20 characters
  //-----------------------------------------------
  s1.trim();
  // String EEPROM address starts at 110
  //-----------------------------------------------
  s2 = ( (s2 - 1) * 30) + 110;
  byte l = s1.length();
  if (s1.length() > 30) {
    s1.remove(30,l+1);
  }
  while(s1.length() < 30) {
    s1 = s1 + " ";  
  }
  // Save 20 chracters to EEPROM
  //------------------------------------------------
  for (byte i = 0;i < 30;i++) {
    EEPROM.write(s2 + i, s1[i]);
  }
}


//###############################################################################################
// READ program data from EEPROM
// 
//###############################################################################################
void ReadData() {
  // See if EEPROM contains valid data.
  // EEPROM location 0 will contain EEPROM_chk if data is valid
  // If not valid, store defult settings
  if (EEPROM.read(0) != EEPROM_chk) {
    EEPROM.write( 0,EEPROM_chk);  // EEPROM check
    EEPROM.write( 1,0  );         // Mode
    EEPROM.write( 2,0  );         // OnMode
    EEPROM.write( 3,0  );         // OffMode
    EEPROM.write( 4,0  );         // TimerHour
    EEPROM.write( 5,0  );         // TimerMinute
    EEPROM.write( 6,12 );         // Time Zone Hour
    EEPROM.write( 7,0  );         // TimZone Minute
    EEPROM.write( 8,129);         // NTP IP 1
    EEPROM.write( 9,6  );         // NTP IP 2
    EEPROM.write(10,15 );         // NTP IP 3
    EEPROM.write(11,28 );         // NTP IP 4
    // Clear all programs
    for (byte i = 20; i < 105; i++) {  
      EEPROM.write(i,0);
    }
    // Save default name
    SaveString(DefaultName,1);
    EEPROM.commit();
  }
  // Read programs
  for (byte i = 0; i < 7; i++) {
    byte nr = i;
    byte t1 = 20 + ( (i) * 12);
    //On Time
    //---------------------
    On_Time[nr]  = EEPROM.read(t1);
    On_Time[nr] = On_Time[nr] * 100;
    On_Time[nr] = On_Time[nr] + EEPROM.read(t1 + 1);
    // Off time
    //---------------------
    Off_Time[nr]  = EEPROM.read(t1 + 2);
    Off_Time[nr] = Off_Time[nr] * 100;
    Off_Time[nr] = Off_Time[nr] + EEPROM.read(t1 + 3);
    // On days
    //---------------------
    On_Days[nr][0] = EEPROM.read(t1 +  4);  
    On_Days[nr][1] = EEPROM.read(t1 +  5);  
    On_Days[nr][2] = EEPROM.read(t1 +  6);  
    On_Days[nr][3] = EEPROM.read(t1 +  7);  
    On_Days[nr][4] = EEPROM.read(t1 +  8);  
    On_Days[nr][5] = EEPROM.read(t1 +  9);  
    On_Days[nr][6] = EEPROM.read(t1 + 10);  
    // Spare
    //---------------------
    On_Days[nr][7] = EEPROM.read(t1 + 11);  
  }
  Mode          = EEPROM.read(1);
  OnMode        = EEPROM.read(2);
  OffMode       = EEPROM.read(3);
  TimerHour     = EEPROM.read(4);
  TimerMinute   = EEPROM.read(5);
  TimeZoneH     = EEPROM.read(6);
  TimeZoneM     = EEPROM.read(7);
  IP_1          = EEPROM.read(8);
  IP_2          = EEPROM.read(9);
  IP_3          = EEPROM.read(10);
  IP_4          = EEPROM.read(11);
  // Setup timeServer IP
  timeServer[0] = IP_1;
  timeServer[1] = IP_2;
  timeServer[2] = IP_3;
  timeServer[3] = IP_4;
  DevName = ReadString(1);
  // Assemble Timezone value
  TimeZone = TimeZoneM;
  TimeZone = TimeZone / 100;
  TimeZone = TimeZone + TimeZoneH;
  TimeZone = TimeZone -12;
  
  
}


//###############################################################################################
// READ string from EEPROM
// 
//###############################################################################################
String ReadString(int s1) {
  String s2 = "";
  // String EEPROM address starts at 110
  //-----------------------------------------------
  s1 = ( (s1 - 1) * 30) + 110;
    // Read 30 characters
  //-----------------------------------------------
  for (byte i = 0;i < 30;i++) {
    s2 = s2 + char(EEPROM.read(s1 + i));
  }
  s2.trim();
  return s2;
}


//###############################################################################################
// NTP Code - do not change
//
//###############################################################################################
const int NTP_PACKET_SIZE = 48;                 // NTP time is in the first 48 bytes of message
byte      packetBuffer[NTP_PACKET_SIZE];        //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  while (Udp.parsePacket() > 0) ;               // discard any previously received packets
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 5000) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      if (TimeOk ==false) {
        TimeOk = true;
      }
      TimeCheck   = true;
      return secsSince1900 - 2208988800UL + TimeZone * SECS_PER_HOUR;
    }
  }
  return 0; // return 0 if unable to get the time
}


//###############################################################################################
// send an NTP request to the time server at the given address
//
//###############################################################################################
void sendNTPpacket(IPAddress & address) {
  // set all bytes in the buffer to 0
  //------------------------------------------------
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  //------------------------------------------------
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;            // Stratum, or type of clock
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;         // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  //------------------------------------------------
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  //NTP requests are to port 123
  //------------------------------------------------
  Udp.beginPacket(address, 123); 
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}


//###############################################################################################
// callback notifying us of the need to save config
// 
//###############################################################################################
void saveConfigCallback () {
  shouldSaveConfig = true;
}


//###############################################################################################
// Start WiFi
// 
//###############################################################################################
void StartWiFi() {
  //read configuration from FS json
  //----------------------------------------------------------------------
  if (SPIFFS.begin()) {
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          //strcpy(mqtt_server, json["mqtt_server"]);
          //strcpy(mqtt_port, json["mqtt_port"]);
          //strcpy(blynk_token, json["blynk_token"]);
          if(json["ip"]) {
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
          }
        } 
      }
    }
  } 
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  //----------------------------------------------------------------------
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  //----------------------------------------------------------------------
  WiFiManager wifiManager;
  //set config save notify callback
  //----------------------------------------------------------------------
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  //set static ip
  //----------------------------------------------------------------------
  IPAddress _ip,_gw,_sn;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  //reset settings
  //----------------------------------------------------------------------
  if (ResetWiFi == true) {
    wifiManager.resetSettings();
    ResetWiFi == false;
  }
  //set minimum quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //----------------------------------------------------------------------
  wifiManager.setMinimumSignalQuality();
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep in seconds
  //----------------------------------------------------------------------
  wifiManager.setTimeout(120);
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "IoT Timer", password = password
  //and goes into a blocking loop awaiting configuration
  //----------------------------------------------------------------------
  if (!wifiManager.autoConnect(DefaultName)) {
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
  //save the custom parameters to FS
  //----------------------------------------------------------------------
  if (shouldSaveConfig) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["ip"] = WiFi.localIP().toString();
    json["gateway"] = WiFi.gatewayIP().toString();
    json["subnet"] = WiFi.subnetMask().toString();
    File configFile = SPIFFS.open("/config.json", "w");
    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  //if you get here you have connected to the WiFi
  // Read settings from EEPROM
  //----------------------------------------------------------------------
  EEPROM.begin(512);
  ReadData();    
  // Setup NTP time requests
  //----------------------------------------------------------------------
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(NTPfastReq);
  // Begin IoT server
  //----------------------------------------------------------------------
  server.begin();
}


//###############################################################################################
// Check if WiFi reset button pressed after power up
// 
//###############################################################################################
void CheckReset() {
  
  old_ms = millis();
  // 10 sec delay to allow user to configure Network 
  //----------------------------------------------------------------------
  while ( ((millis() - old_ms) < 10000 ) and (ResetWiFi == false) ) {
    //Wait for button to be pressed
    //----------------------------------------------------------------------
    digitalWrite(LED,HIGH);
    delay(100);
    digitalWrite(LED,LOW);
    delay(100);
    if (digitalRead(Button) == 0) {
      // Reset WiFi
      old_ms = millis();
      ResetWiFi = true;
      //Wait for button to be released
      //----------------------------------------------------------------------
      while (digitalRead(Button) == 0) {
        digitalWrite(LED,LOW);
      }
    }
  }
  digitalWrite(LED,HIGH);
}


//###############################################################################################
// Scan for NTP Time changes
// 
//###############################################################################################
void CheckNTPtime() {

  // This line needed to keep NTP Time Synch active
  //------------------------------------------------
  timeNow = (10000 * hour()) + (minute() * 100) + second();

  // See if NTP time was set
  //------------------------
  if ( (TimeOk == true) and (NTPtimeOk == false) and (TimeCheck == true) ){
      setSyncInterval(NTPslowReq);
      NTPtimeOk = true;
  }
  // See if NTP Time was updated
  //----------------------------
  if (TimeCheck == true) {
    LastHH = hour();
    LastMM = minute();
    Lastdd = day();
    Lastmm = month();
    Lastyy = year();
    TimeCheck = false;
  }
}


//###############################################################################################
// Scan for Button presses
// 
//###############################################################################################
void ScanButton() {
  if ( (digitalRead(Button) == 0) or (WebButton == true) ) {
    WebButton= false;
    // button pressed, change status of LED
    digitalWrite(LED,!digitalRead(LED));
    // Check manual OFF
    if ( (ManualOn == false) and (ManualTime == false) ) {
      if (digitalRead(Relay) == 1) {
        if (ManualOff == false) {
          if (OffMode == 1) {
            ManualOff = true;
          }
        }
      }
      else {
        if (ManualOff == true) {
            ManualOff = false;
        }
     }
    }
    // Check manual ON
    if ( (ManualOff == false) and (ManualTime == false) ) {
      if (digitalRead(Relay) == 0) {
        if (ManualOn == false) {
          if (OnMode == 1) {
            ManualOn = true;
          }
        }
      }
      else {
        if (ManualOn == true) {
            ManualOn = false;
        }
     }
    }
    // Check manual Timed
    if ( (ManualOff == false) and (ManualOn == false) ) {
      if (digitalRead(Relay) == 0) {
        if (ManualTime == false) {
          if (OnMode == 2) {
            ManualTime = true;
            ManualSec = (TimerHour * 3600) + (TimerMinute * 60);
            old_sec = second();
          }
        }
      }
      else {
        if (ManualTime == true) {
            ManualTime = false;
            ManualSec = 0;
        }
     }
    }
    // wait for button to be released
    while(digitalRead(Button) == 0) {
      delay(200);
    }
    // button relesed, change status of LED
    digitalWrite(LED,!digitalRead(LED));
    timeOld = 0;
  }
}


//###############################################################################################
// Update LED Status
// 
//###############################################################################################
void UpdateLED() {
  //LED Status
  if (TimeOk == false) {
    if ( (millis() - old_ms) > 3000) {
      digitalWrite(LED,HIGH);
      delay(50);
      digitalWrite(LED,LOW);
      old_ms - millis();
    }
  }
  else {
    digitalWrite(LED,digitalRead(Relay));
  }
}


//###############################################################################################
// See if time has changed and update output according to programs
// 
//###############################################################################################
void DoTimeCheck() {
  // Is mode = Off
  if (Mode == 0) {
    digitalWrite(Relay,LOW);
    // clear button overrides
    ManualOff  = false;
    ManualOn   = false;
    ManualTime = false;
    return;
  }
  // Is mode = On
  if (Mode == 1) {
    digitalWrite(Relay,HIGH);
    // clear button overrides
    ManualOff  = false;
    ManualOn   = false;
    ManualTime = false;
    return;
  }
  // Is mode invalid
  if (Mode != 2) {
    digitalWrite(Relay,LOW);
    return;
  }
  // Mode = 2, check Manual On Timer
  if (ManualTime == true) {
    if ( (second() != old_sec) ){
      old_sec = second();
      if (ManualSec > 0) ManualSec = ManualSec - 1;
      if (ManualSec == 0) {
        ManualTime = false; 
        timeOld = 0;
      }
    }
  }
  // Mode = 2, See if time changed
  timeNow = (100 * hour()) + minute();
  if (timeOld != timeNow) {
    // Time changed - check outputs
    boolean Output = false;
    timeOld = timeNow;
    for (byte i = 0; i < 7; i++) {
      // See if relay can be controlled
      if (TimeOk != false) {
        //Time Ok, check if output must be on
        // See if Ontime < OffTime (same day)
        if (On_Time[i] < Off_Time[i]) {
          if ( (timeNow >= On_Time[i]) and (timeNow < Off_Time[i]) ) {
            // See if current day is selected
            if (On_Days[i][weekday() - 1] == true) {
              Output = true;; 
            }
          }
        }
        // See if Ontime > OffTime (over two days)
        if (On_Time[i] > Off_Time[i]) {
          if ( (timeNow < Off_Time[i]) or (timeNow>= On_Time[i]) ) {
            int PrevDay = weekday() - 2;
            if (PrevDay < 0) PrevDay = 6;
            // Check current day
            if (timeNow >= On_Time[i]) {
              if (On_Days[i][weekday() - 1] == true) {
                Output = true;
              }
            }
            // Check previous day
            if (timeNow < Off_Time[i]) {
              if (On_Days[i][PrevDay] == true) {
                 Output = true;
              }
            }
          }
        } 
      }
    }
    // Check manual off 
    if ( (ManualOn == false) and (ManualTime == false) ) {
      if (ManualOff == true) {
        if (Output == true) {
          Output = false;
        }
        else {
          ManualOff = false;
        }
      }
    }
    // Check manual on 
    if ( (ManualOff == false) and (ManualTime == false) ) {
      if (ManualOn == true) {
        if (Output == false) {
          Output = true;
        }
        else {
          ManualOn = false;
        }
      }
    }
    // Check manual time 
    if ( (ManualOff == false) and (ManualOn == false) ) {
      if (ManualTime == true) { 
        if (Output == false) {
          Output = true;
        }
        else {
          ManualTime = false;
        }
      }
    }
    // Set output
    if (Output == true) digitalWrite(Relay,HIGH); else digitalWrite(Relay,LOW);
  } 
}

