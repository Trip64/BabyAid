#ifndef WEBAPP_H
#define WEBAPP_H

#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>Kidsentinel - Infant Vital Monitor</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;background:#0d1117;color:#e6edf3;min-height:100vh;padding:16px;-webkit-font-smoothing:antialiased}
.container{max-width:540px;margin:0 auto}
.hdr{text-align:center;padding:12px 0 16px;border-bottom:1px solid #21262d;margin-bottom:16px}
.hdr h1{font-size:16px;font-weight:600;letter-spacing:2px;text-transform:uppercase;color:#8b949e}
.hdr .st{font-size:12px;color:#8b949e;margin-top:6px;display:flex;align-items:center;justify-content:center;gap:6px}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%}
.dot.on{background:#3fb950;box-shadow:0 0 8px #3fb950}
.dot.off{background:#f85149;box-shadow:0 0 8px #f85149}

.g{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:12px}
.c{background:#161b22;border:1px solid #30363d;border-radius:12px;padding:16px;box-shadow:0 4px 12px rgba(0,0,0,0.15)}
.c .lbl{font-size:11px;text-transform:uppercase;letter-spacing:1.5px;color:#8b949e;margin-bottom:8px;font-weight:600}
.c .val{font-size:38px;font-weight:300;line-height:1}
.c .unit{font-size:14px;font-weight:400;color:#8b949e;margin-left:3px}
.c.bt .val{color:#ff7b72}
.c.hr .val{color:#ffa657}
.c.rt .val{color:#f2cc60}
.c.rh .val{color:#56d364}

.c.hr .beat{display:inline-block;width:10px;height:10px;border-radius:50%;background:#ffa657;margin-left:6px;vertical-align:middle;opacity:.3;transition:opacity .15s,transform .15s}
.c.hr .beat.on{opacity:1;transform:scale(1.25);box-shadow:0 0 10px #ffa657}

.alert-bar{background:#161b22;border:1px solid #30363d;border-radius:10px;padding:14px 16px;margin-bottom:14px;font-size:13px;color:#8b949e;text-align:center;font-weight:500;transition:all .3s}
.alert-bar.active{border-color:#da3633;background:#250f12;color:#ff7b72;box-shadow:0 0 14px rgba(218,54,51,0.25)}

.cfg{background:#161b22;border:1px solid #30363d;border-radius:12px;padding:18px;margin-bottom:12px}
.cfg h2{font-size:12px;text-transform:uppercase;letter-spacing:2px;color:#8b949e;margin-bottom:16px;font-weight:600}
.row{display:flex;align-items:center;justify-content:space-between;margin-bottom:14px}
.row:last-child{margin-bottom:0}
.row label{font-size:14px;color:#c9d1d9}
.rng-row{margin-bottom:16px}
.rng-row .top{display:flex;justify-content:space-between;align-items:center;margin-bottom:6px}
.rng-row .top label{font-size:14px;color:#c9d1d9}
.rng-row .top span{font-size:14px;color:#58a6ff;font-weight:600}
input[type=range]{-webkit-appearance:none;width:100%;height:4px;background:#30363d;border-radius:2px;outline:none;margin:8px 0}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:20px;height:20px;background:#58a6ff;border-radius:50%;cursor:pointer;box-shadow:0 0 6px rgba(88,166,255,0.4)}

.tog{position:relative;width:44px;height:24px;flex-shrink:0}
.tog input{opacity:0;width:0;height:0}
.tog span{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#30363d;border-radius:12px;transition:.2s}
.tog span:before{content:"";position:absolute;height:18px;width:18px;left:3px;bottom:3px;background:#8b949e;border-radius:50%;transition:.2s}
.tog input:checked+span{background:#1f6feb}
.tog input:checked+span:before{transform:translateX(20px);background:#ffffff}

.btn{width:100%;padding:12px;border:1px solid #388bfd;border-radius:8px;background:#1f6feb;color:#ffffff;font-size:13px;font-weight:600;letter-spacing:1px;text-transform:uppercase;cursor:pointer;transition:background .2s;margin-top:8px}
.btn:active{background:#1158c7}
</style>
</head>
<body>
<div class="container">
  <div class="hdr">
    <h1>Kidsentinel Hub</h1>
    <div class="st"><span class="dot off" id="dot"></span><span id="conn">Disconnected</span></div>
  </div>

  <div class="g">
    <div class="c bt"><div class="lbl">Infant Temp</div><div class="val"><span id="bt">--</span><span class="unit">°C</span></div></div>
    <div class="c hr"><div class="lbl">Heart Rate</div><div class="val"><span id="hr">--</span><span class="unit">bpm</span><span class="beat" id="beat"></span></div></div>
    <div class="c rt"><div class="lbl">Room Temp</div><div class="val"><span id="rt">--</span><span class="unit">°C</span></div></div>
    <div class="c rh"><div class="lbl">Room Humidity</div><div class="val"><span id="rh">--</span><span class="unit">%</span></div></div>
  </div>

  <div class="alert-bar" id="abar">All parameters normal</div>

  <div class="cfg">
    <h2>System Calibration</h2>
    <div class="rng-row">
      <div class="top"><label>Temperature Offset</label><span id="tv">+4.0°C</span></div>
      <input type="range" id="ti" min="-10" max="10" step="0.1" value="4.0">
    </div>
    <div class="row">
      <label>Mute Audible Buzzer</label>
      <label class="tog"><input type="checkbox" id="mi"><span></span></label>
    </div>
    <button class="btn" onclick="save()">Apply Configuration</button>
  </div>
</div>

<script>
var ti=document.getElementById('ti'),tv=document.getElementById('tv'),mi=document.getElementById('mi');
ti.oninput=function(){var v=parseFloat(this.value);tv.textContent=(v>=0?'+':'')+v.toFixed(1)+'°C'};
var beatOn=false;
function poll(){
  var x=new XMLHttpRequest();
  x.timeout=2000;
  x.onload=function(){
    if(x.status===200){
      var d=JSON.parse(x.responseText);
      document.getElementById('bt').textContent=d.bt>0?d.bt.toFixed(1):'--';
      document.getElementById('hr').textContent=d.hr>0?d.hr:'--';
      document.getElementById('rt').textContent=d.rt>0?d.rt.toFixed(1):'--';
      document.getElementById('rh').textContent=d.rh>0?d.rh.toFixed(0):'--';
      var dot=document.getElementById('dot'),cn=document.getElementById('conn');
      dot.className='dot on';cn.textContent='Telemetry Active';
      var ab=document.getElementById('abar');
      if(d.alert){ab.className='alert-bar active';ab.textContent=d.msg}
      else{ab.className='alert-bar';ab.textContent='All parameters normal'}
      if(d.hr>0){beatOn=!beatOn;document.getElementById('beat').className='beat'+(beatOn?' on':'')}
      if(document.activeElement!==ti){ti.value=d.trim;var v=d.trim;tv.textContent=(v>=0?'+':'')+v.toFixed(1)+'°C'}
      mi.checked=d.mute;
    }
  };
  x.onerror=function(){
    document.getElementById('dot').className='dot off';
    document.getElementById('conn').textContent='Disconnected';
  };
  x.open('GET','/api/state');
  x.send();
}
function save(){
  var x=new XMLHttpRequest();
  x.open('POST','/api/settings');
  x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
  x.onload=function(){
    var b=document.querySelector('.btn');
    b.textContent='Settings Applied!';
    setTimeout(function(){b.textContent='Apply Configuration'},1500);
  };
  x.send('trim='+ti.value+'&mute='+(mi.checked?1:0));
}
setInterval(poll,2000);
poll();
</script>
</body>
</html>
)rawliteral";

#endif // WEBAPP_H
