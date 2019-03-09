#include <TFT_eSPI.h>            // LCD Screen Hardware driver
#include <TFT_eFEX.h>             // Include the extension graphics functions library
#include <TFT_eFX.h>
#define FS_NO_GLOBALS
#include <FS.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <JsonListener.h>
#include <OpenWeatherMapCurrent.h>
#include <OpenWeatherMapForecast.h>
#include <Astronomy.h>
#include <MiniGrafx.h>
#include <Carousel.h>
#include "ArialRounded.h"
#include "moonphases.h"
#include "weathericons.h"
#include "settings.h"
#include <ArduinoJson.h>
#include "mybase64.h"
#include <string.h>
#define MINI_BLACK 0
#define MINI_WHITE 1
#define MINI_YELLOW 2
#define MINI_BLUE 3
#define MAX_FORECASTS 5//12
TFT_eSPI tft = TFT_eSPI();       // Create object "tft"

TFT_eFEX  fex = TFT_eFEX(&tft);    // Create TFT_eFEX object "fex" with pointer to "tft" object

TFT_eSprite spr = TFT_eSprite(&tft);
TFT_eFEX  sfex = TFT_eFEX(&spr);
// defines the colors usable in the paletted 16 color frame buffer
uint16_t palette[] = {TFT_BLACK, // 0
                      TFT_WHITE, // 1
                      TFT_YELLOW, // 2
                      0x0F21
                      }; //3
OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
simpleDSTadjust dstAdjusted(StartRule, EndRule);
Astronomy::MoonData moonData;
String getTime(time_t *timestamp);
const char* getMeteoconIconFromProgmem(String iconText);
const char* getMiniMeteoconIconFromProgmem(String iconText);
void drawProgress(uint8_t percentage, String text);
void drawWifiQuality();
void updateData();
void drawCurrentWeather();
void drawTime();
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex);
void drawAstronomy();
void drawCurrentWeatherDetail();
void drawForecast1(int16_t x, int16_t y);
void drawForecast2(MiniGrafx *display, CarouselState* state, int16_t x, int16_t y);
void drawForecast3(MiniGrafx *display, CarouselState* state, int16_t x, int16_t y);
void drawBmp(const char *filename, int16_t x, int16_t y,bool z) ;
void drawPCStatus();
long lastDownloadUpdate = millis();
String moonAgeImage = "";
uint8_t moonAge = 0;
uint16_t screen = 0;
bool insys = 0;
bool weathermod = 1;//初始化为天气模式
bool disconnectflag = 1;
bool musicflag = 0;
time_t dstOffset = 0;
// drawPCStatus
float cpuload,gpuload,ramuse,downlink,uplink;
int CPUF,CPUT,CPUU,GPUF,GPUT,RAMU,MBF,RAMA;
float GPUU;
TFT_eSprite dial   = TFT_eSprite(&tft); // Sprite object for dial
TFT_eSprite needle = TFT_eSprite(&tft); // Sprite object for needle

uint32_t startMillis;

int16_t angle = 0;
//is the spectrum
const int BARNUMBER = 16*2;
const int MAXBARVAL = 40;
const int BARSPACING = 7;
const int DISPLAYWIDTH = 470;
const int DISPLAYHEIGHT = 200;
const int SCREENWIDTH = 480;
const int SCREENHEIGHT = 320;
const int BARWIDTH = (DISPLAYWIDTH - BARSPACING * (BARNUMBER - 1)) / BARNUMBER;
const int LINESPACING = DISPLAYHEIGHT / MAXBARVAL;
const int ORIGINY = SCREENHEIGHT;
const int ORIGINX = (SCREENWIDTH - (BARWIDTH * BARNUMBER + BARSPACING * (BARNUMBER - 1))) / 2;
///value from com
int counter = 0;
int value = 0;
String sdata;
String musicstr;
//================ display color constants ==========

#define SPECTRUMCOUNT 16
const int PALETTECOUNT = 4;
const double THRESHOLD[PALETTECOUNT] = {1, 0.7, 0.5, 0};
const int TAILBRIGHTNESSOFFSET = 3;

//================ display variables ================

double barValue[BARNUMBER];
double prevBarValue[BARNUMBER];
uint16_t palettes[BARNUMBER];
uint16_t prevPalette[BARNUMBER];
uint16_t paletteColor[PALETTECOUNT][MAXBARVAL];

//================ tail trace constants =============

