/*
 * cte.c
 *
 * C Program for Extracting CTE Taped Data  
 *
 *  $Id: cte.c,v 1.3 1994/12/19 15:17:52 jak Exp $
 *
 *  Author: John Kassebaum
 *
 * $Log: cte.c,v $
 * Revision 1.3  1994/12/19 15:17:52  jak
 * Made changes at Work to the cte - now reads the alpha bytes! -jak
 *
 * Revision 1.2  1994/12/10  20:28:37  jak
 * Fixed a bug in the channel code.  -jak
 *
*/

static char rcsid_cte_c[] = "$Id: cte.c,v 1.3 1994/12/19 15:17:52 jak Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Global controls */
int report = 0;

/*
 * Usage Info -------------------------------------------------------------
 */
#define USAGE    "Usage: %s [args] < input data_file > output data_file\n"
#define DESCR_0    "Where args are one or more of:\n"
#define DESCR_1    " -h            prints this help message and exits.\n"
#define DESCR_2    " -v            verbosely prints out data\n"
#define DESCR_3    " -s            prints statistics\n"
#define DESCR_4    " -q            quiet - supresses some error messages.\n"
#define DESCR_5    " -i            ignore - ignore bad checksums.\n"
#define DESCR_6    " -o <format>   output style (on of: ascii, binary, debug, channel <int> ) default ascii.\n"
#define DESCR_7    "\n data_file  -  A file of CTE tape data\n"
/*
 * ------------------------------------------------------------------------
 */

/*
 * CTE Structure Definitions
 * - note: These structures have been defined on a machine which 16-bit alignment
 *   requirements for strudtures.  The weird char layout for is to insure byte 
 *   alignment of the data can be read, and to prevent the compiler from inserting
 *   padding bytes anywhere.  The structs are created on a machine with little-endian 
 *   data storage, whereas this code has been written to obtain the value back from
 *   storage even for a big-endian machine.  That conversion is the reason for the 
 *   following converson macros.
 */
#define LONG_OF(a,b,c,d)    ( ((long)a<<24) + ((long)b<<16) + ((long)c<<8) + (long)d )
#define SHORT_OF(a,b)       ( ((short)a<<8) + (short)b )
#define BYTE4(a)            ( ((unsigned long)a & 0xff000000) >> 24 )
#define BYTE3(a)            ( ((unsigned long)a & 0x00ff0000) >> 16 )
#define BYTE2(a)            ( ((unsigned long)a & 0x0000ff00) >> 8 )
#define BYTE1(a)            ( ((unsigned long)a & 0x000000ff) )

enum CTE_Type { CTE128 = 0xDF, CTE64 = 0xEF };

typedef struct {
    unsigned char sync;    /* always 0xEF for CTE128_Sample */
    unsigned char alarm;   /* bit 0 - patient alarm, bit 1 indicates SzAC alarm, all others are 0 */

/*  unsigned long time;    /* time of day in 1/200th of a second increments */
    unsigned char time1;   /* llsb  of long time  - long stored in little endian format */
    unsigned char time2;   /* mlsb  of long time  */
    unsigned char time3;   /* lmsb  of long time  */
    unsigned char time4;   /* mmsb  of long time  */

    unsigned char alpha;   /* ***** WEIRD - stores Date and CTE calibration factors                   */
                           /* *****       - stored a byte at a time in a series of sample data blocks */
    unsigned char msb1;    /* 8 most significant bits - channel A */
    unsigned char msb2;    /* 8 most significant bits - channel B */
    unsigned char lsbs1;   /* 4 least significant bits - channel A (upper nibble), */
                           /* 4 least significant bits - channel B (lower nibble)  */
    struct {               /* packed 12 bit unsigned integers */
        unsigned char msb1;  /* 8 most significant bits - channel A */
        unsigned char msb2;  /* 8 most significant bits - channel B */
        unsigned char lsbs1; /* 4 least significant bits - channel A (upper nibble), */
                             /* 4 least significant bits - channel B (lower nibble)  */
        unsigned char msb3;  /* 8 most significant bits - channel A */
        unsigned char msb4;  /* 8 most significant bits - channel B */
        unsigned char lsbs2; /* 4 least significant bits - channel A (upper nibble), */
                             /* 4 least significant bits - channel B (lower nibble)  */
    } data[31];
    unsigned char msb3;    /* 8 most significant bits - channel A */
    unsigned char msb4;    /* 8 most significant bits - channel B */
    unsigned char lsbs2;   /* 4 least significant bits - channel A (upper nibble), */
                           /* 4 least significant bits - channel B (lower nibble)  */

/*  unsigned short checksum; /* initially 0x5a5a -> add each byte and roll left */
    unsigned char checksum1; /* lsb of checksum */
    unsigned char checksum2; /* msb of checksum */

    unsigned char alpha2;  
} CTE128_Sample;

typedef struct {
    unsigned char sync;    /* always 0xEF for CTE64_Sample */
    unsigned char alarm;   /* bit 0 - patient alarm, bit 1 indicates SzAC alarm, all others are 0 */

/*  unsigned long time;    /* time of day in 1/200th of a second increments */
    unsigned char time1;   /* llsb  of long time  - long stored in little endian format */
    unsigned char time2;   /* mlsb  of long time  */
    unsigned char time3;   /* lmsb  of long time  */
    unsigned char time4;   /* mmsb  of long time  */

    unsigned char alpha;   /* ***** WEIRD - stores Date and CTE calibration factors                   */
                           /* *****       - stored a byte at a time in a series of sample data blocks */

    unsigned char msb1;  /* 8 most significant bits - channel A */
    unsigned char msb2;  /* 8 most significant bits - channel B */
    unsigned char lsbs1; /* 4 least significant bits - channel A (upper nibble), */
                         /* 4 least significant bits - channel B (lower nibble)  */

    struct {               /* packed 12 bit unsigned integers */
        unsigned char msb1;  /* 8 most significant bits - channel A */
        unsigned char msb2;  /* 8 most significant bits - channel B */
        unsigned char lsbs1; /* 4 least significant bits - channel A (upper nibble), */
                             /* 4 least significant bits - channel B (lower nibble)  */

        unsigned char msb3;  /* 8 most significant bits - channel A */
        unsigned char msb4;  /* 8 most significant bits - channel B */
        unsigned char lsbs2; /* 4 least significant bits - channel A (upper nibble), */
                             /* 4 least significant bits - channel B (lower nibble)  */
    } data[15];

    unsigned char msb3;  /* 8 most significant bits - channel A */
    unsigned char msb4;  /* 8 most significant bits - channel B */
    unsigned char lsbs2; /* 4 least significant bits - channel A (upper nibble), */
                         /* 4 least significant bits - channel B (lower nibble)  */

/*  unsigned short checksum; /* initially 0x5a5a -> add each byte and roll left */
    unsigned char checksum1; /* lsb of checksum */
    unsigned char checksum2; /* msb of checksum */

    unsigned char alpha2;
} CTE64_Sample;

