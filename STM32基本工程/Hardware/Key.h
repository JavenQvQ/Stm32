#ifndef __KEY_h
#define __KEY_h
void Key_Init(void );
void KEY_Function(void);
void KEY_FIFO_Put(uint8_t _KeyCode);
void KEY_Scan(void);
uint8_t KEY_FIFO_Get(void);

typedef struct
{
    uint8_t Buf[10];
    uint8_t Write;
    uint8_t Read;
}Key_FIFO;


typedef struct
{
 /* 下面是一个函数指针，指向判断按键手否按下的函数 */
 uint8_t (*IsKeyDownFunc)(void); /* 按键按下的判断函数,1表示按下 */

 uint8_t  Count;   /* 滤波器计数器 */
 uint16_t LongCount;  /* 长按计数器 */
 uint16_t LongTime;  /* 按键按下持续时间, 0表示不检测长按 */
 uint8_t  State;   /* 按键当前状态（按下还是弹起） */
 uint8_t  RepeatSpeed; /* 连续按键周期 */
 uint8_t  RepeatCount; /* 连续按键计数器 */
}KEY_T;

/************************************************************
** 移植说明：一个按键对应3个状态，分别是按下、弹起、长按,每个状态对应一个键值 
*/
typedef enum
{
 KEY_NONE = 0,   /* 0 表示按键事件 */

 KEY_1_DOWN,    /* 1键按下 */
 KEY_1_UP,    /* 1键弹起 */
 KEY_1_LONG,    /* 1键长按 */

 KEY_2_DOWN,    /* 2键按下 */
 KEY_2_UP,    /* 2键弹起 */
 KEY_2_LONG,    /* 2键长按 */

 KEY_3_DOWN,    /* 3键按下 */
 KEY_3_UP,    /* 3键弹起 */
 KEY_3_LONG,    /* 3键长按 */
}KEY_ENUM;




#endif

