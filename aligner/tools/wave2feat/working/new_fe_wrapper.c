#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "new_fe.h"
#include "new_fe_sp.h"
#include "new_fe_wrapper.h"

/*       
	 7-Feb-00 M. Seltzer - wrapper created for new front end -
	 does blockstyle processing if necessary. If input stream is
	 greater than DEFAULT_BLOCKSIZE samples (currently 200000)
	 then it will read and write in DEFAULT_BLOCKSIZE chunks. 
	 
	 Had to change fe_process_utt(). Now the 2d feature array
	 is allocated internally to that function rather than
	 externally in the wrapper. 
	 
	 Added usage display with -help switch for help

	 14-Feb-00 M. Seltzer - added NIST header parsing for 
	 big endian/little endian parsing. kind of a hack.

	 changed -wav switch to -nist to avoid future confusion with
	 MS wav files
	 
	 added -mach_endian switch to specify machine's byte format
*/

int32 main(int32 argc, char **argv)
{
    param_t *P;

    P = fe_parse_options(argc,argv);
    if (fe_convert_files(P)){
	fprintf(stderr,"error converting files...exiting\n");
	return(-1);
    }
    
    fe_free_param(P);
    return(0);
}


int32 fe_convert_files(param_t *P)
{

    fe_t *FE;
    char infile[MAXCHARS],outfile[MAXCHARS], fileroot[MAXCHARS];
    FILE *ctlfile;
    int16 *spdata=NULL;
    int32 splen,rc,i,total_samps,frames_proc,nframes,nblocks,last_frame,numcep;
    int32 fp_in,fp_out, last_blocksize,curr_block,total_frames;
    float32 **cep, **last_frame_cep;
    
    if ((FE = fe_init(P))==NULL){
	fprintf(stderr,"memory alloc failed in fe_convert_files()\n...exiting\n");
	exit(0);
    }
    
    if (P->is_batch){
	if ((ctlfile = fopen(P->ctlfile,"r")) == NULL){
	    fprintf(stderr,"Unable to open control file %s\n",P->ctlfile);
	    return(FE_CONTROL_FILE_ERROR);
	}
	while (fscanf(ctlfile,"%s",fileroot)!=EOF){
	    fe_build_filenames(P,fileroot,infile,outfile);
	    
	    fe_openfiles(P,FE,infile,&fp_in,&total_samps,&nframes,&nblocks,outfile,&fp_out);

	    if (nblocks*P->blocksize>=total_samps) 
		last_blocksize = total_samps - (nblocks-1)*P->blocksize;
	    
	    if (P->verbose) printf("%s\n",infile);
	    
	    if (!fe_start_utt(FE)){
		curr_block=1;
		total_frames=frames_proc=0;
		/*execute this loop only if there is more than 1 block to
		  be processed */
		while(curr_block < nblocks){
		    splen = P->blocksize;
		    if ((spdata = (int16 *)calloc(splen,sizeof(int16)))==NULL){
			fprintf(stderr,"Unable to allocate memory block of %d shorts for input speech\n",splen);
			exit(0);
		    } 
		    if (fe_readblock_spch(P,fp_in,splen,spdata)!=splen){
			fprintf(stderr,"error reading speech data\n");
			exit(0);
		    }
		    frames_proc = fe_process_utt(FE,spdata,splen,&cep);
		    if (frames_proc>0)
			fe_writeblock_feat(P,FE,fp_out,frames_proc,cep);
		    curr_block++;
		    total_frames += frames_proc;
		    
		}
		/* process last (or only) block */
		if (spdata!=NULL) free(spdata);
		splen=last_blocksize;
		
		if ((spdata = (int16 *)calloc(splen,sizeof(int16)))==NULL){
		    fprintf(stderr,"Unable to allocate memory block of %d shorts for input speech\n",splen);
		    exit(0);
		} 

		if (fe_readblock_spch(P,fp_in,splen,spdata)!=splen){
		    fprintf(stderr,"error reading speech data\n");
		    exit(0);
		}
		
		frames_proc = fe_process_utt(FE,spdata,splen,&cep);
		if (frames_proc>0)
		    fe_writeblock_feat(P,FE,fp_out,frames_proc,cep);
		
		curr_block++;
		if (P->logspec != ON)
		    last_frame_cep = (float32 **)fe_create_2d(1,FE->NUM_CEPSTRA,sizeof(float32));
		else
		    last_frame_cep = (float32 **)fe_create_2d(1,FE->FB->num_filters,sizeof(float32));
		last_frame = fe_end_utt(FE, last_frame_cep[0]);
		if (last_frame>0){
		    fe_writeblock_feat(P,FE,fp_out,last_frame,last_frame_cep);
		    frames_proc++;
		}
		total_frames += frames_proc;
		
		fe_closefiles(fp_in,fp_out);		
		free(spdata);
		fe_free_2d((void **)last_frame_cep);
		
	    }
	    else{
		fprintf(stderr,"fe_start_utt() failed\n");
		return(FE_START_ERROR);
	    }
	}
	fe_close(FE);
    }
    
    else if (P->is_single){
	
	fe_build_filenames(P,fileroot,infile,outfile);
	if (P->verbose) printf("%s\n",infile);

	fe_openfiles(P,FE,infile,&fp_in,&total_samps,&nframes,&nblocks,outfile,&fp_out);
	
	if (nblocks*P->blocksize>=total_samps) 
	    last_blocksize = total_samps - (nblocks-1)*P->blocksize;
	
	if (!fe_start_utt(FE)){
	    curr_block=1;
	    total_frames=frames_proc=0;
	    /*execute this loop only if there are more than 1 block to
	      be processed */
	    while(curr_block < nblocks){
		splen = P->blocksize;
		if ((spdata = (int16 *)calloc(splen,sizeof(int16)))==NULL){
		    fprintf(stderr,"Unable to allocate memory block of %d shorts for input speech\n",splen);
		    exit(0);
		} 
		if (fe_readblock_spch(P,fp_in,splen,spdata)!=splen){
		    fprintf(stderr,"Error reading speech data\n");
		    exit(0);
		}
		frames_proc = fe_process_utt(FE,spdata,splen,&cep);
		if (frames_proc>0)
		    fe_writeblock_feat(P,FE,fp_out,frames_proc,cep);
		curr_block++;
		total_frames += frames_proc;
		
	    }
	    /* process last (or only) block */
	    if (spdata!=NULL) free(spdata);
	    splen =last_blocksize;
	    if ((spdata = (int16 *)calloc(splen,sizeof(int16)))==NULL){
		fprintf(stderr,"Unable to allocate memory block of %d shorts for input speech\n",splen);
		exit(0);
	    } 
	    if (fe_readblock_spch(P,fp_in,splen,spdata)!=splen){
		fprintf(stderr,"Error reading speech data\n");
		exit(0);
	    }
	    frames_proc = fe_process_utt(FE,spdata,splen,&cep);
	    if (frames_proc>0)
		fe_writeblock_feat(P,FE,fp_out,frames_proc,cep);
	    
	    curr_block++;
	    if (P->logspec != ON)
	        last_frame_cep = (float32 **)fe_create_2d(1,FE->NUM_CEPSTRA,sizeof(float32));
	    else
	        last_frame_cep = (float32 **)fe_create_2d(1,FE->FB->num_filters,sizeof(float32));
	    last_frame = fe_end_utt(FE, last_frame_cep[0]);
	    if (last_frame>0){
		fe_writeblock_feat(P,FE,fp_out,last_frame,last_frame_cep);
		frames_proc++;
	    }
	    total_frames += frames_proc;
	    	    
	    fe_closefiles(fp_in,fp_out);
	    
	    free(spdata);
	    fe_free_2d((void **)last_frame_cep);
	}
	else{
	    fprintf(stderr,"fe_start_utt() failed\n");
	    return(FE_START_ERROR);
	}
	
	fe_close(FE);
    }
    else{
	fprintf(stderr,"Unknown mode - single or batch?\n");
	return(FE_UNKNOWN_SINGLE_OR_BATCH);
	
    }
    
    return(0);
    
}

