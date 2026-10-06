#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "DHT.h"
#include "bi_mat.h"

IPAddress ipTinh   (192, 168, 0,  50);
IPAddress gateway  (192, 168, 0,   1);
IPAddress subnet   (255, 255, 255, 0);
IPAddress dnsServer(192, 168, 0,   1);

#define DHT_PIN  4
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);
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
  if (!isnan(h) && !isnan(t)) { doAm = h; nhietDo = t; daCoSoDo = true; }
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

void guiJSON() {
  String s = "{\"t\":" + String(nhietDo, 1) + ",\"h\":" + String(doAm, 1) + ",\"T\":[";
  for (int i = 0; i < soDiem; i++) { if (i) s += ","; s += String(lichSuT[i], 1); }
  s += "],\"H\":[";
  for (int i = 0; i < soDiem; i++) { if (i) s += ","; s += String(lichSuH[i], 1); }
  s += "],\"c\":\"" + trangThaiCloud + "\"}";
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
<div class="note">Cloud: <span id="cloud">--</span></div>
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

  WiFi.mode(WIFI_STA);
  if (!WiFi.config(ipTinh, gateway, subnet, dnsServer)) {
    Serial.println("CANH BAO: khong dat duoc IP tinh, se dung IP router cap.");
  }
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Dang ket noi Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  Serial.println("\nDA KET NOI!");
  Serial.print(">>> MO TRINH DUYET VA GO: http://");
  Serial.println(WiFi.localIP());

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
  server.handleClient();
  capNhatCamBien();
  luuLichSu();
  kiemTraCanhBao();
  guiLenCloud();
}
