//---------------------------------------------------------------------------
//-- Filename   aesgcmipdemo.c
//-- Title      AES256GCM10G25G Demo
//-- Project	AES256-GCM-10G25G IP
//-- Company    Design Gateway Co., Ltd.
//--
//--  Ver  |    Date    |  Author      | Remark
//---------+------------+--------------+-------------------------------------
//-- 1.06  | 2024/06/24 | Siraphop S.  | - Update to support KR260 
//---------+------------+--------------+-------------------------------------
//-- 1.05  | 2023/01/20 | Tan T.       | - fix bug size in set_key_or_iv
//---------+------------+--------------+-------------------------------------
//-- 1.04  | 2023/01/16 | Tan T.       | - Modify to AES256GCM10G25G single module
//---------+------------+--------------+-------------------------------------
//-- 1.03  | 2022/10/11 | Tan T.       | - Modify for Nios II 
//         |            |              | - Add key_length in function hex_string_to_key_set
//---------+------------+--------------+-------------------------------------
//-- 1.02  | 2022/09/01 | Tan T.       | - fix function hex_string_to_key_set
//         |            |              | - change addrA1 and addrA2 in aes_command function
//         |            |              |   from word to byte unit
//---------+------------+--------------+-------------------------------------
//-- 1.00  | 2022/06/14 | Taksaporn I. | 1st Released
//---------+------------+--------------+-------------------------------------
//-- 0.50  | 2022/03/24 | Pahol S.     | New Creation
//---------------------------------------------------------------------------

#include "KR260_ioctl.h"
#include <time.h>
//******************************************************************
// Register address
//******************************************************************
#define BASE_ADDR			(unsigned int)(0xA0000000)
#define ADDR_A1_REG			(BASE_ADDR+0x00)
#define ADDR_A2_REG			(BASE_ADDR+0x04)
#define AADINCNT_REG		(BASE_ADDR+0x08)
#define DATAINCNT_REG		(BASE_ADDR+0x0C)
#define VER_REG				(BASE_ADDR+0x10)
#define DECEN_REG			(BASE_ADDR+0x14)
#define BYPASS_REG			(BASE_ADDR+0x18)

#define KEYIN_0_REG			(BASE_ADDR+0x20)
#define KEYIN_1_REG			(BASE_ADDR+0x24)
#define KEYIN_2_REG			(BASE_ADDR+0x28)
#define KEYIN_3_REG			(BASE_ADDR+0x2C)
#define KEYIN_4_REG			(BASE_ADDR+0x30)
#define KEYIN_5_REG			(BASE_ADDR+0x34)
#define KEYIN_6_REG			(BASE_ADDR+0x38)
#define KEYIN_7_REG			(BASE_ADDR+0x3C)

#define IVIN_0_REG			(BASE_ADDR+0x40)
#define IVIN_1_REG			(BASE_ADDR+0x44)
#define IVIN_2_REG			(BASE_ADDR+0x48)
 
#define TAG_0_REG			(BASE_ADDR+0x50)
#define TAG_1_REG			(BASE_ADDR+0x54)
#define TAG_2_REG			(BASE_ADDR+0x58)
#define TAG_3_REG			(BASE_ADDR+0x5C)

// Base Address for reading Plain data, Cipher data, encryption/decryption AAD

#define DATAIN_ADDR			BASE_ADDR+0x2000
#define DATAOUT_ADDR		BASE_ADDR+0x4000

//******************************************************************
// Global parameters
//******************************************************************

#define DEFAULT_SHOW_AFTER_FILL		80
#define MEM_SIZE8					2048
#define MEM_SIZE32					512
#define MEM_SIZE128					128
#define AESKEY_SIZE_HEX				64
#define AESKEY_SIZE_INT				8
#define AESIV_SIZE_HEX				24
#define AESIV_SIZE_INT				3
#define DEFAULT_TIMEOUT				2000000

//******************************************************************
// General function
//******************************************************************

