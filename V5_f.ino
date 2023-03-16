// #include <avr/sleep.h> //librerias para modos de bajo consumo
// #define interruptPin 2
#include <Wire.h>              //libreria para i2c
#include <LiquidCrystal_I2C.h> //libreria para pantalla
// revisar dropout voltage with led's

// definir pines
const int button4 = 2; // this button can use interrupts to wake up the device in future codes, use this when waiting for user input
const int button1 = 4;
const int button2 = 7;
const int button3 = 8;

const int led_k = 3;
const int led_chl = 5; // also known as led_n
const int led_p = 6;

const int vref_generate = 9;
const int vcc_control = 10;

const int vref_feedback = A3;
const int sensor_amplified = A1;
const int battery_feedback = A0;

int button_input = 0;

// bool flag_pboton = false;
// bool flag_chlboton = false;
// bool flag_kboton = false;

// unsigned long tiempo_anterior = 0;
// unsigned long tiempo_actual = 0;

// const long duracion = 5000;
LiquidCrystal_I2C lcd(0x27, 20, 4); // para direccionar a 0x27 (pin A4 y A5)

// Amplificación y bias

float R1 = 8060.0;
float R2 = 3300.0;
float R3 = 1500.0;
float R4 = 10000.0;
float Rc = 500.0; // ohm

float A = (R4 / (R3 + R4)) * ((R1 + R2) / (R2));
float b_ = R1 / R2;

// otras variables
float vcc_source = 0.0;
float vcc_controlled = 0.0;
int warmtime = 1000;    // ms
int measuretime = 9000; // ms
float sensor_middle_value = 0.0;
float battery_level_percentage = 0.0;
float vref_max=0.0;
float vref_min=0.0;

// user variables
int language = 0;   // 0 english |  1 spanish
char element = 'E'; //  E empty, N nitrogen, P phosphorus, K potassium

// measurement variables //to-redefine
float I = 0.0;
float VREF_FB = 0.0;
float PHT_S = 0.0;
float prom_vref_blank = 0.0;
float prom_sensor_blank = 0.0;
float prom_vref_color = 0.0;
float prom_sensor_color = 0.0;
float calculated_V_pht_blank = 0.0;
float calculated_V_pht_color = 0.0;
float calculated_absorbance = 0.0;
float calculated_concentration = 0.0;

int led_pose = 0;
int reading = 0;

void setup()
{
  // define pins mode
  pinMode(led_k, OUTPUT);
  pinMode(led_chl, OUTPUT);
  pinMode(led_p, OUTPUT);

  pinMode(vref_generate, OUTPUT);
  pinMode(vcc_control, OUTPUT);

  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);
  pinMode(button4, INPUT_PULLUP);

  // iniciar comunicación serial
  Serial.begin(115200);

  // test
  //  generate_vref(3.0);

  // Initialize screen
  lcd.init();
  lcd.backlight();
  if (language)
  {
    lcd.setCursor(3, 0);
    lcd.print("Bienvenido");
  }
  else
  {
    lcd.setCursor(4, 0);
    lcd.print("Welcome!");
  }
  lcd.setCursor(6, 1);
  lcd.print("SNAP");
  analogWrite(vref_generate, 0); //
  // añadir logica de welcome y bateria

  // detect if is on development or production mode
  delay(2000);

  // Announce calibration
  if (language)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrando...");
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrating...");
  }




  if (average_adc(battery_feedback) <= 552)
  {
    // development
    vcc_source = 4.51;
    vcc_controlled = 4.49;
    // lcd.setCursor(0, 1); // retirar
    // lcd.print("dev");    // retirar
  }
  else
  {
    // lcd.setCursor(0, 1); // retirar
    // lcd.print("prod");   // retirar

    // production
    vcc_source = 4.99;
    vcc_controlled = 5.02;
  }

  // check sensor middle value

    // Check battery level
  if (battery_level() < 5.0)
  {
    if (language)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Bateria baja");
      lcd.setCursor(0, 1);
      lcd.print("Cargue el dispositivo");
    }
    else
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      //lcd.print("Low battery");
      lcd.print(battery_level());
      lcd.setCursor(0, 1);
      lcd.print("Charge the device");
    }
    //while true and button 2 and 4
    while (1 && (digitalRead(button2) == HIGH && digitalRead(button4) == HIGH))
    {
    }
  }
    
  //check vref max and min values
  generate_vref(0);
  delay(700);
  vref_min= Aread2volt(average_adc(vref_feedback));


  generate_vref(5);
  delay(700);
  vref_max= Aread2volt(average_adc(vref_feedback));
  

  // Check max value
  turnOnLED(read_led_pose());
  generate_vref(0);
  delay(warmtime / 2);
  float sensor_range_max = Aread2volt(average_adc(sensor_amplified));
  turnOffLEDs();

  generate_vref(3.0);
  delay(warmtime / 2);
  float sensor_range_min = Aread2volt(average_adc(sensor_amplified));
  turnOffLEDs();
  turn_off_vref();

  sensor_middle_value = (sensor_range_max + sensor_range_min) / 2;
  
  //  Serial.print("sensor_range_max: ");
  //  Serial.println(sensor_range_max);
  //  Serial.print("sensor_range_min: ");
  //  Serial.println(sensor_range_min);
  //  Serial.print("sensor_middle_value: ");
  //  Serial.println(sensor_middle_value);
}

