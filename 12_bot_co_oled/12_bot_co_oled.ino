#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include "bi_mat.h"

IPAddress ipTinh   (192, 168, 0,  50);
IPAddress gateway  (192, 168, 0,   1);
IPAddress subnet   (255, 255, 255, 0);
IPAddress dnsServer(192, 168, 0,   1);

#define AP_SSID  "ESP32-Phong"
#define AP_PASS  "esp32phong"

IPAddress apIP     (192, 168, 4, 1);
IPAddress apSubnet (255, 255, 255, 0);

void veThongBao(const char* dong1, const char* dong2);

struct MangWifi {
  const char* ten;
  const char* matKhau;
  bool        ipTinhRieng;
};

MangWifi dsWifi[] = {
  { WIFI_SSID,   WIFI_PASS,   true  },
  { WIFI_SSID_2, WIFI_PASS_2, false },
};

#define SO_MANG_WIFI ((int)(sizeof(dsWifi) / sizeof(dsWifi[0])))

int mangDangDung = -1;

bool thuNoiMang(int i, unsigned long hanCho) {
  if (dsWifi[i].ten == nullptr || strlen(dsWifi[i].ten) == 0) return false;

  Serial.printf("Thu noi Wi-Fi \"%s\"", dsWifi[i].ten);
  veThongBao("Dang noi Wi-Fi...", dsWifi[i].ten);

  WiFi.disconnect();
  if (dsWifi[i].ipTinhRieng) WiFi.config(ipTinh, gateway, subnet, dnsServer);
  else                       WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.begin(dsWifi[i].ten, dsWifi[i].matKhau);

  unsigned long batDau = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - batDau < hanCho) {
    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    mangDangDung = i;
    Serial.printf(" OK - IP %s\n", WiFi.localIP().toString().c_str());
    return true;
  }
  Serial.println(" that bai.");
  return false;
}

bool noiWifi(unsigned long hanCho) {
  for (int i = 0; i < SO_MANG_WIFI; i++) if (thuNoiMang(i, hanCho)) return true;
  return false;
}

#define DHT_PIN  4
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

#define OLED_RONG     128
#define OLED_CAO      64
#define OLED_DIA_CHI  0x3C
#define SO_TRANG      3
#define CHU_KY_TRANG  5000UL
#define CHU_KY_VE     500UL

Adafruit_SSD1306 man(OLED_RONG, OLED_CAO, &Wire, -1);
bool coManHinh = false;
unsigned long lanVeCuoi = 0;
WebServer server(80);

float nhietDo = 0, doAm = 0;
unsigned long lanDocCuoi = 0;

#define NHIET_CAO      35.0
#define NHIET_CAO_VE   34.0

#define AM_CAO         80.0
#define AM_CAO_VE      75.0

#define AM_THAP        40.0
#define AM_THAP_VE     45.0

bool dangBaoNhietCao = false;
bool dangBaoAmCao    = false;
bool dangBaoAmThap   = false;

#define CHU_KY_CLOUD  60000UL

unsigned long lanGuiCloudCuoi = 0;
int    soLanGuiCloud = 0;
String trangThaiCloud = "chua gui lan nao";

#define CHU_KY_HOI_BOT  5000UL

unsigned long lanHoiBotCuoi = 0;
long   updateIdCuoi = 0;
String trangThaiBot = "chua hoi lan nao";

#define CHU_KY_KIEM_TRA_WIFI  30000UL
#define CHO_TOI_DA_MAT_WIFI   300000UL

unsigned long lanKiemTraWifi = 0;
unsigned long lucBatDauMatWifi = 0;
int soLanNoiLaiWifi = 0;

#define SO_DIEM     60
#define CHU_KY_LUU  10000UL

float lichSuT[SO_DIEM];
float lichSuH[SO_DIEM];
int   soDiem = 0;
unsigned long lanLuuCuoi = 0;
bool  daCoSoDo = false;

void capNhatCamBien() {
  if (millis() - lanDocCuoi < 2000) return;
  lanDocCuoi = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t) && h >= 1) { doAm = h; nhietDo = t; daCoSoDo = true; }
}

void luuLichSu() {
  if (!daCoSoDo) return;
  if (millis() - lanLuuCuoi < CHU_KY_LUU) return;
  lanLuuCuoi = millis();

  if (soDiem < SO_DIEM) {
    lichSuT[soDiem] = nhietDo;
    lichSuH[soDiem] = doAm;
    soDiem++;
  } else {
    for (int i = 0; i < SO_DIEM - 1; i++) {
      lichSuT[i] = lichSuT[i + 1];
      lichSuH[i] = lichSuH[i + 1];
    }
    lichSuT[SO_DIEM - 1] = nhietDo;
    lichSuH[SO_DIEM - 1] = doAm;
  }
}

