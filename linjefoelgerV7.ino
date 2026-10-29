//Motor ID
/*int motor_1[6][2] = {{S1, 0}, {S2L, S2R}, {S3L, S3R}, {S4L, S4R}, {S5L, S5R}, {M, E}};
int motor_2[6][2] = {{S1, 0}, {S2L, S2R}, {S3L, S3R}, {S4L, S4R}, {S5L, S5R}, {M, E}};*/
#define INGEN_MOTOR false
#define ANTAL_SCENARIER 8

int motor_1[ANTAL_SCENARIER + 1][2]            = {{235, 0}, {235, 235}, {235, 200}, {150, 100}, {150, 100}, {200, 100}, {150, 100}, {0, 0}, {2, 5}};
int motor_2[ANTAL_SCENARIER + 1][2]            = {{235, 0}, {235, 235}, {200, 235}, {100, 150}, {100, 150}, {100, 200}, {100, 150}, {0, 0}, {3, 6}};
int motor_1_ingen_sort[ANTAL_SCENARIER + 1][2] = {{235, 0}, {150, 100}, {150, 100}, {150, 100}, {150, 100}, {200, 100}, {150, 100}, {0, 0}, {2, 5}};
int motor_2_ingen_sort[ANTAL_SCENARIER + 1][2] = {{235, 0}, {100, 150}, {100, 150}, {100, 150}, {100, 150}, {100, 200}, {100, 150}, {0, 0}, {3, 6}};
int cutoff[5];
int kalibrering_graa[5];
int kalibrering_sort[5];
int kalibrering_maalinger = 20;
static const uint8_t analog_pins[5] = {A0, A1, A2, A3, A4};
int seneste_maaling[5] = {0, 0, 0, 0, 0};
int seneste_sorte_maaling[5] = {0, 0, 0, 0, 0};
bool mellem_punkterne;
int sensor_antal = 5;
enum SCENARIE {
    LIGEUD = 0,      //       0 1 0
                     //        0 0
    DREJ_1 = 1,      // 0 1 0
                     //  1 0
    DREJ_2 = 2,      //       0 0 0
                     //        1 0
    DREJ_3 = 3,      // 1 0 0
                     //  1 0
    DREJ_4 = 4,      //       1 1 0
                     //        0 0
    DREJ_5 = 5,      // 1 0 0
                     //  0 0
    DREJ_6 = 6,      //       1 1 0
                     //        1 0
    ALT_HVIDT = 7    // 0 0 0
                     //  0 0
};
SCENARIE scenarie_vaerdi;
enum SIDE {
    VENSTRE = 0,
    HOEJRE = 1,
};
SIDE retnings_vaerdi;
//bool frem_eller_bak; kan evt. bruges i scenarie 5, hvis vi vil det

void setup (){
    Serial.begin(9600); //til debugging
    Serial.println();
    Serial.println();
    Serial.println();
    //Databehandling - sensor pin input
    pinMode(A0, INPUT); //forreste pin
    pinMode(A1, INPUT); //1. orden venstre
    pinMode(A2, INPUT); //1. orden højre
    pinMode(A3, INPUT); //2. orden venstre
    pinMode(A4, INPUT); //2. orden højre

    pinMode(motor_1[ANTAL_SCENARIER][0], OUTPUT);
    pinMode(motor_1[ANTAL_SCENARIER][1], OUTPUT);
    pinMode(motor_2[ANTAL_SCENARIER][0], OUTPUT);
    pinMode(motor_2[ANTAL_SCENARIER][1], OUTPUT);
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
    Serial.print("Cutoffs: "); //find cutoff

    for(int i = 0; i<5; i++)
    {
        cutoff[i] = (kalibrering_sort[i]+kalibrering_graa[i])/2;
        Serial.print(cutoff[i]);
        Serial.print(", ");
    }
}

void loop (){
    input_omsaetter();
    // Serial.println("Sensorer:");
    // Serial.print(seneste_maaling[3]);
    // Serial.print(" ");
    // Serial.print(seneste_maaling[0]);
    // Serial.print(" ");
    // Serial.print(seneste_maaling[4]);
    // Serial.println();
    // Serial.print(" ");
    // Serial.print(seneste_maaling[1]);
    // Serial.print(" ");
    // Serial.print(seneste_maaling[2]);
    // Serial.println();
    vaelg_scenarie();
    // Serial.print("Scenarie: ");
    // Serial.print(scenarie_vaerdi);
    // Serial.print(", retning: ");
    // Serial.print(retnings_vaerdi);
    // Serial.print(", mellem punkterne: ");
    // Serial.print(mellem_punkterne);
    // Serial.println();
    udfoer_scenarie();
};

void input_omsaetter( ){
    bool er_en_sort_maaling = false;
    for(int i=0; i<sensor_antal; i++){
        seneste_maaling[i] = 0; //reset maalinger
        if(analogRead(analog_pins[i]) <= cutoff[i]){
            seneste_maaling[i] = 1; // 1 = sort, 0 = hvid. Kan omvendes afhængigt af cutoff
            er_en_sort_maaling = true;
        };
        // Serial.print(analogRead(analog_pins[i]));
        // Serial.print(",   ");
    };
    // Serial.println();
    if (er_en_sort_maaling)
    {
        for(int i=0; i<sensor_antal; i++)
        {
            seneste_sorte_maaling[i] = seneste_maaling[i];
        }
    }
    mellem_punkterne = !er_en_sort_maaling;
}

