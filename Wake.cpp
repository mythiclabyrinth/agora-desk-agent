// TFLite Micro headers go first: Arduino.h defines macros (min, max, abs,
// DEFAULT...) that trip over their templates.
#include "tensorflow/lite/micro/micro_allocator.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_resource_variable.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "src/microfrontend/lib/frontend.h"
#include "src/microfrontend/lib/frontend_util.h"

#include "Wake.h"

#include <esp_heap_caps.h>
#include <new>

#include "Board.h"
#include "WakeModel.h"

namespace {

// What every microWakeWord model was trained on (ESPHome's
// preprocessor_settings.h). Changing any of these breaks every model.
constexpr int FEATURES = 40;
constexpr size_t WINDOW_MS = 30;
constexpr float LOWER_HZ = 125.0f;
constexpr float UPPER_HZ = 7500.0f;
constexpr int NOISE_SMOOTHING_BITS = 10;
constexpr float NOISE_EVEN_SMOOTHING = 0.025f;
constexpr float NOISE_ODD_SMOOTHING = 0.06f;
constexpr float NOISE_MIN_SIGNAL = 0.05f;
constexpr float PCAN_STRENGTH = 0.95f;
constexpr float PCAN_OFFSET = 80.0f;
constexpr int PCAN_GAIN_BITS = 21;
constexpr int LOG_SCALE_SHIFT = 6;

// Resource variables (the model's streaming state) live in their own small
// arena, as in ESPHome's streaming model; 20 variables is their ceiling.
constexpr size_t VAR_ARENA_BYTES = 1024;
constexpr int MAX_VARIABLES = 20;
constexpr int OP_COUNT = 20;

static_assert(WAKE_MODEL_WINDOW <= 32, "sliding window larger than WakeWord::_probs");
static_assert(WAKE_MODEL_STEP_MS == MIC_BLOCK_MS, "the frontend steps once per mic block");

FrontendState frontend;
tflite::MicroMutableOpResolver<OP_COUNT> resolver;
tflite::MicroInterpreter *interpreter = nullptr;
tflite::MicroResourceVariables *variables = nullptr;
uint8_t *tensorArena = nullptr;
uint8_t *varArena = nullptr;

// Internal RAM first: the model runs every 30 ms and PSRAM would slow each
// pass. PSRAM keeps it working on a board short of heap.
uint8_t *allocArena(size_t bytes, bool &inPsram) {
  inPsram = false;
  void *p = heap_caps_aligned_alloc(16, bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!p && psramFound()) {
    p = heap_caps_aligned_alloc(16, bytes, MALLOC_CAP_SPIRAM);
    inPsram = p != nullptr;
  }
  return static_cast<uint8_t *>(p);
}

bool registerOps() {
  // The ops ESPHome registers for microWakeWord streaming models.
  return resolver.AddCallOnce() == kTfLiteOk && resolver.AddVarHandle() == kTfLiteOk &&
         resolver.AddReshape() == kTfLiteOk && resolver.AddReadVariable() == kTfLiteOk &&
         resolver.AddStridedSlice() == kTfLiteOk && resolver.AddConcatenation() == kTfLiteOk &&
         resolver.AddAssignVariable() == kTfLiteOk && resolver.AddConv2D() == kTfLiteOk &&
         resolver.AddMul() == kTfLiteOk && resolver.AddAdd() == kTfLiteOk &&
         resolver.AddMean() == kTfLiteOk && resolver.AddFullyConnected() == kTfLiteOk &&
         resolver.AddLogistic() == kTfLiteOk && resolver.AddQuantize() == kTfLiteOk &&
         resolver.AddDepthwiseConv2D() == kTfLiteOk && resolver.AddAveragePool2D() == kTfLiteOk &&
         resolver.AddMaxPool2D() == kTfLiteOk && resolver.AddPad() == kTfLiteOk &&
         resolver.AddPack() == kTfLiteOk && resolver.AddSplitV() == kTfLiteOk;
}

// A failed AllocateTensors may leave variables half-created in their arena;
// start the variable arena over before the next attempt.
void resetVariables() {
  tflite::MicroAllocator *allocator = tflite::MicroAllocator::Create(varArena, VAR_ARENA_BYTES);
  variables = allocator ? tflite::MicroResourceVariables::Create(allocator, MAX_VARIABLES) : nullptr;
}

uint8_t cutoffFor(const char *level) {
  float cutoff = WAKE_MODEL_CUTOFF;
  if (level && !strcmp(level, "low")) cutoff += WAKE_CUTOFF_SHIFT;    // harder to trigger
  if (level && !strcmp(level, "high")) cutoff -= WAKE_CUTOFF_SHIFT;   // easier to trigger
  if (cutoff < WAKE_CUTOFF_MIN) cutoff = WAKE_CUTOFF_MIN;
  if (cutoff > WAKE_CUTOFF_MAX) cutoff = WAKE_CUTOFF_MAX;
  return static_cast<uint8_t>(cutoff * 255.0f + 0.5f);
}

}  // namespace