param_t *fe_parse_options(int32 argc, char **argv) 
{
    int32 i;
    param_t *P;
    
    fprintf(stderr,"args: \n");
    if ((P=(param_t *)malloc(sizeof(param_t)))==NULL){
	fprintf(stderr,"memory alloc failed in fe_parse_options()\n...exiting\n");
	exit(0);
    }
    
    if (argc == 1){
	fprintf(stderr,"No Command Line Arguments given\n");
	fe_usage(argv);
    }
	    
    fe_init_params(P);
    
    for (i = 1; i < argc; ++i)
    {
	if (!strcmp("-i",argv[i])){
            P->wavfile = argv[++i]; 
	    P->is_single = ON;
	    fprintf(stderr,"-i %s ",P->wavfile);
	}
	else if (!strcmp("-o",argv[i])){
            P->cepfile = argv[++i];
	    fprintf(stderr,"-o %s ",P->cepfile);
	}
	else if (!strcmp("-c",argv[i])){
            P->ctlfile = argv[++i];
	    P->is_batch = ON;
	    fprintf(stderr,"-c %s ",P->ctlfile);
	}
	else if (!strcmp("-di",argv[i])){
            P->wavdir = argv[++i];
	    fprintf(stderr,"-di %s ",P->wavdir);
	}
	else if (!strcmp("-do",argv[i])){
	    P->cepdir = argv[++i];
	    fprintf(stderr,"-do %s ",P->cepdir);
	}
	else if (!strcmp("-ei",argv[i])){
	    P->wavext = argv[++i];
	    fprintf(stderr,"-ei %s ",P->wavext);
	}
        else if (!strcmp("-eo",argv[i])){
	    P->cepext = argv[++i];
	    fprintf(stderr,"-eo %s ",P->cepext);
	}
	else if (!strcmp("-wav",argv[i])){
	    P->input_format = WAV;
	}
	else if (!strcmp("-nist",argv[i])){
	    P->input_format = NIST;
	}
	else if (!strcmp("-mswav",argv[i])){
	    P->input_format = MSWAV;
	}
	else if (!strcmp("-raw",argv[i])){
	    P->input_format = RAW;
	}
	else if (!strcmp("-raw_endian",argv[i])){
	    if (!strcmp("big",argv[++i]))
		P->raw_endian = BIG;
	    else if (!strcmp("little",argv[i]))
		P->raw_endian = LITTLE;
	    else{
		fprintf(stderr,"Raw data must be big or little Endian\n");
		exit(0);
	    }	
	}
	else if (!strcmp("-nchans",argv[i])){
	    P->nchans = atoi(argv[++i]);
	}
	else if (!strcmp("-whichchan",argv[i])){
	    P->whichchan = atoi(argv[++i]);
	}
	else if (!strcmp("-alpha",argv[i])){
	    P->PRE_EMPHASIS_ALPHA = atof(argv[++i]);
	}
	else if (!strcmp("-srate",argv[i])){
	    P->SAMPLING_RATE = atof(argv[++i]);
	}	
	else if (!strcmp("-wlen",argv[i])){
	    P->WINDOW_LENGTH = atof(argv[++i]);
	}
	else if (!strcmp("-frate",argv[i])){
	    P->FRAME_RATE = atof(argv[++i]);
	}
	else if (!strcmp("-warp",argv[i])){
	    if (!strcmp("mel",argv[++i])){
		P->FB_TYPE = MEL_SCALE;
	    }
	    else if (!strcmp("linear",argv[i])){
		P->FB_TYPE = LINEAR;
	    }
	    else{
		fprintf(stderr,"MEL_SCALE AND LINEAR ARE CURRENTLY THE ONLY IMPLEMENTATIONS\n");
		exit(0);
	    }	
	}
	else if (!strcmp("-feat",argv[i])){
	    if (!strcmp("sphinx",argv[++i])){
		P->output_endian = BIG;
	    }
	    else{
		fprintf(stderr,"sphinx FORMAT IS THE ONLY CURRENT OPTION FOR -feat\n");
		exit(0);
	    }	
	}		    
	else if (!strcmp("-nfilt",argv[i])){
	    P->NUM_FILTERS = atoi(argv[++i]);
	}
        else if (!strcmp("-ncep",argv[i])){
	    P->NUM_CEPSTRA = atoi(argv[++i]);
	}
        else if (!strcmp("-lowerf",argv[i])){
	    P->LOWER_FILT_FREQ = atof(argv[++i]);
	}
        else if (!strcmp("-upperf",argv[i])){
	    P->UPPER_FILT_FREQ = atof(argv[++i]);
	}
	else if (!strcmp("-nfft",argv[i])){
	    P->FFT_SIZE = atoi(argv[++i]);
	}
	else if (!strcmp("-doublebw",argv[i])){
	    P->doublebw = ON;
	}
	else if (!strcmp("-blocksize",argv[i])){
	    P->blocksize = atoi(argv[++i]);
	}
	else if (!strcmp("-verbose",argv[i])){
	    P->verbose = ON;
	}
	else if (!strcmp("-mach_endian",argv[i])){
	    if (!strcmp("big",argv[++i]))
		P->machine_endian = BIG;
	    else if (!strcmp("little",argv[i]))
		P->machine_endian = LITTLE;
	    else{
		fprintf(stderr,"Machine must be big or little Endian\n");
		exit(0);
	    }	
	}
	else if (!strcmp("-dither",argv[i])){
	    P->dither = ON;
	}
	else if (!strcmp("-logspec",argv[i])){
	    P->logspec = ON;
	}
	else if (!strcmp("-help",argv[i])){
	    fe_usage(argv);
	}
        else{
            fprintf(stderr,"%s - unknown argument\n", argv[i]);
	    exit(0);
        }
    }
    if (P->wavfile == NULL && P->wavdir == NULL){
	fprintf(stderr,"No input file or file directory given\n");
	exit(0);
    }
    if (P->cepfile == NULL && P->cepdir == NULL){
	fprintf(stderr,"No cepstra file or file directory given\n");
	exit(0);
    }
    if (P->ctlfile==NULL && P->cepfile==NULL && P->wavfile==NULL){
	fprintf(stderr,"No contol file given\n");
	exit(0);
    }
    if (P->nchans>1){
	fprintf(stderr,"Files have %d channels of data\nWill extract features for channel %d\n",P->nchans,P->whichchan);
    }

    if (P->doublebw==ON)
	fprintf(stderr,"Will use double bandwidth filters\n");

    fprintf(stderr,"\n");
    
    return (P);
}


