// ============================================================
// 🛒 EINKAUFSLISTE - FINAL (POPRAWIONE KOLORY I UKŁAD)
// ============================================================

#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <vector>

const char* ssid = "TU_WPISZ_NAZWE_WIFI";
const char* password = "TU_WPISZ_HASLO_WIFI";

// Data ostatniego automatycznego nocnego restartu ("YYYYMMDD"),
// zeby restart o 4:00 zdarzyl sie tylko RAZ danej nocy (przetrwa
// restart ESP32 - trzymane na karcie SD).
String lastAutoRestartDate = "";

// ---------- PINY KARTY SD (VSPI) ----------
#define SD_CS   5
#define SD_SCK  18
#define SD_MOSI 23
#define SD_MISO 19
SPIClass sdSPI(VSPI);

WebServer server(80);

// ============================================================
// PRODUKTE
// ============================================================
struct Item {
  String name;
  int quantity;
  bool bought;
};

Item pennyItems[200];
Item aldiItems[200];
Item edekaItems[200];

int pennyCount = 0;
int aldiCount = 0;
int edekaCount = 0;

// ============================================================
// RECHNUNG (Paragon Summen)
// ============================================================
float pennyTotal = 0;
float aldiTotal = 0;
float edekaTotal = 0;
float backereiTotal = 0;
float sonstigeTotal = 0;
float grandTotal = 0;

float monthlyBudget = 700.00;
float remainingBudget = 700.00;

std::vector<float> pennyHistory;
std::vector<float> aldiHistory;
std::vector<float> edekaHistory;
std::vector<float> backereiHistory;
std::vector<float> sonstigeHistory;

int currentMonth = -1;
int currentYear = -1;

// ============================================================
// AUTOMATYCZNY BACKUP (cala lista zakupow + Monatliches Budget)
// ============================================================
String lastBackupDate = ""; // "YYYYMMDD" ostatniego zrobionego backupu

struct MonthlyBill {
  int month;
  int year;
  float penny;
  float aldi;
  float edeka;
  float backerei;
  float sonstige;
  float total;
};
MonthlyBill billHistory[12];
int billCount = 0;

// ============================================================
// SPEICHERN / LADEN
// ============================================================
void saveData() {
  File f = SD.open("/penny.txt", "w");
  if (f) {
    f.println(pennyCount);
    for (int i = 0; i < pennyCount; i++) {
      f.println(pennyItems[i].name);
      f.println(pennyItems[i].quantity);
      f.println(pennyItems[i].bought ? 1 : 0);
    }
    f.close();
  }
  f = SD.open("/aldi.txt", "w");
  if (f) {
    f.println(aldiCount);
    for (int i = 0; i < aldiCount; i++) {
      f.println(aldiItems[i].name);
      f.println(aldiItems[i].quantity);
      f.println(aldiItems[i].bought ? 1 : 0);
    }
    f.close();
  }
  f = SD.open("/edeka.txt", "w");
  if (f) {
    f.println(edekaCount);
    for (int i = 0; i < edekaCount; i++) {
      f.println(edekaItems[i].name);
      f.println(edekaItems[i].quantity);
      f.println(edekaItems[i].bought ? 1 : 0);
    }
    f.close();
  }
}

void loadData() {
  if (SD.exists("/penny.txt")) {
    File f = SD.open("/penny.txt", "r");
    if (f) {
      pennyCount = f.parseInt(); f.readStringUntil('\n');
      for (int i = 0; i < pennyCount && i < 200; i++) {
        pennyItems[i].name = f.readStringUntil('\n'); pennyItems[i].name.trim();
        pennyItems[i].quantity = f.parseInt(); f.readStringUntil('\n');
        pennyItems[i].bought = f.parseInt() == 1; f.readStringUntil('\n');
      }
      f.close();
    }
  }
  if (SD.exists("/aldi.txt")) {
    File f = SD.open("/aldi.txt", "r");
    if (f) {
      aldiCount = f.parseInt(); f.readStringUntil('\n');
      for (int i = 0; i < aldiCount && i < 200; i++) {
        aldiItems[i].name = f.readStringUntil('\n'); aldiItems[i].name.trim();
        aldiItems[i].quantity = f.parseInt(); f.readStringUntil('\n');
        aldiItems[i].bought = f.parseInt() == 1; f.readStringUntil('\n');
      }
      f.close();
    }
  }
  if (SD.exists("/edeka.txt")) {
    File f = SD.open("/edeka.txt", "r");
    if (f) {
      edekaCount = f.parseInt(); f.readStringUntil('\n');
      for (int i = 0; i < edekaCount && i < 200; i++) {
        edekaItems[i].name = f.readStringUntil('\n'); edekaItems[i].name.trim();
        edekaItems[i].quantity = f.parseInt(); f.readStringUntil('\n');
        edekaItems[i].bought = f.parseInt() == 1; f.readStringUntil('\n');
      }
      f.close();
    }
  }
}

void saveTotals() {
  File f = SD.open("/totals.txt", "w");
  if (f) {
    f.println(currentMonth);
    f.println(currentYear);
    f.println(pennyTotal);
    f.println(aldiTotal);
    f.println(edekaTotal);
    f.println(backereiTotal);
    f.println(sonstigeTotal);
    f.println(grandTotal);
    f.println(monthlyBudget);
    f.println(remainingBudget);
    f.close();
  }
}

void loadTotals() {
  if (SD.exists("/totals.txt")) {
    File f = SD.open("/totals.txt", "r");
    if (f) {
      currentMonth = f.parseInt(); f.readStringUntil('\n');
      currentYear = f.parseInt(); f.readStringUntil('\n');
      pennyTotal = f.parseFloat(); f.readStringUntil('\n');
      aldiTotal = f.parseFloat(); f.readStringUntil('\n');
      edekaTotal = f.parseFloat(); f.readStringUntil('\n');
      backereiTotal = f.parseFloat(); f.readStringUntil('\n');
      sonstigeTotal = f.parseFloat(); f.readStringUntil('\n');
      grandTotal = f.parseFloat(); f.readStringUntil('\n');
      monthlyBudget = f.parseFloat(); f.readStringUntil('\n');
      remainingBudget = f.parseFloat(); f.readStringUntil('\n');
      f.close();
    }
  } else {
    remainingBudget = monthlyBudget;
  }
}

// Zapisuje aktualna zawartosc billHistory[0..billCount-1] na SD.
// Wydzielone z saveMonthlyBill, zeby moc wywolac to samo tez po imporcie CSV.
void writeHistoryFile() {
  File f = SD.open("/history.txt", "w");
  if (f) {
    f.println(billCount);
    for (int i = 0; i < billCount; i++) {
      f.println(billHistory[i].month);
      f.println(billHistory[i].year);
      f.println(billHistory[i].penny);
      f.println(billHistory[i].aldi);
      f.println(billHistory[i].edeka);
      f.println(billHistory[i].backerei);
      f.println(billHistory[i].sonstige);
      f.println(billHistory[i].total);
    }
    f.close();
  }
}

void saveMonthlyBill(int month, int year, float penny, float aldi, float edeka, float backerei, float sonstige, float total) {
  for (int i = 11; i > 0; i--) billHistory[i] = billHistory[i-1];
  billHistory[0] = {month, year, penny, aldi, edeka, backerei, sonstige, total};
  if (billCount < 12) billCount++;
  writeHistoryFile();
}

void loadHistory() {
  if (SD.exists("/history.txt")) {
    File f = SD.open("/history.txt", "r");
    if (f) {
      billCount = f.parseInt(); f.readStringUntil('\n');
      if (billCount > 12) billCount = 12;
      for (int i = 0; i < billCount; i++) {
        billHistory[i].month = f.parseInt(); f.readStringUntil('\n');
        billHistory[i].year = f.parseInt(); f.readStringUntil('\n');
        billHistory[i].penny = f.parseFloat(); f.readStringUntil('\n');
        billHistory[i].aldi = f.parseFloat(); f.readStringUntil('\n');
        billHistory[i].edeka = f.parseFloat(); f.readStringUntil('\n');
        billHistory[i].backerei = f.parseFloat(); f.readStringUntil('\n');
        billHistory[i].sonstige = f.parseFloat(); f.readStringUntil('\n');
        billHistory[i].total = f.parseFloat(); f.readStringUntil('\n');
      }
      f.close();
    }
  }
}

void calculateTotals() { 
  grandTotal = pennyTotal + aldiTotal + edekaTotal + backereiTotal + sonstigeTotal;
  remainingBudget = monthlyBudget - grandTotal;
}

void checkNewMonth() {
  time_t now = time(nullptr);
  
  // Zabezpieczenie przed rokiem 1970 (czas niezsynchronizowany)
  if (now < 1000000000) {  // około roku 2001
    Serial.println("⏳ Czas niezsynchronizowany - pomijam checkNewMonth()");
    return;
  }
  
  struct tm* tm_now = localtime(&now);
  int month = tm_now->tm_mon + 1;
  int year = tm_now->tm_year + 1900;
  
  if (currentMonth == -1) {
    currentMonth = month;
    currentYear = year;
    loadTotals();
    loadHistory();
    calculateTotals();
  } else if (month != currentMonth) {
    saveMonthlyBill(currentMonth, currentYear, pennyTotal, aldiTotal, edekaTotal, backereiTotal, sonstigeTotal, grandTotal);
    pennyHistory.clear();
    aldiHistory.clear();
    aldiHistory.clear();
    edekaHistory.clear();
    backereiHistory.clear();
    sonstigeHistory.clear();
    pennyTotal = 0;
    aldiTotal = 0;
    edekaTotal = 0;
    backereiTotal = 0;
    sonstigeTotal = 0;
    grandTotal = 0;
    remainingBudget = monthlyBudget;
    currentMonth = month;
    currentYear = year;
    saveTotals();
  }
}

// ============================================================
// AUTOMATYCZNY BACKUP - implementacja
// ============================================================

// Zwraca dzisiejsza date jako "YYYYMMDD" (8 znakow - celowo bez
// myslnikow, zeby nazwa pliku zmiescila sie w formacie 8.3,
// ktorego niektore karty SD/FAT wymagaja), albo "" jesli czas
// nie jest jeszcze zsynchronizowany z NTP.
String todayDateString() {
  time_t now = time(nullptr);
  if (now < 1000000000) return "";
  struct tm* tm_now = localtime(&now);
  char buf[9];
  snprintf(buf, sizeof(buf), "%04d%02d%02d", tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday);
  return String(buf);
}

// Wczytuje date ostatniego backupu (przetrwa restart ESP32)
// Nazwa pliku celowo krotka (8.3-kompatybilna).
void loadBackupMeta() {
  if (SD.exists("/bkupmeta.txt")) {
    File f = SD.open("/bkupmeta.txt", "r");
    if (f) {
      lastBackupDate = f.readStringUntil('\n');
      lastBackupDate.trim();
      f.close();
    }
  }
}

void saveBackupMeta() {
  File f = SD.open("/bkupmeta.txt", "w");
  if (f) {
    f.println(lastBackupDate);
    f.close();
  }
}