typedef union {
    unsigned char type;
    CTE64_Sample  cte64;
    CTE128_Sample  cte128;
} CTE_Sample;

typedef union {
    unsigned char *byte;
    CTE_Sample    *unknown;
    CTE64_Sample  *cte64;
    CTE128_Sample *cte128;
} Sample_p;

#define BLOCKSIZE     4096
#define SAMPLESIZE    1
#define Null(A)             ((A *) 0)
#define New(A)              ((A *) malloc( sizeof(A) ) )
#define NewBlock(A,N)       ((A *) malloc( sizeof(A) * (N)) )
#define BiggerBlock(A,B,N)  ((A *) realloc( (void *)(B), sizeof(A) * (N)))

/********* C code to read DC variables and Input channel descriptors *********/
/********* the printf statements are included for debugging purposes *********/
unsigned char seq_buf[20];
unsigned char DC_Values[8][240];
char DC_Labels[8][5];
char DC_Use[8];
int DC_Interval[8] = { 6, 6, 6, 6, 6, 6, 6, 6 };
float Slope[8];
float Intercept[8];
int ps = 3;   /* page size 0=3 sec, 1=6 sec, 2=12 sec, 3=24 sec  */
int sec_per_page[4] = { 3, 6, 12, 24 }; 

int SlowVarsCheck()
{
    int sumval = 0x5a5a;
	int real_checksum;
    int byte_count = 18;
	int i;
    int temp, rotbit;

    for (i=0; i<byte_count; i++ ){
        sumval += seq_buf[i] ;
        temp = sumval << 1;
        rotbit = (temp & 0x10000) >> 16;
        sumval = (temp & 0xfffe) | rotbit;
    }
	
	real_checksum = ((int) seq_buf[18] )<<8 + ((int) seq_buf[19] );

    return (sumval == real_checksum);
}

/* Alpha2 is the first byte after the checksum on a Beehive data packet */
void Slow_Vars(unsigned char Alpha2)
{
    char Descriptor[128][6];
    static char Byte_num = 0;
    static int sample = 0;
    int checksum_is_good, DCchan, i;
    float *fltptr;

    if((Alpha2 == (unsigned char)0xFE) && (Byte_num == 0))
        seq_buf[Byte_num++] = Alpha2;

    else if((Byte_num > 0) && (Byte_num < 20))
        seq_buf[Byte_num++] = Alpha2;

    if(Byte_num > 19) {
        checksum_is_good = SlowVarsCheck();

        if(checksum_is_good) {
            for(Byte_num = 1; Byte_num < 9; Byte_num++)
                DC_Values[Byte_num-1][sample] = seq_buf[Byte_num];
            if(++sample == (10 * sec_per_page[ps])) sample = 0;

            if((seq_buf[Byte_num] & 0x80) == 0) {         /* sequence 3 */
                if (report) fprintf(stderr, "SLOWVAR SEQUENCE 3");
                DCchan = seq_buf[Byte_num++] & 0x7F;
                for(i = 0; i < 6; i++)
                    Descriptor[DCchan][i] = seq_buf[Byte_num++];
                if (report) fprintf(stderr, "Descriptor[%d] = %s", DCchan, Descriptor[DCchan]); 
            }

            else if((seq_buf[Byte_num] & 0x40) == 0) {    /* sequence 2 */
                if (report) fprintf(stderr, "SLOWVAR SEQUENCE 2");
                DCchan = seq_buf[Byte_num++] & 0x07;
                for(i = 0; i < 5; i++)
                    DC_Labels[DCchan][i] = seq_buf[Byte_num++];
                if (report) fprintf(stderr, "DC_Labels[%d] = %s", DCchan, DC_Labels[DCchan]);
            }

            else {                                         /* sequence 1 */
                if (report) fprintf(stderr, "SLOWVAR SEQUENCE 1");
                DCchan = seq_buf[Byte_num] & 0x07;
                DC_Use[DCchan] = (char)((seq_buf[Byte_num++] & 0x08) ? 1 : 0);
                if (report) fprintf(stderr, "DC_Use[%d] = %d", DCchan, DC_Use[DCchan]);
                fltptr = (float *)&seq_buf[10];
                Slope[DCchan] = *fltptr;
                if (report) fprintf(stderr, "Slope[%d] = %f", DCchan, Slope[DCchan]);
                fltptr = (float *)&seq_buf[14];
                Intercept[DCchan] = *fltptr;
                if (report) fprintf(stderr, "Inter[%d] = %f", DCchan, Intercept[DCchan]);
            }
        }
        Byte_num = 0;
    }
}

/*
 * Check Sum ===============================================
 */

int checksum( Sample_p sample , enum CTE_Type the_type )
{
    int sumval = 0x5a5a;
    int byte_count, i;
    int temp, rotbit;

    if( the_type == CTE128 ){
        byte_count = sizeof(CTE128_Sample) - 3;
    } else if ( the_type == CTE64 ){
        byte_count = sizeof(CTE64_Sample) - 3;
    }    

    for (i=0; i<byte_count; i++ ){
        sumval += sample.byte[i] ;
        temp = sumval << 1;
        rotbit = (temp & 0x10000) >> 16;
        sumval = (temp & 0xfffe) | rotbit;
    }

    return sumval;
}