void loop()
{
  // show screen and let choose button 1 for measure or  button 2 for settings
  // on measure show 1 for N, 2 for P, 3 for K
  // on settings show 1 for language, 2 for battery
  // on language show 1 for english, 2 for spanish
  // screens logics

  lcd.clear();
  if (language)
  {
    lcd.setCursor(0, 0);
    lcd.print("1. Medir");
    lcd.setCursor(0, 1);
    lcd.print("2. Configurar");
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("1. Measure");
    lcd.setCursor(0, 1);
    lcd.print("2. Settings");
  }
  delay(1000);
  // wait for user input 1 or 2
  while (digitalRead(button1) == HIGH && digitalRead(button2) == HIGH)
  {
  }

  if (digitalRead(button1) == LOW)
  {
    // measure
    lcd.clear();
    if (language)
    {
      lcd.setCursor(0, 0);
      lcd.print("Elija elemento:");
      lcd.setCursor(0, 1);
      lcd.print("1.N 2.P 3.K");
    }
    else
    {
      lcd.setCursor(0, 0);
      lcd.print("Choose element:");
      lcd.setCursor(0, 1);
      lcd.print("1.N 2.P 3.K");
    }
    delay(1000);
    while (digitalRead(button1) == HIGH && digitalRead(button2) == HIGH && digitalRead(button3) == HIGH)
    {
    }
    if (digitalRead(button1) == LOW)
    {
      // N
      element = 'N';
    }
    else if (digitalRead(button2) == LOW)
    {
      // P
      element = 'P';
    }
    else if (digitalRead(button3) == LOW)
    {
      // K
      element = 'K';
    }

    // measurement logic

    measure(element); // does not return values, results are stored in global variables

    calculate_results(element);
    show_results(element);
    // show results screen
    // optional, save results
    // 4 to go back to main screen or turn off
    delay(500);
  }
  else if (digitalRead(button2) == LOW)
  {
    // settings
    lcd.clear();
    if (language)
    {
      lcd.setCursor(0, 0);
      lcd.print("Elija opcion:");
      lcd.setCursor(0, 1);
      lcd.print("1.Idioma 2.Bat.");
    }
    else
    {
      lcd.setCursor(0, 0);
      lcd.print("Choose option:");
      lcd.setCursor(0, 1);
      lcd.print("1.Lang. 2.Batt.");
    }
    delay(1000);
    while (digitalRead(button1) == HIGH && digitalRead(button2) == HIGH)
    {
    }
    if (digitalRead(button1) == LOW)
    {
      // language
      lcd.clear();
      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Elija idioma:");
        lcd.setCursor(0, 1);
        lcd.print("1.Ingles 2.Esp");
      }
      else
      {
        lcd.setCursor(0, 0);
        lcd.print("Choose language:");
        lcd.setCursor(0, 1);
        lcd.print("1.Eng. 2.Spanish");
      }
      delay(1000);
      while (digitalRead(button1) == HIGH && digitalRead(button2) == HIGH)
      {
      }
      if (digitalRead(button1) == LOW)
      {
        // english
        language = 0;
      }
      else if (digitalRead(button2) == LOW)
      {
        // spanish
        language = 1;
      }
    }
    else if (digitalRead(button2) == LOW)
    {

      // battery
      while (digitalRead(button4) == HIGH)
      {
        // wait

        lcd.clear();
        if (language)
        {
          lcd.setCursor(0, 0);
          lcd.print("Nivel de bateria:");
          lcd.setCursor(10, 1);
          lcd.print(battery_level());
          lcd.setCursor(15, 1);
          lcd.print("%");
        }
        else
        {
          lcd.setCursor(0, 0);
          lcd.print("Battery level:");
          lcd.setCursor(10, 1);
          lcd.print(battery_level());
          lcd.setCursor(15, 1);
          lcd.print("%");
        }

        // wait for user input to continue
        delay(2000);
        // show press 4 to continue
        lcd.clear();
        if (language)
        {
          lcd.setCursor(0, 0);      // retirar
          lcd.print("Presione 4 "); // is too long, max 16 chars
          lcd.setCursor(0, 1);      // retirar
          lcd.print("para ir al menu");
        }
        else
        {
          lcd.setCursor(0, 0); // retirar
          lcd.print("Push 4 to ");
          lcd.setCursor(0, 1); // retirar
          lcd.print("go to menu");
        }
        delay(1000);
      }
    }
  }
}

