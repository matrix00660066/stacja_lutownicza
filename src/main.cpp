#include <Arduino.h>
#include <DS3231.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>
#include <EEPROM.h>

/*****************************************************************************************************************************/
#define SW 7        // Deklaracja pinu od przycisku Enkodera
#define Sensor 11   // Deklaracja Pinu od czujnika odłożonej kolby
#define Bulb 6      // Pin sterujacy oswietleniem 230V
#define Fan 5       // Pin sterujacy osodciągiem
#define TempRead A0 // Pomiar temperatury
#define Triak 4     // Załączanie grzałki
#define Relay 13    // Załączanie zasilania
#define PW_OFF 12   // Pin wyłączający stację

RotaryEncoder encoder(2, 3);        // Piny do których podłaczamy Enkoder
LiquidCrystal_I2C lcd(0x3F, 20, 4); // Deklaracja adresu i rodzaju LCD

long sysTime;                                                        // Zmienna do przechowywania czasu pracy procesora
int hr = 0, mt = 0, Ahr = 0, Amt = 0;                                // Zmienne do przechowywania godzin i minut
static int pos = 0;                                                  // Zmienne do obsługi enkodera
int newPos = 0;                                                      // Zmienne do obsługi enkodera
int Flag = 1;                                                        // Flaga do przeskakiwania pomiedzy ustawieniami i funkcjami
byte ClockChar[] = {0x00, 0x00, 0x0E, 0x15, 0x15, 0x13, 0x0E, 0x00}; // Icon Clock
byte TempChar[] = {0x04, 0x0A, 0x0A, 0x0E, 0x1F, 0x1F, 0x1F, 0x0E};  // Temp Icon
byte SetChar[] = {0x00, 0x01, 0x03, 0x16, 0x1C, 0x08, 0x00, 0x00};   // Set Icon
byte oChar[] = {0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00, 0x00, 0x00};     // o Icon
byte moonChar[] = {0x03, 0x0E, 0x0C, 0x1C, 0x1C, 0x0C, 0x0E, 0x03};  // Moon Icon
byte onoffChar[] = {0x00, 0x0E, 0x11, 0x15, 0x15, 0x11, 0x0E, 0x00}; // on off Icon
byte bulbChar[] = {0x0E, 0x11, 0x11, 0x11, 0x0A, 0x0E, 0x0E, 0x04};  // Bulb Icon
byte fanChar[] = {0x00, 0x13, 0x14, 0x0E, 0x05, 0x19, 0x00, 0x00};   // Fan Icon

DS3231 clock;
byte year;
byte month;
byte date;
byte dOW;
byte hour;
byte minute;
byte second;

long timeS = 0; // Zmienna do przechowywania czasu dla migania ikoną Clock

long buttonTimer = 0;            // Obsługa długiego i krótkiego przycisku  (timer)
long longPressTime = 1000;       // Obsługa długiego i krótkiego przycisku  (Czas przyciśnięcia długiego)
boolean buttonActive = false;    // Obsługa długiego i krótkiego przycisku  (Flaga)
boolean longPressActive = false; // Obsługa długiego i krótkiego przycisku  (Flaga)
int tempRead;                    // Odczyt temperatury
int temp;                        // Nastawa temperatury
float V = 0;                     // Odczyt ADC

byte sleepTime, offTime, offT, sleepT; // Zmienne od czasu uśpienia oraz czasu wyłączenia stacji

bool sensorState; // Zmienna informująca nas o odłożonej kolbie
long sensorTime;  // Odmierzanie czasu odłożonej kolby
int Flag1 = 0;    // Flaga do odmierzania czasu odłozonej kolby
bool fan, bulb;
//////////////////////////////////////////////////////////////////////////////
// SETUP    SETUP    SETUP    SETUP    SETUP    SETUP    SETUP    SETUP    SETUP
//////////////////////////////////////////////////////////////////////////////

