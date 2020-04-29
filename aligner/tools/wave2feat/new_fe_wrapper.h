#define ON 1
#define OFF 0
#define NULL_CHAR '\0'
#define MAXCHARS 2048

#define WAV 1
#define RAW 2
#define NIST 3
#define MSWAV 4

#define LITTLE 1
#define BIG 2

#define FE_CONTROL_FILE_ERROR 1
#define FE_START_ERROR 2
#define FE_UNKNOWN_SINGLE_OR_BATCH 3
#define FE_INPUT_FILE_OPEN_ERROR 4
#define FE_INPUT_FILE_READ_ERROR 5
#define FE_INPUT_FILE_ALLOC_MEM_ERROR 6
#define FE_OUTPUT_FILE_WRITE_ERROR 7
#define FE_OUTPUT_FILE_OPEN_ERROR 8

#define COUNT_PARTIAL 1
#define COUNT_WHOLE 0
#define HEADER_BYTES 1024
/*
  #if defined(ALPHA) || defined(ALPHA_OSF1) || defined(alpha_osf1) || defined(__alpha) || defined(mips) 
*/
/*#define SWAPBYTES*/
#define SWAPW(x)        *(x) = ((0xff & (*(x))>>8) | (0xff00 & (*(x))<<8))
#define SWAPL(x)        *(x) = ((0xff & (*(x))>>24) | (0xff00 & (*(x))>>8) |\
                        (0xff0000 & (*(x))<<8) | (0xff000000 & (*(x))<<24))
#define SWAPF(x)        SWAPL((int *) x)


/* Some defines for MS Wav Files */
/* The MS Wav file is a RIFF file, and has the following 44 byte header */
typedef struct RIFFHeader{
    char rifftag[4];      /* "RIFF" string */
    int TotalLength;      /* Total length */
    char wavefmttag[8];   /* "WAVEfmt " string (note space after 't') */
    int RemainingLength;  /* Remaining length */
    short data_format;    /* data format tag, 1 = PCM */
    short numchannels;    /* Number of channels in file */
    int SamplingFreq;     /* Sampling frequency */
    int BytesPerSec;      /* Average bytes/sec */
    short BlockAlign;     /* Block align */
    short BitsPerSample;  /* 8 or 16 bit */
    char datatag[4];      /* "data" string */
    int datalength;       /* Raw data length */
} MSWAV_hdr;

    /* #else */ 
 /* don't need byte order conversion, do nothing */
    /*
      #undef NEEDS_SWAP
      #define SWAPW(x)    
      #define SWAPL(x)
      #define SWAPF(x)  
      #endif 
    */

param_t *fe_parse_options(int argc, char **argv);
void fe_init_params(param_t *P);
int32 fe_convert_files(param_t *P);
int32 fe_build_filenames(param_t *P, char *fileroot, char *infilename, char *outfilename);
char *fe_copystr(char *dest_str, char *src_str);
int32 fe_count_frames(fe_t *FE, int32 nsamps, int32 count_partial_frames);
int32 fe_readspch(param_t *P, char *infile, int16 **spdata, int32 *splen);
int32 fe_writefeat(fe_t *FE, char *outfile, int32 nframes, float32 **feat);
int32 fe_free_param(param_t *P);
int32 fe_openfiles(param_t *P, fe_t *FE, char *infile, int32 *fp_in, int32 *nsamps, 
		   int32 *nframes, int32 *nblocks, char *outfile, int32 *fp_out);
int32 fe_readblock_spch(param_t *P, int32 fp, int32 nsamps, int16 *buf);
int32 fe_writeblock_feat(param_t *P, fe_t *FE, int32 fp, int32 nframes, float32 **feat);
int32 fe_closefiles(int32 fp_in, int32 fp_out);
int32 fe_dither(int16 *buffer,int32 nsamps);