float battery_level()
{
  // battery ranges from 9V to 6V
  battery_level_percentage = Aread2volt(average_adc(battery_feedback)) * 2 * 33.33 - 6 * 33.33;
  return battery_level_percentage;
}

void calculate_results(char element)
{
  //calculate calculated_V_pht_blank
  //calculate calculated_V_pht_color
  //calculate absorbance
  //calculate concentration

  calculated_V_pht_blank = corriente_pht(prom_sensor_blank, prom_vref_blank)*(Rc/1000);
  calculated_V_pht_color = corriente_pht(prom_sensor_color, prom_vref_color)*(Rc/1000);
  calculated_absorbance = -log10(calculated_V_pht_color  /calculated_V_pht_blank);
  calculated_concentration = 0.0;

}

void show_results(char element)
{
  // show results
  // show concentration until button 4 is pressed
  // show all blank results until button 4
  // show all color results until button 4
  //hidden option button 3 absorbance
  // show dynamic alternatives, 1 repeat results, 2 main menu, turn off device
  int flag_out = 1;
  int flag_out_1 = 1;
  int flag_out_2 = 1;
  int flab_out_blank = 1;
  int flab_out_color = 1;
  int flab_out_absorbance = 1;
  while(flag_out){


    //indicate that button 4 continues the process
    while (flag_out_1){
      //decir "resultados" o "results
      lcd.clear();
      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Resultados");
      }
      else
      {
        lcd.setCursor(0, 0);
        lcd.print("Results");
      }
      delay(1000);
      if (digitalRead(button4) == LOW)
      {
        flag_out_1 = 0;
      }
      lcd.clear();
      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Presione 4 para");
        lcd.setCursor(0, 1);
        lcd.print("continuar");
      }
      else
      {
        lcd.setCursor(0, 0);
        lcd.print("Push 4 to");
        lcd.setCursor(0, 1);
        lcd.print("continue");
      }
      delay(1000);
      if (digitalRead(button4) == LOW)
      {
        flag_out_1 = 0;
      }
    
    }

    // show concentration until button 4 is pressed
    while (flag_out_2){
      lcd.clear();
      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Concentracion:");
        lcd.setCursor(0, 1);
        lcd.print(calculated_concentration);
      }
      else
      {
        lcd.setCursor(0, 0);
        lcd.print("Concentration:");
        lcd.setCursor(0, 1);
        lcd.print(calculated_concentration);
      }
      delay(1000);
      if (digitalRead(button4) == LOW)
      {
        flag_out_2 = 0;
      }
    }


    // show all blank results until button 4
    while (flab_out_blank){
      lcd.clear();
      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Vref blanco:");
        lcd.setCursor(0, 1);
        lcd.print(prom_vref_blank);
      }
      else
      {
        lcd.setCursor(0, 0);
        lcd.print("Vref blank:");
        lcd.setCursor(0, 1);
        lcd.print(prom_vref_blank);
      }
      delay(2000);
      if (digitalRead(button4) == LOW)
      {
        flab_out_blank = 0;
      }
    
    delay(1000);
      lcd.clear();
      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Amplificacion:");
        lcd.setCursor(0, 1);
        lcd.print("Blank:");
        lcd.setCursor(10, 1);
        lcd.print(prom_sensor_blank);
      } else
      {
        lcd.setCursor(0, 0);
        lcd.print("Amplification:");
        lcd.setCursor(0, 1);
        lcd.print("Blank:");
        lcd.setCursor(10, 1);
        lcd.print(prom_sensor_blank);
      }
      delay(2000);
      if (digitalRead(button4) == LOW)
      {
        flab_out_blank = 0;
      }

      lcd.clear();

      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Vpht blanco:");
        lcd.setCursor(0, 1);
        lcd.print(calculated_V_pht_blank);
      } else
      {
        lcd.setCursor(0, 0);
        lcd.print("Vpht blank:");
        lcd.setCursor(0, 1);
        lcd.print(calculated_V_pht_blank);
      }
      delay(2000);
      if (digitalRead(button4) == LOW)
      {
        flab_out_blank = 0;
      }
      
    }

    
    
    // show all color results until button 4
    while (flab_out_color){
      // wait
      lcd.clear();
      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Vref color:");
        lcd.setCursor(0, 1);
        lcd.print(prom_vref_color);
      } else
      {
        lcd.setCursor(0, 0);
        lcd.print("Vref color:");
        lcd.setCursor(0, 1);
        lcd.print(prom_vref_color);
      }
      delay(3000);
        if (digitalRead(button4) == LOW)
      {
        flab_out_color = 0;
      }
      

      lcd.clear();
      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Amplificacion:");
        lcd.setCursor(0, 1);
        lcd.print("Color:");
        lcd.setCursor(10, 1);
        lcd.print(prom_sensor_color);
      } else
      {
        lcd.setCursor(0, 0);
        lcd.print("Amplification:");
        lcd.setCursor(0, 1);
        lcd.print("Color:");
        lcd.setCursor(10, 1);
        lcd.print(prom_sensor_color);
      }
      delay(3000);

    if (digitalRead(button4) == LOW)
      {
        flab_out_color = 0;
      }

      lcd.clear();

      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Vpht color:");
        lcd.setCursor(0, 1);
        lcd.print(calculated_V_pht_color);
      } else
      {
        lcd.setCursor(0, 0);
        lcd.print("Vpht color:");
        lcd.setCursor(0, 1);
        lcd.print(calculated_V_pht_color);
      }
      delay(3000);

    if (digitalRead(button4) == LOW)
      {
        flab_out_color = 0;
      }

      lcd.clear();

      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Absorbancia:");
        lcd.setCursor(0, 1);
        lcd.print(calculated_absorbance);
      } else
      {
        lcd.setCursor(0, 0);
        lcd.print("Absorbance:");
        lcd.setCursor(0, 1);
        lcd.print(calculated_absorbance);
      }
      delay(3000);

    if (digitalRead(button4) == LOW)
      {
        flab_out_color = 0;
      }
    }


    lcd.clear();
    if (language)
    {
      lcd.setCursor(0, 0);
      lcd.print("1. Repetir datos");
      lcd.setCursor(0, 1);
      lcd.print("2. Menu");
    }
    else
    {
      lcd.setCursor(0, 0);
      lcd.print("1. Repeat data");
      lcd.setCursor(0, 1);
      lcd.print("2. Menu");
    }
    delay(1000);
    
    // wait for button 1 or 2
    while (digitalRead(button1) == HIGH && digitalRead(button2) == HIGH)
    {
      // wait
    }
    
    if (digitalRead(button1) == LOW){
      //repeat
      flag_out = 1;
    } else if (digitalRead(button2) == LOW){
      //menu
      flag_out = 0;

    }

}
}

