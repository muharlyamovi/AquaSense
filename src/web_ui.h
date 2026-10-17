#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="ru"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0b6bcb"><meta name="apple-mobile-web-app-capable" content="yes">
<title>AquaSense</title>
<style>
:root{--bg:#eef4fa;--card:#fff;--txt:#10243a;--mut:#6b7c90;--pri:#0b6bcb;--ok:#1aa260;--warn:#f0a020;--bad:#e0413a;--line:#c9d6e3}
@media(prefers-color-scheme:dark){:root{--bg:#0d1620;--card:#16222f;--txt:#e6eef6;--mut:#8ea0b4;--line:#2a3a4c}}
*{box-sizing:border-box}
body{margin:0;font:15px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;background:var(--bg);color:var(--txt);padding-bottom:80px}
header{background:linear-gradient(135deg,#0b6bcb,#22a6e0);color:#fff;padding:18px 16px 48px}
header h1{margin:0;font-size:21px}header small{opacity:.85}
main{max-width:560px;margin:-36px auto 0;padding:0 12px}
.card{background:var(--card);border-radius:16px;padding:16px;margin-bottom:12px;box-shadow:0 2px 10px rgba(0,0,0,.06)}
.card h2{margin:0 0 10px;font-size:16px}
.row{display:flex;gap:14px;align-items:center}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.kv{background:var(--bg);border-radius:12px;padding:10px}
.kv b{display:block;font-size:20px}.kv span{color:var(--mut);font-size:12px}
.badge{display:inline-block;padding:4px 12px;border-radius:99px;color:#fff;font-weight:600;font-size:13px}
.ring{width:118px;height:118px;flex:none}
.btn{display:block;width:100%;padding:13px;border:0;border-radius:12px;background:var(--pri);color:#fff;font-size:15px;font-weight:600;margin-top:10px;cursor:pointer;text-align:center;text-decoration:none}
.btn.sec{background:var(--bg);color:var(--pri)}.btn.red{background:var(--bad)}.btn.green{background:var(--ok)}
label{display:block;font-size:13px;color:var(--mut);margin:10px 0 4px}
input,select{width:100%;padding:11px;border-radius:10px;border:1px solid var(--line);background:var(--card);color:var(--txt);font-size:15px}
.alert{border-radius:12px;padding:12px 14px;margin-bottom:12px;color:#fff;font-weight:600;display:none}
nav{position:fixed;bottom:0;left:0;right:0;display:flex;background:var(--card);box-shadow:0 -2px 10px rgba(0,0,0,.08);padding-bottom:env(safe-area-inset-bottom)}
nav button{flex:1;border:0;background:none;padding:9px 0 11px;color:var(--mut);font-size:12px}
nav button.on{color:var(--pri);font-weight:700}nav i{display:block;font-style:normal;font-size:19px}
.tab{display:none}.tab.on{display:block}
.mut{color:var(--mut);font-size:13px}
.bar{height:10px;background:var(--bg);border-radius:6px;overflow:hidden}.bar>div{height:100%;width:0;background:var(--pri);transition:width .4s}
.net{padding:10px 12px;border-radius:10px;background:var(--bg);margin:6px 0;cursor:pointer;display:flex;justify-content:space-between}
canvas{width:100%;height:190px;display:block}
.toast{position:fixed;left:50%;bottom:92px;transform:translateX(-50%);background:#222;color:#fff;padding:10px 16px;border-radius:10px;opacity:0;transition:opacity .3s;pointer-events:none;z-index:9}
.toggle{display:flex;align-items:center;justify-content:space-between;gap:12px;margin:8px 0 4px}
.toggle b{font-size:15px}
.sw{position:relative;width:48px;height:28px;flex:none}
.sw input{opacity:0;width:0;height:0;position:absolute}
.sw i{position:absolute;inset:0;background:#c9d6e3;border-radius:99px;transition:.2s}
.sw i:before{content:'';position:absolute;width:22px;height:22px;left:3px;top:3px;background:#fff;border-radius:50%;transition:.2s;box-shadow:0 1px 3px rgba(0,0,0,.2)}
.sw input:checked+i{background:var(--ok)}
.sw input:checked+i:before{transform:translateX(20px)}
@media(prefers-color-scheme:dark){.sw i{background:#2a3a4c}}
</style></head><body>
<header><h1 id="hName">AquaSense</h1><small id="hSub">подключение…</small></header>
<main>
<div id="alert" class="alert"></div>

<section id="t-home" class="tab on">
 <div class="card"><div class="row">
  <svg class="ring" viewBox="0 0 120 120"><circle cx="60" cy="60" r="52" stroke="#dde7f1" stroke-width="12" fill="none"/>
   <circle id="ringArc" cx="60" cy="60" r="52" stroke="#1aa260" stroke-width="12" fill="none" stroke-linecap="round" stroke-dasharray="326.7" stroke-dashoffset="326.7" transform="rotate(-90 60 60)" style="transition:stroke-dashoffset .6s"/>
   <text id="ringTxt" x="60" y="62" text-anchor="middle" font-size="26" font-weight="700" fill="currentColor">–</text>
   <text x="60" y="82" text-anchor="middle" font-size="11" fill="#8ea0b4">ресурс</text></svg>
  <div><div class="mut">Картридж</div><div id="fState" style="font-size:19px;font-weight:700">–</div><div id="fLeft" class="mut"></div></div>
 </div></div>

 <div class="card"><h2>Качество воды</h2>
  <div class="row"><span id="wBadge" class="badge" style="background:#8a97a6">нет данных</span><span id="wNote" class="mut"></span></div>
  <div class="grid" style="margin-top:12px">
   <div class="kv"><b id="vTds">–</b><span>TDS сейчас, ppm</span></div>
   <div class="kv"><b id="vTemp">–</b><span>Температура, °C</span></div>
   <div class="kv"><b id="vBase">–</b><span>TDS нового картриджа</span></div>
   <div class="kv"><b id="vAge">–</b><span>Последний замер</span></div>
  </div>
  <button class="btn sec" onclick="measure()">Измерить сейчас</button>
 </div>

 <div class="card"><h2>Устройство</h2><div class="grid">
  <div class="kv"><b id="vBatt">–</b><span id="vBattS">Батарея</span></div>
  <div class="kv"><b id="vLeak">–</b><span>Протечка</span></div>
  <div class="kv"><b id="vTotal">–</b><span>Всего прошло, л</span></div>
  <div class="kv"><b id="vToday">–</b><span>За текущие сутки, л</span></div>
 </div></div>

 <div class="card"><h2>Сменные картриджи</h2>
  <div class="mut" id="shopNote"></div>
  <a class="btn" id="bBuy" target="_blank" rel="noopener">Заказать картридж</a>
  <a class="btn green" id="bSub" target="_blank" rel="noopener">Оформить подписку</a>
 </div>
</section>

<section id="t-hist" class="tab">
 <div class="card"><h2>Расход воды, л/сутки</h2><canvas id="cL"></canvas></div>
 <div class="card"><h2>Средний TDS, ppm</h2><canvas id="cT"></canvas>
  <div class="mut" id="tdsLeg" style="margin-top:8px">линия — замеры · зелёная — отличная · жёлтая — допустимая · красная — плохая</div></div>
</section>

<section id="t-cart" class="tab">
 <div class="card"><h2>Текущий картридж</h2>
  <div class="mut" id="cInfo"></div>
  <label>Износ по объёму</label><div class="bar"><div id="bVol"></div></div>
  <label>Износ по сроку службы</label><div class="bar"><div id="bTime"></div></div>
  <label>Износ по росту TDS</label><div class="bar"><div id="bTds"></div></div>
  <button class="btn green" onclick="replaced()">Я установил новый картридж</button>
 </div>
 <div class="card"><h2>Параметры картриджа</h2>
  <label>Модель</label><input id="s_model">
  <label>Тип фильтра</label><select id="s_ftype" onchange="preset()"><option value="0">Обратный осмос</option><option value="1">Угольный / проточный</option></select>
  <label>Ресурс, литров</label><input id="s_cl" type="number">
  <label>Срок службы, месяцев</label><input id="s_cm" type="number">
  <button class="btn" onclick="saveSet()">Сохранить</button>
 </div>
 <div class="card"><h2>Калибровка TDS</h2>
  <div class="mut">Опустите датчик в калибровочный раствор (например 342 ppm), введите его значение и нажмите кнопку.</div>
  <label>TDS раствора, ppm</label><input id="calRef" type="number" value="342">
  <button class="btn sec" onclick="calibrate()">Откалибровать</button>
  <div class="mut" id="calRes" style="margin-top:8px"></div>
 </div>
</section>

<section id="t-set" class="tab">
 <div class="card"><h2>Wi-Fi</h2>
  <div class="mut" id="wifiState"></div>
  <button class="btn sec" onclick="scan()">Найти сети</button>
  <div id="nets"></div>
  <label>Имя сети (SSID)</label><input id="w_ssid" autocomplete="off">
  <label>Пароль</label><input id="w_pass" type="password">
  <button class="btn" onclick="saveWifi()">Подключить</button>
 </div>
 <div class="card"><h2>Калибровка счётчика воды</h2>
  <div class="mut">Налейте известный объём (5–10 л). Счётчик импульсов должен расти сразу, как откроете кран. Потом плата сама посчитает долю литра на один импульс. Если раньше литры «летели» — это как раз частые импульсы при коэффициенте 1 л/имп.</div>
  <label>Сколько литров нальёте</label><input id="flowLiters" type="number" step="0.1" value="10">
  <button class="btn" id="bFlowStart" onclick="flowStart()">Начать калибровку</button>
  <div id="flowRun" style="display:none">
   <div class="grid" style="margin-top:10px">
    <div class="kv"><b id="flowPulses">0</b><span>Импульсов</span></div>
    <div class="kv"><b id="flowPin">–</b><span>Вход GPIO</span></div>
    <div class="kv"><b id="flowEdges">0</b><span>Сырых фронтов</span></div>
    <div class="kv"><b id="flowHz">–</b><span>Имп/с сейчас</span></div>
   </div>
   <button class="btn green" onclick="flowStop()">Налив закончен — посчитать</button>
   <button class="btn sec" onclick="flowCancel()">Отмена</button>
  </div>
  <div class="mut" id="flowCalRes" style="margin-top:8px"></div>
  <label>Литров на один импульс</label><input id="s_lpp" type="number" step="0.001">
  <button class="btn sec" onclick="saveSet()">Сохранить вручную</button>
 </div>
 <div class="card"><h2>Датчики и пороги</h2>
  <label>TDS «отличная» до, ppm</label><input id="s_t1" type="number">
  <label>TDS «хорошая» до, ppm</label><input id="s_t2" type="number">
  <label>TDS «допустимая» до, ppm</label><input id="s_t3" type="number">
  <label>Допустимый рост TDS от нового картриджа, раз (0 — не учитывать)</label><input id="s_rise" type="number" step="0.1">
  <button class="btn" onclick="saveSet()">Сохранить</button>
 </div>
 <div class="card"><h2>Облако GitHub</h2>
  <div class="mut" id="cloudInfo">Контроллер пишет статус в приватный репозиторий. Смотреть параметры можно со страницы dashboard — с любого телефона, без локальной сети устройства.</div>
  <label>Репозиторий</label><input id="s_ghrepo" placeholder="владелец/AquaSense">
  <label>GitHub token (fine-grained, Contents: Read and write)</label><input id="s_ght" autocomplete="off">
  <button class="btn" onclick="saveSet()">Сохранить</button>
  <button class="btn sec" onclick="cloudNow()">Отправить статус сейчас</button>
  <a class="btn green" href="https://github.com/muharlyamovi/AquaSense/blob/main/docs/dashboard.html" target="_blank" rel="noopener">Открыть инструкцию к странице</a>
 </div>
 <div class="card"><h2>Уведомления и магазин</h2>
  <label>Имя устройства</label><input id="s_name">
  <label>Telegram bot token</label><input id="s_tgt" autocomplete="off">
  <label>Telegram chat id</label><input id="s_tgc">
  <label>Ссылка магазина</label><input id="s_shop">
  <button class="btn" onclick="saveSet()">Сохранить</button>
  <button class="btn sec" onclick="testTg()">Тестовое сообщение</button>
 </div>
 <div class="card"><h2>Обновление прошивки</h2>
  <div class="mut" id="otaInfo">Текущая версия — в системе ниже.</div>
  <label>Ссылка на manifest.json в облаке</label><input id="s_ota" placeholder="https://…/manifest.json" autocomplete="off">
  <div class="toggle"><b>Автообновление</b><label class="sw"><input id="s_autoOta" type="checkbox" onchange="otaUi();saveSet()"><i></i></label></div>
  <div id="otaFreq">
   <label>Как часто проверять облако</label>
   <select id="s_otaHours" onchange="saveSet()">
    <option value="6">Каждые 6 часов</option>
    <option value="12">Каждые 12 часов</option>
    <option value="24">Раз в сутки</option>
    <option value="72">Раз в 3 дня</option>
    <option value="168">Раз в неделю</option>
   </select>
   <div class="mut" style="margin-top:8px">Каждая проверка поднимает Wi-Fi примерно на 15 секунд. Чем чаще — тем быстрее сядет батарея. Для года работы лучше «раз в сутки» или реже.</div>
  </div>
  <button class="btn" onclick="saveSet()">Сохранить ссылку</button>
  <button class="btn sec" onclick="otaCheck()">Запросить сейчас</button>
  <button class="btn green" id="bOta" style="display:none" onclick="otaInstall()">Установить обновление</button>
 </div>
 <div class="card"><h2>Система</h2>
  <div class="mut" id="sysInfo"></div>
  <button class="btn sec" onclick="sleepNow()">Завершить и уснуть</button>
  <button class="btn red" onclick="factory()">Сброс до заводских настроек</button>
 </div>
</section>
</main>

<nav>
 <button class="on" data-t="home"><i>&#9673;</i>Главная</button>
 <button data-t="hist"><i>&#9636;</i>История</button>
 <button data-t="cart"><i>&#9678;</i>Картридж</button>
 <button data-t="set"><i>&#8801;</i>Настройки</button>
</nav>
<div id="toast" class="toast"></div>

<script>
const $=id=>document.getElementById(id);
const KEYS=['name','model','shop','tgt','tgc','lpp','cl','cm','ftype','t1','t2','t3','rise','ota','autoOta','otaHours','ghrepo'];
const WC=[['нет данных','#8a97a6'],['Отличная','#1aa260'],['Хорошая','#4caf50'],['Допустимая','#f0a020'],['Плохая','#e0413a']];
const FS=[['В норме','#1aa260'],['Скоро замена','#f0a020'],['Требуется замена','#e0413a']];
let st=null;

function toast(m){const t=$('toast');t.textContent=m;t.style.opacity=1;clearTimeout(t._h);t._h=setTimeout(()=>t.style.opacity=0,2500)}
async function api(u,d){const o=d===undefined?{}:{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(d)};const r=await fetch(u,o);return r.json()}
function esc(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
function f(v,d){return v==null||isNaN(v)?'–':(+v).toFixed(d||0)}
function ago(s){if(s<90)return s+' с';if(s<5400)return Math.round(s/60)+' мин';if(s<172800)return Math.round(s/3600)+' ч';return Math.round(s/86400)+' дн'}
function bar(id,v){const e=$(id);e.style.width=Math.min(100,v)+'%';e.style.background=v>=100?'var(--bad)':v>=80?'var(--warn)':'var(--pri)'}
function show(t){document.querySelectorAll('.tab').forEach(e=>e.classList.toggle('on',e.id=='t-'+t));document.querySelectorAll('nav button').forEach(b=>b.classList.toggle('on',b.dataset.t==t));if(t=='hist')loadHist()}
document.querySelectorAll('nav button').forEach(b=>b.onclick=()=>show(b.dataset.t));

function render(s){st=s;
 $('hName').textContent=s.name;
 $('hSub').textContent=(s.mode=='ap'?'Точка доступа':'Wi-Fi «'+s.ssid+'»')+' · '+s.ip+' · уснёт через '+Math.max(0,Math.ceil(s.svcLeft/60))+' мин';
 const fs=FS[s.fstate],left=Math.max(0,100-s.wear),arc=$('ringArc');
 arc.style.strokeDashoffset=(326.7*(1-left/100)).toFixed(1);arc.style.stroke=fs[1];
 $('ringTxt').textContent=Math.round(left)+'%';
 $('fState').textContent=fs[0];$('fState').style.color=fs[1];
 $('fLeft').innerHTML='Осталось ≈ '+f(s.litersLeft)+' л'+(s.forecast>=0?' · ~'+s.forecast+' дн.':'')+
  '<br>Качество воды — по TDS. Ресурс картриджа — расход, срок и TDS';
 const wc=WC[s.wclass],b=$('wBadge');b.textContent=wc[0];b.style.background=wc[1];
 $('wNote').textContent=s.sensorFault?'датчик TDS не в воде':s.tempWarn?'вода тёплая (>30 °C)':s.wclass==0?'ждём первого замера':s.waterOk?'вода качественная':'вода требует внимания';
 $('vTds').textContent=s.tdsAge>=0?f(s.tds):'–';$('vTemp').textContent=s.tdsAge>=0?f(s.temp,1):'–';
 $('vBase').textContent=s.base>0?f(s.base):'копится';$('vAge').textContent=s.tdsAge>=0?ago(s.tdsAge):'–';
 $('vBatt').textContent=s.battMv?s.battPct+'%':'–';$('vBattS').textContent='Батарея'+(s.battMv?' · '+(s.battMv/1000).toFixed(2)+' В':'');
 const lk=$('vLeak');lk.textContent=!s.leakEn?'выкл':s.leak?'ЕСТЬ!':'нет';lk.style.color=s.leak?'var(--bad)':'';
 $('vTotal').textContent=f(s.total);$('vToday').textContent=f(s.today,1);
 $('cInfo').innerHTML=esc(s.model)+'<br>Установлен: '+(s.timeValid?new Date(s.install*1000).toLocaleDateString():f(s.days)+' дн. назад')+
  '<br>Прошло: '+f(s.liters)+' из '+s.cl+' л за '+f(s.days)+' дн.<br>В среднем '+f(s.avgLpd,1)+' л/сутки';
 bar('bVol',s.wearVol);bar('bTime',s.wearTime);bar('bTds',s.wearTds);
 const q='device='+s.id+'&model='+encodeURIComponent(s.model)+'&wear='+Math.round(s.wear)+'&liters='+Math.round(s.liters);
 const base=s.shop+(s.shop.includes('?')?'&':'?');
 $('bBuy').href=base+'action=buy&'+q;$('bSub').href=base+'action=subscribe&'+q;
 $('shopNote').textContent=s.mode=='ap'?'Ссылки откроются, когда телефон будет подключён к интернету.':s.fstate?'Самое время заказать новый картридж.':'С подпиской новый картридж приедет как раз к моменту замены.';
 $('sysInfo').innerHTML='ID '+s.id+' · прошивка '+s.fw+'<br>Время '+(s.timeValid?'синхронизировано':'не синхронизировано')+(s.rssi?' · сигнал Wi-Fi '+s.rssi+' dBm':'');
 if($('cloudInfo')&&!$('cloudInfo').dataset.busy)$('cloudInfo').textContent=s.cloudSet?(s.cloudAge>=0?'Последняя выгрузка в GitHub '+ago(s.cloudAge)+' назад':'Токен задан, статус ещё не отправляли'):'Задайте GitHub token — устройство будет писать data/latest.json в приватный репозиторий.';
 if($('otaInfo')&&!$('otaInfo').dataset.busy)$('otaInfo').textContent='Прошивка '+s.fw+(s.otaAge>=0?' · последняя проверка '+ago(s.otaAge)+' назад':s.autoOta?' · облако ещё не спрашивали':' · автообновление выключено');
 if(!$('wifiState').dataset.busy)$('wifiState').textContent=s.configured?'Сохранена сеть «'+s.ssid+'»':'Устройство ещё не подключено к домашней сети';
 let al=null;
 if(s.leak)al=['ОБНАРУЖЕНА ПРОТЕЧКА ВОДЫ!','var(--bad)'];
 else if(s.fstate==2)al=['Пора заменить картридж','var(--bad)'];
 else if(s.wclass==4)al=['Плохое качество воды','var(--bad)'];
 else if(s.fstate==1)al=['Скоро потребуется замена картриджа','var(--warn)'];
 else if(s.battMv&&s.battMv<3450)al=['Низкий заряд батареи','var(--warn)'];
 else if(!s.configured)al=['Подключите устройство к домашнему Wi-Fi во вкладке «Настройки»','var(--pri)'];
 const e=$('alert');e.style.display=al?'block':'none';if(al){e.textContent=al[0];e.style.background=al[1]}
}
async function refresh(){try{render(await api('/api/status'))}catch(e){$('hSub').textContent='нет связи с устройством (возможно, оно уснуло — нажмите кнопку)'}}

function fieldVal(el){return el.type==='checkbox'?(el.checked?1:0):el.value}
function otaUi(){const on=$('s_autoOta').checked;$('otaFreq').style.display=on?'':'none'}
async function loadSet(){const d=await api('/api/settings');KEYS.forEach(k=>{const el=$('s_'+k);if(!el||d[k]===undefined)return;if(el.type==='checkbox')el.checked=!!+d[k];else el.value=d[k]});$('s_tgt').value='';$('s_tgt').placeholder=d.tgSet?'задан (оставьте пустым)':'не задан';if($('s_ght')){$('s_ght').value='';$('s_ght').placeholder=d.ghSet?'задан (оставьте пустым)':'вставьте token'};otaUi()}
async function saveSet(){const o={};KEYS.forEach(k=>o[k]=fieldVal($('s_'+k)));await api('/api/settings',o);toast('Сохранено');loadSet();refresh()}
function preset(){const ro=$('s_ftype').value=='0';$('s_t1').value=ro?30:150;$('s_t2').value=ro?80:300;$('s_t3').value=ro?150:600;$('s_rise').value=ro?2:0}

async function measure(){toast('Измеряю…');const r=await api('/api/measure',{});render(r);toast(r.sensorFault?'Датчик TDS не в воде':'Готово')}
async function replaced(){if(!confirm('Начать отсчёт ресурса для нового картриджа?'))return;render(await api('/api/replace',{}));toast('Новый картридж учтён')}
async function calibrate(){$('calRes').textContent='Измеряю…';const r=await api('/api/calibrate',{ref:$('calRef').value});$('calRes').textContent=r.ok?'Готово: K = '+r.k.toFixed(3)+' (t = '+r.temp.toFixed(1)+' °C)':'Ошибка: '+r.err}
let flowTimer=null;
function flowUi(on){$('flowRun').style.display=on?'':'none';$('bFlowStart').style.display=on?'none':''}
let flowPrev={t:0,p:0};
async function flowTick(){try{const r=await api('/api/flowcal');$('flowPulses').textContent=r.pulses;if($('flowPin'))$('flowPin').textContent=r.low?'замкнут':'разомкнут';if($('flowEdges'))$('flowEdges').textContent=r.edges||0;const now=Date.now();if(flowPrev.t){const dt=(now-flowPrev.t)/1000;if(dt>0.2&&$('flowHz'))$('flowHz').textContent=(((r.pulses-flowPrev.p)/dt)||0).toFixed(1)}flowPrev={t:now,p:r.pulses};if(!r.on){clearInterval(flowTimer);flowTimer=null;flowUi(false)}}catch(e){}}
async function flowStart(){const r=await api('/api/flowcal',{action:'start'});if(!r.ok)return toast(r.err);flowPrev={t:0,p:0};$('flowCalRes').textContent='Откройте кран — импульсы должны пойти сразу.';flowUi(true);clearInterval(flowTimer);flowTimer=setInterval(flowTick,300);flowTick()}
async function flowStop(){const liters=$('flowLiters').value;clearInterval(flowTimer);flowTimer=null;const r=await api('/api/flowcal',{action:'stop',liters});flowUi(false);
 if(!r.ok){$('flowCalRes').textContent='Ошибка: '+r.err;return}
 $('s_lpp').value=r.lpp.toFixed(3);$('flowCalRes').textContent='Готово: '+r.pulses+' имп. на '+r.liters+' л → '+r.lpp.toFixed(3)+' л/импульс. Сохранено.';toast('Счётчик откалиброван')}
async function flowCancel(){clearInterval(flowTimer);flowTimer=null;await api('/api/flowcal',{action:'cancel'});flowUi(false);$('flowCalRes').textContent='Калибровка отменена'}
async function testTg(){const r=await api('/api/testtg',{});toast(r.ok?'Сообщение отправлено':'Ошибка: '+r.err)}
async function sleepNow(){await api('/api/sleep',{});toast('Устройство уснуло')}
async function factory(){if(!confirm('Стереть все настройки и счётчики?'))return;await api('/api/reset',{});toast('Сброшено, устройство перезагружается')}
async function cloudNow(){const e=$('cloudInfo');e.dataset.busy=1;e.textContent='Отправляю в GitHub…';
 try{const r=await api('/api/cloud',{});e.textContent=r.ok?'Статус записан в data/latest.json':'Ошибка: '+r.err}catch(x){e.textContent='Нет связи с устройством'} delete e.dataset.busy}
async function otaCheck(){const e=$('otaInfo');e.dataset.busy=1;e.textContent='Запрашиваю облако…';$('bOta').style.display='none';
 try{const r=await api('/api/ota');if(!r.ok){e.textContent='Ошибка: '+r.err;delete e.dataset.busy;return}
 e.innerHTML=r.newer?'Доступна <b>'+esc(r.latest)+'</b> (сейчас '+esc(r.current)+')'+(r.notes?'<br>'+esc(r.notes):''):'Уже стоит последняя версия '+esc(r.current);
 $('bOta').style.display=r.newer?'':'none'}catch(x){e.textContent='Нет связи с устройством'} delete e.dataset.busy}
async function otaInstall(){if(!confirm('Установить обновление? Страница отключится на 1–2 минуты, не выключайте питание.'))return;
 $('otaInfo').textContent='Качаю и прошиваю… не выключайте устройство.';
 try{const r=await api('/api/ota',{});if(!r.ok){$('otaInfo').textContent='Ошибка: '+r.err;return}}catch(x){}
 toast('Прошивка идёт, жду перезагрузку…');
 for(let i=0;i<90;i++){await new Promise(z=>setTimeout(z,2000));try{await refresh();$('otaInfo').textContent='Готово, установлена '+st.fw;$('bOta').style.display='none';return}catch(e){}}}

async function scan(){const n=$('nets');n.innerHTML='<div class="mut">Поиск…</div>';const l=await api('/api/scan');n.innerHTML=l.length?'':'<div class="mut">Сети не найдены</div>';
 l.sort((a,b)=>b.rssi-a.rssi).forEach(x=>{const d=document.createElement('div');d.className='net';d.innerHTML='<span>'+esc(x.ssid)+'</span><span class="mut">'+x.rssi+' dBm'+(x.open?' · открытая':'')+'</span>';d.onclick=()=>{$('w_ssid').value=x.ssid;$('w_pass').focus()};n.appendChild(d)})}
async function saveWifi(){const ssid=$('w_ssid').value.trim(),ws=$('wifiState');if(!ssid)return toast('Введите имя сети');
 const r=await api('/api/wifi',{ssid,pass:$('w_pass').value});
 if(!r.ok)return toast(r.err);
 if(r.saved){ws.textContent='Сохранено. Новая сеть будет использоваться со следующего пробуждения.';return}
 ws.dataset.busy=1;ws.textContent='Подключаюсь к «'+ssid+'»…';
 for(let i=0;i<30;i++){await new Promise(z=>setTimeout(z,1000));let w;try{w=await api('/api/wifistatus')}catch(e){continue}
  if(w.state==2){ws.innerHTML='Готово! Устройство подключено к «'+esc(ssid)+'». Адрес: <b>http://'+w.ip+'</b> — сейчас откроется автоматически. Сохраните его в закладки.';goHome('http://'+w.ip+'/');return}
  if(w.state==3){ws.textContent='Не удалось подключиться — проверьте пароль.';delete ws.dataset.busy;return}}
 ws.textContent='Нет ответа. Переподключитесь к точке доступа устройства и попробуйте снова.';delete ws.dataset.busy}

async function goHome(url){for(let i=0;i<120;i++){await new Promise(z=>setTimeout(z,2000));const c=new AbortController(),t=setTimeout(()=>c.abort(),1500);
 try{await fetch(url+'api/status?t='+Date.now(),{mode:'no-cors',cache:'no-store',signal:c.signal});clearTimeout(t);location.href=url;return}catch(e){clearTimeout(t)}}}
function chart(id,vals,labels,color,bands){
 const c=$(id),r=devicePixelRatio||1,W=c.clientWidth,H=c.clientHeight;c.width=W*r;c.height=H*r;
 const x=c.getContext('2d');x.scale(r,r);
 const css=getComputedStyle(document.documentElement);
 const mut=css.getPropertyValue('--mut'),line=css.getPropertyValue('--line');
 const marks=(bands||[]).map(b=>+b.y).filter(v=>v>0);
 const max=Math.max(1,...vals,...marks)*1.15,pl=36,pr=54,pb=22,n=Math.max(1,vals.length),w=(W-pl-pr)/n;
 const Y=v=>(H-pb)*(1-v/max);
 x.font='11px sans-serif';
 for(let i=0;i<=4;i++){const y=Y(max*i/4);x.fillStyle=mut;x.globalAlpha=.85;x.fillText(Math.round(max*i/4),0,y+4);x.globalAlpha=.28;x.fillStyle=line;x.fillRect(pl,y,W-pl-pr,1)}
 x.globalAlpha=1;
 (bands||[]).forEach(b=>{if(!(+b.y>0)||b.y>max)return;const y=Y(b.y);x.strokeStyle=b.color;x.setLineDash([5,4]);x.lineWidth=1.2;x.beginPath();x.moveTo(pl,y);x.lineTo(W-pr,y);x.stroke();x.setLineDash([]);x.fillStyle=b.color;x.fillText(b.label,W-pr+4,y+4)});
 x.fillStyle=mut;const step=Math.ceil(n/6);labels.forEach((l,i)=>{if(i%step==0||i==n-1)x.fillText(l,pl+w*i,H-4)});
 x.strokeStyle=color;x.fillStyle=color;x.lineWidth=2;x.lineJoin='round';x.lineCap='round';
 x.beginPath();let started=false;
 vals.forEach((v,i)=>{if(v==null||v==='')return;const X=pl+w*i+w/2,y=Y(+v||0);started?x.lineTo(X,y):x.moveTo(X,y);started=true});
 x.stroke();
 vals.forEach((v,i)=>{if(v==null||v==='')return;const X=pl+w*i+w/2,y=Y(+v||0);x.beginPath();x.arc(X,y,3,0,Math.PI*2);x.fill()});
}
async function loadHist(){const h=await api('/api/history');
 const L=h.days.map(d=>d.l),T=h.days.map(d=>d.t||null),lab=h.days.map(d=>d.e>1700000000?new Date(d.e*1000).toLocaleDateString(undefined,{day:'numeric',month:'numeric'}):'');
 L.push(h.today);T.push(h.todayTds||null);lab.push('сейчас');
 const t1=h.t1||(st&&st.t1)||30,t2=h.t2||(st&&st.t2)||80,t3=h.t3||(st&&st.t3)||150;
 chart('cL',L,lab,'#0b6bcb');
 chart('cT',T,lab,'#0b6bcb',[{y:t1,color:'#1aa260',label:'отл. '+t1},{y:t2,color:'#f0a020',label:'допуст. '+t2},{y:t3,color:'#e0413a',label:'плохо '+t3}]);
}

(async()=>{try{await api('/api/time',{epoch:Math.floor(Date.now()/1000)})}catch(e){}
 await refresh();loadSet();if(st&&!st.configured)show('set');setInterval(refresh,3000)})();
</script></body></html>)HTML";

const char SETUP_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="ru"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#0b6bcb">
<title>AquaSense — подключение к Wi-Fi</title>
<style>
*{box-sizing:border-box}
body{margin:0;font:16px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;background:#eef4fa;color:#10243a}
header{background:linear-gradient(135deg,#0b6bcb,#22a6e0);color:#fff;padding:24px 18px 44px}
header h1{margin:0;font-size:23px}header p{margin:6px 0 0;opacity:.9}
main{max-width:480px;margin:-28px auto 0;padding:0 12px 24px}
.card{background:#fff;border-radius:16px;box-shadow:0 2px 10px rgba(0,0,0,.06);margin-bottom:12px;overflow:hidden}
.pad{padding:18px}
.net{border-bottom:1px solid #eef2f6}.net:last-child{border:0}
.top{display:flex;justify-content:space-between;align-items:center;padding:15px 16px;cursor:pointer;gap:10px}
.top b{font-weight:500;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.sig{display:inline-flex;gap:2px;align-items:flex-end;height:15px;flex:none}.sig i{width:4px;background:#d3dee9;border-radius:1px}.sig i.on{background:#0b6bcb}
.form{display:none;padding:0 16px 16px}.net.open .form{display:block}.net.open{background:#f7fafd}
input[type=text],input[type=password]{width:100%;padding:13px;border:1px solid #c9d6e3;border-radius:10px;font-size:16px;background:#fff}
.chk{display:flex;gap:8px;align-items:center;margin-top:10px;color:#6b7c90;font-size:14px}
.btn{display:block;width:100%;padding:14px;border:0;border-radius:12px;background:#0b6bcb;color:#fff;font-size:16px;font-weight:600;margin-top:12px;text-align:center;text-decoration:none;cursor:pointer}
.btn.sec{background:#fff;color:#0b6bcb;box-shadow:0 2px 10px rgba(0,0,0,.06)}
.mut{color:#6b7c90;font-size:14px}.err{color:#e0413a;font-size:14px;margin-top:8px}
.spin{width:40px;height:40px;border:4px solid #dde7f1;border-top-color:#0b6bcb;border-radius:50%;animation:s 1s linear infinite;margin:6px auto 14px}
@keyframes s{to{transform:rotate(360deg)}}
.ok{width:56px;height:56px;border-radius:50%;background:#1aa260;color:#fff;font-size:32px;line-height:56px;text-align:center;margin:0 auto 12px}
.ip{font-size:22px;font-weight:700;color:#10243a;margin:4px 0 10px}
</style></head><body>
<header><h1>AquaSense</h1><p>Выберите домашнюю сеть Wi-Fi</p></header>
<main>
<div id="pick">
 <div id="list" class="card"><div class="pad mut">Поиск сетей…</div></div>
 <button class="btn sec" onclick="load(1)">Обновить список</button>
 <button class="btn sec" onclick="manual()">Другая сеть (скрытая)</button>
</div>
<div id="state" class="card pad" style="display:none;text-align:center"></div>
</main>
<script>
const $=id=>document.getElementById(id);
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
function esc(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
async function getJ(u){return (await fetch(u,{cache:'no-store'})).json()}
async function post(u,d){return (await fetch(u,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(d)})).json()}
function bars(r){const n=r>-55?4:r>-65?3:r>-75?2:1;let h='';for(let i=1;i<=4;i++)h+='<i class="'+(i<=n?'on':'')+'" style="height:'+(i*3+3)+'px"></i>';return'<span class="sig">'+h+'</span>'}

function item(ssid,open,rssi){
 const d=document.createElement('div');d.className='net';
 const hidden=ssid===null;
 d.innerHTML='<div class="top"><b>'+(hidden?'Другая сеть':esc(ssid))+'</b><span class="mut">'+(open?'открытая ':'')+(rssi!=null?bars(rssi):'')+'</span></div>'+
  '<div class="form">'+(hidden?'<input type="text" class="ss" placeholder="Имя сети" autocapitalize="off" autocorrect="off" style="margin-bottom:10px">':'')+
  (open?'':'<input type="password" class="pw" placeholder="Пароль Wi-Fi" autocomplete="current-password">'+
  '<label class="chk"><input type="checkbox" class="sh">Показать пароль</label>')+
  '<button class="btn go">Подключить</button><div class="err"></div></div>';
 d.querySelector('.top').onclick=()=>{document.querySelectorAll('.net.open').forEach(x=>x!==d&&x.classList.remove('open'));d.classList.toggle('open');const f=d.querySelector(hidden?'.ss':'.pw');if(f&&d.classList.contains('open'))f.focus()};
 const pw=d.querySelector('.pw'),sh=d.querySelector('.sh');
 if(sh)sh.onchange=()=>pw.type=sh.checked?'text':'password';
 if(pw)pw.onkeydown=e=>{if(e.key=='Enter')d.querySelector('.go').click()};
 d.querySelector('.go').onclick=()=>connect(hidden?d.querySelector('.ss').value.trim():ssid,pw?pw.value:'',d);
 return d}

async function load(refresh){
 const l=$('list');l.innerHTML='<div class="pad mut">Поиск сетей…</div>';
 let n=[];try{n=await getJ('/api/scan'+(refresh?'?refresh=1':''))}catch(e){}
 l.innerHTML='';
 if(!n.length){l.innerHTML='<div class="pad mut">Сети не найдены. Нажмите «Обновить список».</div>';return}
 n.sort((a,b)=>b.rssi-a.rssi).forEach(x=>l.appendChild(item(x.ssid,x.open,x.rssi)))}
function manual(){const d=item(null,false,null);$('list').prepend(d);d.querySelector('.top').click()}

function state(html){$('pick').style.display=html?'none':'';$('state').style.display=html?'':'none';if(html)$('state').innerHTML=html}

async function connect(ssid,pass,el){
 const err=el.querySelector('.err');err.textContent='';
 if(!ssid){err.textContent='Введите имя сети';return}
 if(el.querySelector('.pw')&&pass.length<8){err.textContent='Пароль Wi-Fi — не меньше 8 символов';return}
 let r;try{r=await post('/api/wifi',{ssid,pass})}catch(e){err.textContent='Нет связи с устройством';return}
 if(!r.ok){err.textContent=r.err;return}
 state('<div class="spin"></div><div>Подключаюсь к «'+esc(ssid)+'»…</div><div class="mut" style="margin-top:6px">Это займёт до 20 секунд</div>');
 for(let i=0;i<35;i++){await sleep(1000);let w;try{w=await getJ('/api/wifistatus')}catch(e){continue}
  if(w.state==2)return done(ssid,w.ip);
  if(w.state==3){state('');el.classList.add('open');err.textContent='Не удалось подключиться. Проверьте пароль.';return}}
 state('');err.textContent='Устройство не ответило. Попробуйте ещё раз.'}

function done(ssid,ip){
 const url='http://'+ip+'/';
 try{localStorage.setItem('aquasense',url)}catch(e){}
 state('<div class="ok">&#10003;</div><h2 style="margin:0 0 6px">Готово!</h2>'+
  '<div class="mut">Устройство подключено к «'+esc(ssid)+'». Его адрес в домашней сети:</div>'+
  '<div class="ip">'+ip+'</div>'+
  '<div class="mut" id="sw">Через несколько секунд сеть AquaSense выключится, телефон вернётся в домашний Wi-Fi и приложение откроется само.</div>'+
  '<a class="btn" href="'+url+'">Открыть приложение</a>'+
  '<div class="mut" style="margin-top:12px">Сохраните адрес в закладки или добавьте страницу на главный экран. Чтобы разбудить устройство, нажмите на нём кнопку.</div>');
 waitHome(url)}

async function waitHome(url){
 for(let i=0;i<120;i++){await sleep(2000);
  const c=new AbortController(),t=setTimeout(()=>c.abort(),1500);
  try{await fetch(url+'api/status?t='+Date.now(),{mode:'no-cors',cache:'no-store',signal:c.signal});clearTimeout(t);location.href=url;return}catch(e){clearTimeout(t)}}
 const s=$('sw');if(s)s.textContent='Подключите телефон к домашнему Wi-Fi и нажмите «Открыть приложение».'}

post('/api/time',{epoch:Math.floor(Date.now()/1000)}).catch(()=>{});
load(0);
</script></body></html>)HTML";
