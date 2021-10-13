/*
 * alessio.c
 *
 * Created: 12/10/2021 08:16:32
 * Author : Aléssio Trajano
 */ 

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include "nokia5110.h"

//Definições de macros para o trabalho com bits
#define tst_bit(y,bit) (y&(1<<bit))//retorna 0 ou 1 conforme leitura do bit

typedef enum{
	Tela_1, Tela_2, Tela_3, Tela_4, Tela_5, Tela_6, Tela_7, size_enumSelet
} enum_selet;

uint8_t verde = 1;
uint8_t amarelo = 1;
uint8_t vermelho = 1;
uint32_t tempo_ms = 0;
uint16_t num_carro = 0;
enum_selet selet_tela = 0;
uint8_t selet_modo = 0;
uint8_t atualiza_tela = 0;
uint8_t flagLUX = 0;
uint16_t lux = 0;
uint8_t pessoa = 0;
uint16_t dados_servo = 0;
uint8_t modo_config = 0;
uint8_t cont_vermelho = 0;
uint8_t cont_verde = 0;

void switch_display (uint32_t tempo);
void LCD_nokia (void);
void leituraLUX(uint8_t *flag_lux);

ISR(TIMER0_COMPA_vect){
	tempo_ms++;  // Acressento 1 milisegundo
	
	if (tempo_ms % 50)
		atualiza_tela = 1;
		
	if (tempo_ms % 300)
		flagLUX = 1;
}

ISR(PCINT1_vect){
	pessoa = !pessoa;
}

ISR(PCINT2_vect){
	/*Como a interrupção externa PCINT2 só possui um endereço na memória de
	programa, é necessário testar qual foi o pino responsável pela interrupção.*/
	
	if(!tst_bit(PIND,PD2)){ // sensor de carros
		static uint32_t carro_anterior = 0;
		num_carro = 60000/((tempo_ms - carro_anterior));
		carro_anterior = tempo_ms;
	}
	
	else if(!tst_bit(PIND,PD4)) {// Botão (+)
		switch (selet_tela){
			case Tela_1:
				selet_modo = !selet_modo;
			break;
			case Tela_2:
				if(verde<9)
					verde++;
				if(verde>=9)
					verde = 9;
			break;
			case Tela_3:
				if(vermelho<9)
					vermelho++;
				if(vermelho>=9)
					vermelho = 9;
			break;
			case Tela_4:
				if(amarelo<9)
					amarelo++;
				if(amarelo>=9)
					amarelo = 9;
			break;
		}
		LCD_nokia ();
		_delay_ms(200);
	}
	else if(!tst_bit(PIND,PD5)) // Botão (-)
	{
		switch (selet_tela){
			case Tela_1:
			if(modo_config>=1)
				modo_config--;
			if(modo_config<=1)
				modo_config = 0;
			break;
			case Tela_2:
			if(verde>1)
				verde--;
			if(verde<=1)
				verde = 1;
			break;
			case Tela_3:
			if(vermelho>1)
				vermelho--;
			if(vermelho<1)
				vermelho = 1;
			break;
			case Tela_4:
			if(amarelo>1)
				amarelo--;
			if(amarelo<=1)
				amarelo = 1;
			break;
		}
		LCD_nokia ();
		_delay_ms(200);
	}
	else if(!tst_bit(PIND,PD6))	{  // Botão de seleção (S)
		static uint8_t flag_tela = 0;
		// Ativar modo configuração 
		switch (selet_tela){
			case Tela_5:
		//	if(modo_config<=3)
				modo_config++;
			if(modo_config >= 3){
				modo_config = 3;
				selet_tela = Tela_1;
			}
			break;
			case Tela_6:
	      // if(modo_config<=3)
				modo_config++;
			if(modo_config >= 3){
				modo_config = 3;
				selet_tela = Tela_1;
			}
			break;
			case Tela_7:
			// if(modo_config<=3)
				modo_config++;
			if(modo_config >= 3){
				modo_config = 3;
				selet_tela = Tela_1;
			}
			break;
			default:
				// Selecionar configuração normal
				if (selet_modo)
				{
					if (flag_tela)
					{
						if (selet_tela < (size_enumSelet-4))
							selet_tela++;
						else
							selet_tela = 0;
					}
					flag_tela = !flag_tela;
				}
			break;
		}
		LCD_nokia ();
		_delay_ms(200);
	}
}

