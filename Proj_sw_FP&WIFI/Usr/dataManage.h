#ifndef __DATAMANAGE_H_
#define __DATAMANAGE_H_

#define IF_REBORN	0	/*�Ƿ����¸������ձ��*/// 1������ǰת��	0����������

extern unsigned char xdata MAC_ID[6];

void software_Reset(void);
void Factory_recover(void);
void MAC_ID_Relaes(void);
void birthDay_Judge(void);

#endif