bool guiTelegram(String noiDung) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = String("https://api.telegram.org/bot") + TELEGRAM_TOKEN + "/sendMessage";
  if (!http.begin(client, url)) {
    Serial.println("Telegram: khong mo duoc ket noi");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  String body = String("{\"chat_id\":\"") + TELEGRAM_CHAT_ID +
                "\",\"text\":\"" + noiDung + "\"}";

  int ma = http.POST(body);
  http.end();

  Serial.printf("Telegram: HTTP %d | %s\n", ma, noiDung.c_str());
  return ma == 200;
}

void kiemTraWifi() {
  if (millis() - lanKiemTraWifi < CHU_KY_KIEM_TRA_WIFI) return;
  lanKiemTraWifi = millis();

  if (WiFi.status() == WL_CONNECTED) {
    if (lucBatDauMatWifi != 0) {
      Serial.printf("Wi-Fi da noi lai sau %lu giay.\n",
                    (millis() - lucBatDauMatWifi) / 1000);
      lucBatDauMatWifi = 0;
    }
    return;
  }

  if (lucBatDauMatWifi == 0) {
    lucBatDauMatWifi = millis();
    Serial.println("MAT WI-FI. Bat dau thu noi lai...");
  }

  if (millis() - lucBatDauMatWifi > CHO_TOI_DA_MAT_WIFI) {
    Serial.println("Mat Wi-Fi qua 5 phut -> KHOI DONG LAI BOARD.");
    Serial.flush();
    delay(200);
    ESP.restart();
  }

  static int ketTiep = 0;
  soLanNoiLaiWifi++;
  Serial.printf("Thu noi lai lan %d...\n", soLanNoiLaiWifi);
  thuNoiMang(ketTiep % SO_MANG_WIFI, 6000);
  ketTiep++;
}

String thoiGianChay() {
  unsigned long s = millis() / 1000;
  return String(s / 3600) + "h" + String((s % 3600) / 60) + "m" + String(s % 60) + "s";
}

void xuLyLenh(String lenh, long long chatId) {
  static const long long CHU_SO_HUU = atoll(TELEGRAM_CHAT_ID);
  if (chatId != CHU_SO_HUU) {
    Serial.printf("Bo qua lenh tu chat la: %lld\n", chatId);
    return;
  }

  lenh.trim();
  lenh.toLowerCase();
  Serial.printf("Nhan lenh: %s\n", lenh.c_str());

  if (lenh.startsWith("/nhietdo") || lenh.startsWith("/nhiet")) {
    guiTelegram("Nhiet do: " + String(nhietDo, 1) + " do C\n" +
                "Do am:    " + String(doAm, 1) + " %");
  }
  else if (lenh.startsWith("/trangthai") || lenh.startsWith("/status")) {
    guiTelegram(String("Thiet bi dang chay binh thuong.\n") +
                "Thoi gian chay: " + thoiGianChay() + "\n" +
                "Song Wi-Fi: " + String(WiFi.RSSI()) + " dBm\n" +
                "Dia chi qua mang nha: http://" + WiFi.localIP().toString() + "\n" +
                "Song rieng \"" + AP_SSID + "\": http://" + WiFi.softAPIP().toString() + "\n" +
                "May dang noi vao song rieng: " + String(WiFi.softAPgetStationNum()) + "\n" +
                "Cloud: " + trangThaiCloud + "\n" +
                "Da gui cloud: " + String(soLanGuiCloud) + " lan\n" +
                "Diem lich su: " + String(soDiem) + "/" + String(SO_DIEM) + "\n" +
                "So lan noi lai Wi-Fi: " + String(soLanNoiLaiWifi));
  }
  else if (lenh.startsWith("/bieudo") || lenh.startsWith("/chart")) {
    guiTelegram(String("Bieu do 10 phut (chi trong nha):\n") +
                "http://" + WiFi.localIP().toString() + "\n" +
                "hoac noi Wi-Fi \"" + AP_SSID + "\" roi vao:\n" +
                "http://" + WiFi.softAPIP().toString() + "\n\n" +
                "Bieu do dai han (moi noi):\n" +
                "https://thingspeak.mathworks.com/channels/" + THINGSPEAK_CHANNEL);
  }
  else {
    guiTelegram(String("Cac lenh dung duoc:\n") +
                "/nhietdo - nhiet do va do am hien tai\n" +
                "/trangthai - tinh trang thiet bi\n" +
                "/bieudo - link xem bieu do");
  }
}

