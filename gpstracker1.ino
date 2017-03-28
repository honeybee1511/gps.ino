
#include <AuthClient.h>
#include <debug.h>
#include <MicroGear.h>
#include <MQTTClient.h>
#include <PubSubClient.h>
#include <SHA1.h>

#include <LGPS.h>   
#include <LBattery.h>  


#include <LGPRS.h>
#include <LGPRSClient.h>

#include <LWiFi.h>
#include <LWiFiClient.h>

#include <LEEPROM.h> 

#include <MicroGear.h>


#define WIFI_AP "JinPoKo"       
#define WIFI_PASSWORD "iloveu1234"

#define WIFI_AUTH LWIFI_WPA  // choose from LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP.


#define APPID   "Homework1"
#define KEY     "UrNj4R8R6k4JfkT"
#define SECRET  "1CvZtlRKsJWoWnLEqTjUJ3V4btwNUe"
#define ALIAS   "linkitone"
#define SCOPE       ""

//#define GPRSMODE    //use with sim card

#ifdef GPRSMODE
  LGPRSClient client;
#else
  LWiFiClient client;
#endif

AuthClient *authclient;

gpsSentenceInfoStruct info;
char buff[256];

char battery[10];

int timer = 0;
MicroGear microgear(client);

static unsigned char getComma(unsigned char num,const char *str)
{
  unsigned char i,j = 0;
  int len=strlen(str);
  for(i = 0;i < len;i ++)
  {
     if(str[i] == ',')
      j++;
     if(j == num)
      return i + 1; 
  }
  return 0; 
}

static double getDoubleNumber(const char *s)
{
  char buf[10];
  unsigned char i;
  double rev;
  
  i=getComma(1, s);
  i = i - 1;
  strncpy(buf, s, i);
  buf[i] = 0;
  rev=atof(buf);
  return rev; 
}

static double getIntNumber(const char *s)
{
  char buf[10];
  unsigned char i;
  double rev;
  
  i=getComma(1, s);
  i = i - 1;
  strncpy(buf, s, i);
  buf[i] = 0;
  rev=atoi(buf);
  return rev; 
}

void parseGPGGA(const char* GPGGAstr)
{
  /* Refer to http://www.gpsinformation.org/dale/nmea.htm#GGA
   * Sample data: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
   * Where:
   *  GGA          Global Positioning System Fix Data
   *  123519       Fix taken at 12:35:19 UTC
   *  4807.038,N   Latitude 48 deg 07.038' N
   *  01131.000,E  Longitude 11 deg 31.000' E
   *  1            Fix quality: 0 = invalid
   *                            1 = GPS fix (SPS)
   *                            2 = DGPS fix
   *                            3 = PPS fix
   *                            4 = Real Time Kinematic
   *                            5 = Float RTK
   *                            6 = estimated (dead reckoning) (2.3 feature)
   *                            7 = Manual input mode
   *                            8 = Simulation mode
   *  08           Number of satellites being tracked
   *  0.9          Horizontal dilution of position
   *  545.4,M      Altitude, Meters, above mean sea level
   *  46.9,M       Height of geoid (mean sea level) above WGS84
   *                   ellipsoid
   *  (empty field) time in seconds since last DGPS update
   *  (empty field) DGPS station ID number
   *  *47          the checksum data, always begins with *
   */
  double latitude;
  double longitude;
  int tmp, hour, minute, second, num ;
  if(GPGGAstr[0] == '$')
  {
    tmp = getComma(1, GPGGAstr);
    hour     = (GPGGAstr[tmp + 0] - '0') * 10 + (GPGGAstr[tmp + 1] - '0');
    minute   = (GPGGAstr[tmp + 2] - '0') * 10 + (GPGGAstr[tmp + 3] - '0');
    second    = (GPGGAstr[tmp + 4] - '0') * 10 + (GPGGAstr[tmp + 5] - '0');
    
    sprintf(buff, "UTC timer %2d-%2d-%2d", hour, minute, second);
    Serial.println(buff);
    
    tmp = getComma(2, GPGGAstr);
    latitude = getDoubleNumber(&GPGGAstr[tmp]);
    tmp = getComma(4, GPGGAstr);
    longitude = getDoubleNumber(&GPGGAstr[tmp]);
    sprintf(buff, "latitude = %10.4f, longitude = %10.4f", latitude, longitude);   
    Serial.println(buff); 
    
    tmp = getComma(7, GPGGAstr);
    num = getIntNumber(&GPGGAstr[tmp]);    
    sprintf(buff, "satellites number = %d", num);
    Serial.println(buff); 
  }
  else
  {
    Serial.println("Not get data"); 
  }
}
void waitForGPSFix() {     
  byte i = 0;
  Serial.print("Getting GPS fix...");
  while (info.GPGGA[43] != '1') { // 1 indicates a good fix
    LGPS.getData( &info );
    delay(250);
    // toggle the red LED during GPS init...
    #ifdef GPSPIN
      digitalWrite(GPSPIN, i++ == 0 ? LOW : HIGH);  
    #endif
    i = (i > 1 ? 0 : i);
  }
  Serial.println("GPS locked.");
  #ifdef GPSPIN
    digitalWrite(GPSPIN, LOW);
  #endif
}
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
    Serial.print("Incoming message --> ");
    msg[msglen] = '\0';
    Serial.println((char *)msg);
}

void onFoundgear(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.print("Found new member --> ");
    for (int i=0; i<msglen; i++)
        Serial.print((char)msg[i]);
    Serial.println();  
}

void onLostgear(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.print("Lost member --> ");
    for (int i=0; i<msglen; i++)
        Serial.print((char)msg[i]);
    Serial.println();
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.println("Connected to NETPIE...");
    microgear.setName("mygear");
}


void setup() {
    /* Event listener */
    microgear.on(MESSAGE,onMsghandler);
    microgear.on(PRESENT,onFoundgear);
    microgear.on(ABSENT,onLostgear);
    microgear.on(CONNECTED,onConnected);

    Serial.begin(115200);
    Serial.println("Starting...");

    LGPS.powerOn();

    Serial.println("LGPS Power on, and waiting ..."); 
    delay(1000);
    
    #ifdef GPRSMODE
        Serial.println("Attach to GPRS network by auto-detect APN setting");
        while (!LGPRS.attachGPRS("internet","","")) {
          delay(500);
        }

        Serial.println("GPRS connected");  
    #else
      LWiFi.begin();
      Serial.println("Starting WIFI...");

        // keep retrying until connected to AP
        Serial.println("Connecting to AP");
        while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD))) {
          delay(1000);
        }
  
        Serial.println("WiFi connected");  
    #endif

    microgear.init(KEY,SECRET,ALIAS);
    microgear.connect(APPID);
    waitForGPSFix();   //fix your GPS
}

void loop() {
    if (microgear.connected()) {
        Serial.println("connected");
        microgear.loop();
        if (timer >= 1000) {

            LGPS.getData(&info);
            Serial.println((char*)info.GPGGA); 

            parseGPGGA((const char*)info.GPGGA);
            microgear.publish("/linkitone/gps",(char*)info.GPGGA);  

            sprintf(battery,"%d", LBattery.level() );
            microgear.publish("/linkitone/battery",battery);

            Serial.println(battery);
            
          
            timer = 0;
        } 
        else timer += 100;
    }
    else {
        Serial.println("connection lost, reconnect...");
        if (timer >= 5000) {
            microgear.connect(APPID);
            timer = 0;
        }
        else timer += 100;
    }

    
    
    delay(100);
}

