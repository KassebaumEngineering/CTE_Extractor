/********* C code to read DC variables and Input channel descriptors *********/
/********* the printf statements are included for debugging purposes *********/
unsigned char seq_buf[20];
unsigned char DC_Values[8][240];
char DC_Labels[8][5];
char DC_Use[8];
int DC_Interval[8] = { 6, 6, 6, 6, 6, 6, 6, 6 };
float Slope[8];
float Intercept[8];
extern int ps;   /* page size 0=3 sec, 1=6 sec, 2=12 sec, 3=24 sec  */
extern int sec_per_page[4];

/* Alpha2 is the first byte after the checksum on a Beehive data packet */
void Slow_Vars (unsigned char Alpha2)
{
    char Descriptor[128][6];
    static char Byte_num = 0;
    static int sample = 0;
    int checksum, DCchan, i;
    float *fltptr;

    if((Alpha2 == (unsigned char)0xFE) && (Byte_num == 0))
        seq_buf[Byte_num++] = Alpha2;

    else if((Byte_num > 0) && (Byte_num < 20))
        seq_buf[Byte_num++] = Alpha2;

    if(Byte_num > 19) {
        checksum = SLOWVARSCHK();

        if(checksum > 0) {
            for(Byte_num = 1; Byte_num < 9; Byte_num++)
                DC_Values[Byte_num-1][sample] = seq_buf[Byte_num];
            if(++sample == (10 * sec_per_page[ps])) sample = 0;

            if((seq_buf[Byte_num] & 0x80) == 0) {         /* sequence 3 */
                /*      printf("SEQUENCE 3"); */
                DCchan = seq_buf[Byte_num++] & 0x7F;
                for(i = 0; i < 6; i++)
                    Descriptor[DCchan][i] = seq_buf[Byte_num++];
                /*      printf("Descriptor[%d] = %s", DCchan, Descriptor[DCchan]); */
            }

            else if((seq_buf[Byte_num] & 0x40) == 0) {    /* sequence 2 */
                /*      printf("SEQUENCE 2"); */
                DCchan = seq_buf[Byte_num++] & 0x07;
                for(i = 0; i < 5; i++)
                    DC_Labels[DCchan][i] = seq_buf[Byte_num++];
                /*      printf("DC_Labels[%d] = %s", DCchan, DC_Labels[DCchan]); */
            }

            else {                                         /* sequence 1 */
                /*      printf("SEQUENCE 1"); */
                DCchan = seq_buf[Byte_num] & 0x07;
                DC_Use[DCchan] = (char)((seq_buf[Byte_num++] & 0x08) ? 1 : 0);
                /*      printf("DC_Use[%d] = %d", DCchan, DC_Use[DCchan]); */
                fltptr = (float *)&seq_buf[10];
                Slope[DCchan] = *fltptr;
                /*      printf("Slope[%d] = %f", DCchan, Slope[DCchan]); */
                fltptr = (float *)&seq_buf[14];
                Intercept[DCchan] = *fltptr;
                /*      printf("Inter[%d] = %f", DCchan, Intercept[DCchan]); */
            }
        }
        Byte_num = 0;
    }
}
/****************************************/

.MODEL  MEDIUM,C
.286

	EXTRN   seq_buf:BYTE

.CODE

SLOWVARSCHK     PROC uses SI, c
	LEA     SI,seq_buf
	CLD
	MOV     BX,5A5AH
	MOV     CX,18
	XOR     AX,AX
CHKLP:  LODSB
	ADD     BX,AX
	ROL     BX,1
	LOOP    CHKLP

	LODSW
	CMP     AX,BX
	MOV     AX,1
	JE      DONE
	MOV     AX,-1
DONE:
	RET
SLOWVARSCHK     ENDP

END