int main(void){
	// Config. GPIO
	DDRB = 0b11111111; // Habilita os pinos PB0 ao PB7 todos como saida
	DDRD = 0b10001000; // Habilita o pino PD7 E PD3 como saidas
	PORTD = 0b01110100; // Habilita o resistor de pull up dos pinos PD2, PD4, PD5 e PD6
	DDRC = 0b0100001; // Habilita os pinos PC6 e PC0 como saida
	PORTC = 0b0100000; // Habilita o resistor de pull up do pino PC6
	
	//Configurações das interrupções
	PCICR  = 0b00000110;
	PCMSK1 = 0b01000000;
	PCMSK2 = 0b01110100;
		
	// Configuração da TCT
	TCCR0A = 0b00000010; // habilita o CTC
	TCCR0B = 0b00000011; // Liga TC0 com prescaler = 64
	OCR0A  = 249;        // ajusta o comparador para TC0 contar até 249
	TIMSK0 = 0b00000010; // habilita a interrupção na igualdade de comparação com OCR0A. A interupção ocorre a cada 1ms = (65*(249+1))/16MHz

	//Configura ADC
	ADMUX = 0b01000000; // Vcc com referencia canal PC0
	ADCSRA= 0b11100111; // Habilita o AD, habilita interrupção, modo de conversão continua, prescaler = 128
	ADCSRB= 0b00000000; // Modo de conversão contínua
	DIDR0 = 0b00000000; // habilita pino PC0 e PC1 como entrada digitais
	
 	//CONFIGURAÇÃO PWM
 	TCCR2A = 0b10100011;
 	TCCR2B = 0b00000011;
 	OCR2B = 0;

	sei(); //Habilita interrupção globais, ativando o bit I do SREG
	
	nokia_lcd_init();
	
	cont_vermelho = vermelho;
	cont_verde = verde;
	
	while (1){
		
		switch_display (tempo_ms); // Leva os valores do tempo de cada led para ser atualizado
		LCD_nokia ();
		leituraLUX (&flagLUX);		
	}

}

void switch_display (uint32_t tempo){  // Atualiza os tempo dos leds

	const uint16_t estados[9] = {0b000001111, 0b000000111, 0b000000011, 0b000000001, 0b100000000, 0b011110000, 0b001110000, 0b000110000, 0b000010000};
	static int8_t i = 0;
	static uint32_t tempo_anterior_ms = 0;
	
	PORTB = estados[i] & 0b011111111;
	
	if (estados[i] & 0b100000000) {PORTD |= (1<<7);}
	else PORTD &= 0b01111111;
	
	// Rotina leds verde 
	if (i<=3)
	{
		//apagar barra de LED VERDE
		if ((tempo - tempo_anterior_ms) >= (verde*250))	{i++; tempo_anterior_ms += (verde*250);}
		// teste de tela 
		if(modo_config == 0){selet_tela = Tela_7;}	
		// trecho para contar tempo na tela.
		if ((tempo - tempo_anterior_ms) == 1000) {cont_verde--; LCD_nokia ();}// contar regressivamente(ERRO)
	}
	else
	{
	 //Rotina leds amarelo 
		if(i<=4)
		{
			// atualiza valores de tempo
			cont_verde = verde;
			cont_vermelho = vermelho;
			
 			if(modo_config == 0) selet_tela = Tela_6; 
			if((tempo - tempo_anterior_ms) >= (amarelo*1000)) {i++;	tempo_anterior_ms += (amarelo*1000);}
		}
		else
		{
		//  Rotina leds vermelho
			if(i<=8)
			{
				//trecho para apagra barra de LED VERMELHO
				if((tempo - tempo_anterior_ms) <= (vermelho*250)){ i++;	tempo_anterior_ms += (vermelho*250);}
				if(modo_config == 0) selet_tela = Tela_5;
				if((tempo - tempo_anterior_ms) ==1000){cont_vermelho--; LCD_nokia ();}	// 	contagem regressiva (Erro)
			}
			else {i=0; tempo_anterior_ms = tempo;}
		}
	}
}

