#include <string.h>
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include "Arduino_RouterBridge.h"

Arduino_LED_Matrix matrix;
BridgeTCPClient<> client(Bridge);

char resp[8000];

void scrollText(const char* s, int dir) {
  matrix.beginText(0, 0, 255, 255, 255);
  matrix.print(s);
  matrix.endText(dir);
}

void setLeds(int f) {
  bool hot = (f > 95);
  analogWrite(LED3_R, hot ? 255 : 0);
  analogWrite(LED3_G, 0);
  analogWrite(LED3_B, hot ? 0 : 255);
  digitalWrite(LED4_R, hot ? LOW : HIGH);
  digitalWrite(LED4_G, HIGH);
  digitalWrite(LED4_B, hot ? HIGH : LOW);
}

int fetchCelsius() {
  memset(resp, 0, sizeof(resp));
  bool nws = false;

  if (client.connectSSL("api.weather.gov", 443, "") >= 0) {
    nws = true;
  }

  if (nws) {
    client.println("GET /stations/KLUL/observations/latest HTTP/1.1");
    client.println("Host: api.weather.gov");
    client.println("User-Agent: arduino-unoq/1.0");
    client.println("Accept: application/geo+json");
    client.println("Connection: close");
    client.println();
  } else if (client.connect("api.open-meteo.com", 80) >= 0) {
    client.println("GET /v1/forecast?latitude=31.69&longitude=-89.13&current_weather=true HTTP/1.1");
    client.println("Host: api.open-meteo.com");
    client.println("User-Agent: Arduino-UNOQ");
    client.println("Connection: close");
    client.println();
  } else {
    return -1000;
  }

  size_t off = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < 10000 && off < sizeof(resp) - 1) {
    if (client.available()) {
      off += client.read((uint8_t*)&resp[off], sizeof(resp) - 1 - off);
    }
    delay(0);
  }
  client.stop();

  const char* w;
  if (nws) {
    w = strstr(resp, "\"temperature\"");
  } else {
    w = strstr(resp, "\"current_weather\"");
    if (w) {
      w = strstr(w, "\"temperature\"");
    }
  }
  if (!w) {
    return -1000;
  }
  const char* p;
  if (nws) {
    p = strstr(w, "\"value\"");
    if (!p) {
      return -1000;
    }
    p = strchr(p, ':');
  } else {
    p = strchr(w, ':');
  }
  if (!p) {
    return -1000;
  }
  p++;
  while (*p == ' ') {
    p++;
  }
  if (*p == 'n') {
    return -1000;
  }
  int sign = 1;
  if (*p == '-') {
    sign = -1;
    p++;
  }
  int c = 0;
  while (*p >= '0' && *p <= '9') {
    c = c * 10 + (*p - '0');
    p++;
  }
  if (*p == '.') {
    p++;
    if (*p >= '0' && *p <= '9' && (*p - '0') >= 5) {
      c++;
    }
  }
  return sign * c;
}

void setup() {
  matrix.begin();
  matrix.textFont(Font_5x7);
  matrix.textScrollSpeed(100);
  matrix.clear();
  pinMode(LED3_R, OUTPUT);
  pinMode(LED3_G, OUTPUT);
  pinMode(LED3_B, OUTPUT);
  pinMode(LED4_R, OUTPUT);
  pinMode(LED4_G, OUTPUT);
  pinMode(LED4_B, OUTPUT);
  Bridge.begin();
}

void loop() {
  matrix.textFont(Font_5x7);
  scrollText("     Scrimptech     ", SCROLL_LEFT);
  delay(300);

  int c = fetchCelsius();
  if (c < -100) {
    setLeds(-1000);
    scrollText("     NO NET     ", SCROLL_LEFT);
  } else {
    int f = (c * 9 + 2) / 5 + 32;
    setLeds(f);
    char num[6];
    if (f >= 100) {
      num[0] = '0' + f / 100;
      num[1] = '0' + (f / 10) % 10;
      num[2] = '0' + f % 10;
      num[3] = 'F';
      num[4] = '\0';
    } else if (f >= 10) {
      num[0] = '0' + f / 10;
      num[1] = '0' + f % 10;
      num[2] = 'F';
      num[3] = '\0';
    } else {
      num[0] = '0' + f;
      num[1] = 'F';
      num[2] = '\0';
    }

    scrollText("     Laurel MS     ", SCROLL_LEFT);

    if (f >= 100) {
      matrix.textFont(Font_5x7);
      matrix.beginText(0, 0, 255, 255, 255);
      matrix.print("      ");
      matrix.print(num);
      matrix.print("      ");
      matrix.endText(SCROLL_LEFT);
    } else {
      matrix.textFont(Font_4x6);
      matrix.beginText(0, 1, 255, 255, 255);
      matrix.print(num);
      matrix.endText(NO_SCROLL);
      delay(5000);
    }
  }
  delay(300);
}
