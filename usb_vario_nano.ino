#include <MS5611.h>
#include <avr/wdt.h>

/////////////////////////////////////////
MS5611 sensor(&Wire);

#define USB_SPEED 115200 //serial transmision speed
#define FREQUENCY 10 // freq output in Hz
#define NMEA_MAX_LINE 32 //maximum length
#define CHECKSUM_SIZE 3


const char compile_date[] = "$PHIGHCANFLY,I,USBVARIO," __DATE__;
const char mess_check[] = "$PHIGHCANFLY,E,USBVARIO,checking MS5611 sensor...";
const char mess_error[] = "$PHIGHCANFLY,E,USBVARIO,Error connecting MS5611...";
const char mess_reset[] = "$PHIGHCANFLY,E,USBVARIO,Reset in 1s...";

void setup() {
  wdt_disable();

  Serial.begin(USB_SPEED);
  Serial.println("...");
  Serial.println(compile_date);
  Serial.println(mess_check);
  Serial.println(mess_check);

  wdt_enable(WDTO_1S); //enable the watchdog 1s without reset
  if (sensor.connect()>0) {  
    Serial.println(mess_error);
    Serial.println(mess_reset);
    delay(1200); //for watchdog timeout
  }
  sensor.ReadProm(); //takes about 3ms
}

uint32_t get_time = millis();
uint32_t cumulative_pressure = 0;
uint8_t pressure_order = 0;
char checksum_buffer[CHECKSUM_SIZE];
char line_buffer[NMEA_MAX_LINE];

void getNmeaChecksum(const char *nmeaSentence, char* szChecksum)
{ 
  uint8_t checksum = 0;
  for (int i = 0 ; i< strlen(nmeaSentence) ; i++)
  {
    checksum ^= (unsigned char)nmeaSentence[i];
  }
  snprintf(szChecksum,CHECKSUM_SIZE, "%X",checksum);
}

void getNmeaSentence(uint32_t pressure, uint8_t temp, char *szSentence)
{
  snprintf(szSentence,NMEA_MAX_LINE, "LK8EX1,%" PRIu32 ",0,9999,%" PRIu8 ",999,",pressure,temp);
}

void printNmeaLine(const char *line, const char *checksum)
{
      Serial.print("$");
      Serial.print(line);
      Serial.print("*");
      Serial.println(checksum);
}

void loop(void) {
  wdt_reset();
  sensor.Readout(); // with OSR4096 takes about 10ms
  uint32_t integrated_pressure;
  cumulative_pressure += sensor.GetPres();
  pressure_order++;
  if (millis() >= (get_time + (1000/FREQUENCY))) 
  {
    get_time = millis();
    integrated_pressure = cumulative_pressure / pressure_order ;
    cumulative_pressure=0; 
    pressure_order=0;
    uint8_t temperature = sensor.GetTemp()/100;

    //$LK8EX1,pressure,altitude,vario,temperature,battery,*checksum
    //$LK8EX1,pressure,99999,9999,temp,999,*checksum

    getNmeaSentence(integrated_pressure,temperature,line_buffer);
    getNmeaChecksum(line_buffer, checksum_buffer);

    printNmeaLine(line_buffer, checksum_buffer);
  }
}