/*
 * Print for Debugging ========================================
 */
void print_debug(Sample_p sample , enum CTE_Type the_type, int index)
{
    int tempi, j;

	switch(the_type){
		case CTE128:
			printf("%d: ", index);
			printf("%x,",sample.cte128->sync);
			printf("%x,",sample.cte128->alarm);
			  tempi = (int) sample.cte128->time4;
			  tempi = (tempi << 8) + (int) sample.cte128->time3;
			  tempi = (tempi << 8) + (int) sample.cte128->time2;
			  tempi = (tempi << 8) + (int) sample.cte128->time1;
			printf("%d,", tempi );
			printf("'%c',",sample.cte128->alpha);
			printf(" [ ");
			printf("%d,", (((int) sample.cte128->msb1 )<< 4) 
						+ (((int) sample.cte128->lsbs1 & 0xf0) >> 4)  - 0x07ff);
			printf("%d,", (((int) sample.cte128->msb2 )<< 4)
						+ (((int) sample.cte128->lsbs1 & 0x0f))  - 0x07ff);
			for( j=0; j<31; j++ ){
				printf("%d,", (((int) sample.cte128->data[j].msb1 )<< 4) 
							+ (((int) sample.cte128->data[j].lsbs1 & 0xf0) >> 4)  - 0x07ff);
				printf("%d,", (((int) sample.cte128->data[j].msb2 )<< 4)
							+ (((int) sample.cte128->data[j].lsbs1 & 0x0f))  - 0x07ff);
				printf("%d,", (((int) sample.cte128->data[j].msb3 )<< 4) 
							+ (((int) sample.cte128->data[j].lsbs2 & 0xf0) >> 4)  - 0x07ff);
				printf("%d,", (((int) sample.cte128->data[j].msb4 )<< 4)
							+ (((int) sample.cte128->data[j].lsbs2 & 0x0f))  - 0x07ff);
			}
			printf("%d,", (((int) sample.cte128->msb3 )<< 4) 
						+ (((int) sample.cte128->lsbs2 & 0xf0) >> 4)  - 0x07ff);
			printf("%d,", (((int) sample.cte128->msb4 )<< 4)
						+ (((int) sample.cte128->lsbs2 & 0x0f))  - 0x07ff);
			printf(" ] ");
			printf("%x,",SHORT_OF(sample.cte128->checksum2, sample.cte128->checksum1));
			printf("%x,",sample.cte128->alpha2);
			printf("\n");
			break;
		case CTE64:
			printf("%d: ", index);
			printf("%x,",sample.cte64->sync);
			printf("%x,",sample.cte64->alarm);
			  tempi = (int) sample.cte64->time4;
			  tempi = (tempi << 8) + (int) sample.cte64->time3;
			  tempi = (tempi << 8) + (int) sample.cte64->time2;
			  tempi = (tempi << 8) + (int) sample.cte64->time1;
			printf("%d,", tempi );
			printf("'%c',",sample.cte64->alpha);
			printf(" [ ");
			printf("%d,", (((int) sample.cte64->msb1 )<< 4) 
						+ (((int) sample.cte64->lsbs1 & 0xf0) >> 4) - 0x07ff );
			printf("%d,", (((int) sample.cte64->msb2 )<< 4)
						+ (((int) sample.cte64->lsbs1 & 0x0f))  - 0x07ff );
			for( j=0; j<15; j++ ){
				printf("%d,", (((int) sample.cte64->data[j].msb1 )<< 4) 
							+ (((int) sample.cte64->data[j].lsbs1 & 0xf0) >> 4) - 0x07ff );
				printf("%d,", (((int) sample.cte64->data[j].msb2 )<< 4)
							+ (((int) sample.cte64->data[j].lsbs1 & 0x0f)) - 0x07ff );
				printf("%d,", (((int) sample.cte64->data[j].msb3 )<< 4) 
							+ (((int) sample.cte64->data[j].lsbs2 & 0xf0) >> 4) - 0x07ff );
				printf("%d,", (((int) sample.cte64->data[j].msb4 )<< 4)
							+ (((int) sample.cte64->data[j].lsbs2 & 0x0f)) - 0x07ff );
			}
			printf("%d,", (((int) sample.cte64->msb3 )<< 4) 
						+ (((int) sample.cte64->lsbs2 & 0xf0) >> 4)  - 0x07ff );
			printf("%d,", (((int) sample.cte64->msb4 )<< 4)
						+ (((int) sample.cte64->lsbs2 & 0x0f))  - 0x07ff );
			printf(" ] ");
			printf("%x,",SHORT_OF(sample.cte64->checksum2, sample.cte64->checksum1));
			printf("%x,",sample.cte64->alpha2);
			printf("\n");
			break;
	}
}

/*
 * Print for Plotting ========================================
 */