void hoiTelegram() {
  if (millis() - lanHoiBotCuoi < CHU_KY_HOI_BOT) return;
  lanHoiBotCuoi = millis();

  const int TOI_DA = 5;
  String   dsLenh[TOI_DA];
  long long dsChat[TOI_DA];
  int soLenh = 0;

  {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = String("https://api.telegram.org/bot") + TELEGRAM_TOKEN +
                 "/getUpdates?timeout=0&limit=5&offset=" + String(updateIdCuoi + 1);

    if (!http.begin(client, url)) { trangThaiBot = "khong mo duoc ket noi"; return; }

    int ma = http.GET();
    if (ma != 200) {
      http.end();
      trangThaiBot = "HTTP " + String(ma);
      return;
    }

    JsonDocument boLoc;
    boLoc["result"][0]["update_id"] = true;
    boLoc["result"][0]["message"]["chat"]["id"] = true;
    boLoc["result"][0]["message"]["text"] = true;

    JsonDocument doc;
    DeserializationError loi = deserializeJson(doc, http.getStream(),
                                               DeserializationOption::Filter(boLoc));
    http.end();

    if (loi) { trangThaiBot = String("loi JSON: ") + loi.c_str(); return; }

    trangThaiBot = "OK";
    for (JsonObject u : doc["result"].as<JsonArray>()) {
      updateIdCuoi = u["update_id"].as<long>();
      const char* noiDung = u["message"]["text"];
      if (noiDung && soLenh < TOI_DA) {
        dsLenh[soLenh] = String(noiDung);
        dsChat[soLenh] = u["message"]["chat"]["id"].as<long long>();
        soLenh++;
      }
    }
  }

  for (int i = 0; i < soLenh; i++) xuLyLenh(dsLenh[i], dsChat[i]);
}

void guiLenCloud() {
  if (!daCoSoDo) return;
  if (millis() - lanGuiCloudCuoi < CHU_KY_CLOUD) return;
  lanGuiCloudCuoi = millis();

  WiFiClient client;
  HTTPClient http;

  String url = String("http://api.thingspeak.com/update?api_key=") + THINGSPEAK_KEY +
               "&field1=" + String(nhietDo, 1) +
               "&field2=" + String(doAm, 1);

  if (!http.begin(client, url)) {
    trangThaiCloud = "khong mo duoc ket noi";
    Serial.println("ThingSpeak: khong mo duoc ket noi");
    return;
  }

  int ma = http.GET();
  String dap = http.getString();
  http.end();

  if (ma == 200 && dap != "0") {
    soLanGuiCloud++;
    trangThaiCloud = "OK - ban ghi #" + dap;
  } else {
    trangThaiCloud = "that bai (HTTP " + String(ma) + ", tra ve " + dap + ")";
  }
  Serial.printf("ThingSpeak: %s\n", trangThaiCloud.c_str());
}

void kiemTraNguong(bool &dangBao, float giaTri, float mocVao, float mocRa,
                   const char* ten, const char* donVi) {
  bool nguongCao = (mocVao > mocRa);
  bool vuot = nguongCao ? (giaTri >= mocVao) : (giaTri <= mocVao);
  bool ve   = nguongCao ? (giaTri <= mocRa)  : (giaTri >= mocRa);

  if (!dangBao && vuot) {
    dangBao = true;
    guiTelegram(String("CANH BAO - ") + ten + ": " + String(giaTri, 1) + donVi +
                " (nguong " + String(mocVao, 1) + donVi + ")");
  }
  else if (dangBao && ve) {
    dangBao = false;
    guiTelegram(String("Het canh bao - ") + ten + ": " + String(giaTri, 1) + donVi);
  }
}

void kiemTraCanhBao() {
  if (!daCoSoDo) return;
  kiemTraNguong(dangBaoNhietCao, nhietDo, NHIET_CAO, NHIET_CAO_VE, "nhiet do cao", " do C");
  kiemTraNguong(dangBaoAmCao,    doAm,    AM_CAO,    AM_CAO_VE,    "do am cao",    " %");
  kiemTraNguong(dangBaoAmThap,   doAm,    AM_THAP,   AM_THAP_VE,   "do am thap",   " %");
}

