#ifndef    _LCD_H_
#define    _LCD_H_

#define    LCD_BLA    PDout(10)
#define    LCD_RST    PDout(11)
#define    LCD_CS1    PDout(9)
#define    LCD_CS2    PDout(8)
#define    LCD_EN     PEout(9)
#define    LCD_RW     PEout(10)
#define    LCD_DI     PEout(7)
#define    LCD_DB0    PEout(12)
#define    LCD_DB1    PEout(11)
#define    LCD_DB2    PEout(14)
#define    LCD_DB3    PEout(13)
#define    LCD_DB4    PBout(10)
#define    LCD_DB5    PEout(15)
#define    LCD_DB6    PBout(15)
#define    LCD_DB7    PBout(14)

#define    FullScreen    0x00//全屏
#define    LeftScreen    0x01//左半屏
#define    RighScreen    0x02//右半屏

#define    Page_0    0x00//第一行
#define    Page_1    0x01
#define    Page_2    0x02//第二行
#define    Page_3    0x03
#define    Page_4    0x04//第三行
#define    Page_5    0x05
#define    Page_6    0x06//第四行
#define    Page_7    0x07
#define    Page_8    0x08//第五行
#define    Page_9    0x09
#define    Page_10    0x0a//第六行
#define    Page_11    0x0b
#define    Page_12    0x0c//第七行
#define    Page_13    0x0d
#define    Page_14    0x0c//第八行
#define    Page_15    0x0d

#define    Display_Off   0x00//关闭显示
#define    Display_On    0x01//打开显示

#define    LCD_BLA_ON    (LCD_BLA = 0)//背光开
#define    LCD_BLA_OFF   (LCD_BLA = 1)//背光关

#define    DISPLAY_POSITIVE    1
#define    DISPLAY_NEGATIVE    0

#define    RW_DELAY_US     2

extern    u8 TotalNum_GB16_HanZi;
extern    u8 TotalNum_8x16_char;

void Lcd_Clear(unsigned char cs_sel);
void LCD_Init(void);
void Display_16x16(unsigned char cs, unsigned char page, unsigned char column, const unsigned char *p, unsigned char p_or_n);
void Display_16x16_center(unsigned char page, unsigned char column, unsigned char *p, unsigned char p_or_n);
void Display_8x16(unsigned char cs, unsigned char page, unsigned char column, const unsigned char *p, unsigned char p_or_n);
void Display_Line_16x16(unsigned char page, unsigned char column, unsigned char p[256],unsigned int len,unsigned char p_or_n);
void Display_Line_8x16(unsigned char page, unsigned char column, unsigned char p[256],unsigned int len);
void Display_6x16(unsigned char cs, unsigned char page, unsigned char column, unsigned char *p, unsigned char p_or_n);
void Display_Line_6x16(unsigned char page, unsigned char column, unsigned char p[256],unsigned int len);

void Display_Line_Index_16x16(unsigned char page, unsigned char column, char *str,unsigned char p_or_n);
void Display_Line_Index_8x16(unsigned char page, unsigned char column, char *str,unsigned char p_or_n);
void Display_Line_Index_8x16_16x16(unsigned char page, unsigned char column, char *str,unsigned char p_or_n);
void Display_Line_Index_8x16_pn(unsigned char page, unsigned char column, char *str,unsigned char fx_pos);
void Display_Line_Index_8x16_pn2(unsigned char page, unsigned char column, char *str,unsigned char fx_pos_1,unsigned char fx_pos_2);
void Display_Line_Index_8x16_16x16_pn2(unsigned char page, unsigned char column, char *str,unsigned char fx_pos_1,unsigned char fx_pos_2);

#endif