void measure(char element)
{
  // check for correct led
  // loop
  if (element != 'E')
  {
    int pose = check_led_pose(element);
    measure_base(element, "Blank");
    measure_base(element, "Color");
  }

  // measure blank
  // measure colored
}

void measure_base(char element, String sample)
{

  // ask user to insert sample
  if (language)
  {
    lcd.clear();
    lcd.setCursor(0, 0); // retirar
    lcd.print("Inserte ");
    lcd.setCursor(8, 0); // retirar
    if (sample == "Blank")
    {
      lcd.print("Blanco");
    }
    else
    {
      lcd.print("Color");
    }

    lcd.setCursor(0, 1); // retirar
    lcd.print("luego presione 4");
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0); // retirar
    lcd.print("Insert ");
    lcd.setCursor(7, 0); // retirar
    if (sample == "Blank")
    {
      lcd.print("Blank");
    }
    else
    {
      lcd.print("Color");
    }
    lcd.setCursor(0, 1); // retirar
    lcd.print("then push 4");
  }
  // wait for input
  while (digitalRead(button4) == HIGH)
  {
    // wait
  }

  // say its measuring
  if (language)
  {
    lcd.clear();
    lcd.setCursor(0, 0); // retirar
    lcd.print("Midiendo...");
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0); // retirar
    lcd.print("Measuring...");
  }
  delay(200);

  // turn on peripherals
  generate_vref(2.5); // 2.5V
  turnOnLED(element);
  delay(warmtime);

  // change vref to optimal value
  smart_control_vref();
  //delay(40000);
  // take measurements
  if (sample == "Blank")
  {
    prom_vref_blank = 0.0;
    prom_sensor_blank = 0.0;
    for (int i = 0; i <( measuretime / 1000); i++)
    { // verify later if its better an average or a very short instant take
      prom_vref_blank += Aread2volt(average_adc(vref_feedback));
      prom_sensor_blank += Aread2volt(average_adc(sensor_amplified));
    }
    prom_vref_blank = prom_vref_blank / (measuretime / 1000);
    prom_sensor_blank = prom_sensor_blank / (measuretime / 1000);
  }
  else if (sample == "Color")
  {
    prom_vref_color = 0.0;
    prom_sensor_color = 0.0;
    for (int i = 0; i < (measuretime / 1000); i++)
    { // verify later if its better an average or a very short instant take
      prom_vref_color += Aread2volt(average_adc(vref_feedback));
      prom_sensor_color += Aread2volt(average_adc(sensor_amplified));
    }
    prom_vref_color = prom_vref_color / (measuretime / 1000);
    prom_sensor_color = prom_sensor_color / (measuretime / 1000);
  }

  // turn off peripherals
  turnOffLEDs();
  turn_off_vref();

  // finished measurement
  while (digitalRead(button4) == HIGH)
  {
    // wait

    if (language)
    {
      lcd.clear();
      lcd.setCursor(0, 0); // retirar
      lcd.print("Medicion ");
      lcd.setCursor(0, 1); // retirar
      if (sample == "Blank")
      {
        lcd.print("Blanco");
      }
      else
      {
        lcd.print("Color");
      }
      lcd.setCursor(6, 1); // retirar
      lcd.print("Terminada");
    }
    else
    {
      lcd.clear();
      lcd.setCursor(0, 0); // retirar
      lcd.print("Measurement ");
      lcd.setCursor(0, 1); // retirar
      if (sample == "Blank")
      {
        lcd.print("Blank");
      }
      else
      {
        lcd.print("Color");
      }
      lcd.setCursor(6, 1); // retirar
      lcd.print("Finished");
    }

    // wait for user input to continue
    delay(1500);
    // show press 4 to continue
    
    lcd.clear();
    if (language)
    {
      lcd.setCursor(0, 0);      // retirar
      lcd.print("Presione 4 "); // is too long, max 16 chars
      lcd.setCursor(0, 1);      // retirar
      lcd.print("para continuar");
    }
    else
    {
      lcd.setCursor(0, 0);      // retirar
      lcd.print("Push 4 "); // is too long, max 16 chars
      lcd.setCursor(0, 1);      // retirar
      lcd.print("to continue");
    }
    delay(1500);
  }
  
  
}

