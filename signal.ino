#include <LiquidCrystal.h>

#define PLUS_BUTTON PC0
#define MINUS_BUTTON PC1
#define SET_BUTTON PC2
#define OK_BUTTON PC3
#define PIN_BUTTON PINC

// LCD дисплей
#define D4 7
#define D5 8
#define D6 9
#define D7 10
#define RS 5
#define E 6

#define GK PD2
#define PIN_GK PIND

#define LED PD4
#define PIN_LED PIND

#define BUZZ PD3
#define PORT_BUZZ PORTD

// true - нажаты, false - отжаты
bool pls_btn = false;
bool min_btn = false;
bool set_btn = false;
bool ok_btn = false;

// Время
uint8_t hour;
uint8_t mins;
uint8_t sec;

// Временные переменные
uint8_t hour_temp;
uint8_t mins_temp;
uint8_t sec_temp;

LiquidCrystal lcd(RS, E, D4, D5, D6, D7);

bool reed_open = true;

bool interval_switch = true;

bool security_sys = false;

bool prev = false;

bool alarm = false;

bool setting_flag = false;

bool signal_flag = false;

/*
    TODO:
    необходимо добавить EEPROM, в которой можно будет сохранять все настройки
*/

/*
режимы управления:
    0 - тестовый режим
    1 - просто часы
    2 - настройка по расписанию
    3 - ручной режим
    4 - ввод пароля

режимы настройки:
    8 - настройка интервалов
    9 - настройка времени в RTC

    99 - включилась охрана
*/

// Номер режима
uint8_t mode = 1;

// Курсор при настройке времени
uint8_t current_cursor = 0;

uint8_t counter = 0;

// Время расписания
uint8_t start[] = {0, 0};    // часы - минуты
uint8_t end[] = {0, 0};     // часы - минуты

// Пароль
uint8_t password[] = {0, 0, 0, 0};
uint8_t temp_password[] = {0, 0, 0, 0};

uint8_t sec_timer = 0;
bool timer_expired = false;

ISR(TIMER1_COMPA_vect) {
    sec_timer++;

    // Если время на ввод пароля истек, то воспроизвести звук излучателя
    if(timer_expired) {
        if(sec_timer == 1) {
            PORT_BUZZ |= (1<<BUZZ);
        }
        if(sec_timer == 4) {
            PORT_BUZZ &= ~(1<<BUZZ);
            sec_timer = 0;
        }
    } else {
        if(sec_timer == 15) {
            timer_expired = true;
            sec_timer = 0;
            OCR1A = 7812;
        }
    }

    
}   

void setup() {
    // Тестовый светодиод
    
    // Считыватель потенциала геркона
    DDRD &= ~(1<<PD2);
    PORTD |= (1<<PD2);

    DDRD |= (1<<LED) | (1<<BUZZ);
    PORTD &= ~((1<<LED) | (1<<BUZZ));
    //Serial.begin(9600);

    //previous = reed_switch_status();
    //prev = reed_switch_status();
    prev = true;

    lcd.begin(16, 2);

    lcd.setCursor(0, 1);
    lcd.print("bmstu");
    delay(4000);
    lcd.clear();
    sei();
    ds_init();
}

