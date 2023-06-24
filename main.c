#include "stm32f10x.h "

// Protótipos das funções do display LCD
void lcd_init ( void ); // Iniciar o display corretamente
void lcd_command ( unsigned char cmd ); // Enviar comandos
void lcd_data ( unsigned char data ); // Envia dados ( caractere ASCII )
void lcd_print ( char * str ); // Envia strings
void lcd_putValue ( unsigned char value ); // Usada internamente
void delay_us (uint16_t t); // Atraso de micro segundos
void delay_ms (uint16_t t); // Atraso de mili segundos

// Protótipos das funções criadas
void verificaTecla(uint32_t switches_GPIOA,uint32_t switches_GPIOB,
									 uint32_t switches_GPIOC);
void executaSom(uint16_t freq);
void convertBCD(uint8_t valor);
void BCDtoASCII(uint8_t valor);
void limpaDisplay(void);
void tocaMusica(void);

// Definição das notas musicais 
#define C 132
#define C_sust 140
#define D 148
#define D_sust 157
#define E 166
#define F 176
#define F_sust 187
#define G 198
#define G_sust 209
#define A 222
#define A_sust 235
#define B 249

// Definição dos pinos das portas do GPIOA
#define SW8 3
#define SW9 4
#define SW14 7

// Definição dos pinos das portas do GPIOB
#define BUZZER 0 
#define POTENCIOMETRO 1
#define SW1 12 
#define SW2 13
#define SW3 14
#define SW4 15
#define SW5 5
#define SW6 4
#define SW7 3
#define SW10 8
#define SW11 9
#define SW12 11
#define SW13 10

// Definição dos pinos das portas do GPIOC
#define SW15 15
#define SW16 14
#define SW17 13

// Definição dos pinos para o display LCD
# define LCD_RS 15
# define LCD_EN 12

// Definição das variáveis globais
// Variável oitava é inicializada com 1
uint8_t oitava = 1; 
// Variável pwm é inicializada com 25
uint8_t pwm = 25; 
// Variável para auxiliar na definição do pwm,
// usada na função verificaTecla
uint8_t aux_pwm = 0;
// Valor do potenciômetro
uint16_t pot = 0;

int main()
{
	// Desativar a interface JTAG
	RCC -> APB2ENR |= RCC_APB2ENR_AFIOEN ;
	AFIO -> MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE ;
	
	RCC->APB2ENR |= 0xFC | (1<<9); // Ativar clocks para as portas do GPIO
	RCC->APB1ENR |= (1<<1); // Ativiar o TIM3 clock
	
	// Configurando as portas do GPIOB
	GPIOB->CRL = 0x3344430B; 
	GPIOB->CRH = 0x44444444;
	
	// Configurando as portas do GPIOA
	GPIOA->CRH = 0x33333333;
	GPIOA->CRL = 0x43344333;
	
	// Configurando as portas do GPIOC
	GPIOC->CRH = 0x44433333;
	
	// Variáveis para as chaves
	uint32_t switches_GPIOA, switches_GPIOB, switches_GPIOC; 
	
	lcd_init(); //Inicia o LCD
	ADC1->CR2 = 1; //Configurar ADON (ligar)
	ADC1->SMPR2 = 1<<3; // configurar SMP1 (001)
	delay_us(1);
	
	lcd_print("Oitava: ");
	convertBCD(oitava);
	
	while (1)
	{
		// Potenciômetro:
		ADC1->SQR3 = 9; //escolhe o canal 9 como input
		ADC1->CR2 = 1;  //ADON = 1 (inicia a conversão)
		while((ADC1->SR & (1<<1)) == 0); // Espera o EOC flag
		pot = ADC1->DR;
		
		switches_GPIOA = ~(GPIOA->IDR); // Lê a entrada nas portas do GPIOA
		switches_GPIOB = ~(GPIOB->IDR); // Lê a entrada nas portas do GPIOB
		switches_GPIOC = ~(GPIOC->IDR); // Lê a entrada nas portas do GPIOC
		
		//Chama a função para verificar a tecla apertada
		verificaTecla(switches_GPIOA, switches_GPIOB, switches_GPIOC);
	}
}

