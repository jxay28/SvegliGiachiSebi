/* Sveglia Nano + DS3231 + OLED I2C 1.30 pollici + DFPlayer Mini
   Librerie: RTClib (Adafruit), U8g2 (include U8x8),
             DFRobotDFPlayerMini. Pulsanti verso GND (INPUT_PULLUP).
   DFPlayer: microSD FAT32, cartella /mp3, otto file realmente codificati MP3:
   0001.mp3 = Sveglia trombe (convertire il WAV, non solo rinominarlo)
   Volume trombe: solo 0001.mp3; Volume voci: file 0002..0007.mp3.
   Entrambi 1..30, salvati in EEPROM, validi anche nelle anteprime.
   0002.mp3 = Sveglia Sebi; 0003.mp3 = Sveglia Giachi
   0004.mp3 = Dai su ragazzi; 0005.mp3 = Bravo Sebi
   0006.mp3 = Bravo Giachi; 0007.mp3 = Ok tutti svegli.
   0008.mp3 = messaggio di fine del limite cicli.
   Ciclo: 1, pausa 1 s, 2, pausa 1 s, 3, pausa 5 s, 4, pausa 5 s, ripeti.
   Le pause partono dalla FINE del file. D5 conferma Sebi, D6 conferma Giachi.
   Prima conferma: interrompe il file, risposta 5/6 una volta, pausa 5 s,
   ricomincia da 1 saltando il richiamo della persona gia' sveglia.
   Quando almeno uno ha confermato, il ciclo ridotto NON riproduce 0004.mp3.
   Menu Ripetizioni: 1..20 cicli completi; raggiunto il limite la suoneria
   attende 10 secondi, riproduce 0008.mp3 una volta e termina. Il conteggio
   vale sia per il ciclo con due persone sia con una.
   Seconda conferma: interrompe, Bravo della seconda persona (5/6), pausa 1 s,
   file 7 una volta, termina la suoneria (l'attivazione per i giorni futuri resta).
   Conferme simultanee: Bravo Sebi, pausa 1 s, Bravo Giachi, pausa 1 s, file 7.
   OLED 128x64, indirizzo 0x3C. Core Arduino AVR Boards recente.
   Modulo marcato 1.30 IIC V2.2: controller non identificato con certezza.
   Configurazione di prova SH1106; OLED_USE_SH1106=0 seleziona SSD1306.
   HOME: D2 apre il menu, D3 attiva/disattiva la sveglia (salvata in EEPROM).
   MENU: D3/D4 scorrono, D2 seleziona/conferma. D2 lungo torna a HOME.
   MODIFICA: D3/D4 cambiano il valore, D2 conferma il campo.
   MP3: scegliere un file con D3/D4, D2 avvia/ferma; Indietro torna al menu.
   Data/ora salvate all'ultimo campo. D2 lungo annulla data/ora non confermate.
   Durante la suoneria solo D5/D6 confermano; D3 non interrompe l'allarme.
   Con animazioni attive HOME alterna orologio e animazione con durate 1..60 s
   configurabili; l'orologio puo' essere escluso dal sottomenu animazioni:
   dorme 20:00-06:30; mangia 06:30-07:40, 12:00-14:00 e 19:00-20:00;
   gioca 07:40-12:00 e 14:00-19:00. Durante la riproduzione dei file della
   sveglia mostra l'animazione non bloccante "si sveglia".
   Qualsiasi tasto ripristina i dettagli e mantiene la propria funzione.
   Menu Animazioni: attive/spente, salvato separatamente in EEPROM.
   Con le animazioni spente, dopo 5 s di inattivita' HOME mostra soltanto l'ora.
   Se RTC perde alimentazione impostare data/ora: sveglia inibita fino al salvataggio.
   EEPROM compatibile con la versione precedente. Volume iniziale 5 su 30.
   Speaker HP AURA marcato "4Q 2W": presumibilmente 4 ohm, 2 W (da verificare).
   La scala volume 1..30 NON e' una misura dei watt ne' un limitatore a 2 W.
   D11 -> resistenza 1 kohm -> RX DFPlayer; TX DFPlayer -> D10; BUSY -> D7.
   Speaker SOLO tra SPK1/SPK2. Masse comuni, 5 V stabili, collegamenti corti.
   Bypass vicino al DFPlayer: per esempio 100 nF + 470 uF.
   Verificare con oscilloscopio cadute di alimentazione e disturbo prima dell'MP3.
   Monitor 115200: AVVIO ripetuto indica un nuovo setup (anche aprendo il Monitor).
   RAM MIN e' il minimo campionato, non il massimo consumo dello stack.
   Nessun framebuffer OLED da 1024 byte. Nessuna attesa bloccante della fine MP3.
   I comandi DFPlayer occupano comunque circa 21 ms (seriale + libreria).
   Timeout traccia 120 secondi: aumentare TRACK_LIMIT_MS per file piu' lunghi.
   Blocco antiripetizione per singolo minuto: cambiando l'ora della sveglia puo'
   suonare nuovamente nello stesso giorno; nello stesso minuto parte una sola volta.
*/
#include <Wire.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <RTClib.h>
#include <U8x8lib.h>
#include <DFRobotDFPlayerMini.h>
#include <stdio.h>
#include <string.h>

constexpr uint8_t P_MENU=2, P_UP=3, P_DOWN=4, P_ONE=5, P_TWO=6;
constexpr uint8_t DF_RX=10, DF_TX=11; // Nano RX <- DF TX; Nano TX -> DF RX
constexpr uint8_t P_BUSY=7;           // DF BUSY, LOW durante riproduzione
constexpr uint8_t EEPROM_MAGIC=0xA7;
constexpr uint8_t DEFAULT_VOLUME=5, MAX_VOLUME=30; // Conserva il volume salvato; aumentare gradualmente.
// Se il venditore conferma SSD1306, cambiare solo questo valore in 0.
#define OLED_USE_SH1106 1
#if OLED_USE_SH1106
U8X8_SH1106_128X64_NONAME_HW_I2C display(U8X8_PIN_NONE);
#else
U8X8_SSD1306_128X64_NONAME_HW_I2C display(U8X8_PIN_NONE);
#endif
SoftwareSerial dfSerial(DF_RX,DF_TX);
DFRobotDFPlayerMini player;
bool audioOK=false, screenOK=false, previewPlaying=false;
bool dayVoicePlaying=false;
bool rtcOK=false, rtcSet=false, clockDirty=false, volumePending=true;
bool stopPending=false, audioError=false;
uint8_t previewRequest=0;
uint8_t dayVoiceRequest=0,lastDayTrackOne=0,lastDayTrackTwo=0;
constexpr uint8_t AUDIO_COUNT=8;
uint8_t alarmRequest=0;
uint8_t queuedThanks=0; // seconda risposta solo per conferme simultanee
uint16_t audioGapMs=0;
uint32_t lastCommand=0, audioGapAt=0, lastPoll=0, lastDiag=0;
uint32_t lastAlarmMinuteKey=0xFFFFFFFFUL;
constexpr uint32_t TRACK_LIMIT_MS=120000UL;
uint16_t editYear=2026;
uint8_t editMonth=1,editDay=1;
DateTime currentTime(2026,1,1,0,0,0);
char rowCache[8][17];

