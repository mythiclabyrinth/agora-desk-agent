#!/usr/bin/env python3
"""Preview the embedded page with isolated demo APIs; never contacts the ESP32.
Run: python3 tools/preview.py, then open http://127.0.0.1:8765.
Add ?scenario=offline or ?scenario=empty to exercise connection/setup states,
?scenario=voice to watch a button conversation land in the chat,
?scenario=wake to watch a hands-free (wake word) exchange, or ?scenario=dial to
watch the desk dial cycle the hands-free agent.
"""
import gzip
import json
import math
import struct
import sys
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'web'))
import build  # noqa: E402  (web/build.py assembles the page for the board and for us)
AGENTS = [dict(id=k, title=k.title(), name=k.title(), agent_id=k+'-cli',
               url='http://192.168.0.20:4470' if k == 'claude' else '',
               channel='desk-general' if k == 'claude' else '',
               token_set=k == 'claude', ready=k == 'claude')
          for k in ('claude', 'cursor', 'codex')]
JOB = {'number': 0, 'started': 0, 'agent': 'claude'}
VOICE = dict(keys=dict(groq=False, openai=False), stt_provider='groq', tts_provider='groq',
             stt_models=dict(groq='whisper-large-v3-turbo', openai='gpt-4o-mini-transcribe'),
             tts_models=dict(groq='canopylabs/orpheus-v1-english', openai='gpt-4o-mini-tts'),
             tts_voices=dict(groq='autumn', openai='alloy'), accent='american', agent='claude',
             stt_ready=False, tts_ready=False, mic=True, speaker=True, button_pin=6,
             wake_enabled=False, wake_sensitivity='medium', wake_available=True, wake_phrase='Hey Jarvis',
             wake_button_pin=46, listen_led_pin=18)
# ?scenario=voice walks the button flow: recording -> transcribing -> waiting -> speaking -> done.
# The chat mic button drives the same steps from /api/voice/talk.
VOICE_STEPS = [(0, 'recording'), (3, 'transcribing'), (5, 'waiting'), (9, 'speaking'), (13, 'done')]
VOICE_RUN = {'started': 0, 'source': 'button', 'agent': 'claude', 'stopped': 0}
# ?scenario=wake: the detector's score climbs, the phrase is heard, the VAD ends the clip, and the reply lands.
WAKE_STEPS = [(0, 'idle'), (3, 'recording'), (6, 'transcribing'), (8, 'waiting'), (12, 'speaking'), (16, 'done')]
WAKE_RUN = {'started': 0}
# ?scenario=dial: Cursor is configured too, and the desk dial moves the hands-free agent (VOICE['agent'],
# /api/status voice.target_agent) between the configured agents every few seconds.
DIAL_CYCLE = ('claude', 'cursor')
DIAL_EVERY_S = 3


def agent_ready(a, scenario):
    if 'scenario=empty' in scenario:
        return False
    return a['ready'] or ('scenario=dial' in scenario and a['id'] in DIAL_CYCLE)


def wake_status(armed=True, score=0.0):
    on = VOICE['wake_enabled'] and VOICE['wake_available']
    return dict(enabled=VOICE['wake_enabled'], muted=not VOICE['wake_enabled'], available=VOICE['wake_available'],
                armed=on and armed, sensitivity=VOICE['wake_sensitivity'], phrase=VOICE['wake_phrase'],
                score=round(score if on and armed else 0.0, 2),
                peak=round(min(1.0, score * 1.5) if on and armed else 0.0, 2))


def voice_ready():
    VOICE['stt_ready'] = VOICE['keys'][VOICE['stt_provider']]
    VOICE['tts_ready'] = VOICE['keys'][VOICE['tts_provider']]

def tone_wav(seconds, rate=24000):
    n = max(int(rate * max(seconds, 0.3)), 1)
    pcm = b''.join(struct.pack('<h', int(6000 * math.sin(2 * math.pi * (440 if i < n / 2 else 554) * i / rate)))
                   for i in range(n))
    return b'RIFF' + struct.pack('<I', 36 + len(pcm)) + b'WAVEfmt ' + struct.pack('<IHHIIHH', 16, 1, 1, rate, rate * 2, 2, 16) \
        + b'data' + struct.pack('<I', len(pcm)) + pcm


