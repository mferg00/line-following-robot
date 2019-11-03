#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define THRESHOLD 128
#define FULL_SPEED 65
#define HALF_SPEED 35
#define CORRECTION 20

int straight_count = 0;
int curve_count = 0;

uint8_t read_sensor(int id){

		ADMUX = 0b00000000;
		ADCSRB = 0b00000000;

		// set up ADC
		// use internal 2.56V ref, left-adjust result
		ADMUX |= (1<<REFS1)|(1<<REFS0)|(1<<ADLAR);
		// enable adc, 128 pre-scalar
		ADCSRA |= (1<<ADEN)|(1<<ADATE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);


		switch(id) {

	    case 1:
	        // s2 - adc5 - set MUX0 and MUX2 is set to one
	        ADMUX |= (1<<MUX0)|(1<<MUX2);
					break;

	    case 2:
	        // s3 - adc6 - set MUX1 and MUX2 is set to one
	        ADMUX |= (1<<MUX1)|(1<<MUX2);
					break;

	    case 3:
	        // s4 - adc7 - set MUX0, MUX1 and MUX2 is set to one
	        ADMUX |= (1<<MUX0)|(1<<MUX1)|(1<<MUX2);
	    		break;

	   case 4:
	        // s5 - adc11
					ADCSRB |= (1<<MUX5);
	        ADMUX |= (1<<MUX0)|(1<<MUX1);
	    		break;

	   case 5:
	        // s6 - adc10
					ADCSRB |= (1<<MUX5);
	        ADMUX |= (1<<MUX1);
					break;

	   case 6:
	        // s7 - adc9
					ADCSRB |= (1<<MUX5);
	        ADMUX |=  (1<<MUX0);
					break;

		 case 7:
				// s8 - adc8
				ADCSRB |= (1<<MUX5);
				break;

			case 8:
						// s1 - adc4 - ensure MUX2 bit is set to one
				ADMUX |= (1<<MUX2);
				break;

		}

    // start conversion
    ADCSRA |= (1<<ADSC);

    // wait until conversion is complete
    while(~ADCSRA&(1<<ADIF)){}

    // return the value from the adc
    return ADCH;
}

// This code allows both motors to work
void motor_initialised(){

// Motor 1 (16-Bit Timer)
	DDRB |= (1<<5); //OC1A - Initialise as Output
	DDRB |= (1<<6); //OC1B - Initialise as Output

	TCCR1A |= (1<<COM1A1)|(1<<COM1B1)|(1<<COM1C0)|(1<<0x00FF)|1; //TCCR1A - Clear OC1A/OC1B/OC1C on compare match, set OC1A/OC1B/OC1C at TOP, Fast PWM, 8-bit
	TCCR1B |= (1<<CS12); //TCCR0B - Sets the prescaler to 256(at max)
	OCR1A = 50;

// Motor 2 (8-Bit Timer)
	DDRB |= (1<<COM0A1);	//OC0A - Initialise as Output
	DDRD |= (1<<0);			//OC0B - Initialise as Output

	TCCR0A |= (1<<COM0A1)|(1<<COM0B1)|(1<<WGM02)|1; //TCCR0A - Clears OC0A/OC0B on Compare Match, set OC0A/OC0B at TOP; Fast PWM
	TCCR0B |= (1<<CS02); //TCCR0B - Sets the prescaler to 256(at max)
	OCR0A = 50;

	DDRE |= (1<<6); //led0
	DDRB |= (1<<0); //led1
	DDRB |= (1<<1); //led2
	DDRB |= (1<<2); //led3
	DDRB |= (1<<7); //led4

}

void setMotorSpeeds (float MotorSpeeds[2]){

	OCR1A = MotorSpeeds[0];
	OCR0A = MotorSpeeds[1];
	OCR1B = MotorSpeeds[2];
	OCR0B = MotorSpeeds[3];
//	setMotorSpeeds(MotorSpeeds);
}

void ledoff() {
	PORTE &= ~(1<<6);
	PORTB &= ~(1<<0);
	PORTB &= ~(1<<1);
	PORTB &= ~(1<<2);
}

void ledon() {
	PORTE |= (1<<6);
	PORTB |= (1<<0);
	PORTB |= (1<<1);
	PORTB |= (1<<2);
}

