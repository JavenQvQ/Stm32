/**********************************************************
* @ File name -> sys.c
* @ Version   -> V1.0
* @ Date      -> 12-26-2013
* @ Brief     -> 系统设置相关的函数
**********************************************************/

#include "sys.h"



__asm void WFI_SET(void)
{
	WFI;		  
}
//关闭所有中断(但是不包括fault和NMI中断)
__asm void INTX_DISABLE(void)
{
	CPSID   I
	BX      LR	  
}
//开启所有中断
__asm void INTX_ENABLE(void)
{
	CPSIE   I
	BX      LR  
}
//设置栈顶地址
//addr:栈顶地址
__asm void MSR_MSP(uint32_t  addr) 
{
	MSR MSP, r0 			//set Main Stack value
	BX r14

}