void loop() {

    if(mode == 0) {
        lcd.setCursor(0, 0);
        lcd.print("BUZZER ON  ");
        delay(1000);
        lcd.setCursor(0, 0);
        lcd.print("BUZZER OFF ");
        delay(500);
        return;

    }

    if(mode == 4) {
        /*
            Ввод пароля
        */
        if(!signal_flag) {
            signal_flag = true;
            setting_flag = false;
            clear_temp_password();
        }
        
        // Функция для отображения интерфейса ввода пароля
        enter_password();
        // Кнопка +
        if(~PIN_BUTTON & (1<<PLUS_BUTTON) && !pls_btn) {
            pls_btn = true;
            
            switch(current_cursor) {

                case 0: {
                    if(temp_password[0] + 1 < 10)
                        temp_password[0]++;
                    else
                        temp_password[0] = 0;
                    break;
                }

                case 1: {
                    if(temp_password[1] + 1 < 10)
                        temp_password[1]++;
                    else
                        temp_password[1] = 0;
                    break;
                }

                case 2: {
                    if(temp_password[2] + 1 < 10)
                        temp_password[2]++;
                    else
                        temp_password[2] = 0;
                    break;
                }

                case 3: {
                    if(temp_password[3] + 1 < 10)
                        temp_password[3]++;
                    else
                        temp_password[3] = 0;
                    break;
                }
            }

        }

        if(PIN_BUTTON & (1<<PLUS_BUTTON) && pls_btn) {
            pls_btn = false;
        }

        // Обработка кнопки -
        if(~PIN_BUTTON & (1<<MINUS_BUTTON) && !min_btn) {
            min_btn = true;
            switch(current_cursor) {

                case 0: {
                    if(temp_password[0] - 1 >= 0)
                        temp_password[0]--;
                    else
                        temp_password[0] = 9;
                    break;
                }
                case 1: {
                    if(temp_password[1] - 1 >= 0)
                        temp_password[1]--;
                    else
                        temp_password[1] = 9;
                    break;
                }

                case 2: {
                    if(temp_password[2] - 1 >= 0)
                        temp_password[2]--;
                    else
                        temp_password[2] = 9;
                    break;
                }

                case 3: {
                    if(temp_password[3] - 1 >= 0)
                        temp_password[3]--;
                    else
                        temp_password[3] = 9;
                    break;
                }
            }
        }

        if(PIN_BUTTON & (1<<MINUS_BUTTON) && min_btn) {
            min_btn = false;
        }

        // Set
        if(~PIN_BUTTON & (1<<SET_BUTTON) && !set_btn) {
            set_btn = true;
            if(current_cursor + 1 < 4)
                current_cursor++;
            else
                current_cursor = 0; 
        }

        if(PIN_BUTTON & (1<<SET_BUTTON) && set_btn) {
            set_btn = false;
        }

        // Обработка кнопки - ок
        if(~PIN_BUTTON & (1<<OK_BUTTON) && !ok_btn) {
            ok_btn = true;
            // Если пароль верный
            if(check_password()) {
                lcd.clear();
                mode = 1;   // 
                stop_timer();   // остановить таймер
                clear_temp_password();  // очистить временный пароль
                timer_expired = false;  // время на ввод пароля не истек
                alarm = false;      // снять предупреждение // см для чего нужно в чтении геркона
                clear_signal(); // выключить пьезоизлучатель
            } else {
                clear_temp_password();
            }
            current_cursor = 0;
            //signal_flag = false;
        }

        if(PIN_BUTTON & (1<<OK_BUTTON) && ok_btn) {
            ok_btn = false;
        }

    }

    // Если охранная система включена
    if(security_sys) {
        get_time();     // получает текущее время
        if(start[0] > end[0]) {
            if(hour == start[0] && mins >= start[1] || hour > start[0] || hour < end[0] || hour == end[0] && mins <= end[1]) {
                PORTD |= (1<<LED);
                bool rd = reed_switch_status();
                // Если открылась дверь
                if(rd != prev && rd == false) { 
                    prev = rd;
                    mode = 4;
                    // Если открылась, то зафиксировать, чтобы не очищался каждый раз пароль
                    if(!alarm) {
                        alarm = true;
                        clear_temp_password();      // очистить, если в первый раз вызвалось, чтобы не очищалось при каждом..
                        // ..открытии двери
                    }
                    start_timer();  // Начать отсчет
                    delay(200);     // задержка, чтобы дребезг не фиксировать
                    lcd.clear();
                }
                // Если дверь закрыли
                if(rd != prev && rd == true) {
                    prev = rd;
                    delay(200); // задержка, чтобы дребезг не фиксировать
                }
            }
        }        
    } else {
        prev = true;
        PORTD &= ~(1<<LED);
    }

    // Общее - показывает текущее время
    if(mode == 1) {
        get_time();
        show_time(hour, mins, sec);
        delay(30);

        // Переход в настройки времени
        if((~PIN_BUTTON & (1<<SET_BUTTON)) && !set_btn) {
            mode = 9;
            set_btn = true;

            hour_temp = hour;
            mins_temp = mins;
            sec_temp = sec;
        }

        if(PIN_BUTTON & (1<<OK_BUTTON) && ok_btn) {
            ok_btn = false;
        }

        // Обработка кнопки +
        if(~PIN_BUTTON & (1<<PLUS_BUTTON) && !pls_btn) {
            pls_btn = true;

            if(mode + 1 < 8)
                mode++;
            else
                mode = 1;
        }
        
        if(PIN_BUTTON & (1<<PLUS_BUTTON) && pls_btn) {
            pls_btn = false;
        }

        // Обработка кнопки -
        if(PIN_BUTTON & (1<<MINUS_BUTTON) && min_btn) {
            min_btn = false;
        }

        // Обработка кнопки - ок
        if(~PIN_BUTTON & (1<<OK_BUTTON) && !ok_btn) {
            ok_btn = true;
            //send_sms();
        }

        if(PIN_BUTTON & (1<<OK_BUTTON) && ok_btn) {
            ok_btn = false;
        }

    }

    // Показ интервала
    if(mode == 2) {

        // Вывод на экран
        show_time_param();
        if(security_sys) {
            lcd.setCursor(14, 0);
            lcd.print("ON");
        } else {
            lcd.setCursor(14, 0);
            lcd.print("  ");
        }

        // Обработка кнопки +
        if(~PIN_BUTTON & (1<<PLUS_BUTTON) && !pls_btn) {
            pls_btn = true;

        }

        if(PIN_BUTTON & (1<<PLUS_BUTTON) && pls_btn) {
            pls_btn = false;
        }


        // Обработка кнопки -
        if(~PIN_BUTTON & (1<<MINUS_BUTTON) && !min_btn) {
            min_btn = true;
            mode--;
            lcd.clear();
        }

        if(PIN_BUTTON & (1<<MINUS_BUTTON) && min_btn) {
            min_btn = false;
        }

        // Обработка кнопки set
        if(~PIN_BUTTON & (1<<SET_BUTTON) && !set_btn) {
            set_btn = true;
            mode = 8;
        }

        if(PIN_BUTTON & (1<<SET_BUTTON) && set_btn) {
            set_btn = false;
        }

        // Обработка кнопки - ок
        if(~PIN_BUTTON & (1<<OK_BUTTON) && !ok_btn) {
            ok_btn = true;
            security_sys = !security_sys;
        }

        if(PIN_BUTTON & (1<<OK_BUTTON) && ok_btn) {
            ok_btn = false;
        }


    }

    // Настройка интервала
    if(mode == 8) {
        lcd.setCursor(15, 0);
        lcd.print("*");
        //lcd.setCursor(15, 1);
        //lcd.print("*");
        show_time_param();

        lcd.setCursor(15, 0);
        lcd.print("*");
        // Обработка кнопок

        // Кнопка +
        if(~PIN_BUTTON & (1<<pls_btn) && !pls_btn) {
            pls_btn = true;

            switch(current_cursor) {
                case 0: {
                    if(start[0] + 1 < 24)
                        start[0]++;
                    else
                        start[0] = 0;
                    break;
                }
                case 1: {
                    if(start[1] + 1 < 60)
                        start[1]++;
                    else
                        start[1] = 0;
                    break;
                }
                case 2: {
                    if(end[0] + 1 < 24)
                        end[0]++;
                    else
                        end[0] = 0;
                    break;
                }
                case 3: {
                    if(end[1] + 1 < 60)
                        end[1]++;
                    else
                        end[1] = 0;
                    break;
                }
            }

        }

        if(PIN_BUTTON & (1<<PLUS_BUTTON) && pls_btn) {
            pls_btn = false;
        }

        // Кнопка -
        if(~PIN_BUTTON & (1<<MINUS_BUTTON) && !min_btn) {
            min_btn = true; 

            switch(current_cursor) {
                case 0: {
                    if(start[0] - 1 >= 0)
                        start[0]--;
                    else
                        start[0] = 23;
                    break;
                }
                case 1: {
                    if(start[1] - 1 >= 0)
                        start[1]--;
                    else
                        start[1] = 59;
                    break;
                }
                case 2: {
                    if(end[0] - 1 >= 0)
                        end[0]--;
                    else
                        end[0] = 23;
                    break;
                }
                case 3: {
                    if(end[1] - 1 >= 0)
                        end[1]--;
                    else
                        end[1] = 59;
                    break;
                }
            }
        }

        if(PIN_BUTTON & (1<<MINUS_BUTTON) && min_btn) {
            min_btn = false;
        }

        // Кнопка set
        if(~PIN_BUTTON & (1<<SET_BUTTON) && !set_btn) {
            set_btn = true;
            if(current_cursor + 1 < 4)
                current_cursor++;
            else{
                current_cursor = 0; 
            }
        }

        if(PIN_BUTTON & (1<<SET_BUTTON) && set_btn) {
            set_btn = false;
        }

        // Кнопка ОК
        if(~PIN_BUTTON & (1<<OK_BUTTON) && !ok_btn) {
            ok_btn = true;
            current_cursor = 0;     // установка курсора по умолчанию
            mode = 2;
            lcd.clear();
        }

    }

    // Режим настройки RTC + пароля
    if(mode == 9) {

        // во временный пароль ззаносится реальный пароль, а по окончании настройки, временный будет очищен
        if(!setting_flag) {
            update_temp_password();
            setting_flag = true;
        }

        if(current_cursor < 3) {
            show_time_setings(hour_temp, mins_temp, sec_temp);
        } else {
            show_password_param();
        }
        // TODO: Добавить надпись настройки
        
        // Кнопка +
        if((~PIN_BUTTON & (1<<PLUS_BUTTON)) && !pls_btn) {
            pls_btn = true;
            
            switch(current_cursor) {
                // Настройка часов
                case 0: {
                    if(hour_temp + 1 < 24)
                        hour_temp++;
                    else
                        hour_temp = 0;
                    break;
                }

                // Настройка минут
                case 1: {
                    if(mins_temp + 1 < 60)
                        mins_temp++;
                    else
                        mins_temp = 0;
                    break;
                }

                // Настройка секунд
                case 2: {
                    sec_temp = 0;
                    break;
                }

                case 3: {
                    if(temp_password[0] + 1 < 10)
                        temp_password[0]++;
                    else
                        temp_password[0] = 0;
                    break;
                }

                case 4: {
                    if(temp_password[1] + 1 < 10)
                        temp_password[1]++;
                    else
                        temp_password[1] = 0;
                    break;
                }

                case 5: {
                    if(temp_password[2] + 1 < 10)
                        temp_password[2]++;
                    else
                        temp_password[2] = 0;
                    break;
                }

                case 6: {
                    if(temp_password[3] + 1 < 10)
                        temp_password[3]++;
                    else
                        temp_password[3] = 0;
                    break;
                }
            }
        }

        if(PIN_BUTTON & (1<<PLUS_BUTTON) && pls_btn) {
            pls_btn = false;
        }

        // Кнопка -
        if((~PIN_BUTTON & (1<<MINUS_BUTTON)) && !min_btn) {
            min_btn = true;

            switch(current_cursor) {

                // Убавить часы
                case 0: {
                    if(hour_temp - 1 >= 0)
                        hour_temp--;
                    else
                        hour_temp = 23;
                    break;
                }

                // Убавить минуты
                case 1: {
                    if(mins_temp - 1 >= 0)
                        mins_temp--;
                    else
                        mins_temp = 59;
                    break;
                }

                case 2: {
                    sec_temp = 0;
                    break;
                }

                case 3: {
                    if(temp_password[0] - 1 >= 0)
                        temp_password[0]--;
                    else
                        temp_password[0] = 9;
                    break;
                }

                case 4: {
                    if(temp_password[1] - 1 >= 0)
                        temp_password[1]--;
                    else
                        temp_password[1] = 9;
                    break;
                }

                case 5: {
                    if(temp_password[2] - 1 >= 0)
                        temp_password[2]--;
                    else
                        temp_password[2] = 9;
                    break;
                }

                case 6: {
                    if(temp_password[3] - 1 >= 0)
                        temp_password[3]--;
                    else
                        temp_password[3] = 9;
                    break;
                }
            }

        } 

        if(PIN_BUTTON & (1<<MINUS_BUTTON) && min_btn) {
            min_btn = false;
        }


        // Кнопка settings
        if((~PIN_BUTTON & (1<<SET_BUTTON)) && !set_btn) {
            set_btn = true;
            if(current_cursor + 1 < 7) {
                current_cursor++;
                if(current_cursor == 3)
                    lcd.clear();
            }
            else{
                current_cursor = 0;
                lcd.clear();
            }
        }

        if(PIN_BUTTON & (1<<SET_BUTTON) && set_btn) {
            set_btn = false;
        }


        // Кнопка ОК
        if((~PIN_BUTTON & (1<<OK_BUTTON)) && !ok_btn) {
            ok_btn = true;
            mode = 1;       // Вернуться во вкладку со временем
            current_cursor = 0;     // вернуть курсор в начало
            set_time(hour_temp, mins_temp, sec_temp);       // установить время
            update_password();
            lcd.clear();
            clear_temp_password();
            setting_flag = false;
        }
    }
}