void smart_control_vref()
{
  // if sensor is higher than sensor_middle_value, decrease vref; if lower, increase vref
  // if sensor-sensor_middle_value<=0.02(err), finish function, else , if vref is outside its own range, finish function

  int repeat_flag = 1;
  while (repeat_flag)
  {
    if (Aread2volt(average_adc(sensor_amplified)) < sensor_middle_value)
    {
      // decrease vref
      generate_vref(Aread2volt(average_adc(vref_feedback)) - 0.1);
    }
    else
    {
      // increase vref
      generate_vref(Aread2volt(average_adc(vref_feedback)) + 0.05);
    }

    if (abs(Aread2volt(average_adc(sensor_amplified)) - sensor_middle_value)<= 0.1)
    {
      // finish function
      repeat_flag = 0;
    }
    else if (Aread2volt(average_adc(vref_feedback)) > vref_max || Aread2volt(average_adc(vref_feedback)) < vref_min)
    { // might be the same as sensor_amplified range, test
      // finish function
      repeat_flag = 0;
    }
    else
    {
      // repeat
      repeat_flag = 1;
    }
    // lcd.setCursor(0, 1); // retirar
    // lcd.print("Vr:");
    // lcd.setCursor(4, 1); // retirar
    // lcd.print(Aread2volt(average_adc(vref_feedback)));
    // lcd.setCursor(9, 1); // retirar
    // lcd.print("Sa:");
    // lcd.setCursor(12, 1); // retirar
    // lcd.print(Aread2volt(average_adc(sensor_amplified)));
    // delay(800);
    
  }

}

