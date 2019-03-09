  
#include <string.h>
// Helper function, should be part of the weather station library and should disappear soon
char s[20]; 
const char* getMeteoconIconFromProgmem(String iconText) {
  if (iconText == "01d" || iconText == "01n") strcpy(s,"/clear.bmp");
  if (iconText == "02d" || iconText == "02n") strcpy(s,"/partlysunny.bmp");
  if (iconText == "03d" || iconText == "03n") strcpy(s,"/partlycloudy.bmp");
  if (iconText == "04d" || iconText == "04n") strcpy(s,"/mostlycloudy.bmp");
  if (iconText == "09d" || iconText == "09n") strcpy(s,"/rain.bmp");
  if (iconText == "10d" || iconText == "10n") strcpy(s,"/rain.bmp");
  if (iconText == "11d" || iconText == "11n") strcpy(s,"/tstorms.bmp");
  if (iconText == "13d" || iconText == "13n") strcpy(s, "/snow.bmp");
  if (iconText == "50d" || iconText == "50n") strcpy(s,"/fog.bmp");
  //strcpy(s,"/unknown.bmp");
  return s;
}

const char* getBigMeteoconIconFromProgmem(String iconText) {
  if (iconText == "01d" || iconText == "01n") strcpy(s,"/big/clear.bmp");
  if (iconText == "02d" || iconText == "02n") strcpy(s,"/big/partlysunny.bmp");
  if (iconText == "03d" || iconText == "03n") strcpy(s,"/big/partlycloudy.bmp");
  if (iconText == "04d" || iconText == "04n") strcpy(s,"/big/mostlycloudy.bmp");
  if (iconText == "09d" || iconText == "09n") strcpy(s,"/big/rain.bmp");
  if (iconText == "10d" || iconText == "10n") strcpy(s,"/big/rain.bmp");
  if (iconText == "11d" || iconText == "11n") strcpy(s,"/big/tstorms.bmp");
  if (iconText == "13d" || iconText == "13n") strcpy(s, "/big/snow.bmp");
  if (iconText == "50d" || iconText == "50n") strcpy(s,"/big/fog.bmp");
  //strcpy(s,"/unknown.bmp");
  return s;
}