WakeWord wakeWord;

const char *WakeWord::phrase() const { return WAKE_MODEL_PHRASE; }
const char *WakeWord::name() const { return WAKE_MODEL_NAME; }

bool WakeWord::begin() {
  _cutoff.store(cutoffFor("medium"));

  FrontendConfig config;
  FrontendFillConfigWithDefaults(&config);
  config.window.size_ms = WINDOW_MS;
  config.window.step_size_ms = WAKE_MODEL_STEP_MS;
  config.filterbank.num_channels = FEATURES;
  config.filterbank.lower_band_limit = LOWER_HZ;
  config.filterbank.upper_band_limit = UPPER_HZ;
  config.noise_reduction.smoothing_bits = NOISE_SMOOTHING_BITS;
  config.noise_reduction.even_smoothing = NOISE_EVEN_SMOOTHING;
  config.noise_reduction.odd_smoothing = NOISE_ODD_SMOOTHING;
  config.noise_reduction.min_signal_remaining = NOISE_MIN_SIGNAL;
  config.pcan_gain_control.enable_pcan = 1;
  config.pcan_gain_control.strength = PCAN_STRENGTH;
  config.pcan_gain_control.offset = PCAN_OFFSET;
  config.pcan_gain_control.gain_bits = PCAN_GAIN_BITS;
  config.log_scale.enable_log = 1;
  config.log_scale.scale_shift = LOG_SCALE_SHIFT;
  if (!FrontendPopulateState(&config, &frontend, MIC_SAMPLE_RATE)) {
    Serial.println("Wake word: audio frontend did not start (out of memory)");
    return false;
  }

  if (!loadModel()) {
    FrontendFreeStateContents(&frontend);
    return false;
  }
  _ready = true;
  Serial.print("Wake word: listening for \"");
  Serial.print(WAKE_MODEL_PHRASE);
  Serial.println("\" when enabled");
  return true;
}

