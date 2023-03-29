                                                                                                                                                                                 // #include <avr/sleep.h> //librerias para modos de bajo consumo
// #define interruptPin 2
//#include <Wire.h>              //libreria para i2c
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

const float R1 = 8060.0;
const float R2 = 3300.0;
const float R3 = 1500.0;
const float R4 = 10000.0;
const float Rc = 500.0; // ohm

const float A = (R4 / (R3 + R4)) * ((R1 + R2) / (R2));
const float b_ = R1 / R2;

// otras variables
float vcc_source = 0.0;
float vcc_controlled = 0.0;
const int warmtime = 1000;    // ms
const int measuretime = 9000; // ms
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
    lcd.print("Calibrando, no");
    lcd.setCursor(0, 1);
    lcd.print("coloque muestra");
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrating, do");
    lcd.setCursor(0, 1);
    lcd.print("not place sample");
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

    // Announce calibration
  if (language)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrando, no");
    lcd.setCursor(0, 1);
    lcd.print("coloque muestra");
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrating, do");
    lcd.setCursor(0, 1);
    lcd.print("not place sample");
  }
    
  //check vref max and min values
  generate_vref(0.0);
  delay(700);
  vref_min= Aread2volt(average_adc(vref_feedback));


  generate_vref(5.0);
  delay(1000);
  vref_max= Aread2volt(average_adc(vref_feedback));
  

  // Check max value
  turnOnLED(read_led_pose());
  generate_vref(0.0);
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
      //say element
      if (language)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Elemento: N");
      }
      else
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Element: N");
      }
    }
    else if (digitalRead(button2) == LOW)
    {
      // P
      element = 'P';
      //say element
      if (language)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Elemento: P");
      }
      else
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Element: P");
      }
      
    }
    else if (digitalRead(button3) == LOW)
    {
      // K
      element = 'K';
      //say element
      if (language)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Elemento: K");
      }
      else
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Element: K");
      }
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
        bool flag99 = 0;
        for (int i = 0; i < 15; i++)
        {
          if (digitalRead(button4) == LOW)
          {
            flag99 = 1;
            break;
            
          }
          delay(100);
        }
        
        if (flag99)
        {
          break;
        }
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
        bool flag97 = 0;
        for (int i = 0; i < 15; i++)
        {
          if (digitalRead(button4) == LOW)
          {
            flag97 = 1;
            break;
            
          }
          delay(100);
        }
        
        if (flag97)
        {
          break;
        }
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
  if (element=='N') {

  calculated_absorbance = (-log10(calculated_V_pht_color  /calculated_V_pht_blank))/0.657;
  calculated_concentration = 0.0;
  }

  else if (element=='K') {

  calculated_absorbance = (-log10(calculated_V_pht_color  /calculated_V_pht_blank))/0.495;
  calculated_concentration = 0.0;

  }

  else if (element=='P') {

  calculated_absorbance = (-log10(calculated_V_pht_color  /calculated_V_pht_blank))/0.97;
  calculated_concentration = 0.0;

  }

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
  while(flag_out){
    //decir "resultados" o "results y boton 4 para continuar

    //indicate that button 4 continues the process
    delay(350);
    
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
          // wait 1.5s while checking button 4 press TO BREAK
      bool flag99 = 0;
      for (int i = 0; i < 15; i++)
      {
        if (digitalRead(button4) == LOW)
        {
          flag99 = 1;
          break;
          
        }
        delay(100);
      }
    
      if (flag99)
      { 
        flag_out_1 = 0;
        break;
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
      bool flag98 = 0;
      for (int i = 0; i < 15; i++)
      {
        if (digitalRead(button4) == LOW)
        {
          flag98 = 1;
          break;
          
        }
        delay(100);
      }
    
      if (flag98)
      { 
        flag_out_1 = 0;
        break;
      }
    }

    // show concentration until button 4 is pressed
    delay(350);
    lcd.clear();
    while (true){
      
      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Concentracion:");
        lcd.setCursor(0, 1);
        lcd.print(calculated_concentration);
        //units ppm
        lcd.setCursor(13, 1);
        lcd.print("ppm");
      }
      else
      {
        lcd.setCursor(0, 0);
        lcd.print("Concentration:");
        lcd.setCursor(0, 1);
        lcd.print(calculated_concentration);
        lcd.setCursor(13, 1);
        lcd.print("ppm");
      }
      bool flag99 = 0;
      for (int i = 0; i < 15; i++)
      {
        if (digitalRead(button4) == LOW)
        {
          flag99 = 1;
          break;
        }
        delay(100);
      }
    
      if (flag99)
      { 
        
        break;
      }
    }


    delay(350);
    lcd.clear();
    while (true){
      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Vref blanco:");
        lcd.setCursor(0, 1);
        lcd.print(prom_vref_blank);
        //units V
        lcd.setCursor(15, 1);
        lcd.print("V");
      }
      else
      {
        lcd.setCursor(0, 0);
        lcd.print("Vref blank:");
        lcd.setCursor(0, 1);
        lcd.print(prom_vref_blank);
        lcd.setCursor(15, 1);
        lcd.print("V");
      }
      bool flag99 = 0;
      for (int i = 0; i < 15; i++)
      {
        if (digitalRead(button4) == LOW)
        {
          flag99 = 1;
          break;
        }
        delay(100);
      }
    
      if (flag99)
      { 
        
        break;
      }
    }



    delay(350);
    lcd.clear();
    while(true){
      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Amplificacion:");
        lcd.setCursor(0, 1);
        lcd.print("Blank:");
        lcd.setCursor(8, 1);
        lcd.print(prom_sensor_blank);
        //units V
        lcd.setCursor(15, 1);
        lcd.print("V");
      } else
      {
        lcd.setCursor(0, 0);
        lcd.print("Amplification:");
        lcd.setCursor(0, 1);
        lcd.print("Blank:");
        lcd.setCursor(8, 1);
        lcd.print(prom_sensor_blank);
        lcd.setCursor(15, 1);
        lcd.print("V");
      }
      bool flag99 = 0;
      for (int i = 0; i < 15; i++)
      {
        if (digitalRead(button4) == LOW)
        {
          flag99 = 1;
          break;
        }
        delay(100);
      }
    
      if (flag99)
      { 
       
        break;
      }
    }


    delay(350);
    lcd.clear();
    while(true){

      if (language)
      {

        lcd.setCursor(0, 0);
        lcd.print("Calc. Sensor");
        lcd.setCursor(0, 1);
        lcd.print("Blanco:");
        lcd.setCursor(9, 1);
        lcd.print(calculated_V_pht_blank);
        //units V
        lcd.setCursor(15, 1);
        lcd.print("V");
      } else
      {
        lcd.setCursor(0, 0);
        lcd.print("Calc. Sensor");
        lcd.setCursor(0, 1);
        lcd.print("Blank:");
        lcd.setCursor(9, 1);
        lcd.print(calculated_V_pht_blank);
        lcd.setCursor(15, 1);
        lcd.print("V");
      }
      bool flag99 = 0;
      for (int i = 0; i < 15; i++)
      {
        if (digitalRead(button4) == LOW)
        {
          flag99 = 1;
          break;
        }
        delay(100);
      }
    
      if (flag99)
      { 
       
        break;
      }
      
    }

    
    
    delay(350);
    lcd.clear();

    while (true){
      
      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Vref color:");
        lcd.setCursor(0, 1);
        lcd.print(prom_vref_color);
        //units V
        lcd.setCursor(15, 1);
        lcd.print("V");
      }
      else
      {
        lcd.setCursor(0, 0);
        lcd.print("Vref color:");
        lcd.setCursor(0, 1);
        lcd.print(prom_vref_color);
        lcd.setCursor(15, 1);
        lcd.print("V");
      } 
      bool flag99 = 0;
      for (int i = 0; i < 15; i++)
      {
        if (digitalRead(button4) == LOW)
        {
          flag99 = 1;
          break;
        }
        delay(100);
      }
    
      if (flag99)
      { 
       
        break;
      }
    }


    delay(350);
    lcd.clear();
    while(true){

      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Amplificacion:");
        lcd.setCursor(0, 1);
        lcd.print("Color:");
        lcd.setCursor(8, 1);
        lcd.print(prom_sensor_color);
        //units V
        lcd.setCursor(15, 1);
        lcd.print("V");
      } else
      {
        lcd.setCursor(0, 0);
        lcd.print("Amplification:");
        lcd.setCursor(0, 1);
        lcd.print("Color:");
        lcd.setCursor(8, 1);
        lcd.print(prom_sensor_color);
        lcd.setCursor(15, 1);
        lcd.print("V");
      }
      

      bool flag99 = 0;
      for (int i = 0; i < 15; i++)
      {
        if (digitalRead(button4) == LOW)
        {
          flag99 = 1;
          break;
        }
        delay(100);
      }
    
      if (flag99)
      { 
       
        break;
      }

    }

    delay(350);
    lcd.clear();
    while(true){

      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Calc. Sensor");
        lcd.setCursor(0, 1);
        lcd.print("Color:");
        lcd.setCursor(9, 1);
        lcd.print(calculated_V_pht_color);
        //units V
        lcd.setCursor(15, 1);
        lcd.print("V");
      } else
      {
        lcd.setCursor(0, 0);
        lcd.print("Calc. Sensor");
        lcd.setCursor(0, 1);
        lcd.print("Color:");
        lcd.setCursor(9, 1);
        lcd.print(calculated_V_pht_color);
        lcd.setCursor(15, 1);
        lcd.print("V");
      }

      bool flag99 = 0;
      for (int i = 0; i < 15; i++)
      {
        if (digitalRead(button4) == LOW)
        {
          flag99 = 1;
          break;
        }
        delay(100);
      }
    
      if (flag99)
      { 
       
        break;
      }

    }


    delay(350);
    lcd.clear();
    while(true){

      if (language)
      {
        lcd.setCursor(0, 0);
        lcd.print("Absorbancia:");
        lcd.setCursor(0, 1);
        lcd.print(calculated_absorbance);
        //units abs.
        lcd.setCursor(11, 1);
        lcd.print("abs.");
      } else
      {
        lcd.setCursor(0, 0);
        lcd.print("Absorbance:");
        lcd.setCursor(0, 1);
        lcd.print(calculated_absorbance);
        lcd.setCursor(11, 1);
        lcd.print("abs.");
      }
      bool flag99 = 0;
      for (int i = 0; i < 15; i++)
      {
        if (digitalRead(button4) == LOW)
        { 
          flag99 = 1;
          break;
          
        }
        delay(100);
      }
    
      if (flag99)
      { 
       
        break;
      }
    }

    delay(200);
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
    
    
    // wait for button 1 or 2
    while (true)
    {
      // wait
      if (digitalRead(button1) == LOW){
      //repeat
      
      flag_out = 1;
      flag_out_1 = 1;
      break;
    } else if (digitalRead(button2) == LOW){
      //menu
      
      flag_out = 0;
      break;
    }
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
  delay(500);
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
  smart_control_vref(); //TODO
  delay(1000);
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
    // wait 1.5s while checking button 4 press TO BREAK
    bool flag99 = 0;
    for (int i = 0; i < 15; i++)
    {
      if (digitalRead(button4) == LOW)
      {
        flag99 = 1;
        break;
        
      }
      delay(100);
    }
    
    if (flag99)
    {
      break;
    }
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
    bool flag98 = 0;
    for (int i = 0; i < 15; i++)
    {
      if (digitalRead(button4) == LOW)
      {
        flag98 = 1;
        break;
        
      }
      delay(100);
    }
    
    if (flag98)
    {
      break;
    }
  }
  
  
}

