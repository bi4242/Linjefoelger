//Motor ID
/*int motor_1[6][2] = {{S1, 0}, {S2L, S2R}, {S3L, S3R}, {S4L, S4R}, {S5L, S5R}, {M, E}};
int motor_2[6][2] = {{S1, 0}, {S2L, S2R}, {S3L, S3R}, {S4L, S4R}, {S5L, S5R}, {M, E}};*/
int motor_1[6][2] = {{255, 0}, {200, 255}, {150, 50}, {100, 0}, {255, 0}, {1, 2}};
int motor_2[6][2] = {{255, 0}, {255, 200}, {50, 150}, {0, 100}, {0, 255}, {3, 4}};

int cutoff[5];
int calibrering_graa[5];
int calibrering_sort[5];
static const uint8_t analog_pins[5] = {A0, A1, A2, A3, A4};
int sidste_maaling[5] = {0, 0, 0, 0, 0};
int sidste_sorte_maaling[5] = {0, 0, 0, 0, 0};
bool mellem_punkterne;
int sensor_antal = 4; //0-indekseret, så der er 5
int scenarie_vaerdi;
int retning_vaerdi;
//bool frem_eller_bak; kan evt. bruges i senarie 5, hvis vi vil det

void setup ()
{
  Serial.begin(9600); //til debugging
  //Databehandling - sensor pin input
  pinMode(A0, INPUT); //forreste pin
  pinMode(A1, INPUT); //1. orden venstre
  pinMode(A2, INPUT); //1. orden højre
  pinMode(A3, INPUT); //2. orden venstre
  pinMode(A4, INPUT); //2. orden højre

  //calibrering
  pinMode(LED_BUILTIN, OUTPUT);
  int t = 0;
  while(t<30)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    t++;
  };
  Serial.println("Kalibrering af graa paabegyndt...");
  digitalWrite(LED_BUILTIN, HIGH);
  calibrering(calibrering_graa[5]);
  Serial.println("Kalibrering af graa done");
  while(t<30)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    t++;
  };
  Serial.println("Kalibrering af sort paabegyndt...");
  digitalWrite(LED_BUILTIN, HIGH);
  calibrering(calibrering_sort[5]);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Kalibrering af sort done");

  //find cutoff
  for(int i = 0; i<5; i++)
  {
    cutoff[i] = (calibrering_sort[i]+calibrering_graa[i])/2;
  };
};

void loop ()
{
  input_omsaetter();

  // sætter senarier og retning
  mellem_punkterne = true;
  for(int i=0; i<sensor_antal; i++)
  {
    if (sidste_maaling[i] == 1)
    {
      mellem_punkterne = false;
      break;
    };
  };
  Serial.print("Mellem punkterne: ");
  Serial.println(mellem_punkterne);
  
  if (mellem_punkterne == false)
  {
    scenarie_vaerdi = scenarieinator(sidste_maaling);
    retning_vaerdi = retningsinator(sidste_maaling);
  }
  else if (mellem_punkterne == true)
  {
    scenarie_vaerdi = scenarieinator(sidste_sorte_maaling);
    retning_vaerdi = retningsinator(sidste_sorte_maaling);
  };
  Serial.print("Scenarie: ");
  Serial.println(scenarie_vaerdi);

  Serial.print("Retning: ");
  Serial.println(retning_vaerdi);
  
  if (scenarie_vaerdi == 0) //er det scenarie 1?
  {
    motor_ligeud(0);
  }
  else if (scenarie_vaerdi != 0) //andre scenarier
  {
    motor_styring(motor_1, retning_vaerdi, scenarie_vaerdi);
    motor_styring(motor_2, retning_vaerdi, scenarie_vaerdi);
  };
};

void input_omsaetter( )
{
 for(int i=0; i<sensor_antal; i++){
  if(sidste_maaling[i] == 1){
    sidste_sorte_maaling[i] = 1; //gemmer sidste sorte måling
  };
  sidste_maaling[i] = 0; //reset målinger
  if(analogRead(analog_pins[i]) <= cutoff[i]){
    sidste_maaling[i] = 1; // 1 = sort, 0 = hvid. Kan omvendes afhængigt af cutoff
  };
 };
};

int scenarieinator(int maaling[]) //enten sidste sorte måling eller sindste måling
{
  if (maaling[0] == 1 && maaling[1] == 0 && maaling[2] == 0 && maaling[3] == 0 && maaling[4] == 0)
  {
    return(0); //Scenarie 1
  }
  else if (maaling[0] == 1 && (maaling[1] == 1 || maaling[2] == 1) && maaling[3] == 0 && maaling[4] == 0)
  {
    return(1); //Scenarie 2
  }
  else if (maaling[0] == 0 && (maaling[1] == 1 || maaling[2] == 1) && maaling[3] == 0 && maaling[4] == 0)
  {
    return(2); //Scenarie 3
  }
  else if (maaling[0] == 0 && ((maaling[1] == 1 && maaling[3] == 1) || (maaling[2] == 1 && maaling[4] == 1)))
  {
    return(3); //Scenarie 4
  }
  else if (maaling[0] == 1 && maaling[1] == 0 && maaling[2] == 0 && (maaling[3] == 1 || maaling[4] == 1))
  {
    return(4); //Scenarie 5
  };
};

int retningsinator(int maaling[])
{
  if (maaling[1] == 1 || maaling[3] == 1)
  {
    return(0);
  }
  else if (maaling[2] == 1 || maaling[4] == 1)
  {
    return(1);
  };
};

void motor_ligeud(int scenarie)
//senarie er altid 1
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

void motor_styring(int motor_ID[6][2], int retning, int scenarie)
//senarierne skal være 0-indekseret
{
  int power = motor_ID[scenarie][retning];

  digitalWrite(motor_ID[5][0],HIGH);
  analogWrite(motor_ID[5][1], power);   //PWM Speed Control
  
};

void calibrering(int array[])
{
  int calibrering_maalinger[5][50]; //midlertidig matrix til indsamlede data
  int t = 0; //holder styr på antal målinger
  int gns_maaling[5];
  while(t<50) //for ikke at få for mange
  {
    for(int i = 0; i<=4; i++)
    {
      calibrering_maalinger[i][t] = analogRead(analog_pins[i]); //aflæser og tilføjer
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
      sum += calibrering_maalinger[i][j];
    };
    array[i] = sum/50;
    Serial.println(array[i]);
  };
};
