import sys
sys.path.append('/home/pi/Desktop/ZL-PI/')
import time
import car_run as myCar
import ZL_SDK.Z_UartServer as myUart
import Ultrasonic_sensor as myUls
# start
if __name__ == '__main__':
    myUart.setup_uart(115200) #设置串口
    myUls.setup_sensor(23, 22)  # 初始化超声波传感器，引脚为GPIO 23和GPIO 22

    try:
        while 1:
            msg = input("please input value: ")
            print("input is:"+msg)
            dis = myUls.distance()
            print(dis)  # 调用避障函数
            time.sleep(1)
            
            if msg =="l":
                myCar.car_run(-400,400,-400,400,1000)  # 向左1s
                time.sleep(1)

                print("go left")
                #go left
            elif msg =="r":
                myCar.car_run(400, -400, 400, -400, 1000)  # 向右1s
                time.sleep(1)
                #go right
            elif msg =="b":
                myCar.car_run(-400, -400, -400, -400, 1000)  # 后退
                time.sleep(1)
                print("back")
                #go back
            else:
                myCar.car_run(400, 400, 400, 400, 1000)  # 缓速前进
                time.sleep(1)
                print("forward")       
    except:
        myCar.car_stop()

