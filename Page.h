#pragma once

// Self-contained UI: no network fonts, CDNs, or client-side dependencies.
static const char INDEX_HTML[] PROGMEM = R"ESP32PAGE(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#f6f5f0">
<title>Desk agent · Your local workspace</title>
<style>
:root{color-scheme:light;--bg:#f6f5f0;--paper:#fffefa;--ink:#242e2a;--muted:#71776e;--line:#dedfd5;--green:#284e3b;--soft:#e9eee5;--orange:#bd6848;--red:#ab4139;--radius:18px;--shadow:0 4px 20px #24342605}
*{box-sizing:border-box} [hidden]{display:none!important} html{scroll-behavior:smooth} body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.55 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;-webkit-font-smoothing:antialiased} button,input,textarea{font:inherit;color:inherit} button,a,input,textarea{-webkit-tap-highlight-color:transparent} button{cursor:pointer} button:disabled{cursor:not-allowed;opacity:.45} button,a{touch-action:manipulation} a{color:var(--green)} button{transition:background .18s,border-color .18s,transform .18s,box-shadow .18s} button:active:not(:disabled){transform:translateY(1px)} :focus-visible{outline:3px solid #bc8546;outline-offset:4px} svg{width:20px;height:20px;fill:none;stroke:currentColor;stroke-width:1.7;stroke-linecap:round;stroke-linejoin:round;flex-shrink:0} .icon-defs{position:absolute;width:0;height:0;overflow:hidden} .skip{position:fixed;top:-80px;left:16px;z-index:20;background:var(--paper);padding:12px}.skip:focus{top:12px}
.shell{display:grid;grid-template-columns:220px minmax(0,1fr);min-height:100dvh}.sidebar{border-right:1px solid var(--line);padding:34px 22px 24px;display:flex;flex-direction:column;position:sticky;top:0;height:100dvh;background:#efefe8}.brand{display:flex;align-items:center;gap:10px;font-size:20px;font-weight:650;letter-spacing:-.8px;color:var(--ink);text-decoration:none}.brand-mark{width:34px;height:34px;display:grid;place-items:center;background:var(--green);color:#fff;border-radius:10px}.brand-mark svg{width:22px;height:22px}.brand small{color:var(--muted);font-size:9px;letter-spacing:.5px;align-self:flex-start;margin-top:2px}.sidebar-label,.eyebrow{font-size:10px;letter-spacing:1.8px;text-transform:uppercase;font-weight:650;color:var(--muted)}.sidebar-label{margin:51px 12px 12px}.nav{display:grid;gap:7px}.nav button{border:0;background:transparent;text-align:left;display:flex;align-items:center;gap:12px;padding:13px 14px;border-radius:10px;font-size:13px;color:#667065;min-height:46px}.nav button:hover{background:#e6e8df}.nav button.on{background:#dde6d9;color:var(--green);font-weight:650}.nav button.on:after{content:"";width:5px;height:5px;border-radius:50%;background:var(--green);margin-left:auto}.device{margin-top:auto;border:1px solid #d7dccf;border-radius:12px;padding:15px;background:#f5f5ef}.device-top{display:flex;align-items:center;gap:8px;font-size:12px;font-weight:600}.dot{width:7px;height:7px;background:#939b8e;display:inline-block;border-radius:50%;flex-shrink:0}.dot.on{background:#5d8c55;box-shadow:0 0 0 3px #5d8c5515}.device p{font-size:11px;color:var(--muted);margin:8px 0 0;overflow-wrap:anywhere}.device code{font-size:10px}.sidebar-foot{font-size:10px;color:var(--muted);margin:16px 4px 0;display:flex;gap:6px;align-items:center}.sidebar-foot svg{width:12px;height:12px}
.workspace{min-width:0;padding:0 44px 32px}.topbar{height:91px;display:flex;align-items:center;justify-content:space-between;border-bottom:1px solid var(--line);gap:12px}.breadcrumb{font-size:12px;color:var(--muted);display:flex;align-items:center;gap:12px}.breadcrumb span:last-child{color:var(--ink)}.top-status{display:flex;align-items:center;gap:8px;font-size:11px;color:var(--muted)}main{max-width:1180px;margin:auto}section{animation:arrive .25s ease} .page-heading{display:flex;align-items:flex-end;justify-content:space-between;gap:16px;margin:34px 0 25px}.page-heading h1{font-size:28px;font-weight:500;letter-spacing:-1.1px;margin:3px 0 4px}.page-heading p{color:var(--muted);font-size:13px;margin:0}.badge{display:inline-flex;gap:7px;align-items:center;border:1px solid var(--line);border-radius:30px;padding:5px 10px;font-size:10px;white-space:nowrap;background:var(--paper);color:var(--muted)}.badge.on{background:#edf3e8;border-color:#dae4d2;color:#41653a}.hero{display:grid;grid-template-columns:1.25fr 1fr;min-height:278px;border-radius:var(--radius);background:#e7eadf;border:1px solid #dde2d4;overflow:hidden;position:relative}.hero-copy{padding:34px 36px;z-index:1}.hero .eyebrow{color:#66765c}.hero h2{font-family:Georgia,"Times New Roman",serif;font-size:clamp(30px,3vw,43px);font-weight:400;line-height:1.14;letter-spacing:-1.3px;margin:15px 0}.hero h2 em{font-weight:400;color:#58734d}.hero p{font-size:12px;color:#6c7765;max-width:320px;margin:0 0 22px}.primary,.ghost{border:1px solid var(--line);border-radius:9px;padding:10px 16px;min-height:43px;display:inline-flex;justify-content:center;align-items:center;gap:10px;font-size:12px;font-weight:600;background:transparent;white-space:nowrap}.primary{background:var(--green);color:#fff;border-color:var(--green)}.primary:hover:not(:disabled){background:#386249;box-shadow:0 3px 9px #284e3b1c}.ghost:hover:not(:disabled){background:var(--soft);border-color:#bcc8b6}.primary svg,.ghost svg{width:15px;height:15px}.signal-art{position:relative;display:grid;place-items:center;min-height:250px;background:radial-gradient(ellipse at center,#f6f6ee80,transparent 68%);overflow:hidden}.orbit{position:absolute;width:220px;height:220px;border:1px solid #bcc8ad80;border-radius:50%;transform:rotate(-20deg)}.orbit:before,.orbit:after{content:"";position:absolute;border:1px solid #bcc8ad55;border-radius:50%;inset:-32px}.orbit:after{inset:32px}.hub{width:100px;height:112px;background:linear-gradient(145deg,#fffef8,#edeee5);border:1px solid #fffef8;border-radius:23px;box-shadow:5px 14px 25px #4f624a20,inset -4px -4px 0 #d7dece;transform:rotate(-12deg);display:flex;flex-direction:column;align-items:center;justify-content:center;gap:10px;position:relative}.hub svg{width:32px;height:32px;color:var(--green)}.hub span{font:7px ui-monospace,monospace;letter-spacing:2px;color:#7f8a77}.hub i{width:5px;height:5px;border-radius:50%;background:#6f9557;box-shadow:0 0 8px #769a5899}.satellite{position:absolute;display:grid;place-items:center;width:42px;height:42px;border-radius:12px;background:#fafaf4;border:1px solid #d9dfcf;box-shadow:0 5px 12px #44563b0b;font-size:22px}.satellite.s1{top:27px;left:24%;color:var(--orange);transform:rotate(-9deg)}.satellite.s2{right:15%;top:95px;transform:rotate(10deg)}.satellite.s3{bottom:32px;left:24%;font:18px ui-monospace,monospace;transform:rotate(-7deg)}.signal-label{position:absolute;bottom:21px;right:22px;font:8px ui-monospace,monospace;color:#839078;letter-spacing:1.5px}.section-title{display:flex;justify-content:space-between;align-items:center;margin:30px 0 14px;gap:12px}.section-title h2{font-size:16px;font-weight:600;letter-spacing:-.35px;margin:0}.text-button{border:0;background:transparent;display:inline-flex;gap:7px;align-items:center;font-size:11px;color:var(--muted);padding:8px 0;min-height:36px}.text-button:hover{color:var(--green)}.text-button svg{width:14px;height:14px}.cards{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:16px}.card{background:var(--paper);border:1px solid var(--line);border-radius:14px;padding:22px;box-shadow:var(--shadow);display:flex;flex-direction:column;transition:border-color .2s,box-shadow .2s}.card:hover{border-color:#bac7b2;box-shadow:0 8px 22px #24342608}.card-top{display:flex;justify-content:space-between;align-items:center;gap:8px}.agent-icon{width:42px;height:42px;border-radius:12px;display:grid;place-items:center;background:#f1e6dc;color:#b36546;font-size:28px;flex-shrink:0}.agent-icon.cursor{background:#edf0ec;color:#4b554e;font-size:25px}.agent-icon.codex{background:#e6eceb;color:#446760;font:20px ui-monospace,monospace}.card h3{font-size:17px;font-weight:600;margin:18px 0 4px;letter-spacing:-.4px}.card p{font-size:12px;color:var(--muted);margin:0 0 22px;min-height:38px}.card button{width:100%;margin-top:auto}.card .badge{font-size:9px;padding:4px 8px}.bottom-grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;margin-top:24px}.info-panel{padding:23px 24px;border:1px solid var(--line);border-radius:14px}.info-panel h3{font-size:12px;font-weight:600;margin:0 0 10px;display:flex;align-items:center;gap:8px}.info-panel h3 svg{width:15px;height:15px}.info-panel p{font-size:11px;color:var(--muted);margin:0;line-height:1.8}.flow{display:flex;align-items:center;gap:10px;margin-bottom:12px;font-size:11px;color:#586650}.flow .node{padding:5px 9px;background:#e9ece2;border-radius:6px}.flow svg{width:13px;height:13px}.page-footer{font-size:10px;color:#858b7f;margin-top:25px;display:flex;align-items:center;gap:8px;flex-wrap:wrap}.page-footer svg{width:12px;height:12px} #where{margin:0;overflow-wrap:anywhere}.banner{padding:10px 14px;margin-top:15px;font-size:12px;border-radius:9px;background:#e6ecdc;color:#435b37;display:flex;align-items:center;gap:9px}.banner.bad{background:#f5e7df;color:var(--red)}.banner button{margin-left:auto;border:0;background:transparent;text-decoration:underline;font-size:12px;min-height:28px}
.chat-layout{display:grid;grid-template-columns:190px minmax(0,1fr);gap:22px;height:calc(100dvh - 252px);min-height:480px;max-height:1000px}.chat-rail{padding-top:5px}.chat-rail>.eyebrow{padding:0 10px}.picker{display:grid;gap:7px;margin:13px 0 20px}.picker button{display:flex;align-items:center;gap:10px;padding:11px;border:1px solid transparent;background:transparent;border-radius:11px;text-align:left;width:100%}.picker button:hover{background:#eeeFE7}.picker button.on{background:#e5ebdf;border-color:#d7e0cd}.picker .agent-icon{width:33px;height:33px;font-size:21px;border-radius:9px}.picker .agent-icon.codex{font-size:15px}.picker strong{display:block;font-size:12px;font-weight:600}.picker small{display:block;font-size:10px;color:var(--muted)}.rail-note{border-top:1px solid var(--line);padding:17px 10px;font-size:11px;color:var(--muted)}.rail-note p{margin:0 0 10px}.conversation{background:var(--paper);border:1px solid var(--line);border-radius:16px;display:flex;flex-direction:column;min-height:0;min-width:0;box-shadow:var(--shadow);overflow:hidden}.conversation-header{padding:16px 22px;border-bottom:1px solid #e9e9e1;display:flex;justify-content:space-between;gap:10px;align-items:center}.conversation-header strong{font-size:13px;font-weight:600}.conversation-header p{font-size:10px;color:var(--muted);margin:2px 0 0}.conversation-header .badge{font-size:9px}.log{flex:1;overflow:auto;overscroll-behavior:contain;padding:24px;display:flex;flex-direction:column;gap:18px;min-height:160px;scrollbar-width:thin;scrollbar-color:#d8dccf transparent}.empty{margin:auto;text-align:center;padding:24px 8px;max-width:400px}.empty .agent-icon{width:58px;height:58px;border-radius:18px;margin:0 auto 20px;font-size:34px}.empty .agent-icon.codex{font-size:26px}.empty h2{font-family:Georgia,serif;font-weight:400;letter-spacing:-.7px;font-size:28px;margin:0 0 10px}.empty p{font-size:12px;color:var(--muted);margin:0 auto 20px;max-width:300px}.suggestions{display:flex;gap:8px;justify-content:center;flex-wrap:wrap}.suggestions button{background:transparent;border:1px solid var(--line);border-radius:7px;padding:8px 10px;min-height:36px;font-size:10px;color:#65715d}.suggestions button:hover{background:var(--soft)}.bubble{max-width:90%;width:fit-content;overflow-wrap:anywhere;white-space:pre-wrap;font-size:13px;line-height:1.75;padding:14px 17px;border-radius:13px;background:#f1f2eb}.bubble.you{margin-left:auto;background:#e8eee2;border-bottom-right-radius:4px}.bubble.agent{border-bottom-left-radius:4px}.bubble.system{color:var(--red);border:1px solid #ead9d1;background:#fcf5f0;font-size:12px}.who{display:block;font-size:10px;color:var(--muted);margin-bottom:5px}.typing{display:flex;align-items:center;gap:5px;padding:12px 5px;font-size:11px;color:var(--muted)}.typing i{width:4px;height:4px;background:#819075;border-radius:50%;animation:pulse 1.2s infinite}.typing i:nth-child(2){animation-delay:.15s}.typing i:nth-child(3){animation-delay:.3s}.typing span{margin-left:6px}.composer{padding:14px 18px 12px;border-top:1px solid #e9e9e1}.compose-field{border:1px solid #d8dece;border-radius:11px;background:#fcfcf7;padding:12px;transition:border-color .2s,box-shadow .2s}.compose-field:focus-within{border-color:#8caa79;box-shadow:0 0 0 3px #8caa7912}.compose-field textarea{background:transparent;border:0;border-radius:0;box-shadow:none;padding:0;min-height:50px;max-height:150px;resize:none;font-size:14px;outline:none}.compose-actions{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-top:7px}.compose-actions small{font-size:10px;color:#8a9183}.compose-actions .primary{min-height:34px;padding:7px 12px;font-size:11px}.composer-foot{display:flex;justify-content:space-between;gap:8px;padding-top:8px;font-size:9px;color:var(--muted)}.composer-foot .note{font-size:10px;min-height:15px;margin:0}.char-limit{color:var(--red)!important}
.settings-layout{display:grid;grid-template-columns:230px minmax(0,1fr);gap:34px;max-width:930px}.sub{display:flex;gap:6px;background:#eaece3;border-radius:10px;padding:4px;margin-bottom:20px}.sub button{background:transparent;border:0;border-radius:7px;min-height:38px;padding:8px 14px;color:var(--muted);font-size:12px;flex:1}.sub button.on{background:var(--paper);box-shadow:0 1px 4px #2738250a;color:var(--green);font-weight:600}.settings-aside h2{font-size:15px;font-weight:600;margin:0 0 9px}.settings-aside p{font-size:12px;color:var(--muted);margin:0 0 19px}.help-box{padding:16px;border-radius:10px;border:1px solid var(--line);font-size:11px;color:var(--muted)}.help-box strong{display:block;font-weight:600;color:var(--ink);margin-bottom:7px}.help-box code{font-size:11px;color:var(--ink)}.panel{border:1px solid var(--line);border-radius:14px;padding:26px;background:var(--paper);display:grid;gap:20px;box-shadow:var(--shadow)}.panel h3{margin:0;font-size:17px;font-weight:500;letter-spacing:-.4px}.panel-heading p{font-size:11px;color:var(--muted);margin:5px 0 0}label{display:grid;gap:7px;font-size:12px;font-weight:500;align-content:start}label>small{font-size:10px;color:var(--muted);font-weight:400}input,textarea,select{width:100%;border:1px solid #dcdfd3;border-radius:8px;background:#fbfcf7;padding:11px 12px;font-size:16px;transition:border-color .2s;min-width:0}select{font:inherit;color:inherit;appearance:none;-webkit-appearance:none;background-image:linear-gradient(45deg,transparent 50%,#839078 50%),linear-gradient(135deg,#839078 50%,transparent 50%);background-position:calc(100% - 18px) 50%,calc(100% - 13px) 50%;background-size:5px 5px;background-repeat:no-repeat;padding-right:36px}.voice-stack{display:grid;gap:18px}.key-row{display:grid;gap:8px;padding:14px 0;border-top:1px solid #e9e9e1}.key-row:first-of-type{border-top:0;padding-top:0}.key-row:last-of-type{padding-bottom:0}.key-head{display:flex;justify-content:space-between;align-items:center;gap:10px}.key-head strong{font-size:13px;font-weight:600}.key-input{display:flex;gap:8px}.key-input input{flex:1}.key-actions{display:flex;gap:8px;flex-wrap:wrap}.key-row .note{margin:0;min-height:0}.ghost.sm,.primary.sm{min-height:38px;padding:7px 14px;font-size:11px}.badge.on{color:var(--green);border-color:var(--green)}
.compose-tools{display:flex;align-items:center;gap:6px}.icon-btn{width:34px;height:34px;border:1px solid var(--line);border-radius:9px;background:transparent;display:inline-flex;align-items:center;justify-content:center;color:var(--muted);padding:0}.icon-btn svg{width:16px;height:16px}.icon-btn.on{color:var(--green);border-color:var(--green);background:#edf3e8}.icon-btn.rec{color:#fff;background:#b8392e;border-color:#b8392e;animation:rec-pulse 1.1s ease-in-out infinite}.icon-btn:disabled{opacity:.45}@keyframes rec-pulse{50%{opacity:.55}}
.voice-meta{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}.voice-meta div{border:1px solid var(--line);border-radius:10px;padding:12px;font-size:11px;color:var(--muted)}.voice-meta strong{display:block;font-size:13px;color:var(--ink);margin-bottom:4px}@media(max-width:600px){.voice-meta{grid-template-columns:1fr}}input:hover{border-color:#bdc9b3}input::placeholder,textarea::placeholder{color:#929a8a;font-size:12px}input:focus{outline:2px solid #a6bc9650;outline-offset:1px;border-color:#8fa67b}.row{display:flex;gap:12px;align-items:center;justify-content:space-between;border-top:1px solid #e9e9e1;padding-top:18px}.note{font-size:11px;color:var(--muted);margin:0;min-height:17px}.note.bad{color:var(--red)}.note.good{color:#49743c}.field-pair{display:grid;grid-template-columns:1fr 1fr;gap:16px;align-items:start}.settings-content{min-width:0}.form-empty{padding:24px;color:var(--muted);font-size:12px}.sr-only{position:absolute;width:1px;height:1px;padding:0;margin:-1px;overflow:hidden;clip:rect(0,0,0,0);white-space:nowrap;border:0}
.agent-icon svg,.satellite svg{width:25px;height:25px;stroke:none;fill:currentColor}.agent-icon.cursor,.agent-icon.codex{color:#14120b;background:#eeeee8}.satellite{color:#14120b}.picker .agent-icon svg{width:21px;height:21px}.empty .agent-icon svg{width:32px;height:32px}.brand-mark svg{stroke:currentColor;fill:none}.agent-icon.claude{background:#f4eae0}.scan-row{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap}.nets{display:grid;gap:8px}.nets button{display:flex;justify-content:space-between;align-items:center;gap:12px;width:100%;text-align:left;background:#fbfcf7;border:1px solid #dcdfd3;border-radius:8px;padding:11px 12px;min-height:44px;font-size:13px;font-weight:500}.nets button small{color:var(--muted);font-size:11px;font-weight:500}.nets button.on{border-color:var(--green);background:#edf3e8;color:var(--green)}
@media(max-width:600px){.picker .agent-icon svg{width:19px;height:19px}.satellite svg{width:20px;height:20px}.card h3{overflow-wrap:anywhere}.section-title h2{font-size:14px}.section-title .badge{font-size:9px;margin-left:3px!important}}
@keyframes arrive{from{opacity:0;transform:translateY(5px)}to{opacity:1;transform:translateY(0)}}@keyframes pulse{50%{opacity:.3;transform:translateY(-2px)}}
@media(min-width:1500px){.workspace{padding-inline:64px}.topbar{max-width:1180px;margin:auto}.hero{min-height:310px}.hero-copy{padding:40px}.card{padding:25px}}
@media(max-width:1100px){.shell{grid-template-columns:185px minmax(0,1fr)}.sidebar{padding:28px 15px 20px}.workspace{padding-inline:26px}.hero-copy{padding:28px}.card{padding:17px}.card-top{align-items:flex-start;gap:6px}.card .badge{font-size:8px;padding:4px 6px}.settings-layout{grid-template-columns:180px minmax(0,1fr);gap:24px}.chat-layout{grid-template-columns:155px minmax(0,1fr);gap:15px}}
@media(max-width:800px){.shell{grid-template-columns:1fr}.sidebar{position:relative;height:auto;padding:16px 22px;flex-direction:row;align-items:center;justify-content:space-between;border-right:0;border-bottom:1px solid var(--line);gap:20px}.sidebar-label,.device,.sidebar-foot{display:none}.nav{display:flex;gap:3px}.nav button{padding:10px;min-height:42px;gap:7px}.nav button.on:after{display:none}.nav svg{width:17px;height:17px}.brand{font-size:18px;white-space:nowrap}.brand small{display:none}.brand-mark{width:30px;height:30px}.workspace{padding:0 24px 25px}.topbar{height:56px}.page-heading{margin-top:25px}.hero{min-height:250px}.hero h2{font-size:36px}.hero-copy{padding:28px}.signal-art{min-height:245px}.chat-layout{height:calc(100dvh - 284px);min-height:460px}.settings-layout{grid-template-columns:180px minmax(0,1fr)}.panel{padding:22px}}
@media(max-width:600px){.sidebar{padding:13px 16px;gap:10px}.brand{gap:8px;font-size:17px}.nav{gap:2px}.nav button{font-size:0;gap:0;width:42px;justify-content:center}.nav svg{width:19px;height:19px}.workspace{padding:0 18px 22px}.topbar{height:48px}.breadcrumb{font-size:10px;gap:8px}.top-status{font-size:10px}.page-heading{margin:23px 0 20px;gap:10px}.page-heading h1{font-size:25px}.page-heading p{font-size:11px}.page-heading>.badge{display:none}.hero{grid-template-columns:1fr;min-height:0}.hero-copy{padding:26px}.hero h2{font-size:36px;max-width:260px;margin:12px 0}.hero p{max-width:250px;font-size:12px}.signal-art{position:absolute;right:-95px;bottom:-37px;width:225px;height:225px;min-height:0;opacity:.45;pointer-events:none}.hero-copy{padding-right:54px}.hero .primary{position:relative}.hub{width:73px;height:86px}.orbit{width:170px;height:170px}.satellite{width:32px;height:32px;font-size:18px}.satellite.s1{top:15px}.satellite.s2{right:12%;top:88px}.satellite.s3{bottom:20px}.signal-label{display:none}.section-title{margin-top:24px}.cards{grid-template-columns:1fr;gap:12px}.card{padding:18px;display:grid;grid-template-columns:1fr auto;gap:0 12px}.card-top{grid-column:1/-1;align-items:center}.card .badge{font-size:9px;padding:4px 9px}.card h3{margin:12px 0 3px}.card p{grid-column:1;min-height:0;margin:0;max-width:210px;font-size:11px}.card button{grid-column:2;grid-row:2/4;align-self:end;width:auto;min-height:40px;padding:9px 12px;font-size:11px}.card button svg{width:13px}.agent-icon{width:38px;height:38px}.bottom-grid{grid-template-columns:1fr;gap:12px;margin-top:20px}.info-panel{padding:20px}.page-footer{font-size:9px;margin-top:20px}.chat-heading{margin-bottom:16px}.chat-layout{grid-template-columns:1fr;gap:10px;height:calc(100dvh - 246px);min-height:440px;max-height:none;grid-template-rows:auto minmax(0,1fr)}.chat-rail{padding:0;min-width:0}.chat-rail>.eyebrow,.rail-note{display:none}.picker{display:flex;margin:0;gap:6px}.picker button{padding:8px;flex:1;min-width:0;gap:6px;border:1px solid var(--line);background:#fafaf4}.picker .agent-icon{width:24px;height:27px;font-size:18px;border-radius:7px}.picker .agent-icon.codex{font-size:12px}.picker strong{font-size:10px;max-width:65px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.picker small{font-size:8px}.conversation{border-radius:12px}.conversation-header{padding:12px 15px}.conversation-header strong{font-size:12px}.log{padding:16px;gap:14px}.bubble{max-width:94%;font-size:13px;padding:12px 14px}.empty{padding:18px 0}.empty h2{font-size:25px}.empty .agent-icon{margin-bottom:14px}.empty p{font-size:11px}.composer{padding:10px}.compose-field{padding:10px}.compose-field textarea{font-size:16px}.composer-foot{font-size:8px}.keyboard-hint{display:none}.settings-layout{grid-template-columns:1fr;gap:20px}.settings-aside p{margin-bottom:12px}.settings-aside .help-box{display:none}.panel{padding:20px;gap:18px}.field-pair{grid-template-columns:1fr}.sub{margin-bottom:15px}.settings-content .help-box{margin-top:15px}.banner{font-size:11px;margin-top:10px}.flow{gap:7px}.flow .node{padding:5px 8px}}
@media(prefers-reduced-motion:reduce){*,*:before,*:after{animation:none!important;transition:none!important;scroll-behavior:auto!important}}

body:has(#wifi-dialog[open]){overflow:hidden}.wifi-summary{display:flex;gap:14px;align-items:center;min-width:0}.wifi-summary strong{font-size:15px;overflow-wrap:anywhere}.wifi-emblem{display:grid;place-items:center;width:44px;height:44px;flex-shrink:0;border-radius:14px;background:var(--soft);color:var(--green)}
#wifi-dialog{width:calc(100% - 32px);max-width:460px;max-height:calc(100dvh - 32px);padding:28px;border:1px solid var(--line);border-radius:22px;background:var(--paper);color:var(--ink);box-shadow:0 24px 100px #18291e40;overflow:auto;overscroll-behavior:contain}#wifi-dialog::backdrop{background:#18251dc0;backdrop-filter:blur(5px)}.wifi-modal-head{display:flex;align-items:center;justify-content:space-between}#wifi-close{width:44px;padding:0;border:0;font-size:18px}#wifi-title{font-size:25px;letter-spacing:-.8px;font-weight:500;margin:22px 0 6px}#wifi-description{font-size:13px;color:var(--muted);margin:0 0 24px}#scan-note{margin:8px 0 12px;min-height:18px}#networks{max-height:300px;overflow:auto;padding:4px;margin:0 -4px}.nets button{background:transparent;min-height:70px;border-color:var(--line);padding:12px;gap:12px}.nets button:hover{background:var(--soft);border-color:#a8bba0}.net-copy{flex:1;min-width:0}.net-copy strong{display:block;overflow-wrap:anywhere;font-size:13px}.net-copy small{display:block;margin-top:3px}.net-signal{display:flex;align-items:flex-end;gap:3px;height:21px;width:25px;flex-shrink:0}.net-signal i{width:4px;border-radius:2px;background:#d7dccf}.net-signal i:nth-child(1){height:6px}.net-signal i:nth-child(2){height:11px}.net-signal i:nth-child(3){height:16px}.net-signal i:nth-child(4){height:21px}.net-signal i.lit{background:var(--green)}.nets button>svg{width:15px;height:15px;color:var(--muted)}.wifi-manual{width:100%;margin-top:18px;border-style:dashed}.wifi-footnote{margin-top:12px;text-align:center}#wifi-form{display:grid;gap:18px}#wifi-back{justify-self:start}.wifi-selected{padding:14px;background:var(--soft);border-radius:12px;overflow-wrap:anywhere;font-size:14px;font-weight:600}.password-wrap{display:flex;position:relative;margin:7px 0 8px}.password-wrap input{padding-right:68px}.password-wrap button{position:absolute;right:4px;top:3px;bottom:3px;min-width:56px;border:0;background:transparent;color:var(--green);font-size:12px;font-weight:600}.wifi-actions{display:flex;justify-content:flex-end;gap:10px;border-top:1px solid var(--line);padding-top:20px}.wifi-actions button{min-width:100px}#networks[aria-busy="true"]{opacity:.5}.scan-loading:before{content:"";display:inline-block;width:10px;height:10px;border:2px solid var(--line);border-top-color:var(--green);border-radius:50%;margin-right:8px;animation:wifi-spin .8s linear infinite}@keyframes wifi-spin{to{transform:rotate(360deg)}}@media(max-width:600px){#wifi-dialog{padding:22px;max-height:calc(100dvh - 16px);margin:auto auto 8px;border-radius:22px}#networks{max-height:34dvh}.wifi-actions button{flex:1}}
/* Clearer voice hierarchy and comfortable controls across screen sizes. */
.advanced-setting summary{cursor:pointer;font-size:12px;color:var(--green);padding:8px 0}.advanced-setting[open] summary{margin-bottom:10px}.voice-tabs{margin:0;min-height:48px}.voice-tabs button{font-size:13px}.voice-stack>.panel{gap:18px}.voice-stack .panel-heading{padding-bottom:14px;border-bottom:1px solid #e9e9e1}.panel-heading p{font-size:12px;line-height:1.65}.voice-meta{grid-template-columns:1fr 1fr}.voice-save{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:0 4px}.voice-save .note{flex:1}.key-row{gap:12px}.key-input input{width:0;min-width:0}.key-actions .ghost:last-child{color:var(--red)}.settings-aside>.sub{flex-wrap:wrap}.icon-btn{width:40px;height:40px}.info-panel p{font-size:12px}.help-box{line-height:1.7}select:focus-visible{outline:3px solid #bc8546;outline-offset:3px}
@media(max-width:600px){.voice-save{align-items:flex-start;flex-direction:column;gap:0}.voice-tabs button{padding:10px}.voice-meta{grid-template-columns:1fr 1fr}.key-input{flex-wrap:wrap}.key-input input{width:100%;flex-basis:100%}.key-input .primary{width:100%}.key-actions button{flex:1}.voice-stack .panel{padding:20px}.voice-stack{gap:16px}}
@media(prefers-reduced-motion:reduce){html{scroll-behavior:auto}*,*:before,*:after{animation:none!important;transition:none!important}}
</style>
</head>
<body>
<svg class="icon-defs" aria-hidden="true"><defs>
<symbol id="i-grid" viewBox="0 0 24 24"><rect x="3" y="3" width="7" height="7" rx="2"/><rect x="14" y="3" width="7" height="7" rx="2"/><rect x="3" y="14" width="7" height="7" rx="2"/><rect x="14" y="14" width="7" height="7" rx="2"/></symbol>
<symbol id="i-chat" viewBox="0 0 24 24"><path d="M21 11.5a8.5 8.5 0 0 1-8.5 8.5H4l-2 2V11.5A8.5 8.5 0 0 1 10.5 3h2a8.5 8.5 0 0 1 8.5 8.5Z"/><path d="M7 10h9M7 14h5"/></symbol>
<symbol id="i-settings" viewBox="0 0 24 24"><path d="M4 6h16M4 12h16M4 18h16"/><circle cx="9" cy="6" r="2" fill="currentColor"/><circle cx="15" cy="12" r="2" fill="currentColor"/><circle cx="8" cy="18" r="2" fill="currentColor"/></symbol>
<symbol id="i-arrow" viewBox="0 0 24 24"><path d="M5 12h14m-5-5 5 5-5 5"/></symbol>
<symbol id="i-chip" viewBox="0 0 24 24"><rect x="5" y="5" width="14" height="14" rx="4"/><path d="M9 1v4m6-4v4M9 19v4m6-4v4M1 9h4m-4 6h4m14-6h4m-4 6h4"/><rect x="9" y="9" width="6" height="6" rx="1"/></symbol>
<symbol id="i-wifi" viewBox="0 0 24 24"><path d="M2 8a16 16 0 0 1 20 0M5 12a11 11 0 0 1 14 0m-11 4a6 6 0 0 1 8 0"/><circle cx="12" cy="20" r="1"/></symbol>
<symbol id="i-send" viewBox="0 0 24 24"><path d="m12 19 0-14m-6 6 6-6 6 6"/></symbol><symbol id="i-mic" viewBox="0 0 24 24"><rect x="9" y="3" width="6" height="11" rx="3"/><path d="M5 11a7 7 0 0 0 14 0M12 18v3m-4 0h8"/></symbol><symbol id="i-speaker" viewBox="0 0 24 24"><path d="M4 10v4h4l5 4V6l-5 4H4z"/><path d="M16 9.5a3.5 3.5 0 0 1 0 5M18.5 7a7 7 0 0 1 0 10"/></symbol><symbol id="i-stop" viewBox="0 0 24 24"><rect x="7" y="7" width="10" height="10" rx="2"/></symbol>
<symbol id="i-lock" viewBox="0 0 24 24"><rect x="5" y="10" width="14" height="11" rx="3"/><path d="M8 10V7a4 4 0 0 1 8 0v3m-4 5v2"/></symbol>
<symbol id="i-claude" viewBox="0 0 32 32"><image width="32" height="32" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAF+0lEQVRYha2XbXBUZxXHf+e5u4SkCZjg0NIWnQLJLgRF5UWnL75MP9gMjqPT2eyC0qlMiYxOUAR206LOyhiSTVscp52O0xamWgu7WTp0qLUz6kht6Qed1L7M5GUTsAoDTRSIJJos2b33+GFfWMJuNqmcT/c595z/+d3nOc+9zxXKWE9Li7tm0fjyqmT6H0t/Fp8sFz9XM+UCauouH8Rx+ifmWWdP7Q0sLRbT98g3lySC/mcSQd+GGwrQt2vzx1G2ZEa6KJ3S1mJxlpOKIDyEmEM3FGCk5vw5YCw3FmFbb9hXfV2gcm/26vINBfhS+LU0qgcKXB9xT1hbr6kdDhtgcYaDxHSNwVBg92Ao0DMY8n97zgAAZnK8U2AoX1D0++rzWfkC44k6wAVghP7C3EQo8Iiijyq61oEDiV2bPjojwKk234pEsPnh/lDgzpyv/olXrwi6qyDsjsQy2ZQXmJdefPWWvp0v3uZ/CLQ9NxZINpxJj84IYDvmOUT2G/T1DD0CUB/pflnhd1eT5Mcnwl90Adi23JzPl9TbAEN7fF9AeapQW5AOicftGQFEOJ+9tEDbB9v8x99r21wLYBlnJ5AGUKhfklz8QEbY3J71nVnZceziUPAbtzvGxAB3wdPH6yPRx6cXvw4gbbm/B5zIO5SvVKjdMxAMrKvviPcpHMyLqvyoN+ybh7AUwIi+M9TaVOGI/SJwc4Fsf6rS2SqgxQBkukPDYTM40RdCZB/Z5gKmEGmzjStq2alBoDqbvF2hEWgF9oEuAvlugdx/Deaz9ZEjvcWKFwXIWaLNv14cDqmwugDvFRVOi8qOrOMdhT6BzQhvoNxzjbjItxo6o8+VqjEjAMBQa1OFXbXwh4KGuLqmDleXToHzwG1FpLs9kah/uvf98IPz08nkSuZbp+vDL4zNCJAHaQt82lH9JfCJ2cQrTFjq8hiZGrfVtQGcDQhrFNYAywELuFw15SwRgN6dvrrahcnkreGXJ0pCtDZVOFULwsCerEBJExhVGAY8FH/Z2cBvPJHY1ySx13cbaXMaqACSwCVRLqlhVNBRVTOm4lxCOSdqPkB0FdA2m5kosGGEHpC3UP2z6uRJb9fx8SwsJEL+nwBNwEKgWqBSoXaORQqtH+VNhDdErJMNnYf/ViqwbBNO3lRbVZFSS9yyIG1jucRuVuGnM6TZZL4d/wS9KEgawIH/gF5EzAWBvzecto9KPG7PqgkBTu0NLHVs7VBlcxnwS8ALgtyp6Kco3S9/OF850lQWYCD41RpMZUiUH5BZmgkRDqmyVaCqeJYeS1dqwJq4UoG56S5w7hb4PMp6YH426LJR1+qSAOrzWYPLZCvIPuAWAIHXxGGXY3geWAV6DOTrJRR+r5q8P9dskNtJ1etBVl4R19FPdh4eLQowEPR5RKxfgebOeGMqGvLMX/V0YrK/W+B+4ATCYZRnEJ5E+Q5gFM4AFwQ+I8hbLks2Ltt/ZKTUg163Rwfamrcj5q/54spvLZes9nZ2/yIx0b8jW/wCLmeLOLoCAEdeAjmYnaWPGXga9Jiia1O28+bAbt8dswJQEFF5IrO2chGVBzxdsY0r2qNnBx5uvkuELkAFedDTHj+nIl4AIzJsjL2XTAOisB+XtqryKLBcLHNyKLSpsSyAgKrKJoRtbksaPV3R5yFz7BaHOOBGOdAQib6SRV4L4KQZqe+I/wvYmZWqE1ue8nbFggjbgDoH/VOxY3vZXdDT0uKuqR37I+jdIH9JV9r3NIbjU717fLe4jPkASDVEYhW5730i2PwqIvdlZkK3eCPdv06E/GuAbmCBJxJbUnIGill13b8fyxRnRG070BiOTwG4MeuyszBSeNiw3KZFYDTzdPLzU7u3LPZEYu+qTq5Tlev+K2YESAQD94rKjqzgl72Pxd/P3VNLsgAyXJizoj161kG3Z4d1aZP6HIC36/i4tyt6dE4AasQNDIhxNnoisXevvZndJcLw9DxvpLsb5XGE9yzD2ZlqzPpVPN0SIf854FaUZz1dsW0fVqdsD5Q05Qhgg7z+oTX+Xyv2pzNX+x+eTlGviPgh3AAAAABJRU5ErkJggg=="/></symbol>
<symbol id="i-cursor" viewBox="0 0 466.73 532.09"><path fill="#14120b" d="M457.43,125.94L244.42,2.96c-6.84-3.95-15.28-3.95-22.12,0L9.3,125.94c-5.75,3.32-9.3,9.46-9.3,16.11v247.99c0,6.65,3.55,12.79,9.3,16.11l213.01,122.98c6.84,3.95,15.28,3.95,22.12,0l213.01-122.98c5.75-3.32,9.3-9.46,9.3-16.11v-247.99c0-6.65-3.55-12.79-9.3-16.11h-.01ZM444.05,151.99l-205.63,356.16c-1.39,2.4-5.06,1.42-5.06-1.36v-233.21c0-4.66-2.49-8.97-6.53-11.31L24.87,145.67c-2.4-1.39-1.42-5.06,1.36-5.06h411.26c5.84,0,9.49,6.33,6.57,11.39h-.01Z"/></symbol>
<symbol id="i-codex" viewBox="0 0 100 100"><path color="currentColor" d="M38.355 36.52v-9.415c0-.793.297-1.388.99-1.784l18.93-10.902c2.578-1.486 5.65-2.18 8.82-2.18 11.894 0 19.426 9.218 19.426 19.029 0 .694 0 1.486-.1 2.28L66.799 22.05c-1.189-.694-2.379-.694-3.568 0L38.355 36.52Zm44.202 36.67V50.694c0-1.388-.596-2.38-1.785-3.073L55.897 33.15l8.126-4.658c.694-.396 1.289-.396 1.982 0l18.93 10.902c5.452 3.172 9.118 9.91 9.118 16.452 0 7.531-4.46 14.47-11.496 17.344Zm-50.05-19.82-8.127-4.757c-.693-.396-.99-.99-.99-1.784V25.025c0-10.605 8.126-18.633 19.127-18.633 4.163 0 8.028 1.388 11.3 3.865l-19.525 11.3c-1.189.693-1.784 1.684-1.784 3.072v28.74ZM50 63.478l-11.645-6.541V43.062L50 36.522l11.645 6.54v13.875L50 63.477Zm7.483 30.129c-4.163 0-8.028-1.388-11.3-3.865l19.525-11.3c1.189-.693 1.784-1.684 1.784-3.071V46.629l8.226 4.757c.694.396.991.991.991 1.784v21.803c0 10.605-8.226 18.633-19.226 18.633v.001Zm-23.49-22.101-18.93-10.902c-5.45-3.172-9.117-9.91-9.117-16.451 0-7.632 4.559-14.47 11.595-17.344v22.596c0 1.388.595 2.379 1.784 3.072l24.777 14.37-8.126 4.659c-.694.396-1.289.396-1.982 0ZM32.905 87.76c-11.2 0-19.425-8.425-19.425-18.83 0-.794.1-1.587.198-2.38L33.2 77.85c1.189.693 2.379.693 3.568 0l24.876-14.37v9.415c0 .793-.298 1.388-.992 1.784L41.724 85.58c-2.576 1.486-5.649 2.18-8.82 2.18h.001Zm24.579 11.793c11.992 0 22.001-8.523 24.281-19.822C92.864 76.857 100 66.451 100 55.846c0-6.937-2.973-13.676-8.325-18.533.496-2.081.793-4.163.793-6.243 0-14.172-11.496-24.777-24.777-24.777-2.676 0-5.253.396-7.83 1.288C55.401 3.221 49.257.445 42.517.445c-11.992 0-22.001 8.523-24.281 19.822C7.136 23.14 0 33.547 0 44.152c0 6.938 2.973 13.676 8.325 18.533-.496 2.081-.793 4.163-.793 6.243 0 14.172 11.497 24.778 24.777 24.778 2.676 0 5.253-.397 7.83-1.289 4.459 4.36 10.604 7.136 17.344 7.136Z"></path></symbol>
</defs></svg>
<a class="skip" href="#main">Skip to content</a>
<div class="shell">
<aside class="sidebar">
<a class="brand" href="#home" aria-label="Desk agent home"><span class="brand-mark"><svg aria-hidden="true"><use href="#i-chip"/></svg></span>desk agent</a>
<p class="sidebar-label">Workspace</p>
<nav class="nav" aria-label="Main navigation">
<button type="button" data-tab="home" class="on" aria-current="page" title="Overview"><svg aria-hidden="true"><use href="#i-grid"/></svg>Overview</button>
<button type="button" data-tab="chat" title="Conversations"><svg aria-hidden="true"><use href="#i-chat"/></svg>Conversations</button>
<button type="button" data-tab="settings" title="Settings"><svg aria-hidden="true"><use href="#i-settings"/></svg>Settings</button>
</nav>
<div class="device"><div class="device-top"><span class="dot" id="device-dot"></span>Desk connection</div><p id="device-status">Connecting to your board…</p><p><code id="device-ip">Your desk device</code></p></div>
<div class="sidebar-foot"><svg aria-hidden="true"><use href="#i-chip"/></svg>A little closer to your agents.</div>
</aside>
<div class="workspace">
<header class="topbar"><div class="breadcrumb"><span>Workspace</span><span aria-hidden="true">/</span><span id="page-title">Overview</span></div><div class="top-status" role="status"><span class="dot" id="link-dot"></span><span id="link">Connecting…</span></div></header>
<div id="banner" class="banner" role="status" hidden><span id="banner-text"></span><button id="retry" type="button" hidden>Retry</button></div>
<main id="main" tabindex="-1">
<section id="home" aria-labelledby="home-title">
<div class="page-heading"><div><div class="eyebrow">Your desk, connected</div><h1 id="home-title">Your agents, within reach.</h1><p>Pick up a conversation, share an idea, or ask for a hand.</p></div><span class="badge"><svg aria-hidden="true" style="width:12px;height:12px"><use href="#i-chip"/></svg>Your workspace</span></div>
<div class="hero"><div class="hero-copy"><div class="eyebrow">Small device. Big possibilities.</div><h2>Good work starts<br>with a <em>conversation.</em></h2><p>Type a message or say it aloud. Keep the conversation going with the agents you already use.</p><button class="primary" id="start-chat">Start a conversation<svg aria-hidden="true"><use href="#i-arrow"/></svg></button></div><div class="signal-art" aria-hidden="true"><div class="orbit"></div><div class="hub"><svg><use href="#i-chip"/></svg><span>DESK / 01</span><i></i></div><div class="satellite s1"><svg><use href="#i-claude"/></svg></div><div class="satellite s2"><svg><use href="#i-cursor"/></svg></div><div class="satellite s3"><svg><use href="#i-codex"/></svg></div><span class="signal-label">A SIGNAL. A CONNECTION.</span></div></div>
<div class="section-title"><h2>Your agents <span id="agent-count" class="badge" style="margin-left:7px">— / 3 configured</span></h2><button class="text-button" id="manage-agents">Manage agents<svg aria-hidden="true"><use href="#i-arrow"/></svg></button></div>
<div class="cards" id="home-cards" aria-label="Agent connections"><p class="muted">Loading your agents…</p></div>
<div class="bottom-grid"><div class="info-panel"><h3><svg aria-hidden="true"><use href="#i-chat"/></svg>Pick up where you left off</h3><p>Choose an agent to see your recent conversation. Your history stays in this browser, ready for your next idea.</p></div><div class="info-panel"><h3><svg aria-hidden="true"><use href="#i-chip"/></svg>Give your keyboard a break</h3><p>Hold the talk button on your desk device, speak, and release to send. Set up Voice in Settings to hear replies aloud, too.</p></div></div>
<footer class="page-footer"><svg aria-hidden="true"><use href="#i-wifi"/></svg><p id="where">Finding your device on the network…</p></footer>
</section>
<section id="chat" hidden aria-labelledby="chat-title">
<div class="page-heading chat-heading"><div><div class="eyebrow">A direct line to your agents</div><h1 id="chat-title">Conversations</h1></div><span class="badge">History stays in this browser</span></div>
<div class="chat-layout"><aside class="chat-rail" aria-label="Choose an agent"><div class="eyebrow">Your agents</div><div class="picker" id="picker"></div><div class="rail-note"><p>Choose an agent to continue your conversation.</p><button class="text-button" id="chat-settings">Agent settings<svg aria-hidden="true"><use href="#i-arrow"/></svg></button></div></aside>
<div class="conversation"><div class="conversation-header"><div><strong id="chat-name">Your conversation</strong><p id="chat-detail">Choose an agent to begin</p></div><span id="chat-state" class="badge">Getting ready</span></div><div class="log" id="log" role="log" aria-label="Conversation" aria-live="polite" aria-relevant="additions"></div>
<form class="composer" id="composer"><div class="compose-field"><label class="sr-only" for="box">Message your agent</label><textarea id="box" rows="2" placeholder="What’s on your mind?" maxlength="2000" aria-describedby="chat-note char-count"></textarea><div class="compose-actions"><div class="compose-tools"><button class="icon-btn" type="button" id="mic" aria-pressed="false" aria-label="Talk through the desk microphone" title="Talk"><svg aria-hidden="true"><use href="#i-mic"/></svg></button><button class="icon-btn" type="button" id="speak" aria-pressed="false" aria-label="Read replies aloud" title="Read replies aloud"><svg aria-hidden="true"><use href="#i-speaker"/></svg></button><small><span id="char-count">0 / 2,000</span></small></div><button class="primary" type="submit" id="send" disabled>Send message<svg aria-hidden="true"><use href="#i-send"/></svg></button></div></div><div class="composer-foot"><p class="note" id="chat-note" role="status"></p><span class="keyboard-hint">⌘ / Ctrl + Enter to send</span></div></form></div></div>
</section>
<section id="settings" hidden aria-labelledby="settings-title">
<div class="page-heading"><div><div class="eyebrow">Make yourself at home</div><h1 id="settings-title">Workspace settings</h1><p>Manage your connections and make voice feel right for you.</p></div></div>
<div class="settings-layout"><aside class="settings-aside"><div class="sub" aria-label="Settings category"><button type="button" data-sub="wifi" class="on" aria-pressed="true">Wi-Fi</button><button type="button" data-sub="agents" aria-pressed="false">Agents</button><button type="button" data-sub="voice" aria-pressed="false">Voice</button></div><h2 id="settings-intro-title">Connect your desk.</h2><p id="settings-intro">Connect to the same network as your computer.</p><div class="help-box"><strong>Set up once. Keep going.</strong>Your connections and speech preferences are saved on your desk device, even after a restart.</div></aside>
<div class="settings-content"><div id="wifi"><div class="panel"><div class="panel-heading"><h3>Wi-Fi connection</h3><p>Choose the Wi-Fi network you use for your agents.</p></div><div class="wifi-summary"><span class="wifi-emblem"><svg aria-hidden="true"><use href="#i-wifi"/></svg></span><div><strong id="wifi-current">Checking connection…</strong><p id="wifi-state" class="note">Finding your board</p></div></div><button class="primary" type="button" id="choose-network">Choose a network<svg aria-hidden="true"><use href="#i-arrow"/></svg></button><p class="note" id="wifi-note" role="status">Choose a nearby network or enter one manually.</p></div><div class="help-box" style="margin-top:18px"><strong>Need to connect for the first time?</strong>Join <code>Esp32-Agent</code> with password <code>agent-setup</code>, then open <a href="http://192.168.4.1/">192.168.4.1</a>. The setup network is available until the board joins Wi-Fi.</div></div><div id="agents" hidden><div id="agent-forms"></div></div><div id="voice" hidden><div id="voice-form"></div></div></div></div>
</section>
</main>
</div></div>
<dialog id="wifi-dialog" aria-labelledby="wifi-title"><div class="wifi-modal-head"><span class="wifi-emblem"><svg aria-hidden="true"><use href="#i-wifi"/></svg></span><button class="ghost" id="wifi-close" type="button" aria-label="Close Wi-Fi setup">✕</button></div><h2 id="wifi-title" tabindex="-1">Choose a network</h2><p id="wifi-description">Nearby networks, discovered by your board.</p><div id="wifi-list-step"><div class="scan-row"><span class="eyebrow">Available networks</span><button class="text-button" type="button" id="scan">Scan again</button></div><p class="note" id="scan-note" role="status"></p><div id="networks" class="nets" aria-label="Nearby networks"></div><button class="ghost wifi-manual" type="button" id="wifi-manual">＋ Add network manually</button><p class="note wifi-footnote">Don’t see your network? Enter its name and password.</p></div><form id="wifi-form" hidden><button class="text-button" type="button" id="wifi-back">← All networks</button><div class="wifi-selected" id="wifi-selected"></div><label id="ssid-field" for="ssid">Network name<input id="ssid" autocomplete="off" autocapitalize="none" spellcheck="false" maxlength="32" placeholder="Enter network name" required></label><div id="password-field"><label for="wifi-pass">Password</label><div class="password-wrap"><input id="wifi-pass" type="password" autocomplete="new-password" maxlength="63" aria-describedby="password-help" placeholder="Enter network password"><button type="button" id="wifi-show" aria-label="Show password" aria-pressed="false">Show</button></div><p class="note" id="password-help"></p></div><p class="note" id="wifi-join-note" role="status"></p><div class="wifi-actions"><button class="ghost" id="wifi-cancel" type="button">Cancel</button><button class="primary" id="wifi-connect" type="submit">Connect</button></div></form></dialog>
<script>
'use strict';
const $ = (s) => document.querySelector(s);
const agentIds = ['claude', 'cursor', 'codex'];
const descriptions = { claude: 'A thoughtful partner for your next idea.', cursor: 'Keep your coding workflow in reach.', codex: 'Move from a thought to working code.' };
const storage = { get(kind, key) { try { return window[kind].getItem(key); } catch (_) { return null; } }, set(kind,key,value) { try { window[kind].setItem(key,value); } catch (_) {} }, remove(kind,key) { try { window[kind].removeItem(key); } catch (_) {} } };
let agents = [], current = storage.get('sessionStorage','desk-agent') || 'claude', agentTab = current;
let waitingJob = 0, sending = false, boardBusy = false, online = false, statusBusy = false, settingsBuilt = false, voiceBuilt = false, cardsSignature = '', draftAgent = current;
let voiceTab = 'credentials';
let voiceBusy = false, voiceSeen = storage.get('localStorage','desk-voice-seen') || '';
const drafts = {};
let statusAgents = [];
function el(tag, className, text) { const n = document.createElement(tag); if(className) n.className = className; if(text !== undefined) n.textContent = text; return n; }
function icon(name) { const svg = document.createElementNS('http://www.w3.org/2000/svg','svg'); svg.setAttribute('aria-hidden','true'); const use = document.createElementNS(svg.namespaceURI,'use'); use.setAttribute('href','#i-' + name); svg.append(use); return svg; }
function agentIcon(id) { const n = el('span','agent-icon ' + (agentIds.includes(id) ? id : '')); n.setAttribute('aria-hidden','true'); n.append(icon(agentIds.includes(id) ? id : 'chip')); return n; }
function button(text, cls, action) { const n = el('button',cls,text); n.type = 'button'; if(action) n.addEventListener('click',action); return n; }
function note(target,text,bad=false) { target.textContent = text; target.className = 'note' + (bad ? ' bad' : ''); }
function banner(text, bad=false) { $('#banner').hidden = !text; $('#banner-text').textContent = text; $('#banner').classList.toggle('bad',bad); $('#retry').hidden = !bad; }
async function api(path, body, timeout=12000) {
  const controller = new AbortController(); const timer = setTimeout(() => controller.abort(),timeout);
  try { const res = await fetch(path,{cache:'no-store',signal:controller.signal,...(body ? {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)} : {})}); const data = await res.json(); if(!res.ok) throw new Error(data.error || 'The board could not complete this request.'); return data; }
  catch(e) { if(e.name === 'AbortError') throw new Error('The board took too long to respond. Check your connection.'); if(e instanceof TypeError) throw new Error('Cannot reach your board. Check your Wi-Fi connection.'); throw e; }
  finally { clearTimeout(timer); }
}
function showTab(name, focus=false) {
  if(!['home','chat','settings'].includes(name)) name='home';
  ['home','chat','settings'].forEach(t => { $('#'+t).hidden=t!==name; const b=$('[data-tab="'+t+'"]'); b.classList.toggle('on',t===name); if(t===name)b.setAttribute('aria-current','page');else b.removeAttribute('aria-current'); });
  $('#page-title').textContent = {home:'Overview',chat:'Conversations',settings:'Settings'}[name];
  if(name==='chat') renderChat();
  if(name==='settings' && !settingsBuilt) renderAgents();
  if(location.hash!=='#'+name) history.replaceState(null,'','#'+name);
  if(focus) $('#main').focus({preventScroll:true});
}
function showSub(name) {
  document.querySelectorAll('[data-sub]').forEach(b => { const on=b.dataset.sub===name; b.classList.toggle('on',on);b.setAttribute('aria-pressed',String(on)); });
  $('#wifi').hidden=name!=='wifi'; $('#agents').hidden=name!=='agents'; $('#voice').hidden=name!=='voice';
  $('#settings-intro-title').textContent={wifi:'Connect your desk.',agents:'Bring your team along.',voice:'Say it out loud.'}[name];
  $('#settings-intro').textContent={wifi:'Connect to the same network as your computer.',agents:'Connect the agents you use on your computer. You’ll find these details in your Agora setup.',voice:'Connect a speech provider, then choose how your agent listens and speaks.'}[name];
  if(name==='agents' && !settingsBuilt) renderAgents();
  if(name==='voice' && !voiceBuilt) renderVoice();
}
function openSettings(id) { if(id && id!==agentTab) {agentTab=id;activateAgentForm();} showTab('settings');showSub('agents'); }
document.querySelectorAll('[data-tab]').forEach(b=>b.addEventListener('click',()=>showTab(b.dataset.tab)));
document.querySelectorAll('[data-sub]').forEach(b=>b.addEventListener('click',()=>showSub(b.dataset.sub)));
$('.brand').addEventListener('click',e=>{e.preventDefault();showTab('home');});
window.addEventListener('hashchange',()=>showTab(location.hash.slice(1)));
$('#manage-agents').addEventListener('click',()=>openSettings()); $('#chat-settings').addEventListener('click',()=>openSettings(current));
$('#start-chat').addEventListener('click',()=>showTab('chat'));
$('#retry').addEventListener('click',()=>{refreshStatus();loadAgents().catch(e=>banner(e.message,true));});
function loadLog(id) { try {const rows=JSON.parse(storage.get('localStorage','desk-log-'+id)||'[]');return Array.isArray(rows)?rows.filter(r=>r && typeof r.text==='string').slice(-40):[];} catch(_){return [];} }
const volatileLogs = {};
function logs(id) { return volatileLogs[id] || loadLog(id); }
function remember(id,row) { const rows=[...logs(id),row].slice(-40); volatileLogs[id]=rows; storage.set('localStorage','desk-log-'+id,JSON.stringify(rows)); }
function renderCards(list) {
  const sig=JSON.stringify(list); if(sig===cardsSignature)return; cardsSignature=sig;
  $('#home-cards').replaceChildren(); $('#agent-count').textContent=list.filter(a=>a.ready).length+' / '+list.length+' configured';
  list.forEach(a=>{const card=el('article','card');const top=el('div','card-top');const state=el('span','badge'+(a.ready?' on':''),a.ready?'Configured':'Not connected'); if(a.ready)state.prepend(el('span','dot on'));top.append(agentIcon(a.id),state);const action=button(a.ready?'Open conversation':'Set up agent',a.ready?'primary':'ghost',()=>{if(a.ready){selectAgent(a.id);showTab('chat');}else openSettings(a.id);});action.append(icon('arrow'));card.append(top,el('h3','',a.name||a.id),el('p','',descriptions[a.id]||'Your connection to this agent.'),action);$('#home-cards').append(card);});
}
async function refreshStatus() {
  if(statusBusy)return; statusBusy=true;
  try {const s=await api('/api/status');online=!!s.wifi;statusAgents=s.agents||[];const v=s.voice||{};trackVoice(v);boardBusy=!!s.listening||voiceBusy;
    $('#link').textContent=online?'Desk online':'Wi-Fi not connected';$('#link-dot').classList.toggle('on',online);$('#device-dot').classList.toggle('on',online);
    $('#device-status').textContent=online?(s.ssid||'Connected to Wi-Fi'):'Join a network in Settings';$('#device-ip').textContent=online?s.ip:'Setup · '+(s.ap_ip||'192.168.4.1');
    savedSsid=s.ssid||''; $('#wifi-current').textContent=savedSsid||'No network selected'; $('#wifi-state').textContent=online?'Connected · '+s.ip:(savedSsid?'Saved · Not connected':'Ready to connect');
    renderCards(statusAgents);
    $('#where').textContent=online?('Local device · '+s.ip+(s.host?' · '+s.host:'')):('Setup network · '+(s.ap_ip||'192.168.4.1'));
    if(!waitingJob && !sending) banner(voiceBusy?voiceBanner(v):boardBusy?'The board is listening for a reply in another conversation.':(!online?'Your board is available. Connect it to Wi-Fi in Settings to send messages.':''));
    syncComposer();
  } catch(e) {
    // While the board is uploading a clip or fetching speech it cannot answer; that is not an outage.
    if(voiceBusy||pageVoice){return;}
    online=false;$('#link').textContent='Desk unreachable';$('#link-dot').classList.remove('on');$('#device-dot').classList.remove('on');$('#device-status').textContent='Waiting for your board';if(!waitingJob)banner(e.message,true);if(!cardsSignature){$('#home-cards').textContent='Your agents will appear when the board reconnects.';}syncComposer();}
  finally{statusBusy=false;}
}
async function loadAgents() { const first=!agents.length;const data=await api('/api/agents');agents=data.agents||[];if(!agents.some(a=>a.id===current)) current=agents[0]?.id||'claude';if(!settingsBuilt)renderAgents();if(first&&voiceBuilt)renderVoice();if(!$('#chat').hidden)renderChat(); }
function agentLabel(id){const a=agents.find(a=>a.id===id)||statusAgents.find(a=>a.id===id);return a?.name||a?.title||id||'your agent';}
function voiceBanner(v){return {recording:v.source==='page'?'Listening to you… tap the mic again to send.':'Listening to you… let go of the button to send.',transcribing:'Writing down what you said…',waiting:'Waiting for '+agentLabel(v.agent)+' to reply…',speaking:'Reading the reply aloud…'}[v.state]||'';}
// Spoken exchanges happen on the board, not in this page. Copy each finished one into the chat log once.
// A "typed" source is a normal chat message being read aloud: the page already logged it.
let voiceState={state:'idle'};
function trackVoice(v){
  voiceState=v;voiceBusy=['recording','transcribing','waiting','speaking'].includes(v.state);
  if(audioMode==='desk')micRecording=v.state==='recording'&&v.source==='page';syncMic();
  if(!v.job||!(v.state==='done'||v.state==='failed'))return;
  const key=v.job+':'+(v.heard||'').slice(0,40)+':'+v.state;if(key===voiceSeen)return;voiceSeen=key;storage.set('localStorage','desk-voice-seen',key);
  const id=v.agent||current;
  if(v.source==='typed'){if(v.error)note($('#chat-note'),v.error,true);return;}
  if(v.heard)remember(id,{role:'you',text:v.heard,spoken:true});
  if(v.state==='done')remember(id,{role:'agent',name:agentLabel(id),text:v.reply||'(The agent returned an empty reply.)'});
  if(v.error)remember(id,{role:'system',text:v.error});
  if(!$('#chat').hidden&&id===current)renderChat();
}
// Chat audio has two homes. "browser": this page records with its own microphone, the board forwards the clip
// to the speech API and hands back the transcript, and spoken replies come back as WAV for the page to play.
// "desk": the mic button drives the board's INMP441 and replies play on the MAX98357A. The keys never leave the board.
let audioMode=storage.get('localStorage','desk-audio')==='desk'?'desk':'browser';
const micSupported=!!(navigator.mediaDevices&&navigator.mediaDevices.getUserMedia&&window.MediaRecorder);
const micBlockedWhy='This browser cannot use its microphone on this page. Choose Desk device in Settings › Voice › Speech & audio to use the desk microphone.';
function setAudioMode(mode){audioMode=mode==='desk'?'desk':'browser';storage.set('localStorage','desk-audio',audioMode);if(audioMode==='desk')stopBrowserRecording(true);else micRecording=false;stopPlayer();syncSpeak();syncMic();}
// Speaker toggle: off by default; when on, replies are read aloud (by the board or by this page, per audioMode).
let speakOn=storage.get('localStorage','desk-speak')==='1', micRecording=false, micBusy=false, pageVoice=false;
function syncSpeak(){$('#speak').classList.toggle('on',speakOn);$('#speak').setAttribute('aria-pressed',String(speakOn));$('#speak').title=speakOn?'Replies are read aloud':'Read replies aloud'+(audioMode==='desk'?' on the desk speaker':' in this browser');}
$('#speak').addEventListener('click',()=>{speakOn=!speakOn;storage.set('localStorage','desk-speak',speakOn?'1':'0');if(!speakOn)stopPlayer();syncSpeak();note($('#chat-note'),speakOn?(audioMode==='desk'?'Replies will be read aloud through the desk speaker.':'Replies will be read aloud in this browser.'):'Replies stay on screen.');});
syncSpeak();
let player=null;
function stopPlayer(){if(player){player.pause();player=null;}}
async function sayInBrowser(text){
  stopPlayer();pageVoice=true;note($('#chat-note'),'Reading the reply aloud…');
  const controller=new AbortController();const timer=setTimeout(()=>controller.abort(),90000);
  try{const res=await fetch('/api/voice/say',{method:'POST',cache:'no-store',headers:{'Content-Type':'application/json'},body:JSON.stringify({text}),signal:controller.signal});
    if(!res.ok){let msg='The board could not fetch the speech.';try{msg=(await res.json()).error||msg;}catch(_){}throw new Error(msg);}
    const url=URL.createObjectURL(await res.blob());const a=new Audio(url);player=a;a.addEventListener('ended',()=>{URL.revokeObjectURL(url);if(player===a)player=null;syncComposer();});await a.play();syncComposer();
  }catch(e){player=null;note($('#chat-note'),e.name==='AbortError'?'Speech took too long. Replies stay on screen this time.':e.name==='NotAllowedError'?'Your browser blocked playback. Tap the speaker icon once, then send again.':e.message,true);}
  finally{clearTimeout(timer);pageVoice=false;}
}
// Mic button. Desk mode opens the board's microphone for the open agent; tap again to send.
// Browser mode records here, sends the clip to the board for transcription, then posts the text like a typed message.
let recorder=null, recorderChunks=[];
function syncMic(){const m=$('#mic');const a=agents.find(a=>a.id===current);m.classList.toggle('rec',micRecording);m.setAttribute('aria-pressed',String(micRecording));m.querySelector('use').setAttribute('href',micRecording?'#i-stop':'#i-mic');
  m.title=micRecording?'Tap to send':audioMode==='desk'?'Talk through the desk microphone':micSupported?'Talk through this browser’s microphone':'Microphone needs a secure page';m.setAttribute('aria-label',m.title);
  m.disabled=micBusy||!online||!a?.ready||(!micRecording&&(voiceBusy||!!waitingJob||sending||boardBusy||pageVoice));}
function stopBrowserRecording(discard){const r=recorder;if(!r)return;recorder=null;if(discard)recorderChunks=[];try{if(r.state!=='inactive')r.stop();}catch(_){}r.stream.getTracks().forEach(t=>t.stop());}
function pickMime(){return ['audio/webm;codecs=opus','audio/webm','audio/mp4','audio/ogg;codecs=opus'].find(t=>MediaRecorder.isTypeSupported(t))||'';}
async function startBrowserRecording(){
  const stream=await navigator.mediaDevices.getUserMedia({audio:{channelCount:1,echoCancellation:true,noiseSuppression:true}});
  const mime=pickMime();const r=new MediaRecorder(stream,mime?{mimeType:mime}:undefined);recorderChunks=[];
  r.addEventListener('dataavailable',e=>{if(e.data&&e.data.size)recorderChunks.push(e.data);});
  r.addEventListener('stop',()=>{stream.getTracks().forEach(t=>t.stop());if(recorder===r)recorder=null;const chunks=recorderChunks;recorderChunks=[];if(chunks.length)finishBrowserRecording(new Blob(chunks,{type:r.mimeType||mime||'audio/webm'}));});
  recorder=r;r.start(250);
  // The board caps its own recordings at 15 s; a minute is plenty here and keeps the upload small.
  setTimeout(()=>{if(recorder===r&&micRecording){micRecording=false;stopBrowserRecording(false);syncMic();}},60000);
}
async function finishBrowserRecording(blob){
  const id=current;const ext=/mp4/.test(blob.type)?'mp4':/ogg/.test(blob.type)?'ogg':/wav/.test(blob.type)?'wav':'webm';
  if(blob.size<2000){note($('#chat-note'),'That was too short. Hold on a moment longer before tapping again.',true);return;}
  pageVoice=true;micBusy=true;syncMic();note($('#chat-note'),'Writing down what you said…');
  const form=new FormData();form.append('file',blob,'clip.'+ext);const controller=new AbortController();const timer=setTimeout(()=>controller.abort(),60000);
  try{const res=await fetch('/api/voice/transcribe',{method:'POST',cache:'no-store',body:form,signal:controller.signal});let data={};try{data=await res.json();}catch(_){}
    if(!res.ok)throw new Error(data.error||'The board could not transcribe that.');
    const text=(data.text||'').trim();if(!text){note($('#chat-note'),'I didn’t catch anything. Try again a little closer to the microphone.',true);return;}
    pageVoice=false;micBusy=false;await sendMessage(id,text,true);
  }catch(e){note($('#chat-note'),e.name==='AbortError'?'Transcription took too long. Try a shorter clip.':e.message,true);}
  finally{clearTimeout(timer);pageVoice=false;micBusy=false;syncMic();}
}
$('#mic').addEventListener('click',async()=>{
  if(micBusy)return;micBusy=true;const id=current;
  try{
    if(audioMode==='browser'){
      if(micRecording){micRecording=false;stopBrowserRecording(false);note($('#chat-note'),'Sending what you said…');}
      else{if(!micSupported)throw new Error(micBlockedWhy);await startBrowserRecording();micRecording=true;note($('#chat-note'),'Listening… tap the mic again to send.');}
    }else if(micRecording){await api('/api/voice/talk',{action:'stop'});micRecording=false;note($('#chat-note'),'Sending what you said…');pollVoice();}
    else{await api('/api/voice/talk',{action:'start',agent:id});micRecording=true;voiceBusy=true;note($('#chat-note'),'Listening… tap the mic again to send.');pollVoice();}
    syncMic();
  }catch(err){micRecording=false;stopBrowserRecording(true);note($('#chat-note'),err.name==='NotAllowedError'?'Microphone access was denied. Allow it in the browser’s site settings and try again.':err.name==='NotFoundError'?'No microphone was found on this device.':err.message,true);}
  finally{micBusy=false;syncMic();}
});
let voicePolling=false;
async function pollVoice(){if(voicePolling)return;voicePolling=true;try{do{await new Promise(r=>setTimeout(r,1000));await refreshStatus();}while(voiceBusy&&!document.hidden);}finally{voicePolling=false;}}
function selectAgent(id) {if(waitingJob || sending)return;drafts[draftAgent]=$('#box').value;current=id;draftAgent=id;$('#box').value=drafts[id]||'';storage.set('sessionStorage','desk-agent',id);renderChat();resizeComposer();}
function syncComposer() {
  const a=agents.find(a=>a.id===current); const bytes=new TextEncoder().encode($('#box').value.trim()).length;const busy=!!waitingJob||sending||boardBusy;
  $('#send').disabled=!online||busy||!a?.ready||!$('#box').value.trim()||bytes>2000;$('#box').disabled=!a?.ready;
  $('#send').firstChild.textContent=busy?'Waiting…':'Send message';syncMic();
  $('#char-count').textContent=bytes.toLocaleString()+' / 2,000 bytes';$('#char-count').classList.toggle('char-limit',bytes>2000);
  if(bytes>2000)note($('#chat-note'),'Message is too long. Shorten it to 2,000 bytes.',true);
  else if(!waitingJob&&!sending&&!pageVoice)note($('#chat-note'),!online?'Connect your board to Wi-Fi to send.':micRecording?'Listening… tap the mic again to send.':player?'Reading the reply aloud…':voiceBusy?voiceBanner(voiceState):boardBusy?'The board is busy listening.':a?.ready?'Ready for your next message.':'Set up this agent to start a conversation.');
}
function resizeComposer(){const box=$('#box');box.style.height='auto';box.style.height=Math.min(box.scrollHeight,150)+'px';syncComposer();}
$('#box').addEventListener('input',()=>{drafts[current]=$('#box').value;resizeComposer();});
$('#box').addEventListener('keydown',e=>{if(e.key==='Enter' && (e.metaKey||e.ctrlKey) && !e.isComposing){e.preventDefault();if(!$('#send').disabled)$('#composer').requestSubmit();}});
function renderChat(){
  const picker=$('#picker');picker.replaceChildren();
  agents.forEach(a=>{const b=button('','',()=>selectAgent(a.id));b.classList.toggle('on',a.id===current);b.setAttribute('aria-pressed',String(a.id===current));b.disabled=!!waitingJob||sending;const text=el('span');text.append(el('strong','',a.name||a.title||a.id),el('small','',a.ready?'Configured':'Setup needed'));b.append(agentIcon(a.id),text);picker.append(b);});
  const a=agents.find(a=>a.id===current);$('#chat-name').textContent=a?.name||a?.title||'Your conversation';$('#chat-detail').textContent=a?.ready?'Type a message or use the microphone':'Connect this agent to start chatting';$('#chat-state').textContent=waitingJob||sending?'Listening':a?.ready?'Configured':'Setup needed';$('#chat-state').classList.toggle('on',!!a?.ready);
  const log=$('#log');log.replaceChildren();const rows=logs(current);
  if(!rows.length){const empty=el('div','empty');empty.append(agentIcon(current),el('h2','',a?.ready?'What would you like to work on?':'Let’s make a connection.'),el('p','',a?.ready?'Ask a question, share an idea, or pick up where your work left off.':agents.length?'Add this agent’s connection details in Settings to start a conversation.':'Waiting for your board. Your agents will appear here once connected.'));
    if(a?.ready){const suggestions=el('div','suggestions');['What are you working on?','Help me plan my next step'].forEach(text=>suggestions.append(button(text,'',()=>{$('#box').value=text;resizeComposer();$('#box').focus();})));empty.append(suggestions);}else if(a)empty.append(button('Set up '+(a.name||a.title),'ghost',()=>openSettings(a.id)));log.append(empty);
  }else rows.forEach(row=>{const role=['you','agent','system'].includes(row.role)?row.role:'system';const bubble=el('div','bubble '+role);bubble.append(el('span','who',role==='you'?(row.spoken?'You · spoken':'You'):role==='agent'?(row.name||a?.name||'Agent'):'Connection update'),document.createTextNode(row.text));log.append(bubble);});
  if(waitingJob||sending){const typing=el('div','typing');typing.append(el('i'),el('i'),el('i'),el('span','',sending?'Sending your message…':'Giving your agent a moment…'));log.append(typing);}
  log.scrollTop=log.scrollHeight;syncComposer();
}
function field(label,value,opts={}){const wrap=el('label','',label);const input=el('input');input.value=value||'';input.type=opts.type||'text';input.autocomplete='off';input.maxLength=opts.max||180;input.placeholder=opts.placeholder||'';if(opts.required)input.required=true;if(opts.pattern)input.pattern=opts.pattern;wrap.append(input);if(opts.help)wrap.append(el('small','',opts.help));return {wrap,input};}
function activateAgentForm(){document.querySelectorAll('[data-agent-form]').forEach(f=>f.hidden=f.dataset.agentForm!==agentTab);document.querySelectorAll('[data-agent-tab]').forEach(b=>{b.classList.toggle('on',b.dataset.agentTab===agentTab);b.setAttribute('aria-pressed',String(b.dataset.agentTab===agentTab));});}
function renderAgents(){
  if(!agents.length){$('#agent-forms').replaceChildren(el('p','form-empty','Agent settings will appear when the board connects.'));return;}
  settingsBuilt=true;const root=$('#agent-forms');root.replaceChildren();if(!agents.some(a=>a.id===agentTab))agentTab=agents[0].id;
  const tabs=el('div','sub agents');agents.forEach(a=>{const b=button(a.name||a.title,'',()=>{agentTab=a.id;activateAgentForm();});b.dataset.agentTab=a.id;tabs.append(b);});root.append(tabs);
  agents.forEach(agent=>{const form=el('form','panel');form.dataset.agentForm=agent.id;const head=el('div','panel-heading');head.append(el('h3','',(agent.name||agent.title)+' connection'),el('p','','Use the same details as the bridge running on your computer.'));
    const name=field('Display name',agent.name,{max:40,required:true,placeholder:agent.title});const agentId=field('Agent ID',agent.agent_id,{max:64,pattern:'[A-Za-z0-9_\\-]+',help:'Use the agent identifier from your Agora setup.',placeholder:'claude-cli'});const url=field('Agora URL',agent.url,{type:'url',required:true,placeholder:'http://192.168.1.20:4470',help:'The server address, without an /api path.'});const channel=field('Channel ID',agent.channel,{max:80,required:true,pattern:'[A-Za-z0-9_\\-]+',placeholder:'general-a1b2'});const token=field('Access token','',{type:'password',max:500,required:!agent.token_set,placeholder:agent.token_set?'Saved — leave blank to keep it':'Paste your Agora access token',help:'Leave blank to keep your saved token.'});token.input.autocomplete='new-password';const pair=el('div','field-pair');pair.append(name.wrap,agentId.wrap);const row=el('div','row');const status=el('p','note');status.setAttribute('role','status');const save=el('button','primary','Save connection');save.type='submit';row.append(status,save);form.append(head,pair,url.wrap,channel.wrap,token.wrap,row);
    form.addEventListener('submit',async e=>{e.preventDefault();save.disabled=true;note(status,'Saving connection…');try{const data=await api('/api/agents',{id:agent.id,name:name.input.value.trim(),agent_id:agentId.input.value.trim(),url:url.input.value.trim(),channel:channel.input.value.trim(),token:token.input.value.trim()});note(status,data.ready?'Connection saved. Ready when you are.':'Saved. Complete the remaining details.');status.classList.add('good');token.input.value='';token.input.required=false;token.input.placeholder='Saved — leave blank to keep it';const title=name.input.value.trim()||agent.title;tabs.querySelector('[data-agent-tab="'+agent.id+'"]').textContent=title;head.querySelector('h3').textContent=title+' connection';Object.assign(agents.find(a=>a.id===agent.id)||agent,{name:title,ready:data.ready,token_set:true,url:url.input.value.trim(),channel:channel.input.value.trim(),agent_id:agentId.input.value.trim()});refreshStatus();}catch(err){note(status,err.message,true);}finally{save.disabled=false;}});root.append(form);
  });activateAgentForm();
}
// Voice catalog, copied from Agora's config.rs so the two pickers offer the same choices.
const VOICE_CATALOG={
  providers:[{id:'groq',label:'Groq'},{id:'openai',label:'OpenAI'}],
  stt:{groq:['whisper-large-v3-turbo','whisper-large-v3','distil-whisper-large-v3-en'],openai:['gpt-4o-mini-transcribe','whisper-1']},
  tts:{groq:['canopylabs/orpheus-v1-english','canopylabs/orpheus-arabic-saudi'],openai:['gpt-4o-mini-tts','tts-1','tts-1-hd']},
  voices:{openai:[['alloy','Alloy — neutral'],['ash','Ash — male'],['ballad','Ballad — male'],['coral','Coral — female'],['echo','Echo — male'],['fable','Fable — male'],['onyx','Onyx — male'],['nova','Nova — female'],['sage','Sage — neutral'],['shimmer','Shimmer — female'],['verse','Verse — male']],
    groq:[['autumn','Autumn — female'],['diana','Diana — female'],['hannah','Hannah — female'],['austin','Austin — male'],['daniel','Daniel — male'],['troy','Troy — male']],
    groqArabic:[['abdullah','Abdullah — male'],['fahad','Fahad — male'],['sultan','Sultan — male'],['lulwa','Lulwa — female'],['noura','Noura — female'],['aisha','Aisha — female']]},
  accents:[['american','American English'],['british','British English'],['arabic','Arabic (Saudi)']]};
function selectField(label,options,value,help){const wrap=el('label','',label);const select=el('select');options.forEach(o=>{const [id,text]=Array.isArray(o)?o:[o,o];const opt=el('option','',text);opt.value=id;select.append(opt);});if(value&&![...select.options].some(o=>o.value===value)){const opt=el('option','',value);opt.value=value;select.append(opt);}select.value=value||select.options[0]?.value||'';wrap.append(select);if(help)wrap.append(el('small','',help));return {wrap,select};}
function advancedModel(wrap){const details=el('details','advanced-setting');details.append(el('summary','','Model options'),wrap);return details;}
function panel(title,copy){const p=el('div','panel');const head=el('div','panel-heading');head.append(el('h3','',title),el('p','',copy));p.append(head);return p;}
async function renderVoice(){
  const root=$('#voice-form');root.replaceChildren(el('p','form-empty','Loading voice settings…'));
  let v;try{v=await api('/api/voice');}catch(e){root.replaceChildren(el('p','form-empty','Couldn’t load voice settings. '+e.message),button('Try again','ghost',renderVoice));return;}
  voiceBuilt=true;root.replaceChildren();const stack=el('div','voice-stack');let keysChanged=()=>{};
  const tabs=el('div','sub voice-tabs');tabs.setAttribute('role','tablist');tabs.setAttribute('aria-label','Voice settings');
  const credentials=el('div','voice-stack'), speech=el('div','voice-stack');
  const panes={credentials,speech};
  const activate=(name,focus=false)=>{voiceTab=name;Object.entries(panes).forEach(([id,pane])=>{pane.hidden=id!==name;const tab=tabs.querySelector('[data-voice-tab="'+id+'"]');tab.classList.toggle('on',id===name);tab.setAttribute('aria-selected',String(id===name));tab.tabIndex=id===name?0:-1;if(focus&&id===name)tab.focus();});};
  [['credentials','Credentials'],['speech','Speech & audio']].forEach(([id,title],index)=>{
    const tab=button(title,'',()=>activate(id));tab.id='voice-tab-'+id;tab.dataset.voiceTab=id;tab.setAttribute('role','tab');tab.setAttribute('aria-controls','voice-panel-'+id);
    tab.addEventListener('keydown',e=>{if(['ArrowLeft','ArrowRight','Home','End'].includes(e.key)){e.preventDefault();activate(e.key==='Home'?'credentials':e.key==='End'?'speech':index?'credentials':'speech',true);}});
    panes[id].id='voice-panel-'+id;panes[id].setAttribute('role','tabpanel');panes[id].setAttribute('aria-labelledby',tab.id);panes[id].tabIndex=0;tabs.append(tab);
  });
  // Credentials — write-only keys, one row per provider, like Agora's API keys card.
  const creds=panel('Connect a speech provider','Add an API key for the provider you want to use. One key can power both listening and spoken replies.');
  VOICE_CATALOG.providers.forEach(p=>{
    const row=el('div','key-row');const head=el('div','key-head');const state=el('span','badge'+(v.keys[p.id]?' on':''),v.keys[p.id]?'Saved':'Not set');head.append(el('strong','',p.label+' API key'),state);
    const line=el('div','key-input');const input=el('input');input.type='password';input.setAttribute('aria-label',p.label+' API key');input.autocomplete='new-password';input.spellcheck=false;input.maxLength=500;input.placeholder=v.keys[p.id]?'Paste a new key to replace it':(p.id==='groq'?'gsk_…':'sk-…');
    const save=button('Save key','primary sm');const test=button('Test key','ghost sm');const clear=button('Remove','ghost sm');const status=el('p','note');status.setAttribute('role','status');
    const actions=el('div','key-actions');actions.append(test,clear);const setSaved=on=>{state.textContent=on?'Saved':'Not set';state.classList.toggle('on',on);v.keys[p.id]=on;actions.hidden=!on;input.placeholder=on?'Paste a new key to replace it':(p.id==='groq'?'gsk_…':'sk-…');keysChanged();};actions.hidden=!v.keys[p.id];
    save.addEventListener('click',async()=>{const key=input.value.trim();if(!key){note(status,'Paste a key first.',true);return;}save.disabled=true;note(status,'Saving…');try{await api('/api/voice/keys',{provider:p.id,api_key:key});input.value='';setSaved(true);note(status,p.label+' key saved.');status.classList.add('good');refreshStatus();}catch(err){note(status,err.message,true);}finally{save.disabled=false;}});
    test.addEventListener('click',async()=>{test.disabled=true;test.textContent='Testing…';note(status,'Checking the key with '+p.label+'…');try{await api('/api/voice/test',{provider:p.id},20000);note(status,p.label+' credentials work.');status.classList.add('good');}catch(err){note(status,err.message,true);}finally{test.disabled=false;test.textContent='Test key';}});
    clear.addEventListener('click',async()=>{if(!confirm('Forget the saved '+p.label+' key?'))return;clear.disabled=true;try{await api('/api/voice/keys',{provider:p.id,clear:true});setSaved(false);note(status,p.label+' key cleared.');refreshStatus();}catch(err){note(status,err.message,true);}finally{clear.disabled=false;}});
    line.append(input,save);row.append(head,line,actions,status);creds.append(row);
  });
  // Features — providers and models, saved as soon as a picker changes.
  const featureNote=el('p','note');featureNote.setAttribute('role','status');
  const stt=panel('Listening','Turn your voice into a message for your agent. Choose your speech-to-text service.');
  const sttProvider=selectField('Provider',VOICE_CATALOG.providers.map(p=>[p.id,p.label]),v.stt_provider);
  const sttModel=selectField('Model',VOICE_CATALOG.stt[v.stt_provider],v.stt_models[v.stt_provider]);
  const sttHint=el('p','note');stt.append(sttProvider.wrap,advancedModel(sttModel.wrap),sttHint);
  const tts=panel('Spoken replies','Choose how your agent sounds. Turn on the speaker in a conversation to hear replies aloud.');
  const ttsProvider=selectField('Provider',VOICE_CATALOG.providers.map(p=>[p.id,p.label]),v.tts_provider);
  const accent=selectField('Accent',VOICE_CATALOG.accents,v.accent);
  const voice=selectField('Voice',[],'');const ttsModel=selectField('Model',[],'');const ttsHint=el('p','note');
  const pair=el('div','field-pair');pair.append(accent.wrap,voice.wrap);tts.append(ttsProvider.wrap,pair,advancedModel(ttsModel.wrap),ttsHint);
  const talk=panel('Desk talk button','Hold to speak, release to send. Replies play through your desk speaker.');
  const meta=el('div','voice-meta');[['Microphone',v.mic?'Ready':'Unavailable','For your spoken messages'],['Speaker',v.speaker?'Ready':'Unavailable','For replies aloud']].forEach(([k,val,sub])=>{const d=el('div');d.append(el('strong','',val),document.createTextNode(k+' · '+sub));meta.append(d);});
  const agentSel=selectField('Send to',(agents.length?agents:agentIds.map(id=>({id,name:id}))).map(a=>[a.id,(a.name||a.title||a.id)+(a.ready===false?' · setup needed':'')]),v.agent,'The chat page’s mic uses whichever agent is open there.');
  talk.append(meta,agentSel.wrap);
  const fill=(sel,options,value)=>{sel.replaceChildren();options.forEach(o=>{const [id,text]=Array.isArray(o)?o:[o,o];const opt=el('option','',text);opt.value=id;sel.append(opt);});if(value&&![...sel.options].some(o=>o.value===value)){const opt=el('option','',value);opt.value=value;sel.append(opt);}sel.value=value||sel.options[0]?.value||'';};
  const ttsVoiceList=()=>{const p=ttsProvider.select.value;if(p==='openai')return VOICE_CATALOG.voices.openai;const arabic=accent.select.value==='arabic'||/arabic/.test(v.tts_models.groq||'');return arabic?VOICE_CATALOG.voices.groqArabic:VOICE_CATALOG.voices.groq;};
  const syncTts=()=>{const p=ttsProvider.select.value;fill(ttsModel.select,VOICE_CATALOG.tts[p],v.tts_models[p]);const voices=ttsVoiceList();
    // A voice that cannot speak the chosen model falls back to Agora's default for that combination, and that is what gets saved.
    if(!voices.some(([id])=>id===v.tts_voices[p]))v.tts_voices[p]=p==='openai'?(accent.select.value==='british'?'fable':'alloy'):(voices===VOICE_CATALOG.voices.groqArabic?'noura':'autumn');fill(voice.select,voices,v.tts_voices[p]);
    ttsHint.textContent=p==='groq'?(accent.select.value==='arabic'?'Arabic voices are available for this accent.':'Groq uses the same English voices for American and British accents.'):(/^tts-1/.test(ttsModel.select.value)?'This model uses the voice’s own accent. Choose gpt-4o-mini-tts to apply your accent preference.':'Replies use your selected voice and accent.');
    ttsHint.classList.toggle('bad',!v.keys[p]);if(!v.keys[p])ttsHint.textContent='Add the '+(p==='groq'?'Groq':'OpenAI')+' key under Credentials before replies can be spoken.';};
  const syncStt=()=>{const p=sttProvider.select.value;fill(sttModel.select,VOICE_CATALOG.stt[p],v.stt_models[p]);sttHint.textContent=v.keys[p]?'':'Add the '+(p==='groq'?'Groq':'OpenAI')+' key under Credentials before the mic can be used.';sttHint.classList.toggle('bad',!v.keys[p]);};
  let saveTimer=0;const saveFeatures=()=>{clearTimeout(saveTimer);saveTimer=setTimeout(async()=>{note(featureNote,'Saving…');try{const data=await api('/api/voice',{stt_provider:sttProvider.select.value,tts_provider:ttsProvider.select.value,stt_model_groq:v.stt_models.groq,stt_model_openai:v.stt_models.openai,tts_model_groq:v.tts_models.groq,tts_model_openai:v.tts_models.openai,voice_groq:v.tts_voices.groq,voice_openai:v.tts_voices.openai,accent:accent.select.value,agent:agentSel.select.value});note(featureNote,'Voice settings saved.'+(data.stt_ready&&data.tts_ready?'':' Add the missing key under Credentials to finish.'));featureNote.classList.add('good');refreshStatus();}catch(err){note(featureNote,err.message,true);}},250);};
  sttProvider.select.addEventListener('change',()=>{v.stt_provider=sttProvider.select.value;syncStt();saveFeatures();});
  sttModel.select.addEventListener('change',()=>{v.stt_models[sttProvider.select.value]=sttModel.select.value;saveFeatures();});
  ttsProvider.select.addEventListener('change',()=>{v.tts_provider=ttsProvider.select.value;syncTts();saveFeatures();});
  accent.select.addEventListener('change',()=>{v.accent=accent.select.value;syncTts();saveFeatures();});
  voice.select.addEventListener('change',()=>{v.tts_voices[ttsProvider.select.value]=voice.select.value;saveFeatures();});
  ttsModel.select.addEventListener('change',()=>{v.tts_models[ttsProvider.select.value]=ttsModel.select.value;syncTts();saveFeatures();});
  agentSel.select.addEventListener('change',saveFeatures);
  // Per-browser choice, so a phone can use its own mic while the desk's hardware stays for the button.
  const page=panel('Microphone & speaker','Choose where to speak and hear replies when using Conversations. This choice is saved for this browser.');
  const modeSel=selectField('Microphone and speaker',[['browser','This browser'],['desk','Desk device']],audioMode);const modeHint=el('p','note');
  const syncMode=()=>{const m=modeSel.select.value;modeHint.classList.toggle('bad',m==='browser'&&!micSupported);
    modeHint.textContent=m==='browser'?(micSupported?'Use this phone or computer’s microphone and speaker.':micBlockedWhy+' Spoken replies still play here.'):(v.mic&&v.speaker?'Use the microphone and speaker on your desk device.':'Your desk microphone or speaker is unavailable. Check that both are connected.');};
  modeSel.select.addEventListener('change',()=>{setAudioMode(modeSel.select.value);syncMode();});
  page.append(modeSel.wrap,modeHint);syncMode();
  keysChanged=()=>{syncStt();syncTts();};
  syncStt();syncTts();
  const privacy=el('p','note','Keys are saved on your desk device and hidden after saving. Audio and text are sent to your selected speech provider.');
  credentials.append(creds,privacy,button('Continue to speech & audio →','ghost',()=>activate('speech',true)));
  const saveBar=el('div','voice-save');note(featureNote,'Changes save automatically.');saveBar.append(featureNote,button('Manage credentials','text-button',()=>activate('credentials',true)));
  speech.append(saveBar,stt,tts,page,talk);stack.append(tabs,credentials,speech);root.append(stack);activate(voiceTab);
}
let savedSsid='', wifiJoining=false, scanGeneration=0, selectedNetwork=null;
const wifiDialog=$('#wifi-dialog');
function signalLabel(rssi){if(rssi>=-60)return 'Strong';if(rssi>=-75)return 'Good';return 'Weak';}
function wifiList(focus=true){
  $('#wifi-form').hidden=true;$('#wifi-list-step').hidden=false;
  $('#wifi-title').textContent='Choose a network';$('#wifi-description').textContent='Nearby networks, discovered by your board.';
  $('#wifi-pass').value='';if(focus)$('#wifi-title').focus();
}
function wifiDetails(net){
  selectedNetwork=net;$('#wifi-list-step').hidden=true;$('#wifi-form').hidden=false;
  $('#wifi-title').textContent=net?'Join network':'Add a network';
  $('#wifi-description').textContent=net?'One more step to connect your desk.':'Enter the details for a hidden or unlisted network.';
  $('#ssid-field').hidden=!!net;$('#ssid').value=net?net.ssid:'';
  $('#wifi-selected').hidden=!net;$('#wifi-selected').textContent=net?net.ssid:'';
  $('#password-field').hidden=!!net?.open;$('#wifi-pass').value='';$('#wifi-pass').type='password';
  $('#wifi-show').textContent='Show';$('#wifi-show').setAttribute('aria-pressed','false');$('#wifi-show').setAttribute('aria-label','Show password');
  $('#wifi-pass').required=!!net&&!net.open&&net.ssid!==savedSsid;
  $('#password-help').textContent=net&&net.ssid===savedSsid?'Leave blank to use the saved password.':net?'Enter the password for this network.':'Leave blank for an open network, or to use its saved password.';
  note($('#wifi-join-note'),net?.open?'This is an open network. No password is needed.':'Your connection details are saved on this board.');
  (net?(net.open?$('#wifi-connect'):$('#wifi-pass')):$('#ssid')).focus();
}
async function scanNetworks(){
  const generation=++scanGeneration, scan=$('#scan'), status=$('#scan-note'), list=$('#networks');
  scan.disabled=true;list.setAttribute('aria-busy','true');list.replaceChildren();note(status,'Looking for nearby networks…');status.classList.add('scan-loading');
  try{
    const data=await api('/api/wifi/scan',null,20000);if(generation!==scanGeneration)return;
    if(data.error)throw new Error(data.error);
    const nets=(data.networks||[]).sort((a,b)=>b.rssi-a.rssi);
    note(status,nets.length?nets.length+' networks found · strongest first':'No networks found. Scan again or add one manually.');
    nets.forEach(net=>{
      const item=button('','',()=>wifiDetails(net)), bars=el('span','net-signal');bars.setAttribute('aria-hidden','true');
      const strength=net.rssi>=-60?4:net.rssi>=-75?3:1;for(let i=0;i<4;i++)bars.append(el('i',i<strength?'lit':''));
      const copy=el('span','net-copy');copy.append(el('strong','',net.ssid),el('small','',(online&&net.ssid===savedSsid?'Connected · ':'')+signalLabel(net.rssi)+' signal · '+(net.open?'Open':'Password required')));
      item.append(bars,copy,icon(net.open?'arrow':'lock'));list.append(item);
    });
  }catch(err){if(generation===scanGeneration)note(status,err.message+' Try again or add a network manually.',true);}
  finally{if(generation===scanGeneration){scan.disabled=false;list.setAttribute('aria-busy','false');status.classList.remove('scan-loading');}}
}
$('#choose-network').addEventListener('click',()=>{wifiList(false);wifiDialog.showModal();$('#wifi-title').focus();scanNetworks();});
$('#scan').addEventListener('click',scanNetworks);
$('#wifi-manual').addEventListener('click',()=>wifiDetails(null));
$('#wifi-back').addEventListener('click',()=>wifiList());
function closeWifi(){if(!wifiJoining)wifiDialog.close();}
$('#wifi-close').addEventListener('click',closeWifi);$('#wifi-cancel').addEventListener('click',closeWifi);
wifiDialog.addEventListener('cancel',e=>{if(wifiJoining)e.preventDefault();});
wifiDialog.addEventListener('close',()=>{scanGeneration++;$('#wifi-pass').value='';$('#choose-network').focus();});
$('#wifi-show').addEventListener('click',()=>{const show=$('#wifi-pass').type==='password';$('#wifi-pass').type=show?'text':'password';$('#wifi-show').textContent=show?'Hide':'Show';$('#wifi-show').setAttribute('aria-pressed',String(show));$('#wifi-show').setAttribute('aria-label',show?'Hide password':'Show password');});
$('#wifi-form').addEventListener('submit',async e=>{
  e.preventDefault();if(wifiJoining)return;
  const ssid=$('#ssid').value, password=selectedNetwork?.open?'':$('#wifi-pass').value;
  if(!ssid.trim()||new TextEncoder().encode(ssid).length>32){note($('#wifi-join-note'),'Enter a network name of 1–32 bytes.',true);return;}
  wifiJoining=true;$('#wifi-form').querySelectorAll('button,input').forEach(n=>n.disabled=true);$('#wifi-close').disabled=true;$('#wifi-connect').textContent='Connecting…';
  note($('#wifi-join-note'),'Connecting to '+ssid+'… This may take a few seconds.');
  try{
    const data=await api('/api/wifi',{ssid,password,security:selectedNetwork?.open?'open':'password'},25000);$('#wifi-pass').value='';
    if(data.connected){note($('#wifi-note'),'Connected to '+ssid+' · '+data.ip);$('#wifi-note').classList.add('good');wifiDialog.close();}
    else note($('#wifi-join-note'),'Network saved, but not connected yet. Check the password and signal, then try again.',true);
    refreshStatus();
  }catch(err){note($('#wifi-join-note'),err.message+' If the board changed networks, reopen it at esp32-agent.local or its new IP address.',true);}
  finally{wifiJoining=false;$('#wifi-form').querySelectorAll('button,input').forEach(n=>n.disabled=false);$('#wifi-close').disabled=false;$('#wifi-connect').textContent='Connect';}
});
async function waitFor(job,id){
  waitingJob=job;boardBusy=true;renderChat();let failures=0;const started=Date.now();banner('Listening for your agent’s reply…');note($('#chat-note'),'Listening… You can leave this tab open.');
  try{while(true){await new Promise(r=>setTimeout(r,1200));let s;try{s=await api('/api/listen');failures=0;}catch(e){if(++failures>=5)throw e;banner('Connection interrupted. Trying to pick up your reply…');continue;}
    if(s.job!==job){remember(id,{role:'system',text:'The board restarted or a newer conversation replaced this request.'});break;}
    if(s.state==='done'){remember(id,{role:'agent',name:s.from,text:s.reply||'(The agent returned an empty reply.)'});if(speakOn&&audioMode==='browser'&&s.reply)sayInBrowser(s.reply);break;}
    if(s.state==='failed'||s.state==='idle'){remember(id,{role:'system',text:s.error||'The board stopped listening. You can try another message.'});break;}
    if(Date.now()-started>240000)throw new Error('The reply is taking longer than expected. Check your agent bridge before sending again.');
    note($('#chat-note'),'Listening'+(s.from?' for '+s.from:'')+' · '+Math.floor((s.waited_ms||Date.now()-started)/1000)+'s');
  }}catch(e){remember(id,{role:'system',text:e.message+' Check Agora for a reply before resending.'});}
  finally{waitingJob=0;boardBusy=false;storage.remove('sessionStorage','desk-pending');banner('');renderChat();refreshStatus();}
}
// Typed and browser-spoken messages share this path; only the board speaker asks the board to read the reply.
async function sendMessage(id,text,spoken=false){
  const a=agents.find(x=>x.id===id);if(!a?.ready||!text||waitingJob||sending)return;
  sending=true;remember(id,{role:'you',text,spoken});renderChat();note($('#chat-note'),'Sending…');
  try{const data=await api('/api/chat',{agent:id,text,speak:speakOn&&audioMode==='desk'});sending=false;if(data.speak_note)remember(id,{role:'system',text:data.speak_note});storage.set('sessionStorage','desk-pending',JSON.stringify({job:data.job,agent:id}));await waitFor(data.job,id);}
  catch(err){sending=false;waitingJob=0;remember(id,{role:'system',text:err.message+' Your message is back in the composer. Check Agora before retrying.'});if(!$('#box').value){$('#box').value=text;drafts[id]=text;}renderChat();resizeComposer();note($('#chat-note'),err.message,true);refreshStatus();}
}
$('#composer').addEventListener('submit',async e=>{
  e.preventDefault();if($('#send').disabled)return;const text=$('#box').value.trim();if(!text)return;
  $('#box').value='';drafts[current]='';resizeComposer();await sendMessage(current,text);
});
async function start(){
  let pending;try{pending=JSON.parse(storage.get('sessionStorage','desk-pending')||'null');}catch(_){}
  if(pending?.job){waitingJob=pending.job;current=pending.agent||current;draftAgent=current;}
  showTab(pending?.job?'chat':location.hash.slice(1)||'home');
  await refreshStatus();
  try{await loadAgents();}catch(e){banner(e.message,true);}
  if(pending?.job)waitFor(pending.job,current);
}
start();setInterval(()=>{if(!document.hidden){refreshStatus();if(!agents.length)loadAgents().catch(()=>{});}},5000);
window.addEventListener('online',()=>refreshStatus());
</script>
</body>
</html>
)ESP32PAGE";