void vaelg_scenarie(){    // sætter scenarier og retning    mellem_punkterne = true;
    
    if (mellem_punkterne == false)
    {
        scenarie_vaerdi = scenarieinator(seneste_maaling);
        retnings_vaerdi = retningsinator(seneste_maaling);
    }
    else if (mellem_punkterne == true)
    {
        scenarie_vaerdi = scenarieinator(seneste_sorte_maaling);
        retnings_vaerdi = retningsinator(seneste_sorte_maaling);
    };
}

int scenarieinator(int maaling[]) //enten seneste sorte maaling eller sindste maaling
{
    if (maaling[0] == 1 && maaling[1] == 0 && maaling[2] == 0 && maaling[3] == 0 && maaling[4] == 0)
    {
        return LIGEUD; //Scenarie 1
    }
    else if (maaling[0] == 1 && (maaling[1] == 1 || maaling[2] == 1) && maaling[3] == 0 && maaling[4] == 0)
    {
        return DREJ_1; //Scenarie 2
    }
    else if (maaling[0] == 0 && (maaling[1] == 1 || maaling[2] == 1) && maaling[3] == 0 && maaling[4] == 0)
    {
        return DREJ_2; //Scenarie 3
    }
    else if (maaling[0] == 0 && ((maaling[1] == 1 && maaling[3] == 1) || (maaling[2] == 1 && maaling[4] == 1)))
    {
        return DREJ_3; //Scenarie 4
    }
    else if (maaling[0] == 1 && maaling[1] == 0 && maaling[2] == 0 && (maaling[3] == 1 || maaling[4] == 1))
    {
        return DREJ_4; //Scenarie 5
    }
    else if (maaling[0] == 0 && maaling[1] == 0 && maaling[2] == 0 && (maaling[3] == 1 || maaling[4] == 1))
    {
        return DREJ_5; //Scenarie 6
    }
    else if (maaling[0] == 1 && ((maaling[1] == 1 && maaling[3] == 1) || (maaling[2] == 1 && maaling[4] == 1)))
    {
        return DREJ_6; //Scenarie 7
    }
    else if (maaling[0] == 0 && maaling[1] == 0 && maaling[2] == 0 && maaling[3] == 0 && maaling[4] == 0)
    {
        return ALT_HVIDT; //Scenarie 8
    }
    return LIGEUD;
}

int retningsinator(int maaling[]){
    if (maaling[1] == 1 || maaling[3] == 1)
    {
        return(VENSTRE);
    }
    else if (maaling[2] == 1 || maaling[4] == 1)
    {
        return(HOEJRE);
    }
    return VENSTRE; // Det er sandsynligvis problematisk ikke at returnere noget.
}
void udfoer_scenarie(){
    if (!mellem_punkterne) {
        motor_styring(motor_1,            retnings_vaerdi, scenarie_vaerdi);
        motor_styring(motor_2,            retnings_vaerdi, scenarie_vaerdi);
    } else {
        motor_styring(motor_1_ingen_sort, retnings_vaerdi, scenarie_vaerdi);
        motor_styring(motor_2_ingen_sort, retnings_vaerdi, scenarie_vaerdi);
    }
    // if (scenarie_vaerdi == LIGEUD) //er det scenarie LIGEUD?    // {    //     motor_ligeud(LIGEUD);
    // }    // else if (scenarie_vaerdi != LIGEUD) //andre scenarier    // {    //     motor_styring(motor_1, retnings_vaerdi, scenarie_vaerdi);
    //     motor_styring(motor_2, retnings_vaerdi, scenarie_vaerdi);
    // };
}

// void motor_ligeud(SCENARIE scenarie)//
//scenarie er altid LIGEUD
// {
//     int power_1 = motor_1[scenarie][0];
//     int power_2 = motor_2[scenarie][0];
//     
//motor 1
//     digitalWrite(motor_1[5][0],HIGH);
//     analogWrite(motor_1[5][1], power_1);
//PWM Speed Control
//    
//motor 2
//     digitalWrite(motor_2[5][0],HIGH);
//     analogWrite(motor_2[5][1], power_2);
//};

void motor_styring(int motor[ANTAL_SCENARIER + 1][2], int retning, SCENARIE scenarie){
    int power = motor[scenarie][retning];
    if (INGEN_MOTOR) { return; }
    digitalWrite(motor[ANTAL_SCENARIER][0],HIGH); // Giver lowkey ikke mening at gøre dette hver gang.
    analogWrite(motor[ANTAL_SCENARIER][1], power);
   //PWM Speed Control
};

void kalibrering(int array[]){
    for(int i = 0; i<5; i++)
    {
        uint16_t sum = 0;
        for(int j = 0; j<kalibrering_maalinger; j++)
        {
            sum += analogRead(analog_pins[i]);
            //Serial.println(kalibrering_maalinger[i][j]);
            delay(50);
        }
        array[i] = sum/kalibrering_maalinger;
        Serial.println(array[i]);
    }
};
