#include "mbed.h"
//#include "2024obatalib/v2024ak_robocon_lib.h"
#include "2026roboconlib_setting.hpp"
#include <cmath>

rob::KKMD motor1(PB_4,PC_8,false); //実際の配置　左前
rob::KKMD motor2(PC_9,PB_8,false); //実際の配置　左後
rob::KKMD motor3(PB_3,PA_10,true); //実際の配置　右後
rob::KKMD motor4(PB_10,PB_5,true); //実際の配置　右前
rob::KKMD motor5(PA_11,PB_2,false);



class Mecanum {
private:
	rob::KKMD** motors;
	float motor_max_power = 0.95f;
    const int MAX_MOTOR = 4; // 静的なメモリ確保のみにするため
	const int MOTOR_COUNT = 4; // 4輪固定

	
	void calculate_outputs(float x, float y, float rotation, float* outputs) {
    // メカナムホイール用の出力計算（4輪前提）
		outputs[0] = y + x + rotation;  // 左前
		outputs[1] = y - x - rotation;  // 右前
		outputs[2] = y - x + rotation;  // 左後
		outputs[3] = y + x - rotation;  // 右後
	}

	void normalize_outputs(float* outputs) {
		// 出力の正規化
		float max_power = 1.0f;
		for (int i = 0; i < MOTOR_COUNT; i++) {
			if(max_power < fabs(outputs[i])) {
				max_power = fabs(outputs[i]);
			}
		}

		// 出力を-0.95から0.95の範囲に正規化
		for (int i = 0; i < MOTOR_COUNT; i++) {
			outputs[i] = (outputs[i] / max_power) * motor_max_power;
		}
		
	}
	
	
public:
	Mecanum(rob::KKMD* (&motor_array)[4]) {
		// for (int i = 1; i < MOTOR_COUNT; i++) {
			// if (motor_array[i] != motor_array[0]) {
				// //std::exit(-1);
				// break;
			// }
		// }
		motors = motor_array;
	}
	
	
	
	void set_motor_outputs(const float* outputs) {
		for (int i = 0; i < MOTOR_COUNT; i++) {
			*motors[i] = outputs[i];
		}
	}
	
	void move(float x, float y, float rotation) {
		// モーター出力を格納する配列
		float outputs[MAX_MOTOR];

		calculate_outputs(x, y, rotation, outputs);

		// 出力の正規化
		normalize_outputs(outputs);

		// モーターに出力を設定
		set_motor_outputs(outputs);
	}
		
};


char direction = 'n';
float lx_v = 0.0f;
float ly_v = 0.0f;
float rx_v = 0.0f;
float ry_v = 0.0f;

	
void getCmc() {
}

float stick_normalize(uint8_t raw_val) {
	const float DEADZONE = 12.0f;
	float value = static_cast<float>(raw_val);
	
	if(value > 255.0f) value = 127.5f;
	
	float out_val = ((value - 127.5f) / 127.5f);
	
	if(fabs(out_val) < (DEADZONE / 127.5f)) out_val = 0.0f;
	
	
	/*
	//0～255の値を-128～128の範囲に直す
	value -= ((STICK_MAX_VALUE - STICK_MIN_VALUE) / 2.0f);
	
	// デッドゾーンの実装
	if (fabs(value) < DEADZONE) value = 0.0f;
	
	return value;
	*/
	return out_val;
}

void getC(const uint8_t *data) {
	direction = data[0];
	
	// lx_v = data[1];
	// ly_v = data[2];
	// rx_v = data[3];
	// ry_v = data[4];
	
	//スティック上に倒すとy方向が減少するようになってるから負の掛け算
	lx_v = stick_normalize(data[1]);
	ly_v = -stick_normalize(data[2]); 
	rx_v = stick_normalize(data[3]);
	ry_v = -stick_normalize(data[4]);
}

void tekito(float pow) {
	
}
rob::KKMD* motors[4] = {&motor1, &motor4, &motor2, &motor3};

Mecanum robo(motors);

mu3 mu3a;

rob::aRotaryEncoder &Encoder1 = rob::rotaryEncoder1;
rob::aRotaryEncoder &Encoder2 = rob::rotaryEncoder2;

int main(){
	pc.baud(9600);
	pc.attach(getCmc,Serial::RxIrq);
	pc.printf("hello\n");
	wait_ms(1000);
	mu3serial.baud(19200);
	mu3a.set(15,110,2,1);//ch,groupID,myNum,sendNum 1から254までで適当に決める。相手と合わせる。
	mu3a.attach(getC);
	
	wait_ms(1000);
	mu3serial.printf("@rc");
	mu3serial.printf("\r\n");
	mu3a.setup();
	

	int mode = 0;
	wait_ms(100);
	Encoder1.reset();
	Encoder2.reset();
	
	Encoder1.read();
	Encoder2.read();
	
	while(true){
		static rob::regularC_ms printTime(100);
		static rob::oneshotC_ms rotate;
		
		if(printTime){
			pc.printf("%c\n\r",direction);
			pc.printf("%f, %f\n\r", rx_v, ry_v);
		}
		
		robo.move(rx_v,ry_v,0);
		
		switch(direction) {
			case 'h':
				if(!rotate) {
					mode++;
					if(mode % 2) {
						motor5 = 0.0;
					} else {
						motor5 = 0.8;
					}
					rotate = 1000;
				}
				
				break;
			default:
				motor1 = 0.0;
				motor2 = 0.0;
				motor3 = 0.0;
				motor4 = 0.0;
				motor5 = 0.0;
				break;
		}
		// switch(direction) {
			// case 'w':
				// motor1 = pow;
				// motor2 = pow;
				// motor3 = -pow;
				// motor4 = -pow;
				// break;
			// case 'a':
				// motor1 = -pow;
				// motor2 = pow;
				// motor3 = pow;
				// motor4 = -pow;
				// break;
			// case 's':
				// motor1 = -pow;
				// motor2 = -pow;
				// motor3 = pow;
				// motor4 = pow;
				// break;
			// case 'd':
				// motor1 = pow;
				// motor2 = -pow;
				// motor3 = -pow;
				// motor4 = pow;
				// break;
			// case 'l':
				// motor1 = -pow;
				// motor2 = -pow;
				// motor3 = -pow;
				// motor4 = -pow;
				// break;
			// case 'r':
				// motor1 = pow;
				// motor2 = pow;
				// motor3 = pow;
				// motor4 = pow;
				// break;
			// case 'n':
				// motor1 = 0.0;
				// motor2 = 0.0;
				// motor3 = 0.0;
				// motor4 = 0.0;
				// break;
			// default:
				// motor1 = 0.0;
				// motor2 = 0.0;
				// motor3 = 0.0;
				// motor4 = 0.0;
				// break;
				
		// }
		
		
	}
	
    return 0;
}