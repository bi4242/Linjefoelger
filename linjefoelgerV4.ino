//Motor ID
/*int motor_1[6][2] = {{S1, 0}, {S2L, S2R}, {S3L, S3R}, {S4L, S4R}, {S5L, S5R}, {M, E}};
int motor_2[6][2] = {{S1, 0}, {S2L, S2R}, {S3L, S3R}, {S4L, S4R}, {S5L, S5R}, {M, E}};*/
int motor_1[6][2] = {{255, 0}, {200, 255}, {150, 50}, {100, 0}, {255, 0}, {1, 2}};
int motor_2[6][2] = {{255, 0}, {255, 200}, {50, 150}, {0, 100}, {0, 255}, {3, 4}};

int cutoff[5];
int kalibrering_graa[5];
int kalibrering_sort[5];
static const uint8_t analog_pins[5] = {A0, A1, A2, A3, A4};
int seneste_maaling[5] = {0, 0, 0, 0, 0};
int seneste_sorte_maaling[5] = {0, 0, 0, 0, 0};
bool mellem_punkterne;
int sensor_antal = 5;
enum SCENARIE {
    LIGEUD = 0,
    DREJ_LIDT = 1,
    DREJ_MELLEM = 2,
    DREJ_MEGET = 3
    DREJ_MERE = 4,
};
SCENARIE scenarie_vaerdi;
enum SIDE {
    VENSTRE = 0,
    HOEJRE = 1
};
SIDE retnings_vaerdi;
//bool frem_eller_bak; kan evt. bruges i scenarie 5, hvis vi vil det

void setup ()
{
    Serial.begin(9600); //til debugging
    //Databehandling - sensor pin input
    pinMode(A0, INPUT); //forreste pin
    pinMode(A1, INPUT); //1. orden venstre
    pinMode(A2, INPUT); //1. orden højre
    pinMode(A3, INPUT); //2. orden venstre
    pinMode(A4, INPUT); //2. orden højre

    //kalibrering
    pinMode(LED_BUILTIN, OUTPUT);
    int t = 0;
    while(t<10)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
        t++;
    };
    Serial.println("Kalibrering af graa paabegyndt...");
    digitalWrite(LED_BUILTIN, HIGH);
    kalibrering(kalibrering_graa[5]);
    Serial.println("Kalibrering af graa done");
    while(t<10)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
        t++;
    };
    Serial.println("Kalibrering af sort paabegyndt...");
    digitalWrite(LED_BUILTIN, HIGH);
    kalibrering(kalibrering_sort[5]);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Kalibrering af sort done");

    //find cutoff
    for(int i = 0; i<5; i++)
    {
        cutoff[i] = (kalibrering_sort[i]+kalibrering_graa[i])/2;
    };
};

void loop ()
{
    input_omsaetter();

    vaelg_scenarie();

    udfoer_scenarie();
};

void input_omsaetter( )
{
    bool er_en_sort_maaling = false;
    for(int i=0; i<sensor_antal; i++){
        if(seneste_maaling[i] == 1){
            er_en_sort_måling = true;
        };
        seneste_maaling[i] = 0; //reset målinger
        if(analogRead(analog_pins[i]) <= cutoff[i]){
            seneste_maaling[i] = 1; // 1 = sort, 0 = hvid. Kan omvendes afhængigt af cutoff
        };
    };
    if (er_en_sort_maaling)
    {
        for(int i=0; i<sensor_antal; i++)
        {
            seneste_sorte_maaling[i] = seneste_maaling[i];
        }
    }
};