// Wczytuje/zapisuje date ostatniego nocnego auto-restartu (przetrwa
// restart ESP32 - inaczej restart o 4:00 moglby sie zapetlic).
void loadRestartMeta() {
  if (SD.exists("/rstmeta.txt")) {
    File f = SD.open("/rstmeta.txt", "r");
    if (f) {
      lastAutoRestartDate = f.readStringUntil('\n');
      lastAutoRestartDate.trim();
      f.close();
    }
  }
}

void saveRestartMeta() {
  File f = SD.open("/rstmeta.txt", "w");
  if (f) {
    f.println(lastAutoRestartDate);
    f.close();
  }
}

// Zwraca kolejny numer dla backupu, gdy czas NTP nie jest jeszcze
// zsynchronizowany (zeby recznemu backupowi NIE przeszkadzal brak
// czasu - liczony licznik trzyma sie w osobnym pliku).
String nextFallbackBackupName() {
  unsigned long counter = 0;
  if (SD.exists("/bkupcnt.txt")) {
    File cf = SD.open("/bkupcnt.txt", "r");
    if (cf) { counter = cf.parseInt(); cf.close(); }
  }
  counter++;
  File cf = SD.open("/bkupcnt.txt", "w");
  if (cf) { cf.println(counter); cf.close(); }
  char buf[9];
  snprintf(buf, sizeof(buf), "B%07lu", counter % 10000000UL);
  return String(buf);
}

// Buduje CAlA tresc backupu (3 sekcje: listy zakupow, biezacy
// Monatliches Budget, historia) jako jeden String. Uzywane zarowno
// do zapisu na SD, jak i do pobrania pliku wprost w przegladarce.
String buildFullBackupCsv() {
  calculateTotals();
  String csv = "";

  csv += "=== LISTY ZAKUPOW ===\n";
  csv += "Sklep;Nazwa;Ilosc;Kupione\n";
  for (int i = 0; i < aldiCount; i++) {
    csv += "Aldi;" + aldiItems[i].name + ";" + String(aldiItems[i].quantity) + ";" + (aldiItems[i].bought ? "1" : "0") + "\n";
  }
  for (int i = 0; i < edekaCount; i++) {
    csv += "Edeka;" + edekaItems[i].name + ";" + String(edekaItems[i].quantity) + ";" + (edekaItems[i].bought ? "1" : "0") + "\n";
  }
  for (int i = 0; i < pennyCount; i++) {
    csv += "Penny;" + pennyItems[i].name + ";" + String(pennyItems[i].quantity) + ";" + (pennyItems[i].bought ? "1" : "0") + "\n";
  }

  csv += "\n=== BUDZET MIESIECZNY (biezacy miesiac) ===\n";
  csv += "Miesiac;Rok;Aldi (€);Edeka (€);Penny (€);Piekarnia (€);Norma-Netto (€);Suma (€);Budzet (€)\n";
  csv += String(currentMonth) + ";" + String(currentYear) + ";" +
         String(aldiTotal, 2) + ";" + String(edekaTotal, 2) + ";" + String(pennyTotal, 2) + ";" +
         String(backereiTotal, 2) + ";" + String(sonstigeTotal, 2) + ";" + String(grandTotal, 2) + ";" +
         String(monthlyBudget, 2) + "\n";

  csv += "\n=== HISTORIA POPRZEDNICH MIESIECY ===\n";
  csv += "Miesiac;Rok;Aldi (€);Edeka (€);Penny (€);Piekarnia (€);Norma-Netto (€);Suma (€)\n";
  for (int i = 0; i < billCount; i++) {
    csv += String(billHistory[i].month) + ";" + String(billHistory[i].year) + ";" +
           String(billHistory[i].aldi, 2) + ";" + String(billHistory[i].edeka, 2) + ";" +
           String(billHistory[i].penny, 2) + ";" + String(billHistory[i].backerei, 2) + ";" +
           String(billHistory[i].sonstige, 2) + ";" + String(billHistory[i].total, 2) + "\n";
  }

  return csv;
}

// Tworzy JEDEN plik CSV na karcie SD z kompletem danych: wszystkie
// 3 listy zakupow + biezacy Monatliches Budget + cala historia.
// Zwraca true/false - zeby przycisk BACKUP mogl pokazac PRAWDZIWY
// wynik, a nie zawsze "sukces" (tak jak wczesniej).
bool createFullBackup() {
  String date = todayDateString();
  // Recznemu backupowi brak synchronizacji NTP nie powinien
  // przeszkadzac - wtedy uzywamy numerka zamiast daty w nazwie.
  String filename = (date != "") ? (date + ".csv") : (nextFallbackBackupName() + ".csv");

  if (!SD.exists("/backup")) {
    if (!SD.mkdir("/backup")) {
      Serial.println("BLAD: nie udalo sie utworzyc folderu /backup na karcie SD!");
      return false;
    }
  }

  String path = "/backup/" + filename; // np. /backup/20260912.csv albo /backup/B0000001.csv
  File f = SD.open(path.c_str(), "w");
  if (!f) {
    Serial.println("BLAD: nie udalo sie utworzyc pliku backupu: " + path);
    return false;
  }

  f.print(buildFullBackupCsv());
  f.close();

  if (date != "") {
    lastBackupDate = date;
    saveBackupMeta();
  }
  Serial.println("Backup utworzony: " + path);
  return true;
}

// Sprawdza raz dziennie (albo od razu po starcie, jesli dzis
// jeszcze nie bylo backupu), czy trzeba zrobic nowy backup.
// (To jest backup AUTOMATYCZNY - tu czekanie na NTP ma sens,
// zeby nie robic kilku backupow dziennie przez pomylke).
void checkAutoBackup() {
  String date = todayDateString();
  if (date == "") return; // czas jeszcze nie zsynchronizowany
  if (date != lastBackupDate) {
    createFullBackup();
  }
}