void setup()
{
  pinMode(Relay, OUTPUT);        // Zasilanie przekaznika
  digitalWrite(Relay, 1);        // Załączenie przekażnika
  pinMode(SW, INPUT_PULLUP);     // Ustawienie pinu od przycisku na podciągnięcie do +
  pinMode(Sensor, INPUT_PULLUP); // Ustawienie pinu od przycisku na podciągnięcie do +
  pinMode(Bulb, OUTPUT);         // Wyjscie 230V na oświetlenie
  pinMode(Fan, OUTPUT);          // Wyjscie 12V na odciąg
  pinMode(Triak, OUTPUT);        // Wyjscie zasilania kolby

  Serial.begin(9600); // Inicjalizacja Seriala
  // Serial.begin(57600);
  lcd.init();      // Inicjalizacja LCD
  lcd.backlight(); // Włączenie podświetlenia LCD
  // Start the I2C interface
  Wire.begin();

  temp = EEPROM.read(0) * 2; // Odczytujemy wartośc EEPROM i mnożymy x2 aby odczytać prawidłową nastawę
  if (temp < 50 || temp > 450)
  {
    temp = 50;
  }

  sleepTime = EEPROM.read(1); // Odczyt czasu uspienia z EEPROM
  offTime = EEPROM.read(2);   // Odczyt czasu wyłączenia stacji z EEPROM
  sleepT = sleepTime;         // Przypisanie wartości ustawionych czasów uśienia do zmiennych pomocniczych
  offT = offTime;             // Przypisanie wartości ustawionych czasów wyłaczenia do zmiennych pomocniczych

  PCICR |= 0b00000100;                     // Włączenie Portu D ( Do obsługi przerwan dla Enkodera
  PCMSK2 |= (1 << PCINT3) | (1 << PCINT2); // Ustawienie pinu 2 oraz 3 na porcie D jako przerwanie

  sysTime = millis(); // Zapisanie czasu systemowego do zmiennej

  lcd.createChar(0, ClockChar); // Tworzenie ikon
  lcd.createChar(1, TempChar);  // Tworzenie ikon
  lcd.createChar(2, SetChar);   // Tworzenie ikon
  lcd.createChar(3, oChar);     // Tworzenie ikon
  lcd.createChar(4, moonChar);  // Tworzenie ikon
  lcd.createChar(5, onoffChar); // Tworzenie ikon
  lcd.createChar(6, bulbChar);  // Tworzenie ikon
  lcd.createChar(7, fanChar);   // Tworzenie ikon
}

//////////////////////////////////////////////////////////////////////////////

ISR(PCINT2_vect) // Procedura zapisywania zmian w przerwaniach dla enkodera
{
  encoder.tick();
}

void PressButton();
void tempReadadc();
void setoff();
void setsleep();
void menu();
void StateStationRead();
void blinkclock();
void ustawCzas();