void LCD_nokia (void) // Atualiza o display de acordo com o tempo de cada led
{ 
	unsigned char verde_strig[3];
	unsigned char verme_strig[3];
	unsigned char amare_strig[3];
	unsigned char num_carro_strig[5];
	unsigned char inten_lux[4];
	static unsigned char modo_string;
	unsigned char tempo_verde[3];
	unsigned char tempo_verme[3];
	
	if (selet_modo)
		modo_string = 'M';
	else
	{
		modo_string = 'A';
		verde = 9 - (num_carro/60);
		vermelho = (num_carro/60) + 1;
	}
	
	sprintf (&verde_strig, "%u", verde);
	sprintf (&verme_strig, "%u", vermelho); // Converte as variaveis inteiras em char para imprimir no display
	sprintf (&amare_strig, "%u", amarelo);
	sprintf (&num_carro_strig, "%u", num_carro);
	sprintf (&inten_lux, "%u", lux);
	sprintf (&tempo_verde, "%u", cont_verde);
	sprintf (&tempo_verme, "%u", cont_vermelho);
	
	if(modo_config == 3){
		switch(selet_tela){
			case Tela_1:
				nokia_lcd_clear();
				nokia_lcd_draw_Hline(0,47,40); //(Y_inicial, X, Y_final)
			
				nokia_lcd_set_cursor(0, 5);
				nokia_lcd_write_string("Modo", 1);
				nokia_lcd_set_cursor(30, 5);
				nokia_lcd_write_char(modo_string, 1);
				nokia_lcd_set_cursor(42, 5);
				nokia_lcd_write_string("<", 1);
			
				nokia_lcd_set_cursor(0, 15);
				nokia_lcd_write_string("T.Vd", 1);
				nokia_lcd_set_cursor(30,15);
				nokia_lcd_write_string(verde_strig, 1);
				nokia_lcd_set_cursor(35,15);
				nokia_lcd_write_string("s", 1);
			
				nokia_lcd_set_cursor(50,0);
				nokia_lcd_write_string(inten_lux, 2);
				nokia_lcd_set_cursor(50,15);
				nokia_lcd_write_string("lux", 1);
			
				nokia_lcd_set_cursor(50,25);
				nokia_lcd_write_string(num_carro_strig, 2);
				nokia_lcd_set_cursor(50,40);
				nokia_lcd_write_string("c/min", 1);
			
				nokia_lcd_set_cursor(0, 25);
				nokia_lcd_write_string("T.Vm", 1);
				nokia_lcd_set_cursor(30,25);
				nokia_lcd_write_string(verme_strig, 1);
				nokia_lcd_set_cursor(35,25);
				nokia_lcd_write_string("s", 1);
			
				nokia_lcd_set_cursor(0,35);
				nokia_lcd_write_string("T.Am", 1);
				nokia_lcd_set_cursor(30,35);
				nokia_lcd_write_string(amare_strig, 1);
				nokia_lcd_set_cursor(35,35);
				nokia_lcd_write_string("s", 1);
			
				nokia_lcd_render();
				break;
			
				case Tela_2:
				nokia_lcd_clear();
				nokia_lcd_draw_Hline(0,47,40); //(Y_inicial, X, Y_final)
			
				nokia_lcd_set_cursor(0, 5);
				nokia_lcd_write_string("Modo", 1);
				nokia_lcd_set_cursor(30, 5);
				nokia_lcd_write_char(modo_string, 1);
			
				nokia_lcd_set_cursor(0, 15);
				nokia_lcd_write_string("T.Vd", 1);
				nokia_lcd_set_cursor(30,15);
				nokia_lcd_write_string(verde_strig, 1);
				nokia_lcd_set_cursor(35,15);
				nokia_lcd_write_string("s", 1);
				nokia_lcd_set_cursor(42, 15);
				nokia_lcd_write_string("<", 1);
			
				nokia_lcd_set_cursor(50,0);
				nokia_lcd_write_string(inten_lux, 2);
				nokia_lcd_set_cursor(50,15);
				nokia_lcd_write_string("lux", 1);
			
				nokia_lcd_set_cursor(50,25);
				nokia_lcd_write_string(num_carro_strig, 2);
				nokia_lcd_set_cursor(50,40);
				nokia_lcd_write_string("c/min", 1);
			
				nokia_lcd_set_cursor(0, 25);
				nokia_lcd_write_string("T.Vm", 1);
				nokia_lcd_set_cursor(30,25);
				nokia_lcd_write_string(verme_strig, 1);
				nokia_lcd_set_cursor(35,25);
				nokia_lcd_write_string("s", 1);
			
				nokia_lcd_set_cursor(0,35);
				nokia_lcd_write_string("T.Am", 1);
				nokia_lcd_set_cursor(30,35);
				nokia_lcd_write_string(amare_strig, 1);
				nokia_lcd_set_cursor(35,35);
				nokia_lcd_write_string("s", 1);
			
				nokia_lcd_render();
				break;
			
				case Tela_3:
				nokia_lcd_clear();
				nokia_lcd_draw_Hline(0,47,40); //(Y_inicial, X, Y_final)
			
				nokia_lcd_set_cursor(0, 5);
				nokia_lcd_write_string("Modo", 1);
				nokia_lcd_set_cursor(30, 5);
				nokia_lcd_write_char(modo_string, 1);

				nokia_lcd_set_cursor(0, 15);
				nokia_lcd_write_string("T.Vd", 1);
				nokia_lcd_set_cursor(30,15);
				nokia_lcd_write_string(verde_strig, 1);
				nokia_lcd_set_cursor(35,15);
				nokia_lcd_write_string("s", 1);
			
				nokia_lcd_set_cursor(50,0);
				nokia_lcd_write_string(inten_lux, 2);
				nokia_lcd_set_cursor(50,15);
				nokia_lcd_write_string("lux", 1);
			
				nokia_lcd_set_cursor(50,25);
				nokia_lcd_write_string(num_carro_strig, 2);
				nokia_lcd_set_cursor(50,40);
				nokia_lcd_write_string("c/min", 1);
			
				nokia_lcd_set_cursor(0, 25);
				nokia_lcd_write_string("T.Vm", 1);
				nokia_lcd_set_cursor(30,25);
				nokia_lcd_write_string(verme_strig, 1);
				nokia_lcd_set_cursor(35,25);
				nokia_lcd_write_string("s", 1);
				nokia_lcd_set_cursor(42, 25);
				nokia_lcd_write_string("<", 1);
			
				nokia_lcd_set_cursor(0,35);
				nokia_lcd_write_string("T.Am", 1);
				nokia_lcd_set_cursor(30,35);
				nokia_lcd_write_string(amare_strig, 1);
				nokia_lcd_set_cursor(35,35);
				nokia_lcd_write_string("s", 1);
			
				nokia_lcd_render();
				break;
			
				case Tela_4:
				nokia_lcd_clear();
				nokia_lcd_draw_Hline(0,47,40); //(Y_inicial, X, Y_final)
			
				nokia_lcd_set_cursor(0, 5);
				nokia_lcd_write_string("Modo", 1);
				nokia_lcd_set_cursor(30, 5);
				nokia_lcd_write_char(modo_string, 1);
			
				nokia_lcd_set_cursor(0, 15);
				nokia_lcd_write_string("T.Vd", 1);
				nokia_lcd_set_cursor(30,15);
				nokia_lcd_write_string(verde_strig, 1);
				nokia_lcd_set_cursor(35,15);
				nokia_lcd_write_string("s", 1);
			
				nokia_lcd_set_cursor(50,0);
				nokia_lcd_write_string(inten_lux, 2);
				nokia_lcd_set_cursor(50,15);
				nokia_lcd_write_string("lux", 1);
			
				nokia_lcd_set_cursor(50,25);
				nokia_lcd_write_string(num_carro_strig, 2);
				nokia_lcd_set_cursor(50,40);
				nokia_lcd_write_string("c/min", 1);
			
				nokia_lcd_set_cursor(0, 25);
				nokia_lcd_write_string("T.Vm", 1);
				nokia_lcd_set_cursor(30,25);
				nokia_lcd_write_string(verme_strig, 1);
				nokia_lcd_set_cursor(35,25);
				nokia_lcd_write_string("s", 1);
			
				nokia_lcd_set_cursor(0,35);
				nokia_lcd_write_string("T.Am", 1);
				nokia_lcd_set_cursor(30,35);
				nokia_lcd_write_string(amare_strig, 1);
				nokia_lcd_set_cursor(35,35);
				nokia_lcd_write_string("s", 1);
				nokia_lcd_set_cursor(42, 35);
				nokia_lcd_write_string("<", 1);
			
				nokia_lcd_render();
				break;
			}
		}
		else {
			switch(selet_tela){
				case Tela_5:
				nokia_lcd_clear();
				
				nokia_lcd_set_cursor(0,0);
				nokia_lcd_write_string(" CARRO PARE!", 1);
				nokia_lcd_set_cursor(0,15);
				nokia_lcd_write_string("PEDESTRE SIGA!", 1);
				nokia_lcd_set_cursor(10,25);
				nokia_lcd_write_string(tempo_verme, 3);
				nokia_lcd_set_cursor(50,30);
				nokia_lcd_write_string("s", 2);
				
				nokia_lcd_render();
				break;

				case Tela_6:
				nokia_lcd_clear();
				
				nokia_lcd_set_cursor(0,5);
				nokia_lcd_write_string("ATENCAO", 2);
				nokia_lcd_set_cursor(40,25);
				nokia_lcd_write_string("!", 2);
				
				nokia_lcd_render();
				break;
				case Tela_7:
				nokia_lcd_clear();
				
				nokia_lcd_set_cursor(0,0);
				nokia_lcd_write_string("CARRO SIGA!", 1);
				nokia_lcd_set_cursor(0,15);
				nokia_lcd_write_string("PEDESTRE PARE!",1);
				nokia_lcd_set_cursor(10,25);
				nokia_lcd_write_string(tempo_verde, 3);
				nokia_lcd_set_cursor(50,30);
				nokia_lcd_write_string("s", 2);
				
				nokia_lcd_render();
				break;
		}
	}
}

void leituraLUX(uint8_t *flag_lux)
{
	static float testelux = 0;
	
	if(*flag_lux)
	{
		testelux = ADC;
		lux = (1023000/testelux)-1000;
		
		if (lux < 300)
		{
			if(pessoa == 1 || num_carro > 0)
			OCR2B = 255;
			else if(pessoa == 0 || num_carro <= 0)
			OCR2B = 77;
		}
		else
		OCR2B = 0;
	}
	*flag_lux = 0;
}


