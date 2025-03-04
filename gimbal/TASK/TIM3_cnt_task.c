#include "tim3_cnt_task.h"
#include "chassis_task.h"
#include "gimbal_task.h"
#include "shoot_task.h"
#include "tim.h"
#include "bsp_imu.h"
#include "rc_task.h"
#include "calibrate_task.h"
#include "bsp_uart.h"
#include "sent_task.h"
#include "upper_computer.h"
#include "bsp_buzzer.h"

uint8_t buzzer_flag;
extern VISION_t vision_mode;
extern float vision_yaw, vision_pitch; // 定点位置
int time_count = 0, count_2 = 0, time_count_2 = 0;
int IMU_cnt = 0, start_flag = 0;

extern uint32_t rc_update_cnt;
extern uint32_t rc_last_update_cnt;
extern uint32_t rc_offline_cnt;
extern uint8_t rc_offline_flag;
uint16_t rc_tim_cnt = 0;
uint8_t rc_offline_check(void);

// 0.1ms
int psc = 49;
void prevent_jam(void);
void shake_three(void);
void long_pre_shoot(void);
// 任务汇总
void TIM3_CNT_TASK()
{
	if (htim->Instance == TIM3)
	{
		time_count++;
		upper_computer_reboot();
		if (buzzer_flag == 0) // 上电蜂鸣器
		{
			buzzer_on(psc, 450);
		}
		if (buzzer_flag == 1)
			buzzer_off();
		if (psc >= 89)
			psc = 0;

		if (time_count % 13 == 0 && start_flag == 1)
		{
			Gimbal_Task();
			shoot_task();
			canTX_UPPER_COMPUTER(); // 向上位机发送数据
			receive_upper_data();
			//	 DMA_Send();  旧的上位机通讯
		}
		if (time_count % 7 == 0)
		{
			remote_chassis();
			control_mode_judge();
			if (control_mode == KEY_OFF)
				remote_control_data();
			else
				key_control_data();
			judge_key();
			//    long_pre_shoot();
		}
		if (time_count >= 1000)
		{
			psc += 10;
			canTX_UPPER_COMPUTER_2();
			time_count = 0;
		}
	}
	if (htim->Instance == TIM5)
	{
		// 遥控器离线检测  
		rc_tim_cnt++;
		if (rc_tim_cnt >= 35)
		{
			rc_tim_cnt = 0;
			rc_offline_flag=rc_offline_check();
		}



		count_2++;
		if (IMU_cnt > 5)
		{
			start_flag = 1;
			buzzer_flag = 1;
		}
		INS_task();
		if (count_2 >= 1000)
		{
			count_2 = 0;
			if (start_flag == 0)
				IMU_cnt++;
		}
	}
}

// 遥控器离线检测
// 遥控器有数据发过来时会更新rc_update_cnt
// 当rc_update_cnt和rc_last_update_cnt相等时，说明遥控器可能离线
uint8_t rc_offline_check(void)
{

	if (rc_update_cnt == rc_last_update_cnt)
	{
		rc_offline_cnt++;
	}
	else
	{
		rc_offline_cnt = 0;
	}
	if (rc_offline_cnt >= 10)
	{
		rc_ctrl.rc.ch[0] = 0;
		rc_ctrl.rc.ch[1] = 0;
		rc_ctrl.rc.ch[2] = 0;
		rc_ctrl.rc.ch[3] = 0;
		rc_ctrl.rc.ch[4] = 0;
		rc_ctrl.rc.s[0] = 1;
		rc_ctrl.rc.s[1] = 1;
		rc_ctrl.mouse.x = 0;
		rc_ctrl.mouse.y = 0;
		rc_ctrl.mouse.z = 0;
		rc_ctrl.mouse.press_l = 0;
		rc_ctrl.mouse.press_r = 0;
		rc_ctrl.key.v = 0;

		rc_update_cnt = 0;
		rc_last_update_cnt = 0;
		rc_offline_cnt = 0;
		return 1;
	}

	rc_last_update_cnt = rc_update_cnt;
	return 0;
}

int shake_flag = 0, cnt_, One_shoot_trigger_time_count = 0, Five_shoot_trigger_time_count = 0;
uint8_t shoot_jam_flag = 0;

void prevent_jam(void)
{
	if (One_Shoot_flag == 1) // 单发模式
	{
		if ((abs(rc_shoot.trigger.target_angle - rc_shoot.trigger.total_angle) > (36859 / 2))) // 未达到目标角度
		{
			One_shoot_trigger_time_count++;
			if (One_shoot_trigger_time_count >= 50)
			{
				rc_shoot.trigger.target_angle = rc_shoot.trigger.total_angle;
				One_shoot_trigger_time_count = 0;
			}
		}
		else
			One_shoot_trigger_time_count = 0;
	}
	else if (Ten_Shoot_flag == 1) // 五连发
	{
		if ((abs(rc_shoot.trigger.target_angle - rc_shoot.trigger.total_angle) > (36859 / 2))) // 未达到目标角度
		{
			Five_shoot_trigger_time_count++;
			if (Five_shoot_trigger_time_count >= 250)
			{
				rc_shoot.trigger.target_angle = rc_shoot.trigger.total_angle;
				Five_shoot_trigger_time_count = 0;
			}
		}
		else
			Five_shoot_trigger_time_count = 0;
	}
}

void shake_three(void) // 抖动三下
{
	shake_flag = 1;
	if (shake_flag == 1)
	{
		cnt_++;
		if (cnt_ >= 1000)
		{
			rc_shoot.trigger.target_angle = rc_shoot.trigger.total_angle - 5 / 360 * 36859;
			shake_flag = 2;
			cnt_ = 0;
		}
	}
	if (shake_flag == 2)
	{
		cnt_++;
		if (cnt_ >= 1000)
		{
			rc_shoot.trigger.target_angle = rc_shoot.trigger.total_angle + 5 / 360 * 36859;
			shake_flag = 3;
		}
	}
	if (shake_flag == 3)
	{
		cnt_++;
		if (cnt_ >= 1000)
		{
			rc_shoot.trigger.target_angle = rc_shoot.trigger.total_angle + 5 / 360 * 36859;
			shake_flag = 4;
		}
	}
}

uint16_t shoot_count = 0;
void long_pre_shoot(void)
{
	if (MOUSE_pre_right == 1)
		shoot_count++;
	else
		shoot_count = 0;
	if (shoot_count >= 200)
	{
		if (abs(rc_shoot.trigger.target_angle - rc_shoot.trigger.total_angle) < 500) // 视为这一发子弹已经拨出
			rc_shoot.trigger.target_angle -= ((1.0f * 36.0f * (360.0f / 8.0f)) / 360.0f * 8191.0f);
	}
}