const int TRACETIME = 10;
const double TRACEFALLSPEED = 1;
const uint16_t TRACECOLOR = 0xFFFF;

//================ tail trace variables =============

double traceHeight[BARNUMBER];
double prevTraceHeight[BARNUMBER];
int traceTimer[BARNUMBER];
//================ define display palette ===========

/* Make palette for display columns based on amplitude
 * thresholds. For example, a column with max amplitude
 * is displayed in red while small amplitude columns are
 * displayed in blue.
 */
void makePalette() {
  uint16_t red;
  uint16_t green;
  uint16_t blue;
  for (int p = 0; p < PALETTECOUNT; p++) {
    int maxJ = MAXBARVAL * THRESHOLD[max(0, p-1)];
    red = 0;
    green = 0;
    blue = 0;
    for (int j = 0; j < maxJ; j++) {
      if (p == 0 || p == 1) {
        red = map(j, 0, maxJ + TAILBRIGHTNESSOFFSET, 0x1F, 0);
      }
      if (p == 1 || p == 2) {
        green = map(j, 0, maxJ + TAILBRIGHTNESSOFFSET, 0x3F, 0);
      }
      if (p == 3) {
        blue = map(j, 0, maxJ + TAILBRIGHTNESSOFFSET, 0x1F, 0);
      }
      paletteColor[p][j] = (red << 11) + (green << 5) + blue;
    }
  }
}
//================ retrieve signal and update =======

/* Retrieve frequency amplitude readings from MSGEQ7 and
 * update column height.
 */
void updateValues(int index, int value) {
  /*
  for (int i = 0; i < SPECTRUMCOUNT; i++) {
    prevBarValue[i] = barValue[i];
    barValue[i] = map(random(0,254), 0, 254, 0, MAXBARVAL);
    barValue[i] = (floor(barValue[i]) > MAXBARVAL) ? (double) MAXBARVAL : barValue[i];
    barValue[i] = (floor(barValue[i]) < 0) ? 0 : barValue[i];
  }
  */
    prevBarValue[index] = barValue[index];
    barValue[index] = map(value, 0, 254, 0, MAXBARVAL);
    barValue[index] = (floor(barValue[index]) > MAXBARVAL) ? (double) MAXBARVAL : barValue[index];
    barValue[index] = (floor(barValue[index]) < 0) ? 0 : barValue[index];
}
//================ check amplitude threshold ========

/* Helper function for color updater function.*/
int checkThreshold(double barValue) {
  for (int p = 0; p < PALETTECOUNT; p++) {
    if (barValue >= THRESHOLD[p] * MAXBARVAL) {
      return p;
    }
  }
  return -1;
}

//================ change column color ==============

/* Update the color of each column if needed.
 */
void changeColor() {
  for (int i = 0; i < BARNUMBER; i++) {
    prevPalette[i] = palettes[i];
    palettes[i] = checkThreshold(barValue[i]);
  }
}

//================ update current display ===========

/* Update the current display to reflect new readings.
 */
void updateDisplay() {
  for (int i = 0; i < BARNUMBER; i++) {
    int prev = floor(prevBarValue[i]);
    int val = floor(barValue[i]);
    int x = ORIGINX + i * (BARSPACING + BARWIDTH);
    if (prevPalette[i] != palettes[i]) {
      for (int j = 0; j < prev; j++) {
        int y = ORIGINY - j * LINESPACING;
        tft.drawFastHLine(x, y, BARWIDTH, paletteColor[palettes[i]][j]);
      }
      prevPalette[i] = palettes[i];
    }
    if (val < prev) {
      for (int j = prev; j >= val; j--) {
        int y = ORIGINY - j * LINESPACING;
        tft.drawFastHLine(x, y, BARWIDTH, TFT_BLACK);
      }
    } else if (val > prev) {
      for (int j = prev; j < val; j++) {
        int y = ORIGINY - j * LINESPACING;
        tft.drawFastHLine(x, y, BARWIDTH, paletteColor[palettes[i]][j]);
      }
    }
  }
}


//================ draw tail trace ==================

/* Update and redraw tail trace to reflect recent max
 * value.
 */
