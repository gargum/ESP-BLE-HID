#include <Arduino.h>
#include "OLED.h"

OLED::OLED(uint8_t sda_pin, uint8_t scl_pin, tDisplayCtrl displayCtrl, uint8_t i2c_address) :
sda_pin(sda_pin),
scl_pin(scl_pin),
i2c_address(i2c_address),
displayController(displayCtrl)
{
    this->buffer = (uint8_t *) malloc(bufsize);
    X=0;
    Y=0;
    ttyMode=OLED_DEFAULT_TTY_MODE;
    fontInverted=false;
    usingOffset=false;
}

OLED::~OLED()
{
    free(buffer);
}

void OLED::i2c_start()
{
    while (!digitalRead(sda_pin) || !digitalRead(scl_pin));
    digitalWrite(sda_pin, LOW);
    pinMode(sda_pin, OUTPUT);
    OLED_I2C_DELAY;
    digitalWrite(scl_pin, LOW);
    pinMode(scl_pin, OUTPUT);
    OLED_I2C_DELAY;
}

void OLED::i2c_stop()
{
    pinMode(scl_pin, INPUT);
    digitalWrite(scl_pin, HIGH);
    OLED_I2C_DELAY;
    pinMode(sda_pin, INPUT);
    digitalWrite(sda_pin, HIGH);
    OLED_I2C_DELAY;
    while (!digitalRead(sda_pin) || !digitalRead(scl_pin));
}

bool OLED::i2c_send(uint8_t byte)
{
    for (int_fast8_t bit = 7; bit >= 0; bit--)
    {
        if (byte & 128) {
          pinMode(sda_pin, INPUT);
          digitalWrite(sda_pin, HIGH);
        } else {
          digitalWrite(sda_pin, LOW);
          pinMode(sda_pin, OUTPUT);
        }
        OLED_I2C_DELAY;
        pinMode(scl_pin, INPUT);
        digitalWrite(scl_pin, HIGH);        
        OLED_I2C_DELAY;
        while (!digitalRead(scl_pin)); 
        digitalWrite(scl_pin, LOW);
        pinMode(scl_pin, OUTPUT);
        OLED_I2C_DELAY;
        byte = byte << 1;
    }
    pinMode(sda_pin, INPUT);
    digitalWrite(sda_pin, HIGH);
    pinMode(scl_pin, INPUT);
    digitalWrite(scl_pin, HIGH);    
    OLED_I2C_DELAY;
    while (!digitalRead(scl_pin)); 
    bool ack = digitalRead(sda_pin) == 0;    
    digitalWrite(scl_pin, LOW);
    pinMode(scl_pin, OUTPUT);
    OLED_I2C_DELAY;
    return ack;
}

void OLED::begin()
{

    pinMode(sda_pin, INPUT);
    pinMode(scl_pin, INPUT);
    
    delay(100);
    i2c_start();
    i2c_send(i2c_address << 1);
    i2c_send(0x00);
    i2c_send(0xAE);
    i2c_send(0xD5);
    i2c_send(0x80); 
    i2c_send(0xA8);
    i2c_send(63); 
    i2c_send(0xD3);
    i2c_send(0x00); 
    i2c_send(0x40);
    i2c_send(0x8D);
    i2c_send(0x14); 
    i2c_send(0x20);
    i2c_send(0x00); 
    i2c_send(0xA1);
    i2c_send(0xC8);
    i2c_send(0xDA);
    i2c_send(0x12); 
    i2c_send(0x81);
    i2c_send(0x80);
    i2c_send(0xD9);
    i2c_send(0x22); 
    i2c_send(0xDB);
    i2c_send(0x20); 
    i2c_send(0xA4);
    i2c_send(0xA6);
    i2c_send(0x2E);
    i2c_stop();
    delay(100);

    clear();
    display();
    set_power(true);
}

void OLED::set_power(bool enable)
{
    i2c_start();
    i2c_send(i2c_address << 1);
    i2c_send(0x00);
    if (enable)
    {
        i2c_send(0x8D);
        i2c_send(0x14);
        i2c_send(0xAF);
    }
    else
    {
        i2c_send(0xAE);
        i2c_send(0x8D);
        i2c_send(0x10);
    }
    i2c_stop();
}