void print_gnuplot(Sample_p sample , enum CTE_Type the_type, int index)
{
    int tempi, j;

	switch(the_type){
		case CTE128:
			printf("%d ", index);
			  tempi = (int) sample.cte128->time4;
			  tempi = (tempi << 8) + (int) sample.cte128->time3;
			  tempi = (tempi << 8) + (int) sample.cte128->time2;
			  tempi = (tempi << 8) + (int) sample.cte128->time1;
			printf("%d ", tempi );
			printf("%d ", (((int) sample.cte128->msb1 )<< 4) 
						+ (((int) sample.cte128->lsbs1 & 0xf0) >> 4)  - 0x07ff);
			printf("%d ", (((int) sample.cte128->msb2 )<< 4)
						+ (((int) sample.cte128->lsbs1 & 0x0f))  - 0x07ff);
			for( j=0; j<31; j++ ){
				printf("%d ", (((int) sample.cte128->data[j].msb1 )<< 4) 
							+ (((int) sample.cte128->data[j].lsbs1 & 0xf0) >> 4)  - 0x07ff);
				printf("%d ", (((int) sample.cte128->data[j].msb2 )<< 4)
							+ (((int) sample.cte128->data[j].lsbs1 & 0x0f))  - 0x07ff);
				printf("%d ", (((int) sample.cte128->data[j].msb3 )<< 4) 
							+ (((int) sample.cte128->data[j].lsbs2 & 0xf0) >> 4)  - 0x07ff);
				printf("%d ", (((int) sample.cte128->data[j].msb4 )<< 4)
							+ (((int) sample.cte128->data[j].lsbs2 & 0x0f))  - 0x07ff);
			}
			printf("%d ", (((int) sample.cte128->msb3 )<< 4) 
						+ (((int) sample.cte128->lsbs2 & 0xf0) >> 4)  - 0x07ff);
			printf("%d ", (((int) sample.cte128->msb4 )<< 4)
						+ (((int) sample.cte128->lsbs2 & 0x0f))  - 0x07ff);
			printf("\n");
			break;
		case CTE64:
			printf("%d ", index);
			  tempi = (int) sample.cte64->time4;
			  tempi = (tempi << 8) + (int) sample.cte64->time3;
			  tempi = (tempi << 8) + (int) sample.cte64->time2;
			  tempi = (tempi << 8) + (int) sample.cte64->time1;
			printf("%d ", tempi );
			printf("%d ", (((int) sample.cte64->msb1 )<< 4) 
						+ (((int) sample.cte64->lsbs1 & 0xf0) >> 4) - 0x07ff );
			printf("%d ", (((int) sample.cte64->msb2 )<< 4)
						+ (((int) sample.cte64->lsbs1 & 0x0f))  - 0x07ff );
			for( j=0; j<15; j++ ){
				printf("%d ", (((int) sample.cte64->data[j].msb1 )<< 4) 
							+ (((int) sample.cte64->data[j].lsbs1 & 0xf0) >> 4) - 0x07ff );
				printf("%d ", (((int) sample.cte64->data[j].msb2 )<< 4)
							+ (((int) sample.cte64->data[j].lsbs1 & 0x0f)) - 0x07ff );
				printf("%d ", (((int) sample.cte64->data[j].msb3 )<< 4) 
							+ (((int) sample.cte64->data[j].lsbs2 & 0xf0) >> 4) - 0x07ff );
				printf("%d ", (((int) sample.cte64->data[j].msb4 )<< 4)
							+ (((int) sample.cte64->data[j].lsbs2 & 0x0f)) - 0x07ff );
			}
			printf("%d ", (((int) sample.cte64->msb3 )<< 4) 
						+ (((int) sample.cte64->lsbs2 & 0xf0) >> 4)  - 0x07ff );
			printf("%d ", (((int) sample.cte64->msb4 )<< 4)
						+ (((int) sample.cte64->lsbs2 & 0x0f))  - 0x07ff );
			printf("\n");
			break;
	}
}

/*
 * Print Binary ========================================
 */
void print_binary(Sample_p sample , enum CTE_Type the_type, int index)
{
    static int first_time = 1;
    int tempi, j;
    float datum;

    if( first_time ){
        switch(the_type){
            case CTE128:
                datum = 130.0;
                write(1, &datum, sizeof(float));
                break;
            case CTE64:
                datum = 66.0;
                write(1, &datum, sizeof(float));
                break;
        }
        first_time = 0;
    }
    /* note fd = 1 is the stdout */
	switch(the_type){
		case CTE128:
            datum = (float) index;
            write(1, &datum, sizeof(float));
			  tempi = (int) sample.cte128->time4;
			  tempi = (tempi << 8) + (int) sample.cte128->time3;
			  tempi = (tempi << 8) + (int) sample.cte128->time2;
			  tempi = (tempi << 8) + (int) sample.cte128->time1;
              datum = (float) tempi / 200.0;
            write(1, &datum, sizeof(float));
			  datum = ((((int) sample.cte128->msb1 )<< 4) 
						+ (((int) sample.cte128->lsbs1 & 0xf0) >> 4)  - 0x07ff);
            write(1, &datum, sizeof(float));
			  datum = ((((int) sample.cte128->msb2 )<< 4)
						+ (((int) sample.cte128->lsbs1 & 0x0f))  - 0x07ff);
            write(1, &datum, sizeof(float));

			for( j=0; j<31; j++ ){
				  datum = ((((int) sample.cte128->data[j].msb1 )<< 4) 
							+ (((int) sample.cte128->data[j].lsbs1 & 0xf0) >> 4)  - 0x07ff);
                write(1, &datum, sizeof(float));
				  datum = ((((int) sample.cte128->data[j].msb2 )<< 4)
							+ (((int) sample.cte128->data[j].lsbs1 & 0x0f))  - 0x07ff);
                write(1, &datum, sizeof(float));
				  datum = ((((int) sample.cte128->data[j].msb3 )<< 4) 
							+ (((int) sample.cte128->data[j].lsbs2 & 0xf0) >> 4)  - 0x07ff);
                write(1, &datum, sizeof(float));
				  datum = ((((int) sample.cte128->data[j].msb4 )<< 4)
							+ (((int) sample.cte128->data[j].lsbs2 & 0x0f))  - 0x07ff);
                write(1, &datum, sizeof(float));
			}
			  datum = ((((int) sample.cte128->msb3 )<< 4) 
						+ (((int) sample.cte128->lsbs2 & 0xf0) >> 4)  - 0x07ff);
            write(1, &datum, sizeof(float));
			  datum = ((((int) sample.cte128->msb4 )<< 4)
						+ (((int) sample.cte128->lsbs2 & 0x0f))  - 0x07ff);
            write(1, &datum, sizeof(float));
			break;
		case CTE64:
            datum = (float) index;
            write(1, &datum, sizeof(float));
			  tempi = (int) sample.cte64->time4;
			  tempi = (tempi << 8) + (int) sample.cte64->time3;
			  tempi = (tempi << 8) + (int) sample.cte64->time2;
			  tempi = (tempi << 8) + (int) sample.cte64->time1;
              datum = (float) tempi / 200.0;
            write(1, &datum, sizeof(float));
			  datum = ((((int) sample.cte64->msb1 )<< 4) 
						+ (((int) sample.cte64->lsbs1 & 0xf0) >> 4)  - 0x07ff);
            write(1, &datum, sizeof(float));
			  datum = ((((int) sample.cte64->msb2 )<< 4)
						+ (((int) sample.cte64->lsbs1 & 0x0f))  - 0x07ff);
            write(1, &datum, sizeof(float));

			for( j=0; j<15; j++ ){
				  datum = ((((int) sample.cte64->data[j].msb1 )<< 4) 
							+ (((int) sample.cte64->data[j].lsbs1 & 0xf0) >> 4)  - 0x07ff);
                write(1, &datum, sizeof(float));
				  datum = ((((int) sample.cte64->data[j].msb2 )<< 4)
							+ (((int) sample.cte64->data[j].lsbs1 & 0x0f))  - 0x07ff);
                write(1, &datum, sizeof(float));
				  datum = ((((int) sample.cte64->data[j].msb3 )<< 4) 
							+ (((int) sample.cte64->data[j].lsbs2 & 0xf0) >> 4)  - 0x07ff);
                write(1, &datum, sizeof(float));
				  datum = ((((int) sample.cte64->data[j].msb4 )<< 4)
							+ (((int) sample.cte64->data[j].lsbs2 & 0x0f))  - 0x07ff);
                write(1, &datum, sizeof(float));
			}
			  datum = ((((int) sample.cte64->msb3 )<< 4) 
						+ (((int) sample.cte64->lsbs2 & 0xf0) >> 4)  - 0x07ff);
            write(1, &datum, sizeof(float));
			  datum = ((((int) sample.cte64->msb4 )<< 4)
						+ (((int) sample.cte64->lsbs2 & 0x0f))  - 0x07ff);
            write(1, &datum, sizeof(float));
			break;
	}
}