int main(){

	motor_initialised();
	float MotorSpeeds[4] = {0};
	uint8_t sensor_out[9] =  {0};

	while(1){

// read a value from each ADC and assign to array using fuction
	sensor_out[1] = read_sensor(1);
	sensor_out[2] = read_sensor(2);
	sensor_out[3] = read_sensor(3);
	sensor_out[4] = read_sensor(4);
	sensor_out[5] = read_sensor(5);
	sensor_out[6] = read_sensor(6);
	sensor_out[7] = read_sensor(7);
	sensor_out[8] = read_sensor(8);
	sensor_out[0] = 0;
/*
	if(sensor_out[6] < THRESHOLD && sensor_out[7] < THRESHOLD && sensor_out[8] < THRESHOLD && sensor_out[5] < THRESHOLD && sensor_out[4] < THRESHOLD && sensor_out[3] < THRESHOLD && sensor_out[2] < THRESHOLD && sensor_out[1] < THRESHOLD){
		ledon();
	}

	if((sensor_out[4] < THRESHOLD && sensor_out[3] < THRESHOLD && sensor_out[2] < THRESHOLD ) || (sensor_out[5] < THRESHOLD && sensor_out[6] < THRESHOLD && sensor_out[7] < THRESHOLD )){
		ledon();
	}
*/
	if (sensor_out[4]<THRESHOLD && sensor_out[5] < THRESHOLD) { //Go straight
		straight_count ++;

		if ((straight_count > 275)) {
			ledon();
		}

		//ledoff();
		MotorSpeeds[0] = FULL_SPEED;
		MotorSpeeds[1] = FULL_SPEED;
		MotorSpeeds[2] = 0;
		MotorSpeeds[3] = 0;
	}

	else if(sensor_out[3] < THRESHOLD && sensor_out[4] < THRESHOLD ){// left motor normal, right motor half pace
		curve_count ++;
		if (curve_count > 75 && straight_count < 100) {
			ledoff();
		}
		//ledoff();
		MotorSpeeds[0] = HALF_SPEED;
		MotorSpeeds[1] = FULL_SPEED+CORRECTION;
		MotorSpeeds[2] = 0;
		MotorSpeeds[3] = 0;
	}

	else if(sensor_out[5] < THRESHOLD && sensor_out[6] < THRESHOLD ){// right motor normal, left motor half
		curve_count ++;
		if (curve_count > 75 && straight_count < 100) {
			ledoff();
		}

		//ledoff();
		MotorSpeeds[0] = FULL_SPEED+CORRECTION;
		MotorSpeeds[1] = HALF_SPEED;
		MotorSpeeds[2] = 0;
		MotorSpeeds[3] = 0;
	}

	else if(sensor_out[3] < THRESHOLD && sensor_out[2] < THRESHOLD ){// left motor normal, right motor off

		ledoff();
		MotorSpeeds[0] = 0;
		MotorSpeeds[1] = FULL_SPEED+CORRECTION;
		MotorSpeeds[2] = 0;
		MotorSpeeds[3] = 0;
	}

		else if((sensor_out[2] < THRESHOLD && sensor_out[1] < THRESHOLD) || sensor_out[1] < THRESHOLD){// left motor normal, right motor off
		curve_count = 0;
		straight_count = 0;
		ledoff();
		MotorSpeeds[0] = 0;
		MotorSpeeds[1] = FULL_SPEED+CORRECTION+20;
		MotorSpeeds[2] = FULL_SPEED+CORRECTION+20;
		MotorSpeeds[3] = 0;
	}
		else if(sensor_out[6] < THRESHOLD && sensor_out[7] < THRESHOLD ){// left motor normal, right motor off

		ledoff();
		MotorSpeeds[0] = FULL_SPEED+CORRECTION;
		MotorSpeeds[1] = 0;
		MotorSpeeds[2] = 0;
		MotorSpeeds[3] = 0;

	}
		else if((sensor_out[8] < THRESHOLD && sensor_out[7] < THRESHOLD) || sensor_out[8] < THRESHOLD){// left motor normal, right motor off
		straight_count = 0;
		curve_count = 0;
		ledoff();


		MotorSpeeds[0] = FULL_SPEED+CORRECTION+20;
		MotorSpeeds[1] = 0;
		MotorSpeeds[2] = 0;
		MotorSpeeds[3] = FULL_SPEED+CORRECTION+20;
	}
/*
	else if(sensor_out[6] < THRESHOLD || sensor_out[7] < THRESHOLD || sensor_out[8] < THRESHOLD){// right motor normal, left motor off

		ledon();
		MotorSpeeds[1] = 0;
		MotorSpeeds[0] = FULL_SPEED+CORRECTION;
	}*/

	else {// stop all motors

		ledoff();
		MotorSpeeds[0] = 0;
		MotorSpeeds[1] = 0;
		MotorSpeeds[2] = 0;
		MotorSpeeds[3] = 0;
	}

setMotorSpeeds(MotorSpeeds);

}


	return 0;
}