void smart_control_vref()
{
  // if sensor is higher than sensor_middle_value, decrease vref; if lower, increase vref
  // if sensor-sensor_middle_value<=0.02(err), finish function, else , if vref is outside its own range, finish function

  int repeat_flag = 1;
  while (repeat_flag)
  {
    float vref_generated = 0.0;
    if (Aread2volt(average_adc(sensor_amplified)) < sensor_middle_value)
    {
      // decrease vref
      //check if vref generated parameter is positive 
       vref_generated = Aread2volt(average_adc(vref_feedback)) - (0.15*abs(Aread2volt(average_adc(sensor_amplified)) - sensor_middle_value));
      if (vref_generated >= vref_min/1.1 && vref_generated <= vref_max*1.1)
      {
        generate_vref(vref_generated);
      }
      else{
        generate_vref(vref_min);
        repeat_flag = 0;
      }

      // generate_vref(  Aread2volt(average_adc(vref_feedback)) - (0.15*abs(Aread2volt(average_adc(sensor_amplified)) - sensor_middle_value)) );
    }
    else
    {
      // increase vref
       vref_generated = Aread2volt(average_adc(vref_feedback)) + (0.15*abs(Aread2volt(average_adc(sensor_amplified)) - sensor_middle_value));
      if (vref_generated >= vref_min/1.1 && vref_generated <= vref_max*1.1)
      {
        generate_vref(vref_generated);
      }else{
        generate_vref(vref_max);
        repeat_flag = 0;
      }
      // generate_vref(Aread2volt(average_adc(vref_feedback)) + (0.15*abs(Aread2volt(average_adc(sensor_amplified)) - sensor_middle_value)) );
    }
    if (repeat_flag == 1){

    if (abs(Aread2volt(average_adc(sensor_amplified)) - sensor_middle_value)<= 0.1)
    {
      // finish function
      repeat_flag = 0;
      
    }
    else if (Aread2volt(average_adc(vref_feedback)) >= vref_max || Aread2volt(average_adc(vref_feedback)) <= vref_min)
    { // might be the same as sensor_amplified range, test
      // finish function
      repeat_flag = 0;
      
    }
    else
    {
      // repeat
      repeat_flag = 1;
    }
    }
    //check button 2 && 4 to show this info 
    if (digitalRead(button1) == LOW && digitalRead(button2) == LOW){
    lcd.clear();
    lcd.setCursor(0, 0); // retirar
    lcd.print(vref_min);
    lcd.setCursor(5, 0); // retirar
    lcd.print(vref_max);
    lcd.setCursor(10, 0); // retirar
    lcd.print(vref_generated);
    lcd.setCursor(15, 0); // retirar
    lcd.print(repeat_flag);    
    lcd.setCursor(0, 1); // retirar
    lcd.print("Vr:");
    lcd.setCursor(4, 1); // retirar
    lcd.print(Aread2volt(average_adc(vref_feedback)));
    lcd.setCursor(9, 1); // retirar
    lcd.print("Sa:");
    lcd.setCursor(12, 1); // retirar
    lcd.print(Aread2volt(average_adc(sensor_amplified)));
    delay(500);
    
  }

  }
}

