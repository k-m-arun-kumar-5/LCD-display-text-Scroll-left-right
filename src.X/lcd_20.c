/* ********************************************************************
FILE                   : main.c

PROGRAM DESCRIPTION    : In LCD, running text is displayed either to left or to right with specified number of gaps between consecutive same text display.
	 
AUTHOR                : K.M. Arun Kumar alias Arunkumar Murugeswaran
	 
KNOWN BUGS            :    

NOTE                  :  It used as base code for PIC16F887-repo-> 04_lcd-> lcd_22                										
                                    
CHANGE LOGS           : 

*****************************************************************************/   

#ifdef HI_TECH_COMPILER
  #include <pic.h>
  __CONFIG(0X2CE4);
#else // XC8 compiler
  #include <xc.h>
#endif
#include <string.h>

#define STATE_YES             'y'
#define STATE_NO              'n'

#define RS_PIN                RD0
#define RW_PIN                RD1
#define EN_PIN                RD2
#define LCD_PORT              PORTC
#define LCD_PORT_GPIO         TRISC
  
#define LCD_ENABLE_PULSE_WIDTH            (1000ul)
#define SHIFT_DISP_LEFT       (0U)
#define SHIFT_DISP_RIGHT      (1U)
#define NUM_LINE1             (1U) 
#define NUM_LINE2             (2U)
#define NUM_LINE3             (3U) 
#define NUM_LINE4             (4U)
 
/* for 16 * 4 LCD disp */                             
#define BEGIN_LOC_LINE1       (0X80)
#define BEGIN_LOC_LINE2       (0xC0)
#define BEGIN_LOC_LINE3       (0x90) 
#define BEGIN_LOC_LINE4       (0xD0)

#define MAX_AVAIL_NUM_COLS    (16U)
#define MAX_NUM_CHARS_IN_TEXT (30U)

/* for SHIFT_DISP_LEFT, number of trailing gaps between consecutive text  */
#define TRAIL_NUM_GAPS_BETWEEN_TEXT (2U)
/* for SHIFT_DISP_RIGHT, number of leading gaps between consecutive text */
#define LEAD_NUM_GAPS_BETWEEN_TEXT  (2U)

// Ring Buffer for Line 1 
char lcd_buffer_line1[MAX_AVAIL_NUM_COLS]; 
// Ring Buffer for Line 2 
char lcd_buffer_line2[MAX_AVAIL_NUM_COLS]; 
char line1_text_disp_finish_flag = STATE_YES, line2_text_disp_finish_flag = STATE_YES;
 
unsigned int read_command;

void LCD_Init();
void Delay_Time_By_Count(unsigned long int);
void LCD_Write_Pulse();
void LCD_Read_Pulse();
unsigned int Read_LCD_Command();
void Write_LCD_Command_Cannot_Check_BF(const unsigned int lcd_cmd);
void Write_LCD_Command (const unsigned int);
void Write_LCD_Data(const char);
void LCD_Line_Select(const unsigned int line);
void LCD_Running_Text_Display_Line1(const char *text_str, const unsigned int text_str_len, const unsigned int num_gaps_between_text_str, const unsigned int running_text_shift_direction);
void LCD_Running_Text_Display_Line2(const char *text_str, const unsigned int text_str_len, const unsigned int num_gaps_between_text_str, const unsigned int running_text_shift_direction); 
void Data_Str_LCD_Disp(const char *text_str, const unsigned int text_str_len, const unsigned int line);