// Вывод параметров интервала времени
void show_time_param() {
    // Вывод на экран
        lcd.setCursor(0, 0);
        lcd.print("str: ");

        if(start[0] < 10) {
            lcd.print("0");
            lcd.print(start[0]);
        } else
            lcd.print(start[0]);
        
        lcd.print(":");

        if(start[1] < 10) {
            lcd.print("0");
            lcd.print(start[1]);
        } else
            lcd.print(start[1]);

        lcd.print("   ");

        // end
        lcd.setCursor(0, 1);
        lcd.print("end: ");

        if(end[0] < 10) {
            lcd.print("0");
            lcd.print(end[0]);
        } else
            lcd.print(end[0]);
        
        lcd.print(":");

        if(end[1] < 10) {
            lcd.print("0");
            lcd.print(end[1]);
        } else
            lcd.print(end[1]);
}

void show_password_param() {
    lcd.setCursor(0, 0);
    lcd.print("PASS: ");

    lcd.print(temp_password[0]);
    lcd.print(" ");
    lcd.print(temp_password[1]);
    lcd.print(" ");
    lcd.print(temp_password[2]);
    lcd.print(" ");
    lcd.print(temp_password[3]);
    lcd.setCursor(0, 1);
    lcd.print("                ");
}

void show_time(uint8_t hour_, uint8_t mins_, uint8_t sec_) {
    lcd.setCursor(0, 0);
    if(hour_ < 10) {
        lcd.print("0");
        lcd.print(hour_);
    } else
        lcd.print(hour_);

    lcd.print(":");

    if(mins_ < 10) {
        lcd.print("0");
        lcd.print(mins_);
    } else
        lcd.print(mins_);

    lcd.print(":");

    if(sec_ < 10) {
        lcd.print("0");
        lcd.print(sec_);
    } else
        lcd.print(sec_);
    
}

