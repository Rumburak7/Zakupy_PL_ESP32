[README_1.md](https://github.com/user-attachments/files/32156722/README_1.md)
# 🛒 Einkaufsliste ESP32

Domowa lista zakupów z panelem budżetowym, oparta na ESP32, dostępna przez przeglądarkę w sieci WiFi. Dane (listy zakupów, budżet, historia, backupy) trzymane są na karcie SD, więc niczego nie tracisz po restarcie czy zaniku prądu.

Projekt hobbystyczny, pisany "pod siebie" - cały interfejs, kod i komentarze są po polsku. Nazwy sklepów (Aldi, Edeka, Penny, Norma-Netto) zostały bez zmian, bo to niemieckie sieci, w których faktycznie się robi zakupy.

## ✨ Funkcje

**Lista zakupów**
- 3 sklepy do wyboru: 🔵 Aldi, 🟢 Edeka, 🟡 Penny - każdy z osobną listą
- 15 kategorii produktów (Gemüse, Obst, Brot, Milch, Fleisch, Käse, Vegan, Haushalt, itd.) z gotowymi podpowiedziami po kliknięciu - nie trzeba wpisywać nazw ręcznie
- Ilość sztuk (+/-) dla każdego produktu
- Odznaczanie kupionych produktów jednym dotknięciem
- Przeciąganie i zmiana kolejności produktów na liście (drag & drop)
- Eksport/import listy danego sklepu do pliku CSV
- Jasny/ciemny motyw (przełącznik 🌙)

**Rechnung - budżet miesięczny**
- Ustawianie miesięcznego budżetu i śledzenie ile zostało do wydania
- Osobne sumy paragonów dla Aldi, Edeka, Penny, Bäckerei i Norma-Netto (+ cofnięcie ostatniej kwoty)
- Automatyczne przejście na nowy miesiąc i zapis poprzedniego miesiąca do historii (do 12 miesięcy wstecz)
- Eksport/import całego budżetu + historii do CSV

**Backup i bezpieczeństwo danych**
- Automatyczny backup całej aplikacji (listy + budżet + historia) raz dziennie na kartę SD
- Backup ręczny na żądanie (przycisk 💾 BACKUP) z uczciwą informacją o sukcesie/błędzie
- Pobranie backupu jako plik na telefon/komputer (⬇️ POBIERZ) - niezależnie od karty SD
- Przywrócenie z backupu (♻️ PRZYWRÓĆ), np. po wymianie ESP32 lub karty SD
- Podgląd wszystkich zapisanych backupów na karcie z możliwością kasowania pojedynczych kopii

**Stabilność**
- 🟢/🔴 wskaźnik stanu karty SD na stronie głównej (sprawdzany na żywo, nie tylko przy starcie)
- Automatyczne ponowne łączenie z WiFi po zaniku sygnału
- Automatyczny nocny restart ESP32 o godzinie 4:00 (raz dziennie, dla stabilności długo działającego urządzenia)
- Aktualizacja programu przez WiFi (ArduinoOTA) - bez podłączania kabla USB

## 🔧 Sprzęt

- ESP32 (dowolna płytka z WiFi, np. ESP32 DevKit)
- Czytnik kart SD (moduł SPI)
- Karta SD sformatowana w FAT32

### Podłączenie karty SD (magistrala VSPI)

| Pin karty SD | Pin ESP32 |
|---|---|
| CS   | GPIO 5  |
| SCK  | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |

## 📥 Instalacja

1. Zainstaluj [Arduino IDE](https://www.arduino.cc/en/software) i dodaj obsługę płytek ESP32 (Boards Manager → `esp32`).
2. Otwórz plik `Einkaufsliste_ESP32.ino`.
3. Na początku pliku wpisz swoje dane WiFi:

   ```cpp
   const char* ssid = "TU_WPISZ_NAZWE_WIFI";
   const char* password = "TU_WPISZ_HASLO_WIFI";
   ```

4. Podłącz kartę SD do ESP32 zgodnie z tabelą pinów powyżej.
5. Wgraj program na płytkę (Upload).
6. Otwórz Serial Monitor (115200 baud) - program wypisze adres IP urządzenia w sieci.
7. Wejdź na ten adres IP w przeglądarce (telefon/komputer w tej samej sieci WiFi) - to strona główna z listą zakupów. Strona budżetu jest pod adresem `/rechnung` (np. `http://192.168.1.50/rechnung`).

Wszystkie biblioteki (`WiFi`, `WebServer`, `SPI`, `SD`, `ArduinoOTA`) są częścią standardowego pakietu ESP32 dla Arduino IDE - nie trzeba niczego dodatkowo instalować.

## 💾 Backup i przywracanie

Program sam robi pełny backup raz dziennie na kartę SD (folder `/backup`). Można też zrobić backup ręcznie przyciskiem **💾 BACKUP** na stronie Rechnung.

Jeśli zepsuje się sama karta SD (a nie tylko ESP32), warto od czasu do czasu pobrać backup na telefon/komputer przyciskiem **⬇️ POBIERZ** i zachować go w bezpiecznym miejscu (np. Dysk Google). W razie awarii taki plik można wgrać z powrotem przyciskiem **♻️ PRZYWRÓĆ**.

Jeśli padnie tylko sam ESP32 (a karta SD jest cała), wystarczy przełożyć kartę do nowego ESP32 - dane już tam są, restore nie jest potrzebny.

## 📄 Licencja

Projekt prywatny/hobbystyczny - używaj i modyfikuj dowolnie na własne potrzeby.