void fe_init_params(param_t *P)
{
    P->SAMPLING_RATE = DEFAULT_SAMPLING_RATE;
    P->FRAME_RATE = DEFAULT_FRAME_RATE;
    P->WINDOW_LENGTH = DEFAULT_WINDOW_LENGTH;
    P->FB_TYPE = DEFAULT_FB_TYPE;
    P->PRE_EMPHASIS_ALPHA = DEFAULT_PRE_EMPHASIS_ALPHA;
    P->NUM_CEPSTRA = DEFAULT_NUM_CEPSTRA;
    P->FFT_SIZE = DEFAULT_FFT_SIZE;
    P->NUM_FILTERS = DEFAULT_NUM_FILTERS;
    P->UPPER_FILT_FREQ = DEFAULT_UPPER_FILT_FREQ;
    P->LOWER_FILT_FREQ = DEFAULT_LOWER_FILT_FREQ;
    P->blocksize = DEFAULT_BLOCKSIZE;
    P->verbose = OFF;
    P->input_format=NIST;
    P->input_endian = LITTLE;
    P->machine_endian = LITTLE;
    P->raw_endian = LITTLE;
    P->output_endian = BIG;
    P->dither = OFF;
    P->wavfile = NULL;
    P->cepfile = NULL;
    P->ctlfile = NULL;
    P->wavdir = NULL;
    P->cepdir = NULL;
    P->wavext = NULL;
    P->cepext = NULL;
    P->nchans = ONE_CHAN;
    P->whichchan = ONE_CHAN;
    P->doublebw = OFF;
    
  
    
}


