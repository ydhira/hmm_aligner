# Phoneme and word boundaries (Pythonized - Python3) - Done with forced alignment with HMM
This package gives the phoneme and word boundaries given an audio file and its corresponding transcript. Most of the settings are hardcoded since the model was trained using those parameters so changing that could give weird results possibly.

## Command:
```
python aligner.py -wavfile <path to wav file> -transcript <path to corresponding transcript>
```
This returns 2 lists where each element is a triplet of the form (<phoneme>, <startframe>, <endframe>). First list consists of the phoneme boundaries and the second one contains the word boundaries.

## Steps provided currently:
- The python code preprocesses the data as per required parameters and also normalizes and maximizes the volume for the audio file provided.
- Extracts the mfcc features for the processed audio file.
- Aligns the given transcript to frames.

## Requirements:
- You need to make sure that all the words in the transcript that you provide are present in the dictionary/vocabulary (vocab.dict). If not, add the required words and the phoneme breakup at the end. Should still work reasonably well.
- Sampling rate of the audio file should be 16kHz. (I haven't completely debugged it but I think it works only with 16kHz audio files. Will update when I do thorough testing)

TODOS:
- Make paths more robust to working from any folder. Currently you can run it only from this directory.

// This is currently very hacky but is more comfortable than creating folders and changing shell scripts everytime. Gives a lot of automation and ease to use.
// I haven't currently implemented the third script which creates and text file for each phoneme containing infor of all the files that phoneme is present in and from which frame to which frame.
// Let me know if this throws any errors or if you have any doubts.