void veTieuDe(const char* ten) {
  man.setTextSize(1);
  man.setCursor(0, 0);
  man.print(ten);
  man.setCursor(98, 0);
  struct tm gio;
  if (getLocalTime(&gio, 0)) man.printf("%02d:%02d", gio.tm_hour, gio.tm_min);
  else                       man.print("--:--");
  man.drawLine(0, 10, OLED_RONG - 1, 10, SSD1306_WHITE);
}

void veTrangSoDo() {
  veTieuDe("Phong ngu");
  if (!daCoSoDo) {
    man.setTextSize(2);
    man.setCursor(0, 28);
    man.print("Dang do...");
    return;
  }

  man.setTextSize(3);
  man.setCursor(0, 16);
  man.print(nhietDo, 1);
  int x = man.getCursorX();
  man.drawCircle(x + 3, 18, 2, SSD1306_WHITE);
  man.setTextSize(2);
  man.setCursor(x + 8, 23);
  man.print("C");

  const char* canhBao = dangBaoNhietCao ? "NHIET DO CAO"
                      : dangBaoAmCao    ? "DO AM CAO"
                      : dangBaoAmThap   ? "DO AM THAP" : nullptr;

  if (canhBao && (millis() / 1000) % 2 == 0) {
    man.fillRect(0, 44, OLED_RONG, 20, SSD1306_WHITE);
    man.setTextColor(SSD1306_BLACK);
    man.setTextSize(1);
    man.setCursor(4, 46);
    man.print("CANH BAO");
    man.setCursor(4, 55);
    man.print(canhBao);
    man.setCursor(98, 50);
    man.print(doAm, 0);
    man.print(" %");
    man.setTextColor(SSD1306_WHITE);
  } else {
    man.setTextSize(2);
    man.setCursor(0, 46);
    man.print(doAm, 0);
    man.print(" %");
    man.setTextSize(1);
    man.setCursor(64, 51);
    man.print(doAm < AM_THAP ? "kho" : doAm > AM_CAO ? "am" : "de chiu");
  }
}

void veTrangBieuDo() {
  veTieuDe("Nhiet do 10 phut");
  if (soDiem < 2) {
    man.setCursor(0, 30);
    man.print("Chua du du lieu...");
    return;
  }

  float nhoNhat = lichSuT[0], lonNhat = lichSuT[0];
  for (int i = 1; i < soDiem; i++) {
    if (lichSuT[i] < nhoNhat) nhoNhat = lichSuT[i];
    if (lichSuT[i] > lonNhat) lonNhat = lichSuT[i];
  }
  float khoang = lonNhat - nhoNhat;
  if (khoang < 1) khoang = 1;
  float giua = (lonNhat + nhoNhat) / 2;

  const int TREN = 14, DUOI = 50;
  int xTruoc = 0, yTruoc = 0;
  for (int i = 0; i < soDiem; i++) {
    int x = i * (OLED_RONG - 1) / (SO_DIEM - 1);
    int y = (TREN + DUOI) / 2 - (int)((lichSuT[i] - giua) / khoang * (DUOI - TREN));
    if (i > 0) man.drawLine(xTruoc, yTruoc, x, y, SSD1306_WHITE);
    xTruoc = x; yTruoc = y;
  }

  man.setCursor(0, 56);
  man.printf("%.1f-%.1f", nhoNhat, lonNhat);
  man.setCursor(86, 56);
  man.printf("%.1f C", nhietDo);
}

void veTrangTrangThai() {
  veTieuDe("He thong");
  bool coWifi = (WiFi.status() == WL_CONNECTED);

  man.setCursor(0, 14);
  if (coWifi) man.printf("WiFi  OK  %d dBm", WiFi.RSSI());
  else        man.print("WiFi  MAT KET NOI");

  man.setCursor(0, 24);
  man.print("IP    ");
  man.print(coWifi ? WiFi.localIP().toString() : String("-"));

  man.setCursor(0, 34);
  man.print("Cloud ");
  if (trangThaiCloud.startsWith("OK")) man.printf("OK  %d lan", soLanGuiCloud);
  else if (soLanGuiCloud == 0 && trangThaiCloud.startsWith("chua")) man.print("dang cho");
  else man.print("LOI");

  man.setCursor(0, 44);
  man.print("Bot   ");
  man.print(trangThaiBot == "OK" ? "OK" : trangThaiBot.startsWith("chua") ? "dang cho" : "LOI");

  man.setCursor(0, 54);
  man.print("Chay  ");
  man.print(thoiGianChay());
}

