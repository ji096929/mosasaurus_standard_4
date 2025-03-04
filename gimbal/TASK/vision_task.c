#include "vision_task.h"
#include "remote_control.h"
#include "gimbal_task.h"
#include "bsp_math.h"
#include "chassis_task.h"
#include <math.h>
VISION_t vision_mode = VISION_OFF;
VISION_GET_t vision_sent;
UPPER_COMPUTER_VISION_t shoot_vision_mode = VISION_NOMAL; // 上位机自瞄模式

float chassis_angle = 0;				// 底盘速度相对于云台参考系的夹角
float vision_x = 0, vision_y = 0;		// 底盘速度在云台坐标系下的速度
float vision_yaw = 0, vision_pitch = 0; // 定点位置
int vision_cnt = 0;
extern uint8_t traget_exit_flag;
extern uint8_t auto_exposure_flag;
extern uint8_t reboot_flag;

int vision_delay(int vt)
{
	vision_cnt++;
	if (vision_cnt > vt)
	{
		vision_cnt = 0;
		return 1;
	}
	else
		return 0;
}
uint8_t vision_check_flag = 0; // 用于防止自瞄操作权切换时出现问题
float last_yaw_target_angle = 0, last_pitch_target_angle = 0;
float yaw_deadline_angle = 0.01f, pitch_deadline_angle = 0.01; // 死区

void Vision_Task(void)
{
	if (vision_mode == VISION_OFF) // 自瞄模式关
	{

		if (gimbal_y.gimbal_motor_mode == GIMBAL_MOTOR_RAW)
		{
			gimbal_y.target_speed = ((rc_sent.yaw.target_angle) / 5);
		}
		if (gimbal_y.gimbal_motor_mode == GIMBAL_MOTOR_ENCONDE)
		{
			gimbal_y.add_angle = (8192.0f / 360.0f) * (rc_sent.yaw.target_angle);
		}
		if (gimbal_y.gimbal_motor_mode == GIMBAL_MOTOR_GYRO)
		{
			gimbal_y.add_angle = rc_sent.yaw.target_angle;
		}

		//		gimbal_y.add_angle=rc_sent.yaw.target_angle;
		gimbal_p.add_angle = rc_sent.pitch.target_angle;
		//		gimbal_y.target_speed=2*(rc_sent.yaw.target_angle);
		//		gimbal_y.target_speed=((rc_sent.yaw.target_angle)/4.5);
		//		gimbal_y.target_angle=Ren;
	}
	else if (vision_mode == VISION_ON) // 自瞄模式开
	{
		if (traget_exit_flag == 1) // 视觉开启操控权在上位机
		{
			vision_check_flag = 1;
			if (vision_sent.yaw.target_angle - last_yaw_target_angle > yaw_deadline_angle || vision_sent.yaw.target_angle - last_yaw_target_angle < -yaw_deadline_angle) // 死区
			{
				//			gimbal_y.add_angle=vision_sent.yaw.target_angle;
				//			last_yaw_target_angle=vision_sent.yaw.target_angle;
				//			gimbal_y.add_angle=(vision_sent.yaw.target_angle-imu_can_error_y);
				//			last_yaw_target_angle=(vision_sent.yaw.target_angle-imu_can_error_y);
				if (gimbal_y.gimbal_motor_mode == GIMBAL_MOTOR_ENCONDE)
				{
					gimbal_y.add_angle = (8192.0f / 360.0f) * (vision_sent.yaw.target_angle - imu_can_error_y);
					last_yaw_target_angle = (8192.0f / 360.0f) * (vision_sent.yaw.target_angle - imu_can_error_y);
				}
				if (gimbal_y.gimbal_motor_mode == GIMBAL_MOTOR_GYRO)
				{
					gimbal_y.add_angle = vision_sent.yaw.target_angle;
					last_yaw_target_angle = vision_sent.yaw.target_angle;
				}
			}
			if (vision_sent.pitch.target_angle - last_pitch_target_angle > pitch_deadline_angle || vision_sent.pitch.target_angle - last_pitch_target_angle < -pitch_deadline_angle)
			{
				gimbal_p.add_angle = vision_sent.pitch.target_angle;
				last_pitch_target_angle = vision_sent.pitch.target_angle;
			}
		}
		else // 视觉开启但是操控权在下位机
		{
			vision_check_flag = 0;
			if (gimbal_y.gimbal_motor_mode == GIMBAL_MOTOR_RAW)
			{
				//				gimbal_y.target_speed=((rc_sent.yaw.target_angle)/5);
			}
			if (gimbal_y.gimbal_motor_mode == GIMBAL_MOTOR_ENCONDE)
			{
				gimbal_y.add_angle = (8192.0f / 360.0f) * (rc_sent.yaw.target_angle);
			}
			if (gimbal_y.gimbal_motor_mode == GIMBAL_MOTOR_GYRO)
			{
				gimbal_y.add_angle = rc_sent.yaw.target_angle;
			}
			gimbal_p.add_angle = rc_sent.pitch.target_angle; // 强制为imu反馈模式
		}
	}
}
float vision_actual_angle_cal()
{
	chassis_angle = gimbal_y.CAN_actual_angle - 343;
	if (chassis_angle < 0)
		chassis_angle += 360;
	return 0;
}

// double vision_actual_speed,vision_actual_angle;
// void vision_getSpeed()
//{
//	vision_actual_speed=(double)(chassis_speed_x*chassis_speed_x+chassis_speed_y*chassis_speed_y);
//	vision_actual_speed=sqrt(vision_actual_speed);
//	vision_x=chassis_speed_x*acos(chassis_angle)-chassis_speed_y*asin(chassis_angle);
//	vision_y=chassis_speed_x*asin(chassis_angle)+chassis_speed_y*acos(chassis_angle);
//	vision_actual_angle_cal();
//	vision_actual_angle=chassis_angle;
// }