void updateTrace() {
  for (int i = 0; i < BARNUMBER; i++) {
    int val = floor(barValue[i]);
    int x = ORIGINX + i * (BARSPACING + BARWIDTH);
    if (floor(traceHeight[i]) <= val * LINESPACING + 1) {
      traceHeight[i] = val * LINESPACING + 1;
      traceTimer[i] = TRACETIME;
    }
    if (traceTimer[i] > 0) {
      traceTimer[i]--;
    } else {
      traceHeight[i] -= TRACEFALLSPEED;
    }
    if (floor(traceHeight[i]) < floor(prevTraceHeight[i]) || (floor(traceHeight[i]) > floor(prevTraceHeight[i]) && (int) floor(prevTraceHeight[i]) % LINESPACING != 0)) {
      tft.drawFastHLine(x, ORIGINY - floor(prevTraceHeight[i]), BARWIDTH, TFT_BLACK);
    }
    tft.drawFastHLine(x, ORIGINY - floor(traceHeight[i]), BARWIDTH, TRACECOLOR);
    prevTraceHeight[i] = traceHeight[i];
  }
}



// =======================================================================================
// Create the dial sprite, the dial outer and place scale markers
// =======================================================================================

void createDialScale(int16_t start_angle, int16_t end_angle, int16_t increment)
{
  // Create the dial Sprite
  dial.setColorDepth(8);       // Size is odd (i.e. 91) so there is a centre pixel at 45,45
  dial.createSprite(91, 91);   // 8bpp requires 91 * 91 = 8281 bytes
  dial.setPivot(45, 45);       // set pivot in middle of dial Sprite

  // Draw dial outline
  dial.fillSprite(TFT_TRANSPARENT);           // Fill with transparent colour
  dial.fillCircle(45, 45, 43, TFT_DARKGREY);  // Draw dial outer

  // Hijack the use of the needle Sprite since that has not been used yet!
  needle.createSprite(3, 3);     // 3 pixels wide, 3 high
  needle.fillSprite(TFT_WHITE);  // Fill with white
  needle.setPivot(1, 43);        //  Set pivot point x to the Sprite centre and y to marker radius

  for (int16_t angle = start_angle; angle <= end_angle; angle += increment) {
    needle.pushRotated(&dial, angle); // Sprite is used to make scale markers
    yield(); // Avoid a watchdog time-out
  }

  needle.deleteSprite(); // Delete the hijacked Sprite
}


// =======================================================================================
// Add the empty dial face with label and value
// =======================================================================================

void drawEmptyDial(String label, String val)
{
  // Draw black face
  dial.fillCircle(45, 45, 40, TFT_BLACK);
  dial.drawPixel(45, 45, TFT_WHITE);        // For demo only, mark pivot point with a while pixel

  dial.setTextDatum(TC_DATUM);              // Draw dial text
  dial.drawString(label, 45, 15, 2);
  dial.drawString(val, 43, 60, 2);
}

// =======================================================================================
// Update the dial and plot to screen with needle at defined angle
// =======================================================================================

void plotDial(int16_t x, int16_t y, int16_t angle, String label, String val)
{
  // Draw the blank dial in the Sprite, add label and number
  drawEmptyDial(label, val);

  // Push a rotated needle Sprite to the dial Sprite, with black as transparent colour
  needle.pushRotated(&dial, angle, TFT_BLACK); // dial is the destination Sprite

  // Push the resultant dial Sprite to the screen, with transparent colour
  dial.pushSprite(x, y, TFT_TRANSPARENT);
}

// =======================================================================================
// Create the needle Sprite and the image of the needle
// =======================================================================================

