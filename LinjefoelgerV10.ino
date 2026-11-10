// Linjefølger - glidende styring (PD) ud fra linjens position under sensorerne.
// Samme kalibrering som før: først sort, så grå (= den hvide baggrund), 20 målinger pr. sensor.

#define INGEN_MOTOR false      // true = motorerne står stille (til test af sensorer)
#define DEBUG true             // true = print til Serial (115200 baud). Sæt til false når den skal køre for alvor
#define DEBUG_PLOT true        // true = print kun de 5 rå værdier som A0:..,A1:.. (til Serial Plotter), false = tekstlinje til Serial Monitor
#define TEST_MOTORER false     // true = kør en motortest i setup før kalibreringen (hjulene SKAL være i luften)
#define SOEG_BRUG_FORTEGN false // false = søgedrejet som i versionen der klarede en hel runde. true = søgedrejet bruger STYR_FORTEGN som styringen
#define SOEG_SVEJ false        // false = drejer bare ved, til linjen er fundet. true = søgningen svinger frem og tilbage (se SOEG_SVEJ_MS)
#define STYR_FORTEGN -1         // sæt til -1, hvis robotten drejer VÆK fra linjen

// ---------- Tuning ----------
#define STEP_MS 5              // hvor ofte styringen opdateres
#define BASE_FART 150          // grundfart på lige stykker (0-255, regnes om til PWM i motor_styring)
#define MIN_FART 60            // laveste grundfart i skarpe sving
#define SOEG_FART 140          // fart når linjen er mistet
#define SOEG_SVEJ_MS 600       // tid (ms) til ca. 120 grader drejning på stedet; søgningen svinger højst så langt fra hvor linjen blev mistet
#define HVID_TID_MS 30         // så længe må alt være hvidt, før linjen regnes for mistet
#define SORT_MIN_SUM 0.5       // samlet sorthed der skal til, før vi tror vi ser linjen (0,5 = én sensor ved din gamle cutoff)
#define SORTHED_STOEJ 0.5      // mørkhed (0 = baggrund, 1 = sort) under dette tæller som 0, så de grå felter ikke tæller som linje
#define DOEDZONE 8             // fartkommandoer under dette giver 0 PWM
#define KICK_MS 40             // startpuls når et hjul går fra stilstand til kørsel
float KP = 2.5;                // fartændring pr. mm fejl
float KD = 8.0;                // dæmpning (pr. mm ændring pr. styringsstep)
float KV = 1.5;                // hvor meget grundfarten sænkes pr. mm fejl

// ---------- Motorer ----------
// {M (retning), E (PWM), LOW/HIGH for fremad}. Index 0 = højre hjul (motor_1), 1 = venstre hjul (motor_2)
int motor_pin[2][3] = {{2, 5, LOW}, {3, 6, HIGH}};
// Målt uden bil på: start 134/153, stop 70/68. Tallene her er startgæt med margin - mål igen med bilen på gulvet.
int motor_start[2] = {175, 175};   // PWM der sikkert får motoren i gang fra stilstand
int motor_min[2]   = {90, 90};     // laveste PWM mens motoren kører
bool motor_koerer[2] = {false, false};
unsigned long motor_kick_slut[2] = {0, 0};

// ---------- Sensorer ----------
static const uint8_t analog_pins[5] = {A0, A1, A2, A3, A4};
// Sensorernes placering på tværs af robotten i mm (negativ = venstre): A0 forrest midt, A1/A2 bagved, A3/A4 yderst foran
const float sensor_x[5] = {0, -24, 24, -48, 48};
int kalibrering_graa[5];
int kalibrering_sort[5];
int kalibrering_maalinger = 20;
int raa_maaling[5] = {0, 0, 0, 0, 0};
float moerke[5] = {0, 0, 0, 0, 0};    // rå mørkhed: 0 = baggrund (kalibreret "graa"), 1 = sort
float sorthed[5] = {0, 0, 0, 0, 0};   // mørkhed efter fratræk af SORTHED_STOEJ, strakt til 0-1
float linje_sum = 0;                  // samlet sorthed
float position = 0;                   // linjens placering i mm (negativ = til venstre)

// ---------- Styringens tilstand ----------
unsigned long sidste_step = 0;
unsigned long seneste_sorte_tid = 0;
float fejl_forrige = 0;
float d_led = 0;
int sidste_side = -1;                 // hvilken side linjen sidst sås på (-1 venstre, 1 højre). Start -1 gav med uret på hvid på denne robot
bool har_set_linje = false;           // false indtil linjen er set første gang
unsigned long soeg_start = 0;         // tidspunkt hvor linjen blev mistet
bool var_mistet = false;
int fart_h = 0;                       // sidste fartkommando (-255..255)
int fart_v = 0;
int debug_taeller = 0;

