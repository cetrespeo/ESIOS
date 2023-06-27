// Importante, si falla con el GxEPD, instalar esto https://github.com/lewisxhe/GxEPD
// LIBS: ESP32 board lib MAX 1.04; GxEPD from Lewisxhe; Adafruit_GFX max 1.11; ESPAsyncWebSrv 1.2.6, FirebaseEsp32 (Mobitz) 3.8.8

#define LILYGO_T5_V213

#ifndef LILYGO_T5_V213
#define LILYGO_T5_V266
#endif

#include <boards.h>
#include <GxEPD.h>
#include <GxFont_GFX.h>
#ifdef LILYGO_T5_V213
#include <GxDEPG0213BN/GxDEPG0213BN.h>    // 2.13" b/w  form DKE GROUP
bool v213 = true;
#endif
#ifdef LILYGO_T5_V266
#include <GxDEPG0266BN/GxDEPG0266BN.h>    // 2.66" b/w  form DKE GROUP
bool v213 = false;
#endif
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <WiFiClientSecure.h>
#include <rom/rtc.h>
#include <FS.h>
#include <esp_wifi.h>
#include <SPIFFS.h>
#include <Update.h>
#include <ESPAsyncWebSrv.h>
#include <FirebaseESP32.h>            // avoid >3.8.8.
#define FIREBASE_HOST "weatheresp32.firebaseio.com"
#define FIREBASE_AUTH "htFuCm2OfTXtP4g5xrQBpSaE0IHYEOrSLoVmSiBF"

AsyncWebServer server(80);
GxIO_Class io(SPI,  EPD_CS, EPD_DC,  EPD_RSET);
GxEPD_Class display(io, EPD_RSET, EPD_BUSY);

String sTimeZone = "CET-1CEST,M3.5.0,M10.5.0/3", sMACADDR, sArrSSID[10], sArrPASS[10] , sRefreshText, sUpdateFirmware = "";
bool bWeHaveWifi = false, bSPIFFSExists = false, bResetBtnPressed;
int32_t tNow = 0, iSPIFFSWifiSSIDs , iScreenXMax , iScreenYMax, iTimeDiffPerDay;
float fPriceHour[24], fScreenXMax , fScreenYMax, fCaptOMIE = -100;
int iMode = -3, iAnalogRead, iVBAT , iMaxVolt = 2200;
const float fCurv[] = { 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.10, 0.23, 0.34, 0.43, 0.46, 0.43, 0.34, 0.23, 0.10, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.04, 0.19, 0.34, 0.48, 0.58, 0.61, 0.58, 0.48, 0.34, 0.19, 0.04, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.11, 0.26, 0.42, 0.55, 0.64, 0.67, 0.64, 0.55, 0.42, 0.26, 0.11, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.11, 0.26, 0.42, 0.55, 0.64, 0.67, 0.64, 0.55, 0.42, 0.26, 0.11, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.06, 0.19, 0.35, 0.50, 0.63, 0.72, 0.75, 0.72, 0.63, 0.50, 0.35, 0.19, 0.06, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.13, 0.28, 0.44, 0.60, 0.74, 0.83, 0.86, 0.83, 0.74, 0.60, 0.44, 0.28, 0.13, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.03, 0.16, 0.31, 0.47, 0.63, 0.76, 0.85, 0.88, 0.85, 0.76, 0.63, 0.47, 0.31, 0.16, 0.03, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.02, 0.16, 0.33, 0.51, 0.69, 0.83, 0.93, 0.97, 0.93, 0.83, 0.69, 0.51, 0.33, 0.16, 0.02, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.09, 0.25, 0.43, 0.60, 0.74, 0.84, 0.88, 0.84, 0.74, 0.60, 0.43, 0.25, 0.09, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.02, 0.16, 0.32, 0.49, 0.63, 0.73, 0.76, 0.73, 0.63, 0.49, 0.32, 0.16, 0.02, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.06, 0.20, 0.35, 0.49, 0.58, 0.61, 0.58, 0.49, 0.35, 0.20, 0.06, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.06, 0.20, 0.35, 0.49, 0.58, 0.61, 0.58, 0.49, 0.35, 0.20, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.11, 0.24, 0.35, 0.43, 0.46, 0.43, 0.35, 0.24, 0.11, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.08, 0.20, 0.31, 0.38, 0.41, 0.38, 0.31, 0.20, 0.08, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00};
FirebaseData firebaseData;
FirebaseJson jData, jVars;

const String sVer = "1.04";

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  int i;
  iAnalogRead = 0;
  for (i = 0; i < 50; i++) {
    delay(10);
    if (i > 0) iAnalogRead += analogRead(35);
  }
  iAnalogRead = iAnalogRead / i ;
  sMACADDR = getMacAddress();
  Serial.begin(115200);
  Serial.printf("\n--------------------------------------------------\n ESIOS - OMIE display v%s %s\n--------------------------------------------------\n", sVer.c_str(), v213 ? "2.13" : "2.66");
  SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI);
  display.init();
  delay(50);
  String sAux;
  sRefreshText = "";
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    bSPIFFSExists = false;
    SendToSleep(600);
  } else bSPIFFSExists = true;
  delay(50);
  pinMode(39, INPUT);
  bLoadWifi();
  bLoadTime();
  iTimeDiffPerDay =  iLoadInt("/timediff");
  iMode =  iLoadInt("/mode");
  iMaxVolt =  iLoadInt("/maxvolt");
  if ((iAnalogRead * 0.95f) > iMaxVolt) {
    iMaxVolt = iAnalogRead * 0.95f;
    bSaveInt("/maxvolt", iMaxVolt);
  }
  if ((iMaxVolt < 1000) || (iMaxVolt > 3000)) iMaxVolt = 2000;
  iVBAT = (((float)(iAnalogRead ) * 100.0f / (float)(iMaxVolt)) - 85.0f) * 100 / 15 ;
  if (iVBAT > 100) iVBAT = 100;
  if (iVBAT < 0) iVBAT = 0;
  if (iMode < 0) {
    bSaveMode(1);
    iMode = 1;
  }
  Serial.printf(" tNow=%s, Diff=%d, Mode=%d, Volt= %d%% with %d/%d, MAC=%s. \n", sTimetoStr(tNow).c_str(), iTimeDiffPerDay, iMode, iVBAT, iAnalogRead, iMaxVolt, sMACADDR.c_str());
  String sRESETREASON = sGetResetReason();
  bResetBtnPressed = (sRESETREASON == "PON") || (sRESETREASON == "RTC");
  Serial.println("--------------------------------------------------\n Reset due to " + sRESETREASON + " @" + sTimetoStr(tNow))  ;
  if (bResetBtnPressed) {
    Serial.println("****** bResetBtnPressed ********");
    delay(1000);
    if (!digitalRead(39)) {
      Serial.println("**************** BUTTON 39 PRESSED ******************");
      int iCount = 0;
      do {
        delay(100);
        iCount++;
      } while (!digitalRead(39) && (iCount < 31));
      Serial.println("**************** BUTTON 39 RELEASED******************" + (String)(iCount));
      if (iCount < 30) {
        iMode = iMode + 1 ;
        if (iMode > 3) iMode = 0;
        bSaveMode(iMode);
        sRefreshText = "New mode =" + (String)(iMode);
        Serial.println("Mode changed to [" + (String)(iMode) + "] @" + sTimetoStr(tNow));
      } else {
        Serial.println(" ENTERING SETUP! ");
        //Enter SETUP
        StartWifiAP();
        StartWebServer();
      }
    } else {
      Serial.println(" Force WifiLoad @" + sTimetoStr(tNow));
      if (!bWeHaveWifi) StartWiFi(10);
    }
  }
  if (bWeHaveWifi) {
    bManageFirebase();
  }
  bGetData(tNow);
}
//**************************************************************************************************************************
void loop() {
  RefreshGraph();
  if (WiFi.status() == WL_CONNECTED)  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  bWeHaveWifi = false;
  display.powerDown();
  tNow += (millis() / 1000);
  int iSleepSecs = 3600 + 15 - (tNow % 3600) - (iTimeDiffPerDay / 24);
  if (iSleepSecs < 180)  iSleepSecs = iSleepSecs + 3600;
  if (hour(tNow) == 23) {
    int iRnd = random(1, 100) * 10;
    tNow += iRnd;
    iSleepSecs += iRnd;
  }
  SendToSleep(iSleepSecs);
}
//**************************************************************************************************************************
bool bManageFirebase() {
  String sAux, sUpdateFirmware;
  if (!bWeHaveWifi) return false;
  Serial.print(" Firebase:");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(firebaseData, 1000 * 40);
  Firebase.setwriteSizeLimit(firebaseData, "medium");
  Firebase.setMaxRetry(firebaseData, 3);
  Firebase.setMaxErrorQueue(firebaseData, 30);
  sAux = "/IoT/dev/" + sMACADDR + "/LastOn";
  Firebase.setString(firebaseData, sAux, sTimetoStr(tNow));
  sAux = "/IoT/dev/" + sMACADDR + "/Ver";
  Firebase.setString(firebaseData, sAux, sVer + " @" + (v213 ? "2.13" : "2.66") + " Mode:" + (String)(iMode));
  sAux = "/IoT/dev/" + sMACADDR + "/TimeDiff";
  Firebase.setInt(firebaseData, sAux, iTimeDiffPerDay);
  sAux = "/IoT/dev/" + sMACADDR + "/VBat";
  Firebase.setInt(firebaseData, sAux, iVBAT);
  sAux = "/IoT/dev/" + sMACADDR + "/VBatMax";
  Firebase.setInt(firebaseData, sAux, iMaxVolt);
  sAux = "/IoT/dev/" + sMACADDR + "/Update";
  Firebase.get(firebaseData, sAux);
  sUpdateFirmware = firebaseData.stringData();
  sUpdateFirmware.trim();
  sUpdateFirmware.toLowerCase();
  if (sUpdateFirmware.length() == 0) Firebase.setString(firebaseData, "/IoT/dev/" + sMACADDR + "/Update", "-");
  if (sUpdateFirmware.length() > 4) {
    if (sUpdateFirmware.substring(sUpdateFirmware.length() - 4) == ".bin") {
      Serial.printf(" UPDATE COMMAND RECEIVED %s\n", sUpdateFirmware.c_str());
      Firebase.setString(firebaseData, sAux, "Started '" + sUpdateFirmware + "' from " + sVer + " @" + sTimetoStr(tNow));
      if (execOTA("omie/" + sUpdateFirmware)) Firebase.setString(firebaseData, sAux, "Updated '" + sUpdateFirmware + "' from " + sVer + " @" + sTimetoStr(tNow));
      else Firebase.setString(firebaseData, sAux, "Failed '" + sUpdateFirmware + "' from " + sVer + " @" + sTimetoStr(tNow));
    }
  }
  sAux = "/IoT/dev/" + sMACADDR + "/Command";
  Firebase.get(firebaseData, sAux);
  sAux = firebaseData.stringData();
  sAux.trim();
  sAux.toLowerCase();
  if (sAux.length() == 0) Firebase.setString(firebaseData, "/IoT/dev/" + sMACADDR + "/Command", "-");
  if ((sAux == "erase") || (sAux == "format")) {
    Firebase.setString(firebaseData, "/IoT/dev/" + sMACADDR + "/Command", "_Formatted @" + sTimetoStr(tNow));
    SPIFFS.format();
    SendToSleep(0);
  }
  Serial.print(" FB_Done.\n");
  return true;
}  //**************************************************************************************************************************
#define VBATW 11
#define CAPT_HX 0.69f
#define CAPT_HY 0.55f

