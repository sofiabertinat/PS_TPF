#include "sapi.h"
#include "arm_math.h"
#include "arm_const_structs.h"


struct header_struct 
{
   char     pre[4];
   uint32_t id;
   uint16_t N;
   uint16_t fs ;
   uint32_t maxIndex;    // indexador de maxima energia por cada fft
   uint32_t maxValue;    // maximo valor de energia del bin por cada fft
   uint32_t maxFrec;
   char     pos[4];
} __attribute__ ((packed));

struct header_struct header={"head",0,64,10000,0,0,0,"tail"};

typedef enum
{
   IMPACT = 0,
   GLASS_FLEXION,
   GLASS_BREAKING
} t_breaking_event;

int main ( void )
{
   uint16_t sample = 0;

   arm_rfft_instance_f32 S;
   arm_cfft_radix4_instance_f32  cS;
   float fftIn[ header.N];        // guarda copia de samples en Q15 como in para la fft. La fft corrompe los datos de la entrada!
   float fftOut[ header.N *2];    // salida de la fft   
   float fftMag[ header.N /2+1 ]; // magnitud de la fft 
   float adc[header.N];     
   float maxValue;
   int cont = 0;
   t_breaking_event event = IMPACT;
   
   boardConfig();
   uartConfig(UART_USB ,460800);
   adcConfig(ADC_ENABLE);
   cyclesCounterInit(EDU_CIAA_NXP_CLOCK_SPEED);

   while(1)
   {
      cyclesCounterReset();
      
      float aux=0;
      int16_t adcRaw;
      
      uartWriteByteArray ( UART_USB ,(uint8_t* )&adc[sample]        ,sizeof(adc[0]));     // envia el sample ANTERIOR
      uartWriteByteArray ( UART_USB ,(uint8_t* )&fftOut[sample*2]   ,sizeof(fftOut[0]));  // envia la fft del sample ANTERIO
      uartWriteByteArray ( UART_USB ,(uint8_t* )&fftOut[sample*2+1] ,sizeof(fftOut[0]));  // envia la fft del sample ANTERIO
      
      adcRaw        = adcRead(CH1)-512;
      adc[sample]   = adcRaw/512.0;            // PISA el sample que se acaba de mandar con una nueva muestra
      fftIn[sample] = adcRaw;                  // copia del adc porque la fft corrompe el arreglo de entrada
      
      if ( ++sample==header.N ) 
      {
         gpioToggle ( LEDR ); // este led blinkea a fs/N
         
         sample = 0;
         
         arm_rfft_init_f32( &S, &cS, header.N, 0, 1 );   // inicializa una estructira que usa la funcion fft para procesar los datos. 
         arm_rfft_f32( &S, fftIn, fftOut);               // ejecuta la rfft REAL fft         
         arm_cmplx_mag_f32(fftOut ,fftMag,header.N/2+1);
         arm_max_f32(fftMag,header.N/2+1 ,&maxValue, &header.maxIndex);
         header.maxValue = (uint32_t)  maxValue;
         header.maxFrec = header.maxIndex*(header.fs/header.N);
         
         switch ( event )
         {             
             case IMPACT:
                  if(header.maxFrec > 2000)
                  {
                     gpioWrite ( LED1 , ON);
                     event = GLASS_FLEXION;
                     cont = 0; 
                  }           
                  else
                  {
                     gpioWrite ( LED1 , OFF);   
                  }
                 break;
             case GLASS_FLEXION:
                  if((header.maxFrec > 20) &&(header.maxFrec < 300))
                  {                     
                     gpioWrite ( LED2 , ON);
                     event = GLASS_BREAKING; 
                     cont = 0;
                  } 
                  else
                  {
                     cont++;
                     if(cont == 2)
                     {
                        cont = 0;
                        event = IMPACT;                     
                        gpioWrite ( LED1 , OFF);
                     }
                  }
                 break;
             case GLASS_BREAKING:
                  if(header.maxFrec > 1300)
                  {
                     gpioToggle ( LED3 );
                     cont = 0;  
                     gpioWrite ( LED1 , OFF);
                     gpioWrite ( LED2 , OFF); 
                     event = IMPACT;
                  } 
                  else
                  {
                     cont++;
                     if(cont == 2)
                     {     
                        gpioWrite ( LED1 , OFF);
                        gpioWrite ( LED2 , OFF); 
                        event = IMPACT;
                     }  
                  }
                 break;
             default:
                 event = IMPACT;
         }
         
         header.id++;
         uartWriteByteArray ( UART_USB ,(uint8_t*)&header ,sizeof(struct header_struct ));
            
         adcRead(CH1); //why?? hay algun efecto minimo en el 1er sample.. puede ser por el blinkeo de los leds o algo que me corre 10 puntos el primer sample. Con esto se resuelve.. habria que investigar el problema en detalle
      }     
      
      while(cyclesCounterRead()< EDU_CIAA_NXP_CLOCK_SPEED/header.fs) // el clk de la CIAA es 204000000
         ;
   }
}