void OLED::set_invert(bool enable)
{
    i2c_start();
    i2c_send(i2c_address << 1);
    i2c_send(0x00);
    if (enable)
    {
        i2c_send(0xA7);
    }
    else
    {
        i2c_send(0xA6);
    }
    i2c_stop();
}

void OLED::set_scrolling(tScrollEffect scroll_type, uint_fast8_t first_page, uint_fast8_t last_page)
{
    i2c_start();
    i2c_send(i2c_address << 1);
    i2c_send(0x00);
    i2c_send(0x2E);
    if (scroll_type == DIAGONAL_LEFT || scroll_type == DIAGONAL_RIGHT)
    {
        i2c_send(0xA3);
        i2c_send(0x00);
        i2c_send(63);
    }
    if (scroll_type != NO_SCROLLING)
    {
        i2c_send(scroll_type);
        i2c_send(0x00);
        i2c_send(first_page);
        i2c_send(0x00);
        i2c_send(last_page);
        if (scroll_type == DIAGONAL_LEFT || scroll_type == DIAGONAL_RIGHT)
        {
            i2c_send(0x01);
        }
        else
        {
            i2c_send(0x00);
            i2c_send(0xFF);
        }
        i2c_send(0x2F);
    }
    i2c_stop();
}

void OLED::set_contrast(uint8_t contrast)
{
    i2c_start();
    i2c_send(i2c_address << 1);
    i2c_send(0x00);
    i2c_send(0x81);
    i2c_send(contrast); 
    i2c_stop();
}

void OLED::clear(tColor color)
{
    if (color == WHITE)
    {
        memset(buffer, 0xFF, bufsize);
    }
    else
    {
        memset(buffer, 0x00, bufsize);
    }
    X=0;
    Y=0;
}

void OLED::display()
{
    uint16_t index = 0;
    for (uint_fast8_t page = 0; page < pages; page++)
    {
        i2c_start();
        i2c_send(i2c_address << 1);
        i2c_send(0x00);
        if (displayController == CTRL_SH1106)
        {
            i2c_send(0xB0 + page);
            i2c_send(0x00);
            i2c_send(0x10);
        }
        else
        {
            i2c_send(0xB0 + page);
            i2c_send(0x21);
            i2c_send(0x00);
            i2c_send(127);
        }
        i2c_stop();

        i2c_start();
        i2c_send(i2c_address << 1);
        i2c_send(0x40);
        if(usingOffset){
            i2c_send(0);
            i2c_send(0);
        }
        for (uint_fast8_t column = 0; column < 128; column++)
        {
            i2c_send(buffer[index++]);
        }
        i2c_stop();
        yield();
    }
}

void OLED::draw_byte(uint_fast8_t x, uint_fast8_t y, uint8_t b, tColor color)
{
    if (x >= 128 || y >= 64) return;

    uint_fast16_t buffer_index = y / 8 * 128 + x;

    if (fontInverted) {
        b^=255;
    }

    if (color == WHITE)
    {
        if (y % 8 == 0)
        {
            if (buffer_index < bufsize) buffer[buffer_index] |= b;
        }
        else
        {
            uint16_t w = (uint16_t) b << (y % 8);
            if (buffer_index < bufsize) buffer[buffer_index] |= (w & 0xFF);
            uint16_t buffer_index2 = buffer_index + 128;
            if (buffer_index2 < bufsize) buffer[buffer_index2] |= (w >> 8);
        }
    }
    else
    {
        if (y % 8 == 0)
        {
            if (buffer_index < bufsize) buffer[buffer_index] &= ~b;
        }
        else
        {
            uint16_t w = (uint16_t) b << (y % 8);
            if (buffer_index < bufsize) buffer[buffer_index] &= ~(w & 0xFF);
            uint16_t buffer_index2 = buffer_index + 128;
            if (buffer_index2 < bufsize) buffer[buffer_index2] &= ~(w >> 8);
        }
    }
}