void veManHinh() {
  if (!coManHinh) return;
  if (millis() - lanVeCuoi < CHU_KY_VE) return;
  lanVeCuoi = millis();

  man.clearDisplay();
  man.setTextColor(SSD1306_WHITE);
  switch ((millis() / CHU_KY_TRANG) % SO_TRANG) {
    case 0: veTrangSoDo();      break;
    case 1: veTrangBieuDo();    break;
    case 2: veTrangTrangThai(); break;
  }
  man.display();
}

void veThongBao(const char* dong1, const char* dong2) {
  if (!coManHinh) return;
  man.clearDisplay();
  man.setTextColor(SSD1306_WHITE);
  man.setTextSize(1);
  man.setCursor(0, 20);
  man.print(dong1);
  man.setCursor(0, 34);
  man.print(dong2);
  man.display();
}

void guiJSON() {
  String s = "{\"t\":" + String(nhietDo, 1) + ",\"h\":" + String(doAm, 1) + ",\"T\":[";
  for (int i = 0; i < soDiem; i++) { if (i) s += ","; s += String(lichSuT[i], 1); }
  s += "],\"H\":[";
  for (int i = 0; i < soDiem; i++) { if (i) s += ","; s += String(lichSuH[i], 1); }
  s += "],\"c\":\"" + trangThaiCloud + "\",\"b\":\"" + trangThaiBot + "\"}";
  server.send(200, "application/json", s);
}