/*
 * Add user defined printing routine here
 */
int print_channel(Sample_p sample, enum CTE_Type the_type, int index, int channel )
{
    float datum;
	int j;

  /*
   * New Printing Code for each data type
   */
    switch( the_type ){
	    case CTE128:
            if( (channel < 1) || (channel >128) ){
			    fprintf(stderr,"Channel %d out of range for format CTE128!\n", channel);
				abort();
			}
			if( channel == 1 ){
			    datum = ((((int) sample.cte128->msb1 )<< 4) 
						+ (((int) sample.cte128->lsbs1 & 0xf0) >> 4)  - 0x07ff);
                write(1, &datum, sizeof(float));
			} else if (channel == 2) {
			    datum = ((((int) sample.cte128->msb2 )<< 4)
						+ (((int) sample.cte128->lsbs1 & 0x0f))  - 0x07ff);
                write(1, &datum, sizeof(float));
			} else if ( (channel > 2) && (channel < 127)) {
			    j = channel - 2;
			    switch ( (channel - 2) % 4 ){
				    case 0:
						datum = ((((int) sample.cte128->data[j].msb1 )<< 4) 
								+ (((int) sample.cte128->data[j].lsbs1 & 0xf0) >> 4)  - 0x07ff);
						write(1, &datum, sizeof(float));
					    break;
				    case 1:
						datum = ((((int) sample.cte128->data[j].msb2 )<< 4)
									+ (((int) sample.cte128->data[j].lsbs1 & 0x0f))  - 0x07ff);
						write(1, &datum, sizeof(float));
					    break;
				    case 2:
						datum = ((((int) sample.cte128->data[j].msb3 )<< 4) 
									+ (((int) sample.cte128->data[j].lsbs2 & 0xf0) >> 4)  - 0x07ff);
						write(1, &datum, sizeof(float));
					    break;
				    case 3:
						datum = ((((int) sample.cte128->data[j].msb4 )<< 4)
									+ (((int) sample.cte128->data[j].lsbs2 & 0x0f))  - 0x07ff);
						write(1, &datum, sizeof(float));
					    break;
				}
			} else if (channel == 127) {
			    datum = ((((int) sample.cte128->msb3 )<< 4) 
						+ (((int) sample.cte128->lsbs2 & 0xf0) >> 4)  - 0x07ff);
                write(1, &datum, sizeof(float));
			} else if (channel == 128) {
			    datum = ((((int) sample.cte128->msb4 )<< 4)
						+ (((int) sample.cte128->lsbs2 & 0x0f))  - 0x07ff);
                write(1, &datum, sizeof(float));
			}
            break;
        case CTE64:
            if( (channel < 1) || (channel >64) ){
			    fprintf(stderr,"Channel %d out of range for format CTE128!\n", channel);
				abort();
			}
			if( channel == 1 ){
			    datum = ((((int) sample.cte64->msb1 )<< 4) 
						+ (((int) sample.cte64->lsbs1 & 0xf0) >> 4)  - 0x07ff);
                write(1, &datum, sizeof(float));
			} else if (channel == 2) {
			    datum = ((((int) sample.cte64->msb2 )<< 4)
						+ (((int) sample.cte64->lsbs1 & 0x0f))  - 0x07ff);
                write(1, &datum, sizeof(float));
			} else if ( (channel > 2) && (channel < 63)) {
			    j = channel - 2;
			    switch ( (channel - 2) % 4 ){
				    case 0:
						datum = ((((int) sample.cte64->data[j].msb1 )<< 4) 
								+ (((int) sample.cte64->data[j].lsbs1 & 0xf0) >> 4)  - 0x07ff);
						write(1, &datum, sizeof(float));
					    break;
				    case 1:
						datum = ((((int) sample.cte64->data[j].msb2 )<< 4)
									+ (((int) sample.cte64->data[j].lsbs1 & 0x0f))  - 0x07ff);
						write(1, &datum, sizeof(float));
					    break;
				    case 2:
						datum = ((((int) sample.cte64->data[j].msb3 )<< 4) 
									+ (((int) sample.cte64->data[j].lsbs2 & 0xf0) >> 4)  - 0x07ff);
						write(1, &datum, sizeof(float));
					    break;
				    case 3:
						datum = ((((int) sample.cte64->data[j].msb4 )<< 4)
									+ (((int) sample.cte64->data[j].lsbs2 & 0x0f))  - 0x07ff);
						write(1, &datum, sizeof(float));
					    break;
				}
			} else if (channel == 63) {
			    datum = ((((int) sample.cte64->msb3 )<< 4) 
						+ (((int) sample.cte64->lsbs2 & 0xf0) >> 4)  - 0x07ff);
                write(1, &datum, sizeof(float));
			} else if (channel == 64) {
			    datum = ((((int) sample.cte64->msb4 )<< 4)
						+ (((int) sample.cte64->lsbs2 & 0x0f))  - 0x07ff);
                write(1, &datum, sizeof(float));
			}
            break;
    }
}

