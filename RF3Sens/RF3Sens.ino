//###########################################################################################
// RF3Sens (RoboForum triangulation range sensor)
// http://roboforum.ru/forum107/topic15929.html (official language for this forum - russian)
// MIT License
// 2015-2016
// Дмитрий Лилик (aka Dmitry__ @ RoboForum.ru)
// Андрей Пожогин (aka dccharacter  @ RoboForum.ru)
// Сергей Тараненко (aka setar @ RoboForum.ru)
//###########################################################################################

#include "Config.h"
#if defined(debug_type) && defined(software_serial)
  #include <SendOnlySoftwareSerial.h>
#endif

unsigned char Str[5];
uint8_t RegPowLaser = 0;

void setup(){
  //set pin I/O direction
  #if defined(use_nCS)
    pin_nCS_Mode_OUTPUT;
    pin_nCS_HIGH;
  #endif

  pin_led_Mode_OUTPUT;
  pin_led_HIGH;

  #ifdef pin_TRIG_bit
    pin_TRIG_Mode_INPUT;
  #endif

  #if defined(pin_HwPwm_bit)
    pin_HwPwm_Mode_OUTPUT;
    pin_HwPwm_LOW;
    analogWrite(pin_HwPwm_bit, 0);
  #endif 

#ifdef power_via_mcu
  #ifdef pin_sensor_gnd_bit
    pin_sensor_gnd_OUTPUT;
    pin_sensor_gnd_LOW;
  #endif
  #ifdef pin_sensor_vcc_bit
    pin_sensor_vcc_OUTPUT;
    pin_sensor_vcc_HIGH;
  #endif
  #ifdef pin_laser_gnd_bit
    pin_laser_gnd_OUTPUT;
    pin_laser_gnd_LOW;
  #endif
  #ifdef pin_laser_vcc_bit
    pin_laser_vcc_OUTPUT;
    pin_laser_vcc_HIGH;
  #endif
#endif // power_via_mcu

//initialize SPI
    pin_nClock_Mode_OUTPUT;
    pin_nClock_HIGH;
    pin_SDIO_Mode_OUTPUT;
    pin_SDIO_LOW;

#if defined(debug_type)
    SERIAL_OUT.begin(SERIAL_SPEED);
#endif

  delay(1000);
  ADNS_reset();
}