unsigned int get_input_int()
{
	unsigned char	key;
	unsigned int	result = 0;
	unsigned int	count = 0;

	while(1) 
	{
		key = get_char();
		if ((key == '\r') || (key == 0xA)) 
		{
			gen_printf("\n");
			break;
		}
		if ( key == 0x08 || key == 0x7f) //0x08 = backspace / 0x7f = Del (ubuntu return 0x7f when press blackspace )
		{						
			if (count > 0) 
			{
				count = count - 1;
				gen_printf("%c %c", 0x08, 0x08); // auto disappear previous character
				result = result / 10;
				continue;
			}
		}
		if ( ('0' <= key) && (key <= '9') ) 
		{
			count = count + 1;
			gen_printf("%c", key);
			result = result * 10 + (key - '0');
		}
		
	}

	return result;
}

// Input string by hex characters
// Return: data count in bytes
int get_input_string_hex(unsigned char hex_str[], unsigned int length) 
{
	unsigned char key;
	int count = 0;
	
	while(count < length) 
	{
		key = get_char();
		
		if ( (key == '\r') || (key == 0x0A) ) // new line
		{
			hex_str[count] = 0x00;
			gen_printf("\n");
			break;
		} 
		else if ( key == 0x08 || key == 0x7f) //0x08 = backspace / 0x7f = Del (ubuntu return 0x7f when press blackspace )
		{						
			if (count > 0) 
			{
				count = count - 1;
				gen_printf("%c %c", 0x08, 0x08); // auto disappear previous character
			}
		}
		else if ( (('0' <= key) && (key <= '9'))
				||(('A' <= key) && (key <= 'F'))
				||(('a' <= key) && (key <= 'f'))) 
		{
			hex_str[count] = key;
			count = count + 1;
			gen_printf("%c", key);
		}
	}
	if (count >= length) 
	{
		gen_printf("\n");
	}
	
	return count;
}

// Convert hex_string to key_set[AESKEY_SIZE_INT]
void hex_string_to_key_set(unsigned int key_set[], unsigned int key_length, unsigned char hex_str[], unsigned int hex_length) 
{
	unsigned char value;
	int count 	= 0;
	int len 	= hex_length;
	
	if (len > (key_length<<3))
	{
		len = (key_length<<3);
	}
	
	for (int i=0; i<key_length; i++) 
	{
		key_set[i] = 0;
	}
	
	while ( (count<len) && (hex_str[count]!=0x00) ) 
	{
		if ( ('0' <= hex_str[count]) && (hex_str[count] <= '9') ) 
		{
			value = hex_str[count] - 0x30;
		} 
		else if ( ('A' <= hex_str[count]) && (hex_str[count] <= 'F') ) 
		{
			value = hex_str[count] - 0x40 + 9;
		} 
		else if ( ('a' <= hex_str[count]) && (hex_str[count] <= 'f') ) 
		{
			value = hex_str[count] - 0x60 + 9;
		} 
		else if ( (hex_str[count] == 0x20) || (hex_str[count] == 0x07) ) // space or tab
		{	
			continue;
		}
		
		for (int i=key_length-1; i>0; i--)
		{
			key_set[i] = (key_set[i]<<4) | (key_set[i-1]>>28);
		}
		
		key_set[0] = (key_set[0]<<4) | (unsigned int)(value);
		count = count + 1;
	}
}

// Show AAD/Data
void show_data(unsigned int base_addr, unsigned int len)
{

	unsigned int 			length128;
	
	if ( (len&15)==0 )
	{
		length128 = len>>4;
	}else
	{
		length128 = (len>>4)+1;
	}
	
	if (length128 > MEM_SIZE128) length128 = MEM_SIZE128;

	for (unsigned int i=0; i<length128; i++)
	{
		gen_printf("%04x:  ", i);
		for (unsigned int k=0; k<16; k++)
		{
			gen_printf("%02x", (unsigned int)(user_read(base_addr+(i<<4)+k , 8))); // cast / update read
			if ((k&3)==3) gen_printf(" ");
		}
		gen_printf(" ");
		for (int k=0; k<16; k++)
		{
			gen_printf("%02x", (unsigned int)(user_read(base_addr+(0x2000)+(i<<4)+k , 8))); // cast / update read
			if ((k&3)==3) gen_printf(" ");
		}
		gen_printf("\n");
	}
	gen_printf("\n");
}

