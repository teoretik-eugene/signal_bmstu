#define DS1307_ADDR 0x68

bool ds3232_en = true;


void i2c_init() {
    TWBR = 8;
    TWSR |= (1<<TWPS0);
    TWSR &= ~(1<<TWPS1);
}

void ds_init() {
    if(ds3232_en) {
        set_value_by_reg(0x0E, 0b00000000);
        set_value_by_reg(0x0F, 0b00000000);
    }
}

void i2c_start() {
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);

    while (!(TWCR & (1<<TWINT)));

}

uint8_t get_value_by_reg(uint8_t addr) {
    uint8_t data;

    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);		// Формируем состояние старт
	while(!(TWCR & (1<<TWINT)));		// Ожидаем когда закончится операция старт

    TWDR = (DS1307_ADDR<<1) | 0;	// Подали адрес на запись
	TWCR = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR & (1<<TWINT)));

    // Передаем адрес регистра, куда будет происходить запись
	TWDR = addr;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));

    // Формируем сигнал стоп
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);

    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);		// Формируем состояние старт
	while(!(TWCR & (1<<TWINT)));		// Ожидаем когда закончится операция старт

    TWDR = (DS1307_ADDR<<1) | 1;	// Подали адрес на чтение
	TWCR = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR & (1<<TWINT)));

    // Читаем данные
	TWCR = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	data = TWDR;

    // Состояние стоп
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
	
	return data;

}

uint8_t set_value_by_reg(uint8_t addr, uint8_t data) {
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);		// Формируем состояние старт
	while(!(TWCR & (1<<TWINT)));		// Ожидаем когда закончится операция старт
	
	// Передаем адресный пакет
	TWDR = (DS1307_ADDR<<1) | 0;	// Передаем адрес приемника и режим на запись
	TWCR = (1<<TWINT)|(1<<TWEN);	// Сбрасываем бит прерывания и разрешаем работу пинов
	while(!(TWCR & (1<<TWINT)));	// Пока не завершится операция
	
	// Передаем адрес регистра, с которого будет происходить запись
	TWDR = addr;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));

    TWDR = data;
    TWCR = (1<<TWINT)|(1<<TWEN); 
	while(!(TWCR & (1<<TWINT)));
	
	// Формируем сигнал стоп
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

uint8_t get_seconds() {
    uint8_t bytes = get_value_by_reg(0b00);
    uint8_t sec = (bytes>>4) * 10 + (bytes&0b1111);

    return sec;
}

uint8_t get_mins() {
    uint8_t bytes = get_value_by_reg(0b01);
    return (bytes>>4)*10 + (bytes&0b1111);
}

uint8_t get_hour() {
    uint8_t bytes = get_value_by_reg(0b10);
    return ((bytes>>5) & 1)*20 + ((bytes>>4) & 1)*10 + (bytes & 0b1111);
}

uint8_t get_hour_mode_bit() {
    uint8_t bytes = get_value_by_reg(0b10);
    return (bytes>>4) & 1;
}

void set_hour_mode_24() {
    
    uint8_t bytes = get_value_by_reg(2);
    bytes &= ~(1<<6);

}

void set_time(uint8_t hour_, uint8_t mins_, uint8_t sec_) {
    uint8_t data;
    
    // Установка секунд
    data = ((sec_ / 10) << 4) | (sec_ % 10);
    set_value_by_reg(0, data);

    // Установка минут
    data = ((mins_/10) << 4) | (mins_ % 10);
    set_value_by_reg(1, data);

    // Установка часов
    data = ((hour_/20) << 5) | ((hour_/10) << 4) | (hour_ % 10);
    set_value_by_reg(2, data);
}