void RefreshGraph() {
  int i, iPos1, iPos2, iSum1 = 0, iSum2 = 0, iData, iDataNum, iDataDec;
  float fMax = -100, fMin = 1000, iGAPGx1 , iGAPGx2 , iGAPX , iGAPY , iSTEPY , fGraphY;
  bool bD1 = false, bD2 = false;
  String sAux;
  for (i = 1; i < 24; i++) {
    iSum1 = iSum1 + fPriceHour[i];
  }
  bD1 = (iSum1 > 0);
  if (!bD1) {
    Serial.println("\n NEW LOAD DB=0\n");
    bGetData(tNow);
    for (i = 0; i < 24; i++) {
      iSum1 = iSum1 + fPriceHour[i];
    }
    bD1 = (iSum1 > 0);
  }
  for (i = 0; i < 24; i++) {
    //Serial.printf("\n i=%d Data=%f <%f,%f>", i, fPriceHour[i], fMin, fMax);
    if (bD1) {
      if (fPriceHour[i] > fMax) fMax = fPriceHour[i];
      if (fPriceHour[i] < fMin) fMin = fPriceHour[i];
    }
  }
  iData = 100 * fPriceHour[hour(tNow)];
  //iData = 8426;
  iDataNum = iData / 100;
  iDataDec = iData % 100;
  //Modes
  Serial.printf(" Display mode=%d Hour=%d Data=%f <%f,%f>\n", iMode, hour(tNow), fPriceHour[hour(tNow)], fMin, fMax);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  if (!bD1) sRefreshText = sRefreshText + " NO DATA";
  if (sRefreshText.length() > 0) {
    display.setRotation(1);
    DisplayTextAligned(display.width() / 2 , display.height() - 10, sRefreshText, 0, 10);
    sRefreshText = "";
    display.update();
  }
  int iMillIn = millis();
  if (bD1) {
    if (iMode < 2) { //////////// VERTICAL MODE /////////////////////////////////////////
      // Now
      display.setRotation(0);
      iScreenXMax = display.width() ;
      iScreenYMax = display.height();
      fScreenXMax = (float)(display.width()) / 104.0f;
      fScreenYMax = (float)(display.height()) / 212.0f;
      iGAPGx1 = 5.0 * fScreenXMax;
      iGAPGx2 = 30.0 * fScreenXMax;
      iGAPX = 35.0 * fScreenXMax;
      iGAPY = 46.0 * fScreenYMax;
      iSTEPY = 6.4 * fScreenYMax;

      // VBAT
      if (iVBAT > 1) {
        display.fillRect(3, iGAPY - 2, VBATW - 6, 2, GxEPD_BLACK);
        display.fillRect(1, iGAPY - 1, VBATW - 2,  iScreenYMax * 0.07f, GxEPD_BLACK);
        display.fillRect(2, iGAPY + 0, VBATW - 4,  iScreenYMax * 0.07f - 2 , GxEPD_WHITE);
        display.fillRect(3, iGAPY + 1 + ((iScreenYMax * 0.07f - 4) * (100 - iVBAT) / 100), VBATW - 6, ((iScreenYMax * 0.07f - 4)*iVBAT / 100) , GxEPD_BLACK);
        DisplayTextAligned(5 + VBATW , (iGAPY - 2), "eur/MWh", 1, 10);
        display.setFont(&FreeMono9pt7b);
        if (iMode % 2) {
          DisplayTextAligned(3 + VBATW , (iGAPY + 0.06f * iScreenYMax), "esios", 1, 10);
        } else {
          DisplayTextAligned(3 + VBATW , (iGAPY + 0.06f * iScreenYMax), "omie", 1, 10);
        }
      } else {
        display.setFont(&FreeMonoBold9pt7b);
        DisplayTextAligned(1 , (iGAPY + 0.06f * iScreenYMax), "LOW BATT", 1, 10);
      }

      //Now
      display.setFont(&FreeMonoBold24pt7b);
      if ((iDataNum > 99) || (iDataDec == 0)) {
        DisplayTextAligned(iScreenXMax * 0.5f, fScreenYMax * 32.0f , (String)(iDataNum), 0, 6);
      } else {
        String sDec = "." + (String)(iDataDec);
        if (iDataDec  < 10) sDec = ".0" + (String)(iDataDec);
        if (iDataNum > 9) {
          DisplayTextAligned(iScreenXMax * 0.68f, fScreenYMax * 32.0f , (String)(iDataNum), -1, 5);
          display.setFont(&FreeMonoBold9pt7b);
          DisplayTextAligned(iScreenXMax * 0.7f , fScreenYMax * 32.0f , sDec, 1, 10);
        } else {
          DisplayTextAligned(iScreenXMax * 0.48f, fScreenYMax * 32.0f , (String)(iDataNum), -1, 6);
          display.setFont(&FreeMonoBold9pt7b);
          DisplayTextAligned(iScreenXMax * 0.6f , fScreenYMax * 32.0f , sDec, 1, 10);
        }
      }
      //CAPT
      if ((!(iMode % 2)) && (fCaptOMIE > -100)) {
        display.setFont(&FreeMonoBold18pt7b);
        DisplayTextAligned(iScreenXMax * 0.35f, iScreenYMax * 0.66f , (String)((int)(fCaptOMIE)), 0, 6);
        drawBar(0, iScreenYMax * 0.66f, iScreenXMax, iScreenYMax * 0.3f , 2, GxEPD_WHITE);
      }

      //Bottom
      display.setFont(&FreeMono9pt7b);
      DisplayTextAligned(iScreenXMax * 0.5f, iScreenYMax * 0.98f,  "<D" + int2str2dig(day(tNow)) + "<", 0, 11);
      display.setFont(&FreeMonoBold9pt7b);
      DisplayTextAligned(iScreenXMax * 0.02f, iScreenYMax * 0.98f, float2string(fMin, 0), 1, 12);
      DisplayTextAligned(iScreenXMax , iScreenYMax * 0.98f, float2string(fMax, 0), -1, 12);

      //Hours
      display.fillRect(5, (iGAPY + (hour(tNow) * iSTEPY) + 1), iScreenXMax - 5,  2, GxEPD_BLACK);
      display.setFont(&FreeMonoBold9pt7b);
      for (i = 0; i < 24; i++) {
        if (!(i % 6)) { //hour text
          DisplayTextAligned(iScreenXMax, (iGAPY - 2 + iSTEPY + (i * iSTEPY)), (String)(i) + "h", -1, 12);
        }
        //Line
        iPos1 = iGAPGx1 + ((float)(iScreenXMax -  iGAPGx2 ) * (fPriceHour[i] - fMin)) / (fMax - fMin);
        display.fillRect(iPos1 - 3, (iGAPY + (i * iSTEPY)), 3,  3, GxEPD_BLACK);
        if (i < 23) {
          iPos1 = iGAPGx1 - 3 + ((float)(iScreenXMax - iGAPGx2 ) * (fPriceHour[i] - fMin)) / (fMax - fMin);
          iPos2 = iGAPGx1 - 3 + ((float)(iScreenXMax - iGAPGx2 ) * (fPriceHour[i + 1] - fMin)) / (fMax - fMin);
          drawLine(iPos1 + 1, (iGAPY + (i * iSTEPY) + 1), iPos2 + 1, (iGAPY + iSTEPY + (i * iSTEPY) + 1));
          drawLine(iPos1 + 1, (iGAPY + (i * iSTEPY) + 2), iPos2 + 1, (iGAPY + iSTEPY + (i * iSTEPY) + 2));
        }
      }
      //Box SEL
      iPos1 = iGAPGx1 - 4 + ((float)(iScreenXMax - iGAPGx2) * (fPriceHour[hour(tNow) ] - fMin)) / (fMax - fMin);
      display.fillRect(iPos1 - 1, (iGAPY + (hour(tNow) * iSTEPY) - 1), 6,  6 , GxEPD_BLACK);
      display.fillRect(iPos1 + 1, (iGAPY + (hour(tNow) * iSTEPY) + 1), 2,  2 , GxEPD_WHITE);
    } else { // H O R I Z O N T A L /////////////////////////////////////////////////////
      ///////////////////////////////////////////////////////////////////////////////////
      display.setRotation(0);
      iScreenXMax = display.width() ;
      iScreenYMax = display.height();
      fScreenXMax = (float)(display.width()) / 104.0f;
      fScreenYMax = (float)(display.height()) / 212.0f;
      iGAPGx1 = 5.0 * fScreenXMax;
      iGAPGx2 = 30.0 * fScreenXMax;
      iGAPX = 35.0 * fScreenXMax;
      iGAPY = 80.0 * fScreenYMax;
      iSTEPY = 5.4 * fScreenYMax;
      //Hours
      display.fillRect(5, (iGAPY + (hour(tNow) * iSTEPY) + 1), iScreenXMax - 5,  2, GxEPD_BLACK);
      for (i = 0; i < 24; i++) {
        if (!(i % 6)) { //hour text
          if (i == 0) {
            sAux = "D" + int2str2dig(day(tNow ));
          } else {
            sAux = (String)(i) + "h";
          }
          DisplayTextAligned(iScreenXMax * 0.98f, (iGAPY - 4 + iSTEPY + (i * iSTEPY)), sAux, -1, 12);
        }
      }
      DisplayTextAligned(iScreenXMax * 0.98f, iScreenYMax * 0.97f,  "D" + int2str2dig(day(tNow + 86400)), -1, 12); //Next day
      /////////////////////////////////////////
      // Then rotate texts
      display.setRotation(1);
      iScreenXMax = display.width() ;
      iScreenYMax = display.height();
      fScreenXMax = (float)(display.width()) / 212.0f;
      fScreenYMax = (float)(display.height()) / 104.0f;
      iGAPGx1 = 0.0 * fScreenXMax;
      iGAPGx2 = 15.0 * fScreenXMax;
      iGAPX = 35.0 * fScreenXMax;
      iGAPY = 90.0 * fScreenYMax;
      iSTEPY = 5.0 * fScreenYMax;

      // VBAT
      if (iVBAT > 0) {
        display.fillRect(iScreenXMax * 0.17f + 2, iScreenYMax * 0.83f - 1, VBATW - 6, 2, GxEPD_BLACK);
        display.fillRect(iScreenXMax * 0.17f, iScreenYMax * 0.83f, VBATW - 2,  iScreenYMax * 0.18f, GxEPD_BLACK);
        display.fillRect(iScreenXMax * 0.17f + 1, iScreenYMax * 0.83f + 1, VBATW - 4,  iScreenYMax * 0.16f , GxEPD_WHITE);
        display.fillRect(iScreenXMax * 0.17f + 2 , iScreenYMax * 0.83f + 2 + (v213 ? 0 : 0) + ((iScreenYMax * 0.15f) * (100 - iVBAT) / 100), VBATW - 6, ((iScreenYMax * 0.15f)*iVBAT / 100) - (v213 ? 1 : 0) , GxEPD_BLACK);
        DisplayTextAligned(iScreenXMax * 0.22f, iScreenYMax * 0.83f, "eur/MWh", 1, 10);
        if ((!(iMode % 2)) && (fCaptOMIE > -100)) {
          DisplayTextAligned(iScreenXMax * (CAPT_HX + 0.03f) , iScreenYMax * (CAPT_HY + 0.04f), "solar", 0, 6);
        }
        display.setFont(&FreeMono9pt7b);
        if (iMode % 2) {
          DisplayTextAligned(iScreenXMax * 0.22f , iScreenYMax * 0.98f, "esios", 1, 10);
        } else {
          DisplayTextAligned(iScreenXMax * 0.22f , iScreenYMax * 0.98f, "omie", 1, 10);
        }
      } else {
        display.setFont(&FreeMonoBold9pt7b);
        DisplayTextAligned(iScreenXMax * 0.22f , iScreenYMax * 0.98f, "LOW BATT", 1, 10);
      }

      //Now
      display.setFont(&FreeMonoBold24pt7b);
      if ((iDataNum > 99) || (iDataDec  == 0)) {
        DisplayTextAligned(fScreenXMax * 6.0f, fScreenYMax * 62.0f , (String)(iDataNum), 1, 6);
      } else {
        String sDec = "." + (String)(iDataDec );
        if (iDataDec < 10) sDec = ".0" + (String)(iDataDec );
        if (iDataNum > 9) {
          DisplayTextAligned(fScreenXMax * 25.0f, fScreenYMax * 62.0f , (String)(iDataNum), -1, 5);
          display.setFont(&FreeMonoBold9pt7b);
          DisplayTextAligned(fScreenXMax * 46.0f , fScreenYMax * 62.0f , sDec, 1, 10);
        } else {
          DisplayTextAligned(fScreenXMax * 25.0f, fScreenYMax * 62.0f , (String)(iDataNum), -1, 6);
          display.setFont(&FreeMonoBold9pt7b);
          DisplayTextAligned(fScreenXMax * 46.0f , fScreenYMax * 62.0f , sDec, 1, 10);
        }
      }
      //Limits
      display.setFont(&FreeMonoBold9pt7b);
      DisplayTextAligned(iScreenXMax * 0.15f, iScreenYMax * 0.10f, float2string(fMax, 0), -1, 12);
      DisplayTextAligned(iScreenXMax * 0.15f, iScreenYMax * 0.98f, float2string(fMin, 0), -1, 12);
      //CAPT
      if ((!(iMode % 2)) && (fCaptOMIE > -100)) {
        display.setFont(&FreeMonoBold18pt7b);
        DisplayTextAligned(iScreenXMax * CAPT_HX , iScreenYMax * CAPT_HY , (String)((int)(fCaptOMIE)), 0, 6);
        drawBar(iScreenXMax * (CAPT_HX - 0.26f), iScreenYMax * 0.2f, iScreenXMax * (CAPT_HX + 0.27f), iScreenYMax * (CAPT_HY + 0.01f) , 2, GxEPD_WHITE);
      }
      // HORIZONTAL GRAPH LAST
      // LINES
      display.setRotation(0);
      iScreenXMax = display.width() ;
      iScreenYMax = display.height();
      fScreenXMax = (float)(display.width()) / 104.0f;
      fScreenYMax = (float)(display.height()) / 212.0f;
      iGAPGx1 = 5.0 * fScreenXMax;
      iGAPGx2 = 30.0 * fScreenXMax;
      iGAPX = 35.0 * fScreenXMax;
      iGAPY = 80.0 * fScreenYMax;
      iSTEPY = 5.4 * fScreenYMax;

      // Then GRAPH
      display.fillRect(5, (iGAPY + (hour(tNow) * iSTEPY) + 1), iScreenXMax - 5,  2, GxEPD_BLACK);
      for (i = 0; i < 24; i++) {
        //Line
        iPos1 = iGAPGx1 + ((float)(iScreenXMax -  iGAPGx2 ) * (fPriceHour[i] - fMin)) / (fMax - fMin);
        display.fillRect(iPos1 - 3, (iGAPY + (i * iSTEPY)), 3,  3, GxEPD_BLACK);
        if (i < 23) {
          iPos1 = iGAPGx1 - 3 + ((float)(iScreenXMax - iGAPGx2 ) * (fPriceHour[i] - fMin)) / (fMax - fMin);
          iPos2 = iGAPGx1 - 3 + ((float)(iScreenXMax - iGAPGx2 ) * (fPriceHour[i + 1] - fMin)) / (fMax - fMin);
          drawLine(iPos1 + 1, (iGAPY + (i * iSTEPY) + 1), iPos2 + 1, (iGAPY + iSTEPY + (i * iSTEPY) + 1));
          drawLine(iPos1 + 1, (iGAPY + (i * iSTEPY) + 2), iPos2 + 1, (iGAPY + iSTEPY + (i * iSTEPY) + 2));
        }
      }
      //Box SEL
      iPos1 = iGAPGx1 - 3 + ((float)(iScreenXMax - iGAPGx2) * (fPriceHour[hour(tNow) ] - fMin)) / (fMax - fMin);
      display.fillRect(iPos1 - 1, (iGAPY + (hour(tNow) * iSTEPY) - 1), 6,  6 , GxEPD_BLACK);
      display.fillRect(iPos1 + 1, (iGAPY + (hour(tNow) * iSTEPY) + 1), 2,  2 , GxEPD_WHITE);
    }
    display.update();
  }
  Serial.printf(" Display took %f sec. \n", (float)(millis() - iMillIn) / 1000.0f);
}
//////////////////////////////////////////////////////////////////////////////