void setup (){
    Serial.begin(115200); //til debugging (husk at sætte Serial Monitor til 115200)
    Serial.println();
    Serial.println();
    Serial.println();
    //Databehandling - sensor pin input
    pinMode(A0, INPUT); //forreste pin
    pinMode(A1, INPUT); //1. orden venstre
    pinMode(A2, INPUT); //1. orden højre
    pinMode(A3, INPUT); //2. orden venstre
    pinMode(A4, INPUT); //2. orden højre

    for (int i = 0; i < 2; i++)
    {
        pinMode(motor_pin[i][0], OUTPUT);
        pinMode(motor_pin[i][1], OUTPUT);
    }
#if TEST_MOTORER
    motortest();
#endif
    //kalibrering
    pinMode(LED_BUILTIN, OUTPUT);
    int t = 0;
    while(t<2)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
        t++;
    };
    Serial.println("Kalibrering af sort paabegyndt...");
    digitalWrite(LED_BUILTIN, HIGH);
    kalibrering(kalibrering_sort);
    Serial.println("Kalibrering af sort done");
    t = 0;
    while(t<2)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
        t++;
    };
    Serial.println("Kalibrering af graa paabegyndt...");
    digitalWrite(LED_BUILTIN, HIGH);
    kalibrering(kalibrering_graa);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Kalibrering af graa done");
    Serial.print("Kontrast graa-sort pr. sensor (A0-A4): ");   // bør være mindst ca. 150 for alle fem
    for (int i = 0; i < 5; i++) {
        Serial.print(kalibrering_graa[i] - kalibrering_sort[i]);
        Serial.print(" ");
    }
    Serial.println();
    seneste_sorte_tid = millis();
}

void loop (){
    if (millis() - sidste_step < STEP_MS) { return; }
    sidste_step = millis();

    laes_sensorer();

    if (linje_sum >= SORT_MIN_SUM)
    {
        // Linjen er under sensorerne: PD-styring på positionen
        seneste_sorte_tid = millis();
        har_set_linje = true;
        float fejl = position;
        if (fejl > 5) { sidste_side = 1; }
        else if (fejl < -5) { sidste_side = -1; }
        if (var_mistet) {          // lige fundet linjen igen: nulstil differentialleddet
            fejl_forrige = fejl;
            d_led = 0;
            var_mistet = false;
        }
        d_led = 0.7 * d_led + 0.3 * (fejl - fejl_forrige);
        fejl_forrige = fejl;

        float drej = STYR_FORTEGN * (KP * fejl + KD * d_led);
        float grund = BASE_FART - KV * (fejl < 0 ? -fejl : fejl);
        if (grund < MIN_FART) { grund = MIN_FART; }
        // fejl > 0 = linjen er til højre: højre hjul langsommere, venstre hjul hurtigere
        fart_h = begraens(grund - drej);
        fart_v = begraens(grund + drej);
    }
    else if (millis() - seneste_sorte_tid > HVID_TID_MS)
    {
        // Linjen er mistet
        if (!var_mistet) {
            var_mistet = true;
            soeg_start = millis();
        }
        int side = sidste_side;
#if SOEG_SVEJ
        if (har_set_linje) {
            // Først mod den side linjen sidst sås på, så frem og tilbage, så robotten ikke drejer helt rundt.
            // Første sving er SOEG_SVEJ_MS, derefter 2*SOEG_SVEJ_MS.
            unsigned long t = millis() - soeg_start;
            if (t >= SOEG_SVEJ_MS) {
                unsigned long fase = (t - SOEG_SVEJ_MS) / (2UL * SOEG_SVEJ_MS);
                if (fase % 2 == 0) { side = -side; }
            }
        }
#endif
#if SOEG_BRUG_FORTEGN
        bool soeg_gren = (STYR_FORTEGN * side > 0);
#else
        bool soeg_gren = (side > 0);
#endif
        if (soeg_gren) {
            fart_v = SOEG_FART;
            fart_h = -SOEG_FART / 2;
        } else {
            fart_h = SOEG_FART;
            fart_v = -SOEG_FART / 2;
        }
    }
    // ellers: kort hul uden sort, behold forrige fartkommando

    motor_styring(0, fart_h);
    motor_styring(1, fart_v);

#if DEBUG
    debug_taeller++;
    if (debug_taeller >= 10) {      // hver 50 ms
        debug_taeller = 0;
#if DEBUG_PLOT
        for (int i = 0; i < 5; i++) {   // navngivne værdier, så Serial Plotter viser A0..A4
            Serial.print("A");
            Serial.print(i);
            Serial.print(":");
            Serial.print(raa_maaling[i]);
            Serial.print(i < 4 ? "," : "");
        }
        Serial.println();
#else
        for (int i = 0; i < 5; i++) {   // rå værdier i rækkefølgen A0 A1 A2 A3 A4
            Serial.print(raa_maaling[i]);
            Serial.print(i < 4 ? " " : " | ");
        }
        for (int i = 0; i < 5; i++) {   // mørkhed i % (0 = baggrund, 100 = sort)
            Serial.print((int)(moerke[i] * 100));
            Serial.print(i < 4 ? " " : " | ");
        }
        Serial.print("sum=");
        Serial.print(linje_sum);
        Serial.print(" pos=");
        Serial.print(position);
        Serial.print(" fartH=");
        Serial.print(fart_h);
        Serial.print(" fartV=");
        Serial.print(fart_v);
        Serial.print(" mistet=");
        Serial.println(var_mistet);
#endif
    }
#endif
}