void main(void) 
{
    //const char text_str_left[] = {"HELLO I AM ARUN AND SAVE UNIVERSE."};
	const char text_str_left[] = {"HELLO."};
    const char text_str_right[] = {"WORLD."};
    unsigned int text_str_left_len, text_str_right_len;
	
    LCD_PORT_GPIO = 0x00;
	PORTC = 0x00;
    TRISD = 0x00;  
    PORTD = 0x00;
    
    LCD_Init();
	// Flush buffers before starting something new on it
    memset(lcd_buffer_line1, 0, MAX_AVAIL_NUM_COLS ); 
	memset(lcd_buffer_line2, 0, MAX_AVAIL_NUM_COLS ); 
    text_str_left_len = strlen(text_str_left);
	text_str_right_len = strlen(text_str_right);
	
    for(;;)
    {
	    LCD_Running_Text_Display_Line1(text_str_left, text_str_left_len, TRAIL_NUM_GAPS_BETWEEN_TEXT, SHIFT_DISP_LEFT); 
		LCD_Running_Text_Display_Line2(text_str_right, text_str_right_len, LEAD_NUM_GAPS_BETWEEN_TEXT, SHIFT_DISP_RIGHT);  		
    }   
}
void LCD_Running_Text_Display_Line1(const char *text_str, const unsigned int text_str_len, const unsigned int num_gaps_between_text_str, const unsigned int running_text_shift_direction) 
{
    // keeping temp_text_str buffer custom for different string lengths is safe. 
	static char temp_text_str[MAX_NUM_CHARS_IN_TEXT]; 
	static int cur_char_in_str_index = 0;
	int cur_lcd_col, cur_temp_index;
	
	if(line1_text_disp_finish_flag == STATE_YES)
	{
	   //copy string into temp_text_str buffer up to the string length
        memcpy(temp_text_str, text_str, text_str_len); 
		cur_char_in_str_index = 0;	
	    line1_text_disp_finish_flag = STATE_NO;
	}  
	switch(running_text_shift_direction)
	{
		// running_text_shift_direction = LEFT  
	   case SHIFT_DISP_LEFT:    
          //whole shifting length = text_str_len + num_gaps_between_text_str
         if( cur_char_in_str_index < (int)(text_str_len + num_gaps_between_text_str)) 
         {
		    // display lcd_buffer_line1, we use MAX_AVAIL_NUM_COLS as length because we are shifting entire line num 1 in LCD to left 
            Data_Str_LCD_Disp(lcd_buffer_line1, MAX_AVAIL_NUM_COLS, NUM_LINE1 ); 
            for(cur_lcd_col = 0; cur_lcd_col < (int) MAX_AVAIL_NUM_COLS; ++cur_lcd_col)
            {
			   // Left shifting
			   if(cur_lcd_col != (int)(MAX_AVAIL_NUM_COLS - 1))
                  lcd_buffer_line1[(unsigned int)cur_lcd_col] = lcd_buffer_line1[(unsigned int)(cur_lcd_col + 1)]; 
			   // entering first temp_text_str[0] value at the end to do left shift into lcd_buffer_line1
               else
				  //cur_lcd_col = (MAX_AVAIL_NUM_COLS - 1) 
				  lcd_buffer_line1[(unsigned int)cur_lcd_col] = temp_text_str[0]; 
            } 
			// shifting of temp_text_str also needed to get all values step by step left to temp_text_str[0] location to do left shifting
            for(cur_temp_index = 0; cur_temp_index < (int)(text_str_len - cur_char_in_str_index); ++cur_temp_index)
            {
			   if(cur_temp_index <  (int)(text_str_len - 1 - cur_char_in_str_index))
                   temp_text_str[(unsigned int) cur_temp_index] = temp_text_str[(unsigned int)(cur_temp_index + 1)];
			  // cur_temp_index = (text_str_len - cur_char_in_str_index - 1) and temp_text_str[cur_temp_index] = 0 to insert space 
               else
				  temp_text_str[(unsigned int) cur_temp_index] = 0; 
            }
			 ++cur_char_in_str_index;
         }
		 else
		 {
			line1_text_disp_finish_flag = STATE_YES;             		
		 }			 
       break;		
       //RIGHT running_text_shift_direction select (logic is same and inverted for running_text_shift_direction so you can refer the above. 	   
       case SHIFT_DISP_RIGHT:   
          if(cur_char_in_str_index < (int)(text_str_len + num_gaps_between_text_str))
          {
               Data_Str_LCD_Disp(lcd_buffer_line1, MAX_AVAIL_NUM_COLS, NUM_LINE1);
               for(cur_lcd_col = (int)(MAX_AVAIL_NUM_COLS - 1); cur_lcd_col >= 0; --cur_lcd_col)
               {
                   if(cur_lcd_col != 0)
				     lcd_buffer_line1[(unsigned int)cur_lcd_col] = lcd_buffer_line1[(unsigned int)(cur_lcd_col - 1)]; 						
				   else
					 lcd_buffer_line1[(unsigned int)cur_lcd_col] = temp_text_str[(unsigned int)(text_str_len - 1)]; 
               }
               for(cur_temp_index = (int) (text_str_len - 1 ); cur_temp_index >= cur_char_in_str_index; --cur_temp_index)
               {
                   if(cur_temp_index > cur_char_in_str_index )
				     temp_text_str[(unsigned int) cur_temp_index] = temp_text_str[(unsigned int)(cur_temp_index - 1)];
				   else
					 // cur_temp_index = (cur_char_in_str_index ) and temp_text_str[cur_temp_index] =  0 to insert space    
					 temp_text_str[(unsigned int) cur_temp_index] = 0; 
               }
			   ++cur_char_in_str_index;
          }
		  else
		  {
			  line1_text_disp_finish_flag = STATE_YES;               	  
		  }
	   break;
    }
}	 
 