void getDateStuff(byte &year, byte &month, byte &date, byte &dOW,
                  byte &hour, byte &minute, byte &second)
{
  // Call this if you notice something coming in on
  // the serial port. The stuff coming in should be in
  // the order YYMMDDwHHMMSS, with an 'x' at the end.
  boolean gotString = false;
  char inChar;
  // byte temp1, temp2;
  char inString[20];

  byte j = 0;
  while (!gotString)
  {
    if (Serial.available())
    {
      inChar = Serial.read();
      inString[j] = inChar;
      j += 1;
      if (inChar == 'x')
      {
        gotString = true;
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// LOOP    LOOP    LOOP    LOOP    LOOP    LOOP    LOOP    LOOP    LOOP    LOOP
//////////////////////////////////////////////////////////////////////////////

void loop()
{
  if (millis() - sysTime >= 700)
  {
    lcd.setCursor(0, 0);
    lcd.print("  Stacja Lutownicza "); // Napisz na LCD "Stacja Lutownicza"

    lcd.setCursor(0, 1); // ustaw kursor na drugą linię lcd
    lcd.write(1);        // Wydrukuj ikonę temperatury kolby
    lcd.print("    ");
    lcd.setCursor(1, 1);
    lcd.print(tempRead); // Wydrukuj wartość temperatury kolby
    lcd.setCursor(4, 1);
    lcd.print(" ");
    lcd.write(3);
    lcd.print("C       "); // Wydrukuj stopnie C
    lcd.setCursor(0, 2);
    lcd.write(2);    // Wydrukuj ikonę ustawionej Temp.
    lcd.print(temp); // Wydrukuj ustawioną temp
    lcd.print(" ");
    lcd.write(3);
    lcd.print("C       ");
    lcd.setCursor(3, 3);
    lcd.write(6); // Wydrukuj ikonę żarówki i stan pinu
    if (bulb == 1)
    {
      lcd.print("On  ");
    }
    else
    {
      lcd.print("Off ");
    }

    lcd.setCursor(8, 3);
    lcd.write(7); // Wydrukuj ikonę wiatraka i stan pinu
    if (fan == 1)
    {
      lcd.print("On  ");
    }
    else
    {
      lcd.print("Off ");
    }

    lcd.setCursor(14, 2);
    lcd.write(4), lcd.print(" "), lcd.print(sleepT), lcd.print("min"); // Wydrukuj wartość czasu do uśpienia stacji i ikony

    lcd.setCursor(14, 1);
    lcd.write(5);
    if (offT < 10)
    {
      lcd.print(" "), lcd.print(offT), lcd.print("min"); // Wydrukuj wartość czasu do wyłączenia stacji i ikony oraz dopełnij puste miejsce zerami
    }
    else
    {
      lcd.print(offT), lcd.print("min");
    }

    lcd.setCursor(14, 3);
    lcd.write(0); // Wydrukuj ikonę zegara

    lcd.setCursor(15, 3);

    sysTime = millis();
  }

  StateStationRead(); // Aktualizacja stanu odłożonej kolby
  PressButton();      // Sprawdzanie wcisniecia przycisku enkodera
  tempReadadc();      // Odczytywanie temperatury kolby

  if (Flag == 0)
  {
    Flag = 1;
  }

  if (digitalRead(PW_OFF) == 0)
  {
    digitalWrite(Relay, 0);
  }
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION setTime     FUNCTION setTime     FUNCTION setTime   FUNCTION setTime
//////////////////////////////////////////////////////////////////////////////

void setTime()
{
  lcd.setCursor(0, 3);
  lcd.print("               ");
  lcd.setCursor(0, 0);                      // i tak w pierwszej linii nic nie jest wyświetlane
  lcd.print("    Ustaw Godzine    ");       // więc wpiszemy same spacje plus użyteczną informację
  lcd.setCursor(15, 2), lcd.print("__   "); // to wyświetli anm w trzeciej linii podłogę nad godzinami poniżej
  lcd.setCursor(0, 1);
  lcd.print("                    ");
  lcd.setCursor(0, 2);
  lcd.print("               ");
  sysTime = millis();
  /////////////////////////////

  while (Flag == 1)
  {
    newPos = encoder.getPosition(); // Aktualizujemy stan enkodera
    if (pos != newPos)              // Jeśli stan różni się od poprzedniego
    {
      if (pos < newPos) // Jeśli pozycja jest mniejsza od nowej pozycji
      {
        hr--;         // zmniejsz godzinę
        pos = newPos; // aktualizujemy wartosci pozycji
        if (hr < 0)   // jeśli godzina jest mniejsza niz 0
        {
          hr = 23; // zmien godzine na 23
        }
      }
      else
      {
        hr++;         // w przypadku wiekszej nowej pozycji enkodera od starej pozycji zwieksz godzine
        pos = newPos; // aktualizujemy wartosci pozycji
        if (hr > 23)  // Jeśli wartosc godziny jest wieksza od 23
        {
          hr = 0; // zmien godzine na 0
        }
      }
    }

    if (hr != Ahr) // po zakonczeniu petli sprawdz czy wartosc nowej godziny różni się od starej
    {
      lcd.setCursor(15, 3); // ustawiam kursor na odpowiedniej pozycji
      if (hr < 10)          // sprawdzam czy godzina jest mniejsza niż 10
      {
        lcd.print("0"); // jeśli jest to na pierwszej pozycji wyświetlimy 0
      }
      lcd.print(hr); // i wyświetlimy godzinę
      Ahr = hr;
    }
    void blinkClock();                                          // Miganie ikoną zegara
    if ((digitalRead(SW) == 0) && (millis() - sysTime >= 1000)) // Jesli wcisniemy SW i czas od wejscia w petle przekrocył 1s
    {
      sysTime = millis(); // Zapisz nowy czas pracy
      Flag = 2;           // zmien wartosc flagi aby przejsc do ustawiania minut
      lcd.setCursor(0, 0);
      lcd.print("    Ustaw Minute    ");
      lcd.setCursor(15, 2), lcd.print("   __");
    }
  }

  delay(250); // Opoznienie przejscia petli aby nie odczytało wcisniecia SW wielokrotnie

  while (Flag == 2)
  {
    newPos = encoder.getPosition();
    if (pos != newPos)
    {
      if (pos < newPos)
      {
        mt--;
        pos = newPos;
        if (mt < 0)
        {
          mt = 59;
        }
      }
      else
      {
        mt++;
        pos = newPos;
        if (mt > 59)
        {
          mt = 0;
        }
      }
    }

    if (mt != Amt)
    {
      lcd.setCursor(18, 3);
      if (mt < 10)
      {
        lcd.print("0");
      }
      lcd.print(mt);
      Amt = mt;
    }
    void blinkClock();
    if ((digitalRead(SW) == 0) && (millis() - sysTime >= 1000))
    {
      Flag = 1;
      delay(250);
      lcd.clear();
    }
  }
}
//////////////////////////////////////////////////////////////////////////////
// Clock Icon    Clock Icon    Clock Icon    Clock Icon    Clock Icon    Clock
//////////////////////////////////////////////////////////////////////////////

void blinkClock() // Miganie ikonka zegara
{
  lcd.createChar(0, ClockChar);                               // wpisanie znaku clockChar na pozycji 0
  if (((millis() - timeS) > 0) && ((millis() - timeS) < 250)) // Jesli czas pracy jest wiekszy niz 0 i mniejszy niz 250
  {
    lcd.setCursor(14, 3);
    lcd.write(0); // zapal ikone
  }

  if ((millis() - timeS) > 250 && (millis() - timeS) < 500) // jesli czas pracy jest wiekszy niz 250 i mniejszy niz 500
  {
    lcd.setCursor(14, 3);
    lcd.print(" "); // zgas ikone
  }
  if ((millis() - timeS) > 500) // jesli czas upłynie
  {
    timeS = millis(); // zapisz nowa wartosc
  }
}

//////////////////////////////////////////////////////////////////////////////
// Short Long Press      Short Long Press      Short Long Press
//////////////////////////////////////////////////////////////////////////////

void PressButton()
{

  if (digitalRead(SW) == LOW) // Jesli wcisniemy przycisk
  {
    delay(100);                // czekamy 100ms
    if (buttonActive == false) // jesli flaga aktywnego przycisku jest negatywna
    {
      buttonActive = true;    // zmien jej wartosc na pozytywna
      buttonTimer = millis(); // zapisz aktualny czas pracy do zmiennej buttonTimer
    }

    if ((millis() - buttonTimer > longPressTime) && (longPressActive == false)) // Jesli czas timera jest wiekszy od czasu zadeklarowanego jako długi przycisk oraz jesli flaga długiego przycisku jest negatywna
    {
      longPressActive = true; // zmien flage długiego przycisku na pozytywna
      sysTime = millis();     // Zapisz czas pracy
      Flag = 1;               // Ustaw flage na 1 aby uruchomic petle while w funkcji menu
      digitalWrite(Triak, 0); // Po wejsciu do menu wyłączam grzanie lutownicy dla bezpieczeństwa
      menu();                 // Uruchom funkcje menu
    }
  }
  else
  {
    if (buttonActive == true) // jesli flaga aktywnego przycisku jest prawdziwa
    {
      if (longPressActive == true) // oraz jesli flaga długiego przycisku jest aktywna
      {
        longPressActive = false; // zmien wartosc flagi na fałsz
      }
      else // jesli nie to oznacza krótkie wcisniecie SW
      {
        Flag = 1;
        digitalWrite(Triak, 0);
        lcd.setCursor(1, 1);
        lcd.print("---");
        void set(); // Uruchom funkcje set (Menu na pulpicie)
      }
      buttonActive = false; // Zmien flage aktywnego przycisku na fałsz
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// Menu    Menu    Menu    Menu    Menu    Menu    Menu    Menu    Menu    Menu
//////////////////////////////////////////////////////////////////////////////

void menu()
{
  lcd.setCursor(0, 0);
  lcd.print("     Ustawienia     ");

  lcd.setCursor(0, 1);
  lcd.print(" Zegar              ");

  lcd.setCursor(0, 2);
  lcd.print(" Czas uspienia      ");
  lcd.setCursor(0, 3);
  lcd.print(" Czas wyl.          ");

  int item = 0; // Zmienna która przechowuje informacje o aktualnym stanie menu

  while (Flag == 1)
  {
    newPos = encoder.getPosition();
    if (pos != newPos)
    {
      if (pos < newPos)
      {
        item--;
        pos = newPos;

        if (item < 1)
        {
          item = 1;
        }
      }
      else
      {
        item++;
        pos = newPos;
        if (item > 4)
        {
          item = 4;
        }
      }
    }

    if ((digitalRead(SW) == 0) && (item == 1) && (millis() - sysTime >= 1000)) // jesli wcisniemy przycik oraz zmienna item=1 oraz czas pracy przekroczył 1s
    {
      Flag = 1;  // Ustaw flage na 1 aby uruchomic petle while w funkcji menu
      setTime(); // uruchom funkcje
      lcd.setCursor(0, 0);
      lcd.print("     Ustawienia     ");
    }

    if ((digitalRead(SW) == 0) && (item == 2) && (millis() - sysTime >= 1000))
    {
      Flag = 1;
      void setSleep();
      sleepT = sleepTime; // Aktualizacja zmiennych dla poprawnego wyswietlania na pulpicie po opuszczeniu funkcji
      Flag = 1;
      delay(100);
      lcd.setCursor(0, 0);
      lcd.print("     Ustawienia     ");
    }

    if ((digitalRead(SW) == 0) && (item == 3) && (millis() - sysTime >= 1000))
    {
      Flag = 1;
      setOff();
      offT = offTime; // Aktualizacja zmiennych dla poprawnego wyswietlania na pulpicie po opuszczeniu funkcji
      Flag = 1;
      delay(100);
      lcd.setCursor(0, 0);
      lcd.print("     Ustawienia     ");
    }

    if ((digitalRead(SW) == 0) && (item == 4) && (millis() - sysTime >= 1000))
    {
      Flag = 0;
    }

    switch (item)
    { // Petla która drukuje dane na lcd w zaleznosci od wartosci item

    case 1:
      lcd.setCursor(0, 1);
      lcd.print(">Zegar              ");
      lcd.setCursor(0, 2);
      lcd.print(" Czas uspienia      ");
      lcd.setCursor(0, 3);
      lcd.print(" Czas wyl.          ");
      break;

    case 2:
      lcd.setCursor(0, 1);
      lcd.print(" Zegar              ");
      lcd.setCursor(0, 2);
      lcd.print(">Czas uspienia      ");
      lcd.setCursor(0, 3);
      lcd.print(" Czas wyl.          ");
      break;

    case 3:
      lcd.setCursor(0, 1);
      lcd.print(" Zegar              ");
      lcd.setCursor(0, 2);
      lcd.print(" Czas uspienia      ");
      lcd.setCursor(0, 3);
      lcd.print(">Czas wyl.          ");
      break;

    case 4:
      lcd.setCursor(0, 1);
      lcd.print(" Czas uspienia      ");
      lcd.setCursor(0, 2);
      lcd.print(" Czas wyl.          ");
      lcd.setCursor(0, 3);
      lcd.print(">Powrot             ");
      break;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// Set Temp     Set Temp      Set Temp     Set Temp      Ustawianie temperatury
//////////////////////////////////////////////////////////////////////////////

void setTemp()
{
  sysTime = millis();
  int tempA = 0;

  while (Flag == 1)
  {
    newPos = encoder.getPosition();
    if (pos != newPos)
    {
      if (pos < newPos)
      {
        temp = temp - 10;
        pos = newPos;

        if (temp < 50)
        {
          temp = 50;
        }
      }
      else
      {
        temp = temp + 10;
        pos = newPos;
        if (temp > 450)
        {
          temp = 450;
        }
      }
    }

    if (digitalRead(SW) == 0 && (millis() - sysTime >= 1000))
    {
      lcd.noBlink();
      Flag = 0;
      EEPROM.write(0, temp / 2); // EEPROM przyjmuje na jeden bajt wartości od 0 do 255 wiec dzielimy nastawe przez 2
    }

    if (tempA != temp)
    {
      lcd.setCursor(1, 2);
      lcd.print("    ");
      lcd.setCursor(1, 2);
      lcd.print(temp);
      lcd.blink();
      tempA = temp;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// Set      Set      Set       Set      Menu które włącza się po krótkim kliku
//////////////////////////////////////////////////////////////////////////////

void set()
{
  int item = 1; // Zmienna która przechowuje informacje o aktualnym stanie menu
  while (Flag == 1)
  {

    newPos = encoder.getPosition();
    if (pos != newPos)
    {
      if (pos < newPos)
      {
        item--;
        pos = newPos;

        if (item < 1)
        {
          item = 1;
        }
      }
      else
      {
        item++;
        pos = newPos;
        if (item > 3)
        {
          item = 3;
        }
      }
    }

    delay(100);

    if ((digitalRead(SW) == 0) && (item == 1))
    {
      Flag = 1;
      setTemp();
    }

    if ((digitalRead(SW) == 0) && (item == 2))
    {

      bulb = !bulb;
      Flag = 0;
    }

    if ((digitalRead(SW) == 0) && (item == 3))
    {
      fan = !fan;
      Flag = 0;
    }

    digitalWrite(Bulb, bulb);
    digitalWrite(Fan, fan);

    switch (item)
    {
    case 1:
      lcd.setCursor(1, 2);
      lcd.print(temp);
      lcd.print("<");
      lcd.setCursor(4, 3);

      if (bulb == 1)
      {
        lcd.print("On  ");
      }
      else
      {
        lcd.print("Off ");
      }

      lcd.setCursor(9, 3);
      if (fan == 1)
      {
        lcd.print("On  ");
      }
      else
      {
        lcd.print("Off ");
      }
      break;

    case 2:
      lcd.setCursor(1, 2);
      lcd.print(temp);
      lcd.print(" ");
      lcd.setCursor(4, 3);
      if (bulb == 1)
      {
        lcd.print("On ");
      }
      else
      {
        lcd.print("Off");
      }
      lcd.print("<");

      lcd.setCursor(9, 3);
      if (fan == 1)
      {
        lcd.print("On  ");
      }
      else
      {
        lcd.print("Off ");
      }
      break;

    case 3:
      lcd.setCursor(1, 2);
      lcd.print(temp);
      lcd.print(" ");

      lcd.setCursor(4, 3);
      if (bulb == 1)
      {
        lcd.print("On  ");
      }
      else
      {
        lcd.print("Off ");
      }
      lcd.setCursor(9, 3);
      if (fan == 1)
      {
        lcd.print("On ");
      }
      else
      {
        lcd.print("Off");
      }
      lcd.print("<");
      break;
    }
  }

  delay(200);
}

//////////////////////////////////////////////////////////////////////////////
// Set Sleep   SetSleep    SetSleep    SetSleep    SetSleep    SetSleep
//////////////////////////////////////////////////////////////////////////////

void setSleep()
{
  sysTime = millis();
  int sleepT = 0;

  while (Flag == 1)
  {
    newPos = encoder.getPosition();
    if (pos != newPos)
    {
      if (pos < newPos)
      {
        sleepTime = sleepTime - 1;
        pos = newPos;

        if (sleepTime <= 0)
        {
          sleepTime = 1;
        }
      }
      else
      {
        sleepTime = sleepTime + 1;
        pos = newPos;
        if (sleepTime >= 6)
        {
          sleepTime = 5;
        }
      }
    }

    if (digitalRead(SW) == 0 && (millis() - sysTime >= 500))
    {
      Flag = 0;
      EEPROM.write(1, sleepTime); // EEPROM przyjmuje na jeden bajt wartości od 0 do 255 wiec dzielimy nastawe przez 2
    }

    if (sleepT != sleepTime)
    {
      lcd.setCursor(14, 2);
      lcd.print(":   ");
      lcd.setCursor(15, 2);
      lcd.print(sleepTime);
      lcd.print("min.");
      sleepT = sleepTime;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// Set Off Time  Set Off Time Set Off Time Set Off Time Set Off Time
//////////////////////////////////////////////////////////////////////////////

void setOff()
{
  Serial.println("SET Off");
  sysTime = millis();
  int offT = 0;

  while (Flag == 1)
  {
    newPos = encoder.getPosition();
    if (pos != newPos)
    {
      if (pos < newPos)
      {
        offTime = offTime - 1;
        pos = newPos;

        if (offTime <= 0)
        {
          offTime = 1;
        }
      }
      else
      {
        offTime = offTime + 1;
        pos = newPos;
        if (offTime >= 16)
        {
          offTime = 15;
        }
      }
    }

    if (digitalRead(SW) == 0 && (millis() - sysTime >= 500))
    {
      Flag = 0;
      EEPROM.write(2, offTime); // EEPROM przyjmuje na jeden bajt wartości od 0 do 255 wiec dzielimy nastawe przez 4
    }

    if (offT != offTime)
    {
      lcd.setCursor(10, 3);
      lcd.print(":   ");
      lcd.setCursor(11, 3);
      lcd.print(offTime);
      lcd.print("min.");
      offT = offTime;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// StateStationRead        StateStationRead        StateStationRead
//////////////////////////////////////////////////////////////////////////////

void StateStationRead()
{
  sensorState = digitalRead(Sensor); // Przypisujemy odczyt przycisku do zmiennej

  if (sensorState == 0 && Flag1 == 0) // Jeśli kolba jest odłożona(0) i flaga1 ==0
  {
    sensorTime = millis(); // Zapisz czas pracy do zmiennej
    Flag1 = 1;             // I ustaw flage1 na 1
    lcd.setCursor(0, 3);
    lcd.print("v  ");
  }
  ///////////////////////////////////////////////////////////////////

  if (Flag1 == 1) // Jesli flaga1==1
  {
    offT = ((offTime) - ((millis() - sensorTime) / 6000));     // Oblicz czas do konca pracy
    sleepT = ((sleepTime) - ((millis() - sensorTime) / 6000)); // Oblicz czas do uśpienia
    if (sleepT > 5)                                            // Jesli wartośc sleepT jest wieksza niz 5 ustaw ja na wartosc 0
    {
      sleepT = 0;
    }
  }

  if (sensorState == 1 && Flag1 == 1) // Jesli kolba zostanie podniesiona i flaga1 ==1
  {
    Flag1 = 0;                 // ustaw flage1 na 0
    lcd.backlight();           // Włącz podswietlenie lcd
    temp = EEPROM.read(0) * 2; // odczytaj temperature z eeprom
    sleepT = sleepTime;        // zaktualizuj zmienne
    offT = offTime;            // zaktualizuj zmienne
    lcd.setCursor(0, 3);
    lcd.print("^  ");
  }

  if (Flag1 == 1 && sleepT == 0) // jesli flaga == 1 oraz jesli sleepT ==0
  {
    lcd.noBacklight(); // Wyłącz podswietlenie i ustaw temp na 180
    temp = 180;
  }

  if (Flag1 == 1 && offT == 0) // Jesli flag1==1 oraz offT==0 wyłącz stację
  {
    digitalWrite(Relay, 0);
  }

  if (sensorState == 1 && Flag1 == 0)
  {
    lcd.setCursor(0, 3);
    lcd.print("^  ");
  }
}

//////////////////////////////////////////////////////////////////////////////
// Temp Read     Temp Read      Temp Read     Temp Read     Temp Read
//////////////////////////////////////////////////////////////////////////////

void tempReadadc()
{
  V = analogRead(TempRead);
  V = (V * 0.0049);
  tempRead = (V * 200);

  if (tempRead + 4 <= temp)
  {
    digitalWrite(Triak, 1);
  }
  if (tempRead - 4 >= temp)
  {
    digitalWrite(Triak, 0);
  }
}

void ustawCzas()
{
  // If something is coming in on the serial line, it's
  // a time correction so set the clock accordingly.
  if (Serial.available())
  {
    getDateStuff(year, month, date, dOW, hour, minute, second);

    clock.setClockMode(false); // set to 24h
    // setClockMode(true); // set to 12h

    clock.setYear(year);
    clock.setMonth(month);
    clock.setDate(date);
    clock.setDoW(dOW);
    clock.setHour(hour);
    clock.setMinute(minute);
    clock.setSecond(second);

    // Test of alarm functions
    // set A1 to one minute past the time we just set the clock
    // on current day of week.
    clock.setA1Time(dOW, hour, minute + 1, second, 0x0, true,
                    false, false);
    // set A2 to two minutes past, on current day of month.
    clock.setA2Time(date, hour, minute + 2, 0x0, false, false,
                    false);
    // Turn on both alarms, with external interrupt
    // clock.turnOnAlarm(1);
    // clock.turnOnAlarm(2);
  }
  delay(1000);
}