// Fill RAM with data pattern
char fill_data(unsigned int zero_padding, unsigned int base_addr, unsigned int length)
{
	char			pattern;
	
	unsigned int	char_ptr;
	unsigned int	int_tmp=0;
	unsigned int	length128=0;
	
	if ( (length&0xF)==0 )
	{
		length128 = length>>4;
	}else
	{
		length128 = (length>>4)+1;
	}
	
	gen_printf("a. zero pattern\n");
	gen_printf("b. 8-bit counter\n");
	gen_printf("c. 16-bit counter\n");
	gen_printf("d. 32-bit counter\n");
	gen_printf("Choice: ");
	do 
	{
		pattern = get_char();
		gen_printf("%c\n", pattern);
		if ( (('a' > pattern) || (pattern > 'd')) ) 
		{
			gen_printf("invalid choice!! %c\n", pattern);
		}
	} while ( ('a' > pattern) || (pattern > 'd') );
	
	switch (pattern)
	{		
		case 'd':
			// 32-bit pattern
			for (unsigned int i=0; i<length128; i++)
			{
				for(int k=0; k<16; k++)
				{
					if ( ((i<<4)+k)<length )
					{
						user_write( (base_addr+(i<<4)+k) ,8, int_tmp>>8*(3-(k&3)));
						if ( (k&3)== 3 ) int_tmp = int_tmp+1;
					}else
					{
						if(zero_padding==0x01) user_write( (base_addr+(i<<4)+k) ,8, 0);
					}
				}
			}
			break;

		case 'c':
			// 16-bit pattern
			for (unsigned int i=0; i<length128; i++)
			{
				for(unsigned int k=0; k<16; k++)
				{
					if ( ((i<<4)+k)<length )
					{
						int_tmp	  = (int_tmp<<16) | (int_tmp>>16);
						user_write( (base_addr+(i<<4)+k) ,8, int_tmp);
						if ( (k&1)== 1 ) int_tmp = int_tmp+1;
					}else
					{
						if(zero_padding==0x01) user_write( (base_addr+(i<<4)+k) ,8, 0);
					}
				}
			}
			break;
		
		case 'b':
			// 8-bit pattern
			for (unsigned int i=0; i<length128; i++)
			{
				for(unsigned int k=0; k<16; k++)
				{
					if ( ((i<<4)+k)<length )
					{
						user_write( (base_addr+(i<<4)+k) ,8, int_tmp);
						int_tmp   = int_tmp + 1;
					}else
					{
						if(zero_padding==0x01) user_write( (base_addr+(i<<4)+k) ,8, 0); 
					}
				}
			}
			break;
			
		case 'a':
			// zero pattern
			for (int i=0; i<length128; i++)
			{
				for(int k=0; k<16; k++)
				{
					user_write( (base_addr+(i<<4)+k) ,8, 0);
				}
			}
			break;

		default :
			gen_printf("invalid choice!! %c\n", pattern);
			break;
	}
	
	return pattern;
}

int wait_ready(unsigned int CMD_REG)
{
	unsigned int			int_tmp;
	unsigned int i=0;

	do {
		int_tmp	= (unsigned int)user_read(CMD_REG,32);
		i = i+1;
		if (i %DEFAULT_TIMEOUT == DEFAULT_TIMEOUT-1)
		{
			gen_printf("."); 
			if (i>5*DEFAULT_TIMEOUT)
			{
				gen_printf(" TIMEOUT, something went wrong.\n");
				return -1;
			}
		}
	} while ( int_tmp != 0 );

	return 0;
}