void guiTrangChu() {
  String html = R"HTML(<!DOCTYPE html><html lang="vi"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 DHT11</title><style>
:root{--bg:#0f172a;--card:#1e293b;--mut:#94a3b8;--t:#fb923c;--h:#38bdf8}
*{box-sizing:border-box}
body{font-family:system-ui,sans-serif;background:var(--bg);color:#e2e8f0;margin:0;padding:16px}
.wrap{max-width:720px;margin:0 auto;display:flex;flex-direction:column;gap:16px}
.row{display:flex;gap:16px;flex-wrap:wrap}
.card{background:var(--card);border-radius:16px;padding:20px;flex:1;min-width:150px}
.lbl{font-size:12px;color:var(--mut);text-transform:uppercase;letter-spacing:1px}
.val{font-size:44px;font-weight:700;margin:6px 0 0}
.val.t{color:var(--t)} .val.h{color:var(--h)}
.chart{background:var(--card);border-radius:16px;padding:16px}
.head{display:flex;justify-content:space-between;align-items:baseline;margin-bottom:8px}
.minmax{font-size:12px;color:var(--mut)}
svg{width:100%;height:150px;display:block}
.axis{font-size:11px;fill:var(--mut)}
.note{text-align:center;color:var(--mut);font-size:12px}
</style></head><body><div class="wrap">

<div class="row">
  <div class="card"><div class="lbl">Nhiet do</div>
    <div class="val t"><span id="t">--</span>&deg;C</div></div>
  <div class="card"><div class="lbl">Do am</div>
    <div class="val h"><span id="h">--</span>%</div></div>
</div>

<div class="chart">
  <div class="head"><div class="lbl">Nhiet do 10 phut qua</div>
    <div class="minmax" id="mmT"></div></div>
  <svg id="cT" viewBox="0 0 600 150" preserveAspectRatio="none"></svg>
</div>

<div class="chart">
  <div class="head"><div class="lbl">Do am 10 phut qua</div>
    <div class="minmax" id="mmH"></div></div>
  <svg id="cH" viewBox="0 0 600 150" preserveAspectRatio="none"></svg>
</div>

<div class="note" id="note">Dang tai...</div>
<div class="note">Nguong canh bao: nhiet do &ge; 35&deg;C  -  do am &ge; 80% hoac &le; 40%</div>
<div class="note">Cloud: <span id="cloud">--</span>  -  Bot: <span id="bot">--</span></div>
<div class="note"><a href="/test" target="_blank" style="color:#38bdf8">Gui thu tin nhan Telegram</a></div>
</div><script>
const W=600,H=150,P=22;

function ve(id, mang, mau, mmId, donVi){
  const svg=document.getElementById(id);
  if(!mang || mang.length<2){
    svg.innerHTML='<text x="300" y="80" text-anchor="middle" class="axis">Dang thu thap du lieu...</text>';
    document.getElementById(mmId).textContent='';
    return;
  }
  let min=Math.min(...mang), max=Math.max(...mang);
  if(max-min<1){ const g=(1-(max-min))/2; min-=g; max+=g; }
  const x=i=>P+i*(W-2*P)/(mang.length-1);
  const y=v=>H-P-(v-min)*(H-2*P)/(max-min);

  const pts=mang.map((v,i)=>x(i).toFixed(1)+','+y(v).toFixed(1)).join(' ');
  const vung='M'+x(0)+','+(H-P)+' L'+pts.split(' ').join(' L')+' L'+x(mang.length-1)+','+(H-P)+' Z';

  svg.innerHTML=
    '<path d="'+vung+'" fill="'+mau+'" opacity="0.15"/>'+
    '<polyline points="'+pts+'" fill="none" stroke="'+mau+'" stroke-width="2" '+
      'stroke-linejoin="round" stroke-linecap="round"/>'+
    '<circle cx="'+x(mang.length-1)+'" cy="'+y(mang[mang.length-1])+'" r="4" fill="'+mau+'"/>'+
    '<text x="2" y="'+(P-6)+'" class="axis">'+max.toFixed(1)+'</text>'+
    '<text x="2" y="'+(H-P+12)+'" class="axis">'+min.toFixed(1)+'</text>';

  document.getElementById(mmId).textContent='thap '+min.toFixed(1)+donVi+'  -  cao '+max.toFixed(1)+donVi;
}

async function tick(){
  try{
    const d=await (await fetch('/data')).json();
    document.getElementById('t').textContent=d.t.toFixed(1);
    document.getElementById('h').textContent=d.h.toFixed(1);
    ve('cT',d.T,'#fb923c','mmT','°C');
    ve('cH',d.H,'#38bdf8','mmH','%');
    document.getElementById('note').textContent=
      d.T.length+' diem da luu  -  moi diem cach nhau 10 giay';
    document.getElementById('cloud').textContent=d.c||'--';
    document.getElementById('bot').textContent=d.b||'--';
  }catch(e){
    document.getElementById('note').textContent='Mat ket noi toi ESP32...';
  }
}
tick(); setInterval(tick,3000);
</script></body></html>)HTML";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  dht.begin();

  Wire.begin();
  coManHinh = man.begin(SSD1306_SWITCHCAPVCC, OLED_DIA_CHI);
  Serial.println(coManHinh ? "OLED: OK" : "OLED: KHONG THAY, chay tiep khong co man hinh");
  WiFi.mode(WIFI_AP_STA);

  WiFi.softAPConfig(apIP, apIP, apSubnet);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("Mang rieng cua ESP32: %s (mat khau: %s)\n", AP_SSID, AP_PASS);
  Serial.print("   -> noi vao mang do roi vao: http://");
  Serial.println(WiFi.softAPIP());

  WiFi.setAutoReconnect(true);

  if (noiWifi(12000)) {
    configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com");
    veThongBao("Da noi Wi-Fi", WiFi.localIP().toString().c_str());
    Serial.println("DA KET NOI! Co HAI duong vao trang web:");
    Serial.print("  1. Qua mang nha  : http://");
    Serial.println(WiFi.localIP());
  } else {
    veThongBao("Khong co Wi-Fi nao", "Dung song ESP32-Phong");
    Serial.println("KHONG noi duoc Wi-Fi nao trong danh sach.");
    Serial.println("Van chay tiep: man hinh + song rieng. Se thu lai moi 30 giay.");
  }
  Serial.print("  2. Qua song rieng: http://");
  Serial.print(WiFi.softAPIP());
  Serial.printf("   (noi vao Wi-Fi \"%s\")\n", AP_SSID);

  server.on("/",     guiTrangChu);
  server.on("/data", guiJSON);
  server.on("/test", []() {
    bool ok = guiTelegram("Tin nhan thu tu ESP32. Nhiet do: " + String(nhietDo, 1) +
                          " do C | Do am: " + String(doAm, 1) + " %");
    server.send(200, "text/plain; charset=utf-8",
                ok ? "Da gui. Kiem tra Telegram." : "Gui that bai. Xem Serial.");
  });
  server.begin();
}

void loop() {
  kiemTraWifi();
  server.handleClient();
  capNhatCamBien();
  veManHinh();
  luuLichSu();
  kiemTraCanhBao();
  guiLenCloud();
  veManHinh();
  hoiTelegram();
  veManHinh();
}