void show_time_setings(uint8_t hour_, uint8_t mins_, uint8_t sec_) {
    lcd.setCursor(0, 0);
    lcd.print("TIME: ");
    if(hour_ < 10) {
        lcd.print("0");
        lcd.print(hour_);
    } else
        lcd.print(hour_);

    lcd.print(":");

    if(mins_ < 10) {
        lcd.print("0");
        lcd.print(mins_);
    } else
        lcd.print(mins_);

    lcd.print(":");

    if(sec_ < 10) {
        lcd.print("0");
        lcd.print(sec_);
    } else
        lcd.print(sec_);
}

// Получить время из RTC
void get_time() {
    sec = get_seconds();
    mins = get_mins();
    hour = get_hour();
}

bool reed_switch_status() {
    // Если потенциал геркона низкий, т.е. разомкнут, то вернет false
    // иначе - true
    if(~PIND & (1<<PD2)) {
        return false;
    } else
        return true;
    
}

void update_password() {
    for(int i = 0; i < 4; i++) {
        password[i] = temp_password[i];
    }
}

void update_temp_password() {
    for(int i = 0; i < 4; i++) {
        temp_password[i] = password[i];
    }
}

void enter_password() {
    lcd.setCursor(0, 0);
    lcd.print("ENT: ");
    lcd.print(temp_password[0]);
    lcd.print(" ");
    lcd.print(temp_password[1]);
    lcd.print(" ");
    lcd.print(temp_password[2]);
    lcd.print(" ");
    lcd.print(temp_password[3]);
    lcd.print(" ");

}