int32 fe_build_filenames(param_t *P, char *fileroot, char *infilename, char *outfilename)
{
    char cbuf[MAXCHARS];
    char chanlabel[MAXCHARS];
    
    if (P->nchans>1)
	sprintf(chanlabel,".ch%d",P->whichchan);
    
    if (P->is_batch){
	sprintf(cbuf,"");
	strcat(cbuf,P->wavdir);
	strcat(cbuf,"/");
	strcat(cbuf,fileroot);
	strcat(cbuf,".");
	strcat(cbuf,P->wavext);
	strcpy(infilename,cbuf);
	fprintf(stderr,"Input file is %s\n",infilename);
	
	sprintf(cbuf,"");
	strcat(cbuf,P->cepdir);
	strcat(cbuf,"/");
	strcat(cbuf,fileroot);
	if (P->nchans>1)
	    strcat(cbuf,chanlabel);
	strcat(cbuf,".");
	strcat(cbuf,P->cepext);
	strcpy(outfilename,cbuf);
	fprintf(stderr,"Output file is %s\n",outfilename);
    }
    else if (P->is_single){
	sprintf(cbuf,"");
	strcat(cbuf,P->wavfile);
	strcpy(infilename,cbuf);
	fprintf(stderr,"Input file is %s\n",infilename);

	sprintf(cbuf,"");
	strcat(cbuf,P->cepfile);
	strcpy(outfilename,cbuf);
	fprintf(stderr,"Output file is %s\n",outfilename);
    }
    else{
	fprintf(stderr,"Unspecified Batch or Single Mode\n");
    }
    
    
}

