#!/usr/bin/env python3
"""Preview the embedded page with isolated demo APIs; never contacts the ESP32.
Run: python3 tools/preview.py, then open http://127.0.0.1:8765.
Add ?scenario=offline or ?scenario=empty to exercise connection/setup states.
"""
import json
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parents[1]
AGENTS = [dict(id=k, title=k.title(), name=k.title(), agent_id=k+'-cli',
               url='http://192.168.0.20:4470' if k == 'claude' else '',
               channel='desk-general' if k == 'claude' else '',
               token_set=k == 'claude', ready=k == 'claude')
          for k in ('claude', 'cursor', 'codex')]
JOB = {'number': 0, 'started': 0, 'agent': 'claude'}

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
            page = (ROOT / 'Page.h').read_text().split('R"ESP32PAGE(', 1)[1].split(')ESP32PAGE"', 1)[0].encode()
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.send_header('Content-Length', str(len(page)))
            self.end_headers()
            self.wfile.write(page)
        elif path == '/api/agents':
            self.reply({'agents': [dict(a, ready=False, token_set=False) if empty else a for a in AGENTS]})
        elif path == '/api/status':
            self.reply(dict(wifi=not offline, ssid='Studio Wi-Fi', ip='192.168.0.113', ap_ip='192.168.4.1',
                            host='esp32-agent.local', listening=bool(JOB['number'] and time.time()-JOB['started'] < 4),
                            agents=[dict(id=a['id'], name=a['name'], ready=False if empty else a['ready']) for a in AGENTS]))
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
        data = json.loads(self.rfile.read(int(self.headers.get('Content-Length', '0'))))
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
        else:
            self.reply({'error':'Not found'},404)

if __name__ == '__main__':
    print('Isolated Desk agent preview: http://127.0.0.1:8765', flush=True)
    ThreadingHTTPServer(('127.0.0.1',8765),Handler).serve_forever()