void OLED::draw_bytes(uint_fast8_t x, uint_fast8_t y, const uint8_t* data, uint_fast8_t size, tFontScaling scaling, tColor color, bool useProgmem)
{
    for (uint_fast8_t column = 0; column < size; column++)
    {
        uint8_t b = useProgmem ? pgm_read_byte(data) : *data;
        data++;
        if (scaling == DOUBLE_SIZE)
        {
            uint16_t w = 0;
            for (uint_fast8_t bit = 0; bit < 7; bit++)
            {
                if (b & (1 << bit))
                {
                    uint_fast8_t pos = bit << 1;
                    w |= ((1 << pos) | (1 << (pos + 1)));
                }
            }
            draw_byte(x, y, w & 0xFF, color);
            draw_byte(x, y + 8, (w >> 8), color);
            x++;
            draw_byte(x, y, w & 0xFF, color);
            draw_byte(x, y + 8, (w >> 8), color);
            x++;
        }
        else
        {
            draw_byte(x++, y, b, color);
        }
    }
}

size_t OLED::draw_character(uint_fast8_t x, uint_fast8_t y, char c, tFontScaling scaling, tColor color)
{
    if (x >= 128 || y >= 64 || c < 32) return 0;

    switch ((unsigned char) c)
    {
        case 252: c = 127; break;
        case 220: c = 128; break;
        case 228: c = 129; break;
        case 196: c = 130; break;
        case 246: c = 131; break;
        case 214: c = 132; break;
        case 176: c = 133; break;
        case 223: c = 134; break;
    }

    uint16_t font_index = (c - 32)*6;
    if (font_index >= sizeof (oled_font6x8)) return 0;

    draw_bytes(x, y, &oled_font6x8[font_index], 6, scaling, color, true);
    return 1;
}

void OLED::draw_string(uint_fast8_t x, uint_fast8_t y, const char* s, tFontScaling scaling, tColor color)
{
    while (*s)
    {
        draw_character(x, y, *s, scaling, color);
        x += (scaling == DOUBLE_SIZE) ? 12 : 6;
        s++;
    }
}

void OLED::draw_string_P(uint_fast8_t x, uint_fast8_t y, const char* s, tFontScaling scaling, tColor color)
{
    char c;
    while ((c = pgm_read_byte(s)))
    {
        draw_character(x, y, c, scaling, color);
        x += (scaling == DOUBLE_SIZE) ? 12 : 6;
        s++;
    }
}

void OLED::draw_bitmap(uint_fast8_t x, uint_fast8_t y, uint_fast8_t bitmap_width, uint_fast8_t bitmap_height, const uint8_t* data, tColor color)
{
    uint_fast8_t num_pages = (bitmap_height + 7) / 8;
    for (uint_fast8_t page = 0; page < num_pages; page++)
    {
        draw_bytes(x, y, data, bitmap_width, NORMAL_SIZE, color, false);
        data += bitmap_width;
        y += 8;
    }
}

void OLED::draw_bitmap_P(uint_fast8_t x, uint_fast8_t y, uint_fast8_t bitmap_width, uint_fast8_t bitmap_height, const uint8_t* data, tColor color)
{
    uint_fast8_t num_pages = (bitmap_height + 7) / 8;
    for (uint_fast8_t page = 0; page < num_pages; page++)
    {
        draw_bytes(x, y, data, bitmap_width, NORMAL_SIZE, color, true);
        data += bitmap_width;
        y += 8;
    }
}

void OLED::draw_pixel(uint_fast8_t x, uint_fast8_t y, tColor color)
{
    if (x >= 128 || y >= 64) return;
    if (color == WHITE)
    {
        buffer[x + (y / 8) * 128] |= (1 << (y & 7));
    }
    else
    {
        buffer[x + (y / 8) * 128] &= ~(1 << (y & 7));
    }
}

