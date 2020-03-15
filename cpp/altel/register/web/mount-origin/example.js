
let alive = 0;
let wsa = [];

function dummyEventTimer(){
    let ev_array = getEventArray();
    //UpdateData
    let n_ev =  ev_array.length;
    for(let i=0; i< n_ev; i++ ){
        let ev = ev_array[i];
        let hit_array = ev_array[i].hit_xyz_array;
        let n_hit = hit_array.length;
        for(let j=0; j< n_hit; j++ ){
            let pixelX = hit_array[j][0];
            let pixelY = hit_array[j][1];
            let pixelZ = hit_array[j][2];
            cellX=Math.floor(pixelX/scalerFactorX);
            cellY=Math.floor(pixelY/scalerFactorY);
            cellN= cellX + cellNumberX * cellY;
            data[cellN].hit_count += 1;
            data[cellN].flushed = false;
        }
    }

       
    setTimeout(dummyEventTimer, 2000);
}


function get_appropriate_ws_url(extra_url){
    let pcol;
    let u = document.URL;
    if (u.substring(0, 5) === "https") {
	pcol = "wss://";
	u = u.substr(8);
    }
    else {
	pcol = "ws://";
	if (u.substring(0, 4) === "http")
	    u = u.substr(7);
    }
    u = u.split("/");
    return pcol + u[0] + "/" + extra_url;
}

function new_ws(urlpath, protocol){
    if (typeof MozWebSocket != "undefined")
	return new MozWebSocket(urlpath, protocol);
    return new WebSocket(urlpath, protocol);
}

function on_ws_open() {
    document.getElementById("r").disabled = 0;
    alive++;
};

function on_ws_close(){
    alive --;
    if (alive === 0)
	document.getElementById("r").disabled = 1;
};

function DOMContentLoadedListener() {
    ws = new_ws(get_appropriate_ws_url(""), "lws-minimal");
    wsa.push(ws);
    ws.onopen = on_ws_open;
    ws.onclose = on_ws_close;
    ws.binaryType = 'arraybuffer';
    let head = 0;
    let tail = 0;
    let ring = [];
    ws.onmessage = function got_packet(msg) {
        if(writer){
            let view = new Uint8Array(msg.data);
            writer.write(view);
        }        
    };
    
    function sendmsg(){
	ws.send(document.getElementById("m").value);
	document.getElementById("m").value = "";
    }
    
    document.getElementById("b").addEventListener("click", sendmsg);   

    // dummyEventTimer();
    
    document.getElementById("btn_download_start").addEventListener("click", startDownload);
    document.getElementById("btn_download_stop").addEventListener("click", stopDownload);
    //TODO: start and stop download
}

let fileStream;
let writer;

function startDownload(){
    streamSaver.mitm = location.href.substr(0, location.href.lastIndexOf('/')) +'/mitm.html'
    fileStream = streamSaver.createWriteStream('sample_yi.txt')
    writer = fileStream.getWriter()
    let a = new Uint8Array(1).fill(10)  // 10 = asicii /n
    writer.write(a);
}

function stopDownload(){
    writer.close();
    writer = null;
}

document.addEventListener("DOMContentLoaded", DOMContentLoadedListener, false);
