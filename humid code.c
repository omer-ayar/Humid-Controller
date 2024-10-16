#include <nem.h>

#define LCD_RS_PIN PIN_D0
#define LCD_RW_PIN PIN_D1
#define LCD_ENABLE_PIN PIN_D2
#define LCD_DATA4 PIN_D3
#define LCD_DATA5 PIN_D4
#define LCD_DATA6 PIN_D5
#define LCD_DATA7 PIN_D6

#fuses NOMCLR INTRC_IO
#use delay(clock = 8000000)
#include <lcd.c>
#use fast_io(B)
#BIT Data_Pin = 0xF81.0
#BIT Data_Pin_Direction = 0xF93.0

#define DEBOUNCE_DELAY 200  

unsigned int16 reference_humidity = 90;
int increase_button = pin_b3;
int decrease_button = pin_b4;

unsigned int16 humidity, temperature;
int fan_one = pin_a0;
int fan_two = pin_a1;

short Time_out;
unsigned int8 T_byte1, T_byte2, RH_byte1, RH_byte2, CheckSum;

int debounce(int pin) {
    if (input(pin)) {
        delay_ms(DEBOUNCE_DELAY);
        if (input(pin)) {
            return 1;
        }
    }
    return 0;
}

void start_signal() {
    Data_Pin_Direction = 0;
    Data_Pin = 0;
    delay_ms(25);
    Data_Pin = 1;
    delay_us(30);
    Data_Pin_Direction = 1;
}

short check_response() {
    delay_us(40);
    if (!Data_Pin) {
        delay_us(80);
        if (Data_Pin) {
            delay_us(50);
            return 1;
        }
    }
    return 0;
}

unsigned int8 Read_Data() {
    unsigned int8 i, k, _data = 0;
    if (Time_out) break;
    for (i = 0; i < 8; i++) {
        k = 0;
        while (!Data_Pin) {
            k++;
            if (k > 100) { Time_out = 1; break; }
            delay_us(1);
        }
        delay_us(30);
        if (!Data_Pin) bit_clear(_data, (7 - i));
        else {
            bit_set(_data, (7 - i));
            while (Data_Pin) {
                k++;
                if (k > 100) { Time_out = 1; break; }
                delay_us(1);
            }
        }
    }
    return _data;
}

void main() {
    setup_oscillator(OSC_8MHZ);
    setup_adc_ports(NO_ANALOGS);
    lcd_init();
    lcd_putc('\f');
    
    while (TRUE) {
        delay_ms(1000);
        Time_out = 0;
        Start_signal();
        
        if (check_response()) {
            RH_byte1 = Read_Data();
            RH_byte2 = Read_Data();
            T_byte1 = Read_Data();
            T_byte2 = Read_Data();
            Checksum = Read_Data();

            if (CheckSum == ((RH_Byte1 + RH_Byte2 + T_Byte1 + T_Byte2) & 0xFF)) {
                temperature = T_Byte1;
                humidity = RH_Byte1;
                
                lcd_putc('\f');
                lcd_gotoxy(1, 1);
                lcd_putc("HUMIDITY: ");
                printf(lcd_putc, "%3Lu", humidity);
                lcd_putc("% REF:");
                printf(lcd_putc, "%3Lu", reference_humidity);

                if (debounce(increase_button)) {
                    reference_humidity++;
                    if (reference_humidity > 100) reference_humidity = 100;
                }

                if (debounce(decrease_button)) {
                    reference_humidity--;
                    if (reference_humidity < 0) reference_humidity = 0;
                }

                lcd_gotoxy(1, 2);
                if (humidity > reference_humidity) {
                    output_high(fan_one);
                    output_low(fan_two); 
                    lcd_putc("FAN A ON");
                } else {
                    output_low(fan_one);
                    output_high(fan_two); 
                    lcd_putc("FAN B ON");
                }

                lcd_gotoxy(1, 3);
                lcd_putc("TEMP: ");
                printf(lcd_putc, "%3Lu", temperature);
                lcd_putc("C");
            }
        }
    }
}