bool check_password() {
    //bool correct = true;

    for(int i = 0; i < 4; i++) {
        if(password[i] != temp_password[i]) {
            //correct = false;
            return false;
        }
    }

    return true;
}

void clear_temp_password() {
    temp_password[0] = 0;
    temp_password[1] = 0;
    temp_password[2] = 0;
    temp_password[3] = 0;
}

// Функция для отключения сигнализации
void clear_signal() {
    PORT_BUZZ &= ~(1<<BUZZ);
}

void start_timer() {
    TCCR1A = 0;
    TCCR1B = 0;

    TCCR1B |= (1<<CS12);
    TCCR1B &= ~((1<<CS11) | (1<<CS10));
    TIMSK1 |= (1<<OCIE1A);
    OCR1A = 15625;
    TCNT1 = 0;
    TCCR1B |= (1<<WGM12);
}

void stop_timer() {
    TCNT1 = 0;
    TCCR1B &= ~((1<<CS12) | (1<<CS11) | (1<<CS10));
    sec_timer = 0;
}

void send_sms() {
    Serial.println("AT");
    updateSerial();
    Serial.println("AT+CCID");
    updateSerial();
    Serial.println("AT+CREG?");
    updateSerial();
    Serial.println("AT+CPAS");
    updateSerial();
    Serial.println("AT+CMGF=1");
    updateSerial();
    delay(1000);
    Serial.println("AT+CMGS=\"+79163710048\"");
    updateSerial();
    Serial.print("Door has been opened");
    updateSerial();
    Serial.write(26);
}

void updateSerial() {
    delay(500);
    while (Serial.available()) {
        Serial.read();
    }
}

// Функции для работы с EEPROM
void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
    /* Wait for completion of previous write */
    while(EECR & (1<<EEPE));
    /* Set up address and Data Registers */
    EEAR = uiAddress;
    EEDR = ucData;
    /* Write logical one to EEMPE */
    EECR |= (1<<EEMPE);
    /* Start eeprom write by setting EEPE */
    EECR |= (1<<EEPE);
}

unsigned char EEPROM_read(unsigned int uiAddress)
{
/* Wait for completion of previous write */
    while(EECR & (1<<EEPE));
    /* Set up address register */
    EEAR = uiAddress;
    /* Start eeprom read by writing EERE */
    EECR |= (1<<EERE);
    /* Return data from Data Register */
    return EEDR;
}
