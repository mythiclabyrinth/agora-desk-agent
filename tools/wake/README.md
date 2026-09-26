# Wake word model

The board runs a [microWakeWord](https://github.com/kahrendt/microWakeWord) streaming
model on TensorFlow Lite Micro. The model is compiled into the firmware from
`WakeModel.h`, which is **generated** from a `.tflite` file and its `.json` manifest:

```bash
python3 tools/wake/make_model_header.py tools/wake/models/hey_jarvis.tflite
python3 tools/wake/make_model_header.py --check   # is WakeModel.h current?
```

The phrase shown in the page and serial log, the probability cutoff, the sliding
window and the arena size all come from the manifest, so swapping models is that
one command plus a reflash.

## What ships

`models/hey_agora.tflite` + `.json`: a **Hey Agora** model trained with
[microWakeWord-Trainer-AppleSilicon](https://github.com/TaterTotterson/microWakeWord-Trainer-AppleSilicon)
(microWakeWord v2 format, int8, stride 2). Its manifest carries the arena size
measured on this board (52 000 bytes; the trainer's own estimate is lower). The
firmware strips both "hey agora" and "hey jarvis" from transcripts.

`models/hey_jarvis.tflite` + `.json`: the pre-trained **Hey Jarvis** v2 model from
[esphome/micro-wake-word-models](https://github.com/esphome/micro-wake-word-models)
(`models/v2/`, commit `05b6592`, Apache-2.0), kept as a known-good fallback:
`python3 tools/wake/make_model_header.py tools/wake/models/hey_jarvis.tflite`.

## Training "Hey Agora"

Training needs a GPU for a few hours; it cannot run on the board or in this repo.

1. Open microWakeWord's training notebook
   (`notebooks/basic_training_notebook.ipynb` in the microWakeWord repo) in Colab or
   on a machine with an NVIDIA GPU.
2. Set the target phrase to `hey agora`. The notebook generates positive samples
   with [Piper](https://github.com/rhasspy/piper-sample-generator) text-to-speech
   across many voices; add a few spellings Piper pronounces well (`hey agora`,
   `hey ah-go-ra`) and listen to a handful before training.
3. Keep the notebook's negative sets (speech, music, ambient noise, and the
   precomputed negative feature sets). Add recordings of *your* room: keyboard,
   the desk speaker reading agent replies, and people saying close words
   ("hey Siri", "a quarter", "Angora"). False wakes come from what the model
   never heard.
4. Keep the default feature settings (40 channels, 30 ms window, **10 ms step**).
   The firmware's frontend is fixed to what ESPHome uses; the header script refuses
   any other step size.
5. Export the streaming, quantized model (`stream_state_internal_quant.tflite`),
   rename it `hey_agora.tflite`, and write `hey_agora.json` beside it in the same
   format as `hey_jarvis.json`:

   ```json
   {
     "type": "micro",
     "wake_word": "Hey Agora",
     "author": "you",
     "model": "hey_agora.tflite",
     "trained_languages": ["en"],
     "version": 2,
     "micro": {
       "probability_cutoff": 0.97,
       "feature_step_size": 10,
       "sliding_window_size": 5,
       "tensor_arena_size": 30000,
       "minimum_esphome_version": "2024.7.0"
     }
   }
   ```

   Pick `probability_cutoff` from the notebook's false-accept/false-reject curve
   (the cutoff where false accepts per hour stay under ~0.5). `tensor_arena_size`
   can be generous; the firmware prints how much it actually used.
6. Put both files in `tools/wake/models/`, run
   `python3 tools/wake/make_model_header.py tools/wake/models/hey_agora.tflite`,
   compile, flash. Settings › Voice now says "Hey Agora".

If the model uses an operator the firmware does not register, the serial log says
the wake word could not start and everything else keeps working; add the op to
`registerOps()` in `Wake.cpp`.

## Tuning on the desk

Settings › Voice › Devices › Wake word shows a live score (the smoothed probability). Say the
phrase from where you sit and watch it: it should jump past the cutoff; normal
talk, the speaker, and typing should stay low. Low / Medium / High sensitivity
moves the cutoff by `WAKE_CUTOFF_SHIFT` (`Board.h`). The VAD that ends hands-free
recordings has its thresholds in `Board.h` too (`VAD_*`).