// Set Resgister
void set_key_or_iv(unsigned int start_addr, unsigned int length_hex, char *label)
{	
	unsigned int 			int_ptr;
	unsigned char			hex_str[64];
	unsigned int			length;
	unsigned int			data_length_hex = length_hex;
	unsigned int			data_length_int = length_hex>>3;
	unsigned int			reg_set[data_length_int];
	
	char					*str_label=label;
	
	// encryption
	gen_printf("               %s= 0x", str_label);
	
	for (int i=data_length_int-1; i>=0; i--) 
	{
		reg_set[i] = user_read(start_addr+4*i,32);
		gen_printf("%08x",reg_set[i]);
	}
	gen_printf("\n");

	gen_printf("(enter to use %s)= 0x", str_label);
	length = get_input_string_hex(hex_str, data_length_hex);
	if (length>0) 
	{
		hex_string_to_key_set(reg_set, data_length_int, hex_str, length);
	}
	
	if (wait_ready(DATAINCNT_REG) < 0)return;
	for (int i=data_length_int-1; i>=0; i--) 
	{
		user_write(start_addr+4*i , 32 , reg_set[i]);
	}
	if (wait_ready(DATAINCNT_REG) < 0)return;
	
	gen_printf("           new %s= 0x", str_label);
	for (int i=data_length_int-1; i>=0; i--) 
	{ 
		reg_set[i] = user_read(start_addr+4*i, 32);
		gen_printf("%08x",reg_set[i]);
	}
	gen_printf("\n");
}

// send AES command : set AadInCount and DataInCount
void aes_command(unsigned int mode, unsigned int AADCNT_REG, unsigned int aad_cnt, unsigned int DATACNT_REG, unsigned int data_cnt)
{
	volatile unsigned int 	*int_ptr;
	
	// set Encrypt/Decrypt Mode
	if(mode==0x02)
	{
		user_write(BYPASS_REG , 32 , 0x01);
	}
	else
	{
		user_write(DECEN_REG , 32 , mode);
		user_write(BYPASS_REG , 32 , 0x00);
	}
	
	// set Start Address
	user_write(ADDR_A1_REG , 32 , 0x00);
	user_write(ADDR_A2_REG , 32 , 0x00);
	
	// set AADCNT_REG with length
	user_write(AADCNT_REG , 32 , aad_cnt);
	
	// set DATACNT_REG with length to start encrypt/decrypt operation
	user_write(DATACNT_REG , 32 , data_cnt);

	if (wait_ready(DATACNT_REG) < 0)
	{
		return;
	}
}

// show authentication tag
void show_tag(unsigned int TAG_REG)
{
	
	gen_printf("\nTag : ");
	for (int i=3;i>=0;i--)
		gen_printf("%08x", (unsigned int)user_read(TAG_REG+4*i , 32) );
	gen_printf("\n");
}

