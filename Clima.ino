/* DHT22 su D8. Lettura intera in decimi, senza librerie o heap.
   DATA richiede pull-up 4.7k..10k verso 5V (spesso gia' sul modulo).
   Il sensore viene interrogato solo a seriale audio inattiva.
   La cattura dei 40 bit dura pochi millisecondi, con timeout per ogni impulso. */
constexpr uint8_t DHT_PIN=8;
int16_t climateTemperature=0;
uint16_t climateHumidity=0;
bool climateValid=false,dhtStarting=false;
uint32_t lastClimateRead=0,dhtStartAt=0;

uint16_t dhtPulse(uint8_t level) {
  uint16_t count=0;
  while(digitalRead(DHT_PIN)==level) {
    if(++count>=250) return 0;
  }
  return count;
}

void updateClimate() {
  uint32_t ms=millis();
  if(!dhtStarting) {
    if(ms-lastClimateRead<5000UL||trackStarted||sounding||dayVoiceRequest||
       previewRequest||stopPending||ms-lastCommand<150UL) return;
    pinMode(DHT_PIN,OUTPUT); digitalWrite(DHT_PIN,LOW);
    dhtStartAt=ms; dhtStarting=true; return;
  }
  if(ms-dhtStartAt<2) return;
  dhtStarting=false; lastClimateRead=ms;
  // Un comando audio puo' essere arrivato durante l'impulso iniziale.
  if(trackStarted||sounding||dayVoiceRequest||previewRequest) {
    pinMode(DHT_PIN,INPUT_PULLUP); return;
  }
  uint8_t data[5]={0};
  uint8_t savedSreg=SREG;
  noInterrupts();
  pinMode(DHT_PIN,INPUT_PULLUP);
  // Rilascia la linea prima della risposta: non pilotarla HIGH mentre
  // il sensore potrebbe gia' portarla LOW.
  delayMicroseconds(5);
  bool ok=dhtPulse(HIGH)&&dhtPulse(LOW)&&dhtPulse(HIGH);
  for(uint8_t bit=0;bit<40&&ok;bit++) {
    uint16_t low=dhtPulse(LOW);
    uint16_t high=low?dhtPulse(HIGH):0;
    ok=low&&high;
    if(ok) data[bit/8]=(data[bit/8]<<1)|(high>low);
  }
  SREG=savedSreg;
  uint16_t humidity=((uint16_t)data[0]<<8)|data[1];
  uint16_t magnitude=((uint16_t)(data[2]&0x7F)<<8)|data[3];
  int16_t temperature=(data[2]&0x80)?-(int16_t)magnitude:(int16_t)magnitude;
  climateValid=ok&&((uint8_t)(data[0]+data[1]+data[2]+data[3])==data[4])&&
               humidity<=1000&&temperature>=-400&&temperature<=800;
  if(climateValid) { climateHumidity=humidity; climateTemperature=temperature; }
  if(climateValid) {
    Serial.print(F("DHT22 OK T(decimi C)=")); Serial.print(climateTemperature);
    Serial.print(F(" U(decimi %)=")); Serial.println(climateHumidity);
  } else Serial.println(F("DHT22 lettura fallita: verificare D8, 5V, GND e pull-up"));
  screenDirty=true;
}

int16_t correctedTemperature() { return climateTemperature+temperatureOffset; }
uint16_t correctedHumidity() { return constrain((int16_t)climateHumidity+humidityOffset,0,1000); }

void formatClimateLine(char *line,uint8_t length) {
  if(!climateValid) { snprintf_P(line,length,PSTR("T:--.-C U:--.-%%")); return; }
  uint16_t magnitude=abs(correctedTemperature());
  snprintf_P(line,length,PSTR("%s%u.%uC U:%u.%u%%"),correctedTemperature()<0?"-":"",
    magnitude/10,magnitude%10,correctedHumidity()/10,correctedHumidity()%10);
}

// Otto caratteri 16x16 occupano tutta la larghezza del display.
// La cache evita di riscrivere i valori a ogni passaggio del loop.
void drawClimateValue(uint8_t row,const char *value) {
  char padded[9]; memset(padded,' ',8); padded[8]=0;
  uint8_t length=strlen(value); if(length>8) length=8;
  memcpy(padded+(8-length)/2,value,length);
  if(strcmp(rowCache[row],padded)==0) return;
  display.draw2x2String(0,row,padded);
  if(Wire.getWireTimeoutFlag()) { screenOK=false; screenDirty=true; return; }
  strcpy(rowCache[row],padded);
}

void drawClimateScreen() {
  char line[16];
  drawRow(0,F("  TEMPERATURA"));
  if(climateValid) {
    uint16_t magnitude=abs(correctedTemperature());
    snprintf_P(line,sizeof(line),PSTR("%s%u.%u C"),correctedTemperature()<0?"-":"",magnitude/10,magnitude%10);
  } else strcpy_P(line,PSTR("--.- C"));
  drawClimateValue(1,line);
  drawRow(3,F("")); drawRow(4,F("    UMIDITA'"));
  if(climateValid) snprintf_P(line,sizeof(line),PSTR("%u.%u %%"),correctedHumidity()/10,correctedHumidity()%10);
  else strcpy_P(line,PSTR("--.- %"));
  drawClimateValue(5,line);
  drawRow(7,climateValid?F(""):F("DHT22: NO DATI"));
}