// Função que vai verificar a tecla apertada
void verificaTecla(uint32_t switches_GPIOA,uint32_t switches_GPIOB,
									 uint32_t switches_GPIOC){
										 
	// Variável que será usada para testar se alguma tecla de nota
	// musical foi apertada é definida como 0
	uint8_t testeFlag = 0;
										 
	// Verificando a chave para as oitavas
	// 1ª oitava
	if(switches_GPIOB & (1<<SW1)){
		oitava = 1;
		limpaDisplay();
		lcd_print("Oitava: ");
		convertBCD(oitava);
	}
	// 2ª oitava
	if(switches_GPIOB & (1<<SW2)){
		oitava = 2;
		limpaDisplay();
		lcd_print("Oitava: ");
		convertBCD(oitava);
	}
	// Verificando a chave do PWM e configurando-o
	if(switches_GPIOB & (1<<SW3)){
		aux_pwm++;
		if(aux_pwm > 3)
			aux_pwm = 1;
		pwm = aux_pwm*25;
		limpaDisplay();
		lcd_print("Oitava: ");
		convertBCD(oitava);
		lcd_command(0xC0);
		lcd_print("Ciclo: ");
		convertBCD(pwm);
	}
	
	// Parte extra:
	// Se aperta a chave SW4 é tocado um som
	if(switches_GPIOB & (1<<SW4))
		tocaMusica();
	
	// Verificando as chaves das notas, e chamando a função
	// que irá executar o som. Caso alguma nota tenha sido apertada,
	// a variável testeFlag recebe valor 1
	if(switches_GPIOA & (1<<SW8)){
		executaSom(oitava * F);
		testeFlag = 1;
	}
	if(switches_GPIOA & (1<<SW9)){
		executaSom(oitava * G);
		testeFlag = 1;
	}
	if(switches_GPIOA & (1<<SW14)){
		executaSom(oitava * D_sust);
		testeFlag = 1;
	}
	if(switches_GPIOB & (1<<SW5)){
		executaSom(oitava * C);
		testeFlag = 1;
	}
	if(switches_GPIOB & (1<<SW6)){
		executaSom(oitava * D);
		testeFlag = 1;
	}
	if(switches_GPIOB & (1<<SW7)){
		executaSom(oitava * E);
		testeFlag = 1;
	}
	if(switches_GPIOB & (1<<SW10)){
		executaSom(oitava * A);
		testeFlag = 1;
	}
	if(switches_GPIOB & (1<<SW11)){
		executaSom(oitava * B);
		testeFlag = 1;
	}
	if(switches_GPIOB & (1<<SW12)){
		executaSom(oitava * C);
		testeFlag = 1;
	}
	if(switches_GPIOB & (1<<SW13)){
		executaSom(oitava * C_sust);
		testeFlag = 1;
	}
	if(switches_GPIOC & (1<<SW15)){
		executaSom(oitava * F_sust);
		testeFlag = 1;
	}
	if(switches_GPIOC & (1<<SW16)){
		executaSom(oitava * G_sust);
		testeFlag = 1;
	}
	if(switches_GPIOC & (1<<SW17)){
		executaSom(oitava * A_sust);
		testeFlag = 1;
	}
	
	// Se nenhuma nota for apertada, tem-se essa condição
	if(testeFlag == 0)
		executaSom(1);
}

void executaSom(uint16_t freq){
	// Se nenhuma nota for apertada, não realiza nenhum som
	if(freq == 1){
		TIM3->CCR3 = 0;
	}
	// Caso alguma nota tenha sido apertada, realiza o som de acordo
	// com a frequência da nota e com o valor do potenciometro
	else{
		uint16_t arr = (1000000/(freq + (pot/150)))-1;
		TIM3->CCR3 = ((arr+1)*pwm)/100;
		TIM3->CCER = 0x1 << 8; /* 0 em CC3P, 1 em CC3E */
		TIM3->CCMR2 = 0x0060; /* PWM1 */
		TIM3->PSC = 72-1; // Valor calculado para o prescaler
		TIM3->ARR = arr; // Valor para ser feita a comparação
		TIM3->CR1 = 1;
	}
}

// Função que toca uma música (parte extra)
void tocaMusica(void){
	executaSom(F_sust*oitava); delay_ms(500); executaSom(1); delay_ms(50); 
	executaSom(D*oitava); delay_ms(500); executaSom(1); delay_ms(30); 
	executaSom(D*oitava); delay_ms(100); executaSom(1); delay_ms(30);
	executaSom(E*oitava); delay_ms(200); executaSom(1); delay_ms(30); 
	executaSom(F*oitava); delay_ms(300); executaSom(1); delay_ms(30); 
	executaSom(E*oitava); delay_ms(300); executaSom(1); delay_ms(30);
	executaSom(D*oitava); delay_ms(200); executaSom(1); delay_ms(30);
	executaSom(C_sust*oitava); delay_ms(300); executaSom(1); delay_ms(30);
	executaSom(D*oitava); delay_ms(300); executaSom(1); delay_ms(30);
	executaSom(E*oitava); delay_ms(300); executaSom(1); delay_ms(10);
	executaSom(F_sust*oitava); delay_ms(500); executaSom(1); delay_ms(30);
	executaSom(B*oitava); delay_ms(400); executaSom(1); delay_ms(10);
}

