#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"
#include "bi_mat.h"

#define DHT_PIN  4
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

float nhietDo = 0, doAm = 0;
unsigned long lanDocCuoi = 0;

void capNhatCamBien() {
  if (millis() - lanDocCuoi < 2000) return;
  lanDocCuoi = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) { doAm = h; nhietDo = t; }
}

void guiJSON() {
  capNhatCamBien();
  char buf[96];
  snprintf(buf, sizeof(buf), "{\"t\":%.1f,\"h\":%.1f}", nhietDo, doAm);
  server.send(200, "application/json", buf);
}

void guiTrangChu() {
  capNhatCamBien();
  String html = R"HTML(<!DOCTYPE html><html lang="vi"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 DHT11</title><style>
body{font-family:system-ui,sans-serif;background:#0f172a;color:#e2e8f0;margin:0;
display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:100vh;gap:20px}
.card{background:#1e293b;border-radius:16px;padding:28px 40px;text-align:center;min-width:180px}
.val{font-size:48px;font-weight:700;margin:8px 0}
.lbl{font-size:14px;color:#94a3b8;text-transform:uppercase;letter-spacing:1px}
#t{color:#fb923c} #h{color:#38bdf8}
</style></head><body>
<div class="card"><div class="lbl">Nhiet do</div><div class="val"><span id="t">--</span>&deg;C</div></div>
<div class="card"><div class="lbl">Do am</div><div class="val"><span id="h">--</span>%</div></div>
<script>
async function tick(){
  try{ const r = await fetch('/data'); const d = await r.json();
       document.getElementById('t').textContent = d.t.toFixed(1);
       document.getElementById('h').textContent = d.h.toFixed(1); }catch(e){}
}
tick(); setInterval(tick, 3000);
</script></body></html>)HTML";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Dang ket noi Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  Serial.println("\nDA KET NOI!");
  Serial.print(">>> MO TRINH DUYET VA GO: http://");
  Serial.println(WiFi.localIP());

  server.on("/",     guiTrangChu);
  server.on("/data", guiJSON);
  server.begin();
}

void loop() {
  server.handleClient();
}