struct Settings { uint8_t magic,hour,minute,weekdaysOnly,enabled; } cfg;
// Volume in EEPROM separata: conserva le impostazioni della vecchia versione.
constexpr int VOLUME_EEPROM_ADDR=sizeof(Settings);
constexpr int SMILE_EEPROM_ADDR=VOLUME_EEPROM_ADDR+1;
constexpr int TRUMPET_VOLUME_EEPROM_ADDR=SMILE_EEPROM_ADDR+1;
constexpr int LOOP_COUNT_EEPROM_ADDR=TRUMPET_VOLUME_EEPROM_ADDR+1;
constexpr int ANIM_CLOCK_ENABLED_EEPROM_ADDR=LOOP_COUNT_EEPROM_ADDR+1;
constexpr int ANIM_CLOCK_SECONDS_EEPROM_ADDR=ANIM_CLOCK_ENABLED_EEPROM_ADDR+1;
constexpr int ANIM_SECONDS_EEPROM_ADDR=ANIM_CLOCK_SECONDS_EEPROM_ADDR+1;
constexpr int CLIMATE_SECONDS_EEPROM_ADDR=ANIM_SECONDS_EEPROM_ADDR+1;
constexpr int OFFSET_MAGIC_ADDR=CLIMATE_SECONDS_EEPROM_ADDR+1;
constexpr int OFFSET_T_ADDR=OFFSET_MAGIC_ADDR+1;
constexpr int OFFSET_H_ADDR=OFFSET_T_ADDR+2;
int16_t temperatureOffset=0,humidityOffset=0; // decimi di C e punti percentuali
constexpr uint8_t MENU_COUNT=12,MAX_ALARM_LOOPS=20;
enum Page : uint8_t { HOME, MENU_LIST, AUDIO_LIST, ANIMATION_LIST, ANIM_CLOCK_ENABLED, ANIM_CLOCK_SECONDS, ANIM_SECONDS, CLIMATE_SECONDS, OFFSET_TEMP, OFFSET_HUM, ALARM_H, ALARM_M, DAYS, ENABLED, VOLUME, TRUMPET_VOLUME, LOOP_COUNT, SMILE_ENABLED, CLOCK_Y, CLOCK_MON, CLOCK_D, CLOCK_H, CLOCK_M };
Page page=HOME;
uint8_t menuSelection=0,audioSelection=0,animationSelection=0;
bool animationPreview=false;
bool animationClockEnabled=true;
uint8_t animationClockSeconds=5,animationSeconds=5;
bool menuLongHandled=false,clockSaveError=false;
uint32_t menuPressedAt=0;
int8_t paintedLayout=-1;
uint8_t paintedDigits[4]={255,255,255,255};
int8_t paintedColon=-1;
uint8_t editHour=0,editMinute=0,volumeLevel=DEFAULT_VOLUME;
uint8_t trumpetVolume=DEFAULT_VOLUME,appliedVolume=255;
uint8_t alarmLoopLimit=5,alarmLoopsDone=0;
bool sounding=false, doneOne=false, doneTwo=false, trackStarted=false, busySeen=false;
uint8_t currentTrack=0, lastMinute=255, lastHour=255;
uint32_t trackIssuedAt=0;
bool screenDirty=true;
uint32_t lastPress[5]={0};
bool lastButton[5]={false};
bool stableButton[5]={false};
const uint8_t pins[5]={P_MENU,P_UP,P_DOWN,P_ONE,P_TWO};
constexpr uint32_t SMILE_IDLE_MS=5000UL,SMILE_DURATION_MS=5000UL;
uint32_t lastActivity=0,animationStartedAt=0;
bool animationActive=false;
bool smileEnabled=true;
bool clockOnly=false;
bool climateScreen=false;
uint8_t climateSeconds=5;