String sSSIDWebServerValue, sPASSWebServerValue;

String sWebServerHtml(String sMessage) {
  String sRet;
  sRet = (String)(R"=====(<!DOCTYPE HTML><html><head><title> WIFI SETUP </title><meta name="viewport" content="width=device-width, initial-scale=1"></head><body><h1>Define tu WIFI</h1>)=====");
  if (sSSIDWebServerValue == "") {
    sRet = sRet + (String)(R"=====(<form action="/get">Wifi SSID <input type="text" name="SSID" value=""><input type="submit" value="Update"></form><br>)=====");
  } else {
    sRet = sRet + (String)(R"=====(<form action="/get">Wifi SSID <input type="text" name="SSID" value=")=====") + sSSIDWebServerValue + (String)(R"=====("><input type="submit" value="Update"></form><br>)=====");
  }
  if (sPASSWebServerValue == "") {
    sRet = sRet + (String)(R"=====(<form action="/get">Wifi PASS <input type="text" name="PASS" value=""><input type="submit" value="Update"></form><br>)=====");
  } else {
    sRet = sRet + (String)(R"=====(<form action="/get">Wifi PASS <input type="text" name="PASS" value=")=====") + sPASSWebServerValue + (String)(R"=====("><input type="submit" value="Update"></form><br>)=====");
  }
  sRet = sRet + sMessage + "<br>";
  if (sMessage.length() < 2) {
    if ((sSSIDWebServerValue.length() > 0) && (sPASSWebServerValue.length() > 0)) {
      sRet = sRet + (String)(R"=====(<form action="/get"><p>Puedes guardar SSID=)=====") + sSSIDWebServerValue + " con PASS=" + sPASSWebServerValue + (String)(R"=====(</p><button class="submit" name="SAVE" value="3">SAVE</button></form>)=====");
    } else {
      if (sSSIDWebServerValue.length() > 0) {
        sRet = sRet + "<p>Cargado SSID='" + sSSIDWebServerValue + "'. Falta por actualizar PASS." + "</p><br>";
      }
      if (sPASSWebServerValue.length() > 0) {
        sRet = sRet + "<p>Cargado PASS='" + sPASSWebServerValue + "'. Falta por actualizar SSID." + "</p><br>";
      }
    }
  }
  sRet = sRet + "<br><br><br><p>" + sTimetoStr(tNow) + "</p></body></html>";
  return sRet;
}
//////////////////////////////////////////////////////////////////////////////
bool StartWebServer() {
  sSSIDWebServerValue = "";
  sPASSWebServerValue = "";

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", sWebServerHtml("."));
  });
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    if (request->hasParam("SSID")) {
      sSSIDWebServerValue = request->getParam("SSID")->value();
      sSSIDWebServerValue.trim();
    }
    if (request->hasParam("PASS")) {
      sPASSWebServerValue = request->getParam("PASS")->value();
      sPASSWebServerValue.trim();
    }
    Serial.println("SSID=[" + sSSIDWebServerValue + "] PASS=[" + sPASSWebServerValue + "]");
    if (request->hasParam("SAVE")) {
      if ((sSSIDWebServerValue.length() > 0) && (sPASSWebServerValue.length() > 0)) {
        String sAux = "WIFI " + sSSIDWebServerValue + " SAVED with PASS=" + sPASSWebServerValue + ", REBOOTING...";
        bAddWifi(sSSIDWebServerValue, sPASSWebServerValue);
        sSSIDWebServerValue = "";
        sPASSWebServerValue = "";
        request->send(200, "text/html", sWebServerHtml(sAux));
        Serial.println("Rebooting...");
        delay(10000);
        SendToSleep(0);
        exit;
      }
    } else {
      request->send(200, "text/html", sWebServerHtml(""));
    }
  });
  server.begin();
  Serial.println("HTTP Server Started");
  do {
    delay(10);
  }  while (true);
}
//////////////////////////////////////////////////////////////////////////////


