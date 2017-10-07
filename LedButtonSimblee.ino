/*
  This Simblee sketch demonstrates a full bi-directional Bluetooth Low
  Energy 4 connection between an iPhone application and an Simblee.

  This sketch works with the simbleeLedButton iPhone application.

  The button on the iPhone can be used to turn the green led on or off.
  The button state of button 1 is transmitted to the iPhone and shown in
  the application.
*/

/*
   Copyright (c) 2015 RF Digital Corp. All Rights Reserved.

   The source code contained in this file and all intellectual property embodied in
   or covering the source code is the property of RF Digital Corp. or its licensors.
   Your right to use this source code and intellectual property is non-transferable,
   non-sub licensable, revocable, and subject to terms and conditions of the
   SIMBLEE SOFTWARE LICENSE AGREEMENT.
   http://www.simblee.com/licenses/SimbleeSoftwareLicenseAgreement.txt

   THE SOURCE CODE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND.

   This heading must NOT be removed from this file.
*/

#include <SimbleeBLE.h>

// pin 3 on the RGB shield is the red led
// (can be turned on/off from the iPhone app)
int led = 3;

// pin 5 on the RGB shield is button 1
// (button press will be shown on the iPhone app)
int button = 5;

// debounce time (in ms)
int debounce_time = 10;

// maximum debounce timeout (in ms)
int debounce_timeout = 100;

byte canTurnOnAlarm = (byte) 0;
bool buttonStatus = false;

void setup()
{
  // led turned on/off from the iPhone app
  pinMode(led, OUTPUT);

  Serial.begin(9600);
  
  // button press will be shown on the iPhone app)
  pinMode(button, INPUT);

  // this is the data we want to appear in the advertisement
  // (if the deviceName and advertisementData are too long to fix into the 31 byte
  // ble advertisement packet, then the advertisementData is truncated first down to
  // a single byte, then it will truncate the deviceName)
  SimbleeBLE.advertisementData = "ledbtn";
  SimbleeBLE.deviceName = "RoJo";

  // start the BLE stack
  SimbleeBLE.begin();
}

/*
   Cuando uno presiona un boton justo en el momento de ser presionado,
   es decir el intervalo de tiempo en el que los pines del boton empiezan
   a hacer contacto con el circuito, puede ocurrir que el circuito este en un
   estado indeterminado: abierto o cerrado, luego de un tiempo se puede considerar
   mas confiable la información que nos dice si el boton esta cerrando el circuito o no
*/
int debounce(int state)
{
  /*
     millis() restart after 50 days aproximatly
     (2**(4*8))/86400000
     So the number of milliseconds is sotored in an int variable.
  */
  int start = millis();
  int debounce_start = start;

  /*
     millis()-start es el tiempo en milisegundos que ha transcurrido
     desde el llamado de la funcion debounce() hasta el momento actual.

     Mientras dicho intervalo de tiempo sea menor al debounce_timeout ejecutar
     el codigo que esta dentro del while
  */

  /*
     Hay 100 milisegundos para verificar si se presiono el boton
  */
  while (millis() - start < debounce_timeout)
    // Si el boton esta en el estado que queremos
    if (digitalRead(button) == state)
    {
      /* Si el tiempo entre el ultimo ciclo del while en el que el boton
          no estaba en el estado que queremos y el momento actual
          es mayor a 10 milisegundos, entonces la funcion retorna 1 (true)

      */
      if (millis() - debounce_start >= debounce_time)
        return 1;
    }
    else // Si el boton no esta en el estado que quermos
      debounce_start = millis();

  // Si pasan 100 milisegundos y la funcion no retorno 1, entonces retornar 0
  return 0;
}
// La funcion no retorna un integer en ningun lado, porque funciona esto?
int delay_until_button(int state)
{
  // set button edge to wake up on
  if (state)
    Simblee_pinWake(button, HIGH); // Congifures pin button to wake up the device on a high signal
  else
    Simblee_pinWake(button, LOW); // Configures pin button to wake up the device on a LOW signal

  do 
  {
    // switch to lower power mode until a button edge wakes us up
    // en debate:
    // La alarma se mantiene ejecutando mientras el pin sea false, (state=true)
    // es decir cuando esta funcion esta esperando a que se precione el boton
    //Serial.println(buttonStatus);
    if (!buttonStatus && canTurnOnAlarm)
    {
        blinkLed(500);
    }
    else
    {
      Simblee_ULPDelay(1000);
    }
//    Simblee_ULPDelay(INFINITE);
  }
  while (! debounce(state)); // Mantener dormido mientras debounce sea igual a false
  // Si el simblee se duerme porque las condiciones del do..while siguen ejecutandose?
  // que es exactamente lo que se duerme cuando el simblee se pone en modo ultra low power delay

  // debounce(state) va a dar false mientras digitalRead() != state
  // Cuando el programa inicia, queda bloqueado aca, hasta que halla una señal true en el buttonA
  /* Cuando ocurre la señal true, la condicion del do..while se hace false
     y se procede con la siguiente linea
  */

  // SIMBLEE framework stuffs
  // if multiple buttons were configured, this is how you would determine what woke you up
  if (Simblee_pinWoke(button))
  {
    // execute code here
    Simblee_resetPinWake(button);
  }
}

void blinkLed(int ms)
{
  digitalWrite(led, HIGH);
  Simblee_ULPDelay(ms);
  digitalWrite(led, LOW);
  Simblee_ULPDelay(ms);
}

void loop() {
  // Que pasa si el boton esta en LOW
  // El Simblee se mantendra en estado dormido
  delay_until_button(HIGH);
  buttonStatus = true;
  SimbleeBLE.send(1); // Por tanto este codigo no se ejecutara
  // Cuando el digitalRead(ButtonA) == true por primera vez, se enviara el valor anterior.


  /*
     Ahora el Simblee quedara dormido mientras se mantiene precionado el botonA
     Ya que digitalRead(ButtonA) es diferente de true
  */
  delay_until_button(LOW); // Cuando se deje de presionar el boton, se procedera a ejecutar la siguiente linea
  //buttonStatus = false;
  SimbleeBLE.send(0); // y por tanto se envia un 0 por BLE
  // And we come back to the first line of the loop() function
  // And consencuently Simblee keeps in ultra low power mode until buttonA will be pressed again.
}

void SimbleeBLE_onDisconnect()
{
  // don't leave the led on if they disconnect
  digitalWrite(led, LOW);
}

void SimbleeBLE_onReceive(char *data, int len)
{
  // if the first byte is 0x01 or great than zero / on / true
  if (data[0])
  {
    canTurnOnAlarm = (byte) 1;
    buttonStatus = false;
    //digitalWrite(led, HIGH);
    //blinkLed(500);
  }
  else 
  {
    digitalWrite(led, LOW);
    canTurnOnAlarm = (byte) 0;
  }
}