// Função que converte um valor para BCD (usada no display LCD)
void convertBCD(uint8_t valor){
	uint8_t centena;
	uint8_t dezena;
	uint8_t unidade;
	// Realiza a conversão através do resto de divisão
	// e envia o valor convertido para a função BCDtoASCII
	unidade = valor % 10;
	dezena = (valor/10)%10;
	centena = (valor/100)%10;
	BCDtoASCII(centena);
	BCDtoASCII(dezena);
	BCDtoASCII(unidade);
	delay_ms(200);
}

// Função que envia o dígito BCD para o display LCD em ASCII
void BCDtoASCII(uint8_t valor){
	// Envia o ASCII do valor obtido para o lcd
	lcd_data(valor + 0x30);
}

// Função para limpar o display LCD
void limpaDisplay(){
	lcd_command (0x01); // limpa o display
	lcd_command (0x02); // move para a primeira posição
}

void lcd_putValue ( unsigned char value )
{
	uint16_t aux ; // variable to help to build appropriate data out
	aux = 0x0000 ; // clear aux
	GPIOA -> BRR = (1 <<5)|(1 <<6)|(1 <<8)|(1 <<11); // clear data lines
	aux = value & 0xF0 ;
	aux = aux >>4;
	GPIOA -> BSRR = (( aux &0x0008 ) <<8) | (( aux & 0x0004 ) <<3) |
		(( aux &0x0002 ) <<5) | (( aux &0x0001 ) <<8);
	GPIOA -> ODR |= (1 << LCD_EN ); /* EN = 1 for H - to - L pulse */
	delay_ms (3); /* make EN pulse wider */
	GPIOA -> ODR &= ~(1 << LCD_EN ); /* EN = 0 for H - to - L pulse */
	delay_ms (1); /* wait */
	GPIOA -> BRR = (1 <<5)|(1 <<6)|(1 <<8)|(1 <<11); // clear data lines
	aux = 0x0000 ; // clear aux
	aux = value & 0x0F ;
	GPIOA -> BSRR = (( aux &0x0008 ) <<8) | (( aux &0x0004 ) <<3) |
		(( aux &0x0002 ) <<5) | (( aux &0x0001 ) <<8);
	GPIOA -> ODR |= (1 << LCD_EN ); /* EN = 1 for H - to - L pulse */
	delay_ms (3); /* make EN pulse wider */
	GPIOA -> ODR &= ~(1 << LCD_EN ); /* EN = 0 for H - to - L pulse */
	delay_ms (1); /* wait */
}

void lcd_command ( unsigned char cmd )
{
	GPIOA -> ODR &= ~(1 << LCD_RS ); /* RS = 0 for command */
	lcd_putValue ( cmd );
}


void lcd_data ( unsigned char data )
{
	GPIOA->ODR |= (1 << LCD_RS ); /* RS = 1 for data */
	lcd_putValue ( data );
}

void lcd_print ( char * str )
{
	unsigned char i = 0;

	while (str[i] != 0) /* while it is not end of string */
	{
		lcd_data (str [i]); /* show str [ i ] on the LCD */
		i++;
	}
}

void lcd_init ()
{
	delay_ms (15);
	GPIOA -> ODR &= ~(1 << LCD_EN ); /* LCD_EN = 0 */
	delay_ms (3); /* wait 3 ms */
	lcd_command (0x33); // lcd init .
	delay_ms (5);
	lcd_command (0x32); // lcd init .
	delay_us (3000);
	lcd_command (0x28); // 4 - bit mode , 1 line and 5 x8 charactere set
	delay_ms (3);
	lcd_command (0x0e); // display on , cursor on
	delay_ms (3);
	lcd_command (0x01); // display clear
	delay_ms (3);
	lcd_command (0x06); // move right
	delay_ms (3);
}

void delay_us ( uint16_t t )
{
	volatile unsigned long l = 0;
	for ( uint16_t i = 0; i < t ; i ++)
		for ( l = 0; l < 6; l ++)
		{
		}

}

void delay_ms ( uint16_t t )
{
	volatile unsigned long l = 0;
	for ( uint16_t i = 0; i < t ; i ++)
		for ( l = 0; l < 6000; l ++)
		{
		}
}