void drawString(int x, int y, String sTr, int iScale, int iStep) {
  int i;
  for (i = 0; i < sTr.length(); i++) {
    display.drawChar(x + (i * iStep), y, sTr.charAt(i), GxEPD_BLACK, GxEPD_WHITE, iScale);
  }
}
///////////////////////////////////////////////////////////////////////////////////////////////
bool StartWiFi(int iRetries) {
  int i = 0, j = 0, iLastWifiNum = -1;
  if (WiFi.status() == WL_CONNECTED)  WiFi.disconnect();
  Serial.print(" |WIFI (" + (String)(iSPIFFSWifiSSIDs) + "):" );
  int iMaxSSIDs = WiFi.scanNetworks();
  if (!iMaxSSIDs) {
    Serial.print(" NO NETWORKS NEARBY ");
    bWeHaveWifi = false;
    return false;
  }
  if (!iSPIFFSWifiSSIDs) {
    Serial.print(" NO NETWORKS LOADED ");
    bWeHaveWifi = false;
    return false;
  }
  for (i = 0; i < iSPIFFSWifiSSIDs; i++) {
    for (j = 0; j < iMaxSSIDs; j++) {
      if (sArrSSID[i] == WiFi.SSID(j)) {
        iLastWifiNum = i;
        break;
      }
    }
    if (iLastWifiNum > -1) break;
  }
  if (iLastWifiNum < 0) {
    bWeHaveWifi = false;
    String sAux = "";
    for (i = 0; i < iSPIFFSWifiSSIDs; i++) sAux = sAux + "," + sArrSSID[i];
    Serial.print(" none of " + sAux );
    bAddWifi("SSID", "PASSWORD");
    return false;
  }
  Serial.print(" found " + sArrSSID[iLastWifiNum] + ":" + sArrPASS[iLastWifiNum] );
  WiFi.begin(sArrSSID[iLastWifiNum].c_str(), sArrPASS[iLastWifiNum].c_str());
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500); Serial.print(".");
    if (j > iRetries) {
      Serial.print(" FAILED!");
      WiFi.disconnect();
      bWeHaveWifi = false;
      return false;
      break;
    }
    j++;
  }
  bWeHaveWifi = true;
  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_STA, &conf);
  String sWifiSsid = String(reinterpret_cast<const char*>(conf.sta.ssid));
  String sWifiPassword = String(reinterpret_cast<const char*>(conf.sta.password));
  String sWifiIP = WiFi.localIP().toString();
  int iWifiRSSI = WiFi.RSSI();
  Serial.print(" @" + sWifiIP + " RSSI:" + (String)(iWifiRSSI) + "dBm ");
  NtpConnect();
  Serial.print("|");
  return true;
}
//////////////////////////////////////////////////////////////////////////////
void NtpConnect() {
  struct tm tmLocal;
  int i = 0;
  configTime( 0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", sTimeZone.c_str(), 1);
  Serial.print(",NTP=");
  while (!getLocalTime(&tmLocal) && (i < 10)) {
    i++;
    Serial.print(".");
    delay(500);
  }
  if (i == 10) {
    tNow = 0;
    Serial.print(" ERROR.");
    return;
  }
  delay(1000);
  int32_t tNowOld = tNow - iTimeDiffPerDay;
  tNow = time(nullptr);
  if (!bResetBtnPressed)  {
    if (tNowOld > 0) iTimeDiffPerDay = tNow - tNowOld;
    else iTimeDiffPerDay = 60;
    if (iTimeDiffPerDay < -240) iTimeDiffPerDay = -240;
    if (iTimeDiffPerDay > 240) iTimeDiffPerDay = 240;
  } else iTimeDiffPerDay = 0;
  char buff[30];
  sprintf(buff, "%04d/%02d/%02d_%02d:%02d:%02d", year(tNow), month(tNow), day(tNow), hour(tNow), minute(tNow), second(tNow));
  Serial.printf(" %s Ok.", buff);
  bSaveInt("/timediff", iTimeDiffPerDay);
}///////////////////////////////////////////////////////////////////////////////////////////////////
void SendToSleep(int iSecs) {
  Serial.printf("--------------------------------------------------\nSent to sleep %d mins (alive %d secs).\n", iSecs / 60, millis() / 1000);
  bSaveTime(tNow + iSecs );
  delay(100);
  SPIFFS.end();
  delay(100);
  uint64_t sleep_time_us = (uint64_t)(iSecs) * 1000000ULL;
  esp_sleep_enable_timer_wakeup(sleep_time_us);
  Serial.print("--------------------------------------------------\n");
  esp_deep_sleep_start();
}
//////////////////////////////////////////////////////////////////////////////
bool bLoadWifi() {
  if (!bSPIFFSExists) {
    Serial.print("\nERROR: No SPIFFS\n");
    return false;
  }
  if (!SPIFFS.exists("/wifi")) {
    Serial.print("\nERROR: No Wifi file\n");
    bAddWifi("SSID", "PASSWORD");
    return true;
  }
  int i, iPos1 = 0, iPos2 = 0;
  String sAux;
  String sWifi = readSPIFFSFile("/wifi");
  iSPIFFSWifiSSIDs = 0;
  //Serial.print("\nWifi Loading: [" + sWifi + "]\n");
  for (i = 0; i < 10; i++) {
    iPos1 = sWifi.indexOf(" ", iPos2);
    sAux = sWifi.substring(iPos2 + 1, iPos1);
    sAux.trim();
    if (!sAux.length()) break;
    sArrSSID[i] = sAux;
    //Serial.print("'" + sAux + "'" + " ");
    iPos2 = sWifi.indexOf("\n", iPos1);
    sAux = sWifi.substring(iPos1 + 1, iPos2);
    sAux.trim();
    if (!sAux.length()) break;
    sArrPASS[i] = sAux;
    iSPIFFSWifiSSIDs = i + 1;
    //Serial.print("'" + sAux + "'" + ",");
  }
  Serial.print(" Wifi SSID loaded #" + (String)(i) + "\n");
  return true;
}
//////////////////////////////////////////////////////////////////////////////
bool bSaveWifi() {
  int i;
  String sAux = "-";
  for (i = 0; i < 10; i++) {
    sAux = sAux + sArrSSID[i] + " " + sArrPASS[i] + "\n";
  }
  writeSPIFFSFile("/wifi", sAux.c_str());
  return true;
}
//////////////////////////////////////////////////////////////////////////////
bool bAddWifi(String sSSID, String sPass) {
  int i;
  bool bUpdated = false;
  sSSID.trim();
  sPass.trim();
  if (sSSID.length() == 0) return false;
  if (sPass.length() == 0) return false;
  Serial.print(" Adding Wifi:" + sSSID + ":" + sPass + ".");
  for (i = 0; i < 10; i++) {
    if (sArrSSID[i] == "") break;
    if (sArrSSID[i] == sSSID) {
      bUpdated = true;
      sArrPASS[i] = sPass;
      Serial.print(" Updated\n");
    }
  }
  if (!bUpdated) {
    if (i < 9) {
      Serial.print(" on slot " + (String)(i) + "\n");
      sArrSSID[i] = sSSID;
      sArrPASS[i] = sPass;
    } else {
      Serial.print(" overload->deleting 0\n");
      for (i = 0; i < 9; i++) {
        sArrSSID[i] = sArrSSID[i + 1];
        sArrPASS[i] = sArrPASS[i + 1];
      }
      sArrSSID[9] = sSSID;
      sArrPASS[9] = sPass;
    }
  }
  bSaveWifi();
  return true;
}
//////////////////////////////////////////////////////////////////////////////
bool bSaveTime(int iTime) {
  if (!bSPIFFSExists) {
    Serial.print("\nERROR: No SPIFFS\n");
    return false;
  }
  String sAux = (String)(iTime);
  writeSPIFFSFile("/time", sAux.c_str());
  Serial.println(" Hour written " + sTimetoStr(iTime) );
  return true;
}
///////////////////
bool bLoadTime() {
  if (!bSPIFFSExists) {
    Serial.print("\nERROR: No SPIFFS\n");
    return false;
  }
  if (!SPIFFS.exists("/time")) {
    Serial.print("\nERROR: No Time file\n");
    return false;
  }
  setenv("TZ", sTimeZone.c_str(), 1);
  String sTime = readSPIFFSFile("/time");
  tNow = atoi(sTime.c_str());
  Serial.println(" Hour loaded " + sTimetoStr(tNow) );
  return true;
}
///////////////////
int iLoadMode() {
  if (!bSPIFFSExists) {
    Serial.print("\nERROR: No SPIFFS\n");
    return -2;
  }
  if (!SPIFFS.exists("/mode")) {
    Serial.print("\nERROR: No Mode file\n");
    return -1;
  }
  String sMode = readSPIFFSFile("/mode");
  int iRes = atoi(sMode.c_str());
  Serial.printf(" Mode loaded %d. \n", iRes);
  return iRes;
}
//////////////////////////////////////////////////////////////////////////////
bool bSaveMode(int iSaveMode) {
  if (!bSPIFFSExists) {
    Serial.print("\nERROR: No SPIFFS\n");
    return false;
  }
  String sAux = (String)(iSaveMode);
  writeSPIFFSFile("/mode", sAux.c_str());
  Serial.printf(" Mode written %s. \n", sAux);
  int i;
  for (i = 0; i < 24; i++) {
    fPriceHour[i] = 0;
  }
  //  bSaveData("1971-01-01");
  return true;
}

//////////////////////////////////////////////////////////////////////////////
bool bSaveInt(char* sName, int iData) {
  if (!bSPIFFSExists) {
    Serial.print("\nERROR: No SPIFFS\n");
    return false;
  }
  String sAux = (String)(iData);
  writeSPIFFSFile(sName, sAux.c_str());
  Serial.printf(" %s written %s. \n", sName, sAux.c_str());
  return true;
}
///////////////////
int iLoadInt(char* sName) {
  if (!bSPIFFSExists) {
    Serial.print("\nERROR: No SPIFFS\n");
    return -2;
  }
  if (!SPIFFS.exists(sName)) {
    Serial.printf("\nERROR: No %s file\n", sName);
    return -1;
  }
  String sData = readSPIFFSFile(sName);
  int iRes = atoi(sData.c_str());
  Serial.printf(" %s loaded = %d. \n", sName, iRes);
  return iRes;
}
//////////////////////////////////////////////////////////////////////////////
bool StartWifiAP() {
  const char *APssid = "ESP32";
  WiFi.mode(WIFI_STA);
  WiFi.softAPdisconnect();
  //IPAddress Ip(192, 168, 1, 1);
  //IPAddress NMask(255, 255, 255, 0);
  //WiFi.softAPConfig(Ip, Ip, NMask);
  WiFi.softAP(APssid);
  Serial.print("Started AP ESP32 with IP address : ");
  Serial.println(WiFi.softAPIP());
  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_STA, &conf);
  String sWifiSsid = String(reinterpret_cast<const char*>(conf.sta.ssid));
  String sWifiPassword = String(reinterpret_cast<const char*>(conf.sta.password));
  display.setRotation(0);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(10, 10);
  display.println("WIFI SETUP");
  display.setCursor(0, 40);
  display.println("Red: " + (String)(APssid));
  display.setCursor(0, 70);
  display.println("Navega web\n" + WiFi.softAPIP().toString());
  display.setCursor(0, 190);
  display.println("Desactiva\ndatos\nmoviles");
  display.update();
  return true;
}//////////////////////////////////////////////////////////////////////////////