void createNeedle(void)
{
  needle.setColorDepth(8);
  needle.createSprite(11, 49); // create the needle Sprite 11 pixels wide by 49 high

  needle.fillSprite(TFT_BLACK);          // Fill with black

  // Define needle pivot point
  uint16_t piv_x = needle.width() / 2;   // x pivot of Sprite (middle)
  uint16_t piv_y = needle.height() - 10; // y pivot of Sprite (10 pixels from bottom)
  needle.setPivot(piv_x, piv_y);         // Set pivot point in this Sprite

  // Draw the red needle with a yellow tip
  // Keep needle tip 1 pixel inside dial circle to avoid leaving stray pixels
  needle.fillRect(piv_x - 1, 2, 3, piv_y + 8, USE_YELLOW);
  needle.fillRect(piv_x - 1, 2, 3, 5,USE_YELLOW);

  // Draw needle centre boss
  needle.fillCircle(piv_x, piv_y, 5, TFT_MAROON);
  needle.drawPixel( piv_x, piv_y, TFT_WHITE);     // Mark needle pivot point with a white pixel
}
// =======================================================================================
// Loop
// =======================================================================================
void connectWifi() {
  WiFiManager wifiManager;
  drawProgress(50,"连接WiFi");
  wifiManager.autoConnect("ESPTEST");
  if (WiFi.status() == WL_CONNECTED) return;
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    drawProgress(i,"连接WiFi");
    delay(500);
    if (i>99) {
      i=0;
      drawProgress(i,"resetSettings");
      wifiManager.resetSettings();
      }
    i+=1;
    
    Serial.print(".");
  }
  drawProgress(100,"成功");
  Serial.print("Connected...");
}
void setup() {
  // put your setup code here, to run once:
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Load a smooth font from SPIFFS
  
  Serial.begin(230400); 
  Serial.setTimeout(5);
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  //tft.println("连接wifi.....");
  WiFiManager wifiManager;
  delay(100); 
  bool isFSMounted = SPIFFS.begin();
  tft.loadFont("yahei16");
  connectWifi();
  updateData();
  Serial.println("Mounting file system...");
  if (!isFSMounted) {
    Serial.println("file system error");
    drawProgress(50,"file system error");
  }
  drawProgress(100,"file system done");
  tft.fillScreen(TFT_BLACK);
  createDialScale(-120, 120, 30);   // create scale (start angle, end angle, increment angle)
  createNeedle();                // draw the needle graphic
  drawWifiQuality();
  makePalette();
  //insys = 1;
}
void loop() {
    
    //JsonArray
  while(Serial.available()) {
     sdata = Serial.readString();
  }
  //解码计算机json数据
  const size_t capacity = JSON_OBJECT_SIZE(14) + 260;
  //const size_t capacity = 1024;
  DynamicJsonBuffer jsonBuffer(capacity);
  //const char* json = "{\"CPUF\":739,\"CPUT\":44,\"CPUU\":25,\"GPUF\":1101,\"GPUT\":41,\"GPUU\":11.4,\"RAMU\":25,\"music\":\"AAAAAAAAAAAAAAAAAAAAAA==\"}";
  JsonObject& root = jsonBuffer.parseObject(sdata);
  CPUF = root["CPUF"]; // 739
  CPUT = root["CPUT"]; // 44
  CPUU = root["CPUU"]; // 25
  GPUF = root["GPUF"]; // 1101
  GPUT = root["GPUT"]; // 41
  GPUU = root["GPUU"]; // 11.4
  RAMU = root["RAMU"]; // 25
  RAMA = root["RAMA"];
  MBF = root["MBF"]; // 903
  uplink = root["uplink"];
  downlink = root["downlink"];
  const char* music = root["music"]; // "AAAAAAAAAAAAAAAAAAAAAA=="
  const char* pctime = root["time"]; // "23:26:07"
  const char* diskusage = root["diskusage"]; // "C:\\64.00%D:\\15.92%E:\\40.63%"

  

  
  if (!root.success()&&disconnectflag) { 
    // 未连接计算机
    drawCurrentWeather();  
    drawTime();  
    if(!insys)
    {
    drawForecast1(0,20);    
      for(int i = 0;i < 24 ; i++)
      {
        String tempicon = "";
        tempicon = "/moonphase_L" + String(i) + ".bmp";
        const char * c = tempicon.c_str();
        drawBmp(c,350, 55,0);
        delay(50);
      }
      drawAstronomy();
    }
    if(!weathermod){
      tft.fillScreen(TFT_BLACK);
      drawCurrentWeather();  
      drawTime();  
      drawAstronomy();
      drawForecast1(0,20); 
      weathermod = 1;
    }
    
    if (millis() - lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS) {
      lastDownloadUpdate = millis();
      drawWifiQuality();
      drawForecast1(0,20);
      updateData();
      drawAstronomy();
      yield();
     } 
    insys = 1;
    tft.setCursor(50,460);
    if(disconnectflag)
    tft.println("PC not Connect!");
    yield(); 
  }
  //仪表和音乐
  else{
    if(weathermod)  
    {
     tft.fillScreen(TFT_BLACK);
     weathermod = 0;
    }
    musicstr = music;  
    int var; 
    if (musicstr == "off")  var = 1;
    else {
      if (musicstr == "disconnect")  var = 2;
      else
      {
         var = 3;
      }
    }
    
    switch (var){
      case 1:  //off
        if(disconnectflag){  
            tft.setCursor(50,460);
            tft.fillScreen(TFT_BLACK);
            tft.fillRect(0,41,480,2,TFT_WHITE);
        }
        if(musicflag){
            tft.fillScreen(TFT_BLACK);
            tft.fillRect(0,41,480,2,TFT_WHITE);
            createDialScale(-120, 120, 30);   // create scale (start angle, end angle, increment angle)
            createNeedle();                // draw the needle graphic
        }
        musicflag = 0;
        disconnectflag = 0;
        //写计算机数据
        float diskinfo[10];
        int disknum;
        drawPCStatus();
        //time
        spr.createSprite(100, 40);
        spr.fillSprite(TFT_BLACK);
        spr.setTextDatum(TC_DATUM); 
        spr.setTextColor(TFT_WHITE, TFT_BLACK);
        spr.drawString(pctime,50,10,4);
        spr.pushSprite(175,0);
        spr.deleteSprite();
        //about disk information
        spr.createSprite(150, 300);
        spr.setColorDepth(1);
        spr.fillSprite(TFT_BLACK);
        disknum = strlen(diskusage)/9;
        for(int i = 0;i < disknum ; i++)
        {
          String temp = diskusage;
          String temp2;
          temp2 = temp.substring(9*i, 9*i+1);
          temp = temp.substring(9*i+3, 9*i+7);
          spr.setTextDatum(TL_DATUM); 
          spr.setTextColor(TFT_WHITE, TFT_BLACK);
          spr.drawString(temp2+"://"+temp+"%",0,i*40,2);
          sfex.drawProgressBar(10,i*40+20,130,10,temp.toFloat(),TFT_BLUE,TFT_GREEN);
        }
        spr.pushSprite(320,50);
        spr.deleteSprite();
        //about download speed
        spr.createSprite(300, 75);
        spr.fillSprite(TFT_BLACK);
        spr.setTextDatum(TL_DATUM);
        spr.setTextColor(TFT_WHITE, TFT_BLACK);
        spr.drawString("MB Fan:    " + String(MBF)+"RPM",10,0,2);
        spr.drawString("UPLINK:    " + String(uplink) + "MB/S",10,25,2);//上载
        spr.drawString("DOWNLINK:  " + String(downlink) + "MB/S",10,50,2);//下载
        spr.drawString("RAMUSE:    " + String(RAMU) + "%",160,0,2);//下载
        spr.drawString("RAMLEFT:   " + String(RAMA) + "GB",160,25,2);//下载
        spr.pushSprite(20,250);
        spr.unloadFont();
        spr.deleteSprite();
        musicflag = 0;
        break;

      case 2: // dis
        disconnectflag = 1;
        musicflag = 0;
        sdata = "";
        tft.fillRect(0,350,320,130,TFT_BLACK); //清除音乐的图
        tft.setCursor(50,460);
        break;
      case 3: //music on
        disconnectflag = 0;
        if(!musicflag){
          tft.fillScreen(TFT_BLACK);
          tft.drawString("---MUSIC ON---",175,6); //音乐模式
         for(uint clo =0;clo<32;clo++){
          updateValues(clo,0x00);
          }
          
        }
        musicflag = 1;
        char decoded[BARNUMBER];
        String ss(musicstr);
        //解码音频数据
        b64_decode( decoded , (char *)ss.c_str() , ss.length() );
        for(uint clo =0;clo<BARNUMBER;clo++){
        updateValues(clo,decoded[clo]);
        }
        changeColor();
        updateDisplay();//spectrum start
        updateTrace();
        yield(); // Avoid a watchdog time-out      
        break;
    }
    
  }  
yield(); 
}
// Progress bar helper
// Progress bar helper
void drawProgress(uint8_t percentage, String text) {
  spr.createSprite(100,20);
  spr.fillSprite(TFT_BLACK);
  spr.loadFont("yahei16");
  spr.drawString(text,0,0);
  if(!insys) {  
  fex.drawProgressBar(50,100,220,20,percentage,TFT_BLUE,TFT_DARKGREEN);
  spr.pushSprite(50,80);
  } 
  spr.unloadFont();
  spr.deleteSprite();
}
// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if(dbm <= -100) {
      return 0;
  } else if(dbm >= -50) {
      return 100;
  } else {
      return 2 * (dbm + 100);
  }
}

