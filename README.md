# DTSpeechPreprocessor
This project provides a preprocessor to convert DT Speech to M-AILABS speech dataset format

## download dataset
download DT speech from [here](https://www.kaggle.com/etaifour/trump-speeches-audio-and-word-transcription)

## how to compile
compile with the following command
```bash
make
```

## preprocess each audio
preprocess audio with the following command
```bash
converter -i <transcription in json> -a <mp3 file> -o <dirname>
```
