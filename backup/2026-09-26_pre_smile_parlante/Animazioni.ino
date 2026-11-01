/* Animazioni OLED per SvegliGiachiSebi.
   Questo file e' un modulo dello sketch: non deve contenere setup() o loop().
   Tutti i fotogrammi sono non bloccanti, cosi' pulsanti, RTC e DFPlayer
   continuano a essere gestiti dallo sketch principale. */

enum AnimationMode : uint8_t { ANIM_NONE, ANIM_SLEEP, ANIM_WAKE, ANIM_PLAY, ANIM_EAT };
AnimationMode animationMode=ANIM_NONE;
uint8_t animationFrame=255;

uint8_t scheduledAnimation() {
  uint16_t minuteOfDay=(uint16_t)currentTime.hour()*60U+currentTime.minute();
  if(minuteOfDay>=20U*60U || minuteOfDay<6U*60U+30U) return ANIM_SLEEP;
  if(minuteOfDay<7U*60U+40U) return ANIM_EAT;   // colazione 06:30-07:40
  if(minuteOfDay<12U*60U) return ANIM_PLAY;     // mattina   07:40-12:00
  if(minuteOfDay<14U*60U) return ANIM_EAT;      // pranzo    12:00-14:00
  if(minuteOfDay<19U*60U) return ANIM_PLAY;     // pomeriggio 14:00-19:00
  return ANIM_EAT;                              // cena      19:00-20:00
}

void setAnimationState(bool active,uint8_t mode,uint32_t startedAt,uint8_t frame) {
  bool changed=animationActive!=active || animationMode!=mode ||
               animationStartedAt!=startedAt || animationFrame!=frame;
  animationActive=active;
  animationMode=(AnimationMode)mode;
  animationStartedAt=startedAt;
  animationFrame=frame;
  if(changed) screenDirty=true;
}

void updateAnimations(uint32_t ms,bool activity) {
  bool homeReady=page==HOME&&rtcOK&&rtcSet&&screenOK;

  if(activity) {
    lastActivity=ms;
    // Se un'anteprima era gia' visibile, qualunque tasto la chiude e
    // lascia aperto il sottomenu. Non cancellare la richiesta nel medesimo
    // ciclo in cui D2 avvia una nuova anteprima.
    if(animationPreview&&animationActive) animationPreview=false;
    if(clockOnly) { clockOnly=false; screenDirty=true; }
    if(animationActive) setAnimationState(false,ANIM_NONE,0,255);
    return;
  }

  // Anteprima scelta dal menu: continua fino a D2, cambio voce o uscita.
  if(animationPreview&&!sounding&&page==ANIMATION_LIST&&screenOK) {
    static const uint8_t previewModes[4]={ANIM_SLEEP,ANIM_WAKE,ANIM_PLAY,ANIM_EAT};
    uint8_t mode=previewModes[animationSelection-4];
    if(!animationActive||animationMode!=mode) {
      setAnimationState(true,mode,ms,0); return;
    }
    uint32_t elapsed=ms-animationStartedAt;
    uint8_t frame;
    if(mode==ANIM_SLEEP) frame=(elapsed/650U)&1U;
    else if(mode==ANIM_PLAY) frame=(elapsed/350U)%7U;
    else if(mode==ANIM_EAT) frame=(elapsed/450U)%4U;
    else {
      uint16_t t=elapsed%5910UL;
      if(t<650) frame=0;
      else if(t<1300) frame=1;
      else if(t<1950) frame=2;
      else if(t<2600) frame=3;
      else if(t<3250) frame=4;
      else if(t<5210) frame=5+(t-3250)/280;
      else frame=12;
    }
    if(frame!=animationFrame) { animationFrame=frame; screenDirty=true; }
    return;
  }

  // Durante l'allarme l'animazione segue la riproduzione reale indicata da BUSY.
  if(smileEnabled&&homeReady&&sounding&&trackStarted) {
    if(!animationActive||animationMode!=ANIM_WAKE) {
      setAnimationState(true,ANIM_WAKE,ms,0);
      if(clockOnly) { clockOnly=false; screenDirty=true; }
      return;
    }
    uint16_t t=(ms-animationStartedAt)%5910UL;
    uint8_t frame;
    if(t<650) frame=0;                         // dorme
    else if(t<1300) frame=1;                   // si solleva
    else if(t<1950) frame=2;                   // seduto
    else if(t<2600) frame=3;                   // scende
    else if(t<3250) frame=4;                   // in piedi
    else if(t<5210) frame=5+(t-3250)/280;      // cammina verso destra
    else frame=12;                             // letto vuoto
    if(frame!=animationFrame) { animationFrame=frame; screenDirty=true; }
    return;
  }

  if(animationActive&&animationMode==ANIM_WAKE)
    setAnimationState(false,ANIM_NONE,0,255);
  if(!homeReady||sounding) {
    if(animationActive) setAnimationState(false,ANIM_NONE,0,255);
    if(clockOnly) { clockOnly=false; screenDirty=true; }
    return;
  }

  uint32_t idle=ms-lastActivity;
  if(!smileEnabled) {
    if(animationActive) setAnimationState(false,ANIM_NONE,0,255);
    // Con animazioni disattivate, dopo 5 s resta la sola ora.
    if(!clockOnly&&idle>=SMILE_IDLE_MS) { clockOnly=true; screenDirty=true; }
    return;
  }

  uint32_t animationElapsed,animationStart;
  if(animationClockEnabled) {
    uint32_t clockMs=(uint32_t)animationClockSeconds*1000UL;
    uint32_t animationMs=(uint32_t)animationSeconds*1000UL;
    uint32_t period=clockMs+animationMs;
    uint32_t cycle=idle%period;
    if(cycle<clockMs) {
      if(animationActive) setAnimationState(false,ANIM_NONE,0,255);
      // Il primo intervallo mostra i dettagli; i successivi la sola ora.
      bool onlyClock=idle>=period;
      if(clockOnly!=onlyClock) { clockOnly=onlyClock; screenDirty=true; }
      return;
    }
    animationElapsed=cycle-clockMs;
    animationStart=ms-animationElapsed;
  } else {
    // Lascia cinque secondi per leggere HOME, poi mostra solo animazioni.
    if(idle<SMILE_IDLE_MS) return;
    animationElapsed=idle-SMILE_IDLE_MS;
    animationStart=lastActivity+SMILE_IDLE_MS;
  }

  if(clockOnly) { clockOnly=false; screenDirty=true; }
  uint8_t mode=scheduledAnimation();
  uint8_t frame;
  if(mode==ANIM_SLEEP) frame=(animationElapsed/650U)&1U;
  else if(mode==ANIM_PLAY) frame=(animationElapsed/350U)%7U;
  else frame=(animationElapsed/450U)%4U;
  if(!animationActive||animationMode!=mode) {
    setAnimationState(true,mode,animationStart,frame);
  } else if(frame!=animationFrame) {
    animationFrame=frame; screenDirty=true;
  }
}