void drawWifiQuality() {
  spr.createSprite(20, 20);
  spr.fillSprite(TFT_BLACK);
  int8_t quality = getWifiQuality();
  tft.drawString(String(quality) + "%",400, 6);
  for (int8_t i = 0; i < 4; i++) {
    for (int8_t j = 0; j < 2 * (i + 1); j++) {
      if (quality > i * 25 || j == 0) {
        spr.drawPixel(2 * i, 18 - j,TFT_WHITE);
      }
    }
  }
  spr.pushSprite(450, 2);
  spr.deleteSprite();
}
// Update the internet based information and update screen
void updateData() {

  if(!insys) {
  drawProgress(10, "更新中....");
  //fex.drawProgressBar(50,100,220,20,100,TFT_BLUE,TFT_BLACK);
  }
  
  configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
  while(!time(nullptr)) {
    Serial.print("#");
    yield(); 
  }
  // calculate for time calculation how much the dst class adds.
  dstOffset = UTC_OFFSET * 3600 + dstAdjusted.time(nullptr) - time(nullptr);
  Serial.printf("Time difference for DST: %d\n", dstOffset);

  if(!insys) drawProgress(50, "更新天气");
  OpenWeatherMapCurrent *currentWeatherClient = new OpenWeatherMapCurrent();
  currentWeatherClient->setMetric(IS_METRIC);
  currentWeatherClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  currentWeatherClient->updateCurrentById(&currentWeather, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
  delete currentWeatherClient;
  currentWeatherClient = nullptr;

  if(!insys) drawProgress(70, "更新天气....");
  OpenWeatherMapForecast *forecastClient = new OpenWeatherMapForecast();
  forecastClient->setMetric(IS_METRIC);
  forecastClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {12, 0};
  forecastClient->setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient->updateForecastsById(forecasts, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);
  delete forecastClient;
  forecastClient = nullptr;
  yield(); 
  if(!insys) drawProgress(80, "更新天气.......");
  Astronomy *astronomy = new Astronomy();
  moonData = astronomy->calculateMoonData(time(nullptr));
  float lunarMonth = 29.53;
  moonAge = moonData.phase <= 4 ? lunarMonth * moonData.illumination / 2 : lunarMonth - moonData.illumination * lunarMonth / 2;
  moonAgeImage = String((char) (65 + ((uint8_t) ((26 * moonAge / 30) % 26))));
  delete astronomy;
  astronomy = nullptr;
  yield(); 
  if(!insys)  drawProgress(100, "Done!                  ");
}
void drawCurrentWeather() {
  drawBmp(getBigMeteoconIconFromProgmem(currentWeather.icon), 55 , 55 , 1);
  spr.createSprite(100, 54);
  spr.fillSprite(TFT_BLACK);
  // Weather Text
  spr.loadFont("yahei16");
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.drawString(currentWeather.cityName,0, 0);
  spr.drawString(String(currentWeather.temp, 1) + (IS_METRIC ? "°C" : "°F"),0, 13);
  spr.drawString(currentWeather.description,0, 53);
  spr.pushSprite(160,80);
  spr.unloadFont();
  spr.deleteSprite();
}
// draws the clock
void drawTime() {
  spr.createSprite(120, 40);
  spr.setColorDepth(1);
  spr.fillSprite(TFT_BLACK);
  spr.loadFont("yahei16");
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  char time_str[11];
  char *dstAbbrev;
  time_t now = dstAdjusted.time(&dstAbbrev);
  struct tm * timeinfo = localtime (&now);

  String date = ctime(&now);
  date = date.substring(0,11) + String(1900 + timeinfo->tm_year);
  spr.drawString(date,0, 0);
  if (IS_STYLE_12HR) {
    int hour = (timeinfo->tm_hour+11)%12+1;  // take care of noon and midnight
    sprintf(time_str, "%2d:%02d:%02d\n",hour, timeinfo->tm_min, timeinfo->tm_sec);
    spr.drawString( time_str,0, 14);
  } else {
    sprintf(time_str, "%02d:%02d:%02d\n",timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    spr.drawString(time_str,0, 14);
  }

  if (IS_STYLE_12HR) {
    sprintf(time_str, "%s\n%s", dstAbbrev, timeinfo->tm_hour>=12?"PM":"AM");
    spr.drawString(time_str,75, 14);
  } else {
    sprintf(time_str, "%s", dstAbbrev);
    spr.drawString(time_str,75, 14);  // Known bug: Cuts off 4th character of timezone abbreviation
  }
  spr.pushSprite(165,6);
  spr.unloadFont();
  spr.deleteSprite();
}
// helper for the forecast columns
void drawForecast1(int16_t x, int16_t y) {
  drawForecastDetail(x + 10, y + 165, 0);
  drawForecastDetail(x + 115, y + 165, 1);
  drawForecastDetail(x + 220, y + 165, 2);
}
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex) {
  time_t time = forecasts[dayIndex].observationTime + dstOffset;
  struct tm * timeinfo = localtime (&time);
  tft.drawString( WDAY_NAMES[timeinfo->tm_wday] + " " + String(timeinfo->tm_hour) + ":00",x + 15, y - 15);

  tft.drawString(String(forecasts[dayIndex].temp, 1) + (IS_METRIC ? "°C" : "°F"),x + 15, y);
   drawBmp(getMeteoconIconFromProgmem(forecasts[dayIndex].icon), x+15 , y + 15,1);
  //gfx.drawPalettedBitmapFromPgm(x, y + 15, getMiniMeteoconIconFromProgmem(forecasts[dayIndex].icon));
  tft.drawString(String(forecasts[dayIndex].rain, 1) + (IS_METRIC ? "mm" : "in"),x + 15, y + 60);
}
// draw moonphase and sunrise/set and moonrise/set
void drawAstronomy() {
  String tempicon = "";
  tempicon = "/moonphase_L" + String(moonAge) + ".bmp";
  const char * c = tempicon.c_str();
  drawBmp(c,350, 55,0);
  tft.drawString("Sun",370, 170);
  time_t time = currentWeather.sunrise + dstOffset;
  tft.drawString("Rise:",360-15, 170+15);
  tft.drawString(getTime(&time),360+40, 170+15);
  time = currentWeather.sunset + dstOffset;
  tft.drawString("Set:",360-15, 170+30);
  tft.drawString(getTime(&time),360+40, 170+30);

  tft.drawString("Moon ",360, 170+45);
  tft.drawString("Age: ",360-15, 170+60);
  tft.drawString(String(moonAge) + "d",360+40, 170+60);
  tft.drawString("Illum: ",360-15, 170+75);
  tft.drawString(String(moonData.illumination * 100, 0) + "%",360+40, 170+75);
}
String getTime(time_t *timestamp) {
  struct tm *timeInfo = gmtime(timestamp);
  
  char buf[6];
  sprintf(buf, "%02d:%02d", timeInfo->tm_hour, timeInfo->tm_min);
  return String(buf);
}
// Bodmers BMP image rendering function
// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
void drawBmp(const char *filename, int16_t x, int16_t y, bool z) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  fs::File bmpFS;

  // Open requested file on SD card
  bmpFS = SPIFFS.open(filename, "r");

  if (!bmpFS)
  {
    Serial.print("File not found");
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row;
  uint8_t  r, g, b;

  uint32_t startTime = millis();

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {
        
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t*  bptr = lineBuffer;
        uint16_t* tptr = (uint16_t*)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        if(z)  tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer,TFT_BLACK);
        else
        {
          tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);/* code */
        }
         
      }
      Serial.print("Loaded in "); Serial.print(millis() - startTime);
      Serial.println(" ms");
    }
    else Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
}
void drawPCStatus(){
  
    int xpos = 15;
    int ypos = 50;
    String temp1,temp2,temp3;
    gpuload = -120+GPUU*2.4;
    cpuload = -120+CPUU*2.4;
    int cpufan = -120+(CPUF/2000)*2.4;
    int cputemp = -120+(CPUT/120)*2.4;
    ramuse = -120+RAMU*2.4;
    temp1 = String(CPUU) + "%";
    temp2 = String(CPUT) + "C";
    temp3 = String(CPUF) + "RPM";
    plotDial(xpos, ypos, cpuload, "CPU", temp1);
    plotDial(xpos+100, ypos, cputemp, "CPU", temp2);
    plotDial(xpos+200, ypos, cpufan, "CPU",temp3);
    int gpufan = -120+(CPUF/4000)*2.4;
    int gputemp = -120+(CPUT/120)*2.4;
    temp1 = String(GPUU) + "%";
    temp2 = String(GPUT) + "C";
    temp3 = String(GPUF) + "RPM";
    plotDial(xpos, ypos+100, gpuload, "GPU", temp1);
    plotDial(xpos+100, ypos+100, gputemp, "GPU", temp2);
    plotDial(xpos+200, ypos+100, gpufan, "GPU",temp3);
}
