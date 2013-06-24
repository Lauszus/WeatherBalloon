#ifdef ENABLE_GPS

TinyGPS gps;
//SoftwareSerial nss(5, 6); // RX, TX

static void gpsdump(TinyGPS &gps);
static bool feedgps();
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);

void gpsInit() {
  Serial1.begin(9600); // TODO: Increase baud rate of module
  
  Serial.print(F("Testing TinyGPS library v. ")); Serial.println(TinyGPS::library_version());
  Serial.println(F("by Mikal Hart\r\n"));
  Serial.println();
  Serial.print(F("Sizeof(gpsobject) = ")); Serial.println(sizeof(TinyGPS));
  Serial.println();
  Serial.println(F("Sats HDOP Latitude Longitude Fix  Date       Time       Date Alt     Course Speed Card  Chars Sentences Checksum"));
  Serial.println(F("          (deg)    (deg)     Age                        Age  (m)     --- from GPS ----  RX    RX        Fail"));
  Serial.println(F("----------------------------------------------------------------------------------------------------------------"));
}

void printGps() {
  bool newdata = false;
  unsigned long start = millis();
  
  // Every second we print an update
  while (millis() - start < 1000) {
    if (feedgps())
      newdata = true;
  }
  
  gpsdump(gps);
}

static void gpsdump(TinyGPS &gps) {
  unsigned long age, date, time, chars = 0;
  unsigned short sentences = 0, failed = 0;
  
  gps.get_position(&lat,&lng,&age);
  char buf[50];
  sprintf(buf, "My coordinates are: %d.%ld,%d.%ld",(int16_t)floor(lat/100000UL),lat%100000UL,(int16_t)floor(lng/100000UL),lng%100000UL);
  Serial.println(buf);
  
  print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5);
  print_int(gps.hdop(), TinyGPS::GPS_INVALID_HDOP, 5);
  gps.f_get_position(&flat, &flon, &age);
  
  print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 9, 5);
  print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 10, 5);
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);

  print_date(gps);

  print_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 8, 2);
  print_float(gps.f_course(), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
  print_float(gps.f_speed_kmph(), TinyGPS::GPS_INVALID_F_SPEED, 6, 2);
  print_str(gps.f_course() == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(gps.f_course()), 6);

  gps.stats(&chars, &sentences, &failed);
  print_int(chars, 0xFFFFFFFF, 6);
  print_int(sentences, 0xFFFFFFFF, 10);
  print_int(failed, 0xFFFFFFFF, 9);
  Serial.println();
}

static void print_int(unsigned long val, unsigned long invalid, int len) {
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  feedgps();
}

static void print_float(float val, float invalid, int len, int prec) {
  char sz[32];
  if (val == invalid) {
    strcpy(sz, "*******");
    sz[len] = 0;
        if (len > 0) 
          sz[len-1] = ' ';
    for (int i=7; i<len; ++i)
        sz[i] = ' ';
    Serial.print(sz);
  }
  else {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1);
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(" ");
  }
  feedgps();
}

static void print_date(TinyGPS &gps) {
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE)
    Serial.print("*******    *******    ");
  else {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d   ",
        month, day, year, hour, minute, second);
    Serial.print(sz);
  }
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  feedgps();
}

static void print_str(const char *str, int len) {
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  feedgps();
}

static bool feedgps() {
  while (Serial1.available()) {
    if (gps.encode(Serial1.read()))
      return true;
  }
  return false;
}

#endif
