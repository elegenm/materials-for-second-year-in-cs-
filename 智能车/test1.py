# -*- coding: utf-8 -*-
import sys
sys.path.append('/home/pi/Desktop/ZL-PI/')
import time
import car_run as myCar
import ZL_SDK.Z_UartServer as myUart
import Ultrasonic_sensor as myUls

if __name__ == '__main__':
    myUart.setup_uart(115200) #设置串口
    myUls.setup_sensor(23, 22)  # 初始化超声波传感器，引脚为GPIO 23和GPIO 22

# 速度控制参数
current_speed = 400  # 初始速度
speed_step = 100     # 每次加减速的步长

try:
    while 1:
        # 显示控制菜单
        print("\n智能小车控制系统")
        print("w:前进 s:后退 a:左转 d:右转 p:停止 +:加速 -:减速")
        print(f"当前速度: {current_speed}")
        
        # 获取用户输入
        msg = input("please input value: ").lower()
        
        # 执行对应操作
        if msg == "w":  # 前进
            myCar.car_run(current_speed, current_speed, current_speed, current_speed, 1000)
            #四个轮子速度，运行时间毫秒
            print("前进")
        elif msg == "s":  # 后退
            myCar.car_run(-current_speed, -current_speed, -current_speed, -current_speed, 1000)
            print("后退")
        elif msg == "a":  # 左转
            myCar.car_run(-current_speed, current_speed, -current_speed, current_speed, 600)
            print("go left")
        elif msg == "d":  # 右转
            myCar.car_run(current_speed, -current_speed, current_speed, -current_speed, 600)
            print("go right")
        elif msg == "p":  # 停止
            myCar.car_stop()
            print("stop")
        elif msg == "+":  # 加速
            current_speed += speed_step
            print(f"加速到 {current_speed}")
        elif msg == "-":  # 减速
            if current_speed > speed_step:  # 防止速度变为负数
                current_speed -= speed_step
                print(f"减速到 {current_speed}")
            else:
                print("已是最低速度")
        else:
            print("无效指令，请重新输入")
        
        time.sleep(0.1)  # 短暂延迟

except KeyboardInterrupt:
    print("\n程序结束")
    myCar.car_stop()
except Exception as e:
    print(f"程序出错: {e}")
    myCar.car_stop()