void loop(){
//###########################################################################################
// штатный режим датчика для 3D принтера
#ifndef debug_type
	byte dataMax;
	byte dataSum;

/*
  while(1)
  {
    while(pin_TRIG_IN){}      //ждать engage_z_probe
    pin_led_LOW;
    while(!pin_TRIG_IN){}
    pin_led_HIGH;
  }
*/

	while(1)
	{
		pin_led_HIGH;
		RegPowLaser =0;								//выключить лазер
		analogWrite(pin_HwPwm_bit, RegPowLaser);	//

		while(pin_TRIG_IN){}			//ждать engage_z_probe

		RegPowLaser =255;							//включить лазер
		analogWrite(pin_HwPwm_bit, RegPowLaser);	//

		while(!pin_TRIG_IN)	//шаг1
		{	
			dataMax = ADNS_read(Maximum_Pixel);
			dataSum = ADNS_read(Pixel_Sum);
			RefrPowerLaser(dataMax);
	
			//if(dataMax > (ConstMax - 30) && RegPowLaser < 170)
			if(dataMax > (ConstMax -3) && dataSum < 6) break;
		}
		pin_led_LOW;
		delay(200);
		pin_led_HIGH;

		while(!pin_TRIG_IN) //шаг2
		{
			dataMax = ADNS_read(Maximum_Pixel);
			dataSum = ADNS_read(Pixel_Sum);
			RefrPowerLaser(dataMax);
	
			if(dataMax < (ConstMax-10) && dataSum > 6) break;
		}
		pin_led_LOW;
		delay(10);
		while(!pin_TRIG_IN){}			//ждать выключения engage_z_probe
	}
//###########################################################################################
// отладочные режимы
#else
//-------------------------------------------------------------------------------------------
#if debug_type ==1
//-------------------------------------------------------------------------------------------
  byte Frame[NUM_PIXS + 6];

  while(1){
    pixel_and_params_grab(Frame);
	Frame[NUM_PIXS + 0] = RegPowLaser;	//ADNS_read(squal)

    SERIAL_OUT.write(Frame, NUM_PIXS + 6); // send frame in raw format

    //для слежения за мощностью лазера
    for(uint8_t a = 0; a < 100; a++)
    {
      Frame[NUM_PIXS + 1] = ADNS_read(Maximum_Pixel);
      RefrPowerLaser(Frame[NUM_PIXS + 1]);
      delay(5);
    }

  }
//-------------------------------------------------------------------------------------------
#elif debug_type ==2
//-------------------------------------------------------------------------------------------
  byte Frame[NUM_PIXS + 6];

  byte data;
  while(1){
    //data = ADNS_read(squal);
    data = ADNS_read(Maximum_Pixel);
    //data = ADNS_read(Pixel_Sum);

    if(data > ConstMax){
      pin_led_LOW;
    }
    else{
      pin_led_HIGH;
      pixel_and_params_grab(Frame);
      SERIAL_OUT.write(Frame, NUM_PIXS+6); // send frame in raw format
    }
  }
//-------------------------------------------------------------------------------------------
#elif debug_type ==3
//-------------------------------------------------------------------------------------------
  //листинг для электронных таблиц: В шапке названия, дальше только данные разделенные "tab".
  byte Frame[6];
  //заголовок
  SERIAL_OUT.println  (F  ("PowLas:\tMax:\tMin:\tSum:\tShutter:"));
  while(1){
    params_grab(Frame);

    ByteToString(RegPowLaser); SERIAL_OUT.write(Str[2]); SERIAL_OUT.write(Str[1]); SERIAL_OUT.write(Str[0]); SERIAL_OUT.write(0x09);
    ByteToString(Frame[1]); SERIAL_OUT.write(Str[2]); SERIAL_OUT.write(Str[1]); SERIAL_OUT.write(Str[0]); SERIAL_OUT.write(0x09);
    ByteToString(Frame[2]); SERIAL_OUT.write(Str[2]); SERIAL_OUT.write(Str[1]); SERIAL_OUT.write(Str[0]); SERIAL_OUT.write(0x09);
    ByteToString(Frame[3]); SERIAL_OUT.write(Str[2]); SERIAL_OUT.write(Str[1]); SERIAL_OUT.write(Str[0]); SERIAL_OUT.write(0x09);
    Uint16ToString(Frame[4] *256 + Frame[5]);
    SERIAL_OUT.write(Str[4]);
    SERIAL_OUT.write(Str[3]);
    SERIAL_OUT.write(Str[2]);
    SERIAL_OUT.write(Str[1]);
    SERIAL_OUT.write(Str[0]);
    SERIAL_OUT.write(0x0a);
    //SERIAL_OUT.write(0x0d);

	RefrPowerLaser(Frame[1]);

#if SERIAL_SPEED > 115200
    delay(20);
#else
    delay(40);  //задержка больше из-за низкой скорости serial
#endif
  }
//-------------------------------------------------------------------------------------------
#elif debug_type ==4
//-------------------------------------------------------------------------------------------
  //Как 3-й режим, но по разрешению сигнала pin_TRIG (лог точно ограничен сигналом z_probe)
  byte Frame[6];
  while(1){
    if(pin_TRIG_IN){
      //заголовок
      SERIAL_OUT.println (F  ("Squal:\tMax:\tMin:\tSum:\tShutter:"));

      params_grab(Frame);

      ByteToString(Frame[0]); SERIAL_OUT.write(Str[2]); SERIAL_OUT.write(Str[1]); SERIAL_OUT.write(Str[0]); SERIAL_OUT.write(0x09);
      ByteToString(Frame[1]); SERIAL_OUT.write(Str[2]); SERIAL_OUT.write(Str[1]); SERIAL_OUT.write(Str[0]); SERIAL_OUT.write(0x09);
      ByteToString(Frame[2]); SERIAL_OUT.write(Str[2]); SERIAL_OUT.write(Str[1]); SERIAL_OUT.write(Str[0]); SERIAL_OUT.write(0x09);
      ByteToString(Frame[3]); SERIAL_OUT.write(Str[2]); SERIAL_OUT.write(Str[1]); SERIAL_OUT.write(Str[0]); SERIAL_OUT.write(0x09);
      Uint16ToString(Frame[4] *256 + Frame[5]);
      SERIAL_OUT.write(Str[4]);
      SERIAL_OUT.write(Str[3]);
      SERIAL_OUT.write(Str[2]);
      SERIAL_OUT.write(Str[1]);
      SERIAL_OUT.write(Str[0]);
      SERIAL_OUT.write(0x0a);
      //SERIAL_OUT.write(0x0d);

#if SERIAL_SPEED > 115200
    delay(20);
#else
    delay(60);  //задержка больше из-за низкой скорости serial
#endif
    }
  }
//-------------------------------------------------------------------------------------------
#elif debug_type ==5
//-------------------------------------------------------------------------------------------
  while(1){
    unsigned int motion = 0;
    //check for movement
    motion = ADNS_read(Motion);
    if (motion > 127){
      //there has been a motion! Print the x and y movements
      SERIAL_OUT.println("X:" + String(ADNS_read(Delta_X)) + " Y:" + String(ADNS_read(Delta_Y)));
    }
  }
//-------------------------------------------------------------------------------------------
#endif
#endif
}

