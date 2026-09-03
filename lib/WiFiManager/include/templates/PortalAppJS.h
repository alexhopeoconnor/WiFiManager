/**
 * PortalAppJS.h
 *
 * @author alexhopeoconnor
 * @license MIT
 *
 * Embedded SPA for WiFiManager config portal (hash routing + JSON APIs).
 */

#ifndef _WM_PORTAL_APP_JS_H_
#define _WM_PORTAL_APP_JS_H_

#include <Arduino.h>

const char PORTAL_APP_JS[] PROGMEM = R"rawliteral(
(function(){
'use strict';
var boot={};
try{var el=document.getElementById('wm-bootstrap');if(el&&el.textContent)boot=JSON.parse(el.textContent);}catch(e){boot={};}

// --- Core / DOM ---
function $(id){return document.getElementById(id);}
function esc(t){if(!t)return'';return String(t).replace(/[&<>"']/g,function(c){return({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]);});}

// --- Toast / dialog ---
var _wmToastTimer=null;
var _wmPortalTimeoutTimer=null;
function showToast(msg,isErr){
  var el=$('wm-toast');
  if(!el)return;
  el.textContent=msg||'';
  el.className='wm-toast '+(isErr?'err':'ok');
  el.style.display='block';
  if(_wmToastTimer)clearTimeout(_wmToastTimer);
  _wmToastTimer=setTimeout(function(){el.style.display='none';el.textContent='';},4500);
}
function showConfirm(msg,onOk){
  var dlg=$('wm-dialog'),dmsg=$('wm-dialog-msg'),bk=$('wm-dialog-backdrop'),ok=$('wm-dialog-ok'),cancel=$('wm-dialog-cancel');
  if(!dlg||!dmsg||!ok||!cancel){if(onOk)onOk();return;}
  dmsg.textContent=msg||'';
  dlg.style.display='flex';
  dlg.setAttribute('aria-hidden','false');
  function hide(){
    dlg.style.display='none';
    dlg.setAttribute('aria-hidden','true');
    ok.onclick=null;
    cancel.onclick=null;
    if(bk)bk.onclick=null;
  }
  function onCancel(){hide();}
  cancel.onclick=onCancel;
  if(bk)bk.onclick=onCancel;
  ok.onclick=function(){hide();if(onOk)onOk();};
}

function bindPortalChromeEvents(){
  var back=$('wm-nav-back');
  if(back){
    back.addEventListener('click', function(ev){ ev.preventDefault(); history.back(); });
  }
  var r=$('wm-restart-btn'), e=$('wm-exit-btn'), c=$('wm-close-captive-btn'), er=$('wm-erase-btn');
  if(r && window.portalRestart) r.addEventListener('click', window.portalRestart);
  if(e && window.portalExit) e.addEventListener('click', window.portalExit);
  if(c && window.portalCloseCaptive) c.addEventListener('click', window.portalCloseCaptive);
  if(er && window.portalErase) er.addEventListener('click', window.portalErase);
}

function bindFormSubmitHandlers(){
  var wf=document.querySelectorAll('.wm-wifi-data-form'), pf=$('wm-param-form'), of=$('wm-ota-form'), wa=document.querySelectorAll('[data-wm-wifi-action]'), i;
  if(window.portalWifiSave){
    for(i=0;i<wf.length;i++)wf[i].addEventListener('submit', window.portalWifiSave);
  }
  if(window.portalWifiAction){
    for(i=0;i<wa.length;i++)wa[i].addEventListener('click', window.portalWifiAction);
  }
  if(pf && window.portalParamSave) pf.addEventListener('submit', window.portalParamSave);
  if(of && window.portalOtaSubmit) of.addEventListener('submit', window.portalOtaSubmit);
}

// --- View shell ---
function stopPortalTimeoutCountdown(){
  if(_wmPortalTimeoutTimer){
    clearInterval(_wmPortalTimeoutTimer);
    _wmPortalTimeoutTimer=null;
  }
}

function setView(html){
  var a=$('app');
  if(!a)return;
  stopPortalTimeoutCountdown();
  stopWifiScanPolling();
  stopWifiConnectPolling();
  a.innerHTML="<div class='wm-layout'>"+html+"</div>";
  bindPortalChromeEvents();
  bindFormSubmitHandlers();
}

function formatDurationSeconds(totalSeconds){
  totalSeconds=Math.max(0,Math.floor(totalSeconds||0));
  var hours=Math.floor(totalSeconds/3600);
  var minutes=Math.floor((totalSeconds%3600)/60);
  var seconds=totalSeconds%60;
  if(hours>0){
    return hours+":"+String(minutes).padStart(2,'0')+":"+String(seconds).padStart(2,'0');
  }
  return minutes+":"+String(seconds).padStart(2,'0');
}

function startPortalTimeoutCountdown(initialSeconds){
  var el=$('wm-portal-timeout');
  var remaining=Math.max(0,Math.floor(initialSeconds||0));
  if(!el||remaining<=0)return;
  function levelClass(seconds){
    if(seconds<10)return'wm-status--danger';
    if(seconds<30)return'wm-status--caution';
    if(seconds<60)return'wm-status--warning';
    return'';
  }
  function render(){
    if(!el)return;
    el.className='wm-status'+(levelClass(remaining)?' '+levelClass(remaining):'');
    el.textContent='Portal closes in '+formatDurationSeconds(remaining);
  }
  render();
  _wmPortalTimeoutTimer=setInterval(function(){
    remaining=Math.max(0,remaining-1);
    render();
    if(remaining<=0)stopPortalTimeoutCountdown();
  },1000);
}

// --- Field render (WiFi / params meta) ---
function renderStandardField(f){
  var id=f.id||f.name||'';
  var nm=f.name||f.id||'';
  var domId=(id==='s')?'wm-s':(id==='p')?'wm-p':('wm-f-'+id);
  var t=f.type||'text';
  var h='<div class="wm-field">';
  h+='<label for="'+domId+'">'+esc(f.label||'')+'</label>';
  h+='<input id="'+domId+'" name="'+esc(nm)+'" type="'+esc(t)+'"';
  if(f.maxlength)h+=' maxlength="'+f.maxlength+'"';
  if(f.value!==undefined&&f.value!==null)h+=' value="'+esc(f.value)+'"';
  if(f.placeholder)h+=' placeholder="'+esc(f.placeholder)+'"';
  if(f.customAttrs)h+=' '+f.customAttrs;
  h+='/>';
  h+='</div>';
  return h;
}

function renderMetaField(f){
  if(!f)return'';
  if(f.kind==='html')return f.html||'';
  if(f.kind==='field' || !f.kind)return renderStandardField(f);
  return '';
}

function renderFieldList(arr){
  if(!arr||!arr.length)return'';
  var h='',i;
  for(i=0;i<arr.length;i++)h+=renderMetaField(arr[i]);
  return h;
}

function iconSvg(name){
  var body='';
  switch(name){
    case 'home':
      body="<path d='M3 10.5 12 3l9 7.5'/><path d='M5 9.5V20h14V9.5'/>";
      break;
    case 'wifi':
      body="<path d='M4.5 9.5a12 12 0 0 1 15 0'/><path d='M7.5 12.5a7.5 7.5 0 0 1 9 0'/><path d='M10.5 15.5a3.5 3.5 0 0 1 3 0'/><circle cx='12' cy='19' r='1.2' fill='currentColor' stroke='none'/>";
      break;
    case 'setup':
      body="<line x1='5' y1='6' x2='19' y2='6'/><circle cx='9' cy='6' r='2'/><line x1='5' y1='12' x2='19' y2='12'/><circle cx='15' cy='12' r='2'/><line x1='5' y1='18' x2='19' y2='18'/><circle cx='11' cy='18' r='2'/>";
      break;
    case 'info':
      body="<circle cx='12' cy='12' r='9'/><line x1='12' y1='10.5' x2='12' y2='16.5'/><circle cx='12' cy='7.2' r='1' fill='currentColor' stroke='none'/>";
      break;
    case 'update':
      body="<path d='M12 4v10'/><path d='m8.5 10.5 3.5 3.5 3.5-3.5'/><path d='M5 19h14'/>";
      break;
    case 'back':
      body="<path d='M10 6 4 12l6 6'/><path d='M4 12h16'/>";
      break;
    case 'restart':
      body="<path d='M20 12a8 8 0 1 1-2.3-5.7'/><path d='M20 4v5h-5'/>";
      break;
    case 'exit':
      body="<path d='M10 6H6v12h4'/><path d='M14 8l4 4-4 4'/><path d='M9 12h9'/>";
      break;
    case 'close':
      body="<path d='M4 12a8 8 0 1 0 16 0A8 8 0 1 0 4 12'/><path d='m8.5 8.5 7 7'/>";
      break;
    case 'erase':
      body="<path d='M5 7h14'/><path d='M9 7V5h6v2'/><path d='M8 7l1 12h6l1-12'/><path d='M10 10v6'/><path d='M14 10v6'/>";
      break;
    case 'status':
      body="<circle cx='12' cy='12' r='9'/><path d='M12 7v5l3 2'/>";
      break;
    case 'device':
      body="<rect x='7' y='7' width='10' height='10' rx='1.5'/><path d='M9 1v4M15 1v4M9 19v4M15 19v4M1 9h4M1 15h4M19 9h4M19 15h4'/>";
      break;
    case 'diagnostics':
      body="<path d='M4 17h3l2-5 3 3 2-7 2 9h4'/>";
      break;
    case 'about':
      body="<circle cx='12' cy='12' r='9'/><path d='M12 10.5v5.5'/><circle cx='12' cy='7.2' r='1' fill='currentColor' stroke='none'/>";
      break;
    case 'actions':
      body="<path d='M12 3v18'/><path d='M3 12h18'/><path d='m6.5 6.5 11 11'/><path d='m17.5 6.5-11 11'/>";
      break;
    case 'scan':
      body="<path d='M4 7h16'/><path d='M4 12h10'/><path d='M4 17h7'/><path d='m16 15 4 4'/><circle cx='14' cy='13' r='4'/>";
      break;
    case 'save':
      body="<path d='M5 4h11l3 3v13H5z'/><path d='M8 4v5h8'/><path d='M9 19v-5h6v5'/>";
      break;
    case 'connect':
      body="<path d='M4.5 12a7.5 7.5 0 0 1 7.5-7.5'/><path d='m17 6 2 2-2 2'/><path d='M12 4.5H19'/>";
      break;
    case 'upload':
      body="<path d='M12 20V8'/><path d='m8.5 11.5 3.5-3.5 3.5 3.5'/><path d='M5 20h14'/>";
      break;
  }
  if(!body)return'';
  return "<span class='wm-icon wm-icon--"+name+"' aria-hidden='true'><svg viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='1.9' stroke-linecap='round' stroke-linejoin='round'>"+body+"</svg></span>";
}

function labelWithIcon(name,label){
  return "<span class='wm-icon-label'>"+iconSvg(name)+"<span>"+esc(label||'')+"</span></span>";
}

function cardTitleHtml(name,title){
  return "<h2 class='wm-card-title'>"+labelWithIcon(name,title)+"</h2>";
}

function sectionCardHtml(name,title,subtitle,rows){
  if(!rows||!rows.length)return'';
  var h="<div class='wm-card'>"+cardTitleHtml(name,title);
  if(subtitle)h+="<p class='wm-card-subtitle'>"+esc(subtitle)+"</p>";
  h+="<dl class='wm-kv'>";
  for(var i=0;i<rows.length;i++){
    h+="<dt>"+esc(rows[i].label)+"</dt><dd>"+rows[i].value+"</dd>";
  }
  h+="</dl></div>";
  return h;
}

function rowsWithoutKeys(rows,blocked){
  if(!rows||!rows.length)return[];
  var out=[],i,row,key;
  for(i=0;i<rows.length;i++){
    row=rows[i];
    if(!row)continue;
    key=row.key||'';
    if(blocked.indexOf(key)!==-1)continue;
    out.push(row);
  }
  return out;
}

function portalHeader(){
  var brand=boot.brand||{};
  var ctx=boot.context||{};
  var identity=esc(ctx.identityText||brand.title||'WiFiManager');
  var logoAlt=esc(brand.logoAltText||'');
  var h="<header class='wm-site-header'>";
  if(brand.logoSvg){h+="<div class='wm-site-logo'"+(logoAlt?" role='img' aria-label='"+logoAlt+"'":"")+">"+brand.logoSvg+"</div>";}
  h+="<div class='wm-site-brand-copy'><p class='wm-site-brand-name'>"+identity+"</p>";
  if(brand.tagline){h+="<p class='wm-site-tagline'>"+esc(brand.tagline)+"</p>";}
  h+="</div></header>";
  return h;
}

function applyPortalFavicon(){
  var brand=boot.brand||{};
  if(!brand.logoSvg)return;
  var icon=document.querySelector("link[rel~='icon']");
  if(!icon){icon=document.createElement('link');icon.rel='icon';document.head.appendChild(icon);}
  icon.type='image/svg+xml';
  icon.href='data:image/svg+xml,'+encodeURIComponent(brand.logoSvg);
}

function navBar(active){
  active=active||'home';
  var pages=boot.pages||{};
  var layout=boot.layout||{};
  var actions=boot.actions||{};
  var pinWifi=(layout.paramsLocation||'wifi')==='wifi';
  var ps=pages.setup||{};
  var pi=pages.info||{};
  var pu=pages.update||{};
  var h=portalHeader()+"<nav class='wm-nav' aria-label='Configuration'>";
  h+="<a class='wm-nav-link"+(active==='home'?' wm-nav-link--active':'')+"' href='#/'>"+labelWithIcon('home','Overview')+"</a>";
  h+="<a class='wm-nav-link"+(active==='wifi'?' wm-nav-link--active':'')+"' href='#/wifi'>"+labelWithIcon('wifi','Wi-Fi')+"</a>";
  if(!pinWifi && ps.visible!==false){h+="<a class='wm-nav-link"+(active==='setup'?' wm-nav-link--active':'')+"' href='#/setup'>"+labelWithIcon('setup','Settings')+"</a>";}
  if(pi.visible!==false){h+="<a class='wm-nav-link"+(active==='info'?' wm-nav-link--active':'')+"' href='#/info'>"+labelWithIcon('info','Device')+"</a>";}
  if(pu.visible){h+="<a class='wm-nav-link"+(active==='update'?' wm-nav-link--active':'')+"' href='#/update'>"+labelWithIcon('update','Update')+"</a>";}
  if(actions.back&&actions.back.visible){h+="<a href='#' class='wm-nav-link' id='wm-nav-back'>"+labelWithIcon('back','Back')+"</a>";}
  h+="</nav>";
  return h;
}

function deviceActionsHtml(){
  var actions=boot.actions||{};
  var h='', ok=false;
  if(actions.restart&&actions.restart.visible){h+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-restart-btn'>"+labelWithIcon('restart','Restart device')+"</button>";ok=true;}
  if(actions.exitPortal&&actions.exitPortal.visible){h+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-exit-btn'>"+labelWithIcon('exit','Exit portal')+"</button>";ok=true;}
  if(actions.closeCaptive&&actions.closeCaptive.visible){h+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-close-captive-btn'>"+labelWithIcon('close','Stop captive portal')+"</button>";ok=true;}
  if(!ok)return'';
  return "<div class='wm-card'>"+cardTitleHtml('device','Device')+"<p class='wm-card-subtitle'>Quick device-level actions while the portal is open.</p><div class='wm-btn-group'>"+h+"</div></div>";
}

function renderExtraHomeCards(cards){
  if(!cards||!cards.length)return'';
  var out='',i,c,j,k,cls;
  for(i=0;i<cards.length;i++){
    c=cards[i];
    if(!c)continue;
    out+="<div class='wm-card'>";
    if(c.title){out+="<h2 class='wm-card-title'>"+esc(c.title)+"</h2>";}
    if(c.kind==='text'||c.kind==='callout'){
      cls=c.kind==='callout'?'wm-callout wm-callout--info':'wm-lead';
      out+="<p class='"+cls+"'>"+esc(c.text||'')+"</p>";
    }else{
      out+="<dl class='wm-kv'>";
      if(c.items){for(j=0;j<c.items.length;j++){k=c.items[j];out+="<dt>"+esc(k.label)+"</dt><dd>"+esc(k.value)+"</dd>";}}
      out+="</dl>";
    }
    out+="</div>";
  }
  return out;
}

function renderHomeStatusDetails(info){
  var st=info.status||{};
  var rows=[];
  if(st.connected!==undefined)rows.push({label:'Wi-Fi',value:st.connected?'Connected':'Not connected'});
  if(st.ssid)rows.push({label:'Network',value:st.ssid});
  if(st.connected&&st.stationIp&&st.stationIp!=='0.0.0.0'&&st.stationIp!=='(IP unset)')rows.push({label:'Address',value:st.stationIp});
  else if(st.apIp)rows.push({label:'Portal',value:st.apIp});
  var device=info.device||[];
  var wanted={uptime:true,freeheap:true};
  for(var i=0;i<device.length;i++){
    if(device[i]&&wanted[device[i].key]){
      rows.push({label:device[i].label||device[i].key,value:device[i].value||''});
    }
  }
  if(!rows.length)return"<p class='wm-lead'>Device details are unavailable.</p>";
  var html="<dl class='wm-kv wm-home-status-details'>";
  for(var j=0;j<rows.length;j++){
    html+="<dt>"+esc(rows[j].label)+"</dt>";
    html+="<dd>"+esc(rows[j].value)+"</dd>";
  }
  return html+="</dl>";
}

function loadHomeStatus(){
  var box=$('wm-home-status-details');
  if(!box)return;
  api('/api/info').then(function(res){
    var info={};
    try{info=JSON.parse(res.body);}catch(e){}
    box.innerHTML=renderHomeStatusDetails(info);
  }).catch(function(){
    box.innerHTML="<p class='wm-lead'>Unable to load device details.</p>";
  });
}

function resetPortalTimeout(){
  var button=$('wm-reset-portal-timeout');
  if(button)button.disabled=true;
  api('/api/portal/timeout-reset',{method:'POST'}).then(function(res){
    var data={};
    try{data=JSON.parse(res.body);}catch(e){}
    if(!res.ok){
      showToast(data.message||'Unable to reset portal timeout.',true);
      return;
    }
    startPortalTimeoutCountdown(data.timeoutSecondsRemaining||0);
    showToast('Portal timeout reset.',false);
  }).catch(function(){
    showToast('Unable to reset portal timeout.',true);
  }).then(function(){
    if(button)button.disabled=false;
  });
}

function viewHome(){
  var brand=boot.brand||{};
  var ctx=boot.context||{};
  var title=esc(brand.title||'WiFiManager');
  var summary=ctx.statusSummary||'';
  var timeoutSeconds=ctx.portalTimeoutSecondsRemaining||0;
  var html=navBar('home');
  html+="<div class='wm-page-head'><h1>"+title+"</h1></div>";
  html+="<div class='wm-card'>"+cardTitleHtml('status','Device status');
  if(summary){
    html+="<p class='wm-home-summary'>"+esc(summary)+"</p>";
  }
  if(timeoutSeconds>0){
    html+="<div class='wm-portal-timeout-row'>";
    html+="<p class='wm-status' id='wm-portal-timeout'></p>";
    html+="<button type='button' class='wm-icon-button'";
    html+=" id='wm-reset-portal-timeout' aria-label='Reset portal timeout'";
    html+=" title='Reset portal timeout'>"+iconSvg('restart')+"</button></div>";
  }
  html+="<div id='wm-home-status-details'>";
  html+="<p class='wm-lead'>Loading device details…</p></div>";
  html+="<a class='wm-inline-link' href='#/info'>View full device details</a></div>";
  html+=renderExtraHomeCards(boot.extraHomeCards||[]);
  html+=deviceActionsHtml();
  setView(html);
  if(timeoutSeconds>0){
    startPortalTimeoutCountdown(timeoutSeconds);
    var reset=$('wm-reset-portal-timeout');
    if(reset)reset.addEventListener('click',resetPortalTimeout);
  }
  loadHomeStatus();
}

// --- HTTP ---
function api(path, opt){
  opt=opt||{};
  return fetch(path,{method:opt.method||'GET',headers:opt.headers,body:opt.body,credentials:'same-origin'})
    .then(function(r){return r.text().then(function(t){return {ok:r.ok,status:r.status,body:t};});});
}

// --- WiFi scan view ---
var _wmWifiScanPollTimer=null;
var _wmWifiScanPollToken=0;

function getScanResultsBox(){
  return $('wm-scan-results');
}

function setScanRefreshEnabled(enabled){
  var btn=$('wm-refresh-scan');
  if(btn)btn.disabled=(enabled===false);
}

function stopWifiScanPolling(){
  _wmWifiScanPollToken++;
  if(_wmWifiScanPollTimer){
    clearTimeout(_wmWifiScanPollTimer);
    _wmWifiScanPollTimer=null;
  }
  setScanRefreshEnabled(true);
}

function setWifiScanProgress(visible,title,detail){
  var overlay=$('wm-wifi-scan-overlay');
  var titleEl=$('wm-wifi-scan-title');
  var detailEl=$('wm-wifi-scan-detail');
  if(!overlay)return;
  if(titleEl)titleEl.textContent=title||'Scanning Wi-Fi';
  if(detailEl)detailEl.textContent=detail||'';
  overlay.style.display=visible?'flex':'none';
  overlay.setAttribute('aria-hidden',visible?'false':'true');
}

function startWifiScanPolling(box){
  stopWifiScanPolling();
  setScanRefreshEnabled(false);
  setWifiScanProgress(true,'Scanning Wi-Fi','Looking for nearby networks…');
  var token=++_wmWifiScanPollToken;
  var unavailableDeadline=Date.now()+20000;
  function retryAfterTemporaryRadioLoss(){
    if(Date.now()<unavailableDeadline){
      setWifiScanProgress(true,'Scanning Wi-Fi','Waiting for the device radio to return…');
      _wmWifiScanPollTimer=setTimeout(poll,1000);
      return true;
    }
    return false;
  }
  function poll(){
    api('/api/wifi/scan-status').then(function(res){
      if(token!==_wmWifiScanPollToken)return;
      if(!res.ok)throw new Error('scan status unavailable');
      var data={};
      try{data=JSON.parse(res.body);}catch(e){
        if(retryAfterTemporaryRadioLoss())return;
        throw e;
      }
      box=getScanResultsBox()||box;
      if(!data.scanning){
        if(box)box.innerHTML=renderScanList(data);
        setWifiScanProgress(false);
        stopWifiScanPolling();
        return;
      }
      _wmWifiScanPollTimer=setTimeout(poll,800);
    }).catch(function(){
      if(token!==_wmWifiScanPollToken)return;
      if(retryAfterTemporaryRadioLoss())return;
      stopWifiScanPolling();
      setWifiScanProgress(false);
      box=getScanResultsBox()||box;
      if(box)box.innerHTML=renderScanError('The portal did not return after scanning.');
    });
  }
  poll();
}

function renderScanList(data){
  if(!data||data.scanning){
    return "<div class='wm-scan-list'><p class='wm-lead' style='margin:0;padding:14px 12px'>Scanning for networks…</p></div>";
  }
  if(data.error==='timeout'){
    return "<div class='wm-scan-list'><p class='wm-lead' style='margin:0;padding:14px 12px'>Scan timed out. Try Scan again.</p></div>";
  }
  if(data.error==='failed'){
    return "<div class='wm-scan-list'><p class='wm-lead' style='margin:0;padding:14px 12px'>Scan failed.</p></div>";
  }
  if(!data.networks||data.count===0){
    return "<div class='wm-scan-list'><p class='wm-lead' style='margin:0;padding:14px 12px'>No networks found.</p></div>";
  }
  var html="<div class='wm-scan-list'>",i,n,pct,qi,enc;
  for(i=0;i<data.networks.length;i++){
    n=data.networks[i];
    pct=(n.quality||0)+'%';
    qi=Math.round((n.quality/100)*3)+1;if(qi<1)qi=1;if(qi>4)qi=4;
    html+="<a class='wm-scan-row' href='#p' data-ssid='"+esc(n.ssid)+"'>";
    html+="<span class='wm-scan-ssid'>"+esc(n.ssid)+"</span>";
    html+="<span class='wm-scan-meta'>";
    if(n.encrypted){html+="<span class='wm-scan-lock q l' aria-hidden='true'></span>";}
    html+="<span role='img' aria-label='"+pct+"' title='"+pct+"' class='wm-scan-bars q q-"+qi+"'></span>";
    html+="<span class='wm-scan-signal'>"+pct+"</span>";
    html+="</span>";
    html+="</a>";
  }
  html+="</div>";
  return html;
}

function renderScanError(msg){
  return "<div class='wm-scan-list'><p class='wm-lead' style='margin:0;padding:14px 12px'>"+esc(msg||'Failed to reach portal while scanning.')+"</p></div>";
}

function shouldAutoStartWifiScan(data){
  if(!data)return true;
  if(data.scanning)return false;
  if((data.lastscan||0)>0)return false;
  if(data.results_valid)return false;
  if(data.error==='timeout' || data.error==='failed')return false;
  return true;
}

function wifiRefresh(){
  var box=getScanResultsBox();
  if(!box)return;
  setScanRefreshEnabled(false);
  setWifiScanProgress(true,'Preparing Wi-Fi scan','Starting a scan…');
  api('/api/wifi/scan',{method:'POST'}).then(function(res){
    box=getScanResultsBox()||box;
    if(!res.ok){
      setScanRefreshEnabled(true);
      setWifiScanProgress(false);
      if(box)box.innerHTML=renderScanError('Failed to start Wi-Fi scan.');
      showToast('Failed to start Wi-Fi scan.',true);
      return;
    }
    startWifiScanPolling(box);
  }).catch(function(){
    stopWifiScanPolling();
    setWifiScanProgress(false);
    box=getScanResultsBox()||box;
    if(box)box.innerHTML=renderScanError('Failed to reach the portal while starting a scan.');
    showToast('Failed to reach the portal while starting a scan.',true);
  });
}

function bindWifiViewEvents(){
  var refreshBtn=$('wm-refresh-scan');
  if(refreshBtn){
    refreshBtn.addEventListener('click', wifiRefresh);
  }

  var showPass=$('wm-showpass');
  if(showPass){
    showPass.addEventListener('change', function(){
      var p=$('wm-p');
      if(p)p.type=this.checked?'text':'password';
    });
  }
  var profileShowPass=document.querySelectorAll('.wm-showpass');
  var profileIndex;
  for(profileIndex=0;profileIndex<profileShowPass.length;profileIndex++){
    profileShowPass[profileIndex].addEventListener('change',function(){
      var target=$(this.getAttribute('data-target')||'');
      if(target)target.type=this.checked?'text':'password';
    });
  }

  var scanResults=$('wm-scan-results');
  if(scanResults){
    scanResults.addEventListener('click', function(ev){
      var el=ev.target;
      while(el && el !== scanResults){
        if(el.tagName==='A' && el.getAttribute('data-ssid')){
          ev.preventDefault();
          var inp=$('wm-s0')||$('wm-s');
          var pass=$('wm-p0')||$('wm-p');
          if(inp)inp.value=el.getAttribute('data-ssid')||'';
          if(pass){
            if(pass.scrollIntoView)pass.scrollIntoView({behavior:'smooth',block:'center'});
            if(pass.focus)pass.focus();
          }else if(inp){
            if(inp.scrollIntoView)inp.scrollIntoView({behavior:'smooth',block:'center'});
            if(inp.focus)inp.focus();
          }
          return;
        }
        el=el.parentElement;
      }
    });
  }
}

function renderStationProfileFields(profiles){
  var html='';
  var i;
  for(i=0;i<profiles.length;i++){
    var p=profiles[i]||{};
    var primary=i===0;
    var title=primary?'Primary WiFi':'Fallback WiFi (optional)';
    var hint=primary
      ? 'This network is tried first for a new connection.'
      : 'Tried when the primary network is unavailable.';
    html+="<form class='wm-card wm-station-profile wm-wifi-data-form' id='wm-wifi-profile-"+i+"'><h2>"+esc(title)+"</h2><p class='wm-help'>"+esc(hint)+"</p>";
    html+="<label for='wm-s"+i+"'>SSID"+(primary?'':' (leave empty to disable)')+"</label>";
    html+="<input id='wm-s"+i+"' name='s"+i+"' type='text' autocomplete='username' maxlength='32' value='"+esc(p.ssid||'')+"'/>";
    html+="<label for='wm-p"+i+"'>Password</label>";
    html+="<input id='wm-p"+i+"' class='wm-profile-password' name='p"+i+"' type='password' autocomplete='current-password' maxlength='64' placeholder='"+(p.passwordSet?'Configured — enter a new password to replace it':'Leave blank for an open network')+"'/>";
    html+="<div class='wm-checkbox-row'><input type='checkbox' id='wm-showpass"+i+"' class='wm-showpass' data-target='wm-p"+i+"'/> <label for='wm-showpass"+i+"'>Show password</label></div>";
    if(p.passwordSet){
      html+="<div class='wm-checkbox-row'><input type='checkbox' id='wm-clear"+i+"' name='clear"+i+"' value='1'/> <label for='wm-clear"+i+"'>Clear password / use an open network</label></div>";
    }
    html+='</form>';
  }
  return html;
}

function viewWifi(){
  setView(navBar('wifi')+"<div class='wm-card'><p class='wm-lead'>Loading Wi-Fi options…</p></div>");
  api('/api/wifi/meta').then(function(res){
    var meta={};
    try{meta=JSON.parse(res.body);}catch(e){}
    var html=navBar('wifi');
    html+="<div class='wm-page-head'><h1>Connect to Wi-Fi</h1>";
    html+="<p class='wm-page-desc'>Choose a network, then save and connect.</p></div>";
    html+="<div class='wm-card wm-scan-card'>"+cardTitleHtml('scan','Networks nearby');
    html+="<div id='wm-scan-results' class='wm-scan-results'>";
    html+="<p class='wm-lead'>Preparing nearby networks…</p></div>";
    html+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block'";
    html+=" id='wm-refresh-scan'>"+labelWithIcon('scan','Scan again')+"</button>";
    html+="<div id='wm-wifi-scan-overlay' class='wm-operation-overlay'";
    html+=" role='status' aria-live='polite' aria-atomic='true' aria-hidden='true'>";
    html+="<div class='wm-operation-panel'><span class='wm-spinner' aria-hidden='true'></span>";
    html+="<div><strong id='wm-wifi-scan-title'>Scanning Wi-Fi</strong>";
    html+="<p id='wm-wifi-scan-detail'>Looking for nearby networks…</p></div></div></div></div>";
    html+="<div class='wm-card wm-wifi-details-card'>"+cardTitleHtml('wifi','Network details');
    if(Array.isArray(meta.profiles)&&meta.profiles.length){
      html+=renderStationProfileFields(meta.profiles);
      html+="<form class='wm-wifi-data-form' id='wm-wifi-form'>";
    }else{
      html+="<form class='wm-wifi-data-form' id='wm-wifi-form'>";
      html+=renderFieldList(meta.wifiFields||[]);
      var wifiFields=meta.wifiFields||[];
      var hasPassword=false;
      for(var i=0;i<wifiFields.length;i++){
        if(wifiFields[i]&&wifiFields[i].id==='p'){hasPassword=true;break;}
      }
      if(hasPassword){
        html+="<div class='wm-checkbox-row'><input type='checkbox' id='wm-showpass'/>";
        html+=" <label for='wm-showpass'>Show password</label></div>";
      }
    }
    html+=renderFieldList(meta.staticFields||[]);
    html+=renderFieldList(meta.params||[]);
    html+="</form>";
    html+="<div class='wm-form-actions'><button type='button'";
    html+=" class='wm-btn wm-btn--primary wm-btn--block' data-wm-wifi-action='connect'>";
    html+=labelWithIcon('connect','Save and connect')+"</button>";
    if(Array.isArray(meta.profiles)&&meta.profiles.length){
      html+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block'";
      html+=" data-wm-wifi-action='save'>Save for later</button>";
    }
    html+="</div><div id='wm-wifi-msg'></div></div>";
    html+="<div id='wm-wifi-save-overlay' class='wm-wifi-save-overlay'";
    html+=" role='status' aria-live='polite' aria-atomic='true' aria-hidden='true'>";
    html+="<div class='wm-wifi-save-panel'><span class='wm-spinner' aria-hidden='true'></span>";
    html+="<div><strong id='wm-wifi-save-title'>Saving settings</strong>";
    html+="<p id='wm-wifi-save-detail'>Please wait…</p></div></div></div>";
    setView(html);
    bindWifiViewEvents();
    setScanRefreshEnabled(true);
    api('/api/wifi/scan-status').then(function(scanRes){
      var scan={};
      try{scan=JSON.parse(scanRes.body);}catch(e){}
      var box=getScanResultsBox();
      if(shouldAutoStartWifiScan(scan)){
        wifiRefresh();
        return;
      }
      if(box)box.innerHTML=renderScanList(scan);
      if(scan&&scan.scanning)startWifiScanPolling(box);
    }).catch(function(){
      var box=getScanResultsBox();
      if(box)box.innerHTML=renderScanError('Failed to load nearby networks.');
      setScanRefreshEnabled(true);
    });
  }).catch(function(){
    setView(navBar('wifi')+"<div class='wm-card'><p class='wm-lead'>Failed to load Wi-Fi options. Reconnect to the portal and try again.</p></div>");
  });
}

// --- WiFi connect polling ---
var _wmWifiConnectPollTimer=null;
var _wmWifiConnectPollToken=0;
var _wmWifiConnectPending=false;
var _wmWifiSaveInFlight=false;

function setWifiSaveControlsDisabled(disabled){
  var controls=document.querySelectorAll('.wm-wifi-data-form input,.wm-wifi-data-form select,.wm-wifi-data-form textarea,[data-wm-wifi-action],#wm-refresh-scan');
  for(var i=0;i<controls.length;i++)controls[i].disabled=disabled;
}

function setWifiSaveProgress(visible,title,detail){
  var overlay=$('wm-wifi-save-overlay'),titleEl=$('wm-wifi-save-title'),detailEl=$('wm-wifi-save-detail');
  if(!overlay)return;
  if(titleEl)titleEl.textContent=title||'Saving settings';
  if(detailEl)detailEl.textContent=detail||'';
  overlay.style.display=visible?'flex':'none';
  overlay.setAttribute('aria-hidden',visible?'false':'true');
}

function beginWifiSave(title,detail){
  _wmWifiSaveInFlight=true;
  setWifiSaveControlsDisabled(true);
  setWifiSaveProgress(true,title,detail);
}

function finishWifiSave(){
  _wmWifiSaveInFlight=false;
  setWifiSaveControlsDisabled(false);
  setWifiSaveProgress(false);
}

function stopWifiConnectPolling(){
  _wmWifiConnectPollToken++;
  if(_wmWifiConnectPollTimer){
    clearTimeout(_wmWifiConnectPollTimer);
    _wmWifiConnectPollTimer=null;
  }
}

function completeWifiHandoff(data,attempt){
  attempt=attempt||0;
  api('/api/wifi/connect-complete',{method:'POST'}).then(function(res){
    if(!res.ok){
      if(attempt<2){setTimeout(function(){completeWifiHandoff(data,attempt+1);},500);return;}
      throw new Error('handoff acknowledgement rejected');
    }
    if(data.redirectUrl){
      setTimeout(function(){window.location.href=data.redirectUrl;},1200);
    }else{
      location.hash='#/';
    }
  }).catch(function(){
    if(attempt<2){
      setTimeout(function(){completeWifiHandoff(data,attempt+1);},500);
      return;
    }
    setWifiSaveProgress(true,'Wi-Fi connected','Open the device address shown below.');
    showToast('Wi-Fi connected, but automatic redirect was unavailable.',true);
  });
}

function pollWifiConnectStatus(){
  stopWifiConnectPolling();
  var token=++_wmWifiConnectPollToken;
  function poll(){
    api('/api/wifi/connect-status').then(function(res){
      if(token!==_wmWifiConnectPollToken)return;
      var data={};
      var msg=$('wm-wifi-msg');
      try{data=JSON.parse(res.body);}catch(e){}
      if(msg&&data.message)msg.innerHTML=esc(data.message);
      if(data.state==='success'){
        stopWifiConnectPolling();
        _wmWifiConnectPending=false;
        if(!data.stationIp){
          finishWifiSave();
          if(msg)msg.innerHTML=esc(data.message||'Settings saved.');
          showToast(data.message||'Settings saved.',false);
          return;
        }
        var destination=data.redirectUrl||'';
        setWifiSaveProgress(true,'Connected to Wi-Fi','Opening '+data.stationIp+'…');
        if(msg){
          msg.innerHTML="Connected. Opening <a href='"+esc(destination||('http://'+data.stationIp+'/'))+"'>";
          msg+=esc(data.stationIp)+"</a>…";
        }
        showToast(data.message||'Wi-Fi connected',false);
        completeWifiHandoff(data);
      }else if(data.state==='failed'){
        stopWifiConnectPolling();
        _wmWifiConnectPending=false;
        finishWifiSave();
        showToast(data.message||'Wi-Fi connection failed',true);
      }else{
        setWifiSaveProgress(true,'Connecting to Wi-Fi',data.message||'Waiting for the device to join the selected network…');
        _wmWifiConnectPollTimer=setTimeout(poll,700);
      }
    }).catch(function(){
      if(token!==_wmWifiConnectPollToken)return;
      stopWifiConnectPolling();
      finishWifiSave();
      var msg=$('wm-wifi-msg');
      if(msg)msg.innerHTML='Unable to check Wi-Fi connection status. Try again.';
      _wmWifiConnectPending=false;
      showToast('Unable to check Wi-Fi connection status.',true);
    });
  }
  poll();
}

function collectWifiFormData(){
  var fd=new FormData(), forms=document.querySelectorAll('.wm-wifi-data-form'), i;
  for(i=0;i<forms.length;i++){
    new FormData(forms[i]).forEach(function(value,key){fd.append(key,value);});
  }
  return fd;
}

function submitWifi(action){
  if(_wmWifiSaveInFlight)return false;
  var fd=collectWifiFormData();
  if(action)fd.append('stationAction',action);
  var msg=$('wm-wifi-msg');
  beginWifiSave(action==='save'?'Saving settings':'Saving and connecting',action==='save'?'Saving these settings for a later connection...':'Saving settings and preparing the WiFi connection...');
  if(msg)msg.innerHTML='Submitting...';
  api('/api/wifi/save',{method:'POST',body:fd}).then(function(res){
    var j={};
    try{j=JSON.parse(res.body);}catch(e){}
    if(msg){
      msg.innerHTML=(j&&j.message)?esc(j.message):(res.ok?'Queued.':'Error');
    }
    if(res.status===202){
      _wmWifiConnectPending=true;
      setWifiSaveProgress(true,'Connecting to WiFi',(j&&j.message)?j.message:'Waiting for the device to join the selected network...');
      pollWifiConnectStatus();
    }else if(res.ok){
      finishWifiSave();
      showToast((j&&j.message)?j.message:'Settings saved.',false);
    }else{
      finishWifiSave();
      showToast((j&&j.message)?j.message:'Unable to save settings.',true);
    }
  }).catch(function(){
    finishWifiSave();
    if(msg)msg.innerHTML='Unable to reach the portal while saving. Please try again.';
    showToast('Unable to reach the portal while saving.',true);
  });
  return false;
}

window.portalWifiSave=function(ev){
  ev.preventDefault();
  return submitWifi(ev.submitter&&ev.submitter.value?ev.submitter.value:'connect');
};

window.portalWifiAction=function(ev){
  ev.preventDefault();
  return submitWifi(ev.currentTarget.getAttribute('data-wm-wifi-action'));
};

// --- Info view ---
function viewInfo(){
  setView(navBar('info')+"<div class='wm-card'><p class='wm-lead'>Loading device info…</p></div>");
  api('/api/info').then(function(res){
    try{var d=JSON.parse(res.body);}catch(e){d={};}
    var html=navBar('info');
    html+="<div class='wm-page-head'><h1>Device</h1><p class='wm-page-desc'>Connection status, hardware, and actions.</p></div>";
    var st=d.status;
    if(st && typeof st==='object' && !Array.isArray(st)){
      html+="<div class='wm-card'>"+cardTitleHtml('status','Status');
      if(st.summary){html+="<p class='wm-callout wm-callout--info wm-info-summary'>"+esc(st.summary)+"</p>";}
      html+="<dl class='wm-kv'>";
      html+="<dt>Connected</dt><dd>"+esc(st.connected?'Yes':'No')+"</dd>";
      if(st.ssid!==undefined && st.ssid!=='')html+="<dt>SSID</dt><dd>"+esc(st.ssid)+"</dd>";
      if(st.stationIp)html+="<dt>Station IP</dt><dd>"+esc(st.stationIp)+"</dd>";
      if(st.apIp)html+="<dt>Portal IP</dt><dd>"+esc(st.apIp)+"</dd>";
      html+="</dl></div>";
    } else {
      html+=sectionCardHtml('status','Status','Connection summary and main addresses.',d.status||[]);
    }
    html+=sectionCardHtml('device','Device','Runtime, memory, and hardware details.',d.device||[]);
    html+=sectionCardHtml('diagnostics','WiFi details','Network diagnostics and interface metadata.',rowsWithoutKeys(d.wifi||[],['conx','stassid','staip','apip']));
    html+=sectionCardHtml('about','About','Library, SDK, and build information.',d.about||[]);
    if(d.extraSections){
      var xi,zs,zrows,zi,it;
      for(xi=0;xi<d.extraSections.length;xi++){
        zs=d.extraSections[xi];
        zrows=[];
        if(zs.items){for(zi=0;zi<zs.items.length;zi++){it=zs.items[zi];zrows.push({label:it.label,value:esc(it.value)});}}
        html+=sectionCardHtml('info',zs.title||'Details','',zrows);
      }
    }
    var act=d.actions||{};
    var actionsHtml='', hasActions=false;
    if(act.showRestart){actionsHtml+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-restart-btn'>"+labelWithIcon('restart','Restart device')+"</button>";hasActions=true;}
    if(act.showExitPortal){actionsHtml+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-exit-btn'>"+labelWithIcon('exit','Exit portal')+"</button>";hasActions=true;}
    if(act.showCloseCaptive){actionsHtml+="<button type='button' class='wm-btn wm-btn--secondary wm-btn--block' id='wm-close-captive-btn'>"+labelWithIcon('close','Stop captive portal')+"</button>";hasActions=true;}
    if(act.showUpdate){actionsHtml+="<a class='wm-btn wm-btn--secondary wm-btn--block' href='#/update'>"+labelWithIcon('update','Firmware update')+"</a>";hasActions=true;}
    if(act.showErase){actionsHtml+="<button type='button' class='wm-btn wm-btn--danger wm-btn--block' id='wm-erase-btn'>"+labelWithIcon('erase','Erase WiFi config')+"</button>";hasActions=true;}
    if(hasActions){
      html+="<div class='wm-card'>"+cardTitleHtml('actions','Actions')+"<p class='wm-card-subtitle'>Maintenance and recovery actions for this device.</p><div class='wm-btn-group'>"+actionsHtml+"</div></div>";
    }
    setView(html);
  });
}

function portalConfirmPost(url, promptMsg){
  showConfirm(promptMsg||'Continue?',function(){
    api(url,{method:'POST'}).then(function(res){
      try{var j=JSON.parse(res.body);}catch(e){j={};}
      var m=(j&&j.message)?j.message:(res.ok?'OK':'Error');
      showToast(m,!res.ok);
    });
  });
}

window.portalRestart=function(){portalConfirmPost('/api/device/restart','Restart the device?');};
window.portalExit=function(){portalConfirmPost('/api/portal/exit','Exit the configuration portal?');};
window.portalCloseCaptive=function(){portalConfirmPost('/api/portal/close','Stop redirecting clients to this portal (captive detection off)?');};

window.portalErase=function(){
  showConfirm('Erase WiFi configuration?',function(){
    api('/api/device/erase',{method:'POST'}).then(function(res){
      try{var j=JSON.parse(res.body);}catch(e){j={};}
      showToast((j&&j.message)?j.message:(res.ok?'Done':'Error'),!res.ok);
    });
  });
};

// --- Params / OTA ---
function viewSetup(){
  setView(navBar('setup')+"<div class='wm-card'><p class='wm-lead'>Loading parameters…</p></div>");
  api('/api/params').then(function(res){
    try{var d=JSON.parse(res.body);}catch(e){d={};}
    var html=navBar('setup');
    html+="<div class='wm-page-head'><h1>Settings</h1><p class='wm-page-desc'>Custom parameters stored on this device.</p></div>";
    html+="<div class='wm-card'><form id='wm-param-form'>";
    html+=renderFieldList(d.params||[]);
    html+="<div class='wm-form-actions'><button type='submit' class='wm-btn wm-btn--primary wm-btn--block'>"+labelWithIcon('save','Save parameters')+"</button></div>";
    html+="</form><div id='wm-param-msg'></div></div>";
    setView(html);
  });
}

window.portalParamSave=function(ev){
  ev.preventDefault();
  var fd=new FormData(document.getElementById('wm-param-form'));
  var msg=$('wm-param-msg');
  if(msg)msg.innerHTML='Saving...';
  api('/api/params/save',{method:'POST',body:fd}).then(function(res){
    try{var j=JSON.parse(res.body);}catch(e){j={};}
    if(msg)msg.innerHTML=(j&&j.message)?esc(j.message):'Saved.';
  });
  return false;
};

function setOtaControlsDisabled(disabled){
  var controls=document.querySelectorAll('#wm-ota-form input,#wm-ota-form button');
  for(var i=0;i<controls.length;i++)controls[i].disabled=disabled;
}

function setOtaProgress(visible,title,detail,percent){
  var overlay=$('wm-ota-overlay');
  var titleEl=$('wm-ota-title');
  var detailEl=$('wm-ota-detail');
  var progress=$('wm-ota-progress');
  var bar=$('wm-ota-progress-bar');
  var label=$('wm-ota-progress-label');
  if(!overlay)return;
  if(titleEl)titleEl.textContent=title||'Uploading firmware';
  if(detailEl)detailEl.textContent=detail||'';
  var hasPercent=typeof percent==='number';
  if(progress)progress.style.display=hasPercent?'block':'none';
  if(hasPercent){
    var value=Math.max(0,Math.min(100,Math.round(percent)));
    if(bar)bar.style.width=value+'%';
    if(label)label.textContent=value+'%';
  }
  overlay.style.display=visible?'flex':'none';
  overlay.setAttribute('aria-hidden',visible?'false':'true');
}

function viewUpdate(){
  var pages=boot.pages||{};
  var update=pages.update||{};
  var html=navBar('update');
  html+="<div class='wm-page-head'><h1>Update firmware</h1>";
  html+="<p class='wm-page-desc'>Upload a firmware build for this device.</p></div>";
  if(!update.visible){
    html+="<div class='wm-card'><p class='wm-lead'>Firmware update is disabled for this device.</p></div>";
    setView(html);
    return;
  }
  html+="<div class='wm-card wm-ota-card'><form id='wm-ota-form'>";
  html+="<div class='wm-field'><label for='wm-ota-file'>Firmware file</label>";
  html+="<input type='file' id='wm-ota-file' name='update' accept='.bin,.bin.gz'/></div>";
  html+="<div class='wm-form-actions'><button type='submit'";
  html+=" class='wm-btn wm-btn--primary wm-btn--block'>";
  html+=labelWithIcon('upload','Upload firmware')+"</button></div></form>";
  html+="<p class='wm-lead'>Use the binary built for this device. It will restart after a successful update.</p>";
  html+="<div id='wm-ota-overlay' class='wm-operation-overlay'";
  html+=" role='status' aria-live='polite' aria-atomic='true' aria-hidden='true'>";
  html+="<div class='wm-operation-panel'><span class='wm-spinner' aria-hidden='true'></span><div>";
  html+="<strong id='wm-ota-title'>Uploading firmware</strong>";
  html+="<p id='wm-ota-detail'>Preparing upload…</p>";
  html+="<div id='wm-ota-progress' class='wm-operation-progress' style='display:none'>";
  html+="<div class='wm-operation-progress-track'><span id='wm-ota-progress-bar'></span></div>";
  html+="<span id='wm-ota-progress-label'>0%</span></div></div></div></div></div>";
  setView(html);
}

window.portalOtaSubmit=function(ev){
  ev.preventDefault();
  var fileInput=$('wm-ota-file');
  if(!fileInput||!fileInput.files||!fileInput.files[0]){
    showToast('Choose a firmware file first.',true);
    return false;
  }
  setOtaControlsDisabled(true);
  setOtaProgress(true,'Uploading firmware','Preparing upload…');
  var xhr=new XMLHttpRequest();
  xhr.upload.onprogress=function(event){
    if(!event.lengthComputable)return;
    var percent=100*event.loaded/event.total;
    setOtaProgress(true,'Uploading firmware','Transferring firmware…',percent);
  };
  xhr.onreadystatechange=function(){
    if(xhr.readyState!==4)return;
    var text=xhr.responseText||'';
    var data={};
    try{data=JSON.parse(text);}catch(e){}
    var ok=xhr.status>=200&&xhr.status<300&&data.ok!==false;
    if(ok){
      setOtaProgress(true,'Firmware updated','Restarting device…',100);
      showToast(data.message||'Firmware updated. Restarting device…',false);
      return;
    }
    setOtaProgress(false);
    setOtaControlsDisabled(false);
    showToast(data.message||('Upload failed (HTTP '+xhr.status+').'),true);
  };
  xhr.onerror=function(){
    setOtaProgress(false);
    setOtaControlsDisabled(false);
    showToast('Upload failed before the device responded.',true);
  };
  xhr.open('POST','/u');
  var formData=new FormData();
  formData.append('update',fileInput.files[0]);
  xhr.send(formData);
  return false;
};

// --- Routing ---
function route(){
  var h=location.hash||'#/';
  if(h.indexOf('#/')!==0)h='#/';
  var pages=boot.pages||{};
  var ps=pages.setup||{};
  var pi=pages.info||{};
  var pu=pages.update||{};
  if(h==='#/setup'&&ps.visible===false){viewHome();try{document.dispatchEvent(new CustomEvent('wm:view-changed',{detail:{route:'#/'}}));}catch(e){}return;}
  if(h==='#/info'&&pi.visible===false){viewHome();try{document.dispatchEvent(new CustomEvent('wm:view-changed',{detail:{route:'#/'}}));}catch(e){}return;}
  if(h==='#/update'&&pu.visible===false){viewHome();try{document.dispatchEvent(new CustomEvent('wm:view-changed',{detail:{route:'#/'}}));}catch(e){}return;}
  if(h==='#/'||h==='#'){viewHome();}
  else if(h==='#/wifi'){viewWifi();}
  else if(h==='#/info'){viewInfo();}
  else if(h==='#/setup'){viewSetup();}
  else if(h==='#/update'){viewUpdate();}
  else {viewHome();h='#/';}
  try{document.dispatchEvent(new CustomEvent('wm:view-changed',{detail:{route:h}}));}catch(e){}
}

function bootPortal(){
  applyPortalFavicon();
  route();
  try{document.dispatchEvent(new CustomEvent('wm:ready',{detail:{boot:boot}}));}catch(e){}
}

window.addEventListener('hashchange',route);
window.addEventListener('load',bootPortal);
})();

)rawliteral";

#endif  // _WM_PORTAL_APP_JS_H_