// Hvert hjul skal dreje fremad = mod sensorsiden. Rettes ved at bytte LOW/HIGH i motor_pin (3. værdi).
void motortest(){
    Serial.println("Hoejre hjul frem (motor_1)");
    motor_styring(0, 150);
    delay(1500);
    motor_styring(0, 0);
    delay(500);
    Serial.println("Venstre hjul frem (motor_2)");
    motor_styring(1, 150);
    delay(1500);
    motor_styring(1, 0);
    delay(500);
    Serial.println("Begge frem");
    motor_styring(0, 150);
    motor_styring(1, 150);
    delay(1500);
    motor_styring(0, 0);
    motor_styring(1, 0);
    delay(500);
    Serial.println("Drej paa stedet mod hoejre (venstre frem, hoejre bagud)");
    motor_styring(1, 150);
    motor_styring(0, -150);
    delay(1500);
    motor_styring(0, 0);
    motor_styring(1, 0);
    delay(1000);
}

void laes_sensorer(){
    float vsum = 0;
    linje_sum = 0;
    for (int i = 0; i < 5; i++)
    {
        analogRead(analog_pins[i]); // blind læsning: ADC'en skal nå at skifte kanal (høj kildemodstand i spændingsdeleren)
        raa_maaling[i] = analogRead(analog_pins[i]);
        int spaend = kalibrering_graa[i] - kalibrering_sort[i];
        if (spaend < 1) { spaend = 1; }
        // Sort giver lav værdi, baggrunden høj. m: 0 = baggrund, 1 = helt sort
        float m = (float)(kalibrering_graa[i] - raa_maaling[i]) / spaend;
        moerke[i] = m;
        // Kun det mørkeste tæller (de grå felter er mørkere end baggrunden, men lysere end linjen)
        float s = (m - SORTHED_STOEJ) / (1 - SORTHED_STOEJ);
        if (s < 0) { s = 0; }
        if (s > 1) { s = 1; }
        sorthed[i] = s;
        linje_sum += s;
        vsum += s * sensor_x[i];
    }
    if (linje_sum > 0) {
        position = vsum / linje_sum;
    }
}

int begraens(float v){
    if (v > 255) { return 255; }
    if (v < -255) { return -255; }
    return (int)v;
}

void motor_styring(int id, int fart){
    if (INGEN_MOTOR) { return; }
    int frem_bak = motor_pin[id][2];
    if (fart < 0) {
        frem_bak = !frem_bak;
        fart = -fart;
    }
    int pwm = 0;
    if (fart < DOEDZONE) {
        motor_koerer[id] = false;
    } else {
        // 1-255 i fartkommandoen fordeles over motor_min..255 i PWM, så motorernes dødzone ikke mærkes i styringen
        pwm = motor_min[id] + (long)fart * (255 - motor_min[id]) / 255;
        if (!motor_koerer[id]) {          // går fra stilstand til kørsel: startpuls
            motor_koerer[id] = true;
            motor_kick_slut[id] = millis() + KICK_MS;
        }
        if (millis() < motor_kick_slut[id] && pwm < motor_start[id]) {
            pwm = motor_start[id];
        }
    }
    digitalWrite(motor_pin[id][0], frem_bak);
    analogWrite(motor_pin[id][1], pwm);
   //PWM Speed Control
}

void kalibrering(int array[]){
    for(int i = 0; i<5; i++)
    {
        uint16_t sum = 0;
        for(int j = 0; j<kalibrering_maalinger; j++)
        {
            sum += analogRead(analog_pins[i]);
            delay(50);
        }
        array[i] = sum/kalibrering_maalinger;
        Serial.println(array[i]);
    }
}
