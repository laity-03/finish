#ifndef FINISH_VOFA_H
#define FINISH_VOFA_H

#include "main.h"

/* 通过 JustFloat 协议发送一组浮点数。 */
void Vofa_SendFloats(float *data, uint8_t count);

/* 初始化 VOFA 命令接收，并在主循环中调用 Vofa_PollCommand() 解析命令。 */
void Vofa_Init(void);
void Vofa_PollCommand(void);

/* 发送固定 14 通道遥测帧，通道含义见工程控制框架文档。 */
void Vofa_SendTelemetry(void);

#endif // FINISH_VOFA_H
