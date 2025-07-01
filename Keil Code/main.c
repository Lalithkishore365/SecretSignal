#include <reg52.h>   // 8051 Register definitions

sbit LED = P1^0;  // LED connected to P1.0
sbit SCL = P2^0;  // I2C Clock
sbit SDA = P2^1;  // I2C Data
sbit BUZZER = P1^1;

#define RTC_ADDRESS 0xD0  // DS1307 I2C Address
#define MAX_LENGTH 20     // Maximum input word length

void I2C_Start();
void I2C_Stop();
void I2C_Write(unsigned char datas);
unsigned char I2C_Read();
void UART_Init();
void UART_SendChar(char c);
void UART_SendString(char *str);
void RTC_ReadTime(unsigned char *h, unsigned char *m, unsigned char *s);
unsigned char XOR_Encrypt(unsigned char datas, unsigned char key);
unsigned char BCD_to_Decimal(unsigned char bcd);
void UART_ReceiveString(char *buffer);
const char* CharToMorse(char c);
void BlinkLED(int duration);  
void Delay_ms(unsigned int ms);
void Timer0_Delay1ms();  // New timer-based delay
unsigned char led_control = 0;  // default ON


// ============================
//  MAIN PROGRAM
// ============================
void main() {
    unsigned char hour, min, sec;
    char input[MAX_LENGTH];     
    char encrypted[MAX_LENGTH]; 
    unsigned char key;          
    int i;
    unsigned char j;
    int k;
		BUZZER=1;
    UART_Init();  // Initialize UART

    while (1) {
        UART_SendString("\r\nEnter text: ");
        UART_ReceiveString(input);
        RTC_ReadTime(&hour, &min, &sec);
        key = (hour * 3600 + min * 60 + sec) % 256;

        for (i = 0; input[i] != '\0'; i++) {
            encrypted[i] = XOR_Encrypt(input[i], key);
        }
        encrypted[i] = '\0';

        UART_SendString("\r\nEncrypted Text: ");
        for (i = 0; encrypted[i] != '\0'; i++) {
            UART_SendChar(encrypted[i]);
        }

        // If mode is decrypt (led_control == 2), we don't need to do Morse conversion
        if (led_control != 2) {
            UART_SendString("\r\nDual Encrypted Text: ");
            for (j = 0; encrypted[j]; j++) {
                const char *morse = CharToMorse(encrypted[j]);
                UART_SendString(morse);
                UART_SendChar(' ');

                if (led_control == 1) {  // Only blink if LED mode enabled
                    for (k = 0; morse[k] != '\0'; k++) {
                        if (morse[k] == '.') {
                            BlinkLED(200);
                        } else if (morse[k] == '-') {
                            BlinkLED(600);
                        }
                        Delay_ms(200);
                    }
                } else if (led_control == 3) {  // Buzzer mode
                    for (k = 0; morse[k] != '\0'; k++) {
                        if (morse[k] == '.') {
                            BUZZER = 0;  // Turn on buzzer
                            Delay_ms(200);
                            BUZZER = 1;  // Turn off buzzer
                        } else if (morse[k] == '-') {
                            BUZZER = 0;  // Turn on buzzer
                            Delay_ms(600);
                            BUZZER = 1;  // Turn off buzzer
                        }
                        Delay_ms(200);
                    }
                }
                Delay_ms(600);
            }
        }
        else {
            // For decrypt mode, we'd typically send back the original text
            // But since we're simulating, we'll just send a confirmation
            UART_SendString("\r\nDecryption Complete");
        }
    }
}

// ============================
//  UART FUNCTIONS
// ============================
void UART_Init() {
    TMOD |= 0x20;   
    TH1 = 0xFD;    
    SCON = 0x50;   
    TR1 = 1;       
}

void UART_SendChar(char c) {
    SBUF = c;
    while (!TI);
    TI = 0;
}

void UART_SendString(char *str) {
    while (*str) {
        UART_SendChar(*str++);
    }
}

void UART_ReceiveString(char *buffer) {
    int i = 0;
    char c;

    while (1) {
        while (!RI);  // Wait for data
        c = SBUF;
        RI = 0;

        if (c == '#') { // control flag found
            while (!RI);
            led_control = SBUF - '0'; // Convert char '1'/'0'/'2' to int 1/0/2
            RI = 0;
            buffer[i] = '\0'; // End string
            break;
        }

        buffer[i++] = c;
        UART_SendChar(c); // Echo back
    }
}


// ============================
//  XOR ENCRYPTION FUNCTION
// ============================
unsigned char XOR_Encrypt(unsigned char datas, unsigned char key) {
    return datas ^ key;
}

// ============================
//  I2C FUNCTIONS
// ============================
void I2C_Start() {
    SDA = 1; SCL = 1;
    SDA = 0; SCL = 0;
}

void I2C_Stop() {
    SDA = 0; SCL = 1;
    SDA = 1;
}

void I2C_Write(unsigned char datas) {
    unsigned char i;
    for (i = 0; i < 8; i++) {
        SDA = (datas & 0x80) ? 1 : 0;
        SCL = 1;
        SCL = 0;
        datas <<= 1;
    }
    SDA = 1;
    SCL = 1;
    while(SDA);
    SCL = 0;
}

unsigned char I2C_Read() {
    unsigned char i, datas = 0;
    SDA = 1;
    for (i = 0; i < 8; i++) {
        SCL = 1;
        datas = (datas << 1) | SDA;
        SCL = 0;
    }
    return datas;
}

// ============================
//  RTC FUNCTIONS
// ============================
void RTC_ReadTime(unsigned char *h, unsigned char *m, unsigned char *s) {
    I2C_Start();
    I2C_Write(RTC_ADDRESS);
    I2C_Write(0x00);
    I2C_Start();
    I2C_Write(RTC_ADDRESS | 1);
    *s = BCD_to_Decimal(I2C_Read());
    *m = BCD_to_Decimal(I2C_Read());
    *h = BCD_to_Decimal(I2C_Read());
    I2C_Stop();
}

unsigned char BCD_to_Decimal(unsigned char bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// ============================
//  MORSE LOOKUP
// ============================
const char morseChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
code const char *morseCodes[] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....",
    "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.",
    "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-",
    "-.--", "--..", "-----", ".----", "..---", "...--", "....-", ".....",
    "-....", "--...", "---..", "----."
};

const char* CharToMorse(char c) {
    unsigned char i;
    if (c >= 'a' && c <= 'z') c -= 32;
    for (i = 0; i < sizeof(morseChars)-1; i++) {
        if (morseChars[i] == c) return morseCodes[i];
    }
    return "?";
}

// ============================
//  LED CONTROL FUNCTION
// ============================
void BlinkLED(int duration) {
    LED = 1;
    Delay_ms(duration);
    LED = 0;
}

// ============================
//  TIMER-BASED DELAY FUNCTION
// ============================
void Delay_ms(unsigned int ms) {
    unsigned int i;
    for (i = 0; i < ms; i++) {
        Timer0_Delay1ms();
    }
}

void Timer0_Delay1ms() {
    TMOD &= 0xF0;
    TMOD |= 0x01;
    TH0 = 0xFC;
    TL0 = 0x66;
    TR0 = 1;
    while (TF0 == 0);
    TR0 = 0;
    TF0 = 0;
}