// encrypt plain data with current encryption parameters and decrypt encrypted data with current decryption parameters.
// Return : 0 for verification succeeded and -1 for verification failed
int loop_verify(unsigned int aad_cnt, unsigned int data_cnt)
{	
	unsigned int			mask;
	unsigned int			length8;
	unsigned int			aad128;
	unsigned int			int_tmp;
	unsigned int 			plain_text[MEM_SIZE32];
	unsigned int 			enc_tag[4], dec_tag[4];
	
	//print current parameters
	gen_printf("\n");
	gen_printf("KeyIn= 0x");
	for (int i=(AESKEY_SIZE_HEX>>3)-1; i>=0; i--) 
	{
		gen_printf("%08x",(unsigned int)user_read(KEYIN_0_REG+4*i,32));
	}
	gen_printf("\n");
	gen_printf(" IvIn= 0x");
	for (int i=(AESIV_SIZE_HEX>>3)-1; i>=0; i--) 
	{
		gen_printf("%08x",(unsigned int)user_read(IVIN_0_REG+4*i,32));
	}
	gen_printf("\n");
	gen_printf("Length of encrypt-AAD  : %d byte\n", aad_cnt);
	gen_printf("Length of encrypt-Data : %d byte\n", data_cnt);
	
	if((aad_cnt&0xF)>0 )
		aad128	= (aad_cnt>>4) + 1;
	else
		aad128	= (aad_cnt>>4);
	
	// store original DataIn from Memory
	for (int i=0; i<MEM_SIZE32; i++)
	{
		plain_text[i] = (unsigned int)user_read(DATAIN_ADDR+(i<<2),32);
	}
	
	// clear data memory for DataOut
	for (int i=0; i<MEM_SIZE32; i++)
	{
		user_write(DATAOUT_ADDR+(i<<2) , 32 , 0);
	}
	
	// start encryption with current parameters
	aes_command(0x00, AADINCNT_REG, aad_cnt, DATAINCNT_REG, data_cnt);
	
	// Show encryption data
	if((aad128<<4)+data_cnt>0)
	{
		gen_printf("\n");
		gen_printf("                Original Data                        Encrypted Data\n");
		gen_printf("Addr#  .0.....3 .4.....7 .8.....B .C.....F  .0.....3 .4.....7 .8.....B .C.....F\n");
		show_data(DATAIN_ADDR, (aad128<<4)+data_cnt);
	}
	
	// store and show encrypted tag
	gen_printf("Encrypted Tag : ");
	for (int i=3; i>=0; i--) 
	{
		enc_tag[i] = (unsigned int)user_read(TAG_0_REG+4*i,32);
		gen_printf("%08x", enc_tag[i]);
	}
	gen_printf("\n");
	
	// Copy memory from dataout to datain
	for (int i=0; i<MEM_SIZE32; i++)
	{
		int_tmp		= (unsigned int)user_read(DATAOUT_ADDR+(i<<2),32);
		user_write(DATAIN_ADDR+(i<<2) , 32 ,int_tmp );
	}
	
	// start decryption with current parameters
	aes_command(0x01, AADINCNT_REG, aad_cnt, DATAINCNT_REG, data_cnt);
	
	// Show encryption data
	if((aad128<<4)+data_cnt>0)
	{
		gen_printf("\n");
		gen_printf("               Encrypted Data                        Decrypted Data\n");
		gen_printf("Addr#  .0.....3 .4.....7 .8.....B .C.....F  .0.....3 .4.....7 .8.....B .C.....F\n");
		show_data(DATAIN_ADDR, (aad128<<4)+data_cnt);
	}
	
	// store and show decrypted tag
	gen_printf("Decrypted Tag : ");
	for (int i=3; i>=0; i--) 
	{
		dec_tag[i] = (unsigned int)user_read(TAG_0_REG+4*i,32);
		gen_printf("%08x", dec_tag[i]);
	}
	gen_printf("\n");
	
	// write DataIn Memory with plaintext
	for (int i=0; i<MEM_SIZE32; i++)
	{
		user_write(DATAIN_ADDR+(i<<2) , 32 ,plain_text[i] );
	}
	
	length8		=	data_cnt + (aad128<<4);
	// check decrypted data
	for (int i=0; i<MEM_SIZE32; i++)
	{
		int_tmp = (unsigned int)user_read(DATAOUT_ADDR+(i<<2),32);
		//set DataMask
		if ( length8 > 0x04 )
		{
			mask		= 0xFFFFFFFF;
			length8		= length8 - 0x04;
		}
		else if( length8 > 0x00 )
		{
			mask		= 0xFFFFFFFF >> (length8<<3);
			length8 	= 0x00;
		}
		else
		{
			mask		= 0x00000000;
			break;
		}
		//Compare between Plain data and Cipher data
		if ( (plain_text[i]&mask) != (int_tmp&mask) )
		{
			gen_printf("\nLoop verification Failed.\n");
			gen_printf("Decrypted data does not match with Plain data at Addr# 0x%04x\n", (i>>2));
			show_data(DATAIN_ADDR, data_cnt + (aad128<<4));
			return -1;
		}
	}
	// check Tag
	for (int i=3; i>=0; i--)
	{
		if ( enc_tag[i] != dec_tag[i] )
		{
			gen_printf("\nLoop verification Failed.\n");
			gen_printf("Decryption tag does not match with Encryption tag.\n\n");
			gen_printf("Encryption tag : ");
			for (int i=3; i>=0; i--) gen_printf("%08x", enc_tag[i]); gen_printf("\n");
			gen_printf("Decryption tag : ");
			for (int i=3; i>=0; i--) gen_printf("%08x", dec_tag[i]); gen_printf("\n");

			return -1;
		}
	}
	
	gen_printf("Loop verification succeeded.\n");
	return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++ Main
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int main(void)
{
	unsigned int			int_tmp;
	unsigned char			key;
	unsigned int			length;
	
	unsigned int			start_addr	= DATAIN_ADDR;
	unsigned int			aad_cnt		= 0;
	unsigned int			data_cnt	= 0;
	
	unsigned int			aad128	= 0;
	unsigned int			data128	= 0;
	
	int_tmp		= (unsigned int)user_read(VER_REG , 32);
	gen_printf("\n");
	gen_printf("\n");
	gen_printf("=====================================================\n");
	gen_printf("AES256GCM Version = 0x");
	gen_printf("%08x\n", int_tmp);
	
//-----------------------------------------------------------------------------------

	/* Event loop never exits. */
	while (1)
	{
		gen_printf("\n\n");
		gen_printf("++++++ AES256GCM Demo Menu ++++++\n");
		gen_printf("0. KeyIn Setting\n");
		gen_printf("1. IvIn Setting\n");
		gen_printf("2. Show Data Memory\n");
		gen_printf("3. Fill AAD Memory\n");
		gen_printf("4. Fill DataIn Memory\n");
		gen_printf("5. Encrypt Data\n");
		gen_printf("6. Decrypt Data\n");
		gen_printf("7. Bypass Data\n");
		gen_printf("8. Clone Memory\n");
		gen_printf("9. Loop verification\n");
		gen_printf("q. Exit program\n");
		gen_printf("Choice: ");
	
		key = get_char();
		gen_printf("%c\n", key);

		switch (key)
		{
			
			case '0' :
				gen_printf("\n");
				gen_printf("+++ KeyIn Setting +++\n");
				set_key_or_iv(KEYIN_0_REG, AESKEY_SIZE_HEX, "KeyIn");
				break;
				
			case '1' :
			gen_printf("\n");
				gen_printf("+++ IvIn Setting +++\n");
				set_key_or_iv(IVIN_0_REG, AESIV_SIZE_HEX, "IvIn");
				break;
				
			case '2' :
				gen_printf("\n+++ Show Data Memory +++\n");
				gen_printf("Number of Data in byte (enter = %d): ", DEFAULT_SHOW_AFTER_FILL);
				length = get_input_int();
				if (length == 0) length = DEFAULT_SHOW_AFTER_FILL;
				gen_printf("\n");
				gen_printf("                DataIn Memory                        DataOut Memory\n");
				gen_printf("Addr#  .0.....3 .4.....7 .8.....B .C.....F  .0.....3 .4.....7 .8.....B .C.....F\n");
				show_data(DATAIN_ADDR, length);
				break;
				
			case '3' :
				gen_printf("\n+++ Fill AAD Memory +++\n");
				gen_printf("Length of AAD in byte (enter = 0): ");
				aad_cnt = get_input_int();
				
				if (aad_cnt>MEM_SIZE8) aad_cnt=MEM_SIZE8;
				if((aad_cnt & 0xF) > 0 ){
					aad128		= (aad_cnt>>4) + 1;
					start_addr	= DATAIN_ADDR + (aad_cnt & ~(0xF)) + 0x10;
				}else{
					aad128		= (aad_cnt>>4);
					start_addr	= DATAIN_ADDR + aad_cnt;
				}
				if (aad_cnt>0)
				{
					gen_printf("Choose AAD pattern\n");
					fill_data(0x01, DATAIN_ADDR, aad_cnt);
					gen_printf("\n");
					gen_printf("                DataIn Memory                        DataOut Memory\n");
					gen_printf("Addr#  .0.....3 .4.....7 .8.....B .C.....F  .0.....3 .4.....7 .8.....B .C.....F\n");
					show_data(DATAIN_ADDR, (aad128<<4)+data_cnt);
				}	
				break;
			
			case '4' :
				gen_printf("\n+++ Fill DataIn Memory +++\n");
				gen_printf("Length of DataIn in byte (enter = 0): ");
				data_cnt = get_input_int();
				if((data_cnt & 0xF) > 0 )
					data128		= (data_cnt>>4) + 1;
				else
					data128		= (data_cnt>>4);
				if (data_cnt>0)
				{
					if(MEM_SIZE128-aad128 < data128) {data_cnt = MEM_SIZE8-(aad128<<4);}
					gen_printf("Choose DataIn pattern\n");
					fill_data(0x00, start_addr, data_cnt);
					gen_printf("\n");
					gen_printf("                DataIn Memory                        DataOut Memory\n");
					gen_printf("Addr#  .0.....3 .4.....7 .8.....B .C.....F  .0.....3 .4.....7 .8.....B .C.....F\n");
					show_data(DATAIN_ADDR, data_cnt+(aad128<<4));
				}
				break;

			case '5' :
				gen_printf("\n+++ Encrypt Data +++\n");
				aes_command(0x00, AADINCNT_REG, aad_cnt, DATAINCNT_REG, data_cnt);
				gen_printf("Length of encrypt-AAD : %d byte\n", aad_cnt);
				gen_printf("Length of encrypt-Data : %d byte\n", data_cnt);
				if((aad128<<4)+data_cnt>0)
				{
					gen_printf("\n");
					gen_printf("                DataIn Memory                        DataOut Memory\n");
					gen_printf("Addr#  .0.....3 .4.....7 .8.....B .C.....F  .0.....3 .4.....7 .8.....B .C.....F\n");
					show_data(DATAIN_ADDR, (aad128<<4)+data_cnt);
				}
				show_tag(TAG_0_REG);
				break;
				
			case '6' :
				gen_printf("\n+++ Decrypt Data +++\n");
				aes_command(0x01, AADINCNT_REG, aad_cnt, DATAINCNT_REG, data_cnt);
				gen_printf("Length of decrypt-AAD : %d byte\n", aad_cnt);
				gen_printf("Length of decrypt-Data : %d byte\n", data_cnt);
				if((aad128<<4)+data_cnt>0)
				{
					gen_printf("\n");
					gen_printf("                DataIn Memory                        DataOut Memory\n");
					gen_printf("Addr#  .0.....3 .4.....7 .8.....B .C.....F  .0.....3 .4.....7 .8.....B .C.....F\n");
					show_data(DATAIN_ADDR, (aad128<<4)+data_cnt);
				}
				show_tag(TAG_0_REG);
				break;

			case '7' :
				gen_printf("\n+++ Bypass Data +++\n");
				aes_command(0x02, AADINCNT_REG, aad_cnt, DATAINCNT_REG, data_cnt);
				gen_printf("Length of decrypt-AAD : %d byte\n", aad_cnt);
				gen_printf("Length of decrypt-Data : %d byte\n", data_cnt);
				if((aad128<<4)+data_cnt>0)
				{
					gen_printf("\n");
					gen_printf("                DataIn Memory                        DataOut Memory\n");
					gen_printf("Addr#  .0.....3 .4.....7 .8.....B .C.....F  .0.....3 .4.....7 .8.....B .C.....F\n");
					show_data(DATAIN_ADDR, (aad128<<4)+data_cnt);
				}
				break;

			case '8' :
				gen_printf("\n+++ Clone Memory +++\n");
				for (int i=0; i<MEM_SIZE32; i++)
				{
					int_tmp		=   (unsigned int)user_read(DATAOUT_ADDR+(i<<2),32);
					user_write(DATAIN_ADDR+(i<<2) , 32 ,int_tmp );
				}
				if((aad128<<4)+data_cnt>0)
				{
					gen_printf("\n");
					gen_printf("                DataIn Memory                        DataOut Memory\n");
					gen_printf("Addr#  .0.....3 .4.....7 .8.....B .C.....F  .0.....3 .4.....7 .8.....B .C.....F\n");
					show_data(DATAIN_ADDR, (aad128<<4)+data_cnt);
				}
				break;

			case '9' :
				gen_printf("\n+++ Loop verification +++\n");
				loop_verify(aad_cnt, data_cnt);
				break;
			case 'q' :
				close_device();  // Close the device file before exiting
				printf("Exiting program\n");
				return 0;	
			default : 
				gen_printf("invalid choice!! %c\n", key);
				break;
		} 
	
	}
	return 0;
}