bool bGetOMIEData(int iDate, float * fArray) {
  if (!iDate) return false;
  int httpPort = 443, iTries = 5, i;
  String sHost = "www.omie.es";
  String sPath = "https://www.omie.es/es/file-download?parents%5B0%5D=marginalpdbc&filename=marginalpdbc_";
  String sSearch = (String)(year(iDate)) + ";" + int2str2dig(month(iDate)) + ";" + int2str2dig(day(iDate));
  bool bConn = false, bBadData = false;
  unsigned long timeout ;
  WiFiClientSecure  SecureClient;
  sPath = sPath + (String)(iDateNum(iDate)) + ".1";
  Serial.print("  Connecting to {" + String(sHost) + "} ");
  for (i = 0; i < 24; i++) fArray[i] = -100;
  do {
    if (SecureClient.connect(const_cast<char*>(sHost.c_str()), httpPort)) bConn = true;
    else     delay(1000);
    iTries--;
  } while ((iTries) && (!bConn));
  if (!bConn) {
    Serial.print("\n ERROR **Connection failed for  {" + sPath + "} **\n");
    return false;
  }
  Serial.print(" connected.");
  SecureClient.print(String("GET ") + sPath + " HTTP/1.1\r\n" + "Host: " + sHost + "\r\n" + "Connection: close\r\n\r\n");
  timeout = millis();
  while (SecureClient.available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.print("\n ERROR Client Connection Timeout 10 seg... Stopping.");
      SecureClient.stop();
      return false;
    }
  }
  Serial.print(" download \n");
  while (SecureClient.available()) {
    String sTemp = SecureClient.readStringUntil('\n');
    sTemp.replace("\n", "");
    sTemp.replace("\r", "");
    sTemp.trim();
    if (sTemp.indexOf(sSearch) > -1) {
      //     Serial.print("\n" + String(sTemp.length()) + "B,[" + sTemp + "] ");
      int iHour, iPos1, iPos2, iPos3, iPos4;
      iPos1 = sTemp.indexOf(";", 10);
      iPos2 = sTemp.indexOf(";", iPos1 + 1);
      iPos3 = sTemp.indexOf(";", iPos2 + 1);
      iPos4 = sTemp.indexOf(";", iPos3 + 1);
      if ((iPos1 > -1) && (iPos2 > -1) && (iPos3 > -1) && (iPos4 > -1) ) {
        float fPrice = 0;
        String sAux;
        sAux = sTemp.substring(iPos1 + 1, iPos2);
        iHour = sAux.toInt();
        sAux = sTemp.substring(iPos3 + 1, iPos4);
        fArray[iHour - 1] = atof(sAux.c_str());
        //Serial.printf(" hour=%d price=%f,", iHour, fArray[iHour-1]);
      }
    }
  }
  SecureClient.stop();
  for (i = 0; i < 24; i++) {
    if (fArray[i] == -100) {
      bBadData = true;
    }
  }
  if (bBadData) {
    Serial.print(" BAD DATA.");
  } else {
    bCalcOMIECapt();
    Serial.printf(" Capt=%f  Ok.", fCaptOMIE);
  }
  return (!bBadData);
}//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool bGetESIOSData(int iDate, float * fArray) {
  if (!iDate) return false;
  int httpPort = 443, iTries = 5;
  String sHost = "api.esios.ree.es";
  String sPath = "https://api.esios.ree.es/indicators/1001";
  String sSearchDate = "\"datetime\":\"" + (String)(year(iDate)) + "-" + int2str2dig(month(iDate)) + "-" + int2str2dig(day(iDate)) + "T";
  String sSearchPeninsula = "\"geo_id\":8741";
  String sJson = "";
  bool bConn = false, bCollect = false;
  unsigned long timeout ;
  int iHour, iPos1, iPos2, iPos3;
  WiFiClientSecure  SecureClient;
  Serial.print("  Connecting to {" + String(sHost) + "} ");
  do {
    if (SecureClient.connect(const_cast<char*>(sHost.c_str()), httpPort)) bConn = true;
    else     delay(1000);
    iTries--;
  } while ((iTries) && (!bConn));
  if (!bConn) {
    Serial.print("\n ERROR **Connection failed for  {" + sPath + "} **\n");
    return false;
  }
  Serial.print(" connected.");
  Serial.print("\nGET " + sPath + " HTTP/1.1\r\n" + "Host: " + sHost + "\r\n" + "Connection: close\r\n\r\n");
  SecureClient.print("GET " + sPath + " HTTP/1.1\r\n" + "Host: " + sHost + "\r\n" + "Connection: close\r\n\r\n");
  timeout = millis();
  while (SecureClient.available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.print("\n ERROR Client Connection Timeout 10 seg... Stopping.");
      SecureClient.stop();
      return false;
    }
  }
  Serial.print(" connected -> download ");
  while (SecureClient.available()) {
    String sTemp = SecureClient.readStringUntil('\n');
    sTemp.trim();
    if (sTemp == "{") bCollect = true;
    if (sTemp == "") bCollect = false;
    if (bCollect) {
      sJson = sJson + sTemp;
      Serial.print("-");
    } //e lse Serial.print(String(sTemp.length()) + "B [" + sTemp + "]\n");
    sTemp = "";
  }
  SecureClient.stop();

  //  Serial.print("\n\nJSON [" + (String)(sJson.length()) + "]:\n" + sJson + "\n");
  sJson.replace(" ", "");
  if (sJson.length() > 1000) {
    sJson.replace(":00:00.000", "");
    sJson.trim();
    Serial.print("\n" + String(sJson.length()) + "B found.\n");
    for (iHour = 0; iHour < 24; iHour++) {
      iPos1 = 0;
      iPos2 = 0;
      iPos3 = 0;
      fArray[iHour] = -1;
      while (iPos1 > -1) {
        iPos1 = sJson.indexOf(sSearchDate + int2str2dig(iHour), iPos1 + 1);
        if (iPos1 > -1) {
          iPos2 = sJson.indexOf(sSearchPeninsula, iPos1);
          if ((iPos2 > -1) && ((iPos2 - iPos1) < 600)) {
            iPos3 = sJson.indexOf("value", iPos1 - 20);
            if ((iPos3 > -1) && (iPos3 < iPos1)) {
              String sAux;
              sAux = sJson.substring(iPos3 + 7, iPos3 + 13) ;
              fArray[iHour] = atof(sAux.c_str());
              //Serial.print("Value[" + int2str2dig(iHour) + "]:'" + sAux + "'=" + (String)(fArray[iHour]) + "\n");
            } else {
              //Serial.print("hour" + (String)(iHour) + " not value.\n");
            }
          } else {
            //Serial.print("hour" + (String)(iHour) + " not Peninsula with '" + sSearchPeninsula + "' -> " + (String)(iPos2) + "-" + (String)(iPos1) + "=" + (String)(iPos2 - iPos1) + " \n");
          }
        } else {
          //Serial.print("hour" + (String)(iHour) + " not found with '" + sSearchDate + int2str2dig(iHour) + "'.\n");
        }
      }
      Serial.printf(" hour[%d] = %f, ", iHour, fArray[iHour]);
    } //FOR
  }
  Serial.print(" Ok.\n");
  bSaveData((String)(year(iDate)) + "-" + int2str2dig(month(iDate)) + "-" + int2str2dig(day(iDate)));
  return true;
}//////////////////////////////////////////////////////////////////////////////////////////////////
bool bCalcOMIECapt() {
  fCaptOMIE = 0;
  float fCurvTotal = 0;
  int iOffset = (month(tNow) - 1) * 24;
  if ((month(tNow) == 3) && (day(tNow) > 24) )  iOffset += 24;
  else {
    if (month(tNow) > 3)  iOffset += 24;
  }
  if ((month(tNow) == 10) && (day(tNow) > 28))  iOffset += 24;
  else {
    if (month(tNow) > 10) iOffset += 24;
  }
  //  Serial.printf(" bCalcOMIECapt Month=%d , offset=%d  ", month(tNow), iOffset);
  for (int i = 0; i < 14; i++) {
    fCaptOMIE = fCaptOMIE + fPriceHour[i + 6] * fCurv[i + iOffset + 6];
    fCurvTotal = fCurvTotal + fCurv[i + iOffset + 6];
    //   Serial.printf(",%f*%f", fPriceHour[i + 6], fCurv[i + iOffset + 6]);
  }
  fCaptOMIE = fCaptOMIE / fCurvTotal;
  // Serial.printf("   CAPT=%f\n", fCaptOMIE);
  return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
void drawLine(float x1, float y1, float x2, float y2 ) {
  uint16_t fcolor;
  int tmp, i, j , iRot;
  iRot = display.getRotation();

  if (x1 < 0) x1 = 0; if (x2 < 0) x2 = 0; if (y1 < 0) y1 = 0; if (y2 < 0) y2 = 0;
  if (x1 > iScreenXMax) x1 = iScreenXMax; if (x2 > iScreenXMax) x2 = iScreenXMax; if (y1 > iScreenYMax) y1 = iScreenYMax; if (y2 > iScreenYMax) y2 = iScreenYMax;
  if (abs(x2 - x1) > abs(y2 - y1)) {
    if (x1 > x2) {
      tmp = x1; x1 = x2; x2 = tmp;
      tmp = y1; y1 = y2; y2 = tmp;
    }
    for (i = x1; i < x2 ; i++) {
      tmp = y1 + (i - x1) * (y2 - y1) / (x2 - x1);
      display.drawPixel(i, tmp + j, GxEPD_BLACK);
    }
  } else {
    if (y1 > y2) {
      tmp = y1; y1 = y2; y2 = tmp;
      tmp = x1; x1 = x2; x2 = tmp;
    }
    for (i = y1; i < y2 ; i++) {
      tmp = x1 + (i - y1) * (x2 - x1) / (y2 - y1);
      display.drawPixel(tmp + j, i , GxEPD_BLACK);
    }
  }
}
//////////////////////////////////////////////////////////////////////////////
String float2string(float n, int ndec) {
  if (ndec == 0) {
    return (String)(int)(n + 0.5);
  }
  String r = "";
  int v = n;
  r += v;
  r += '.';
  int i;
  for (i = 0; i < ndec; i++) {
    n -= v;
    n *= 10;
    v = n;
    r += v;
  }
  return r;
}
String int2str2dig(int i) {
  String sTmp = "";
  if (i < 10) sTmp = "0";
  sTmp += (String)i;
  return sTmp;
}
int iDateNum(time_t t) {
  return ((year(t) * 10000) + month(t) * 100 + day(t));
}
int hour(time_t t) {
  struct tm * timeinfo;
  timeinfo = localtime(&t);
  return timeinfo->tm_hour;
}
int minute(time_t t) {
  struct tm * timeinfo;
  timeinfo = localtime(&t);
  return timeinfo->tm_min;
}

int second(time_t t) {
  struct tm * timeinfo;
  timeinfo = localtime(&t);
  return timeinfo->tm_sec;
}

int day(time_t t) {
  struct tm * timeinfo;
  timeinfo = localtime(&t);
  return timeinfo->tm_mday;
}

int weekday(time_t t) {
  struct tm * timeinfo;
  timeinfo = localtime(&t);
  return (timeinfo->tm_wday + 1);
}

int month(time_t t) {
  struct tm * timeinfo;
  timeinfo = localtime(&t);
  return (timeinfo->tm_mon + 1);
}

int year(time_t t) {
  struct tm * timeinfo;
  timeinfo = localtime(&t);
  if (timeinfo->tm_year < 2000)
    return (1900 + timeinfo->tm_year);
  else
    return (timeinfo->tm_year);
}

//////////////////////////////////////////////////////////////////////////////
String sInt32TimetoStr(int32_t tTime) {
  time_t tAux = tTime;
  return (String)(asctime(localtime(&tAux)));
}

String sTimetoStr(int32_t tTime) {
  char buff[30];
  sprintf(buff, "%04d/%02d/%02d_%02d:%02d:%02d", year(tTime), month(tTime), day(tTime), hour(tTime), minute(tTime), second(tTime));
  return (String)(buff);
}


//////////////////////////////////////////////////////////////////////////////
bool bLoadData(String sToday) {
  String sData, sAux, sModeName;
  float fDataSum = 0;
  int iPos1 = 0, iPos2 = 0 , i;
  if (iMode % 2) sModeName = "/esios";
  else sModeName = "/omie";
  if (!SPIFFS.exists(sModeName.c_str())) return false;
  sData = readSPIFFSFile(sModeName.c_str());
  iPos1 = sData.indexOf("\n", 0);
  if (iPos1 == -1) return false;
  sAux = sData.substring(0, iPos1);
  if (sToday == "") sToday = sAux;
  //Serial.printf("\nDATA = '%s' & Today = '%s'", sData.c_str(), sToday.c_str());
  if (sAux != sToday)  {
    Serial.printf("\nDATA is old with '%s' and '%s'", sAux.c_str(), sToday.c_str());
    for (i = 0; i < 24; i++) {
      fPriceHour[i] = -1;
    }
    return false;
  }
  for (i = 0; i < 24; i++) {
    iPos2 = sData.indexOf("\n", iPos1 + 1);
    sAux = sData.substring(iPos1 + 1, iPos2);
    //Serial.printf("\n %d:%s", i, sAux.c_str());
    fPriceHour[i] = atof(sAux.c_str());
    if (i > 1) fDataSum += fPriceHour[i] ;
    iPos1 = iPos2;
  }
  if (iMode % 2) {
    Serial.printf("\nLoadData; mean ESIOS value = '%f'\n", fDataSum / 24);
  } else {
    bCalcOMIECapt();
    Serial.printf("\nLoadData; Captured OMIE value = '%f'\n", fCaptOMIE);
  }

  return (fDataSum > 0);
}
//////////////////////////////////////////////////////////////////////////////
bool bSaveData(String sToday) {
  String sData, sAux, sModeName;
  int i, iSum = 0;
  sData = sToday + "\n";
  for (i = 0; i < 24; i++) {
    sData = sData + (String)(fPriceHour[i]) + "\n";
    iSum += fPriceHour[i];
  }
  if (iMode % 2) sModeName = "/esios";
  else sModeName = "/omie";
  if (iSum > 0)  writeSPIFFSFile(sModeName.c_str(), sData.c_str());
  return true;
}
//////////////////////////////////////////////////////////////////////////////
bool bGetData(int tTime) {
  String sAux = (String)(year(tTime)) + "-" + int2str2dig(month(tTime)) + "-" + int2str2dig(day(tTime));
  if (!bLoadData(sAux)) {
    if (!bWeHaveWifi) StartWiFi(10);
    if (iMode % 2)  {
      bGetESIOSData(tTime, fPriceHour);
    } else {
      bGetOMIEData(tTime, fPriceHour);
    }
  }
  int i, iSum = 0;
  for (i = 0; i < 24; i++) iSum += fPriceHour[i];
  if (iSum > 0) bSaveData(sAux);
  return (iSum > 0);
}
//////////////////////////////////////////////////////////////////////////////
String readFSFile(fs::FS & fs, const char * path) {
  Serial.printf(" {R'%s'", path);
  delay(50);
  File file = fs.open(path);
  delay(50);
  if (!file || file.isDirectory()) {
    Serial.println(" failed!}");
    return "";
  }
  String sAux = "";
  int iFileLength = file.size();
  while (file.available() && (sAux.length() < iFileLength)) {
    sAux = sAux + (char)(file.read());
  }
  //  Serial.printf("\n<%s>\n", sAux.c_str());
  Serial.printf("%dB}", sAux.length());
  return sAux;
}
//////////////////////////////////////////////////////////////////////////////
bool writeFSFile(fs::FS & fs, const char * path, const char * message) {
  Serial.printf(" {W'%s'", path);
  delay(50);
  File file = fs.open(path, FILE_WRITE);
  delay(50);
  if (!file) {
    Serial.println("- failed to open file for writing}");
    return false;
  }
  if (file.print(message)) {
    Serial.print(">" + (String)(strlen(message)) + "B}");
    delay(50);
    return true;
  } else {
    Serial.println(" failed!}");
    delay(50);
    return false;
  }
}
//////////////////////////////////////////////////////////////////////////////
int sizeFSFile(const char * path) {
  Serial.printf(" {S'%s'", path);
  delay(50);
  File file = SPIFFS.open(path);
  delay(50);
  if (!file) {
    Serial.println("- failed to size}");
    return false;
  }
  Serial.print(">" + (String)(file.size()) + "B}");
  delay(50);
  return file.size();
}
//////////////////////////////////////////////////////////////////////////////
bool  deleteFSFile(fs::FS & fs, const char * path) {
  delay(10);
  if (!fs.exists(path)) {
    return true;
  }
  Serial.printf(" {D'%s'", path);
  delay(10);
  if (fs.remove(path)) {
    Serial.println("}");
    delay(50);
    return true;
  } else {
    Serial.println("- failed}");
    delay(50);
    return false;
  }
}
//////////////////////////////////////////////////////////////////////////////
String listDir(fs::FS & fs, const char * dirname, uint8_t levels, bool bSerialPrint) {
  static int iFileNum = 0;
  String sListRet = "";
  if (bSerialPrint) Serial.printf("Listing directory: %s\r\n", dirname);
  File root = fs.open(dirname);
  if (!root) {
    if (bSerialPrint) Serial.println("- failed to open directory");
    return "[FAILED]";
  }
  if (!root.isDirectory()) {
    if (bSerialPrint) Serial.println(" - not a directory");
    return "[NO DIR]";
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      if (bSerialPrint) Serial.print(" DIR: ");
      if (bSerialPrint) Serial.print(file.name());
      if (levels) {
        sListRet = sListRet + listDir(fs, file.name(), levels - 1, bSerialPrint);
      }
    } else {
      iFileNum++;
      if (iFileNum > 5) {
        iFileNum = 0;
        if (bSerialPrint) Serial.print("\n");
      }
      if (bSerialPrint) Serial.print(file.name());
      String sAux = file.name();
      //      if ((bSerialPrint) && (sAux.length() < 8)) Serial.print("\t");
      if (bSerialPrint) Serial.print(":");
      if (bSerialPrint) Serial.print(file.size());
      if (bSerialPrint) Serial.print("B,");
    }
    sListRet = sListRet + "," + (String)(file.name()) + ":" + (String)(file.size()) + "B";
    file = root.openNextFile();
  }
  return sListRet;
}
//////////////////////////////////////////////////////////////////////////////
String listSPIFFSDir(const char * dirname, uint8_t levels, bool bSerialPrint) {
  return listDir(SPIFFS, dirname, levels, bSerialPrint);
}
//////////////////////////////////////////////////////////////////////////////
String readSPIFFSFile(const char * path) {
  return readFSFile(SPIFFS, path);
}
//////////////////////////////////////////////////////////////////////////////
bool writeSPIFFSFile(const char * path, const char * message) {
  return writeFSFile(SPIFFS, path, message);
}
//////////////////////////////////////////////////////////////////////////////
bool deleteSPIFFSFile(const char * path) {
  return deleteFSFile(SPIFFS, path);
}
//////////////////////////////////////////////////////////////////////////////
String sGetResetReason() {
  String ret = "";
  switch (rtc_get_reset_reason(0)) {
    case 1   : ret = "PON"; break;
    case 5   : ret = "SLP"; break;
    case 7   : ret = "WD0"; break;
    case 8   : ret = "WD1"; break;
    case 12  : ret = "RST"; break;
    case 13  : ret = "BUT"; break;
    case 16  : ret = "RTC"; break;
    default:    ret = (String)(rtc_get_reset_reason(0));
  }
  return ret;
}
//////////////////////////////////////////////////////////////////////////////
String getMacAddress() {
  uint8_t baseMac[6];
  // Get MAC address for WiFi station
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  char baseMacChr[18] = {0};
  sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  return String(baseMacChr);
}
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
#define PIX_FACTOR 10
bool DisplayTextAligned(int xO, int yO, String sText, int alig, int iPixel) {
  int16_t tbx, tby, iRot;
  uint16_t tbw, tbh, x , y;
  display.getTextBounds(sText, xO, yO, &tbx, &tby, &tbw, &tbh);
  iRot = display.getRotation();
  y = yO;
  if (alig < 0) {
    if (xO > (tbw * iPixel / PIX_FACTOR))    {
      x = xO - (tbw * iPixel / PIX_FACTOR);
    }    else {
      x = 0;
    }
  }
  if (alig == 0) {
    if (xO > ((tbw * iPixel / PIX_FACTOR) / 2)) {
      x = xO - ((tbw * iPixel / PIX_FACTOR) / 2);
    }    else {
      x = 0;
    }
  }
  if (alig > 0) {
    x = xO ;
  }
  display.setCursor(x, y);
  display.print(sText);
  //Serial.printf("\nTextAllign: '%s', %d,%d->%d,%d tbx=%d tby=%d tbw=%d tbh=%d", sText.c_str(), xO, yO, x, y, tbx, tby, tbw, tbh);
  return true;
}//////////////////////////////////////////////////////////////////////////////
void drawBar(int x1, int y1, int x2, int y2, int trama, uint16_t color ) {
  uint16_t i, j, tmp;
  if (x1 < 0) x1 = 0; if (x2 < 0) x2 = 0; if (y1 < 0) y1 = 0; if (y2 < 0) y2 = 0;
  if (x1 > iScreenXMax) x1 = iScreenXMax; if (x2 > iScreenXMax) x2 = iScreenXMax; if (y1 > iScreenYMax) y1 = iScreenYMax; if (y2 > iScreenYMax) y2 = iScreenYMax;
  if (x1 > x2) {
    tmp = x1; x1 = x2; x2 = tmp;
  }
  if (y1 > y2) {
    tmp = y1; y1 = y2; y2 = tmp;
  }
  for (i = x1; i < x2 ; i++) {
    for (j = y1; j < y2 ; j++) {
      if (((i % trama) == 0) && ((j % trama) == 0)) display.drawPixel(i, j, color);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// OTA Logic
String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}//////////////////////////////////////////////////////////////////////////////////////////////////////////
bool execOTA(String sOtaBinName) {
  //AWS S3 OTA Variables, Update to yours
  const String sOtaHost = "s3.eu-west-3.amazonaws.com";
  const String sOtaBucket = "/otabin-weather/";
  const int iOtaPort = 443;
  int contentLength = 0;
  bool isValidContentType = false;
  bool bResult = false;
  WiFiClientSecure  SecureClient;
  Serial.println("Connecting to: " + String(sOtaHost));
  if (SecureClient.connect(sOtaHost.c_str(), iOtaPort)) {
    SecureClient.print(String("GET ") + sOtaBucket + sOtaBinName +  " HTTP/1.1\r\n" + "Host: " + sOtaHost + "\r\n" + "Cache-Control: no-cache\r\n" + "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (SecureClient.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("Client Timeout !");
        SecureClient.stop();
        return false;
      }
    }
    while (SecureClient.available()) {
      String line = SecureClient.readStringUntil('\n');
      line.trim();
      if (!line.length())         break;
      if (line.startsWith("HTTP/1.1")) {
        if (line.indexOf("200") < 0) {
          Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
          break;
        }
      }
      if (line.startsWith("Content-Length: ")) {
        contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str()); //atoi(line.substring(strlen("Content-Length: ")));
        Serial.println("Got " + String(contentLength) + " bytes from server");
      }
      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        Serial.println("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
      }
    }
  } else     Serial.println("Connection to " + String(sOtaHost) + " failed. Please check your setup");
  Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));
  if (contentLength && isValidContentType) {
    bool canBegin = Update.begin(contentLength);
    if (canBegin) {
      Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      size_t written = Update.writeStream(SecureClient);
      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
        bResult = true;
      } else Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
          return true;
        } else   Serial.println("Update not finished? Something went wrong!");
      } else         Serial.println("Error Occurred. Error #: " + String(Update.getError()));
    } else {
      Serial.println("Not enough space to begin OTA");
      SecureClient.flush();
    }
  } else {
    Serial.println("There was no content in the response");
    SecureClient.flush();
    return bResult;
  }
  return true;
}
//////////////////////////////////////////////////////////////////////////////
