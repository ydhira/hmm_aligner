import os
import sys
import argparse
import subprocess

TOOL_DIR = "/home/hyd/workhorse2/phonealign_shared/exp/aligner/tools/"
MODEL_DIR = "/home/hyd/workhorse2/phonealign_shared/exp/aligner/MODELS/"
BASE_DIR = "/home/hyd/workhorse2/phonealign_shared/exp/aligner/"

def preprocess_audiofile(audio_path):
    temp_dir = 'temp_dir'
    if not os.path.isdir(temp_dir):
        os.system('mkdir -p ' + temp_dir)
    outfile_proc1 = os.path.join(temp_dir,
                                 os.path.basename(audio_path))
    outfile_proc1 = outfile_proc1.replace('.wav', '_proc1.wav')
    proc1_command = ' '.join(['sox', audio_path, '-t .wav',
                              '-R', '-b 16', '-c 1', '-r 16000',
                              outfile_proc1])
    os.system(proc1_command)
    outfile_proc2 = outfile_proc1.replace('_proc1.wav', '_proc2.wav')
    proc2_command1 = ' '.join(['sox', '-R', outfile_proc1, '-n',
                               'stat', '-v'])
    result1 = subprocess.Popen(proc2_command1.split(' '),
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
    vol = result1.stdout.read()[:-1].decode('utf-8')
    proc2_command2 = ' '.join(['sox', '-R', '-v', vol, outfile_proc1,
                               outfile_proc2])
    result = subprocess.Popen(proc2_command2.split(' '),
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT)
    result.wait()
    return outfile_proc2


def extract_mfcc(audio_path):
    wave2feat = TOOL_DIR + 'wave2feat/working/wave2feat'
    outpath = os.path.join('temp_dir', os.path.basename(audio_path))
    outpath = outpath.replace('.wav', '.80-7200-40f.mfc')
    flags = '-srate 16000 -frate 100 -lowerf 80 -upperf 7200 -dither ' + \
            '-nfilt 40 -nfft 512 -mswav'
    fea_command = ' '.join([wave2feat, '-i', audio_path,
                            '-o', outpath, flags])
    result = subprocess.Popen(fea_command.split(' '),
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT)
    result.wait()
    return outpath


def align_phonemes(audio_path, mfc_path, transcript_path):
    command = ''
    s3align = TOOL_DIR + 'decoderlatest/bin/linux/s3align'
    command += s3align + ' -logbase 1.0001 '

    part = 1
    npart = 1

    fd = open('temp_dir/aligner_temp.ctl', 'w')
    fd.write(os.path.abspath(audio_path.replace('.wav', '')) + '\n')
    fd.close()
    ctlfn = 'temp_dir/aligner_temp.ctl'
    fd = open('temp_dir/aligner_temp.trans', 'w')
    transcript = open(transcript_path).read().splitlines()[0]
    fd.write(transcript.upper() +
             ' ({})\n'.format(os.path.abspath(audio_path.replace('.wav',
                                                                 ''))))
    fd.close()
    inlsnfn = 'temp_dir/aligner_temp.trans'
    dictfn = BASE_DIR+'vocab.dict'
    fillerdictfn = BASE_DIR+'vocab.fillerdict'
    cepdir = './temp_dir'
    cepext = '80-7200-40f.mfc'
    name = os.path.basename(mfc_path).replace('.mfc', '')
    outlsnfn = os.path.join('temp_dir', name + '.trans')
    outctlfn = os.path.join('temp_dir', name + '.ctl')
    logfn = os.path.join('temp_dir', name + '.log')
    modeldir = MODEL_DIR + 'ads/ads.80-7200-40f.1-3/' + \
               'ads.80-7200-40f.1-3.ci_continuous.8gau'
    mdeffn =  MODEL_DIR+ 'ads/ads.80-7200-40f.1-3.ci.mdef'
    nlines = 1
    ctloffset = ((nlines*(part - 1))/float(npart))
    ctlcount = ((nlines*part)/npart) - ctloffset
    phsegdir = 'temp_dir'

    command += '-mdeffn {} -senmgaufn .cont. '.format(mdeffn)
    command += '-meanfn {}/means -varfn {}/variances '.format(modeldir,
                                                              modeldir)
    command += '-mixwfn {}/mixture_weights '.format(modeldir)
    command += '-tmatfn {}/transition_matrices '.format(modeldir)
    command += '-feat 1s_c_d_dd -topn 32 -beam 1e-80 '
    command += '-dictfn {} -fdictfn {} '.format(dictfn, fillerdictfn)
    command += '-ctlfn {} -ctloffset {} -ctlcount {} '.format(ctlfn,
                                                              ctloffset,
                                                              ctlcount)
    command += '-cepdir {} -cepext {} -ceplen 13 -agc none '.format(cepdir,
                                                                    cepext)
    command += '-cmn current -phsegdir {},CTL '.format(phsegdir)
    command += '-wdsegdir {},CTL -insentfn {} '.format(phsegdir, inlsnfn)
    command += '-outsentfn {} -outctlfn {} -logfn {}'.format(outlsnfn,
                                                             outctlfn,
                                                             logfn)
    print("****\n {} \n******" .format(command))
    result = subprocess.Popen(command.split(' '),
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT)
    result.wait()
    phoneme_boundaries = []
    word_boundaries = []
    phonemes_file = os.path.basename(audio_path).replace('.wav', '.phseg')
    words_file = os.path.basename(audio_path).replace('.wav', '.wdseg')
    phonemes_file = open('temp_dir/' + phonemes_file).read().splitlines()[1:-1]
    words_file = open('temp_dir/' + words_file).read().splitlines()[1:-1]
    for line in phonemes_file:
        line = line.split()
        phoneme = line[3]
        start_frame = int(line[0])
        end_frame = int(line[1])
        phoneme_boundaries.append((phoneme, start_frame, end_frame))
    for line in words_file:
        line = line.split()
        word = line[3]
        start_frame = int(line[0])
        end_frame = int(line[1])
        word_boundaries.append((word, start_frame, end_frame))

    # os.system('rm -rf temp_dir')
    return phoneme_boundaries, word_boundaries


def extract_frames(audio_path, transcript):
    if os.path.isfile(audio_path):
        if os.path.isfile(transcript):
            proc_filepath = preprocess_audiofile(audio_path=audio_path)
            mfcc_filepath = extract_mfcc(audio_path=proc_filepath)
            phonemes, words = align_phonemes(audio_path=proc_filepath,
                                             mfc_path=mfcc_filepath,
                                             transcript_path=transcript)
            return phonemes, words
        else:
            print("Input transcript path {} \
                  does not exist".format(transcript))
            return
    else:
        print("Input audio path {} does not exist".format(audio_path))
        return


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-wavfile', help='Path to wav file.', required=True)
    parser.add_argument('-transcript', help='Path to corresponding \
                        transcript.', required=True)
    args = parser.parse_args()
    print(args)

    phonemes, words = extract_frames(audio_path=args.wavfile,
                                     transcript=args.transcript)
    print('PHONEME BOUNDARIES:\n', phonemes)
    print('')
    print('WORD BOUNDARIES:\n', words)


if __name__ == "__main__":
    status = main()
    sys.exit(status)