void LCD_Running_Text_Display_Line2(const char *text_str, const unsigned int text_str_len, const unsigned int num_gaps_between_text_str, const unsigned int running_text_shift_direction) 
{
   // keeping temp_text_str buffer custom for different string lengths is safe. 
	static char temp_text_str[MAX_NUM_CHARS_IN_TEXT]; 
	static int cur_char_in_str_index = 0;
	int cur_lcd_col, cur_temp_index;
	
	if(line2_text_disp_finish_flag == STATE_YES)
	{
	   //copy string into temp_text_str buffer up to the string length
        memcpy(temp_text_str, text_str, text_str_len); 
		cur_char_in_str_index = 0;
	    line2_text_disp_finish_flag = STATE_NO;
	}  
	switch(running_text_shift_direction)
	{
		// running_text_shift_direction = LEFT  
	   case SHIFT_DISP_LEFT:    
          //whole shifting length = text_str_len + num_gaps_between_text_str
         if( cur_char_in_str_index < (int)(text_str_len + num_gaps_between_text_str)) 
         {
		    // display lcd_buffer_line1, we use MAX_AVAIL_NUM_COLS as length because we are shifting entire line num 2 in LCD to left 
            Data_Str_LCD_Disp(lcd_buffer_line2, MAX_AVAIL_NUM_COLS, NUM_LINE2 ); 
            for(cur_lcd_col = 0; cur_lcd_col < (int) MAX_AVAIL_NUM_COLS; ++cur_lcd_col)
            {
			   // Left shifting
			   if(cur_lcd_col != (int)(MAX_AVAIL_NUM_COLS - 1))
                  lcd_buffer_line2[(unsigned int)cur_lcd_col] = lcd_buffer_line2[(unsigned int)(cur_lcd_col + 1)]; 
			   // entering first temp_text_str[0] value at the end to do left shift into lcd_buffer_line2
               else
				  //cur_lcd_col = (MAX_AVAIL_NUM_COLS - 1) 
				  lcd_buffer_line2[(unsigned int)cur_lcd_col] = temp_text_str[0]; 
            } 
			// shifting of temp_text_str also needed to get all values step by step left to temp_text_str[0] location to do left shifting
            for(cur_temp_index = 0; cur_temp_index < (int)(text_str_len - cur_char_in_str_index); ++cur_temp_index)
            {
			   if(cur_temp_index <  (int)(text_str_len - 1 - cur_char_in_str_index))
                   temp_text_str[(unsigned int) cur_temp_index] = temp_text_str[(unsigned int)(cur_temp_index + 1)];
			  // cur_temp_index = (text_str_len - cur_char_in_str_index - 1) and temp_text_str[cur_temp_index] = 0 to insert space 
               else
				  temp_text_str[(unsigned int) cur_temp_index] = 0; 
            }
			 ++cur_char_in_str_index;
         }
		 else
		 {
			line2_text_disp_finish_flag = STATE_YES;
           				
		 }			 
       break;		
       //RIGHT running_text_shift_direction select (logic is same and inverted for running_text_shift_direction so you can refer the above. 	   
       case SHIFT_DISP_RIGHT:   
          if(cur_char_in_str_index < (int)(text_str_len + num_gaps_between_text_str))
          {
               Data_Str_LCD_Disp(lcd_buffer_line2, MAX_AVAIL_NUM_COLS, NUM_LINE2);
               for(cur_lcd_col = (int)(MAX_AVAIL_NUM_COLS - 1); cur_lcd_col >= 0; --cur_lcd_col)
               {
                   if(cur_lcd_col != 0)
				     lcd_buffer_line2[(unsigned int)cur_lcd_col] = lcd_buffer_line2[(unsigned int)(cur_lcd_col - 1)]; 						
				   else
					 lcd_buffer_line2[(unsigned int)cur_lcd_col] = temp_text_str[(unsigned int)(text_str_len - 1)]; 
               }
               for(cur_temp_index = (int) (text_str_len - 1 ); cur_temp_index >= cur_char_in_str_index; --cur_temp_index)
               {
                   if(cur_temp_index > cur_char_in_str_index )
				     temp_text_str[(unsigned int) cur_temp_index] = temp_text_str[(unsigned int)(cur_temp_index - 1)];
				   else
					 // cur_temp_index = (cur_char_in_str_index ) and temp_text_str[cur_temp_index] =  0 to insert space    
					 temp_text_str[(unsigned int) cur_temp_index] = 0; 
               }
			   ++cur_char_in_str_index;
          }
		  else
		  {
			  line2_text_disp_finish_flag = STATE_YES;               			  
		  }
	   break;
    }   
} 
void LCD_Write_Pulse()
{
    EN_PIN = 1;
    Delay_Time_By_Count(LCD_ENABLE_PULSE_WIDTH);
    EN_PIN = 0;
    Delay_Time_By_Count(LCD_ENABLE_PULSE_WIDTH);
}
void LCD_Read_Pulse()
{
    EN_PIN = 0;
    Delay_Time_By_Count(LCD_ENABLE_PULSE_WIDTH);
    EN_PIN = 1;
    Delay_Time_By_Count(LCD_ENABLE_PULSE_WIDTH);	
}
void Write_LCD_Command_Cannot_Check_BF(const unsigned int lcd_cmd)
{
   RW_PIN = 0;
   RS_PIN = 0; 
   LCD_PORT = lcd_cmd;
   LCD_Write_Pulse();
}