//###########################################################################################
// процедуры
//-------------------------------------------------------------------------------------------
void ADNS_reset(void){
#ifdef sens_type_ADNS_5020
  ADNS_write(0x3a,0x5a);
  delay(1000);
  ADNS_write(0x0d, 0x01);  // Set 1000cpi resolution
  delay(1000);
#endif
#if  defined(sens_type_ADNS_2610) || defined(sens_type_ADNS_2620)
  //ADNS_write(0x00,0x80);
  delay(1000);
  ADNS_write(0x00,0x01); //Always awake
#endif
}

//-------------------------------------------------------------------------------------------
void ADNS_write(byte address, byte data){
  #if defined(use_nCS)
    pin_nCS_LOW;
  #endif
  // send in the address and value via SPI:
  address |= 0x80;  //признак записи адреса
  for (byte i = 0x80; i; i >>= 1){
    pin_nClock_LOW;
    address & i ? pin_SDIO_HIGH : pin_SDIO_LOW;
    asm volatile ("nop");
    pin_nClock_HIGH;
  }

  //delayMicroseconds(1);

  for (byte i = 0x80; i; i >>= 1){
    pin_nClock_LOW;
    data & i ? pin_SDIO_HIGH : pin_SDIO_LOW;
    asm volatile ("nop");
    pin_nClock_HIGH;
  }
  //t SWW. SPI Time between Write Commands
  delayMicroseconds(delay_tSWW);

  #if defined(use_nCS)
    pin_nCS_HIGH;
  #endif
}