// ============================================================
// STRONA GŁÓWNA (lista zakupów)
// ============================================================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
 <meta charset="UTF-8">
 <link rel="icon" type="image/png" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Ctext x='0' y='20' font-size='20' fill='white'%3E🐒%3C/text%3E%3C/svg%3E">
 <meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
 <meta name="apple-mobile-web-app-capable" content="yes">
 <title>🛒 Lista zakupów</title>
 <style>
 .drag-handle {
  display: none !important;
}
*{margin:0;padding:0;box-sizing:border-box}
:root{--bg:linear-gradient(135deg,#667eea,#764ba2);--card:#fff;--text:#333}
body.dark{--bg:linear-gradient(135deg,#1a1a2e,#16213e);--card:#2d2d3f;--text:#eee}
body{background:var(--bg);font-family:'Segoe UI',Arial;padding:15px;transition:all 0.3s}
.container{max-width:600px;margin:0 auto;min-height:100vh}
.title-row{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px}
.title-left{display:flex;align-items:center;gap:10px}
h1{color:white;font-size:1.5em;margin:0}
.sd-dot{font-size:1.1em;color:#888;transition:color 0.3s}
.sd-dot.ok{color:#4CAF50;text-shadow:0 0 6px #4CAF50}
.sd-dot.bad{color:#ff4757;text-shadow:0 0 6px #ff4757}
.theme-btn{background:rgba(255,255,255,0.2);border:none;width:44px;height:44px;border-radius:50%;font-size:1.3em;cursor:pointer}
.buttons{display:flex;gap:10px;margin-bottom:20px;justify-content:center}
.bill-btn{background:rgba(255,255,255,0.2);border:none;padding:12px 20px;border-radius:30px;background:#FF9800;font-size:1em;color:white;cursor:pointer}
.shops{display:flex;gap:10px;margin-bottom:20px}
.shop{flex:1;background:rgba(255,255,255,0.2);border-radius:15px;padding:12px;text-align:center;cursor:pointer;color:white;font-weight:bold}
.shop.active{background:rgba(255,255,255,0.4);transform:scale(1.02)}
.categories{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:20px}
.cat-btn{background:rgba(255,255,255,0.2);border:none;border-radius:16px;padding:8px 4px;text-align:center;cursor:pointer;color:white;font-size:0.7em}
.cat-btn span{font-size:1.3em;display:block}
.add-section{background:rgba(255,255,255,0.15);border-radius:20px;padding:15px;margin-bottom:20px}
.add-row{display:flex;gap:10px}
.add-row input{flex:1;padding:12px;border-radius:30px;border:none}
.add-row button{background:#fd79a8;color:white;border:none;padding:12px 20px;border-radius:30px;cursor:pointer;font-weight:bold}
.product-list{overflow-y:visible}
.product{background:var(--card);border-radius:15px;padding:8px 10px;margin-bottom:8px;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;cursor:grab}
.product:active{cursor:grabbing}
.product.bought{background:#2196f3;color:white}
.product-left{display:flex;align-items:center;gap:8px;flex:2}
.drag-handle{font-size:1.8em;cursor:grab;color:#aaa;padding:4px}
.drag-handle:active{cursor:grabbing}
.check{width:32px;height:32px;border-radius:50%;border:3px solid #00b894;display:flex;align-items:center;justify-content:center;cursor:pointer;background:white;flex-shrink:0}
.check.checked{background:#00b894;color:white}
.product-info{display:flex;flex-direction:column}
.name{font-size:1.1em;font-weight:bold;cursor:pointer;color:var(--text)}
.product.bought .name{color:white}
.quantity-num{font-size:0.8em;color:var(--text);margin-top:3px}
.product.bought .quantity-num{color:white}
.product-right{display:flex;align-items:center;gap:12px}
.quantity-control{display:flex;align-items:center;gap:4px;background:rgba(0,0,0,0.08);border-radius:20px;padding:3px 8px}
.quantity-btn{background:#667eea;color:white;border:none;width:24px;height:24px;border-radius:50%;cursor:pointer;font-size:0.9em;line-height:1}
.quantity-btn.minus{background:#ff4757}
.quantity-num-big{font-weight:bold;min-width:25px;text-align:center;font-size:0.9em;color:var(--text)}
.product.bought .quantity-num-big{color:white}
.delete{background:#ff4757;color:white;border:none;width:36px;height:36px;border-radius:50%;cursor:pointer;font-size:1.1em}
.modal{display:none;position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.5);backdrop-filter:blur(8px);justify-content:center;align-items:center}
.modal-content{background:var(--card);border-radius:30px;padding:25px;width:90%;max-width:400px}
.modal-content h3{color:var(--text);text-align:center}
.modal-content button{background:#4CAF50;color:white;border:none;padding:12px;border-radius:25px;width:100%;margin-top:8px}
.product-list-modal{max-height:300px;overflow-y:auto}
.product-item{padding:12px;border-bottom:1px solid rgba(0,0,0,0.1);cursor:pointer;color:var(--text)}
.footer{text-align:center;color:white;margin-top:20px;font-size:0.7em}
</style>
</head>
<body>
<div class="container">
<div class="title-row">
<div class="title-left">
<h1>🛒 Lista zakupów</h1>
<span id="sdDot" class="sd-dot" title="Karta SD: sprawdzam...">●</span>
</div>
<button class="theme-btn" onclick="toggleTheme()">🌙</button>
</div>
<div class="buttons">
<button class="bill-btn" onclick="window.open('/rechnung', '_blank')">🧾 RACHUNEK</button>
<button class="bill-btn" onclick="exportShoppingList()" style="background:#2196F3">📥 EKSPORTUJ</button>
<button class="bill-btn" onclick="importShoppingList()" style="background:#4CAF50">📤 IMPORTUJ</button>
<input type="file" id="importFile" accept=".csv" style="display:none" onchange="doImport()">
</div>
<div class="shops">
<div class="shop" id="shopAldi" onclick="selectShop('aldi')">🔵 Aldi</div>
<div class="shop" id="shopEdeka" onclick="selectShop('edeka')">🟢 Edeka</div>
<div class="shop" id="shopPenny" onclick="selectShop('penny')">🟡 Penny</div>
</div>
<div class="categories">
<div class="cat-btn" onclick="showCategory('gemuse')"><span>🥬</span>Warzywa</div>
<div class="cat-btn" onclick="showCategory('obst')"><span>🍎</span>Owoce</div>
<div class="cat-btn" onclick="showCategory('brot')"><span>🥖</span>Pieczywo</div>
<div class="cat-btn" onclick="showCategory('milch')"><span>🥛</span>Nabiał</div>
<div class="cat-btn" onclick="showCategory('fleisch')"><span>🥩</span>Mięso</div>
<div class="cat-btn" onclick="showCategory('marmelade')"><span>🧉</span>Dżemy</div>
<div class="cat-btn" onclick="showCategory('kekse')"><span>🍪</span>Ciastka</div>
<div class="cat-btn" onclick="showCategory('nudeln')"><span>🍜</span>Makaron</div>
<div class="cat-btn" onclick="showCategory('hülsenfrüchte')"><span>🫘</span>Strączkowe</div>
<div class="cat-btn" onclick="showCategory('fertig')"><span>🥰</span>Gotowe dania</div>
<div class="cat-btn" onclick="showCategory('pulver')"><span>🥗</span>Przyprawy</div>
<div class="cat-btn" onclick="showCategory('haus')"><span>🧴</span>Dom</div>
<div class="cat-btn" onclick="showCategory('kase')"><span>🧀</span>Ser</div>
<div class="cat-btn" onclick="showCategory('vegan')"><span>🌱</span>Wegańskie</div>
<div class="cat-btn" onclick="showCategory('sonstige')"><span>💫</span>Inne</div>
</div>
<div class="add-section">
<div class="add-row">
<input type="text" id="itemName" placeholder="Nazwa produktu" autocomplete="off">
<button onclick="addToCurrentShop()">➕</button>
</div>
</div>
<div class="product-list" id="list"></div>
<div class="footer">✨ Dotknij produktu = kupione | ☰ = przeciągnij, aby posortować ✨</div>
</div>
<div id="productModal" class="modal" onclick="closeModal()">
<div class="modal-content" onclick="event.stopPropagation()">
<h3 id="modalTitle">Produkty</h3>
<div id="modalList" class="product-list-modal"></div>
<button onclick="closeModal()">Zamknij</button>
</div>
</div>
<script>
let currentShop = "penny";
let isDark = localStorage.getItem('theme') === 'dark';   
let products = [];
let dragSrc = null;

const produkte = {
  gemuse: [{name:"Ogórek",emoji:"🥒"},{name:"Pomidor",emoji:"🍅"},{name:"Fasolka szparagowa, mrożona",emoji:"🫛"},{name:"Sałata",emoji:"🥬"},{name:"Marchewka",emoji:"🥕"},{name:"Ziemniaki",emoji:"🥔"},{name:"Cebula",emoji:"🧅"},{name:"Włoszczyzna",emoji:"🥬"},{name:"Dymka (zielona cebulka)",emoji:"🪴"},{name:"Papryka",emoji:"🫑"},{name:"Brokuł",emoji:"🥦"},{name:"Kalafior",emoji:"🥦"},{name:"Kapusta",emoji:"🥬"},{name:"Cukinia",emoji:"🥒"},{name:"Pomidorki koktajlowe",emoji:"🍅"}],
  obst: [{name:"Jabłko",emoji:"🍎"},{name:"Banan",emoji:"🍌"},{name:"Truskawka",emoji:"🍓"},{name:"Pomarańcza",emoji:"🍊"},{name:"Cytryna",emoji:"🍋"},{name:"Winogrona",emoji:"🍇"},{name:"Arbuz",emoji:"🍉"},{name:"Kiwi",emoji:"🥝"},{name:"Gruszka",emoji:"🍐"},{name:"Rodzynki",emoji:"🍇"},{name:"Czereśnie",emoji:"🍒"}],
  brot: [{name:"Chleb biały",emoji:"🍞"},{name:"Chleb żytni",emoji:"🍞"},{name:"Bułki",emoji:"🥖"},{name:"Bagietka",emoji:"🥖"},{name:"Rogaliki",emoji:"🥐"},{name:"Precel",emoji:"🥨"},{name:"Chleb tostowy",emoji:"🍞"},{name:"Chleb chrupki",emoji:"🍞"}],
  milch: [{name:"Mleko",emoji:"🥛"},{name:"Ryż na mleku",emoji:"🍚"},{name:"Mleko owsiane",emoji:"🥛"},{name:"Jogurt",emoji:"🥤"},{name:"Twaróg",emoji:"🥄"},{name:"Masło",emoji:"🧈"},{name:"Śmietana",emoji:"🥛"},{name:"Kefir",emoji:"🥛"},{name:"Maślanka",emoji:"🥛"},{name:"Jajka",emoji:"🥚"},{name:"Śmietanka sojowa",emoji:"🥛"},{name:"Serek wiejski",emoji:"🍛"},{name:"Serek śmietankowy",emoji:"🍚"}],
  fleisch: [{name:"Pieczeń wieprzowa",emoji:"🥩"},{name:"Kotlet",emoji:"🥩"},{name:"Łopatka",emoji:"🥩"},{name:"Gulasz",emoji:"🥩"},{name:"Pierś z kurczaka",emoji:"🍗"},{name:"Kurczak",emoji:"🍗"},{name:"Wołowina",emoji:"🥩"},{name:"Mięso mielone",emoji:"🥩"},{name:"Kiełbasa",emoji:"🌭"},{name:"Szynka",emoji:"🥩"},{name:"Boczek",emoji:"🥓"},{name:"Kotleciki mielone",emoji:"🫓"},{name:"Kiełbasa wegetariańska",emoji:"🥩"}],
  marmelade: [{name:"Masło orzechowe",emoji:"🥜"},{name:"Dżem truskawkowy",emoji:"🍓"},{name:"Borówki",emoji:"🫐"},{name:"Dżem malinowy",emoji:"🍓"},{name:"Dżem morelowy",emoji:"🍑"}],
  kekse: [{name:"Ciastka cytrynowe",emoji:"🥮"},{name:"Płatki kukurydziane",emoji:"🥣"},{name:"Płatki owsiane",emoji:"🌾"}],
  nudeln: [{name:"Spaghetti",emoji:"🍝"},{name:"Makaron",emoji:"🍜"},{name:"Ryż",emoji:"🍚"},{name:"Płaty lasagne",emoji:"📁"}],
  hülsenfrüchte: [{name:"Fasola",emoji:"🫘"},{name:"Nasiona chia",emoji:"𓇢"},{name:"Groszek",emoji:"🫛"},{name:"Ciecierzyca",emoji:"🫛"},{name:"Kukurydza",emoji:"🌽"},{name:"Soczewica",emoji:"🍲"}],
  fertig: [{name:"Pizza",emoji:"🍕"},{name:"Kotleciki mielone",emoji:"🫓"},{name:"Nuggetsy",emoji:"🍗"},{name:"Ryba",emoji:"🐟"},{name:"Sznycel",emoji:"🥩"},{name:"Leberkäse (pieczony pasztet)",emoji:"🧇"}],
  pulver: [{name:"Ketchup",emoji:"🥫🍅"},{name:"Makaron smażony (danie instant)",emoji:"🍝"},{name:"Przyprawa Yam Yam",emoji:"🥗"},{name:"Chili (danie instant)",emoji:"🌶"},{name:"Majonez",emoji:"🍶"},{name:"Pieprz",emoji:"🧂"},{name:"Sos pomidorowy (3 małe opakowania)",emoji:"🍅"},{name:"Sól",emoji:"🧂"},{name:"Cukier",emoji:"⬜"},{name:"Cukier puder",emoji:"💭"},{name:"Papryka mielona",emoji:"🌶️"},{name:"Musztarda",emoji:"🌭"},{name:"Pietruszka",emoji:"🌿"},{name:"Koperek",emoji:"🪴"},{name:"Zioła włoskie",emoji:"🤌"},{name:"Cynamon",emoji:"🪵"},{name:"Sól do frytek",emoji:"🧂"},{name:"Bulion warzywny",emoji:"♨️"},{name:"Czosnek",emoji:"🧄"},{name:"Maggi",emoji:"🪄"}],
  haus: [{name:"Papier toaletowy",emoji:"🧻"},{name:"Płyn do płukania ust",emoji:"💧"},{name:"Chusteczki nawilżane (toaletowe)",emoji:"🌬️"}, {name:"Płyn do płukania Lenor",emoji:"🧴"},{name:"Ręczniki papierowe",emoji:"🧻"},{name:"Mydło",emoji:"🧼"},{name:"Szampon",emoji:"🧴"},{name:"Płyn do naczyń",emoji:"🧴"},{name:"Proszek do prania",emoji:"🧴"},{name:"Worki na śmieci",emoji:"🗑️"}],
  kase: [{name:"Ser",emoji:"🧀"},{name:"Ser Gouda",emoji:"🧀"},{name:"Ser w kawałku",emoji:"🧀"},{name:"Serek śmietankowy",emoji:"🧀"},{name:"Ser tarty",emoji:"🧀"},{name:"Cheddar",emoji:"🧀"},{name:"Feta",emoji:"🧀"},{name:"Mozzarella",emoji:"🧀"},{name:"Parmezan",emoji:"🧀"},{name:"Ser topiony",emoji:"🧀"}],
  vegan: [{name:"Tofu",emoji:"🌱"},{name:"Kiełbasa wegetariańska",emoji:"🌱"},{name:"Kawałki sojowe",emoji:"🌱"},{name:"Tofu",emoji:"🧈"},{name:"Tortille duże (na burrito)",emoji:"🌮"},{name:"Mielone wegetariańskie",emoji:"🌱"},{name:"Mleko sojowe",emoji:"🌱"},{name:"Seitan",emoji:"🌱"},{name:"Falafel",emoji:"🌱"},{name:"Tempeh",emoji:"🌱"},{name:"Sos sojowy",emoji:"🌱"},{name:"Jogurt sojowy",emoji:"🥛"},{name:"Tahini",emoji:"🥙"},{name:"Płaty nori (do sushi)",emoji:"🥢"},{name:"Ser wegański",emoji:"🌱"},{name:"Jogurt wegański",emoji:"🌱"}],
  sonstige: [{name:"Tytoń",emoji:"🚬"},{name:"Herbata Adama",emoji:"😎"},{name:"Steam",emoji:"💲"},{name:"Kawa",emoji:"🍵"}]
};

function setTheme() { if(isDark) document.body.classList.add('dark'); else document.body.classList.remove('dark'); }
function toggleTheme() { isDark = !isDark; localStorage.setItem('theme', isDark ? 'dark' : 'light'); setTheme(); }
setTheme();

function selectShop(shop) {
  currentShop = shop;
  document.getElementById('shopPenny').classList.remove('active');
  document.getElementById('shopAldi').classList.remove('active');
  document.getElementById('shopEdeka').classList.remove('active');
  let id = 'shop' + shop.charAt(0).toUpperCase() + shop.slice(1);
  document.getElementById(id).classList.add('active');
  loadProducts();
}

function showCategory(category) {
  let productList = produkte[category];
  let title = "";
  switch(category) {
    case "gemuse": title = "🥬 Warzywa 🥬"; break;
    case "obst": title = "🍎 Owoce 🍎"; break;
    case "brot": title = "🥖 Pieczywo 🥖"; break;
    case "milch": title = "🥛 Nabiał 🥛"; break;
    case "fleisch": title = "🥩 Mięso 🥩"; break;
    case "marmelade": title = "🧉 Dżemy 🧉"; break;
    case "kekse": title = "🍪 Ciastka 🍪"; break;
    case "nudeln": title = "🍜 Makaron 🍜"; break;
    case "hülsenfrüchte": title = "🫘 Strączkowe 🫘"; break;
    case "fertig": title = "🥰 Gotowe dania 🥰"; break;
    case "pulver": title = "🥗 Przyprawy 🥗"; break;
    case "haus": title = "🧴 Dom 🧴"; break;
    case "kase": title = "🧀 Ser 🧀"; break;
    case "vegan": title = "🌱 Wegańskie 🌱"; break;
    case "sonstige": title = "💫 Inne 💫"; break;
    default: title = category;
  }
  let modalList = document.getElementById('modalList');
  modalList.innerHTML = '';
  productList.forEach(p => { modalList.innerHTML += `<div class="product-item" onclick="addProductFromModal('${p.emoji} ${p.name}')">${p.emoji} ${p.name}</div>`; });
  document.getElementById('modalTitle').innerHTML = title;
  document.getElementById('productModal').style.display = 'flex';
}

function addProductFromModal(productName) {
  let input = document.getElementById('itemName');
  if(input.value) input.value = productName + ', ' + input.value;
  else input.value = productName;
  closeModal();
  input.focus();
}
function closeModal() { document.getElementById('productModal').style.display = 'none'; }

function loadProducts() { 
  fetch('/list?shop=' + currentShop)
    .then(r => r.json())
    .then(data => { products = data; renderList(); });
}

function dragStart(e) {
  dragSrc = this;
  e.dataTransfer.setData('text/plain', this.getAttribute('data-id'));
  this.style.opacity = '0.5';
}
function dragEnd(e) { this.style.opacity = ''; }
function dragOver(e) { e.preventDefault(); }
function dragDrop(e) {
  e.preventDefault();
  let fromId = parseInt(dragSrc.getAttribute('data-id'));
  let toId = parseInt(this.getAttribute('data-id'));
  if(fromId !== toId) {
    fetch('/move?shop=' + currentShop + '&from=' + fromId + '&to=' + toId)
      .then(() => loadProducts());
  }
  this.style.opacity = '';
}

function renderList() {
  const container = document.getElementById('list');
  container.innerHTML = '';
  if(products.length === 0) {
    container.innerHTML = '<div style="text-align:center;color:white;padding:30px">📭 Brak produktów</div>';
    return;

  }
  for(let i = 0; i < products.length; i++) {
    let p = products[i];
    let checkClass = p.bought ? 'checked' : '';
    let checkMark = p.bought ? '✓' : '';
    let boughtClass = p.bought ? 'bought' : '';
    
    let div = document.createElement('div');
    div.className = `product ${boughtClass}`;
    div.setAttribute('draggable', 'true');
    div.setAttribute('data-id', p.id);
    div.setAttribute('data-index', i);
    
    div.innerHTML = `
  <div class="product-left">
    <div class="drag-handle" draggable="false">☰</div>
    <div class="check ${checkClass}" onclick="toggleBought(${p.id})">${checkMark}</div>
    <div class="product-info">
      <div class="name" onclick="toggleBought(${p.id})">${escapeHtml(p.name)}</div>
      <div class="quantity-num">${p.qty} szt.</div>
    </div>
  </div>
  <div class="product-right">
    <div class="quantity-control">
      <button class="quantity-btn minus" onclick="event.stopPropagation(); changeQuantity(${p.id}, -1)">-</button>
      <span class="quantity-num-big">${p.qty}</span>
      <button class="quantity-btn" onclick="event.stopPropagation(); changeQuantity(${p.id}, 1)">+</button>
    </div>
    <button class="delete" onclick="event.stopPropagation(); deleteProduct(${p.id})">🗑️</button>
  </div>
`;
    div.addEventListener('dragstart', dragStart);
    div.addEventListener('dragend', dragEnd);
    div.addEventListener('dragover', dragOver);
    div.addEventListener('drop', dragDrop);
    container.appendChild(div);
  }
}

function changeQuantity(id, delta) { fetch('/quantity?shop=' + currentShop + '&id=' + id + '&delta=' + delta).then(() => loadProducts()); }
function toggleBought(id) { fetch('/toggle?shop=' + currentShop + '&id=' + id).then(() => loadProducts()); }
function addToCurrentShop() {
  let name = document.getElementById('itemName').value;
  if(!name) { alert('Podaj nazwę!'); return; }
  fetch('/add?shop=' + currentShop + '&name=' + encodeURIComponent(name)).then(() => { loadProducts(); document.getElementById('itemName').value = ''; });
}
function deleteProduct(id) { if(confirm('Usunąć?')) fetch('/delete?shop=' + currentShop + '&id=' + id).then(() => loadProducts()); }
function escapeHtml(text) { if(!text) return ''; return text.replace(/[&<>]/g, function(m) { if(m === '&') return '&amp;'; if(m === '<') return '&lt;'; if(m === '>') return '&gt;'; return m; }); }

selectShop('penny');

// Sprawdza co jakis czas, czy karta SD faktycznie odpowiada, i
// pokazuje to jako zielona/czerwona kropke obok tytulu.
function checkSdStatus() {
  fetch('/sdstatus').then(r => r.json()).then(d => {
    const dot = document.getElementById('sdDot');
    if (d.ok) {
      dot.classList.add('ok'); dot.classList.remove('bad');
      dot.title = 'Karta SD: OK';
    } else {
      dot.classList.add('bad'); dot.classList.remove('ok');
      dot.title = 'Karta SD: BŁĄD - sprawdź kartę!';
    }
  }).catch(() => {
    const dot = document.getElementById('sdDot');
    dot.classList.add('bad'); dot.classList.remove('ok');
    dot.title = 'Karta SD: brak połączenia z ESP32';
  });
}
checkSdStatus();
setInterval(checkSdStatus, 10000);

function exportShoppingList() { let shop = currentShop; fetch('/exportshop?shop=' + shop).then(r => r.text()).then(data => { const blob = new Blob([data], {type: 'text/csv'}); const link = document.createElement('a'); link.href = URL.createObjectURL(blob); link.download = shop + '_lista_zakupow.csv'; link.click(); alert('✅ Eksportowano ' + shop); }); } function importShoppingList() { document.getElementById('importFile').click(); } function doImport() { let file = document.getElementById('importFile').files[0]; if (!file) return; let reader = new FileReader(); reader.onload = function(e) { let data = e.target.result; fetch('/importshop?shop=' + currentShop, { method: 'POST', headers: {'Content-Type': 'text/csv'}, body: data }).then(() => { alert('✅ Importowano!'); loadProducts(); }); }; reader.readAsText(file); }
</script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

// ============================================================
// STRONA RECHNUNG (z budżetem i sonstige)
// ============================================================
void handleRechnung() {
  calculateTotals();
  
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>🧾 Rachunek</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{--bg:linear-gradient(135deg,#667eea,#764ba2);--card:#fff;--text:#333}
body.dark{--bg:linear-gradient(135deg,#1a1a2e,#16213e);--card:#2d2d3f;--text:#eee}
body{background:var(--bg);font-family:'Segoe UI',Arial;padding:20px;min-height:100vh;transition:all 0.3s}
.container{max-width:500px;margin:0 auto}
.header{display:flex;justify-content:center;align-items:center;margin-bottom:20px}
h1{color:white;font-size:1.5em}
.btn-group{display:flex;gap:10px;flex-wrap:wrap;justify-content:center;width:100%}
.theme-btn,.export-btn,.import-btn{background:rgba(255,255,255,0.2);border:none;width:44px;height:44px;border-radius:50%;font-size:1.3em;cursor:pointer}
.export-btn{width:auto;padding:0 15px;border-radius:30px;background:#2196F3;font-size:0.9em;color:white}
.import-btn{width:auto;padding:0 15px;border-radius:30px;background:#4CAF50;font-size:0.9em;color:white}
.budget-card{background:rgba(255,255,255,0.15);border-radius:20px;padding:20px;margin-bottom:20px;text-align:center}
.budget-title{font-size:1.2em;font-weight:bold;margin-bottom:15px;color:#FF9800}
.budget-row{display:flex;gap:10px;justify-content:center;align-items:center;flex-wrap:wrap;margin-bottom:10px}
.budget-row input{width:120px;padding:12px;border-radius:30px;font-size:1em;border:none;background:white;color:#333;text-align:center}
.budget-row button{background:#FF9800;color:white;border:none;padding:12px 20px;border-radius:30px;cursor:pointer;font-weight:bold}
.budget-info{font-size:1.1em;margin-top:10px;color:#ddd}
.budget-info strong{color:white}
.budget-remaining{font-size:1.5em;font-weight:bold;color:#4CAF50}
.budget-remaining.negative{color:#ff4757}
.paragon-card{background:rgba(255,255,255,0.15);border-radius:20px;padding:20px;margin-bottom:20px}
.paragon-title{font-size:1.2em;font-weight:bold;margin-bottom:15px;color:white}
.paragon-row{display:flex;gap:10px;flex-wrap:wrap}
.paragon-row input{flex:2;padding:12px;border-radius:30px;font-size:1em;border:none;background:white;color:#333}
.paragon-row button{background:#4CAF50;color:white;border:none;padding:12px 20px;border-radius:30px;cursor:pointer;font-weight:bold}
.paragon-total{display:flex;justify-content:space-between;align-items:center;margin-top:10px;color:white;font-weight:bold;flex-wrap:wrap}
.paragon-total span span{color:#FF9800;font-size:1.3em}
.undo-btn{background:#ff9800;color:white;border:none;padding:6px 12px;border-radius:30px;cursor:pointer;font-weight:bold;font-size:0.8em}
.summary-card{background:rgba(255,255,255,0.15);border-radius:20px;padding:20px;margin-bottom:20px}
.summary-item{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px solid rgba(255,255,255,0.2);color:white;font-size:1.1em}
.summary-total{display:flex;justify-content:space-between;padding:15px 0;font-weight:bold;font-size:1.3em;color:#FF9800}
.summary-item .amount,.summary-total .amount{display:inline-block;min-width:100px;text-align:right}
.history-card{background:rgba(255,255,255,0.15);border-radius:20px;padding:20px}
.history-title{font-size:1.1em;font-weight:bold;margin-bottom:15px;color:white}
.history-item{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid rgba(255,255,255,0.1);color:white}
.backup-list-card{background:rgba(255,255,255,0.15);border-radius:20px;padding:20px;margin-top:20px}
.backup-status-line{color:#ddd;margin-bottom:12px;font-size:0.9em}
.backup-item{display:flex;justify-content:space-between;align-items:center;gap:10px;padding:8px 0;border-bottom:1px solid rgba(255,255,255,0.1);color:white}
.backup-item button{background:#ff4757;color:white;border:none;width:34px;height:34px;border-radius:50%;cursor:pointer;font-size:0.9em;flex-shrink:0}
.back-btn{display:block;text-align:center;margin-top:20px;color:white;text-decoration:none;background:#2196F3;padding:12px;border-radius:30px}
body.dark .paragon-row input{background:#333;color:white}
body.dark .budget-row input{background:#333;color:white}
</style>
</head>
<body>
<div class="container">
<div class="header">
<div class="btn-group">
<button class="export-btn" onclick="exportData()">📥 EKSPORTUJ</button>
<button class="import-btn" onclick="importData()">📤 IMPORTUJ</button>
<button class="import-btn" style="background:#9c27b0" onclick="backupNow()">💾 BACKUP</button>
<button class="import-btn" style="background:#009688" onclick="downloadBackup()">⬇️ POBIERZ</button>
<button class="import-btn" style="background:#e91e63" onclick="restoreBackup()">♻️ PRZYWRÓĆ</button>
<input type="file" id="restoreFile" accept=".csv" style="display:none" onchange="doRestoreBackup()">
<button class="theme-btn" onclick="toggleTheme()">🌙</button>
</div>
</div>

<!-- BUDGET CARD -->
<div class="budget-card">
<div class="budget-title">💰 BUDŻET MIESIĘCZNY</div>
<div class="budget-row">
<input type="number" id="budgetAmount" step="10" placeholder="Budżet (€)">
<button onclick="setBudget()">💾 ZAPISZ</button>
</div>
<div class="budget-info">
<div>Budżet miesięczny: <strong id="budgetValue">0.00</strong> €</div>
<div>Wydano: <strong id="spentValue">0.00</strong> €</div>
<div class="budget-remaining" id="remainingValue">0.00 € pozostało</div>
</div>
</div>

<div class="paragon-card">
<div class="paragon-title">🔵 ALDI </div>
<div class="paragon-row">
<input type="number" id="aldiAmount" step="0.01" placeholder="Kwota (€)">
<button onclick="addParagon('aldi')">➕ DODAJ</button>
</div>
<div class="paragon-total">
<span>Obecnie: <span id="aldiTotal">0.00</span> €</span>
<button class="undo-btn" onclick="undoParagon('aldi')">↩️ Cofnij</button>
</div>
</div>

<div class="paragon-card">
<div class="paragon-title">🟢 EDEKA </div>
<div class="paragon-row">
<input type="number" id="edekaAmount" step="0.01" placeholder="Kwota (€)">
<button onclick="addParagon('edeka')">➕ DODAJ</button>
</div>
<div class="paragon-total">
<span>Obecnie: <span id="edekaTotal">0.00</span> €</span>
<button class="undo-btn" onclick="undoParagon('edeka')">↩️ Cofnij</button>
</div>
</div>

<div class="paragon-card">
<div class="paragon-title">🟡 PENNY </div>
<div class="paragon-row">
<input type="number" id="pennyAmount" step="0.01" placeholder="Kwota (€)">
<button onclick="addParagon('penny')">➕ DODAJ</button>
</div>
<div class="paragon-total">
<span>Obecnie: <span id="pennyTotal">0.00</span> €</span>
<button class="undo-btn" onclick="undoParagon('penny')">↩️ Cofnij</button>
</div>
</div>

<div class="paragon-card">
<div class="paragon-title">🥨 PIEKARNIA </div>
<div class="paragon-row">
<input type="number" id="backereiAmount" step="0.01" placeholder="Kwota (€)">
<button onclick="addParagon('backerei')">➕ DODAJ</button>
</div>
<div class="paragon-total">
<span>Obecnie: <span id="backereiTotal">0.00</span> €</span>
<button class="undo-btn" onclick="undoParagon('backerei')">↩️ Cofnij</button>
</div>
</div>

<div class="paragon-card">
<div class="paragon-title">🟤 NORMA-NETTO </div>
<div class="paragon-row">
<input type="number" id="sonstigeAmount" step="0.01" placeholder="Kwota (€)">
<button onclick="addParagon('sonstige')">➕ DODAJ</button>
</div>
<div class="paragon-total">
<span>Obecnie: <span id="sonstigeTotal">0.00</span> €</span>
<button class="undo-btn" onclick="undoParagon('sonstige')">↩️ Cofnij</button>
</div>
</div>

<div class="summary-card">
<div class="summary-item"><span>🔵 Aldi</span><span class="amount"><span id="sumAldi">0.00</span> €</span></div>
<div class="summary-item"><span>🟢 Edeka</span><span class="amount"><span id="sumEdeka">0.00</span> €</span></div>
<div class="summary-item"><span>🟡 Penny</span><span class="amount"><span id="sumPenny">0.00</span> €</span></div>
<div class="summary-item"><span>🥨 Piekarnia</span><span class="amount"><span id="sumBackerei">0.00</span> €</span></div>
<div class="summary-item"><span>🟤 Norma-Netto</span><span class="amount"><span id="sumSonstige">0.00</span> €</span></div>
<div class="summary-total"><span>💰 SUMA</span><span class="amount"><span id="sumTotal">0.00</span> €</span></div>
</div>

<div class="history-card">
<div class="history-title">📜 POPRZEDNIE MIESIĄCE</div>
<div id="historyList"></div>
</div>

<div class="backup-list-card">
<div class="history-title">📦 KOPIE ZAPASOWE NA KARCIE SD</div>
<div class="backup-status-line" id="backupStatusLine">Sprawdzam...</div>
<div id="backupList"></div>
</div>

<a href="/" class="back-btn">← WRÓĆ DO LISTY</a>
</div>

<script>
let isDark = localStorage.getItem('theme') === 'dark';
function setTheme() { if(isDark) document.body.classList.add('dark'); else document.body.classList.remove('dark'); }
function toggleTheme() { isDark = !isDark; localStorage.setItem('theme', isDark ? 'dark' : 'light'); setTheme(); }
setTheme();

function addParagon(shop) {
  let amount = parseFloat(document.getElementById(shop + 'Amount').value);
  if(isNaN(amount) || amount <= 0) { alert('Podaj prawidłową kwotę!'); return; }
  fetch('/paragon?shop=' + shop + '&amount=' + amount).then(() => { 
    document.getElementById(shop + 'Amount').value = ''; 
    loadAll(); 
  });
}

function undoParagon(shop) {
  fetch('/undo?shop=' + shop).then(() => { loadAll(); });
}

function setBudget() {
  let amount = parseFloat(document.getElementById('budgetAmount').value);
  if(isNaN(amount) || amount <= 0) { alert('Podaj prawidłową kwotę!'); return; }
  fetch('/budget?amount=' + amount).then(() => { 
    document.getElementById('budgetAmount').value = ''; 
    loadAll(); 
  });
}

function backupNow() {
  fetch('/backupnow')
    .then(r => {
      if (r.ok) { alert('✅ Backup zapisany na karcie SD!'); loadBackupList(); }
      else alert('❌ Backup NIE powiódł się - sprawdź kartę SD (pełna/wyjęta?) i Serial Monitor');
    })
    .catch(() => alert('❌ Nie udało się połączyć z ESP32 - backup NIE został zrobiony'));
}

// Zamienia nazwe pliku backupu na czytelna forme, np.
// "20260913.csv" -> "13.09.2026", a "B0000001.csv" (backup zrobiony
// zanim ESP32 zdazyl zsynchronizowac czas) zostawia jak jest.
function formatBackupName(name) {
  let base = name.replace('.csv', '');
  if (/^\d{8}$/.test(base)) {
    return base.substring(6, 8) + '.' + base.substring(4, 6) + '.' + base.substring(0, 4);
  }
  return base + ' (bez daty)';
}

// Pokazuje liste wszystkich backupow zapisanych na karcie SD, wraz
// z data ostatniego, i pozwala skasowac kazdy z osobna.
function loadBackupList() {
  fetch('/backuplist').then(r => r.json()).then(list => {
    const el = document.getElementById('backupList');
    const statusLine = document.getElementById('backupStatusLine');
    if (!list || list.length === 0) {
      el.innerHTML = '';
      statusLine.innerHTML = '⚠️ Jeszcze nie zrobiono żadnego backupu';
      return;
    }
    list.sort((a, b) => a.name < b.name ? 1 : -1);
    statusLine.innerHTML = '✅ Ostatni backup: <strong>' + formatBackupName(list[0].name) + '</strong> (' + list.length + ' kopii na karcie)';
    let html = '';
    list.forEach(b => {
      html += `<div class="backup-item"><span>🗂️ ${formatBackupName(b.name)}</span><button onclick="deleteBackup('${b.name}')" title="Usuń ten backup">🗑️</button></div>`;
    });
    el.innerHTML = html;
  }).catch(() => {
    document.getElementById('backupStatusLine').innerHTML = '❌ Nie można odczytać listy backupów z karty SD';
  });
}

function deleteBackup(name) {
  if (!confirm('Usunąć backup ' + formatBackupName(name) + '?')) return;
  fetch('/deletebackup?name=' + encodeURIComponent(name))
    .then(r => { if (r.ok) loadBackupList(); else alert('❌ Nie udało się usunąć tego backupu'); })
    .catch(() => alert('❌ Nie udało się połączyć z ESP32'));
}

// Pobiera aktualny backup jako plik NA TELEFON/KOMPUTER (poza karta SD).
// To warto robic od czasu do czasu - gdyby zgubila/uszkodzila sie
// sama karta SD, kopia na SD by nic nie dala.
function downloadBackup() {
  fetch('/downloadbackup')
    .then(r => r.text())
    .then(data => {
      const blob = new Blob([data], {type: 'text/csv'});
      const link = document.createElement('a');
      link.href = URL.createObjectURL(blob);
      link.download = 'einkaufsliste_backup.csv';
      link.click();
      alert('✅ Backup pobrany! Zapisz go gdzieś bezpiecznie (np. Dysk Google/komputer).');
    });
}

// Przywraca WSZYSTKO (listy + budzet + historia) z wczesniej
// pobranego pliku backupu - np. po wymianie ESP32 i/lub karty SD.
function restoreBackup() {
  if (!confirm('To NADPISZE wszystkie obecne listy zakupów, budżet i historię danymi z wybranego pliku backupu. Kontynuować?')) return;
  document.getElementById('restoreFile').click();
}
function doRestoreBackup() {
  let file = document.getElementById('restoreFile').files[0];
  if (!file) return;
  let reader = new FileReader();
  reader.onload = function(e) {
    fetch('/restorebackup', {
      method: 'POST',
      headers: {'Content-Type': 'text/csv'},
      body: e.target.result
    }).then(r => {
      if (r.ok) { alert('✅ Przywrócono z backupu!'); loadAll(); }
      else { alert('❌ Nie udało się przywrócić - czy to na pewno plik backupu z tego programu?'); }
    });
    document.getElementById('restoreFile').value = '';
  };
  reader.readAsText(file);
}

function exportData() {
  fetch('/exportdata')
    .then(r => r.text())
    .then(data => {
      const blob = new Blob([data], {type: 'text/csv'});
      const link = document.createElement('a');
      link.href = URL.createObjectURL(blob);
      link.download = 'backup.csv';
      link.click();
      alert('✅ Dane wyeksportowane!');
    });
}

function importData() {
  let input = document.createElement('input');
  input.type = 'file';
  input.accept = '.csv';
  input.onchange = e => {
    let file = e.target.files[0];
    let reader = new FileReader();
    reader.onload = event => {
      let data = event.target.result;
      fetch('/importdata', {
        method: 'POST',
        headers: {'Content-Type': 'text/csv'},
        body: data
      }).then(() => {
        alert('✅ Dane zaimportowane!');
        loadAll();
      });
    };
    reader.readAsText(file);
  };
  input.click();
}

function loadAll() {
  fetch('/totals').then(r => r.json()).then(data => {
    document.getElementById('aldiTotal').innerHTML = data.aldi.toFixed(2);
    document.getElementById('edekaTotal').innerHTML = data.edeka.toFixed(2);
    document.getElementById('pennyTotal').innerHTML = data.penny.toFixed(2);
    document.getElementById('backereiTotal').innerHTML = data.backerei.toFixed(2);
    document.getElementById('sonstigeTotal').innerHTML = data.sonstige.toFixed(2);
    document.getElementById('sumAldi').innerHTML = data.aldi.toFixed(2);
    document.getElementById('sumEdeka').innerHTML = data.edeka.toFixed(2);
    document.getElementById('sumPenny').innerHTML = data.penny.toFixed(2);
    document.getElementById('sumBackerei').innerHTML = data.backerei.toFixed(2);
    document.getElementById('sumSonstige').innerHTML = data.sonstige.toFixed(2);
    document.getElementById('sumTotal').innerHTML = data.total.toFixed(2);
    document.getElementById('budgetValue').innerHTML = data.budget.toFixed(2);
    document.getElementById('spentValue').innerHTML = data.total.toFixed(2);
    let remaining = data.budget - data.total;
    let remainingElem = document.getElementById('remainingValue');
    remainingElem.innerHTML = remaining.toFixed(2) + ' € pozostało';
    if(remaining < 0) {
      remainingElem.style.color = '#ff4757';
      remainingElem.innerHTML = Math.abs(remaining).toFixed(2) + ' € ponad budżet';
    } else {
      remainingElem.style.color = '#4CAF50';
    }
  });
  fetch('/bill').then(r => r.json()).then(data => {
    let html = '';
    if(data.history && data.history.length > 0) {
      data.history.forEach(h => {
        html += `<div class="history-item"><span>${h.month}.${h.year}</span><span>${h.total.toFixed(2)} €</span></div>`;
      });
    } else {
      html = '<div class="history-item">Brak poprzednich miesięcy</div>';
    }
    document.getElementById('historyList').innerHTML = html;
  });
}
loadAll();
loadBackupList();
setInterval(loadAll, 5000);
setInterval(loadBackupList, 20000);
</script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

// ============================================================
// API
// ============================================================

// Sprawdza, czy karta SD naprawde odpowiada (nie tylko czy sie
// kiedys zainicjowala przy starcie) - probuje otworzyc katalog
// glowny. Uzywane przez zielona/czerwona kropke na stronie glownej.
bool isSdOk() {
  File root = SD.open("/");
  if (!root) return false;
  bool ok = root.isDirectory();
  root.close();
  return ok;
}

void handleSdStatus() {
  bool ok = isSdOk();
  server.send(200, "application/json", String("{\"ok\":") + (ok ? "true" : "false") + "}");
}

// Zwraca liste plikow backupu z folderu /backup (nazwa + rozmiar w
// bajtach), zeby moc je pokazac na stronie Rechnung i pozwolic
// skasowac stare.
void handleBackupList() {
  String json = "[";
  bool first = true;
  File dir = SD.open("/backup");
  if (dir && dir.isDirectory()) {
    File entry = dir.openNextFile();
    while (entry) {
      if (!entry.isDirectory()) {
        String name = String(entry.name());
        int slash = name.lastIndexOf('/');
        if (slash >= 0) name = name.substring(slash + 1);
        if (!first) json += ",";
        first = false;
        json += "{\"name\":\"" + name + "\",\"size\":" + String(entry.size()) + "}";
      }
      entry.close();
      entry = dir.openNextFile();
    }
    dir.close();
  }
  json += "]";
  server.send(200, "application/json", json);
}

// Kasuje POJEDYNCZY plik backupu z karty SD (np. stary, juz
// niepotrzebny). Zabezpieczone przed wyjsciem poza folder /backup.
void handleDeleteBackup() {
  if (!server.hasArg("name")) { server.send(400, "text/plain", "ERROR"); return; }
  String name = server.arg("name");
  if (name.length() == 0 || name.indexOf('/') >= 0 || name.indexOf("..") >= 0) {
    server.send(400, "text/plain", "ERROR: zla nazwa pliku");
    return;
  }
  String path = "/backup/" + name;
  if (SD.exists(path.c_str()) && SD.remove(path.c_str())) {
    server.send(200, "text/plain", "OK");
  } else {
    server.send(500, "text/plain", "ERROR");
  }
}

void handleBudget() {
  if (server.hasArg("amount")) {
    monthlyBudget = server.arg("amount").toFloat();
    calculateTotals();
    saveTotals();
  }
  server.send(200, "text/plain", "OK");
}

void handleParagon() {
  if (server.hasArg("shop") && server.hasArg("amount")) {
    String shop = server.arg("shop");
    float amount = server.arg("amount").toFloat();
    
    if (shop == "penny") { 
      pennyHistory.push_back(amount);
      pennyTotal += amount; 
    }
    else if (shop == "aldi") { 
      aldiHistory.push_back(amount);
      aldiTotal += amount; 
    }
    else if (shop == "edeka") { 
      edekaHistory.push_back(amount);
      edekaTotal += amount; 
    }
    else if (shop == "backerei") { 
      backereiHistory.push_back(amount);
      backereiTotal += amount; 
    }
    else if (shop == "sonstige") { 
      sonstigeHistory.push_back(amount);
      sonstigeTotal += amount; 
    }
    calculateTotals();
    saveTotals();
  }
  server.send(200, "text/plain", "OK");
}

void handleUndo() {
  if (server.hasArg("shop")) {
    String shop = server.arg("shop");
    
    if (shop == "penny" && pennyHistory.size() > 0) {
      float lastAmount = pennyHistory.back();
      pennyHistory.pop_back();
      pennyTotal -= lastAmount;
    }
    else if (shop == "aldi" && aldiHistory.size() > 0) {
      float lastAmount = aldiHistory.back();
      aldiHistory.pop_back();
      aldiTotal -= lastAmount;
    }
    else if (shop == "edeka" && edekaHistory.size() > 0) {
      float lastAmount = edekaHistory.back();
      edekaHistory.pop_back();
      edekaTotal -= lastAmount;
    }
    else if (shop == "backerei" && backereiHistory.size() > 0) {
      float lastAmount = backereiHistory.back();
      backereiHistory.pop_back();
      backereiTotal -= lastAmount;
    }
    else if (shop == "sonstige" && sonstigeHistory.size() > 0) {
      float lastAmount = sonstigeHistory.back();
      sonstigeHistory.pop_back();
      sonstigeTotal -= lastAmount;
    }
    calculateTotals();
    saveTotals();
  }
  server.send(200, "text/plain", "OK");
}

void handleExportData() {
  String csv = "Miesiac;Rok;Aldi (€);Edeka (€);Penny (€);Piekarnia (€);Norma-Netto (€);Suma (€);Budzet (€)\n";
  csv += String(currentMonth) + ";" + String(currentYear) + ";";
  csv += String(aldiTotal, 2) + ";" + String(edekaTotal, 2) + ";" + String(pennyTotal, 2) + ";" + String(backereiTotal, 2) + ";" + String(sonstigeTotal, 2) + ";" + String(grandTotal, 2) + ";" + String(monthlyBudget, 2) + "\n";
  
  for (int i = 0; i < billCount; i++) {
    csv += String(billHistory[i].month) + ";" + String(billHistory[i].year) + ";";
    csv += String(billHistory[i].aldi, 2) + ";" + String(billHistory[i].edeka, 2) + ";" + String(billHistory[i].penny, 2) + ";" + String(billHistory[i].backerei, 2) + ";" + String(billHistory[i].sonstige, 2) + ";" + String(billHistory[i].total, 2) + ";" + "\n";
  }
  server.send(200, "text/csv", csv);
}

// Parsuje jeden wiersz historii z CSV (Miesiac;Rok;Aldi;Edeka;Penny;Backerei;Sonstige;Suma;)
// do struktury MonthlyBill. Zwraca false, jesli wiersz jest niepoprawny.
bool parseHistoryLine(String line, MonthlyBill &out) {
  line.trim();
  if (line.length() == 0) return false;
  int s1 = line.indexOf(';');
  int s2 = line.indexOf(';', s1 + 1);
  int s3 = line.indexOf(';', s2 + 1);
  int s4 = line.indexOf(';', s3 + 1);
  int s5 = line.indexOf(';', s4 + 1);
  int s6 = line.indexOf(';', s5 + 1);
  int s7 = line.indexOf(';', s6 + 1);
  if (s1 < 0 || s2 < 0 || s3 < 0 || s4 < 0 || s5 < 0 || s6 < 0 || s7 < 0) return false;

  out.month     = line.substring(0, s1).toInt();
  out.year      = line.substring(s1 + 1, s2).toInt();
  float aldi     = line.substring(s2 + 1, s3).toFloat();
  float edeka    = line.substring(s3 + 1, s4).toFloat();
  float penny    = line.substring(s4 + 1, s5).toFloat();
  out.backerei  = line.substring(s5 + 1, s6).toFloat();
  out.sonstige  = line.substring(s6 + 1, s7).toFloat();
  out.total     = line.substring(s7 + 1).toFloat(); // ewentualny "koncowy ;" nie przeszkadza
  out.aldi = aldi;
  out.edeka = edeka;
  out.penny = penny;
  return true;
}

void handleImportData() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "ERROR");
    return;
  }
  String csv = server.arg("plain");

  // Linia 1 = naglowek, pomijamy
  int headerEnd = csv.indexOf('\n');
  if (headerEnd <= 0) { server.send(400, "text/plain", "ERROR"); return; }
  String rest = csv.substring(headerEnd + 1);

  // Linia 2 = biezacy miesiac
  int line2End = rest.indexOf('\n');
  String currentLine = (line2End >= 0) ? rest.substring(0, line2End) : rest;
  currentLine.trim();

  MonthlyBill current;
  if (!parseHistoryLine(currentLine, current)) {
    server.send(400, "text/plain", "ERROR");
    return;
  }
  aldiTotal = current.aldi;
  edekaTotal = current.edeka;
  pennyTotal = current.penny;
  backereiTotal = current.backerei;
  sonstigeTotal = current.sonstige;
  currentMonth = current.month;
  currentYear = current.year;
  calculateTotals();
  saveTotals();

  // Kolejne linie (jesli sa) = "VORHERIGE MONATE" / historia
  billCount = 0;
  if (line2End >= 0) {
    String historyPart = rest.substring(line2End + 1);
    int pos = 0;
    while (pos < (int)historyPart.length() && billCount < 12) {
      int eol = historyPart.indexOf('\n', pos);
      String line = (eol >= 0) ? historyPart.substring(pos, eol) : historyPart.substring(pos);
      pos = (eol >= 0) ? eol + 1 : historyPart.length();

      MonthlyBill entry;
      if (parseHistoryLine(line, entry)) {
        billHistory[billCount] = entry;
        billCount++;
      }
    }
  }
  writeHistoryFile();

  server.send(200, "text/plain", "OK");
}

void handleTotals() {
  String json = "{\"penny\":" + String(pennyTotal, 2) + ",\"aldi\":" + String(aldiTotal, 2) + ",\"edeka\":" + String(edekaTotal, 2) + ",\"backerei\":" + String(backereiTotal, 2) + ",\"sonstige\":" + String(sonstigeTotal, 2) + ",\"total\":" + String(grandTotal, 2) + ",\"budget\":" + String(monthlyBudget, 2) + "}";
  server.send(200, "application/json", json);
}

void handleBill() {
  calculateTotals();
  String json = "{\"penny\":" + String(pennyTotal, 2) + ",\"aldi\":" + String(aldiTotal, 2) + ",\"edeka\":" + String(edekaTotal, 2) + ",\"backerei\":" + String(backereiTotal, 2) + ",\"sonstige\":" + String(sonstigeTotal, 2) + ",\"total\":" + String(grandTotal, 2) + ",\"history\":[";
  for (int i = 0; i < billCount; i++) {
    if (i > 0) json += ",";
    json += "{\"month\":" + String(billHistory[i].month) + ",\"year\":" + String(billHistory[i].year) + ",\"total\":" + String(billHistory[i].total, 2) + "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleList() {
  String shop = server.arg("shop");
  String json = "[";
  if (shop == "penny") {
    for (int i = 0; i < pennyCount; i++) {
      if (i > 0) json += ",";
      json += "{\"id\":" + String(i) + ",\"name\":\"" + pennyItems[i].name + "\",\"qty\":" + String(pennyItems[i].quantity) + ",\"bought\":" + (pennyItems[i].bought ? "true" : "false") + "}";
    }
  } else if (shop == "aldi") {
    for (int i = 0; i < aldiCount; i++) {
      if (i > 0) json += ",";
      json += "{\"id\":" + String(i) + ",\"name\":\"" + aldiItems[i].name + "\",\"qty\":" + String(aldiItems[i].quantity) + ",\"bought\":" + (aldiItems[i].bought ? "true" : "false") + "}";
    }
  } else if (shop == "edeka") {
    for (int i = 0; i < edekaCount; i++) {
      if (i > 0) json += ",";
      json += "{\"id\":" + String(i) + ",\"name\":\"" + edekaItems[i].name + "\",\"qty\":" + String(edekaItems[i].quantity) + ",\"bought\":" + (edekaItems[i].bought ? "true" : "false") + "}";
    }
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleAdd() {
  if (server.hasArg("shop") && server.hasArg("name")) {
    String shop = server.arg("shop");
    String name = server.arg("name");
    if (shop == "penny" && pennyCount < 200) {
      pennyItems[pennyCount].name = name;
      pennyItems[pennyCount].quantity = 1;
      pennyItems[pennyCount].bought = false;
      pennyCount++;
      saveData();
    }
    else if (shop == "aldi" && aldiCount < 200) {
      aldiItems[aldiCount].name = name;
      aldiItems[aldiCount].quantity = 1;
      aldiItems[aldiCount].bought = false;
      aldiCount++;
      saveData();
    }
    else if (shop == "edeka" && edekaCount < 200) {
      edekaItems[edekaCount].name = name;
      edekaItems[edekaCount].quantity = 1;
      edekaItems[edekaCount].bought = false;
      edekaCount++;
      saveData();
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleQuantity() {
  if (server.hasArg("shop") && server.hasArg("id") && server.hasArg("delta")) {
    String shop = server.arg("shop");
    int id = server.arg("id").toInt();
    int delta = server.arg("delta").toInt();
    
    if (shop == "penny" && id < pennyCount) {
      int newQty = pennyItems[id].quantity + delta;
      if (newQty >= 1 && newQty <= 99) {
        pennyItems[id].quantity = newQty;
        saveData();
      }
    }
    else if (shop == "aldi" && id < aldiCount) {
      int newQty = aldiItems[id].quantity + delta;
      if (newQty >= 1 && newQty <= 99) {
        aldiItems[id].quantity = newQty;
        saveData();
      }
    }
    else if (shop == "edeka" && id < edekaCount) {
      int newQty = edekaItems[id].quantity + delta;
      if (newQty >= 1 && newQty <= 99) {
        edekaItems[id].quantity = newQty;
        saveData();
      }
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleToggle() {
  if (server.hasArg("shop") && server.hasArg("id")) {
    String shop = server.arg("shop");
    int id = server.arg("id").toInt();
    
    if (shop == "penny" && id < pennyCount) {
      pennyItems[id].bought = !pennyItems[id].bought;
      saveData();
    }
    else if (shop == "aldi" && id < aldiCount) {
      aldiItems[id].bought = !aldiItems[id].bought;
      saveData();
    }
    else if (shop == "edeka" && id < edekaCount) {
      edekaItems[id].bought = !edekaItems[id].bought;
      saveData();
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleDelete() {
  if (server.hasArg("shop") && server.hasArg("id")) {
    String shop = server.arg("shop");
    int id = server.arg("id").toInt();
    
    if (shop == "penny" && id < pennyCount) {
      for (int i = id; i < pennyCount - 1; i++) pennyItems[i] = pennyItems[i + 1];
      pennyCount--;
      saveData();
    }
    else if (shop == "aldi" && id < aldiCount) {
      for (int i = id; i < aldiCount - 1; i++) aldiItems[i] = aldiItems[i + 1];
      aldiCount--;
      saveData();
    }
    else if (shop == "edeka" && id < edekaCount) {
      for (int i = id; i < edekaCount - 1; i++) edekaItems[i] = edekaItems[i + 1];
      edekaCount--;
      saveData();
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleClearHistory() {
  billCount = 0;
  if (SD.exists("/history.txt")) {
    SD.remove("/history.txt");
  }
  server.send(200, "text/html", "<html><body style='background:#1a1a2e;color:white;text-align:center;padding:50px'><h1>✅ Historia wyczyszczona!</h1><a href='/rechnung' style='color:#4CAF50'>← Wróć do rachunku</a></body></html>");
}

// Recznie wywolany backup (przycisk na stronie Rechnung)
void handleBackupNow() {
  bool ok = createFullBackup();
  if (ok) {
    server.send(200, "text/plain", "OK");
  } else {
    server.send(500, "text/plain", "ERROR");
  }
}

// Pozwala pobrac aktualny backup wprost do telefonu/komputera
// (niezalezna kopia POZA karta SD - przyda sie, gdyby kiedys
// zgubila sie/uszkodzila sie karta SD, a nie tylko sam ESP32).
void handleDownloadBackup() {
  server.send(200, "text/csv", buildFullBackupCsv());
}

// Przywraca WSZYSTKO (3 listy zakupow + Monatliches Budget +
// historia) z pliku backupu utworzonego przez ten sam program
// (przez BACKUP albo POBIERZ BACKUP). Uzywane np. po wymianie
// ESP32 i/lub karty SD.
void handleRestoreBackup() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "ERROR: brak danych");
    return;
  }
  String data = server.arg("plain");

  int idxLists = data.indexOf("=== LISTY ZAKUPOW ===");
  int idxBudget = data.indexOf("=== BUDZET MIESIECZNY");
  if (idxBudget < 0) idxBudget = data.indexOf("=== MONATLICHES BUDGET"); // wsteczna kompatybilnosc ze starszymi kopiami (sprzed spolszczenia)
  int idxHistory = data.indexOf("=== HISTORIA POPRZEDNICH MIESIECY ===");
  if (idxLists < 0 || idxBudget < 0 || idxHistory < 0) {
    server.send(400, "text/plain", "ERROR: to nie jest plik backupu z tego programu");
    return;
  }

  String listsSection = data.substring(idxLists, idxBudget);
  String budgetSection = data.substring(idxBudget, idxHistory);
  String historySection = data.substring(idxHistory);

  // --- 1. LISTY ZAKUPOW ---
  aldiCount = 0; edekaCount = 0; pennyCount = 0;
  {
    int pos = listsSection.indexOf('\n');       // koniec linii "=== LISTY ZAKUPOW ==="
    pos = listsSection.indexOf('\n', pos + 1);  // koniec linii naglowka kolumn
    while (pos >= 0) {
      int eol = listsSection.indexOf('\n', pos + 1);
      String line = (eol >= 0) ? listsSection.substring(pos + 1, eol) : listsSection.substring(pos + 1);
      pos = eol;
      line.trim();
      if (line.length() == 0) continue;

      int s1 = line.indexOf(';');
      int s2 = line.indexOf(';', s1 + 1);
      int s3 = line.indexOf(';', s2 + 1);
      if (s1 < 0 || s2 < 0 || s3 < 0) continue;

      String shop = line.substring(0, s1);
      String name = line.substring(s1 + 1, s2);
      int qty = line.substring(s2 + 1, s3).toInt();
      bool bought = (line.substring(s3 + 1).toInt() == 1);

      if (shop.equalsIgnoreCase("Aldi") && aldiCount < 200) {
        aldiItems[aldiCount] = { name, qty, bought }; aldiCount++;
      } else if (shop.equalsIgnoreCase("Edeka") && edekaCount < 200) {
        edekaItems[edekaCount] = { name, qty, bought }; edekaCount++;
      } else if (shop.equalsIgnoreCase("Penny") && pennyCount < 200) {
        pennyItems[pennyCount] = { name, qty, bought }; pennyCount++;
      }
    }
  }
  saveData();

  // --- 2. MONATLICHES BUDGET ---
  {
    int pos = budgetSection.indexOf('\n');      // koniec linii "=== MONATLICHES BUDGET ==="
    pos = budgetSection.indexOf('\n', pos + 1); // koniec linii naglowka kolumn
    if (pos >= 0) {
      int eol = budgetSection.indexOf('\n', pos + 1);
      String line = (eol >= 0) ? budgetSection.substring(pos + 1, eol) : budgetSection.substring(pos + 1);
      line.trim();

      int s1 = line.indexOf(';');
      int s2 = line.indexOf(';', s1 + 1);
      int s3 = line.indexOf(';', s2 + 1);
      int s4 = line.indexOf(';', s3 + 1);
      int s5 = line.indexOf(';', s4 + 1);
      int s6 = line.indexOf(';', s5 + 1);
      int s7 = line.indexOf(';', s6 + 1);
      int s8 = line.indexOf(';', s7 + 1);
      if (s1 > 0 && s2 > 0 && s3 > 0 && s4 > 0 && s5 > 0 && s6 > 0 && s7 > 0 && s8 > 0) {
        currentMonth   = line.substring(0, s1).toInt();
        currentYear    = line.substring(s1 + 1, s2).toInt();
        aldiTotal      = line.substring(s2 + 1, s3).toFloat();
        edekaTotal     = line.substring(s3 + 1, s4).toFloat();
        pennyTotal     = line.substring(s4 + 1, s5).toFloat();
        backereiTotal  = line.substring(s5 + 1, s6).toFloat();
        sonstigeTotal  = line.substring(s6 + 1, s7).toFloat();
        // s7+1..s8 = Gesamt - pomijamy, przeliczamy sami nizej
        monthlyBudget  = line.substring(s8 + 1).toFloat();
        calculateTotals();
        saveTotals();
      }
    }
  }

  // --- 3. HISTORIA POPRZEDNICH MIESIECY ---
  {
    billCount = 0;
    int pos = historySection.indexOf('\n');       // koniec linii "=== HISTORIA ... ==="
    pos = historySection.indexOf('\n', pos + 1);  // koniec linii naglowka kolumn
    while (pos >= 0 && billCount < 12) {
      int eol = historySection.indexOf('\n', pos + 1);
      String line = (eol >= 0) ? historySection.substring(pos + 1, eol) : historySection.substring(pos + 1);
      pos = eol;

      MonthlyBill entry;
      if (parseHistoryLine(line, entry)) {
        billHistory[billCount] = entry;
        billCount++;
      }
    }
    writeHistoryFile();
  }

  server.send(200, "text/plain", "OK");
}

void handleExportShop() { String shop = server.arg("shop"); String csv = "Nazwa;Ilosc;Kupione\n"; if (shop == "penny") { for (int i = 0; i < pennyCount; i++) { csv += pennyItems[i].name + ";" + String(pennyItems[i].quantity) + ";" + (pennyItems[i].bought ? "1" : "0") + "\n"; } } else if (shop == "aldi") { for (int i = 0; i < aldiCount; i++) { csv += aldiItems[i].name + ";" + String(aldiItems[i].quantity) + ";" + (aldiItems[i].bought ? "1" : "0") + "\n"; } } else if (shop == "edeka") { for (int i = 0; i < edekaCount; i++) { csv += edekaItems[i].name + ";" + String(edekaItems[i].quantity) + ";" + (edekaItems[i].bought ? "1" : "0") + "\n"; } } server.send(200, "text/csv", csv); } void handleImportShop() { if (server.hasArg("plain")) { String shop = server.arg("shop"); String csv = server.arg("plain"); int firstNewline = csv.indexOf('\n'); if (firstNewline > 0) { csv = csv.substring(firstNewline + 1); } Item tempItems[200]; int tempCount = 0; int start = 0; int end = csv.indexOf('\n'); while (end > 0 && tempCount < 200) { String line = csv.substring(start, end); line.trim(); start = end + 1; end = csv.indexOf('\n', start); if (line.length() > 0) { int sem1 = line.indexOf(';'); int sem2 = line.indexOf(';', sem1 + 1); if (sem1 > 0) { String name = line.substring(0, sem1); int qty = 1; bool bought = false; if (sem2 > 0) { qty = line.substring(sem1 + 1, sem2).toInt(); bought = (line.substring(sem2 + 1).toInt() == 1); } else { qty = line.substring(sem1 + 1).toInt(); } tempItems[tempCount].name = name; tempItems[tempCount].quantity = qty; tempItems[tempCount].bought = bought; tempCount++; } } } if (shop == "penny") { for (int i = 0; i < tempCount; i++) pennyItems[i] = tempItems[i]; pennyCount = tempCount; } else if (shop == "aldi") { for (int i = 0; i < tempCount; i++) aldiItems[i] = tempItems[i]; aldiCount = tempCount; } else if (shop == "edeka") { for (int i = 0; i < tempCount; i++) edekaItems[i] = tempItems[i]; edekaCount = tempCount; } saveData(); server.send(200, "text/plain", "OK"); } else { server.send(400, "text/plain", "ERROR"); } }
void handleMove() {
  if (server.hasArg("shop") && server.hasArg("from") && server.hasArg("to")) {
    String shop = server.arg("shop");
    int from = server.arg("from").toInt();
    int to = server.arg("to").toInt();
    
    if (shop == "penny" && from != to && from >= 0 && from < pennyCount && to >= 0 && to < pennyCount) {
      Item temp = pennyItems[from];
      if (from < to) {
        for (int i = from; i < to; i++) pennyItems[i] = pennyItems[i + 1];
      } else {
        for (int i = from; i > to; i--) pennyItems[i] = pennyItems[i - 1];
      }
      pennyItems[to] = temp;
      saveData();
    }
    else if (shop == "aldi" && from != to && from >= 0 && from < aldiCount && to >= 0 && to < aldiCount) {
      Item temp = aldiItems[from];
      if (from < to) {
        for (int i = from; i < to; i++) aldiItems[i] = aldiItems[i + 1];
      } else {
        for (int i = from; i > to; i--) aldiItems[i] = aldiItems[i - 1];
      }
      aldiItems[to] = temp;
      saveData();
    }
    else if (shop == "edeka" && from != to && from >= 0 && from < edekaCount && to >= 0 && to < edekaCount) {
      Item temp = edekaItems[from];
      if (from < to) {
        for (int i = from; i < to; i++) edekaItems[i] = edekaItems[i + 1];
      } else {
        for (int i = from; i > to; i--) edekaItems[i] = edekaItems[i - 1];
      }
      edekaItems[to] = temp;
      saveData();
    }
  }
  server.send(200, "text/plain", "OK");
}

// ============================================================
// STABILNOSC - AUTOMATYCZNY RESTART I PONOWNE LACZENIE Z WIFI
// ============================================================

// Sprawdza co jakis czas, czy WiFi jest nadal polaczone - jesli nie
// (np. router sie zresetowal albo sygnal na chwile zanikl), probuje
// polaczyc sie ponownie samo, bez potrzeby recznego resetu ESP32.
void checkWifiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi rozlaczone - probuje polaczyc ponownie...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }
}

// Restartuje ESP32 automatycznie RAZ dziennie w nocy o godzinie 4:00
// (a nie w losowym momencie) - dla stabilnosci dlugo dzialajacego
// urzadzenia, bez przeszkadzania w trakcie robienia zakupow w ciagu
// dnia. Wszystkie dane sa juz na biezaco zapisywane na karcie SD,
// wiec restart niczego nie gubi. Data ostatniego auto-restartu jest
// zapamietana na SD, zeby restart nie powtorzyl sie kilka razy pod
// rzad w tej samej godzinie.
void checkDailyRestart() {
  time_t now = time(nullptr);
  if (now < 1000000000) return; // czas jeszcze nie zsynchronizowany
  struct tm* tm_now = localtime(&now);
  String today = todayDateString();
  if (tm_now->tm_hour == 4 && today != lastAutoRestartDate) {
    Serial.println("🔄 Godzina 4:00 - automatyczny nocny restart ESP32...");
    lastAutoRestartDate = today;
    saveRestartMeta();
    delay(200);
    ESP.restart();
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== EINKAUFSLISTE FINAL ===\n");

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, sdSPI)) {
    Serial.println("BLAD: nie wykryto karty SD! Sprawdz okablowanie i format (FAT32).");
  }

  loadData(); loadHistory(); loadTotals();

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(ssid, password);
  Serial.print("Laczenie z WiFi");
  int wifiTries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifiTries++;
    if (wifiTries > 60) { // ok. 30 sekund probowania - jak dalej nic, restart i sprobuj od nowa
      Serial.println("\nBLAD: nie udalo sie polaczyc z WiFi - restart ESP32...");
      ESP.restart();
    }
  }
  Serial.println();
  Serial.println(WiFi.localIP());

  configTime(3600, 3600, "pool.ntp.org", "time.google.com");

// Czekaj na poprawny czas
Serial.print("⏳ Oczekiwanie na czas NTP");
int timeout = 0;
struct tm timeinfo;
while (!getLocalTime(&timeinfo, 1000) && timeout < 10) {
  Serial.print(".");
  timeout++;
}
Serial.println();

checkNewMonth();

loadBackupMeta();
checkAutoBackup();
loadRestartMeta();

  ArduinoOTA.setHostname("Einkaufsliste");
  ArduinoOTA.begin();
  
  server.on("/", handleRoot);
  server.on("/rechnung", handleRechnung);
  server.on("/list", handleList);
  server.on("/add", handleAdd);
  server.on("/quantity", handleQuantity);
  server.on("/toggle", handleToggle);
  server.on("/delete", handleDelete);
  server.on("/move", handleMove);
  server.on("/exportshop", handleExportShop);
  server.on("/importshop", HTTP_POST, handleImportShop);
  server.on("/paragon", handleParagon);
  server.on("/undo", handleUndo);
  server.on("/budget", handleBudget);
  server.on("/exportdata", handleExportData);
  server.on("/importdata", HTTP_POST, handleImportData);
  server.on("/totals", handleTotals);
  server.on("/bill", handleBill);
  server.on("/clearhistory", handleClearHistory);
  server.on("/backupnow", handleBackupNow);
  server.on("/downloadbackup", handleDownloadBackup);
  server.on("/restorebackup", HTTP_POST, handleRestoreBackup);
  server.on("/sdstatus", handleSdStatus);
  server.on("/backuplist", handleBackupList);
  server.on("/deletebackup", handleDeleteBackup);
  server.begin();
  
  Serial.print("🌐 IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("📌 Rechnung: http://" + WiFi.localIP().toString() + "/rechnung");
}

void loop() {
  static unsigned long lastCheck = 0;
  static unsigned long lastWifiCheck = 0;

  checkDailyRestart(); // sprawdzenie taniej niz raz na 10ms nie szkodzi

  if (millis() - lastWifiCheck > 15000) {
    lastWifiCheck = millis();
    checkWifiConnection();
  }

  if (millis() - lastCheck > 3600000) {
    lastCheck = millis();
    checkNewMonth();
    checkAutoBackup(); // raz dziennie robi pelny backup listy + budzetu
  }
  server.handleClient();
  ArduinoOTA.handle();
  delay(10);
}