char *fe_copystr(char *dest_str, char *src_str)
{
    int i,src_len, dest_len, len;
    char *s;
    
    src_len = strlen(src_str);
    len = src_len;
    s = (char *)malloc((char)(len+1));
    for (i=0;i<src_len;i++)
	*(s+i) = *(src_str+i);
    *(s+src_len) = NULL_CHAR;
    
    return(s);
}

int32 fe_count_frames(fe_t *FE, int32 nsamps, int32 count_partial_frames)
{
    int32 frame_start,frame_count = 0;
    
    for (frame_start=0;frame_start+FE->FRAME_SIZE<=nsamps;
	 frame_start+=FE->FRAME_SHIFT)
	frame_count++;
   
    if (count_partial_frames){
	if (frame_count*FE->FRAME_SHIFT < nsamps)
	    frame_count++;
    }
    
    return(frame_count);
}
	    
int32 fe_openfiles(param_t *P, fe_t *FE, char *infile, int32 *fp_in, int32 *nsamps, 
		   int32 *nframes, int32 *nblocks, char *outfile, int32 *fp_out)
{
    struct stat filestats;
    int fp=0, len=0, header_read, outlen, numframes, numblocks;
    FILE *fp2;
    char line[MAXCHARS];
    int got_it=0;

    /* Note: this is kind of a hack to read the byte format from the
       NIST header */
    if (P->input_format == NIST){
        if ((fp2 = fopen(infile,"r")) == NULL){
	    fprintf(stderr,"Cannot read %s\n",infile);
	    return(FE_INPUT_FILE_READ_ERROR);
	}
	*line=NULL;
	got_it = 0;
	while(strcmp(line,"end_head") && !got_it){
	    fscanf(fp2,"%s",line);
	    if (!strcmp(line,"sample_byte_format")){
		fscanf(fp2,"%s",line);
		if (!strcmp(line,"-s2")){
		    fscanf(fp2,"%s",line);
		    if (!strcmp(line,"01")){
			P->input_endian=LITTLE;
			got_it=1;
		    }
		    else if(!strcmp(line,"10")){
			P->input_endian=BIG;
			got_it = 1;
		    }
		    else
			fprintf(stderr,"Unknown/unsupported byte order\n");	
		}
		else 
		    fprintf(stderr,"Error determining byte format\n");
	    }
	}
	if (!got_it){
	    fprintf(stderr,"Can't find byte format in header, setting to machine's endian\n");
	    P->input_endian = P->machine_endian;
	}	    
	fclose(fp2);
    }
    else if (P->input_format == RAW){
	P->input_endian = P->machine_endian;
    }
    else if (P->input_format == MSWAV){
	P->input_endian = LITTLE; // Default for MS WAV riff files
    }
    
    
    if ((fp = open(infile, O_RDONLY, 0644))<0){
        fprintf(stderr,"Cannot open %s\n",infile);
  	return (FE_INPUT_FILE_OPEN_ERROR);
    }
    else{
        if (fstat(fp,&filestats)!=0) printf("fstat failed\n");
	
	if (P->input_format == NIST){
            short *hdr_buf;

	    len = (filestats.st_size-HEADER_BYTES)/sizeof(short);
	    /* eat header */
	    hdr_buf = (short *)calloc(HEADER_BYTES/sizeof(short),sizeof(short));
	    if (read(fp,hdr_buf,HEADER_BYTES)!=HEADER_BYTES){
		fprintf(stderr,"Cannot read %s\n",infile);
		return (FE_INPUT_FILE_READ_ERROR);
	    }
	    free(hdr_buf);    
	}
	else if (P->input_format == RAW){
	    len = filestats.st_size/sizeof(int16);
	}
	else if (P->input_format == MSWAV){
            /* Read the header */
            MSWAV_hdr *hdr_buf;
	    if ((hdr_buf = (MSWAV_hdr*) calloc(1,sizeof(MSWAV_hdr))) == NULL){
	        fprintf(stderr,"Cannot allocate for input file header\n");
	        return (FE_INPUT_FILE_READ_ERROR);
            }
            if (read(fp,hdr_buf,sizeof(MSWAV_hdr)) != sizeof(MSWAV_hdr)){
	        fprintf(stderr,"Cannot allocate for input file header\n");
	        return (FE_INPUT_FILE_READ_ERROR);
            }
	    /* Check header */
	    if (strncmp(hdr_buf->rifftag,"RIFF",4)!=0 ||
	           strncmp(hdr_buf->wavefmttag,"WAVEfmt",7)!=0 ||
	           strncmp(hdr_buf->datatag,"data",4)!=0) {
	        fprintf(stderr,"Error in mswav file header\n");
	        return (FE_INPUT_FILE_READ_ERROR);
            }
            if (P->input_endian!=P->machine_endian) { // If machine is Big Endian
                hdr_buf->datalength = SWAPL(&(hdr_buf->datalength));
	        hdr_buf->data_format = SWAPW(&(hdr_buf->data_format));
                hdr_buf->numchannels = SWAPW(&(hdr_buf->numchannels));
                hdr_buf->BitsPerSample = SWAPW(&(hdr_buf->BitsPerSample));
                hdr_buf->SamplingFreq = SWAPL(&(hdr_buf->SamplingFreq));
	        hdr_buf->BytesPerSec = SWAPL(&(hdr_buf->BytesPerSec));
	    }
	    /* Check Format */
	    if (hdr_buf->data_format != 1 || hdr_buf->BitsPerSample != 16){
                fprintf(stderr,"MS WAV file not in 16-bit PCM format\n");
		return (FE_INPUT_FILE_READ_ERROR);
	    }
	    len = hdr_buf->datalength / sizeof(short);
	    P->nchans = hdr_buf->numchannels;
	    /* DEBUG: Dump Info */
	    fprintf(stderr,"Reading MS Wav file %s:\n", infile);
	    fprintf(stderr,"\t16 bit PCM data, %d channels %d samples\n",P->nchans,len);
	    fprintf(stderr,"\tSampled at %d\n",hdr_buf->SamplingFreq);
	    free(hdr_buf);
	}
	else {
	    fprintf(stderr,"Unknown input file format\n");
	    return(FE_INPUT_FILE_OPEN_ERROR);
	}
    }


    len = len/P->nchans;
    *nsamps = len;
    *fp_in = fp;

    numblocks = (int)((float)len/(float)P->blocksize);
    if (numblocks*P->blocksize<len)
	numblocks++;

    *nblocks = numblocks;  

    if ((fp = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0644)) < 0) {
        printf("Unable to open %s for writing features\n",outfile);
        return(FE_OUTPUT_FILE_OPEN_ERROR);
    }
    else{
	/* compute number of frames and write cepfile header */
	numframes = fe_count_frames(FE,len,COUNT_PARTIAL);
	if (P->logspec != ON)
	    outlen = numframes*FE->NUM_CEPSTRA;
	else
	    outlen = numframes*FE->FB->num_filters;
	if  (write(fp, &outlen, 4) != 4) {
	    printf("Data write error on %s\n",outfile);
	    close(fp);
	    return(FE_OUTPUT_FILE_WRITE_ERROR);
	}
    }

    *nframes = numframes;  
    *fp_out = fp;
    
}