bool WakeWord::loadModel() {
  const tflite::Model *model = tflite::GetModel(WAKE_MODEL_DATA);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Wake word: model schema version is not supported by this TFLite Micro");
    return false;
  }
  if (!registerOps()) {
    Serial.println("Wake word: could not register the model's operators");
    return false;
  }
  varArena = static_cast<uint8_t *>(heap_caps_malloc(VAR_ARENA_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (!varArena) {
    Serial.println("Wake word: no memory for the model's state");
    return false;
  }
  resetVariables();

  // The manifest's arena size is what ESPHome measured; esp-nn versions
  // differ, so try a little more before giving up, never past the cap.
  const size_t base = (WAKE_MODEL_ARENA + 15) & ~static_cast<size_t>(15);
  const size_t attempts[] = {base, (base * 3 / 2 + 15) & ~static_cast<size_t>(15),
                             (base * 2 + 15) & ~static_cast<size_t>(15)};
  for (size_t bytes : attempts) {
    if (bytes > WAKE_ARENA_MAX || !variables) break;
    bool inPsram = false;
    tensorArena = allocArena(bytes, inPsram);
    if (!tensorArena) continue;
    interpreter = new (std::nothrow) tflite::MicroInterpreter(model, resolver, tensorArena, bytes, variables);
    if (interpreter && interpreter->AllocateTensors() == kTfLiteOk) {
      Serial.printf("Wake word: tensor arena %u bytes (%u used) in %s\n", static_cast<unsigned>(bytes),
                    static_cast<unsigned>(interpreter->arena_used_bytes()), inPsram ? "PSRAM" : "internal RAM");
      break;
    }
    delete interpreter;
    interpreter = nullptr;
    heap_caps_free(tensorArena);
    tensorArena = nullptr;
    resetVariables();
  }
  if (!interpreter) {
    Serial.println("Wake word: could not fit the model's tensor arena");
    heap_caps_free(varArena);
    varArena = nullptr;
    return false;
  }

  // The contract every microWakeWord model keeps: [1, stride, 40] int8 in,
  // one uint8 probability out.
  TfLiteTensor *input = interpreter->input(0);
  TfLiteTensor *output = interpreter->output(0);
  bool shapeOk = input && output && input->type == kTfLiteInt8 && input->dims->size == 3 &&
                 input->dims->data[0] == 1 && input->dims->data[2] == FEATURES && input->dims->data[1] >= 1 &&
                 output->type == kTfLiteUInt8 && output->dims->size == 2 && output->dims->data[0] == 1 &&
                 output->dims->data[1] == 1;
  if (!shapeOk) {
    Serial.println("Wake word: model inputs/outputs are not a microWakeWord streaming model");
    delete interpreter;
    interpreter = nullptr;
    heap_caps_free(tensorArena);
    heap_caps_free(varArena);
    tensorArena = varArena = nullptr;
    return false;
  }
  _stride = static_cast<uint8_t>(input->dims->data[1]);
  return true;
}

void WakeWord::holdOff(unsigned long ms) {
  uint32_t until = millis() + ms;
  // Only extend: a short beep must not cut a longer hold from the speaker.
  if (static_cast<int32_t>(until - _holdUntil.load()) > 0) _holdUntil.store(until);
}

void WakeWord::setSensitivity(const char *level) {
  _cutoff.store(cutoffFor(level));
}

bool WakeWord::takeDetection(uint32_t &samplePos) {
  if (!_detected.exchange(false)) return false;
  samplePos = _detectedPos.load();
  return true;
}

void WakeWord::resetWindow() {
  memset(_probs, 0, sizeof(_probs));
  _probIndex = 0;
  _strideStep = 0;
  _score.store(0);
}

void WakeWord::feed(const int16_t *samples, size_t count, uint32_t endPos) {
  if (!_ready) return;
  uint32_t now = millis();
  bool listen = _armedWanted.load() && static_cast<int32_t>(now - _holdUntil.load()) >= 0 &&
                static_cast<int32_t>(now - _refractoryUntil) >= 0;
  if (listen && !_listening.load()) {
    // Coming back from deafness: the model's streaming state still holds old
    // audio, so let it run a while before a detection counts.
    resetWindow();
    _warmup = WAKE_WARMUP_MS / WAKE_MODEL_STEP_MS;
  }
  if (!listen && _listening.load()) _score.store(0);
  _listening.store(listen);

  // The frontend runs even while deaf so its noise estimate keeps up with
  // the room; only the model (the expensive part) waits.
  while (count) {
    size_t used = 0;
    FrontendOutput out = FrontendProcessSamples(&frontend, samples, count, &used);
    if (!used) break;
    samples += used;
    count -= used;
    if (!out.size || !listen) continue;
    if (featureReady(out.values, out.size, now)) {
      _detectedPos.store(endPos);
      _detected.store(true);
      _refractoryUntil = now + WAKE_REFRACTORY_MS;
      _listening.store(false);
      listen = false;
    }
  }
}

// One feature slice in; true when the smoothed probability crosses the cutoff.
bool WakeWord::featureReady(const uint16_t *values, size_t size, uint32_t now) {
  TfLiteTensor *input = interpreter->input(0);
  int8_t *slot = input->data.int8 + FEATURES * _strideStep;
  for (size_t i = 0; i < static_cast<size_t>(FEATURES); i++) {
    // The frontend's log-mel values run 0..~670. Training divided them by
    // 25.6 (range ~0-26), and quantisation mapped 0..26 onto -128..127, so
    // q = v * 256 / (25.6 * 26) - 128, done in integers with rounding.
    int32_t v = i < size ? values[i] : 0;
    int32_t q = (v * 256 + 333) / 666 - 128;
    slot[i] = static_cast<int8_t>(q < -128 ? -128 : (q > 127 ? 127 : q));
  }
  if (_warmup) _warmup--;
  if (++_strideStep < _stride) return false;
  _strideStep = 0;

  if (interpreter->Invoke() != kTfLiteOk) return false;
  uint8_t p = interpreter->output(0)->data.uint8[0];
  // The first second after re-arming can spike on stale streaming state;
  // those steps cannot detect, so they do not count for the meter either.
  if (!_warmup && (p >= _peak.load() || now - _peakAt > WAKE_PEAK_HOLD_MS)) {
    _peak.store(p);
    _peakAt = now;
  }
  _probs[_probIndex] = p;
  _probIndex = (_probIndex + 1) % WAKE_MODEL_WINDOW;

  uint32_t sum = 0;
  for (uint8_t i = 0; i < WAKE_MODEL_WINDOW; i++) sum += _probs[i];
  _score.store(static_cast<uint8_t>(sum / WAKE_MODEL_WINDOW));
  if (_warmup) return false;
  if (sum <= static_cast<uint32_t>(_cutoff.load()) * WAKE_MODEL_WINDOW) return false;
  resetWindow();
  return true;
}