void OLED::draw_line(uint_fast8_t x0, uint_fast8_t y0, uint_fast8_t x1, uint_fast8_t y1, tColor color)
{        
    int_fast16_t dx = abs(static_cast<int_fast16_t>(x1) - static_cast<int_fast16_t>(x0));
    int_fast16_t sx = x0 < x1 ? 1 : -1;
    int_fast16_t dy = -abs(static_cast<int_fast16_t>(y1) - static_cast<int_fast16_t>(y0));
    int_fast16_t sy = y0 < y1 ? 1 : -1;
    int_fast16_t err = dx + dy;
    int_fast16_t e2;

    while (1)
    {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 > dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void OLED::draw_circle(uint_fast8_t x0, uint_fast8_t y0, uint_fast8_t radius, tFillmode fillMode, tColor color)
{
    int_fast16_t f = 1 - radius;
    int_fast16_t ddF_x = 0;
    int_fast16_t ddF_y = -2 * radius;
    int_fast16_t x = 0;
    int_fast16_t y = radius;

    if (fillMode == SOLID)
    {
        draw_pixel(x0, y0 + radius, color);
        draw_pixel(x0, y0 - radius, color);
        draw_line(x0 - radius, y0, x0 + radius, y0, color);
    }
    else
    {
        draw_pixel(x0, y0 + radius, color);
        draw_pixel(x0, y0 - radius, color);
        draw_pixel(x0 + radius, y0, color);
        draw_pixel(x0 - radius, y0, color);
    }

    while (x < y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x + 1;

        if (fillMode == SOLID)
        {
            draw_line(x0 - x, y0 + y, x0 + x, y0 + y, color);
            draw_line(x0 - x, y0 - y, x0 + x, y0 - y, color);
            draw_line(x0 - y, y0 + x, x0 + y, y0 + x, color);
            draw_line(x0 - y, y0 - x, x0 + y, y0 - x, color);
        }
        else
        {
            draw_pixel(x0 + x, y0 + y, color);
            draw_pixel(x0 - x, y0 + y, color);
            draw_pixel(x0 + x, y0 - y, color);
            draw_pixel(x0 - x, y0 - y, color);
            draw_pixel(x0 + y, y0 + x, color);
            draw_pixel(x0 - y, y0 + x, color);
            draw_pixel(x0 + y, y0 - x, color);
            draw_pixel(x0 - y, y0 - x, color);
        }
    }
}

void OLED::draw_rectangle(uint_fast8_t x0, uint_fast8_t y0, uint_fast8_t x1, uint_fast8_t y1, tFillmode fillMode, tColor color)
{
    if (x0 > x1)
    {
        uint_fast8_t tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    if (y0 > y1)
    {
        uint_fast8_t tmp = y0;
        y0 = y1;
        y1 = tmp;
    }
    if (fillMode == SOLID)
    {        
        for (uint_fast8_t y = y0; y <= y1; y++)
        {
            draw_line(x0, y, x1, y, color);
        }
    }
    else
    {
        draw_line(x0, y0, x1, y0, color);
        draw_line(x0, y1, x1, y1, color);
        draw_line(x0, y0, x0, y1, color);
        draw_line(x1, y0, x1, y1, color);
    }
}

void OLED::scroll_up(uint_fast8_t num_lines, uint_fast8_t delay_ms)
{
    if (delay_ms == 0)
    {
        uint_fast8_t scroll_pages = num_lines / 8;
        for (uint_fast8_t i = 0; i < pages; i++)
        {
            for (uint_fast8_t x = 0; x < 128; x++)
            {
                uint16_t index = i * 128 + x;
                uint16_t index2 = (i + scroll_pages) * 128 + x;
                if (index2 < bufsize) buffer[index] = buffer[index2];
                else buffer[index] = 0;
            }
        }
        num_lines -= scroll_pages * 8;
    }

    bool need_refresh=true;
    if (num_lines > 0)
    {
        uint16_t start=millis() & 0xFFFF;
        uint16_t target_time=0;
        
        for (uint_fast8_t i = 0; i < num_lines; i++)
        {
            for (uint_fast8_t j = 0; j < pages; j++)
            {
                uint16_t index = j*128;
                uint16_t index2 = index + 128;
                for (uint_fast8_t x = 0; x < 128; x++)
                {
                    uint_fast8_t carry = 0;
                    if (index2 < bufsize && (buffer[index2] & 1)) carry = 128;
                    buffer[index] = (buffer[index] >> 1) | carry;
                    index++;
                    index2++;
                }
            }
            need_refresh=true;
            target_time+=delay_ms;
            
            uint16_t now=millis() & 0xFFFF;
            if (now-start < target_time)
            {
                display();
                need_refresh=false;
            }
                       
            while((millis() & 0xFFFF)-start < target_time) yield();
        }
    }
    
    if (need_refresh) display();
}

size_t OLED::write(uint8_t c)
{
	int n=draw_character(X,Y,c);
	X+=OLED_FONT_WIDTH;
	return n;
}

void OLED::setCursor(uint_fast8_t x, uint_fast8_t y)
{
	if (ttyMode) return;
	X=x;
	Y=y;
}

size_t OLED::printf(uint_fast8_t x, uint_fast8_t y, const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    char temp[64];
    char* buffer = temp;
    size_t len = vsnprintf(temp, sizeof(temp), format, arg);
    va_end(arg);
    if (len > sizeof(temp) - 1) {
        buffer = new char[len + 1];
        if (!buffer) return 0;
        va_start(arg, format);
        vsnprintf(buffer, len + 1, format, arg);
        va_end(arg);
    }
    X=x;
    Y=y;
    len = write((const uint8_t*) buffer, len);
    if (buffer != temp) delete[] buffer;
    return len;
}

size_t OLED::printf(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    char temp[64];
    char* buffer = temp;
    size_t len = vsnprintf(temp, sizeof(temp), format, arg);
    va_end(arg);
    if (len > sizeof(temp) - 1) {
        buffer = new char[len + 1];
        if (!buffer) return 0;
        va_start(arg, format);
        vsnprintf(buffer, len + 1, format, arg);
        va_end(arg);
    }
    len = write((const uint8_t*) buffer, len);
    if (buffer != temp) delete[] buffer;
    return len;
}

size_t OLED::write(const uint8_t *buffer, size_t len)
{
    for (size_t ix=0;ix<len;ix++)
    {
    	if (buffer[ix] == '\r')
		{
    		X=0;
			Y+=(OLED_FONT_HEIGHT);
			if (buffer[ix+1] == '\n') ix++;
		}
		else if (buffer[ix] == '\n')
		{
			X=0;
			Y+=(OLED_FONT_HEIGHT);
			if (buffer[ix+1] == '\r') ix++;
		}
		else if (*(buffer+ix) == '\f')
		{
			scroll_up(64);
			X=0;
			Y=0;
		}
		else
		{
			write(buffer[ix]);
		}

		if (ttyMode)
		{
			if (Y >= 64)
			{
				scroll_up(OLED_FONT_HEIGHT);
				Y=64-OLED_FONT_HEIGHT;
			}
		}
    }
    if (ttyMode) display();
    return len;
}

void OLED::setTTYMode(bool enabled)
{ ttyMode=enabled; }

void OLED::useOffset(bool enabled)
{ if(displayController == CTRL_SH1106) usingOffset=enabled; }

void OLED::drawString(uint_fast8_t col, uint_fast8_t row, const char* s, tFontScaling scaling, tColor color)
{ draw_string(ToX(col),ToY(row),s,scaling,color); }

void OLED::inverse(void)
{ set_font_inverted(true); }

void OLED::noInverse(void)
{ set_font_inverted(false); }

void OLED::set_font_inverted(bool enabled)
{ fontInverted=enabled; }

uint_fast8_t OLED::ToCol(uint_fast8_t x)
{ return(x/OLED_FONT_WIDTH); }

uint_fast8_t OLED::ToRow(uint_fast8_t y)
{ return(y/OLED_FONT_HEIGHT); }

uint_fast8_t OLED::ToX(uint_fast8_t col)
{ return(col*OLED_FONT_WIDTH); }

uint_fast8_t OLED::ToY(uint_fast8_t row)
{ return(row*OLED_FONT_HEIGHT); }
