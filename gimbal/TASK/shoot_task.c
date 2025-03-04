#include "shoot_task.h"
#include "remote_control.h"
#include "sent_task.h"
#include "gimbal_task.h"
#include "can_receive.h"
#include "bsp_math.h"
#include <math.h>
uint8_t data[2] = {1, 0};
shoot_task_t rc_shoot;
FRIC_SPEED fricspeed = FRIC_MIN;
extern float Ren;
int shoot_times = 0;
int One_Shoot_flag = 0;
int Ten_Shoot_flag = 0;
int16_t SHOOT_LEFT_FRIC_SPEED_MAX = -7000;
int16_t SHOOT_LEFT_FRIC_SPEED_MIN = -7000;
int16_t SHOOT_RIGHT_FRIC_SPEED_MAX = 7000;
int16_t SHOOT_RIGHT_FRIC_SPEED_MIN = 7000;
extern float shoot_speed;
extern uint8_t speed_limit;
float TriggerSpeed[6] = {3.0f, 0.01f, 0.0f, 8000, 500, 0}; // 拨弹轮速度环 8000 4.8

float TriggerAngle[6] = {0.25f, 0.0000f, 0.00f, 5000, 0, 0}; // 拨弹轮角度环 1700

float FricLeftSpeed[6] = {13.0f, 0.0f, 0.0f, 10000.0f, 0.0f, 0}; // 左摩擦轮速度环

float FricRightSpeed[6] = {13.0f, 0.0f, 0.0f, 10000.0f, 0.0f, 0}; // 右摩擦轮速度环

static float abs_f(float a);

void trigger_angle_set(void);
// 更新发子弹个数
static void shoot_angle_clc(void);
// 防卡弹
// static void prevent_jam(void);
// 摩擦轮PID计算
static void fric_pid(void);
// 波弹轮PID计算
static void trigger_pid(void);
// 摩擦轮射速控制
void fric_speed_control(void);

int trigger_back_flag, trigger_back_flag_cnt = 0;
float speed_1;
float speed_2;
// 发射任务
void shoot_task(void)
{
	//	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET); //关掉激光
	if (trigger_cnt_flag == 1)
		trigger_cnt++;

	fric_speed_control();
	fric_pid();

	if ((rc_shoot.left_fric.actual_speed < -4000) && (rc_shoot.right_fric.actual_speed > 4000)) // 等待摩擦轮转动之后才能拨弹
		shoot_angle_clc();

	trigger_pid();

	canTX_fric(rc_shoot.left_fric.set_currunt, rc_shoot.right_fric.set_currunt, rc_shoot.trigger.set_currunt);
}

float speed_average;
int yuyuyu_flag = 65;
uint16_t count;
void fric_speed_control(void)
{
	if (speed_limit == 30)
	{
		fricspeed = FRIC_MAX;
		//		rc_shoot.left_fric.target_speed =  SHOOT_LEFT_FRIC_SPEED_MAX;
		//		rc_shoot.right_fric.target_speed = SHOOT_RIGHT_FRIC_SPEED_MAX;
	}
	else
	{
		fricspeed = FRIC_MIN;
		//		rc_shoot.left_fric.target_speed = SHOOT_LEFT_FRIC_SPEED_MIN;
		//    rc_shoot.right_fric.target_speed = SHOOT_RIGHT_FRIC_SPEED_MIN;
	}
	//
	//  if(speed_change_flag)
	//  {
	//	  //平均射速
	//	 speed_average=((shoot_speed+speed_average*((float)count))/((float)(count+1.0f)));
	//	  count++;
	//	  if(count>30)
	//	  {
	//		  if(fricspeed==FRIC_MIN) //15
	//			{
	//
	//				   if((speed_limit-speed_average<0.3f) && (speed_limit>=10.0f))
	//					{
	//							SHOOT_LEFT_FRIC_SPEED_MIN+=45;
	//							SHOOT_RIGHT_FRIC_SPEED_MIN-=45;
	//					}
	//				   else if(speed_limit-speed_average>1.0f && (speed_limit>=10.0f))
	//					{
	//							SHOOT_LEFT_FRIC_SPEED_MIN-=20;
	//							SHOOT_RIGHT_FRIC_SPEED_MIN+=20;
	//					}
	//			}
	//			else if(fricspeed==FRIC_MAX)  //30
	//			{
	//				   if((speed_limit-speed_average<0.75f) && (speed_limit>=10.0f))
	//					{
	//							SHOOT_LEFT_FRIC_SPEED_MAX+=40;
	//							SHOOT_RIGHT_FRIC_SPEED_MAX-=40;
	//					}
	//				   else if((speed_limit-speed_average>1.3f) && (speed_limit>=10.0f))
	//					{
	//							SHOOT_LEFT_FRIC_SPEED_MAX-=25;
	//							SHOOT_RIGHT_FRIC_SPEED_MAX+=25;
	//					}
	//			}
	//		}
	//	  speed_change_flag=0;
	//  }
	//	if(fricspeed==FRIC_MIN)
	//	{
	//		rc_shoot.left_fric.target_speed = SHOOT_LEFT_FRIC_SPEED_MIN;
	//	  rc_shoot.right_fric.target_speed = SHOOT_RIGHT_FRIC_SPEED_MIN;
	//	}
	//	else
	//	{
	//		rc_shoot.left_fric.target_speed =  SHOOT_LEFT_FRIC_SPEED_MAX;
	//	  rc_shoot.right_fric.target_speed = SHOOT_RIGHT_FRIC_SPEED_MAX;
	//	}
	// 读取到裁判系统射速限制  还有更多限制阶段超级对抗赛再说
}