void vaelg_scenarie()
{
    // sætter scenarier og retning
    mellem_punkterne = true;
    for(int i=0; i<sensor_antal; i++)
    {
        if (seneste_maaling[i] == 1)
        {
            mellem_punkterne = false;
            break;
        };
    };
    Serial.print("Mellem punkterne: ");
    Serial.println(mellem_punkterne);

    if (mellem_punkterne == false)
    {
        for(int i = 0; i < 5; i++)
        {
            seneste_sorte_maaling[i] = 0;
        }
        scenarie_vaerdi = scenarieinator(seneste_maaling);
        SIDE retnings_vaerdi = retningsinator(seneste_maaling);
    }
    else if (mellem_punkterne == true)
    {
        scenarie_vaerdi = scenarieinator(seneste_sorte_maaling);
        SIDE retnings_vaerdi = retningsinator(seneste_sorte_maaling);
    };
    Serial.print("Scenarie: ");
    Serial.println(scenarie_vaerdi);

    Serial.print("Retning: ");
    Serial.println(retnings_vaerdi);
}

int scenarieinator(int maaling[]) //enten seneste sorte måling eller sindste måling
{
    if (maaling[0] == 1 && maaling[1] == 0 && maaling[2] == 0 && maaling[3] == 0 && maaling[4] == 0)
    {
        return LIGEUD; //Scenarie 1
    }
    else if (maaling[0] == 1 && (maaling[1] == 1 || maaling[2] == 1) && maaling[3] == 0 && maaling[4] == 0)
    {
        return DREJ_LIDT; //Scenarie 2
    }
    else if (maaling[0] == 0 && (maaling[1] == 1 || maaling[2] == 1) && maaling[3] == 0 && maaling[4] == 0)
    {
        return DREJ_MELLEM; //Scenarie 3
    }
    else if (maaling[0] == 0 && ((maaling[1] == 1 && maaling[3] == 1) || (maaling[2] == 1 && maaling[4] == 1)))
    {
        return DREJ_MEGET; //Scenarie 4
    }
    else if (maaling[0] == 1 && maaling[1] == 0 && maaling[2] == 0 && (maaling[3] == 1 || maaling[4] == 1))
    {
        return DREJ_MERE; //Scenarie 5
    };
};

int retningsinator(int maaling[])
{
    if (maaling[1] == 1 || maaling[3] == 1)
    {
        return(VENSTRE);
    }
    else if (maaling[2] == 1 || maaling[4] == 1)
    {
        return(HOEJRE);
    };
};

void udfoer_scenarie()
{
    if (scenarie_vaerdi == LIGEUD) //er det scenarie LIGEUD?
    {
        motor_ligeud(LIGEUD);
    }
    else if (scenarie_vaerdi != LIGEUD) //andre scenarier
    {
        motor_styring(motor_1, retnings_vaerdi, scenarie_vaerdi);
        motor_styring(motor_2, retnings_vaerdi, scenarie_vaerdi);
    };
}

void motor_ligeud(SCENARIE scenarie)
//scenarie er altid LIGEUD
{
    int power_1 = motor_1[scenarie][0];
    int power_2 = motor_2[scenarie][0];

    //motor 1
    digitalWrite(motor_1[5][0],HIGH);
    analogWrite(motor_1[5][1], power_1);   //PWM Speed Control
    //motor 2
    digitalWrite(motor_2[5][0],HIGH);
    analogWrite(motor_2[5][1], power_2);
};

void motor_styring(int motor[6][2], int retning, SCENARIE scenarie)
{
    int power = motor[scenarie][retning];

    digitalWrite(motor[5][0],HIGH);
    analogWrite(motor[5][1], power);   //PWM Speed Control
};

void kalibrering(int array[])
{
    int kalibrering_maalinger[5][50]; //midlertidig matrix til indsamlede data
    int t = 0; //holder styr på antal målinger
    int gns_maaling[5];
    while(t<50) //for ikke at få for mange
    {
        for(int i = 0; i<=4; i++)
        {
            kalibrering_maalinger[i][t] = analogRead(analog_pins[i]); //aflæser og tilføjer
        };
        t++;
        delay(100);
    };
    //data behandling
    for(int i = 0; i<=4; i++)
    {
        int sum;
        for(int j = 0; j<50; j++)
        {
            sum += kalibrering_maalinger[i][j];
        };
        array[i] = sum/50;
        Serial.println(array[i]);
    };
};
