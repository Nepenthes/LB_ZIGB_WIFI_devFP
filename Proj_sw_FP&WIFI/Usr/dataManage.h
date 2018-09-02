#ifndef __DATAMANAGE_H_
#define __DATAMANAGE_H_

#define IF_REBORN	0	/*是否重新覆盖生日标记*/// 1：出厂前转世	0：正常启动

extern unsigned char xdata MAC_ID[6];

void software_Reset(void);
void Factory_recover(void);
void MAC_ID_Relaes(void);
void birthDay_Judge(void);

#endif