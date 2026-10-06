# Kiến thức IoT — từ con số 0

Ghi lại toàn bộ những gì đã dùng để xây project **ESP32 + DHT11 → web server → Telegram → cloud**, từ lúc chưa biết gì (26/09/2026).

Đây là tài liệu tra cứu: không cần đọc một mạch, cần gì nhảy tới đó.

---

## Mục lục

0. [Nguyên tắc làm việc](#0-nguyên-tắc-làm-việc)
1. [Phần cứng](#1-phần-cứng)
2. [Lập trình vi điều khiển](#2-lập-trình-vi-điều-khiển)
3. [Mạng](#3-mạng)
4. [Web server trên ESP32](#4-web-server-trên-esp32)
5. [Gọi ra internet](#5-gọi-ra-internet)
6. [Kỹ thuật thiết kế](#6-kỹ-thuật-thiết-kế)
7. [Đo lường và vật lý](#7-đo-lường-và-vật-lý)
8. [Công cụ và quy trình](#8-công-cụ-và-quy-trình)
9. [Mua linh kiện thực tế](#9-mua-linh-kiện-thực-tế)
10. [Nhật ký lỗi đã gặp](#10-nhật-ký-lỗi-đã-gặp)
11. [Bản đồ các bài](#11-bản-đồ-các-bài)
12. [Tra cứu nhanh](#12-tra-cứu-nhanh)

---

## 0. Nguyên tắc làm việc

> **Mỗi bước chỉ thêm ĐÚNG MỘT thứ mới.**

Đây là thứ giá trị nhất học được, hơn cả kiến thức kỹ thuật.

```
01_blink      = board + cáp + nạp code      (chưa cảm biến, chưa Wi-Fi)
02_dht_test   = + cảm biến                   (chưa Wi-Fi)
03_wifi_test  = + Wi-Fi                      (chưa cảm biến, chưa web)
04_webserver  = + web server                 (tất cả)
```

Người mới hay nhảy thẳng vào bài cuối, rồi khi không chạy thì không biết lỗi ở đâu: cáp? chân cắm? mật khẩu Wi-Fi? code? Đi tuần tự thì mỗi lần lỗi **biết chắc lỗi nằm trong thứ vừa mới thêm vào**.

Hệ quả: **giữ lại tất cả các bài cũ**. Khi hỏng, luôn có một mốc đã biết là chạy để quay về.

---

## 1. Phần cứng

### 1.1. Cáp USB — loại chỉ sạc vs loại truyền data

Nhìn ngoài y hệt nhau. Cáp chỉ sạc có 2 dây nguồn, cáp data có thêm 2 dây tín hiệu.

**Đèn trên board sáng KHÔNG chứng minh gì** — đèn chỉ báo có điện.

Cách kiểm tra (Linux): chạy `lsusb` trước và sau khi cắm, so sánh. Nếu xuất hiện thiết bị mới và có `/dev/ttyUSB0` → là cáp data.

```bash
./test-cap.sh     # script tự động làm việc này
```

### 1.2. Chip USB-serial

Board ESP32 không nói USB trực tiếp. Nó có một chip trung gian dịch USB ↔ serial:

| Mã trong `lsusb` | Chip | Driver trên Ubuntu |
|---|---|---|
| `10c4:ea60` | CP2102 (Silicon Labs) | Có sẵn |
| `1a86:7523` / `1a86:55d4` | CH340 / CH9102 | Có sẵn |

Board này dùng **CP2102**.

`arduino-cli board list` báo `Board Name: Unknown` là **bình thường** — chip USB-serial không biết nó đang gắn trên board gì, nên ta phải tự khai FQBN.

### 1.3. Quyền truy cập cổng serial (Linux)

`/dev/ttyUSB0` thuộc nhóm `dialout`. User phải ở trong nhóm đó:

```bash
id -nG                      # xem mình thuộc nhóm nào
sudo usermod -aG dialout $USER   # thêm vào nhóm (phải đăng xuất/đăng nhập lại)
```

### 1.4. Chân GPIO — chân nào dùng được

| Chân | Ghi chú |
|---|---|
| **GPIO6 – GPIO11** | ⛔ **TUYỆT ĐỐI KHÔNG DÙNG** — nối trực tiếp tới chip flash chứa chương trình. Dùng là board treo/chết |
| GPIO34 – GPIO39 | Chỉ **đọc** được, không xuất tín hiệu ra được (input-only) |
| GPIO0, 2, 12, 15 | "Strapping pin" — lúc khởi động quyết định chế độ boot. Cắm cảm biến vào có thể làm board không boot |
| GPIO2 | Có LED xanh gắn sẵn trên DevKit V1 |
| **An toàn** | 4, 5, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33 |

Chân I2C mặc định của ESP32: **SDA = GPIO21, SCL = GPIO22**.

### 1.5. Module vs cảm biến trần

- **Cảm biến trần** (DHT11 4 chân) — cần tự gắn thêm điện trở kéo lên 10kΩ
- **Module** (DHT11 3 chân có bo mạch) — đã hàn sẵn điện trở, chỉ cần nối 3 dây

Với người mới: luôn chọn module.

### 1.6. Breadboard

```
       cột:   a  b  c  d  e  │ rãnh │  f  g  h  i  j
  hàng 1      ●──●──●──●──●  │ giữa │  ●──●──●──●──●
  hàng 2      ●──●──●──●──●  │      │  ●──●──●──●──●
```

**Hai quy tắc duy nhất:**

1. **Cùng hàng ngang, cùng bên rãnh** → 5 lỗ là **cùng một điểm điện**
2. **Khác hàng, hoặc khác bên rãnh** → **không nối**

Vì vậy board/chip phải cắm **vắt ngang rãnh giữa**, để chân trái và chân phải không nối tắt.

Khi một chân board chiếm 1 lỗ, **4 lỗ còn lại cùng hàng, cùng bên vẫn nối với chân đó** — đó là chỗ cắm dây jumper. Nên không bao giờ "hết chỗ".

**Hai thanh dọc ở rìa** (dấu `+` đỏ và `–` xanh) là **đường nguồn**, nối dọc suốt chiều dài. Dùng chúng cho 3V3 và GND thì cắm được rất nhiều module:

```
ESP32 chân 3V3 ──1 dây──→ thanh (+)  ──→ mọi module lấy điện từ đây
ESP32 chân GND ──1 dây──→ thanh (−)
```

### 1.7. Loại đầu dây Dupont

| Loại | Dùng khi |
|---|---|
| Đực – Đực | Nối giữa 2 lỗ breadboard |
| **Cái – Đực** | Đầu cái lên chân module (chân module là đực), đầu đực xuống breadboard |
| Cái – Cái | Nối thẳng chân module với chân board, không cần breadboard |

**Chân ESP32 và chân module đều là ĐỰC** → không cắm vào nhau được. Phải qua breadboard hoặc dùng dây có đầu cái.

### 1.8. An toàn khi đấu dây

> **Rút USB → đấu dây → kiểm tra lại bằng mắt → cắm USB**

Cắm nhầm `3V3` vào `GND` khi đang có điện = đoản mạch, có thể chết chip vĩnh viễn.

Không để board trên **miếng xốp đen chống tĩnh điện** khi cấp điện — loại xốp đó dẫn điện nhẹ.

---

## 2. Lập trình vi điều khiển

### 2.1. Cấu trúc chương trình

```cpp
void setup() {
  // Chạy MỘT LẦN khi board bật nguồn
}

void loop() {
  // Chạy LẶP LẠI VÔ TẬN ngay sau đó
}
```

Không có `main()`, không có lệnh kết thúc. Hết `loop()` thì tự chạy lại từ đầu `loop()`, mãi mãi, cho tới khi mất điện.

### 2.2. Các lệnh cơ bản

```cpp
Serial.begin(115200);          // mở kênh nói chuyện với máy tính
pinMode(4, OUTPUT);            // khai báo chân 4 là chân xuất
digitalWrite(4, HIGH);         // đẩy 3.3V ra chân 4
delay(500);                    // dừng 500 mili-giây
millis();                      // số mili-giây kể từ lúc board bật
```

`Serial.begin(115200)` — hai đầu phải cùng tốc độ, lệch là ra ký tự rác `⸮⸮⸮`.

### 2.3. `millis()` thay cho `delay()` — kỹ thuật quan trọng nhất

`delay()` **khoá toàn bộ chương trình**. Trong web server, chèn `delay(2000)` vào `loop()` là trang web lag hoặc không tải được.

Cách "chờ mà không dừng":

```cpp
unsigned long lanCuoi = 0;

void capNhatCamBien() {
  if (millis() - lanCuoi < 2000) return;   // chưa đủ 2 giây thì bỏ qua
  lanCuoi = millis();
  // ... việc cần làm mỗi 2 giây
}
```

Gọi hàm này mỗi vòng `loop()` cũng không sao — nó tự biết khi nào cần làm việc.

Dùng kỹ thuật này cho: đọc cảm biến (2s), lưu lịch sử (10s), hỏi Telegram (5s), gửi cloud (60s), soát Wi-Fi (30s).

### 2.4. `isnan()` — bẫy im lặng

Thư viện DHT **không báo lỗi** khi đọc thất bại, nó trả về `nan` ("not a number"):

```cpp
if (isnan(h) || isnan(t)) {
  Serial.println("LOI: khong doc duoc DHT11");
  return;
}
```

Không kiểm tra thì màn hình in `Nhiet do: nan C` mà không hiểu vì sao.

### 2.5. Mảng (array)

```cpp
float lichSuT[60];       // 60 ô nhớ, đánh số từ 0 đến 59
lichSuT[0] = 33.5;       // ô đầu tiên
lichSuT[59] = 34.0;      // ô cuối cùng
```

Số đầu là **0 chứ không phải 1** — nguồn nhầm lẫn kinh điển.

**Khi mảng đầy — dồn sang trái:**

```cpp
if (soDiem < SO_DIEM) {
  lichSuT[soDiem++] = nhietDo;          // còn chỗ → thêm vào cuối
} else {
  for (int i = 0; i < SO_DIEM - 1; i++)
    lichSuT[i] = lichSuT[i + 1];        // dồn tất cả sang trái 1 ô
  lichSuT[SO_DIEM - 1] = nhietDo;       // số mới vào ô cuối
}
```

Như hàng người xếp hàng: người mới vào cuối, người đầu rời đi, cả hàng nhích lên.

Tốn công dồn 59 ô mỗi 10 giây — với ESP32 là vài micro-giây, không đáng kể. Có cách hiệu quả hơn (*ring buffer*) nhưng khó hiểu hơn. **Giai đoạn đầu: dễ hiểu quan trọng hơn tối ưu.**

### 2.6. Tham chiếu `&`

```cpp
void kiemTraNguong(bool &dangBao, float giaTri, ...) {
  dangBao = true;    // sửa THẲNG vào biến gốc bên ngoài
}
```

Không có `&`: hàm nhận một **bản sao**, sửa bản sao thì biến gốc không đổi — như photo tờ giấy rồi viết lên bản photo.

Có `&`: đưa thẳng tờ gốc vào. Bắt buộc khi hàm cần **ghi nhớ trạng thái** giữa các lần gọi.

### 2.7. Kiểu số — bẫy 32-bit

**Trên ESP32 (vi xử lý 32-bit):**

| Kiểu | Chứa tối đa |
|---|---|
| `long` | 2.147.483.647 |
| `unsigned long` | 4.294.967.295 |
| `long long` / `int64_t` | 9.223.372.036.854.775.807 |

> Trên máy tính Linux 64-bit, `long` chứa tới 9 tỷ tỷ. **Cùng một dòng code, chạy đúng trên máy tính, sai trên vi điều khiển.**

Mọi ID từ dịch vụ ngoài (Telegram chat id, timestamp mili-giây, ID database) **phải dùng `long long`**.

```cpp
long long chatId = u["message"]["chat"]["id"].as<long long>();
static const long long CHU_SO_HUU = atoll(TELEGRAM_CHAT_ID);
```

### 2.8. `String` trên vi điều khiển — cẩn thận

`loop()` chạy vài nghìn lần mỗi giây. Ghép chuỗi ở đó sẽ tạo và huỷ hàng nghìn chuỗi mỗi giây → **phân mảnh bộ nhớ heap**, chạy vài ngày là treo.

```cpp
// SAI: ghép chuỗi mỗi vòng loop
kiemTraNguong(..., "CANH BAO: " + String(nhietDo));

// ĐÚNG: chỉ ghép khi thật sự cần gửi
if (!dangBao && vuot) {
  guiTelegram(String("CANH BAO - ") + ten + ": " + String(giaTri, 1));
}
```

### 2.9. Phạm vi biến `{ }` và bộ nhớ

```cpp
{
  WiFiClientSecure client;   // ~35KB RAM cho phiên TLS
  // ... dùng client
}   // ← hết khối, client bị huỷ, RAM được trả lại

guiTelegram(...);   // giờ mới mở kết nối mới
```

Không bọc `{ }`: hai phiên TLS tồn tại cùng lúc, tốn gấp đôi RAM. Chưa chết ngay nhưng là quả bom hẹn giờ.

Trong C++ gọi là **RAII** — trên thiết bị ít RAM, đây là kỹ thuật sống còn.

---

## 3. Mạng

### 3.1. ESP32 CHỈ bắt được Wi-Fi 2.4GHz

Không có phần cứng cho 5GHz.

> Router đời mới thường phát **cả hai băng tần dưới CÙNG MỘT TÊN**. Điện thoại tự chọn 5GHz và chạy ngon, nên tưởng mạng bình thường — nhưng ESP32 nối mãi không được và **không báo lỗi gì**, chỉ in dấu chấm vô tận.

Cách loại trừ chắc chắn: dùng hotspot điện thoại, ép băng tần 2.4GHz.
- iPhone: bật **"Tối đa khả năng tương thích"**
- Android: Băng tần AP → **2.4 GHz**

### 3.2. Chế độ Wi-Fi

```cpp
WiFi.mode(WIFI_STA);      // Station: ESP32 làm máy khách, xin vào router
WiFi.mode(WIFI_AP);       // Access Point: ESP32 tự phát sóng
WiFi.mode(WIFI_AP_STA);   // CẢ HAI cùng lúc
```

`WIFI_AP_STA` rất hữu ích: vẫn nối router để ra internet, đồng thời phát sóng riêng cho điện thoại nối thẳng vào — bỏ qua được router.

```cpp
WiFi.softAPConfig(apIP, apIP, apSubnet);
WiFi.softAP("ESP32-Phong", "matkhau8kytu");
// Mặc định truy cập tại http://192.168.4.1
```

### 3.3. Vòng lặp chờ kết nối

```cpp
while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
```

Dấu chấm chạy mãi `..........` = **không vào được mạng**, không phải máy treo.

### 3.4. DHCP vs IP tĩnh

**DHCP**: thiết bị hỏi router "cho tôi xin số nhà", router **cho mượn** một thời gian. Router khởi động lại → có thể nhận số khác.

**IP tĩnh**: thiết bị tự tuyên bố "tôi luôn dùng số 50".

```cpp
IPAddress ipTinh   (192, 168, 0,  50);   // DẤU PHẨY, không phải dấu chấm
IPAddress gateway  (192, 168, 0,   1);
IPAddress subnet   (255, 255, 255, 0);
IPAddress dnsServer(192, 168, 0,   1);

WiFi.mode(WIFI_STA);
WiFi.config(ipTinh, gateway, subnet, dnsServer);   // PHẢI gọi TRƯỚC begin()
WiFi.begin(WIFI_SSID, WIFI_PASS);
```

> `WiFi.config()` gọi **sau** `WiFi.begin()` thì không có tác dụng — code trông đúng mà vẫn ra IP cũ. Lỗi kinh điển.

**Chọn số nào:** phải nằm **ngoài dải DHCP** của router. Nếu laptop nhận `.103` và ESP32 nhận `.111` thì dải DHCP nhiều khả năng bắt đầu từ `.100` → chọn `.50`. Chọn trùng dải DHCP sẽ có ngày **xung đột IP**.

Kiểm tra địa chỉ còn trống: `ping -c 2 192.168.0.50` — không ai trả lời là trống.

**Hạn chế:** IP tĩnh gắn chặt với một mạng. Mang board sang mạng khác (hotspot thường là `192.168.43.x`) là không vào được → phải sửa 4 số, hoặc dùng bản DHCP.

### 3.5. Các khái niệm mạng

| Khái niệm | Là gì | Ví như |
|---|---|---|
| **IP** | Số nhà của thiết bị trong mạng | Địa chỉ nhà |
| **Gateway** | Router, cửa ra internet | Cổng chính khu phố |
| **Subnet** `255.255.255.0` | Ranh giới mạng nội bộ: mọi `192.168.0.x` cùng một khu | Ranh giới khu phố |
| **DNS** | Dịch tên miền thành IP | Quyển danh bạ |
| **Cổng (port)** | Một IP chạy nhiều dịch vụ, phân biệt bằng số cổng. **80 = HTTP** | Số phòng trong toà nhà |

`192.168.x.x` là **địa chỉ nội bộ** — chỉ có nghĩa bên trong mạng đó. Ra khỏi nhà là vô nghĩa, như nói "cho tôi tới phòng 111" khi đứng ngoài đường.

### 3.6. RSSI — độ mạnh sóng

Đơn vị dBm, luôn âm, **càng gần 0 càng mạnh**.

| RSSI | Đánh giá |
|---|---|
| −30 … −60 | Rất tốt |
| −60 … −70 | Ổn |
| dưới −80 | Yếu, dễ rớt |

### 3.7. Vì sao không xem được từ ngoài nhà

```
Chỉ trong nhà:
   Điện thoại ──hỏi──> ESP32 (192.168.0.50)
   ✗ Ra khỏi nhà không tìm thấy địa chỉ này

Với cloud:
   ESP32 ──đẩy lên──> ThingSpeak <──xem── Điện thoại (bất cứ đâu)
   ✓ Cả hai bên đều chủ động đi ra ngoài, gặp nhau ở giữa
```

**Mấu chốt: ESP32 gửi đi, không ngồi chờ ai gọi vào.** Router cho thiết bị bên trong gọi ra thoải mái nhưng chặn người ngoài gọi vào — đó là tường lửa, và nó bảo vệ bạn.

### 3.8. Mở cổng router (port forwarding) — tại sao KHÔNG nên

Web server trên ESP32 **không mật khẩu, không HTTPS**. Mở cổng là phơi nó ra internet công cộng, nơi có hàng nghìn máy quét tự động — một cổng mở thường bị tìm thấy trong vài giờ.

Thêm nữa: đường `/test` cho phép **bất kỳ ai gửi tin nhắn Telegram** qua bot của bạn. Và ESP32 không được thiết kế để chịu tấn công.

Cách an toàn hơn: **đẩy dữ liệu ra cloud**, hoặc **VPN về nhà** (Tailscale/WireGuard).

### 3.9. CGNAT

Nhiều nhà mạng không cấp IP public riêng mà dùng chung (dải `100.64.0.0/10`). Khi đó **mở cổng không bao giờ chạy**, dù cấu hình đúng.

```bash
curl -4 -s https://api.ipify.org     # xem IP public
```

Nếu IP nằm trong `100.64.x.x` – `100.127.x.x` → bị CGNAT.

### 3.10. AP Isolation — cách ly thiết bị

Router cho mỗi máy ra internet thoải mái nhưng **không cho chúng nói chuyện với nhau**.

**Dấu hiệu:** ESP32 vẫn đẩy được ThingSpeak và trả lời Telegram, nhưng laptop ping không thấy. Quét cả 254 địa chỉ chỉ thấy mỗi router.

**Chẩn đoán:**

```bash
ip route get 192.168.0.50     # xem gói tin đi đường nào
ip neigh show                 # bảng ARP: thiết bị nào thật sự thấy được
```

Nếu định tuyến đúng (đi thẳng ra `wlo1`) mà vẫn không có hồi âm → tầng Wi-Fi chặn.

**Sửa:** tắt "AP Isolation" trong trang quản trị router (TP-Link: *Wireless → Wireless Advanced*). Mạng khách (Guest Network) **mặc định bật cách ly** — đó là mục đích của nó.

**Nếu không có quyền với router:** cho ESP32 chạy `WIFI_AP_STA`, phát sóng riêng, điện thoại nối thẳng vào `192.168.4.1`.

---

## 4. Web server trên ESP32

### 4.1. Web server là gì

> Một chương trình ngồi chờ. Ai đó hỏi "cho tôi xem trang này", nó gửi lại nội dung.

```cpp
WebServer server(80);              // nghe ở cổng 80 (cổng HTTP mặc định)

server.on("/",     guiTrangChu);   // vào "/" → gọi hàm guiTrangChu
server.on("/data", guiJSON);       // vào "/data" → trả JSON
server.begin();

void loop() {
  server.handleClient();           // BẮT BUỘC gọi liên tục, càng nhanh càng tốt
}
```

**Không được có `delay()` trong `loop()`** của web server.

### 4.2. Vì sao cần 2 đường riêng

Chỉ có trang chủ thì muốn thấy số mới phải bấm F5 — chớp nháy, khó chịu.

Cách làm: trang chủ gửi **một lần**, trong đó có JavaScript cứ **mỗi 3 giây âm thầm gọi `/data`** lấy số mới rồi thay vào chỗ cũ.

```javascript
async function tick(){
  const d = await (await fetch('/data')).json();
  document.getElementById('t').textContent = d.t.toFixed(1);
}
tick(); setInterval(tick, 3000);
```

Kết quả: **số tự cập nhật, trang không nháy**. Kỹ thuật cơ bản của mọi dashboard hiện đại.

### 4.3. JSON

```json
{"t":33.5,"h":61.0,"T":[33.1,33.2,33.5],"H":[62.0,61.0,61.0]}
```

- Chữ thường `t`, `h` — số hiện tại
- Chữ hoa `T`, `H` — **mảng** lịch sử, trong ngoặc vuông

### 4.4. Trang web hiển thị đúng trên điện thoại

```html
<meta name="viewport" content="width=device-width,initial-scale=1">
```

Không có dòng này, điện thoại hiển thị trang như màn hình máy tính rồi thu nhỏ — chữ bé xíu phải zoom.

### 4.5. Cảnh báo "Not secure"

Bình thường, không phải lỗi. Chỉ báo dùng `http://` chứ không phải `https://`. Với trang trong mạng nhà, chỉ hiển thị nhiệt độ, không có mật khẩu → chấp nhận được.

Làm HTTPS trên ESP32 được nhưng phức tạp (cần chứng chỉ) và tốn tài nguyên.

---

## 5. Gọi ra internet

### 5.1. Server vs Client — hai vai ngược nhau

| | Thư viện | Vai trò | Ví như |
|---|---|---|---|
| Bài 1–6 | `WebServer` | **Ngồi chờ** ai đó hỏi | Cửa hàng mở cửa chờ khách |
| Bài 7+ | `HTTPClient` | **Chủ động gọi** đi | Bạn gọi điện cho người khác |

ESP32 làm được **cả hai vai cùng lúc**.

### 5.2. HTTPS và `setInsecure()`

```cpp
WiFiClientSecure client;
client.setInsecure();     // vẫn mã hoá, nhưng BỎ QUA kiểm tra chứng chỉ
```

**Đánh đổi:** dữ liệu vẫn được mã hoá nên người ngoài không đọc trộm. Nhưng nếu ai đó kiểm soát đường mạng, họ có thể giả làm `api.telegram.org` và lấy token.

**Vì sao vẫn dùng:** cách đúng là nhúng chứng chỉ gốc vào code — nhưng chứng chỉ có hạn, hết hạn là ESP32 ngừng gửi được. Với project học tập trong mạng nhà, đây là đánh đổi hợp lý. **Làm sản phẩm thật thì phải làm cho đúng.**

**Chi phí:** thêm TLS làm sketch tăng từ 70% → 80% flash (hơn 120KB).

### 5.3. Hai cách truyền dữ liệu

```
POST + JSON (Telegram):     dữ liệu nằm trong thân request
  Content-Type: application/json
  {"chat_id":"...","text":"..."}

GET + query string (ThingSpeak):  dữ liệu nằm trong URL
  http://api.thingspeak.com/update?api_key=XXX&field1=33.6&field2=60.0
                                   └──────── sau dấu ? là dữ liệu ────────┘
```

Dấu `?` bắt đầu phần tham số, `&` ngăn cách các tham số. GET đơn giản hơn nhưng chỉ hợp dữ liệu ngắn, không ký tự đặc biệt.

### 5.4. HTTP vs HTTPS — khác một chữ, tiết kiệm 120KB

```cpp
WiFiClientSecure client;   // HTTPS — nặng
WiFiClient client;         // HTTP thường — nhẹ hơn 120KB flash
```

ThingSpeak nhận HTTP thường → dùng được bản nhẹ. Telegram bắt buộc HTTPS.

### 5.5. Kiểm tra kết quả — đừng chỉ nhìn mã HTTP

```cpp
int ma = http.GET();
String dap = http.getString();

if (ma == 200 && dap != "0") { ... }   // PHẢI kiểm tra cả nội dung
```

ThingSpeak trả **mã số bản ghi** (1, 2, 3...) nếu thành công và **`"0"`** nếu bị từ chối — nhưng **vẫn trả HTTP 200**. Kiểu lỗi rất dễ bỏ sót.

### 5.6. Polling vs Webhook

| Cách | Cơ chế | Dùng được với ESP32? |
|---|---|---|
| **Webhook** | Telegram **gọi vào** máy chủ của bạn | ❌ Cần địa chỉ công khai có HTTPS hợp lệ |
| **Polling** | Bot **tự hỏi** Telegram: "có gì mới không?" | ✅ Chỉ cần gọi ra internet |

Đây **chính xác** là vấn đề đã gặp ở phần cloud: thiết bị trong nhà gọi ra được, người ngoài không gọi vào được.

### 5.7. `offset` — xác nhận đã nhận

```cpp
"/getUpdates?timeout=0&limit=5&offset=" + String(updateIdCuoi + 1)
```

Mỗi tin nhắn có `update_id` tăng dần. Gọi `getUpdates` trống không → Telegram trả về **toàn bộ tin chưa xác nhận**, lần nào cũng vậy → bot trả lời lặp vô tận.

`offset = update_id_cuối + 1` nghĩa là *"những tin có số nhỏ hơn con số này tôi xử lý rồi, xoá đi"*.

Kiểu **"xác nhận đã nhận" (acknowledgement)** này gặp lại ở MQTT, RabbitMQ, Kafka và mọi hệ thống hàng đợi tin nhắn.

### 5.8. Đọc JSON tiết kiệm RAM

```cpp
JsonDocument boLoc;
boLoc["result"][0]["update_id"] = true;
boLoc["result"][0]["message"]["chat"]["id"] = true;
boLoc["result"][0]["message"]["text"] = true;

deserializeJson(doc, http.getStream(),
                DeserializationOption::Filter(boLoc));
```

Telegram trả về rất nhiều thứ thừa (tên, họ, ảnh, ngôn ngữ, is_premium...). Đọc hết vào RAM có thể **tràn bộ nhớ** và làm ESP32 khởi động lại.

Bộ lọc giữ đúng 3 trường, vứt phần còn lại **ngay khi đọc**. `http.getStream()` đọc trực tiếp từ luồng mạng, không nạp cả cục vào RAM.

> Trên máy tính không cần nghĩ tới. Trên vi điều khiển 320KB RAM, đây là khác biệt giữa **chạy được** và **treo**.

### 5.9. Bảo mật — mặc định từ chối

```cpp
if (chatId != CHU_SO_HUU) {
  Serial.printf("Bo qua lenh tu chat la: %lld\n", chatId);
  return;
}
```

Username bot là công khai — ai cũng tìm thấy và nhắn được. Không có dòng này, người lạ hỏi `/nhietdo` là bot trả lời ngay.

> Quy tắc chung cho mọi thiết bị nối mạng: **mặc định là từ chối, chỉ cho phép thứ mình biết rõ.**

### 5.10. Tách bí mật khỏi mã nguồn

```cpp
#include "bi_mat.h"    // chứa TELEGRAM_TOKEN, CHAT_ID, THINGSPEAK_KEY
```

```cpp
// bi_mat.h — quyền 600, KHÔNG đưa lên GitHub
#pragma once
#define TELEGRAM_TOKEN   "..."
#define TELEGRAM_CHAT_ID "..."
```

Thói quen nên tập từ đầu: đưa code lên GitHub hay gửi cho người khác chỉ cần bỏ đúng một file ra, không phải rà từng dòng.

---

## 6. Kỹ thuật thiết kế

### 6.1. Máy trạng thái — chống spam

Viết `if (nhietDo >= 35) guiTelegram(...)` thì mỗi vòng `loop()` gửi một tin — vài nghìn tin mỗi phút, Telegram khoá bot ngay.

```cpp
bool dangCanhBao = false;   // ghi nhớ đang ở trạng thái nào
```

Chỉ gửi đúng lúc **chuyển trạng thái**: bình thường → cảnh báo, và ngược lại.

### 6.2. Hysteresis — ngưỡng kép

```cpp
#define NHIET_CAO      35.0     // vượt lên → báo
#define NHIET_CAO_VE   34.0     // tụt xuống → hết báo
```

> Dùng chung một mốc 35 thì nhiệt độ dao động `35.0 → 34.9 → 35.0...` sẽ sinh ra một cặp tin nhắn mỗi lần — vẫn spam.
>
> Hai mốc lệch nhau 1 độ: nhiệt độ phải thực sự giảm hẳn mới báo "bình thường".

Kỹ thuật này dùng khắp nơi: máy lạnh, tủ lạnh, bình nóng lạnh.

**Ngưỡng hai đầu:** độ ẩm cần cảnh báo cả cao (>80%, nấm mốc) lẫn thấp (<40%, khô da).

### 6.3. Gộp code lặp lại

```cpp
void kiemTraNguong(bool &dangBao, float giaTri, float mocVao, float mocRa,
                   const char* ten, const char* donVi) {
  bool nguongCao = (mocVao > mocRa);     // tự suy ra hướng so sánh
  bool vuot = nguongCao ? (giaTri >= mocVao) : (giaTri <= mocVao);
  bool ve   = nguongCao ? (giaTri <= mocRa)  : (giaTri >= mocRa);
  ...
}

kiemTraNguong(dangBaoNhietCao, nhietDo, 35.0, 34.0, "nhiet do cao", " do C");
kiemTraNguong(dangBaoAmCao,    doAm,    80.0, 75.0, "do am cao",    " %");
kiemTraNguong(dangBaoAmThap,   doAm,    40.0, 45.0, "do am thap",   " %");
```

> Copy-paste 3 khối `if` gần giống nhau thì sau này đổi định dạng tin nhắn phải sửa **3 chỗ**. Quên một chỗ là sinh lỗi khó tìm.

Mẹo: nếu mốc vào **lớn hơn** mốc ra (35 và 34) → ngưỡng "cao quá". Ngược lại (40 và 45) → ngưỡng "thấp quá".

### 6.4. Lưới an toàn Wi-Fi

`WiFi.begin()` chỉ gọi **một lần** trong `setup()`. Router khởi động lại lúc 3h sáng → board có thể nằm im có điện mà không mạng, cho tới khi có người phát hiện.

```cpp
void kiemTraWifi() {
  if (millis() - lanKiemTra < 30000) return;   // 30 giây soát 1 lần
  lanKiemTra = millis();

  if (WiFi.status() == WL_CONNECTED) { lucBatDauMat = 0; return; }

  if (lucBatDauMat == 0) lucBatDauMat = millis();

  if (millis() - lucBatDauMat > 300000) {      // mất quá 5 phút
    Serial.flush();                            // đẩy hết chữ ra trước khi reset
    delay(200);
    ESP.restart();                             // tự nhấn nút EN
  }

  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}
```

Ba mức tăng dần: **còn mạng → kệ** · **mất → thử nối lại mỗi 30 giây** · **mất quá 5 phút → khởi động lại board**.

Chi tiết:
- Biến `lucBatDauMat` dùng số 0 làm tín hiệu "đang bình thường", khác 0 là mốc thời gian — một biến làm hai việc
- `ESP.restart()` hợp lý trên vi điều khiển: khởi động lại hết 5 giây, chỉ mất lịch sử trong RAM
- `Serial.flush()` trước reset, nếu không thì mất manh mối duy nhất
- Gọi `kiemTraWifi()` ở **dòng đầu `loop()`**, trước mọi việc cần mạng

### 6.5. Giới hạn kiến trúc — `loop()` bị nghẽn

| Việc | Tần suất | Chặn |
|---|---|---|
| Hỏi Telegram | 5 giây/lần | ~1 giây |
| Gửi cloud | 60 giây/lần | ~1 giây |
| Gửi cảnh báo | khi vượt ngưỡng | ~1 giây |

ESP32 dành khoảng **20% thời gian** đứng chờ mạng, không trả lời trang web.

**Lời giải:** ESP32 có **2 lõi CPU**. Đẩy việc mạng sang lõi thứ hai (FreeRTOS), lõi chính chỉ lo cảm biến và web server. → việc cần làm tiếp theo.

### 6.6. Đọc cảm biến trong `loop()`, không phải khi có người xem

```cpp
void loop() {
  kiemTraWifi();
  server.handleClient();
  capNhatCamBien();     // đọc liên tục, kể cả khi không ai mở trang
  luuLichSu();
  kiemTraCanhBao();
  guiLenCloud();
  hoiTelegram();
}
```

Chỉ đọc khi có người truy cập thì lúc không mở trang, ESP32 **không ghi lại gì** — mở lại sẽ thấy biểu đồ trống hoác.

---

## 7. Đo lường và vật lý

### 7.1. Độ phân giải ≠ độ chính xác

| | DHT11 | DHT22 | AHT20 |
|---|---|---|---|
| Sai số nhiệt độ | ±2°C | ±0.5°C | ±0.3°C |
| Sai số độ ẩm | ±5% | ±2–5% | ±2% |
| Độ phân giải độ ẩm | 1% (số nguyên) | 0.1% | 0.024% |
| Dải đo độ ẩm | 20–90% | 0–100% | 0–100% |

> **Độ phân giải** = số nhỏ nhất nó phân biệt được. **Độ chính xác** = số đó lệch thực tế bao nhiêu. Hai thứ khác nhau.

Biểu đồ độ ẩm DHT11 trông **"vuông góc"** nhảy giữa 59 và 60 — đó là độ phân giải 1%, không phải lỗi vẽ.

Cảm biến rẻ cho biết **xu hướng** (đang nóng lên hay mát đi) chính xác hơn nhiều so với **con số tuyệt đối**.

### 7.2. DHT11 rất chậm

Chỉ tự đo lại mỗi ~2 giây. Đọc nhanh hơn thì trả dữ liệu cũ hoặc báo lỗi. **Giới hạn vật lý của cảm biến, không phải của code.**

Lần đọc đầu tiên ngay sau khi bật thường sai (ra `0.3 C | 0.0 %`) vì cảm biến chưa kịp đo.

### 7.3. Trong nhà nóng hơn ngoài trời

App thời tiết lấy số từ **trạm khí tượng**, đo theo chuẩn: **ngoài trời, trong bóng râm, cách mặt đất 1.5–2m, thoáng gió**. Có khi cách nhà vài km.

Ban ngày tường, mái, sàn hấp thụ nhiệt. Đêm xuống, không khí ngoài trời nguội nhanh nhưng **khối bê tông nhả nhiệt từ từ suốt đêm**. Chênh 3–6°C vào buổi tối ở Việt Nam là bình thường.

**Cách kiểm chứng không cần cảm biến thứ hai:** tắt board vài tiếng cho nguội hẳn rồi bật lại. Nếu số đo ngay giây đầu đã cao → không phải do cảm biến tự nóng.

### 7.4. Điểm sương (dew point)

Nhiệt độ mà hơi nước bắt đầu ngưng tụ. **Chỉ phụ thuộc lượng hơi nước thật sự**, không đổi khi không khí nóng lên hay lạnh đi → là **phép thử chéo** rất tốt.

Công thức Magnus:

```
γ  = ln(RH/100) + (17.625 × T) / (243.04 + T)
Td = 243.04 × γ / (17.625 − γ)
```

Ví dụ thật: 31.6°C + 69% RH → điểm sương **25.2°C**.

Dùng để kiểm tra số đo có hợp lý không: nếu phòng thật sự chỉ 26°C với cùng lượng hơi nước đó thì độ ẩm phải là **95%** — mà 95% thì tường rịn nước, quần áo ẩm dính. Không thấy vậy → số 31.6°C đáng tin hơn.

### 7.5. Điều hoà vắt nước ra khỏi không khí

Muốn làm lạnh xuống dưới điểm sương thì hơi nước **bắt buộc phải ngưng tụ** — đó chính là nước nhỏ ra từ ống cục nóng.

Dàn lạnh thường ở 8–14°C, nên điểm sương phòng tụt về quanh đó:

| Nhiệt độ phòng | Độ ẩm dự kiến (điểm sương 12°C) |
|---|---|
| 28°C | ~37% |
| 26°C | ~42% |
| 24°C | ~47% |
| 22°C | ~53% |

Nên bật điều hoà là **độ ẩm cắm đầu xuống**, có khi trước cả khi nhiệt độ kịp giảm nhiều.

**Con số trên remote là MỤC TIÊU, không phải số đo.** Cảm biến điều hoà nằm ở dàn lạnh trên cao; khí lạnh chìm xuống, khí nóng bay lên (chênh 2–4°C giữa trần và sàn); gió thổi ra chỉ 12–16°C nên đặt cảm biến trong luồng gió sẽ ra số sai; và phòng cần 20–45 phút mới ổn định — nhiều khi **không bao giờ chạm 20°C**.

### 7.6. Áp suất khí quyển → dự báo mưa

Quy tắc khí tượng: áp suất **tụt hơn 2 hPa trong 3 giờ** → khả năng cao mưa hoặc chuyển trời trong 3–6 tiếng.

---

## 8. Công cụ và quy trình

### 8.1. Ba thành phần môi trường

| | Là gì |
|---|---|
| **`arduino-cli`** | Dịch code C++ thành `.bin` rồi đẩy vào ESP32 |
| **Core ESP32** | ESP32 **không phải** Arduino — chip khác, tập lệnh khác. "Core" giúp viết code kiểu Arduino mà chạy trên ESP32. Nặng ~1GB |
| **Thư viện** | DHT, ArduinoJson... — lo phần giao thức phức tạp |

### 8.2. Lệnh hay dùng

```bash
arduino-cli board listall | grep -i devkit     # tìm FQBN
arduino-cli board list                         # xem cổng đang cắm
arduino-cli core list                          # core đã cài
arduino-cli lib list                           # thư viện đã cài
arduino-cli lib install "ArduinoJson"

FQBN=esp32:esp32:esp32doit-devkit-v1
arduino-cli compile -b $FQBN 09_bot_hai_chieu
arduino-cli upload  -b $FQBN -p /dev/ttyUSB0 09_bot_hai_chieu
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

**Script tự viết:**

```bash
./test-cap.sh              # kiểm tra cáp có truyền data không
./nap.sh 09_bot_hai_chieu  # biên dịch + nạp + mở Serial
./xem.sh                   # CHỈ xem Serial, không nạp lại
./telegram-setup.sh        # lấy Chat ID, ghi bi_mat.h
```

### 8.3. Đọc Serial khi `arduino-cli monitor` không ghi ra file

`arduino-cli monitor` chỉ xuất ra terminal thật, ghi vào file sẽ ra 0 byte. Cách khác:

```bash
stty -F /dev/ttyUSB0 115200 raw -echo -hupcl
timeout 75 cat /dev/ttyUSB0 > serial.log
```

Lưu ý: mở bằng `cat` **không reset board**, nên chỉ bắt được chữ in ra từ lúc đó. Muốn xem từ đầu thì nhấn nút `EN`.

### 8.4. Phân vùng flash

4MB flash chia thành:

| Vùng | Kích thước | Dùng làm gì |
|---|---|---|
| `nvs` | 20 KB | Lưu cài đặt nhỏ, giữ khi mất điện |
| `otadata` | 8 KB | Ghi nhớ đang chạy bản nào |
| **`app0`** | **1.25 MB** | **Chương trình của bạn** |
| `app1` | 1.25 MB | Chỗ dự phòng cho cập nhật qua Wi-Fi (OTA) |
| `spiffs` | 1.375 MB | Chỗ lưu file |
| `coredump` | 64 KB | Ghi hiện trường khi crash |

Đó là lý do biên dịch báo `Maximum is 1310720 bytes` — chính là `app0`, không phải cả 4MB.

**ESP32 chỉ giữ MỘT chương trình.** Nạp mới là đè lên cũ.

**Nếu hết chỗ:** đổi kiểu chia ổ đĩa sang `huge_app` (3MB, mất khả năng OTA):

```bash
arduino-cli compile -b 'esp32:esp32:esp32:PartitionScheme=huge_app' 09_bot_hai_chieu
# 81% → 34%
```

Lưu ý: board `esp32doit-devkit-v1` **không có menu chọn phân vùng**, phải dùng `esp32:esp32:esp32` (ESP32 Dev Module) — cùng chip, khác tên gọi.

### 8.5. Lỗi nạp thường gặp

| Hiện tượng | Xử lý |
|---|---|
| `Failed to connect` | **Giữ nút `BOOT`**, chạy lại lệnh nạp, giữ tới khi thấy `Writing at...` |
| Serial ra ký tự rác | Sai baudrate, hoặc nhấn `EN` |
| `Permission denied` | User chưa ở nhóm `dialout` |
| Board treo, có điện mà không mạng | Nhấn `EN` — đó là nút reset |

---

## 9. Mua linh kiện thực tế

### 9.1. Những cái bẫy

| Bẫy | Cách tránh |
|---|---|
| **Chân chưa hàn** | Ảnh có thanh chân nằm rời bên cạnh = chưa hàn. Tìm chữ "đã hàn sẵn" |
| **BMP280 ≠ BME280** | BMP280 **không đo độ ẩm**. Nhìn chữ khắc trên chip: `BME` hay `BMP`. "BME280 giá 25k" = gần như chắc chắn hàng BMP280 |
| **OLED SPI vs I2C** | Mua loại **I2C 4 chân**, không phải SPI 7 chân |
| **Thứ tự chân OLED** | Có 2 kiểu: `GND·VCC·SCL·SDA` và `VCC·GND·SCL·SDA`. Cắm nhầm là **cháy màn** |
| **Giá gạch ngang** | Người bán tự điền, không ai kiểm chứng. Chỉ so giá cùng model ở nhiều shop |
| **Shopee Mall ≠ rẻ hơn** | Mall đảm bảo chính hãng + đổi trả, nhưng thường **đắt hơn** shop nhỏ |
| **Biến thể cùng listing** | Cùng một món, màu vàng 247k / màu đỏ 115k — khác nhau chỉ vỏ nhựa |

### 9.2. Chọn shop

- **Đã bán > 100** (quan trọng nhất)
- Đánh giá **4.8★ trở lên**
- Giao **2–4 ngày** (shop trong nước)
- **Mua gộp một shop** để trả ship một lần

Dấu hiệu rủi ro: shop mới lập, đã bán 0, vài người theo dõi, tỉ lệ phản hồi vô nghĩa kiểu `999900%`.

### 9.3. An toàn

**Mỏ hàn** — mũi ~350°C, bỏng tức thì và rất sâu. Luôn đặt vào đế khi rời tay. Hàn nơi thoáng khí (khói nhựa thông hại phổi). Mũi giữ nhiệt rất lâu sau khi rút điện.

**Đồng hồ vạn năng rẻ (30–150k)** — **đừng dùng đo điện lưới 220V**. Chúng thường CAT II hoặc không ghi; gặp xung điện áp đột biến có thể phóng hồ quang. Đo mạch 3.3V/5V thì hoàn toàn an toàn. Cần đo 220V thì mua loại ghi rõ **CAT III 600V** trở lên.

**Relay điều khiển 220V** — khoan. Bắt đầu bằng quạt USB 5V hoặc dải LED 12V.

**Module 3.3V** — bo nhỏ không có mạch hạ áp (như AHT20+BMP280 bo tím) thì **chỉ cắm vào chân `3V3`**, không cắm `VIN` (5V).

### 9.4. Cảm biến nên tránh

**MQ-135** (quảng cáo "đo CO₂") — không đo được CO₂, chỉ là điện trở đổi giá trị theo đủ thứ khí, cần hiệu chuẩn phức tạp, ngốn điện.

**Cảm biến độ ẩm đất loại điện trở** — ăn mòn, hỏng sau vài tuần.

### 9.5. Thứ tự ưu tiên mua

1. **Đồng hồ vạn năng** (~150–250k) — là **công cụ**, dùng cả chục năm. Quan trọng nhất: kiểm tra **cầu thiếc** giữa VCC và GND *trước khi cắm điện*
2. **Mỏ hàn** (~100–150k) — mở khoá mọi module rẻ. Chọn bộ có **đế đỡ** (an toàn) và **ống hút thiếc** (sửa sai)
3. Cảm biến mới, màn hình
4. Cảm biến đắt tiền (PM2.5 ~450k, CO₂ ~350–500k)

---

## 10. Nhật ký lỗi đã gặp

### 10.1. Cáp USB chỉ sạc
**Hiện tượng:** cắm vào không thấy `/dev/ttyUSB0`.
**Cách tìm:** so sánh `lsusb` trước/sau khi cắm.
**Bài học:** đèn board sáng không chứng minh gì.

### 10.2. Board treo — có điện mà không vào mạng
**Hiện tượng:** ESP32 có điện, firmware đã nạp đủ (`No changed sectors found ... verified`), nhưng không trả lời trên mạng và Serial không in gì.
**Cách tìm:** `ls /dev/ttyUSB*` (board còn cắm), `ping` (không thấy), nạp lại → lệnh nạp **hard reset** board và nó chạy lại bình thường.
**Kết luận:** không xác định được nguyên nhân chính xác vì Serial không in gì để lần.
**Hệ quả:** đã thêm **lưới an toàn Wi-Fi** (mục 6.4) để tự phục hồi nếu lặp lại.
**Bài học:** nhấn `EN` là đủ, không cần nạp lại code.

### 10.3. Bot Telegram im lặng — tràn số 32-bit
**Hiện tượng:** bot nhận lệnh nhưng không trả lời. Không báo lỗi gì.
**Nguyên nhân:** Chat ID là `8xxxxxxxxx`, vượt giới hạn `long` 32-bit (2.147.483.647). ArduinoJson gặp số không vừa kiểu thì **trả về 0 mà không báo lỗi** → mọi so sánh Chat ID đều sai → bot tưởng người gửi là người lạ nên im lặng (đúng theo dòng bảo mật).
**Sửa:** dùng `long long` + `atoll()`.
**Bài học:** code y hệt chạy đúng trên Linux 64-bit nhưng sai trên ESP32. Và tính năng bảo mật hoạt động hoàn hảo — chỉ là nó chặn nhầm chính chủ.

### 10.4. Cảnh báo thiếu độ ẩm
**Hiện tượng:** chỉ cảnh báo nhiệt độ, độ ẩm bị bỏ quên dù vẫn đo và vẫn vẽ biểu đồ.
**Sửa:** thêm 2 ngưỡng độ ẩm (cao/thấp) và **gộp 3 khối `if` lặp lại thành một hàm dùng chung**.

### 10.5. Router chặn thiết bị (AP Isolation)
**Hiện tượng:** `192.168.0.50` không vào được từ cả laptop lẫn điện thoại, trong khi ESP32 vẫn đẩy ThingSpeak và trả lời Telegram bình thường.
**Cách tìm:**
- Hỏi `/trangthai` qua Telegram → bot báo IP **vẫn là** `192.168.0.50` → loại trừ khả năng đổi IP
- `ip route get 192.168.0.50` → định tuyến đúng, đi thẳng ra `wlo1`, không vòng qua Tailscale
- Quét cả 254 địa chỉ → **chỉ thấy mỗi router**
**Kết luận:** router bật cách ly thiết bị. Không phải lỗi code, ESP32, hay laptop.
**Sửa:** tắt AP Isolation trên router — nhưng không có mật khẩu quản trị → chuyển sang `WIFI_AP_STA`, ESP32 tự phát sóng riêng.
**Bài học:** **bot Telegram trở thành công cụ debug** — nó đi qua máy chủ Telegram nên vẫn hỏi được khi đường LAN đứt.

---

## 11. Bản đồ các bài

| Bài | Dòng | Thêm gì |
|---|---|---|
| `01_blink` | 18 | Nạp code, `setup`/`loop`, Serial |
| `02_dht_test` | 32 | Đọc cảm biến, `isnan`, giới hạn 2 giây |
| `03_wifi_test` | 33 | Wi-Fi STA, lấy IP, RSSI |
| `04_webserver` | 85 | Web server, route, JSON, `fetch`, `millis()` |
| `05_ip_tinh` | 98 | IP tĩnh, gateway/subnet/DNS |
| `06_bieu_do` | 194 | Mảng, lịch sử trong RAM, vẽ SVG |
| `07_telegram` | 281 | HTTP client, HTTPS, hysteresis, máy trạng thái |
| `08_cloud` | 327 | ThingSpeak, HTTP thường, GET query string |
| `09_bot_hai_chieu` | ~500 | Polling, ArduinoJson + bộ lọc, `long long`, lưới an toàn Wi-Fi, `WIFI_AP_STA` |

Mỗi bài chạy độc lập → hỏng chỗ nào luôn có mốc cũ để quay về.

---

## 12. Tra cứu nhanh

### Phần cứng hiện có

```
ESP32 DevKit V1, 30 chân, ESP32-WROOM-32, cổng micro-USB
Chip USB-serial: CP2102  (10c4:ea60)
MAC: 8c:94:df:6c:b4:24
FQBN: esp32:esp32:esp32doit-devkit-v1
```

### Đấu dây DHT11

```
DHT11 VCC   →  ESP32 3V3     (chân thứ 1 của hàng có chữ 3V3)
DHT11 GND   →  ESP32 GND     (chân thứ 2)
DHT11 DATA  →  ESP32 D4      (chân thứ 5)
```

Đếm từ chân trên cùng: `3V3 · GND · D15 · D2 · D4 · RX2 · TX2 · D5 ...`

### Mạng

```
Wi-Fi     <TEN_WIFI> (2.4GHz)
ESP32     192.168.0.50   (IP tĩnh)
Gateway   192.168.0.1    (TP-Link)
Subnet    255.255.255.0
Laptop    192.168.0.103
AP riêng  ESP32-Phong → http://192.168.4.1
```

### Ngưỡng cảnh báo

```
Nhiệt độ cao   : vượt 35.0°C → báo, tụt 34.0°C → hết
Độ ẩm cao      : vượt 80%    → báo, tụt 75%    → hết
Độ ẩm thấp     : dưới 40%    → báo, lên 45%    → hết
```

### Chu kỳ

```
Đọc cảm biến      2 giây
Lưu lịch sử      10 giây   (60 điểm = 10 phút)
Trang web làm mới 3 giây
Hỏi Telegram      5 giây
Gửi cloud        60 giây
Soát Wi-Fi       30 giây   (mất >5 phút → ESP.restart())
```

### Lệnh Telegram

```
/nhietdo     nhiệt độ và độ ẩm hiện tại
/trangthai   tình trạng thiết bị (uptime, RSSI, IP, cloud, số lần nối lại)
/bieudo      link xem biểu đồ
```

### Cloud

```
ThingSpeak kênh "Phong ngu", ID <ID_KENH>
https://thingspeak.mathworks.com/channels/<ID_KENH>

Đọc dữ liệu (thêm timezone cho giờ Việt Nam):
https://api.thingspeak.com/channels/<ID_KENH>/feeds.json?api_key=<READ_KEY>&results=100&timezone=Asia/Bangkok
```

Biểu đồ trên trang kênh lấy giờ từ **trình duyệt**, không chỉnh trong ThingSpeak được. API mặc định trả **UTC**.

---

*Cập nhật lần cuối: 06/10/2026 — sau bài 09, trước khi lắp AHT20 + BMP280, OLED, BH1750.*
