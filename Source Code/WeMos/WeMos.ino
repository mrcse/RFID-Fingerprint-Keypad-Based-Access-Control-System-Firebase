#include <TimeLib.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>


// Set these to run example.
#define FIREBASE_HOST "messapp-1d0b2.firebaseio.com"
#define FIREBASE_AUTH "ZK8FdlvgSaecO3bvj9XlbUeEA0xaRae2Ghg40FUU"
#define WIFI_SSID "AR"
#define WIFI_PASSWORD "bhai1998"


String name_1="Jamshid Ali";
String id_1="62bf471c";

String name_2="Abdul Hadi";
String id_2="e144f01a";

String name_3="User 1";
String id_3="61c87f1c";

String name_4="Engr Sumeyya";
String id_4="027c521b";

char card[8];

String Time="";
// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";

const int timeZone = 5;     // Islamabad +5

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void digitalClockDisplay();

void sendNTPpacket(IPAddress &address);

void setup()
{

  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);      //This initialize the client with the given firebase host and credentials.
  Udp.begin(localPort);
  Serial.print("Local port: ");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
}
  time_t prevDisplay = 0; // when the digital clock was displayed

void loop() {
  
                                        // TIME SITTEING
  Time =String(hour())+ ":" + Jdart(minute()) + ":" + Jdart(second());
  
  Serial.println(Time);
  
                                        //  FIREBASE DATA
  //##############################################################################################################################
  Serial.read(card,8);
  Serial.println("After Read  ->  ");
  Serial.println(card);
// Serial.print(strcmpi(card,id_1));
  if(strcmpi(card,id_1)){
    Firebase.pushString("User/18PWCSE1654/Name", name_1);
    Firebase.pushString("User/18PWCSE1654/Time",Time );
  }
  else if(strcmpi(card,id_2)){
    Firebase.pushString("User/18PWCSE1624/Name", name_2);
    Firebase.pushString("User/18PWCSE1624/Time",Time );
  }
  else if(strcmpi(card,id_3)){
    Firebase.pushString("User/18PWCSE1604/Name", name_3);
    Firebase.pushString("User/18PWCSE1604/Time",Time );
  }
    else if(strcmpi(card,id_4)){
    Firebase.pushString("User/18PWCSE1600/Name", name_4);
    Firebase.pushString("User/18PWCSE1600/Time",Time );
  }

  
  if (Firebase.failed()) {
      Serial.print("Test Failed:");
      Serial.println(Firebase.error());  
      return;
  }
  for(int i=0;i<8;i++)
  {
    card[i]=' ';
  }
  delay(500);
}

String Jdart(int digits)
{
  String Time;
  if(digits<10)
    Time="0" + String(digits);
   else 
   Time=String(digits);
   return Time;
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
bool strcmpi(char ch[],String str)
{
  bool flag;
  int i=0;
  for(i=0;i<8;i++)
  {
    if(ch[i]==str[i]);
    else
    break;
  }
  if(i==8)
  flag=true;
  else
  flag=false;
  return flag;
}