bool pressed(uint8_t i) {
  bool down=(digitalRead(pins[i])==LOW);
  if(down!=lastButton[i]) { lastPress[i]=millis(); lastButton[i]=down; }
  if(down!=stableButton[i] && millis()-lastPress[i]>=25) {
    stableButton[i]=down;
    if(i==0) {
      if(down) { menuPressedAt=millis(); menuLongHandled=false; return false; }
      return !menuLongHandled; // D2 corto al rilascio, lungo non conferma
    }
    return down;
  }
  return false;
}
int freeRam() {
  extern int __heap_start; extern void *__brkval; int local;
  return (int)&local-(__brkval?(int)__brkval:(int)&__heap_start);
}
uint8_t monthDays(uint16_t y,uint8_t m) {
  if(m==2) return y%4==0?29:28; // anni 2000..2099
  return m==4||m==6||m==9||m==11?30:31;
}
bool probe(uint8_t address) {
  Wire.clearWireTimeoutFlag(); Wire.beginTransmission(address);
  uint8_t error=Wire.endTransmission();
  return error==0 && !Wire.getWireTimeoutFlag();
}
bool readClock() {
  // Verifica anche il numero di byte ricevuti: rtc.now() da solo non lo espone.
  Wire.clearWireTimeoutFlag(); Wire.beginTransmission(0x68); Wire.write((uint8_t)0);
  if(Wire.endTransmission(false)!=0) return false;
  if(Wire.requestFrom((uint8_t)0x68,(uint8_t)7)!=7) {
    while(Wire.available()) Wire.read(); return false;
  }
  uint8_t b[7]; for(uint8_t i=0;i<7;i++) b[i]=Wire.read();
  uint8_t h=b[2]&((b[2]&0x40)?0x1F:0x3F);
  for(uint8_t i=0;i<7;i++) {
    if(i==3) continue;
    uint8_t v=i==2?h:(i==5?(b[i]&0x1F):b[i]);
    if((v&15)>9||(v>>4)>9) return false;
  }
  uint8_t sec=(b[0]>>4)*10+(b[0]&15),minu=(b[1]>>4)*10+(b[1]&15);
  uint8_t hour=(h>>4)*10+(h&15);
  if(b[2]&0x40) { if(hour<1||hour>12) return false; hour=hour%12+((b[2]&0x20)?12:0); }
  uint8_t d=(b[4]>>4)*10+(b[4]&15),m=((b[5]&0x1F)>>4)*10+(b[5]&15);
  uint16_t y=2000+(b[6]>>4)*10+(b[6]&15);
  if((b[5]&0x80)||sec>59||minu>59||hour>23||m<1||m>12||d<1||d>monthDays(y,m)) return false;
  Wire.beginTransmission(0x68); Wire.write((uint8_t)0x0F);
  if(Wire.endTransmission(false)!=0||Wire.requestFrom((uint8_t)0x68,(uint8_t)1)!=1) return false;
  rtcSet=(Wire.read()&0x80)==0;
  if(Wire.getWireTimeoutFlag()) return false;
  currentTime=DateTime(y,m,d,hour,minu,sec); return true;
}
uint8_t toBCD(uint8_t v) { return (v/10)*16+v%10; }
bool writeClock() {
  // Scritture controllate: niente allocazioni heap ne' letture non inizializzate.
  Wire.clearWireTimeoutFlag();
  Wire.beginTransmission(0x68); Wire.write((uint8_t)0x0F);
  if(Wire.endTransmission(false)!=0||Wire.requestFrom((uint8_t)0x68,(uint8_t)1)!=1) return false;
  uint8_t status=Wire.read();
  DateTime dt(editYear,editMonth,editDay,editHour,editMinute,0);
  Wire.beginTransmission(0x68); Wire.write((uint8_t)0);
  Wire.write((uint8_t)0); Wire.write(toBCD(editMinute)); Wire.write(toBCD(editHour));
  Wire.write((uint8_t)(dt.dayOfTheWeek()+1)); Wire.write(toBCD(editDay));
  Wire.write(toBCD(editMonth)); Wire.write(toBCD(editYear-2000));
  if(Wire.endTransmission()!=0||Wire.getWireTimeoutFlag()) return false;
  Wire.beginTransmission(0x68); Wire.write((uint8_t)0x0F); Wire.write((uint8_t)(status&0x7F));
  if(Wire.endTransmission()!=0||Wire.getWireTimeoutFlag()) return false;
  return readClock()&&rtcSet&&currentTime.year()==editYear&&currentTime.month()==editMonth&&
    currentTime.day()==editDay&&currentTime.hour()==editHour&&currentTime.minute()==editMinute;
}
void saveSettings() { EEPROM.put(0,cfg); }
void stopPreview() {
  if(!sounding) { stopPending=true; trackStarted=false; previewPlaying=false; previewRequest=0; }
}
void stopAlarm() {
  sounding=false; trackStarted=false; currentTrack=0; screenDirty=true;
  stopPending=true; previewRequest=0; previewPlaying=false;
  alarmRequest=0;
  queuedThanks=0;
  alarmLoopsDone=0;
}
bool isDayVoiceTime() {
  uint16_t minuteOfDay=(uint16_t)currentTime.hour()*60U+currentTime.minute();
  return minuteOfDay>=6U*60U+30U&&minuteOfDay<20U*60U;
}
void requestDayVoice(bool personOne) {
  if(sounding||page!=HOME||!rtcOK||!rtcSet||!isDayVoiceTime()||
     trackStarted||dayVoiceRequest) return;
  uint8_t &previous=personOne?lastDayTrackOne:lastDayTrackTwo;
  uint8_t first=personOne?10:20,count=10; // Sebi 0010..0019, Giachi 0020..0029
  uint8_t selected;
  do selected=first+random(count); while(selected==previous&&count>1);
  previous=selected; dayVoiceRequest=selected;
  screenDirty=true;
  Serial.print(personOne?F("D5 messaggio giorno "):F("D6 messaggio giorno "));
  Serial.println(selected);
}
void startAlarm() {
  sounding=true; doneOne=false; doneTwo=false; trackStarted=false; currentTrack=0; screenDirty=true;
  stopPending=true; previewPlaying=false; previewRequest=0; audioGapAt=millis();
  page=HOME; clockDirty=false; clockSaveError=false;
  alarmRequest=1; audioGapMs=0;
  queuedThanks=0;
  alarmLoopsDone=0;
  animationPreview=false;
  dayVoiceRequest=0; dayVoicePlaying=false;
}
void confirmAwake(bool one,bool two) {
  bool newOne=one&&!doneOne,newTwo=two&&!doneTwo;
  if(!sounding||(!newOne&&!newTwo)) return;
  if(newOne) doneOne=true;
  if(newTwo) doneTwo=true;
  // Anche la seconda persona riceve il proprio Bravo prima del messaggio finale.
  alarmRequest=newOne?5:6;
  queuedThanks=newOne&&newTwo?6:0;
  trackStarted=false; stopPending=true; previewPlaying=false; previewRequest=0;
  audioGapAt=millis(); audioGapMs=0; screenDirty=true;
}
void completeAlarmCycle(uint32_t ms) {
  if(++alarmLoopsDone>=alarmLoopLimit) {
    Serial.println(F("Limite cicli raggiunto"));
    // L'attesa parte dalla fine reale dell'ultimo MP3, rilevata tramite BUSY.
    alarmRequest=8; audioGapAt=ms; audioGapMs=10000;
  } else {
    alarmRequest=1; audioGapAt=ms; audioGapMs=5000;
  }
}
void alarmTrackFinished(uint32_t ms) {
  audioGapAt=ms;
  switch(currentTrack) {
    case 1:
      alarmRequest=doneOne?(doneTwo?4:3):2; audioGapMs=1000; break;
    case 2:
      if(doneTwo) completeAlarmCycle(ms);
      else { alarmRequest=3; audioGapMs=1000; }
      break;
    case 3:
      if(doneOne) completeAlarmCycle(ms);
      else { alarmRequest=4; audioGapMs=5000; }
      break;
    case 4:
      completeAlarmCycle(ms); break;
    case 5: case 6:
      if(queuedThanks) {
        alarmRequest=queuedThanks; queuedThanks=0; audioGapMs=1000;
      } else if(doneOne&&doneTwo) {
        alarmRequest=7; audioGapMs=1000;
      } else { alarmRequest=1; audioGapMs=5000; }
      break;
    case 7:
      stopAlarm(); break; // nessun altro ciclo, nemmeno se si ripreme D5/D6
    case 8:
      stopAlarm(); break; // messaggio di fine del limite cicli
    default:
      alarmRequest=1; audioGapMs=5000; break;
  }
}
void updateAudio() {
  uint32_t ms=millis();
  if(!audioOK) {
    if(ms<2000) return;
    player.begin(dfSerial,false,false); audioOK=true;
    stopPending=true; volumePending=true;
    Serial.println(F("DF seriale pronta: presenza NON verificata, attendere BUSY"));
  }
  if(player.available()) {
    uint8_t type=player.readType(); int value=player.read();
    if(type==DFPlayerError||type==DFPlayerCardRemoved) {
      audioError=true; screenDirty=true;
      Serial.print(F("DF evento/errore ")); Serial.print(type); Serial.print(' '); Serial.println(value);
    }
  }
  if(trackStarted) {
    static uint32_t highSince=0;
    static bool highPending=false;
    bool low=digitalRead(P_BUSY)==LOW;
    if(low) { if(!busySeen) { audioError=false; screenDirty=true; } busySeen=true; highPending=false; }
    else if(!highPending) { highSince=ms; highPending=true; }
    bool failed=(!busySeen&&ms-trackIssuedAt>=4000)||ms-trackIssuedAt>=TRACK_LIMIT_MS;
    if(failed||(busySeen&&!low&&ms-highSince>=50)) {
      bool wasDayVoice=dayVoicePlaying;
      trackStarted=false; previewPlaying=false; dayVoicePlaying=false;
      audioGapAt=ms; screenDirty=true; highPending=false;
      if(failed) { stopPending=true; audioError=true; Serial.println(F("AUDIO timeout: verificare BUSY/SD/alimentazione")); }
      if(sounding) alarmTrackFinished(ms);
      else if(wasDayVoice) Serial.println(F("Messaggio giorno terminato"));
    }
  }
  if(ms-lastCommand<120) return;
  if(stopPending) { player.stop(); stopPending=false; lastCommand=millis(); return; }
  // Imposta il volume PRIMA del play, senza consumare la richiesta pendente.
  // Durante la pausa prepara gia' il volume del prossimo file; i comandi
  // restano distanziati di 120 ms e non bloccano pulsanti o display.
  uint8_t pendingTrack=dayVoiceRequest?dayVoiceRequest:previewRequest;
  uint8_t volumeTrack=trackStarted?currentTrack:(sounding?alarmRequest:pendingTrack);
  uint8_t targetVolume=volumeTrack==1?trumpetVolume:volumeLevel;
  if(volumePending||appliedVolume!=targetVolume) {
    player.volume(targetVolume); appliedVolume=targetVolume; volumePending=false;
    lastCommand=millis(); return;
  }
  if(trackStarted) return;
  if(!sounding&&dayVoiceRequest) {
    currentTrack=dayVoiceRequest; dayVoiceRequest=0; dayVoicePlaying=true;
  }
  else if(!sounding&&previewRequest) { currentTrack=previewRequest; previewRequest=0; previewPlaying=true; }
  else if(sounding&&alarmRequest&&ms-audioGapAt>=audioGapMs) {
    currentTrack=alarmRequest; alarmRequest=0;
  }
  else return;
  player.playMp3Folder(currentTrack);
  lastCommand=trackIssuedAt=millis(); trackStarted=true; busySeen=false; screenDirty=true;
  Serial.print(F("MP3 ")); Serial.println(currentTrack);
}
void drawRow(uint8_t y,const char *s) {
  if(!screenOK) return;
  char padded[17]; memset(padded,' ',16); padded[16]=0;
  uint8_t n=strlen(s); if(n>16) n=16; memcpy(padded,s,n);
  for(uint8_t x=0;x<16;x++) if(rowCache[y][x]!=padded[x]) {
    display.drawGlyph(x,y,padded[x]);
    if(Wire.getWireTimeoutFlag()) { screenOK=false; screenDirty=true; return; }
    rowCache[y][x]=padded[x];
  }
}
void drawRow(uint8_t y,const __FlashStringHelper *s) {
  char text[17]; strncpy_P(text,(PGM_P)s,16); text[16]=0; drawRow(y,text);
}
// Cifre a sette segmenti 24x32: solo 24 byte temporanei, nessun framebuffer.
void drawBigDigit(uint8_t tileX,uint8_t digit) {
  static const uint8_t segments[10] PROGMEM={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
  uint8_t mask=digit<10?pgm_read_byte(segments+digit):0x40;
  for(uint8_t tileY=0;tileY<4;tileY++) {
    uint8_t pixels[24]={0};
    for(uint8_t x=0;x<24;x++) for(uint8_t b=0;b<8;b++) {
      uint8_t y=tileY*8+b;
      bool on=((mask&1)&&x>=5&&x<=18&&y>=1&&y<=3)||
        ((mask&2)&&x>=19&&x<=21&&y>=4&&y<=13)||
        ((mask&4)&&x>=19&&x<=21&&y>=17&&y<=27)||
        ((mask&8)&&x>=5&&x<=18&&y>=28&&y<=30)||
        ((mask&16)&&x>=2&&x<=4&&y>=17&&y<=27)||
        ((mask&32)&&x>=2&&x<=4&&y>=4&&y<=13)||
        ((mask&64)&&x>=5&&x<=18&&y>=14&&y<=16);
      if(on) pixels[x]|=(1<<b);
    }
    display.drawTile(tileX,(clockOnly?2:1)+tileY,3,pixels);
    if(Wire.getWireTimeoutFlag()) { screenOK=false; screenDirty=true; return; }
  }
}
void drawColon(bool on) {
  for(uint8_t tileY=0;tileY<4;tileY++) {
    uint8_t pixels[16]={0};
    for(uint8_t x=6;x<=9;x++) for(uint8_t b=0;b<8;b++) {
      uint8_t y=tileY*8+b;
      if(on&&((y>=8&&y<=11)||(y>=21&&y<=24))) pixels[x]|=(1<<b);
    }
    display.drawTile(7,tileY+(clockOnly?2:1),2,pixels);
    if(Wire.getWireTimeoutFlag()) { screenOK=false; screenDirty=true; return; }
  }
}
void menuLabel(uint8_t item,char *line) {
  switch(item) {
    case 0: strcpy_P(line,PSTR("Ora sveglia")); break;
    case 1: strcpy_P(line,PSTR("Attiva / spenta")); break;
    case 2: strcpy_P(line,PSTR("Giorni sveglia")); break;
    case 3: strcpy_P(line,PSTR("Volume voci")); break;
    case 4: strcpy_P(line,PSTR("Volume trombe")); break;
    case 5: strcpy_P(line,PSTR("Ripetizioni")); break;
    case 6: strcpy_P(line,PSTR("Ascolta MP3")); break;
    case 7: strcpy_P(line,PSTR("Ora attuale")); break;
    case 8: strcpy_P(line,PSTR("Data")); break;
    case 9: strcpy_P(line,PSTR("Menu animazioni")); break;
    case 10: strcpy_P(line,PSTR("Calibra DHT22")); break;
    default: strcpy_P(line,PSTR("Esci")); break;
  }
}
void animationLabel(uint8_t item,char *line) {
  switch(item) {
    case 0: strcpy_P(line,PSTR("Attiva animaz.")); break;
    case 1: strcpy_P(line,PSTR("Mostra orologio")); break;
    case 2: strcpy_P(line,PSTR("Secondi orolog.")); break;
    case 3: strcpy_P(line,PSTR("Secondi animaz.")); break;
    case 4: strcpy_P(line,PSTR("Secondi temp/um")); break;
    case 5: strcpy_P(line,PSTR("Avvia: Dorme")); break;
    case 6: strcpy_P(line,PSTR("Avvia: Sveglia")); break;
    case 7: strcpy_P(line,PSTR("Avvia: Palla")); break;
    case 8: strcpy_P(line,PSTR("Avvia: Mangia")); break;
    default: strcpy_P(line,PSTR("Indietro")); break;
  }
}
void audioLabel(uint8_t item,char *line) {
  switch(item) {
    case 0: strcpy_P(line,PSTR("Sveglia trombe")); break;
    case 1: strcpy_P(line,PSTR("2 Sveglia Sebi")); break;
    case 2: strcpy_P(line,PSTR("3 Sveglia Giachi")); break;
    case 3: strcpy_P(line,PSTR("4 Dai ragazzi")); break;
    case 4: strcpy_P(line,PSTR("5 Bravo Sebi")); break;
    case 5: strcpy_P(line,PSTR("6 Bravo Giachi")); break;
    case 6: strcpy_P(line,PSTR("7 Tutti svegli")); break;
    case 7: strcpy_P(line,PSTR("8 Fine cicli")); break;
    default: strcpy_P(line,PSTR("Indietro")); break;
  }
}
void drawScreen(const DateTime& now) {
  if(!screenOK||!screenDirty) return;
  screenDirty=false; char line[24];
  Wire.clearWireTimeoutFlag();
  int8_t layout=climateScreen?4:animationActive?2:(page==HOME?(clockOnly?3:0):1);
  if(layout!=paintedLayout) {
    display.clearDisplay(); // solo al cambio schermata, mai tra frame/lampeggi
    memset(rowCache,0,sizeof(rowCache)); memset(paintedDigits,255,sizeof(paintedDigits));
    paintedColon=-1; paintedLayout=layout;
    if(Wire.getWireTimeoutFlag()) { screenOK=false; screenDirty=true; return; }
  }
  if(climateScreen) {
    drawClimateScreen(); return;
  }
  if(animationActive) {
    drawAnimationFrame();
    return;
  }
  if(page==HOME) {
    if(rtcOK&&rtcSet) {
      static const char week[7][4] PROGMEM={"DOM","LUN","MAR","MER","GIO","VEN","SAB"};
      char day[4]; strcpy_P(day,week[now.dayOfTheWeek()]);
      snprintf_P(line,sizeof(line),PSTR(" %s %02u/%02u/%04u"),day,now.day(),now.month(),now.year());
    } else strcpy_P(line,rtcOK?PSTR("IMPOSTA DATA/ORA"):PSTR("RTC NON RISPONDE"));
    if(!clockOnly) drawRow(0,line);
    uint8_t digits[4]={uint8_t(now.hour()/10),uint8_t(now.hour()%10),uint8_t(now.minute()/10),uint8_t(now.minute()%10)};
    const uint8_t xs[4]={1,4,9,12};
    for(uint8_t i=0;i<4&&screenOK;i++) {
      uint8_t value=rtcOK&&rtcSet?digits[i]:255;
      if(value!=paintedDigits[i]||paintedColon==-1) { drawBigDigit(xs[i],value); paintedDigits[i]=value; }
    }
    bool colon=rtcOK&&rtcSet&&(now.second()%2==0);
    if(screenOK&&paintedColon!=colon) { drawColon(colon); paintedColon=colon; }
    if(clockOnly) {
      formatClimateLine(line,sizeof(line)); drawRow(7,line);
      return;
    }
    snprintf_P(line,sizeof(line),PSTR(" SVEGLIA  %02u:%02u"),cfg.hour,cfg.minute); drawRow(5,line);
    if(sounding) snprintf_P(line,sizeof(line),PSTR("Sebi:%s Gia:%s"),doneOne?"OK":"..",doneTwo?"OK":"..");
    else if(cfg.enabled) strcpy_P(line,cfg.weekdaysOnly?PSTR("ATTIVA   LUN-VEN"):PSTR("ATTIVA  TUTTI"));
    else strcpy_P(line,PSTR("     SPENTA"));
    drawRow(6,line);
    if(sounding) strcpy_P(line,audioError?PSTR("ERRORE AUDIO"):(doneOne&&doneTwo?PSTR("TUTTI SVEGLI!"):PSTR("D5:Sebi D6:Gia")));
    else formatClimateLine(line,sizeof(line));
    drawRow(7,line);
  } else {
    // Scrive solo le celle modificate: nessuna cancellazione tra voci del menu.
    if(page==MENU_LIST) {
      snprintf_P(line,sizeof(line),PSTR("MENU       %2u/%u"),(unsigned)(menuSelection+1),(unsigned)MENU_COUNT); drawRow(0,line);
      drawRow(1,F("----------------"));
      uint8_t first=menuSelection<4?0:menuSelection-3;
      for(uint8_t i=0;i<4;i++) {
        line[0]=first+i==menuSelection?'>':' '; menuLabel(first+i,line+1); drawRow(i+2,line);
      }
      drawRow(6,F("+/- scegli D2:OK")); drawRow(7,F("D2 lungo: esci"));
    } else if(page==AUDIO_LIST) {
      snprintf_P(line,sizeof(line),PSTR("AUDIO        %u/8"),(unsigned)(audioSelection+1)); drawRow(0,line);
      drawRow(1,F("----------------"));
      uint8_t first=audioSelection<4?0:audioSelection-3;
      for(uint8_t i=0;i<4;i++) {
        line[0]=first+i==audioSelection?'>':' ';
        audioLabel(first+i,line+1);
        drawRow(i+2,line);
      }
      drawRow(6,previewPlaying||previewRequest?F("D2: ferma MP3"):F("D2: ascolta/OK"));
      drawRow(7,audioError?F("ERRORE AUDIO"):F("+/- seleziona"));
    } else if(page==ANIMATION_LIST) {
      snprintf_P(line,sizeof(line),PSTR("ANIMAZIONI  %u/10"),(unsigned)(animationSelection+1)); drawRow(0,line);
      drawRow(1,F("----------------"));
      uint8_t first=animationSelection<4?0:animationSelection-3;
      for(uint8_t i=0;i<4;i++) {
        line[0]=first+i==animationSelection?'>':' ';
        animationLabel(first+i,line+1); drawRow(i+2,line);
      }
      drawRow(6,animationPreview?F("D2: ferma"):F("D2: seleziona"));
      drawRow(7,F("+/- seleziona"));
    } else {
      if(page==SMILE_ENABLED) strcpy_P(line,PSTR("Attiva animaz."));
      else if(page==ANIM_CLOCK_ENABLED) strcpy_P(line,PSTR("Mostra orologio"));
      else if(page==ANIM_CLOCK_SECONDS) strcpy_P(line,PSTR("Secondi orolog."));
      else if(page==ANIM_SECONDS) strcpy_P(line,PSTR("Secondi animaz."));
      else if(page==CLIMATE_SECONDS) strcpy_P(line,PSTR("Secondi temp/um"));
      else if(page==OFFSET_TEMP) strcpy_P(line,PSTR("Offset temp."));
      else if(page==OFFSET_HUM) strcpy_P(line,PSTR("Offset umidita"));
      else menuLabel(menuSelection,line);
      drawRow(0,line); drawRow(1,F("----------------"));
      drawRow(2,F("")); char marker[17]="",note[17]=""; line[0]=0;
      switch(page) {
        case ALARM_H: case ALARM_M:
          snprintf_P(line,sizeof(line),PSTR("     %02u:%02u"),cfg.hour,cfg.minute);
          strcpy_P(marker,page==ALARM_H?PSTR("     ^^ ore"):PSTR("        ^^ min")); break;
        case ENABLED: strcpy_P(line,cfg.enabled?PSTR("     ATTIVA"):PSTR("     SPENTA")); break;
        case SMILE_ENABLED: strcpy_P(line,smileEnabled?PSTR("     ATTIVA"):PSTR("     SPENTA")); break;
        case DAYS: strcpy_P(line,cfg.weekdaysOnly?PSTR("    LUN - VEN"):PSTR(" TUTTI I GIORNI")); break;
        case VOLUME: case TRUMPET_VOLUME:
          snprintf_P(line,sizeof(line),PSTR("     %02u / 30"),page==TRUMPET_VOLUME?trumpetVolume:volumeLevel);
          strcpy_P(note,page==TRUMPET_VOLUME?PSTR("Solo 0001.mp3"):PSTR("Audio 0002-0007")); break;
        case LOOP_COUNT:
          snprintf_P(line,sizeof(line),PSTR("     %02u volte"),alarmLoopLimit);
          strcpy_P(note,PSTR("Poi si arresta")); break;
        case ANIM_CLOCK_ENABLED:
          strcpy_P(line,animationClockEnabled?PSTR("     ATTIVO"):PSTR("   DISATTIVATO")); break;
        case ANIM_CLOCK_SECONDS:
          snprintf_P(line,sizeof(line),PSTR("   %02u secondi"),animationClockSeconds); break;
        case OFFSET_TEMP: case OFFSET_HUM: {
          int16_t offset=page==OFFSET_TEMP?temperatureOffset:humidityOffset;
          uint16_t magnitude=abs(offset);
          snprintf_P(line,sizeof(line),PSTR("   %c%u.%u %s"),offset<0?'-':'+',magnitude/10,magnitude%10,page==OFFSET_TEMP?"C":"%");
          strcpy_P(note,PSTR("Passo 0.1")); break;
        }
        case CLIMATE_SECONDS:
          snprintf_P(line,sizeof(line),PSTR("   %02u secondi"),climateSeconds); break;
        case ANIM_SECONDS:
          snprintf_P(line,sizeof(line),PSTR("   %02u secondi"),animationSeconds); break;
        case CLOCK_H: case CLOCK_M:
          snprintf_P(line,sizeof(line),PSTR("     %02u:%02u"),editHour,editMinute);
          strcpy_P(marker,page==CLOCK_H?PSTR("     ^^ ore"):PSTR("        ^^ min")); break;
        case CLOCK_Y: case CLOCK_MON: case CLOCK_D:
          snprintf_P(line,sizeof(line),PSTR("   %02u/%02u/%04u"),editDay,editMonth,editYear);
          strcpy_P(marker,page==CLOCK_Y?PSTR("         ^^^^"):(page==CLOCK_MON?PSTR("      ^^ mese"):PSTR("   ^^ giorno"))); break;
        default: break;
      }
      drawRow(3,line); drawRow(4,marker);
      if(clockSaveError) strcpy_P(note,PSTR("ERRORE RTC: D2"));
      drawRow(5,note);
      drawRow(6,F("+/- cambia D2:OK")); drawRow(7,F("D2 lungo: esci"));
    }
  }
  if(Wire.getWireTimeoutFlag()) { screenOK=false; screenDirty=true; }
}
void toggleAlarm() {
  cfg.enabled=!cfg.enabled; saveSettings(); screenDirty=true;
  Serial.println(cfg.enabled?F("Sveglia ATTIVA"):F("Sveglia SPENTA"));
}
void beginClockEdit() {
  editYear=currentTime.year(); editMonth=currentTime.month(); editDay=currentTime.day();
  editHour=currentTime.hour(); editMinute=currentTime.minute(); clockDirty=false; clockSaveError=false;
}
bool finishClockEdit(bool dateOnly) {
  if(clockDirty||!rtcSet) {
    if(dateOnly&&rtcOK) { editHour=currentTime.hour(); editMinute=currentTime.minute(); }
    // Se si imposta solo l'ora, conserva la data piu' recente (anche a mezzanotte).
    if(!dateOnly&&rtcOK) { editYear=currentTime.year(); editMonth=currentTime.month(); editDay=currentTime.day(); }
    if(!writeClock()) { rtcOK=false; clockSaveError=true; Serial.println(F("RTC salvataggio fallito")); return false; }
    rtcOK=true; clockDirty=false; clockSaveError=false;
    if(currentTime.hour()==cfg.hour&&currentTime.minute()==cfg.minute)
      lastAlarmMinuteKey=currentTime.unixtime()/60UL;
  }
  return true;
}
void navigateMenu(bool menu,bool up,bool down) {
  int8_t delta=up==down?0:(up?1:-1);
  if(page==HOME) {
    if(menu) { page=MENU_LIST; screenDirty=true; }
    else if(up) toggleAlarm();
    return;
  }
  if(menu) {
    screenDirty=true;
    if(page==MENU_LIST) {
      switch(menuSelection) {
        case 0: page=ALARM_H; break;
        case 1: page=ENABLED; break;
        case 2: page=DAYS; break;
        case 3: page=VOLUME; break;
        case 4: page=TRUMPET_VOLUME; break;
        case 5: page=LOOP_COUNT; break;
        case 6: page=AUDIO_LIST; audioSelection=0; break;
        case 7: beginClockEdit(); page=CLOCK_H; break;
        case 8: beginClockEdit(); page=CLOCK_Y; break;
        case 9: page=ANIMATION_LIST; animationSelection=0; animationPreview=false; break;
        case 10: page=OFFSET_TEMP; break;
        default: page=HOME; break;
      }
    } else if(page==AUDIO_LIST) {
      if(audioSelection==AUDIO_COUNT) { stopPreview(); page=MENU_LIST; }
      else if(previewPlaying||previewRequest) stopPreview();
      else { stopPreview(); previewRequest=audioSelection+1; }
    } else if(page==ANIMATION_LIST) {
      if(animationSelection==0) page=SMILE_ENABLED;
      else if(animationSelection==1) page=ANIM_CLOCK_ENABLED;
      else if(animationSelection==2) page=ANIM_CLOCK_SECONDS;
      else if(animationSelection==3) page=ANIM_SECONDS;
      else if(animationSelection==4) page=CLIMATE_SECONDS;
      else if(animationSelection==9) { animationPreview=false; page=MENU_LIST; }
      else animationPreview=!animationPreview;
    } else if(page==OFFSET_TEMP) page=OFFSET_HUM;
    else if(page==ALARM_H) page=ALARM_M;
    else if(page==CLOCK_H) page=CLOCK_M;
    else if(page==CLOCK_Y) page=CLOCK_MON;
    else if(page==CLOCK_MON) page=CLOCK_D;
    else if(page==CLOCK_M||page==CLOCK_D) { if(finishClockEdit(page==CLOCK_D)) page=MENU_LIST; }
    else if(page==SMILE_ENABLED||page==ANIM_CLOCK_ENABLED||page==ANIM_CLOCK_SECONDS||page==ANIM_SECONDS||page==CLIMATE_SECONDS) page=ANIMATION_LIST;
    else page=MENU_LIST;
    return;
  }
  if(!delta) return;
  screenDirty=true; clockSaveError=false;
  switch(page) {
    case MENU_LIST: menuSelection=(menuSelection+MENU_COUNT+delta)%MENU_COUNT; break;
    case AUDIO_LIST: stopPreview(); audioSelection=(audioSelection+AUDIO_COUNT+1+delta)%(AUDIO_COUNT+1); break;
    case ANIMATION_LIST:
      animationPreview=false;
      animationSelection=(animationSelection+10+delta)%10; break;
    case OFFSET_TEMP:
      temperatureOffset=constrain(temperatureOffset+delta,-100,100);
      EEPROM.put(OFFSET_T_ADDR,temperatureOffset); break;
    case OFFSET_HUM:
      humidityOffset=constrain(humidityOffset+delta,-300,300);
      EEPROM.put(OFFSET_H_ADDR,humidityOffset); break;
    case CLIMATE_SECONDS:
      climateSeconds=constrain((int)climateSeconds+delta,1,60);
      EEPROM.update(CLIMATE_SECONDS_EEPROM_ADDR,climateSeconds); break;
    case ANIM_CLOCK_ENABLED:
      animationClockEnabled=!animationClockEnabled;
      EEPROM.update(ANIM_CLOCK_ENABLED_EEPROM_ADDR,animationClockEnabled); break;
    case ANIM_CLOCK_SECONDS:
      animationClockSeconds=constrain((int)animationClockSeconds+delta,1,60);
      EEPROM.update(ANIM_CLOCK_SECONDS_EEPROM_ADDR,animationClockSeconds); break;
    case ANIM_SECONDS:
      animationSeconds=constrain((int)animationSeconds+delta,1,60);
      EEPROM.update(ANIM_SECONDS_EEPROM_ADDR,animationSeconds); break;
    case ALARM_H: cfg.hour=(cfg.hour+24+delta)%24; saveSettings(); break;
    case ALARM_M: cfg.minute=(cfg.minute+60+delta)%60; saveSettings(); break;
    case DAYS: cfg.weekdaysOnly=!cfg.weekdaysOnly; saveSettings(); break;
    case ENABLED: toggleAlarm(); break;
    case SMILE_ENABLED:
      smileEnabled=!smileEnabled; EEPROM.update(SMILE_EEPROM_ADDR,smileEnabled); break;
    case VOLUME:
      volumeLevel=constrain((int)volumeLevel+delta,1,(int)MAX_VOLUME);
      EEPROM.update(VOLUME_EEPROM_ADDR,volumeLevel); volumePending=true; break;
    case TRUMPET_VOLUME:
      trumpetVolume=constrain((int)trumpetVolume+delta,1,(int)MAX_VOLUME);
      EEPROM.update(TRUMPET_VOLUME_EEPROM_ADDR,trumpetVolume); volumePending=true; break;
    case LOOP_COUNT:
      alarmLoopLimit=constrain((int)alarmLoopLimit+delta,1,(int)MAX_ALARM_LOOPS);
      EEPROM.update(LOOP_COUNT_EEPROM_ADDR,alarmLoopLimit); break;
    case CLOCK_Y: editYear=2000+(editYear-2000+100+delta)%100; break;
    case CLOCK_MON: editMonth=1+(editMonth-1+12+delta)%12; break;
    case CLOCK_D: editDay=1+(editDay-1+monthDays(editYear,editMonth)+delta)%monthDays(editYear,editMonth); break;
    case CLOCK_H: editHour=(editHour+24+delta)%24; break;
    case CLOCK_M: editMinute=(editMinute+60+delta)%60; break;
    default: break;
  }
  if(page>=CLOCK_Y) { clockDirty=true; editDay=min(editDay,monthDays(editYear,editMonth)); }
}
void setup() {
  Serial.begin(115200);
  Serial.println(F("AVVIO SVEGLIA"));
  for(uint8_t i=0;i<5;i++) pinMode(pins[i],INPUT_PULLUP);
  pinMode(P_BUSY,INPUT_PULLUP);
  Wire.begin(); Wire.setWireTimeout(25000,true);
  display.setI2CAddress(0x3C*2); display.setBusClock(100000);
  screenOK=probe(0x3C);
  if(screenOK) { display.begin(); display.setFont(u8x8_font_chroma48medium8_r); }
  Wire.setWireTimeout(25000,true);
  rtcOK=readClock();
  Serial.print(F("RTC ")); Serial.println(rtcOK?(rtcSet?F("OK"):F("DATA DA IMPOSTARE")):F("ERRORE"));
  Serial.print(F("OLED ")); Serial.println(screenOK?F("OK"):F("ERRORE"));
  EEPROM.get(0,cfg);
  if(cfg.magic!=EEPROM_MAGIC || cfg.hour>23 || cfg.minute>59 || cfg.weekdaysOnly>1 || cfg.enabled>1) {
    cfg={EEPROM_MAGIC,7,0,0,1}; saveSettings();
  }
  volumeLevel=EEPROM.read(VOLUME_EEPROM_ADDR);
  if(volumeLevel<1 || volumeLevel>MAX_VOLUME) {
    volumeLevel=DEFAULT_VOLUME; EEPROM.update(VOLUME_EEPROM_ADDR,volumeLevel);
  }
  dfSerial.begin(9600);
  trumpetVolume=EEPROM.read(TRUMPET_VOLUME_EEPROM_ADDR);
  if(trumpetVolume<1||trumpetVolume>MAX_VOLUME) {
    // Prima installazione: stessa intensita' del volume generale gia' salvato.
    trumpetVolume=volumeLevel; EEPROM.update(TRUMPET_VOLUME_EEPROM_ADDR,trumpetVolume);
  }
  alarmLoopLimit=EEPROM.read(LOOP_COUNT_EEPROM_ADDR);
  if(alarmLoopLimit<1||alarmLoopLimit>MAX_ALARM_LOOPS) {
    alarmLoopLimit=5; EEPROM.update(LOOP_COUNT_EEPROM_ADDR,alarmLoopLimit);
  }
  uint8_t savedSmile=EEPROM.read(SMILE_EEPROM_ADDR);
  if(savedSmile>1) { savedSmile=1; EEPROM.update(SMILE_EEPROM_ADDR,savedSmile); }
  smileEnabled=savedSmile!=0;
  uint8_t savedClockEnabled=EEPROM.read(ANIM_CLOCK_ENABLED_EEPROM_ADDR);
  if(savedClockEnabled>1) {
    savedClockEnabled=1; EEPROM.update(ANIM_CLOCK_ENABLED_EEPROM_ADDR,savedClockEnabled);
  }
  animationClockEnabled=savedClockEnabled!=0;
  animationClockSeconds=EEPROM.read(ANIM_CLOCK_SECONDS_EEPROM_ADDR);
  if(animationClockSeconds<1||animationClockSeconds>60) {
    animationClockSeconds=5; EEPROM.update(ANIM_CLOCK_SECONDS_EEPROM_ADDR,animationClockSeconds);
  }
  animationSeconds=EEPROM.read(ANIM_SECONDS_EEPROM_ADDR);
  if(animationSeconds<1||animationSeconds>60) {
    animationSeconds=5; EEPROM.update(ANIM_SECONDS_EEPROM_ADDR,animationSeconds);
  }
  climateSeconds=EEPROM.read(CLIMATE_SECONDS_EEPROM_ADDR);
  if(climateSeconds<1||climateSeconds>60) { climateSeconds=5; EEPROM.update(CLIMATE_SECONDS_EEPROM_ADDR,climateSeconds); }
  if(EEPROM.read(OFFSET_MAGIC_ADDR)==0xC2) {
    EEPROM.get(OFFSET_T_ADDR,temperatureOffset); EEPROM.get(OFFSET_H_ADDR,humidityOffset);
  }
  if(temperatureOffset<-100||temperatureOffset>100) temperatureOffset=0;
  if(humidityOffset<-300||humidityOffset>300) humidityOffset=0;
  EEPROM.put(OFFSET_T_ADDR,temperatureOffset); EEPROM.put(OFFSET_H_ADDR,humidityOffset);
  EEPROM.update(OFFSET_MAGIC_ADDR,0xC2);
  pinMode(8,INPUT_PULLUP);
  Serial.print(F("RAM libera setup: ")); Serial.println(freeRam());
  randomSeed(micros()^currentTime.unixtime());
  lastActivity=millis();
}
void loop() {
  uint32_t ms=millis();
  if(ms-lastPoll>=250) {
    lastPoll=ms; bool oldOK=rtcOK,oldSet=rtcSet;
    rtcOK=readClock();
    if(oldOK!=rtcOK||oldSet!=rtcSet) { screenDirty=true; Serial.println(rtcOK?F("RTC risponde"):F("RTC errore I2C/dati")); }
  }
  const DateTime &now=currentTime;
  bool menu=pressed(0), up=pressed(1), down=pressed(2);
  bool one=pressed(3), two=pressed(4);
  if(menu) Serial.println(F("TASTO MENU"));
  if(up) Serial.println(F("TASTO PIU"));
  if(down) Serial.println(F("TASTO MENO"));
  if(one) Serial.println(F("TASTO PERSONA 1"));
  if(two) Serial.println(F("TASTO PERSONA 2"));
  if(sounding) confirmAwake(one,two);
  else if(one||two) requestDayVoice(one); // se simultanei, D5 ha precedenza
  if(stableButton[0]&&!menuLongHandled&&millis()-menuPressedAt>=800) {
    menuLongHandled=true;
    if(!sounding) { stopPreview(); animationPreview=false; page=HOME; clockDirty=false; clockSaveError=false; screenDirty=true; }
    menu=false;
  }
  if(!sounding&&!one&&!two) navigateMenu(menu,up,down);
  static uint8_t lastSecond=255;
  if(now.second()!=lastSecond) {
    lastSecond=now.second(); if(page==HOME) screenDirty=true;
  }
  // Display aggiornato su cambiamenti, senza cancellazioni globali.
  if(now.hour()!=lastHour || now.minute()!=lastMinute) {
    screenDirty=true;
    lastHour=now.hour(); lastMinute=now.minute();
  }
  bool dayOK=!cfg.weekdaysOnly || (now.dayOfTheWeek()>=1 && now.dayOfTheWeek()<=5);
  uint32_t alarmMinuteKey=now.unixtime()/60UL;
  if(rtcOK&&rtcSet&&!sounding&&cfg.enabled&&dayOK&&alarmMinuteKey!=lastAlarmMinuteKey&&
     now.hour()==cfg.hour&&now.minute()==cfg.minute) {
    lastAlarmMinuteKey=alarmMinuteKey; startAlarm();
  }
  updateAudio();
  updateClimate();
  static uint32_t lastScreenProbe=0;
  if(ms-lastScreenProbe>=2000) {
    lastScreenProbe=ms;
    if(!probe(0x3C)) screenOK=false;
    else if(!screenOK) {
      display.begin(); display.setFont(u8x8_font_chroma48medium8_r); Wire.setWireTimeout(25000,true);
      memset(rowCache,0,sizeof(rowCache)); paintedLayout=-1; screenOK=true; screenDirty=true;
    }
  }
  // Anche un pulsante tenuto premuto conta come attivita'. D2 viene comunque
  // eseguito al rilascio e D3 conserva la sua funzione di attivazione.
  bool activity=menu||up||down||one||two;
  for(uint8_t i=0;i<5;i++) activity|=lastButton[i]||stableButton[i];
  updateAnimations(millis(),activity);
  drawScreen(now);
  static int minimum=32767;
  int available=freeRam(); if(available<minimum) minimum=available;
  if(ms-lastDiag>=10000) {
    lastDiag=ms; Serial.print(F("UP(s)=")); Serial.print(ms/1000UL);
    Serial.print(F(" RAM=")); Serial.print(available); Serial.print(F(" MIN=")); Serial.println(minimum);
  }
}