/*
 * --------------------------------------
 */

/*
 * Main Program ===============================================
 */

main(int argc,char **argv)
{
    int               verbose, stats, quiet, ignore, channel_num;
    int               flag, alpha_state, tempi, bad_chksum;
    long int          c,i,j,k, chksum;
    long int          blocksize;
    long int          data_count;
    long int          space_alloc;
    long int          sample_count, bad_count, total_count;
    long int          last_time;
    unsigned char     temp_type, temp_alarm;
    enum CTE_Type     the_cte_type;
    Sample_p          sample;
    unsigned char    *data;
	char              Date_String[12];
	unsigned short    Calibration_Table[128];
    char              alpha_string[130];
	int               alpha_index;
    int               fd, bytes_read;
    enum ostyle { ascii, binary, debug, channel } output;
	long int          startTime, finishTime;
	
	startTime = finishTime = -1;

    fd = 0;    /* note: stdin is always fd = 0 */
    verbose = 0;
    stats = 0;
    quiet = 0;
    ignore = 0;
	channel_num = 0;
    output = ascii;
    bzero( alpha_string, 20 );
        
  /*
   * Parse Command Line
   */
    for (c=1; c< argc; c++) {
        if ( !strcmp( argv[ c ],"-h") || !strcmp( argv[ c ],"-help")){
            usage:
            fprintf(stderr,USAGE,argv[0]);
            fprintf(stderr,DESCR_0);
            fprintf(stderr,DESCR_1);
            fprintf(stderr,DESCR_2);
            fprintf(stderr,DESCR_3);
            fprintf(stderr,DESCR_4);
            fprintf(stderr,DESCR_5);
            fprintf(stderr,DESCR_6);
            fprintf(stderr,DESCR_7);
            exit(0);
        } else if (!strcmp( argv[ c ],"-o")){
            c++;
            if ( !strcmp( argv[ c ],"ascii") ){
                output = ascii;
            } else if ( !strcmp( argv[ c ],"binary") ) {
                output = binary;
            } else if (!strcmp( argv[ c ],"debug")) {
                output = debug;
            } else if (!strcmp( argv[ c ],"channel")) {
                output = channel;
				c++;
				channel_num = atoi( argv[c] );
            } else {
                goto usage;
            }
        } else if (!strcmp( argv[ c ],"-i")){
            ignore = 1;
        } else if (!strcmp( argv[ c ],"-q")){
            quiet = 1;
        } else if (!strcmp( argv[ c ],"-v")){
            verbose = 1;
        } else if (!strcmp( argv[ c ],"-s")){
            stats = 1;
			report = 1;
        }
    }
    
    if( ignore && !quiet ) 
        fprintf( stderr, "WARNING: Ignoring Bad Checksums! Some data may be corrupt!\n");
       
    if( !quiet ){
        if( output == ascii ) {
            fprintf( stderr, "Output Style: ascii\n"); 
        } else if( output == binary ) {
            fprintf( stderr, "Output Style: binary\n"); 
        } else if( output == debug ) {
            fprintf( stderr, "Output Style: debug\n"); 
        } else if( output == channel ) {
            fprintf( stderr, "Output Style: channel %d\n", channel_num); 
        }
     }
  /* 
   * read in the first byte to determine type info!
   */
    k = 0;
    while( (bytes_read = read( fd, &temp_type, 1 )) == 1 ){
        if( temp_type == CTE128 || temp_type == CTE64 ){
            if ( (bytes_read = read( fd, &temp_alarm, 1 )) == 1 ){
                if((temp_alarm == 0)||(temp_alarm == 0x01)||(temp_alarm == 0x02)||(temp_alarm == 0x03)){
                    if( verbose ) fprintf(stderr,"Skipped %d bytes at start of input file.\n",k);
                    break;
                }
            } else {
                break;
            }
         }
        k+=2;
    }
    if( bytes_read == -1 ){
        perror(argv[0]);
        abort();
    }
    if( bytes_read == 0 ){
        fprintf(stderr,"%s: Unrecognized Data Type in input file.\n",argv[0]);
        abort();
    }

    switch( temp_type ){
        case CTE128:
            the_cte_type = CTE128;
            blocksize = sizeof(CTE128_Sample);
            break;
        case CTE64:
            the_cte_type = CTE64;
            blocksize = sizeof(CTE64_Sample);
            break;
        default:
            fprintf(stderr,"%s: Unknown Data Type %d\n",argv[0], temp_type);
            abort();
    }

  /* 
   * Allocate Space
   */
    sample_count = 0;
    bad_count = 0;
    total_count = 0;
    space_alloc = 0;
    data = (char *)0;
    
    if( data = NewBlock( char, blocksize*SAMPLESIZE ) ){
        space_alloc = blocksize*SAMPLESIZE;
    } else {
        perror(argv[0]);
        abort();
    };

    if (stats) {
        switch(the_cte_type){
            case CTE128:
                fprintf(stderr, "\nData is of type CTE128 with blocksize %d bytes.\n", blocksize);
                break;
            case CTE64:
                fprintf(stderr, "\nData is of type CTE64 with blocksize %d bytes.\n", blocksize);
                break;
        }
        fprintf(stderr, "Space Allocated for %d bytes.\n", space_alloc);
    }
        
    sample.unknown = (CTE_Sample *)data;
    data[0] = temp_type;
    data[1] = temp_alarm;
    alpha_state = 0;
    alpha_index = 0;
    last_time = -1;
  /* 
   * read in the rest of the data
   */
    while ((bytes_read = read( fd, &data[ 2 ], (blocksize - 2) ))
                       > 0 )
    {
    start_of_loop:
        sample_count++;
        total_count++;

        bad_chksum = 0;
        chksum = checksum( sample, the_cte_type );

        /*
        * Write out the Data in a new format Here ---------------
        */  
        switch(the_cte_type){
            case CTE128:
                if( chksum != SHORT_OF(sample.cte128->checksum2, sample.cte128->checksum1) ){
                    if( !quiet ) {
                        fprintf(stderr, "\n%d: Bad Checksum! %x != %x\n", sample_count,  chksum, 
                            SHORT_OF(sample.cte128->checksum2, sample.cte128->checksum1));
                    }
                    bad_chksum = 1;
                    bad_count ++;
                    if( !ignore ){
                        sample_count--;
                        break;
                    }
                }
                tempi = (int) sample.cte128->time4;
                tempi = (tempi << 8) + (int) sample.cte128->time3;
                tempi = (tempi << 8) + (int) sample.cte128->time2;
                tempi = (tempi << 8) + (int) sample.cte128->time1;
                if( last_time != -1 ){
                    if( abs( last_time - tempi ) > 1 ){
                        if( !quiet ){ 
                            fprintf(stderr,"Missing %d sample%c at index %d\n",
                                last_time - tempi - 1,
                                ( abs(tempi - last_time - 1) > 1 ) ? 's' : ' ', 
                                sample_count);
                            /* fprintf(stderr,"last_time == %d, now == %d\n",last_time, tempi); */
                        }
                    }
                }
                if ( (bad_chksum) && (last_time != -1) && ( (tempi - last_time < 0) || ( abs(tempi - last_time) > 1000 ) )){
                    if( !quiet ) {
						fprintf(stderr,"Attempting to correct bizarre time %d to %d at index %d\n",
                            tempi,
                            last_time + 1,
                            sample_count );
                    }
                    tempi = last_time + 1;
					last_time = -1;
					
                    /* note little endian formatting */
                    sample.cte128->time4 = BYTE4(tempi);
                    sample.cte128->time3 = BYTE3(tempi);
                    sample.cte128->time2 = BYTE2(tempi);
                    sample.cte128->time1 = BYTE1(tempi);
                } else {
                    last_time = tempi;
				}
				
                if( startTime == -1 ) 
				    startTime = tempi;
					
                switch( alpha_state ){
                    case 0:
                        if( sample.cte128->alpha == 0xdd )
                            alpha_state = 1;
                        if( sample.cte128->alpha == 0x77 )
                            alpha_state = 2;
                        break;
                    case 1:
                        alpha_string[alpha_index++] = sample.cte128->alpha ;
                        if( (sample.cte128->alpha == 0x00) || (alpha_index >= 11)) {
						    bcopy( alpha_string, Date_String, alpha_index);
                            alpha_state = 0;
                            alpha_index = 0;
                            if( verbose ) fprintf( stderr, "\ndate_string = %s\n", alpha_string);
                        }
                        break;
                    case 2:
                        alpha_string[alpha_index++] = sample.cte128->alpha ;
                        if( (sample.cte128->alpha == 0x00) || (alpha_index >= 256)) {
						    int i;
						    for( i=0; i< alpha_index; i+=2 ){
							    Calibration_Table[i/2] = SHORT_OF(alpha_string[i+1],alpha_string[i]);
							}
                            if( verbose ) {
							    fprintf( stderr, "\ncalibration_table = \n");
						        for( i=0; i< 128; i++ ){
							        fprintf( stderr, "%d - %d, ",i, Calibration_Table[i]);
							    }
							} 
                            alpha_state = 0;
                            alpha_index = 0;
                        }
					    break;
                }

                Slow_Vars( sample.cte128->alpha2 );

                if( output == debug ){
                    print_debug( sample, the_cte_type, sample_count );
                } else if (output == ascii ) {
                    print_gnuplot( sample, the_cte_type, sample_count );
                } else if (output == binary ) {
                    print_binary( sample, the_cte_type, sample_count );
                } else if (output == channel ) {
                    print_channel( sample, the_cte_type, sample_count, channel_num);
                } 
                break;
            case CTE64:
                if( chksum != SHORT_OF(sample.cte64->checksum2, sample.cte64->checksum1) ){
                   if( !quiet ) {
                        fprintf(stderr, "\n%d: Bad Checksum! %x != %x\n", sample_count,  chksum,
                            SHORT_OF(sample.cte64->checksum2, sample.cte64->checksum1));
                    }
                    bad_count ++;
                    bad_chksum = 1;
                    if( !ignore ){
                        sample_count--;
                        break;
                    }
                }
                tempi = (int) sample.cte64->time4;
                tempi = (tempi << 8) + (int) sample.cte64->time3;
                tempi = (tempi << 8) + (int) sample.cte64->time2;
                tempi = (tempi << 8) + (int) sample.cte64->time1;
                if( last_time != -1 ){
                    if( abs( last_time - tempi ) > 1 ){
                        if( !quiet ) {
                            fprintf(stderr,"Missing %d sample%c at index %d\n",
                                tempi - last_time - 1,
                                ( abs(tempi - last_time - 1) > 1 ) ? 's' : ' ', 
                                sample_count);
                            /* fprintf(stderr,"last_time == %d, now == %d\n",last_time, tempi);*/
                        }
                    }
                }
				
                if ( (bad_chksum) && (last_time != -1) && ( (tempi - last_time < 0) || ( abs(tempi - last_time) > 1000 ) )){
                    if( !quiet ) {
						fprintf(stderr,"Attempting to correct bizarre time %d to %d at index %d\n",
                            tempi,
                            last_time+1,
                            sample_count );
                    }
                    tempi = last_time + 1;
					last_time = -1;
				
                    /* note little endian formatting */
				
                    sample.cte64->time4 = BYTE4(tempi);
                    sample.cte64->time3 = BYTE3(tempi);
                    sample.cte64->time2 = BYTE2(tempi);
                    sample.cte64->time1 = BYTE1(tempi);

                } else {
				    last_time = tempi;
				}
					
                if( startTime == -1 ) 
				    startTime = tempi;

                switch( alpha_state ){
                    case 0:
                        if( sample.cte64->alpha == 0xdd )
                            alpha_state = 1;
                        if( sample.cte64->alpha == 0x77 )
                            alpha_state = 2;
                        break;
                    case 1:
                        alpha_string[alpha_index++] = sample.cte64->alpha ;
                        if( (sample.cte64->alpha == 0x00) || (alpha_index >= 11)) {
						    bcopy( alpha_string, Date_String, alpha_index);
                            alpha_state = 0;
                            alpha_index = 0;
                            if( verbose ) fprintf( stderr, "\ndate_string = %s\n", alpha_string);
                        }
                        break;
                    case 2:
                        alpha_string[alpha_index++] = sample.cte64->alpha ;
                        if( (sample.cte64->alpha == 0x00) || (alpha_index >= 128)) {
						    int i;
						    for( i=0; i< alpha_index; i+=2 ){
							    Calibration_Table[i/2] = SHORT_OF(alpha_string[i+1],alpha_string[i]);
							}
                            if( verbose ) {
							    fprintf( stderr, "\ncalibration_table = \n");
						        for( i=0; i< 64; i++ ){
							        fprintf( stderr, "%d - %d, ",i, Calibration_Table[i]);
							    }
							} 
                            alpha_state = 0;
                            alpha_index = 0;
                        }
					    break;
                }
				
                Slow_Vars( sample.cte64->alpha2 );

                if( output == debug ){
                    print_debug( sample, the_cte_type, sample_count );
                } else if (output == ascii) {
                    print_gnuplot( sample, the_cte_type, sample_count );
                } else if (output == binary) {
                    print_binary( sample, the_cte_type, sample_count );
                } else if (output == channel ) {
                    print_channel( sample, the_cte_type, sample_count, channel_num );
                } 
                break;
        }

       /*
        *  find next record - deal with bad samples and lost data
        */

        if( bad_chksum ){
         /*
          *  look for start of next record inside current sample
          */
          
           for(k=1; k<blocksize-1; k++ ){
               if( data[k] == the_cte_type ){
                    if((data[k+1] == 0)||(data[k+1] == 0x01)||(data[k+1] == 0x02)||(data[k+1] == 0x03)){
                        bcopy( &data[k], &data[0], blocksize - k );
                        if( (bytes_read = read( fd, &data[ blocksize - k ], k )) == -1){
                            perror(argv[0]);
                            abort();
                        } else {
                            fprintf(stderr, "Skipped %d bytes.\n", k);
                            goto start_of_loop;
                        }
                    }
                }
            }
           

            if( data[blocksize - 1] == the_cte_type ){

                /* fprintf(stderr, "Skipped %d bytes.\n", blocksize - 1); */
                temp_type = data[blocksize - 1];
                bytes_read = 1;
                k=0;
                goto next_loop;

            } else {

                /* fprintf(stderr, "Skipped %d bytes.\n", blocksize ); */
                goto else_clause;

            }

        } else {
            else_clause:
         /*
          *  discard to start of next sample
          */
            k = 0;
            while( (bytes_read = read( fd, &temp_type, 1 )) == 1 ){
                next_loop:
                if( temp_type == the_cte_type ){
                    if ( (bytes_read = read( fd, &temp_alarm, 1 )) == 1 ){
                        if((temp_alarm == 0)||(temp_alarm == 0x01)||(temp_alarm == 0x02)||(temp_alarm == 0x03)){
                            if( verbose ){
                                if( k != 0 ) fprintf(stderr,"):");
                                fprintf(stderr,"%d,",k);
                            }
                            break;
                        } else {
                            if( verbose ) {
                                if( k == 0 ) fprintf(stderr,"(");
                                fprintf(stderr,"%x,", temp_type );
                                fprintf(stderr,"%x,", temp_alarm );
                            }
                            k += 1;
                        }
                    } else {
                        break;
                    }
                } else {
                    if( verbose ) {
                        if( k == 0 ) fprintf(stderr,"(");
                        fprintf(stderr,"%x,", temp_type );
                    }
                }
                k += 1;
            }
            if( bytes_read == -1 ){
                perror(argv[0]);
                abort();
            }
            data[0] = temp_type;
            data[1] = temp_alarm;
        }


    }

	finishTime = tempi;
    
    if (stats) {
        fprintf(stderr, "%d samples recovered out of %d.\n", sample_count, total_count);
        fprintf(stderr, "%d samples were damaged.\n", bad_count);
		fprintf(stderr, "Date String = %s\n", Date_String);
		{
		    int sh,sm,ss, fh,fm,fs;
			sh = startTime/720000;  startTime -= sh* 720000;
			sm = startTime/12000;   startTime -= sm* 12000;
			ss = startTime/200;     startTime -= ss* 200;
			fh = finishTime/720000; finishTime -= fh* 720000;
			fm = finishTime/12000;  finishTime -= fm* 12000;
			fs = finishTime/200;    finishTime -= fs* 200;
		    fprintf(stderr, "Time %2.2d:%2.2d:%2.2d.%.2d - %2.2d:%2.2d:%2.2d.%.2d\n",sh,sm,ss, startTime/2,  fh,fm,fs, finishTime/2);
		}
		if( output == channel ){
		    fprintf(stderr, "Calibration Factor (channel %d) = %d\n", channel_num, Calibration_Table[channel_num-1]);
		} else {
			fprintf(stderr, "Calibration Table = \n");
			{
				int i;
				for(i=0; i<(the_cte_type == CTE64?64:128); i++){
					fprintf(stderr, "\t%d -> %d\n", i+1, Calibration_Table[i]);
				}
			}
		}
    }

    if( bytes_read == -1 ) {
        perror(argv[0]);
        abort();
    }

    free( data );
}