class Handler(BaseHTTPRequestHandler):
    def reply(self, data, code=200):
        raw = json.dumps(data).encode()
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def do_GET(self):
        path = urlparse(self.path).path
        scenario = self.headers.get('Referer', '')
        offline = 'scenario=offline' in scenario
        empty = 'scenario=empty' in scenario
        if path == '/':
            # Assembled from web/ on every request so edits show on reload, and
            # served gzipped exactly as the board does.
            html = build.assemble()
            if build.current_digest() != build.digest(html):
                print('note: Page.h is stale; run python3 web/build.py before flashing', flush=True)
            page = gzip.compress(html.encode('utf-8'), compresslevel=9, mtime=0)
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.send_header('Content-Encoding', 'gzip')
            self.send_header('Cache-Control', 'no-store')
            self.send_header('Content-Length', str(len(page)))
            self.end_headers()
            self.wfile.write(page)
        elif path == '/api/agents':
            self.reply({'agents': [dict(a, ready=agent_ready(a, scenario), token_set=not empty and (a['token_set'] or agent_ready(a, scenario)))
                                   for a in AGENTS]})
        elif path == '/api/status':
            if 'scenario=dial' in scenario:
                VOICE['agent'] = DIAL_CYCLE[int(time.time() / DIAL_EVERY_S) % len(DIAL_CYCLE)]
            # agent: this exchange's; target_agent: the hands-free setting (talk button, wake word, dial).
            voice = dict(state='idle', source='button', job=0, agent='claude', target_agent=VOICE['agent'], recorded_ms=0,
                         stt_ready=VOICE['stt_ready'], tts_ready=VOICE['tts_ready'], mic=True, speaker=True)
            voice['wake'] = wake_status(score=0.02 + 0.03 * abs(math.sin(time.time())))
            if 'scenario=wake' in scenario:
                if not WAKE_RUN['started']:
                    WAKE_RUN['started'] = time.time()
                    VOICE['wake_enabled'] = True
                elapsed = time.time() - WAKE_RUN['started']
                state = [s for at, s in WAKE_STEPS if elapsed >= at][-1]
                # The score rises as the phrase is spoken, then the detector is disarmed until the exchange ends.
                score = min(0.99, 0.1 + 0.3 * elapsed) if state == 'idle' else 0.0
                voice['wake'] = wake_status(armed=state in ('idle', 'done'), score=score)
                if state != 'idle':
                    voice.update(state=state, source='wake', agent='claude', job=int(WAKE_RUN['started']),
                                 recorded_ms=min(int((elapsed - 3) * 1000), 2400))
                if elapsed >= 8:
                    voice['heard'] = 'What is left on the wake word branch?'
                if state == 'done':
                    voice['reply'] = 'Train the Hey Agora model, then tune the VAD thresholds on the real board.'
            if 'scenario=voice' in scenario and not VOICE_RUN['started']:
                VOICE_RUN.update(started=time.time(), source='button', agent='claude', stopped=0)
            if VOICE_RUN['started']:
                elapsed = time.time() - VOICE_RUN['started']
                if VOICE_RUN['source'] == 'page':
                    # A page recording runs until the mic is tapped again, then plays out the rest.
                    elapsed = 0 if not VOICE_RUN['stopped'] else 3 + time.time() - VOICE_RUN['stopped']
                state = [s for at, s in VOICE_STEPS if elapsed >= at][-1]
                voice.update(state=state, source=VOICE_RUN['source'], agent=VOICE_RUN['agent'],
                             job=int(VOICE_RUN['started']), recorded_ms=min(int(elapsed*1000), 2600))
                if elapsed >= 5:
                    voice['heard'] = 'What should I work on this afternoon?'
                if state == 'done':
                    voice['reply'] = 'Finish the voice branch, then take the board for a walk around the room and see how far the mic reaches.'
            if voice['state'] == 'recording':
                voice['wake']['armed'] = False
            self.reply(dict(wifi=not offline, ssid='Studio Wi-Fi', ip='192.168.0.113', ap_ip='192.168.4.1',
                            host='esp32-agent.local', listening=bool(JOB['number'] and time.time()-JOB['started'] < 4),
                            voice=voice,
                            agents=[dict(id=a['id'], name=a['name'], ready=agent_ready(a, scenario)) for a in AGENTS],
                            display=dict(present=True)))
        elif path == '/api/voice':
            self.reply(VOICE)
        elif path == '/api/wifi/scan':
            time.sleep(1)
            if 'scan-error' in scenario:
                return self.reply({'error':'Scan unavailable. Please retry.'},503)
            networks = [] if 'scan-empty' in scenario else [
                dict(ssid='Studio Wi-Fi', rssi=-42, open=False),
                dict(ssid='Garden network', rssi=-64, open=False),
                dict(ssid='Guest Wi-Fi', rssi=-70, open=True),
                dict(ssid='A very long network name upstairs', rssi=-83, open=False)]
            self.reply({'networks':networks})
        elif path == '/api/listen':
            self.reply(dict(job=JOB['number'], state='done' if time.time()-JOB['started'] >= 4 else 'listening',
                            from_=JOB['agent'], **{'from':JOB['agent'].title()}, waited_ms=int((time.time()-JOB['started'])*1000),
                            reply='Your desk is connected.\n\nI can help you think through an idea, review your code, or plan the next step. What would you like to work on?\n\nThis is a local preview response; no message was sent to a real agent.'))
        else:
            self.reply({'error':'Not found'},404)

    def do_POST(self):
        raw = self.rfile.read(int(self.headers.get('Content-Length', '0')))
        if self.path == '/api/voice/transcribe':
            # Browser-mode mic: the page uploads a multipart clip and gets the transcript back.
            time.sleep(1.2)
            if not VOICE['stt_ready']:
                return self.reply({'error':'Add the speech-to-text provider\'s API key under Settings › Voice.'},400)
            if b'filename="clip.' not in raw:
                return self.reply({'error':'No audio arrived. Try recording again.'},400)
            return self.reply({'ok':True,'text':'What should I work on this afternoon?','bytes':len(raw)})
        data = json.loads(raw or b'{}')
        if self.path == '/api/voice/say':
            # Browser-mode speaker: a finite WAV the page plays itself. A soft two-tone stands in for speech.
            time.sleep(1.0)
            if not VOICE['tts_ready']:
                return self.reply({'error':'Add the text-to-speech provider\'s API key under Settings › Voice.'},400)
            wav = tone_wav(min(len(data.get('text','')) * 0.03, 2.0))
            self.send_response(200)
            self.send_header('Content-Type','audio/wav'); self.send_header('Content-Length',str(len(wav)))
            self.send_header('Cache-Control','no-store'); self.end_headers(); self.wfile.write(wav)
            return
        if self.path == '/api/chat':
            if 'preview-error' in data.get('text',''):
                return self.reply({'error':'Demo connection failed. Please retry.'},503)
            JOB.update(number=JOB['number']+1, started=time.time(), agent=data['agent'])
            self.reply({'ok':True, 'job':JOB['number']})
        elif self.path == '/api/agents':
            a = next(a for a in AGENTS if a['id'] == data['id'])
            a.update({k:v for k,v in data.items() if k != 'token'})
            a.update(ready=True, token_set=True)
            self.reply({'ok':True,'ready':True})
        elif self.path == '/api/wifi':
            self.reply({'ok':True,'connected':True,'ip':'192.168.0.113'})
        elif self.path == '/api/voice':
            # Like the board, every field is optional; the page sends agent only when it was picked there.
            if 'agent' in data:
                if data['agent'] not in ('claude', 'cursor', 'codex'):
                    return self.reply({'error':'Choose which agent the desk should reach hands-free.'},400)
                VOICE['agent'] = data['agent']
            VOICE.update(stt_provider=data['stt_provider'], tts_provider=data['tts_provider'], accent=data['accent'],
                         stt_models=dict(groq=data['stt_model_groq'], openai=data['stt_model_openai']),
                         tts_models=dict(groq=data['tts_model_groq'], openai=data['tts_model_openai']),
                         tts_voices=dict(groq=data['voice_groq'], openai=data['voice_openai']))
            if 'wake_enabled' in data:
                VOICE['wake_enabled'] = bool(data['wake_enabled'])
            if data.get('wake_sensitivity') in ('low', 'medium', 'high'):
                VOICE['wake_sensitivity'] = data['wake_sensitivity']
            voice_ready()
            self.reply({'ok':True,'stt_ready':VOICE['stt_ready'],'tts_ready':VOICE['tts_ready']})
        elif self.path == '/api/voice/wake':
            # The page's wake toggle and sensitivity; the same setting as the desk's mute button.
            if 'sensitivity' in data:
                if data['sensitivity'] not in ('low', 'medium', 'high'):
                    return self.reply({'error':'Sensitivity must be low, medium, or high.'},400)
                VOICE['wake_sensitivity'] = data['sensitivity']
            if 'enabled' in data:
                if data['enabled'] and not VOICE['wake_available']:
                    return self.reply({'error':'The wake word could not start on this board (see the serial log). The talk button still works.'},409)
                VOICE['wake_enabled'] = bool(data['enabled'])
            self.reply({'ok':True,'wake':wake_status()})
        elif self.path == '/api/voice/keys':
            if not data.get('clear') and not data.get('api_key','').startswith(('gsk_','sk-')):
                return self.reply({'error':'That does not look like an API key.'},400)
            VOICE['keys'][data['provider']] = not data.get('clear')
            voice_ready()
            self.reply({'ok':True})
        elif self.path == '/api/voice/tone':
            time.sleep(1.0)
            self.reply({'ok':True})
        elif self.path == '/api/voice/test':
            time.sleep(0.8)
            if not VOICE['keys'].get(data['provider']):
                return self.reply({'error':'No key saved for that provider yet.'},400)
            self.reply({'ok':True})
        elif self.path == '/api/voice/talk':
            if data.get('action') == 'start':
                VOICE_RUN.update(started=time.time(), source='page', agent=data.get('agent','claude'), stopped=0)
            else:
                VOICE_RUN['stopped'] = time.time()
            self.reply({'ok':True,'job':int(VOICE_RUN['started']),'state':'recording'})
        else:
            self.reply({'error':'Not found'},404)

if __name__ == '__main__':
    print('Isolated Desk agent preview: http://127.0.0.1:8765', flush=True)
    ThreadingHTTPServer(('127.0.0.1',8765),Handler).serve_forever()