int check_led_pose(char element)
{

  while (element != read_led_pose())
  {
    if (element ==read_led_pose() ) {
      break;
    }

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
    if (element ==read_led_pose() ) {
      break;
    }
    //show "pull lateral pin", "move lever to element", "push lateral pin"
    if (language)
    {
      lcd.clear();
      lcd.setCursor(0, 0); // retirar
      lcd.print("Jale pin lateral");
      
    }
    else
    {
      lcd.clear();
      lcd.setCursor(0, 0); // retirar
      lcd.print("Pull lateral pin");
      
    }
    delay(2000);
    if (element ==read_led_pose() ) {
      break;
    }

    if (language)
    {
      lcd.clear();
      lcd.setCursor(0, 0); // retirar
      lcd.print("Mueva palanca a:");
      lcd.setCursor(0, 1); // retirar
      lcd.print(element);
    }
    else
    {
      lcd.clear();
      lcd.setCursor(0, 0); // retirar
      lcd.print("Move lever to:");
      lcd.setCursor(0, 1); // retirar
      lcd.print(element);
    }
    delay(2000);
    if (element ==read_led_pose() ) {
      break;
    }

    if (language)
    {
      lcd.clear();
      lcd.setCursor(0, 0); // retirar
      lcd.print("Empuje pin");
      lcd.setCursor(0, 1); // retirar
      lcd.print("lateral");
      
    }
    else
    {
      lcd.clear();
      lcd.setCursor(0, 0); // retirar
      lcd.print("Push lateral pin");
      
    }
    delay(2000);
    if (element ==read_led_pose() ) {
      break;
    }

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
