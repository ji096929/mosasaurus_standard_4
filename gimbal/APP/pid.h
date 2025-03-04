#ifndef __pid_H
#define __pid_H
#include "main.h"
#include "usart.h"
#include <string.h>

typedef struct
{
    //PID 三参数
    float Kp;
    float Ki;
    float Kd;

    float max_out;  //最大输出
    float max_iout; //最大积分输出
		float deadband;
	
    float ref;
    float fdb;
	float lastfdb;

		float interal_seprate; //积分分离项
    float out;
    float Pout;
    float Iout;
    float Dout;
    float Dbuf[3];  //微分项 0最新 1上一次 2上上次
    float error[3]; //误差项 0最新 1上一次 2上上次

} PidTypeDef;


extern void PID_Init(PidTypeDef *pid,  const float PID[5]);
extern float PID_Calc(PidTypeDef *pid, float ref, float set);
extern void PID_clear(PidTypeDef *pid);

#endif