int32 fe_readblock_spch(param_t *P, int32 fp, int32 nsamps, int16 *buf)
{
    int32 bytes_read, cum_bytes_read, nreadbytes, actsamps, offset, i, j, k;
    int16 *tmpbuf;
    int32 nchans, whichchan;
    
    nchans = P->nchans;
    whichchan = P->whichchan;
    
    if (nchans==1){
	
	if (P->input_format==RAW || P->input_format==NIST || P->input_format==MSWAV){ 
	    nreadbytes = nsamps*sizeof(int16);
	    if ((bytes_read = read(fp,buf,nreadbytes))!=nreadbytes){
		fprintf(stderr,"error reading block\n");
		return(0);
	    }
	}
	else{
	    fprintf(stderr,"unknown input file format\n");
	    return(0);
	}   
	cum_bytes_read = bytes_read;
    }
    
    else if (nchans>1){
	
	if (nsamps<P->blocksize){
	    actsamps = nsamps*nchans;
	    tmpbuf = (int16 *)calloc(nsamps*nchans,sizeof(int16));
	    cum_bytes_read = 0;
	    if (P->input_format==RAW || P->input_format==NIST){    
		
		k=0;
		nreadbytes = actsamps*sizeof(int16);
		
		if ((bytes_read = read(fp,tmpbuf,nreadbytes))!=nreadbytes){
		    fprintf(stderr,"error reading block (got %d not %d)\n",bytes_read,nreadbytes);
		    return(0);
		}
		
		for (j=whichchan-1;j<actsamps;j=j+nchans){
		    buf[k] = tmpbuf[j];
		    k++;
		}
		cum_bytes_read += bytes_read/nchans;
	    }
	    else{
		fprintf(stderr,"unknown input file format\n");
		return(0);
	    }
	    free(tmpbuf);
	}
	else{
	    tmpbuf = (int16 *)calloc(nsamps,sizeof(int16));
	    actsamps = nsamps/nchans;	    
	    cum_bytes_read = 0;
	    
	    if (actsamps*nchans != nsamps){
		fprintf(stderr,"Warning: Blocksize %d is not an integer multiple of Number of channels %d\n",nsamps, nchans);
	    }
	    
	    if (P->input_format==RAW || P->input_format==NIST){    
		for (i=0;i<nchans;i++){
		    
		    offset = i*actsamps;
		    k=0;
		    nreadbytes = nsamps*sizeof(int16);
		    
		    if ((bytes_read = read(fp,tmpbuf,nreadbytes))!=nreadbytes){
			fprintf(stderr,"error reading block (got %d not %d)\n",bytes_read,nreadbytes);
			return(0);
		    }
		    
		    for (j=whichchan-1;j<nsamps;j=j+nchans){
			buf[offset+k] = tmpbuf[j];
			k++;
		    }
		    cum_bytes_read += bytes_read/nchans;
		}
	    }
	    else{
		fprintf(stderr,"unknown input file format\n");
		return(0);
	    }
	    free(tmpbuf);
	}
    }
    
    else{
	fprintf(stderr,"unknown number of channels!\n");
	return(0);
    }
    
    
    if ((P->input_format==NIST && P->input_endian!=P->machine_endian) ||
	(P->input_format==RAW  && P->raw_endian!=P->machine_endian)){
	for(i=0;i<nsamps;i++)
	    SWAPW(&buf[i]);
    }
    
    if (P->dither==ON)
	fe_dither(buf,nsamps);
    
    return(cum_bytes_read/sizeof(int16));
    
}