void putAnimationText(char rows[8][17],uint8_t row,uint8_t column,const char *text) {
  if(row>=8||column>=16) return;
  while(*text&&column<16) rows[row][column++]=*text++;
}

void drawAnimationBed(char rows[8][17]) {
  putAnimationText(rows,5,1,"============");
  putAnimationText(rows,6,1,"|__________|");
  putAnimationText(rows,7,1,"|          |");
}

void drawStandingPerson(char rows[8][17],uint8_t x) {
  if(x>=16) return;
  putAnimationText(rows,1,x,"O");
  if(x) putAnimationText(rows,2,x-1,"/|\\");
  else putAnimationText(rows,2,x,"|\\");
  putAnimationText(rows,3,x,"|");
  if(x) putAnimationText(rows,4,x-1,"/ \\");
  else putAnimationText(rows,4,x,"\\");
}

void drawPlayingPerson(char rows[8][17],bool footForward) {
  putAnimationText(rows,1,3,"O");
  putAnimationText(rows,2,2,"/|\\");
  putAnimationText(rows,3,3,"|");
  putAnimationText(rows,4,footForward?2:3,footForward?"/ ":"/\\");
}

void drawEatingScene(char rows[8][17],bool armUp,bool mouthOpen) {
  putAnimationText(rows,1,3,"O");
  putAnimationText(rows,2,2,armUp?"/|--":"/|\\");
  putAnimationText(rows,3,3,"|");
  putAnimationText(rows,4,2,"/ \\");
  putAnimationText(rows,5,7,"========");
  putAnimationText(rows,6,8,"|    |");
  putAnimationText(rows,1,4,mouthOpen?"<":".");
}

void drawAnimationFrame() {
  char rows[8][17];
  for(uint8_t y=0;y<8;y++) { memset(rows[y],' ',16); rows[y][16]=0; }

  if(animationMode==ANIM_SLEEP) {
    drawAnimationBed(rows);
    putAnimationText(rows,0,animationFrame?7:6,"Z");
    putAnimationText(rows,1,animationFrame?5:4,"z");
    putAnimationText(rows,4,3,"O________");
  } else if(animationMode==ANIM_WAKE) {
    drawAnimationBed(rows);
    switch(animationFrame) {
      case 0:
        putAnimationText(rows,0,6,"Z"); putAnimationText(rows,1,4,"z");
        putAnimationText(rows,4,3,"O________"); break;
      case 1:
        putAnimationText(rows,1,4,"o"); putAnimationText(rows,2,5,"/|");
        putAnimationText(rows,3,4,"/ |__"); break;
      case 2:
        putAnimationText(rows,1,5,"O"); putAnimationText(rows,2,5,"|\\");
        putAnimationText(rows,3,5,"| \\"); putAnimationText(rows,4,5,"/"); break;
      case 3:
        drawStandingPerson(rows,7); break;
      case 4:
        drawStandingPerson(rows,10); break;
      case 12:
        break;
      default:
        drawStandingPerson(rows,10+(animationFrame-5)); break;
    }
  } else if(animationMode==ANIM_PLAY) {
    static const uint8_t ballX[7]={6,8,10,12,10,8,6};
    static const uint8_t ballY[7]={4,3,4,5,4,3,4};
    drawPlayingPerson(rows,animationFrame==0||animationFrame==6);
    putAnimationText(rows,ballY[animationFrame],ballX[animationFrame],"o");
    putAnimationText(rows,6,0,"________________");
  } else if(animationMode==ANIM_EAT) {
    bool armUp=animationFrame==1||animationFrame==3;
    bool fullBite=animationFrame<2;
    drawEatingScene(rows,armUp,armUp);
    putAnimationText(rows,armUp?2:3,armUp?7:9,fullBite?"o":".");
  }

  for(uint8_t y=0;y<8&&screenOK;y++) drawRow(y,rows[y]);
}