//-------------------------------------------------------------------------------------------
byte ADNS_read(byte address){
  #if defined(use_nCS)
    pin_nCS_LOW;
  #endif

  address &= ~0x80;  //признак записи данных
  for (byte i = 0x80; i; i >>= 1){
    pin_nClock_LOW;
    address & i ? pin_SDIO_HIGH : pin_SDIO_LOW;
    asm volatile ("nop");
    pin_nClock_HIGH;
  }

  // prepare io pin for reading
  pin_SDIO_Mode_INPUT;

  // t SRAD. SPI Read Address-Data Delay
  delayMicroseconds(delay_tSRAD);

  // read the data
  byte data = 0;
  for (byte i = 8; i; i--){
    // tick, tock, read
    pin_nClock_LOW;
    asm volatile ("nop");
    pin_nClock_HIGH;
    data <<= 1;
    if (pin_SDIO_IN) data |= 0x01;
  }

  #if defined(use_nCS)
    pin_nCS_HIGH;
  #endif
  pin_SDIO_Mode_OUTPUT;

  // t SRW & t SRR = 1μs.
  delayMicroseconds(2);
  return data;
}
//-------------------------------------------------------------------------------------------
inline void pixel_grab(uint8_t *buffer, uint16_t nBytes) {
  uint8_t temp_byte;

  //reset the pixel grab counter
  ADNS_write(Pixel_Grab, 0x00);

  for (int count = 0; count < nBytes; count++) {
    while (1) {
      temp_byte = ADNS_read(Pixel_Grab);
      if (temp_byte & Pixel_data_valid) {
        break;
      }
    }
    *(buffer + count) = temp_byte & Mask_pixel_value;  // only n bits are used for data
  }
}
//-------------------------------------------------------------------------------------------
inline void params_grab(uint8_t *buffer) {
	*(buffer + 0) = ADNS_read(squal);
	*(buffer + 1) = ADNS_read(Maximum_Pixel);
	*(buffer + 2) = ADNS_read(Minimum_Pixel);
	*(buffer + 3) = ADNS_read(Pixel_Sum);
	*(buffer + 4) = ADNS_read(Shutter_Upper);
	*(buffer + 5) = ADNS_read(Shutter_Lower);
}
//-------------------------------------------------------------------------------------------
inline void pixel_and_params_grab(uint8_t *buffer) {
	params_grab((buffer + NUM_PIXS));
	pixel_grab(buffer, NUM_PIXS);
}
//-------------------------------------------------------------------------------------------
void ByteToString(uint8_t a){
  Str[0] = '0';
  Str[1] = '0';
  Str[2] = '0';
  if (!a) return;
  Str[0] = a % 10 + '0'; a /= 10;
  if (!a) return;
  Str[1] = a % 10 + '0'; a /= 10;
  if (!a) return;
  Str[2] = a % 10 + '0';
}
//-------------------------------------------------------------------------------------------
void Uint16ToString(uint16_t a){
  Str[0] = '0';
  Str[1] = '0';
  Str[2] = '0';
  Str[3] = '0';
  Str[4] = '0';
  if (!a) return;
  Str[0] = a % 10 + '0'; a /= 10;
  if (!a) return;
  Str[1] = a % 10 + '0'; a /= 10;
  if (!a) return;
  Str[2] = a % 10 + '0'; a /= 10;
  if (!a) return;
  Str[3] = a % 10 + '0'; a /= 10;
  if (!a) return;
  Str[4] = a % 10 + '0';
}
//-------------------------------------------------------------------------------------------
uint8_t ByteToAscii_h(uint8_t x){
  uint8_t tmpByte;
  tmpByte = (x >> 4) + 0x30;
  if (tmpByte > 0x39) { tmpByte += 0x07; }
  return tmpByte;
}
//-------------------------------------------------------------------------------------------
uint8_t ByteToAscii_l(uint8_t x){
  uint8_t tmpByte;
  tmpByte = (x & 0x0f) + 0x30;
  if (tmpByte > 0x39) { tmpByte += 0x07; }
  return tmpByte;
}
//-------------------------------------------------------------------------------------------
void RefrPowerLaser(uint8_t power)
{
  if(power < ConstMax && RegPowLaser < 255)
  {
    RegPowLaser++;
    analogWrite(pin_HwPwm_bit, RegPowLaser);
  }
  else if(power > ConstMax && RegPowLaser > 0)
  {
    RegPowLaser--;
    analogWrite(pin_HwPwm_bit, RegPowLaser);
  }
}
//-------------------------------------------------------------------------------------------
void RefrPowerLaserMin(uint8_t power)
{
  if(power > ConstMax && RegPowLaser > 0)
  {
    RegPowLaser--;
    analogWrite(pin_HwPwm_bit, RegPowLaser);
  }
}
//-------------------------------------------------------------------------------------------
/*
g91
g1 y-5 f600
g30
g1 y+5 f600
g30
*/
