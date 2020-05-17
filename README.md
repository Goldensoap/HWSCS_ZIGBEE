# 多空间自组网网关（STM32部分）
## 项目简介
本项目为多空间自组网系统的**无线组网节点**，采用低功耗嵌入式设备（CC2530），使用Zigbee mesh 将传感器、控制器加入自组网，采集数据、控制设备。
## 功能
- 采集传感数据（光照、温湿度）
- 存储、分析自组网的传感信息
- LCD显示相关信息
## 环境依赖
- 硬件平台：CC2530、各类传感器
- 协议栈：TI Z-stack 2.5.1a
## 部署步骤
- 针对不同设备烧录对应版本的工程文件
- 启动组网
- 接收端连接网关
## 目录结构
```bash
├─Components
│
└─Projects
    └─zstack
        ├─Libraries
        │  ├─TI2530DB
        │  │  └─bin
        │  └─TIMAC
        │      └─bin
        ├─Samples（功能代码区）
        │  └─GenericApp
        │      ├─CC2530DB
        │      │  ├─CoordinatorEB
        │      │  │  ├─Exe
        │      │  │  ├─List
        │      │  │  └─Obj
        │      │  ├─EndDeviceEB
        │      │  │  ├─Exe
        │      │  │  ├─List
        │      │  │  └─Obj
        │      │  ├─RouterEB
        │      │  │  ├─Exe
        │      │  │  ├─List
        │      │  │  └─Obj
        │      │  ├─SensorApp
        │      │  │  ├─Exe
        │      │  │  ├─List
        │      │  │  └─Obj
        │      │  └─settings
        │      └─Source
        │          ├─Protocol
        │          └─Sensorlib
        ├─Tools
        │  └─CC2530DB
        └─ZMain
            └─TI2530DB
```
## 更新日志
[更新日志](./更新日志.md)
## 协议
[MIT协议](LICENSE.md)