int check_led_pose(char element)
{

  while (element != read_led_pose())
  {
    if (language)
    {
      lcd.clear();
      lcd.setCursor(0, 0); // retirar
      lcd.print("LED incorrecto");
      lcd.setCursor(0, 1); // retirar
      lcd.print("Mover a: ");

      lcd.setCursor(9, 1); // retirar
      lcd.print(element);
    }
    else
    {
      lcd.clear();
      lcd.setCursor(0, 0); // retirar
      lcd.print("Wrong LED");
      lcd.setCursor(0, 1); // retirar
      lcd.print("Move to: ");

      lcd.setCursor(9, 1); // retirar
      lcd.print(element);
    }
    delay(2000);
  }
  // should i wait for user input? pro: less energy, con: less intuitive
  // add instructions? like pull out, move, pull in?
  return 0;
}

float generate_vref(float desired_V)
{
  digitalWrite(vcc_control, HIGH);
  analogWrite(vref_generate, (255.0 * desired_V) / vcc_source);
  delay(600);
  return Aread2volt(average_adc(vref_feedback));
}

void turn_off_vref()
{
  digitalWrite(vcc_control, LOW);
  analogWrite(vref_generate, 0);
  delay(600);
}

float Aread2volt(float aread)
{
  float tempav = vcc_source * aread;
  return vcc_source * aread / 1024.0;
}

char read_led_pose()
{
  lcd.clear();
  lcd.setCursor(0, 0); // retirar
  if (language)
  {
    lcd.print("Verificando LED");
  }
  else
  {
    lcd.print("Checking LED");
  }

  float vref_response = generate_vref(2.68);
  delay(400);

  turnOnLED('N');
  delay(50);
  float readingN = Aread2volt(average_adc(sensor_amplified));

  turnOnLED('P');
  delay(50);
  float readingP = Aread2volt(average_adc(sensor_amplified));

  turnOnLED('K');
  delay(50);
  float readingK = Aread2volt(average_adc(sensor_amplified));

  turnOffLEDs();
  turn_off_vref();

  if (readingN > readingP && readingN > readingK)
  {
    if (readingN > 0.5)
    { // revisar 0.5
      // is N, verified
      return 'N';
    }
    else
    {
      return 'L';
    }
  }

  if (readingK > readingP && readingK > readingN)
  {
    if (readingK > 0.5)
    { // revisar 0.5
      // es K, verified
      return 'K';
    }
    else
    {
      return 'L';
    }
  }

  if (readingP > readingK && readingP > readingN)
  {
    if (readingP > 0.5)
    { // revisar 0.5
      // es P, verified
      return 'P';
    }
    else
    {
      return 'L';
    }
  }
}

float average_adc(int Pin)
{
  float acum_lect = 0;
  int i = 1;
  for (i = 1; i < 500; i = i + 1)
  {
    acum_lect = acum_lect + float(analogRead(Pin));
  }
  acum_lect = acum_lect / i;
  return acum_lect;
}

float corriente_pht(float medicion, float valor_vref) //CORRIENTE EN mA
{ // funcion que retorna la corriente del fototransistor basado en parametros  actuales del circuito

  I = (medicion  + b_ * valor_vref) / (A * Rc / 1000); // notar que b_*valor_aref es el bias  // V/kohm = mA

  return I;
}

void turnOnLED(char led_pose)
{
  if (led_pose == 'N')
  {
    digitalWrite(led_chl, HIGH);
    digitalWrite(led_p, LOW);
    digitalWrite(led_k, LOW);
  }
  if (led_pose == 'P')
  {
    digitalWrite(led_p, HIGH);
    digitalWrite(led_chl, LOW);
    digitalWrite(led_k, LOW);
  }
  if (led_pose == 'K')
  {
    digitalWrite(led_k, HIGH);
    digitalWrite(led_p, LOW);
    digitalWrite(led_chl, LOW);
  }
  //
  //    if(led_pose=='E'){
  //      lcd.clear();
  //      lcd.setCursor(0, 0);
  //      lcd.print("No LED");
  //      delay(1000);
  //    }
}

void turnOffLEDs()
{
  digitalWrite(led_k, LOW);
  digitalWrite(led_p, LOW);
  digitalWrite(led_chl, LOW);
}
