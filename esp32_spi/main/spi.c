#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"


#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22


//To speed up transfers, every SPI transfer sends a bunch of lines. This define specifies how many. More means more memory use,
//but less overhead for setting up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
#define PIN_NUM_BCKL 5




//LCD Constants

//RS and R/WB Bits
#define DEFAULT 0x00
#define READ_BUSY_FLAG 0x01
#define WRITE_DATA 0x02
#define READ_DATA 0x03

void hello_spi_h()
{
    printf("Hello Spi.h!\n");
}

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    printf("lcd_spi_pre_transfer_callback\n");
    int dc=(int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}


void configure_initialize_SPI_Bus()
{
	esp_err_t ret;
	spi_bus_config_t buscfg = {
				.miso_io_num=PIN_NUM_MISO,
				.mosi_io_num=PIN_NUM_MOSI,
				.sclk_io_num=PIN_NUM_CLK,
				.quadwp_io_num=-1,
				.quadhd_io_num=-1,
				.max_transfer_sz=PARALLEL_LINES*320*2+8
	};

	ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
	ESP_ERROR_CHECK(ret);
}

void configure_initialize_LCD(spi_device_handle_t * spi)
{
	esp_err_t ret;
	spi_device_interface_config_t devcfg = {
				.clock_speed_hz = 10 * 1000,
				.mode=0,                                //SPI mode 0
				.spics_io_num=PIN_NUM_CS,               //CS pin
				.queue_size=7,                          //We want to be able to queue 7 transactions at a time
				.pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
				.command_bits = 2, //Set Command Bits to 10
				.address_bits = 8 //Set Address Bits to 2
	};

	//Attach the LCD to the SPI bus
	ret=spi_bus_add_device(HSPI_HOST, &devcfg, spi);
	ESP_ERROR_CHECK(ret);
}

void lcd_init(spi_device_handle_t spi)
{

	/*
	while(true)
	{
		esp_err_t ret;

		spi_transaction_t t;
		memset(&t, 0, sizeof(t));

		//set cmd and addr field
		t.cmd = 0x03;

		for(uint8_t i = 0; i < 0xFF; i++)
		{
			t.addr = i;
			ret = spi_device_polling_transmit(spi,&t);
			assert(ret == ESP_OK);
			vTaskDelay(10/ portTICK_PERIOD_MS);
		}
	}


	*/


}

void send_command(uint8_t command, uint8_t data, spi_device_handle_t spi)
{
	esp_err_t ret;

	spi_transaction_t t;

	memset(&t,0,sizeof(t));

	t.cmd = command;

	t.addr = data;

	ret = spi_device_polling_transmit(spi, &t);
	assert(ret == ESP_OK);

}

void initial_initialization_display(spi_device_handle_t spi)
{
	//Clear the Display
	send_command(DEFAULT, 0x01,spi);

	vTaskDelay(100/ portTICK_PERIOD_MS);

	send_command(DEFAULT,0x30,spi);
	send_command(DEFAULT,0x13,spi);
	send_command(DEFAULT,0x08,spi);
	send_command(DEFAULT,0x04,spi);
	send_command(DEFAULT,0x14,spi);
	send_command(DEFAULT,0x17,spi);
}

void initialize_display(spi_device_handle_t spi)
{
	//Function set, RS=0,R/W=0
	// 7654 3210
	// 0011 NFFT
	//  N = lines: N=1 is 2 lines
	//  F = Font: 0 = 5x8, 1 = 5x10
	//  FT = Font Table:
	//     FT=00 is English/Japanese ~"standard" for character LCDs
	//     FT=01 is Western European I fractions, circle-c some arrows
	//     FT=10 is English/Russian
	//     FT=11 is Western European II my favorite, arrows, Greek letters
	send_command(DEFAULT, 0x3B,spi);

	//Graphic Vs. Character Mode Setting
	send_command(DEFAULT,0x17,spi);

	//Display On/Off
	send_command(DEFAULT,0x0E,spi);

	//Clear the Display
	send_command(DEFAULT, 0x01,spi);

	vTaskDelay(100/ portTICK_PERIOD_MS);

	//Home Display
	send_command(DEFAULT, 0x02,spi);

	//Entry Mode
	send_command(DEFAULT,0x06,spi);

	//Clear the Display
	send_command(DEFAULT, 0x01,spi);

	vTaskDelay(100/ portTICK_PERIOD_MS);
}

void position_cursor(uint8_t column, uint8_t line, spi_device_handle_t spi)
{
	send_command(DEFAULT,0x80 | (line?0x40:0x00) | (column & 0x3F), spi);
}
void spi_main()
{
	spi_device_handle_t spi;
	configure_initialize_SPI_Bus();
	configure_initialize_LCD(&spi);

	initial_initialization_display(spi);
	initialize_display(spi);

	//Display text screen
    //              0123456789012345
    char line1[17]="Hello World!";
    

    //              0123456789012345
    char line2[17]="TEAM 16 WAS HERE";
    

	position_cursor(0,0, spi);

	for(int col=0;col<16;col++)
      {
		 printf("%c\n",line1[col]);
      	send_command(WRITE_DATA,line1[col],spi);
		  
      }

    // write to the second line
    position_cursor(0,1, spi);
    for(int col=0;col<16;col++)
      {
      send_command(WRITE_DATA,line2[col],spi);
	  
      }

}