// 拨弹轮和摩擦轮的初始化
void shoot_init(void)
{

	PID_Init(&rc_shoot.trigger.speed_pid, TriggerSpeed);
	PID_Init(&rc_shoot.trigger.angle_pid, TriggerAngle);

	PID_Init(&rc_shoot.left_fric.speed_pid, FricLeftSpeed);
	PID_Init(&rc_shoot.right_fric.speed_pid, FricRightSpeed);

	rc_shoot.trigger.actual_angle = 0.0f;
	rc_shoot.trigger.actual_speed = 0.0f;

	rc_shoot.trigger.last_shoot_flag = 0;
	rc_shoot.trigger.last_back_flag = 0;
	rc_shoot.trigger.target_angle = 0.0f;
	rc_shoot.trigger.target_speed = 0.0f;

	rc_shoot.left_fric.actual_speed = 0.0f;
	rc_shoot.left_fric.target_speed = 0;

	rc_shoot.right_fric.actual_speed = 0.0f;
	rc_shoot.right_fric.target_speed = 0;
}

static float abs_f(float a)
{
	if (a < 0)
		a = -1.0f * a;
	return a;
}

int trigger_round;

static void shoot_angle_clc(void)
{
	if (rc_shoot.trigger.rounds <= -90)
	{
		rc_shoot.trigger.target_angle -= (float)(rc_shoot.trigger.rounds) * 8191.0f;
		if (rc_shoot.trigger.target_angle > 0)
			rc_shoot.trigger.target_angle = 0;
		rc_shoot.trigger.total_angle -= (float)(rc_shoot.trigger.rounds) * 8191.0f;
		rc_shoot.trigger.begin_angle = rc_shoot.trigger.total_angle;
		rc_shoot.trigger.rounds = 0;
	}

	if (rc_shoot.trigger.last_shoot_flag == 0)
	{
		if (One_Shoot_flag == 1 || Ten_Shoot_flag == 1)
		{
			rc_shoot.trigger.last_shoot_flag = 1;
			trigger_angle_set();
			shoot_times += 1;
			if (shoot_times > 6)
				shoot_times = 0;
		}
	}

	if (One_Shoot_flag != 1 && Ten_Shoot_flag != 1)
		rc_shoot.trigger.last_shoot_flag = 0;
}
static void fric_pid(void)
{

	PID_Calc(&rc_shoot.left_fric.speed_pid, rc_shoot.left_fric.target_speed, rc_shoot.left_fric.actual_speed);

	rc_shoot.left_fric.set_currunt = rc_shoot.left_fric.speed_pid.out;

	PID_Calc(&rc_shoot.right_fric.speed_pid, rc_shoot.right_fric.target_speed, rc_shoot.right_fric.actual_speed);

	rc_shoot.right_fric.set_currunt = rc_shoot.right_fric.speed_pid.out;
}

float Ren;
static void trigger_pid(void)
{
	//	rc_shoot.trigger.target_angle=Ren;    //用于测试速度环
	PID_Calc(&rc_shoot.trigger.angle_pid, rc_shoot.trigger.target_angle, rc_shoot.trigger.total_angle);

	rc_shoot.trigger.target_speed = rc_shoot.trigger.angle_pid.out;
	//	rc_shoot.trigger.target_speed=Ren;    //用于测试角度环
	PID_Calc(&rc_shoot.trigger.speed_pid, rc_shoot.trigger.target_speed, rc_shoot.trigger.actual_speed);

	rc_shoot.trigger.set_currunt = rc_shoot.trigger.speed_pid.out;
}
int trigger_cnt = 0, trigger_cnt_flag = 0, remain_bullet = 0;
void trigger_angle_set(void)
{
	trigger_cnt_flag = 1;
	remain_bullet = ((heat_limit - heat) / 10) - 2; // 计算热量限制下的剩余弹量
	if (remain_bullet < 0)
		remain_bullet = 0;
	if (One_Shoot_flag == 1) // 单发
	{
		if (fabs(rc_shoot.trigger.target_angle - rc_shoot.trigger.total_angle) < ((1.0f * 36.0f * (360.0f / 8.0f)) / 360.0f * 8191.0f))
		{
			if (remain_bullet >= 1)
				rc_shoot.trigger.target_angle -= ((1.0f * 36.0f * (360.0f / 8.0f)) / 360.0f * 8191.0f);
		}
	}
	else // 五连发
	{
		if (fabs(rc_shoot.trigger.target_angle - rc_shoot.trigger.total_angle) < ((1.0f * 36.0f * (360.0f / 8.0f)) / 360.0f * 8191.0f))
		{
			if (remain_bullet >= 5)
				rc_shoot.trigger.target_angle -= ((5.0f * 36.0f * (360.0f / 8.0f)) / 360.0f * 8191.0f); // 改成五连发了
			else
				rc_shoot.trigger.target_angle -= ((remain_bullet * 36.0f * (360.0f / 8.0f)) / 360.0f * 8191.0f);
		}
	}
}
// 拨弹轮数据返回
void Trigger_Motor_Callback(trigger_t *motor, uint16_t angle, int16_t speed)
{
	motor->last_angle = motor->actual_angle;
	motor->actual_angle = angle;
	motor->actual_speed = 0.5 * (speed + motor->last_speed);
	motor->last_speed = speed;

	if (motor->record_begin_angle_status == 0)
	{
		motor->begin_angle = angle;
		motor->record_begin_angle_status++;
	}
	if (motor->actual_angle - motor->last_angle > 4096)
		motor->rounds--;
	else if (motor->actual_angle - motor->last_angle < -4096)
		motor->rounds++;
	motor->total_angle = motor->rounds * 8192 + motor->actual_angle;
}
