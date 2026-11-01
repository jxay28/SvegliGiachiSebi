# SvegliGiachiSebi

Sveglia parlante per **Sebi e Giachi**, costruita con **Arduino Nano ATmega328P**, orologio **DS3231**, display **OLED I²C 1,30" 128×64**, **DFPlayer Mini** e sensore **DHT22**. Due pulsanti personali permettono ai ragazzi di confermare il risveglio; la sequenza audio si adatta a chi deve ancora alzarsi.

Il progetto comprende firmware, messaggi MP3, animazioni, modelli SolidWorks/STL del contenitore e copie storiche del software.

> **Contenitore e DHT22:** i file SolidWorks **.SLDPRT non sono aggiornati per l’alloggiamento del DHT22**. Il contenitore già stampato in 3D è stato modificato **a mano con le forbici** per inserire il sensore. Anche gli STL e l’assieme inclusi vanno considerati riferimenti della versione precedente: non è stato verificato un alloggiamento DHT22 nei modelli. Prima di ristampare occorre adattare il progetto CAD.

## Indice

- [Funzioni](#funzioni)
- [File del progetto](#file-del-progetto)
- [Componenti e collegamenti](#componenti-e-collegamenti)
- [Installazione e caricamento](#installazione-e-caricamento)
- [MicroSD e file audio](#microsd-e-file-audio)
- [Sequenza della sveglia](#sequenza-della-sveglia)
- [Pulsanti e menu](#pulsanti-e-menu)
- [Animazioni e schermate](#animazioni-e-schermate)
- [DHT22 e calibrazione](#dht22-e-calibrazione)
- [Impostazioni e memoria](#impostazioni-e-memoria)
- [Diagnostica e problemi comuni](#diagnostica-e-problemi-comuni)
- [Contenitore e backup](#contenitore-e-backup)
- [Verifiche e limiti](#verifiche-e-limiti)

## Funzioni

- Ora e data mantenute dal DS3231, con impostazione dal menu.
- Sveglia unica con ore/minuti regolabili, attivazione salvata e modalità tutti i giorni oppure lunedì–venerdì.
- Conferme individuali: D5 per Sebi, D6 per Giachi.
- Ciclo audio completo o ridotto, con numero di ripetizioni regolabile da 1 a 20.
- Messaggio finale dopo entrambe le conferme oppure dopo esaurimento dei cicli.
- Volume delle trombe separato dal volume delle altre voci, entrambi 1–30.
- Ascolto manuale degli otto file della sveglia dal menu.
- Messaggi casuali diurni per ciascun ragazzo con smile parlante.
- Animazioni dorme, si sveglia, palla e mangia; anteprime dal menu.
- Ciclo automatico orologio → clima → animazione con tre durate indipendenti.
- Temperatura e umidità DHT22 sulla Home e in una schermata con valori grandi.
- Offset separati per temperatura e umidità, salvati in EEPROM.
- Diagnostica seriale a 115200 baud e controllo degli errori RTC, I²C e audio.

## File del progetto

| File/cartella | Contenuto |
|---|---|
| [SvegliGiachiSebi.ino](SvegliGiachiSebi.ino) | Sketch principale: setup, loop, RTC, menu, pulsanti, EEPROM, DFPlayer e sveglia |
| [Animazioni.ino](Animazioni.ino) | Pianificazione e disegno delle animazioni; gestione del ciclo delle schermate |
| [Clima.ino](Clima.ino) | Lettura DHT22, correzioni, riga compatta e schermata clima ingrandita |
| 0001.mp3–0008.mp3 | File della sveglia, conservati nella radice del repository |
| [Seby/](Seby/) | Messaggi 0010.mp3–0019.mp3 per Sebi |
| [Giachy/](Giachy/) | Messaggi 0020.mp3–0029.mp3 per Giachi |
| [animazioni 1.3 pollici.txt](animazioni%201.3%20pollici.txt) | Esempio storico delle animazioni; non è il firmware da caricare |
| .SLDPRT, .SLDASM, .STL | Parti, assieme SolidWorks ed esportazioni per il contenitore |
| [backup/](backup/) | Versioni precedenti del firmware |

**I tre file .ino attuali devono restare insieme nella cartella SvegliGiachiSebi.** Arduino IDE li compila come un unico programma. Non caricare Clima.ino o Animazioni.ino come sketch indipendenti e non copiare gli .ino dei backup nella radice: si creerebbero definizioni duplicate.

## Componenti e collegamenti

### Componenti

- Arduino Nano classico, ATmega328P, logica 5 V.
- OLED I²C 1,30", risoluzione 128×64, modulo marcato 1.30 IIC V2.2.
- Modulo RTC DS3231 con batteria di mantenimento appropriata al modulo.
- DFPlayer Mini e microSD FAT32.
- Cinque pulsanti normalmente aperti.
- DHT22/AM2302; eventuale resistenza di pull-up dati.
- Altoparlante recuperato da un portatile HP, marcato AURA 4Q 2W: interpretato come circa 4 Ω, 2 W, da verificare.
- Resistenza di circa 1 kΩ sulla seriale verso il DFPlayer; nell’esemplare realizzato circa 950 Ω ottenuti con più resistenze.
- Alimentazione 5 V stabile; disaccoppiamento vicino al DFPlayer.

### Riepilogo completo del Nano

| Pin Nano | Collegamento |
|---|---|
| D2 | Pulsante MENU/conferma, altro terminale a GND |
| D3 | Pulsante PIÙ, altro terminale a GND; sulla Home attiva/disattiva la sveglia |
| D4 | Pulsante MENO, altro terminale a GND |
| D5 | Pulsante Sebi, altro terminale a GND |
| D6 | Pulsante Giachi, altro terminale a GND |
| D7 | BUSY del DFPlayer, LOW durante riproduzione |
| D8 | DATA/OUT del DHT22 |
| D9 | Libero nel firmware attuale |
| D10 | TX del DFPlayer: ricezione SoftwareSerial del Nano |
| D11 | RX del DFPlayer, attraverso circa 1 kΩ: trasmissione SoftwareSerial del Nano |
| A4 | SDA OLED e SDA DS3231, in parallelo |
| A5 | SCL OLED e SCL DS3231, in parallelo |
| 5V | Alimentazione dei moduli compatibili con 5 V |
| GND | Massa comune di moduli, pulsanti e alimentazione |
| D0/D1 | Lasciati disponibili alla seriale USB/diagnostica |

### OLED e DS3231: bus I²C condiviso

| Modulo | Pin modulo | Nano |
|---|---|---|
| OLED | SDA | A4 |
| OLED | SCL | A5 |
| OLED | GND | GND |
| OLED | VCC | 5V solo se ammesso dal proprio modulo |
| DS3231 | SDA | A4 |
| DS3231 | SCL | A5 |
| DS3231 | VCC | 5V |
| DS3231 | GND | GND |
| DS3231 | SQW e 32K | Non collegati |

Indirizzi usati nel firmware: **OLED 0x3C**, **DS3231 0x68**. L’indirizzo OLED viene passato a U8x8 nel formato richiesto dalla libreria, 0x3C × 2.

Il controller OLED non è identificato con certezza dalla sola scritta sul modulo. La configurazione corrente è SH1106. All’inizio dello sketch, OLED_USE_SH1106 vale 1; impostarlo a 0 seleziona SSD1306 se il proprio display lo richiede.

La batteria del DS3231 mantiene l’ora a Nano spento, non alimenta tutta la sveglia. Verificare se il modulo ricarica la batteria: una CR2032 non va ricaricata. Data e ora non vengono sincronizzate da Internet e **il passaggio ora solare/legale non è automatico**.

### Pulsanti

I cinque ingressi usano INPUT_PULLUP: pulsante aperto = HIGH, premuto = LOW. Ogni pulsante chiude il proprio pin verso GND. Non occorrono resistenze esterne per questi pulsanti. Antirimbalzo software: circa 25 ms. D2 corto viene riconosciuto al rilascio; D2 lungo dopo circa 800 ms.

### DFPlayer e altoparlante

| DFPlayer | Collegamento |
|---|---|
| VCC | 5 V stabili |
| GND | Massa comune |
| TX | Nano D10, diretto |
| RX | Nano D11 attraverso circa 1 kΩ |
| BUSY | Nano D7 |
| SPK_1 | Primo terminale altoparlante |
| SPK_2 | Secondo terminale altoparlante |

Seriale DFPlayer: **9600 baud**. TX e RX sono incrociati. La resistenza non ha polarità. Il resistore da circa 950 Ω impiegato in questo progetto ha risolto il disturbo audio segnalato durante gli avvii delle tracce.

L’altoparlante va **solo tra SPK_1 e SPK_2**: nessuno dei due terminali va a GND. L’uscita è a ponte. Il numero del volume non rappresenta watt e non limita automaticamente la potenza allo speaker: partire bassi e aumentare gradualmente.

Come disaccoppiamento si possono collocare circa 100 nF ceramici e 470 µF elettrolitici tra VCC e GND vicino al DFPlayer, rispettando la polarità dell’elettrolitico. Usare collegamenti corti e un’alimentazione adeguata; evitare percorsi di massa sottili o lunghi per la corrente dello speaker.

### DHT22 su D8

| DHT22 | Nano |
|---|---|
| VCC / + | 5V, **non VIN** |
| DATA / OUT | D8 |
| GND / − | GND |
| NC, se presente | Non collegato |

Sul sensore nudo a quattro piedini serve una resistenza di circa **4,7–10 kΩ tra DATA e 5V**. Molti moduli a tre pin l’hanno già montata. Seguire le etichette e il pinout del proprio sensore, senza dedurre l’ordine dei pin da quello di un modulo differente.

VIN del Nano non è un’uscita regolata a 5 V e può ricevere tensioni maggiori: il sensore deve essere collegato al pin 5V. Collocarlo dove circola aria, lontano dal calore di Nano, regolatore e DFPlayer; un offset corregge uno scostamento fisso, non il riscaldamento variabile del contenitore.

## Installazione e caricamento

1. Clonare il repository o scaricare lo ZIP da GitHub.
2. Se la cartella scaricata si chiama SvegliGiachiSebi-main, rinominarla **SvegliGiachiSebi**.
3. Verificare che SvegliGiachiSebi.ino, Animazioni.ino e Clima.ino siano nella stessa cartella.
4. Installare Arduino IDE e il core **Arduino AVR Boards**.
5. Dal Gestore librerie installare RTClib di Adafruit, U8g2 e DFRobotDFPlayerMini. Installare anche Adafruit BusIO se richiesto da RTClib.
6. Aprire SvegliGiachiSebi.ino. Gli altri due file appariranno come schede dello stesso sketch.
7. Selezionare **Arduino Nano**, processore **ATmega328P** e porta corretta. Per alcuni Nano/cloni è necessario ATmega328P (Old Bootloader) in fase di caricamento.
8. Compilare e caricare. Il caricamento del firmware non copia gli MP3 sulla microSD.
9. Impostare data, ora, ora sveglia, giorni e volumi. Provare i file dal menu prima del primo uso.

### Librerie della compilazione verificata

| Libreria/core | Versione |
|---|---|
| Arduino AVR Boards | 1.8.8 |
| RTClib | 2.1.4 |
| Adafruit BusIO | 1.17.4 |
| U8g2, incluso U8x8lib | 2.36.19 |
| DFRobotDFPlayerMini | 1.0.6 |
| Wire, EEPROM, SoftwareSerial, SPI | Incluse nel core AVR |

Il DHT22 è letto direttamente in Clima.ino: **non serve installare una libreria DHT esterna**.

Compilazione con Arduino CLI, eseguita dalla directory che contiene la cartella del progetto:

~~~sh
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328 --warnings all SvegliGiachiSebi
~~~

## MicroSD e file audio

Usare file realmente codificati MP3 e nomi numerici a quattro cifre. Un WAV non diventa MP3 cambiandone soltanto l’estensione: va convertito. Il firmware usa playMp3Folder(numero).

**La disposizione nel repository è diversa da quella richiesta sulla scheda.** Nella microSD tutti i 28 MP3 vanno insieme nella cartella **/mp3**, senza le sottocartelle Seby e Giachy.

~~~text
microSD/
└── mp3/
    ├── 0001.mp3
    ├── 0002.mp3
    ├── 0003.mp3
    ├── 0004.mp3
    ├── 0005.mp3
    ├── 0006.mp3
    ├── 0007.mp3
    ├── 0008.mp3
    ├── 0010.mp3 ... 0019.mp3
    └── 0020.mp3 ... 0029.mp3
~~~

| File | Significato e utilizzo |
|---|---|
| 0001.mp3 | Sveglia trombe; volume dedicato |
| 0002.mp3 | Richiamo Sebi: è ora di svegliarsi |
| 0003.mp3 | Richiamo Giachi: è ora di svegliarsi |
| 0004.mp3 | Dai su ragazzi |
| 0005.mp3 | Bravo Sebi, vèstiti… |
| 0006.mp3 | Bravo Giachi, vèstiti… |
| 0007.mp3 | Ok, tutti svegli; chiusura dopo entrambe le conferme |
| 0008.mp3 | Fine del numero di cicli impostato |
| 0010.mp3–0019.mp3 | Dieci messaggi casuali diurni per D5/Sebi |
| 0020.mp3–0029.mp3 | Dieci messaggi casuali diurni per D6/Giachi |

0009.mp3 non è utilizzato. Le descrizioni riassumono lo scopo dei file, non ne costituiscono una trascrizione parola per parola.

Per sostituire gli audio spegnere il dispositivo, estrarre la scheda, copiarli con un lettore microSD e reinserirla dopo espulsione corretta. La USB del Nano non espone la microSD del DFPlayer come disco.

## Sequenza della sveglia

### Nessuno ha ancora confermato

~~~text
0001 trombe
  → pausa 1 s → 0002 Sebi
  → pausa 1 s → 0003 Giachi
  → pausa 5 s → 0004 Dai su ragazzi
  → pausa 5 s → nuovo ciclo
~~~

Le pause iniziano **alla fine del file**, rilevata tramite BUSY, non all’invio del comando play. Il numero di cicli è impostato in Ripetizioni.

### Prima conferma

| Evento | Risposta immediata | Cicli successivi |
|---|---|---|
| Sebi preme D5 | Interrompe la traccia e riproduce 0005 | Dopo 5 s: 0001 → pausa 1 s → 0003 |
| Giachi preme D6 | Interrompe la traccia e riproduce 0006 | Dopo 5 s: 0001 → pausa 1 s → 0002 |

Dopo la prima conferma vengono esclusi sia il richiamo della persona già sveglia sia **0004 Dai su ragazzi**. Finito un ciclo ridotto, la ripartenza avviene dopo 5 s, se restano cicli disponibili. Una nuova pressione della stessa persona non ripete il Bravo.

### Seconda conferma e fine

Il secondo ragazzo riceve il proprio Bravo prima della chiusura: **0005 oppure 0006 → pausa 1 s → 0007 → stop**. Se le conferme vengono rilevate nello stesso passaggio: 0005 → pausa 1 s → 0006 → pausa 1 s → 0007.

Terminare la suoneria **non disabilita l’attivazione per i giorni futuri**.

### Limite ripetizioni

Il limite da 1 a 20 conta i cicli completati, sia completi sia ridotti. La conferma non azzera il conteggio già accumulato; un ciclo interrotto non viene contato come completo.

Al termine dell’ultimo ciclo: **pausa 10 s → 0008 una sola volta → stop**. Se una conferma arriva durante la pausa, il normale trattamento delle conferme può sostituire il messaggio pendente.

L’antiripetizione è per singolo minuto RTC: nello stesso minuto la sveglia parte una volta. Spostandola a un altro minuto può suonare di nuovo nello stesso giorno. Il blocco è in RAM: un riavvio durante il minuto di allarme può permetterne un nuovo avvio.

## Pulsanti e menu

### Comandi

| Contesto | D2 | D3 | D4 | D5 / D6 |
|---|---|---|---|---|
| Home | Apre menu al rilascio | Attiva/disattiva sveglia | Ripristina dettagli | Messaggio diurno, se consentito |
| Menu | Seleziona/conferma | Scorre/aumenta | Scorre/diminuisce | Attività display |
| Suoneria | Non annulla | Non annulla | Non annulla | Conferma Sebi / Giachi |
| Anteprima animazione | Ferma/gestisce selezione | Ferma e cambia voce | Ferma e cambia voce | Ferma anteprima |

D2 lungo, fuori dalla suoneria, torna alla Home e interrompe le anteprime. I tasti ripristinano la schermata dettagliata anche durante l’inattività; mantengono la funzione prevista dal contesto.

### Menu principale

| Voce | Funzione |
|---|---|
| Ora sveglia | Ore, poi minuti |
| Attiva / spenta | Abilita la sveglia |
| Giorni sveglia | Tutti i giorni oppure lunedì–venerdì |
| Volume voci | 1–30 per tutti i file diversi da 0001, inclusi i messaggi diurni e finali |
| Volume trombe | 1–30 soltanto per 0001 |
| Ripetizioni | 1–20 cicli |
| Ascolta MP3 | Anteprime 0001–0008 e Indietro |
| Ora attuale | Ore e minuti del DS3231 |
| Data | Anno, mese e giorno |
| Menu animazioni | Impostazioni display e anteprime |
| Calibra DHT22 | Offset temperatura, poi offset umidità |
| Esci | Torna alla Home |

Le impostazioni ordinarie vengono salvate a ogni variazione; D2 lungo non annulla quelle già salvate. L’editing di data/ora è invece confermato all’ultimo campo: D2 lungo annulla i valori non confermati. Per l’anno sono previsti 2000–2099.

### Menu animazioni

| Voce | Funzione |
|---|---|
| Attiva animaz. | Attiva/disattiva il ciclo automatico e l’animazione della suoneria |
| Mostra orologio | Include l’orologio e la schermata clima nel ciclo automatico |
| Secondi orolog. | 1–60 s |
| Secondi animaz. | 1–60 s |
| Secondi temp/um | 1–60 s |
| Avvia: Dorme | Anteprima manuale |
| Avvia: Sveglia | Anteprima manuale |
| Avvia: Palla | Anteprima manuale |
| Avvia: Mangia | Anteprima manuale |
| Indietro | Menu principale |

Le anteprime funzionano anche con animazioni automatiche disabilitate. Premere un tasto le interrompe; D2 lungo torna alla Home. L’arrivo dell’orario di sveglia ha precedenza sull’anteprima.

## Animazioni e schermate

### Fasce automatiche

| Fascia, inizio incluso e fine esclusa | Animazione |
|---|---|
| 06:30–07:40 | Mangia: colazione |
| 07:40–12:00 | Gioca con la palla |
| 12:00–14:00 | Mangia: pranzo |
| 14:00–19:00 | Gioca con la palla |
| 19:00–20:00 | Mangia: cena |
| 20:00–06:30 | Dorme |

Con animazioni attive e Mostra orologio attivo: **orologio → clima → animazione**, con durate indipendenti. Valori iniziali 5 s per ciascuna fase. Il primo intervallo mostra la Home dettagliata, i successivi l’ora grande con la riga clima.

Con Mostra orologio disattivato, dopo 5 s senza attività restano le sole animazioni della fascia corrente: la schermata clima dedicata viene esclusa insieme all’orologio.

Con animazioni automatiche disattivate, dopo 5 s la Home mostra l’ora grande e la riga temperatura/umidità; non viene eseguito il ciclo automatico. Premere un tasto ripristina data, stato e ora sveglia.

La Home ha i due punti dell’ora lampeggianti. La schermata clima dedicata non ha il titolo generale: mostra etichette temperatura/umidità e **valori centrati a caratteri doppi 16×16**.

### Durante la sveglia

Con animazioni abilitate, durante una traccia viene eseguita la sequenza si sveglia: dorme, si solleva, si siede, scende dal letto, si alza e cammina a destra. Nelle pause torna la schermata sveglia. La visualizzazione segue lo stato di riproduzione gestito dal firmware tramite BUSY e relativi timeout.

### Messaggi diurni e smile parlante

Dalla Home, fra **06:30 comprese e 20:00 escluse**, senza sveglia o altra traccia in corso:

- D5 sceglie casualmente uno dei dieci MP3 da 0010 a 0019.
- D6 sceglie casualmente uno dei dieci MP3 da 0020 a 0029.
- L’ultimo file scelto per ciascuno viene escluso dalla scelta immediatamente successiva; la memoria della scelta si azzera al riavvio.
- Se D5 e D6 vengono rilevati insieme, prevale D5.
- Le pressioni durante una traccia non accodano ulteriori messaggi.

Lo smile muove la bocca con un ritmo grafico predefinito durante l’MP3: **non analizza il suono e non sincronizza i movimenti con le sillabe**. Questa animazione dei messaggi diurni funziona anche se il ciclo automatico è disattivato. Al termine torna la schermata prevista dal programma.

## DHT22 e calibrazione

Il sensore viene interrogato circa ogni **5 s**, dopo il tempo iniziale di attesa, soltanto quando non sono in corso riproduzioni o richieste audio. Durante l’intera suoneria le letture sono sospese; i valori visualizzati possono quindi essere quelli precedenti.

La lettura è una piccola macchina a stati: l’impulso iniziale non usa un’attesa lunga; la cattura dei 40 bit sospende brevemente gli interrupt ed è limitata da timeout. Vengono verificati checksum e intervalli fisici. Non è una lettura completamente asincrona: per pochi millisecondi il loop non avanza.

- Lettura valida: valori con una cifra decimale.
- Sensore assente o lettura fallita: trattini e, nella schermata dedicata, DHT22: NO DATI.
- Il codice non sostituisce un errore con un valore inventato.

### Offset

Entrare in Calibra DHT22; D3 aumenta e D4 diminuisce. D2 passa da temperatura a umidità e poi torna al menu.

| Correzione | Intervallo | Passo |
|---|---|---|
| Temperatura | −10,0…+10,0 °C | 0,1 °C |
| Umidità | −30,0…+30,0 punti percentuali | 0,1 punti percentuali |

**Valore visualizzato = lettura grezza + offset.** Per l’umidità il risultato viene limitato a 0–100%.

Esempio: il sensore legge 24,0 °C e un riferimento affidabile 23,0 °C → offset −1,0 °C. Se legge 60% e il riferimento 55% → offset −5,0 punti. Gli offset iniziano da zero e sono salvati indipendentemente dalle altre impostazioni. La diagnostica seriale riporta le misure grezze in decimi, prima degli offset.

## Impostazioni e memoria

### Valori iniziali su EEPROM non inizializzata

| Impostazione | Valore |
|---|---|
| Ora sveglia | 07:00 |
| Sveglia | Attiva |
| Giorni | Tutti |
| Volume voci | 5/30 |
| Volume trombe | Copia del volume voci al primo avvio |
| Ripetizioni | 5 |
| Animazioni | Attive |
| Mostra orologio | Attivo |
| Durate orologio / clima / animazione | 5 s ciascuna |
| Offset T / U | 0,0 |

I dati preesistenti validi hanno precedenza su questi valori. Un nuovo caricamento ordinario non azzera automaticamente l’EEPROM. Data e ora risiedono nel DS3231.

### Mappa EEPROM attuale, Nano AVR

| Indirizzi | Dato |
|---|---|
| 0–4 | Magic, ora, minuti, modalità giorni, attivazione |
| 5 | Volume voci |
| 6 | Animazioni attive |
| 7 | Volume trombe |
| 8 | Numero cicli |
| 9 | Mostra orologio |
| 10 | Secondi orologio |
| 11 | Secondi animazione |
| 12 | Secondi clima |
| 13 | Magic calibrazione |
| 14–15 | Offset temperatura int16 in decimi |
| 16–17 | Offset umidità int16 in decimi |

Conservare questa disposizione quando si estende il firmware per non reinterpretare le impostazioni precedenti.

### Ultima compilazione verificata

Target Arduino Nano ATmega328P, limite applicazione 30.720 byte:

| Risorsa | Uso |
|---|---|
| Memoria programma | **29.838 / 30.720 byte (97%)** |
| Variabili globali | **1.247 / 2.048 byte (60%)** |
| RAM residua per stack e variabili locali | **801 byte** |

La flash è quasi piena: restano 882 byte rispetto al limite del target verificato. La RAM residua non è una garanzia del massimo stack durante ogni funzione.

Il display usa U8x8 e piccoli buffer temporanei, senza un framebuffer completo da 1.024 byte. Testi e parte della grafica sono in memoria programma. Il programma evita String dinamiche. Le animazioni non impiegano i delay lunghi dell’esempio originale; comandi seriali, transazioni I²C e acquisizione DHT hanno comunque un tempo di esecuzione finito.

## Diagnostica e problemi comuni

Aprire il Monitor Seriale a **115200 baud**. L’apertura può riavviare il Nano tramite auto-reset.

| Messaggio/sintomo | Controllo |
|---|---|
| AVVIO SVEGLIA ripetuto senza aprire il Monitor | Possibili reset: alimentazione, cablaggi, disturbi, RAM |
| RTC OK | RTC raggiungibile e ora impostata |
| DATA DA IMPOSTARE | Impostare data/ora; la sveglia è inibita fino a salvataggio valido |
| OLED ERRORE | VCC/GND, A4/A5, indirizzo 0x3C e controller selezionato |
| DHT22 OK T(decimi C)=… U(decimi %)=… | Lettura grezza valida; 235 significa 23,5 °C, 567 significa 56,7% |
| DHT22 lettura fallita | DATA su D8, 5V, GND, pull-up, pinout e sensore corretto |
| MP3 numero | Comando di riproduzione inviato |
| AUDIO timeout | BUSY, presenza/nome file, microSD, alimentazione o durata traccia |
| UP(s), RAM, MIN | Tempo dal boot, RAM libera campionata e minimo campionato |

MIN non misura necessariamente il picco massimo dello stack e non è un certificato di stabilità. Non dedurre un guasto da un singolo numero: confrontare anche riavvii e comportamento del dispositivo.

### Audio sporco, click o reset quando parte un file

Controllare la resistenza D11 → RX DFPlayer, masse, cablaggi, alimentazione e condensatori vicini al modulo. Nel prototipo il problema audio di avvio è stato risolto aggiungendo circa 950 Ω sulla linea seriale. Questo non esclude altre cause su un montaggio diverso.

### Display lampeggiante o pulsanti lenti

Verificare reset sul Monitor, cadute della 5 V e bus I²C. Il programma aggiorna le celle cambiate e pulisce il display ai cambi di layout, non a ogni fotogramma. Una configurazione SH1106/SSD1306 errata può produrre una resa scorretta.

### Non si vedono temperatura e umidità

Caricare la versione attuale con tutti e tre gli .ino. Attendere almeno 5 s a riposo, verificare la riga in basso della Home e il ciclo clima se abilitato. Se sono visibili trattini, il problema è la lettura, non un valore pari a zero. Durante audio/sveglia le misure non vengono aggiornate.

### La sveglia non suona

Verificare attivazione, ora/data RTC, modalità lunedì–venerdì e giorno corrente. Nei weekend la modalità feriale non avvia la suoneria. Per riprovare nello stesso giorno scegliere un minuto diverso. Se si imposta l’ora corrente esattamente al minuto della sveglia, il salvataggio dell’orologio può marcare quel minuto come già gestito, evitando un avvio immediato.

### COM occupata / accesso negato

Chiudere Monitor Seriale, Plotter, altre IDE e programmi che tengono aperta la porta. Verificare porta, cavo e processore/bootloader. Un errore di caricamento non annulla una compilazione riuscita: sono due fasi distinte.

### Limiti temporali audio

Il firmware attende circa 2 s prima di inizializzare la seriale DFPlayer. I comandi sono distanziati di almeno circa 120 ms. L’assenza di BUSY LOW entro 4 s viene considerata errore. Una traccia oltre **120 s** raggiunge il limite TRACK_LIMIT_MS: adattarlo se occorrono file più lunghi. L’inizializzazione non verifica da sola la presenza fisica del modulo; BUSY e diagnostica sono essenziali.

## Contenitore e backup

File meccanici inclusi:

- COVER.SLDPRT e COVER.STL.
- coperchio.SLDPRT e coperchio.STL.
- coversveglia.SLDASM.

**I .SLDPRT non includono la modifica per ospitare il DHT22. Il pezzo fisico stampato è stato ritagliato a mano con le forbici.** Il software è stato aggiornato per il sensore, ma il CAD non documenta quell’adattamento. Non assumere che una nuova stampa abbia già il foro o lo spazio necessario.

Backup presenti:

| Cartella | Stato storico |
|---|---|
| backup/2026-09-26_pre_smile_parlante | Versione precedente ai messaggi diurni con smile parlante |
| backup/2026-11-01_pre_DHT22 | Versione precedente al sensore e alle schermate clima |

Per ripristinare, lavorare su una copia dei file del backup in una cartella di sketch con nome corrispondente al file principale. I backup non includono necessariamente le nuove impostazioni EEPROM o funzioni successive.

## Verifiche e limiti

La versione attuale è stata **compilata con successo per Arduino Nano ATmega328P**. I numeri sopra provengono dalla compilazione con i valori clima ingranditi. Librerie o core differenti possono modificare il consumo di memoria.

La compilazione non verifica fisicamente: tensioni, tipo effettivo del display, pinout del proprio DHT22, checksum sul cablaggio reale, corretto BUSY del DFPlayer, potenza dello speaker, qualità audio, prestazioni della microSD o dimensioni del contenitore. Il firmware e il montaggio vanno provati insieme, inclusi i due pulsanti di conferma, i cicli ridotti, il limite di ripetizioni e il finale 0008 dopo 10 s.

Non è stata aggiunta una licenza automatica: la pubblicazione del repository non attribuisce da sola una licenza specifica a firmware, audio o CAD. Eventuali riusi devono rispettare i diritti dei rispettivi autori.