void Check_LCD_Busy()
{
    LCD_PORT_GPIO = 0xFF;
	LCD_PORT = 0x00;
	RW_PIN = 1;
    RS_PIN = 0;
    
    /* busy flag = Bit 7 in LCD PORT, if busy flag == 1, wait till busy flag = 0, then any operation on LCD can be done */
   while(((read_command = Read_LCD_Command()) & 0x80) == 0x80)
   {
	   LCD_Read_Pulse();
	   LCD_Read_Pulse();
	   EN_PIN = 0;
	   Delay_Time_By_Count(LCD_ENABLE_PULSE_WIDTH);
   }	   
}
void Write_LCD_Command(const unsigned int lcd_cmd)
{
   Check_LCD_Busy();
   LCD_PORT_GPIO = 0x00;
   LCD_PORT = 0x00;
   RW_PIN = 0;
   RS_PIN = 0; 
   LCD_PORT = lcd_cmd;
   LCD_Write_Pulse();
}
 void Write_LCD_Data(const char lcd_data)
{
	 Check_LCD_Busy();
	 LCD_PORT_GPIO = 0x00; 
     LCD_PORT = 0x00;	 
     RW_PIN = 0;
     RS_PIN = 1;
     LCD_PORT = lcd_data;
     LCD_Write_Pulse();
}

void Delay_Time_By_Count(unsigned long int time_delay)
{
     while(time_delay--);
}
unsigned int Read_LCD_Command()
{
	 LCD_Write_Pulse();
	 read_command = LCD_PORT;	 
     return read_command;
}
// LCD line select 
void LCD_Line_Select(const unsigned int line)    
{
     switch(line)
	 {
		 //Set cursor at begin of line 1 ie DDRAM address 
		case NUM_LINE1:
           Write_LCD_Command(BEGIN_LOC_LINE1);    
        break;
		 //Set cursor at begin of line 2 ie DDRAM address 
        case NUM_LINE2:		
           Write_LCD_Command(BEGIN_LOC_LINE2);   
        break;
	 }	
 }
 //write string of length on line
 void Data_Str_LCD_Disp(const char *lcd_line_buffer, const unsigned int lcd_line_buffer_len, const unsigned int line) 
 {
     unsigned int cur_char_in_text_str_index;
	 
	 // LCD line select 
     LCD_Line_Select(line);    
     for(cur_char_in_text_str_index = 0; cur_char_in_text_str_index < lcd_line_buffer_len; ++cur_char_in_text_str_index)
     {
        Write_LCD_Data(lcd_line_buffer[cur_char_in_text_str_index]);         
     }     
 }
 

 void LCD_Init()
{
	/* wait for more than 15ms after LCD VSS rises to 4.5V, Busy Flag in LCD (BF) cannot be checked */
	Delay_Time_By_Count(15000UL);
    Write_LCD_Command_Cannot_Check_BF(0x30);
	/* wait for more than 4.1 ms, Busy Flag in LCD (BF) cannot be checked */
	Delay_Time_By_Count(4100UL);
    Write_LCD_Command_Cannot_Check_BF(0x30);
	/* wait for more than 100 us, Busy Flag in LCD (BF) cannot be checked */
	Delay_Time_By_Count(100);
    Write_LCD_Command_Cannot_Check_BF(0x30);
	Write_LCD_Command(0x38);  //function set, 2 lines, 8 bit and 5 * 7 dot matrix
	Write_LCD_Command(0x01);  //clear display
	Write_LCD_Command(0x0C);  //display on, cursor off, blink off
	Write_LCD_Command(0x06);  //entry mode, set increment 
	
}  