int32 fe_writeblock_feat(param_t *P, fe_t *FE, int32 fp, int32 nframes, float32 **feat)
{   

    int32 i, length, nwritebytes;

    if (P->logspec == ON)
        length = nframes*FE->FB->num_filters;
    else
        length = nframes*FE->NUM_CEPSTRA;
    
    nwritebytes = length*sizeof(float32);
    if  (write(fp, feat[0], nwritebytes) != nwritebytes) {
        fprintf(stderr,"Error writing block of features\n");
        close(fp);
        return(FE_OUTPUT_FILE_WRITE_ERROR);
    }

    return(length);
}


int32 fe_closefiles(int32 fp_in, int32 fp_out)
{
    close(fp_in);
    close(fp_out);
}


/* adds 1/2-bit noise */
int32 fe_dither(int16 *buffer,int32 nsamps)
{
  int32 i;
  srand48((long)time(0));
  for (i=0;i<nsamps;i++)
    buffer[i] += (short)((!(lrand48()%4))?1:0);
  
  return(0);
}

int32 fe_free_param(param_t *P)
{
}

int32 fe_usage(char **argv)
{

    fprintf(stderr,
	    "wave2mfcc - Convert audio stream into Mel-Frequency Cepstrum Coefficients\n"
	    "Usage: %s\n"
	    " FILE CONTROL)\n"
	    "\t (single) -i <audio file> -o <cep file>\n"
	    "\t (batch)  -c <control file> -di <input directory>   -ei <input extension>\n"
	    "\t                            -do <output directory>  -eo <output extension>\n"
	    "\n"
	    " INPUT FORMAT\n"
	    "\t -nist (NIST sphere format)\n"
	    "\t -mswav (Microsoft Wav file)\n"
	    "\t -raw (raw binary data)\n"
	    "\t -raw_endian format of raw data <big | little [def = little]>\n"
	    "\t -nchans <number of channels of data [def=1]> (interlaced samples assumed)\n"
	    "\t -whichchan <channel to process [def=1]>\n"
	    "\n"
	    " OUTPUT FORMAT\n"
	    "\t -logspec (write out logspectral files instead of cepstra)\n"
	    "\t -feat <sphinx> (SPHINX format - big endian)\n"
	    "\n"
	    " MACHINE FORMAT\n"
	    "\t -mach_endian <big | little [def=little]>\n"
	    "\n"
	    " FILTER PARAMETERS\n"
	    "\t -alpha  <premphasis parameter [def=%.3f]>\n"
	    "\t -srate  <sampling rate [def=%.3f Hz]>\n"
	    "\t -frate  <frame rate [def=%.3f frames/sec]>\n"
	    "\t -wlen   <Hamming window length [def=%.4f sec]>\n"
	    "\t -nfft   <size of FFT [def=%d samples]>\n"
	    "\t -warp  <mel or linear (linear = no warping, default mel)>\n"
	    "\t -nfilt  <number of fitler banks [def=%d]>\n"
	    "\t -lowerf <lower edge of filters [def=%.5f Hz]>\n"
	    "\t -upperf <upper edge of filters [def=%.5f Hz]>\n"
	    "\t -ncep   <number of cep coefficients [def=%d]>\n"
	    "\t -doublebw <use double bandwidth filters (same center freq)>\n"
	    "\n"
	    " INPUT PROCESSING\n"
	    "\t -dither (add 1/2-bit noise)\n"
	    "\n"
	    " DIAGNOSTICS\n"
	    "\t -verbose (show input filenames)\n" 
	    "\t -h[elp] (this screen)\n"
	    "** compiled %s %s **\n",
	    argv[0],
	    (float)DEFAULT_PRE_EMPHASIS_ALPHA,
	    (float)DEFAULT_SAMPLING_RATE,
	    (float)DEFAULT_FRAME_RATE,
	    (float)DEFAULT_WINDOW_LENGTH,
	    (int32)DEFAULT_FFT_SIZE,
	    (int32)DEFAULT_NUM_FILTERS,
	    (float)DEFAULT_LOWER_FILT_FREQ,
	    (float)DEFAULT_UPPER_FILT_FREQ,
	    (int32)DEFAULT_NUM_CEPSTRA,
	    __DATE__,__TIME__);
    
    exit(0);
}
